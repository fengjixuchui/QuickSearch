#include "UsnMonitorThread.h"
#include "Volume.h"
#include "ntfsUtils.h"
#define SCAN_USN_JARNAL_TIME 2000 // 2s

CUsnMonitorThread::CUsnMonitorThread()
{
}


CUsnMonitorThread::~CUsnMonitorThread()
{
}

BOOL CUsnMonitorThread::IsQuit()
{
    if (WAIT_OBJECT_0 == WaitForSingleObject(m_hQuitEvent, 0))
    {
        return TRUE;
    }

    return FALSE;
}

BOOL CUsnMonitorThread::Wait()
{
    if (WAIT_TIMEOUT == WaitForSingleObject(m_hQuitEvent, SCAN_USN_JARNAL_TIME))
    {
        return TRUE;
    }

    return FALSE;
}

BOOL CUsnMonitorThread::StartThread()
{
    for (size_t i = 0; i < VOLUME_COUNT; i++)
    {
        m_arrayNextUsn[i] = g_ArrayVolumeInfo[i].m_usnNextUSN;
    }
    this->Start();
    return TRUE;
}

void CUsnMonitorThread::StopThread()
{
    this->Stop();
}

BOOL CUsnMonitorThread::UsnRecordChanged(PUSN_RECORD pRecord, UINT, int nVolIndex, HANDLE hVolHandle, USN nNextUsn)
{
    if (!(USN_REASON_CLOSE & pRecord->Reason))
    {
        return FALSE;
    }

    int nEvent = NtfsCore::Code_Default;

    // 创建文件
    if ((USN_REASON_FILE_CREATE & pRecord->Reason) &&
        !(USN_REASON_FILE_DELETE & pRecord->Reason) &&
        !(USN_REASON_RENAME_OLD_NAME & pRecord->Reason))
    {
        nEvent = NtfsCore::Code_Create;
    }
    // 重命名后的文件新名字
    else if ((USN_REASON_RENAME_NEW_NAME & pRecord->Reason)
        && !(USN_REASON_FILE_CREATE & pRecord->Reason))
    {
        nEvent = NtfsCore::Code_Rename;
    }
    else if ((USN_REASON_FILE_DELETE & pRecord->Reason) &&
        !(USN_REASON_FILE_CREATE & pRecord->Reason))
    {
        nEvent = NtfsCore::Code_Delete;
    }
    else if ((USN_REASON_DATA_EXTEND & pRecord->Reason) ||
        (USN_REASON_DATA_TRUNCATION & pRecord->Reason))
    {
        nEvent = NtfsCore::Code_Update;
    }

    // 缓存USN变更记录信息
    if (NtfsCore::Code_Default != nEvent)
    {
        UsnUpdateRecord record;
        record.nKey = (KEY)(pRecord->FileReferenceNumber & KEY_MASK);
        record.llUsn = nNextUsn;
        record.nVolIndex = nVolIndex;
        record.nEvent = nEvent;
        record.bDir = (pRecord->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;

        if (NtfsCore::Code_Delete != nEvent && NtfsCore::Code_Update != nEvent)
        {
            FileEntry entry;
            NtfsUtils::USNRecord2FileEntry(pRecord, nVolIndex,entry);

            if (KEY_MASK == entry.FileReferenceNumber)
            {
                LOG(INFO)<<__FUNCTIONW__<<" USNRecord2FileEntry Failed";
                return FALSE;
            }
            record.fileEntry = entry;
        }

        m_nNextUsn = nNextUsn;
        m_listUsnRecord.push_back(record);

        std::wstring strFileName;
        strFileName.assign(pRecord->FileName, pRecord->FileNameLength / 2);
        LOG(INFO) << __FUNCTIONW__ << " UsnMonitir Event:" << nEvent << " Volume:" << (char)(nVolIndex + 'A') << " Name:" << strFileName << \
            " FRN:" << (pRecord->FileReferenceNumber & KEY_MASK) << " PFRN:" << pRecord->ParentFileReferenceNumber & KEY_MASK;
    }

    return TRUE;
}

BOOL CUsnMonitorThread::Init()
{
    m_hQuitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_hPauseEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    return TRUE;
}

void CUsnMonitorThread::UnInit()
{
    if (m_hQuitEvent)
    {
        ::SetEvent(m_hQuitEvent);
    }
    StopThread();

    if (m_hQuitEvent)
    {
        ::CloseHandle(m_hQuitEvent);
        m_hQuitEvent = NULL;
    }

    if (m_hPauseEvent)
    {
        ::CloseHandle(m_hPauseEvent);
        m_hPauseEvent = NULL;
    }
}

void CUsnMonitorThread::Pause()
{
    if (m_hPauseEvent)
    {
        ResetEvent(m_hPauseEvent);
    }
}

void CUsnMonitorThread::Continue()
{
    if (m_hPauseEvent)
    {
        SetEvent(m_hPauseEvent);
    }
}

void CUsnMonitorThread::ThreadFunc()
{
    do
    {
        DWORD beginTime = ::GetTickCount();
        for (int volIndex = 0; volIndex < VOLUME_COUNT; ++volIndex)
        {
            if (IsQuit()) return;
            if (g_ArrayVolumeInfo[volIndex].m_hVolHandle != NULL)
            {
                MonitorDeviceIo(volIndex);
            }
        }
        DWORD endTime = ::GetTickCount() - beginTime;

        MonitorPeriodFinished();


        if (WillPause())
        {
            //OnPause();
            DoPause();
        }
    } while (Wait());
    return;
}

BOOL CUsnMonitorThread::MonitorDeviceIo(int volIndex)
{
    BYTE outBuffer[USN_PAGE_SIZE] = { 0 };
    READ_USN_JOURNAL_DATA rujd;
    PUSN_RECORD pRecord;

    DWORD dwMonitorEvent = 0;
    BOOL bReturnOnlyOnClose = TRUE;

    rujd.StartUsn = m_arrayNextUsn[volIndex];
    rujd.ReasonMask = dwMonitorEvent;
    rujd.ReturnOnlyOnClose = bReturnOnlyOnClose;
    rujd.Timeout = 0;
    rujd.BytesToWaitFor = 0;
    rujd.UsnJournalID = g_ArrayVolumeInfo[volIndex].m_dwlJournalID;

    DWORD dwBytes;
    DWORD dwRetBytes;
    while (!IsQuit() &&
        DeviceIoControl(g_ArrayVolumeInfo[volIndex].m_hVolHandle, FSCTL_READ_USN_JOURNAL, &rujd,
            sizeof(rujd), outBuffer, USN_PAGE_SIZE, &dwBytes, NULL))
    {
        if (IsQuit()) return TRUE;
        //FSCTL_READ_USN_JOURNAL前八个字节，是本次枚举USN记录的最后一个USN号码。
        //g_ArrayVolumeInfo[volIndex].usnNextUSN
        m_arrayNextUsn[volIndex] = *(USN *)&outBuffer;
        if (dwBytes == sizeof(USN)) return TRUE;

        dwRetBytes = dwBytes - sizeof(USN);
        pRecord = (PUSN_RECORD)(((PUCHAR)outBuffer) + sizeof(USN));

        while (!IsQuit() && dwRetBytes > 0 && pRecord)
        {
            UsnRecordChanged(pRecord, dwRetBytes, volIndex, g_ArrayVolumeInfo[volIndex].m_hVolHandle,
                    m_arrayNextUsn[volIndex]);
            dwRetBytes -= pRecord->RecordLength;
            pRecord = (PUSN_RECORD)(((PCHAR)pRecord) + pRecord->RecordLength);
        }

        rujd.StartUsn = m_arrayNextUsn[volIndex];//g_ArrayVolumeInfo[volIndex].usnNextUSN;
    }

    if (IsQuit())
    {
        return TRUE;
    }

    switch (GetLastError())
    {
    case ERROR_JOURNAL_DELETE_IN_PROGRESS:
        break;

    case ERROR_JOURNAL_NOT_ACTIVE:
        break;

    case ERROR_JOURNAL_ENTRY_DELETED:
        LOG(INFO)<<__FUNCTIONW__<<" Lost Data when monitoring. Need Recreate database";
        break;

    default:
        LOG(INFO)<<__FUNCTIONW__<<" Unknown error when monitoring vol:"<<(char)( volIndex + 'A');
        return FALSE;
        break;
    }

    return TRUE;
}

void CUsnMonitorThread::MonitorPeriodFinished()
{
    m_UpdateUsnRecord.Update(&m_listUsnRecord);
}

BOOL CUsnMonitorThread::WillPause()
{
    if (NULL == m_hPauseEvent)
    {
        return FALSE;
    }

    return WAIT_TIMEOUT == ::WaitForSingleObject(m_hPauseEvent, 0);
}

void CUsnMonitorThread::DoPause()
{
    HANDLE pHandle[] = { m_hPauseEvent , m_hQuitEvent };
    ::WaitForMultipleObjects(2, pHandle, FALSE, INFINITE);
}

void CUsnMonitorThread::OnPause()
{

}
