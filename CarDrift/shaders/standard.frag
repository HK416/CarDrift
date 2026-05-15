#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV0;
layout(location = 3) in vec4 inColor;

struct Light {
	float xPos, yPos, zPos;
	uint type;
	float xDir, yDir, zDir;
	float range;
	float r, g, b;
	float intensity;
	float spotInner, spotOuter;
	int shadowIndex;
	uint pad0;
};

layout(set = 0, binding = 0) uniform GlobalData {
	mat4 view;
	mat4 proj;
	vec3 cameraPos;
	uint pad0;

	Light lights[16];
	uint lightCount;
	float ambientIntensity;
	uint pad1[2];

	mat4 shadowMatrices[4];
} globalData;

layout(set = 1, binding = 0) uniform MaterialParams {
	vec4 albedoFactor;
	float metallicFactor;
	float roughnessFactor;
	float aoFactor;
	uint pad0;
} material;

layout(set = 1, binding = 1) uniform sampler2D albedoMap;
layout(set = 1, binding = 2) uniform sampler2D normalMap;
layout(set = 1, binding = 3) uniform sampler2D metallicRoughnessMap;

layout(location = 0) out vec4 outFragColor;

void main() {
	vec4 albedo = texture(albedoMap, inUV0) * material.albedoFactor;

	vec3 N = normalize(inNormal);
	vec3 resultColor = albedo.rgb * globalData.ambientIntensity;

	for (uint i = 0; i < globalData.lightCount; i++) {
		Light light = globalData.lights[i];

		if (light.type == 0) {
			vec3 L = normalize(-vec3(light.xDir, light.yDir, light.zDir));
			float diff = max(dot(N, L), 0.0);
			vec3 diffuse = diff * vec3(light.r, light.g, light.b) * light.intensity;
			resultColor += albedo.rgb * diffuse;
		}
	}

	outFragColor = vec4(resultColor, albedo.a);
}