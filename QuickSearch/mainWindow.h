#pragma once
#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP
#include <windows.h>
#include "resource.h"
#include "NtfsMgr.h"
#include "SearchMgr.h"
#include <windowsx.h>
#include <CommCtrl.h>
#include "ntfsUtils.h"
#include "Util.h"
#include "ListviewMgr.h"
#include <WinUser.h>
#include <tchar.h>
#include "UsnMonitorThread.h"
#include "ShellContextMenu.h"
#pragma comment(lib,"comctl32.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' \
version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

HINSTANCE g_hInstance = 0;

std::string strSearchText = "";
BOOL bInitFinish = FALSE;
//window
int Width_Window = 1000;
int Height_Window = 800;

//editText

int X_SearchEdit = 5;
int Y_SearchEdit = 30;
int Width_SearchEdit;// = Width_Window - 10;
int Height_SearchEdit = 25;

//listview
int X_Listview = 0;
int Y_Listview = Y_SearchEdit + 30;
int Width_Listview; //= Width_Window - 5;
int Height_Listview;//; = Height_Window - 100;

                    //note static
int X_StaticText = -1;
int Y_StaticText;// = Height_Window - 50;
int Width_StaticText;// = Width_Window;
int Height_StaticText = 20;



//filter
HWND hBKStatic;
HWND hFilterStaticText;
HWND hFilterDoc;
HWND hFilterIMG;
HWND hFilterAudio;
HWND hFilterVideo;
HWND hFilterCompressed;
HWND hFilterFolder;
//saerch edit text
HWND searchEdit;
//result listview
HWND listview;
LVCOLUMN list1;
LVITEM item1;
HIMAGELIST imglist1;
//mainwindow
HWND mainWindow;
//note static text
HWND staticText;
//processbar
HWND progressBar;
CListviewMgr listViewMgr;

BOOL bUseRegex = FALSE;
//thread
HANDLE InitThread;
//HANDLE SearchThread;

HFONT hFont = CreateFont(
    -12/*高度*/, -6.5/*宽度*/, 0/*不用管*/, 0/*不用管*/, 400 /*一般这个值设为400*/,
    FALSE/*不带斜体*/, FALSE/*不带下划线*/, FALSE/*不带删除线*/,
    DEFAULT_CHARSET,  //这里我们使用默认字符集，还有其他以 _CHARSET 结尾的常量可用
    OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS,  //这行参数不用管
    DEFAULT_QUALITY,  //默认输出质量
    FF_DONTCARE,  //不指定字体族*/
    L"微软雅黑"  //字体名
);

NOTIFYICONDATA nid;     //托盘属性  
HMENU hMenu;            //托盘菜单  

                        //实例化托盘  
void InitTray(HINSTANCE hInstance, HWND hWnd)
{
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = ID_TRAY;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
    nid.uCallbackMessage = WM_TRAY;
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    lstrcpy(nid.szTip, L"QuickSearch");

    hMenu = CreatePopupMenu();//生成托盘菜单  
                              //为托盘菜单添加两个选项  
    AppendMenu(hMenu, MF_STRING, ID_TRAY_SHOW, TEXT("提示"));
    AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, TEXT("退出"));

    Shell_NotifyIcon(NIM_ADD, &nid);
    
}

void UpdatePos(int width, int height)
{
    Width_Window = width;
    Height_Window = height;

    Width_SearchEdit = Width_Window - 10;
    Width_Listview = Width_Window;
    Height_Listview = Height_Window - 80;
    Y_StaticText = Height_Window - 18;
    Width_StaticText = Width_Window + 10;
}



bool CALLBACK SetFont(HWND child, LPARAM font) {
    SendMessage(child, WM_SETFONT, font, true);
    return true;
}

void WINAPI INIT()
{
    SendMessage(staticText, WM_SETTEXT, 0, (LPARAM)L"扫描中...");
    //初始化磁盘信息
    CNtfsMgr::Instance()->initVolumes();
    //扫描磁盘并建立索引
    CNtfsMgr::Instance()->ScanVolumeFileData();

    wchar_t strNote[255];
    _stprintf_s(strNote, L"扫描结束，文件数量为: %d",CNtfsMgr::Instance()->GetAllFileCnt());
    SendMessage(staticText, WM_SETTEXT, 0, (LPARAM)strNote);

    //搜索线程初始化
    g_SearchMgr->Init(mainWindow);
    CUsnMonitorThread::Instance()->Init();
    bInitFinish = TRUE;
    PostMessage(mainWindow, MSG_FINISH_INIT, 0, 0);
}



