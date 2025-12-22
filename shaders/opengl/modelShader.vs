#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec4 a_Tangent;
layout(location = 3) in vec2 a_TexCoord0;
layout(location = 4) in vec2 a_TexCoord1;
layout(location = 6) in vec3 a_TexCoord2;
layout(location = 7) in vec4 a_Color;
layout(location = 8) in uvec4 a_Joints;
layout(location = 9) in vec4 a_Weights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 v_Position;
out vec3 v_Normal;
out vec2 v_TexCoord;

void main()
{
        vec4 worldPos = model * vec4(a_Position, 1.0);
        v_Position = worldPos.xyz;
        v_Normal = mat3(transpose(inverse(model))) * a_Normal;
        v_TexCoord = a_TexCoord0;
        
        gl_Position = projection * view * worldPos;
}
