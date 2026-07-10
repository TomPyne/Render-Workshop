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

function(create_proj projname rapi)
	add_executable(${projname}${rapi} ${CMAKE_CURRENT_SOURCE_DIR}/Source/${projname}.cpp)

	target_link_libraries(${projname}${rapi} HalfPipe)
	target_link_libraries(${projname}${rapi} Render${rapi})
	target_link_libraries(${projname}${rapi} RenderImGui${rapi})
	target_link_libraries(${projname}${rapi} RenderUtils)
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
	"${PROJECT_SOURCE_DIR}/RenderImgui/imgui"
	"${PROJECT_SOURCE_DIR}/Shared"
	"${PROJECT_SOURCE_DIR}/HalfPipe/Source/Public"
	)

	target_sources(${projname}${rapi}
	PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/Source/RaytracingDemoApp.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Source/RaytracingDemoApp.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/Source/RTDModel.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Source/RTDModel.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/Source/RTDGlobals.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Source/RTDGlobals.h"
	)

	target_sources(${projname}${rapi}
	PRIVATE
	${shader_list}
	)
endfunction()

create_proj(RaytracingDemo Dx12)
