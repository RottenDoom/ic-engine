#version 450 core

in vec3 v_Position;
in vec3 v_Normal;
in vec2 v_TexCoord;

uniform sampler2D u_defaultMaterial;
uniform bool u_usedefaultMaterial;

uniform sampler2D u_BaseColorTexture;
uniform sampler2D u_MetallicRoughnessTexture;
uniform sampler2D u_NormalTexture;
uniform sampler2D u_OcclusionTexture;
uniform sampler2D u_EmissiveTexture;

uniform bool u_HasBaseColorTexture;
uniform bool u_HasMetallicRoughnessTexture;
uniform bool u_HasNormalTexture;
uniform bool u_HasOcclusionTexture;
uniform bool u_HasEmissiveTexture;

uniform vec4 u_BaseColorFactor;
uniform float u_MetallicFactor;
uniform float u_RoughnessFactor;
uniform vec3 u_EmissiveFactor;
uniform float u_NormalScale;
uniform float u_OcclusionStrength;

out vec4 FragColor;

const float PI = 3.14159265359;

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
        return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
        float a = roughness * roughness;
        float a2 = a * a;
        float NdotH = max(dot(N, H), 0.0);
        float NdotH2 = NdotH * NdotH;
        
        float num = a2;
        float denom = (NdotH2 * (a2 - 1.0) + 1.0);
        denom = PI * denom * denom;
        
        return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
        float r = (roughness + 1.0);
        float k = (r * r) / 8.0;
        
        float num = NdotV;
        float denom = NdotV * (1.0 - k) + k;
        
        return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
        float NdotV = max(dot(N, V), 0.0);
        float NdotL = max(dot(N, L), 0.0);
        float ggx2 = GeometrySchlickGGX(NdotV, roughness);
        float ggx1 = GeometrySchlickGGX(NdotL, roughness);
        
        return ggx1 * ggx2;
}

void main()
{
        // Sample textures
        vec4 baseColor = u_BaseColorFactor;
        if (u_HasBaseColorTexture)
        {
                baseColor *= texture(u_BaseColorTexture, v_TexCoord);
        }
        
        float metallic = u_MetallicFactor;
        float roughness = u_RoughnessFactor;
        if (u_HasMetallicRoughnessTexture)
        {
                vec4 mr = texture(u_MetallicRoughnessTexture, v_TexCoord);
                metallic *= mr.b;
                roughness *= mr.g;
        }
        
        vec3 N = normalize(v_Normal);
        if (u_HasNormalTexture)
        {
                // Sample and apply normal map
                vec3 tangentNormal = texture(u_NormalTexture, v_TexCoord).xyz * 2.0 - 1.0;
                tangentNormal.xy *= u_NormalScale;
                // TODO: Proper TBN matrix calculation
                N = normalize(tangentNormal);
        }
        
        float ao = 1.0;
        if (u_HasOcclusionTexture)
        {
                ao = texture(u_OcclusionTexture, v_TexCoord).r;
                ao = mix(1.0, ao, u_OcclusionStrength);
        }
        
        vec3 emissive = u_EmissiveFactor;
        if (u_HasEmissiveTexture)
        {
                emissive *= texture(u_EmissiveTexture, v_TexCoord).rgb;
        }
        
        // Simple PBR lighting (simplified - add proper lighting in production)
        vec3 V = normalize(vec3(0, 0, 5) - v_Position);
        vec3 L = normalize(vec3(1, 1, 1));
        vec3 H = normalize(V + L);
        
        vec3 F0 = vec3(0.04);
        F0 = mix(F0, baseColor.rgb, metallic);
        
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        
        float NdotL = max(dot(N, L), 0.0);
        vec3 Lo = (kD * baseColor.rgb / PI + specular) * vec3(1.0) * NdotL;
        
        vec3 ambient = vec3(0.03) * baseColor.rgb * ao;
        vec3 color = ambient + Lo + emissive;
        
        // Tone mapping
        color = color / (color + vec3(1.0));
        // Gamma correction
        color = pow(color, vec3(1.0/2.2));
        
        FragColor = vec4(color, baseColor.a);
}