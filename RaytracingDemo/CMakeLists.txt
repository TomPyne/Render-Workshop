set(
	shader_list
	"Shaders/ClearUAV.hlsl"
	"Shaders/DebugView.hlsl"
	"Shaders/Deferred.hlsl"
	"Shaders/DeferredCommon.h"
	"Shaders/ExposureFusion.hlsl"
	"Shaders/Mesh.hlsl"
	"Shaders/RenderToTexture.hlsl"
	"Shaders/RTShadows.hlsl"
	"Shaders/ScreenPassShared.h"
	"Shaders/ScreenPassVS.hlsl"
	"Shaders/ShadowDenoiser.hlsl"
	"Shaders/ShadowTemporalRecombine.hlsl"
	"Shaders/Tonemapping.hlsl"
)

foreach(source IN LISTS shader_list)
	get_filename_component(source_path "${source}" PATH)
	string(REPLACE "/" "\\" source_path_msvc "${source_path}")
	source_group("${source_path_msvc}" FILES "${source}")
endforeach()

# Loaded and compiled at runtime - keep them as HLSL items (so VS keeps IntelliSense/colorization)
# but excluded from the actual VS build step.
set_source_files_properties(${shader_list} PROPERTIES VS_SETTINGS "ExcludedFromBuild=true")

add_executable(RaytracingDemo ${CMAKE_CURRENT_SOURCE_DIR}/Source/RaytracingDemo.cpp)

target_link_libraries(RaytracingDemo HalfPipe)
target_link_libraries(RaytracingDemo Render)
target_link_libraries(RaytracingDemo RenderImGui)
target_link_libraries(RaytracingDemo RenderUtils)
target_link_libraries(RaytracingDemo Shared)

set_render_api_exe_name(RaytracingDemo)

set_target_properties(RaytracingDemo
					PROPERTIES
					VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

target_include_directories(RaytracingDemo
PRIVATE
"${PROJECT_SOURCE_DIR}"
"${PROJECT_SOURCE_DIR}/Include"
"${PROJECT_SOURCE_DIR}/Render"
"${PROJECT_SOURCE_DIR}/RenderImgui/imgui"
"${PROJECT_SOURCE_DIR}/Shared"
"${PROJECT_SOURCE_DIR}/HalfPipe/Source/Public"
)

target_sources(RaytracingDemo
PRIVATE
"${CMAKE_CURRENT_SOURCE_DIR}/Source/RaytracingDemoApp.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/RaytracingDemoApp.h"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/RTDModel.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/RTDModel.h"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/RTDGlobals.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/RTDGlobals.h"
)

target_sources(RaytracingDemo
PRIVATE
${shader_list}
)
