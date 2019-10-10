#pragma once
#include "GRiPreInclude.h"
#include "GRiBoundingBox.h"

// KdTreeAccel Declarations
struct KdTreeNode;
struct BoundEdge;

class GRiKdTree
{
public:

	GRiKdTree(
		std::vector<std::shared_ptr<Primitive>> p,
		int isectCost = 80,
		int traversalCost = 1,
		float emptyBonus = 0.5,
		int maxPrims = 1, 
		int maxDepth = -1
	);

	~GRiKdTree();

	GRiBoundingBox WorldBound() const { return bounds; }

	bool Intersect(const Ray &ray, SurfaceInteraction *isect) const;
	bool IntersectP(const Ray &ray) const;

	static std::shared_ptr<GRiKdTree> CreateKdTreeAccelerator(
		std::vector<std::shared_ptr<Primitive>> prims, const ParamSet &ps);

private:
	// KdTreeAccel Private Methods
	void buildTree(int nodeNum, const GRiBoundingBox &bounds,
		const std::vector<GRiBoundingBox> &primBounds, int *primNums,
		int nprims, int depth,

		const std::unique_ptr<BoundEdge[]> edges[3], int *prims0,
		int *prims1, int badRefines = 0);

	// KdTreeAccel Private Data
	const int isectCost, traversalCost, maxPrims;
	const float emptyBonus;
	std::vector<std::shared_ptr<Primitive>> primitives;
	std::vector<int> primitiveIndices;
	KdTreeNode *nodes;
	int nAllocedNodes, nextFreeNode;
	GRiBoundingBox bounds;
};

struct KdToDo {
	const KdTreeNode *node;
	float tMin, tMax;
};


