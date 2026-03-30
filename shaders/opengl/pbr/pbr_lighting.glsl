#ifndef PBR_LIGHTING_GLSL
#define PBR_LIGHTING_GLSL

const float PI = 3.14159265359;

// --- Light definition ---
struct GPULight
{
	vec4 position;    // xyz = pos,       w = type (0=dir,1=point,2=spot)
	vec4 direction;   // xyz = dir,       w = range
	vec4 color;       // xyz = RGB,       w = intensity
	vec4 spotAngles;  // x = cosInner,    y = cosOuter
};

// --- BRDF helpers ---
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(float NdotH, float roughness)
{
	float a  = roughness * roughness;
	float a2 = a * a;
	float d  = NdotH * NdotH * (a2 - 1.0) + 1.0;
	return a2 / (PI * d * d);
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

// --- Single-light contribution ---
vec3 CalcLightContribution(GPULight light, vec3 N, vec3 V, vec3 albedo, float metallic, float roughness)
{
	int   type        = int(light.position.w);
	vec3  lightPos    = light.position.xyz;
	vec3  lightDir    = light.direction.xyz;
	float range       = light.direction.w;
	vec3  lightColor  = light.color.xyz;
	float intensity   = light.color.w;

	vec3 L;
	float attenuation = 1.0;

	if (light.type == 0)            // directional
	{
		L = normalize(-lightDir);
	}
	else                            // point / spot
	{
		vec3  delta = lightPos - v_WorldPos;
		float dist  = length(delta);
		L           = delta / dist;
		attenuation = 1.0 / (dist * dist);

		// Inverse-square with smooth range cutoff
		float falloff = clamp(1.0 - pow(dist / range, 4.0), 0.0, 1.0);
		attenuation   = (falloff * falloff) / (dist * dist + 1.0);

		if (type == 2) // Spot cone
		{
			float cosTheta = dot(-L, normalize(lightDir));
			float cosInner = light.spotAngles.x;
			float cosOuter = light.spotAngles.y;
			float cone     = clamp((cosTheta - cosOuter) / (cosInner - cosOuter), 0.0, 1.0);
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

	vec3 specular = (NDF * G * F)
			/ (4.0 * NdotV * NdotL + 0.0001);

	vec3 kD = (1.0 - F) * (1.0 - metallic);
	vec3 radiance = lightColor * intensity * attenuation;

	return (kD * albedo / PI + specular) * radiance * NdotL;
}

#endif // PBR_LIGHTING_GLSL