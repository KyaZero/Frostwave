#include "ShadowRenderer.h"
#include <Engine/Graphics/Framework.h>
#include <d3d11.h>

frostwave::ShadowRenderer::ShadowRenderer()
{
}

frostwave::ShadowRenderer::~ShadowRenderer()
{
}

void frostwave::ShadowRenderer::Init()
{
	m_FrameBuffer.Init(sizeof(FrameBuffer), BufferUsage::Dynamic, BufferType::Constant, 0, &m_FrameBufferData);
	m_ObjectBuffer.Init(sizeof(ObjectBuffer), BufferUsage::Dynamic, BufferType::Constant, 0, &m_ObjectBufferData);
	m_ShadowShader.Load(Shader::Type::Vertex | Shader::Type::Pixel, "assets/shaders/shadow_ps.fx", "assets/shaders/shadow_vs.fx");
}

void frostwave::ShadowRenderer::Render(Camera* camera)
{
	auto dirProjection = Mat4f::CreateOrthographicProjection(50, 50, -50, 50);
	for (auto* light : m_DirectionalLights)
	{
		auto& shadowData = light->GetShadowData();
		i32 ShadowMapSize = 4096;

		//Create shadowmap and depth
		if (!shadowData.shadowMap && !shadowData.depth)
		{
			shadowData.shadowMap = Allocate();
			shadowData.shadowMap->Create(Vec2i(ShadowMapSize, ShadowMapSize), ImageFormat::DXGI_FORMAT_R32_FLOAT);
			shadowData.depth = Allocate();
			shadowData.depth->CreateDepth(Vec2i(ShadowMapSize, ShadowMapSize));
		}

		auto dir = light->GetDirection().GetNormalized();

		auto rot = Mat4f::CreateLookAt(Vec3f(), dir, { 0,1,0 });

		Vec3f camPos = camera->GetPosition();
		Vec3f forward = camera->GetRotation().GetForwardVector();

		auto pos = camPos;
		auto f = forward;
		f = Vec3f(f.x, 0, f.z).GetNormalized();

		auto v = Mat4f::CreateTransform(pos + f * 25.0f, rot, 1);
		auto vp = v.FastInverse(v) * dirProjection;

		//
		Vec3f shadowOrigin = Vec3f() * vp;
		shadowOrigin *= (ShadowMapSize / 2.0f);
		Vec2f roundedOrigin = Vec2f((f32)round(shadowOrigin.x), (f32)round(shadowOrigin.y));
		Vec2f rounding = roundedOrigin - Vec2f(shadowOrigin.x, shadowOrigin.y);
		rounding /= (ShadowMapSize / 2.0f);

		Mat4f roundMatrix = Mat4f::CreateTranslationMatrix(rounding.x, rounding.y, 0.0f);

		vp *= roundMatrix;

		shadowData.ViewProj = vp;

		m_FrameBufferData.VP = vp;
		m_FrameBuffer.SetData(m_FrameBufferData);
		m_FrameBuffer.Bind(0);

		shadowData.shadowMap->Clear({ 1,1,1,1 });
		shadowData.depth->ClearDepth();
		shadowData.shadowMap->SetAsActiveTarget(shadowData.depth);

		std::sort(m_Models.begin(), m_Models.end(), [&](Model* a, Model* b) {
			return (a->GetPosition() - camera->GetPosition()).LengthSqr() > (b->GetPosition() - camera->GetPosition()).LengthSqr();
		});

		m_ShadowShader.Bind();

		auto* context = Framework::GetContext();
		for (auto* model : m_Models)
		{
			m_ObjectBufferData.model = model->GetTransform();
			m_ObjectBuffer.SetData(m_ObjectBufferData);
			m_ObjectBuffer.Bind(1);

			for (auto* mesh : model->GetMeshes())
			{
				mesh->vertexBuffer.Bind();
				mesh->indexBuffer.Bind();
				context->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)mesh->topology);
				context->DrawIndexed(mesh->indexCount, 0, 0);
			}
		}
	}

	m_Models.clear();
	m_DirectionalLights.clear();
}

void frostwave::ShadowRenderer::Submit(Model* model)
{
	m_Models.push_back(model);
}

void frostwave::ShadowRenderer::Submit(DirectionalLight* light)
{
	m_DirectionalLights.push_back(light);
}
