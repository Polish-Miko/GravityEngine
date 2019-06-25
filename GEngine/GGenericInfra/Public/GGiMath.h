#pragma once
#include "GGiPreInclude.h"

#define USE_GGI_VECTOR_CALL 1

#if USE_GGI_VECTOR_CALL
#define _GGI_VECTOR_CALL __vectorcall
#else
#define _GGI_VECTOR_CALL __cdecl
#endif

typedef __m128 GGiVector4;
typedef __m128d GGiVectorI4;

// Fix-up for (1st-3rd) XMVECTOR parameters that are pass-in-register for x86, ARM, ARM64, and vector call; by reference otherwise
#if ( defined(_M_IX86) || defined(_M_ARM) || defined(_M_ARM64) || USE_GGI_VECTOR_CALL )
typedef const GGiVector4 GGiVector4_F;
#else
typedef const GGiVector4& GGiVector4_F;
#endif

// Fix-up for (4th) XMVECTOR parameter to pass in-register for ARM, ARM64, and x64 vector call; by reference otherwise
#if ( defined(_M_ARM) || defined(_M_ARM64) || defined(_M_HYBRID_X86_ARM64) || (USE_GGI_VECTOR_CALL && !defined(_M_IX86) ) )
typedef const GGiVector4 GGiVector4_G;
#else
typedef const GGiVector4& GGiVector4_G;
#endif

// Fix-up for (5th & 6th) XMVECTOR parameter to pass in-register for ARM64 and vector call; by reference otherwise
#if ( defined(_M_ARM64) || defined(_M_HYBRID_X86_ARM64) || USE_GGI_VECTOR_CALL )
typedef const GGiVector4 GGiVector4_H;
#else
typedef const GGiVector4& GGiVector4_H;
#endif

// Fix-up for (7th+) XMVECTOR parameters to pass by reference
typedef const GGiVector4& GGiVector4_C;



#ifndef GGIGLOBALCONST
#define GGIGLOBALCONST extern const __declspec(selectany)
#endif

#ifndef GGI_CONST
#define GGI_CONST constexpr
#endif

#define _GGI_SHUFFLE_PS( v, c ) _mm_shuffle_ps( v, v, c )

//*****************************************************************************************************************************
// Constant Scalars.
//*****************************************************************************************************************************

GGI_CONST float GGI_PI = 3.141592654f;
GGI_CONST float GGI_2PI = 6.283185307f;
GGI_CONST float GGI_1DIVPI = 0.318309886f;
GGI_CONST float GGI_1DIV2PI = 0.159154943f;
GGI_CONST float GGI_PIDIV2 = 1.570796327f;
GGI_CONST float GGI_PIDIV4 = 0.785398163f;

GGI_CONST uint32_t GGI_SELECT_0 = 0x00000000;
GGI_CONST uint32_t GGI_SELECT_1 = 0xFFFFFFFF;

GGI_CONST uint32_t GGI_PERMUTE_0X = 0;
GGI_CONST uint32_t GGI_PERMUTE_0Y = 1;
GGI_CONST uint32_t GGI_PERMUTE_0Z = 2;
GGI_CONST uint32_t GGI_PERMUTE_0W = 3;
GGI_CONST uint32_t GGI_PERMUTE_1X = 4;
GGI_CONST uint32_t GGI_PERMUTE_1Y = 5;
GGI_CONST uint32_t GGI_PERMUTE_1Z = 6;
GGI_CONST uint32_t GGI_PERMUTE_1W = 7;

GGI_CONST uint32_t GGI_SWIZZLE_X = 0;
GGI_CONST uint32_t GGI_SWIZZLE_Y = 1;
GGI_CONST uint32_t GGI_SWIZZLE_Z = 2;
GGI_CONST uint32_t GGI_SWIZZLE_W = 3;

GGI_CONST uint32_t GGI_CRMASK_CR6 = 0x000000F0;
GGI_CONST uint32_t GGI_CRMASK_CR6TRUE = 0x00000080;
GGI_CONST uint32_t GGI_CRMASK_CR6FALSE = 0x00000020;
GGI_CONST uint32_t GGI_CRMASK_CR6BOUNDS = GGI_CRMASK_CR6FALSE;

GGI_CONST size_t GGI_CACHE_LINE_SIZE = 64;

//*****************************************************************************************************************************
// Constant Vectors.
//*****************************************************************************************************************************

