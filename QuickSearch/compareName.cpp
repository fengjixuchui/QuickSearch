#include "compareName.h"
#include <string>
#include <locale>
#include <codecvt>
#include <algorithm>
bool CompareName(char* chr1,char* chr2)
{
    //std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    std::string str(chr1);
    //int sLenth = str.length() + 1;
    int wstrLen = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)str.c_str(), -1,NULL,NULL);
    wchar_t* wcharArr1 = new wchar_t[wstrLen];
    memset(wcharArr1, 0, wstrLen);
    MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)str.c_str(), -1, (LPWSTR)wcharArr1, wstrLen);

    std::string str2(chr2);
    //sLenth = str2.length() + 1;
    wstrLen = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)str2.c_str(), -1, NULL, NULL);
    wchar_t* wcharArr2 = new wchar_t[wstrLen];
    memset(wcharArr2, 0, wstrLen);
    MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)str2.c_str(), -1, (LPWSTR)wcharArr2, wstrLen);

    
    int nRet = wcscmp(wcharArr1, wcharArr2);
    delete[] wcharArr1;
    delete[] wcharArr2;
    if (nRet == CSTR_LESS_THAN)
    {
        return true;
    }
    /*if (nRet == CSTR_GREATER_THAN)
    {
        return false;
    }*/
    return false;
}

bool SUBSTR(std::wstring& fileName, std::wstring searchName)
{
    //int nRet = strToLower(fileName).find(strToLower(searchName));
    int nRet = fileName.find(searchName);
    if (nRet != std::wstring::npos && nRet < fileName.length())
    {
        return true;
    }
    return false;
}

std::wstring strToLower(std::wstring& str)
{
    std::wstring strTmp = str;
    transform(strTmp.begin(), strTmp.end(), strTmp.begin(), towlower);
    return strTmp;
}


