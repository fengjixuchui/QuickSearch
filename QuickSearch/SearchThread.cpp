#include "SearchThread.h"
#include "IndexManager.h"
#include "ntfsUtils.h"
#include "boost/regex.hpp"
CSearchThread::CSearchThread()
{

}


CSearchThread::~CSearchThread()
{
}

BOOL CSearchThread::Init(DWORD dwIndex)
{
    dwVolIndex = dwIndex;
    m_hQuitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hPauseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    this->Start();
    return TRUE;
}

void CSearchThread::Uninit()
{
    this->Stop();
    CloseHandle(m_hPauseEvent);
    CloseHandle(m_hQuitEvent);
    m_vecResult.clear();
    std::vector<FileEntry> nullVec;
    m_vecResult.swap(nullVec);
}

void CSearchThread::DoSearch(SearchOpt opt)
{
    m_SearchOpt = opt;
    ::SetEvent(m_hPauseEvent);
    //this->Start();
}

void CSearchThread::Wait()
{
    if (m_hQuitEvent)
    {
        ::WaitForSingleObject(m_hQuitEvent, INFINITE);
    }
}

void CSearchThread::ThreadFunc()
{
    while (this->IsExist())
    {
        if (!this->IsExist())
        {
            break;
        }
        ::WaitForSingleObject(m_hPauseEvent, INFINITE);
        DWORD dwBegin = ::GetTickCount();

        //search function
        Search();

        DWORD dwUseTime = ::GetTickCount() - dwBegin;
        LOG(INFO)<<__FUNCTIONW__<<" vol:"<<(char)(dwVolIndex + 'A')<<" dwUseTime:"<<dwUseTime<<" count:"<<m_vecResult.size();
        ::SetEvent(m_hQuitEvent);
    }
}

void CSearchThread::Search()
{
    m_vecResult.clear();
    std::vector<FileEntry> nullVec;
    m_vecResult.swap(nullVec);
    m_vecResult.reserve(50);

    std::string strSearch(m_SearchOpt.name.c_str());
    int slength = strSearch.length() + 1;
    int len = MultiByteToWideChar(CP_UTF8, 0, strSearch.c_str(), slength, 0, 0);

    wchar_t* wchr = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)strSearch.c_str(), slength, (LPWSTR)wchr, len);
    std::wstring wstrSearch(wchr);
    std::wstring wstrSearchLowcase = strToLower(wstrSearch);
    delete[] wchr;
    switch (m_SearchOpt.sortType)
    {
    case Sort_By_Name:
    {
        auto& index = g_pIndexManager->m_VolFileIndex[dwVolIndex].get<1>();
        if (m_SearchOpt.bAscending == TRUE)
        {
            auto itr = index.begin();
            for (; itr != index.end(); ++itr)
            {
                Match(*itr, wstrSearchLowcase);
                if (m_vecResult.size() == RESULT_LIMIT)
                    break;
            }
        }
        else
        {
            auto itr = index.rbegin();
            for (; itr != index.rend(); ++itr)
            {
                Match(*itr, wstrSearchLowcase);
                if (m_vecResult.size() == RESULT_LIMIT)
                    break;
            }
        }  
    }
        break;
    case Sort_By_FileSize:
    {
        auto& index = g_pIndexManager->m_VolFileIndex[dwVolIndex].get<2>();
        if (m_SearchOpt.bAscending == TRUE)
        {
            auto itr = index.begin();
            for (; itr != index.end(); ++itr)
            {
                Match(*itr, wstrSearchLowcase);
                if (m_vecResult.size() == RESULT_LIMIT)
                    break;
            }
        }
        else
        {
            auto itr = index.rbegin();
            for (; itr != index.rend(); ++itr)
            {
                Match(*itr, wstrSearchLowcase);
                if (m_vecResult.size() == RESULT_LIMIT)
                    break;
            }
        }
    }
        break;
    case Sort_By_MfTime:
    {
        auto& index = g_pIndexManager->m_VolFileIndex[dwVolIndex].get<3>();
        if (m_SearchOpt.bAscending == TRUE)
        {
            auto itr = index.begin();
            for (; itr != index.end(); ++itr)
            {
                Match(*itr, wstrSearchLowcase);
                if (m_vecResult.size() == RESULT_LIMIT)
                    break;
            }
        }
        else
        {
            auto itr = index.rbegin();
            for (; itr != index.rend(); ++itr)
            {
                Match(*itr, wstrSearchLowcase);
                if (m_vecResult.size() == RESULT_LIMIT)
                    break;
            }
        }
    }
        break;
    default:
        break;
    }
}

void CSearchThread::Match(FileEntry fileEntry,std::wstring& wstrSearch)
{
    std::string str(fileEntry.pFileName->FileName);
    //int slength = str.length() + 1;
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, 0, 0);
    wchar_t* wchr = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)str.c_str(), -1, (LPWSTR)wchr, len);
    std::wstring wstrName(wchr);
    std::wstring wstrNameLowcase = strToLower(wstrName);
    delete[] wchr;

    if (m_SearchOpt.bUseRegex)
    {        
        boost::wregex reg(wstrSearch);
        if (boost::regex_match(wstrNameLowcase, reg))
        {
            m_vecResult.push_back(fileEntry);
        }
    }
    else
    {
        if (SUBSTR(wstrNameLowcase, wstrSearch))
        {
            m_vecResult.push_back(fileEntry);
        }
    }
}
