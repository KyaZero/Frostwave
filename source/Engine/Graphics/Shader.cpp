#include "Shader.h"
#include <Engine/Logging/Logger.h>
#include <Engine/Graphics/Framework.h>
#include <Engine/Graphics/Error.h>
#include <Engine/Memory/Allocator.h>
#include <fstream>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <unordered_map>
#include <Windows.h>
#include <Engine/FileWatcher.h>
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"dxguid.lib")

struct frostwave::Shader::Data
{
	std::string sourcePS, sourceVS;
	ID3D11PixelShader* pixel = nullptr;
	ID3D11VertexShader* vertex = nullptr;
	ID3D11InputLayout* layout = nullptr;
	u32 type;

	D3D_SHADER_MACRO defines[16] = { };
};

bool CompileShader(const std::string& source, const std::string& entry, const std::string& profile, D3D_SHADER_MACRO* macros, ID3DBlob** blob)
{
	*blob = nullptr;

	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	flags |= D3DCOMPILE_DEBUG;
#endif

	std::ifstream in(source);
	std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	HRESULT hr = D3DCompile(contents.c_str(), contents.size(), source.c_str(), macros, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entry.c_str(), profile.c_str(), flags, 0, &shaderBlob, &errorBlob);

	if (FAILED(hr))
	{
		if ((u32)hr & ERROR_FILE_NOT_FOUND)
		{
			WARNING_LOG("File '%s' not found", source.c_str());
		}

		if (errorBlob)
		{
			ERROR_LOG("Shader failed to compile: %s", (char*)errorBlob->GetBufferPointer());
			errorBlob->Release();
		}

		if (shaderBlob)
			shaderBlob->Release();

		return false;
	}

	*blob = shaderBlob;

	return true;
}

bool CreateInputLayout(const std::string& source, char* bytes, u32 size, ID3D11InputLayout** layout)
{
	HRESULT result = S_OK;

	ID3D11ShaderReflection* vsReflection = NULL;
	result = D3DReflect(bytes, size, IID_ID3D11ShaderReflection, (void**)&vsReflection);
	if (FAILED(result))
	{
		ERROR_LOG("Failed to reflect shader '%s'", source.c_str());
		return false;
	}

	D3D11_SHADER_DESC shaderDesc;
	vsReflection->GetDesc(&shaderDesc);

	std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayoutDesc;
	for (u32 i = 0; i < shaderDesc.InputParameters; i++)
	{
		D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
		vsReflection->GetInputParameterDesc(i, &paramDesc);

		D3D11_INPUT_ELEMENT_DESC elementDesc;
		elementDesc.SemanticName = paramDesc.SemanticName;
		elementDesc.SemanticIndex = paramDesc.SemanticIndex;
		elementDesc.InputSlot = 0;
		elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		elementDesc.InstanceDataStepRate = 0;

		if (paramDesc.Mask == 1)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) 
				elementDesc.Format = DXGI_FORMAT_R32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) 
				elementDesc.Format = DXGI_FORMAT_R32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) 
				elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
		}
		else if (paramDesc.Mask <= 3)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) 
				elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) 
				elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) 
				elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
		}
		else if (paramDesc.Mask <= 7)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) 
				elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) 
				elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) 
				elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		}
		else if (paramDesc.Mask <= 15)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) 
				elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) 
				elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		}

		inputLayoutDesc.push_back(elementDesc);
	}

	result = fw::Framework::GetDevice()->CreateInputLayout(&inputLayoutDesc[0], (UINT)inputLayoutDesc.size(), bytes, size, layout);
	if (FAILED(result))
	{
		ERROR_LOG("Failed to create input layout for shader '%s'", source.c_str());
		return false;
	}

	vsReflection->Release();

	return true;
}

frostwave::Shader::Shader() : m_Data(nullptr)
{
}

frostwave::Shader::Shader(u32 type, const std::string& source, const std::string& sourceVS) : m_Data(nullptr)
{
	Load(type, source, sourceVS);
}

frostwave::Shader::~Shader()
{
	if (!m_Data) return;
	FileWatcher::Get()->Unregister(this, m_Data->sourcePS);
	FileWatcher::Get()->Unregister(this, m_Data->sourceVS);
	if (m_Data->type & Type::Vertex)
	{
		if (m_Data->vertex)
			SafeRelease(&m_Data->vertex);
		if (m_Data->layout)
			SafeRelease(&m_Data->layout);
	}
	if (m_Data->type & Type::Pixel)
	{
		if (m_Data->pixel)
			SafeRelease(&m_Data->pixel);
	}
	Free(m_Data);
}

void frostwave::Shader::Load(u32 type, const std::string& pixel, const std::string& vertex)
{
	if(!m_Data)
		m_Data = Allocate();

	m_Data->type = type;
	m_Data->sourcePS = pixel;
	m_Data->sourceVS = vertex.length() > 0 ? vertex : pixel;

	auto compileVertex = [&]()
	{
		if (m_Data->type & Type::Vertex)
		{
			ID3DBlob* vs = nullptr;
			if (CompileShader(m_Data->sourceVS, "VSMain", "vs_5_0", m_Data->defines, &vs))
			{
				if (m_Data->vertex)
					SafeRelease(&m_Data->vertex);
				if(m_Data->layout)
					SafeRelease(&m_Data->layout);
				ErrorCheck(Framework::GetDevice()->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &m_Data->vertex));
				CreateInputLayout(m_Data->sourceVS, (char*)vs->GetBufferPointer(), (u32)vs->GetBufferSize(), &m_Data->layout);
				VERBOSE_LOG("Compiled Vertex Shader '%s'", m_Data->sourceVS.c_str());
			}
			if (vs) vs->Release();
		}
	};

	auto compilePixel = [&]() 
	{
		if (m_Data->type & Type::Pixel)
		{
			ID3DBlob* ps = nullptr;
			if (CompileShader(m_Data->sourcePS, "PSMain", "ps_5_0", m_Data->defines, &ps))
			{
				if(m_Data->pixel)
					SafeRelease(&m_Data->pixel);

				ErrorCheck(Framework::GetDevice()->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &m_Data->pixel));
				VERBOSE_LOG("Compiled Pixel Shader '%s'", m_Data->sourcePS.c_str());
			}
			if (ps) ps->Release();
		}
	};

	compileVertex();
	compilePixel();

	FileWatcher::Get()->Register(this, m_Data->sourcePS, [=](const std::string&) {
		compilePixel();
	});

	FileWatcher::Get()->Register(this, m_Data->sourceVS, [=](const std::string&) {
		compileVertex();
	});
}

void frostwave::Shader::Bind(u32 mask) const
{
	if (m_Data->type & Type::Vertex && !(mask & Type::Vertex))
	{
		Framework::GetContext()->IASetInputLayout(m_Data->layout);
		Framework::GetContext()->VSSetShader(m_Data->vertex, nullptr, 0);
	}
	if (m_Data->type & Type::Pixel && !(mask & Type::Pixel))
	{
		Framework::GetContext()->PSSetShader(m_Data->pixel, nullptr, 0);
	}
}

void frostwave::Shader::AddDefine(const std::string& name, const std::string& value)
{
	name; value;
	for (size_t i = 0; i < 16; i++)
	{
		if (m_Data->defines[i].Name || m_Data->defines[i].Definition)
			continue;

		m_Data->defines[i].Name = name.c_str();
		m_Data->defines[i].Definition = value.c_str();
		break;
	}
	Load(m_Data->type, m_Data->sourcePS, m_Data->sourceVS);
}

bool frostwave::Shader::IsInitialized()
{
	return m_Data != nullptr;
}