GGIGLOBALCONST GGiVector4 g_GGiSinCoefficients0 = { -0.16666667f, +0.0083333310f, -0.00019840874f, +2.7525562e-06f };
GGIGLOBALCONST GGiVector4 g_GGiSinCoefficients1 = { -2.3889859e-08f, -0.16665852f /*Est1*/, +0.0083139502f /*Est2*/, -0.00018524670f /*Est3*/ };
GGIGLOBALCONST GGiVector4 g_GGiCosCoefficients0 = { -0.5f, +0.041666638f, -0.0013888378f, +2.4760495e-05f };
GGIGLOBALCONST GGiVector4 g_GGiCosCoefficients1 = { -2.6051615e-07f, -0.49992746f /*Est1*/, +0.041493919f /*Est2*/, -0.0012712436f /*Est3*/ };
GGIGLOBALCONST GGiVector4 g_GGiTanCoefficients0 = { 1.0f, 0.333333333f, 0.133333333f, 5.396825397e-2f };
GGIGLOBALCONST GGiVector4 g_GGiTanCoefficients1 = { 2.186948854e-2f, 8.863235530e-3f, 3.592128167e-3f, 1.455834485e-3f };
GGIGLOBALCONST GGiVector4 g_GGiTanCoefficients2 = { 5.900274264e-4f, 2.391290764e-4f, 9.691537707e-5f, 3.927832950e-5f };
GGIGLOBALCONST GGiVector4 g_GGiArcCoefficients0 = { +1.5707963050f, -0.2145988016f, +0.0889789874f, -0.0501743046f };
GGIGLOBALCONST GGiVector4 g_GGiArcCoefficients1 = { +0.0308918810f, -0.0170881256f, +0.0066700901f, -0.0012624911f };
GGIGLOBALCONST GGiVector4 g_GGiATanCoefficients0 = { -0.3333314528f, +0.1999355085f, -0.1420889944f, +0.1065626393f };
GGIGLOBALCONST GGiVector4 g_GGiATanCoefficients1 = { -0.0752896400f, +0.0429096138f, -0.0161657367f, +0.0028662257f };
GGIGLOBALCONST GGiVector4 g_GGiATanEstCoefficients0 = { +0.999866f, +0.999866f, +0.999866f, +0.999866f };
GGIGLOBALCONST GGiVector4 g_GGiATanEstCoefficients1 = { -0.3302995f, +0.180141f, -0.085133f, +0.0208351f };
GGIGLOBALCONST GGiVector4 g_GGiTanEstCoefficients = { 2.484f, -1.954923183e-1f, 2.467401101f, GGI_1DIVPI };
GGIGLOBALCONST GGiVector4 g_GGiArcEstCoefficients = { +1.5707288f, -0.2121144f, +0.0742610f, -0.0187293f };
GGIGLOBALCONST GGiVector4 g_GGiPiConstants0 = { GGI_PI, GGI_2PI, GGI_1DIVPI, GGI_1DIV2PI };
GGIGLOBALCONST GGiVector4 g_GGiIdentityR0 = { 1.0f, 0.0f, 0.0f, 0.0f };
GGIGLOBALCONST GGiVector4 g_GGiIdentityR1 = { 0.0f, 1.0f, 0.0f, 0.0f };
GGIGLOBALCONST GGiVector4 g_GGiIdentityR2 = { 0.0f, 0.0f, 1.0f, 0.0f };
GGIGLOBALCONST GGiVector4 g_GGiIdentityR3 = { 0.0f, 0.0f, 0.0f, 1.0f };
GGIGLOBALCONST GGiVector4 g_GGiNegIdentityR0 = { -1.0f, 0.0f, 0.0f, 0.0f };
GGIGLOBALCONST GGiVector4 g_GGiNegIdentityR1 = { 0.0f, -1.0f, 0.0f, 0.0f };
GGIGLOBALCONST GGiVector4 g_GGiNegIdentityR2 = { 0.0f, 0.0f, -1.0f, 0.0f };
GGIGLOBALCONST GGiVector4 g_GGiNegIdentityR3 = { 0.0f, 0.0f, 0.0f, -1.0f };
GGIGLOBALCONST GGiVector4 g_GGiNegativeZero = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 };
GGIGLOBALCONST GGiVector4 g_GGiNegate3 = { 0x80000000, 0x80000000, 0x80000000, 0x00000000 };
GGIGLOBALCONST GGiVector4 g_GGiMaskXY = { 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000 };
GGIGLOBALCONST GGiVector4 g_GGiMask3 = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 };
GGIGLOBALCONST GGiVector4 g_GGiMaskX = { 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 };
GGIGLOBALCONST GGiVector4 g_GGiMaskY = { 0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000 };
GGIGLOBALCONST GGiVector4 g_GGiMaskZ = { 0x00000000, 0x00000000, 0xFFFFFFFF, 0x00000000 };
GGIGLOBALCONST GGiVector4 g_GGiMaskW = { 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF };
GGIGLOBALCONST GGiVector4 g_GGiOne = { 1.0f, 1.0f, 1.0f, 1.0f };
GGIGLOBALCONST GGiVector4 g_GGiOne3 = { 1.0f, 1.0f, 1.0f, 0.0f };
GGIGLOBALCONST GGiVector4 g_GGiZero = { 0.0f, 0.0f, 0.0f, 0.0f };
GGIGLOBALCONST GGiVector4 g_GGiTwo = { 2.f, 2.f, 2.f, 2.f };
GGIGLOBALCONST GGiVector4 g_GGiFour = { 4.f, 4.f, 4.f, 4.f };
GGIGLOBALCONST GGiVector4 g_GGiSix = { 6.f, 6.f, 6.f, 6.f };
GGIGLOBALCONST GGiVector4 g_GGiNegativeOne = { -1.0f, -1.0f, -1.0f, -1.0f };
GGIGLOBALCONST GGiVector4 g_GGiOneHalf = { 0.5f, 0.5f, 0.5f, 0.5f };
GGIGLOBALCONST GGiVector4 g_GGiNegativeOneHalf = { -0.5f, -0.5f, -0.5f, -0.5f };
GGIGLOBALCONST GGiVector4 g_GGiNegativeTwoPi = { -GGI_2PI, -GGI_2PI, -GGI_2PI, -GGI_2PI };
GGIGLOBALCONST GGiVector4 g_GGiNegativePi = { -GGI_PI, -GGI_PI, -GGI_PI, -GGI_PI };
GGIGLOBALCONST GGiVector4 g_GGiHalfPi = { GGI_PIDIV2, GGI_PIDIV2, GGI_PIDIV2, GGI_PIDIV2 };
GGIGLOBALCONST GGiVector4 g_GGiPi = { GGI_PI, GGI_PI, GGI_PI, GGI_PI };
GGIGLOBALCONST GGiVector4 g_GGiReciprocalPi = { GGI_1DIVPI, GGI_1DIVPI, GGI_1DIVPI, GGI_1DIVPI };
GGIGLOBALCONST GGiVector4 g_GGiTwoPi = { GGI_2PI, GGI_2PI, GGI_2PI, GGI_2PI };
GGIGLOBALCONST GGiVector4 g_GGiReciprocalTwoPi = { GGI_1DIV2PI, GGI_1DIV2PI, GGI_1DIV2PI, GGI_1DIV2PI };
GGIGLOBALCONST GGiVector4 g_GGiEpsilon = { 1.192092896e-7f, 1.192092896e-7f, 1.192092896e-7f, 1.192092896e-7f };
GGIGLOBALCONST GGiVector4 g_GGiInfinity = { 0x7F800000, 0x7F800000, 0x7F800000, 0x7F800000 };
GGIGLOBALCONST GGiVector4 g_GGiQNaN = { 0x7FC00000, 0x7FC00000, 0x7FC00000, 0x7FC00000 };
GGIGLOBALCONST GGiVector4 g_GGiQNaNTest = { 0x007FFFFF, 0x007FFFFF, 0x007FFFFF, 0x007FFFFF };
GGIGLOBALCONST GGiVector4 g_GGiAbsMask = { 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF };
GGIGLOBALCONST GGiVector4 g_GGiFltMin = { 0x00800000, 0x00800000, 0x00800000, 0x00800000 };
GGIGLOBALCONST GGiVector4 g_GGiFltMax = { 0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF };
GGIGLOBALCONST GGiVector4 g_GGiNegOneMask = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
GGIGLOBALCONST GGiVector4 g_GGiMaskA8R8G8B8 = { 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000 };
GGIGLOBALCONST GGiVector4 g_GGiFlipA8R8G8B8 = { 0x00000000, 0x00000000, 0x00000000, 0x80000000 };
GGIGLOBALCONST GGiVector4 g_GGiFixAA8R8G8B8 = { 0.0f, 0.0f, 0.0f, float(0x80000000U) };
GGIGLOBALCONST GGiVector4 g_GGiNormalizeA8R8G8B8 = { 1.0f / (255.0f*float(0x10000)), 1.0f / (255.0f*float(0x100)), 1.0f / 255.0f, 1.0f / (255.0f*float(0x1000000)) };
GGIGLOBALCONST GGiVector4 g_GGiMaskA2B10G10R10 = { 0x000003FF, 0x000FFC00, 0x3FF00000, 0xC0000000 };
GGIGLOBALCONST GGiVector4 g_GGiFlipA2B10G10R10 = { 0x00000200, 0x00080000, 0x20000000, 0x80000000 };
GGIGLOBALCONST GGiVector4 g_GGiFixAA2B10G10R10 = { -512.0f, -512.0f*float(0x400), -512.0f*float(0x100000), float(0x80000000U) };
GGIGLOBALCONST GGiVector4 g_GGiNormalizeA2B10G10R10 = { 1.0f / 511.0f, 1.0f / (511.0f*float(0x400)), 1.0f / (511.0f*float(0x100000)), 1.0f / (3.0f*float(0x40000000)) };
GGIGLOBALCONST GGiVector4 g_GGiMaskX16Y16 = { 0x0000FFFF, 0xFFFF0000, 0x00000000, 0x00000000 };
GGIGLOBALCONST GGiVector4 g_GGiFlipX16Y16 = { 0x00008000, 0x00000000, 0x00000000, 0x00000000 };
GGIGLOBALCONST GGiVector4 g_GGiFixX16Y16 = { -32768.0f, 0.0f, 0.0f, 0.0f };
GGIGLOBALCONST GGiVector4 g_GGiNormalizeX16Y16 = { 1.0f / 32767.0f, 1.0f / (32767.0f*65536.0f), 0.0f, 0.0f };
GGIGLOBALCONST GGiVector4 g_GGiMaskX16Y16Z16W16 = { 0x0000FFFF, 0x0000FFFF, 0xFFFF0000, 0xFFFF0000 };
GGIGLOBALCONST GGiVector4 g_GGiFlipX16Y16Z16W16 = { 0x00008000, 0x00008000, 0x00000000, 0x00000000 };
GGIGLOBALCONST GGiVector4 g_GGiFixX16Y16Z16W16 = { -32768.0f, -32768.0f, 0.0f, 0.0f };
GGIGLOBALCONST GGiVector4 g_GGiNormalizeX16Y16Z16W16 = { 1.0f / 32767.0f, 1.0f / 32767.0f, 1.0f / (32767.0f*65536.0f), 1.0f / (32767.0f*65536.0f) };
GGIGLOBALCONST GGiVector4 g_GGiNoFraction = { 8388608.0f, 8388608.0f, 8388608.0f, 8388608.0f };
GGIGLOBALCONST GGiVector4 g_GGiMaskByte = { 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF };
GGIGLOBALCONST GGiVector4 g_GGiNegateX = { -1.0f, 1.0f, 1.0f, 1.0f };
GGIGLOBALCONST GGiVector4 g_GGiNegateY = { 1.0f, -1.0f, 1.0f, 1.0f };
GGIGLOBALCONST GGiVector4 g_GGiNegateZ = { 1.0f, 1.0f, -1.0f, 1.0f };
GGIGLOBALCONST GGiVector4 g_GGiNegateW = { 1.0f, 1.0f, 1.0f, -1.0f };
GGIGLOBALCONST GGiVector4 g_GGiSelect0101 = { GGI_SELECT_0, GGI_SELECT_1, GGI_SELECT_0, GGI_SELECT_1 };
GGIGLOBALCONST GGiVector4 g_GGiSelect1010 = { GGI_SELECT_1, GGI_SELECT_0, GGI_SELECT_1, GGI_SELECT_0 };
GGIGLOBALCONST GGiVector4 g_GGiOneHalfMinusEpsilon = { 0x3EFFFFFD, 0x3EFFFFFD, 0x3EFFFFFD, 0x3EFFFFFD };
GGIGLOBALCONST GGiVector4 g_GGiSelect1000 = { GGI_SELECT_1, GGI_SELECT_0, GGI_SELECT_0, GGI_SELECT_0 };
GGIGLOBALCONST GGiVector4 g_GGiSelect1100 = { GGI_SELECT_1, GGI_SELECT_1, GGI_SELECT_0, GGI_SELECT_0 };
GGIGLOBALCONST GGiVector4 g_GGiSelect1110 = { GGI_SELECT_1, GGI_SELECT_1, GGI_SELECT_1, GGI_SELECT_0 };
GGIGLOBALCONST GGiVector4 g_GGiSelect1011 = { GGI_SELECT_1, GGI_SELECT_0, GGI_SELECT_1, GGI_SELECT_1 };
GGIGLOBALCONST GGiVector4 g_GGiFixupY16 = { 1.0f, 1.0f / 65536.0f, 0.0f, 0.0f };
GGIGLOBALCONST GGiVector4 g_GGiFixupY16W16 = { 1.0f, 1.0f, 1.0f / 65536.0f, 1.0f / 65536.0f };
GGIGLOBALCONST GGiVector4 g_GGiFlipY = { 0, 0x80000000, 0, 0 };
GGIGLOBALCONST GGiVector4 g_GGiFlipZ = { 0, 0, 0x80000000, 0 };
GGIGLOBALCONST GGiVector4 g_GGiFlipW = { 0, 0, 0, 0x80000000 };
GGIGLOBALCONST GGiVector4 g_GGiFlipYZ = { 0, 0x80000000, 0x80000000, 0 };
GGIGLOBALCONST GGiVector4 g_GGiFlipZW = { 0, 0, 0x80000000, 0x80000000 };
GGIGLOBALCONST GGiVector4 g_GGiFlipYW = { 0, 0x80000000, 0, 0x80000000 };
GGIGLOBALCONST GGiVector4 g_GGiMaskDec4 = { 0x3FF, 0x3FF << 10, 0x3FF << 20, static_cast<int>(0xC0000000) };
GGIGLOBALCONST GGiVector4 g_GGiXorDec4 = { 0x200, 0x200 << 10, 0x200 << 20, 0 };
GGIGLOBALCONST GGiVector4 g_GGiAddUDec4 = { 0, 0, 0, 32768.0f*65536.0f };
GGIGLOBALCONST GGiVector4 g_GGiAddDec4 = { -512.0f, -512.0f*1024.0f, -512.0f*1024.0f*1024.0f, 0 };
GGIGLOBALCONST GGiVector4 g_GGiMulDec4 = { 1.0f, 1.0f / 1024.0f, 1.0f / (1024.0f*1024.0f), 1.0f / (1024.0f*1024.0f*1024.0f) };
GGIGLOBALCONST GGiVector4 g_GGiMaskByte4 = { 0xFF, 0xFF00, 0xFF0000, 0xFF000000 };
GGIGLOBALCONST GGiVector4 g_GGiXorByte4 = { 0x80, 0x8000, 0x800000, 0x00000000 };
GGIGLOBALCONST GGiVector4 g_GGiAddByte4 = { -128.0f, -128.0f*256.0f, -128.0f*65536.0f, 0 };
GGIGLOBALCONST GGiVector4 g_GGiFixUnsigned = { 32768.0f*65536.0f, 32768.0f*65536.0f, 32768.0f*65536.0f, 32768.0f*65536.0f };
GGIGLOBALCONST GGiVector4 g_GGiMaxInt = { 65536.0f*32768.0f - 128.0f, 65536.0f*32768.0f - 128.0f, 65536.0f*32768.0f - 128.0f, 65536.0f*32768.0f - 128.0f };
GGIGLOBALCONST GGiVector4 g_GGiMaxUInt = { 65536.0f*65536.0f - 256.0f, 65536.0f*65536.0f - 256.0f, 65536.0f*65536.0f - 256.0f, 65536.0f*65536.0f - 256.0f };
GGIGLOBALCONST GGiVector4 g_GGiUnsignedFix = { 32768.0f*65536.0f, 32768.0f*65536.0f, 32768.0f*65536.0f, 32768.0f*65536.0f };
GGIGLOBALCONST GGiVector4 g_GGisrgbScale = { 12.92f, 12.92f, 12.92f, 1.0f };
GGIGLOBALCONST GGiVector4 g_GGisrgbA = { 0.055f, 0.055f, 0.055f, 0.0f };
GGIGLOBALCONST GGiVector4 g_GGisrgbA1 = { 1.055f, 1.055f, 1.055f, 1.0f };
GGIGLOBALCONST GGiVector4 g_GGiExponentBias = { 127, 127, 127, 127 };
GGIGLOBALCONST GGiVector4 g_GGiSubnormalExponent = { -126, -126, -126, -126 };
GGIGLOBALCONST GGiVector4 g_GGiNumTrailing = { 23, 23, 23, 23 };
GGIGLOBALCONST GGiVector4 g_GGiMinNormal = { 0x00800000, 0x00800000, 0x00800000, 0x00800000 };
GGIGLOBALCONST GGiVector4 g_GGiNegInfinity = { 0xFF800000, 0xFF800000, 0xFF800000, 0xFF800000 };
GGIGLOBALCONST GGiVector4 g_GGiNegQNaN = { 0xFFC00000, 0xFFC00000, 0xFFC00000, 0xFFC00000 };
GGIGLOBALCONST GGiVector4 g_GGiBin128 = { 0x43000000, 0x43000000, 0x43000000, 0x43000000 };
GGIGLOBALCONST GGiVector4 g_GGiBinNeg150 = { 0xC3160000, 0xC3160000, 0xC3160000, 0xC3160000 };
GGIGLOBALCONST GGiVector4 g_GGi253 = { 253, 253, 253, 253 };
GGIGLOBALCONST GGiVector4 g_GGiExpEst1 = { -6.93147182e-1f, -6.93147182e-1f, -6.93147182e-1f, -6.93147182e-1f };
GGIGLOBALCONST GGiVector4 g_GGiExpEst2 = { +2.40226462e-1f, +2.40226462e-1f, +2.40226462e-1f, +2.40226462e-1f };
GGIGLOBALCONST GGiVector4 g_GGiExpEst3 = { -5.55036440e-2f, -5.55036440e-2f, -5.55036440e-2f, -5.55036440e-2f };
GGIGLOBALCONST GGiVector4 g_GGiExpEst4 = { +9.61597636e-3f, +9.61597636e-3f, +9.61597636e-3f, +9.61597636e-3f };
GGIGLOBALCONST GGiVector4 g_GGiExpEst5 = { -1.32823968e-3f, -1.32823968e-3f, -1.32823968e-3f, -1.32823968e-3f };
GGIGLOBALCONST GGiVector4 g_GGiExpEst6 = { +1.47491097e-4f, +1.47491097e-4f, +1.47491097e-4f, +1.47491097e-4f };
GGIGLOBALCONST GGiVector4 g_GGiExpEst7 = { -1.08635004e-5f, -1.08635004e-5f, -1.08635004e-5f, -1.08635004e-5f };
GGIGLOBALCONST GGiVector4 g_GGiLogEst0 = { +1.442693f, +1.442693f, +1.442693f, +1.442693f };
GGIGLOBALCONST GGiVector4 g_GGiLogEst1 = { -0.721242f, -0.721242f, -0.721242f, -0.721242f };
GGIGLOBALCONST GGiVector4 g_GGiLogEst2 = { +0.479384f, +0.479384f, +0.479384f, +0.479384f };
GGIGLOBALCONST GGiVector4 g_GGiLogEst3 = { -0.350295f, -0.350295f, -0.350295f, -0.350295f };
GGIGLOBALCONST GGiVector4 g_GGiLogEst4 = { +0.248590f, +0.248590f, +0.248590f, +0.248590f };
GGIGLOBALCONST GGiVector4 g_GGiLogEst5 = { -0.145700f, -0.145700f, -0.145700f, -0.145700f };
GGIGLOBALCONST GGiVector4 g_GGiLogEst6 = { +0.057148f, +0.057148f, +0.057148f, +0.057148f };
GGIGLOBALCONST GGiVector4 g_GGiLogEst7 = { -0.010578f, -0.010578f, -0.010578f, -0.010578f };
GGIGLOBALCONST GGiVector4 g_GGiLgE = { +1.442695f, +1.442695f, +1.442695f, +1.442695f };
GGIGLOBALCONST GGiVector4 g_GGiInvLgE = { +6.93147182e-1f, +6.93147182e-1f, +6.93147182e-1f, +6.93147182e-1f };
GGIGLOBALCONST GGiVector4 g_UByteMax = { 255.0f, 255.0f, 255.0f, 255.0f };
GGIGLOBALCONST GGiVector4 g_ByteMin = { -127.0f, -127.0f, -127.0f, -127.0f };
GGIGLOBALCONST GGiVector4 g_ByteMax = { 127.0f, 127.0f, 127.0f, 127.0f };
GGIGLOBALCONST GGiVector4 g_ShortMin = { -32767.0f, -32767.0f, -32767.0f, -32767.0f };
GGIGLOBALCONST GGiVector4 g_ShortMax = { 32767.0f, 32767.0f, 32767.0f, 32767.0f };
GGIGLOBALCONST GGiVector4 g_UShortMax = { 65535.0f, 65535.0f, 65535.0f, 65535.0f };

