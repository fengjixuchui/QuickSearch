#include "FileNameMemoryManager.h"
#include "Lock.h"
FileNameMemoryManager *g_pMemoryManager = new FileNameMemoryManager();
CLock memory_lock;
//enum _tagEnumMinMBlock
//{
//    MBlock_Default = 0,
//    MBlock_2Byte = 1,
//    MBlock_4Byte = 2,
//    MBlock_6Byte = 3,
//    MBlock_8Byte = 4,
//    MBlock_10Byte = 5,
//    MBlock_12Byte = 6,
//    MBlock_14Byte = 7,
//    MBlock_16Byte = 8,
//    MBlock_18Byte = 9,
//    MBlock_20Byte = 10,
//    MBlock_22Byte = 11,
//    MBlock_24Byte = 12,
//    MBlock_26Byte = 13,
//    MBlock_28Byte = 14,
//    MBlock_30Byte = 15,
//    MBlock_32Byte = 16,
//    MBlock_48Byte = 17,
//    MBlock_64Byte = 18,
//    MBlock_80Byte = 19,
//    MBlock_128Byte = 20,
//    MBlock_256Byte = 21,
//    MBlock_512Byte = 22,
//    MBlock_1024Byte = 23, //最大记录，一般文件不会超过这个值。
//
//};

enum _tagEnumMinMBlock
{
    MBlock_Default = 0,
    MBlock_8Byte = 1,
    MBlock_16Byte = 2,
    MBlock_24Byte = 3,
    MBlock_32Byte = 4,
    MBlock_48Byte = 5,
    MBlock_64Byte = 6,
    MBlock_80Byte = 7,
    MBlock_128Byte = 8,
    MBlock_256Byte = 9,
    MBlock_512Byte = 10,
    MBlock_1024Byte = 11, //最大记录，一般文件不会超过这个值。
};

FileNameMemoryManager::FileNameMemoryManager()
{
    m_dwFreeidleCount = 0;
    m_dwUseidleCount = 0;
    m_dwFileRecordCount = 0;
    m_dwTotalMBolcksCount = 0;
    m_dwWasteMemoryCount = 0;

    for (int volIndex = 0; volIndex < VOLUME_COUNT; volIndex++)
    {
        m_pMemoryBlockHead[volIndex] = NULL;
        m_pMemoryBlockNow[volIndex] = NULL;
        m_dwVolMBolcksCount[volIndex] = 0;
    }
}


FileNameMemoryManager::~FileNameMemoryManager()
{
    //FreeAllFileMemory();
    for (int volIndex = 0; volIndex < VOLUME_COUNT; volIndex++)
    {
        m_idleRecordMultimap[volIndex].clear();
    }
}

PFileNameEntry FileNameMemoryManager::GetNewFileRecord(int recordLength, int volIndex)
{
    memory_lock.Lock();
    _tagEnumMinMBlock MinMBlockType = (_tagEnumMinMBlock)GetMinMBlockType(recordLength);
    PFileNameEntry idleFileEntry = GetIdleFileRecord(MinMBlockType, volIndex);
    if (idleFileEntry)
    {
        memory_lock.UnLock();
        return idleFileEntry;
    }

    if (m_pMemoryBlockNow[volIndex] == NULL)
    {
        m_pMemoryBlockNow[volIndex] = NewMemoryBolck(volIndex);
        if (m_pMemoryBlockNow[volIndex] == NULL)
        {
            memory_lock.UnLock();
            LOG(INFO) << __FUNCTIONW__ << " Allocation Error";
            return NULL;
        }
        if (m_pMemoryBlockHead[volIndex] == NULL)
        {
            m_pMemoryBlockHead[volIndex] = m_pMemoryBlockNow[volIndex];
        }
    }

    DWORD dwMBlockLength = GetMinMBlockSize(MinMBlockType);
    if (ALLOC_SIZE < (m_pMemoryBlockNow[volIndex]->m_used + dwMBlockLength))
    {
        MemoryBlock *pMemoryBolck = NewMemoryBolck(volIndex);
        if (pMemoryBolck == NULL)
        {
            memory_lock.UnLock();
            LOG(INFO) << __FUNCTIONW__ << " Allocation Error";
            return NULL;
        }
        m_pMemoryBlockNow[volIndex]->m_pNext = pMemoryBolck;
        m_pMemoryBlockNow[volIndex] = pMemoryBolck;
    }

    PFileNameEntry pFileEntry = (PFileNameEntry)((PCHAR)m_pMemoryBlockNow[volIndex] + m_pMemoryBlockNow[volIndex]->m_used);
    m_pMemoryBlockNow[volIndex]->m_used += dwMBlockLength;
    m_pMemoryBlockNow[volIndex]->m_fileRecordCount += 1;
    m_dwFileRecordCount++;
    memory_lock.UnLock();
    *(((char*)pFileEntry) + recordLength) = 0;
    *(((char*)pFileEntry) + recordLength-1)= 0;
    return pFileEntry;
}

