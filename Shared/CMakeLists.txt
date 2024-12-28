add_library(Shared
            "Camera/Camera.cpp"
            "Camera/Camera.h"
            "Camera/FlyCamera.cpp"
            "Camera/FlyCamera.h"
            "FileUtils/FileLoader.cpp"
            "FileUtils/FileLoader.h"
            "ImGui/Imgui_Impl_Render.cpp"
            "ImGui/Imgui_Impl_Render.h"
            "Logging/Logging.cpp"
            "Logging/Logging.h"
            "MeshProcessing/MeshProcessing.cpp"
            "MeshProcessing/MeshProcessing.h"
            "MeshProcessing/WaveFrontReader.cpp"
            "MeshProcessing/WaveFrontReader.h"
            "ModelUtils/ModelLoader.cpp"
            "ModelUtils/Model.cpp"
            "ModelUtils/Model.h"
            "Noise/Perlin.cpp"
            "Noise/Perlin.h"
            "Profiling/ScopeTimer.cpp"
            "Profiling/ScopeTimer.h"
            "TextureUtils/TextureManager.cpp"
            "TextureUtils/TextureManager.h"
)

set_target_properties(Shared PROPERTIES OUTPUT_NAME_DEBUG Shared_Debug)

target_include_directories(Shared
	PRIVATE 
    "${PROJECT_SOURCE_DIR}/imgui"
	"${PROJECT_SOURCE_DIR}/Include"
    "${PROJECT_SOURCE_DIR}/Render"
    "${PROJECT_SOURCE_DIR}/Shared"
	)