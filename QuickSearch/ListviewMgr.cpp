#include "ListviewMgr.h"
#include "SearchMgr.h"
#include <Strsafe.h>
#include "FileHandler.h"
#include "Util.h"
#include "resource.h"
#include "ShellContextMenu.h"
CListviewMgr::CListviewMgr()
{

}


CListviewMgr::~CListviewMgr()
{
}

void CListviewMgr::Init(HWND hwnd,HWND mainWindow)
{
    this->m_hListview = hwnd;
    this->m_hMainWindow = mainWindow;
    m_iconList = ImageList_Create(20, 20, ILC_COLOR32, 1, 1);
    ListView_SetImageList(m_hListview, m_iconList, LVSIL_SMALL);
    m_ResultTypeMask = 0;
    bSortAscending = FALSE;
    nSortColumn = 3;
    nRClickItem = 0;
}

void CListviewMgr::UnInit()
{
    for (auto tmp:VeciconIndex)
    {
        DestroyIcon(tmp);
    }
}

void CListviewMgr::RetrieveItem(SearchResultItem& pENtry, int index)
{
    int nTrueIndex = 0;
    nTrueIndex = m_vecValidIndex[index];
    pENtry = g_SearchMgr->m_vecResult[nTrueIndex];
}

LRESULT CListviewMgr::OnNotify(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr;
    LRESULT lrt = FALSE;
    
    NMHDR* pnmhdr = reinterpret_cast<NMHDR*>(lParam);
    switch (pnmhdr->code)
    {
    case LVN_GETDISPINFO:
    {
        SearchResultItem rndItem;
        NMLVDISPINFO* plvdi = (NMLVDISPINFO*)pnmhdr;
        if (plvdi->item.iItem < 0 || // typo fixed 11am
            plvdi->item.iItem >= m_ResultCnt) {
            return lrt;         // requesting invalid item
        }

        // Retrieve information for item at index iItem.
        RetrieveItem(rndItem, plvdi->item.iItem);

        if (plvdi->item.mask & LVIF_TEXT)
        {
            // Fill in the text information.
            switch (plvdi->item.iSubItem)
            {
            case 0:
                // Copy the main item text.
                hr = StringCchCopy(plvdi->item.pszText, 255, (LPWSTR)rndItem.filename.c_str());
                if (plvdi->item.mask & LVIF_IMAGE)
                {
                    // Fill in the image information.
                    std::wstring strFileType;
                    if (rndItem.fileType == TypeFolders)
                    {
                        strFileType = L"/.FOLDERS";
                    }
                    else
                    {
                        FileHandler::SplitFileType(rndItem.filename, strFileType);
                    }
                    auto itr = filetype_iconIndex_Map.find(strFileType);
                    int iIcon = -1;
                    if (itr != filetype_iconIndex_Map.end())
                    {
                        iIcon = itr->second;
                    }
                    plvdi->item.iImage = iIcon;
                }
                if (FAILED(hr))
                {
                    // Insert error handling code here. MAX_COUNT
                    // is a user-defined value. You must not enter                                
                    // more characters than specified by MAX_COUNT or  
                    // the text will be truncated.
                }
                break;

            case 1:
                // Copy SubItem1 text.
                hr = StringCchCopy(plvdi->item.pszText, 255, (LPWSTR)rndItem.path.c_str());

                if (FAILED(hr))
                {
                    // Insert error handling code here. MAX_COUNT
                    // is a user-defined value. You must not enter               
                    // more characters than specified by MAX_COUNT or   
                    // the text will be truncated..
                }
                break;

            case 2:
                {
                    // Copy SubItem2 text.
                    std::wstring sizeTxT;
                    sizeTxT = GetInstallSize(rndItem.fileSize);
                    hr = StringCchCopy(plvdi->item.pszText, 20, (LPWSTR)sizeTxT.c_str());
                }
                
                break;
            case 3:
                {
                    wchar_t time[20];
                    tm Mfdata;
                    __time32_t timeT = rndItem.modifiedTime;
                    _localtime32_s(&Mfdata, &timeT);
                    _stprintf_s(time, L"%d/%d/%d %02d:%02d", Mfdata.tm_year + 1900, \
                        Mfdata.tm_mon + 1, Mfdata.tm_mday, Mfdata.tm_hour, Mfdata.tm_min);

                    hr = StringCchCopy(plvdi->item.pszText, 20, (LPWSTR)time);
                }
                break;
            default:
                break;

            }

        }

        lrt = FALSE;
        break;
    }

    case LVN_ODCACHEHINT:
    {
        NMLVCACHEHINT* pcachehint = (NMLVCACHEHINT*)pnmhdr;

        // Load the cache with the recommended range.
        //PrepCache(pcachehint->iFrom, pcachehint->iTo);
        break;
    }

    case LVN_ODFINDITEM:
    {
        LPNMLVFINDITEM pnmfi = NULL;

        pnmfi = (LPNMLVFINDITEM)pnmhdr;
        
        // Call a user-defined function that finds the index according to
        // LVFINDINFO (which is embedded in the LPNMLVFINDITEM structure).
        // If nothing is found, then set the return value to -1.

        break;
    }
    case NM_DBLCLK:
    {
        NMITEMACTIVATE* plvdi = (LPNMITEMACTIVATE)pnmhdr;
        int index = 0;
        index = plvdi->iItem;
        if (plvdi->iSubItem == 0)
        {

            int nTrueIndex = m_vecValidIndex[index];
            std::wstring path = g_SearchMgr->m_vecResult[nTrueIndex].path;
            //ShellExecute(NULL, L"properties", (LPWSTR)path.c_str(), NULL, NULL, SW_SHOWNORMAL);

            SHELLEXECUTEINFO   sei;
            sei.hwnd = m_hMainWindow;
            sei.lpVerb = TEXT("open");
            sei.lpFile = (LPWSTR)path.c_str();
            sei.lpDirectory = NULL;
            sei.lpParameters = NULL;
            sei.nShow = SW_SHOWNORMAL;
            sei.fMask = SEE_MASK_INVOKEIDLIST;
            sei.lpIDList = NULL;
            sei.cbSize = sizeof(SHELLEXECUTEINFO);
            ShellExecuteEx(&sei);
        }
    }
    break;
    case NM_RCLICK:
    {
        NMITEMACTIVATE* plvdi = (LPNMITEMACTIVATE)pnmhdr;
        //NMHDR* pnmhdr = reinterpret_cast<NMHDR*>(lParam);
        //pnmhdr->code = NM_SETFOCUS;
        //SendMessage(m_hMainWindow, WM_NOTIFY, 0, LPARAM(pnmhdr));
        if (plvdi->iSubItem == 0)
        {
            int index = 0;
            index = plvdi->iItem;
            nRClickItem = m_vecValidIndex[index];
            std::wstring path = g_SearchMgr->m_vecResult[nRClickItem].path;
            POINT pt = { 0 };
            GetCursorPos(&pt);
            if (path.length() == 0)
            {
                return lrt;
            }
            CShellContextMenu scm;
            scm.SetObjects(path);
            DWORD idCommand = scm.ShowContextMenu(m_hMainWindow, pt);
        }
    }
        break;
    case LVN_COLUMNCLICK:
    {
        OnColumnClick((LPNMLISTVIEW)pnmhdr);
    }
    default:
        break;
    }       // End Switch block.
    return(lrt);
}

