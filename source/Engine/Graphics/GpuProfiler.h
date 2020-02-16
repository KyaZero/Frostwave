#pragma once

struct ID3D11DeviceContext;
struct ID3D11Query;

#include <Engine/Core/Timer.h>
#include <unordered_map>
#include <string>
#include <vector>

namespace frostwave
{
	class GPUProfiler
	{
	public:
		GPUProfiler();

		bool Init();
		void Shutdown();

		void BeginFrame();
		void Timestamp(const std::string& gts);
		void EndFrame();

		// Wait on GPU for last frame's data (not this frame's) to be available
		void WaitForDataAndUpdate();
		void DrawDebug();

	protected:
		i32 m_FrameQuery;
		i32 m_FrameCollect;
		ID3D11Query* m_QueryDisjoint[2];

		struct TimeStampInfo
		{
			ID3D11Query* query[2];
			std::string id;
			f32 dt;
			f32 average;
			f32 totalAverage;
		};
		std::unordered_map<std::string, size_t> m_Indices;
		std::vector<TimeStampInfo> m_TimeStamps;

		i32 m_frameCountAvg;
		f32 m_tBeginAvg;

		Timer m_Timer;
	};
}

namespace fw = frostwave;