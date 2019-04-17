#pragma once
#include "GDxPreInclude.h"



class GDxFloat4x4 : public GGiFloat4x4
{

public:

	GDxFloat4x4();
	~GDxFloat4x4();

	virtual GGiFloat4x4& operator =(GGiFloat4x4& mat) override;

	virtual GGiFloat4x4& operator *(GGiFloat4x4& mat) override;

	virtual GGiFloat4x4& operator +(GGiFloat4x4& mat) override;

	virtual void SetByTranslation(float x, float y, float z) override;

	virtual void SetByRotationPitchYawRoll(float pitch, float yaw, float roll) override;

	virtual void SetByScale(float x, float y, float z) override;

	static GGiFloat4x4* Identity4x4();

	DirectX::XMFLOAT4X4 GetValue();

	void SetValue(DirectX::XMFLOAT4X4 v);

private:

	DirectX::XMFLOAT4X4 value;

};

