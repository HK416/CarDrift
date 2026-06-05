#ifndef COMMON_GLSL
#define COMMON_GLSL

#extension GL_EXT_scalar_block_layout : require

//---------------------------------------------------------
// Specialization Constants
//---------------------------------------------------------
layout(constant_id = 0) const bool HAS_SKINNING = false;
layout(constant_id = 1) const bool IS_TRANSPARENT = false;
layout(constant_id = 2) const bool USE_CUTOFF = false;

//---------------------------------------------------------
// Set 1: Global Uniforms
//---------------------------------------------------------
layout(scalar, set = 1, binding = 0) uniform GlobalUniforms {
	mat4 view;
	mat4 proj;
	
	vec3 cameraPos;
	uint _pad0;

	Light lights[MAX_LIGHTS];

	uint lightCount;
	float ambientIntensity;
	float gamma;
	uint cascadeCount;

	vec4 cascadeSplits;
	mat4 shadowMatrices[MAX_SHADOW_MAPS];
} global;

layout(set = 1, binding = 1) uniform sampler2DArrayShadow shadowMap;
layout(set = 1, binding = 2) readonly buffer BoneMatBuffer { mat4 boneMatrices[]; };

//---------------------------------------------------------
// Push Constants
//---------------------------------------------------------
layout(push_constant) uniform PushContants {
	mat4 worldMatrix;
	uint attributeMask;
	uint cascadeIndex;
	int boneOffset;
} pc;

#endif // COMMON_GLSL
