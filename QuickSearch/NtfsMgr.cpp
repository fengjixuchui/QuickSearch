#include "NtfsMgr.h"
#include <WinBase.h>
#include "ntfsUtils.h"
#include "ScanNtfsOrDBThread.h"

#include "Volume.h"
#include <Shlwapi.h>
#include <ShlObj.h>
#include "FileHandler.h"
#include "FileNameMemoryManager.h"
#pragma comment(lib, "shlwapi.lib")


CNtfsMgr* CNtfsMgr::CNtfsMgrInstance = nullptr;

CNtfsMgr * CNtfsMgr::Instance()
{
	if (CNtfsMgrInstance == nullptr)
	{
		CNtfsMgrInstance = new CNtfsMgr();
	}
	return CNtfsMgrInstance;
}

void CNtfsMgr::DestroyInstance()
{
	delete CNtfsMgrInstance;
}

CNtfsMgr::CNtfsMgr()
{
}


CNtfsMgr::~CNtfsMgr()
{
}

void CNtfsMgr::UnInit()
{
    
    for (DWORD i = 0; i < m_vecScanThread.size(); ++i)
    {
        if (m_vecScanThread[i] != NULL)
            m_vecScanThread[i]->UnInit();
        m_vecScanThread[i] = NULL;
    }
    DestroyInstance();
    //g_pMemoryManager->FreeAllFileMemory();

}

DWORD CNtfsMgr::initVolumes()
{
	//获取电脑上的卷的数量及盘号
	char driverStrings[MAX_PATH] = { '\0' };
	if (!::GetLogicalDriveStringsA(sizeof(driverStrings), driverStrings))
	{
		return 1;
	}

	for (char *pVolumeChar = driverStrings; *pVolumeChar != '\0';
		pVolumeChar += 4)
	{
		if (*pVolumeChar >= 'a') *pVolumeChar -= 32;
		int volIndex = -1;
		volIndex = *pVolumeChar - 'A';
		if (volIndex >= 26) break;

		
		UINT uiDriveType = ::GetDriveTypeA(pVolumeChar);
        g_ArrayVolumeInfo[volIndex].m_dwVolumeDriveType = uiDriveType;
        g_ArrayVolumeInfo[volIndex].m_bValidVolume = TRUE;
		char szVolName[32];
		DWORD dwVolSerialNumber;
		char szFileSysNameBuffer[8];
		DWORD dwMaxComLen, dwFileSysNameFlags;
		::GetVolumeInformationA(pVolumeChar, szVolName, 32, &dwVolSerialNumber,
			&dwMaxComLen, &dwFileSysNameFlags, szFileSysNameBuffer, 8);

        g_ArrayVolumeInfo[volIndex].m_dwVolSerialNum = dwVolSerialNumber;
		NtfsCore::_tagVOLUME_FILE_SYS_TYPE fileSysType = NtfsCore::ENUM_VOLUME_FILE_SYS_UNKNOW; 
		fileSysType = (NtfsCore::_tagVOLUME_FILE_SYS_TYPE)NtfsUtils::QueryVolumeFileSys(szFileSysNameBuffer);

		if (fileSysType != NtfsCore::ENUM_VOLUME_FILE_SYS_NTFS)
			continue;

		char szVolPath[10] = { 0 };
		sprintf_s(szVolPath, 10, "\\\\.\\%c:", *pVolumeChar);

		HANDLE hVolume = ::CreateFileA(szVolPath,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, 0, NULL);

		if (INVALID_HANDLE_VALUE == hVolume)
		{
			hVolume = NULL;
			continue;
		}
		
		g_ArrayVolumeInfo[volIndex].m_hVolHandle = hVolume;

		//获取磁盘的更多信息
		NtfsUtils::QueryVolumeFRecordInfo(hVolume, g_ArrayVolumeInfo[volIndex].m_dwFileRecordSize, g_ArrayVolumeInfo[volIndex].m_dwBytesPerCluster);
		NtfsUtils::QueryVolumeSpaceInfo(pVolumeChar, g_ArrayVolumeInfo[volIndex].m_dwVolumeTotalSpace, g_ArrayVolumeInfo[volIndex].m_dwVolumeFreeSpace);

		//初始化USN
        USN_JOURNAL_DATA ujd;               // 第一次查询USN状态
        BOOL bQueryUsnJournal = NtfsUtils::QueryVolumeUsnJournal(hVolume, volIndex, &ujd);
        if (!bQueryUsnJournal)
        {
            DWORD errorCode = ::GetLastError();
            LOG(INFO) << __FUNCTIONW__ << " QueryVolumeUsnJournal vol:" << pVolumeChar<< " error:" << errorCode;
            continue;
        }
        m_vecValidVolumes.push_back(volIndex);
        g_ArrayVolumeInfo[volIndex].m_usnFirstUSN = ujd.FirstUsn;
        g_ArrayVolumeInfo[volIndex].m_dwlJournalID = ujd.UsnJournalID;

        //TODO 根据DB中的保存的USN ID来赋值
        LONGLONG llNextUsn = 0;
        BOOL bDbStatusRet = CheckValidVolumeDB(volIndex, llNextUsn);
        if (bDbStatusRet == FALSE) continue;

        g_ArrayVolumeInfo[volIndex].m_usnNextUSN = llNextUsn;
        g_ArrayVolumeInfo[volIndex].m_bValidDB = TRUE;
	}
	return 0;
}

