function(add_sample TARGET_NAME SOURCE_FILE)
    add_executable(${TARGET_NAME} ${SOURCE_FILE})
    target_link_libraries(${TARGET_NAME} PRIVATE engine)
    set_target_properties(${TARGET_NAME} PROPERTIES
        FOLDER "samples"
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
    )
    
    # I just prefer all the executables even samples in the build dir
#     add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_directory
#             ${CMAKE_SOURCE_DIR}/samples/assets                    # Source
#             ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/assets      # Destination
#         COMMENT "Copying assets for ${TARGET_NAME}..."
#     )
    
    if (WINDOWS)
        target_dxc_copy(${TARGET_NAME})
    endif()
endfunction()

add_sample(fs filesystem_example.cpp)
add_sample(asset assets_example.cpp)