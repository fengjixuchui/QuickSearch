#include "ntfsUtils.h"
#include "NtfsStructure.h"
#include "NtfsMgr.h"
#include "FileNameMemoryManager.h"
#include "Volume.h"

namespace NtfsUtils
{
    const DWORDLONG  MAXINUMSIZE = 0x4000000;    // USN Journal max size in bytes
    const DWORDLONG  ALLOCATIONDELTA = 0x800000; // 超过max后仍可用的字节数

    //返回LCN-d-gap  注意d-gap可为负
    __forceinline LONGLONG RunLCN(PUCHAR run)
    {
        UCHAR n1 = *run & 0x0f;
        UCHAR n2 = (*run >> 4) & 0x0f;
        if (0 == n2) return 0;
        LONGLONG lcn = (CHAR)(run[n1 + n2]);//带符号转换
        LONG i = 0;
        for (i = n1 + n2 - 1; i > n1; --i)
            lcn = (lcn << 8) + run[i];
        return lcn;
    }

    __forceinline ULONGLONG RunCount(PUCHAR run)
    {
        UCHAR n = *run & 0xf;
        ULONGLONG count = 0;
        ULONG i = 0;
        for (i = n; i > 0; i--)
            count = (count << 8) + run[i];
        return count;
    }

    LONGLONG  FileTimeToTime_t(FILETIME  ft)
    {
        LONGLONG  ll;
        ULARGE_INTEGER            ui;
        ui.LowPart = ft.dwLowDateTime;
        ui.HighPart = ft.dwHighDateTime;
        ll = (LONGLONG)ft.dwHighDateTime;
        ll = ((LONGLONG)ll << 32) + ft.dwLowDateTime;
        return ((LONGLONG)(ui.QuadPart - 116444736000000000) / 10000000);
    }

    BOOL CreateUsnJournal(HANDLE hVolume, DWORDLONG MaximumSize, DWORDLONG AllocationDelta)
    {
        DWORD cb;
        CREATE_USN_JOURNAL_DATA cujd;
        cujd.MaximumSize = MaximumSize;
        cujd.AllocationDelta = AllocationDelta;
        BOOL bRet = ::DeviceIoControl(hVolume, FSCTL_CREATE_USN_JOURNAL,
            &cujd, sizeof(cujd), NULL, 0, &cb, NULL);
        DWORD error = ::GetLastError();
        return bRet;
    }

    BOOL DeleteUsnJournal(HANDLE hVolume, DWORDLONG UsnJournalID, DWORD DeleteFlags)
    {
        DWORD cb;
        DELETE_USN_JOURNAL_DATA dujd;
        dujd.UsnJournalID = UsnJournalID;
        dujd.DeleteFlags = DeleteFlags;
        BOOL fOk = ::DeviceIoControl(hVolume, FSCTL_DELETE_USN_JOURNAL,
            &dujd, sizeof(dujd), NULL, 0, &cb, NULL);
        return (fOk);
    }

