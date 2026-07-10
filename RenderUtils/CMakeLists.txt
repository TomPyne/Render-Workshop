set(
	source_list
    "GPUContext/GPUContext.cpp"
    "GPUContext/GPUContext.h"
    "RenderGraph/RenderGraph.cpp"
    "RenderGraph/RenderGraph.h"
    "RenderPasses/BloomRenderPass.cpp"
    "RenderPasses/BloomRenderPass.h"
    "RenderPasses/DisocclusionRenderPass.cpp"
    "RenderPasses/DisocclusionRenderPass.h"
    "RenderPasses/ScreenTracedAmbientOcclusion.cpp"
    "RenderPasses/ScreenTracedAmbientOcclusion.h"
    "RenderPasses/ScreenTracedReflections.cpp"
    "RenderPasses/ScreenTracedReflections.h"
    "RenderPasses/SkyRenderPass.cpp"
    "RenderPasses/SkyRenderPass.h"
)

set(
	shader_list
    "Shaders/Atmosphere/SkySphere.hlsl"
    "Shaders/Bloom/BloomApply.hlsl"
    "Shaders/Bloom/BloomDownsample.hlsl"
    "Shaders/Bloom/BloomUpsample.hlsl"
    "Shaders/Denoising/GenerateDisocclusion.hlsl"
    "Shaders/STAO/ScreenSpaceTracing.hlsl"
    "Shaders/STReflections/STReflectionCombine.hlsl"
    "Shaders/STReflections/STReflections.hlsl"
    "Shaders/ScreenTracing/ScreenTracing.h"
    "Shaders/Util/Macros.h"
    "Shaders/Util/Random.h"
    "Shaders/Util/Transforms.h"
)

foreach(source IN LISTS source_list shader_list)
	get_filename_component(source_path "${source}" PATH)
	string(REPLACE "/" "\\" source_path_msvc "${source_path}")
	source_group("${source_path_msvc}" FILES "${source}")
endforeach()

# Loaded and compiled at runtime (see rl::CreateXShader calls in RenderPasses) - keep them as
# HLSL items (so VS keeps IntelliSense/colorization) but excluded from the actual VS build step.
set_source_files_properties(${shader_list} PROPERTIES VS_SETTINGS "ExcludedFromBuild=true")

add_library(RenderUtils ${source_list} ${shader_list})

set_target_properties(RenderUtils PROPERTIES OUTPUT_NAME_DEBUG RenderUtils_Debug)

target_include_directories(RenderUtils
	PRIVATE
    "${PROJECT_SOURCE_DIR}/RenderImGui/imgui"
	"${PROJECT_SOURCE_DIR}/Include"
    "${PROJECT_SOURCE_DIR}/Render"
    "${PROJECT_SOURCE_DIR}/Shared"
    "${PROJECT_SOURCE_DIR}"
	)
