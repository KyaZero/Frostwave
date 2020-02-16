#include "Editor.h"
#include <Engine\Graphics\imgui\imgui.h>
#include <Engine/Platform/Window.h>
#include <Engine/Graphics/Scene.h>

frostwave::Editor::Editor()
{
}

frostwave::Editor::~Editor()
{
}

void frostwave::Editor::Update(Engine* engine, f32 dt, const Texture* renderedScene)
{
	dt;
	engine;

	ImGui::DockSpaceOverViewport();
	ImGui::ShowDemoWindow();

	{
		ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.f,0.f,0.f,1.f });
		ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, { 0.f,0.f,0.f,1.f });
		if (ImGui::Begin("Viewport"))
		{
			ImVec2 min = ImGui::GetWindowContentRegionMin();
			ImVec2 max = ImGui::GetWindowContentRegionMax();
			ImVec2 offset = min;
			ImVec2 regionSize = ImVec2(max.x - min.x, max.y - min.y);
			ImVec2 size = ImVec2((f32)renderedScene->GetSize().x, (f32)renderedScene->GetSize().y);

			f32 regionRatio = size.x / size.y;
			f32 imageRatio = (f32)renderedScene->GetSize().x / (f32)renderedScene->GetSize().y;

			if (regionRatio > imageRatio)
			{
				size.x *= regionSize.y / size.y;
				size.y = regionSize.y;
			}
			else
			{
				size.y *= regionSize.x / size.x;
				size.x = regionSize.x;
			}

			ImGui::SetCursorPosX((regionSize.x - size.x) * 0.5f + offset.x);
			ImGui::SetCursorPosY((regionSize.y - size.y) * 0.5f + offset.y);

			ImGui::Image(renderedScene->GetShaderResourceView(), size, { 0, 0 }, { 1, 1 });
		}
		ImGui::End();
		ImGui::PopStyleColor(2);
	}
}