BOOL CNtfsMgr::ScanVolumeFileData()
{
    DWORD beginTime = ::GetTickCount();

    CreateScanThread();
    if (m_vecScanThread.size() <= 0) return FALSE;

    //串行扫描无DB的Ntfs数据。
    for (DWORD i = 0; i<m_vecScanThread.size(); ++i)
    {
        DWORD dwVolIndex = m_vecScanThread[i]->GetVolIndex();
        BOOL bValidDB = g_ArrayVolumeInfo[dwVolIndex].IsValidDB();
        if (bValidDB == FALSE)
        {
            m_vecScanThread[i]->StartScan();
            //m_vecScanThread[i]->Wait();
        }
    }

    //并行扫描有DB的Ntfs数据。
    /*for (DWORD i = 0; i < m_vecScanThread.size(); ++i)
    {
        DWORD dwVolIndex = m_vecScanThread[i]->GetVolIndex();
        BOOL bValidDB = g_ArrayVolumeInfo[dwVolIndex].IsValidDB();
        if (bValidDB)
        {
            m_vecScanThread[i]->StartScan();
            m_vecScanThread[i]->Wait();
        }
    }*/

    for (DWORD i = 0; i < m_vecScanThread.size(); ++i)
    {
        if (m_vecScanThread[i] != NULL)
        {
            m_vecScanThread[i]->Wait();
        }
    }

    //释放线程
    for (DWORD i = 0; i<m_vecScanThread.size(); ++i)
    {
        if (m_vecScanThread[i] != NULL)
            m_vecScanThread[i]->UnInit();
        m_vecScanThread[i] = NULL;
    }

    DWORD scanTime = ::GetTickCount() - beginTime;

    LOG(INFO) << __FUNCTIONW__ << " ScanVolumeFileData Time:" << scanTime;
    
    ReleaseScanThread();
    for (auto tmp : m_vecValidVolumes)
    {
        LOG(INFO) << __FUNCTIONW__ << " vol:" << (char)(tmp + 'A') << " fileCnt:" << GetVolFileCnt(tmp);
    }
    LOG(INFO) << __FUNCTIONW__ << " all file count:" << GetAllFileCnt();
    return TRUE;
}

BOOL CNtfsMgr::DoScanVolume(DWORD dwIndex)
{
    if (g_ArrayVolumeInfo[dwIndex].IsValidVolume() == FALSE) return FALSE;
    DWORD beginTime = ::GetTickCount();
    BOOL bValidDB = g_ArrayVolumeInfo[dwIndex].IsValidDB();
    LOG(INFO) << __FUNCTIONW__ << " vol:" << (char)(dwIndex + 'A') << " DoScanVolume bValidDB:" << bValidDB;
    BOOL bRet = FALSE;
    if (bValidDB)
    {
        bRet = LoadDatabase(dwIndex);
    }
    else
    {
        bRet = CreateDatabase(dwIndex);
    }
    g_ArrayVolumeInfo[dwIndex].m_bCanWriteDB = bRet;
    DWORD endTime = ::GetTickCount() - beginTime;
    LOG(INFO) << __FUNCTIONW__ << " vol:" << (char)(dwIndex + 'A') << " Time:" << endTime;
    return bRet;
}

