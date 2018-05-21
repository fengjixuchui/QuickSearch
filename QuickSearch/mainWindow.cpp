#include "mainWindow.h"
#include <WinUser.h>

WNDPROC oldEditProc;
LRESULT CALLBACK searchEditProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_RETURN:
        {
            TCHAR szBuf[255];
            ::GetWindowText(searchEdit, szBuf, 255);
            int dwlength = WideCharToMultiByte(CP_UTF8, 0, szBuf, 255, NULL, NULL, NULL, NULL);
            char* str = (char*)malloc(dwlength);
            WideCharToMultiByte(CP_UTF8, 0, szBuf, 255, str, dwlength + 2, NULL, NULL);
            strSearchText = str;
            delete[] str;
            if (bInitFinish)
            {
                if (strSearchText.length() != 0)
                {
                    SearchOpt opt;
                    opt.name = strSearchText;
                    opt.sortType = (SortType)listViewMgr.nSortColumn;
                    opt.bAscending = listViewMgr.bSortAscending;
                    opt.bUseRegex = bUseRegex;
                    g_SearchMgr->Search(opt);
                }
            } 
        }
        break;  
        }
        break;
    default:
        return CallWindowProc(oldEditProc, wnd, msg, wParam, lParam);
    }
    return 0;
}

