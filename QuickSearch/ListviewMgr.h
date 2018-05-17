#pragma once
#include <windows.h>
#include "global.h"
#include <map>
#include <CommCtrl.h>


class CListviewMgr
{
public:
    CListviewMgr();
    ~CListviewMgr();
    void Init(HWND hwnd);
    void UnInit();
    void RetrieveItem(SearchResultItem& pENtry, int index);
    LRESULT OnNotify(HWND hwnd, NMHDR* pnmhdr);
    void PrepCache(int iFrom, int iTo);
    HICON fileIcon(std::wstring extention);
    void updateIcon();
    void UpdateValidResultCnt();
public:
    int m_ResultTypeMask;
    int m_ResultCnt;
private:
    HWND m_hListview;
    HIMAGELIST m_iconList;
    std::map<std::wstring, UINT> filetype_iconIndex_Map;
    std::vector<HICON> VeciconIndex;
    std::vector<int> m_vecValidIndex;
};

