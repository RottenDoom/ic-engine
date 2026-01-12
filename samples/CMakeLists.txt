function(add_sample TARGET_NAME SOURCE_FILE)
        add_executable(${TARGET_NAME} ${SOURCE_FILE})
	target_link_libraries(${TARGET_NAME} PRIVATE engine)

	set_target_properties(${TARGET_NAME} PROPERTIES
		FOLDER "samples"
		RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/Debug # TODO: fix this
	)
	if (WINDOWS)
		target_dxc_copy(${TARGET_NAME})
	endif()
endfunction()

add_sample(fs filesystem_example.cpp)