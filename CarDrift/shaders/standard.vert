#version 450
#extension GL_GOOGLE_include_directive : require

#include "global_types.glsl"
#include "input_attributes.glsl"
#include "common.glsl"

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec4 outTangent;
layout(location = 3) out vec2 outTexcoord0;
layout(location = 4) out vec2 outTexcoord1;
layout(location = 5) out vec4 outColor;

void main() {
	uint index = gl_VertexIndex;

	vec3 inPosition = positions[index];
	vec3 inNormal = vec3(0.0, 1.0, 0.0);
	vec4 inTangent = vec4(1.0, 0.0, 0.0, 1.0);
	vec2 inTexcoord0 = vec2(0.0, 0.0);
	vec2 inTexcoord1 = vec2(0.0, 0.0);
	vec4 inColor = vec4(1.0, 1.0, 1.0, 1.0);

	if ((pc.attributeMask & FLAG_NORMAL) != 0)	inNormal = normals[index];
	if ((pc.attributeMask & FLAG_TANGENT) != 0) inTangent = tangents[index];
	if ((pc.attributeMask & FLAG_UV0) != 0)		inTexcoord0 = texcoords0[index];
	if ((pc.attributeMask & FLAG_UV1) != 0)		inTexcoord1 = texcoords1[index];
	if ((pc.attributeMask & FLAG_COLOR) != 0)	inColor = colors[index];

	mat4 skinMat = mat4(1.0);
	if (HAS_SKINNING && (pc.attributeMask & FLAG_JOINT_INDEX) != 0 && (pc.attributeMask & FLAG_JOINT_WEIGHT) != 0) {
		ivec4 jointIndex = jointIndices[index];
		vec4 jointWeight = jointWeights[index];
	}

	mat4 modelMat = pc.worldMatrix * skinMat;
	vec4 worldPos = modelMat * vec4(inPosition, 1.0);

	mat3 normalMat = transpose(inverse(mat3(modelMat)));
	vec3 worldNormal = normalize(normalMat * inNormal);
	vec4 worldTangent = vec4(normalize(normalMat * inTangent.xyz), inTangent.w);

	outWorldPos = worldPos.xyz;
	outNormal = worldNormal;
	outTangent = worldTangent;
	outTexcoord0 = inTexcoord0;
	outTexcoord1 = inTexcoord1;
	outColor = inColor;

	gl_Position = global.proj * global.view * worldPos;
}
