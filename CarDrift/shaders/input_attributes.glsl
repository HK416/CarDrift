#ifndef INPUT_ATTRIBUTES_GLSL
#define INPUT_ATTRIBUTES_GLSL

#extension GL_EXT_scalar_block_layout: require

layout(scalar, set = 0, binding = 0) readonly buffer PositionBuffer { vec3 positions[]; };
layout(scalar, set = 0, binding = 1) readonly buffer NormalBuffer { vec3 normals[]; };
layout(scalar, set = 0, binding = 2) readonly buffer TangentBuffer { vec4 tangents[]; };
layout(scalar, set = 0, binding = 3) readonly buffer Texcoord0Buffer { vec2 texcoords0[]; };
layout(scalar, set = 0, binding = 4) readonly buffer Texcoord1Buffer { vec2 texcoords1[]; };
layout(scalar, set = 0, binding = 5) readonly buffer ColorBuffer { vec4 colors[]; };
layout(scalar, set = 0, binding = 6) readonly buffer JointIndexBuffer { ivec4 jointIndices[]; };
layout(scalar, set = 0, binding = 7) readonly buffer JointWeightBuffer { vec4 jointWeights[]; };

layout(push_constant) uniform PushContants {
	mat4 worldMatrix;
	uint attributeMask;
} pc;

#endif // INPUT_ATTRIBUTES_GLSL
