#pragma once

#include <memory>

class Object_c;
class Space_c;

class SceneEditor_c
{
public:
	// Call between ImGui::BeginMainMenuBar and ImGui::EndMainMenuBar
	void DrawMenu();

	void DrawWindows(Space_c* Space);

	// Null once the selected object is destroyed
	Object_c* GetSelection() const;
	void SetSelection(Object_c* Object);

private:
	void DrawOutliner(Space_c& Space);
	void DrawOutlinerNode(Object_c& Object);

	std::weak_ptr<Object_c> Selection;

	char FilterText[128] = {};
	bool ShowOutliner = false;
};
