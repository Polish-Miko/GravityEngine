#pragma once
#include "GRiPreInclude.h"
#include "GRiBoundingBox.h"
#include "GRiVertex.h"
#include "GRiRay.h"


// KdTreeAccel Declarations
//struct KdTreeNode;
//struct BoundEdge;
//struct GRiKdRay;

enum class EdgeType
{
	Start,
	End
};

struct BoundEdge
{
	// BoundEdge Public Methods
	BoundEdge() {}

	BoundEdge(float t, int primNum, bool starting) : t(t), primNum(primNum)
	{
		type = starting ? EdgeType::Start : EdgeType::End;
	}

	float t;
	int primNum;
	EdgeType type;
};

/*
struct GRiKdRay
{
	GRiKdRay() {}
};
*/

struct GRiKdPrimitive
{
public:

	GRiKdPrimitive() {}

	GRiKdPrimitive(GRiVertex* vert1, GRiVertex* vert2, GRiVertex* vert3)
	{
		vertices[0] = vert1;
		vertices[1] = vert2;
		vertices[2] = vert3;

		float vMax[3] = { 0,0,0 };
		float vMin[3] = { 0,0,0 };

		vMax[0] = max(max(vertices[0]->Position[0], vertices[1]->Position[0]), vertices[2]->Position[0]);
		vMax[1] = max(max(vertices[0]->Position[1], vertices[1]->Position[1]), vertices[2]->Position[1]);
		vMax[2] = max(max(vertices[0]->Position[2], vertices[1]->Position[2]), vertices[2]->Position[2]);

		vMin[0] = min(min(vertices[0]->Position[0], vertices[1]->Position[0]), vertices[2]->Position[0]);
		vMin[1] = min(min(vertices[0]->Position[1], vertices[1]->Position[1]), vertices[2]->Position[1]);
		vMin[2] = min(min(vertices[0]->Position[2], vertices[1]->Position[2]), vertices[2]->Position[2]);

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

	bool Intersect(const GRiRay &ray, float *tHit, bool& bBackface);

private:

	GRiVertex* vertices[3];

	GRiBoundingBox bound;
};

struct KdTreeNode
{
	void InitLeaf(int *primNums, int np, std::vector<int> *primitiveIndices);

	void InitInterior(int axis, int ac, float s)
	{
		split = s;
		flags = axis;
		aboveChild |= (ac << 2);
	}

	float SplitPos() const
	{
		return split;
	}

	int nPrimitives() const
	{
		return nPrims >> 2;
	}

	int SplitAxis() const
	{
		return flags & 3;
	}

	bool IsLeaf() const
	{
		return (flags & 3) == 3;
	}

	int AboveChild() const
	{
		return aboveChild >> 2;
	}

	union
	{
		float split;                 // Interior
		int onePrimitive;            // Leaf
		int primitiveIndicesOffset;  // Leaf
	};

private:

	union
	{
		int flags;       // Both
		int nPrims;      // Leaf
		int aboveChild;  // Interior
	};
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

	//bool Intersect(const GRiRay &ray, SurfaceInteraction *isect) const;
	bool IntersectDis(const GRiRay &ray, float* OutDis, bool& bBackface);

	/*
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
	KdTreeNode *nodes;
	std::vector<int> primitiveIndices;
	int nAllocedNodes, nextFreeNode;
	GRiBoundingBox bounds;
};

struct KdToDo 
{
	const KdTreeNode *node;
	float tMin, tMax;
};


