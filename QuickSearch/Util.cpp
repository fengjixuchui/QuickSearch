#include "Util.h"
#include <tchar.h>
std::string WCharToBytes(wchar_t * wchr)
{
    std::wstring wstr(wchr);
    int wstrLen = wstr.length() + 1;
    int strLen = ::WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)wstr.c_str(), wstrLen, NULL, NULL, NULL, NULL);
    char* buf = new char[strLen];
    ::WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)wstr.c_str(), wstrLen, buf, strLen, NULL, NULL);
    std::string tmpStr(buf);
    delete[] buf;
    return tmpStr;
}

std::wstring BytesToWChar(char * chr)
{
    std::string str(chr);
    int strLen = str.length() + 1;
    int wstrLen = 0;
    wstrLen = ::MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)str.c_str(), wstrLen, NULL, NULL);
    wchar_t* buf = new wchar_t[wstrLen];
    ::MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)str.c_str(), strLen, buf, wstrLen);
    std::wstring tmpStr(buf);
    delete[] buf;
    return tmpStr;
}

std::wstring GetInstallSize(DWORD dwInstallSize)
{
    TCHAR str[20] = { 0 };
    if (dwInstallSize > (DWORD)1024 * 1024 * 1024)  //1G
    {
        _sntprintf_s(str, _countof(str), _countof(str), _T("%.1fGB"), dwInstallSize / (1024 * 1024 * 1024.0));
    }
    else if (dwInstallSize > 10 * 1024 * 1024)
    {
        _sntprintf_s(str, _countof(str), _countof(str), _T("%.0fMB"), dwInstallSize / (1024 * 1024.0));
    }
    else if (dwInstallSize > 1024 * 1024)
    {
        _sntprintf_s(str, _countof(str), _countof(str), _T("%.2fMB"), dwInstallSize / (1024 * 1024.0));
    }
    else if (dwInstallSize > 0)
    {
        _sntprintf_s(str, _countof(str), _countof(str), _T("%.0fKB"), dwInstallSize / 1024.0);
    }
    return std::wstring(str);
}

void StringFillString(std::wstring & source, std::wstring fillStr, std::wstring referenceStr)
{
    size_t nPos = 0;
    while ((nPos = source.find(referenceStr, nPos)) != source.npos)
    {
        source.insert(nPos, fillStr);
        nPos += (referenceStr.size() + fillStr.size());
        source.insert(nPos, fillStr);
    }
}
