#include "Model.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <Engine/Logging/Logger.h>
#include <Engine/Graphics/Framework.h>
#include <Engine/Core/Common.h>
#include <Engine/Memory/Allocator.h>
#include <filesystem>

frostwave::Model::Model() : m_Scale(1, 1, 1)
{
}

frostwave::Model::Model(const std::string& path) : m_Scale(1, 1, 1)
{
	Load(path);
}

frostwave::Model::~Model()
{
	for (auto* mesh : m_Meshes)
	{
		for (auto* tex : mesh->textures)
		{
			if(tex)
				Free(tex);
		}
		Free(mesh);
	}
}

void frostwave::Model::Load(const std::string& path)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_GenNormals | aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		ERROR_LOG("%s", importer.GetErrorString());
		return;
	}
	m_Path = path.substr(0, path.find_last_of('/') + 1);
	m_Name = path.substr(path.find_last_of("/") + 1);

	ProcessNode(scene->mRootNode, scene);

	m_Shader.Load(fw::Shader::Type::Vertex | fw::Shader::Type::Pixel, "assets/shaders/model_ps.fx", "assets/shaders/general_vs.fx");
}

void frostwave::Model::AddMesh(Mesh* mesh)
{
	m_Meshes.push_back(mesh);
}

void frostwave::Model::SetPosition(const Vec3f& position)
{
	m_Dirty = true;
	m_Position = position;
}

const fw::Vec3f& frostwave::Model::GetPosition() const
{
	return m_Position;
}

void frostwave::Model::SetRotation(const Quatf& rotation)
{
	m_Dirty = true;
	m_Rotation = rotation;
}

const fw::Quatf& frostwave::Model::GetRotation() const
{
	return m_Rotation;
}

void frostwave::Model::SetScale(const Vec3f& scale)
{
	m_Dirty = true;
	m_Scale = scale;
}

void frostwave::Model::SetTransform(const Mat4f& transform)
{
	m_Transform = transform;
	m_Dirty = false;
}

const std::vector<frostwave::Mesh*>& frostwave::Model::GetMeshes() const
{
	return m_Meshes;
}

const frostwave::Mat4f& frostwave::Model::GetTransform()
{
	if (m_Dirty)
	{
		SetTransform(Mat4f::CreateTransform(m_Position, m_Rotation, m_Scale));
	}

	return m_Transform;
}

frostwave::Model* frostwave::Model::GetSphere(f32 radius, i32 sliceCount, i32 stackCount, const Vec4f& color)
{
	std::vector<Vertex> vertices;
	std::vector<u32> indices;
	Vertex first;
	first.position = { 0, radius, 0, 1 };
	first.normal = { 0, 1, 0 };
	first.tangent = { 1, 0, 0 };
	first.bitangent = first.tangent.Cross(first.normal);
	first.uv = { 0, 0 };
	first.color = color;
	vertices.push_back(first);

	f32 phiStep = fw::PI / stackCount;
	f32 thetaStep = 2.0f * fw::PI / sliceCount;

	for (i32 i = 1; i <= stackCount - 1; i++) {
		f32 phi = i * phiStep;
		for (i32 j = 0; j <= sliceCount; j++) {
			f32 theta = j * thetaStep;
			Vec3f p = Vec3f(
				(radius * sinf(phi) * cosf(theta)),
				(radius * cosf(phi)),
				(radius * sinf(phi) * sinf(theta))
			);

			Vec3f t = Vec3f(-radius * sinf(phi) * sinf(theta), 0, radius * sinf(phi) * cosf(theta));
			t.Normalize();
			Vec3f n = p;
			n.Normalize();
			Vec2f uv = Vec2f(theta / (fw::PI * 2), phi / fw::PI);
			Vertex v;
			v.position = p;
			v.normal = n;
			v.tangent = t;
			v.bitangent = v.tangent.Cross(v.normal);
			v.uv = uv;
			v.color = color;
			vertices.push_back(v);
		}
	}

	Vertex last;
	last.position = { 0, -radius, 0, 1 };
	last.normal = { 0, -1, 0 };
	last.tangent = { -1, 0, 0 };
	last.bitangent = last.tangent.Cross(last.normal);
	last.uv = { 0, 1 };
	last.color = color;
	vertices.push_back(last);

	for (int i = 1; i <= sliceCount; i++) {
		indices.push_back(0);
		indices.push_back(i + 1);
		indices.push_back(i);
	}

	i32 baseIndex = 1;
	i32 ringVertexCount = sliceCount + 1;
	for (int i = 0; i < stackCount - 2; i++) {
		for (int j = 0; j < sliceCount; j++) {
			indices.push_back(baseIndex + i * ringVertexCount + j);
			indices.push_back(baseIndex + i * ringVertexCount + j + 1);
			indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);

			indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
			indices.push_back(baseIndex + i * ringVertexCount + j + 1);
			indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
		}
	}
	u32 southPoleIndex = (u32)vertices.size() - 1;
	baseIndex = southPoleIndex - ringVertexCount;
	for (int i = 0; i < sliceCount; i++) {
		indices.push_back(southPoleIndex);
		indices.push_back(baseIndex + i);
		indices.push_back(baseIndex + i + 1);
	}

	Mesh* m = Allocate<Mesh>(vertices, indices, std::array<Texture*, MeshTextures::Count>{nullptr});

	Model* model = Allocate();
	model->m_Shader.Load(Shader::Type::Vertex | Shader::Type::Pixel, "assets/shaders/model_ps.fx", "assets/shaders/model_vs.fx");
	model->AddMesh(m);
	return model;
}

