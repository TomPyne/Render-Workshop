#pragma once

#include "Editor/ImGuiPropertyVisitor.h"
#include "Rendering/IRenderable.h"

#include <memory>

class Object_c;
class Space_c;

enum class GizmoOperation_e
{
	Translate,
	Rotate,
	Scale,
};

// Drawn as a space renderer overlay for the selection highlight
class SceneEditor_c : public IRenderable_c
{
public:
	// Call between ImGui::BeginMainMenuBar and ImGui::EndMainMenuBar
	void DrawMenu();

	void DrawWindows(Space_c* Space);

	// Null once the selected object is destroyed
	Object_c* GetSelection() const;
	void SetSelection(Object_c* Object);

	// Begin IRenderable_c interface
	virtual void Render(struct SpatialRenderingCollector_s& Collector) override;
	// End IRenderable_c interface

private:
	void DrawOutliner(Space_c& Space);
	void DrawOutlinerNode(Object_c& Object);
	void DrawInspector();

	// Picking in the outliner or the viewport also opens the inspector
	void SelectAndInspect(Object_c* Object);

	// Selects the mesh under the cursor, or clears the selection on a miss
	void PickAtCursor(Space_c& Space);

	void DrawGizmo(Space_c& Space);
	void DrawGizmoToolbar();

	std::weak_ptr<Object_c> Selection;

	RotationEditState_s RotationEditState;

	GizmoOperation_e GizmoOperation = GizmoOperation_e::Translate;
	bool GizmoLocal = false;
	bool GizmoSnap = false;

	char FilterText[128] = {};
	bool ShowOutliner = false;
	bool ShowInspector = false;
};
