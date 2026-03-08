set(OPENGL_SRC
	gl_material.cpp
	gl_model.cpp
	gl_shader.cpp
	opengl_renderer.cpp
)

add_library(engine-opengl STATIC ${OPENGL_SRC})

target_link_libraries(engine-opengl
PUBLIC
	engine-renderer
	glad
PRIVATE	
	stb
	ktx
	OpenGL::GL
)

target_compile_definitions(engine-opengl PRIVATE IC_EXPORT)
set_target_properties(engine-opengl PROPERTIES POSITION_INDEPENDENT_CODE ON)
