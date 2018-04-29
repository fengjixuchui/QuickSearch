#pragma once

#include "global.h"
#include <vector>

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
    BOOL SaveDatabase(); //保存索引到本地DB
private:
    BOOL LoadDatabase(int volIndex);  // LoadDatabase 加载本地数据库
    BOOL CreateDatabase(int volIndex);// 扫描MFT,创建数据库
    
    BOOL LoadIndexData(int volIndex);
    BOOL LoadFileNameData(int volIndex);

    //DB相关
    BOOL GetVolumeDBStorePath(std::wstring& strDbFilePath, const TCHAR* pFileName, int volIndex, BOOL isFileNameData);  //获取存储文件路径
    BOOL CheckValidVolumeDB(int volIndex, LONGLONG& llNextUsn);
    BOOL SaveNtfsInfoDB(DWORD dwBufSize, int volIndex); //4096Byte
private:
    friend class CScanNtfsOrDBThread;
	static CNtfsMgr* CNtfsMgrInstance;
	std::vector<int> m_vecValidVolumes;
    std::vector<CScanNtfsOrDBThread*> m_vecScanThread;
};

