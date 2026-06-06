# engine/CMakeLists.txt

set(CORE_SRC
    src/core/platform/mmapped.c
    src/core/platform/Win32/input_win32.cpp
    src/core/platform/Win32/window_win32.cpp
    src/core/platform/Win32/filesystem_win32.cpp

    src/core/application.cpp
    src/core/filesystem.cpp
    src/core/uuid.cpp
    src/core/input.cpp
    src/core/logger.cpp
    src/core/allocator.cpp

    src/core/assets/types/model.cpp
    src/core/assets/types/skybox.cpp
    src/core/assets/types/material.cpp
    src/core/assets/asset_cache.cpp
    src/core/assets/asset_manager.cpp
    src/core/assets/asset_registry.cpp
    src/core/assets/asset_serializer.cpp
    src/core/assets/asset_loaders/gltf_loader.cpp

    src/core/ecs/entity.cpp
)

set(RENDERER_SRC
    src/renderer/camera.cpp
    src/renderer/renderer_frontend.cpp
    src/renderer/scene.cpp
    src/renderer/lighting_system.cpp
    src/renderer/scene_serializer.cpp
)

set(OPENGL_SRC
	src/renderer/opengl/gl_material.cpp
	src/renderer/opengl/gl_model.cpp
	src/renderer/opengl/gl_skybox.cpp
	src/renderer/opengl/gl_shader.cpp
	src/renderer/opengl/gl_sampler.cpp
	src/renderer/opengl/light_ubo.cpp
	src/renderer/opengl/gl_framebuffer.cpp
	src/renderer/opengl/opengl_renderer.cpp
	src/renderer/opengl/render_command.cpp
	src/renderer/opengl/render_pass.cpp
	src/renderer/opengl/gpu_resource_cache.cpp
)

set(VULKAN_SRC
    # src/renderer/vulkan/vk_renderer.cpp
)

# ─────────────────────────────────────────────
#  Compiler options
# ─────────────────────────────────────────────
function(set_compiler_options TARGET)
    if(MSVC)
        target_compile_options(${TARGET} PRIVATE
            /W4 /MP /EHsc
            $<$<CONFIG:Debug>:/Od /Zi /RTC1>
            $<$<CONFIG:Release>:/O2 /GL>
        )
        target_link_options(${TARGET} PRIVATE
            $<$<CONFIG:Release>:/LTCG>
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${TARGET} PRIVATE
            -Wall -Wextra -Wpedantic -Wno-unused-parameter
            $<$<CONFIG:Debug>:-g -O0 -fno-omit-frame-pointer>
            $<$<CONFIG:Release>:-O3 -flto>
        )
        target_link_options(${TARGET} PRIVATE
            $<$<CONFIG:Release>:-flto>
        )
    else()
        # GCC / MinGW
        target_compile_options(${TARGET} PRIVATE
            -Wall -Wextra -Wpedantic -Wno-unused-parameter
            $<$<CONFIG:Debug>:-g -O0 -fno-omit-frame-pointer>
            $<$<CONFIG:Release>:-O2>
        )
    endif()
endfunction()

# ─────────────────────────────────────────────
#  ENGINE
# ─────────────────────────────────────────────
add_library(engine STATIC
    src/engine.cpp
    ${CORE_SRC}
    ${RENDERER_SRC}
)

target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/engine/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/generated>
        $<INSTALL_INTERFACE:include>
)

target_compile_definitions(engine
    PRIVATE
        IC_BUILD_ENGINE
    PUBLIC
        $<$<CONFIG:Debug>:DEBUG IC_ENABLE_ASSERTS>
        $<$<CONFIG:Release>:NDEBUG>
)

target_link_libraries(engine
    PUBLIC
        glm
        entt
        spdlog::spdlog
    PRIVATE
        glfw
        glad
        yaml-cpp
        fastgltf
        ktx
        stb
)

# OpenGL backend sources + libs
if(IC_ENGINE_USE_OPENGL)
    find_package(OpenGL REQUIRED)
    target_sources(engine PRIVATE ${OPENGL_SRC})
    target_link_libraries(engine PRIVATE OpenGL::GL)
    target_compile_definitions(engine PUBLIC IC_ENGINE_USE_OPENGL)
endif()

# Vulkan backend sources + libs
if(IC_ENGINE_USE_VULKAN)
    find_package(Vulkan REQUIRED)
    include(${CMAKE_SOURCE_DIR}/cmake/shaders.cmake)
    target_sources(engine PRIVATE ${VULKAN_SRC})
    target_link_libraries(engine PRIVATE Vulkan::Vulkan)
    target_compile_definitions(engine PUBLIC IC_ENGINE_USE_VULKAN)

    file(GLOB_RECURSE ENGINE_SHADERS
        "${CMAKE_SOURCE_DIR}/shaders/*.vert"
        "${CMAKE_SOURCE_DIR}/shaders/*.frag"
    )
    if(ENGINE_SHADERS)
        target_link_spv_shaders(engine ${ENGINE_SHADERS})
    else()
        message(WARNING "IC_ENGINE_USE_VULKAN=ON but no shaders found in shaders/")
    endif()
endif()

set_compiler_options(engine)

set_target_properties(engine PROPERTIES
    OUTPUT_NAME               "engine"
    POSITION_INDEPENDENT_CODE ON
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin/Debug
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
)

add_custom_command(TARGET engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/assets
    COMMENT "Copying engine assets..."
    VERBATIM
)

# ─────────────────────────────────────────────
#  INSTALL
# ─────────────────────────────────────────────
install(TARGETS engine
    EXPORT   ic_engineTargets
    ARCHIVE  DESTINATION lib
    INCLUDES DESTINATION include
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include/ic_engine
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)