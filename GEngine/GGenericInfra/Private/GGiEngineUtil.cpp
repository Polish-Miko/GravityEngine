#include "stdafx.h"
#include "GGiEngineUtil.h"


GGiEngineUtil::GGiEngineUtil()
{
}


GGiEngineUtil::~GGiEngineUtil()
{
}

const float GGiEngineUtil::PI = 3.14159265359f;
const float GGiEngineUtil::Infinity = FLT_MAX;
const float GGiEngineUtil::DegToRad = GGiEngineUtil::PI / 180.0f;
const float GGiEngineUtil::RadToDeg = 180.0f / GGiEngineUtil::PI;

// Memory Allocation Functions
void *AllocAligned(size_t size)
{
	return _aligned_malloc(size, GGI_L1_CACHE_LINE_SIZE);
}

void FreeAligned(void *ptr)
{
	if (!ptr) return;
	_aligned_free(ptr);
}

