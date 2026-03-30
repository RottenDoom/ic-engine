#version 450 core

// -----------------------------------------------------------------------
// Varyings
// -----------------------------------------------------------------------
in vec3  v_WorldPos;
in vec3  v_Normal;
in vec4  v_Tangent;
in vec2  v_TexCoord;
in uvec4 v_Joints;
in vec4  v_Weights;

out vec4 FragColor;

// -----------------------------------------------------------------------
// UBO  binding = 0 : per-frame
// -----------------------------------------------------------------------
layout(std140, binding = 0) uniform PerFrameBlock
{
    mat4  u_View;
    mat4  u_Projection;
    vec4  u_CameraPos;
    float u_Time;
};

// -----------------------------------------------------------------------
// UBO  binding = 1 : lights
// Struct must be declared OUTSIDE the block, then used inside.
// -----------------------------------------------------------------------
struct GPULight
{
    vec4 position;    // xyz = world pos,  w = type (0=dir, 1=point, 2=spot)
    vec4 direction;   // xyz = direction,  w = range
    vec4 color;       // xyz = RGB,        w = intensity
    vec4 spotAngles;  // x = cosInner,     y = cosOuter
};

layout(std140, binding = 1) uniform LightBlock
{
    GPULight u_Lights[16];
    int      u_LightCount;
};

// -----------------------------------------------------------------------
// Material uniforms
// -----------------------------------------------------------------------
uniform sampler2D u_BaseColorTexture;
uniform sampler2D u_MetallicRoughnessTexture;
uniform sampler2D u_NormalTexture;
uniform sampler2D u_OcclusionTexture;
uniform sampler2D u_EmissiveTexture;

uniform bool  u_HasBaseColorTexture;
uniform bool  u_HasMetallicRoughnessTexture;
uniform bool  u_HasNormalTexture;
uniform bool  u_HasOcclusionTexture;
uniform bool  u_HasEmissiveTexture;

uniform vec4  u_BaseColorFactor;
uniform float u_MetallicFactor;
uniform float u_RoughnessFactor;
uniform vec3  u_EmissiveFactor;
uniform float u_NormalScale;
uniform float u_OcclusionStrength;

// -----------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------
const float PI = 3.14159265359;

// -----------------------------------------------------------------------
// Material sampling
// -----------------------------------------------------------------------
struct MaterialData
{
    vec4  baseColor;
    float metallic;
    float roughness;
    vec3  normal;
    float ao;
    vec3  emissive;
};

vec3 CalcNormal()
{
    vec3 N = normalize(v_Normal);
    vec3 T = normalize(v_Tangent.xyz);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T) * v_Tangent.w;

    vec3 sample_n  = texture(u_NormalTexture, v_TexCoord).xyz * 2.0 - 1.0;
    sample_n.xy   *= u_NormalScale;
    return normalize(mat3(T, B, N) * sample_n);
}

MaterialData SampleMaterial()
{
    MaterialData m;

    m.baseColor = u_BaseColorFactor;
    if (u_HasBaseColorTexture)
        m.baseColor *= texture(u_BaseColorTexture, v_TexCoord);

    m.metallic  = u_MetallicFactor;
    m.roughness = u_RoughnessFactor;
    if (u_HasMetallicRoughnessTexture)
    {
        vec4 mr     = texture(u_MetallicRoughnessTexture, v_TexCoord);
        m.metallic  *= mr.b;
        m.roughness *= mr.g;
    }

    m.normal = u_HasNormalTexture ? CalcNormal() : normalize(v_Normal);

    m.ao = 1.0;
    if (u_HasOcclusionTexture)
        m.ao = mix(1.0, texture(u_OcclusionTexture, v_TexCoord).r, u_OcclusionStrength);

    m.emissive = u_EmissiveFactor;
    if (u_HasEmissiveTexture)
        m.emissive *= texture(u_EmissiveTexture, v_TexCoord).rgb;

    return m;
}

// -----------------------------------------------------------------------
// BRDF helpers
// -----------------------------------------------------------------------
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(float NdotH, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / max(PI * d * d, 0.0001);
}

float GeometrySchlickGGX(float NdotX, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotX / (NdotX * (1.0 - k) + k);
}

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    return GeometrySchlickGGX(NdotV, roughness)
         * GeometrySchlickGGX(NdotL, roughness);
}

// -----------------------------------------------------------------------
// Per-light contribution
// -----------------------------------------------------------------------
vec3 CalcLightContribution(GPULight light, vec3 N, vec3 V,
                            vec3 albedo, float metallic, float roughness)
{
    int   ltype      = int(light.position.w);
    vec3  lightPos   = light.position.xyz;
    vec3  lightDir   = normalize(light.direction.xyz);
    float range      = light.direction.w;
    vec3  lightColor = light.color.xyz;
    float intensity  = light.color.w;

    vec3  L;
    float attenuation = 1.0;

    if (ltype == 0) // Directional
    {
        L = normalize(-lightDir);
    }
    else            // Point or Spot
    {
        vec3  delta = lightPos - v_WorldPos;
        float dist  = length(delta);
        L           = delta / dist;

        float falloff = clamp(1.0 - pow(dist / range, 4.0), 0.0, 1.0);
        attenuation   = (falloff * falloff) / (dist * dist + 1.0);

        if (ltype == 2) // Spot
        {
            float cosTheta = dot(-L, lightDir);
            float cosInner = light.spotAngles.x;
            float cosOuter = light.spotAngles.y;
            float cone     = clamp((cosTheta - cosOuter)
                                 / (cosInner  - cosOuter + 0.0001), 0.0, 1.0);
            attenuation   *= cone;
        }
    }

    vec3  H     = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    vec3  F0      = mix(vec3(0.04), albedo, metallic);
    vec3  F       = FresnelSchlick(HdotV, F0);
    float NDF     = DistributionGGX(NdotH, roughness);
    float G       = GeometrySmith(NdotV, NdotL, roughness);

    vec3  specular  = (NDF * G * F) / (4.0 * NdotV * NdotL + 0.0001);
    vec3  kD        = (1.0 - F) * (1.0 - metallic);
    vec3  radiance  = lightColor * intensity * attenuation;

    return (kD * albedo / PI + specular) * radiance * NdotL;
}

// -----------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------
void main()
{
    MaterialData mat = SampleMaterial();

    vec3 V  = normalize(u_CameraPos.xyz - v_WorldPos);
    vec3 Lo = vec3(0.0);

    for (int i = 0; i < u_LightCount; ++i)
        Lo += CalcLightContribution(u_Lights[i], mat.normal, V,
                                    mat.baseColor.rgb, mat.metallic, mat.roughness);

    vec3 ambient = vec3(0.03) * mat.baseColor.rgb * mat.ao;
    vec3 color   = ambient + Lo + mat.emissive;

    // Reinhard tonemap
    color = color / (color + vec3(1.0));
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, mat.baseColor.a);
}