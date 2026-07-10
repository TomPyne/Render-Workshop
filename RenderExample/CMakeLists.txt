add_executable(RenderExampleDx11 ${PROJECT_SOURCE_DIR}/RenderExample/RenderExample.cpp)
add_executable(RenderExampleDx12 ${PROJECT_SOURCE_DIR}/RenderExample/RenderExample.cpp)

target_link_libraries(RenderExampleDx11 RenderDx11)
target_link_libraries(RenderExampleDx12 RenderDx12)

set_target_properties(RenderExampleDx11
						PROPERTIES
						RUNTIME_OUTPUT_NAME_DEBUG "RenderExampleDx11_Debug"
						VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/RenderExample"
)

set_target_properties(RenderExampleDx12
						PROPERTIES
						RUNTIME_OUTPUT_NAME_DEBUG "RenderExampleDx12_Debug"
						VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/RenderExample"
)

target_include_directories(RenderExampleDx11 PRIVATE "${PROJECT_SOURCE_DIR}/Include")
target_include_directories(RenderExampleDx12 PRIVATE "${PROJECT_SOURCE_DIR}/Include")

target_include_directories(RenderExampleDx11 PRIVATE "${PROJECT_SOURCE_DIR}/Render")
target_include_directories(RenderExampleDx12 PRIVATE "${PROJECT_SOURCE_DIR}/Render")

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

target_sources(RenderExampleDx11 PRIVATE ${shader_list})
target_sources(RenderExampleDx12 PRIVATE ${shader_list})
