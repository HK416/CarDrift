#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inUV0;
layout(location = 4) in vec2 inUV1;
layout(location = 5) in vec4 inColor;

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

layout(push_constant) uniform PushConstants {
	mat4 worldMatrix;
} pushConstants;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV0;
layout(location = 3) out vec4 outColor;

void main() {
	vec4 worldPos = pushConstants.worldMatrix * vec4(inPosition, 1.0);
	outWorldPos = worldPos.xyz;

	outNormal = normalize(mat3(pushConstants.worldMatrix) * inNormal);

	outUV0 = inUV0;
	outColor = inColor;

	gl_Position = globalData.proj * globalData.view * worldPos;
}