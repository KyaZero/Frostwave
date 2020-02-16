#include "Sampler.h"
#include <Engine/Memory/Allocator.h>
#include <Engine/Graphics/Framework.h>
#include <Engine/Graphics/Error.h>
#include <d3d11.h>

struct frostwave::Sampler::Data
{
	ID3D11SamplerState* sampler;
};

frostwave::Sampler::Sampler()
{
}

frostwave::Sampler::Sampler(Filter filter, Address address, Vec4f border)
{
	Init(filter, address, border);
}

frostwave::Sampler::~Sampler()
{
	SafeRelease(&m_Data->sampler);
	Free(m_Data);
}

void frostwave::Sampler::Init(Filter filter, Address address, Vec4f border)
{
	m_Data = Allocate();

	D3D11_SAMPLER_DESC desc;
	switch (filter)
	{
	case frostwave::Sampler::Filter::Linear:
		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		break;
	case frostwave::Sampler::Filter::Point:
		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		break;
	}

	switch (address)
	{
	case frostwave::Sampler::Address::Clamp:
		desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		break;
	case frostwave::Sampler::Address::Wrap:
		desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		break;
	case frostwave::Sampler::Address::Mirror:
		desc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
		break;
	case frostwave::Sampler::Address::Border:
		desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
		break;
	}

	desc.MipLODBias = 0.0f;
	desc.MaxAnisotropy = 2;
	desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	desc.BorderColor[0] = border.x;
	desc.BorderColor[1] = border.y;
	desc.BorderColor[2] = border.z;
	desc.BorderColor[3] = border.w;
	desc.MinLOD = 0;
	desc.MaxLOD = D3D11_FLOAT32_MAX;

	ErrorCheck(Framework::GetDevice()->CreateSamplerState(&desc, &m_Data->sampler));
}

void frostwave::Sampler::Bind(u32 slot)
{
	Framework::GetContext()->PSSetSamplers(slot, 1, &m_Data->sampler);
}

void frostwave::Sampler::Unbind(u32 slot)
{
	ID3D11SamplerState* sampler = nullptr;
	Framework::GetContext()->PSSetSamplers(slot, 1, &sampler);
}