#pragma once
#include <vector>
#include "SearchThread.h"
#include <unordered_map>
class CSearchMgr : public CThread
{
public:
    CSearchMgr();
    ~CSearchMgr();
    void Init(HWND mainwindow);
    void UnInit();
    void Search(SearchOpt opt);
    int GetResultCnt(int dwIndex);
    void ReSortResult();
    short GetFileType(std::wstring& filename);
    void InitClassFilter(const std::wstring* arrayPtr, int size, int filterType);
    void InitResource();   
private:
    void ThreadFunc() override;
public:
    std::vector<CSearchThread*> m_vecSearchThread;
    std::vector<SearchResultItem> m_vecResult;
    DWORD dwResultCnt;
    DWORD dwSearchTime;
private:
    SearchOpt searchOpt;
    std::unordered_map<std::wstring, int> m_MapExtClassFilter;
    HANDLE m_hPauseEvent;
    HWND m_mainWindow;
};
extern CSearchMgr* g_SearchMgr;

