#version 450
#extension GL_GOOGLE_include_directive : require

#include "global_types.glsl"
#include "common.glsl"

layout(location = 0) in vec2 inTexcoord0;

void main() {
	if (USE_CUTOFF) {
		vec4 texColor = texture(albedoMap, inTexcoord0);
		float alpha = texColor.a * material.albedoFactor.a;
		if (alpha < material.alphaCutoff) {
			discard;
		}
	}
}
