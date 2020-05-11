#include "PostProcessor.h"
#include <Engine/Graphics/Framework.h>
#include <Engine/Memory/Allocator.h>
#include <cassert>
#include <d3d11.h>

frostwave::PostProcessor::PostProcessor()
{
}

frostwave::PostProcessor::~PostProcessor()
{
	for (auto& tech : m_Techniques)
	{
		for (auto& stage : tech.stages)
		{
			if (stage.pixelShader)
				Free(stage.pixelShader);
		}
	}
	Free(m_FullscreenVertexShader);
}

void frostwave::PostProcessor::Init()
{
	m_FrameBuffer.Init(sizeof(FrameBuffer), BufferUsage::Dynamic, BufferType::Constant, 0, &m_FrameBufferData);
	m_FullscreenVertexShader = Allocate<Shader>(Shader::Type::Vertex, "../source/Engine/Shaders/fullscreen_vs.fx");

	//ssao stuff
	for (u32 i = 0; i < 16; ++i)
	{
		Vec4f sample(
			fw::Rand() * 2.0f - 1.0f,
			fw::Rand() * 2.0f - 1.0f,
			fw::Rand(),
			0.0f
		);
		sample.Normalize();
		sample *= fw::Rand();
		f32 scale = (f32)i / 16.0f;
		scale = fw::Lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		m_SSAOKernel.push_back(sample);
	}
}

void frostwave::PostProcessor::Render(Texture* backBuffer, Camera* camera, DirectionalLight* light)
{
	m_FullscreenVertexShader->Bind();

	m_FrameBufferData.view = camera->GetView();
	m_FrameBufferData.projection = camera->GetProjection();
	m_FrameBufferData.invProjection = Mat4f::Inverse(camera->GetProjection());
	m_FrameBufferData.invView = Mat4f::Inverse(camera->GetView());
	m_FrameBufferData.lightMatrix = light ? light->GetShadowData().viewProj : Mat4f();
	m_FrameBufferData.cameraPos = camera->GetPosition();
	m_FrameBufferData.lightDirection = light ? light->GetDirection().GetNormalized() : Vec4f();
	m_FrameBufferData.lightColor = light ? Vec4f(light->GetColor(), light->GetIntensity()) : Vec4f();
	m_FrameBufferData.resolution = Vec2f((f32)Window::Get()->GetWidth(), (f32)Window::Get()->GetHeight());
	m_FrameBufferData.farPlane = camera->GetFarPlane();
	m_FrameBufferData.nearPlane = camera->GetNearPlane();
	for (size_t i = 0; i < 16; i++)
	{
		m_FrameBufferData.kernel[i] = m_SSAOKernel[i];
	}

	for (auto&& tech : m_Techniques)
	{
		Framework::BeginEvent(tech.name);
		for (auto&& stage : tech.stages)
		{
			Framework::BeginEvent(stage.name);
			RenderStage(stage, backBuffer);
			Framework::EndEvent();
		}
		Framework::Timestamp(tech.name);
		Framework::EndEvent();
	}
}

void frostwave::PostProcessor::Push(PostProcessStage stage)
{
	Technique technique;
	technique.name = stage.name;
	technique.stages.push_back(stage);
	m_Techniques.push_back(technique);
}

void frostwave::PostProcessor::Push(Technique tech)
{
	m_Techniques.push_back(tech);
}

void frostwave::PostProcessor::Clear()
{
	m_Techniques.clear();
}

void frostwave::PostProcessor::RenderStage(PostProcessStage stage, Texture* backBuffer)
{
	if (!stage.renderToBackbuffer)
	{
		m_FrameBufferData.size = Vec2f((f32)stage.renderTarget->GetSize().x, (f32)stage.renderTarget->GetSize().y);
		m_FrameBufferData.texelSize = Vec2f(1.0f / m_FrameBufferData.size.x, 1.0f / m_FrameBufferData.size.y);
	}

	m_FrameBuffer.SetData(m_FrameBufferData);
	m_FrameBuffer.Bind(0);

	Texture::UnsetActiveTarget();
	stage.pixelShader->Bind();

	if (stage.renderToBackbuffer)
		backBuffer->SetAsActiveTarget();
	else if (stage.renderTarget)
		stage.renderTarget->SetAsActiveTarget();

	i32 shaderResourceIndex = 0;
	for (auto* resource : stage.shaderResources)
	{
		resource->Bind(shaderResourceIndex++);
	}

	auto* context = Framework::GetContext();
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(nullptr);
	context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
	context->Draw(3, 0);

	for (i32 i = 0; i < shaderResourceIndex; ++i)
	{
		Texture::Unbind(i);
	}
}