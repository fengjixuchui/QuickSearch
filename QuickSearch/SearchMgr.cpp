#include "SearchMgr.h"
#include "NtfsMgr.h"
#include "ntfsUtils.h"
#include "FileHandler.h"

const std::wstring docExtBuffer[] = {
    L".doc",L".txt",L".rtf",L".hlp",L".pdf",L".ppt",L".pptx",L".xls",L".xlsx",L".docx",L".vsd",L".vsdx",L".ai",L".accdb",
    L".odt",L".xmind",L".xmap",L".mmap",L".xml",L".html",L".potx",L".dotx",L".wps",L".dps",L".et",L".wpt",L".dot",L".mhtml",
    L".html",L".docm",L".dotm",L".mht",L".htm",L".xml",L".ett",L".xlt",L".xlsm",L".dbf",L".csv",L".prn",L".dif",L".xltx",
    L".xltm",L".dpt",L".pot",L".pps",L".pptm",L".potx",L".potm",L".ppsx",L".ppsm",L".rp",L".vsdx",L".caj",L".dwt",L".dwg",
    L".dws",L".dxf"
};

const std::wstring audioExtBuffer[] = {
    L".mp3",L".aac",L".ape",L".3gp",L".m4a",L".flac",L".m4b",L".oga",L".ogg",L".rm",L".wav",L".wma",L".webm"
};

const std::wstring videoExtBuffer[] = {
    L".ac3",L".asf",L".avi",L".bik",L".divx",L".dts",L".dv",L".dvr-ms",L".f4v",L".flv",L".hdmov",L".iso",L".m1v",
    L".m2t",L".m2ts",L".m2v",L".m4v",L".mka",L".mkv",L".mov",L".mp4",L".mpeg",L".mpg",L".mpv", L".mqv",L".mts",
    L".mxf",L".nsv",L".ogm",L".ogv",L".ogx",L".ra",L".ram",L".rec",L".rmvb",L".smk",L".swf",
    L".thd",L".ts",L".vcd",L".vfw",L".vob",L".vp8",L".wmv",L".wtv"
};

const std::wstring pictureExtBuffer[] = {
    L".bmp",L".cur",L".dds",L".gif",L".icns",L".ico",L".jp2",L".jpeg",L".jpg",L".mng",
    L".pbm",L".pgm",L".png",L".ppm",L".psb",L".psd",L".svg",L".svgz",L".tga",L".tif",
    L".tiff",L".wbmp",L".webp",L".xbm",L".xpm",L".wmf"
};

const std::wstring zipExtBuffer[] = {
    L".7z",L".bz2",L".bzip2",L".gz",L".jar",L".lzma",L".lzma86",L".rar",
    L".tar",L".taz",L".tbz2",L".tgz",L".txz",L".xz",L".z",L".zip",L".zipx",L".cab"
};

bool compFileSize(const FileEntry &a, const FileEntry &b)
{
    return a.dwFileSize > b.dwFileSize;
}
bool compFileTime(const FileEntry &a, const FileEntry &b)
{
    return a.dwMftTime > b.dwMftTime;
}

CSearchMgr* g_SearchMgr = new CSearchMgr();
CSearchMgr::CSearchMgr()
{
    
}


CSearchMgr::~CSearchMgr()
{
}

void CSearchMgr::Init()
{
    for (auto index : CNtfsMgr::Instance()->m_vecValidVolumes)
    {
        CSearchThread *ptr = new CSearchThread;
        ptr->Init(index);
        m_vecSearchThread.push_back(ptr);
    }

    int sum = 0;
    sum = sizeof(docExtBuffer) / sizeof(docExtBuffer[0]);
    InitClassFilter(docExtBuffer, sum, (int)TypeDocuments);

    sum = sizeof(audioExtBuffer) / sizeof(audioExtBuffer[0]);
    InitClassFilter(audioExtBuffer, sum, (int)TypeAudios);

    sum = sizeof(videoExtBuffer) / sizeof(videoExtBuffer[0]);
    InitClassFilter(videoExtBuffer, sum, (int)TypeVideos);

    sum = sizeof(pictureExtBuffer) / sizeof(pictureExtBuffer[0]);
    InitClassFilter(pictureExtBuffer, sum, (int)TypeImages);

    sum = sizeof(zipExtBuffer) / sizeof(zipExtBuffer[0]);
    InitClassFilter(zipExtBuffer, sum, (int)TypeCompressed);
}

