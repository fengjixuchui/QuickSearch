#pragma once
#include "Thread.h"
#include "global.h"
#include "UpdateUSNRecord.h"
class CUsnMonitorThread:public CThread
{
public:
    CUsnMonitorThread();
    ~CUsnMonitorThread();
private:
    BOOL	IsQuit();
    BOOL	Wait();
    BOOL	StartThread();
    void	StopThread();
    void    ThreadFunc() override;
    BOOL	MonitorDeviceIo(int volIndex);
    void	MonitorPeriodFinished();
    BOOL	WillPause();
    void	DoPause();
    void	OnPause();
    BOOL    UsnRecordChanged(PUSN_RECORD, UINT, int, HANDLE, USN);
public:
    BOOL    Init();
    void    UnInit();
    void    Pause();
    void    Continue();
    
private:
    HANDLE	        m_hQuitEvent;
    HANDLE	        m_hPauseEvent;
    USN		        m_arrayNextUsn[VOLUME_COUNT];
    USN				m_nNextUsn;
    UsnRecordList	m_listUsnRecord;
    UpdateUSNRecord	m_UpdateUsnRecord;
};

