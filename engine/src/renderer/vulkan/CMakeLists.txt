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

target_include_directories(engine-vulkan
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        $ENV{VULKAN_SDK}/Include
)

target_link_libraries(engine-vulkan
    PRIVATE
	dependencies
	renderer_dependencies
        Vulkan::Vulkan
        ktx
)
