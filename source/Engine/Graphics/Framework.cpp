#include "Framework.h"
#include "Error.h"
#include <Engine/Core/Types.h>
#include <Engine/Memory/Allocator.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/imgui/imgui.h>
#include <Engine/Graphics/imgui/imgui_impl_dx11.h>
#include <Engine/Graphics/imgui/imgui_impl_win32.h>
#include <Engine/Graphics/imgui/imguizmo/ImGuizmo.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <atlbase.h>
#include <locale>
#include <codecvt>
#include <string>

void* fw::Framework::s_Annot = nullptr;
ID3D11Debug* fw::Framework::s_Debug = nullptr;
ID3D11Device* fw::Framework::s_Device = nullptr;
ID3D11DeviceContext* fw::Framework::s_Context = nullptr;
frostwave::GPUProfiler* fw::Framework::s_Profiler = nullptr;

struct frostwave::Framework::Data
{
	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* context = nullptr;
	IDXGISwapChain* swapchain = nullptr;
	std::vector<IDXGIAdapter*> adapters;
	Texture backBuffer;
};

frostwave::Framework::Framework() : m_Data(nullptr)
{
	m_Data = Allocate();
}

frostwave::Framework::~Framework()
{
	s_Profiler->Shutdown();
	Free(s_Profiler);

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	for (auto* adapter : m_Data->adapters)
	{
		adapter->Release();
	}

	SafeRelease(&m_Data->swapchain);
	m_Data->backBuffer.Release();

	//m_Data->device->QueryInterface(IID_PPV_ARGS(&s_Debug));
	m_Data->context->ClearState();
	m_Data->context->Flush();

	SafeRelease(&m_Data->device);
	SafeRelease(&m_Data->context);
	Free(m_Data);

}

std::vector<void*> frostwave::Framework::EnumerateAdapters()
{
	IDXGIAdapter* adapter;
	std::vector<void*> adapters;
	IDXGIFactory* factory = nullptr;

	if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)& factory)))
	{
		ERROR_LOG("Failed to gather Adapters");
		return adapters;
	}

	for (u32 i = 0; factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(adapter);
	}

	if (factory)
	{
		factory->Release();
	}

	return adapters;
}

void frostwave::Framework::Init()
{
	auto adaps = EnumerateAdapters();
	for (auto a : adaps)
		m_Data->adapters.push_back((IDXGIAdapter*)a);

	IDXGIAdapter* adapter = nullptr;
	for (auto a : m_Data->adapters)
	{
		DXGI_ADAPTER_DESC adapterDescription;
		a->GetDesc(&adapterDescription);

		INFO_LOG("Found adapter: %ls VRAM: %umb", adapterDescription.Description, adapterDescription.DedicatedVideoMemory / (1024 * 1024));

		if (adapter)
		{
			DXGI_ADAPTER_DESC retDescription;
			adapter->GetDesc(&retDescription);

			if (retDescription.DedicatedVideoMemory < adapterDescription.DedicatedVideoMemory)
			{
				adapter = a;
			}
		}
		else
		{
			adapter = a;
		}
	}

	if (!adapter)
	{
		FATAL_LOG("Failed to get graphics adapter!");
		return;
	}

	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_1
	};

	DXGI_SWAP_CHAIN_DESC swapchainDesc = { 0 };
	swapchainDesc.BufferCount = 1;
	swapchainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
	swapchainDesc.OutputWindow = (HWND)Window::Get()->GetHandle();
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.Windowed = true;

	ErrorCheck(D3D11CreateDeviceAndSwapChain(adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, creationFlags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &swapchainDesc, &m_Data->swapchain, &m_Data->device, NULL, &m_Data->context));

	s_Context = m_Data->context;
	s_Device = m_Data->device;

	ID3D11Texture2D* backBuffer;
	ErrorCheck(m_Data->swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer));

	m_Data->backBuffer.CreateFromTexture(backBuffer);
	m_Data->backBuffer.SetAsActiveTarget();

	D3D11_VIEWPORT view = { };
	view.TopLeftX = 0.0f;
	view.TopLeftY = 0.0f;
	view.Width = (f32)Window::Get()->GetWidth();
	view.Height = (f32)Window::Get()->GetHeight();
	view.MinDepth = 0.0f;
	view.MaxDepth = 1.0f;

	m_Data->context->RSSetViewports(1, &view);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;        // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

	SetImGuiStyle();

	ImGui_ImplWin32_Init(Window::Get()->GetHandle());
	ImGui_ImplDX11_Init(m_Data->device, m_Data->context);

	Window::Get()->Subscribe(WM_SIZE, [&](u64 wp, u32 lp) {
		wp, lp;
		ResizeBackbuffer();
	});

	s_Profiler = Allocate();
	s_Profiler->Init();

	VERBOSE_LOG("Finished Initializing Graphics Framework!");
}

