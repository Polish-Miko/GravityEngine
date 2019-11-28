
#ifndef _SHADER_UTIL_HLSLI
#define _SHADER_UTIL_HLSLI

#include "ShaderDefinition.h"



float Linear01Depth(float depth)
{
#if USE_REVERSE_Z
	return (1.0f - (depth * NEAR_Z) / (FAR_Z - depth * (FAR_Z - NEAR_Z)));
#else
	return (depth * NEAR_Z) / (FAR_Z - depth * (FAR_Z - NEAR_Z));
#endif
}

float ViewDepth(float depth)
{
	return (FAR_Z * NEAR_Z) / (FAR_Z - depth * (FAR_Z - NEAR_Z));
}

#endif 