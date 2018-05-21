// ShellContextMenu.cpp: Implementierung der Klasse CShellContextMenu.
//
//////////////////////////////////////////////////////////////////////


#include "ShellContextMenu.h"
#include <strsafe.h>
#include "Util.h"
#include "Shlwapi.h"
#include "easylogging++.h"
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

#define MIN_ID 1
#define MAX_ID 10000

#define Menu_OpenPath (0x4000)
#define Menu_CopyPathToClipboard (0x4001)

IContextMenu2 * g_IContext2 = NULL;
IContextMenu3 * g_IContext3 = NULL;

CShellContextMenu::CShellContextMenu()
{
	m_psfFolder = NULL;
	m_pidlArray = NULL;
	m_Menu = NULL;
}

CShellContextMenu::~CShellContextMenu()
{
	// free all allocated datas
	if (m_psfFolder && bDelete)
		m_psfFolder->Release ();
	m_psfFolder = NULL;
    if (m_pidlArray != NULL)
    {
        FreePIDLArray (m_pidlArray);
        m_pidlArray = NULL;
    }
// 	if (m_Menu)
// 		delete m_Menu;
}



// this functions determines which version of IContextMenu is avaibale for those objects (always the highest one)
// and returns that interface
BOOL CShellContextMenu::GetContextMenu (void ** ppContextMenu, int & iMenuType)
{
	*ppContextMenu = NULL;
	LPCONTEXTMENU icm1 = NULL;
	if (!m_psfFolder)
	{
		return FALSE;
	}
	
	// first we retrieve the normal IContextMenu interface (every object should have it)
	m_psfFolder->GetUIObjectOf (NULL, nItems, (LPCITEMIDLIST *) m_pidlArray, IID_IContextMenu, NULL, (void**) &icm1);

	if (icm1)
	{	// since we got an IContextMenu interface we can now obtain the higher version interfaces via that
		if (icm1->QueryInterface (IID_IContextMenu3, ppContextMenu) == NOERROR)
			iMenuType = 3;
		else if (icm1->QueryInterface (IID_IContextMenu2, ppContextMenu) == NOERROR)
			iMenuType = 2;

		if (*ppContextMenu) 
			icm1->Release(); // we can now release version 1 interface, cause we got a higher one
		else 
		{	
			iMenuType = 1;
			*ppContextMenu = icm1;	// since no higher versions were found
		}							// redirect ppContextMenu to version 1 interface
	}
	else
		return (FALSE);	// something went wrong
	
	return (TRUE); // success
}


LRESULT CALLBACK CShellContextMenu::HookWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{ 
	case WM_MENUCHAR:	// only supported by IContextMenu3
		if (g_IContext3)
		{
			LRESULT lResult = 0;
			g_IContext3->HandleMenuMsg2 (message, wParam, lParam, &lResult);
			return (lResult);
		}
		break;

	case WM_DRAWITEM:
	case WM_MEASUREITEM:
		if (wParam) 
			break; // if wParam != 0 then the message is not menu-related

	case WM_INITMENUPOPUP:
		if (g_IContext2)
			g_IContext2->HandleMenuMsg (message, wParam, lParam);
		else if(g_IContext3)// version 3
			g_IContext3->HandleMenuMsg (message, wParam, lParam);
		//return (message == WM_INITMENUPOPUP ? 0 : TRUE); // inform caller that we handled WM_INITPOPUPMENU by ourself
		break;

	default:
		break;
	}

	// call original WndProc of window to prevent undefined bevhaviour of window
	//return ::CallWindowProc ((WNDPROC) GetProp ( hWnd, TEXT ("OldWndProc")), hWnd, message, wParam, lParam);
	return ::DefWindowProc((HWND)hWnd, message, wParam, lParam);
}


