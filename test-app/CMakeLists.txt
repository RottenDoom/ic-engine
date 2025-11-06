# test-app/CMakeLists.txt (preferred)
set(TEST_SOURCES
    src/entry.cpp
    src/game.cpp
    src/game.h
)

add_executable(app ${TEST_SOURCES})

# Only need to add test-app local headers
target_include_directories(app PRIVATE
    ${CMAKE_SOURCE_DIR}/test-app/src
    ${CMAKE_SOURCE_DIR}/engine/src/
)

# Compiler flags
target_compile_options(app PRIVATE
    $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:Release>:-O2>
)

# Link to engine (engine should export its public includes)
target_link_libraries(app PRIVATE
    engine
)

# Preprocessor defs (IC_IMPORT so __declspec(dllimport) is used)
target_compile_definitions(app PRIVATE
    $<$<CONFIG:Debug>:DEBUG;IC_ENABLE_ASSERTS>
    $<$<CONFIG:Release>:NDEBUG>
)

# Linker flags
target_link_options(app PRIVATE
    $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:Release>:-s>
)

# Output directories
set_target_properties(app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
)
