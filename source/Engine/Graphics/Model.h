#pragma once
#include <Engine/Graphics/Mesh.h>
#include <Engine/Graphics/Material.h>
#include <Engine/Core/Math/Mat4.h>
#include <assimp/scene.h>

namespace frostwave
{
	class Model
	{
	public:
		Model();
		Model(const std::string& path);
		~Model();

		void Load(const std::string& path);

		void AddMesh(Mesh* mesh);
		void SetPosition(const Vec3f& position);
		const Vec3f& GetPosition() const;
		void SetRotation(const Quatf& rotation);
		const Quatf& GetRotation() const;
		void SetScale(const Vec3f& scale);
		void SetTransform(const Mat4f& transform);

		Shader* GetShader();

		const std::vector<Mesh*>& GetMeshes() const;

		const Mat4f& GetTransform();
		Material& GetMaterial() { return m_Material; }

		static Model* GetSphere(f32 radius, i32 sliceCount, i32 stackCount, const Vec4f& color = Vec4f(1, 1, 1, 1));
		static Model* GetCube();

	private:
		Texture* LoadMaterialTexture(aiMaterial* material, aiTextureType type);
		void ProcessNode(aiNode* node, const aiScene* scene);
		Mesh* ProcessMesh(aiMesh* mesh, const aiScene* scene);

		Material m_Material;
		Shader m_Shader;

		std::vector<Mesh*> m_Meshes;
		std::string m_Path, m_Name;
		Mat4f m_Transform;
		Vec3f m_Position, m_Scale;
		Quatf m_Rotation;
		bool m_Dirty;
	};
}
namespace fw = frostwave;