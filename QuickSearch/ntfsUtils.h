#pragma once
#include <Windows.h>
#include <WinIoCtl.h>
#include <string>
#include "global.h"
namespace NtfsUtils
{
	void QueryFileInfoByFRN(HANDLE hVol, DWORDLONG FRN, DWORD dwIndex, DWORDLONG *pdwMfTime, DWORDLONG *pdwFileSize);
	BOOL QueryVolumeFRecordInfo(HANDLE hVolume, DWORD& dwFRLen, DWORD& dwBytesPerCluster);
    BOOL QueryVolumeSpaceInfo(std::string root, DWORD& dwTotalSpace, DWORD& dwFreeSpace);
    BOOL QueryVolumeUsnJournal(HANDLE hVolume, DWORD dwVolIndex, PUSN_JOURNAL_DATA pUsnJournalData);
    DWORD QueryVolumeFileSys(char *pNameBuffer);
    BOOL USNRecord2FileEntry(PUSN_RECORD pRecord, int volIndex, FileEntry& fileEntry);
    BOOL GetFilePath(int nVol, PFileEntry pfileEntry, std::wstring& path);
};