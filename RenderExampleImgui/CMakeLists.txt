set(
	shader_list
	"Shaders/ImGui.hlsl"
)

foreach(source IN LISTS shader_list)
	get_filename_component(source_path "${source}" PATH)
	string(REPLACE "/" "\\" source_path_msvc "${source_path}")
	source_group("${source_path_msvc}" FILES "${source}")
endforeach()

# Loaded and compiled at runtime - keep them as HLSL items (so VS keeps IntelliSense/colorization)
# but excluded from the actual VS build step.
set_source_files_properties(${shader_list} PROPERTIES VS_SETTINGS "ExcludedFromBuild=true")

set(
	source_list
	"RenderExampleImgui.cpp"
)

add_executable(RenderExampleImGui ${source_list} ${shader_list})

target_link_libraries(RenderExampleImGui Render)
target_link_libraries(RenderExampleImGui RenderImGui)

set_render_api_exe_name(RenderExampleImGui)

set_target_properties(RenderExampleImGui
	PROPERTIES
	VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

target_include_directories(RenderExampleImGui
	PRIVATE
	"${PROJECT_SOURCE_DIR}"
	"${PROJECT_SOURCE_DIR}/Include"
	"${PROJECT_SOURCE_DIR}/Render"
	"${PROJECT_SOURCE_DIR}/RenderImgui/imgui"
)
