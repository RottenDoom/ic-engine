#engine/src/renderer/vulkan/CMakeLists.txt
set(VULKAN_SRC
    buffer.cpp
    context.cpp
    device.cpp
    extensions.cpp
    framebuffer.cpp
    physical_device.cpp
    pipeline_config.cpp
    renderpass.cpp
    shader.cpp
    surface.cpp
    swapchain.cpp
    sync_objects.cpp
    vulkan_renderer.cpp
    model.cpp
    texture.cpp
)

add_library(engine-vulkan STATIC ${VULKAN_SRC})

add_library(engine-vulkan STATIC ${VULKAN_SRC})

target_link_libraries(engine-vulkan
    PUBLIC engine-renderer
    PRIVATE renderer_dependencies
            Vulkan::Vulkan
)

target_compile_definitions(engine-vulkan PRIVATE IC_EXPORT)
set_target_properties(engine-vulkan PROPERTIES POSITION_INDEPENDENT_CODE ON)