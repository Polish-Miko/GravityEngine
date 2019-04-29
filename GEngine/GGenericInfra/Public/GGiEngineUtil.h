#pragma once
#include "GGiPreInclude.h"


class GGiEngineUtil
{
public:

	GGiEngineUtil();
	~GGiEngineUtil();

	static const float PI;
	static const float Infinity;

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

	static void NormalizeFloat3(float& x, float& y, float& z)
	{
		float l = (float)pow(x * x + y * y + z * z, 0.5);
		x /= l;
		y /= l;
		z /= l;
	}

	static std::vector<float> GetNormalizedFloat3(float* x)
	{
		std::vector<float> y(3);
		float l = (float)pow(x[0] * x[0] + x[1] * x[1] + x[2] * x[2], 0.5);
		y[0] = x[0] / l;
		y[1] = x[1] / l;
		y[2] = x[2] / l;
		return y;
	}

	static void NormalizeFloat3(float* x)
	{
		float l = (float)pow(x[0] * x[0] + x[1] * x[1] + x[2] * x[2], 0.5);
		x[0] /= l;
		x[1] /= l;
		x[2] /= l;
	}

	static std::vector<float> Normalize(std::vector<float> in)
	{
		float size = 0;
		for (auto i : in)
		{
			size = size + i * i;
		}
		size = (float)pow(size, 0.5);
		std::vector<float> out;
		for (auto i : in)
		{
			out.push_back(i / size);
		}
		return out;
	}

	static std::wstring GetExtension(std::wstring path)
	{
		if ((path.rfind('.') != std::wstring::npos) && (path.rfind('.') != (path.length() - 1)))
			return path.substr(path.rfind('.') + 1);
		else
			return L"";
	}

	static std::wstring GetFileNameAndExtension(std::wstring path)
	{
		if ((path.rfind('\\') != std::wstring::npos) && (path.rfind('\\') != (path.length() - 1)))
			return path.substr(path.rfind('\\') + 1);
		else
			return L"";
	}

	static std::wstring GetFileName(std::wstring path)
	{
		auto slash = path.rfind('\\');
		auto dot = path.rfind('.');
		if ((slash != std::wstring::npos) &&
			(slash != (path.length() - 1)) &&
			(dot != std::wstring::npos) &&
			(dot > slash))
			return path.substr(path.rfind('\\') + 1, dot - slash - 1);
		else
			return L"";
	}

	static std::vector<float> CrossFloat3(std::vector<float> i1, std::vector<float> i2)
	{
		std::vector<float> o(3);
		o[0] = i1[1] * i2[2] - i2[1] * i1[2];
		o[1] = i1[2] * i2[0] - i1[0] * i2[2];
		o[2] = i1[0] * i2[1] - i2[0] * i1[1];
		return o;
	}

	static std::vector<float> VectorSubtract(std::vector<float> i1, std::vector<float> i2)
	{
		size_t size = min(i1.size(), i2.size());
		std::vector<float> o(size);
		for (auto i = 0u; i < size; i++)
			o[i] = i1[i] - i2[i];
		return o;
	}

	static std::vector<float> VectorAdd(std::vector<float> i1, std::vector<float> i2)
	{
		size_t size = min(i1.size(), i2.size());
		std::vector<float> o(size);
		for (auto i = 0u; i < size; i++)
			o[i] = i1[i] + i2[i];
		return o;
	}

	static std::vector<float> VectorMultiply(std::vector<float> i1, std::vector<float> i2)
	{
		size_t size = min(i1.size(), i2.size());
		std::vector<float> o(size);
		for (auto i = 0u; i < size; i++)
			o[i] = i1[i] * i2[i];
		return o;
	}

	static float VectorDotProduct(std::vector<float> i1, std::vector<float> i2)
	{
		size_t size = min(i1.size(), i2.size());
		float o = 0;
		for (auto i = 0u; i < size; i++)
			o += i1[i] * i2[i];
		return o;
	}

};
