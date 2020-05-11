#include "DeferredRenderer.h"
#include <Engine/Graphics/Framework.h>
#include <Engine/Core/Math/Vec.h>
#include <d3d11.h>

frostwave::DeferredRenderer::DeferredRenderer()
{
}

frostwave::DeferredRenderer::~DeferredRenderer()
{
	Free(m_IrradianceTexture);
	Free(m_PrefilteredTexture);
	Free(m_BRDFTexture);
	Free(m_LightSphere);
}

void frostwave::DeferredRenderer::Init()
{
	m_GeometryFrameBuffer.Init(sizeof(GeometryFrameBuffer), BufferUsage::Dynamic, BufferType::Constant, 0, &m_GeometryFrameBufferData);
	m_ObjectBuffer.Init(sizeof(ObjectBuffer), BufferUsage::Dynamic, BufferType::Constant, 0, &m_ObjectBufferData);
	m_LightingBuffer.Init(sizeof(LightingBuffer), BufferUsage::Dynamic, BufferType::Constant, 0, &m_LightingBufferData);
	m_RenderGeometryShader.Load(Shader::Type::Vertex | Shader::Type::Pixel, "../source/Engine/Shaders/deferred_ps.fx", "../source/Engine/Shaders/general_vs.fx");
	m_PointLightShader.Load(Shader::Type::Vertex | Shader::Type::Pixel, "../source/Engine/Shaders/deferred_pointlight_ps.fx", "../source/Engine/Shaders/general_vs.fx");
	m_AmbientLightShader.Load(Shader::Type::Vertex | Shader::Type::Pixel, "../source/Engine/Shaders/deferred_ambientlight_ps.fx", "../source/Engine/Shaders/fullscreen_vs.fx");
	m_DirectionalLightShader.Load(Shader::Type::Vertex | Shader::Type::Pixel, "../source/Engine/Shaders/deferred_directionallight_ps.fx", "../source/Engine/Shaders/fullscreen_vs.fx");

	m_LightSphere = Model::GetSphere(1.0f, 10, 10);
}

frostwave::Texture* frostwave::DeferredRenderer::GenerateCubemap(Texture* hdriTexture)
{
	Framework::BeginEvent("Generate HDR Cubemap");
	Texture* cubemapTexture = Allocate();
	TextureCreateInfo texInfo = {
		.size = { 4096, 4096 },
		.format = ImageFormat::DXGI_FORMAT_R16G16B16A16_FLOAT,
		.data = nullptr,
		.renderTarget = true,
		.hdr = true,
		.cubemap = true,
		.numMips = 0
	};
	cubemapTexture->Create(texInfo);
	cubemapTexture->Clear({ 0, 0, 0, 0 });

	Mat4f proj = Mat4f::CreatePerspectiveProjection(90.0f, 1, 0.1f, 10.0f);

	Mat4f views[6] = {
		Mat4f::CreateLookAt(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(-1.0f, 0.0f, 0.0f),   Vec3f(0.0f, 1.0f, 0.0f)),
		Mat4f::CreateLookAt(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(1.0f, 0.0f, 0.0f),  Vec3f(0.0f, 1.0f, 0.0f)),
		Mat4f::CreateLookAt(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 1.0f, 0.0f),   Vec3f(0.0f, 0.0f, -1.0f)),
		Mat4f::CreateLookAt(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, -1.0f, 0.0f),  Vec3f(0.0f, 0.0f, 1.0f)),
		Mat4f::CreateLookAt(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 0.0f, 1.0f),   Vec3f(0.0f, 1.0f, 0.0f)),
		Mat4f::CreateLookAt(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f),  Vec3f(0.0f, 1.0f, 0.0f))
	};

	struct FrameBuffer
	{
		Mat4f VP[6];
		f32 roughness;
	} frameBufferData;

	for (i32 i = 0; i < 6; i++)
		frameBufferData.VP[i] = views[i] * proj;

	Buffer* frameBuffer = Allocate();
	frameBuffer->Init(sizeof(FrameBuffer), BufferUsage::Dynamic, BufferType::Constant, 0, &frameBufferData);

	Shader generateCubemapShader(Shader::Type::Pixel | Shader::Type::Vertex | Shader::Type::Geometry,
		"../source/Engine/Shaders/generate_cubemap_ps.fx",
		"../source/Engine/Shaders/generate_cubemap_vs.fx",
		"../source/Engine/Shaders/generate_cubemap_gs.fx");

	//Render the cubemap
	hdriTexture->Bind(0);
	cubemapTexture->SetAsActiveTarget();
	generateCubemapShader.Bind();
	frameBuffer->SetData(frameBufferData);
	frameBuffer->Bind(0);
	Model* cube = Model::GetCube();

	m_ObjectBufferData.model = cube->GetTransform();
	m_ObjectBufferData.material = cube->GetMaterial();
	m_ObjectBuffer.SetData(m_ObjectBufferData);
	m_ObjectBuffer.Bind(1);

	//There should only be one.
	auto* cubeMesh = cube->GetMeshes()[0];
	auto* context = Framework::GetContext();
	cubeMesh->vertexBuffer.Bind();
	cubeMesh->indexBuffer.Bind();
	context->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)cubeMesh->topology);
	context->DrawIndexed(cubeMesh->indexCount, 0, 0);

	context->GSSetShader(nullptr, nullptr, 0);

	Framework::GetContext()->GenerateMips(cubemapTexture->GetShaderResourceView());

	//Free resources
	Free(cube);
	Free(frameBuffer);

	Framework::EndEvent();
	return cubemapTexture;
}

