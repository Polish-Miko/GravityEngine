#pragma once
#include "GGiPreInclude.h"


class GGiEngineUtil
{
public:

	GGiEngineUtil();
	~GGiEngineUtil();

	static const float PI;

	static std::wstring StringToWString(const std::string& str)
	{
		int num = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
		wchar_t *wide = new wchar_t[num];
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wide, num);
		std::wstring w_str(wide);
		delete[] wide;
		return w_str;
	}

	static std::string WStringToString(const std::wstring &wstr)
	{
		std::string str;
		int nLen = (int)wstr.length();
		str.resize(nLen, ' ');
		int nResult = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)wstr.c_str(), nLen, (LPSTR)str.c_str(), nLen, NULL, NULL);
		if (nResult == 0)
		{
			return "";
		}
		return str;
	}
};

