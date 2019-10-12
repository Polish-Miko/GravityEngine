#pragma once
#include "GRiPreInclude.h"
#include "GRiBoundingBox.h"
#include "GRiVertex.h"


// KdTreeAccel Declarations
struct KdTreeNode;
struct BoundEdge;
struct GRiKdRay;

struct GRiKdPrimitive
{
public:

	GRiKdPrimitive() {}

	GRiKdPrimitive(GRiVertex* vert)
	{
		vertices[0] = vert[0];
		vertices[1] = vert[1];
		vertices[2] = vert[2];

		float vMax[3] = { 0,0,0 };
		float vMin[3] = { 0,0,0 };

		vMax[0] = max(max(vertices[0].Position[0], vertices[1].Position[0]), vertices[2].Position[0]);
		vMax[1] = max(max(vertices[0].Position[1], vertices[1].Position[1]), vertices[2].Position[1]);
		vMax[2] = max(max(vertices[0].Position[2], vertices[1].Position[2]), vertices[2].Position[2]);

		vMin[0] = min(min(vertices[0].Position[0], vertices[1].Position[0]), vertices[2].Position[0]);
		vMin[1] = min(min(vertices[0].Position[1], vertices[1].Position[1]), vertices[2].Position[1]);
		vMin[2] = min(min(vertices[0].Position[2], vertices[1].Position[2]), vertices[2].Position[2]);

		bound.Center[0] = (vMax[0] + vMin[0]) / 2;
		bound.Center[1] = (vMax[1] + vMin[1]) / 2;
		bound.Center[2] = (vMax[2] + vMin[2]) / 2;
		bound.Extents[0] = (vMax[0] - vMin[0]) / 2;
		bound.Extents[1] = (vMax[1] - vMin[1]) / 2;
		bound.Extents[2] = (vMax[2] - vMin[2]) / 2;
	}

	inline GRiBoundingBox WorldBound()
	{
		return bound;
	}

private:

	GRiVertex vertices[3];

	GRiBoundingBox bound;
};

class GRiKdTree
{
public:

	GRiKdTree(
		std::vector<std::shared_ptr<GRiKdPrimitive>> p,
		int isectCost = 80,
		int traversalCost = 1,
		float emptyBonus = 0.5,
		int maxPrims = 1, 
		int maxDepth = -1
	);

	~GRiKdTree();

	GRiBoundingBox WorldBound() const { return bounds; }

	/*
	bool Intersect(const GRiKdRay &ray, SurfaceInteraction *isect) const;
	bool IntersectP(const GRiKdRay &ray) const;

	static std::shared_ptr<GRiKdTree> CreateKdTreeAccelerator(
		std::vector<std::shared_ptr<GRiKdPrimitive>> prims, const ParamSet &ps);
	*/

private:
	// KdTree Private Methods
	void buildTree(
		int nodeNum, 
		const GRiBoundingBox &bounds,
		const std::vector<GRiBoundingBox> &primBounds, 
		int *primNums,
		int nprims, 
		int depth,
		const std::unique_ptr<BoundEdge[]> edges[3],
		int *prims0,
		int *prims1, 
		int badRefines = 0
	);

	// KdTree Private Data
	const int isectCost, traversalCost, maxPrims;
	const float emptyBonus;
	std::vector<std::shared_ptr<GRiKdPrimitive>> primitives;
	std::vector<int> primitiveIndices;
	KdTreeNode *nodes;
	int nAllocedNodes, nextFreeNode;
	GRiBoundingBox bounds;
};

struct KdToDo {
	const KdTreeNode *node;
	float tMin, tMax;
};


