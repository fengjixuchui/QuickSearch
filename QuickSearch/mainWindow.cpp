#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP
#include<windows.h>
#include "NtfsMgr.h"
HINSTANCE g_hInstance = 0;


//���ڴ�����  
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);//����ʹGetMessage����0  
        break;
    case WM_LBUTTONDOWN:
        MessageBox(NULL, L"���������", L"Win32_Mouse", MB_OK);
        CNtfsMgr::Instance()->initVolumes();
        CNtfsMgr::Instance()->ScanVolumeFileData();
        CNtfsMgr::Instance()->SaveDatabase();
        break;
    case WM_RBUTTONDOWN:
        MessageBox(NULL, L"����Ҽ����", L"Win32_Mouse", MB_OK);
        break;
    default:
        break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
//ע�ᴰ����  
BOOL Register(LPCWSTR lpClassName, WNDPROC wndProc)
{
    WNDCLASSEX wce = { 0 };
    wce.cbSize = sizeof(wce);
    wce.cbClsExtra = 0;
    wce.cbWndExtra = 0;
    wce.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wce.hCursor = NULL;
    wce.hIcon = NULL;
    wce.hIconSm = NULL;
    wce.hInstance = g_hInstance;
    wce.lpfnWndProc = wndProc;
    wce.lpszClassName = lpClassName;
    wce.lpszMenuName = NULL;
    wce.style = CS_HREDRAW | CS_VREDRAW;
    ATOM nAtom = RegisterClassEx(&wce);
    if (nAtom == 0)
        return FALSE;
    return true;

}
//����������  
HWND CreateMain(LPCWSTR lpClassName, LPCWSTR lpWndName)
{
    HWND hWnd = CreateWindowEx(0, lpClassName, lpWndName,
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, g_hInstance, NULL);
    return hWnd;
}
//��ʾ����  
void Display(HWND hWnd)
{
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
}
//��Ϣѭ��  
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
        MessageBox(NULL, L"ע��ʧ��", L"Infor", MB_OK);
        return 0;
    }
    
    
    HWND hWnd = CreateMain(L"Main", L"window");
    Display(hWnd);

    Message();
    return 0;
}