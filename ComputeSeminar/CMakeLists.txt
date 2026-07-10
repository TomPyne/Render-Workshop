set(
	shader_list
	"Shaders/BallData.h"
	"Shaders/BallMovementCompute.hlsl"
	"Shaders/ClearIndirectDrawIndexed.hlsl"
	"Shaders/ImGui.hlsl"
	"Shaders/IndirectArguments.h"
	"Shaders/Mesh.hlsl"
	"Shaders/Samplers.h"
	"Shaders/Scene.h"
	"Shaders/ShapeMesh.hlsl"
	"Shaders/Terrain.hlsl"
	"Shaders/View.h"
)

foreach(source IN LISTS shader_list)
	get_filename_component(source_path "${source}" PATH)
	string(REPLACE "/" "\\" source_path_msvc "${source_path}")
	source_group("${source_path_msvc}" FILES "${source}")
endforeach()

# Loaded and compiled at runtime - keep them as HLSL items (so VS keeps IntelliSense/colorization)
# but excluded from the actual VS build step.
set_source_files_properties(${shader_list} PROPERTIES VS_SETTINGS "ExcludedFromBuild=true")

function(create_compute_seminar_proj rapi)
	add_executable(ComputeSeminar${rapi} ${PROJECT_SOURCE_DIR}/ComputeSeminar/ComputeSeminar.cpp)

	target_link_libraries(ComputeSeminar${rapi} Render${rapi})
	target_link_libraries(ComputeSeminar${rapi} RenderImGui${rapi})
	target_link_libraries(ComputeSeminar${rapi} Shared)

	set_target_properties(ComputeSeminar${rapi}
						PROPERTIES
						RUNTIME_OUTPUT_NAME_DEBUG "ComputeSeminar${rapi}_Debug"
						VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/ComputeSeminar"
	)

	target_include_directories(ComputeSeminar${rapi}
		PRIVATE
		"${PROJECT_SOURCE_DIR}"
		"${PROJECT_SOURCE_DIR}/Include"
		"${PROJECT_SOURCE_DIR}/Render"
		"${PROJECT_SOURCE_DIR}/RenderImgui/imgui"
		"${PROJECT_SOURCE_DIR}/Shared"
	)

	target_sources(ComputeSeminar${rapi}
		PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/ComputeSeminarApp.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/ComputeSeminarApp.h"
	)

	target_sources(ComputeSeminar${rapi}
		PRIVATE
		${shader_list}
	)
endfunction()

create_compute_seminar_proj(Dx11)
create_compute_seminar_proj(Dx12)
