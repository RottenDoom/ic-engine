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

add_library(engine-vulkan OBJECT ${VULKAN_SRC})

set_target_properties(engine-vulkan PROPERTIES POSITION_INDEPENDENT_CODE ON)

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


# Ensure engine-vulkan static objects are compiled with export macro so
# their symbols are available in the `engine` shared library.
target_compile_definitions(engine-vulkan PRIVATE IC_EXPORT)
