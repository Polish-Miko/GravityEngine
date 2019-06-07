#pragma once
#include "GRiPreInclude.h"
#include "GRiRendererFactory.h"



class GRiCamera
{
public:
	GRiCamera();
	~GRiCamera();

	GRiCamera(GRiRendererFactory* pFac);

	void SetRendererFactory(GRiRendererFactory* pFac);

	// Get/Set world camera position.
	std::vector<float> GetPosition()const;
	void SetPosition(float x, float y, float z);

	// Get camera basis vectors.
	std::vector<float> GetRight()const;
	std::vector<float> GetUp()const;
	std::vector<float> GetLook()const;

	// Get frustum properties.
	float GetNearZ()const;
	float GetFarZ()const;
	float GetAspect()const;
	float GetFovY()const;
	float GetFovX()const;

	// Get near and far plane dimensions in view space coordinates.
	float GetNearWindowWidth()const;
	float GetNearWindowHeight()const;
	float GetFarWindowWidth()const;
	float GetFarWindowHeight()const;

	// Set frustum.
	void SetLens(float fovY, float aspect, float zn, float zf);

	// Define camera space via LookAt parameters.
	void LookAt(std::vector<float> pos, std::vector<float> target, std::vector<float> worldUp);

	// Get View/Proj matrices.
	GGiFloat4x4* GetView()const;
	GGiFloat4x4* GetProj()const;
	GGiFloat4x4* GetReversedProj()const;

	// Strafe/Walk/Ascend the camera a distance d.
	void Strafe(float d);
	void Walk(float d);
	void Ascend(float d);

	// Rotate the camera.
	void Pitch(float angle);
	void RotateY(float angle);

	// After modifying camera position/orientation, call to rebuild the view matrix.
	void UpdateViewMatrix();

	void SetPrevViewProj(GGiFloat4x4* prev);
	GGiFloat4x4* GetPrevViewProj();
	void InitPrevViewProj();

	void SetPrevPosition(std::vector<float> prev);
	std::vector<float> GetPrevPosition();
	void InitPrevPosition();

private:
	
	GRiRendererFactory* pRendererFactory;

	// Camera coordinate system with coordinates relative to world space.
	float mPosition[3] = { 0.0f, 0.0f, 0.0f };
	float mRight[3] = { 1.0f, 0.0f, 0.0f };
	float mUp[3] = { 0.0f, 1.0f, 0.0f };
	float mLook[3] = { 0.0f, 0.0f, 1.0f };

	GGiFloat4x4* prevViewProj;

	float mPrevPosition[3] = { 0.0f,0.0f,0.0f };

	// Cache frustum properties.
	float mNearZ = 0.0f;
	float mFarZ = 0.0f;
	float mAspect = 0.0f;
	float mFovY = 0.0f;
	float mNearWindowHeight = 0.0f;
	float mFarWindowHeight = 0.0f;

	bool mViewDirty = true;

	// Cache View/Proj matrices.
	GGiFloat4x4* mView;
	GGiFloat4x4* mProj;

	GGiFloat4x4* mReversedProj;
};

