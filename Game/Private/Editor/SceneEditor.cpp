#include "Editor/SceneEditor.h"

#include "Input/Input.h"
#include "Object/CameraComponent.h"
#include "Object/MeshComponent.h"
#include "Object/Object.h"
#include "Object/SpatialObject.h"
#include "Physics/IPhysical.h"
#include "Rendering/DebugDraw.h"
#include "Rendering/Mesh.h"
#include "Space/Space.h"

#include <RenderImGui/imgui/imgui.h>
#include <RenderImGui/ImGuizmo/ImGuizmo.h>
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

	const float4 SelectionColor = float4{ 1.0f, 0.6f, 0.1f, 1.0f };

	static_assert(sizeof(matrix) == sizeof(float) * 16, "ImGuizmo reads matrices as 16 contiguous floats");

	ImGuizmo::OPERATION ToImGuizmoOperation(GizmoOperation_e Operation)
	{
		switch (Operation)
		{
		case GizmoOperation_e::Translate: return ImGuizmo::TRANSLATE;
		case GizmoOperation_e::Rotate: return ImGuizmo::ROTATE;
		case GizmoOperation_e::Scale: return ImGuizmo::SCALE;
		}
		return ImGuizmo::TRANSLATE;
	}

	// Corners are transformed individually, so the box stays exact under any parent scale or shear
	void DrawMeshBounds(SpatialRenderingCollector_s& Collector, const MeshComponent_c& Component)
	{
		const Mesh_s* Mesh = Component.GetMesh().get();
		if (!Mesh || Mesh->Bounds.mins.x > Mesh->Bounds.maxs.x)
			return;

		const matrix& World = Component.GetWorldMatrix();
		const float3& Mins = Mesh->Bounds.mins;
		const float3& Maxs = Mesh->Bounds.maxs;

		float3 Corners[8];
		for (uint32_t Index = 0; Index < 8; ++Index)
		{
			const float3 Local = { (Index & 1) ? Maxs.x : Mins.x, (Index & 2) ? Maxs.y : Mins.y, (Index & 4) ? Maxs.z : Mins.z };
			Corners[Index] = TransformF3(Local, World);
		}

		// Each edge joins two corners whose indices differ in one bit
		for (uint32_t Index = 0; Index < 8; ++Index)
		{
			for (uint32_t Bit = 1; Bit < 8; Bit <<= 1)
			{
				if (!(Index & Bit))
				{
					DebugLine(Collector, Corners[Index], Corners[Index | Bit], SelectionColor, DebugDepthMode_e::OnTop);
				}
			}
		}
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

	// Only while a panel is open, so play without the editor keeps the left mouse button
	const bool PanelOpen = ShowOutliner || ShowInspector;
	if (PanelOpen)
	{
		// First, so a click that starts a gizmo drag is not also taken as a pick
		DrawGizmo(*Space);

		const bool OverGizmo = ImGuizmo::IsOver() || ImGuizmo::IsUsing();
		if (!OverGizmo && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::GetIO().WantCaptureMouse && !Input::IsMouseCaptured())
		{
			PickAtCursor(*Space);
		}
	}

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

void SceneEditor_c::Render(SpatialRenderingCollector_s& Collector)
{
	SpatialObject_c* Spatial = dynamic_cast<SpatialObject_c*>(GetSelection());
	if (!Spatial)
		return;

	DebugAxes(Collector, Spatial->GetWorldMatrix(), 1.0f, DebugDepthMode_e::OnTop);

	for (const MeshComponent_c* Mesh : Spatial->GetComponents<MeshComponent_c>())
	{
		DrawMeshBounds(Collector, *Mesh);
	}
}

void SceneEditor_c::PickAtCursor(Space_c& Space)
{
	const CameraComponent_c* Camera = Space.GetCamera();
	if (!Camera)
		return;

	float3 Start, End;
	Camera->CalculateRayForScreenPosition(Input::GetMousePosition(), Start, End);

	IntersectionCtx_s Trace = IntersectionCtx_s::CreateLineSegmentTrace(Start, End, true);
	Space.Trace(Trace);

	// Only mesh components register as physical, so the hit's owner is the object to select
	const MeshComponent_c* Hit = dynamic_cast<const MeshComponent_c*>(Trace.GetClosest());
	SelectAndInspect(Hit ? Hit->GetOwner() : nullptr);
}

void SceneEditor_c::DrawGizmo(Space_c& Space)
{
	SpatialObject_c* Spatial = dynamic_cast<SpatialObject_c*>(GetSelection());
	const CameraComponent_c* Camera = Space.GetCamera();
	if (!Spatial || !Camera)
		return;

	const ImGuiIO& IO = ImGui::GetIO();
	if (!IO.WantCaptureKeyboard && !Input::IsMouseCaptured())
	{
		if (ImGui::IsKeyPressed(ImGuiKey_W, false))
		{
			GizmoOperation = GizmoOperation_e::Translate;
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_E, false))
		{
			GizmoOperation = GizmoOperation_e::Rotate;
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_R, false))
		{
			GizmoOperation = GizmoOperation_e::Scale;
		}
	}

	// The same matrices as the picking ray, so the handles line up with what the renderer draws
	const matrix View = Camera->CalculateViewMatrix();
	const matrix Projection = Camera->CalculateProjectionMatrix();
	matrix World = Spatial->GetWorldMatrix();

	const float TranslateSnap = 0.5f;
	const float RotateSnapDegrees = 15.0f;
	const float ScaleSnap = 0.1f;
	const float SnapValue = GizmoOperation == GizmoOperation_e::Translate ? TranslateSnap : GizmoOperation == GizmoOperation_e::Rotate ? RotateSnapDegrees : ScaleSnap;
	const float Snap[3] = { SnapValue, SnapValue, SnapValue };

	ImGuizmo::SetRect(0.0f, 0.0f, IO.DisplaySize.x, IO.DisplaySize.y);
	if (ImGuizmo::Manipulate(&View.m[0][0], &Projection.m[0][0], ToImGuizmoOperation(GizmoOperation), GizmoLocal ? ImGuizmo::LOCAL : ImGuizmo::WORLD, &World.m[0][0], nullptr, GizmoSnap ? Snap : nullptr))
	{
		const SpatialObject_c* Parent = Spatial->GetParent();
		const matrix Local = Parent ? World * InverseMatrix(Parent->GetWorldMatrix()) : World;

		float3 Position, Scale;
		quat Rotation;
		if (DecomposeMatrix(Local, Position, Rotation, Scale))
		{
			Spatial->SetPosition(Position);
			Spatial->SetRotation(Rotation);
			Spatial->SetScale(Scale);
		}
	}

	// The gizmo is not an ImGui item, so claim the mouse for it or the game would also see the drag
	if (ImGuizmo::IsOver() || ImGuizmo::IsUsing())
	{
		ImGui::SetNextFrameWantCaptureMouse(true);
	}
}

void SceneEditor_c::DrawGizmoToolbar()
{
	if (ImGui::RadioButton("Move (W)", GizmoOperation == GizmoOperation_e::Translate))
	{
		GizmoOperation = GizmoOperation_e::Translate;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("Rotate (E)", GizmoOperation == GizmoOperation_e::Rotate))
	{
		GizmoOperation = GizmoOperation_e::Rotate;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("Scale (R)", GizmoOperation == GizmoOperation_e::Scale))
	{
		GizmoOperation = GizmoOperation_e::Scale;
	}

	ImGui::Checkbox("Local", &GizmoLocal);
	ImGui::SameLine();
	ImGui::Checkbox("Snap", &GizmoSnap);
	ImGui::Separator();
}

void SceneEditor_c::SelectAndInspect(Object_c* Object)
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
							SelectAndInspect(Object);
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
		SelectAndInspect(&Object);
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
			if (dynamic_cast<SpatialObject_c*>(Object))
			{
				DrawGizmoToolbar();
			}

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
