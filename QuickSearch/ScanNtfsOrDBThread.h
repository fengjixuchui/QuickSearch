#pragma once
#include "Thread.h"
#include <windows.h>
class CNtfsMgr;
class CScanNtfsOrDBThread : public CThread
{
public:
    CScanNtfsOrDBThread();
    ~CScanNtfsOrDBThread();
    void Init(DWORD dwIndex, CNtfsMgr* pNtfsMgr);
    void UnInit();
    void ThreadFunc();
    void StartScan();
    DWORD GetVolIndex();
    void Wait();
    void Continue();
private:
    DWORD m_dwVolumeIndex;
    CNtfsMgr* m_pNtfsMgr;
    DWORD m_dwScanTime;
    HANDLE m_hFinishEvent;
    HANDLE m_hPauseEvent;
};

