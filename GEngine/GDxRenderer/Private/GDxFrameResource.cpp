#include "stdafx.h"
#include "GDxFrameResource.h"

GDxFrameResource::GDxFrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

	PassCB = std::make_unique<GDxUploadBuffer<PassConstants>>(device, passCount, true);
	SsaoCB = std::make_unique<GDxUploadBuffer<SsaoConstants>>(device, 1, true);
	MaterialBuffer = std::make_unique<GDxUploadBuffer<MaterialData>>(device, materialCount, false);
	SceneObjectSdfDescriptorBuffer = std::make_unique<GDxUploadBuffer<SceneObjectSdfDescriptor>>(device, MAX_SCENE_OBJECT_NUM, false);
	ObjectCB = std::make_unique<GDxUploadBuffer<ObjectConstants>>(device, objectCount, true);
	LightCB = std::make_unique<GDxUploadBuffer<LightConstants>>(device, 1, true);
	SkyCB = std::make_unique<GDxUploadBuffer<SkyPassConstants>>(device, 1, true);
}

GDxFrameResource::~GDxFrameResource()
{

}