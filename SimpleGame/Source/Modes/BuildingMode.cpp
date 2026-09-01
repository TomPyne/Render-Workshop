#include "BuildingMode.h"
#include "Objects/SelectionCursorObject.h"

#include <RenderImGui/imgui/imgui.h>

#include <Space/Space.h>

void BuildingMode_c::Enter()
{
	ColonyMode_c::Enter();

	if (Space)
	{
		SelectionCursor = Space->CreateObject<SelectionCursorObject_c>().get();
	}
}

void BuildingMode_c::UpdateMode(float Delta)
{
	ColonyMode_c::UpdateMode(Delta);

	// Should update cursor pos here so we can handle clicks in the world.
}

void BuildingMode_c::ImGuiUpdate()
{
	ColonyMode_c::ImGuiUpdate();

	if (ImGui::Begin("Building"))
	{
		ImVec2 button_sz(40, 40);

		ImGui::Text("Available:");
		ImGuiStyle& style = ImGui::GetStyle();
		int buttons_count = 8;
		float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
		for (int n = 0; n < buttons_count; n++)
		{
			ImGui::PushID(n);
			ImGui::Button("Box", button_sz);
			float last_button_x2 = ImGui::GetItemRectMax().x;
			float next_button_x2 = last_button_x2 + style.ItemSpacing.x + button_sz.x; // Expected position if next button was on same line
			if (n + 1 < buttons_count && next_button_x2 < window_visible_x2)
				ImGui::SameLine();
			ImGui::PopID();
		}
	}
	ImGui::End();
}

void BuildingMode_c::Exit()
{
	if (Space && SelectionCursor)
	{
		Space->DestroyObject(SelectionCursor);
		SelectionCursor = nullptr;
	}

	ColonyMode_c::Exit();
}