void SearchFinish()
{
    ListView_DeleteAllItems(listview);
    listViewMgr.updateIcon();
    listViewMgr.UpdateValidResult();
    wchar_t strNote[255];
    _stprintf_s(strNote, L"搜索结束，耗时: %dms，找到文件数量为: %d, 显示文件数量：%d", g_SearchMgr->dwSearchTime,\
        g_SearchMgr->dwResultCnt, listViewMgr.m_ResultCnt);
    SendMessage(staticText, WM_SETTEXT, 0, (LPARAM)strNote);
    for (int i = 0; i < listViewMgr.m_ResultCnt; ++i)
    {
        //创建项目  
        item1.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_INDENT;
        item1.iItem = i;//项目号  
        SendMessage(listview, LVM_INSERTITEM, 0, (LPARAM)&item1);
    }
    SetFocus(listview);
}
void FilterResult()
{
    ListView_DeleteAllItems(listview);
    listViewMgr.UpdateValidResult();
    wchar_t strNote[30];
    _stprintf_s(strNote, L"搜索结束,文件数量为 %d", listViewMgr.m_ResultCnt);
    SendMessage(staticText, WM_SETTEXT, 0, (LPARAM)strNote);
    for (int i = 0; i < listViewMgr.m_ResultCnt; ++i)
    {
        //创建项目  
        item1.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_INDENT;
        item1.iItem = i;//项目号  
        SendMessage(listview, LVM_INSERTITEM, 0, (LPARAM)&item1);
    }
    SetFocus(listview);
}
void UpdateResultType(int nID)
{
    switch (nID)
    {
    case ID_FILTER_DOCS:
        if (SendDlgItemMessage(mainWindow, ID_FILTER_DOCS, BM_GETCHECK, 0, 0))
        {
            listViewMgr.m_ResultTypeMask |= TypeDocuments;
        }
        else
        {
            listViewMgr.m_ResultTypeMask &= ~TypeDocuments;
        }
        break;
    case ID_FILTER_IMGS:
        if (SendDlgItemMessage(mainWindow, ID_FILTER_IMGS, BM_GETCHECK, 0, 0))
        {
            listViewMgr.m_ResultTypeMask |= TypeImages;
        }
        else
        {
            listViewMgr.m_ResultTypeMask &= ~TypeImages;
        }
        break;
    case ID_FILTER_AUDIOS:
        if (SendDlgItemMessage(mainWindow, ID_FILTER_AUDIOS, BM_GETCHECK, 0, 0))
        {
            listViewMgr.m_ResultTypeMask |= TypeAudios;
        }
        else
        {
            listViewMgr.m_ResultTypeMask &= ~TypeAudios;
        }
        break;
    case ID_FILTER_VIDEOS:
        if (SendDlgItemMessage(mainWindow, ID_FILTER_VIDEOS, BM_GETCHECK, 0, 0))
        {
            listViewMgr.m_ResultTypeMask |= TypeVideos;
        }
        else
        {
            listViewMgr.m_ResultTypeMask &= ~TypeVideos;
        }
        break;
    case ID_FILTER_COMPRESSED:
        if (SendDlgItemMessage(mainWindow, ID_FILTER_COMPRESSED, BM_GETCHECK, 0, 0))
        {
            listViewMgr.m_ResultTypeMask |= TypeCompressed;
        }
        else
        {
            listViewMgr.m_ResultTypeMask &= ~TypeCompressed;
        }
        break;
    case ID_FILTER_FOLDERS:
        if (SendDlgItemMessage(mainWindow, ID_FILTER_FOLDERS, BM_GETCHECK, 0, 0))
        {
            listViewMgr.m_ResultTypeMask |= TypeFolders;
        }
        else
        {
            listViewMgr.m_ResultTypeMask &= ~TypeFolders;
        }
        break;
    default:
        break;
    }
    FilterResult();
}

