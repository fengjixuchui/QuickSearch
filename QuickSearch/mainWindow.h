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
#pragma comment(lib,"comctl32.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' \
version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")

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

CListviewMgr listViewMgr;


HFONT hFont = CreateFont(
    -12/*�߶�*/, -6.5/*���*/, 0/*���ù�*/, 0/*���ù�*/, 400 /*һ�����ֵ��Ϊ400*/,
    FALSE/*����б��*/, FALSE/*�����»���*/, FALSE/*����ɾ����*/,
    DEFAULT_CHARSET,  //��������ʹ��Ĭ���ַ��������������� _CHARSET ��β�ĳ�������
    OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS,  //���в������ù�
    DEFAULT_QUALITY,  //Ĭ���������
    FF_DONTCARE,  //��ָ��������*/
    L"΢���ź�"  //������
);

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
    SendMessage(staticText, WM_SETTEXT, 0, (LPARAM)L"ɨ����...");
    CNtfsMgr::Instance()->initVolumes();
    CNtfsMgr::Instance()->ScanVolumeFileData();

    wchar_t strNote[30];
    _stprintf_s(strNote, L"ɨ��������ļ�����Ϊ %d",CNtfsMgr::Instance()->GetAllFileCnt());
    SendMessage(staticText, WM_SETTEXT, 0, (LPARAM)strNote);
    g_SearchMgr->Init();
    listViewMgr.Init(listview);
    SendMessage(mainWindow, MSG_FINISH_INIT, 0, 0);
    bInitFinish = TRUE;
}

void WINAPI Search(SearchOpt opt)
{
    ListView_DeleteAllItems(listview);
    g_SearchMgr->Search(opt);
    listViewMgr.updateIcon();
    listViewMgr.UpdateValidResultCnt();
    wchar_t strNote[30];
    _stprintf_s(strNote, L"��������,�ļ�����Ϊ %d", listViewMgr.m_ResultCnt);
    SendMessage(staticText, WM_SETTEXT, 0, (LPARAM)strNote);
    for (int i = 0; i < listViewMgr.m_ResultCnt; ++i)
    {
        //������Ŀ  
        item1.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_INDENT;
        item1.iItem = i;//��Ŀ��  
        SendMessage(listview, LVM_INSERTITEM, 0, (LPARAM)&item1);
    }
}
void FilterResult()
{
    ListView_DeleteAllItems(listview);
    listViewMgr.UpdateValidResultCnt();
    wchar_t strNote[30];
    _stprintf_s(strNote, L"��������,�ļ�����Ϊ %d", listViewMgr.m_ResultCnt);
    SendMessage(staticText, WM_SETTEXT, 0, (LPARAM)strNote);
    for (int i = 0; i < listViewMgr.m_ResultCnt; ++i)
    {
        //������Ŀ  
        item1.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_INDENT;
        item1.iItem = i;//��Ŀ��  
        SendMessage(listview, LVM_INSERTITEM, 0, (LPARAM)&item1);
    }
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
    int a = 0;
}