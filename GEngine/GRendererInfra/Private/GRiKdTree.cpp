#include "stdafx.h"
#include "GRiKdTree.h"


//#include "paramset.h"
//#include "interaction.h"
//#include "stats.h"
//#include <algorithm>

GRiKdTree::~GRiKdTree()
{
	FreeAligned(nodes);
}

// KdTreeAccel Local Declarations

// KdTreeAccel Method Definitions
GRiKdTree::GRiKdTree(
	std::vector<std::shared_ptr<GRiKdPrimitive>> p,
	int isectCost,
	int traversalCost,
	float emptyBonus,
	int maxPrims,
	int maxDepth)
	: isectCost(isectCost),
	traversalCost(traversalCost),
	maxPrims(maxPrims),
	emptyBonus(emptyBonus),
	primitives(std::move(p))
{
	// Build kd-tree for accelerator
	nextFreeNode = nAllocedNodes = 0;
	if (maxDepth <= 0)
		maxDepth = std::round(8 + 1.3f * log2(int64_t(primitives.size())));

	// Compute bounds for kd-tree construction
	std::vector<GRiBoundingBox> primBounds;
	primBounds.reserve(primitives.size());
	if (!primitives.empty())
		bounds = primitives[0]->WorldBound();
	for (const std::shared_ptr<GRiKdPrimitive> &prim : primitives)
	{
		GRiBoundingBox b = prim->WorldBound();
		bounds = GRiBoundingBox::Union(bounds, b);
		primBounds.push_back(b);
	}

	// Allocate working memory for kd-tree construction
	std::unique_ptr<BoundEdge[]> edges[3];
	for (int i = 0; i < 3; ++i)
		edges[i].reset(new BoundEdge[2 * primitives.size()]);
	std::unique_ptr<int[]> prims0(new int[primitives.size()]);
	std::unique_ptr<int[]> prims1(new int[(maxDepth + 1) * primitives.size()]);

	// Initialize _primNums_ for kd-tree construction
	std::unique_ptr<int[]> primNums(new int[primitives.size()]);
	for (size_t i = 0; i < primitives.size(); ++i) primNums[i] = i;

	// Start recursive construction of kd-tree
	buildTree(0, bounds, primBounds, primNums.get(), primitives.size(),
		maxDepth, edges, prims0.get(), prims1.get());
}

void KdTreeNode::InitLeaf(
	int *primNums,
	int np,
	std::vector<int> *primitiveIndices
) 
{
	flags = 3;
	nPrims |= (np << 2);
	// Store primitive ids for leaf node
	if (np == 0)
		onePrimitive = 0;
	else if (np == 1)
		onePrimitive = primNums[0];
	else 
	{
		primitiveIndicesOffset = primitiveIndices->size();
		for (int i = 0; i < np; ++i) primitiveIndices->push_back(primNums[i]);
	}
}