namespace GGiMath
{

	// Slow path fallback for permutes that do not map to a single SSE shuffle opcode.
	template<uint32_t Shuffle, bool WhichX, bool WhichY, bool WhichZ, bool WhichW> struct PermuteHelper
	{
		static GGiVector4 _GGI_VECTOR_CALL Permute(GGiVector4_F v1, GGiVector4_F v2)
		{
			static const __m128 selectMask =
			{
				WhichX ? 0xFFFFFFFF : 0,
				WhichY ? 0xFFFFFFFF : 0,
				WhichZ ? 0xFFFFFFFF : 0,
				WhichW ? 0xFFFFFFFF : 0,
			};

			GGiVector4 shuffled1 = _GGI_SHUFFLE_PS(v1, Shuffle);
			GGiVector4 shuffled2 = _GGI_SHUFFLE_PS(v2, Shuffle);

			GGiVector4 masked1 = _mm_andnot_ps(selectMask, shuffled1);
			GGiVector4 masked2 = _mm_and_ps(selectMask, shuffled2);

			return _mm_or_ps(masked1, masked2);
		}
	};

	class GGiMathHelper
	{
	public:

		static __forceinline GGiVector4 QuaternionRotationFromPitchYawRollVector
		(
			__m128 Angles // <Pitch, Yaw, Roll, 0>
		)
		{
			static const __m128  Sign = { 1.0f, -1.0f, -1.0f, 1.0f };
			static const __m128 OneHalf = { 0.5f, 0.5f, 0.5f, 0.5f };

			__m128 HalfAngles = _mm_mul_ps(Angles, OneHalf);

			__m128 SinAngles, CosAngles;
			GGiVectorSinCos(&SinAngles, &CosAngles, HalfAngles);

			__m128 P0 = GGiVectorPermute<GGI_PERMUTE_0X, GGI_PERMUTE_1X, GGI_PERMUTE_1X, GGI_PERMUTE_1X>(SinAngles, CosAngles);
			__m128 Y0 = GGiVectorPermute<GGI_PERMUTE_1Y, GGI_PERMUTE_0Y, GGI_PERMUTE_1Y, GGI_PERMUTE_1Y>(SinAngles, CosAngles);
			__m128 R0 = GGiVectorPermute<GGI_PERMUTE_1Z, GGI_PERMUTE_1Z, GGI_PERMUTE_0Z, GGI_PERMUTE_1Z>(SinAngles, CosAngles);
			__m128 P1 = GGiVectorPermute<GGI_PERMUTE_0X, GGI_PERMUTE_1X, GGI_PERMUTE_1X, GGI_PERMUTE_1X>(CosAngles, SinAngles);
			__m128 Y1 = GGiVectorPermute<GGI_PERMUTE_1Y, GGI_PERMUTE_0Y, GGI_PERMUTE_1Y, GGI_PERMUTE_1Y>(CosAngles, SinAngles);
			__m128 R1 = GGiVectorPermute<GGI_PERMUTE_1Z, GGI_PERMUTE_1Z, GGI_PERMUTE_0Z, GGI_PERMUTE_1Z>(CosAngles, SinAngles);

			__m128 Q1 = _mm_mul_ps(P1, Sign);
			__m128 Q0 = _mm_mul_ps(P0, Y0);
			Q1 = _mm_mul_ps(Q1, Y1);
			Q0 = _mm_mul_ps(Q0, R0);
			__m128 Q = _mm_add_ps(_mm_mul_ps(Q1, R1), Q0);

			return Q;
		}

