#include "Editor/SceneEditor.h"

#include "Object/Object.h"
#include "Object/SpatialObject.h"
#include "Space/Space.h"

#include <RenderImGui/imgui/imgui.h>
#include <Shared/StringUtils/StringUtils.h>

#include <string>

namespace
{
	std::string GetDisplayName(const Object_c& Object)
	{
		const std::string ClassName = WideToNarrow(Object.ClassName());
		return Object.GetName().empty() ? ClassName : Object.GetName() + "  (" + ClassName + ")";
	}

	bool IsRoot(const Object_c& Object)
	{
		const SpatialObject_c* Spatial = dynamic_cast<const SpatialObject_c*>(&Object);
		return !Spatial || !Spatial->GetParent();
	}
}

void SceneEditor_c::DrawMenu()
{
	if (ImGui::BeginMenu("Editor"))
	{
		ImGui::MenuItem("Outliner", nullptr, &ShowOutliner);
		ImGui::MenuItem("Inspector", nullptr, &ShowInspector);
		ImGui::EndMenu();
	}
}

void SceneEditor_c::DrawWindows(Space_c* Space)
{
	if (!Space)
		return;

	if (ShowOutliner)
	{
		DrawOutliner(*Space);
	}

	if (ShowInspector)
	{
		DrawInspector();
	}
}

Object_c* SceneEditor_c::GetSelection() const
{
	// Levels also hold their objects, so the pointer can outlive the object's removal from the space
	Object_c* Object = Selection.lock().get();
	return Object && !Object->IsDestroyed() ? Object : nullptr;
}

void SceneEditor_c::SetSelection(Object_c* Object)
{
	Selection = Object ? Object->weak_from_this() : std::weak_ptr<Object_c>();
}

void SceneEditor_c::SelectFromOutliner(Object_c* Object)
{
	SetSelection(Object);
	if (Object)
	{
		ShowInspector = true;
	}
}

void SceneEditor_c::DrawOutliner(Space_c& Space)
{
	ImGui::SetNextWindowSize(ImVec2(360.0f, 480.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Outliner", &ShowOutliner))
	{
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputTextWithHint("##Filter", "Filter", FilterText, sizeof(FilterText));
		const ImGuiTextFilter Filter(FilterText);

		if (ImGui::BeginChild("##Objects"))
		{
			if (Filter.IsActive())
			{
				// Flat while filtering, so matches under a collapsed parent still show
				Space.ForEachObject([this, &Filter](Object_c* Object)
				{
					const std::string Label = GetDisplayName(*Object);
					if (Filter.PassFilter(Label.c_str()))
					{
						ImGui::PushID(Object);
						if (ImGui::Selectable(Label.c_str(), Object == GetSelection()))
						{
							SelectFromOutliner(Object);
						}
						ImGui::PopID();
					}
				});
			}
			else
			{
				Space.ForEachObject([this](Object_c* Object)
				{
					if (IsRoot(*Object))
					{
						DrawOutlinerNode(*Object);
					}
				});
			}

			if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
			{
				SetSelection(nullptr);
			}
		}
		ImGui::EndChild();
	}
	ImGui::End();
}

void SceneEditor_c::DrawOutlinerNode(Object_c& Object)
{
	const SpatialObject_c* Spatial = dynamic_cast<const SpatialObject_c*>(&Object);
	const bool HasChildren = Spatial && !Spatial->GetChildren().empty();

	ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (!HasChildren)
	{
		Flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}
	if (&Object == GetSelection())
	{
		Flags |= ImGuiTreeNodeFlags_Selected;
	}

	const bool Open = ImGui::TreeNodeEx(&Object, Flags, "%s", GetDisplayName(Object).c_str());
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		SelectFromOutliner(&Object);
	}

	if (Open && HasChildren)
	{
		for (const std::weak_ptr<SpatialObject_c>& Child : Spatial->GetChildren())
		{
			const std::shared_ptr<SpatialObject_c> ChildShared = Child.lock();
			if (ChildShared && !ChildShared->IsDestroyed())
			{
				DrawOutlinerNode(*ChildShared);
			}
		}
		ImGui::TreePop();
	}
}

void SceneEditor_c::DrawInspector()
{
	ImGui::SetNextWindowSize(ImVec2(420.0f, 560.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Inspector", &ShowInspector))
	{
		Object_c* Object = GetSelection();
		if (!Object)
		{
			ImGui::TextDisabled("Nothing selected");
		}
		else
		{
			ImGuiPropertyVisitor_c Visitor(RotationEditState);
			ImGui::PushItemWidth(-160.0f);

			ImGui::PushID(Object);
			if (ImGui::CollapsingHeader(WideToNarrow(Object->ClassName()).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				Object->VisitProperties(Visitor);
			}

			// Copied, so a component edit that changes the list cannot invalidate the loop
			const std::vector<std::shared_ptr<ObjectComponent_c>> Components = Object->Components;
			for (const std::shared_ptr<ObjectComponent_c>& Component : Components)
			{
				ImGui::PushID(Component.get());
				if (ImGui::CollapsingHeader(WideToNarrow(Component->ClassName()).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
				{
					Component->VisitProperties(Visitor);
				}
				ImGui::PopID();
			}
			ImGui::PopID();

			ImGui::PopItemWidth();
		}
	}
	ImGui::End();
}
