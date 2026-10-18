
set(
	source_list
	"Source/SunTemple.cpp"
	"Source/SunTempleApp.cpp"
	"Source/SunTempleApp.h"
	"Source/Components/MoverComponent.cpp"
	"Source/Components/MoverComponent.h"
	"Source/Components/RotatorComponent.cpp"
	"Source/Components/RotatorComponent.h"
	"Source/Materials/SunTempleMaterials.h"
	"Source/Materials/SunTempleMaterials.cpp"
)

add_executable(SunTemple ${source_list})

foreach(source IN LISTS source_list)
	get_filename_component(source_path "${source}" PATH)
	string(REPLACE "/" "\\" source_path_msvc "${source_path}")
	source_group("${source_path_msvc}" FILES "${source}")
endforeach()

target_link_libraries(SunTemple Game)
target_link_libraries(SunTemple HalfPipe)
target_link_libraries(SunTemple Render)
target_link_libraries(SunTemple RenderUtils)
target_link_libraries(SunTemple Shared)

set_render_api_exe_name(SunTemple)

set_target_properties(SunTemple
					PROPERTIES
					VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

target_include_directories(SunTemple
PRIVATE
"${PROJECT_SOURCE_DIR}"
"${PROJECT_SOURCE_DIR}/Include"	
"${PROJECT_SOURCE_DIR}/Render"
"${PROJECT_SOURCE_DIR}/RenderImGui"
"${PROJECT_SOURCE_DIR}/Shared"
"${PROJECT_SOURCE_DIR}/SunTemple/Source"
"${PROJECT_SOURCE_DIR}/HalfPipe/Source/Public"
)
