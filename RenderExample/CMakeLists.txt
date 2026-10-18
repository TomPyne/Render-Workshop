add_executable(RenderExample ${PROJECT_SOURCE_DIR}/RenderExample/RenderExample.cpp)

target_link_libraries(RenderExample Render)

set_render_api_exe_name(RenderExample)

set_target_properties(RenderExample
						PROPERTIES
						VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/RenderExample"
)

target_include_directories(RenderExample PRIVATE "${PROJECT_SOURCE_DIR}/Include")
target_include_directories(RenderExample PRIVATE "${PROJECT_SOURCE_DIR}/Render")

set(
	shader_list
	"Shaders/Mesh.hlsl"
)

foreach(source IN LISTS shader_list)
	get_filename_component(source_path "${source}" PATH)
	string(REPLACE "/" "\\" source_path_msvc "${source_path}")
	source_group("${source_path_msvc}" FILES "${source}")
endforeach()

# Loaded and compiled at runtime - keep them as HLSL items (so VS keeps IntelliSense/colorization)
# but excluded from the actual VS build step.
set_source_files_properties(${shader_list} PROPERTIES VS_SETTINGS "ExcludedFromBuild=true")

target_sources(RenderExample PRIVATE ${shader_list})
