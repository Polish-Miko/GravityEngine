#pragma once
#include "GRiPreInclude.h"
#include "GRiSubmesh.h"


class GRiMesh
{
public:

	GRiMesh() = default;
	//GRiMesh(const GRiMesh& rhs) = delete;
	~GRiMesh() = default;

	// Unique name for hash table lookup.
	std::wstring UniqueName;

	std::wstring Name;

	bool bIsSkeletalMesh = false;

	std::unordered_map<std::wstring, GRiSubmesh> Submeshes;

	virtual void AllowDynamicCast() {}

};