void CListviewMgr::PrepCache(int iFrom, int iTo)
{
}

HICON CListviewMgr::fileIcon(std::wstring extention)
{
    CoInitialize(NULL);
    HICON icon = NULL;
    //icon = ExtractIcon(g_hInstance, (LPCWSTR)extention.c_str(), -1);
    if (extention.length() > 0)
    {
        LPCWSTR path = extention.c_str();
        SHFILEINFOW info;
        DWORD dwFileAttributes = GetFileAttributes(path);
        HIMAGELIST imageList = (HIMAGELIST)SHGetFileInfoW(path,
            dwFileAttributes,
            &info,
            sizeof(info),
            SHGFI_SYSICONINDEX | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES);
        if (imageList)
        {
            UINT uImageFlags = ILD_IMAGE;

            icon = ImageList_GetIcon(imageList, info.iIcon, uImageFlags);
        }
    }
    CoUninitialize();
    return icon;
}

void CListviewMgr::updateIcon()
{
    for (auto& tmp : g_SearchMgr->m_vecResult)
    {
        std::wstring strFileType;
        //如果是目录，不再进行后缀名截取
        if (tmp.fileType == TypeFolders)
        {
            strFileType = L"/.FOLDERS";
        }
        else
        {
            FileHandler::SplitFileType(tmp.filename, strFileType);
        }
        auto itr = filetype_iconIndex_Map.find(strFileType);
        if (itr == filetype_iconIndex_Map.end())
        {
            HICON icon = fileIcon(tmp.path);
            VeciconIndex.push_back(icon);
            ImageList_AddIcon(m_iconList, icon);
            filetype_iconIndex_Map.insert(std::make_pair(strFileType, VeciconIndex.size() - 1));
        }
    }
    ListView_SetImageList(m_hListview, m_iconList, LVSIL_SMALL);
}

