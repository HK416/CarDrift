#ifndef PBR_COMMON_GLSL
#define PBR_COMMON_GLSL

layout(set = 2, binding = 0) uniform MaterialUniforms {
	vec4 albedoFactor;
	float roughnessFactor;
	float metallicFactor;
	float aoFactor;
	float alphaCutoff;
} material;

layout(set = 2, binding = 1) uniform sampler2D albedoMap;
layout(set = 2, binding = 2) uniform sampler2D normalMap;
layout(set = 2, binding = 3) uniform sampler2D metallicRoughnessMap;

#endif // PBR_COMMON_GLSL
