#include "ForwardRenderer.h"
#include <Engine/Graphics/Error.h>
#include <Engine/Core/Common.h>
#include <Engine/Memory/Allocator.h>
#include <Engine/Graphics/Framework.h>
#include <Engine/Graphics/imgui/imgui.h>
#include <Engine/Graphics/imgui/imguizmo/ImGuizmo.h>
#include <algorithm>
#include <d3d11.h>

frostwave::ForwardRenderer::ForwardRenderer()
{
}

frostwave::ForwardRenderer::~ForwardRenderer()
{
}

void frostwave::ForwardRenderer::Init()
{
	m_FrameBuffer.Init(sizeof(FrameBuffer), BufferUsage::Dynamic, BufferType::Constant, 0, &m_FrameBufferData);
	m_ObjectBuffer.Init(sizeof(ObjectBuffer), BufferUsage::Dynamic, BufferType::Constant, 0, &m_ObjectBufferData);
}

void frostwave::ForwardRenderer::Render(f32 totalTime, Camera* camera)
{
	totalTime;
	m_FrameBufferData.projection = camera->GetProjection();
	m_FrameBufferData.view = camera->GetView();
	m_FrameBufferData.cameraPos = Vec4f(camera->GetPosition(), 1.0f);
	m_FrameBufferData.lightColor = Vec4f(1, 1, 1, 1);
	m_FrameBufferData.lightDir = Vec4f(1, 1, 1, 0);

	memset(m_FrameBufferData.lights, 0, sizeof(PointLight) * 8);

	for (size_t i = 0; i < m_Lights.size(); i++)
	{
		if (i >= 32) break;
		m_FrameBufferData.lights[i] = m_Lights[i];
	}
	m_FrameBuffer.SetData(m_FrameBufferData);
	m_FrameBuffer.Bind(0);

	std::sort(m_Models.begin(), m_Models.end(), [&](Model* a, Model* b) {
		return (a->GetPosition() - camera->GetPosition()).LengthSqr() > (b->GetPosition() - camera->GetPosition()).LengthSqr();
		});

	if(m_EnvironmentMap)
		m_EnvironmentMap->Bind(16);

	for (auto* model : m_Models)
	{
		m_ObjectBufferData.model = model->GetTransform();
		m_ObjectBufferData.material = model->GetMaterial();
		m_ObjectBuffer.SetData(m_ObjectBufferData);
		m_ObjectBuffer.Bind(1);

		model->GetShader()->Bind();
		for (auto* mesh : model->GetMeshes())
		{
			auto* context = Framework::GetContext();
			for (size_t i = 0; i < mesh->textures.size(); i++)
			{
				if (mesh->textures[i])
					mesh->textures[i]->Bind((u32)i);
			}

			//Bind empty texture to all slots if there is no texture
			if (mesh->textures.size() == 0)
				for (size_t i = 0; i < 4; i++)
					m_NullTexture.Bind((u32)i);

			mesh->vertexBuffer.Bind();
			mesh->indexBuffer.Bind();

			//mesh->shader.Bind();

			context->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)mesh->topology);
			context->DrawIndexed(mesh->indexCount, 0, 0);
		}
	}

	m_Models.clear();
	m_Lights.clear();
}

void frostwave::ForwardRenderer::Submit(Model* model)
{
	m_Models.push_back(model);
}

void frostwave::ForwardRenderer::Submit(const PointLight& light)
{
	m_Lights.push_back(light);
}

void frostwave::ForwardRenderer::Submit(Texture* envMap)
{
	m_EnvironmentMap = envMap;
}