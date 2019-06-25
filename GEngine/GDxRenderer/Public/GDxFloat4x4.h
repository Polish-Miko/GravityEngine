#pragma once
#include "GDxPreInclude.h"


/*
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

	virtual void SetByPerspectiveFovLH(float fovAngleY, float aspectRatio, float nearZ, float farZ) override;

	virtual void SetByRotationAxis(float axisX, float axisY, float axisZ, float angle) override;

	virtual void SetByRotationY(float angle) override;

	virtual std::vector<float> TransformNormal(std::vector<float> vec) override;

	static GGiFloat4x4* Identity4x4();

	DirectX::XMFLOAT4X4 GetValue();

	void SetValue(DirectX::XMFLOAT4X4 v);

	virtual float GetElement(int i, int j) override;

	virtual void SetElement(int i, int j, float v) override;

	virtual void Transpose() override;

	virtual GGiFloat4x4* GetInverse() override;

private:

	DirectX::XMFLOAT4X4 value;

};
*/

