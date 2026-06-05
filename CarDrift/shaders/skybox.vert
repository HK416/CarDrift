#version 450
#extension GL_GOOGLE_include_directive : require

#include "global_types.glsl"
#include "input_attributes.glsl"
#include "common.glsl"

layout(location = 0) out vec3 outUVW;

void main() {
	uint index = gl_VertexIndex;
	vec3 inPosition = positions[index];
    
    outUVW = inPosition;

    mat4 rotView = mat4(mat3(global.view));
    vec4 clipPos = global.proj * rotView * vec4(inPosition, 1.0);
    gl_Position = clipPos.xyww;
}
