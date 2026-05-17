#ifndef GLOBAL_TYPES_GLSL
#define GLOBAL_TYPES_GLSL

const uint FLAG_POSITION	 = 1 << 0;
const uint FLAG_NORMAL		 = 1 << 1;
const uint FLAG_TANGENT		 = 1 << 2;
const uint FLAG_UV0			 = 1 << 3;
const uint FLAG_UV1			 = 1 << 4;
const uint FLAG_COLOR		 = 1 << 5;
const uint FLAG_JOINT_INDEX  = 1 << 6;
const uint FLAG_JOINT_WEIGHT = 1 << 7;

const uint MAX_LIGHTS	   = 16;
const uint MAX_SHADOW_MAPS = 4;

struct Light {
	vec3 position;
	uint type;

	vec3 direction;
	float range;

	vec3 color;
	float intensity;

	float spotInner;
	float spotOuter;
	int shadowIndex;
	uint _pad0;
};

#endif // GLOBAL_TYPES_GLSL
