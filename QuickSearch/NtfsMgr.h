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
    BOOL SaveDatabase(); //��������������DB
private:
    BOOL LoadDatabase(int volIndex);  // LoadDatabase ���ر������ݿ�
    BOOL CreateDatabase(int volIndex);// ɨ��MFT,�������ݿ�
    
    BOOL LoadIndexData(int volIndex);
    BOOL LoadFileNameData(int volIndex);

    //DB���
    BOOL GetVolumeDBStorePath(std::wstring& strDbFilePath, const TCHAR* pFileName, int volIndex, BOOL isFileNameData);  //��ȡ�洢�ļ�·��
    BOOL CheckValidVolumeDB(int volIndex, LONGLONG& llNextUsn);
    BOOL SaveNtfsInfoDB(DWORD dwBufSize, int volIndex); //4096Byte
private:
    friend class CScanNtfsOrDBThread;
	static CNtfsMgr* CNtfsMgrInstance;
	std::vector<int> m_vecValidVolumes;
    std::vector<CScanNtfsOrDBThread*> m_vecScanThread;
};

