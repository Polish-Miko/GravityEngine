#pragma once
#include "GGiPreInclude.h"
#include "GGiException.h"
#include "GGiEngineUtil.h"


#define ThrowGGiException(x)											\
{																		\
	GGiException::Throw(x, __FILE__, __LINE__);							\
}																		\

class GGiException : public std::exception
{

public:

	GGiException() = delete;
	~GGiException() {}

	GGiException(std::wstring description, const std::string& fileName, int lineNumber) :
		mDescription(description),
		mFileName(ToWString(fileName)),
		mLineNumber(std::to_wstring(lineNumber))
	{}

	GGiException(std::string description, const std::string& fileName, int lineNumber) :
		mDescription(GGiEngineUtil::StringToWString(description)),
		mFileName(ToWString(fileName)),
		mLineNumber(std::to_wstring(lineNumber))
	{}

	std::wstring mDescription = L"Undefined";

	std::wstring mFileName = L"Undefined";

	std::wstring mLineNumber = L"Undefined";

	static inline std::wstring ToWString(const std::string& str)
	{
		WCHAR buffer[512];
		MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
		return std::wstring(buffer);
	}

	static inline void Throw(std::wstring description, const std::string& fileName, int lineNumber)
	{
		throw new GGiException(description, fileName, lineNumber);
	}

	static inline void Throw(std::string description, const std::string& fileName, int lineNumber)
	{
		throw new GGiException(description, fileName, lineNumber);
	}

	std::wstring GetErrorMessage()
	{
		std::wstring message = L"Error description : " + mDescription + L"\nThrown in file : " + mFileName + L"\nLine : " + mLineNumber;
		return message;
	}

};