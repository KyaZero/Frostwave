#include "DebugVisualizer.h"

#ifdef _DEBUG
#include <Engine/Memory/Allocator.h>
#include <Engine/Graphics/imgui/imgui.h>

void frostwave::DebugVisualizer::Draw()
{
	//ImGui::Begin("Debug");

	//ImGui::Text("Allocator Memory Usage");
	//auto memory = Allocator::Get()->GetStats();
	//ImGui::ProgressBar((float)memory.current / (float)memory.max);
	//ImGui::Text("%.2fMB/%.2fMB", memory.current.AsMegabytes(), memory.max.AsMegabytes());

	//ImGui::End();
}
#endif