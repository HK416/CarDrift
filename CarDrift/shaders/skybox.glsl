#ifndef SKYBOX_GLSL
#define SKYBOX_GLSL

#extension GL_EXT_scalar_block_layout : require

//---------------------------------------------------------
// Set 2: Skybox Uniforms
//---------------------------------------------------------
layout(scalar, set = 2, binding = 0) uniform SkyboxUniforms {
	vec4 tintFactor;
	float exposure;
	uvec3 _padding;
} skybox;

layout(set = 2, binding = 1) uniform samplerCube cubeMap;

#endif // SKYBOX_GLSL