void CListviewMgr::UpdateValidResultCnt()
{
    m_vecValidIndex.clear();
    m_ResultCnt = 0;
    for (int i = 0;i< g_SearchMgr->m_vecResult.size();++i)
    {
        if (m_ResultTypeMask == 0)
        {
            m_ResultCnt = g_SearchMgr->m_vecResult.size();
            m_vecValidIndex.push_back(i);
        }
        else
        {
            if (g_SearchMgr->m_vecResult[i].fileType & m_ResultTypeMask)
            {
                ++m_ResultCnt;
                m_vecValidIndex.push_back(i);
            }
        }
    }
}

void CListviewMgr::setListViewSortIcon()
{
    HWND headerWnd;
    const int bufLen = 256;
    wchar_t headerText[bufLen];
    HD_ITEM item;
    int numColumns, curCol;

    headerWnd = ListView_GetHeader(m_hListview);
    numColumns = Header_GetItemCount(headerWnd);

    for (curCol = 0; curCol < numColumns; curCol++)
    {
        item.mask = HDI_FORMAT | HDI_TEXT;
        item.pszText = headerText;
        item.cchTextMax = bufLen - 1;
        SendMessage(headerWnd, HDM_GETITEM, curCol, (LPARAM)&item);

        if (curCol == nSortColumn)
            switch (bSortAscending)
            {
            case TRUE:
                item.fmt &= ~HDF_SORTDOWN;
                item.fmt |= HDF_SORTUP;
                break;
            case FALSE:
                item.fmt &= ~HDF_SORTUP;
                item.fmt |= HDF_SORTDOWN;
                break;
            }
        else
        {
            item.fmt &= ~HDF_SORTUP & ~HDF_SORTDOWN;
        }
        item.fmt |= HDF_STRING;
        item.mask = HDI_FORMAT | HDI_TEXT;
        SendMessage(headerWnd, HDM_SETITEM, curCol, (LPARAM)&item);
    }
}

void CListviewMgr::OnColumnClick(LPNMLISTVIEW pLVInfo)
{
    m_ResultCnt = 0;
    if (pLVInfo->iSubItem == 1)
        return;
    // get new sort parameters
    if (pLVInfo->iSubItem == nSortColumn)
    {
        bSortAscending = !bSortAscending;
    }
    else
    {
        nSortColumn = pLVInfo->iSubItem;
        bSortAscending = FALSE;
    }

    // sort list
    //ListView_SortItems(pLVInfo->hdr.hwndFrom, myCompFunc, lParamSort);
    setListViewSortIcon();
    SendMessage(m_hMainWindow, MSG_SORT_CHANGE, nSortColumn, bSortAscending);
}
