#include "stdafx.h"
#include "GGiCpuProfiler.h"






GGiCpuProfiler::GGiCpuProfiler()
{
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	mSecondsPerCount = 1.0 / (double)countsPerSec;
}

GGiCpuProfiler& GGiCpuProfiler::GetInstance()
{
	static GGiCpuProfiler *instance = new GGiCpuProfiler();
	return *instance;
}

void GGiCpuProfiler::BeginFrame()
{
	mProfiles.clear();
}

void GGiCpuProfiler::StartCpuProfile(std::string profileName)
{
	CpuProfileData profile;
	profile.name = profileName;

	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	profile.startTime = double(currTime * mSecondsPerCount * 1000.0f);

	mProfiles.push_back(profile);
}

void GGiCpuProfiler::EndCpuProfile(std::string profileName)
{
	auto it = mProfiles.begin();
	for (it = mProfiles.begin(); it != mProfiles.end(); it++)
	{
		if (it->name == profileName)
			break;
	}

	if (it == mProfiles.end())
		return;

	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	it->endTime = double(currTime * mSecondsPerCount * 1000.0f);
}

float GGiCpuProfiler::GetProfileByName(std::string name)
{
	auto it = mProfiles.begin();
	for (it = mProfiles.begin(); it != mProfiles.end(); it++)
	{
		if (it->name == name)
			break;
	}

	if (it == mProfiles.end())
		return -1.0f;
	else
		return (it->endTime - it->startTime);
}

std::vector<CpuProfileData> GGiCpuProfiler::GetProfiles()
{
	return mProfiles;
}




