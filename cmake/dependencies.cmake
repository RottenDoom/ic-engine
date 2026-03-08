include(FetchContent)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY "https://github.com/glfw/glfw.git"
    GIT_TAG        master  # or latest stable
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glfw)


# SPDLOG
FetchContent_Declare(spdlog
    GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
    GIT_TAG v1.x
)

set(SPDLOG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)  # Build as static library

FetchContent_MakeAvailable(spdlog)


# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY "https://github.com/g-truc/glm.git"
    GIT_TAG 1.0.1
)

FetchContent_MakeAvailable(glm)

# stb
FetchContent_Declare(
    stb
    GIT_REPOSITORY "https://github.com/nothings/stb.git"
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)

FetchContent_Declare(
    ktx
    GIT_REPOSITORY "https://github.com/KhronosGroup/KTX-Software.git"
    GIT_TAG v4.4.2
)

# Build only the library (skip CLI tools and tests)
set(KTX_FEATURE_TESTS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_GL_UPLOAD OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_VK_UPLOAD ON CACHE BOOL "" FORCE)
set(KTX_FEATURE_LOADTEST_APPS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_TOOLS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_DOC OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(ktx)	

# fastgltf
# FetchContent_Declare(
#     fastgltf
#     GIT_REPOSITORY "https://github.com/spnda/fastgltf.git"
#     GIT_TAG main
# )



add_library(dependencies INTERFACE)


target_include_directories(dependencies
INTERFACE
	${glm_SOURCE_DIR}
	${ktx_SOURCE_DIR}/include
	${spdlog_SOURCE_DIR}/include
	${glfw_SOURCE_DIR}/include
)

target_link_libraries(dependencies
INTERFACE
	glm
	ktx
	spdlog::spdlog
glfw
)
set(IC_INTERNAL_HEADERS
	${CMAKE_SOURCE_DIR}/third-party/tiny_gltf
	${CMAKE_SOURCE_DIR}/third-party/stb
	${CMAKE_SOURCE_DIR}/third-party/basisu/transcoder
	${CMAKE_SOURCE_DIR}/third-party/basisu/zstd
)

add_library(renderer_dependencies INTERFACE)

target_include_directories(renderer_dependencies INTERFACE ${IC_INTERNAL_HEADERS})