void CSearchMgr::UnInit()
{
    for (auto& tmp : m_vecSearchThread)
    {
        tmp->Uninit();
    }
    m_vecResult.clear();
    m_MapExtClassFilter.clear();

}

void CSearchMgr::Search(SearchOpt opt)
{
    std::vector<SearchResultItem> nullVec;
    m_vecResult.swap(nullVec);
    m_vecResult.reserve(100);
    searchOpt = opt;
    for (auto tmp : m_vecSearchThread)
    {
        tmp->DoSearch(opt);
    }
    for (auto tmp : m_vecSearchThread)
    {
        tmp->Wait();
    }
    ReSortResult();
}

int CSearchMgr::GetResultCnt(int dwIndex)
{

    return 0;
}

void CSearchMgr::ReSortResult()
{
    std::vector<FileEntry> vecFileEntry;
    vecFileEntry.reserve(100);
    for (int index=0; index < CNtfsMgr::Instance()->m_vecValidVolumes.size();++index)
    {
        std::vector<FileEntry>& vecTmp = m_vecSearchThread[index]->m_vecResult;
        vecFileEntry.insert(vecFileEntry.end(), vecTmp.begin(), vecTmp.end());
        std::vector<FileEntry> nullVec;
        vecTmp.swap(nullVec);
    }
    switch (searchOpt.sortType)
    {
    case Srot_By_Name:
        std::sort(vecFileEntry.begin(), vecFileEntry.end());
        break;
    case Sort_By_MfTime:
        std::sort(vecFileEntry.begin(), vecFileEntry.end(), compFileTime);
        break;
    case Sort_By_FileSize:
        std::sort(vecFileEntry.begin(), vecFileEntry.end(), compFileSize);
        break;
    default:
        std::sort(vecFileEntry.begin(), vecFileEntry.end());
        break;
    }
    for (int i = 0; i<vecFileEntry.size() && i < RESULT_LIMIT; ++i)
    {
        std::wstring path;
        NtfsUtils::GetFilePath(vecFileEntry[i].fileInfo.volIndex, &vecFileEntry[i], path);

        std::string str(vecFileEntry[i].pFileName->FileName);
        int slength = str.length() + 1;
        int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), slength, 0, 0);
        wchar_t* wchr = new wchar_t[len];
        MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)str.c_str(), slength, (LPWSTR)wchr, len);
        std::wstring wstrName(wchr);
        short nFileType = 0;
        if (vecFileEntry[i].fileInfo.dir == 1)
        {
            nFileType = TypeFolders;
        }
        else
        {
            nFileType = GetFileType(wstrName);
        }
        m_vecResult.push_back(SearchResultItem(wstrName,path, vecFileEntry[i].dwFileSize, vecFileEntry[i].dwMftTime, nFileType));
    }
}

short CSearchMgr::GetFileType(std::wstring& filename)
{
    std::wstring strFileType;
    FileHandler::SplitFileType(filename, strFileType);
    if (strFileType.length() != 0)
    {
        if (m_MapExtClassFilter.find(strFileType) != m_MapExtClassFilter.end())
        {
            return m_MapExtClassFilter[strFileType];
        }
    }
    return 0;
}

void CSearchMgr::InitClassFilter(const std::wstring * arrayPtr, int size, int filterType)
{
    for (int i = 0; i < size; i++)
    {
        m_MapExtClassFilter[arrayPtr[i]] = filterType;
    }
}
