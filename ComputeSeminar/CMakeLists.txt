function(create_compute_seminar_proj rapi)
	add_executable(ComputeSeminar${rapi} ${PROJECT_SOURCE_DIR}/ComputeSeminar/ComputeSeminar.cpp)

	target_link_libraries(ComputeSeminar${rapi} Render${rapi})

	set_target_properties(ComputeSeminar${rapi}
						PROPERTIES
						RUNTIME_OUTPUT_NAME_DEBUG "ComputeSeminar${rapi}_Debug"
						VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/ComputeSeminar"
	)

	target_include_directories(ComputeSeminar${rapi}
	PRIVATE 
	"${PROJECT_SOURCE_DIR}/Include"
	"${PROJECT_SOURCE_DIR}/Render"
	"${PROJECT_SOURCE_DIR}/imgui"
	)

	target_sources(ComputeSeminar${rapi}
	PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/ComputeSeminarApp.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/ComputeSeminarApp.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/camera/camera.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/camera/camera.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/camera/flycamera.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/camera/flycamera.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_render.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_render.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/Noise/Perlin.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Noise/Perlin.h"
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

create_compute_seminar_proj(Dx11)
create_compute_seminar_proj(Dx12)