frostwave::Texture* frostwave::Model::LoadMaterialTexture(aiMaterial* material, aiTextureType type)
{
	Texture* texture = Allocate();
	aiString str;
	material->GetTexture(type, 0, &str);
	if (str.length > 0 && texture->Load(m_Path + str.C_Str()))
	{
	}
	else //Manually try to find matching texture file
	{
		switch (type)
		{
		case aiTextureType_DIFFUSE: //Albedo
			if (std::filesystem::exists(std::filesystem::path(m_Path + m_Name.substr(0, m_Name.find_last_of('.')) + "_A.dds")))
				texture->Load(m_Path + m_Name.substr(0, m_Name.find_last_of('.')) + "_A.dds");
			else
				texture->Load("assets/textures/default/default_A.dds");
			break;
		case aiTextureType_SHININESS: //Roughness
			if (std::filesystem::exists(std::filesystem::path(m_Path + m_Name.substr(0, m_Name.find_last_of('.')) + "_R.dds")))
				texture->Load(m_Path + m_Name.substr(0, m_Name.find_last_of('.')) + "_R.dds");
			else
				texture->Load("assets/textures/default/default_R.dds");
			break;
		case aiTextureType_UNKNOWN: //AmbientOcclusion
			if (std::filesystem::exists(std::filesystem::path(m_Path + m_Name.substr(0, m_Name.find_last_of('.')) + "_AO.dds")))
				texture->Load(m_Path + m_Name.substr(0, m_Name.find_last_of('.')) + "_AO.dds");
			else
				texture->Load("assets/textures/default/default_AO.dds");
			break;
		case aiTextureType_EMISSIVE: //Emissive
			if (std::filesystem::exists(std::filesystem::path(m_Path + m_Name.substr(0, m_Name.find_last_of('.')) + "_E.dds")))
				texture->Load(m_Path + m_Name.substr(0, m_Name.find_last_of('.')) + "_E.dds");
			else
				texture->Load("assets/textures/default/default_E.dds");
			break;
		case aiTextureType_HEIGHT:
		case aiTextureType_NORMALS: //Normal
			if (std::filesystem::exists(std::filesystem::path(m_Path + m_Name.substr(0, m_Name.find_last_of('.')) + "_N.dds")))
				texture->Load(m_Path + m_Name.substr(0, m_Name.find_last_of('.')) + "_N.dds");
			else
				texture->Load("assets/textures/default/default_N.dds");
			break;
		case aiTextureType_AMBIENT: //Metalness
			if (std::filesystem::exists(std::filesystem::path(m_Path + m_Name.substr(0, m_Name.find_last_of('.')) + "_M.dds")))
				texture->Load(m_Path + m_Name.substr(0, m_Name.find_last_of('.')) + "_M.dds");
			else
				texture->Load("assets/textures/default/default_M.dds");
			break;
		}

		if (!texture->Valid())
		{
			Free(texture);
			texture = nullptr;
		}
	}
	return texture;
}

void frostwave::Model::ProcessNode(aiNode* node, const aiScene* scene)
{
	for (u32 i = 0; i < node->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		m_Meshes.push_back(ProcessMesh(mesh, scene));
	}

	for (u32 i = 0; i < node->mNumChildren; ++i)
	{
		ProcessNode(node->mChildren[i], scene);
	}
}

frostwave::Mesh* frostwave::Model::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
	std::vector<Vertex> vertices;
	std::vector<u32> indices;
	std::array<Texture*, (i32)MeshTextures::Count> textures = { };

	for (u32 i = 0; i < mesh->mNumVertices; ++i)
	{
		Vertex vertex;

		vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1 };
		vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };

		if (mesh->HasTangentsAndBitangents())
		{
			vertex.tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
			vertex.bitangent = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
		}

		if (mesh->mTextureCoords[0])
			vertex.uv = Vec2f(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
		else
			vertex.uv = Vec2f(0, 0);

		if (mesh->mColors[0])
			vertex.color = Vec4f(mesh->mColors[0][i].r, mesh->mColors[0][i].g, mesh->mColors[0][i].b, mesh->mColors[0][i].a);
		else
			vertex.color = Vec4f(1, 1, 1, 1);

		vertices.push_back(vertex);
	}

	for (u32 i = 0; i < mesh->mNumFaces; ++i)
	{
		aiFace face = mesh->mFaces[i];
		for (u32 j = 0; j < face.mNumIndices; ++j)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		textures[MeshTextures::Albedo]				= LoadMaterialTexture(material, aiTextureType_DIFFUSE);		// TEXTURE_DEFINITION_ALBEDO
		textures[MeshTextures::Normal]				= LoadMaterialTexture(material, aiTextureType_HEIGHT);		// TEXTURE_DEFINITION_NORMAL
		textures[MeshTextures::Metalness]			= LoadMaterialTexture(material, aiTextureType_AMBIENT);
		// TEXTURE_DEFINITION_METALNESS
		textures[MeshTextures::Roughness]			= LoadMaterialTexture(material, aiTextureType_SHININESS);	// TEXTURE_DEFINITION_ROUGHNESS
		textures[MeshTextures::AmbientOcclusion]	= LoadMaterialTexture(material, aiTextureType_UNKNOWN);		// TEXTURE_DEFINITION_AMBIENTOCCLUSION
		textures[MeshTextures::Emissive]			= LoadMaterialTexture(material, aiTextureType_EMISSIVE);	// TEXTURE_DEFINITION_EMISSIVE
	}

	return Allocate<Mesh>(vertices, indices, textures);
}

frostwave::Shader* frostwave::Model::GetShader()
{
	return &m_Shader;
}