		static inline void _GGI_VECTOR_CALL GGiVectorSinCos
		(
			__m128* pSin,
			__m128* pCos,
			GGiVector4_F V
		)
		{
			assert(pSin != nullptr);
			assert(pCos != nullptr);

			// Force the value within the bounds of pi
			GGiVector4 x = GGiVectorModAngles(V);

			// Map in [-pi/2,pi/2] with sin(y) = sin(x), cos(y) = sign*cos(x).
			GGiVector4 sign = _mm_and_ps(x, g_GGiNegativeZero);
			__m128 c = _mm_or_ps(g_GGiPi, sign);  // pi when x >= 0, -pi when x < 0
			__m128 absx = _mm_andnot_ps(sign, x);  // |x|
			__m128 rflx = _mm_sub_ps(c, x);
			__m128 comp = _mm_cmple_ps(absx, g_GGiHalfPi);
			__m128 select0 = _mm_and_ps(comp, x);
			__m128 select1 = _mm_andnot_ps(comp, rflx);
			x = _mm_or_ps(select0, select1);
			select0 = _mm_and_ps(comp, g_GGiOne);
			select1 = _mm_andnot_ps(comp, g_GGiNegativeOne);
			sign = _mm_or_ps(select0, select1);

			__m128 x2 = _mm_mul_ps(x, x);

			// Compute polynomial approximation of sine
			const GGiVector4 SC1 = g_GGiSinCoefficients1;
			GGiVector4 vConstants = _GGI_SHUFFLE_PS(SC1, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 Result = _mm_mul_ps(vConstants, x2);

			const GGiVector4 SC0 = g_GGiSinCoefficients0;
			vConstants = _GGI_SHUFFLE_PS(SC0, _MM_SHUFFLE(3, 3, 3, 3));
			Result = _mm_add_ps(Result, vConstants);
			Result = _mm_mul_ps(Result, x2);

			vConstants = _GGI_SHUFFLE_PS(SC0, _MM_SHUFFLE(2, 2, 2, 2));
			Result = _mm_add_ps(Result, vConstants);
			Result = _mm_mul_ps(Result, x2);

			vConstants = _GGI_SHUFFLE_PS(SC0, _MM_SHUFFLE(1, 1, 1, 1));
			Result = _mm_add_ps(Result, vConstants);
			Result = _mm_mul_ps(Result, x2);

			vConstants = _GGI_SHUFFLE_PS(SC0, _MM_SHUFFLE(0, 0, 0, 0));
			Result = _mm_add_ps(Result, vConstants);
			Result = _mm_mul_ps(Result, x2);
			Result = _mm_add_ps(Result, g_GGiOne);
			Result = _mm_mul_ps(Result, x);
			*pSin = Result;

			// Compute polynomial approximation of cosine
			const GGiVector4 CC1 = g_GGiCosCoefficients1;
			vConstants = _GGI_SHUFFLE_PS(CC1, _MM_SHUFFLE(0, 0, 0, 0));
			Result = _mm_mul_ps(vConstants, x2);

			const GGiVector4 CC0 = g_GGiCosCoefficients0;
			vConstants = _GGI_SHUFFLE_PS(CC0, _MM_SHUFFLE(3, 3, 3, 3));
			Result = _mm_add_ps(Result, vConstants);
			Result = _mm_mul_ps(Result, x2);

			vConstants = _GGI_SHUFFLE_PS(CC0, _MM_SHUFFLE(2, 2, 2, 2));
			Result = _mm_add_ps(Result, vConstants);
			Result = _mm_mul_ps(Result, x2);

			vConstants = _GGI_SHUFFLE_PS(CC0, _MM_SHUFFLE(1, 1, 1, 1));
			Result = _mm_add_ps(Result, vConstants);
			Result = _mm_mul_ps(Result, x2);

			vConstants = _GGI_SHUFFLE_PS(CC0, _MM_SHUFFLE(0, 0, 0, 0));
			Result = _mm_add_ps(Result, vConstants);
			Result = _mm_mul_ps(Result, x2);
			Result = _mm_add_ps(Result, g_GGiOne);
			Result = _mm_mul_ps(Result, sign);
			*pCos = Result;
		}

		static inline void _GGI_VECTOR_CALL GGiScalarSinCos
		(
			float* pSin,
			float* pCos,
			float  Value
		)
		{
			assert(pSin);
			assert(pCos);

			// Map Value to y in [-pi,pi], x = 2*pi*quotient + remainder.
			float quotient = GGI_1DIV2PI * Value;
			if (Value >= 0.0f)
			{
				quotient = static_cast<float>(static_cast<int>(quotient + 0.5f));
			}
			else
			{
				quotient = static_cast<float>(static_cast<int>(quotient - 0.5f));
			}
			float y = Value - GGI_2PI * quotient;

			// Map y to [-pi/2,pi/2] with sin(y) = sin(Value).
			float sign;
			if (y > GGI_PIDIV2)
			{
				y = GGI_PI - y;
				sign = -1.0f;
			}
			else if (y < -GGI_PIDIV2)
			{
				y = -GGI_PI - y;
				sign = -1.0f;
			}
			else
			{
				sign = +1.0f;
			}

			float y2 = y * y;

			// 11-degree minimax approximation
			*pSin = (((((-2.3889859e-08f * y2 + 2.7525562e-06f) * y2 - 0.00019840874f) * y2 + 0.0083333310f) * y2 - 0.16666667f) * y2 + 1.0f) * y;

			// 10-degree minimax approximation
			float p = ((((-2.6051615e-07f * y2 + 2.4760495e-05f) * y2 - 0.0013888378f) * y2 + 0.041666638f) * y2 - 0.5f) * y2 + 1.0f;
			*pCos = sign * p;
		}

		static inline GGiVector4 _GGI_VECTOR_CALL GGiVectorModAngles
		(
			GGiVector4_F Angles
		)
		{
			// Modulo the range of the given angles such that -GGI_PI <= Angles < GGI_PI
			GGiVector4 vResult = _mm_mul_ps(Angles, g_GGiReciprocalTwoPi);
			// Use the inline function due to complexity for rounding
			vResult = GGiVectorRound(vResult);
			vResult = _mm_mul_ps(vResult, g_GGiTwoPi);
			vResult = _mm_sub_ps(Angles, vResult);
			return vResult;
		}

		static inline GGiVector4 _GGI_VECTOR_CALL GGiVectorRound
		(
			GGiVector4_F V
		)
		{
			__m128 sign = _mm_and_ps(V, g_GGiNegativeZero);
			__m128 sMagic = _mm_or_ps(g_GGiNoFraction, sign);
			__m128 R1 = _mm_add_ps(V, sMagic);
			R1 = _mm_sub_ps(R1, sMagic);
			__m128 R2 = _mm_and_ps(V, g_GGiAbsMask);
			__m128 mask = _mm_cmple_ps(R2, g_GGiNoFraction);
			R2 = _mm_andnot_ps(mask, V);
			R1 = _mm_and_ps(R1, mask);
			GGiVector4 vResult = _mm_xor_ps(R1, R2);
			return vResult;
		}

		// General permute template
		template<uint32_t PermuteX, uint32_t PermuteY, uint32_t PermuteZ, uint32_t PermuteW>
		inline static GGiVector4 _GGI_VECTOR_CALL GGiVectorPermute(GGiVector4_F V1, GGiVector4_F V2)
		{
			static_assert(PermuteX <= 7, "PermuteX template parameter out of range");
			static_assert(PermuteY <= 7, "PermuteY template parameter out of range");
			static_assert(PermuteZ <= 7, "PermuteZ template parameter out of range");
			static_assert(PermuteW <= 7, "PermuteW template parameter out of range");

			const uint32_t Shuffle = _MM_SHUFFLE(PermuteW & 3, PermuteZ & 3, PermuteY & 3, PermuteX & 3);

			const bool WhichX = PermuteX > 3;
			const bool WhichY = PermuteY > 3;
			const bool WhichZ = PermuteZ > 3;
			const bool WhichW = PermuteW > 3;

			return PermuteHelper<Shuffle, WhichX, WhichY, WhichZ, WhichW>::Permute(V1, V2);
		}
	};

