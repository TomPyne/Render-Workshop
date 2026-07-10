function(create_proj projname rapi)

	set(
		source_list
		"Assets/MeshAsset.cpp"
		"Assets/MeshAsset.h"
		"Core/GameApp.cpp"
		"Core/GameApp.h"
		"Core/WindowsPlatform.cpp"
		"Core/WindowsPlatform.h"
		"Entity/Entity.cpp"
		"Entity/Entity.h"
		"Entity/EntityFragments.cpp"
		"Entity/EntityFragments.h"
		"Entity/Fragments/CameraFragment.cpp"
		"Entity/Fragments/CameraFragment.h"
		"Entity/Fragments/DirectionalLightFragment.cpp"
		"Entity/Fragments/DirectionalLightFragment.h"
		"Entity/Fragments/TransformFragment.cpp"
		"Entity/Fragments/TransformFragment.h"
		"Input/Input.h"
		"Input/Input.cpp"
		"Level/Level.cpp"
		"Level/Level.h"
		"Object/CameraComponent.cpp"
		"Object/CameraComponent.h"
		"Object/FlyControllerComponent.cpp"
		"Object/FlyControllerComponent.h"
		"Object/MeshComponent.h"
		"Object/MeshComponent.cpp"
		"Object/Object.cpp"
		"Object/Object.h"
		"Object/ObjectComponent.cpp"
		"Object/ObjectComponent.h"
		"Object/RuntimeMeshComponent.cpp"
		"Object/RuntimeMeshComponent.h"
		"Object/SpatialObject.cpp"
		"Object/SpatialObject.h"
		"Object/SpatialObjectComponent.cpp"
		"Object/SpatialObjectComponent.h"
		"Rendering/IRenderable.h"
		"Rendering/GameRenderer.cpp"
		"Rendering/GameRenderer.h"
		"Rendering/Materials.cpp"
		"Rendering/Materials.h"
		"Rendering/SpaceRenderer.cpp"
		"Rendering/SpaceRenderer.h"
		"Space/Space.cpp"
		"Space/Space.h"
		"Utility/Transform.h"
	)

	add_library(${projname}${rapi} ${source_list})

	foreach(source IN LISTS source_list)
		get_filename_component(source_path "${source}" PATH)
		string(REPLACE "/" "\\" source_path_msvc "${source_path}")
		source_group("${source_path_msvc}" FILES "${source}")
	endforeach()

	target_link_libraries(${projname}${rapi} PUBLIC RenderImGui${rapi})

	set_target_properties(${projname}${rapi} PROPERTIES OUTPUT_NAME_DEBUG "${projname}${rapi}_Debug")

	target_include_directories(${projname}${rapi}
		PRIVATE 
    		"${PROJECT_SOURCE_DIR}/imgui"
			"${PROJECT_SOURCE_DIR}/Include"
    		"${PROJECT_SOURCE_DIR}/Render"
    		"${PROJECT_SOURCE_DIR}"
		)
endfunction()

create_proj(Game Dx11)
create_proj(Game Dx12)