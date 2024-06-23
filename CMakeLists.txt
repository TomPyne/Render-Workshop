cmake_minimum_required(VERSION 3.10)

project(RenderDemos VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/lib)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/lib)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/bin)

add_subdirectory(Render)

add_executable(RenderExampleDx11 RenderExample/main.cpp)
add_executable(RenderExampleDx12 RenderExample/main.cpp)

target_link_libraries(RenderExampleDx11 RenderDx11)
target_link_libraries(RenderExampleDx12 RenderDx12)

set_target_properties(RenderExampleDx11
						PROPERTIES
						RUNTIME_OUTPUT_NAME_DEBUG "RenderExampleDx11_Debug"
						VS_DEBUGGER_WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

set_target_properties(RenderExampleDx12
						PROPERTIES
						RUNTIME_OUTPUT_NAME_DEBUG "RenderExampleDx12_Debug"
						VS_DEBUGGER_WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

target_include_directories(RenderExampleDx11 PRIVATE "Include")
target_include_directories(RenderExampleDx12 PRIVATE "Include")

target_include_directories(RenderExampleDx11 PRIVATE "Render")
target_include_directories(RenderExampleDx12 PRIVATE "Render")




