#include "DeferredRenderer.h"
#include <Engine/Graphics/Framework.h>
#include <Engine/Core/Math/Vec.h>
#include <d3d11.h>

frostwave::DeferredRenderer::DeferredRenderer() : m_EnvironmentLight(nullptr), m_DirectionalLight(nullptr)
{
}

frostwave::DeferredRenderer::~DeferredRenderer()
{
	Free(m_LightSphere);
}

void frostwave::DeferredRenderer::Init()
{
	m_GeometryFrameBuffer.Init(sizeof(GeometryFrameBuffer), BufferUsage::Dynamic, BufferType::Constant, 0, &m_GeometryFrameBufferData);
	m_ObjectBuffer.Init(sizeof(ObjectBuffer), BufferUsage::Dynamic, BufferType::Constant, 0, &m_ObjectBufferData);
	m_LightingBuffer.Init(sizeof(LightingBuffer), BufferUsage::Dynamic, BufferType::Constant, 0, &m_LightingBufferData);
	m_RenderGeometryShader.Load(Shader::Type::Vertex | Shader::Type::Pixel, "assets/shaders/deferred_ps.fx", "assets/shaders/general_vs.fx");
	m_PointLightShader.Load(Shader::Type::Vertex | Shader::Type::Pixel, "assets/shaders/deferred_pointlight_ps.fx", "assets/shaders/general_vs.fx");
	m_EnvLightShader.Load(Shader::Type::Vertex | Shader::Type::Pixel, "assets/shaders/deferred_envlight_ps.fx", "assets/shaders/deferred_lighting_vs.fx");

	m_LightSphere = Model::GetSphere(1.0f, 10, 10);
}

void frostwave::DeferredRenderer::RenderGeometry(f32, Camera* camera)
{
	m_GeometryFrameBufferData.view = camera->GetView();
	m_GeometryFrameBufferData.projection = camera->GetProjection();
	m_GeometryFrameBufferData.invProjection = Mat4f::Inverse(camera->GetProjection());
	m_GeometryFrameBufferData.invView = Mat4f::Inverse(camera->GetView());
	m_GeometryFrameBufferData.cameraPos = Vec4f(camera->GetPosition(), 1.0f);
	m_GeometryFrameBufferData.nearZ = camera->GetNearPlane();
	m_GeometryFrameBufferData.farZ = camera->GetFarPlane();
	m_GeometryFrameBufferData.resolution = Window::Get()->GetBoundsf();
	m_GeometryFrameBuffer.SetData(m_GeometryFrameBufferData);
	m_GeometryFrameBuffer.Bind(0);

	m_RenderGeometryShader.Bind();

	std::sort(m_Models.begin(), m_Models.end(), [&](Model* a, Model* b) {
		return (a->GetPosition() - camera->GetPosition()).LengthSqr() > (b->GetPosition() - camera->GetPosition()).LengthSqr();
	});

	for (auto* model : m_Models)
	{
		m_ObjectBufferData.model = model->GetTransform();
		m_ObjectBufferData.material = model->GetMaterial();
		m_ObjectBuffer.SetData(m_ObjectBufferData);
		m_ObjectBuffer.Bind(1);

		for (auto* mesh : model->GetMeshes())
		{
			auto* context = Framework::GetContext();

			if (mesh->textures[MeshTextures::Albedo])
				mesh->textures[MeshTextures::Albedo]->Bind(0);
			if (mesh->textures[MeshTextures::Normal])
				mesh->textures[MeshTextures::Normal]->Bind(1);
			if (mesh->textures[MeshTextures::Metalness])
				mesh->textures[MeshTextures::Metalness]->Bind(2);
			if (mesh->textures[MeshTextures::Roughness])
				mesh->textures[MeshTextures::Roughness]->Bind(3);
			if (mesh->textures[MeshTextures::AmbientOcclusion])
				mesh->textures[MeshTextures::AmbientOcclusion]->Bind(4);
			if (mesh->textures[MeshTextures::Emissive])
				mesh->textures[MeshTextures::Emissive]->Bind(5);

			//Bind empty texture to all slots if there is no texture
			if (mesh->textures.size() == 0)
				for (size_t i = 0; i < 5; i++)
					m_NullTexture.Bind((u32)i);

			mesh->vertexBuffer.Bind();
			mesh->indexBuffer.Bind();

			context->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)mesh->topology);
			context->DrawIndexed(mesh->indexCount, 0, 0);
		}
	}
	m_Models.clear();
}

void frostwave::DeferredRenderer::RenderLighting(f32 totalTime, RenderStateManager* stateManager)
{
	totalTime;
	auto* context = Framework::GetContext();

	stateManager->SetRasterizerState(RenderStateManager::RasterizerStates::BackCull);

	if (m_EnvironmentLight && m_EnvironmentLight->GetCubemap())
		m_EnvironmentLight->GetCubemap()->Bind(0);

	if (m_DirectionalLight && m_DirectionalLight->GetShadowData().shadowMap)
	{
		m_DirectionalLight->GetShadowData().shadowMap->Bind(8);
		m_LightingBufferData.dirLight.direction = Vec4f(m_DirectionalLight->GetDirection().GetNormalized(), 0.0);
		m_LightingBufferData.dirLight.color = Vec4f(m_DirectionalLight->GetColor(), m_DirectionalLight->GetIntensity());
		m_LightingBufferData.lightMatrix = m_DirectionalLight->GetShadowData().ViewProj;
	}

	m_EnvLightShader.Bind();
	m_LightingBuffer.SetData(m_LightingBufferData);
	m_LightingBuffer.Bind(2);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(nullptr);
	context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
	context->Draw(3, 0);

	m_PointLightShader.Bind();

	stateManager->SetRasterizerState(RenderStateManager::RasterizerStates::FrontFace);
	for (auto& light : m_PointLights)
	{
		m_LightingBufferData.pointLight.position = Vec4f(light->GetPosition(), light->GetRadius());
		m_LightingBufferData.pointLight.color = Vec4f(light->GetColor(), light->GetIntensity());
		m_LightingBuffer.SetData(m_LightingBufferData);
		m_LightingBuffer.Bind(2);

		m_LightSphere->SetPosition(light->GetPosition());
		m_LightSphere->SetScale(light->GetRadius());

		for (auto* mesh : m_LightSphere->GetMeshes())
		{
			m_ObjectBufferData.model = m_LightSphere->GetTransform();
			m_ObjectBuffer.SetData(m_ObjectBufferData);
			m_ObjectBuffer.Bind(1);

			mesh->vertexBuffer.Bind();
			mesh->indexBuffer.Bind();

			context->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)mesh->topology);
			context->DrawIndexed(mesh->indexCount, 0, 0);
		}
	}

	m_PointLights.clear();
}

void frostwave::DeferredRenderer::Submit(Model* model)
{
	m_Models.push_back(model);
}

void frostwave::DeferredRenderer::Submit(DirectionalLight* light)
{
	m_DirectionalLight = light;
}

void frostwave::DeferredRenderer::Submit(PointLight* light)
{
	m_PointLights.push_back(light);
}

void frostwave::DeferredRenderer::Submit(EnvironmentLight* light)
{
	m_EnvironmentLight = light;
}
