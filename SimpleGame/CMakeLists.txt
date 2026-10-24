
set(
	source_list
	"Source/SimpleGame.cpp"
	"Source/SimpleGameApp.cpp"
	"Source/SimpleGameApp.h"
	"Source/Components/AIGymFlowComponent.h"
	"Source/Components/CircleMoverComponent.cpp"
	"Source/Components/CircleMoverComponent.h"
	"Source/Components/DeathVolumeComponent.cpp"
	"Source/Components/DeathVolumeComponent.h"
	"Source/Components/GameFlowComponent.cpp"
	"Source/Components/GameFlowComponent.h"
	"Source/Components/HovercraftControllerComponent.cpp"
	"Source/Components/HovercraftControllerComponent.h"
	"Source/Components/HovercraftGymFlowComponent.cpp"
	"Source/Components/HovercraftGymFlowComponent.h"
	"Source/Components/LandingPadComponent.h"
	"Source/Components/NavObstacleComponent.h"
	"Source/Components/NestComponent.h"
	"Source/Materials/SimpleGameMaterials.cpp"
	"Source/Materials/SimpleGameMaterials.h"
	"Source/Modes/ColonyMode.cpp"
	"Source/Modes/ColonyMode.h"
	"Source/Modes/RoadMode.cpp"
	"Source/Modes/RoadMode.h"
	"Source/Objects/BuildingObject.cpp"
	"Source/Objects/BuildingObject.h"
	"Source/Objects/HovercraftObject.cpp"
	"Source/Objects/HovercraftObject.h"
	"Source/Objects/RoadObject.cpp"
	"Source/Objects/RoadObject.h"
	"Source/Objects/SelectionCursorObject.cpp"
	"Source/Objects/SelectionCursorObject.h"
	"Source/World/Ground.h"
)

add_executable(SimpleGame ${source_list})

foreach(source IN LISTS source_list)
	get_filename_component(source_path "${source}" PATH)
	string(REPLACE "/" "\\" source_path_msvc "${source_path}")
	source_group("${source_path_msvc}" FILES "${source}")
endforeach()

target_link_libraries(SimpleGame Game)
target_link_libraries(SimpleGame HalfPipe)
target_link_libraries(SimpleGame Render)
target_link_libraries(SimpleGame RenderUtils)
target_link_libraries(SimpleGame Shared)

set_render_api_exe_name(SimpleGame)

set_target_properties(SimpleGame
					PROPERTIES
					VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

target_include_directories(SimpleGame
PRIVATE
"${PROJECT_SOURCE_DIR}"
"${PROJECT_SOURCE_DIR}/Include"	
"${PROJECT_SOURCE_DIR}/Render"
"${PROJECT_SOURCE_DIR}/RenderImGui"
"${PROJECT_SOURCE_DIR}/Shared"
"${PROJECT_SOURCE_DIR}/SimpleGame/Source"
"${PROJECT_SOURCE_DIR}/HalfPipe/Source/Public"
)
