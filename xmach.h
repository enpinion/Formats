// copyright (c) 2017-2021 hors<horsicq@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#ifndef XMACH_H
#define XMACH_H

#include "xbinary.h"
#include "xmach_def.h"

class XMACH : public XBinary
{
    Q_OBJECT

public:
    struct COMMAND_RECORD
    {
        quint32 nType;
        qint64 nOffset;
        qint64 nSize;
    };

    struct LIBRARY_RECORD
    {
        QString sName;
        QString sFullName;
        quint32 timestamp;
        quint32 current_version;
        quint32 compatibility_version;
    };

    struct SEGMENT_RECORD
    {
        qint64 nStructOffset;
        char segname[16];
        quint64 vmaddr;
        quint64 vmsize;
        quint64 fileoff;
        quint64 filesize;
        qint32 maxprot;
        qint32 initprot;
        quint32 nsects;
        quint32 flags;
    };

    struct SECTION_RECORD
    {
        qint64 nStructOffset;
        char sectname[16]; // TODO const
        char segname[16]; // TODO const
        quint64	addr;
        quint64	size;
        quint32	offset;
        quint32	align;
        quint32	reloff;
        quint32	nreloc;
        quint32	flags;
    };

    enum TYPE
    {
        TYPE_UNKNOWN=0,
        TYPE_OBJECT,
        TYPE_EXECUTE,
        TYPE_FVMLIB,
        TYPE_CORE,
        TYPE_PRELOAD,
        TYPE_DYLIB,
        TYPE_DYLINKER,
        TYPE_BUNDLE
    };

    XMACH(QIODevice *pDevice=nullptr,bool bIsImage=false,qint64 nImageAddress=-1);
    ~XMACH();

    bool isValid();
    bool isBigEndian();

    qint64 getHeaderOffset();
    qint64 getHeader32Size();
    qint64 getHeader64Size();

    quint32 getHeader_magic();
    qint32 getHeader_cputype();
    qint32 getHeader_cpusubtype();
    quint32 getHeader_filetype();
    quint32 getHeader_ncmds();
    quint32 getHeader_sizeofcmds();
    quint32 getHeader_flags();
    quint32 getHeader_reserved();

    void setHeader_magic(quint32 nValue);
    void setHeader_cputype(qint32 nValue);
    void setHeader_cpusubtype(qint32 nValue);
    void setHeader_filetype(quint32 nValue);
    void setHeader_ncmds(quint32 nValue);
    void setHeader_sizeofcmds(quint32 nValue);
    void setHeader_flags(quint32 nValue);
    void setHeader_reserved(quint32 nValue);

    qint64 getHeaderSize();

    static QMap<quint64,QString> getHeaderMagics();
    static QMap<quint64,QString> getHeaderMagicsS();
    static QMap<quint64,QString> getHeaderCpuTypes();
    static QMap<quint64,QString> getHeaderCpuTypesS();
    static QMap<quint64,QString> getHeaderFileTypes();
    static QMap<quint64,QString> getHeaderFileTypesS();
    static QMap<quint64,QString> getHeaderFlags();
    static QMap<quint64,QString> getHeaderFlagsS();
    static QMap<quint64,QString> getLoadCommandTypes();
    static QMap<quint64,QString> getLoadCommandTypesS();

    QList<COMMAND_RECORD> getCommandRecords();
    static QList<COMMAND_RECORD> getCommandRecords(quint32 nCommandID,QList<COMMAND_RECORD> *pListCommandRecords);

    bool isCommandPresent(quint32 nCommandID,int nIndex);
    bool isCommandPresent(quint32 nCommandID,int nIndex,QList<COMMAND_RECORD> *pListCommandRecords);

    QByteArray getCommand(quint32 nCommandID,int nIndex=0);
    bool setCommand(quint32 nCommandID,QByteArray baData,int nIndex=0);
    QByteArray getCommand(quint32 nCommandID,int nIndex,QList<COMMAND_RECORD> *pListCommandRecords);
    bool setCommand(quint32 nCommandID,QByteArray baData,int nIndex,QList<COMMAND_RECORD> *pListCommandRecords);

    qint64 getAddressOfEntryPoint();

    virtual _MEMORY_MAP getMemoryMap();
    virtual qint64 getEntryPointOffset(_MEMORY_MAP *pMemoryMap);

    QList<LIBRARY_RECORD> getLibraryRecords();
    QList<LIBRARY_RECORD> getLibraryRecords(QList<COMMAND_RECORD> *pListCommandRecords);
    static LIBRARY_RECORD getLibraryRecordByName(QString sName,QList<LIBRARY_RECORD> *pListLibraryRecords);
    bool isLibraryRecordNamePresent(QString sName);
    static bool isLibraryRecordNamePresent(QString sName,QList<LIBRARY_RECORD> *pListLibraryRecords);

    QList<SEGMENT_RECORD> getSegmentRecords();
    QList<SEGMENT_RECORD> getSegmentRecords(QList<COMMAND_RECORD> *pListCommandRecords);

    QList<SECTION_RECORD> getSectionRecords();
    QList<SECTION_RECORD> getSectionRecords(QList<COMMAND_RECORD> *pListCommandRecords);

    quint32 getNumberOfSections();
    quint32 getNumberOfSections(QList<COMMAND_RECORD> *pListCommandRecords);

    bool isSectionNamePresent(QString sName);
    static bool isSectionNamePresent(QString sName,QList<SECTION_RECORD> *pListSectionRecords);

    static qint32 getSectionNumber(QString sName,QList<SECTION_RECORD> *pListSectionRecords);
    qint32 getSectionNumber(QString sName);

    quint32 getLibraryCurrentVersion(QString sName,QList<LIBRARY_RECORD> *pListLibraryRecords);

    virtual MODE getMode();
    virtual QString getArch();
    virtual FT getFileType();
    virtual int getType();
    virtual QString typeIdToString(int nType);
};

#endif // XMACH_H
