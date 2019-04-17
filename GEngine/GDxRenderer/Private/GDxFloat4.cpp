#include "stdafx.h"
#include "GDxFloat4.h"
#include "GDxFloat4x4.h"


GDxFloat4::GDxFloat4()
{
	DirectX::XMFLOAT4 zero(0.0f, 0.0f, 0.0f, 0.0f);
	value = zero;
}


GDxFloat4::GDxFloat4(float x, float y, float z, float w)
{
	DirectX::XMFLOAT4 v(x, y, z, w);
	value = v;
}


GDxFloat4::~GDxFloat4()
{
}


GGiFloat4* GDxFloat4::ZeroVector()
{
	GDxFloat4* ret = new GDxFloat4();

	DirectX::XMFLOAT4 z(0.0f, 0.0f, 0.0f, 0.0f);

	ret->value = z;

	return ret;
}

GGiFloat4& GDxFloat4::operator =(GGiFloat4& vec)
{
	if (this != &vec)
	{
		GDxFloat4 dxVec = dynamic_cast<GDxFloat4&>(vec);
		value = dxVec.value;
	}
	return *this;
}

GGiFloat4& GDxFloat4::operator *(GGiFloat4& vec)
{
	GDxFloat4 dxVec = dynamic_cast<GDxFloat4&>(vec);

	DirectX::XMVECTOR fac1 = DirectX::XMLoadFloat4(&dxVec.value);
	DirectX::XMVECTOR fac2 = DirectX::XMLoadFloat4(&value);
	DirectX::XMVECTOR pro = DirectX::XMVector4Dot(fac1, fac2);

	DirectX::XMStoreFloat4(&value, pro);

	return *this;
}

GGiFloat4& GDxFloat4::operator *(GGiFloat4x4& mat)
{
	GDxFloat4x4 dxMat = dynamic_cast<GDxFloat4x4&>(mat);

	DirectX::XMVECTOR fac1 = DirectX::XMLoadFloat4(&value); 
	DirectX::XMMATRIX fac2 = DirectX::XMLoadFloat4x4(&dxMat.GetValue());
	DirectX::XMVECTOR pro = DirectX::XMVector4Transform(fac1, fac2);

	DirectX::XMStoreFloat4(&value, pro);

	return *this;
}

GGiFloat4& GDxFloat4::operator +(GGiFloat4& vec)
{
	GDxFloat4 dxVec = dynamic_cast<GDxFloat4&>(vec);

	DirectX::XMVECTOR add1 = DirectX::XMLoadFloat4(&dxVec.value);
	DirectX::XMVECTOR add2 = DirectX::XMLoadFloat4(&value);
	DirectX::XMVECTOR sum = DirectX::XMVectorAdd(add1, add2);

	DirectX::XMStoreFloat4(&value, sum);

	return *this;
}

float GDxFloat4::GetX()
{
	return value.x;
}

float GDxFloat4::GetY()
{
	return value.y;
}

float GDxFloat4::GetZ()
{
	return value.z;
}

float GDxFloat4::GetW()
{
	return value.w;
}

DirectX::XMFLOAT4 GDxFloat4::GetValue()
{
	return value;
}

void GDxFloat4::SetValue(DirectX::XMFLOAT4 v)
{
	value = v;
}