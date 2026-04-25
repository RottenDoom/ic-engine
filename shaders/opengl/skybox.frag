#version 450 core

in vec3 v_Direction;

uniform samplerCube u_Skybox;

out vec4 FragColor;

void main()
{
    	FragColor = texture(u_Skybox, v_Direction);
	// FragColor = vec4(1.0, 0.0, 0.0, 1.0);
	// FragColor = vec4(abs(v_Direction), 1.0);
}