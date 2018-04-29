#pragma once
#include <tchar.h>
#include <windef.h>

namespace CFileHandler
{
    void GetAppStorePath(TCHAR * ptszFilePath);
    BOOL IsDirExist(const TCHAR * lpszDirName);
    BOOL CreateDirectory(const TCHAR* lpszDirName);
}
