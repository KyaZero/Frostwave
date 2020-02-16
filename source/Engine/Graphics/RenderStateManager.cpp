#include "RenderStateManager.h"
#include <Engine/Core/Types.h>
#include <Engine/Graphics/Framework.h>
#include <Engine/Graphics/Error.h>
#include <Engine/Memory/Allocator.h>
#include <array>
#include <d3d11.h>

struct frostwave::RenderStateManager::Data
{
	std::array<ID3D11BlendState*, (i32)BlendStates::Count> blendStates = { nullptr };
	std::array<ID3D11DepthStencilState*, (i32)DepthStencilStates::Count> depthStencilStates = { nullptr };
	std::array<ID3D11RasterizerState*, (i32)RasterizerStates::Count> rasterizerStates = { nullptr };
};

frostwave::RenderStateManager::RenderStateManager()
{
	m_Data = Allocate();
}

frostwave::RenderStateManager::~RenderStateManager()
{
	SetBlendState(BlendStates::Disable);
	SetDepthStencilState(DepthStencilStates::Default);
	SetRasterizerState(RasterizerStates::Default);

	for (auto* resource : m_Data->blendStates)
	{
		if (resource)
			SafeRelease(&resource);
	}
	for (auto* resource : m_Data->depthStencilStates)
	{
		if (resource)
			SafeRelease(&resource);
	}
	for (auto* resource : m_Data->rasterizerStates)
	{
		if (resource)
			SafeRelease(&resource);
	}

	Free(m_Data);
}

bool frostwave::RenderStateManager::Init()
{
	CreateBlendStates();
	CreateDepthStencilStates();
	CreateRasterizerStates();
	return true;
}

void frostwave::RenderStateManager::SetBlendState(BlendStates aBlendState)
{
	const float blendFactor = 0.0F;
	Framework::GetContext()->OMSetBlendState(m_Data->blendStates[(i32)aBlendState], &blendFactor, 0xffffffff);
}

void frostwave::RenderStateManager::SetDepthStencilState(DepthStencilStates aDepthState)
{
	Framework::GetContext()->OMSetDepthStencilState(m_Data->depthStencilStates[(i32)aDepthState], 0);
}

void frostwave::RenderStateManager::SetRasterizerState(RasterizerStates aRasterizerState)
{
	Framework::GetContext()->RSSetState(m_Data->rasterizerStates[(i32)aRasterizerState]);
}

bool frostwave::RenderStateManager::CreateBlendStates()
{
	D3D11_BLEND_DESC alphaBlendDesc = { 0 };
	alphaBlendDesc.IndependentBlendEnable = true;
	alphaBlendDesc.RenderTarget[0].BlendEnable = true;
	alphaBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	alphaBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	alphaBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	alphaBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	alphaBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	alphaBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;
	alphaBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	D3D11_BLEND_DESC additiveBlendDesc = { 0 };
	additiveBlendDesc.RenderTarget[0].BlendEnable = true;
	additiveBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	additiveBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	additiveBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	additiveBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	additiveBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	additiveBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;
	additiveBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	ErrorCheck(Framework::GetDevice()->CreateBlendState(&alphaBlendDesc, &m_Data->blendStates[(i32)BlendStates::AlphaBlend]));
	ErrorCheck(Framework::GetDevice()->CreateBlendState(&additiveBlendDesc, &m_Data->blendStates[(i32)BlendStates::Additive]));

	m_Data->blendStates[(i32)BlendStates::Disable] = nullptr;

	return true;
}

bool frostwave::RenderStateManager::CreateDepthStencilStates()
{
	D3D11_DEPTH_STENCIL_DESC readOnlyDepthDesc = { 0 };
	ZeroMemory(&readOnlyDepthDesc, sizeof(readOnlyDepthDesc));
	readOnlyDepthDesc.DepthEnable = true;
	readOnlyDepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	readOnlyDepthDesc.DepthFunc = D3D11_COMPARISON_LESS;
	readOnlyDepthDesc.StencilEnable = false;
	readOnlyDepthDesc.StencilReadMask = 0xFF;
	readOnlyDepthDesc.StencilWriteMask = 0xFF;
	readOnlyDepthDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	readOnlyDepthDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	readOnlyDepthDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	readOnlyDepthDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	readOnlyDepthDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_ZERO;
	readOnlyDepthDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_ZERO;
	readOnlyDepthDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_ZERO;
	readOnlyDepthDesc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;

	D3D11_DEPTH_STENCIL_DESC lessEqualsDepthDesc = { 0 };
	lessEqualsDepthDesc.DepthEnable = true;
	lessEqualsDepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	lessEqualsDepthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	lessEqualsDepthDesc.StencilEnable = false;

	ErrorCheck(Framework::GetDevice()->CreateDepthStencilState(&readOnlyDepthDesc, &m_Data->depthStencilStates[(i32)DepthStencilStates::ReadOnly]));
	ErrorCheck(Framework::GetDevice()->CreateDepthStencilState(&lessEqualsDepthDesc, &m_Data->depthStencilStates[(i32)DepthStencilStates::LessEquals]));

	m_Data->depthStencilStates[(i32)DepthStencilStates::Default] = nullptr;
	return true;
}

