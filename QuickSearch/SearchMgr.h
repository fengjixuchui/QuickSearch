#pragma once
#include <vector>
#include "SearchThread.h"
#include <unordered_map>
class CSearchMgr
{
public:
    CSearchMgr();
    ~CSearchMgr();
    void Init();
    void UnInit();
    void Search(SearchOpt opt);
    int GetResultCnt(int dwIndex);
    void ReSortResult();
    short GetFileType(std::wstring& filename);
    void InitClassFilter(const std::wstring* arrayPtr, int size, int filterType);
public:
    std::vector<CSearchThread*> m_vecSearchThread;
    std::vector<SearchResultItem> m_vecResult;
private:
    SearchOpt searchOpt;
    std::unordered_map<std::wstring, int> m_MapExtClassFilter;
};
extern CSearchMgr* g_SearchMgr;

