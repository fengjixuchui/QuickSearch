#pragma once
#include "easylogging++.h"
#define DIRVE_COUNT 26 
extern volatile BOOL g_bQuitSearCore;

#define KEY DWORD
#define KEY_MASK 0xFFFFFFFF                             //����

#define MAX_FILE_NAME_LENGTH 1000
#define DW_SEARCHDB_VERSION  1000         // ���ݿ�汾�ų���
#define DW_SEARCHDB_HEADER_LENGTH  1024   //1kb DB�ļ�ͷ
#define FILEENTRY_DB_LENGTH 1024          //1kb
typedef char UTF8,*PUTF8;
#pragma pack(push,1)

#define FILEENTRY_HEADER_SIZE (int)sizeof(FileEntry) - 1
struct FileEntry
{
    KEY FileReferenceNumber;
    KEY ParentFileReferenceNumber;
    //FileEntry* pParent;
    DWORD dwFileSize;      //�ļ���С
    DWORD dwMftTime;       //�ļ��޸�ʱ��
    struct
    {
        unsigned char dir : 1;    //�Ƿ���Ŀ¼
        unsigned char parentroot : 1;    //��Ŀ¼�Ƿ��Ǹ�Ŀ¼
        unsigned char chname : 1;	//�Ƿ��������ַ�
        unsigned char volIndex : 5;    //�����̷�
        unsigned short FileNameLength;    //�ļ�������
    }fileInfo;
    PCHAR  FileName;                      //�ļ�����������\0
    DWORD GetPFRN()
    {
        return ParentFileReferenceNumber;
    }
    FileEntry()
    {
        FileReferenceNumber = KEY_MASK;
    }
    bool operator < (const FileEntry& tmp) const
    {
        return this->FileName < tmp.FileName;
    }
    bool operator > (const FileEntry& tmp) const
    {
        return this->FileName > tmp.FileName;
    }
    bool operator == (const FileEntry& tmp) const
    {
        return this->FileName == tmp.FileName;
    }
};
#pragma pack(pop)
typedef FileEntry *PFileEntry;



namespace NtfsCore 
{
    enum _tagVOLUME_FILE_SYS_TYPE
    {
        ENUM_VOLUME_FILE_SYS_DEFAULT,
        ENUM_VOLUME_FILE_SYS_UNKNOW,
        ENUM_VOLUME_FILE_SYS_NTFS,
        ENUM_VOLUME_FILE_SYS_FAT,
    };

    enum USNCODE
    {
        Code_Default,
        Code_Create,		// ����
        Code_Delete,		// ɾ��
        Code_Rename,		// ������
        Code_Update,      // ������Ϣ
    };
}

struct UsnUpdateRecord
{
public:
    UINT8 nEvent;
    UINT8 bDir;
    UINT8 nVolIndex;
    KEY nKey;
    USN	llUsn;
    FileEntry fileEntry;
    UsnUpdateRecord()
    {
        nKey = 0;
        llUsn = 0;
        bDir = 0;
        nVolIndex = 0;
        nEvent = NtfsCore::Code_Default;
    }
};
typedef std::list<UsnUpdateRecord> UsnRecordList;


