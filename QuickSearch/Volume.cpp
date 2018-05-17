#include "Volume.h"
#include <WinBase.h>
CVolume g_ArrayVolumeInfo[VOLUME_COUNT];
CVolume::CVolume()
{
	ResetData();
}


CVolume::~CVolume()
{
}

void CVolume::ResetData()
{
	m_hVolHandle = nullptr;
	m_dwFileRecordSize = 0;
	m_usnNextUSN = 0;
	m_usnFirstUSN = 0;
	m_dwlJournalID = 0;
	m_dwVolSerialNum = 0;
	m_dwVolumeId = 0;
	m_dwVolumeTotalSpace = 0;
	m_dwVolumeFreeSpace = 0;
	m_dwVolumeTotalFileCnt = 0;//文件和目录总量
	m_dwVolumeDirCnt = 0;      //目录量
	m_dwVolumeFileCnt = 0;     //文件量
	m_dwBytesPerCluster = 0;
	m_dwVolumeDriveType = DRIVE_UNKNOWN;
    m_bHaveDB = FALSE;
    m_bCanWriteDB = FALSE;
    m_bValidDB = FALSE;
    m_bValidVolume = FALSE;
}

BOOL CVolume::IsValidVolume()
{
    return m_bValidVolume;
}

BOOL CVolume::IsValidDB()
{
    return m_bValidDB;
}

BOOL CVolume::IsHaveDB()
{
    return m_bHaveDB;
}