UINT CShellContextMenu::ShowContextMenu(HWND hWnd, POINT pt)
{
	int iMenuType = 0;	// to know which version of IContextMenu is supported
	LPCONTEXTMENU pContextMenu;	// common pointer to IContextMenu and higher version interface
   
	if (!GetContextMenu ((void**) &pContextMenu, iMenuType))	
		return (0);	// something went wrong
// 
// 	if (!m_Menu)
// 	{
// 		delete m_Menu;
// 		m_Menu = NULL;
// 		m_Menu = new CMenu;
// 		m_Menu->CreatePopupMenu ();
// 	}
	m_Menu = ::CreatePopupMenu ();
	// lets fill the our popupmenu  
	pContextMenu->QueryContextMenu (m_Menu, GetMenuItemCount(m_Menu), MIN_ID, MAX_ID, CMF_NORMAL | CMF_EXPLORE);
 
	InsertCustomMenu(m_Menu);
	// subclass window to handle menurelated messages in CShellContextMenu 
	WNDPROC OldWndProc;
	if (iMenuType > 1)	// only subclass if its version 2 or 3
	{
		OldWndProc = (WNDPROC) SetWindowLongPtr (hWnd, -4, (LONG_PTR) HookWndProc);
		if (iMenuType == 2)
			g_IContext2 = (LPCONTEXTMENU2) pContextMenu;
		else	// version 3
			g_IContext3 = (LPCONTEXTMENU3) pContextMenu;
	}
	else
		OldWndProc = NULL;

	UINT idCommand = ::TrackPopupMenu (m_Menu, TPM_RETURNCMD | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
    UINT idRet = 0;
	if (OldWndProc) // unsubclass
		SetWindowLongPtr (hWnd, -4, (LONG_PTR) OldWndProc);
	if (idCommand == Menu_OpenPath)
	{
		OnOpenFolder(strArray.front());
        LOG(INFO) << __FUNCTIONW__ << " open path";
	}
	else if (idCommand == Menu_CopyPathToClipboard)
	{
		OnCopyPath(hWnd, strArray.front());
        LOG(INFO) << __FUNCTIONW__ << " Copy file To Clipboard";
	}

	if (idCommand >= MIN_ID && idCommand <= MAX_ID)	// see if returned idCommand belongs to shell menu entries
	{
		char szBuf[64] = { 0 };
		std::wstring strRealPath;
		pContextMenu->GetCommandString(idCommand - MIN_ID, GCS_VERBA, NULL, szBuf, sizeof(szBuf) - 1);

		if (::_stricmp(szBuf, "cut") == 0)
		{
			 OnCopyOrCutFiles(NULL, FALSE, strArray);
             LOG(INFO) << __FUNCTIONW__ << " cut file To Clipboard";

		}
		else if(::_stricmp(szBuf, "copy") == 0)
		{
			 OnCopyOrCutFiles(NULL, TRUE, strArray);
             LOG(INFO) << __FUNCTIONW__ << " copy file To Clipboard";
		}
		else
		{
			if (::_stricmp(szBuf, "open") == 0)
			{
                LOG(INFO) << __FUNCTIONW__ << " open file";
			}
			else if (::_stricmp(szBuf, "delete") == 0)
			{
                LOG(INFO) << __FUNCTIONW__ << " delete file";
			}
			HRESULT hr = InvokeCommand (pContextMenu, idCommand - MIN_ID);	// execute related command
			if (hr == S_OK)
			{
				//OutputDebugStringA("----");
			}
 		}
		idCommand = 0;
	}
	
	pContextMenu->Release();
	DestroyMenu(m_Menu);
	m_Menu = NULL;
	g_IContext2 = NULL;
	g_IContext3 = NULL;

	return (idRet);
}


HRESULT CShellContextMenu::InvokeCommand (LPCONTEXTMENU pContextMenu, UINT idCommand)
{
	CMINVOKECOMMANDINFO cmi = {0};
	cmi.cbSize = sizeof (CMINVOKECOMMANDINFO);
	cmi.lpVerb = (LPSTR) MAKEINTRESOURCE (idCommand);
	cmi.nShow = SW_SHOWNORMAL;
	
	return pContextMenu->InvokeCommand (&cmi);
}


void CShellContextMenu::SetObjects(std::wstring strObject)
{
	// only one object is passed
	//std::vector<std::wstring> strArray;
	if (!::PathFileExists(strObject.c_str()))
	{
		return;
	}
	strArray.push_back(strObject);	// create a CStringArray with one element
	
	SetObjects (strArray);		// and pass it to SetObjects (CStringArray &strArray)
								// for further processing
}


void CShellContextMenu::SetObjects(const std::vector<std::wstring>& strArray)
{
	// free all allocated datas
	if (m_psfFolder && bDelete)
		m_psfFolder->Release ();
	m_psfFolder = NULL;
	FreePIDLArray (m_pidlArray);
	m_pidlArray = NULL;
	
	// get IShellFolder interface of Desktop (root of shell namespace)
	IShellFolder * psfDesktop = NULL;
	SHGetDesktopFolder (&psfDesktop);	// needed to obtain full qualified pidl

	// ParseDisplayName creates a PIDL from a file system path relative to the IShellFolder interface
	// but since we use the Desktop as our interface and the Desktop is the namespace root
	// that means that it's a fully qualified PIDL, which is what we need
	LPITEMIDLIST pidl = NULL;
	
#ifndef _UNICODE
// 	OLECHAR * olePath = NULL;
// 	olePath = (OLECHAR *) calloc (strArray.GetAt (0).GetLength () + 1, sizeof (OLECHAR));
// 	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, ((CString)(strArray.GetAt(0))).GetBuffer (0), -1, olePath, strArray.GetAt (0).GetLength () + 1);	
// 	psfDesktop->ParseDisplayName (NULL, 0, olePath, NULL, &pidl, NULL);
// 	free (olePath);
#else
	psfDesktop->ParseDisplayName (NULL, 0, (LPOLESTR)strArray.at(0).c_str(), NULL, &pidl, NULL);
#endif


	// now we need the parent IShellFolder interface of pidl, and the relative PIDL to that interface
	LPITEMIDLIST pidlItem = NULL;	// relative pidl
	SHBindToParentEx (pidl, IID_IShellFolder, (void **) &m_psfFolder, NULL);
	free (pidlItem);
	// get interface to IMalloc (need to free the PIDLs allocated by the shell functions)
	LPMALLOC lpMalloc = NULL;
	SHGetMalloc (&lpMalloc);
	lpMalloc->Free (pidl);

	// now we have the IShellFolder interface to the parent folder specified in the first element in strArray
	// since we assume that all objects are in the same folder (as it's stated in the MSDN)
	// we now have the IShellFolder interface to every objects parent folder
	
	IShellFolder * psfFolder = NULL;
	nItems = static_cast<int>(strArray.size());
	for (int i = 0; i < nItems; i++)
	{
#ifndef _UNICODE
// 		olePath = (OLECHAR *) calloc (strArray.GetAt (i).GetLength () + 1, sizeof (OLECHAR));
// 		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, ((CString)strArray.GetAt (i)).GetBuffer (0), -1, olePath, strArray.GetAt (i).GetLength () + 1);	
// 		psfDesktop->ParseDisplayName (NULL, 0, olePath, NULL, &pidl, NULL);
// 		free (olePath);
#else
		psfDesktop->ParseDisplayName (NULL, 0, (LPOLESTR)strArray.at(i).c_str(), NULL, &pidl, NULL);
#endif
		m_pidlArray = (LPITEMIDLIST *) realloc (m_pidlArray, (i + 1) * sizeof (LPITEMIDLIST));
		// get relative pidl via SHBindToParent
		HRESULT hr = SHBindToParentEx (pidl, IID_IShellFolder, (void **) &psfFolder, (LPCITEMIDLIST *) &pidlItem);
		if (SUCCEEDED(hr))
		{
			m_pidlArray[i] = CopyPIDL (pidlItem);	// copy relative pidl to pidlArray
			free (pidlItem);
			lpMalloc->Free (pidl);		// free pidl allocated by ParseDisplayName
			psfFolder->Release ();
		}
	}
	lpMalloc->Release ();
	psfDesktop->Release ();

	bDelete = TRUE;	// indicates that m_psfFolder should be deleted by CShellContextMenu
}


// only one full qualified PIDL has been passed
void CShellContextMenu::SetObjects(LPITEMIDLIST pidl)
{
	// free all allocated datas
	if (m_psfFolder && bDelete)
		m_psfFolder->Release ();
	m_psfFolder = NULL;
	FreePIDLArray (m_pidlArray);
	m_pidlArray = NULL;

		// full qualified PIDL is passed so we need
	// its parent IShellFolder interface and its relative PIDL to that
	LPITEMIDLIST pidlItem = NULL;
	SHBindToParent ((LPCITEMIDLIST) pidl, IID_IShellFolder, (void **) &m_psfFolder, (LPCITEMIDLIST *) &pidlItem);	

	m_pidlArray = (LPITEMIDLIST *) malloc (sizeof (LPITEMIDLIST));	// allocate ony for one elemnt
	m_pidlArray[0] = CopyPIDL (pidlItem);


	// now free pidlItem via IMalloc interface (but not m_psfFolder, that we need later
	LPMALLOC lpMalloc = NULL;
	SHGetMalloc (&lpMalloc);
	lpMalloc->Free (pidlItem);
	lpMalloc->Release();

	nItems = 1;
	bDelete = TRUE;	// indicates that m_psfFolder should be deleted by CShellContextMenu
}


// IShellFolder interface with a relative pidl has been passed
void CShellContextMenu::SetObjects(IShellFolder *psfFolder, LPITEMIDLIST pidlItem)
{
	// free all allocated datas
	if (m_psfFolder && bDelete)
		m_psfFolder->Release ();
	m_psfFolder = NULL;
	FreePIDLArray (m_pidlArray);
	m_pidlArray = NULL;

	m_psfFolder = psfFolder;

	m_pidlArray = (LPITEMIDLIST *) malloc (sizeof (LPITEMIDLIST));
	m_pidlArray[0] = CopyPIDL (pidlItem);
	
	nItems = 1;
	bDelete = FALSE;	// indicates wheter m_psfFolder should be deleted by CShellContextMenu
}

void CShellContextMenu::SetObjects(IShellFolder * psfFolder, LPITEMIDLIST *pidlArray, int nItemCount)
{
	// free all allocated datas
	if (m_psfFolder && bDelete)
		m_psfFolder->Release ();
	m_psfFolder = NULL;
	FreePIDLArray (m_pidlArray);
	m_pidlArray = NULL;

	m_psfFolder = psfFolder;

	m_pidlArray = (LPITEMIDLIST *) malloc (nItemCount * sizeof (LPITEMIDLIST));

	for (int i = 0; i < nItemCount; i++)
		m_pidlArray[i] = CopyPIDL (pidlArray[i]);

	nItems = nItemCount;
	bDelete = FALSE;	// indicates wheter m_psfFolder should be deleted by CShellContextMenu
}


void CShellContextMenu::FreePIDLArray(LPITEMIDLIST *pidlArray)
{
	if (!pidlArray)
		return;

	int iSize = static_cast<int>( _msize (pidlArray) / sizeof (LPITEMIDLIST) ) ;

	for (int i = 0; i < iSize; i++)
		free (pidlArray[i]);
	free (pidlArray);
}


LPITEMIDLIST CShellContextMenu::CopyPIDL (LPCITEMIDLIST pidl, int cb)
{
	if (cb == -1)
		cb = GetPIDLSize (pidl); // Calculate size of list.

    LPITEMIDLIST pidlRet = (LPITEMIDLIST) calloc (cb + sizeof (USHORT), sizeof (BYTE));
    if (pidlRet)
		CopyMemory(pidlRet, pidl, cb);

    return (pidlRet);
}


UINT CShellContextMenu::GetPIDLSize (LPCITEMIDLIST pidl)
{  
	if (!pidl) 
		return 0;
	int nSize = 0;
	LPITEMIDLIST pidlTemp = (LPITEMIDLIST) pidl;
	while (pidlTemp->mkid.cb)
	{
		nSize += pidlTemp->mkid.cb;
		pidlTemp = (LPITEMIDLIST) (((LPBYTE) pidlTemp) + pidlTemp->mkid.cb);
	}
	return nSize;
}

HMENU CShellContextMenu::GetMenu()
{
	if (!m_Menu)
	{
		m_Menu = ::CreatePopupMenu();	// create the popupmenu (its empty)
	}
	return (m_Menu);
}


// this is workaround function for the Shell API Function SHBindToParent
// SHBindToParent is not available under Win95/98
HRESULT CShellContextMenu::SHBindToParentEx (LPCITEMIDLIST pidl, REFIID riid, VOID **ppv, LPCITEMIDLIST *ppidlLast)
{
	HRESULT hr = 0;
	if (!pidl || !ppv)
		return E_POINTER;
	
	int nCount = GetPIDLCount (pidl);
	if (nCount == 0)	// desktop pidl of invalid pidl
		return E_POINTER;

	IShellFolder * psfDesktop = NULL;
	SHGetDesktopFolder (&psfDesktop);
	if (nCount == 1)	// desktop pidl
	{
		if ((hr = psfDesktop->QueryInterface(riid, ppv)) == S_OK)
		{
			if (ppidlLast) 
				*ppidlLast = CopyPIDL (pidl);
		}
		psfDesktop->Release ();
		return hr;
	}

	LPBYTE pRel = GetPIDLPos (pidl, nCount - 1);
	LPITEMIDLIST pidlParent = NULL;
	pidlParent = CopyPIDL (pidl, static_cast<int>(pRel - (LPBYTE) pidl));
	IShellFolder * psfFolder = NULL;
	
	if ((hr = psfDesktop->BindToObject (pidlParent, NULL, __uuidof (psfFolder), (void **) &psfFolder)) != S_OK)
	{
		free (pidlParent);
		psfDesktop->Release ();
		return hr;
	}
	if ((hr = psfFolder->QueryInterface (riid, ppv)) == S_OK)
	{
		if (ppidlLast)
			*ppidlLast = CopyPIDL ((LPCITEMIDLIST) pRel);
	}
	free (pidlParent);
	psfFolder->Release ();
	psfDesktop->Release ();
	return hr;
}


LPBYTE CShellContextMenu::GetPIDLPos (LPCITEMIDLIST pidl, int nPos)
{
	if (!pidl)
		return 0;
	int nCount = 0;
	
	BYTE * pCur = (BYTE *) pidl;
	while (((LPCITEMIDLIST) pCur)->mkid.cb)
	{
		if (nCount == nPos)
			return pCur;
		nCount++;
		pCur += ((LPCITEMIDLIST) pCur)->mkid.cb;	// + sizeof(pidl->mkid.cb);
	}
	if (nCount == nPos) 
		return pCur;
	return NULL;
}


int CShellContextMenu::GetPIDLCount (LPCITEMIDLIST pidl)
{
	if (!pidl)
		return 0;

	int nCount = 0;
	BYTE*  pCur = (BYTE *) pidl;
	while (((LPCITEMIDLIST) pCur)->mkid.cb)
	{
		nCount++;
		pCur += ((LPCITEMIDLIST) pCur)->mkid.cb;
	}
	return nCount;
}

VOID CShellContextMenu::OnCopyOrCutFiles(HWND hWnd, BOOL bCopy, const std::vector<std::wstring>& strFiles)
{
	if (!::OpenClipboard(hWnd))
	{
		return;
	}

	::EmptyClipboard();

	UINT uMessage = ::RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
	HANDLE hMemMessage = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(DWORD));
	DWORD* pMsgMem = (DWORD*)::GlobalLock(hMemMessage);
	if (!pMsgMem)
	{
		::GlobalFree(hMemMessage);
	}
	else
	{
		if (bCopy)
		{
			*pMsgMem = DROPEFFECT_COPY;
		}
		else
		{
			*pMsgMem = DROPEFFECT_MOVE;
		}
		::GlobalUnlock(hMemMessage);
	}

	DWORD dwFileDataSize = 0;
	BYTE* pData = NULL;
	std::vector<std::wstring>::const_iterator Iter = strFiles.begin();
	for (; Iter != strFiles.end(); ++Iter)
	{
		DWORD dwCurrentSize = (DWORD)((Iter->size() + 1) * 2);       // 宽字符
		dwFileDataSize += dwCurrentSize;
		BYTE* pTmp = (BYTE*)::realloc(pData, dwFileDataSize);
		if (pTmp)
		{
			pData = pTmp;
			::ZeroMemory(pData + dwFileDataSize - dwCurrentSize, dwCurrentSize);
			::memcpy(pData + dwFileDataSize - dwCurrentSize, Iter->c_str(), dwCurrentSize);
		}
	}

	DROPFILES FileObj = {sizeof(DROPFILES), {0, 0}, FALSE, TRUE};
	int nGlobalAllocSize = sizeof(DROPFILES) + dwFileDataSize + 4;

	HANDLE hMem = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, nGlobalAllocSize);
	BYTE* pDataMem = (BYTE*)::GlobalLock(hMem);
	if (!pDataMem)
	{
		::GlobalFree(hMem);
	}
	else
	{
		::memcpy(pDataMem, &FileObj, sizeof(DROPFILES));
		::memcpy(pDataMem + sizeof(DROPFILES), pData, dwFileDataSize);
		::GlobalUnlock(hMem);
	}

	if (pData)
	{
		::free(pData);
	}

	if (pDataMem)
	{
		::SetClipboardData(CF_HDROP, hMem);
	}
	if (pMsgMem)
	{
		::SetClipboardData(uMessage, hMemMessage);
	}

	::CloseClipboard();
}

