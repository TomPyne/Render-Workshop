add_library(HalfPipe "${CMAKE_CURRENT_SOURCE_DIR}/Source/Private/HalfPipe.cpp")

target_link_libraries(HalfPipe Shared)

set_target_properties(HalfPipe
					PROPERTIES
					RUNTIME_OUTPUT_NAME_DEBUG HalfPipe_Debug
					VS_DEBUGGER_WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/HalfPipe
)

target_include_directories(HalfPipe
PRIVATE
"${PROJECT_SOURCE_DIR}/Include"
"${PROJECT_SOURCE_DIR}/Render"
"${PROJECT_SOURCE_DIR}/Shared"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Public"
)

target_sources(HalfPipe
PUBLIC
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Public/HalfPipe.h"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Public/HPModel.h"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Public/HPTexture.h"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Public/HPWfMtlLib.h"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Public/MeshProcessing.h"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Public/ModelPipe.h"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Public/WaveFrontReader.h"
)

target_sources(HalfPipe
PRIVATE
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Private/HPPipe.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Private/HPPipe.h"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Private/HPModel.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Private/HPModelPipe.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Private/HPModelPipe.h"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Private/HPTexture.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Private/HPTexturePipe.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Private/HPTexturePipe.h"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Private/HPWfMtlLib.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Private/HPWfMtlLibPipe.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Private/HPWfMtlLibPipe.h"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Private/HPWavefrontMaterialLibraryFactory.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Private/HPWavefrontMaterialLibraryFactory.h"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Private/MeshProcessing.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Private/ModelPipe.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Source/Private/WaveFrontReader.cpp"
)

