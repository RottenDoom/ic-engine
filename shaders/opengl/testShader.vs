#version 450 core

// Vertex attributes (matches your VAO setup)
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec4 a_Tangent;
layout(location = 3) in vec2 a_TexCoord0;
layout(location = 4) in vec2 a_TexCoord1;
layout(location = 5) in vec2 a_TexCoord1;
layout(location = 6) in vec4 a_Color;
layout(location = 7) in uvec4 a_Joints;
layout(location = 8) in vec4 a_Weights;

// Your camera uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Output to fragment shader
out vec2 v_TexCoord;
out vec3 v_Normal;
out vec4 v_Color;

void main()
{
    // Transform position
    gl_Position = projection * view * model * vec4(a_Position, 1.0);
    
    // Pass data to fragment shader
    v_TexCoord = a_TexCoord0;
    v_Normal = mat3(model) * a_Normal;
    v_Color = a_Color;
}