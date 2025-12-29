set(OBJ_DIR ${PROJECT_BINARY_DIR}/obj)
get_subdirectories(SUBDIRS ${CMAKE_CURRENT_LIST_DIR})

# build library (create target)
add_subdirectory(src/core)
add_subdirectory(src/renderer)

if (IC_ENGINE_USE_VULKAN)
    find_package(Vulkan REQUIRED)
    include(${CMAKE_SOURCE_DIR}/cmake/shaders.cmake)
    add_subdirectory(src/renderer/vulkan)
endif()

if (IC_ENGINE_USE_OPENGL)
    find_package(OpenGL REQUIRED)
    add_subdirectory(src/renderer/opengl)
endif()

set(ENGINE_OBJECT_SOURCES
	$<TARGET_OBJECTS:engine-core>
	$<TARGET_OBJECTS:engine-renderer>
)

if (IC_ENGINE_USE_VULKAN)
    list(APPEND ENGINE_OBJECT_SOURCES $<TARGET_OBJECTS:engine-vulkan>)
endif()

if (IC_ENGINE_USE_OPENGL)
    list(APPEND ENGINE_OBJECT_SOURCES $<TARGET_OBJECTS:engine-opengl>)
endif()

# Main engine shared library
add_library(engine SHARED ${ENGINE_OBJECT_SOURCES})
set_target_properties(engine PROPERTIES OUTPUT_NAME "engine")

target_include_directories(engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${IC_INTERNAL_HEADERS}
)

target_link_libraries(engine
PUBLIC
	spdlog::spdlog
    	dependencies
PRIVATE
        engine-core
        engine-renderer
)

if (IC_ENGINE_USE_VULKAN)
    target_link_libraries(engine PRIVATE engine-vulkan Vulkan::Vulkan) # todo: vulkan not needed here
endif()

if (IC_ENGINE_USE_OPENGL)
    target_link_libraries(engine PRIVATE engine-opengl)
endif()

set(ENGINE_GENERATED_SPVS "")

if (IC_ENGINE_USE_VULKAN AND IC_ENGINE_USE_GLSLC) # or whatever flag you set
    file(GLOB_RECURSE ENGINE_SHADERS
        "${CMAKE_SOURCE_DIR}/shaders/*.vert"
        "${CMAKE_SOURCE_DIR}/shaders/*.frag"
    )

    if (ENGINE_SHADERS)
        target_link_spv_shaders(engine ${ENGINE_SHADERS})
        # shaders.cmake sets this variable in PARENT_SCOPE for this target
        set(ENGINE_GENERATED_SPVS ${engine_GENERATED_SPVS})
    else()
        message(WARNING "IC_ENGINE_USE_VULKAN=ON but no shaders were found in ${CMAKE_SOURCE_DIR}/shaders")
    endif()
endif()

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
	INCLUDES DESTINATION includes
)

install(DIRECTORY ${CMAKE_CURRECT_SOURCE_DIR}/src/
	DESTINATION include/ic_engine
	FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
	# exclude internal headers maybe
	# PATTERN "internal" EXCLUDE
)

if (ENGINE_GENERATED_SPVS)
    install(FILES ${ENGINE_GENERATED_SPVS}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/ic_engine/shaders
        OPTIONAL
    )
endif()