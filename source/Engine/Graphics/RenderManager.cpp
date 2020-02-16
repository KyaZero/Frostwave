#include "RenderManager.h"
#include <Engine/Graphics/Framework.h>
#include <Engine/Memory/Allocator.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/ForwardRenderer.h>
#include <Engine/Graphics/DeferredRenderer.h>
#include <Engine/Graphics/ShadowRenderer.h>
#include <Engine/Graphics/PostProcessor.h>
#include <Engine/Graphics/Error.h>
#include <windows.h>

frostwave::RenderManager::RenderManager() : m_DirectionalLight(nullptr)
{
	m_ForwardRenderer = Allocate();
	m_DeferredRenderer = Allocate();
	m_ShadowRenderer = Allocate();
	m_PostProcessor = Allocate();
	m_IntermediateTexture = Allocate();
	m_IntermediateTexture2 = Allocate();
	m_IntermediateTexture3 = Allocate();
	m_HalfSizeTexturePing = Allocate();
	m_HalfSizeTexturePong = Allocate();
	m_QuarterSizeTexturePing = Allocate();
	m_QuarterSizeTexturePong = Allocate();
	m_BloomTexture = Allocate();
	m_ResolveHDRTexture = Allocate();
	m_IntermediateDepth = Allocate();
	m_GBuffer = Allocate();
	m_SSAONoiseTexture = Allocate();
}

frostwave::RenderManager::~RenderManager()
{
	Free(m_SSAONoiseTexture);
	Free(m_GBuffer);
	Free(m_IntermediateDepth);
	Free(m_ResolveHDRTexture);
	Free(m_BloomTexture);
	Free(m_QuarterSizeTexturePong);
	Free(m_QuarterSizeTexturePing);
	Free(m_HalfSizeTexturePing);
	Free(m_HalfSizeTexturePong);
	Free(m_IntermediateTexture3);
	Free(m_IntermediateTexture2);
	Free(m_IntermediateTexture);
	Free(m_PostProcessor);
	Free(m_ShadowRenderer);
	Free(m_DeferredRenderer);
	Free(m_ForwardRenderer);
	Free(m_LinearClampSampler);
	Free(m_LinearWrapSampler);
	Free(m_PointClampSampler);
	Free(m_PointWrapSampler);
}