void frostwave::DeferredRenderer::PrefilterPBRTextures(Texture* environmentMap)
{
	ConvoluteCubemap(environmentMap);
	PrefilterSpecularCubemap(environmentMap);
	GenerateBRDFTexture();
}

void frostwave::DeferredRenderer::ConvoluteCubemap(Texture* environmentMap)
{
	Framework::BeginEvent("Convolute Irradiance Cubemap");
	m_IrradianceTexture = Allocate();
	TextureCreateInfo texInfo = {
		.size = { 32, 32 },
		.format = ImageFormat::DXGI_FORMAT_R16G16B16A16_FLOAT,
		.data = nullptr,
		.renderTarget = true,
		.hdr = true,
		.cubemap = true
	};
	m_IrradianceTexture->Create(texInfo);
	m_IrradianceTexture->Clear({ 0, 0, 0, 0 });

	Mat4f proj = Mat4f::CreatePerspectiveProjection(90.0f, 1, 0.1f, 10.0f);

	Mat4f views[6] = {
		Mat4f::CreateLookAt(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(-1.0f, 0.0f, 0.0f),   Vec3f(0.0f, 1.0f, 0.0f)),
		Mat4f::CreateLookAt(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(1.0f, 0.0f, 0.0f),  Vec3f(0.0f, 1.0f, 0.0f)),
		Mat4f::CreateLookAt(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 1.0f, 0.0f),   Vec3f(0.0f, 0.0f, -1.0f)),
		Mat4f::CreateLookAt(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, -1.0f, 0.0f),  Vec3f(0.0f, 0.0f, 1.0f)),
		Mat4f::CreateLookAt(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 0.0f, 1.0f),   Vec3f(0.0f, 1.0f, 0.0f)),
		Mat4f::CreateLookAt(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f),  Vec3f(0.0f, 1.0f, 0.0f))
	};

	struct FrameBuffer
	{
		Mat4f VP[6];
		f32 roughness;
	} frameBufferData;

	for (i32 i = 0; i < 6; i++)
		frameBufferData.VP[i] = views[i] * proj;

	Buffer* frameBuffer = Allocate();
	frameBuffer->Init(sizeof(FrameBuffer), BufferUsage::Dynamic, BufferType::Constant, 0, &frameBufferData);

	Shader generateCubemapShader(Shader::Type::Pixel | Shader::Type::Vertex | Shader::Type::Geometry,
		"../source/Engine/Shaders/generate_irradiance_map_ps.fx",
		"../source/Engine/Shaders/generate_cubemap_vs.fx",
		"../source/Engine/Shaders/generate_cubemap_gs.fx");

	//Render the cubemap
	m_IrradianceTexture->SetAsActiveTarget();
	environmentMap->Bind(0);
	generateCubemapShader.Bind();
	frameBuffer->SetData(frameBufferData);
	frameBuffer->Bind(0);
	Model* cube = Model::GetCube();

	m_ObjectBufferData.model = cube->GetTransform();
	m_ObjectBufferData.material = cube->GetMaterial();
	m_ObjectBuffer.SetData(m_ObjectBufferData);
	m_ObjectBuffer.Bind(1);

	//There should only be one.
	auto* cubeMesh = cube->GetMeshes()[0];
	auto* context = Framework::GetContext();
	cubeMesh->vertexBuffer.Bind();
	cubeMesh->indexBuffer.Bind();
	context->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)cubeMesh->topology);
	context->DrawIndexed(cubeMesh->indexCount, 0, 0);

	context->GSSetShader(nullptr, nullptr, 0);

	//Free resources
	Free(cube);
	Free(frameBuffer);

	Framework::EndEvent();
}

