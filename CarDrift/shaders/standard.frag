#version 450
#extension GL_GOOGLE_include_directive : require

#include "global_types.glsl"
#include "common.glsl"
#include "pbr_common.glsl"

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inTexcoord0;
layout(location = 4) in vec2 inTexcoord1;
layout(location = 5) in vec4 inColor;
layout(location = 6) in vec3 inViewPos;

layout(location = 0) out vec4 outColor;

//---------------------------------------------------------
// CSM (Cascaded Shadow Mapping)
//---------------------------------------------------------
float CalculateCSM(vec3 worldPos, vec3 N, vec3 L) {
	uint cascadeIndex = 0;
	for (uint i = 0; i < global.cascadeCount - 1; i++) {
		if (abs(inViewPos.z) > global.cascadeSplits[i]) {
			cascadeIndex = i + 1;
		}
	}

	// Calculate Light space coordinate
	vec4 fragPosLightSpace = global.shadowMatrices[cascadeIndex] * vec4(worldPos, 1.0);
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoords.xy = projCoords.xy * 0.5 + 0.5;

	if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
		return 0.0;
	}

	// Shadow Bias
	float bias = max(0.005 * (1.0 - dot(N, L)), 0.0005);
	bias *= 1.0 / (global.cascadeSplits[cascadeIndex] * 0.5);

	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(shadowMap, 0).xy;
	float currentDepth = projCoords.z;

	for (int x = -1; x <= 1; x++) {
		for (int y = -1; y <= 1; y++) {
			float pcfDepth = texture(shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, cascadeIndex)).r;
			shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
		}
	}

	shadow /= 9.0;
	return shadow;
}

void main() {
	vec4 texColor = texture(albedoMap, inTexcoord0);
	vec3 albedo = texColor.rgb * material.albedoFactor.rgb * inColor.rgb;
	float alpha = texColor.a * material.albedoFactor.a * inColor.a;

	if (USE_CUTOFF && alpha < material.alphaCutoff) {
		discard;
	}

	float ao        = mix(1.0, texture(aoMap, inTexcoord0).r, material.aoFactor);
	vec3 mrSample	= texture(metallicRoughnessMap, inTexcoord0).rgb;
	float roughness = material.roughnessFactor * mrSample.g;
	float metallic	= material.metallicFactor * mrSample.b;

	vec3 N = normalize(inNormal);
	if ((pc.attributeMask & FLAG_TANGENT) != 0) {
		vec3 T = normalize(inTangent.xyz);
		vec3 B = normalize(cross(N, T) * inTangent.w);
		mat3 TBN = mat3(T, B, N);

		vec3 mapNormal = texture(normalMap, inTexcoord0).rgb * 2.0 - 1.0;
		N = normalize(TBN * mapNormal);
	}

	vec3 V = normalize(global.cameraPos - inWorldPos);
	vec3 F0 = mix(vec3(0.04), albedo, metallic);
	vec3 Lo = vec3(0.0);

	for (uint i = 0; i < global.lightCount; i++) {
		Light light = global.lights[i];

		vec3 L;
		float attenuation = 1.0;
		float shadow = 0.0;
		if (light.type == LIGHT_TYPE_DIRECTIONAL) {
			L = normalize(-light.direction);

			if (light.shadowIndex >= 0) {
				shadow = CalculateCSM(inWorldPos, N, L);
			}
		} else {
			vec3 lightVec = light.position - inWorldPos;
			float distance = length(lightVec);
			L = normalize(lightVec);

			float distanceRatio = distance / max(light.range, 0.0001);
			float distanceRatio2 = distanceRatio * distanceRatio;
			float falloff = clamp(1.0 - distanceRatio2 * distanceRatio2, 0.0, 1.0);
			attenuation = (falloff * falloff) / (distance * distance + 1.0);

			if (light.type == LIGHT_TYPE_SPOT) {
				float theta = dot(L, normalize(-light.direction));
				float epsilon = light.spotInner - light.spotOuter;
				float spotIntensity = clamp((theta - light.spotOuter) / epsilon, 0.0, 1.0);
				attenuation *= spotIntensity * spotIntensity;
			}
		}

		if (attenuation <= 0.0) {
			continue;
		}

		vec3 radiance = light.color * light.intensity * attenuation;

		float NDF = DistributionGGX(N, normalize(V + L), roughness);
		float G   = GeometrySmith(N, V, L, roughness);
		vec3 F    = fresnelSchlick(max(dot(normalize(V + L), V), 0.0), F0);

		vec3 kS = F;
		vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

		vec3 numerator	  = NDF * G * F;
		float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
		vec3 specular	  = numerator / denominator;

		float NdotL = max(dot(N, L), 0.0);

		Lo += (1.0 - shadow) * (kD * albedo / PI + specular) * radiance * NdotL;
	}

	vec3 ambient = vec3(0.03) * albedo * ao * global.ambientIntensity;
	vec3 color = ambient + Lo;

	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0 / global.gamma));

	outColor = vec4(color, alpha);
}
