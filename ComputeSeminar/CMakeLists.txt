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
endfunction()

create_compute_seminar_proj(Dx11)
create_compute_seminar_proj(Dx12)
