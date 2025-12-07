#engine/src/core/CMakeLists.txt
set(CORE_SRC
    application.cpp
    logger.cpp
    platform/Win32/input_win32.cpp
    platform/Win32/window_win32.cpp
)

add_library(engine-core OBJECT ${CORE_SRC})

target_include_directories(engine-core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(engine-core PROPERTIES POSITION_INDEPENDENT_CODE ON)


target_link_libraries(engine-core 
PUBLIC 
	spdlog::spdlog
	glfw
PRIVATE
	dependencies
)

# When building the engine as a DLL we must compile these static subtargets
# with the export macro so their symbols are emitted into the final shared
# `engine` library. This ensures classes annotated with `IC_API` are
# exported correctly.
target_compile_definitions(engine-core PRIVATE IC_EXPORT)
