#pragma once
#include <Engine/Platform/Window.h>
#include <Engine\Core\Math\Vec4.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/GpuProfiler.h>
#include <vector>

struct ID3D11Debug;
struct ID3D11Device;
struct ID3D11DeviceContext;

namespace frostwave
{
	class Framework
	{
	public:
		Framework();
		~Framework();

		std::vector<void*> EnumerateAdapters();
		void Init();
		void BeginFrame(const Vec4f& clearColor);
		void SetImGuiStyle();
		void EndFrame();
		void ResizeBackbuffer();

		Texture* GetBackbuffer() const;

		static ID3D11Device* GetDevice();
		static ID3D11DeviceContext* GetContext();
		static void ReportLiveObjects();

		static void BeginEvent(std::string name);
		static void EndEvent();
		static void Timestamp(std::string name);

	private:
		struct Data;
		Data* m_Data;

		static ID3D11Debug* s_Debug;
		static ID3D11Device* s_Device;
		static ID3D11DeviceContext* s_Context;
		static GPUProfiler* s_Profiler;

		//void ptr because renderdoc bug?
		static void* s_Annot;
	};
}
namespace fw = frostwave;