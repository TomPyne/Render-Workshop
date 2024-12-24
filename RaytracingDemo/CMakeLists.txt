function(create_proj rapi)
	add_executable(RaytracingDemo${rapi} ${CMAKE_CURRENT_SOURCE_DIR}/Source/RaytracingDemo.cpp)

	target_link_libraries(RaytracingDemo${rapi} Render${rapi})

	set_target_properties(RaytracingDemo${rapi}
						PROPERTIES
						RUNTIME_OUTPUT_NAME_DEBUG "RaytracingDemo${rapi}_Debug"
						VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/RaytracingDemo"
	)

	target_include_directories(RaytracingDemo${rapi}
	PRIVATE 
	"${PROJECT_SOURCE_DIR}/imgui"
	"${PROJECT_SOURCE_DIR}/Include"
	"${PROJECT_SOURCE_DIR}/Render"
	"${PROJECT_SOURCE_DIR}/Shared"
	)

	target_sources(RaytracingDemo${rapi}
	PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/Source/RaytracingDemoApp.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Source/RaytracingDemoApp.h"
	"${PROJECT_SOURCE_DIR}/Shared/camera/camera.cpp"
	"${PROJECT_SOURCE_DIR}/Shared/camera/camera.h"
	"${PROJECT_SOURCE_DIR}/Shared/camera/flycamera.cpp"
	"${PROJECT_SOURCE_DIR}/Shared/camera/flycamera.h"
	"${PROJECT_SOURCE_DIR}/Shared/FileUtils/FileLoader.cpp"
	"${PROJECT_SOURCE_DIR}/Shared/FileUtils/FileLoader.h"
	"${PROJECT_SOURCE_DIR}/Shared/imgui/imgui_impl_render.cpp"
	"${PROJECT_SOURCE_DIR}/Shared/imgui/imgui_impl_render.h"
	"${PROJECT_SOURCE_DIR}/Shared/ModelUtils/ModelLoader.cpp"
	"${PROJECT_SOURCE_DIR}/Shared/ModelUtils/ModelLoader.h"
	"${PROJECT_SOURCE_DIR}/Shared/Noise/Perlin.cpp"
	"${PROJECT_SOURCE_DIR}/Shared/Noise/Perlin.h"
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

create_proj(Dx12)
