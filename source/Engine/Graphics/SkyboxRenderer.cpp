#include "SkyboxRenderer.h"
#include <Engine/Memory/Allocator.h>
#include <Engine/Graphics/Framework.h>
#include <d3d11.h>

frostwave::SkyboxRenderer::SkyboxRenderer()
{
}

frostwave::SkyboxRenderer::~SkyboxRenderer()
{
	Free(m_Cube);
	Free(m_FrameBuffer);
	Free(m_SkyboxShader);
}

void frostwave::SkyboxRenderer::Init()
{
	m_SkyboxShader = Allocate();
	m_SkyboxShader->Load(Shader::Type::Vertex | Shader::Type::Pixel, "../source/Engine/Shaders/skybox_ps.fx", "../source/Engine/Shaders/skybox_vs.fx");
	m_FrameBuffer = Allocate();
	m_FrameBuffer->Init(sizeof(FrameBuffer), BufferUsage::Dynamic, BufferType::Constant, 0, &m_FrameBufferData);

	m_Cube = Model::GetCube();
}

void frostwave::SkyboxRenderer::SetTexture(Texture* skyboxTexture)
{
	m_SkyboxTexture = skyboxTexture;
}

void frostwave::SkyboxRenderer::Render(f32, Camera* camera)
{
	m_FrameBufferData.projection = camera->GetProjection();
	m_FrameBufferData.view = camera->GetView();

	m_SkyboxShader->Bind();
	m_SkyboxTexture->Bind(0);

	//There should only be one.
	for (auto* mesh : m_Cube->GetMeshes())
	{
		auto* context = Framework::GetContext();
		mesh->vertexBuffer.Bind();
		mesh->indexBuffer.Bind();
		context->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)mesh->topology);
		context->DrawIndexed(mesh->indexCount, 0, 0);
	}
}