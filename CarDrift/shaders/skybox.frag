#version 450
#extension GL_GOOGLE_include_directive : require

#include "global_types.glsl"
#include "common.glsl"
#include "skybox.glsl"
#include "pbr_common.glsl"

layout(location = 0) in vec3 inUVW;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(cubeMap, inUVW);
    vec3 color = texColor.rgb * skybox.tintFactor.rgb;

    color *= skybox.exposure;

    color = ACESFilm(color);
    color = pow(color, vec3(1.0 / global.gamma));

    outColor = vec4(color, texColor.a * skybox.tintFactor.a);
}