	// General swizzle template
	template<uint32_t SwizzleX, uint32_t SwizzleY, uint32_t SwizzleZ, uint32_t SwizzleW>
	inline GGiVector4 _GGI_VECTOR_CALL GGiVectorSwizzle(GGiVector4_F V)
	{
		static_assert(SwizzleX <= 3, "SwizzleX template parameter out of range");
		static_assert(SwizzleY <= 3, "SwizzleY template parameter out of range");
		static_assert(SwizzleZ <= 3, "SwizzleZ template parameter out of range");
		static_assert(SwizzleW <= 3, "SwizzleW template parameter out of range");

		return _GGI_SHUFFLE_PS(V, _MM_SHUFFLE(SwizzleW, SwizzleZ, SwizzleY, SwizzleX));
	}

	//------------------------------------------------------------------------------

	inline bool GGiScalarNearEqual
	(
		float S1,
		float S2,
		float Epsilon
	)
	{
		float Delta = S1 - S2;
		return (fabsf(Delta) <= Epsilon);
	}

	//------------------------------------------------------------------------------

	// Initialize a vector with four floating point values
	inline GGiVector4 _GGI_VECTOR_CALL GGiVectorSet
	(
		float x,
		float y,
		float z,
		float w
	)
	{
		return _mm_set_ps(w, z, y, x);
	}

