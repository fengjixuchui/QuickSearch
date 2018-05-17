#pragma once

#include "global.h"
#include <vector>
#include "IndexManager.h"
class CNtfsMgr
{
public:
	static CNtfsMgr* Instance();
	static void DestroyInstance();
	CNtfsMgr();
	~CNtfsMgr();
    void UnInit();
public:
    DWORD initVolumes();
    BOOL ScanVolumeFileData();
    BOOL DoScanVolume(DWORD dwIndex);
    void CreateScanThread();
    void ReleaseScanThread();
    BOOL IsQuit();
    DWORD GetVolFileCnt(int nvolIndex);
    DWORD GetAllFileCnt();
    BOOL SaveDatabase(); //保存索引到本地DB
private:
    BOOL LoadDatabase(int volIndex);  // LoadDatabase 加载本地数据库
    BOOL CreateDatabase(int volIndex);// 扫描MFT,创建数据库
   
    //DB相关
    BOOL GetVolumeDBStorePath(std::wstring& strDbFilePath, const TCHAR* pFileName, int volIndex, BOOL isFileNameData);  //获取存储文件路径
    BOOL CheckValidVolumeDB(int volIndex, LONGLONG& llNextUsn);
    BOOL SaveNtfsInfoDB(TCHAR* ParamHeaderBuffer,DWORD dwBufSize, int volIndex); //4096Byte
    BOOL LoadFileNameDatabase(int volIndex);
    BOOL SaveFileNameDataBase(int volIndex);
public:
    std::vector<int> m_vecValidVolumes;
private:
    friend class CScanNtfsOrDBThread;
	static CNtfsMgr* CNtfsMgrInstance;
    std::vector<CScanNtfsOrDBThread*> m_vecScanThread;
};

