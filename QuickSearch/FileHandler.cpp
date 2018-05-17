#include "FileHandler.h"
#include <ShlObj.h>
#include <Shlwapi.h>
#include <assert.h>

#pragma comment(lib, "shlwapi.lib")
namespace FileHandler
{
    BOOL CreateDirectory(const TCHAR * lpszDirName)
    {

        assert(lpszDirName != NULL);
        if (lpszDirName == NULL || _tcslen(lpszDirName) == 0)
            return FALSE;

        if (lpszDirName[1] != _T(':') || lpszDirName[2] != _T('\\'))
            return FALSE;

        int DirNameLen = (int)_tcslen(lpszDirName);
        TCHAR *pTempDirName = new TCHAR[DirNameLen + sizeof(TCHAR) * 2];
        lstrcpyn(pTempDirName, lpszDirName, DirNameLen + 1);
        if (pTempDirName[DirNameLen - 1] != _T('\\'))
            lstrcpyn(pTempDirName + DirNameLen, _T("\\"), 2);

        BOOL bRet = TRUE;
        TCHAR *pEndPos = pTempDirName + 3;
        while (1)
        {
            pEndPos = _tcschr(pEndPos, _T('\\'));
            if (pEndPos == NULL)
                break;
            *pEndPos = NULL;

            if (!IsDirExist(pTempDirName))
            {
                if (!::CreateDirectory(pTempDirName, NULL))
                {
                    bRet = FALSE;
                    break;
                }
            }
            *pEndPos = _T('\\');
            pEndPos++;
        }

        delete[]pTempDirName;
        return bRet;
    }

    BOOL DeleteFile(const TCHAR *  path)
    {
        WIN32_FILE_ATTRIBUTE_DATA attrs = { 0 };
        if (::GetFileAttributesEx(path, ::GetFileExInfoStandard, &attrs))
        {
            ::DeleteFile(path);
            return TRUE;
        }
        return FALSE;
    }

    BOOL SplitFileType(const std::wstring& filename, std::wstring& filetype)
    {
        size_t uPos = filename.find_last_of(_T("."), filename.size());
        if (uPos != std::wstring::npos)
            filetype = filename.substr(uPos, filename.size());
        return filetype.size() > 0;
    }

    void GetAppStorePath(TCHAR * ptszFilePath)
    {
        TCHAR wcFolder[MAX_PATH + 1] = { L'Q', L'u', L'i', L'c', L'k', L'S', L'e', L'a', L'r', L's', L'h', 0, 0, 0 };
        TCHAR wcFileFullPath[2 * MAX_PATH + 1] = { 0 };
        LPITEMIDLIST pidl = NULL;
        HRESULT hr = ::SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);
        if (SUCCEEDED(hr) && pidl)
        {
            ::SHGetPathFromIDList(pidl, wcFileFullPath);
            ::CoTaskMemFree(pidl);

        }
        ::PathAppend(wcFileFullPath, wcFolder);
        if (!IsDirExist(wcFileFullPath))
        {
            CreateDirectory(wcFileFullPath);
        }

        _tcsncpy_s(ptszFilePath, MAX_PATH, wcFileFullPath, MAX_PATH);
    }

    BOOL IsDirExist(const TCHAR * lpszDirName)
    {
        assert(lpszDirName != NULL);
        DWORD dwAttr = GetFileAttributes(lpszDirName);
        return ((dwAttr != INVALID_FILE_ATTRIBUTES)
            && (dwAttr & FILE_ATTRIBUTE_DIRECTORY));
    }
}