void frostwave::DeferredRenderer::PrefilterSpecularCubemap(Texture* environmentMap)
{
	Framework::BeginEvent("Prefilter Specular map");
	m_PrefilteredTexture = Allocate();
	m_PrefilteredTexture->Create(TextureCreateInfo{
		.size = { 128, 128 },
		.format = ImageFormat::DXGI_FORMAT_R16G16B16A16_FLOAT,
		.data = nullptr,
		.renderTarget = true,
		.hdr = true,
		.cubemap = true,
		.numMips = 5
		});
	m_PrefilteredTexture->Clear({ 0, 0, 0, 0 });

	Mat4f proj = Mat4f::CreatePerspectiveProjection(90.0f, 1, 0.1f, 10.0f);

	Mat4f views[6] = {
		Mat4f::CreateLookAt(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(-1.0f, 0.0f, 0.0f),   Vec3f(0.0f, 1.0f, 0.0f)),
		Mat4f::CreateLookAt(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(1.0f, 0.0f, 0.0f),  Vec3f(0.0f, 1.0f, 0.0f)),
		Mat4f::CreateLookAt(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 1.0f, 0.0f),   Vec3f(0.0f, 0.0f, -1.0f)),
		Mat4f::CreateLookAt(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, -1.0f, 0.0f),  Vec3f(0.0f, 0.0f, 1.0f)),
		Mat4f::CreateLookAt(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 0.0f, 1.0f),   Vec3f(0.0f, 1.0f, 0.0f)),
		Mat4f::CreateLookAt(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f),  Vec3f(0.0f, 1.0f, 0.0f))
	};

	struct FrameBuffer
	{
		Mat4f VP[6];
		f32 roughness;
	} frameBufferData;

	for (i32 i = 0; i < 6; i++)
		frameBufferData.VP[i] = views[i] * proj;

	Buffer* frameBuffer = Allocate();
	frameBuffer->Init(sizeof(FrameBuffer), BufferUsage::Dynamic, BufferType::Constant, 0, &frameBufferData);

	Shader generateCubemapShader(Shader::Type::Pixel | Shader::Type::Vertex | Shader::Type::Geometry,
		"../source/Engine/Shaders/generate_prefiltered_map_ps.fx",
		"../source/Engine/Shaders/generate_cubemap_vs.fx",
		"../source/Engine/Shaders/generate_cubemap_gs.fx");

	auto* context = Framework::GetContext();
	Model* cube = Model::GetCube();

	u32 maxMipLevels = 5;
	for (u32 mip = 0; mip < maxMipLevels; ++mip)
	{
		u32 mipWidth = (u32)(128 * std::pow(0.5, mip));
		u32 mipHeight = (u32)(128 * std::pow(0.5, mip));
		Framework::BeginEvent("Mip: " + std::to_string(mipWidth) + "x" + std::to_string(mipHeight));

		//Render the cubemap
		auto* renderTarget = m_PrefilteredTexture->CreateRenderTargetViewForMip(mip, true);
		Framework::GetContext()->OMSetRenderTargets(1, &renderTarget, nullptr);
		m_PrefilteredTexture->SetCustomViewport(0.0f, 0.0f, (f32)mipWidth, (f32)mipHeight);
		environmentMap->Bind(0);
		generateCubemapShader.Bind();
		frameBufferData.roughness = (f32)mip / (f32)(maxMipLevels - 1);
		frameBuffer->SetData(frameBufferData);
		frameBuffer->Bind(0);

		m_ObjectBufferData.model = cube->GetTransform();
		m_ObjectBufferData.material = cube->GetMaterial();
		m_ObjectBuffer.SetData(m_ObjectBufferData);
		m_ObjectBuffer.Bind(1);

		//There should only be one.
		auto* cubeMesh = cube->GetMeshes()[0];
		cubeMesh->vertexBuffer.Bind();
		cubeMesh->indexBuffer.Bind();
		context->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)cubeMesh->topology);
		context->DrawIndexed(cubeMesh->indexCount, 0, 0);
		renderTarget->Release();
		Framework::EndEvent();
	}

	context->GSSetShader(nullptr, nullptr, 0);

	//Free resources
	Free(cube);
	Free(frameBuffer);

	Framework::EndEvent();
}
void frostwave::DeferredRenderer::GenerateBRDFTexture()
{
	Framework::BeginEvent("Prefilter BRDF map");
	m_BRDFTexture = Allocate();
	m_BRDFTexture->Create(TextureCreateInfo{
		.size = { 512, 512 },
		.format = ImageFormat::DXGI_FORMAT_R16G16_FLOAT
		});
	m_BRDFTexture->Clear({ 0, 0, 0, 0 });

	Shader brdfShader(Shader::Type::Pixel | Shader::Type::Vertex,
		"../source/Engine/Shaders/generate_brdf_ps.fx",
		"../source/Engine/Shaders/fullscreen_vs.fx");

	brdfShader.Bind();
	m_BRDFTexture->SetAsActiveTarget();

	auto* context = Framework::GetContext();
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(nullptr);
	context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
	context->Draw(3, 0);

	Framework::EndEvent();
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

		//Bind empty texture to all slots if there is no texture
		for (size_t i = 0; i < 5; i++)
			m_NullTexture.Bind((u32)i);

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
	ID3D11DeviceContext* context = Framework::GetContext();

	Framework::BeginEvent("Ambient Light");
	{
		if (m_IrradianceTexture)
			m_IrradianceTexture->Bind(14);
		if (m_PrefilteredTexture)
			m_PrefilteredTexture->Bind(15);
		if (m_BRDFTexture)
			m_BRDFTexture->Bind(16);


		stateManager->SetRasterizerState(RenderStateManager::RasterizerStates::BackCull);

		m_AmbientLightShader.Bind();

		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->IASetInputLayout(nullptr);
		context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
		context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
		context->Draw(3, 0);
	}
	Framework::EndEvent();

	Framework::BeginEvent("Directional Lights");
	m_DirectionalLightShader.Bind();
	for (auto* light : m_DirectionalLights)
	{
		if (light->GetShadowData().shadowMap)
			light->GetShadowData().shadowMap->Bind(8);

		m_LightingBufferData.dirLight.direction = Vec4f(light->GetDirection().GetNormalized(), 0.0);
		m_LightingBufferData.dirLight.color = Vec4f(light->GetColor(), light->GetIntensity());
		m_LightingBufferData.lightMatrix = light->GetShadowData().viewProj;
		m_LightingBuffer.SetData(m_LightingBufferData);
		m_LightingBuffer.Bind(2);

		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->IASetInputLayout(nullptr);
		context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
		context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
		context->Draw(3, 0);
	}
	Framework::EndEvent();
	m_DirectionalLights.clear();

	Framework::BeginEvent("Point Lights");
	m_PointLightShader.Bind();
	stateManager->SetRasterizerState(RenderStateManager::RasterizerStates::NoCull);
	for (auto* light : m_PointLights)
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
	Framework::EndEvent();
	m_PointLights.clear();
}

void frostwave::DeferredRenderer::Submit(Model* model)
{
	m_Models.push_back(model);
}

void frostwave::DeferredRenderer::Submit(DirectionalLight* light)
{
	m_DirectionalLights.push_back(light);
}

void frostwave::DeferredRenderer::Submit(PointLight* light)
{
	m_PointLights.push_back(light);
}