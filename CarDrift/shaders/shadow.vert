#version 450
#extension GL_GOOGLE_include_directive : require

#include "global_types.glsl"
#include "input_attributes.glsl"
#include "common.glsl"

layout(location = 0) out vec2 outTexcoord0;

void main() {
	uint index = gl_VertexIndex;

	vec3 inPosition = positions[index];
	vec2 inTexcoord0 = vec2(0.0, 0.0);

	if ((pc.attributeMask & FLAG_UV0) != 0) {
		inTexcoord0 = texcoords0[index];
	}

	mat4 skinMat = mat4(1.0);
	if (HAS_SKINNING && (pc.attributeMask & FLAG_JOINT_INDEX) != 0 && (pc.attributeMask & FLAG_JOINT_WEIGHT) != 0) {
		ivec4 jointIndex = jointIndices[index];
		vec4 jointWeight = jointWeights[index];

		skinMat = (jointWeight.x * jointMatrices[jointIndex.x]) +
				  (jointWeight.y * jointMatrices[jointIndex.y]) +
				  (jointWeight.z * jointMatrices[jointIndex.z]) +
				  (jointWeight.w * jointMatrices[jointIndex.w]);
	}

	mat4 modelMat = pc.worldMatrix * skinMat;
	vec4 worldPos = modelMat * vec4(inPosition, 1.0);

	outTexcoord0 = inTexcoord0;

	gl_Position = global.shadowMatrices[pc.cascadeIndex] * worldPos;
}
