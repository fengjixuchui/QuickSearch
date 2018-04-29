#include "ScanNtfsOrDBThread.h"
#include "NtfsMgr.h"


CScanNtfsOrDBThread::CScanNtfsOrDBThread()
    : m_dwVolumeIndex(0)
    , m_pNtfsMgr(NULL)
    , m_dwScanTime(0)
{
    m_hFinishEvent = CreateEvent(
        NULL,               // default security attributes
        TRUE,               // manual-reset event
        FALSE,              // initial state is nonsignaled
        TEXT("finishEvent")  // object name
    );
}


CScanNtfsOrDBThread::~CScanNtfsOrDBThread()
{
    if (m_hFinishEvent != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hFinishEvent);
        m_hFinishEvent = NULL;
    }
}

void CScanNtfsOrDBThread::Init(DWORD dwIndex, CNtfsMgr * pNtfsMgr)
{
    m_dwVolumeIndex = dwIndex;
    m_pNtfsMgr = pNtfsMgr;

}

void CScanNtfsOrDBThread::UnInit()
{
    this->Stop();
}

void CScanNtfsOrDBThread::ThreadFunc()
{
    if (!this->IsExist())
    {
        return;
    }
    DWORD dwBegin = ::GetTickCount();
    if (m_pNtfsMgr != NULL)
    {
        m_pNtfsMgr->DoScanVolume(m_dwVolumeIndex);
    }

    DWORD dwUseTime = ::GetTickCount() - dwBegin;
    m_dwScanTime = dwUseTime;
    LOG(INFO) << __FUNCTIONW__ << " vol:" << (char)(m_dwVolumeIndex + 'A') << " dwUseTime:" << dwUseTime;
    Continue();
}

void CScanNtfsOrDBThread::StartScan()
{
    ::ResetEvent(m_hFinishEvent);
    this->Start();
}

DWORD CScanNtfsOrDBThread::GetVolIndex()
{
    return m_dwVolIndex;
}

void CScanNtfsOrDBThread::Wait()
{
    if (m_hFinishEvent)
    {
        ::WaitForSingleObject(m_hFinishEvent, INFINITE);
    }
}

void CScanNtfsOrDBThread::Continue()
{
    if (m_hFinishEvent)
    {
        ::SetEvent(m_hFinishEvent);
    }
}