//窗口处理函数  
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    mainWindow = hwnd;
    switch (uMsg)
    {
    case WM_CREATE:
    {

        RECT rect;
        GetClientRect(hwnd, &rect);
        UpdatePos(rect.right, rect.bottom);
        
        //添加服务器ip的编辑框  
        searchEdit = CreateWindow(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER,
            X_SearchEdit, Y_SearchEdit, Width_SearchEdit, Height_SearchEdit, hwnd, (HMENU)ID_EDITTEXT_FILENAME, NULL, NULL);
        oldEditProc = (WNDPROC)SetWindowLongPtr(searchEdit, GWLP_WNDPROC, (LONG_PTR)searchEditProc);

        int nWidth = 100;
        hBKStatic = CreateWindow(L"static", L"", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, \
                0, 0, Width_Window, 30, hwnd, (HMENU)ID_STATICTEXT_BK, NULL, NULL);
        hFilterStaticText = CreateWindow(L"static", L" 筛 选 : ", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, \
            0, 0, 50, 30, hwnd, (HMENU)ID_STATICTEXT_FILTER, NULL, NULL);

        hFilterDoc = CreateWindowEx(NULL, L"Button", L"文档", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, \
            50, 0, nWidth, 30, hwnd, (HMENU)ID_FILTER_DOCS, NULL, NULL);

        hFilterIMG = CreateWindowEx(NULL, L"Button", L"图片", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, \
            50+nWidth, 0, nWidth, 30, hwnd, (HMENU)ID_FILTER_IMGS, NULL, NULL);

        hFilterAudio = CreateWindowEx(NULL, L"Button", L"音频", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, \
            50+nWidth * 2, 0, nWidth, 30, hwnd, (HMENU)ID_FILTER_AUDIOS, NULL, NULL);

        hFilterVideo = CreateWindowEx(NULL, L"Button", L"视频", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, \
            50 + nWidth * 3, 0, nWidth, 30, hwnd, (HMENU)ID_FILTER_VIDEOS, NULL, NULL);

        hFilterCompressed = CreateWindowEx(NULL, L"Button", L"压缩文件", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, \
            50 + nWidth * 4, 0, nWidth, 30, hwnd, (HMENU)ID_FILTER_COMPRESSED, NULL, NULL);

        hFilterFolder = CreateWindowEx(NULL, L"Button", L"文件夹", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, \
            50 + nWidth * 5, 0, nWidth, 30, hwnd, (HMENU)ID_FILTER_FOLDERS, NULL, NULL);

        //创建listview  
        listview = CreateWindowEx(NULL, TEXT("SysListView32"), NULL, WS_CHILD | WS_VISIBLE | \
            LVS_REPORT | LVS_SINGLESEL | LVS_OWNERDATA | WS_HSCROLL, \
            X_Listview, Y_Listview, Width_Listview, Height_Listview, \
            hwnd, (HMENU)1, g_hInstance, NULL);

        ListView_SetExtendedListViewStyle(listview, LVS_EX_SUBITEMIMAGES);//设置listview扩展风格
        list1.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;//掩码  
        list1.fmt = LVCFMT_LEFT;
        list1.cx = 260;//列宽  
        list1.pszText = TEXT("文件名");
        SendMessage(listview, LVM_INSERTCOLUMN, 0, (LPARAM)&list1);//创建列
        list1.cx = 400;//列宽  
        list1.pszText = TEXT("路径");
        SendMessage(listview, LVM_INSERTCOLUMN, 1, (LPARAM)&list1);
        list1.fmt = LVCFMT_RIGHT;
        list1.pszText = TEXT("大小");
        list1.cx = 150;//列宽 
        SendMessage(listview, LVM_INSERTCOLUMN, 2, (LPARAM)&list1);
        list1.fmt = LVCFMT_LEFT;
        list1.pszText = TEXT("修改日期");
        list1.cx = 150;//列宽 
        SendMessage(listview, LVM_INSERTCOLUMN, 3, (LPARAM)&list1);

        //左下角提示框
        staticText = CreateWindow(L"static", L"", WS_CHILD | WS_VISIBLE | SS_LEFT | WS_BORDER, \
            X_StaticText, Y_StaticText, Width_StaticText, Height_StaticText, \
            hwnd, (HMENU)ID_STATICTEXT_NOTE, NULL, NULL);
        InitCommonControls();
        /*progressBar = CreateWindowEx(0, PROGRESS_CLASS, (LPTSTR)NULL,
            WS_CHILD | WS_VISIBLE, X_StaticText,
            Y_StaticText-30,Width_StaticText, Height_StaticText,hwnd, (HMENU)0, g_hInstance, NULL);
        SendMessage(progressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

        SendMessage(progressBar, PBM_SETSTEP, (WPARAM)10, 0);*/
        EnumChildWindows(hwnd, (WNDENUMPROC)SetFont, (LPARAM)hFont);
    }
        break;
    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        ListView_DeleteAllItems(listview);
        g_SearchMgr->m_vecResult.swap(std::vector<SearchResultItem>());
        SetWindowText(searchEdit, L"");
        SetWindowText(staticText, L"");
        return 0;
        //break;
    case WM_DESTROY:
        Shell_NotifyIcon(NIM_DELETE, &nid);
        CNtfsMgr::Instance()->UnInit();
        g_SearchMgr->UnInit();
        CUsnMonitorThread::Instance()->UnInit();   
        g_pIndexManager->UnInit();
        listViewMgr.UnInit();

        CloseHandle(InitThread);
        PostQuitMessage(0);//可以使GetMessage返回0  
        break;
    case WM_COMMAND:
        switch (HIWORD(wParam))
        {
        case EN_UPDATE:
            break;
        default:
            break;
        }
        if (HIWORD(wParam) == BN_CLICKED)
        {
            switch (LOWORD(wParam))
            {
            case ID_REGEX_CK:
            {
                // 获取窗口上的整个菜单栏的句柄  
                auto hmm = GetMenu(hwnd);
                // 获取第一个弹出菜单
                auto hfmn = GetSubMenu(hmm, 0);
                bUseRegex = !bUseRegex;
                CheckMenuItem(hfmn, ID_REGEX_CK, bUseRegex ? MF_CHECKED : MF_UNCHECKED);
            }
                break;
            case ID_ABOUT:
                MessageBox(hwnd, L"QuickSearch 1.0", L"关于", MB_OK | MB_ICONINFORMATION);
                break;
            case ID_EXIT:
                SendMessage(hwnd, WM_DESTROY, 0, 0);
                break;
            default:
                UpdateResultType(LOWORD(wParam));
                break;
            }
        }
        break;
    case WM_NOTIFY:
        listViewMgr.OnNotify(hwnd, wParam,lParam);
        break;
    case WM_SIZE:
    {
        HDWP hdwp;
        RECT rect;
        GetClientRect(hwnd, &rect);
        hdwp = BeginDeferWindowPos(4);
        UpdatePos(rect.right, rect.bottom);
        DeferWindowPos(hdwp, searchEdit, NULL, X_SearchEdit, Y_SearchEdit, Width_SearchEdit, Height_SearchEdit, SWP_NOZORDER);
        DeferWindowPos(hdwp, listview, NULL, X_Listview, Y_Listview, Width_Listview, Height_Listview, SWP_NOZORDER);
        DeferWindowPos(hdwp, staticText, NULL, X_StaticText, Y_StaticText, Width_StaticText, Height_StaticText, SWP_NOZORDER);
        DeferWindowPos(hdwp, hBKStatic, NULL, 0, 0, Width_StaticText, 30,SWP_NOZORDER);
        EndDeferWindowPos(hdwp);
        
    }
        break;
    case MSG_FINISH_INIT:
    {
        if (strSearchText.length() > 0)
        {
            SearchOpt opt;
            opt.name = strSearchText;
            opt.sortType = (SortType)listViewMgr.nSortColumn;
            opt.bAscending = listViewMgr.bSortAscending;
            opt.bUseRegex = bUseRegex;
            g_SearchMgr->Search(opt);
        }
    }
        break;
    case MSG_SORT_CHANGE:
    {
        if (bInitFinish && strSearchText.length() > 0)
        {
            SearchOpt opt;
            opt.name = strSearchText;
            opt.sortType = (SortType)listViewMgr.nSortColumn;
            opt.bAscending = listViewMgr.bSortAscending;
            opt.bUseRegex = bUseRegex;
            g_SearchMgr->Search(opt);
        }
    }
        break;
    case MSG_SEARCH_FINISH:
    {
        SearchFinish();
    }
    case WM_TRAY:
        switch (lParam)
        {
        case WM_RBUTTONDOWN:
        {
            //获取鼠标坐标  
            POINT pt; 
            GetCursorPos(&pt);

            //解决在菜单外单击左键菜单不消失的问题  
            SetForegroundWindow(hwnd);
            //使菜单某项变灰  
            //EnableMenuItem(hMenu, ID_SHOW, MF_GRAYED);      
            //显示并获取选中的菜单  
            int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, NULL, hwnd,NULL);
            if (cmd == ID_TRAY_SHOW)
                MessageBox(hwnd, L"实现托盘", L"托盘", MB_OK);
            if (cmd == ID_TRAY_EXIT)
                PostMessage(hwnd, WM_DESTROY, NULL, NULL);
        }
        break;
        case WM_LBUTTONDOWN:
            ShowWindow(hwnd, SW_SHOW);
            break;
        case WM_LBUTTONDBLCLK:
            ShowWindow(hwnd, SW_SHOW);
            break;
        }
        break;
    default:
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
//注册窗口类  
BOOL Register(LPCWSTR lpClassName, WNDPROC wndProc)
{
    WNDCLASSEX wce = { 0 };
    wce.cbSize = sizeof(wce);
    wce.cbClsExtra = 0;
    wce.cbWndExtra = 0;
    wce.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wce.hCursor = NULL;
    wce.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON1));;
    wce.hIconSm = NULL;
    wce.hInstance = g_hInstance;
    wce.lpfnWndProc = wndProc;
    wce.lpszClassName = lpClassName;
    wce.lpszMenuName = MAKEINTRESOURCE(IDR_MENU_MAIN);
    wce.style = CS_HREDRAW | CS_VREDRAW;
    ATOM nAtom = RegisterClassEx(&wce);
    if (nAtom == 0)
        return FALSE;
    return true;

}
//创建主窗口  
HWND CreateMain(LPCWSTR lpClassName, LPCWSTR lpWndName)
{
    HWND hWnd = CreateWindowEx(0, lpClassName, lpWndName,
        WS_OVERLAPPEDWINDOW, 500, 300, 1000,\
        800, NULL, NULL, g_hInstance, NULL);
    return hWnd;
}
//显示窗口  
void Display(HWND hWnd)
{
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
}
//消息循环  
void Message()
{
    MSG nMsg = { 0 };
    while (GetMessage(&nMsg, NULL, 0, 0))
    {
        TranslateMessage(&nMsg);
        DispatchMessage(&nMsg);
    }
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    // TODO: Place code here. 

    el::Configurations conf("elConfig.conf");
    el::Loggers::reconfigureAllLoggers(conf);
    el::Logger* defaultLogger = el::Loggers::getLogger("default");
    g_hInstance = hInstance;
    BOOL nRet = Register(L"Main", WndProc);
    
    if (!nRet)
    {
        MessageBox(NULL, L"注册失败", L"Infor", MB_OK);
        return 0;
    }
    HWND hWnd = CreateMain(L"Main", L"QuickSearch");
    Display(hWnd);
    InitTray(hInstance, hWnd);
    listViewMgr.Init(listview, mainWindow);
    listViewMgr.setListViewSortIcon();
    InitThread = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)INIT, NULL, 0, 0);

    //CloseHandle(InitThread);
    Message();
    return 0;
}

