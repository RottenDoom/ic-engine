#engine/src/core/CMakeLists.txt
set(CORE_SRC
    application.cpp
    logger.cpp
    platform/Win32/input_win32.cpp
    platform/Win32/window_win32.cpp
)

add_library(engine-core STATIC ${CORE_SRC})

target_include_directories(engine-core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(engine-core 
PUBLIC 
	spdlog::spdlog
	glfw
PRIVATE
	Vulkan::Vulkan
	dependencies
)
