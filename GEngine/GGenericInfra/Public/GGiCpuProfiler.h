#pragma once
#include "GGiPreInclude.h"





struct CpuProfileData
{
	std::string name;
	double startTime;
	double endTime;
};

class GGiCpuProfiler
{

public:

	GGiCpuProfiler(const GGiCpuProfiler& rhs) = delete;

	GGiCpuProfiler& operator=(const GGiCpuProfiler& rhs) = delete;

	~GGiCpuProfiler() = default;

	static GGiCpuProfiler& GetInstance();

	void StartCpuProfile(std::string profileName);
	void EndCpuProfile(std::string profileName);

	float GetProfileByName(std::string name);
	std::vector<CpuProfileData> GetProfiles();

	void BeginFrame();

private:

	GGiCpuProfiler();

	double mSecondsPerCount;

	std::vector<CpuProfileData> mProfiles;

};

