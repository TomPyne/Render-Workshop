cmake_minimum_required(VERSION 3.10)

project(RenderWorkshop VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if( CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "19.15" )
    add_compile_options( /JMC )
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/lib)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/lib)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/bin)

add_subdirectory(RenderExample)
add_subdirectory(RenderExampleImGui)
add_subdirectory(ComputeSeminar)
add_subdirectory(RayTracingDemo)
add_subdirectory(Render)
add_subdirectory(Shared)
add_subdirectory(HalfPipe)
add_subdirectory(HalfPipeApp)