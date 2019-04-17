#pragma once
#include "GRiPreInclude.h"
#include "GRiMeshData.h"
#include "GRiVertex.h"


class GRiGeometryGenerator
{
public:
	GRiGeometryGenerator();
	~GRiGeometryGenerator();

	///<summary>
	/// Creates a box centered at the origin with the given dimensions, where each
	/// face has m rows and n columns of vertices.
	///</summary>
	GRiMeshData CreateBox(float width, float height, float depth, uint32_t numSubdivisions);

	///<summary>
	/// Creates a sphere centered at the origin with the given radius.  The
	/// slices and stacks parameters control the degree of tessellation.
	///</summary>
	GRiMeshData CreateSphere(float radius, uint32_t sliceCount, uint32_t stackCount);

	///<summary>
	/// Creates a geosphere centered at the origin with the given radius.  The
	/// depth controls the level of tessellation.
	///</summary>
	GRiMeshData CreateGeosphere(float radius, uint32_t numSubdivisions);

	///<summary>
	/// Creates a cylinder parallel to the y-axis, and centered about the origin.  
	/// The bottom and top radius can vary to form various cone shapes rather than true
	// cylinders.  The slices and stacks parameters control the degree of tessellation.
	///</summary>
	GRiMeshData CreateCylinder(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount);

	///<summary>
	/// Creates an mxn grid in the xz-plane with m rows and n columns, centered
	/// at the origin with the specified width and depth.
	///</summary>
	GRiMeshData CreateGrid(float width, float depth, uint32_t m, uint32_t n);

	///<summary>
	/// Creates a quad aligned with the screen.  This is useful for postprocessing and screen effects.
	///</summary>
	GRiMeshData CreateQuad(float x, float y, float w, float h, float depth);

private:
	void Subdivide(GRiMeshData& meshData);
	GRiVertex MidPoint(const GRiVertex& v0, const GRiVertex& v1);
	void BuildCylinderTopCap(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount, GRiMeshData& meshData);
	void BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount, GRiMeshData& meshData);
};