    BOOL SalculateUsnLogSize(DWORD dwVolIndex, DWORDLONG curUsnMaxSize, DWORDLONG& salcUsnMaxSize, DWORDLONG& salcAllocation)
    {
        //第一个参数是卷中日志的最大字节数，应该占到卷大小的很小的百分比，并且限制最大是4GB。举个例子，一个9GB的卷中，合理的日志大小是8MB
        //，第二个参数指定了当日志需要扩容时的字节数。应该为卷簇大小的偶数倍，是MaximumSize值的八分之一到四分之一
        DWORDLONG dwTmpUsnMaxSize = salcUsnMaxSize;
        if (dwVolIndex < 0 || dwVolIndex >= DIRVE_COUNT) return FALSE;
        DWORDLONG dwVolumeTotalSpace = (DWORDLONG)g_ArrayVolumeInfo[dwVolIndex].m_dwVolumeTotalSpace;
        DWORDLONG dwBytesPerCluster = (DWORDLONG)g_ArrayVolumeInfo[dwVolIndex].m_dwBytesPerCluster;
        if (dwVolumeTotalSpace <= 0 || dwBytesPerCluster <= 0) return FALSE;

        dwVolumeTotalSpace = dwVolumeTotalSpace * 1024 * 1024;
        if (dwVolumeTotalSpace > dwTmpUsnMaxSize)
            dwTmpUsnMaxSize = dwVolumeTotalSpace;
        if (curUsnMaxSize == 0 || curUsnMaxSize * 12 / 10 < dwTmpUsnMaxSize || curUsnMaxSize * 12 / 10 > dwTmpUsnMaxSize)
        {
            //限定最大值是4GB.
            DWORDLONG dwMaxUsnMaxSize = (DWORDLONG)4096 * (1024 * 1024);
            if (dwTmpUsnMaxSize > dwMaxUsnMaxSize)
                dwTmpUsnMaxSize = dwMaxUsnMaxSize;
            salcUsnMaxSize = dwTmpUsnMaxSize;
            //取八分之一
            salcAllocation = salcUsnMaxSize / 8;
            //卷簇大小的偶数倍
            salcAllocation = (salcAllocation / dwBytesPerCluster / 2) * 2 * dwBytesPerCluster;
            return TRUE;
        }
        return FALSE;
    }
    void QueryFileInfoByFRN(HANDLE hVol, DWORDLONG FRN, DWORD dwIndex, DWORDLONG * pdwMfTime, DWORDLONG * pdwFileSize)
    {
        DWORD dwBytesPerCluster = g_ArrayVolumeInfo[dwIndex].m_dwBytesPerCluster;
        DWORD dwFileRecSize = g_ArrayVolumeInfo[dwIndex].m_dwFileRecordSize;


        PNTFS_FILE_RECORD_OUTPUT_BUFFER pMftRecord = (PNTFS_FILE_RECORD_OUTPUT_BUFFER)malloc(dwFileRecSize);
        PFILE_RECORD_HEADER pfileRecordheader = (PFILE_RECORD_HEADER)pMftRecord->FileRecordBuffer;
        PATTRIBUTE pAttribute = NULL;

        NTFS_FILE_RECORD_INPUT_BUFFER mftRecordInput;
        mftRecordInput.FileReferenceNumber.QuadPart = 0xffffffffffff & FRN;    //取后六个字节，有效（MFT序号）

        DWORD dwRet = 0;
        DeviceIoControl(hVol, FSCTL_GET_NTFS_FILE_RECORD, &mftRecordInput, sizeof(mftRecordInput)
            , pMftRecord, dwFileRecSize, &dwRet, NULL);
        //valid file
        
        if (0x01 & pfileRecordheader->Flags)
        {
            //获取文件修改时间
            pAttribute = (PATTRIBUTE)((PBYTE)pfileRecordheader + pfileRecordheader->AttributeOffset);
            if (AttributeStandardInformation == pAttribute->AttributeType)
            {
                PSTANDARD_INFORMATION pStandardFileNameAttr;
                pStandardFileNameAttr = PSTANDARD_INFORMATION((PBYTE)pAttribute + PRESIDENT_ATTRIBUTE(pAttribute)->ValueOffset);
                *pdwMfTime = FileTimeToTime_t(pStandardFileNameAttr->ChangeTime);
            }

            //获取文件大小
            if ((0x02 & pfileRecordheader->Flags) == 0)
            {
                //读80属性获得文件大小
                for (pAttribute = (PATTRIBUTE)((PBYTE)pfileRecordheader + pfileRecordheader->AttributeOffset)
                    ; pAttribute->AttributeType != AttributeEnd && pAttribute->AttributeType < AttributeData && pAttribute->AttributeType != 0
                    ; pAttribute = (PATTRIBUTE)((PBYTE)pAttribute + pAttribute->Length)
                    );
                if (pAttribute->AttributeType == AttributeEnd || pAttribute->AttributeType > AttributeData) 
                {
                    for (pAttribute = (PATTRIBUTE)((PBYTE)pfileRecordheader + pfileRecordheader->AttributeOffset)
                        ; pAttribute->AttributeType < AttributeAttributeList
                        ; pAttribute = (PATTRIBUTE)((PBYTE)pAttribute + pAttribute->Length)
                        );
                    if (pAttribute->AttributeType > AttributeAttributeList) {
                        LOG(ERROR) << __FUNCTIONW__ << " 没有找到80属性也没有20属性 FRN:" << FRN;
                    }
                    else 
                    {
                        PATTRIBUTE_LIST pAttriList;
                        if (pAttribute->Nonresident) {
                            PNONRESIDENT_ATTRIBUTE pNonResident = PNONRESIDENT_ATTRIBUTE(pAttribute);
                            PBYTE pRun = (PBYTE)pAttribute + pNonResident->RunArrayOffset;
                            ULONGLONG Lcn = RunLCN(pRun);//只获取第1簇                              
                            ULONGLONG nCount = RunCount(pRun);
                            LARGE_INTEGER file_offset;
                            file_offset.QuadPart = Lcn*dwBytesPerCluster;
                            SetFilePointerEx(hVol, file_offset, NULL, FILE_BEGIN);
                            PBYTE   pBuffferRead = (PBYTE)malloc(dwBytesPerCluster);
                            DWORD   dwRead = 0;
                            ReadFile(hVol, pBuffferRead, dwBytesPerCluster, &dwRead, NULL);
                            PBYTE   pBufferEnd = pBuffferRead + dwRead;
                            for (pAttriList = PATTRIBUTE_LIST(pBuffferRead);
                                pAttriList->AttributeType != AttributeData && pAttriList->AttributeType != 0;
                                pAttriList = PATTRIBUTE_LIST(PBYTE(pAttriList) + pAttriList->Length)
                                );
                            if (pAttriList->AttributeType == AttributeData)
                            {
                                mftRecordInput.FileReferenceNumber.QuadPart = 0xffffffffffff & pAttriList->FileReferenceNumber;
                                DeviceIoControl(hVol, FSCTL_GET_NTFS_FILE_RECORD
                                    , &mftRecordInput, sizeof(mftRecordInput)
                                    , pMftRecord, dwFileRecSize, &dwRet, NULL);
                                pfileRecordheader = (PFILE_RECORD_HEADER)pMftRecord->FileRecordBuffer;
                                for (pAttribute = (PATTRIBUTE)((PBYTE)pfileRecordheader + pfileRecordheader->AttributeOffset)
                                    ; pAttribute->AttributeType != AttributeEnd && pAttribute->AttributeType < AttributeData \
                                    && pAttriList->AttributeType != 0
                                    ; pAttribute = (PATTRIBUTE)((PBYTE)pAttribute + pAttribute->Length)
                                    );
                                if (AttributeData == pAttribute->AttributeType)
                                {
                                    if (pAttribute->Nonresident) {
                                        *pdwFileSize = PNONRESIDENT_ATTRIBUTE(pAttribute)->DataSize;
                                    }
                                    else {
                                        *pdwFileSize = PRESIDENT_ATTRIBUTE(pAttribute)->ValueLength;
                                    }
                                }
                                else
                                {
                                    LOG(INFO) << __FUNCTIONW__ << " 没有找到80属性也没有20属性 FRN:" << FRN;
                                }
                            }
                            free(pBuffferRead);
                        }
                        else {
                            for (pAttriList = PATTRIBUTE_LIST((PBYTE)pAttribute + PRESIDENT_ATTRIBUTE(pAttribute)->ValueOffset);
                                pAttriList->AttributeType < AttributeData && pAttriList->AttributeType != 0;
                                pAttriList = PATTRIBUTE_LIST(PBYTE(pAttriList) + pAttriList->Length)
                                );
                            if (pAttriList->AttributeType == AttributeData)
                            {
                                mftRecordInput.FileReferenceNumber.QuadPart = 0xffffffffffff & pAttriList->FileReferenceNumber;
                                DeviceIoControl(hVol, FSCTL_GET_NTFS_FILE_RECORD
                                    , &mftRecordInput, sizeof(mftRecordInput)
                                    , pMftRecord, dwFileRecSize, &dwRet, NULL);
                                pfileRecordheader = (PFILE_RECORD_HEADER)pMftRecord->FileRecordBuffer;
                                for (pAttribute = (PATTRIBUTE)((PBYTE)pfileRecordheader + pfileRecordheader->AttributeOffset)
                                    ; pAttribute->AttributeType != AttributeEnd && pAttribute->AttributeType < AttributeData \
                                    && pAttriList->AttributeType != 0
                                    ; pAttribute = (PATTRIBUTE)((PBYTE)pAttribute + pAttribute->Length)
                                    );
                                if (AttributeData == pAttribute->AttributeType)
                                {
                                    if (pAttribute->Nonresident) {
                                        *pdwFileSize = PNONRESIDENT_ATTRIBUTE(pAttribute)->DataSize;
                                    }
                                    else {
                                        *pdwFileSize = PRESIDENT_ATTRIBUTE(pAttribute)->ValueLength;
                                    }
                                    return;
                                }
                            }
                            LOG(INFO) << __FUNCTIONW__ << " 没有找到80属性 FRN:" << FRN;
                        }
                    }
                }
                else {
                    if (pAttribute->Nonresident)
                    {
                        *pdwFileSize = PNONRESIDENT_ATTRIBUTE(pAttribute)->DataSize;
                    }
                    else {
                        *pdwFileSize = PRESIDENT_ATTRIBUTE(pAttribute)->ValueLength;
                    }
                }
            }
        }
        free(pMftRecord);
    }

