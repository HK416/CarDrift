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

layout(location = 0) out vec4 outColor;

void main() {
	vec4 albedo = texture(albedoMap, inTexcoord0) * material.albedoFactor;

	if (USE_CUTOFF) {
		if (albedo.a < material.alphaCutoff) {
			discard;
		}
	}

	vec3 N = normalize(inNormal);
	vec3 resultColor = albedo.rgb * global.ambientIntensity;

	for (uint i = 0; i < global.lightCount; i++) {
		Light light = global.lights[i];

		if (light.type == 0) {
			vec3 L = normalize(-light.direction);
			float diff = max(dot(N, L), 0.0);
			vec3 diffuse = diff * light.color * light.intensity;
			resultColor += albedo.rgb * diffuse;
		}
	}

	outColor = vec4(resultColor, albedo.a);
}