void GRiKdTree::buildTree(
	int nodeNum,
	const GRiBoundingBox &nodeBounds,
	const std::vector<GRiBoundingBox> &allPrimBounds,
	int *primNums, 
	int nPrimitives, 
	int depth,
	const std::unique_ptr<BoundEdge[]> edges[3],
	int *prims0, 
	int *prims1, 
	int badRefines
)
{
	// Get next free node from _nodes_ array
	if (nextFreeNode == nAllocedNodes)
	{
		int nNewAllocNodes = max(2 * nAllocedNodes, 512);
		KdTreeNode *n = AllocAligned<KdTreeNode>(nNewAllocNodes);
		if (nAllocedNodes > 0) 
		{
			memcpy(n, nodes, nAllocedNodes * sizeof(KdTreeNode));
			FreeAligned(nodes);
		}
		nodes = n;
		nAllocedNodes = nNewAllocNodes;
	}
	++nextFreeNode;

	// Initialize leaf node if termination criteria met
	if (nPrimitives <= maxPrims || depth == 0) 
	{
		nodes[nodeNum].InitLeaf(primNums, nPrimitives, &primitiveIndices);
		return;
	}

	// Initialize interior node and continue recursion

	// Choose split axis position for interior node
	int bestAxis = -1, bestOffset = -1;
	float bestCost = GGiEngineUtil::Infinity;
	float oldCost = isectCost * (float)nPrimitives;
	float totalSA = nodeBounds.SurfaceArea();
	float invTotalSA = 1 / totalSA;
	float d[3];
	d[0] = nodeBounds.Extents[0] * 2;
	d[1] = nodeBounds.Extents[1] * 2;
	d[2] = nodeBounds.Extents[2] * 2;

	// Choose which axis to split along
	int axis = nodeBounds.MaximumExtent();
	int retries = 0;

retrySplit:

	// Initialize edges for _axis_
	for (int i = 0; i < nPrimitives; ++i) 
	{
		int pn = primNums[i];
		const GRiBoundingBox &bounds = allPrimBounds[pn];
		edges[axis][2 * i] = BoundEdge(bounds.Center[axis] - bounds.Extents[axis], pn, true);
		edges[axis][2 * i + 1] = BoundEdge(bounds.Center[axis] + bounds.Extents[axis], pn, false);
	}

	// Sort _edges_ for _axis_
	std::sort(&edges[axis][0], &edges[axis][2 * nPrimitives],
		[](const BoundEdge &e0, const BoundEdge &e1) -> bool 
		{
			if (e0.t == e1.t)
				return (int)e0.type < (int)e1.type;
			else
				return e0.t < e1.t;
		});

	// Compute cost of all splits for _axis_ to find best
	int nBelow = 0, nAbove = nPrimitives;
	for (int i = 0; i < 2 * nPrimitives; ++i) 
	{
		if (edges[axis][i].type == EdgeType::End) --nAbove;
		float edgeT = edges[axis][i].t;
		if (edgeT > nodeBounds.BoundMin(axis) && edgeT < nodeBounds.BoundMax(axis))
		{
			// Compute cost for split at _i_th edge

			// Compute child surface areas for split at _edgeT_
			int otherAxis0 = (axis + 1) % 3, otherAxis1 = (axis + 2) % 3;
			float belowSA = 2 * (d[otherAxis0] * d[otherAxis1] +
				(edgeT - nodeBounds.BoundMin(axis)) *
				(d[otherAxis0] + d[otherAxis1]));
			float aboveSA = 2 * (d[otherAxis0] * d[otherAxis1] +
				(nodeBounds.BoundMax(axis) - edgeT) *
				(d[otherAxis0] + d[otherAxis1]));
			float pBelow = belowSA * invTotalSA;
			float pAbove = aboveSA * invTotalSA;
			float eb = (nAbove == 0 || nBelow == 0) ? emptyBonus : 0;
			float cost =
				traversalCost +
				isectCost * (1 - eb) * (pBelow * nBelow + pAbove * nAbove);

			// Update best split if this is lowest cost so far
			if (cost < bestCost)
			{
				bestCost = cost;
				bestAxis = axis;
				bestOffset = i;
			}
		}
		if (edges[axis][i].type == EdgeType::Start) ++nBelow;
	}
	//CHECK(nBelow == nPrimitives && nAbove == 0);
	if (!(nBelow == nPrimitives && nAbove == 0))
		ThrowGGiException("KdTree Error.");

	// Create leaf if no good splits were found
	if (bestAxis == -1 && retries < 2)
	{
		++retries;
		axis = (axis + 1) % 3;
		goto retrySplit;
	}
	if (bestCost > oldCost) ++badRefines;
	if ((bestCost > 4 * oldCost && nPrimitives < 16) || bestAxis == -1 ||
		badRefines == 3) 
	{
		nodes[nodeNum].InitLeaf(primNums, nPrimitives, &primitiveIndices);
		return;
	}

	// Classify primitives with respect to split
	int n0 = 0, n1 = 0;
	for (int i = 0; i < bestOffset; ++i)
		if (edges[bestAxis][i].type == EdgeType::Start)
			prims0[n0++] = edges[bestAxis][i].primNum;
	for (int i = bestOffset + 1; i < 2 * nPrimitives; ++i)
		if (edges[bestAxis][i].type == EdgeType::End)
			prims1[n1++] = edges[bestAxis][i].primNum;

	// Recursively initialize children nodes
	float tSplit = edges[bestAxis][bestOffset].t;
	GRiBoundingBox bounds0 = nodeBounds, bounds1 = nodeBounds;
	//bounds0.pMax[bestAxis] = bounds1.pMin[bestAxis] = tSplit;
	bounds0.Center[bestAxis] = (tSplit + nodeBounds.BoundMin(bestAxis)) / 2;
	bounds0.Extents[bestAxis] = (tSplit - nodeBounds.BoundMin(bestAxis)) / 2;
	bounds1.Center[bestAxis] = (tSplit + nodeBounds.BoundMax(bestAxis)) / 2;
	bounds1.Extents[bestAxis] = (nodeBounds.BoundMax(bestAxis) - tSplit) / 2;
	buildTree(nodeNum + 1, bounds0, allPrimBounds, prims0, n0, depth - 1, edges,
		prims0, prims1 + nPrimitives, badRefines);
	int aboveChild = nextFreeNode;
	nodes[nodeNum].InitInterior(bestAxis, aboveChild, tSplit);
	buildTree(aboveChild, bounds1, allPrimBounds, prims1, n1, depth - 1, edges,
		prims0, prims1 + nPrimitives, badRefines);
}