void frostwave::RenderManager::Init()
{
	m_LinearClampSampler = Allocate<Sampler>(Sampler::Filter::Linear, Sampler::Address::Clamp, Vec4f());
	m_LinearClampSampler->Bind(0);

	m_LinearWrapSampler = Allocate<Sampler>(Sampler::Filter::Linear, Sampler::Address::Wrap, Vec4f());
	m_LinearWrapSampler->Bind(1);

	m_PointClampSampler = Allocate<Sampler>(Sampler::Filter::Point, Sampler::Address::Clamp, Vec4f());
	m_PointClampSampler->Bind(2);

	m_PointWrapSampler = Allocate<Sampler>(Sampler::Filter::Point, Sampler::Address::Wrap, Vec4f());
	m_PointWrapSampler->Bind(3);

	m_ForwardRenderer->Init();
	m_DeferredRenderer->Init();
	m_ShadowRenderer->Init();
	m_PostProcessor->Init();
	m_StateManager.Init();
	m_IntermediateTexture->Create(Window::Get()->GetBounds(), ImageFormat::DXGI_FORMAT_R32G32B32A32_FLOAT);
	m_IntermediateTexture2->Create(Window::Get()->GetBounds(), ImageFormat::DXGI_FORMAT_R32G32B32A32_FLOAT);
	m_IntermediateTexture3->Create(Window::Get()->GetBounds(), ImageFormat::DXGI_FORMAT_R32G32B32A32_FLOAT);
	m_HalfSizeTexturePing->Create(Window::Get()->GetBounds() / 2, ImageFormat::DXGI_FORMAT_R32G32B32A32_FLOAT);
	m_HalfSizeTexturePong->Create(Window::Get()->GetBounds() / 2, ImageFormat::DXGI_FORMAT_R32G32B32A32_FLOAT);
	m_QuarterSizeTexturePing->Create(Window::Get()->GetBounds() / 4, ImageFormat::DXGI_FORMAT_R32G32B32A32_FLOAT);
	m_QuarterSizeTexturePong->Create(Window::Get()->GetBounds() / 4, ImageFormat::DXGI_FORMAT_R32G32B32A32_FLOAT);
	m_BloomTexture->Create(Window::Get()->GetBounds(), ImageFormat::DXGI_FORMAT_R32G32B32A32_FLOAT);
	m_ResolveHDRTexture->Create(Window::Get()->GetBounds());
	m_IntermediateDepth->CreateDepth(Window::Get()->GetBounds());
	m_GBuffer->Create(Window::Get()->GetBounds());

	float noiseTextureFloats[256];
	for (int i = 0; i < 64; i++)
	{
		int index = i * 3;
		noiseTextureFloats[index] = fw::Rand11();
		noiseTextureFloats[index + 1] = fw::Rand11();
		noiseTextureFloats[index + 2] = 0.0f;
		noiseTextureFloats[index + 3] = 0.0f;
	}

	m_SSAONoiseTexture->Create({ 8,8 }, ImageFormat::DXGI_FORMAT_R32G32B32A32_FLOAT, noiseTextureFloats, false);

	Window::Get()->Subscribe(WM_SIZE, [&](auto, auto) {
		ResizeTextures(Window::Get()->GetWidth(), Window::Get()->GetHeight());
	});

	InitPostProcessing();
}

void frostwave::RenderManager::Render(f32 totalTime, Camera* camera, Texture* backBuffer)
{
	if (!camera)
	{
		ERROR_LOG("No camera set whilst in 3D render path... returning.");
		return;
	}

	Framework::BeginEvent("Clear Textures");
	ClearTextures();
	Framework::EndEvent();

	Texture::UnbindAll();

	Framework::BeginEvent("Render Shadowmaps");
	m_StateManager.SetRasterizerState(RenderStateManager::RasterizerStates::NoCull);
	m_ShadowRenderer->Render(camera);
	Texture::UnsetActiveTarget();
	m_StateManager.SetRasterizerState(RenderStateManager::RasterizerStates::Default);
	Framework::Timestamp("Render Shadowmaps");
	Framework::EndEvent();

	Framework::BeginEvent("Render Geometry to GBuffer");
	m_GBuffer->SetAsActiveTarget(m_IntermediateDepth);
	m_DeferredRenderer->RenderGeometry(totalTime, camera);
	Framework::Timestamp("Render Geometry to GBuffer");
	Framework::EndEvent();

	Framework::BeginEvent("Render Deferred lighting");
	m_StateManager.SetBlendState(RenderStateManager::BlendStates::Additive);
	m_IntermediateTexture->SetAsActiveTarget();
	m_GBuffer->SetAllAsResources();
	m_IntermediateDepth->Bind(4);
	m_DeferredRenderer->RenderLighting(totalTime, &m_StateManager);
	m_GBuffer->RemoveAllAsResources();
	Framework::Timestamp("Render Deferred lighting");
	Framework::EndEvent();

	//Render all alpha meshes (forward shading)
	Framework::BeginEvent("Render Forward");
	m_StateManager.SetRasterizerState(RenderStateManager::RasterizerStates::Default);
	m_StateManager.SetBlendState(RenderStateManager::BlendStates::AlphaBlend);
	//m_IntermediateTexture->SetAsActiveTarget(m_IntermediateDepth);
	//m_ForwardRenderer->Render(totalTime, camera);
	Framework::Timestamp("Render Forward");
	Framework::EndEvent();

	//Render post processing
	m_StateManager.SetBlendState(RenderStateManager::BlendStates::Disable);
	m_SSAONoiseTexture->Bind(15);
	Framework::BeginEvent("Post Processing");
	m_PostProcessor->Render(backBuffer, camera, m_DirectionalLight);
	Framework::EndEvent();
}

