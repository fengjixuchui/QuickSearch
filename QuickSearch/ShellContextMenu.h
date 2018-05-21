// ShellContextMenu.h: Schnittstelle für die Klasse CShellContextMenu.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SHELLCONTEXTMENU_H__A358AACF_7C7C_410D_AD29_67310B2DDC22__INCLUDED_)
#define AFX_SHELLCONTEXTMENU_H__A358AACF_7C7C_410D_AD29_67310B2DDC22__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <string>
#include <vector>
#include <Shlobj.h>
#include <Shobjidl.h>
/////////////////////////////////////////////////////////////////////
// class to show shell contextmenu of files/folders/shell objects
// developed by R. Engels 2003
/////////////////////////////////////////////////////////////////////

class CShellContextMenu  
{
public:
	HMENU  GetMenu ();
	void SetObjects (IShellFolder * psfFolder, LPITEMIDLIST pidlItem);
	void SetObjects (IShellFolder * psfFolder, LPITEMIDLIST * pidlArray, int nItemCount);
	void SetObjects (LPITEMIDLIST pidl);
	void SetObjects (std::wstring strObject);
	void SetObjects (const std::vector<std::wstring>& strArray);
	UINT ShowContextMenu (HWND hWnd, POINT pt);
	CShellContextMenu();
	virtual ~CShellContextMenu();
	VOID OnCopyOrCutFiles(HWND hWnd, BOOL bCopy, const std::vector<std::wstring>& strFiles);

private:
	int nItems;
	BOOL bDelete;
	HMENU m_Menu;
	IShellFolder * m_psfFolder;
	LPITEMIDLIST * m_pidlArray;	
	
	HRESULT InvokeCommand (LPCONTEXTMENU pContextMenu, UINT idCommand);
	BOOL GetContextMenu (void ** ppContextMenu, int & iMenuType);
	HRESULT SHBindToParentEx (LPCITEMIDLIST pidl, REFIID riid, VOID **ppv, LPCITEMIDLIST *ppidlLast);
	static LRESULT CALLBACK HookWndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void FreePIDLArray (LPITEMIDLIST * pidlArray);
	LPITEMIDLIST CopyPIDL (LPCITEMIDLIST pidl, int cb = -1);
	UINT GetPIDLSize (LPCITEMIDLIST pidl);
	LPBYTE GetPIDLPos (LPCITEMIDLIST pidl, int nPos);
	int GetPIDLCount (LPCITEMIDLIST pidl);
	BOOL InsertNewMenuItem(HMENU hMenu, DWORD dwMenuID, const TCHAR* pszItemName, int nMenuIndex, BOOL bAddSep, int nIconIndex);
	void InsertCustomMenu(HMENU hMenu);
	void OnOpenFolder(const std::wstring& strPath);
	void OnCopyPath(HWND hWnd, const std::wstring& strPath);
	std::vector<std::wstring> strArray;
};

#endif // !defined(AFX_SHELLCONTEXTMENU_H__A358AACF_7C7C_410D_AD29_67310B2DDC22__INCLUDED_)
