set(EDITOR_SOURCES
    src/main.cpp
    src/editor.cpp
    src/panels/component.cpp
    src/panels/heirarchy.cpp
)

add_executable(editor ${EDITOR_SOURCES})

target_include_directories(editor
    PRIVATE
        ${CMAKE_SOURCE_DIR}/editor/src
)

target_link_libraries(editor
    PRIVATE
        engine
	imgui
)

target_compile_options(editor PRIVATE
    $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:Release>:-O2>
)

target_compile_definitions(editor PRIVATE
    $<$<CONFIG:Debug>:DEBUG;IC_ENABLE_ASSERTS>
    $<$<CONFIG:Release>:NDEBUG>
)

set_target_properties(editor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
)

add_custom_command(TARGET editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders/opengl
        ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/shaders/opengl
    COMMENT "Copying editor shaders..."
    VERBATIM
)

add_custom_command(TARGET editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/editor/default_scene
        ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/assets
    COMMENT "Copying editor assets..."
    VERBATIM
)