void FileNameMemoryManager::FreeFileEntry(PFileNameEntry pFileRecord, int volIndex)
{
    DWORD dwMBlockType = pFileRecord->FileNameLength+FileNameEntryHeaderSize;
    memory_lock.Lock();
    m_idleRecordMultimap[volIndex].insert(std::make_pair(dwMBlockType, pFileRecord));
    memory_lock.UnLock();
    m_dwFreeidleCount++;

    if (m_dwFreeidleCount % 500 == 0)
    {
        LOG(INFO) << __FUNCTIONW__ << " m_dwFreeidleCount:" << m_dwFreeidleCount << " m_dwUseidleCount:" << \
            m_dwUseidleCount << " m_dwFileRecordCount:" << m_dwFileRecordCount;
    }
}

PFileNameEntry FileNameMemoryManager::GetIdleFileRecord(DWORD dwMinBlockType, int volIndex)
{
    PFileNameEntry idleFileEntry = NULL;
    IdleRecordMultimap::iterator iter = m_idleRecordMultimap[volIndex].find(dwMinBlockType);
    if (iter != m_idleRecordMultimap[volIndex].end())
    {
        idleFileEntry = iter->second;
        m_idleRecordMultimap[volIndex].erase(iter);
        m_dwUseidleCount++;
    }
    return idleFileEntry;
}

DWORD FileNameMemoryManager::GetMinMBlockType(DWORD FileNameLength)
{
    if (FileNameLength > GetMinMBlockSize(MBlock_512Byte)) return (DWORD)MBlock_1024Byte;
    else if (FileNameLength > GetMinMBlockSize(MBlock_256Byte)) return (DWORD)MBlock_512Byte;
    else if (FileNameLength > GetMinMBlockSize(MBlock_128Byte)) return (DWORD)MBlock_256Byte;
    else if (FileNameLength > GetMinMBlockSize(MBlock_80Byte)) return (DWORD)MBlock_128Byte;
    else if (FileNameLength > GetMinMBlockSize(MBlock_64Byte)) return (DWORD)MBlock_80Byte;
    else if (FileNameLength > GetMinMBlockSize(MBlock_48Byte)) return (DWORD)MBlock_64Byte;
    else if (FileNameLength > GetMinMBlockSize(MBlock_32Byte)) return (DWORD)MBlock_48Byte;
    else if (FileNameLength > GetMinMBlockSize(MBlock_24Byte)) return (DWORD)MBlock_32Byte;
    else if (FileNameLength > GetMinMBlockSize(MBlock_16Byte)) return (DWORD)MBlock_24Byte;
    else if (FileNameLength > GetMinMBlockSize(MBlock_8Byte)) return (DWORD)MBlock_16Byte;
    else if (FileNameLength > GetMinMBlockSize(MBlock_Default)) return (DWORD)MBlock_8Byte;
    return MBlock_Default;
}

DWORD FileNameMemoryManager::GetMinMBlockSize(DWORD dwMinMBlock)
{
    switch (dwMinMBlock)
    {
    case MBlock_8Byte: return 8;
    case MBlock_16Byte: return 16;
    case MBlock_24Byte: return 24;
    case MBlock_32Byte: return 32;
    case MBlock_48Byte: return 48;
    case MBlock_64Byte: return 64;
    case MBlock_80Byte: return 80;
    case MBlock_128Byte: return 128;
    case MBlock_256Byte: return 256;
    case MBlock_512Byte: return 512;
    case MBlock_1024Byte: return 1024;
    }
    return 0;
}

MemoryBlock* FileNameMemoryManager::NewMemoryBolck(int volIndex)
{
    MemoryBlock *pMemoryBlock = (MemoryBlock *)calloc(1, ALLOC_SIZE);
    memset(pMemoryBlock, 0, ALLOC_SIZE);
    if (pMemoryBlock != NULL)
    {
        pMemoryBlock->m_used = sizeof(MemoryBlock);
    }
    else
    {
        //LOG_INFO(GID_DESKTOP, _T("%s Allocation when calloc MemoryBolck error"), __FUNCTIONW__);
        return NULL;
    }
    m_dwTotalMBolcksCount += 1;
    m_dwVolMBolcksCount[volIndex] += 1;
    return pMemoryBlock;
}

DWORD FileNameMemoryManager::FreeAllFileMemory()
{
    DWORD dwFreeBlockCnt = 0;
    for (int i = 0; i < VOLUME_COUNT; ++i)
    {
        if (m_pMemoryBlockHead[i] != NULL)
        {
            MemoryBlock *pMemoryBlockHead = m_pMemoryBlockHead[i];
            while (pMemoryBlockHead != NULL)
            {
                MemoryBlock *pTemp = pMemoryBlockHead;
                pMemoryBlockHead = pMemoryBlockHead->m_pNext;
                dwFreeBlockCnt++;
                free(pTemp);
            }
            m_pMemoryBlockHead[i] = NULL;
            m_pMemoryBlockNow[i] = NULL;
        }
    }
    return dwFreeBlockCnt;
}
