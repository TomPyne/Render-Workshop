set(
	source_list
    "Assets/Assets.cpp"
    "Assets/Assets.h"
    "Camera/Camera.cpp"
    "Camera/Camera.h"
    "Camera/FlyCamera.cpp"
    "Camera/FlyCamera.h"
    "FileUtils/FileLoader.cpp"
    "FileUtils/FileLoader.h"
    "FileUtils/FileStream.cpp"
    "FileUtils/FileStream.h"
    "FileUtils/PathUtils.cpp"
    "FileUtils/PathUtils.h"
    "Logging/Logging.cpp"
    "Logging/Logging.h"
    "Materials/Materials.cpp"
    "Materials/Materials.h"
    "ModelUtils/ModelLoader.cpp"
    "ModelUtils/Model.cpp"
    "ModelUtils/Model.h"
    "ModelUtils/PlaneBuilder.cpp"
    "ModelUtils/PlaneBuilder.h"
    "ModelUtils/SphereBuilder.cpp"
    "ModelUtils/SphereBuilder.h"
    "Noise/Perlin.cpp"
    "Noise/Perlin.h"
    "Profiling/ScopeTimer.cpp"
    "Profiling/ScopeTimer.h"
    "StringUtils/StringUtils.cpp"
    "StringUtils/StringUtils.h"
    "TextureUtils/DDSTextureLoader.cpp"
    "TextureUtils/DDSTextureLoader.h"
    "TextureUtils/TextureManager.cpp"
    "TextureUtils/TextureManager.h"
)

set(
	shader_list
    "Shaders/ImGui/ImGui.hlsl"
)

foreach(source IN LISTS source_list shader_list)
	get_filename_component(source_path "${source}" PATH)
	string(REPLACE "/" "\\" source_path_msvc "${source_path}")
	source_group("${source_path_msvc}" FILES "${source}")
endforeach()

# Loaded and compiled at runtime - keep them as HLSL items (so VS keeps IntelliSense/colorization)
# but excluded from the actual VS build step.
set_source_files_properties(${shader_list} PROPERTIES VS_SETTINGS "ExcludedFromBuild=true")

add_library(Shared ${source_list} ${shader_list})

set_target_properties(Shared PROPERTIES OUTPUT_NAME_DEBUG Shared_Debug)

target_include_directories(Shared
	PRIVATE 
    "${PROJECT_SOURCE_DIR}/RenderImGui/imgui"
	"${PROJECT_SOURCE_DIR}/Include"
    "${PROJECT_SOURCE_DIR}/Render"
    "${PROJECT_SOURCE_DIR}/Shared"
	)