void frostwave::Framework::BeginFrame(const Vec4f& clearColor)
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
	m_Data->backBuffer.Clear(clearColor);
	s_Profiler->BeginFrame();
}

void frostwave::Framework::SetImGuiStyle()
{
	ImGuiStyle* style = &ImGui::GetStyle();
	ImVec4* colors = style->Colors;

	colors[ImGuiCol_Text] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.280f, 0.280f, 0.280f, 0.000f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
	colors[ImGuiCol_Border] = ImVec4(0.266f, 0.266f, 0.266f, 1.000f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.280f, 0.280f, 0.280f, 1.000f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.277f, 0.277f, 0.277f, 1.000f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_CheckMark] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_Button] = ImVec4(1.000f, 1.000f, 1.000f, 0.000f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
	colors[ImGuiCol_ButtonActive] = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);
	colors[ImGuiCol_Header] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
	colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_Tab] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.352f, 0.352f, 0.352f, 1.000f);
	colors[ImGuiCol_TabActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
	colors[ImGuiCol_DockingPreview] = ImVec4(1.000f, 0.391f, 0.000f, 0.781f);
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.586f, 0.586f, 0.586f, 1.000f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_NavHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);

	style->ChildRounding = 4.0f;
	style->FrameBorderSize = 1.0f;
	style->FrameRounding = 2.0f;
	style->GrabMinSize = 7.0f;
	style->PopupRounding = 2.0f;
	style->ScrollbarRounding = 12.0f;
	style->ScrollbarSize = 13.0f;
	style->TabBorderSize = 1.0f;
	style->TabRounding = 0.0f;
	style->WindowRounding = 4.0f;

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style->WindowRounding = 0.0f;
		style->Colors[ImGuiCol_WindowBg].w = 1.0f;
	}
}

void frostwave::Framework::EndFrame()
{
	s_Profiler->WaitForDataAndUpdate();
	s_Profiler->DrawDebug();

	s_Profiler->EndFrame();

	BeginEvent("Render ImGui");
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
	EndEvent();
	m_Data->swapchain->Present(1, 0);
}

void frostwave::Framework::ResizeBackbuffer()
{
	if (Window::Get()->GetWidth() == 0 || Window::Get()->GetHeight() == 0) return;

	m_Data->context->OMSetRenderTargets(0, 0, 0);
	m_Data->backBuffer.Release();

	ErrorCheck(m_Data->swapchain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0));

	ID3D11Texture2D* buffer = nullptr;
	ErrorCheck(m_Data->swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&buffer));
	m_Data->backBuffer.CreateFromTexture(buffer);
}

frostwave::Texture* frostwave::Framework::GetBackbuffer() const
{
	return &m_Data->backBuffer;
}

ID3D11Device* frostwave::Framework::GetDevice()
{
	return s_Device;
}

ID3D11DeviceContext* frostwave::Framework::GetContext()
{
	return s_Context;
}

void frostwave::Framework::ReportLiveObjects()
{
	// dump output only if we actually grabbed a debug interface
	if (s_Debug != nullptr)
	{
		s_Debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
		SafeRelease(&s_Debug);
	}
}

void frostwave::Framework::BeginEvent(std::string name)
{
	if (!s_Annot)
	{
		ID3DUserDefinedAnnotation* annot = nullptr;
		GetContext()->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)&annot);
		s_Annot = annot;
	}
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wide = converter.from_bytes(name);
	((ID3DUserDefinedAnnotation*)s_Annot)->BeginEvent(wide.c_str());
}

void frostwave::Framework::EndEvent()
{
	if (!s_Annot) return;
	((ID3DUserDefinedAnnotation*)s_Annot)->EndEvent();
}

void frostwave::Framework::Timestamp(std::string name)
{
	s_Profiler->Timestamp(name);
}
