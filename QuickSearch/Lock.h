#pragma once
#include <WinBase.h>
class CLock
{
    CRITICAL_SECTION m_cs;
public:
    CLock()
    {
        InitializeCriticalSection(&m_cs);
    }
    ~CLock()
    {
        DeleteCriticalSection(&m_cs);
    }
    void Lock()
    {
        EnterCriticalSection(&m_cs);
    }
    void UnLock()
    {
        LeaveCriticalSection(&m_cs);
    }
};