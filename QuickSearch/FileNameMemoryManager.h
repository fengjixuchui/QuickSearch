#pragma once
#include <map>
#include <windows.h>
#include "global.h"
//#include <google/sparse_hash_map>
typedef std::multimap<DWORD, PCHAR> IdleRecordMultimap;
//typedef google::sparse_hash_map<int, PCHAR> fileNameMap;

#define ALLOC_SIZE 65536
struct MemoryBlock
{
    int m_fileRecordCount; // 文件记录的个数
    int m_used;  // 当前已经用掉的字节数
    MemoryBlock *m_pNext; // 指向下一块内存块的指针
};
class FileNameMemoryManager
{
public:
    FileNameMemoryManager();
    ~FileNameMemoryManager();
    
    char* GetNewFileRecord(int FileNameLength, int volIndex); // 获取有效的内存块插入位置
    void FreeFileEntry(FileEntry& FileRecord, int volIndex);
    DWORD FileNameMemoryManager::FreeAllFileMemory();
private:
    char* GetIdleFileRecord(DWORD dwMinBlockType, int volIndex);
    DWORD GetMinMBlockType(DWORD FileNameLength);
    DWORD GetMinMBlockSize(DWORD dwMinMBlock);
    MemoryBlock *NewMemoryBolck(int volIndex);  // 创建新的内存块
    
public:
    MemoryBlock *m_pMemoryBlockHead[DIRVE_COUNT]; // 内存块头指针
    MemoryBlock *m_pMemoryBlockNow[DIRVE_COUNT]; // 内存块尾指针
    IdleRecordMultimap m_idleRecordMultimap[DIRVE_COUNT];
private:
    DWORD m_dwFreeidleCount;        //回收记录数
    DWORD m_dwUseidleCount;         //重复利用数
    DWORD m_dwFileRecordCount;      //重新从内存块中分配记录数
    DWORD m_dwTotalMBolcksCount;    //内存块数
    DWORD m_dwVolMBolcksCount[DIRVE_COUNT];    //内存块数
    DWORD m_dwWasteMemoryCount;     //丢失的无效内存
};
extern FileNameMemoryManager *g_pMemoryManager;

