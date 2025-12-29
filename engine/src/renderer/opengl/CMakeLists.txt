set(OPENGL_SRC
	opengl_renderer.cpp
	backend_opengl.cpp
	gl_shader.cpp
	gl_model.cpp
	gl_material.cpp
	gltf_loader.cpp
)

add_library(engine-opengl OBJECT ${OPENGL_SRC})

target_include_directories(engine-opengl PRIVATE
	${CMAKE_SOURCE_DIR}/engine/include
	${CMAKE_SOURCE_DIR}/third-party/glad/include
	${CMAKE_SOURCE_DIR}/third-party/stb
)

target_link_libraries(engine-opengl PRIVATE
	dependencies
	OpenGL::GL
)

target_compile_definitions(engine-opengl PRIVATE IC_EXPORT)