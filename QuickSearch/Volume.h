#pragma once
#include <WinDef.h>
#include "global.h"
class CVolume
{
public:
	HANDLE		m_hVolHandle;           // 卷的句柄
	DWORD		m_dwFileRecordSize;     // 文件块记录地址的大小
	USN			m_usnNextUSN;           // 每个卷的NextUSN, 用于下次启动更新
	USN			m_usnFirstUSN;          
	DWORDLONG	m_dwlJournalID;		    // 每个卷的JournalID
	DWORD		m_dwVolSerialNum;		// 每个卷的序列号, 用于数据库校验
	DWORD		m_dwBytesPerCluster;	// 每个簇大小
    //Volume Data info  磁盘数据
	DWORD       m_dwVolumeId;
	DWORD       m_dwVolumeDriveType;        //卷类别(eg:可移动磁盘)
	DWORD       m_dwVolumeTotalSpace;		//卷上的总空间 MB
	DWORD       m_dwVolumeFreeSpace;		//卷上的可用空间 MB
	DWORD       m_dwVolumeTotalFileCnt;		//文件和目录总量
	DWORD       m_dwVolumeDirCnt;			//目录量
	DWORD       m_dwVolumeFileCnt;			//文件量
    //
    BOOL        m_bValidDB;              // DB是否有效
    BOOL        m_bHaveDB;               // DB是否存在
    BOOL        m_bCanWriteDB;           // DB满足被写的条件。初始化的时候，有可能扫描不完全。
    BOOL        m_bValidVolume;           //
public:
	CVolume();
	~CVolume();
	void ResetData();
    BOOL IsValidVolume();       //磁盘是否有效，
    BOOL IsValidDB();           //DB是否有效。
    BOOL IsHaveDB();
};

extern CVolume g_ArrayVolumeInfo[VOLUME_COUNT];
