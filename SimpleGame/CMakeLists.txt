function(create_proj projname rapi)

	set(
		source_list
		"Source/${projname}.cpp"
		"Source/SimpleGameApp.cpp"
		"Source/SimpleGameApp.h"
		"Source/Levels/SimpleLevel.cpp"
		"Source/Levels/SimpleLevel.h"
	)

	add_executable(${projname}${rapi} ${source_list})

	foreach(source IN LISTS source_list)
		get_filename_component(source_path "${source}" PATH)
		string(REPLACE "/" "\\" source_path_msvc "${source_path}")
		source_group("${source_path_msvc}" FILES "${source}")
	endforeach()

	target_link_libraries(${projname}${rapi} Game${rapi})
	target_link_libraries(${projname}${rapi} HalfPipe)
	target_link_libraries(${projname}${rapi} Render${rapi})
	target_link_libraries(${projname}${rapi} Shared)

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
	"${PROJECT_SOURCE_DIR}/RenderImGui"
	"${PROJECT_SOURCE_DIR}/Shared"
	"${PROJECT_SOURCE_DIR}/HalfPipe/Source/Public"
	)

endfunction()

create_proj(SimpleGame Dx12)
