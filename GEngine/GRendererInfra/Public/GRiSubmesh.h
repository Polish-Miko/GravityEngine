#pragma once
#include "GRiPreInclude.h"


class GRiSubmesh
{

public:

	GRiSubmesh();
	~GRiSubmesh();

	std::wstring Name;

	UINT IndexCount = 0;     

	UINT StartIndexLocation = 0;
	INT BaseVertexLocation = 0;

};

