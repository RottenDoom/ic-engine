#engine/src/renderer/CMakeLists.txt
set(RENDERER_SRC
    camera.cpp
    renderer.cpp
)

add_library(engine-renderer STATIC ${RENDERER_SRC})

target_include_directories(engine-renderer
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# The renderer depends on Vulkan backend internally
target_link_libraries(engine-renderer
    PRIVATE
	Vulkan::Vulkan
	renderer_dependencies
	engine-vulkan
    	dependencies
)
