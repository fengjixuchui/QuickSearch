#pragma once
#include "easylogging++.h"
#define DIRVE_COUNT 26 
extern volatile BOOL g_bQuitSearCore;

#define KEY DWORD
#define KEY_MASK 0xFFFFFFFF                             //掩码

#define MAX_FILE_NAME_LENGTH 1000
#define DW_SEARCHDB_VERSION  1000         // 数据库版本号长度
#define DW_SEARCHDB_HEADER_LENGTH  1024   //1kb DB文件头
#define FILEENTRY_DB_LENGTH 1024          //1kb
typedef char UTF8,*PUTF8;
#pragma pack(push,1)

#define FILEENTRY_HEADER_SIZE (int)sizeof(FileEntry) - 1
struct FileEntry
{
    KEY FileReferenceNumber;
    KEY ParentFileReferenceNumber;
    //FileEntry* pParent;
    DWORD dwFileSize;      //文件大小
    DWORD dwMftTime;       //文件修改时间
    struct
    {
        unsigned char dir : 1;    //是否是目录
        unsigned char parentroot : 1;    //父目录是否是根目录
        unsigned char chname : 1;	//是否有中文字符
        unsigned char volIndex : 5;    //所属盘符
        unsigned short FileNameLength;    //文件名长度
    }fileInfo;
    PCHAR  FileName;                      //文件名，不包含\0
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
        Code_Create,		// 创建
        Code_Delete,		// 删除
        Code_Rename,		// 重命名
        Code_Update,      // 更新信息
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


