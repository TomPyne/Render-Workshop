function(create_proj projname rapi)

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

	add_library(${projname}${rapi} ${source_list})

	foreach(source IN LISTS source_list)
    		get_filename_component(source_path "${source}" PATH)
    		string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    		source_group("${source_path_msvc}" FILES "${source}")
	endforeach()

	target_link_libraries(${projname}${rapi} Render${rapi})

	target_include_directories(${projname}${rapi}
		PRIVATE
		"${PROJECT_SOURCE_DIR}"
		"${PROJECT_SOURCE_DIR}/Render"
		"Source/Public"
		"imgui"
	)

	set_target_properties(${projname}${rapi} PROPERTIES 
		OUTPUT_NAME_DEBUG "${projname}${rapi}_Debug"
		VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
	)
endfunction()

create_proj(RenderImGui Dx11)
create_proj(RenderImGui Dx12)