bool frostwave::RenderStateManager::CreateRasterizerStates()
{
	D3D11_RASTERIZER_DESC wireframeRasterizerDesc = { };
	wireframeRasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	wireframeRasterizerDesc.CullMode = D3D11_CULL_BACK;
	wireframeRasterizerDesc.FrontCounterClockwise = false;
	wireframeRasterizerDesc.DepthBias = 0;
	wireframeRasterizerDesc.SlopeScaledDepthBias = 0.0f;
	wireframeRasterizerDesc.DepthBiasClamp = 0.0f;
	wireframeRasterizerDesc.DepthClipEnable = true;
	wireframeRasterizerDesc.ScissorEnable = false;
	wireframeRasterizerDesc.MultisampleEnable = false;
	wireframeRasterizerDesc.AntialiasedLineEnable = false;

	D3D11_RASTERIZER_DESC nocullRasterizerDesc = { };
	nocullRasterizerDesc.FillMode = D3D11_FILL_SOLID;
	nocullRasterizerDesc.CullMode = D3D11_CULL_NONE;
	nocullRasterizerDesc.FrontCounterClockwise = false;
	nocullRasterizerDesc.DepthBias = 0;
	nocullRasterizerDesc.SlopeScaledDepthBias = 0.0f;
	nocullRasterizerDesc.DepthBiasClamp = 0.0f;
	nocullRasterizerDesc.DepthClipEnable = true;
	nocullRasterizerDesc.ScissorEnable = false;
	nocullRasterizerDesc.MultisampleEnable = false;
	nocullRasterizerDesc.AntialiasedLineEnable = false;

	D3D11_RASTERIZER_DESC frontRasterizerDesc = { };
	frontRasterizerDesc.FillMode = D3D11_FILL_SOLID;
	frontRasterizerDesc.CullMode = D3D11_CULL_FRONT;
	frontRasterizerDesc.FrontCounterClockwise = false;
	frontRasterizerDesc.DepthBias = 0;
	frontRasterizerDesc.SlopeScaledDepthBias = 0.0f;
	frontRasterizerDesc.DepthBiasClamp = 0.0f;
	frontRasterizerDesc.DepthClipEnable = true;
	frontRasterizerDesc.ScissorEnable = false;
	frontRasterizerDesc.MultisampleEnable = false;
	frontRasterizerDesc.AntialiasedLineEnable = false;

	D3D11_RASTERIZER_DESC backCullRasterizerDesc = {};
	backCullRasterizerDesc.FillMode = D3D11_FILL_SOLID;
	backCullRasterizerDesc.CullMode = D3D11_CULL_BACK;
	backCullRasterizerDesc.FrontCounterClockwise = false;
	backCullRasterizerDesc.DepthBias = 0;
	backCullRasterizerDesc.SlopeScaledDepthBias = 0.0f;
	backCullRasterizerDesc.DepthBiasClamp = 0.0f;
	backCullRasterizerDesc.DepthClipEnable = true;
	backCullRasterizerDesc.ScissorEnable = false;
	backCullRasterizerDesc.MultisampleEnable = false;
	backCullRasterizerDesc.AntialiasedLineEnable = false;

	ErrorCheck(Framework::GetDevice()->CreateRasterizerState(&wireframeRasterizerDesc, &m_Data->rasterizerStates[(i32)RasterizerStates::Wireframe]));
	ErrorCheck(Framework::GetDevice()->CreateRasterizerState(&nocullRasterizerDesc, &m_Data->rasterizerStates[(i32)RasterizerStates::NoCull]));
	ErrorCheck(Framework::GetDevice()->CreateRasterizerState(&frontRasterizerDesc, &m_Data->rasterizerStates[(i32)RasterizerStates::FrontFace]));
	ErrorCheck(Framework::GetDevice()->CreateRasterizerState(&backCullRasterizerDesc, &m_Data->rasterizerStates[(i32)RasterizerStates::BackCull]));

	m_Data->rasterizerStates[(i32)RasterizerStates::Default] = nullptr;

	return true;
}