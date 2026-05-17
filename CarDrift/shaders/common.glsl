#ifndef COMMON_GLSL
#define COMMON_GLSL

layout(constant_id = 0) const bool HAS_SKINNING = false;
layout(constant_id = 1) const bool IS_TRANSPARENT = false;
layout(constant_id = 2) const bool USE_CUTOFF = false;

layout(set = 1, binding = 0) uniform GlobalUniforms {
	mat4 view;
	mat4 proj;
	
	vec3 cameraPos;
	uint _pad0;

	Light lights[MAX_LIGHTS];

	uint lightCount;
	float ambientIntensity;
	uint _pad1[2];

	mat4 shadowMatrices[MAX_SHADOW_MAPS];
} global;

#endif // COMMON_GLSL
