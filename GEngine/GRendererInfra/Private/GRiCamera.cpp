#include "stdafx.h"
#include "GRiCamera.h"


GRiCamera::GRiCamera()
{
	//SetLens(0.25f * GGiEngineUtil::PI, 1.0f, 1.0f, 1000.0f);
}


GRiCamera::~GRiCamera()
{
}

GRiCamera::GRiCamera(GRiRendererFactory* pFac)
{
	//SetLens(0.25f * GGiEngineUtil::PI, 1.0f, 1.0f, 1000.0f);
	SetRendererFactory(pFac);
}

void GRiCamera::SetRendererFactory(GRiRendererFactory* pFac)
{
	pRendererFactory = pFac;
}

std::vector<float> GRiCamera::GetPosition()const
{
	std::vector<float> pos(3);
	pos[0] = mPosition[0];
	pos[1] = mPosition[1];
	pos[2] = mPosition[2];
	return pos;
}

void GRiCamera::SetPosition(float x, float y, float z)
{
	mPosition[0] = x;
	mPosition[1] = y;
	mPosition[2] = z;
	mViewDirty = true;
}

std::vector<float> GRiCamera::GetRight()const
{
	std::vector<float> right(3);
	right[0] = mRight[0];
	right[1] = mRight[1];
	right[2] = mRight[2];
	return right;
}

std::vector<float> GRiCamera::GetUp()const
{
	std::vector<float> up(3);
	up[0] = mUp[0];
	up[1] = mUp[1];
	up[2] = mUp[2];
	return up;
}

std::vector<float> GRiCamera::GetLook()const
{
	std::vector<float> look(3);
	look[0] = mLook[0];
	look[1] = mLook[1];
	look[2] = mLook[2];
	return look;
}

float GRiCamera::GetNearZ()const
{
	return mNearZ;
}

float GRiCamera::GetFarZ()const
{
	return mFarZ;
}

float GRiCamera::GetAspect()const
{
	return mAspect;
}

float GRiCamera::GetFovY()const
{
	return mFovY;
}

float GRiCamera::GetFovX()const
{
	float halfWidth = 0.5f*GetNearWindowWidth();
	return 2.0f*atan(halfWidth / mNearZ);
}

float GRiCamera::GetNearWindowWidth()const
{
	return mAspect * mNearWindowHeight;
}

float GRiCamera::GetNearWindowHeight()const
{
	return mNearWindowHeight;
}

float GRiCamera::GetFarWindowWidth()const
{
	return mAspect * mFarWindowHeight;
}

float GRiCamera::GetFarWindowHeight()const
{
	return mFarWindowHeight;
}

void GRiCamera::SetLens(float fovY, float aspect, float zn, float zf)
{
	// cache properties
	mFovY = fovY;
	mAspect = aspect;
	mNearZ = zn;
	mFarZ = zf;

	mNearWindowHeight = 2.0f * mNearZ * tanf(0.5f*mFovY);
	mFarWindowHeight = 2.0f * mFarZ * tanf(0.5f*mFovY);

	GGiFloat4x4* P = pRendererFactory->CreateFloat4x4();
	P->SetByPerspectiveFovLH(mFovY, mAspect, mNearZ, mFarZ);
	mProj = P;
}

void GRiCamera::LookAt(std::vector<float> pos, std::vector<float> target, std::vector<float> worldUp)
{
	std::vector<float> L = GGiEngineUtil::Normalize(GGiEngineUtil::VectorSubtract(target, pos));
	std::vector<float> R = GGiEngineUtil::Normalize(GGiEngineUtil::CrossFloat3(worldUp, L));
	std::vector<float> U = GGiEngineUtil::CrossFloat3(L, R);

	mPosition[0] = pos[0];
	mPosition[1] = pos[1];
	mPosition[2] = pos[2];
	mLook[0] = L[0];
	mLook[1] = L[1];
	mLook[2] = L[2];
	mRight[0] = R[0];
	mRight[1] = R[1];
	mRight[2] = R[2];
	mUp[0] = U[0];
	mUp[1] = U[1];
	mUp[2] = U[2];

	mViewDirty = true;
}

GGiFloat4x4* GRiCamera::GetView()const
{
	if (mViewDirty)
	{
		ThrowGGiException("Trying to read dirty camera view data.")
	}
	return mView;
}

GGiFloat4x4* GRiCamera::GetProj()const
{
	if (mViewDirty)
	{
		ThrowGGiException("Trying to read dirty camera view data.")
	}
	return mProj;
}

