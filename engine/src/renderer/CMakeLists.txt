#engine/src/renderer/CMakeLists.txt
set(RENDERER_SRC
    camera.cpp
    renderer.cpp
)

add_library(engine-renderer OBJECT ${RENDERER_SRC})

target_include_directories(engine-renderer
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
)
set_target_properties(engine-renderer PROPERTIES POSITION_INDEPENDENT_CODE ON)

# The renderer depends on Vulkan backend internally
target_link_libraries(engine-renderer
    PRIVATE
	renderer_dependencies
    	dependencies
)

# Same export propagation for renderer sub-target so its public classes
# are built with dllexport when the final `engine` shared lib is created.
target_compile_definitions(engine-renderer PRIVATE IC_EXPORT)