void CNtfsMgr::CreateScanThread()
{
    m_vecScanThread.clear();
    for (auto index : m_vecValidVolumes)
    {
        CScanNtfsOrDBThread *pPtr = new CScanNtfsOrDBThread();
        if (pPtr != NULL)
        {
            pPtr->Init(index, this);
            m_vecScanThread.push_back(pPtr);
        }
    }
}

void CNtfsMgr::ReleaseScanThread()
{
    for (auto & tempThread : m_vecScanThread)
    {
        if (tempThread != NULL)
        {
            delete tempThread;
            tempThread = NULL;
        }
    }
    m_vecScanThread.clear();
}

BOOL CNtfsMgr::IsQuit()
{
    return g_bQuitSearCore;
}

DWORD CNtfsMgr::GetVolFileCnt(int nvolIndex)
{
    return g_pIndexManager->GetVolFileCnt(nvolIndex);
}

DWORD CNtfsMgr::GetAllFileCnt()
{
    DWORD dwFileCnt = 0;
    for (auto& tmp : g_pIndexManager->m_VolFileIndex)
    {
        dwFileCnt += tmp.size();
    }
    return dwFileCnt;
}

BOOL CNtfsMgr::LoadDatabase(int volIndex)
{

    /*LOG(INFO) << __FUNCTIONW__ << " vol:" << (char)(volIndex + 'A') << " Loading Database...";
    DWORD dwStart = ::GetTickCount();

    std::wstring strDbFilePath;
    GetVolumeDBStorePath(strDbFilePath, TC_DB_FILE_NAME, volIndex, FALSE);
    if (strDbFilePath.length() == 0) return FALSE;

    g_pIndexManager->Load(strDbFilePath, volIndex);
    LoadFileNameDatabase(volIndex);
    for (auto & tmp : g_pIndexManager->m_VolFileIndex[volIndex])
    {
        tmp.pFileName = g_pMemoryManager->m_fileNameMap[volIndex][tmp.FileReferenceNumber];
    }*/
    return TRUE;
}

BOOL CNtfsMgr::CreateDatabase(int volIndex)
{
    USN_JOURNAL_DATA pUjd;
    if (NtfsUtils::QueryVolumeUsnJournal(g_ArrayVolumeInfo[volIndex].m_hVolHandle, volIndex, &pUjd) == FALSE)
    {
        LOG(INFO) << __FUNCTIONW__ << " vol:" << (char)(volIndex + 'A') << " Creating QueryVolumeUsnJournal error";
        return FALSE;
    }
    DWORD dwStart = ::GetTickCount();

    g_ArrayVolumeInfo[volIndex].m_dwlJournalID = pUjd.UsnJournalID;
    g_ArrayVolumeInfo[volIndex].m_usnNextUSN = pUjd.NextUsn;

    LOG(INFO) << __FUNCTIONW__ << " vol:" << (char)(volIndex + 'A') << " JournalID:" << pUjd.UsnJournalID << " FirstUsn:" << pUjd.FirstUsn << \
        " NextUsn:" << pUjd.NextUsn << " LowestValidUsn" << pUjd.LowestValidUsn;
    // Enumerate MFT for files with 'Last USN'
    // between LowUsn and HighUsn.
    DWORD cb = 0;
    BYTE outBuffer[sizeof(USN) + 0x80000];
    MFT_ENUM_DATA_V0 med;
    med.StartFileReferenceNumber = 0;
    med.LowUsn = 0;
    med.HighUsn = pUjd.NextUsn;

    DWORD dwReadMftUsnUseTime = 0, beginTime = 0, dwIndexTime = 0, dwLoopCount = 0;
    BOOL bRet = TRUE;
    int recordLength = 0;
    while (DeviceIoControl(g_ArrayVolumeInfo[volIndex].m_hVolHandle, FSCTL_ENUM_USN_DATA, &med,
        sizeof(med), &outBuffer, sizeof(outBuffer), &cb, NULL))
    {
        if (beginTime != 0)
        {
            dwReadMftUsnUseTime += ::GetTickCount() - beginTime;
        }
        if (IsQuit()) { bRet = FALSE; break; }

        //第一个8字节存储的是下一个StartFileReferenceNumber
        PUSN_RECORD pRecord = (PUSN_RECORD)(((PBYTE)outBuffer) + sizeof(USN));

        while ((PBYTE)pRecord < (outBuffer + cb))
        {
            if (IsQuit()) { bRet = FALSE; break; }
            DWORD dwIndexBegin = ::GetTickCount();
            FileEntry fileEntry;
            NtfsUtils::USNRecord2FileEntry(pRecord, volIndex, fileEntry);
            
            if (fileEntry.FileReferenceNumber != KEY_MASK &&
                fileEntry.fileInfo.volIndex >= 2 &&
                fileEntry.fileInfo.volIndex < 26 &&
                fileEntry.pFileName->FileNameLength > 0)
            {
                g_pIndexManager->AddEntry(fileEntry, volIndex);
                dwIndexTime += ::GetTickCount() - dwIndexBegin;
            }
            pRecord = (PUSN_RECORD)(((PBYTE)pRecord) + pRecord->RecordLength);
            
        }
        med.StartFileReferenceNumber = *((USN*)&outBuffer);
        beginTime = ::GetTickCount();
    }
    DWORD dwEndUse = ::GetTickCount() - dwStart;

    LOG(INFO) << __FUNCTIONW__ << " vol:" << (char)(volIndex + 'A') << " totalTime:" << dwEndUse << " ioTime:" << \
        dwReadMftUsnUseTime << " IndexTime:" << dwIndexTime;
    return bRet;
}

