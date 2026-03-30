#ifndef PBR_MATERIAL_GLSL
#define PBR_MATERIAL_GLSL

// --- Textures & Factors ---
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

// --- Filled by SampleMaterial(), consumed by lighting ---
struct MaterialData
{
	vec4  baseColor;
	float metallic;
	float roughness;
	vec3  normal;
	float ao;
	vec3  emissive;
};

vec3 CalcNormal(vec2 texCoord, vec3 vNormal, vec4 vTangent)
{
    vec3 N = normalize(vNormal);
    vec3 T = normalize(vTangent.xyz);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T) * vTangent.w;

    vec3 sampledN  = texture(u_NormalTexture, texCoord).xyz * 2.0 - 1.0;
    sampledN.xy   *= u_NormalScale;
    return normalize(mat3(T, B, N) * sampledN);
}

MaterialData SampleMaterial(vec2 texCoord, vec3 vNormal, vec4 vTangent)
{
	MaterialData m;

	// Base color
	m.baseColor = u_BaseColorFactor;
	if (u_HasBaseColorTexture)
		m.baseColor *= texture(u_BaseColorTexture, texCoord);

	// Metallic / Roughness  (glTF: B = metallic, G = roughness)
	m.metallic  = u_MetallicFactor;
	m.roughness = u_RoughnessFactor;
	if (u_HasMetallicRoughnessTexture)
	{
		vec4 mr     = texture(u_MetallicRoughnessTexture, texCoord);
		m.metallic  *= mr.b;
		m.roughness *= mr.g;
	}

	// Normal
	m.normal = (u_HasNormalTexture)
		? CalcNormal(texCoord, vNormal, vTangent)
		: normalize(vNormal);

	// Occlusion
	m.ao = 1.0;
	if (u_HasOcclusionTexture)
		m.ao = mix(1.0, texture(u_OcclusionTexture, texCoord).r, u_OcclusionStrength);

	// Emissive
	m.emissive = u_EmissiveFactor;
	if (u_HasEmissiveTexture)
		m.emissive *= texture(u_EmissiveTexture, texCoord).rgb;

	return m;
}

#endif // PBR_MATERIAL_GLSL