    BOOL QueryVolumeFRecordInfo(HANDLE hVolume, DWORD& dwFRLen, DWORD& dwBytesPerCluster)
    {
        if (hVolume == NULL) return FALSE;
        DWORD BytesReturned;
        NTFS_VOLUME_DATA_BUFFER nvdb;
        if (::DeviceIoControl(hVolume, FSCTL_GET_NTFS_VOLUME_DATA, NULL, 0, &nvdb, sizeof(nvdb), &BytesReturned, NULL) == FALSE)
            return FALSE;
        DWORD len = sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER) + nvdb.BytesPerFileRecordSegment - 1;
        dwFRLen = len;
        dwBytesPerCluster = nvdb.BytesPerCluster;
        return TRUE;
    }

    BOOL QueryVolumeSpaceInfo(std::string root, DWORD& dwTotalSpace, DWORD& dwFreeSpace)
    {
        ULARGE_INTEGER nAvail;
        ULARGE_INTEGER nTotal;
        ULARGE_INTEGER nTotalFree;
        ULONGLONG ullTotalSpace = 0, ullFreeSpace = 0;
        //利用GetDiskFreeSpaceEx函数获得指定磁盘信息
        if (::GetDiskFreeSpaceExA(root.c_str(), &nAvail, &nTotal, &nTotalFree))
        {
            ullTotalSpace = nTotal.QuadPart / 1024 / 1024; //得到磁盘总空间，单位为MB
            ullFreeSpace = nTotalFree.QuadPart / 1024 / 1024;//得到磁盘剩余空间，单位为MB
            dwTotalSpace = (DWORD)ullTotalSpace;
            dwFreeSpace = (DWORD)ullFreeSpace;
            return TRUE;
        }
        return FALSE;
    }

    BOOL QueryVolumeUsnJournal(HANDLE hVolume, DWORD dwVolIndex, PUSN_JOURNAL_DATA pUsnJournalData)
    {
        DWORD cb;
        DWORDLONG dwlJUsnMaxSize = MAXINUMSIZE, dwlAllocation = ALLOCATIONDELTA;
        if (::DeviceIoControl(hVolume, FSCTL_QUERY_USN_JOURNAL, NULL, 0
            , pUsnJournalData, sizeof(USN_JOURNAL_DATA), &cb, NULL) == FALSE)
        {
            DWORD errorCode = ::GetLastError();
            LOG(INFO) << __FUNCTIONW__ << " FSCTL_QUERY_USN_JOURNAL error:" << errorCode;
            switch (errorCode)
            {
            case ERROR_JOURNAL_NOT_ACTIVE:
            {
                SalculateUsnLogSize(dwVolIndex, 0, dwlJUsnMaxSize, dwlAllocation);
                NtfsUtils::CreateUsnJournal(hVolume, dwlJUsnMaxSize, dwlAllocation);

                LOG(INFO) << __FUNCTIONW__ << " ERROR_JOURNAL_NOT_ACTIVE JUsnMaxSize:" << dwlJUsnMaxSize << " Allocation:" << dwlAllocation;
                return ::DeviceIoControl(hVolume, FSCTL_QUERY_USN_JOURNAL, NULL, 0, pUsnJournalData, sizeof(USN_JOURNAL_DATA), &cb, NULL);
            }
            break;

            case ERROR_JOURNAL_DELETE_IN_PROGRESS:
            {
                if (NtfsUtils::DeleteUsnJournal(hVolume, pUsnJournalData->UsnJournalID,
                    USN_DELETE_FLAG_NOTIFY))
                {
                    SalculateUsnLogSize(dwVolIndex, 0, dwlJUsnMaxSize, dwlAllocation);
                    NtfsUtils::CreateUsnJournal(hVolume, dwlJUsnMaxSize, dwlAllocation);

                    LOG(INFO) << __FUNCTIONW__ << " ERROR_JOURNAL_DELETE_IN_PROGRESS JUsnMaxSize:" << \
                        dwlJUsnMaxSize << " Allocation:" << dwlAllocation;
                    return ::DeviceIoControl(hVolume, FSCTL_QUERY_USN_JOURNAL, NULL, 0, pUsnJournalData, sizeof(USN_JOURNAL_DATA), &cb, NULL);
                }

                return FALSE;
            }
            break;

            default:
                return FALSE;
                break;
            }
        }
        else
        {
            if (SalculateUsnLogSize(dwVolIndex, pUsnJournalData->MaximumSize, dwlJUsnMaxSize, dwlAllocation))
            {
                BOOL bRetTmp = NtfsUtils::CreateUsnJournal(hVolume, dwlJUsnMaxSize, dwlAllocation);
                BOOL bRet = ::DeviceIoControl(hVolume, FSCTL_QUERY_USN_JOURNAL, NULL, 0, \
                    pUsnJournalData, sizeof(USN_JOURNAL_DATA), &cb, NULL);

                LOG(INFO) << __FUNCTIONW__ << " bRet:" << bRet << " MaxSize:" << pUsnJournalData->MaximumSize\
                    << " JUsnMaxSize:" << dwlJUsnMaxSize << " Allocation:" << dwlAllocation;
            }
            return TRUE;
        }
        return FALSE;
    }

    DWORD QueryVolumeFileSys(char *pNameBuffer)
    {
        if (strcmp(pNameBuffer, "NTFS") == 0)
        {
            return NtfsCore::ENUM_VOLUME_FILE_SYS_NTFS;
        }
        else if (strcmp(pNameBuffer, "FAT32") == 0)
        {
            return NtfsCore::ENUM_VOLUME_FILE_SYS_FAT;
        }
        else if (strcmp(pNameBuffer, "FAT16") == 0)
        {
            return NtfsCore::ENUM_VOLUME_FILE_SYS_FAT;
        }
        else
            return NtfsCore::ENUM_VOLUME_FILE_SYS_UNKNOW;
    }

    BOOL USNRecord2FileEntry(PUSN_RECORD pRecord, int volIndex, FileEntry& fileEntry)
    {
        if (volIndex < 2 || volIndex >= 26) return FALSE;
        int wStrlen = pRecord->FileNameLength / sizeof(wchar_t);
        int FileNameLengthInBytes = WideCharToMultiByte(CP_UTF8, 0, (wchar_t *)(((PBYTE)pRecord) + pRecord->FileNameOffset), \
            wStrlen, NULL, 0, NULL, NULL);

        if (FileNameLengthInBytes <= 0) return FALSE;
        //对于超过255个字符的 utf8 名字，直接截断
        if (FileNameLengthInBytes > MAX_FILE_NAME_LENGTH)
            FileNameLengthInBytes = MAX_FILE_NAME_LENGTH;

        fileEntry.FileReferenceNumber = ((KEY)pRecord->FileReferenceNumber & KEY_MASK);
        fileEntry.ParentFileReferenceNumber = (KEY)(pRecord->ParentFileReferenceNumber & KEY_MASK);
        fileEntry.fileInfo.dir = (pRecord->FileAttributes&FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;
        //pFileEntry->fileInfo.hidden     = is_hidden(pRecord) ? 1 : 0;
        fileEntry.fileInfo.volIndex = volIndex;
        fileEntry.fileInfo.FileNameLength = FileNameLengthInBytes;
        //pFileEntry->fileInfo.StrLen = wStrlen;
        fileEntry.fileInfo.chname = FileNameLengthInBytes > wStrlen ? 1 : 0;
        fileEntry.FileName = g_pMemoryManager->GetNewFileRecord(FileNameLengthInBytes, volIndex);
        int retCount = WideCharToMultiByte(CP_UTF8, 0, (wchar_t *)(((PBYTE)pRecord) + pRecord->FileNameOffset), \
            wStrlen, fileEntry.FileName, FileNameLengthInBytes, NULL, NULL);
        if (FileNameLengthInBytes != retCount)
        {
            LOG(INFO) << __FUNCTIONW__ << " FileNameLengthInBytes:" << FileNameLengthInBytes << \
                " retCount:" << retCount << " FileNameLength:" << pRecord->FileNameLength;
        }
        return TRUE;
    }
}