	//------------------------------------------------------------------------------

	inline GGiVector4 _GGI_VECTOR_CALL GGiLoadFloat3
	(
		float floatX,
		float floatY,
		float floatZ
	)
	{
		__m128 x = _mm_load_ss(&floatX);
		__m128 y = _mm_load_ss(&floatY);
		__m128 z = _mm_load_ss(&floatZ);
		__m128 xy = _mm_unpacklo_ps(x, y);
		return _mm_movelh_ps(xy, z);
	}

	//------------------------------------------------------------------------------

	inline bool _GGI_VECTOR_CALL GGiVector3Equal
	(
		GGiVector4_F V1,
		GGiVector4_F V2
	)
	{
		GGiVector4 vTemp = _mm_cmpeq_ps(V1, V2);
		return (((_mm_movemask_ps(vTemp) & 7) == 7) != 0);
	}

	//------------------------------------------------------------------------------

	inline bool _GGI_VECTOR_CALL GGiVector3IsInfinite
	(
		GGiVector4_F V
	)
	{
		// Mask off the sign bit
		__m128 vTemp = _mm_and_ps(V, g_GGiAbsMask);
		// Compare to infinity
		vTemp = _mm_cmpeq_ps(vTemp, g_GGiInfinity);
		// If x,y or z are infinity, the signs are true.
		return ((_mm_movemask_ps(vTemp) & 7) != 0);
	}

