#pragma once
#include <tchar.h>
#include <windef.h>
#include <string>
namespace FileHandler
{
    void GetAppStorePath(TCHAR * ptszFilePath);
    BOOL IsDirExist(const TCHAR * lpszDirName);
    BOOL CreateDirectory(const TCHAR* lpszDirName);
    BOOL DeleteFile(const TCHAR* path);
    BOOL SplitFileType(const std::wstring& filename, std::wstring& filetype);
}