void frostwave::RenderManager::ResizeTextures(i32 width, i32 height)
{
	if (width == 0 || height == 0) return;

	fw::Vec2i size = { width, height };

	m_IntermediateTexture->Release();
	m_IntermediateTexture2->Release();
	m_IntermediateTexture3->Release();
	m_HalfSizeTexturePong->Release();
	m_HalfSizeTexturePing->Release();
	m_QuarterSizeTexturePing->Release();
	m_QuarterSizeTexturePong->Release();
	m_BloomTexture->Release();
	m_ResolveHDRTexture->Release();
	m_IntermediateDepth->Release();
	m_GBuffer->Release();

	m_GBuffer->Create(size);
	m_IntermediateTexture->Create(size, ImageFormat::DXGI_FORMAT_R32G32B32A32_FLOAT);
	m_IntermediateTexture2->Create(size, ImageFormat::DXGI_FORMAT_R32G32B32A32_FLOAT);
	m_IntermediateTexture3->Create(size, ImageFormat::DXGI_FORMAT_R32G32B32A32_FLOAT);
	m_HalfSizeTexturePing->Create(size / 2, ImageFormat::DXGI_FORMAT_R32G32B32A32_FLOAT);
	m_HalfSizeTexturePong->Create(size / 2, ImageFormat::DXGI_FORMAT_R32G32B32A32_FLOAT);
	m_QuarterSizeTexturePing->Create(size / 4, ImageFormat::DXGI_FORMAT_R32G32B32A32_FLOAT);
	m_QuarterSizeTexturePong->Create(size / 4, ImageFormat::DXGI_FORMAT_R32G32B32A32_FLOAT);
	m_BloomTexture->Create(size, ImageFormat::DXGI_FORMAT_R32G32B32A32_FLOAT);
	m_ResolveHDRTexture->Create(size);
	m_IntermediateDepth->CreateDepth(size);
}

void frostwave::RenderManager::Submit(Model* model)
{
	if (!model) return;
	m_ForwardRenderer->Submit(model);
	m_DeferredRenderer->Submit(model);
	m_ShadowRenderer->Submit(model);
}

void frostwave::RenderManager::Submit(DirectionalLight* light)
{
	if (!light) return;
	m_DirectionalLight = light;
	m_DeferredRenderer->Submit(light);
	m_ShadowRenderer->Submit(light);
}

void frostwave::RenderManager::Submit(PointLight* light)
{
	if (!light) return;
	//m_ForwardRenderer->Submit(light);
	m_DeferredRenderer->Submit(light);
}

void frostwave::RenderManager::Submit(EnvironmentLight* light)
{
	if (!light) return;
	m_DeferredRenderer->Submit(light);
}

void frostwave::RenderManager::ClearTextures()
{
	Texture::UnsetActiveTarget();

	m_IntermediateTexture->Clear();
	m_IntermediateTexture2->Clear();
	m_IntermediateTexture3->Clear();
	m_HalfSizeTexturePing->Clear();
	m_HalfSizeTexturePong->Clear();
	m_QuarterSizeTexturePing->Clear();
	m_QuarterSizeTexturePong->Clear();
	m_BloomTexture->Clear();
	m_ResolveHDRTexture->Clear();
	m_GBuffer->ClearTexture();
	m_IntermediateDepth->ClearDepth();
}

