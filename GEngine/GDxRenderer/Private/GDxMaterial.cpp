#include "stdafx.h"
#include "GDxMaterial.h"
#include "GDxFloat4x4.h"


GDxMaterial::GDxMaterial()
{
	MatTransform = &GDxFloat4x4::Identity4x4();
}


GDxMaterial::~GDxMaterial()
{
}
