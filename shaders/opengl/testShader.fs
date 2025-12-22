#version 450 core

in vec2 v_TexCoord;
in vec3 v_Normal;
in vec4 v_Color;

uniform sampler2D u_BaseColorTexture;
uniform vec4 u_BaseColorFactor;
uniform int u_HasBaseColorTexture;

out vec4 FragColor;

void main()
{
    // Start with base color factor
    vec4 baseColor = u_BaseColorFactor;
    
    // Multiply by texture if present
    if (u_HasBaseColorTexture != 0)
    {
        baseColor *= texture(u_BaseColorTexture, v_TexCoord);
    }
    
    // Simple lighting (just to see the shape better)
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 normal = normalize(v_Normal);
    float diffuse = max(dot(normal, lightDir), 0.0);
    
    vec3 lighting = vec3(0.3) + vec3(0.7) * diffuse; // Ambient + diffuse
    vec3 color = baseColor.rgb * lighting;
    
    FragColor = vec4(color, baseColor.a);
}