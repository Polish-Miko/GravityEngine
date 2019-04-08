
#ifndef _MATERIAL_HLSLI
#define _MATERIAL_HLSLI


//
// max number must be a multiple of 4, and must be the same with the macro defined in GMaterial.h
//
#define MATERIAL_MAX_TEXTURE_NUM 16
#define MATERIAL_MAX_SCALAR_NUM 16
#define MATERIAL_MAX_VECTOR_NUM 16


struct MaterialData
{
	float4x4 MatTransform;
	uint	 TextureIndex[MATERIAL_MAX_TEXTURE_NUM];
	float    ScalarParams[MATERIAL_MAX_SCALAR_NUM];
	float4	 VectorParams[MATERIAL_MAX_VECTOR_NUM];
};

// Put in space1, so the texture array does not overlap with these resources.  
// The texture array will occupy registers t0, t1, ..., t3 in space0. 
StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);

#endif 