set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

set(SPDLOG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_SHARED   OFF CACHE BOOL "" FORCE)

set(YAML_CPP_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS   OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(FASTGLTF_ENABLE_SIMDJSON OFF CACHE BOOL "" FORCE)
set(FASTGLTF_BUILD_TESTS     OFF CACHE BOOL "" FORCE)
set(FASTGLTF_BUILD_EXAMPLES  OFF CACHE BOOL "" FORCE)
set(FASTGLTF_ENABLE_INSTALL  OFF CACHE BOOL "" FORCE)

set(KTX_FEATURE_TESTS     OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_VK_UPLOAD OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_GL_UPLOAD ON  CACHE BOOL "" FORCE)
set(KTX_FEATURE_TOOLS     OFF CACHE BOOL "" FORCE)

# Submodule directories
add_subdirectory(${PROJECT_SOURCE_DIR}/third-party/glfw)
add_subdirectory(${PROJECT_SOURCE_DIR}/third-party/spdlog)
add_subdirectory(${PROJECT_SOURCE_DIR}/third-party/yaml-cpp)
add_subdirectory(${PROJECT_SOURCE_DIR}/third-party/glm)
add_subdirectory(${PROJECT_SOURCE_DIR}/third-party/fastgltf)
add_subdirectory(${PROJECT_SOURCE_DIR}/third-party/ktx)

# GLAD
add_library(glad STATIC
    ${PROJECT_SOURCE_DIR}/third-party/glad/src/glad.c
)
target_include_directories(glad PUBLIC
    ${PROJECT_SOURCE_DIR}/third-party/glad/include
)

# STB
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE
    ${PROJECT_SOURCE_DIR}/third-party/stb
)

# ENTT
add_library(entt INTERFACE)
target_include_directories(entt SYSTEM INTERFACE
    ${PROJECT_SOURCE_DIR}/third-party/entt/single_include
)

# ImGui
set(IMGUI_DIR ${PROJECT_SOURCE_DIR}/third-party/imgui)

set(IMGUI_SRC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

if(IC_ENGINE_USE_VULKAN)
    list(APPEND IMGUI_SRC
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    )
endif()

add_library(imgui STATIC ${IMGUI_SRC})

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_compile_definitions(imgui PUBLIC
    IMGUI_ENABLE_DOCKING
    IMGUI_ENABLE_VIEWPORTS
)

target_link_libraries(imgui PRIVATE glfw)

if(IC_ENGINE_USE_OPENGL)
    find_package(OpenGL REQUIRED)
    target_link_libraries(imgui PUBLIC OpenGL::GL glad)
endif()

if(IC_ENGINE_USE_VULKAN)
    find_package(Vulkan REQUIRED)
    target_link_libraries(imgui PUBLIC Vulkan::Vulkan)
endif()
