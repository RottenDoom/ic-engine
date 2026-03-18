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

# compiler selection
function(set_compiler_options TARGET)
    if(MSVC)
        target_compile_options(${TARGET} PRIVATE
            /W4             # High warning level
            /MP             # Multi-processor compilation
            /EHsc           # Standard C++ exception handling
            $<$<CONFIG:Debug>:/Od /Zi /RTC1>
            $<$<CONFIG:Release>:/O2 /GL>    # Whole-program optimization in release
        )
        target_link_options(${TARGET} PRIVATE
            $<$<CONFIG:Release>:/LTCG>      # Link-time code gen to match /GL
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${TARGET} PRIVATE
            -Wall -Wextra -Wpedantic
            -Wno-unused-parameter
            $<$<CONFIG:Debug>:-g -O0 -fno-omit-frame-pointer>
            $<$<CONFIG:Release>:-O3 -flto>
        )
        target_link_options(${TARGET} PRIVATE
            $<$<CONFIG:Release>:-flto>
        )
    else()
        # GCC / MinGW
        target_compile_options(${TARGET} PRIVATE
            -Wall -Wextra -Wpedantic
            -Wno-unused-parameter
            $<$<CONFIG:Debug>:-g -O0 -fno-omit-frame-pointer>
            $<$<CONFIG:Release>:-O2>
        )
    endif()
endfunction()

function(link_whole_archive TARGET STATIC_LIB)
    if(MSVC)
        # MSVC: /WHOLEARCHIVE:<lib>
        target_link_options(${TARGET} PRIVATE
            /WHOLEARCHIVE:$<TARGET_FILE:${STATIC_LIB}>
        )
        target_link_libraries(${TARGET} PRIVATE ${STATIC_LIB})
    elseif(APPLE)
        # macOS linker uses -force_load
        target_link_libraries(${TARGET} PRIVATE
            -Wl,-force_load,$<TARGET_FILE:${STATIC_LIB}>
        )
    else()
        # GCC / MinGW / Clang on Linux & Windows
        target_link_libraries(${TARGET} PRIVATE
            -Wl,--whole-archive
            ${STATIC_LIB}
            -Wl,--no-whole-archive
        )
    endif()
endfunction()

# ─────────────────────────────────────────────
#  STATIC CORE
# ─────────────────────────────────────────────
add_library(engine-core STATIC ${CORE_SRC} ${RENDERER_SRC})

target_include_directories(engine-core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/engine/include
        ${CMAKE_BINARY_DIR}/generated
)

target_compile_definitions(engine-core
    PRIVATE IC_BUILD_ENGINE
)

# Windows: tell MSVC/clang-cl to export all symbols automatically
# (equivalent of --whole-archive on the consumer side)
if(WIN32 AND MSVC)
    set_target_properties(engine-core PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
endif()

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

set_compiler_options(engine-core)
set_target_properties(engine-core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# build the lib
add_library(engine SHARED src/engine.cpp)

target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

link_whole_archive(engine engine-core)

target_link_libraries(engine
    PUBLIC
        glm
        entt
        spdlog::spdlog
        imgui
    PRIVATE
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
    PRIVATE IC_BUILD_ENGINE
)

set_compiler_options(engine)

set_target_properties(engine PROPERTIES
    OUTPUT_NAME                      "engine"
    POSITION_INDEPENDENT_CODE        ON
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
)

# ─────────────────────────────────────────────
#  INSTALL
# ─────────────────────────────────────────────
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