#version 450 core

layout(location = 0) in vec3  a_Position;
layout(location = 1) in vec3  a_Normal;
layout(location = 2) in vec4  a_Tangent;
layout(location = 3) in vec2  a_TexCoord0;
layout(location = 4) in vec2  a_TexCoord1;
layout(location = 8) in uvec4 a_Joints;
layout(location = 9) in vec4  a_Weights;

// ---- UBO binding = 0 : per-frame camera data ----
layout(std140, binding = 0) uniform PerFrameBlock
{
    mat4  u_View;
    mat4  u_Projection;
    vec4  u_CameraPos;
    float u_Time;
};

uniform mat4 u_Model;

out vec3  v_WorldPos;
out vec3  v_Normal;
out vec4  v_Tangent;
out vec2  v_TexCoord;
out uvec4 v_Joints;
out vec4  v_Weights;

void main()
{
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    v_WorldPos    = worldPos.xyz;
    v_Normal      = mat3(transpose(inverse(u_Model))) * a_Normal;
    v_Tangent     = vec4(mat3(transpose(inverse(u_Model))) * a_Tangent.xyz, a_Tangent.w);
    v_TexCoord    = a_TexCoord0;
    v_Joints      = a_Joints;
    v_Weights     = a_Weights;

    gl_Position = u_Projection * u_View * worldPos;
}