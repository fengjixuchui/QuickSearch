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
    void Init(HWND hwnd,HWND mainWindow);
    void UnInit();
    void RetrieveItem(SearchResultItem& pENtry, int index);
    LRESULT OnNotify(HWND hwnd, WPARAM, LPARAM);
    void PrepCache(int iFrom, int iTo);
    HICON fileIcon(std::wstring extention);
    void updateIcon();
    void UpdateValidResultCnt();
    void setListViewSortIcon();
    void OnColumnClick(LPNMLISTVIEW pLVInfo);
public:
    int m_ResultTypeMask;
    int m_ResultCnt;
    BOOL bSortAscending;
    int nSortColumn;
    int nRClickItem;
private:
    HWND m_hListview;
    HWND m_hMainWindow;
    HIMAGELIST m_iconList;
    std::map<std::wstring, UINT> filetype_iconIndex_Map;
    std::vector<HICON> VeciconIndex;
    std::vector<int> m_vecValidIndex;
    NMITEMACTIVATE m_itemActivate;
};