BOOL CNtfsMgr::LoadFileNameDatabase(int volIndex)
{
    DWORD dwStart = ::GetTickCount();

    std::wstring strDbFilePath;
    GetVolumeDBStorePath(strDbFilePath, TC_DB_FILE_NAME, volIndex,TRUE);
    if (strDbFilePath.length() == 0) return FALSE;

    HANDLE hfile = ::CreateFile(strDbFilePath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (INVALID_HANDLE_VALUE == hfile)
    {
        LOG(INFO)<<__FUNCTIONW__<<" can not open "<<strDbFilePath;
        return FALSE;
    }
    PBYTE pOutBuffer = (PBYTE)malloc(ALLOC_SIZE);
    if (pOutBuffer == NULL)
    {
        LOG(INFO) << __FUNCTIONW__ << " Allocation when loadDatabase Error";
        ::CloseHandle(hfile);
        return FALSE;
    }

    DWORD cb;
    if (::ReadFile(hfile, pOutBuffer, DW_SEARCHDB_HEADER_LENGTH * sizeof(TCHAR), &cb, NULL) == FALSE)
    {
        LOG(INFO) << __FUNCTIONW__ << " ReadFile  Error";
        ::CloseHandle(hfile);
        hfile = NULL;
        ::DeleteFile(strDbFilePath.c_str());
        return FALSE;
    }
    DWORD dwDbMemoryBlocknCount = 0;
    BOOL bRet = TRUE;

    DWORD dwReadDBUseTime = 0, beginTime = 0,dwMemoryTime = 0, dwLoopCount = 0;
    while (::ReadFile(hfile, pOutBuffer, ALLOC_SIZE, &cb, NULL) && !IsQuit())
    {
        if (beginTime != 0)
        {
            dwReadDBUseTime += ::GetTickCount() - beginTime;
        }

        if (IsQuit()) { bRet = FALSE; break; }
        if (cb != ALLOC_SIZE)
        {
            break;
        }
        dwDbMemoryBlocknCount++;
        PFileNameEntry pHead = (PFileNameEntry)(pOutBuffer + sizeof(int) + sizeof(int) + sizeof(void*));
        int *pfileRecordCount = (int*)pOutBuffer;

        DWORD addHashBegin = ::GetTickCount();
        for (int i = 0; i < *pfileRecordCount; ++i)
        {
            if (IsQuit()) { bRet = FALSE; break; }

            if (pHead == NULL)
            {
                break;
            }
            if (pHead->FRN == KEY_MASK) //无效数据就不需要重新入库了
            {
                pHead = (PFileNameEntry)((PBYTE)pHead + g_pMemoryManager->GetMinMBlockSize(\
                    g_pMemoryManager->GetMinMBlockType(pHead->FileNameLength + FileNameEntryHeaderSize)));
                continue;
            }

            DWORD memoryBegin = ::GetTickCount();
            PFileNameEntry pFileEntryTmp = (PFileNameEntry)pHead;
            PFileNameEntry pNewFile = g_pMemoryManager->GetNewFileRecord(pFileEntryTmp->FileNameLength + FileNameEntryHeaderSize, volIndex);

            dwMemoryTime += ::GetTickCount() - memoryBegin;
            if (pFileEntryTmp->FileNameLength > 0)
            {
                memcpy(pNewFile, pFileEntryTmp, pHead->FileNameLength + FileNameEntryHeaderSize);
                g_pMemoryManager->m_fileNameMap[volIndex][pNewFile->FRN] = pNewFile;
            }
            pHead = (PFileNameEntry)((PBYTE)pHead + g_pMemoryManager->GetMinMBlockSize(\
                g_pMemoryManager->GetMinMBlockType(pHead->FileNameLength + FileNameEntryHeaderSize)));
        }
        memset(pOutBuffer, 0, ALLOC_SIZE);
        beginTime = ::GetTickCount();
    }
    ::CloseHandle(hfile);
    free(pOutBuffer);
    pOutBuffer = NULL;
    DWORD endTime = ::GetTickCount() - dwStart;

    return bRet;
}

BOOL CNtfsMgr::SaveFileNameDataBase(int volIndex)
{
    std::wstring tempPath;
    GetVolumeDBStorePath(tempPath, TC_DB_FILE_NAME, volIndex,TRUE);
    if (tempPath.size() == 0) return FALSE;
    std::wstring dbPath = tempPath;
    tempPath += L"_tmp";

    TCHAR szHeaderBuffer[DW_SEARCHDB_HEADER_LENGTH] = { 0 };
    BOOL bRet = SaveNtfsInfoDB(szHeaderBuffer, DW_SEARCHDB_HEADER_LENGTH, volIndex);
    if (!bRet) return FALSE;

    HANDLE hfile = NULL;
    DWORD cb = 0;
    hfile = ::CreateFile(tempPath.c_str(), GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hfile)
    {
        LOG(INFO)<<__FUNCTIONW__<<" Create filename DB falid error:"<<GetLastError();
        return FALSE;
    }

    if (!::WriteFile(hfile, (LPCVOID)szHeaderBuffer, DW_SEARCHDB_HEADER_LENGTH * sizeof(TCHAR), &cb, NULL))
    {
        DWORD dwError = ::GetLastError();
        LOG(INFO)<<__FUNCTIONW__<<" Write version number to local error:"<<dwError;

        ::CloseHandle(hfile);
        FileHandler::DeleteFile(tempPath.c_str());
        return FALSE;
    }

    MemoryBlock *pMemoryBlockHead = g_pMemoryManager->m_pMemoryBlockHead[volIndex];
    while (pMemoryBlockHead != NULL)
    {
        unsigned int dwTemp = 0;
        if (!::WriteFile(hfile, pMemoryBlockHead, ALLOC_SIZE, &cb, NULL))
        {
            LOG(INFO)<<__FUNCTIONW__<<" Write file to local error";
            ::CloseHandle(hfile);
            FileHandler::DeleteFile(tempPath.c_str());
            return FALSE;
        }
        MemoryBlock *pTemp = pMemoryBlockHead;
        pMemoryBlockHead = pMemoryBlockHead->m_pNext;
    }
    ::CloseHandle(hfile);


    FileHandler::DeleteFile(dbPath.c_str());
    _wrename(tempPath.c_str(), dbPath.c_str());
    return TRUE;
    return 0;
}

BOOL CNtfsMgr::SaveDatabase()
{
    DWORD dwStart = ::GetTickCount();
    for (int i = 0; i < VOLUME_COUNT; ++i)
    {
        //if (g_hVolHandle[i] != NULL && m_pMemoryBlockHead[i] != NULL)
        if (g_ArrayVolumeInfo[i].m_hVolHandle != INVALID_HANDLE_VALUE)
        {
            if (g_ArrayVolumeInfo[i].m_bCanWriteDB == TRUE)
            {
                //WriteIndexDataToLocal(i);
                //WriteFileNameData(i);
                //std::wstring strDbFilePath;
                //GetVolumeDBStorePath(strDbFilePath, TC_DB_FILE_NAME, i, FALSE);
                //if (strDbFilePath.length() == 0) return FALSE;
                //g_pIndexManager->Save(strDbFilePath,i);
                SaveFileNameDataBase(i);
                LOG(INFO) << __FUNCTIONW__ << " Write DB vol:" <<(char)(i+'A') ;
            }
        }
    }
    DWORD endTime = ::GetTickCount() - dwStart;
    LOG(INFO) << __FUNCTIONW__ << " Write DB useTime:" << endTime;
    return TRUE;
}

BOOL CNtfsMgr::GetVolumeDBStorePath(std::wstring & strDbFilePath, const TCHAR * pFileName, int volIndex,BOOL isFileNameData)
{
    TCHAR szStorePath[MAX_PATH + 1] = { 0 };
    FileHandler::GetAppStorePath(szStorePath);
    ::PathAddBackslash(szStorePath);

    TCHAR szCurVolSerialNum[20] = { 0 };    //每个卷的SerialNum
    DWORDLONG dlVolSerialNum = (DWORDLONG)g_ArrayVolumeInfo[volIndex].m_dwVolSerialNum;
    if (_i64tow_s(dlVolSerialNum, szCurVolSerialNum, 20, 10))
    {
        LOG(INFO) << __FUNCTIONW__ << " Can not convert from VolSerialNum to string";
        return FALSE;
    }
    strDbFilePath = szStorePath;
    strDbFilePath += pFileName;
    strDbFilePath += L"_";
    strDbFilePath += szCurVolSerialNum;
    if (isFileNameData == TRUE)
    {
        strDbFilePath += L"_FileNameInfo";
    }
    return TRUE;
}

BOOL CNtfsMgr::CheckValidVolumeDB(int volIndex, LONGLONG & llNextUsn)
{
    std::wstring strDbFilePath;
    GetVolumeDBStorePath(strDbFilePath, TC_DB_FILE_NAME, volIndex,TRUE);
    WIN32_FILE_ATTRIBUTE_DATA attrs = { 0 };
    if (::GetFileAttributesEx(strDbFilePath.c_str(), ::GetFileExInfoStandard, &attrs))
    {
        HANDLE hfile;
        hfile = ::CreateFile(strDbFilePath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (INVALID_HANDLE_VALUE == hfile) return FALSE;
        DWORD cb;
        // 读取 Journal ID and NextUSN
        TCHAR szCurVersion[20] = { 0 };            // DB版本号
        TCHAR szCurVolSerialNum[20] = { 0 };     //每个卷的SerialNum
        TCHAR szCurJournalID[20] = { 0 };     // 每个卷的JournalID
        TCHAR szCurNextUSN[20] = { 0 };     // 每个卷的NextUSN
        BOOL bRetVesion = ::ReadFile(hfile, (PVOID)szCurVersion, 20 * sizeof(TCHAR), &cb, NULL);
        BOOL bRetSNum = ::ReadFile(hfile, (PVOID)szCurVolSerialNum, 20 * sizeof(TCHAR), &cb, NULL);
        BOOL bRetJID = ::ReadFile(hfile, (PVOID)szCurJournalID, 20 * sizeof(TCHAR), &cb, NULL);
        BOOL bRetNUSN = ::ReadFile(hfile, (PVOID)szCurNextUSN, 20 * sizeof(TCHAR), &cb, NULL);
        ::CloseHandle(hfile);
        hfile = NULL;
        g_ArrayVolumeInfo[volIndex].m_bHaveDB = TRUE;

        if (!bRetVesion || !bRetSNum || !bRetJID || !bRetNUSN) return FALSE;
        DWORD dwDbVersion = (DWORD)_wtoi64(szCurVersion);

        if (dwDbVersion != DW_SEARCHDB_VERSION)
        {
            LOG(INFO) << __FUNCTIONW__ << " vol:" << (char)(volIndex + 'A') << " Ver not match, recreate database";
            return FALSE;
        }

        DWORD dwSerialNum = (DWORD)_wtoi64(szCurVolSerialNum);
        ULONGLONG ullJournalID = _wtoi64(szCurJournalID);
        llNextUsn = _wtoi64(szCurNextUSN);

        LOG(INFO) << __FUNCTIONW__ << " vol:" << (char)(volIndex + 'A') << " Ver:" << dwDbVersion << \
            " VolSNum:" << dwSerialNum << " JID:" << ullJournalID << " NUsn" << llNextUsn;
        
        LOG(INFO) << __FUNCTIONW__ << " vol:" << (char)(volIndex + 'A') << " Vol Info JournalID:" << \
            g_ArrayVolumeInfo[volIndex].m_dwlJournalID << " FirstUsn:" << \
            g_ArrayVolumeInfo[volIndex].m_usnFirstUSN;

        if (dwSerialNum != g_ArrayVolumeInfo[volIndex].m_dwVolSerialNum)
        {
            LOG(INFO) << __FUNCTIONW__ << " vol:" << (char)(volIndex + 'A') << " VSID not match, need recreate database";
            return FALSE;
        }

        if (ullJournalID != g_ArrayVolumeInfo[volIndex].m_dwlJournalID)
        {
            LOG(INFO) << __FUNCTIONW__ << " vol:" << (char)(volIndex + 'A') << " JID not match, need recreate database";
            return FALSE;
        }

        if (g_ArrayVolumeInfo[volIndex].m_usnFirstUSN > llNextUsn)
        {
            LOG(INFO) << __FUNCTIONW__ << " vol:" << (char)(volIndex + 'A') << " Lose Data, need recreate database";
            return FALSE;
        }
        //检查IndexDB文件是否存在
        GetVolumeDBStorePath(strDbFilePath, TC_DB_FILE_NAME, volIndex, FALSE);
        WIN32_FILE_ATTRIBUTE_DATA attrs = { 0 };
        if (::GetFileAttributesEx(strDbFilePath.c_str(), ::GetFileExInfoStandard, &attrs))
        {
            return TRUE;
        }
        return FALSE;
    }

    LOG(INFO) << __FUNCTIONW__ << " vol:" << (char)(volIndex + 'A') << " Database not exist";
    return FALSE;
}

BOOL CNtfsMgr::SaveNtfsInfoDB(TCHAR* szHeaderBuffer, DWORD dwBufSize, int volIndex)
{ 
    if (szHeaderBuffer == NULL) return FALSE;
    if (volIndex < 0 || volIndex >= 26) return FALSE;

    //4KByte的头部，采用Unicode编码, 2048个字符。
    TCHAR szCurVersion[20] = { 0 };            // DB版本号
    TCHAR szCurVolSerialNum[20] = { 0 };     //每个卷的SerialNum
    TCHAR szCurJournalID[20] = { 0 };     // 每个卷的JournalID
    TCHAR szCurNextUSN[20] = { 0 };     // 每个卷的NextUSN, 用于下次启动更新

    DWORDLONG dlVersion = (DWORDLONG)DW_SEARCHDB_VERSION;
    if (_i64tow_s(dlVersion, szCurVersion, 20, 10))
    {
        LOG(INFO) << __FUNCTIONW__ << " Can not convert from DW_SEARCHDB_VERSION to string";
        return FALSE;
    }
    DWORDLONG dlVolSerialNum = (DWORDLONG)g_ArrayVolumeInfo[volIndex].m_dwVolSerialNum;
    if (_i64tow_s(dlVolSerialNum, szCurVolSerialNum, 20, 10))
    {
        LOG(INFO) << __FUNCTIONW__ << " Can not convert from VolSerialNum to string";
        return FALSE;
    }
    if (_i64tow_s(g_ArrayVolumeInfo[volIndex].m_dwlJournalID, szCurJournalID, 20, 10))
    {
        LOG(INFO) << __FUNCTIONW__ << " Can not convert from JournalID to string";
        return FALSE;
    }

    //g_curHandledUSN，无效数据，为了DB兼容写法，继续保持写入。
    if (_i64tow_s(g_ArrayVolumeInfo[volIndex].m_usnNextUSN, szCurNextUSN, 20, 10))
    {
        LOG(INFO) << __FUNCTIONW__ << " Can not convert from JournalID to string";
        return FALSE;
    }
    DWORD dwBuf = dwBufSize;
    TCHAR* pNext = szHeaderBuffer;
    wcscpy_s(pNext, dwBuf, szCurVersion);
    pNext = pNext + 20; dwBuf -= 20;
    wcscpy_s(pNext, dwBuf, szCurVolSerialNum);
    pNext = pNext + 20; dwBuf -= 20;
    wcscpy_s(pNext, dwBuf, szCurJournalID);
    pNext = pNext + 20; dwBuf -= 20;
    wcscpy_s(pNext, dwBuf, szCurNextUSN);


    LOG(INFO) << __FUNCTIONW__ << " vol:" << (char)(volIndex + 'A') << " JournalID:" << \
        g_ArrayVolumeInfo[volIndex].m_dwlJournalID << \
        " NextUSN:" << g_ArrayVolumeInfo[volIndex].m_usnNextUSN;
    return TRUE;
}