	//------------------------------------------------------------------------------
	
	// Return a vector with all elements equaling zero
	inline GGiVector4 _GGI_VECTOR_CALL GGiVectorZero()
	{
		return _mm_setzero_ps();
	}

	//------------------------------------------------------------------------------

	inline GGiVector4 _GGI_VECTOR_CALL GGiVector3Normalize
	(
		GGiVector4_F V
	)
	{
		// Perform the dot product on x,y and z only
		GGiVector4 vLengthSq = _mm_mul_ps(V, V);
		GGiVector4 vTemp = _GGI_SHUFFLE_PS(vLengthSq, _MM_SHUFFLE(2, 1, 2, 1));
		vLengthSq = _mm_add_ss(vLengthSq, vTemp);
		vTemp = _GGI_SHUFFLE_PS(vTemp, _MM_SHUFFLE(1, 1, 1, 1));
		vLengthSq = _mm_add_ss(vLengthSq, vTemp);
		vLengthSq = _GGI_SHUFFLE_PS(vLengthSq, _MM_SHUFFLE(0, 0, 0, 0));
		// Prepare for the division
		GGiVector4 vResult = _mm_sqrt_ps(vLengthSq);
		// Create zero with a single instruction
		GGiVector4 vZeroMask = _mm_setzero_ps();
		// Test for a divide by zero (Must be FP to detect -0.0)
		vZeroMask = _mm_cmpneq_ps(vZeroMask, vResult);
		// Failsafe on zero (Or epsilon) length planes
		// If the length is infinity, set the elements to zero
		vLengthSq = _mm_cmpneq_ps(vLengthSq, g_GGiInfinity);
		// Divide to perform the normalization
		vResult = _mm_div_ps(V, vResult);
		// Any that are infinity, set to zero
		vResult = _mm_and_ps(vResult, vZeroMask);
		// Select qnan or result based on infinite length
		GGiVector4 vTemp1 = _mm_andnot_ps(vLengthSq, g_GGiQNaN);
		GGiVector4 vTemp2 = _mm_and_ps(vResult, vLengthSq);
		vResult = _mm_or_ps(vTemp1, vTemp2);
		return vResult;
	}

