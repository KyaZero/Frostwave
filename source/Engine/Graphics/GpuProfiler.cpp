#include "GpuProfiler.h"
#include <Engine/Logging/Logger.h>
#include <Engine/Graphics/Framework.h>
#include <Engine/Graphics/imgui/imgui.h>
#include <Engine/Graphics/Error.h>
#include <algorithm>
#include <d3d11.h>

frostwave::GPUProfiler::GPUProfiler()
	: m_FrameQuery(0),
	m_FrameCollect(-1),
	m_frameCountAvg(0),
	m_tBeginAvg(0.0f)
{
}

bool frostwave::GPUProfiler::Init()
{
	D3D11_QUERY_DESC queryDesc = { D3D11_QUERY_TIMESTAMP_DISJOINT, 0 };
	if (FAILED(Framework::GetDevice()->CreateQuery(&queryDesc, &m_QueryDisjoint[0])))
	{
		ERROR_LOG("Could not create timestamp disjoint query for frame 0!");
		return false;
	}

	if (FAILED(Framework::GetDevice()->CreateQuery(&queryDesc, &m_QueryDisjoint[1])))
	{
		ERROR_LOG("Could not create timestamp disjoint query for frame 1!");
		return false;
	}

	return true;
}

void frostwave::GPUProfiler::Shutdown()
{
	if (m_QueryDisjoint[0])
		SafeRelease(&m_QueryDisjoint[0]);

	if (m_QueryDisjoint[1])
		SafeRelease(&m_QueryDisjoint[1]);

	for (auto& ts : m_TimeStamps)
	{
		auto& query = ts.query;
		if (query[0])
			SafeRelease(&query[0]);
		if (query[1])
			SafeRelease(&query[1]);
	}
}

void frostwave::GPUProfiler::BeginFrame()
{
	Framework::GetContext()->Begin(m_QueryDisjoint[m_FrameQuery]);
	Timestamp("Begin");
}

void frostwave::GPUProfiler::Timestamp(const std::string& id)
{
	if(m_Indices.find(id) == m_Indices.end())
	{
		m_TimeStamps.push_back({});
		auto& info = m_TimeStamps.back();
		info.id = id;
		D3D11_QUERY_DESC queryDesc = { D3D11_QUERY_TIMESTAMP, 0 };
		Framework::GetDevice()->CreateQuery(&queryDesc, &info.query[0]);
		Framework::GetDevice()->CreateQuery(&queryDesc, &info.query[1]);
		m_Indices[id] = m_TimeStamps.size() - 1;
	}

	Framework::GetContext()->End(m_TimeStamps[m_Indices[id]].query[m_FrameQuery]);
}

void frostwave::GPUProfiler::EndFrame()
{
	Framework::GetContext()->End(m_QueryDisjoint[m_FrameQuery]);

	++m_FrameQuery &= 1;
}

void frostwave::GPUProfiler::WaitForDataAndUpdate()
{
	if (m_FrameCollect < 0)
	{
		// Haven't run enough frames yet to have data
		m_FrameCollect = 0;
		return;
	}

	// Wait for data
	while (Framework::GetContext()->GetData(m_QueryDisjoint[m_FrameCollect], NULL, 0, 0) == S_FALSE)
	{
		//Sleep(1);
	}

	int iFrame = m_FrameCollect;
	++m_FrameCollect &= 1;

	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT timestampDisjoint;
	if (Framework::GetContext()->GetData(m_QueryDisjoint[iFrame], &timestampDisjoint, sizeof(timestampDisjoint), 0) != S_OK)
	{
		INFO_LOG("Couldn't retrieve timestamp disjoint query data");
		return;
	}

	if (timestampDisjoint.Disjoint)
	{
		// Throw out this frame's data
		INFO_LOG("Timestamps disjoint");
		return;
	}

	UINT64 timestampPrev = 0;

	for (auto& ts : m_TimeStamps)
	{
		UINT64 timestamp;
		if (Framework::GetContext()->GetData(ts.query[iFrame], &timestamp, sizeof(UINT64), 0) != S_OK)
		{
			INFO_LOG("Couldn't retrieve timestamp query data for '%s'", ts.id.c_str());
			return;
		}

		if (timestampPrev == 0) 
			timestampPrev = timestamp;

		f32 diff = float(timestamp - timestampPrev);
		ts.dt = diff / float(timestampDisjoint.Frequency);
		timestampPrev = timestamp;

		ts.totalAverage += ts.dt;
	}

	++m_frameCountAvg;
	if (m_Timer.GetTotalTime() > m_tBeginAvg + 0.5f)
	{
		for (auto& ts : m_TimeStamps)
		{
			ts.average = ts.totalAverage / m_frameCountAvg;
			ts.totalAverage = 0.0f;
		}

		m_frameCountAvg = 0;
		m_tBeginAvg = m_Timer.GetTotalTime();
	}

	m_Timer.Update();
}

void frostwave::GPUProfiler::DrawDebug()
{
	ImGui::Begin("Profiling", 0, ImGuiWindowFlags_AlwaysAutoResize);
	float dTDrawTotal = 0.0f;
	for (auto& ts : m_TimeStamps)
	{
		if (ts.id == "Begin") continue;
		dTDrawTotal += ts.average;
		ImGui::Text("%s: %0.2f ms", ts.id.c_str(), 1000.0f * ts.average);
		//ImGui::Text("Shadows: %0.2f ms", 1000.0f * m_Profiler.DtAvg(GTS_Shadows));
		//ImGui::Text("Deferred: %0.2f ms", 1000.0f * m_Profiler.Dt(GTS_Deferred));
		//ImGui::Text("Deferred Lighting: %0.2f ms", 1000.0f * m_Profiler.DtAvg(GTS_DeferredLighting));
		//ImGui::Text("Forward: %0.2f ms", 1000.0f * m_Profiler.DtAvg(GTS_Forward));
		//ImGui::Text("PostProcessing: %0.2f ms", 1000.0f * m_Profiler.DtAvg(GTS_PostProcessing));
	}
	ImGui::Text("GPU frame time: %0.2f ms", 1000.0f * (dTDrawTotal));
	ImGui::End();
}
