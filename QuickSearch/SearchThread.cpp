#include "SearchThread.h"
#include "IndexManager.h"
#include "ntfsUtils.h"

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
    std::wstring wstrSearch(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)strSearch.c_str(), slength, (LPWSTR)wstrSearch.c_str(), len);
    switch (m_SearchOpt.sortType)
    {
    case Srot_By_Name:
    {
        auto& index = g_pIndexManager->m_VolFileIndex[dwVolIndex].get<1>();
        auto itr = index.begin();
        for (;itr != index.end();++itr)
        {
            std::string str(itr->pFileName->FileName);
            slength = str.length() + 1;
            len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), slength, 0, 0);
            std::wstring wstrName(len, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)str.c_str(), slength, (LPWSTR)wstrName.c_str(), len);

            if (SUBSTR(wstrName, wstrSearch))
            {
                m_vecResult.push_back(*itr);
                if (m_vecResult.size() >= RESULT_LIMIT)
                    break;
            }
        }
    }
        
        break;
    case Sort_By_FileSize:
    {
        auto& index = g_pIndexManager->m_VolFileIndex[dwVolIndex].get<2>();
        auto itr = index.begin();
        for (; itr != index.end(); ++itr)
        {
            std::string str(itr->pFileName->FileName);
            slength = str.length() + 1;
            len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), slength, 0, 0);
            std::wstring wstrName(len, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)str.c_str(), slength, (LPWSTR)wstrName.c_str(), len);

            if (SUBSTR(wstrName, wstrSearch))
            {
                m_vecResult.push_back(*itr);
                if (m_vecResult.size() >= RESULT_LIMIT)
                    break;
            }
        }
    }
        
        break;
    case Sort_By_MfTime:
    {
        auto& index = g_pIndexManager->m_VolFileIndex[dwVolIndex].get<3>();
        auto itr = index.begin();
        for (; itr != index.end(); ++itr)
        {
            std::string str(itr->pFileName->FileName);
            slength = str.length() + 1;
            len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), slength, 0, 0);
            std::wstring wstrName(len, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)str.c_str(), slength, (LPWSTR)wstrName.c_str(), len);

            if (SUBSTR(wstrName, wstrSearch))
            {
                m_vecResult.push_back(*itr);
                if (m_vecResult.size() >= RESULT_LIMIT)
                    break;
            }
        }
    }
        
        break;
    default:
        break;
    }
    


}