	//------------------------------------------------------------------------------

	inline GGiVector4 _GGI_VECTOR_CALL GGiVectorMultiply
	(
		GGiVector4_F V1,
		GGiVector4_F V2
	)
	{
		return _mm_mul_ps(V1, V2);
	}

	//------------------------------------------------------------------------------

	inline GGiVector4 _GGI_VECTOR_CALL GGiVectorNegativeMultiplySubtract
	(
		GGiVector4_F V1,
		GGiVector4_F V2,
		GGiVector4_F V3
	)
	{
		GGiVector4 R = _mm_mul_ps(V1, V2);
		return _mm_sub_ps(V3, R);
	}

	//------------------------------------------------------------------------------

	inline GGiVector4 _GGI_VECTOR_CALL GGiVectorMultiplyAdd
	(
		GGiVector4_F V1,
		GGiVector4_F V2,
		GGiVector4_F V3
	)
	{
		GGiVector4 vResult = _mm_mul_ps(V1, V2);
		return _mm_add_ps(vResult, V3);
	}

	//------------------------------------------------------------------------------

	inline GGiVector4 _GGI_VECTOR_CALL GGiVector4Dot
	(
		GGiVector4_F V1,
		GGiVector4_F V2
	)
	{
		GGiVector4 vTemp2 = V2;
		GGiVector4 vTemp = _mm_mul_ps(V1, vTemp2);
		vTemp2 = _mm_shuffle_ps(vTemp2, vTemp, _MM_SHUFFLE(1, 0, 0, 0)); // Copy X to the Z position and Y to the W position
		vTemp2 = _mm_add_ps(vTemp2, vTemp);          // Add Z = X+Z; W = Y+W;
		vTemp = _mm_shuffle_ps(vTemp, vTemp2, _MM_SHUFFLE(0, 3, 0, 0));  // Copy W to the Z position
		vTemp = _mm_add_ps(vTemp, vTemp2);           // Add Z and W together
		return _GGI_SHUFFLE_PS(vTemp, _MM_SHUFFLE(2, 2, 2, 2));    // Splat Z and return
	}

}





