set(OBJ_DIR ${PROJECT_BINARY_DIR}/obj)
get_subdirectories(SUBDIRS ${CMAKE_CURRENT_LIST_DIR})

find_package(Vulkan REQUIRED)

# build library (create target)
add_subdirectory(src/core)
add_subdirectory(src/renderer)
add_subdirectory(src/renderer/vulkan)

# Main engine shared library
add_library(engine SHARED
	$<TARGET_OBJECTS:engine-core>
	$<TARGET_OBJECTS:engine-renderer>
	$<TARGET_OBJECTS:engine-vulkan>
)
set_target_properties(engine PROPERTIES OUTPUT_NAME "engine")

target_include_directories(engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src  # For public API headers
    PRIVATE
        ${IC_INTERNAL_HEADERS}
)

target_link_libraries(engine
    PUBLIC
	spdlog::spdlog
    PRIVATE
    	dependencies
        engine-core
        engine-renderer
        engine-vulkan
)

set(ENGINE_SHADERS
	${CMAKE_SOURCE_DIR}/shaders/*.vert
	${CMAKE_SOURCE_DIR}/shaders/*.frag
)

target_link_spv_shaders(engine ${ENGINE_SHADERS})

set(ENGINE_GENERATED_SPVS ${engine_GENERATED_SPVS})

set_target_properties(engine PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
)

install(TARGETS engine 
	EXPORT ic_engineTargets
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	INCLUDES DESTINATION includes # make sure these are all the .h files instead of this or change the directory structure.
)

install(DIRECTORY ${CMAKE_CURRECT_SOURCE_DIR}/src/
	DESTINATION include/ic_engine
	FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
	# exclude internal headers maybe
	# PATTERN "internal" EXCLUDE
)

install(FILES ${ENGINE_GENERATED_SPVS}
	DESTINATION ${CMAKE_INSTALL_DATADIR}/ic_engine/shaders
	OPTIONAL
)