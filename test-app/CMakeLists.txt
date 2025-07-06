# setup for application

set(TEST_SOURCES
    src/entry.cpp
    src/game.cpp

    src/game.h
)

set(OBJ_DIR ${PROJECT_BINARY_DIR}/${PROJECT_BINARY_DIR}/bin)
get_subdirectories(SUBDIRS ${CMAKE_CURRENT_LIST_DIR})

# build target
add_executable(app ${TEST_SOURCES})

# Set include directories
target_include_directories(app PRIVATE 
    ${CMAKE_SOURCE_DIR}/engine/src 
    ${CMAKE_SOURCE_DIR}/test-app/src
)

# set compiler flags
target_compile_options(app PRIVATE
    $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:Release>:-O2>
)

# Add linker search paths
link_directories(${OBJ_DIR})

# Set preprocessor definitions
target_compile_definitions(app PRIVATE
    $<$<CONFIG:Debug>:DEBUG IC_ENABLE_ASSERTS IC_IMPORT>
    $<$<CONFIG:Release>:NDEBUG IC_IMPORT>
)

# Set linker flags
target_link_options(app PRIVATE
    $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:Release>:-s>
)

target_link_libraries(app PRIVATE engine)

set_target_properties(app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
)