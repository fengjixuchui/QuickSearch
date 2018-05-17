#include "UpdateUSNRecord.h"
#include "NtfsMgr.h"
#include "Volume.h"
#include "IndexManager.h"
UpdateUSNRecord::UpdateUSNRecord()
{
}


UpdateUSNRecord::~UpdateUSNRecord()
{
}

void UpdateUSNRecord::Update(UsnRecordList* pList)
{
    for (auto it = pList->begin(); it != pList->end(); ++it)
    {
        switch (it->nEvent)
        {
        case NtfsCore::Code_Create:
        {
            CreateDiskEntry(it->nVolIndex, it->fileEntry);
            break;
        }
        case NtfsCore::Code_Delete:
        {
            DeleteDiskEntry(it->nVolIndex, it->nKey);
            break;
        }
        case NtfsCore::Code_Rename:
        {
            RenameDiskEntry(it->nVolIndex, it->fileEntry);
            break;
        }
        case NtfsCore::Code_Update:
        {
            UpdateDiskEntry(it->nVolIndex, it->fileEntry);
            break;
        }
        default:
            break;
        }
        if (g_ArrayVolumeInfo[it->nVolIndex].m_usnNextUSN < it->llUsn)
        {
            g_ArrayVolumeInfo[it->nVolIndex].m_usnNextUSN = it->llUsn;
        }
    }
}

void UpdateUSNRecord::CreateDiskEntry(int volIndex, FileEntry entry)
{
    if (entry.FileReferenceNumber==KEY_MASK) return;
    g_pIndexManager->AddEntry(entry, volIndex);
}

BOOL UpdateUSNRecord::DeleteDiskEntry(int volIndex, KEY nFRN)
{
    if (nFRN == KEY_MASK) return FALSE;
    g_pIndexManager->DeleteEntry(nFRN, volIndex);
    return TRUE;
}

BOOL UpdateUSNRecord::RenameDiskEntry(int volIndex, FileEntry entry)
{
    if (entry.FileReferenceNumber == KEY_MASK) return FALSE;
    g_pIndexManager->DeleteEntry(entry.FileReferenceNumber, volIndex);
    g_pIndexManager->AddEntry(entry, volIndex);
    return 0;
}

void UpdateUSNRecord::UpdateDiskEntry(int volIndex, FileEntry entry)
{
    if (entry.FileReferenceNumber == KEY_MASK) return;
    g_pIndexManager->DeleteEntry(entry.FileReferenceNumber, volIndex);
    g_pIndexManager->AddEntry(entry, volIndex);
}
