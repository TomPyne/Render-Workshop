cmake_minimum_required(VERSION 3.21)

set(RENDER_API Dx12 CACHE STRING "Rendering backend")
set_property(CACHE RENDER_API PROPERTY STRINGS Dx11 Dx12)

project(RenderWorkshop${RENDER_API} VERSION 1.0)

message(STATUS "Render API: ${RENDER_API}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if( CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "19.15" )
    add_compile_options( /JMC )
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/lib/${RENDER_API})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/lib/${RENDER_API})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/lib/${RENDER_API})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/lib/${RENDER_API})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/bin/${RENDER_API})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/bin/${RENDER_API})

# Executables keep the API in their name, e.g. SunTempleDx12.exe / SunTempleDx12_Debug.exe
function(set_render_api_exe_name target)
	set_target_properties(${target}
		PROPERTIES
		RUNTIME_OUTPUT_NAME "${target}${RENDER_API}"
		RUNTIME_OUTPUT_NAME_DEBUG "${target}${RENDER_API}_Debug"
	)
endfunction()

add_subdirectory(RenderExample)
add_subdirectory(RenderExampleImGui)
add_subdirectory(ComputeSeminar)
add_subdirectory(Render)
add_subdirectory(RenderImGui)
add_subdirectory(Shared)
add_subdirectory(RenderUtils)
add_subdirectory(HalfPipe)
add_subdirectory(HalfPipeApp)
add_subdirectory(Game)

if(RENDER_API STREQUAL "Dx12")
	add_subdirectory(RayTracingDemo)
	add_subdirectory(SimpleGame)
	add_subdirectory(SunTemple)

	set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT SunTemple)
else()
	set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT RenderExampleImGui)
endif()