BOOL CShellContextMenu::InsertNewMenuItem(HMENU hMenu, DWORD dwMenuID, const TCHAR* pszItemName, int nMenuIndex, BOOL bAddSep, int nIconIndex)
{
	if (!hMenu || !pszItemName)
	{
		return FALSE;
	}

	std::wstring wsMarkText = pszItemName;
	MENUITEMINFO mii = { 0 };
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_ID | MIIM_STRING;
	mii.wID = dwMenuID;
	mii.dwTypeData = (LPWSTR)wsMarkText.c_str();

	::InsertMenuItem(hMenu, nMenuIndex, TRUE, &mii);
	if (bAddSep)
	{
		::InsertMenu(hMenu, nMenuIndex + 1, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);   //分割线
	}

 //	::SetMenuItemBitmaps(hMenu, dwMenuID, MF_BYCOMMAND, GetBitmapByIndex(nIconIndex), NULL);

	return TRUE;
}

void CShellContextMenu::InsertCustomMenu( HMENU hMenu )
{
	if (strArray.size() != 1)
	{
		return;
	}
	int nFoundIndex = -1;
	int nMenuCount = ::GetMenuItemCount(hMenu);
	int nSepIndex = 0;
	for (; nSepIndex < nMenuCount; ++nSepIndex)
	{
        wchar_t strText[255];
		MENUITEMINFO info = { 0 };
		info.cbSize = sizeof(info);
		info.fMask  = MIIM_FTYPE | MIIM_STRING; 
		info.dwTypeData = strText; 
		info.cch = 254;
		::GetMenuItemInfo(hMenu, nSepIndex, TRUE, &info);

		if(wcscmp(info.dwTypeData, L"打开(&o)") == 0)
		{
			nFoundIndex = nSepIndex;
			break;
		}
	}
	if (nFoundIndex != -1)
	{
		InsertNewMenuItem(hMenu, Menu_OpenPath, L"打开文件所在位置(&I)", nFoundIndex+1, FALSE, 0);
		InsertNewMenuItem(hMenu, Menu_CopyPathToClipboard, L"复制完整路径和文件名(&F)", nFoundIndex + 2, FALSE, 0);
	}
	else
	{
		InsertNewMenuItem(hMenu, Menu_OpenPath, L"打开文件所在位置(&I)",1, FALSE, 0);
		InsertNewMenuItem(hMenu, Menu_CopyPathToClipboard, L"复制完整路径和文件名(&F)", 2, FALSE, 0);
	}
}

