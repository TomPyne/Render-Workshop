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

function(create_proj projname rapi)

	set(
		source_list
		"RenderExampleImgui.cpp"
	)

	add_executable(${projname}${rapi} ${source_list} ${shader_list})

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