/*
bool GRiKdTree::Intersect(const GRiRay &ray, SurfaceInteraction *isect) const
{
	// Compute initial parametric range of ray inside kd-tree extent
	float tMin, tMax;
	if (!bounds.Intersect(ray, &tMin, &tMax))
	{
		return false;
	}

	// Prepare to traverse kd-tree for ray
	GGiFloat3 invDir(1.0f / ray.Direction[0], 1.0f / ray.Direction[1], 1.0f / ray.Direction[2]);
	const int maxTodo = 64;
	//Vector3f invDir(1 / ray.d.x, 1 / ray.d.y, 1 / ray.d.z);
	//PBRT_CONSTEXPR int maxTodo = 64;
	KdToDo todo[maxTodo];
	int todoPos = 0;

	// Traverse kd-tree nodes in order for ray
	bool hit = false;
	const KdTreeNode *node = &nodes[0];
	while (node != nullptr) 
	{
		// Bail out if we found a hit closer than the current node
		if (ray.tMax < tMin) break;
		if (!node->IsLeaf()) 
		{
			// Process kd-tree interior node

			// Compute parametric distance along ray to split plane
			int axis = node->SplitAxis();
			float tPlane = (node->SplitPos() - ray.Origin[axis]) * invDir[axis];

			// Get node children pointers for ray
			const KdTreeNode *firstChild, *secondChild;
			int belowFirst =
				(ray.Origin[axis] < node->SplitPos()) ||
				(ray.Origin[axis] == node->SplitPos() && ray.Direction[axis] <= 0);
			if (belowFirst) 
			{
				firstChild = node + 1;
				secondChild = &nodes[node->AboveChild()];
			}
			else 
			{
				firstChild = &nodes[node->AboveChild()];
				secondChild = node + 1;
			}

			// Advance to next child node, possibly enqueue other child
			if (tPlane > tMax || tPlane <= 0)
				node = firstChild;
			else if (tPlane < tMin)
				node = secondChild;
			else
			{
				// Enqueue _secondChild_ in todo list
				todo[todoPos].node = secondChild;
				todo[todoPos].tMin = tPlane;
				todo[todoPos].tMax = tMax;
				++todoPos;
				node = firstChild;
				tMax = tPlane;
			}
		}
		else
		{
			// Check for intersections inside leaf node
			int nPrimitives = node->nPrimitives();
			if (nPrimitives == 1)
			{
				const std::shared_ptr<GRiKdPrimitive> &p =
					primitives[node->onePrimitive];
				// Check one primitive inside leaf node
				if (p->Intersect(ray, isect)) hit = true;
			}
			else 
			{
				for (int i = 0; i < nPrimitives; ++i)
				{
					int index =
						primitiveIndices[node->primitiveIndicesOffset + i];
					const std::shared_ptr<GRiKdPrimitive> &p = primitives[index];
					// Check one primitive inside leaf node
					if (p->Intersect(ray, isect)) hit = true;
				}
			}

			// Grab next node to process from todo list
			if (todoPos > 0) 
			{
				--todoPos;
				node = todo[todoPos].node;
				tMin = todo[todoPos].tMin;
				tMax = todo[todoPos].tMax;
			}
			else
				break;
		}
	}
	return hit;
}
*/

