#include "stdafx.h"
#include "GDxFloat4x4.h"


GDxFloat4x4::GDxFloat4x4()
{
	static DirectX::XMFLOAT4X4 Id(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	value = Id;
}


GDxFloat4x4::~GDxFloat4x4()
{
}

GGiFloat4x4& GDxFloat4x4::Identity4x4()
{
	GDxFloat4x4 ret;

	static DirectX::XMFLOAT4X4 I(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

	ret.value = I;

	return ret;
}

GGiFloat4x4& GDxFloat4x4::operator =(GGiFloat4x4& mat)
{
	if (this != &mat)
	{
		GDxFloat4x4 dxMat = dynamic_cast<GDxFloat4x4&>(mat);
		value = dxMat.value;
	}
	return *this;
}

GGiFloat4x4& GDxFloat4x4::operator *(GGiFloat4x4& mat)
{
	GDxFloat4x4 dxMat = dynamic_cast<GDxFloat4x4&>(mat);

	DirectX::XMMATRIX fac1 = DirectX::XMLoadFloat4x4(&dxMat.value);
	DirectX::XMMATRIX fac2 = DirectX::XMLoadFloat4x4(&value);
	DirectX::XMMATRIX pro = fac1 * fac2;

	DirectX::XMStoreFloat4x4(&value, pro);
	
	return *this;
}

GGiFloat4x4& GDxFloat4x4::operator +(GGiFloat4x4& mat)
{
	GDxFloat4x4 dxMat = dynamic_cast<GDxFloat4x4&>(mat);

	DirectX::XMMATRIX add1 = DirectX::XMLoadFloat4x4(&dxMat.value);
	DirectX::XMMATRIX add2 = DirectX::XMLoadFloat4x4(&value);
	DirectX::XMMATRIX sum = add1 + add2;

	DirectX::XMStoreFloat4x4(&value, sum);

	return *this;
}