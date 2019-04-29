#include "stdafx.h"
#include "GDxFloat4x4.h"


GDxFloat4x4::GDxFloat4x4()
{
	DirectX::XMFLOAT4X4 Id(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	value = Id;
}


GDxFloat4x4::~GDxFloat4x4()
{
}

DirectX::XMFLOAT4X4 GDxFloat4x4::GetValue()
{
	return value;
}

void GDxFloat4x4::SetValue(DirectX::XMFLOAT4X4 v)
{
	value = v;
}

float GDxFloat4x4::GetElement(int i, int j)
{
	return value.m[i][j];
}

void GDxFloat4x4::SetElement(int i, int j, float v)
{
	value.m[i][j] = v;
}

void GDxFloat4x4::Transpose()
{
	DirectX::XMStoreFloat4x4(&value, DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&value)));
}

GGiFloat4x4* GDxFloat4x4::Identity4x4()
{
	GDxFloat4x4* ret = new GDxFloat4x4();

	DirectX::XMFLOAT4X4 I(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

	ret->value = I;

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

	GDxFloat4x4* ret = new GDxFloat4x4();
	DirectX::XMStoreFloat4x4(&(ret->value), pro);
	return *ret;
	
	//DirectX::XMStoreFloat4x4(&value, pro);
	
	//return *this;
}

GGiFloat4x4& GDxFloat4x4::operator +(GGiFloat4x4& mat)
{
	GDxFloat4x4 dxMat = dynamic_cast<GDxFloat4x4&>(mat);

	DirectX::XMMATRIX add1 = DirectX::XMLoadFloat4x4(&dxMat.value);
	DirectX::XMMATRIX add2 = DirectX::XMLoadFloat4x4(&value);
	DirectX::XMMATRIX sum = add1 + add2;

	GDxFloat4x4* ret = new GDxFloat4x4();
	DirectX::XMStoreFloat4x4(&(ret->value), sum);
	return *ret;

	//DirectX::XMStoreFloat4x4(&value, sum);

	//return *this;
}

void GDxFloat4x4::SetByTranslation(float x, float y, float z)
{
	DirectX::XMStoreFloat4x4(&value, DirectX::XMMatrixTranslation(x, y, z));
}

void GDxFloat4x4::SetByRotationPitchYawRoll(float pitch, float yaw, float roll)
{
	DirectX::XMStoreFloat4x4(&value, DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll));
}

void GDxFloat4x4::SetByScale(float x, float y, float z)
{
	DirectX::XMStoreFloat4x4(&value, DirectX::XMMatrixScaling(x, y, z));
}

void GDxFloat4x4::SetByPerspectiveFovLH(float fovAngleY, float aspectRatio, float nearZ, float farZ)
{
	DirectX::XMStoreFloat4x4(&value, DirectX::XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, nearZ, farZ));
}

void GDxFloat4x4::SetByRotationAxis(float axisX, float axisY, float axisZ, float angle)
{
	DirectX::XMFLOAT3 axis(axisX, axisY, axisZ);
	DirectX::XMStoreFloat4x4(&value, DirectX::XMMatrixRotationAxis(DirectX::XMLoadFloat3(&axis), angle));
}

void GDxFloat4x4::SetByRotationY(float angle)
{
	DirectX::XMStoreFloat4x4(&value, DirectX::XMMatrixRotationY(angle));
}

std::vector<float> GDxFloat4x4::TransformNormal(std::vector<float> vec)
{
	DirectX::XMFLOAT3 norm(vec[0], vec[1], vec[2]);
	DirectX::XMVECTOR result = DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&norm), DirectX::XMLoadFloat4x4(&value));
	std::vector<float> out(3);
	out[0] = result.m128_f32[0];
	out[1] = result.m128_f32[1];
	out[2] = result.m128_f32[2];
	return out;
}

GGiFloat4x4* GDxFloat4x4::GetInverse()
{
	GDxFloat4x4* ret = new GDxFloat4x4();
	DirectX::XMMATRIX valueMat = DirectX::XMLoadFloat4x4(&value);
	DirectX::XMStoreFloat4x4(&ret->value, DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(valueMat), valueMat));
	return ret;
}


