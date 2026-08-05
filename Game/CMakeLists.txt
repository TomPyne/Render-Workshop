function(create_proj projname rapi)

	set(
		source_list
		"Public/Assets/MaterialManager.h"
		"Private/Assets/MaterialManager.cpp"		
		"Private/Assets/MeshAsset.cpp"
		"Public/Assets/MeshAsset.h"
		"Private/Assets/MeshManager.cpp"
		"Public/Assets/MeshManager.h"
		"Private/Core/GameApp.cpp"
		"Public/Core/GameApp.h"
		"Private/Core/WindowsPlatform.cpp"
		"Public/Core/WindowsPlatform.h"
		"Private/Entity/Entity.cpp"
		"Public/Entity/Entity.h"
		"Private/Entity/EntityFragments.cpp"
		"Public/Entity/EntityFragments.h"
		"Private/Entity/Fragments/CameraFragment.cpp"
		"Public/Entity/Fragments/CameraFragment.h"
		"Private/Entity/Fragments/DirectionalLightFragment.cpp"
		"Public/Entity/Fragments/DirectionalLightFragment.h"
		"Private/Entity/Fragments/TransformFragment.cpp"
		"Public/Entity/Fragments/TransformFragment.h"
		"Public/Input/Input.h"
		"Private/Input/Input.cpp"
		"Private/Level/Level.cpp"
		"Public/Level/Level.h"
		"Private/Object/CameraComponent.cpp"
		"Public/Object/CameraComponent.h"
		"Private/Object/FlyControllerComponent.cpp"
		"Public/Object/FlyControllerComponent.h"
		"Public/Object/MeshComponent.h"
		"Private/Object/MeshComponent.cpp"
		"Private/Object/Object.cpp"
		"Public/Object/Object.h"
		"Private/Object/ObjectComponent.cpp"
		"Public/Object/ObjectComponent.h"
		"Private/Object/RuntimeMeshComponent.cpp"
		"Public/Object/RuntimeMeshComponent.h"
		"Private/Object/SpatialObject.cpp"
		"Public/Object/SpatialObject.h"
		"Private/Object/SpatialObjectComponent.cpp"
		"Public/Object/SpatialObjectComponent.h"
		"Public/Rendering/IRenderable.h"
		"Private/Rendering/GameRenderer.cpp"
		"Public/Rendering/GameRenderer.h"
		"Private/Rendering/Materials.cpp"
		"Public/Rendering/Materials.h"
		"Private/Rendering/Mesh.cpp"
		"Public/Rendering/Mesh.h"
		"Private/Rendering/SpaceRenderer.cpp"
		"Public/Rendering/SpaceRenderer.h"
		"Private/Space/Space.cpp"
		"Public/Space/Space.h"
		"Public/Utility/Transform.h"
	)

	set(
		shader_list
		"Shaders/BasicMaterial.hlsl"
		"Shaders/ShaderDefines.h"
	)

	foreach(source IN LISTS source_list shader_list)
		get_filename_component(source_path "${source}" PATH)
		string(REPLACE "/" "\\" source_path_msvc "${source_path}")
		source_group("${source_path_msvc}" FILES "${source}")
	endforeach()

	# Loaded and compiled at runtime (see rl::CreateXShader calls) - keep them as HLSL items
	# (so VS keeps IntelliSense/colorization) but excluded from the actual VS build step.
	set_source_files_properties(${shader_list} PROPERTIES VS_SETTINGS "ExcludedFromBuild=true")

	add_library(${projname}${rapi} ${source_list} ${shader_list})

	target_link_libraries(${projname}${rapi} PUBLIC RenderImGui${rapi})
	target_link_libraries(${projname}${rapi} PRIVATE Shared)
	target_link_libraries(${projname}${rapi} PRIVATE HalfPipe)

	set_target_properties(${projname}${rapi} PROPERTIES OUTPUT_NAME_DEBUG "${projname}${rapi}_Debug")

	target_include_directories(${projname}${rapi}
		PUBLIC
    		"${CMAKE_CURRENT_SOURCE_DIR}/Public"
		PRIVATE
    		"${PROJECT_SOURCE_DIR}/imgui"
			"${PROJECT_SOURCE_DIR}/Include"
    		"${PROJECT_SOURCE_DIR}/Render"
    		"${PROJECT_SOURCE_DIR}"
		)
endfunction()

create_proj(Game Dx11)
create_proj(Game Dx12)