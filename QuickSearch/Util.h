#pragma once
#include <iostream>
#include <string>
#include <windows.h>
std::string WCharToBytes(wchar_t* wchr);
std::wstring BytesToWChar(char* chr);

std::wstring GetInstallSize(DWORD dwInstallSize);
void StringFillString(std::wstring& source, std::wstring fillStr, std::wstring referenceStr);