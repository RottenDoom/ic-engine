#engine/src/renderer/CMakeLists.txt
set(RENDERER_SRC
	camera.cpp
	backend_opengl.cpp
)


add_library(engine-renderer OBJECT ${RENDERER_SRC})

target_include_directories(engine-renderer
    PUBLIC ${CMAKE_SOURCE_DIR}/engine/include
)

# The renderer depends on Vulkan backend internally
target_link_libraries(engine-renderer
PUBLIC
	engine-core
    	dependencies
	spdlog::spdlog
	glm
PRIVATE
	glad
)

set_target_properties(engine-renderer PROPERTIES POSITION_INDEPENDENT_CODE ON)
