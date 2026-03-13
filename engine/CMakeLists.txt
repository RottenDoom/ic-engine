# engine/CMakeLists.txt
set(CORE_SRC
    src/core/platform/mmapped.c
    src/core/platform/win32/input_win32.cpp
    src/core/platform/win32/window_win32.cpp
    src/core/platform/win32/filesystem_win32.cpp
    src/core/application.cpp
    src/core/filesystem.cpp
    src/core/uuid.cpp
    src/core/input.cpp
    src/core/logger.cpp
    src/core/allocator.cpp
    src/core/assets/types/model.cpp
    src/core/assets/types/material.cpp
    src/core/assets/asset_cache.cpp
    src/core/assets/asset_manager.cpp
    src/core/assets/asset_registry.cpp
    src/core/assets/asset_serializer.cpp
    src/core/assets/asset_loaders/gltf_loader.cpp
    # src/core/ecs/components.cpp
)

set(RENDERER_SRC
    src/renderer/camera.cpp
    src/renderer/renderer_frontend.cpp
)

set(OPENGL_SRC
    src/renderer/opengl/gl_material.cpp
    src/renderer/opengl/gl_model.cpp
    src/renderer/opengl/gl_shader.cpp
    src/renderer/opengl/opengl_renderer.cpp
)

set(VULKAN_SRC
    # src/renderer/vulkan/vk_renderer.cpp
)

# STATIC CORE
add_library(engine-core STATIC ${CORE_SRC} ${RENDERER_SRC})

target_include_directories(engine-core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/engine/include
        ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(engine-core
    PUBLIC
        glm
        entt
        spdlog::spdlog
        imgui
    PRIVATE
        glfw
        yaml-cpp
        fastgltf
        ktx
        stb
)

target_compile_definitions(engine-core
    PUBLIC  IC_ENGINE
    PRIVATE IC_EXPORT
)

set_target_properties(engine-core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# SHARED LIB (TODO: Make hot reload stuff.)
add_library(engine SHARED src/engine.cpp)

target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(engine
    PUBLIC
        glm
        entt
        spdlog::spdlog
        imgui
    PRIVATE
        engine-core
        glad
)

if(IC_ENGINE_USE_OPENGL)
    find_package(OpenGL REQUIRED)
    target_sources(engine PRIVATE ${OPENGL_SRC})
    target_link_libraries(engine PRIVATE OpenGL::GL glad ktx stb)
endif()

if(IC_ENGINE_USE_VULKAN)
    find_package(Vulkan REQUIRED)
    include(${CMAKE_SOURCE_DIR}/cmake/shaders.cmake)
    target_sources(engine PRIVATE ${VULKAN_SRC})
    target_link_libraries(engine PRIVATE Vulkan::Vulkan ktx)
    file(GLOB_RECURSE ENGINE_SHADERS
        "${CMAKE_SOURCE_DIR}/shaders/*.vert"
        "${CMAKE_SOURCE_DIR}/shaders/*.frag"
    )
    if(ENGINE_SHADERS)
        target_link_spv_shaders(engine ${ENGINE_SHADERS})
    else()
        message(WARNING "IC_ENGINE_USE_VULKAN=ON but no shaders found")
    endif()
endif()

target_compile_definitions(engine
    PUBLIC
        $<$<BOOL:${IC_ENGINE_USE_OPENGL}>:IC_USE_OPENGL>
        $<$<BOOL:${IC_ENGINE_USE_VULKAN}>:IC_USE_VULKAN>
    PRIVATE
        IC_EXPORT
)

set_target_properties(engine PROPERTIES
    OUTPUT_NAME                      "engine"
    POSITION_INDEPENDENT_CODE        ON
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
)

# INSTALL
install(TARGETS engine
    EXPORT   ic_engineTargets
    RUNTIME  DESTINATION bin
    LIBRARY  DESTINATION lib
    ARCHIVE  DESTINATION lib
    INCLUDES DESTINATION include
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include/ic_engine
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)