bool GRiKdTree::IntersectDis(const GRiRay &ray, float* OutDis, bool& bBackface)
{
	/*
	// Compute initial parametric range of ray inside kd-tree extent
	float tMin, tMax;
	if (!bounds.Intersect(ray, &tMin, &tMax))
	{
		return false;
	}

	// Prepare to traverse kd-tree for ray
	GGiFloat3 invDir(1.0f / ray.Direction[0], 1.0f / ray.Direction[1], 1.0f / ray.Direction[2]);
	const int maxTodo = 64;
	//Vector3f invDir(1 / ray.d.x, 1 / ray.d.y, 1 / ray.d.z);
	//PBRT_CONSTEXPR int maxTodo = 64;
	KdToDo todo[maxTodo];
	int todoPos = 0;
	const KdTreeNode *node = &nodes[0];
	while (node != nullptr)
	{
		if (node->IsLeaf()) 
		{
			// Check for shadow ray intersections inside leaf node
			int nPrimitives = node->nPrimitives();
			if (nPrimitives == 1) 
			{
				const std::shared_ptr<GRiKdPrimitive> &p =
					primitives[node->onePrimitive];
				if (p->Intersect(ray, OutDis))
				{
					return true;
				}
			}
			else
			{
				for (int i = 0; i < nPrimitives; ++i)
				{
					int primitiveIndex =
						primitiveIndices[node->primitiveIndicesOffset + i];
					const std::shared_ptr<GRiKdPrimitive> &prim =
						primitives[primitiveIndex];
					if (prim->Intersect(ray, OutDis))
					{
						return true;
					}
				}
			}

			// Grab next node to process from todo list
			if (todoPos > 0)
			{
				--todoPos;
				node = todo[todoPos].node;
				tMin = todo[todoPos].tMin;
				tMax = todo[todoPos].tMax;
			}
			else
				break;
		}
		else
		{
			// Process kd-tree interior node

			// Compute parametric distance along ray to split plane
			int axis = node->SplitAxis();
			float tPlane = (node->SplitPos() - ray.Origin[axis]) * invDir[axis];

			// Get node children pointers for ray
			const KdTreeNode *firstChild, *secondChild;
			int belowFirst =
				(ray.Origin[axis] < node->SplitPos()) ||
				(ray.Origin[axis] == node->SplitPos() && ray.Direction[axis] <= 0);
			if (belowFirst)
			{
				firstChild = node + 1;
				secondChild = &nodes[node->AboveChild()];
			}
			else 
			{
				firstChild = &nodes[node->AboveChild()];
				secondChild = node + 1;
			}

			// Advance to next child node, possibly enqueue other child
			if (tPlane > tMax || tPlane <= 0)
				node = firstChild;
			else if (tPlane < tMin)
				node = secondChild;
			else 
			{
				// Enqueue _secondChild_ in todo list
				todo[todoPos].node = secondChild;
				todo[todoPos].tMin = tPlane;
				todo[todoPos].tMax = tMax;
				++todoPos;
				node = firstChild;
				tMax = tPlane;
			}
		}
	}
	return false;
	*/

	// Compute initial parametric range of ray inside kd-tree extent
	float tMin, tMax;
	if (!bounds.Intersect(ray, &tMin, &tMax))
	{
		return false;
	}

	// Prepare to traverse kd-tree for ray
	GGiFloat3 invDir(1.0f / ray.Direction[0], 1.0f / ray.Direction[1], 1.0f / ray.Direction[2]);
	const int maxTodo = 64;
	KdToDo todo[maxTodo];
	int todoPos = 0;

	// Traverse kd-tree nodes in order for ray
	bool hit = false;
	const KdTreeNode *node = &nodes[0];
	while (node != nullptr) 
	{
		// Bail out if we found a hit closer than the current node
		if (ray.tMax < tMin)
			break;
		if (!node->IsLeaf())
		{
			// Process kd-tree interior node

			// Compute parametric distance along ray to split plane
			int axis = node->SplitAxis();
			float tPlane = (node->SplitPos() - ray.Origin[axis]) * invDir[axis];

			// Get node children pointers for ray
			const KdTreeNode *firstChild, *secondChild;
			int belowFirst =
				(ray.Origin[axis] < node->SplitPos()) ||
				(ray.Origin[axis] == node->SplitPos() && ray.Direction[axis] <= 0);
			if (belowFirst) 
			{
				firstChild = node + 1;
				secondChild = &nodes[node->AboveChild()];
			}
			else 
			{
				firstChild = &nodes[node->AboveChild()];
				secondChild = node + 1;
			}

			// Advance to next child node, possibly enqueue other child
			if (tPlane > tMax || tPlane <= 0)
				node = firstChild;
			else if (tPlane < tMin)
				node = secondChild;
			else
			{
				// Enqueue _secondChild_ in todo list
				todo[todoPos].node = secondChild;
				todo[todoPos].tMin = tPlane;
				todo[todoPos].tMax = tMax;
				++todoPos;
				node = firstChild;
				tMax = tPlane;
			}
		}
		else 
		{
			// Check for intersections inside leaf node
			int nPrimitives = node->nPrimitives();
			if (nPrimitives == 1)
			{
				const std::shared_ptr<GRiKdPrimitive> &p =
					primitives[node->onePrimitive];
				// Check one primitive inside leaf node
				if (p->Intersect(ray, OutDis, bBackface)) hit = true;
			}
			else
			{
				float lowestDis = ray.tMax;
				bool lowestBackface = ray.bBackface;
				for (int i = 0; i < nPrimitives; ++i)
				{
					int index =
						primitiveIndices[node->primitiveIndicesOffset + i];
					const std::shared_ptr<GRiKdPrimitive> &p = primitives[index];
					// Check one primitive inside leaf node
					if (p->Intersect(ray, OutDis, bBackface))
					{
						hit = true;
						if (*OutDis < lowestDis)
						{
							lowestDis = *OutDis;
							lowestBackface = bBackface;
						}
					}
				}
				*OutDis = lowestDis;
				bBackface = lowestBackface;
			}

			// Grab next node to process from todo list
			if (todoPos > 0)
			{
				--todoPos;
				node = todo[todoPos].node;
				tMin = todo[todoPos].tMin;
				tMax = todo[todoPos].tMax;
			}
			else
				break;
		}
	}
	return hit;
}

