#engine/src/renderer/CMakeLists.txt
set(RENDERER_SRC
	camera.cpp
	renderer_frontend.cpp
)


add_library(engine-renderer OBJECT ${RENDERER_SRC})

target_include_directories(engine-renderer
    PUBLIC 
    ${CMAKE_SOURCE_DIR}/engine/include
)

target_link_libraries(engine-renderer
PUBLIC
	engine-core
    	dependencies
	spdlog::spdlog
	glm
	entt
	imgui
PRIVATE
	glad
)

set_target_properties(engine-renderer PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(engine-renderer PRIVATE IC_EXPORT)