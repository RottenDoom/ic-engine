#engine/src/core/CMakeLists.txt
set(CORE_SRC
    platform/mmapped.c
    platform/win32/input_win32.cpp
    platform/win32/window_win32.cpp
    platform/win32/filesystem_win32.cpp

    application.cpp
    filesystem.cpp
    input.cpp
    logger.cpp
    allocator.cpp
    assets/types/model.cpp
    assets/types/material.cpp
    assets/asset_cache.cpp
    assets/asset_manager.cpp
    assets/asset_registry.cpp
    assets/asset_serializer.cpp
    assets/asset_loaders/gltf_loader.cpp
)

add_library(engine-core OBJECT ${CORE_SRC})

target_include_directories(engine-core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/engine/include
)

target_link_libraries(engine-core 
PUBLIC 
	dependencies
	glm
	spdlog::spdlog
	imgui # this is public because we need the editor.
PRIVATE
	stb
	entt
)

# When building the engine as a DLL we must compile these static subtargets
# with the export macro so their symbols are emitted into the final shared
# `engine` library. This ensures classes annotated with `IC_API` are
# exported correctly.
target_compile_definitions(engine-core PRIVATE IC_EXPORT)
set_target_properties(engine-core PROPERTIES POSITION_INDEPENDENT_CODE ON)
