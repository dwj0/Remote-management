
#include "stdafx.h"
#include <windows.h>
#include "CodeConverter.h"
#include <comdef.h>

#include <vector>
using std::vector;

//ASCII转Unicode
wstring CodeConverter::AsciiToUnicode(const string& ascii_string)
{
	wstring unicode_string;

	//CP_ACP - default to ANSI code page
	int len = MultiByteToWideChar(CP_ACP, 0, ascii_string.c_str(), -1, NULL, 0);
	if (ERROR_NO_UNICODE_TRANSLATION == len || 0 == len)
	{
		//return empty wstring
		return unicode_string;
	}

	vector<wchar_t> vec_result(len);
	int result_len = MultiByteToWideChar(CP_ACP, 0, ascii_string.c_str(), -1, &vec_result[0], len);
	if (result_len != len)
	{
		//return empty wstring
		return unicode_string;
	}

	unicode_string = wstring(&vec_result[0]);
	return unicode_string;
}

//ASCII转UTF8
string CodeConverter::AsciiToUtf8(const string& ascii_string)
{
	wstring unicode_string = AsciiToUnicode(ascii_string);	//将ASCII转换为Unicode
	string utf8_string = UnicodeToUtf8(unicode_string);		//将Unicode转换为UTF8
	return utf8_string;
}

//Unicode转ASCII
string CodeConverter::UnicodeToAscii(const wstring& unicode_string)
{
	string ascii_string;

	//CP_OEMCP - default to OEM  code page
	int len = WideCharToMultiByte(CP_OEMCP, 0, unicode_string.c_str(), -1, NULL, 0, NULL, NULL);
	if (ERROR_NO_UNICODE_TRANSLATION == len || 0 == len)
	{
		//return empty wstring
		return ascii_string;
	}

	vector<char> vec_result(len);
	int result_len = WideCharToMultiByte(CP_OEMCP, 0, unicode_string.c_str(), -1, &vec_result[0], len, NULL, NULL);;
	if (result_len != len)
	{
		//return empty wstring
		return ascii_string;
	}

	ascii_string = string(&vec_result[0]);
	return ascii_string;
}

//Unicode转UTF8
string CodeConverter::UnicodeToUtf8(const wstring& unicode_string)
{
	string utf8_string;

	//CP_UTF8 - UTF-8 translation
	int len = WideCharToMultiByte(CP_UTF8, 0, unicode_string.c_str(), -1, NULL, 0, NULL, NULL);
	if (0 == len)
	{
		//return empty wstring
		return utf8_string;
	}

	vector<char> vec_result(len);
	int result_len = WideCharToMultiByte(CP_UTF8, 0, unicode_string.c_str(), -1, &vec_result[0], len, NULL, NULL);;
	if (result_len != len)
	{
		//return empty wstring
		return utf8_string;
	}

	utf8_string = string(&vec_result[0]);
	return utf8_string;
}

//UTF8转ASCII
string CodeConverter::Utf8ToAscii(const string& utf8_string)
{
	wstring unicode_string = Utf8ToUnicode(utf8_string);		//将UTF8转换为Unicode
	string ascii_string = UnicodeToAscii(unicode_string);	//将Unicode转换为ASCII
	return ascii_string;
}

//UTF8转Unicode
wstring CodeConverter::Utf8ToUnicode(const string& utf8_string)
{
	wstring unicode_string;

	//CP_UTF8 - UTF-8 translation
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8_string.c_str(), -1, NULL, 0);
	if (ERROR_NO_UNICODE_TRANSLATION == len || 0 == len)
	{
		//return empty wstring
		return unicode_string;
	}

	vector<wchar_t> vec_result(len);
	int result_len = MultiByteToWideChar(CP_UTF8, 0, utf8_string.c_str(), -1, &vec_result[0], len);
	if (result_len != len)
	{
		//return empty wstring
		return unicode_string;
	}

	unicode_string = wstring(&vec_result[0]);
	return unicode_string;
}

//BSTR转string
string CodeConverter::BSTRToString(const BSTR& str)
{
	string newStr = (_bstr_t)str;
	return newStr;
}

//string转BSTR
BSTR CodeConverter::StringToBSTR(const string& str)
{
	_variant_t strVar(str.c_str());
	return strVar.bstrVal;
}