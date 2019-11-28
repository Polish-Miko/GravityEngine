
#ifndef _STATIC_SAMPLERS_HLSLI
#define _STATIC_SAMPLERS_HLSLI



SamplerState			pointWrapSampler			: register(s0);
SamplerState			pointClampSampler			: register(s1);
SamplerState			linearWrapSampler			: register(s2);
SamplerState			linearClampSampler			: register(s3);
SamplerState			anisotropicWrapSampler		: register(s4);
SamplerState			anisotropicClampSampler		: register(s5);
SamplerState			shadowSampler				: register(s6);


#define basicSampler linearWrapSampler

#endif 