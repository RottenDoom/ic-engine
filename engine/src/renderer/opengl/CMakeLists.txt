add_library(glad STATIC 
	${CMAKE_SOURCE_DIR}/third-party/glad/src/glad.c
)

target_include_directories(glad PRIVATE
	# ${CMAKE_CURRENT_SOURCE_DIR}/../..
	${CMAKE_SOURCE_DIR}/third-party/glad/include
)

set(OPENGL_SRC
	opengl_renderer.cpp
	backend_opengl.cpp
	gl_shader.cpp
)

add_library(engine-opengl OBJECT ${OPENGL_SRC}) # todo: fix objects

target_include_directories(engine-opengl PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/../..
	${CMAKE_SOURCE_DIR}/third-party/glad/include
)

target_link_libraries(engine-opengl PRIVATE
	dependencies
	glad
	OpenGL::GL
)

target_compile_definitions(engine-opengl PRIVATE IC_EXPORT)