void frostwave::RenderManager::InitPostProcessing()
{
	Technique bloom = { };
	bloom.name = "Bloom";
	bloom.Push(PostProcessStage("assets/shaders/luminance_ps.fx", { m_IntermediateTexture }, m_IntermediateTexture2, "Calculate Luminance"));
	bloom.Push(PostProcessStage("assets/shaders/copy_ps.fx", { m_IntermediateTexture2 }, m_HalfSizeTexturePing, "Downscale To Half"));
	bloom.Push(PostProcessStage("assets/shaders/copy_ps.fx", { m_HalfSizeTexturePing }, m_QuarterSizeTexturePing, "Downscale to Quarter"));
	bloom.Push(PostProcessStage("assets/shaders/gaussianh_ps.fx", { m_QuarterSizeTexturePing }, m_QuarterSizeTexturePong, "Gaussian Blur Horizontal"));
	bloom.Push(PostProcessStage("assets/shaders/gaussianv_ps.fx", { m_QuarterSizeTexturePong }, m_QuarterSizeTexturePing, "Gaussian Blur Vertical"));
	bloom.Push(PostProcessStage("assets/shaders/copy_ps.fx", { m_QuarterSizeTexturePing }, m_HalfSizeTexturePing, "Upscale to Half"));
	bloom.Push(PostProcessStage("assets/shaders/copy_ps.fx", { m_HalfSizeTexturePing }, m_BloomTexture, "Upscale to Full"));
	bloom.Push(PostProcessStage("assets/shaders/blend_bloom_ps.fx", { m_IntermediateTexture, m_BloomTexture }, m_IntermediateTexture2, "Blend Bloom with Scene"));
	m_PostProcessor->Push(bloom);

	Technique ssao = { };
	ssao.name = "Screen-Space Ambient Occlusion";
	ssao.Push(PostProcessStage("assets/shaders/ssao_ps.fx", { m_IntermediateDepth, m_GBuffer->GetTexture(GBuffer::Textures::Normal) }, m_IntermediateTexture3, "Calculate Occlusion"));
	ssao.Push(PostProcessStage("assets/shaders/ssao_blur_h_ps.fx", { m_IntermediateTexture3, m_IntermediateDepth }, m_IntermediateTexture, "Aware Blur Horizontal"));
	ssao.Push(PostProcessStage("assets/shaders/ssao_blur_v_ps.fx", { m_IntermediateTexture, m_IntermediateDepth }, m_IntermediateTexture3, "Aware Blur Vertical"));
	//ssao.Push(PostProcessStage("assets/shaders/copy_ps.fx",	{ m_IntermediateTexture3 }, m_BloomTexture, "Upscale to Full"));
	ssao.Push(PostProcessStage("assets/shaders/blend_ssao_ps.fx", { m_IntermediateTexture2, m_IntermediateTexture3 }, m_IntermediateTexture, "Blend SSAO with Scene"));
	m_PostProcessor->Push(ssao);

	Technique volumetricLighting = { };
	volumetricLighting.name = "Volumetric Lighting";
	volumetricLighting.Push(PostProcessStage("assets/shaders/volumetric_light_ps.fx", { m_IntermediateDepth }, m_HalfSizeTexturePing, "Raymarching"));
	volumetricLighting.Push(PostProcessStage("assets/shaders/copy_ps.fx", { m_HalfSizeTexturePing }, m_BloomTexture, "Upscale to Full"));
	volumetricLighting.Push(PostProcessStage("assets/shaders/blend_volumetric_ps.fx", { m_IntermediateTexture, m_BloomTexture }, m_IntermediateTexture2, "Blend Volumetrics with Scene"));
	m_PostProcessor->Push(volumetricLighting);

	m_PostProcessor->Push(PostProcessStage("assets/shaders/fxaa_ps.fx", { m_IntermediateTexture2 }, m_IntermediateTexture, "FXAA"));

	m_PostProcessor->Push(PostProcessStage("assets/shaders/tonemapping_ps.fx", { m_IntermediateTexture }, m_ResolveHDRTexture, "Tonemapping"));
	m_PostProcessor->Push(PostProcessStage("assets/shaders/copy_ps.fx", { m_ResolveHDRTexture }, nullptr, "Copy To Backbuffer", true));
}