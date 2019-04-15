#pragma once
#include "GDxPreInclude.h"

class GDxFloat4 : public GGiFloat4
{
public:
	GDxFloat4();
	~GDxFloat4();

	virtual GGiFloat4& operator =(GGiFloat4& vec) override;

	virtual GGiFloat4& operator *(GGiFloat4& vec) override;

	virtual GGiFloat4& operator +(GGiFloat4& vec) override;

	static GGiFloat4& ZeroVector();

private:

	DirectX::XMFLOAT4 value;

};