void GRiCamera::Strafe(float d)
{
	// mPosition += d * mRight
	mPosition[0] = d * mRight[0] + mPosition[0];
	mPosition[1] = d * mRight[1] + mPosition[1];
	mPosition[2] = d * mRight[2] + mPosition[2];

	mViewDirty = true;
}

void GRiCamera::Walk(float d)
{
	// mPosition += d * mLook
	mPosition[0] = d * mLook[0] + mPosition[0];
	mPosition[1] = d * mLook[1] + mPosition[1];
	mPosition[2] = d * mLook[2] + mPosition[2];

	mViewDirty = true;
}

void GRiCamera::Ascend(float d)
{
	// mPosition += d * mUp
	mPosition[0] = d * mUp[0] + mPosition[0];
	mPosition[1] = d * mUp[1] + mPosition[1];
	mPosition[2] = d * mUp[2] + mPosition[2];

	mViewDirty = true;
}

void GRiCamera::Pitch(float angle)
{
	// Rotate up and look vector about the right vector.
	GGiFloat4x4* R = pRendererFactory->CreateFloat4x4();
	R->SetByRotationAxis(mRight[0], mRight[1], mRight[2], angle);

	std::vector<float> u = R->TransformNormal(GetUp());
	mUp[0] = u[0];
	mUp[1] = u[1];
	mUp[2] = u[2];

	std::vector<float> l = R->TransformNormal(GetLook());
	mLook[0] = l[0];
	mLook[1] = l[1];
	mLook[2] = l[2];

	mViewDirty = true;
}

void GRiCamera::RotateY(float angle)
{
	// Rotate the basis vectors about the world y-axis.
	GGiFloat4x4* R = pRendererFactory->CreateFloat4x4();
	R->SetByRotationY(angle);

	std::vector<float> r = R->TransformNormal(GetRight());
	mRight[0] = r[0];
	mRight[1] = r[1];
	mRight[2] = r[2];

	std::vector<float> u = R->TransformNormal(GetUp());
	mUp[0] = u[0];
	mUp[1] = u[1];
	mUp[2] = u[2];

	std::vector<float> l = R->TransformNormal(GetLook());
	mLook[0] = l[0];
	mLook[1] = l[1];
	mLook[2] = l[2];

	mViewDirty = true;
}

void GRiCamera::UpdateViewMatrix()
{
	if (mViewDirty)
	{
		std::vector<float> R = { mRight[0] ,mRight[1] ,mRight[2] };
		std::vector<float> U = { mUp[0] ,mUp[1] ,mUp[2] };
		std::vector<float> L = { mLook[0] ,mLook[1] ,mLook[2] };
		std::vector<float> P = { mPosition[0] ,mPosition[1] ,mPosition[2] };

		L = GGiEngineUtil::Normalize(L);
		U = GGiEngineUtil::Normalize(GGiEngineUtil::CrossFloat3(L, R));
		R = GGiEngineUtil::CrossFloat3(U, L);

		float x = -GGiEngineUtil::VectorDotProduct(P, R);
		float y = -GGiEngineUtil::VectorDotProduct(P, U);
		float z = -GGiEngineUtil::VectorDotProduct(P, L);

		mRight[0] = R[0];
		mRight[1] = R[1];
		mRight[2] = R[2];
		mUp[0] = U[0];
		mUp[1] = U[1];
		mUp[2] = U[2];
		mLook[0] = L[0];
		mLook[1] = L[1];
		mLook[2] = L[2];

		mView = pRendererFactory->CreateFloat4x4();

		mView->SetElement(0, 0, mRight[0]);
		mView->SetElement(1, 0, mRight[1]);
		mView->SetElement(2, 0, mRight[2]);
		mView->SetElement(3, 0, x);
		mView->SetElement(0, 1, mUp[0]);
		mView->SetElement(1, 1, mUp[1]);
		mView->SetElement(2, 1, mUp[2]);
		mView->SetElement(3, 1, y);
		mView->SetElement(0, 2, mLook[0]);
		mView->SetElement(1, 2, mLook[1]);
		mView->SetElement(2, 2, mLook[2]);
		mView->SetElement(3, 2, z);
		mView->SetElement(0, 3, 0.0f);
		mView->SetElement(1, 3, 0.0f);
		mView->SetElement(2, 3, 0.0f);
		mView->SetElement(3, 3, 1.0f);

		mViewDirty = false;
	}
}


