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

