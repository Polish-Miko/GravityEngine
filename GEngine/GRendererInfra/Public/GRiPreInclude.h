#pragma once

#include <windows.h>
#include <wrl.h>
//#include <dxgi1_4.h>
//#include <d3d12.h>
//#include <D3Dcompiler.h>
//#include <DirectXMath.h>
#include <math.h>
//#include <DirectXPackedVector.h>
//#include <DirectXColors.h>
//#include <DirectXCollision.h>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <cassert>
//#include "d3dx12.h"
//#include "DDSTextureLoader.h"
//#include "MathHelper.h"
//#include "ResourceUploadBatch.h"
//#include <WICTextureLoader.h>
#include <fbxsdk.h>
#include "GGiInclude.h"


#define NUM_FRAME_RESOURCES 3

enum class RenderLayer : int
{
	Deferred = 0,
	Debug,
	Sky,
	ScreenQuad,
	Count
};

struct ProfileData
{
	std::string name;
	float time;
};


