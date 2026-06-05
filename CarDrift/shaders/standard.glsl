#ifndef STANDARD_GLSL
#define STANDARD_GLSL

#extension GL_EXT_scalar_block_layout : require

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

#endif // STANDARD_GLSL
