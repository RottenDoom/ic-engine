#version 450 core

layout(location = 0) in vec3 a_Position;

layout(std140, binding = 0) uniform PerFrameBlock
{
    mat4  u_View;
    mat4  u_Projection;
    vec4  u_CameraPos;
    float u_Time;
};

out vec3 v_Direction;

void main()
{
    mat4 rotView = mat4(mat3(u_View));
    vec4 clipPos = u_Projection * rotView * vec4(a_Position, 1.0);
    gl_Position  = clipPos.xyww;
    v_Direction  = a_Position;
}