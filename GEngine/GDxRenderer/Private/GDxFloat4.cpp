#include "stdafx.h"
#include "GDxFloat4.h"


GDxFloat4::GDxFloat4()
{
	static DirectX::XMFLOAT4 zero(0.0f, 0.0f, 0.0f, 0.0f);
	value = zero;
}


GDxFloat4::~GDxFloat4()
{
}


GGiFloat4& GDxFloat4::ZeroVector()
{
	GDxFloat4 ret;

	static DirectX::XMFLOAT4 z(0.0f, 0.0f, 0.0f, 0.0f);

	ret.value = z;

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

GGiFloat4& GDxFloat4::operator +(GGiFloat4& vec)
{
	GDxFloat4 dxVec = dynamic_cast<GDxFloat4&>(vec);

	DirectX::XMVECTOR add1 = DirectX::XMLoadFloat4(&dxVec.value);
	DirectX::XMVECTOR add2 = DirectX::XMLoadFloat4(&value);
	DirectX::XMVECTOR sum = DirectX::XMVectorAdd(add1, add2);

	DirectX::XMStoreFloat4(&value, sum);

	return *this;
}