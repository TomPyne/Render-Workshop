set(
	source_list
	"imgui/imconfig.h"
	"imgui/imgui.cpp"
	"imgui/imgui.h"
	"imgui/imgui_demo.cpp"
	"imgui/imgui_draw.cpp"
	"imgui/imgui_internal.h"
	"imgui/imgui_tables.cpp"
	"imgui/imgui_widgets.cpp"
	"imgui/backends/imgui_impl_win32.cpp"
	"imgui/backends/imgui_impl_win32.h"
	"Source/Public/imgui_impl_render.h"
	"Source/Private/imgui_impl_render.cpp"
)

add_library(RenderImGui ${source_list})

foreach(source IN LISTS source_list)
    	get_filename_component(source_path "${source}" PATH)
    	string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    	source_group("${source_path_msvc}" FILES "${source}")
endforeach()

target_link_libraries(RenderImGui Render)

target_include_directories(RenderImGui
	PRIVATE
	"${PROJECT_SOURCE_DIR}"
	"${PROJECT_SOURCE_DIR}/Render"
	"Source/Public"
	"imgui"
)

set_target_properties(RenderImGui PROPERTIES
	OUTPUT_NAME_DEBUG "RenderImGui_Debug"
	VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)
