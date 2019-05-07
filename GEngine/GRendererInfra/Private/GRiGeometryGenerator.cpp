#include "stdafx.h"
#include "GRiGeometryGenerator.h"



GRiGeometryGenerator::GRiGeometryGenerator()
{
}


GRiGeometryGenerator::~GRiGeometryGenerator()
{
}

GRiMeshData GRiGeometryGenerator::CreateBox(float width, float height, float depth, uint32_t numSubdivisions)
{
	GRiMeshData meshData;

	//
	// Create the vertices.
	//

	GRiVertex v[24];

	float w2 = 0.5f*width;
	float h2 = 0.5f*height;
	float d2 = 0.5f*depth;

	// Fill in the front face GRiVertex data.
	v[0] = GRiVertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[1] = GRiVertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[2] = GRiVertex(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[3] = GRiVertex(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the back face GRiVertex data.
	v[4] = GRiVertex(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[5] = GRiVertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[6] = GRiVertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[7] = GRiVertex(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Fill in the top face GRiVertex data.
	v[8] = GRiVertex(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[9] = GRiVertex(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[10] = GRiVertex(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[11] = GRiVertex(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the bottom face GRiVertex data.
	v[12] = GRiVertex(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[13] = GRiVertex(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[14] = GRiVertex(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[15] = GRiVertex(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Fill in the left face GRiVertex data.
	v[16] = GRiVertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[17] = GRiVertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[18] = GRiVertex(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[19] = GRiVertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	// Fill in the right face GRiVertex data.
	v[20] = GRiVertex(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	v[21] = GRiVertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	v[22] = GRiVertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
	v[23] = GRiVertex(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

	meshData.Vertices.assign(&v[0], &v[24]);

	//
	// Create the indices.
	//

	uint32_t i[36];

	// Fill in the front face index data
	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 0; i[4] = 2; i[5] = 3;

	// Fill in the back face index data
	i[6] = 4; i[7] = 5; i[8] = 6;
	i[9] = 4; i[10] = 6; i[11] = 7;

	// Fill in the top face index data
	i[12] = 8; i[13] = 9; i[14] = 10;
	i[15] = 8; i[16] = 10; i[17] = 11;

	// Fill in the bottom face index data
	i[18] = 12; i[19] = 13; i[20] = 14;
	i[21] = 12; i[22] = 14; i[23] = 15;

	// Fill in the left face index data
	i[24] = 16; i[25] = 17; i[26] = 18;
	i[27] = 16; i[28] = 18; i[29] = 19;

	// Fill in the right face index data
	i[30] = 20; i[31] = 21; i[32] = 22;
	i[33] = 20; i[34] = 22; i[35] = 23;

	meshData.Indices.assign(&i[0], &i[36]);

	// Put a cap on the number of subdivisions.
	numSubdivisions = std::min<uint32_t>(numSubdivisions, 6u);

	for (uint32_t i = 0; i < numSubdivisions; ++i)
		Subdivide(meshData);

	meshData.SubmeshName = L"Box";

	return meshData;
}

GRiMeshData GRiGeometryGenerator::CreateSphere(float radius, uint32_t sliceCount, uint32_t stackCount)
{
	GRiMeshData meshData;

	//
	// Compute the vertices stating at the top pole and moving down the stacks.
	//

	// Poles: note that there will be texture coordinate distortion as there is
	// not a unique point on the texture map to assign to the pole when mapping
	// a rectangular texture onto a sphere.
	GRiVertex topGRiVertex(0.0f, +radius, 0.0f, 0.0f, +1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	GRiVertex bottomGRiVertex(0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	meshData.Vertices.push_back(topGRiVertex);

	float phiStep = GGiEngineUtil::PI / stackCount;
	float thetaStep = 2.0f * GGiEngineUtil::PI / sliceCount;

	// Compute vertices for each stack ring (do not count the poles as rings).
	for (uint32_t i = 1; i <= stackCount - 1; ++i)
	{
		float phi = i * phiStep;

		// Vertices of ring.
		for (uint32_t j = 0; j <= sliceCount; ++j)
		{
			float theta = j * thetaStep;

			GRiVertex v;

			// spherical to cartesian
			v.Position[0] = radius * sinf(phi)*cosf(theta);
			v.Position[1] = radius * cosf(phi);
			v.Position[2] = radius * sinf(phi)*sinf(theta);

			// Partial derivative of P with respect to theta
			v.TangentU[0] = -radius * sinf(phi)*sinf(theta);
			v.TangentU[1] = 0.0f;
			v.TangentU[2] = +radius * sinf(phi)*cosf(theta);

			float l = (float)pow(v.TangentU[0] * v.TangentU[0] + v.TangentU[2] * v.TangentU[2], 0.5);
			v.TangentU[0] /= l;
			v.TangentU[2] /= l;

			l = (float)pow(v.Position[0] * v.Position[0] + v.Position[1] * v.Position[1] + v.Position[2] * v.Position[2], 0.5);
			v.Normal[0] = v.Position[0] / l;
			v.Normal[1] = v.Position[1] / l;
			v.Normal[2] = v.Position[2] / l;

			v.UV[0] = theta / (2 * GGiEngineUtil::PI);
			v.UV[1] = phi / GGiEngineUtil::PI;

			meshData.Vertices.push_back(v);
		}
	}

	meshData.Vertices.push_back(bottomGRiVertex);

	//
	// Compute indices for top stack.  The top stack was written first to the GRiVertex buffer
	// and connects the top pole to the first ring.
	//

	for (uint32_t i = 1; i <= sliceCount; ++i)
	{
		meshData.Indices.push_back(0);
		meshData.Indices.push_back(i + 1);
		meshData.Indices.push_back(i);
	}

	//
	// Compute indices for inner stacks (not connected to poles).
	//

	// Offset the indices to the index of the first GRiVertex in the first ring.
	// This is just skipping the top pole GRiVertex.
	uint32_t baseIndex = 1;
	uint32_t ringGRiVertexCount = sliceCount + 1;
	for (uint32_t i = 0; i < stackCount - 2; ++i)
	{
		for (uint32_t j = 0; j < sliceCount; ++j)
		{
			meshData.Indices.push_back(baseIndex + i * ringGRiVertexCount + j);
			meshData.Indices.push_back(baseIndex + i * ringGRiVertexCount + j + 1);
			meshData.Indices.push_back(baseIndex + (i + 1)*ringGRiVertexCount + j);

			meshData.Indices.push_back(baseIndex + (i + 1)*ringGRiVertexCount + j);
			meshData.Indices.push_back(baseIndex + i * ringGRiVertexCount + j + 1);
			meshData.Indices.push_back(baseIndex + (i + 1)*ringGRiVertexCount + j + 1);
		}
	}

	//
	// Compute indices for bottom stack.  The bottom stack was written last to the GRiVertex buffer
	// and connects the bottom pole to the bottom ring.
	//

	// South pole GRiVertex was added last.
	uint32_t southPoleIndex = (uint32_t)meshData.Vertices.size() - 1;

	// Offset the indices to the index of the first GRiVertex in the last ring.
	baseIndex = southPoleIndex - ringGRiVertexCount;

	for (uint32_t i = 0; i < sliceCount; ++i)
	{
		meshData.Indices.push_back(southPoleIndex);
		meshData.Indices.push_back(baseIndex + i);
		meshData.Indices.push_back(baseIndex + i + 1);
	}

	//meshData.Transform = MathHelper::Identity4x4();

	meshData.SubmeshName = L"Sphere";

	return meshData;
}

void GRiGeometryGenerator::Subdivide(GRiMeshData& meshData)
{
	// Save a copy of the input geometry.
	GRiMeshData inputCopy = meshData;


	meshData.Vertices.resize(0);
	meshData.Indices.resize(0);

	//       v1
	//       *
	//      / \
	//     /   \
	//  m0*-----*m1
	//   / \   / \
	//  /   \ /   \
	// *-----*-----*
	// v0    m2     v2

	uint32_t numTris = (uint32_t)inputCopy.Indices.size() / 3;
	for (uint32_t i = 0; i < numTris; ++i)
	{
		GRiVertex v0 = inputCopy.Vertices[inputCopy.Indices[i * 3 + 0]];
		GRiVertex v1 = inputCopy.Vertices[inputCopy.Indices[i * 3 + 1]];
		GRiVertex v2 = inputCopy.Vertices[inputCopy.Indices[i * 3 + 2]];

		//
		// Generate the midpoints.
		//

		GRiVertex m0 = MidPoint(v0, v1);
		GRiVertex m1 = MidPoint(v1, v2);
		GRiVertex m2 = MidPoint(v0, v2);

		//
		// Add new geometry.
		//

		meshData.Vertices.push_back(v0); // 0
		meshData.Vertices.push_back(v1); // 1
		meshData.Vertices.push_back(v2); // 2
		meshData.Vertices.push_back(m0); // 3
		meshData.Vertices.push_back(m1); // 4
		meshData.Vertices.push_back(m2); // 5

		meshData.Indices.push_back(i * 6 + 0);
		meshData.Indices.push_back(i * 6 + 3);
		meshData.Indices.push_back(i * 6 + 5);

		meshData.Indices.push_back(i * 6 + 3);
		meshData.Indices.push_back(i * 6 + 4);
		meshData.Indices.push_back(i * 6 + 5);

		meshData.Indices.push_back(i * 6 + 5);
		meshData.Indices.push_back(i * 6 + 4);
		meshData.Indices.push_back(i * 6 + 2);

		meshData.Indices.push_back(i * 6 + 3);
		meshData.Indices.push_back(i * 6 + 1);
		meshData.Indices.push_back(i * 6 + 4);
	}
}

GRiVertex GRiGeometryGenerator::MidPoint(const GRiVertex& v0, const GRiVertex& v1)
{
	GRiVertex v;
	v.Position[0] = (v0.Position[0] + v1.Position[0]) / 2;
	v.Position[1] = (v0.Position[1] + v1.Position[1]) / 2;
	v.Position[2] = (v0.Position[2] + v1.Position[2]) / 2;
	v.Normal[0] = (v0.Normal[0] + v1.Normal[0]) / 2;
	v.Normal[1] = (v0.Normal[1] + v1.Normal[1]) / 2;
	v.Normal[2] = (v0.Normal[2] + v1.Normal[2]) / 2;
	GGiEngineUtil::NormalizeFloat3(v.Normal[0], v.Normal[1], v.Normal[2]);
	v.TangentU[0] = (v0.TangentU[0] + v1.TangentU[0]) / 2;
	v.TangentU[1] = (v0.TangentU[1] + v1.TangentU[1]) / 2;
	v.TangentU[2] = (v0.TangentU[2] + v1.TangentU[2]) / 2;
	GGiEngineUtil::NormalizeFloat3(v.TangentU[0], v.TangentU[1], v.TangentU[2]);
	v.UV[0] = (v0.UV[0] + v1.UV[0]) / 2;
	v.UV[1] = (v0.UV[1] + v1.UV[1]) / 2;

	return v;
}

GRiMeshData GRiGeometryGenerator::CreateGeosphere(float radius, uint32_t numSubdivisions)
{
	GRiMeshData meshData;

	// Put a cap on the number of subdivisions.
	numSubdivisions = std::min<uint32_t>(numSubdivisions, 6u);

	// Approximate a sphere by tessellating an icosahedron.

	const float X = 0.525731f;
	const float Z = 0.850651f;

	float pos[36] =
	{
		-X, 0.0f, Z,  
		X, 0.0f, Z,
		-X, 0.0f, -Z,
		X, 0.0f, -Z,
		0.0f, Z, X,  
		0.0f, Z, -X,
		0.0f, -Z, X, 
		0.0f, -Z, -X,
		Z, X, 0.0f,  
		-Z, X, 0.0f,
		Z, -X, 0.0f, 
		-Z, -X, 0.0f
	};

	uint32_t k[60] =
	{
		1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,
		1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,
		3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
		10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7
	};

	meshData.Vertices.resize(12);
	meshData.Indices.assign(&k[0], &k[60]);

	for (uint32_t i = 0; i < 12; ++i)
		for (uint32_t j = 0; j < 3; ++j)
			meshData.Vertices[i].Position[j] = pos[i * 3 + j];

	for (uint32_t i = 0; i < numSubdivisions; ++i)
		Subdivide(meshData);

	// Project vertices onto sphere and scale.
	for (uint32_t i = 0; i < meshData.Vertices.size(); ++i)
	{
		// Project onto unit sphere.
		std::vector<float> n = GGiEngineUtil::GetNormalizedFloat3(meshData.Vertices[i].Position);
		meshData.Vertices[i].Normal[0] = n[0];
		meshData.Vertices[i].Normal[1] = n[1];
		meshData.Vertices[i].Normal[2] = n[2];

		// Project onto sphere.
		// float p[3];
		meshData.Vertices[i].Position[0] = radius * n[0];
		meshData.Vertices[i].Position[1] = radius * n[1];
		meshData.Vertices[i].Position[2] = radius * n[2];

		// Derive texture coordinates from spherical coordinates.
		float theta = atan2f(meshData.Vertices[i].Position[2], meshData.Vertices[i].Position[0]);

		// Put in [0, 2pi].
		if (theta < 0.0f)
			theta += 2 * GGiEngineUtil::PI;

		float phi = acosf(meshData.Vertices[i].Position[2] / radius);

		meshData.Vertices[i].UV[0] = theta / 2 * GGiEngineUtil::PI;
		meshData.Vertices[i].UV[1] = phi / GGiEngineUtil::PI;

		// Partial derivative of P with respect to theta
		meshData.Vertices[i].TangentU[0] = -radius * sinf(phi)*sinf(theta);
		meshData.Vertices[i].TangentU[1] = 0.0f;
		meshData.Vertices[i].TangentU[2] = +radius * sinf(phi)*cosf(theta);

		GGiEngineUtil::NormalizeFloat3(meshData.Vertices[i].TangentU);
	}

	//meshData.Transform = MathHelper::Identity4x4();

	meshData.SubmeshName = L"Geosphere";

	return meshData;
}

GRiMeshData GRiGeometryGenerator::CreateCylinder(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount)
{
	GRiMeshData meshData;

	//
	// Build Stacks.
	// 

	float stackHeight = height / stackCount;

	// Amount to increment radius as we move up each stack level from bottom to top.
	float radiusStep = (topRadius - bottomRadius) / stackCount;

	uint32_t ringCount = stackCount + 1;

	// Compute vertices for each stack ring starting at the bottom and moving up.
	for (uint32_t i = 0; i < ringCount; ++i)
	{
		float y = -0.5f*height + i * stackHeight;
		float r = bottomRadius + i * radiusStep;

		// vertices of ring
		float dTheta = 2.0f*GGiEngineUtil::PI / sliceCount;
		for (uint32_t j = 0; j <= sliceCount; ++j)
		{
			GRiVertex vert;

			float c = cosf(j*dTheta);
			float s = sinf(j*dTheta);

			vert.Position[0] = r*c;
			vert.Position[1] = y;
			vert.Position[2] = r * s;

			vert.UV[0] = (float)j / sliceCount;
			vert.UV[1] = 1.0f - (float)i / stackCount;

			// Cylinder can be parameterized as follows, where we introduce v
			// parameter that goes in the same direction as the v tex-coord
			// so that the bitangent goes in the same direction as the v tex-coord.
			//   Let r0 be the bottom radius and let r1 be the top radius.
			//   y(v) = h - hv for v in [0,1].
			//   r(v) = r1 + (r0-r1)v
			//
			//   x(t, v) = r(v)*cos(t)
			//   y(t, v) = h - hv
			//   z(t, v) = r(v)*sin(t)
			// 
			//  dx/dt = -r(v)*sin(t)
			//  dy/dt = 0
			//  dz/dt = +r(v)*cos(t)
			//
			//  dx/dv = (r0-r1)*cos(t)
			//  dy/dv = -h
			//  dz/dv = (r0-r1)*sin(t)

			// This is unit length.
			vert.TangentU[0] = -s;
			vert.TangentU[1] = 0.0f;
			vert.TangentU[2] = c;

			float dr = bottomRadius - topRadius;
			float bitangent[3] = { dr*c, -height, dr*s };

			// Normal = tangent cross bitangent
			vert.Normal[0] = vert.TangentU[1] * bitangent[2] - vert.TangentU[2] * bitangent[1];
			vert.Normal[1] = vert.TangentU[2] * bitangent[0] - vert.TangentU[0] * bitangent[2];
			vert.Normal[2] = vert.TangentU[0] * bitangent[1] - vert.TangentU[1] * bitangent[0];

			meshData.Vertices.push_back(vert);
		}
	}

	// Add one because we duplicate the first and last GRiVertex per ring
	// since the texture coordinates are different.
	uint32_t ringGRiVertexCount = sliceCount + 1;

	// Compute indices for each stack.
	for (uint32_t i = 0; i < stackCount; ++i)
	{
		for (uint32_t j = 0; j < sliceCount; ++j)
		{
			meshData.Indices.push_back(i*ringGRiVertexCount + j);
			meshData.Indices.push_back((i + 1)*ringGRiVertexCount + j);
			meshData.Indices.push_back((i + 1)*ringGRiVertexCount + j + 1);

			meshData.Indices.push_back(i*ringGRiVertexCount + j);
			meshData.Indices.push_back((i + 1)*ringGRiVertexCount + j + 1);
			meshData.Indices.push_back(i*ringGRiVertexCount + j + 1);
		}
	}

	BuildCylinderTopCap(bottomRadius, topRadius, height, sliceCount, stackCount, meshData);
	BuildCylinderBottomCap(bottomRadius, topRadius, height, sliceCount, stackCount, meshData);

	//meshData.Transform = MathHelper::Identity4x4();

	meshData.SubmeshName = L"Cylinder";

	return meshData;
}

void GRiGeometryGenerator::BuildCylinderTopCap(float bottomRadius, float topRadius, float height,
	uint32_t sliceCount, uint32_t stackCount, GRiMeshData& meshData)
{
	uint32_t baseIndex = (uint32_t)meshData.Vertices.size();

	float y = 0.5f*height;
	float dTheta = 2.0f*GGiEngineUtil::PI / sliceCount;

	// Duplicate cap ring vertices because the texture coordinates and normals differ.
	for (uint32_t i = 0; i <= sliceCount; ++i)
	{
		float x = topRadius * cosf(i*dTheta);
		float z = topRadius * sinf(i*dTheta);

		// Scale down by the height to try and make top cap texture coord area
		// proportional to base.
		float u = x / height + 0.5f;
		float v = z / height + 0.5f;

		meshData.Vertices.push_back(GRiVertex(x, y, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
	}

	// Cap center GRiVertex.
	meshData.Vertices.push_back(GRiVertex(0.0f, y, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

	// Index of center GRiVertex.
	uint32_t centerIndex = (uint32_t)meshData.Vertices.size() - 1;

	for (uint32_t i = 0; i < sliceCount; ++i)
	{
		meshData.Indices.push_back(centerIndex);
		meshData.Indices.push_back(baseIndex + i + 1);
		meshData.Indices.push_back(baseIndex + i);
	}
}

void GRiGeometryGenerator::BuildCylinderBottomCap(float bottomRadius, float topRadius, float height,
	uint32_t sliceCount, uint32_t stackCount, GRiMeshData& meshData)
{
	// 
	// Build bottom cap.
	//

	uint32_t baseIndex = (uint32_t)meshData.Vertices.size();
	float y = -0.5f*height;

	// vertices of ring
	float dTheta = 2.0f*GGiEngineUtil::PI / sliceCount;
	for (uint32_t i = 0; i <= sliceCount; ++i)
	{
		float x = bottomRadius * cosf(i*dTheta);
		float z = bottomRadius * sinf(i*dTheta);

		// Scale down by the height to try and make top cap texture coord area
		// proportional to base.
		float u = x / height + 0.5f;
		float v = z / height + 0.5f;

		meshData.Vertices.push_back(GRiVertex(x, y, z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
	}

	// Cap center GRiVertex.
	meshData.Vertices.push_back(GRiVertex(0.0f, y, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

	// Cache the index of center GRiVertex.
	uint32_t centerIndex = (uint32_t)meshData.Vertices.size() - 1;

	for (uint32_t i = 0; i < sliceCount; ++i)
	{
		meshData.Indices.push_back(centerIndex);
		meshData.Indices.push_back(baseIndex + i);
		meshData.Indices.push_back(baseIndex + i + 1);
	}
}

GRiMeshData GRiGeometryGenerator::CreateGrid(float width, float depth, uint32_t m, uint32_t n)
{
	GRiMeshData meshData;

	uint32_t GRiVertexCount = m * n;
	uint32_t faceCount = (m - 1)*(n - 1) * 2;

	//
	// Create the vertices.
	//

	float halfWidth = 0.5f*width;
	float halfDepth = 0.5f*depth;

	float dx = width / (n - 1);
	float dz = depth / (m - 1);

	float du = 1.0f / (n - 1);
	float dv = 1.0f / (m - 1);

	meshData.Vertices.resize(GRiVertexCount);
	for (uint32_t i = 0; i < m; ++i)
	{
		float z = halfDepth - i * dz;
		for (uint32_t j = 0; j < n; ++j)
		{
			float x = -halfWidth + j * dx;

			meshData.Vertices[i*n + j].Position[0] = x;
			meshData.Vertices[i*n + j].Position[1] = 0.0f;
			meshData.Vertices[i*n + j].Position[2] = z;
			meshData.Vertices[i*n + j].Normal[0] = 0.0f;
			meshData.Vertices[i*n + j].Normal[1] = 1.0f;
			meshData.Vertices[i*n + j].Normal[2] = 0.0f;
			meshData.Vertices[i*n + j].TangentU[0] = 1.0f;
			meshData.Vertices[i*n + j].TangentU[1] = 0.0f;
			meshData.Vertices[i*n + j].TangentU[2] = 0.0f;

			// Stretch texture over grid.
			meshData.Vertices[i*n + j].UV[0] = j * du;
			meshData.Vertices[i*n + j].UV[1] = i * dv;
		}
	}

	//
	// Create the indices.
	//

	meshData.Indices.resize(faceCount * 3); // 3 indices per face

	// Iterate over each quad and compute indices.
	uint32_t k = 0;
	for (uint32_t i = 0; i < m - 1; ++i)
	{
		for (uint32_t j = 0; j < n - 1; ++j)
		{
			meshData.Indices[k] = i * n + j;
			meshData.Indices[k + 1] = i * n + j + 1;
			meshData.Indices[k + 2] = (i + 1)*n + j;

			meshData.Indices[k + 3] = (i + 1)*n + j;
			meshData.Indices[k + 4] = i * n + j + 1;
			meshData.Indices[k + 5] = (i + 1)*n + j + 1;

			k += 6; // next quad
		}
	}

	//meshData.Transform = MathHelper::Identity4x4();

	meshData.SubmeshName = L"Grid";

	return meshData;
}

GRiMeshData GRiGeometryGenerator::CreateQuad(float x, float y, float w, float h, float depth)
{
	GRiMeshData meshData;

	meshData.Vertices.resize(4);
	meshData.Indices.resize(6);

	// Position coordinates specified in NDC space.
	meshData.Vertices[0] = GRiVertex(
		x, y, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f);

	meshData.Vertices[1] = GRiVertex(
		x, y + h, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 0.0f);

	meshData.Vertices[2] = GRiVertex(
		x + w, y+h, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f);

	meshData.Vertices[3] = GRiVertex(
		x + w, y, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 1.0f);

	meshData.Indices[0] = 0;
	meshData.Indices[1] = 1;
	meshData.Indices[2] = 2;

	meshData.Indices[3] = 0;
	meshData.Indices[4] = 2;
	meshData.Indices[5] = 3;

	//meshData.Transform = MathHelper::Identity4x4();

	meshData.SubmeshName = L"Quad";

	return meshData;
}

