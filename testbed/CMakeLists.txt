# -------------------------------------------------------
# Setup ic_engine include directories and source files
# -------------------------------------------------------

set(TEST_SOURCES
    src/entry.cpp
    src/game.cpp

    src/game.h
)

set(OBJ_DIR ${PROJECT_BINARY_DIR}/${PROJECT_BINARY_DIR}/bin)

get_subdirectories(SUBDIRS ${CMAKE_CURRENT_LIST_DIR})

# build library (create target)
add_executable(testbed  ${TEST_SOURCES})

# Set compiler flags
target_compile_options(testbed PRIVATE -g)

# Set include directories
target_include_directories(testbed PRIVATE ${CMAKE_SOURCE_DIR}/engine/src ${CMAKE_SOURCE_DIR}/testbed/src)

# Add linker search paths
link_directories(${OBJ_DIR})

# Set preprocessor definitions
target_compile_definitions(engine PRIVATE _DEBUG IC_IMPORT)

# Set linker flags
target_link_options(testbed PRIVATE -g)
target_link_libraries(testbed PRIVATE engine)

