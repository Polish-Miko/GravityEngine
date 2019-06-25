#pragma once

#include "Public/GGiPreInclude.h"

#include "Public/GGiGameTimer.h"
#include "Public/GGiFloat4x4.h"
#include "Public/GGiFloat4.h"
#include "Public/GGiException.h"
#include "Public/GGiEngineUtil.h"
#include "Public/GGiCpuProfiler.h"
#include "Public/GGiThreadPool.h"
#include "Public/GGiMath.h"

template<typename T1, typename T2>
inline T1 DynamicCast(T2 toCast, const std::string& fileName = __FILE__, int lineNumber = __LINE__)
{
	T1 casted = dynamic_cast<T1>(toCast);
	if (casted == nullptr)
		throw GGiException("Dynamic cast failed.", fileName, lineNumber);
	return casted; 
}

#define GDynamicCast(T,src,dest)\
{\
	T casted = dynamic_cast<T>(src);\
	if (casted == nullptr)\
		throw GGiException("Dynamic cast failed.", __FILE__, __LINE__);\
	dest = casted;\
}





