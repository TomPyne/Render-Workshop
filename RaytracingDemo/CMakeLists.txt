function(create_proj projname rapi)
	add_executable(${projname}${rapi} ${CMAKE_CURRENT_SOURCE_DIR}/Source/${projname}.cpp)

	target_link_libraries(${projname}${rapi} Render${rapi})
	target_link_libraries(${projname}${rapi} Shared)

	set_target_properties(${projname}${rapi}
						PROPERTIES
						RUNTIME_OUTPUT_NAME_DEBUG "${projname}${rapi}_Debug"
						VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/${projname}"
	)

	target_include_directories(${projname}${rapi}
	PRIVATE 
	"${PROJECT_SOURCE_DIR}/imgui"
	"${PROJECT_SOURCE_DIR}/Include"	
	"${PROJECT_SOURCE_DIR}/Render"
	"${PROJECT_SOURCE_DIR}/Shared"
	)

	target_sources(${projname}${rapi}
	PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/Source/RaytracingDemoApp.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Source/RaytracingDemoApp.h"
	"${PROJECT_SOURCE_DIR}/imgui/imconfig.h"
	"${PROJECT_SOURCE_DIR}/imgui/imgui.cpp"
	"${PROJECT_SOURCE_DIR}/imgui/imgui.h"
	"${PROJECT_SOURCE_DIR}/imgui/imgui_demo.cpp"
	"${PROJECT_SOURCE_DIR}/imgui/imgui_draw.cpp"
	"${PROJECT_SOURCE_DIR}/imgui/imgui_internal.h"
	"${PROJECT_SOURCE_DIR}/imgui/imgui_tables.cpp"
	"${PROJECT_SOURCE_DIR}/imgui/imgui_widgets.cpp"
	"${PROJECT_SOURCE_DIR}/imgui/backends/imgui_impl_win32.cpp"
	"${PROJECT_SOURCE_DIR}/imgui/backends/imgui_impl_win32.h"
	)
endfunction()

create_proj(RaytracingDemo Dx12)
