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

	static GGiFloat4x4& Identity4x4();

private:

	DirectX::XMFLOAT4X4 value;

};