// Moller¨CTrumbore intersection.
bool GRiKdPrimitive::Intersect(const GRiRay &ray, float *tHit, bool& bBackface)
{
	/*
	static const float EPSILON = 0.0000001;
	static GGiFloat3 edge1, edge2, h, s, q, rayDir, rayOrigin, outIntersectionPoint;
	static GGiFloat3 vPos[3];
	static float a, f, u, v, t;
	vPos[0] = GGiFloat3(vertices[0].Position[0], vertices[0].Position[1], vertices[0].Position[2]);
	vPos[1] = GGiFloat3(vertices[1].Position[0], vertices[1].Position[1], vertices[1].Position[2]);
	vPos[2] = GGiFloat3(vertices[2].Position[0], vertices[2].Position[1], vertices[2].Position[2]);
	rayDir = GGiFloat3(ray.Direction[0], ray.Direction[1], ray.Direction[2]);
	rayOrigin = GGiFloat3(ray.Origin[0], ray.Origin[1], ray.Origin[2]);
	edge1 = vPos[1] - vPos[0];
	edge2 = vPos[2] - vPos[0];

	h = GGiFloat3::Cross(rayDir, edge2);
	a = GGiFloat3::Dot(edge1, h);
	if (a > -EPSILON && a < EPSILON)
		return false;    // This ray is parallel to this triangle.
	f = 1.0 / a;
	s = rayOrigin - vPos[0];
	u = f * GGiFloat3::Dot(s, h);
	if (u < 0.0 || u > 1.0)
		return false;
	q = GGiFloat3::Cross(s, edge1);
	v = f * GGiFloat3::Dot(rayDir, q);
	if (v < 0.0 || u + v > 1.0)
		return false;
	// At this stage we can compute t to find out where the intersection point is on the line.
	t = f * GGiFloat3::Dot(edge2, q);
	if (t > EPSILON && t < 1 / EPSILON) // ray intersection
	{
		*tHit = t;
		return true;
	}
	else // This means that there is a line intersection but not a ray intersection.
		return false;
	*/
	static GGiFloat3 vPos[3], rayDir, rayOrigin, u, v, norm, w0, interPoint, w;
	static float b, a, r, uu, uv, vv, wu, wv, D, s, t;
	vPos[0] = GGiFloat3(vertices[0]->Position[0], vertices[0]->Position[1], vertices[0]->Position[2]);
	vPos[1] = GGiFloat3(vertices[1]->Position[0], vertices[1]->Position[1], vertices[1]->Position[2]);
	vPos[2] = GGiFloat3(vertices[2]->Position[0], vertices[2]->Position[1], vertices[2]->Position[2]);
	rayDir = GGiFloat3(ray.Direction[0], ray.Direction[1], ray.Direction[2]);
	rayOrigin = GGiFloat3(ray.Origin[0], ray.Origin[1], ray.Origin[2]);

	// is the ray parallel to the triangle? 
	u = vPos[1] - vPos[0];    // edge1.
	v = vPos[2] - vPos[0];    // edge2.
	norm = GGiFloat3::Cross(u, v);  // normal.

	//if (norm.x == 0.0f && norm.y == 0.0f && norm.z == 0.0f)  // triangle is degenerate.
		//return false;

	// calculate the angle between the ray and the triangle normal.
	b = GGiFloat3::Dot(norm, rayDir);
	if (fabs(b) < 1e-5)      // ray is parallel to triangle plane.
		return false;

	// calculate the vector from v0 to the ray origin.
	w0 = rayOrigin - vPos[0];
	a = -GGiFloat3::Dot(norm, w0);

	// get intersect point of ray with triangle plane.
	r = a / b;
	if (r < 0.0f)                 // ray goes away from triangle.
		return false;                 // => no intersect.
	*tHit = r;
	if (b > 0)
		bBackface = true;
	else
		bBackface = false;
	// for a segment, also test if (r > 1.0) => no intersect.   

	// calculate intersect point.
	interPoint = rayOrigin + rayDir * r;

	// is the intersect point inside the triangle?   
	uu = GGiFloat3::Dot(u, u);
	uv = GGiFloat3::Dot(u, v);
	vv = GGiFloat3::Dot(v, v);
	w = interPoint - vPos[0];
	wu = GGiFloat3::Dot(w, u);
	wv = GGiFloat3::Dot(w, v);
	D = uv * uv - uu * vv;

	// get and test parametric coords.
	s = (uv * wv - vv * wu) / D;
	if (s < 0.0f || s > 1.0f)       // I is outside T.
		return false;

	t = (uv * wu - uu * wv) / D;
	if (t < 0.0f || (s + t) > 1.0f) // I is outside T.
		return false;

	if (r < ray.tMax)
	{
		ray.tMax = r;
		ray.bBackface = bBackface;
	}
	return true;   // intersect point is inside the triangle.
}

/*                            
std::shared_ptr<KdTreeAccel> CreateKdTreeAccelerator(
	std::vector<std::shared_ptr<Primitive>> prims, const ParamSet &ps) {
	int isectCost = ps.FindOneInt("intersectcost", 80);
	int travCost = ps.FindOneInt("traversalcost", 1);
	Float emptyBonus = ps.FindOneFloat("emptybonus", 0.5f);
	int maxPrims = ps.FindOneInt("maxprims", 1);
	int maxDepth = ps.FindOneInt("maxdepth", -1);
	return std::make_shared<KdTreeAccel>(std::move(prims), isectCost, travCost, emptyBonus,
		maxPrims, maxDepth);
}
*/