void CShellContextMenu::OnOpenFolder( const std::wstring& strPath )
{
	if (::PathFileExists(strPath.c_str()))
	{
        /*
        if (GetFileAttributes(strPath.c_str()) == FILE_ATTRIBUTE_DIRECTORY)
            WindowUtil::OpenDir(strPath.c_str());
        else*/
        {
            std::wstring fillStr = L"\"";
            std::wstring referenceStr = L",";
            std::wstring sourceStr = strPath;
            StringFillString(sourceStr, fillStr, referenceStr);
            std::wstring lpParameters = L"/select, " + sourceStr;
            SHELLEXECUTEINFO shex = { 0 };
            shex.cbSize = sizeof(SHELLEXECUTEINFO);
            shex.lpFile = TEXT("explorer");
            shex.lpParameters = lpParameters.c_str();
            shex.lpVerb = TEXT("open");
            shex.nShow = SW_SHOWDEFAULT;
            shex.lpDirectory = NULL;
            ShellExecuteEx(&shex);
        }
        /*
		std::wstring strTmp = strPath;
        std::wstring fillStr = L"\"";
        std::wstring referenceStr = L",";
        stringUtils::StringFillString(strTmp, fillStr, referenceStr);
		if (GetFileAttributes(strPath.c_str()) != FILE_ATTRIBUTE_DIRECTORY)
		{
			strTmp = L"/select, ";
			strTmp += strPath;
		}
		TCHAR szExplorer[MAX_PATH] = {0};
		ExpandEnvironmentStringsW(L"%windir%\\explorer.exe", szExplorer, MAX_PATH);
		ShellExecuteW(NULL, L"open", szExplorer, strTmp.c_str(), NULL, SW_SHOW);*/
	}
}

void CShellContextMenu::OnCopyPath( HWND hWnd, const std::wstring& strPath )
{
	HGLOBAL hClip = NULL; 
	size_t nLen = (strPath.size() + 1)*sizeof(wchar_t);  
	if (OpenClipboard(hWnd))
	{  
		hClip = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, nLen);
		if (hClip)
		{
			EmptyClipboard();  
			wchar_t* buff = (wchar_t*)GlobalLock(hClip);
			StringCchCopyW(buff, strPath.size() + 1, strPath.c_str());
			GlobalUnlock(hClip);
			SetClipboardData(CF_UNICODETEXT, hClip);
		}
		CloseClipboard();
	}
}