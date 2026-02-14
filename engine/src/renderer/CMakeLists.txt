#engine/src/renderer/CMakeLists.txt
set(RENDERER_SRC
    camera.cpp
)

add_library(engine-renderer OBJECT ${RENDERER_SRC})

target_include_directories(engine-renderer
    PUBLIC ${CMAKE_SOURCE_DIR}/engine/include
)
set_target_properties(engine-renderer PROPERTIES POSITION_INDEPENDENT_CODE ON)

# The renderer depends on Vulkan backend internally
target_link_libraries(engine-renderer
    PRIVATE
    	dependencies
)
