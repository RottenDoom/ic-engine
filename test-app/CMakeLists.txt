set(TEST_SOURCES
    src/game-test.cpp
)

add_executable(app ${TEST_SOURCES})

target_include_directories(app
    PRIVATE
        ${CMAKE_SOURCE_DIR}/test-app/src
)

target_link_libraries(app
    PRIVATE
        engine      # gets glm, entt, spdlog, imgui, engine-core all transitively
)

target_compile_options(app PRIVATE
    $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:Release>:-O2>
)

target_compile_definitions(app PRIVATE
    $<$<CONFIG:Debug>:DEBUG;IC_ENABLE_ASSERTS>
    $<$<CONFIG:Release>:NDEBUG>
)

set_target_properties(app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
)


# # Copy assets to build dir so the exe can find them at runtime
add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders/opengl
        ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/shaders/opengl
    COMMENT "Copying test-app assets..."
    VERBATIM
)