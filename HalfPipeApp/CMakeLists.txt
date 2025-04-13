add_executable(HalfPipeApp "${CMAKE_CURRENT_SOURCE_DIR}/Source/Private/HalfPipeApp.cpp")

target_link_libraries(HalfPipeApp HalfPipe)
target_link_libraries(HalfPipeApp Shared)

set_target_properties(HalfPipeApp
					PROPERTIES
					RUNTIME_OUTPUT_NAME_DEBUG HalfPipeApp_Debug
					VS_DEBUGGER_WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/HalfPipeApp
)

target_include_directories(HalfPipeApp
PRIVATE
"${PROJECT_SOURCE_DIR}/Include"
"${PROJECT_SOURCE_DIR}/HalfPipe/Source/Public"
"${PROJECT_SOURCE_DIR}/Render"
"${PROJECT_SOURCE_DIR}/Shared"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Public"
)
