#include "IndexManager.h"
#include "Lock.h"
#include <Shlwapi.h>
#include "FileNameMemoryManager.h"

IndexManager *g_pIndexManager = new IndexManager();

IndexManager::IndexManager()
{

}

IndexManager::~IndexManager()
{
}
BOOL IndexManager::AddEntry(FileEntry fileEntry,int nVolIndex)
{
    //if (isFindEntry(fileEntry.FileReferenceNumber, nVolIndex) == TRUE)
    //{
    DeleteEntry(fileEntry.FileReferenceNumber, nVolIndex);
    //}
    m_VolFileIndex[nVolIndex].insert(fileEntry);
    return TRUE;
}
BOOL IndexManager::DeleteEntry(KEY FRN, int nVolIndex)
{
    FileEntry_By_FRN& index = m_VolFileIndex[nVolIndex].get<0>();
    FileEntry_By_FRN::iterator itr = index.find(FRN);
    if (itr != index.end())
    {
        FileEntry tmpEntry = *itr;
        g_pMemoryManager->FreeFileEntry(tmpEntry.pFileName, nVolIndex);
        index.erase(itr);
    }
    
    return 0;
}

DWORD IndexManager::GetVolFileCnt(int nVolIndex)
{
    return m_VolFileIndex[nVolIndex].size();
}

BOOL IndexManager::isFindEntry(KEY FRN, int nVolIndex)
{
    FileEntry_By_FRN& fileSet = m_VolFileIndex[nVolIndex].get<0>();
    FileEntry_By_FRN::iterator itr = m_VolFileIndex[nVolIndex].find(FRN);
    if (fileSet.find(FRN) != fileSet.end())
    {
        return TRUE;
    }
    return FALSE;
}

//FileEntry_Set & IndexManager::GetVolIndex(int nVolIndex)
//{
//    return m_VolFileIndex[nVolIndex];
//}

void IndexManager::Save(std::wstring& strDbFilePath, int nVolIndex)
{
    const FileEntry_Set& fs = m_VolFileIndex[nVolIndex];
    std::ofstream ofs(strDbFilePath);
    boost::archive::text_oarchive oa(ofs);
    oa << fs;
}

void IndexManager::Load(std::wstring& strDbFilePath, int nVolIndex)
{
    FileEntry_Set& fs = m_VolFileIndex[nVolIndex];
    std::ifstream ifs(strDbFilePath);
    boost::archive::text_iarchive ia(ifs);
    ia >> fs;
}

void IndexManager::UnInit()
{
    for (int i = 0; i < VOLUME_COUNT; ++i)
    {
        m_VolFileIndex[i].clear();
    }
}
