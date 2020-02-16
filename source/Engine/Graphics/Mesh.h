#pragma once
#include <Engine/Core/Math/Vec.h>
#include <Engine/Core/Types.h>
#include <Engine/Graphics/Buffer.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/Shader.h>
#include <vector>
#include <array>

namespace frostwave
{
	namespace MeshTextures
	{
		enum
		{
			Albedo,
			Normal,
			Metalness,
			Roughness,
			AmbientOcclusion,
			Emissive,
			Count
		};
	}

	struct Vertex
	{
		Vec4f position;
		Vec4f normal;
		Vec4f tangent;
		Vec4f bitangent;
		Vec4f color;
		Vec2f uv;
	};

	struct Mesh
	{
		Mesh(std::vector<Vertex> vertices, std::vector<u32> indices, std::array<Texture*, (i32)MeshTextures::Count> inTextures) :
			vertexCount((u32)vertices.size()),
			indexCount((u32)indices.size()),
			topology(4), //D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
			shader(),
			vertexBuffer((u32)vertices.size() * sizeof(Vertex), BufferUsage::Immutable, BufferType::Vertex, sizeof(Vertex), vertices.data()),
			indexBuffer((u32)indices.size() * sizeof(u32), BufferUsage::Immutable, BufferType::Index, sizeof(u32), indices.data()),
			textures(inTextures) { }

		std::array<Texture*, MeshTextures::Count> textures;
		Buffer vertexBuffer, indexBuffer;
		Shader shader;
		u32 vertexCount, indexCount, topology;
	};
}