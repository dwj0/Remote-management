/*
** FileName     : CodeConverter.h
** Author       : pigautumn
** Date         : 2016/8/23
** Description  : 编码转换类（提供静态方法）
*/

#pragma once

#include <string>
#include <xstring>

using std::string;
using std::wstring;

class CodeConverter
{
public:
	static wstring AsciiToUnicode(const string& ascii_string);		//ASCII转Unicode
	static string AsciiToUtf8(const string& ascii_string);			//ASCII转UTF8

	static string UnicodeToAscii(const wstring& unicode_string);	//Unicode转ASCII
	static string UnicodeToUtf8(const wstring& unicode_string);		//Unicode转UTF8

	static string Utf8ToAscii(const string& utf8_string);			//UTF8转ASCII
	static wstring Utf8ToUnicode(const string& utf8_string);		//UTF8转Unicode

	static string BSTRToString(const BSTR& str);					//BSTR转string
	static BSTR StringToBSTR(const string& str);					//string转BSTR
};
