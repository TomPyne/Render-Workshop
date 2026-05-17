add_library(Game
        "Core/WindowsPlatform.cpp"
		"Core/WindowsPlatform.h"
        "Core/GameApp.cpp"
		"Core/GameApp.h"
		"Entity/Entity.cpp"
		"Entity/Entity.h"
		"Entity/EntityFragments.cpp"
		"Entity/EntityFragments.h"
		"Entity/TransformFragment.cpp"
		"Entity/TransformFragment.h"
)

set_target_properties(Game PROPERTIES OUTPUT_NAME_DEBUG Game_Debug)

target_include_directories(Game
	PRIVATE 
    	"${PROJECT_SOURCE_DIR}/imgui"
		"${PROJECT_SOURCE_DIR}/Include"
    	"${PROJECT_SOURCE_DIR}/Render"
    	"${PROJECT_SOURCE_DIR}"
	)