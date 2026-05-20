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

//---------------------------------------------------------
// Set 2: Material Uniforms
//---------------------------------------------------------
layout(scalar, set = 2, binding = 0) uniform MaterialUniforms {
	vec4 albedoFactor;
	float roughnessFactor;
	float metallicFactor;
	float alphaCutoff;
	float aoFactor;
} material;

layout(set = 2, binding = 1) uniform sampler2D albedoMap;
layout(set = 2, binding = 2) uniform sampler2D normalMap;
layout(set = 2, binding = 3) uniform sampler2D metallicRoughnessMap;
layout(set = 2, binding = 4) uniform sampler2D aoMap;

//---------------------------------------------------------
// Push Constants
//---------------------------------------------------------
layout(push_constant) uniform PushContants {
	mat4 worldMatrix;
	uint attributeMask;
	uint cascadeIndex;
} pc;

#endif // COMMON_GLSL
