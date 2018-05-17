#pragma once
#include "easylogging++.h"
#include "compareName.h"
#define VOLUME_COUNT 26 
extern volatile BOOL g_bQuitSearCore;

#define KEY DWORD
#define KEY_MASK 0xFFFFFFFF                             //����

#define MAX_FILE_NAME_LENGTH 1000
#define DW_SEARCHDB_VERSION  1000         // ���ݿ�汾�ų���
#define DW_SEARCHDB_HEADER_LENGTH  1024   //1kb DB�ļ�ͷ
#define FILEENTRY_DB_LENGTH 1024          //1kb
typedef char UTF8,*PUTF8;

const KEY ROOT_NUMBER = ((KEY)1407374883553285 & KEY_MASK);
#define RESULT_LIMIT 1000
#pragma pack(push,1)
#define FileNameEntryHeaderSize (int)sizeof(FileNameEntry)-1
struct FileNameEntry
{
    unsigned short FileNameLength;
    KEY FRN;
    char FileName[1];
};
#pragma pack(pop)
typedef FileNameEntry *PFileNameEntry;

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
    }fileInfo;
    PFileNameEntry  pFileName;                      //�ļ���ָ�룬������\0
    
    FileEntry()
    {
        FileReferenceNumber = KEY_MASK;
        ParentFileReferenceNumber = KEY_MASK;
        dwFileSize = 0;
        dwMftTime = 0;
        dwMftTime = 0;
    }
    
    bool operator < (const FileEntry& tmp) const
    {
        return CompareName(this->pFileName->FileName, tmp.pFileName->FileName);
    }
    bool operator > (const FileEntry& tmp) const
    {
        return CompareName(this->pFileName->FileName, tmp.pFileName->FileName);
    }
    bool operator == (const FileEntry& tmp) const
    {
        return CompareName(this->pFileName->FileName, tmp.pFileName->FileName);
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

enum SortType
{
    Srot_By_Name,
    Sort_By_MfTime,
    Sort_By_FileSize
};
struct SearchOpt
{
    std::string name; //��ѯ�ַ���
    SortType sortType;
    BOOL bAscending; //������� ���� ����
    SearchOpt()
    {

    }
    SearchOpt(std::string name, SortType sortType, BOOL bAscending)
    {
        this->bAscending = bAscending;
        this->name = name;
        this->sortType = sortType;
    }
    SearchOpt& operator = (const SearchOpt& opt)
    {
        this->bAscending = opt.bAscending;
        this->name = opt.name;
        this->sortType = opt.sortType;
        return *this;
    }
};

struct SearchResultItem
{
    std::wstring     filename;
    std::wstring     path;
    DWORD            fileSize;
    DWORD            modifiedTime;
    short            fileType;
    SearchResultItem()
    {
        filename = L"";
        path = L"";
        fileSize = 0;
        modifiedTime = 0;
        fileType = 0;
    }
    SearchResultItem(std::wstring filename, std::wstring path, DWORD fileSize, DWORD modifiedTime,short filetype)
    {
        this->filename = filename;
        this->fileSize = fileSize;
        this->modifiedTime = modifiedTime;
        this->path = path;
        this->fileType = filetype;
    }
    SearchResultItem& operator = (const SearchResultItem& item)
    {
        this->filename = item.filename;
        this->fileSize = item.fileSize;
        this->modifiedTime = item.modifiedTime;
        this->path = item.path;
        this->fileType = item.fileType;
        return *this;
    }
    SearchResultItem(const SearchResultItem& item)
    {
        this->filename = item.filename;
        this->fileSize = item.fileSize;
        this->modifiedTime = item.modifiedTime;
        this->path = item.path;
        this->fileType = item.fileType;
    }
};

enum fileType
{
    TypeDocuments = 1,
    TypeImages = 2,
    TypeAudios = 4,
    TypeVideos = 8,
    TypeCompressed = 16,
    TypeFolders = 32
};