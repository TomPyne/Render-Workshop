function(create_proj projname rapi)

	set(
		source_list
		"RenderExampleImgui.cpp"
	)

	add_executable(${projname}${rapi} ${source_list})

	target_link_libraries(${projname}${rapi} Render${rapi})
	target_link_libraries(${projname}${rapi} RenderImGui${rapi})

	set_target_properties(${projname}${rapi}
		PROPERTIES
		RUNTIME_OUTPUT_NAME_DEBUG "${projname}${rapi}_Debug"
		VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
	)

	target_include_directories(${projname}${rapi}
		PRIVATE 
		"${PROJECT_SOURCE_DIR}"
		"${PROJECT_SOURCE_DIR}/Include"
		"${PROJECT_SOURCE_DIR}/Render"
		"${PROJECT_SOURCE_DIR}/RenderImgui/imgui"
	)

endfunction()

create_proj(RenderExampleImGui Dx11)
create_proj(RenderExampleImGui Dx12)