#pragma once
#include <WinDef.h>
#include "global.h"
class CVolume
{
public:
	HANDLE		m_hVolHandle;           // ��ľ��
	DWORD		m_dwFileRecordSize;     // �ļ����¼��ַ�Ĵ�С
	USN			m_usnNextUSN;           // ÿ�����NextUSN, �����´���������
	USN			m_usnFirstUSN;          
	DWORDLONG	m_dwlJournalID;		    // ÿ�����JournalID
	DWORD		m_dwVolSerialNum;		// ÿ��������к�, �������ݿ�У��
	DWORD		m_dwBytesPerCluster;	// ÿ���ش�С
    //Volume Data info  ��������
	DWORD       m_dwVolumeId;
	DWORD       m_dwVolumeDriveType;        //�����(eg:���ƶ�����)
	DWORD       m_dwVolumeTotalSpace;		//���ϵ��ܿռ� MB
	DWORD       m_dwVolumeFreeSpace;		//���ϵĿ��ÿռ� MB
	DWORD       m_dwVolumeTotalFileCnt;		//�ļ���Ŀ¼����
	DWORD       m_dwVolumeDirCnt;			//Ŀ¼��
	DWORD       m_dwVolumeFileCnt;			//�ļ���
    //
    BOOL        m_bValidDB;              // DB�Ƿ���Ч
    BOOL        m_bHaveDB;               // DB�Ƿ����
    BOOL        m_bCanWriteDB;           // DB���㱻д����������ʼ����ʱ���п���ɨ�費��ȫ��
    BOOL        m_bValidVolume;           //
public:
	CVolume();
	~CVolume();
	void ResetData();
    BOOL IsValidVolume();       //�����Ƿ���Ч��
    BOOL IsValidDB();           //DB�Ƿ���Ч��
    BOOL IsHaveDB();
};

extern CVolume g_ArrayVolumeInfo[VOLUME_COUNT];
