#pragma once
#include "global.h"
#include <map>
#include <string>
#include <tchar.h>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <fstream>

using boost::multi_index_container;
using namespace boost::multi_index;
typedef multi_index_container<
    FileEntry,
    indexed_by<
    ordered_unique<
    BOOST_MULTI_INDEX_MEMBER(FileEntry,KEY, FileReferenceNumber)>,
    ordered_non_unique<
    identity<FileEntry>>,
    ordered_non_unique<
    BOOST_MULTI_INDEX_MEMBER(FileEntry, DWORD, dwFileSize)>,
    ordered_non_unique<
    BOOST_MULTI_INDEX_MEMBER(FileEntry, DWORD, dwMftTime)> >
    > FileEntry_Set;
typedef FileEntry_Set::nth_index<0>::type FileEntry_By_FRN;
typedef FileEntry_Set::nth_index<1>::type FileEntry_By_Name;
typedef FileEntry_Set::nth_index<2>::type FileEntry_By_FileSize;
typedef FileEntry_Set::nth_index<3>::type FileEntry_By_MftTime;

//Multi Index 序列化保存
namespace boost {
    namespace serialization {

        template<class Archive>
        void serialize(Archive & ar, FileEntry & g, const unsigned int version)
        {
            ar & g.FileReferenceNumber;
            ar & g.ParentFileReferenceNumber;
            ar & g.dwFileSize;
            ar & g.dwMftTime;
            //位域必须这样保存
            BYTE bDir = g.fileInfo.dir;
            BYTE bParentroot = g.fileInfo.parentroot;
            BYTE bChname = g.fileInfo.chname;
            BYTE bVolIndex = g.fileInfo.volIndex;
            ar & BOOST_SERIALIZATION_NVP(bDir);
            ar & BOOST_SERIALIZATION_NVP(bParentroot);
            ar & BOOST_SERIALIZATION_NVP(bChname);
            ar & BOOST_SERIALIZATION_NVP(bVolIndex);

            g.fileInfo.dir = bDir;
            g.fileInfo.parentroot = bParentroot;
            g.fileInfo.chname = bChname;
            g.fileInfo.volIndex = bVolIndex;
        }

    } // namespace serialization
} // namespace boost

const TCHAR TC_DB_FILE_NAME[] = _T("QuickSearch");       // 数据库文件名
class IndexManager
{
public:

    IndexManager();
    ~IndexManager();
    BOOL AddEntry(FileEntry fileEntry,int nVolIndex);
    BOOL DeleteEntry(KEY FRN, int nVolIndex);
    DWORD GetVolFileCnt(int nVolIndex);
    BOOL isFindEntry(KEY FRN, int nVolIndex);
    //FileEntry_Set& GetVolIndex(int nVolIndex);

    //DB
    void Save(std::wstring& strDbFilePath, int nVolIndex);
    void Load(std::wstring& strDbFilePath,int nVolIndex);

    void UnInit();
public:
    FileEntry_Set m_VolFileIndex[VOLUME_COUNT];
};
extern IndexManager *g_pIndexManager;
