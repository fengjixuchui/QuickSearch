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
    int m_fileRecordCount; // �ļ���¼�ĸ���
    int m_used;  // ��ǰ�Ѿ��õ����ֽ���
    MemoryBlock *m_pNext; // ָ����һ���ڴ���ָ��
};
class FileNameMemoryManager
{
public:
    FileNameMemoryManager();
    ~FileNameMemoryManager();
    
    char* GetNewFileRecord(int FileNameLength, int volIndex); // ��ȡ��Ч���ڴ�����λ��
    void FreeFileEntry(FileEntry& FileRecord, int volIndex);
    DWORD FileNameMemoryManager::FreeAllFileMemory();
private:
    char* GetIdleFileRecord(DWORD dwMinBlockType, int volIndex);
    DWORD GetMinMBlockType(DWORD FileNameLength);
    DWORD GetMinMBlockSize(DWORD dwMinMBlock);
    MemoryBlock *NewMemoryBolck(int volIndex);  // �����µ��ڴ��
    
public:
    MemoryBlock *m_pMemoryBlockHead[DIRVE_COUNT]; // �ڴ��ͷָ��
    MemoryBlock *m_pMemoryBlockNow[DIRVE_COUNT]; // �ڴ��βָ��
    IdleRecordMultimap m_idleRecordMultimap[DIRVE_COUNT];
private:
    DWORD m_dwFreeidleCount;        //���ռ�¼��
    DWORD m_dwUseidleCount;         //�ظ�������
    DWORD m_dwFileRecordCount;      //���´��ڴ���з����¼��
    DWORD m_dwTotalMBolcksCount;    //�ڴ����
    DWORD m_dwVolMBolcksCount[DIRVE_COUNT];    //�ڴ����
    DWORD m_dwWasteMemoryCount;     //��ʧ����Ч�ڴ�
};
extern FileNameMemoryManager *g_pMemoryManager;

