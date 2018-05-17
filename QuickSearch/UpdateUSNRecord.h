#pragma once
#include "global.h"
class UpdateUSNRecord
{
public:
    UpdateUSNRecord();
    ~UpdateUSNRecord();

public:
    void Update(UsnRecordList* pList);

private:
    void CreateDiskEntry(int volIndex, FileEntry entry);
    BOOL DeleteDiskEntry(int volIndex, KEY nFRN);
    BOOL RenameDiskEntry(int volIndex, FileEntry entry);
    void UpdateDiskEntry(int volIndex, FileEntry entry);
};

