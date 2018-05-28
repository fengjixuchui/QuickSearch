#pragma once
#include "global.h"
#include "Thread.h"
#include <vector>
#include <unordered_set>
class CSearchThread : public CThread
{

public:
    CSearchThread();
    ~CSearchThread();
    BOOL Init(DWORD dwIndex);
    void Uninit();
    void DoSearch(SearchOpt opt);
    void Wait();
private:
    void ThreadFunc() override;
    void Search();
    void Match(FileEntry fileEntry,std::wstring& wstrSearch);
public:
    std::vector<FileEntry> m_vecResult;
private:
    DWORD dwVolIndex;
    SearchOpt m_SearchOpt;
    HANDLE m_hQuitEvent;
    HANDLE m_hPauseEvent;
};

