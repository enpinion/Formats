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
#include "xpe.h"

XPE::XPE(QIODevice *pDevice, bool bIsImage, qint64 nImageBase): XMSDOS(pDevice,bIsImage,nImageBase)
{
}

bool XPE::isValid()
{
    bool bResult=false;

    quint16 magic=get_magic();

    if( (magic==XMSDOS_DEF::S_IMAGE_DOS_SIGNATURE_MZ)||
        (magic==XMSDOS_DEF::S_IMAGE_DOS_SIGNATURE_ZM))
    {
        qint32 lfanew=get_lfanew();

        if(lfanew>0)
        {
            quint32 signature=read_uint32(lfanew);

            if(signature==XPE_DEF::S_IMAGE_NT_SIGNATURE)
            {
                bResult=true;
            }
        }
    }

    return bResult;
}

XBinary::MODE XPE::getMode()
{
    MODE result=MODE_32;

    quint16 nMachine=getFileHeader_Machine();

    if( (nMachine==XPE_DEF::S_IMAGE_FILE_MACHINE_AMD64)||
        (nMachine==XPE_DEF::S_IMAGE_FILE_MACHINE_IA64)||
        (nMachine==XPE_DEF::S_IMAGE_FILE_MACHINE_ARM64))
    {
        result=MODE_64;
    }
    else
    {
        result=MODE_32;
    }

    return result;
}

QString XPE::getArch()
{
    return getImageFileHeaderMachinesS().value(getFileHeader_Machine(),QString("UNKNOWN"));
}

bool XPE::isBigEndian()
{
    // TODO Check Machine
    return false;
}

XBinary::FT XPE::getFileType()
{
    FT result=FT_PE32;

    MODE mode=getMode();

    if(mode==MODE_32)
    {
        result=FT_PE32;
    }
    else if(mode==MODE_64)
    {
        result=FT_PE64;
    }

    return result;
}

int XPE::getType()
{
    TYPE result=TYPE_UNKNOWN;

    quint16 nSubsystem=getOptionalHeader_Subsystem();

    if(nSubsystem==XPE_DEF::S_IMAGE_SUBSYSTEM_NATIVE)
    {
        result=TYPE_DRIVER;
    }
    else if(nSubsystem==XPE_DEF::S_IMAGE_SUBSYSTEM_WINDOWS_CUI)
    {
        result=TYPE_CONSOLE;
    }
    else if(nSubsystem==XPE_DEF::S_IMAGE_SUBSYSTEM_WINDOWS_GUI)
    {
        result=TYPE_GUI;
    }
    else if(nSubsystem==XPE_DEF::S_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER)
    {
        result=TYPE_EFIBOOT;
    }
    else if(nSubsystem==XPE_DEF::S_IMAGE_SUBSYSTEM_EFI_APPLICATION)
    {
        result=TYPE_EFI;
    }
    else if(nSubsystem==XPE_DEF::S_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER)
    {
        result=TYPE_EFIRUNTIMEDRIVER;
    }
    else if(nSubsystem==XPE_DEF::S_IMAGE_SUBSYSTEM_XBOX)
    {
        result=TYPE_XBOX;
    }
    else if(nSubsystem==XPE_DEF::S_IMAGE_SUBSYSTEM_OS2_CUI)
    {
        result=TYPE_OS2;
    }
    else if(nSubsystem==XPE_DEF::S_IMAGE_SUBSYSTEM_POSIX_CUI)
    {
        result=TYPE_POSIX;
    }
    else if(nSubsystem==XPE_DEF::S_IMAGE_SUBSYSTEM_WINDOWS_CE_GUI)
    {
        result=TYPE_CE;
    }

    if(result!=TYPE_DRIVER) // TODO Check
    {
        if((getFileHeader_Characteristics()&XPE_DEF::S_IMAGE_FILE_DLL))
        {
            result=TYPE_DLL;
        }
    }

    return result;
}

QString XPE::typeIdToString(int nType)
{
    QString sResult="Unknown"; // mb TODO translate

    switch(nType)
    {
        case TYPE_UNKNOWN:          sResult=QString("Unknown");             break; // mb TODO translate
        case TYPE_GUI:              sResult=QString("GUI");                 break;
        case TYPE_CONSOLE:          sResult=QString("Console");             break;
        case TYPE_DLL:              sResult=QString("DLL");                 break;
        case TYPE_DRIVER:           sResult=QString("Driver");              break;
        case TYPE_EFIBOOT:          sResult=QString("EFI Boot");            break;
        case TYPE_EFI:              sResult=QString("EFI");                 break;
        case TYPE_EFIRUNTIMEDRIVER: sResult=QString("EFI Runtime driver");  break;
        case TYPE_XBOX:             sResult=QString("XBOX");                break;
        case TYPE_OS2:              sResult=QString("OS2");                 break;
        case TYPE_POSIX:            sResult=QString("POSIX");               break;
        case TYPE_CE:               sResult=QString("CE");                  break;
    }

    return sResult;
}

qint64 XPE::getNtHeadersOffset()
{
    qint64 result=get_lfanew();

    if(!_isOffsetValid(result))
    {
        result=-1;
    }

    return result;
}

quint32 XPE::getNtHeaders_Signature()
{
    qint64 nOffset=getNtHeadersOffset();

    return read_uint32(nOffset);
}

void XPE::setNtHeaders_Signature(quint32 nValue)
{
    write_uint32(getNtHeadersOffset(),nValue);
}

qint64 XPE::getFileHeaderOffset()
{
    qint64 result=get_lfanew()+4;

    if(!_isOffsetValid(result))
    {
        result=-1;
    }

    return result;
}

qint64 XPE::getFileHeaderSize()
{
    return sizeof(XPE_DEF::S_IMAGE_FILE_HEADER);
}

XPE_DEF::S_IMAGE_FILE_HEADER XPE::getFileHeader()
{
    XPE_DEF::S_IMAGE_FILE_HEADER result={};

    qint64 nFileHeaderOffset=getFileHeaderOffset();

    if(nFileHeaderOffset!=-1)
    {
        result.Machine=read_uint16(nFileHeaderOffset+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,Machine));
        result.NumberOfSections=read_uint16(nFileHeaderOffset+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,NumberOfSections));
        result.TimeDateStamp=read_uint32(nFileHeaderOffset+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,TimeDateStamp));
        result.PointerToSymbolTable=read_uint32(nFileHeaderOffset+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,PointerToSymbolTable));
        result.NumberOfSymbols=read_uint32(nFileHeaderOffset+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,NumberOfSymbols));
        result.SizeOfOptionalHeader=read_uint16(nFileHeaderOffset+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,SizeOfOptionalHeader));
        result.Characteristics=read_uint16(nFileHeaderOffset+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,Characteristics));
    }

    return result;
}

void XPE::setFileHeader(XPE_DEF::S_IMAGE_FILE_HEADER *pFileHeader)
{
    qint64 nFileHeaderOffset=getFileHeaderOffset();

    if(nFileHeaderOffset!=-1)
    {
        write_uint16(nFileHeaderOffset+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,Machine),pFileHeader->Machine);
        write_uint16(nFileHeaderOffset+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,NumberOfSections),pFileHeader->NumberOfSections);
        write_uint32(nFileHeaderOffset+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,TimeDateStamp),pFileHeader->TimeDateStamp);
        write_uint32(nFileHeaderOffset+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,PointerToSymbolTable),pFileHeader->PointerToSymbolTable);
        write_uint32(nFileHeaderOffset+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,NumberOfSymbols),pFileHeader->NumberOfSymbols);
        write_uint16(nFileHeaderOffset+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,SizeOfOptionalHeader),pFileHeader->SizeOfOptionalHeader);
        write_uint16(nFileHeaderOffset+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,Characteristics),pFileHeader->Characteristics);
    }
}

quint16 XPE::getFileHeader_Machine()
{
    return read_uint16(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,Machine));
}

quint16 XPE::getFileHeader_NumberOfSections()
{
    return read_uint16(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,NumberOfSections));
}

quint32 XPE::getFileHeader_TimeDateStamp()
{
    return read_uint32(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,TimeDateStamp));
}

quint32 XPE::getFileHeader_PointerToSymbolTable()
{
    return read_uint32(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,PointerToSymbolTable));
}

quint32 XPE::getFileHeader_NumberOfSymbols()
{
    return read_uint32(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,NumberOfSymbols));
}

quint16 XPE::getFileHeader_SizeOfOptionalHeader()
{
    return read_uint16(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,SizeOfOptionalHeader));
}

quint16 XPE::getFileHeader_Characteristics()
{
    return read_uint16(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,Characteristics));
}

void XPE::setFileHeader_Machine(quint16 nValue)
{
    write_uint16(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,Machine),nValue);
}

void XPE::setFileHeader_NumberOfSections(quint16 nValue)
{
    write_uint16(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,NumberOfSections),nValue);
}

void XPE::setFileHeader_TimeDateStamp(quint32 nValue)
{
    write_uint32(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,TimeDateStamp),nValue);
}

void XPE::setFileHeader_PointerToSymbolTable(quint32 nValue)
{
    write_uint32(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,PointerToSymbolTable),nValue);
}

void XPE::setFileHeader_NumberOfSymbols(quint32 nValue)
{
    write_uint32(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,NumberOfSymbols),nValue);
}

void XPE::setFileHeader_SizeOfOptionalHeader(quint16 nValue)
{
    write_uint16(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,SizeOfOptionalHeader),nValue);
}

void XPE::setFileHeader_Characteristics(quint16 nValue)
{
    write_uint16(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,Characteristics),nValue);
}

qint64 XPE::getOptionalHeaderOffset()
{
    qint64 result=get_lfanew()+4+sizeof(XPE_DEF::S_IMAGE_FILE_HEADER);

    if(!_isOffsetValid(result))
    {
        result=-1;
    }

    return result;
}

qint64 XPE::getOptionalHeaderSize()
{
    qint64 nResult=0;

    if(is64())
    {
        nResult=sizeof(XPE_DEF::IMAGE_OPTIONAL_HEADER64);
    }
    else
    {
        nResult=sizeof(XPE_DEF::IMAGE_OPTIONAL_HEADER32);
    }

    return nResult;
}

XPE_DEF::IMAGE_OPTIONAL_HEADER32 XPE::getOptionalHeader32()
{
    XPE_DEF::IMAGE_OPTIONAL_HEADER32 result={};
    // TODO
    read_array(getOptionalHeaderOffset(),(char *)&result,sizeof(XPE_DEF::IMAGE_OPTIONAL_HEADER32));

    return result;
}

XPE_DEF::IMAGE_OPTIONAL_HEADER64 XPE::getOptionalHeader64()
{
    XPE_DEF::IMAGE_OPTIONAL_HEADER64 result={};
    // TODO
    read_array(getOptionalHeaderOffset(),(char *)&result,sizeof(XPE_DEF::IMAGE_OPTIONAL_HEADER64));

    return result;
}

void XPE::setOptionalHeader32(XPE_DEF::IMAGE_OPTIONAL_HEADER32 *pOptionalHeader32)
{
    // TODO
    write_array(getOptionalHeaderOffset(),(char *)pOptionalHeader32,sizeof(XPE_DEF::IMAGE_OPTIONAL_HEADER32));
}

void XPE::setOptionalHeader64(XPE_DEF::IMAGE_OPTIONAL_HEADER64 *pOptionalHeader64)
{
    // TODO
    write_array(getOptionalHeaderOffset(),(char *)pOptionalHeader64,sizeof(XPE_DEF::IMAGE_OPTIONAL_HEADER64));
}

XPE_DEF::IMAGE_OPTIONAL_HEADER32S XPE::getOptionalHeader32S()
{
    XPE_DEF::IMAGE_OPTIONAL_HEADER32S result={};
    // TODO
    read_array(getOptionalHeaderOffset(),(char *)&result,sizeof(XPE_DEF::IMAGE_OPTIONAL_HEADER32S));

    return result;
}

XPE_DEF::IMAGE_OPTIONAL_HEADER64S XPE::getOptionalHeader64S()
{
    XPE_DEF::IMAGE_OPTIONAL_HEADER64S result={};
    // TODO
    read_array(getOptionalHeaderOffset(),(char *)&result,sizeof(XPE_DEF::IMAGE_OPTIONAL_HEADER64S));

    return result;
}

void XPE::setOptionalHeader32S(XPE_DEF::IMAGE_OPTIONAL_HEADER32S *pOptionalHeader32S)
{
    // TODO check -1
    write_array(getOptionalHeaderOffset(),(char *)pOptionalHeader32S,sizeof(XPE_DEF::IMAGE_OPTIONAL_HEADER32S));
}

void XPE::setOptionalHeader64S(XPE_DEF::IMAGE_OPTIONAL_HEADER64S *pOptionalHeader64S)
{
    // TODO check -1
    write_array(getOptionalHeaderOffset(),(char *)pOptionalHeader64S,sizeof(XPE_DEF::IMAGE_OPTIONAL_HEADER64S));
}

quint16 XPE::getOptionalHeader_Magic()
{
    return read_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,Magic));
}

quint8 XPE::getOptionalHeader_MajorLinkerVersion()
{
    return read_uint8(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MajorLinkerVersion));
}

quint8 XPE::getOptionalHeader_MinorLinkerVersion()
{
    return read_uint8(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MinorLinkerVersion));
}

quint32 XPE::getOptionalHeader_SizeOfCode()
{
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfCode));
}

quint32 XPE::getOptionalHeader_SizeOfInitializedData()
{
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfInitializedData));
}

quint32 XPE::getOptionalHeader_SizeOfUninitializedData()
{
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfUninitializedData));
}

quint32 XPE::getOptionalHeader_AddressOfEntryPoint()
{
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,AddressOfEntryPoint));
}

quint32 XPE::getOptionalHeader_BaseOfCode()
{
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,BaseOfCode));
}

quint32 XPE::getOptionalHeader_BaseOfData()
{
    // TODO no for x64
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,BaseOfData));
}

quint64 XPE::getOptionalHeader_ImageBase()
{
    quint64 nResult=0;

    if(is64())
    {
        nResult=read_uint64(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,ImageBase));
    }
    else
    {
        nResult=read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,ImageBase));
    }

    return nResult;
}

quint32 XPE::getOptionalHeader_SectionAlignment()
{
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SectionAlignment));
}

quint32 XPE::getOptionalHeader_FileAlignment()
{
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,FileAlignment));
}

quint16 XPE::getOptionalHeader_MajorOperatingSystemVersion()
{
    return read_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MajorOperatingSystemVersion));
}

quint16 XPE::getOptionalHeader_MinorOperatingSystemVersion()
{
    return read_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MinorOperatingSystemVersion));
}

quint16 XPE::getOptionalHeader_MajorImageVersion()
{
    return read_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MajorImageVersion));
}

quint16 XPE::getOptionalHeader_MinorImageVersion()
{
    return read_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MinorImageVersion));
}

quint16 XPE::getOptionalHeader_MajorSubsystemVersion()
{
    return read_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MajorSubsystemVersion));
}

quint16 XPE::getOptionalHeader_MinorSubsystemVersion()
{
    return read_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MinorSubsystemVersion));
}

quint32 XPE::getOptionalHeader_Win32VersionValue()
{
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,Win32VersionValue));
}

quint32 XPE::getOptionalHeader_SizeOfImage()
{
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfImage));
}

quint32 XPE::getOptionalHeader_SizeOfHeaders()
{
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfHeaders));
}

quint32 XPE::getOptionalHeader_CheckSum()
{
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,CheckSum));
}

quint16 XPE::getOptionalHeader_Subsystem()
{
    return read_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,Subsystem));
}

quint16 XPE::getOptionalHeader_DllCharacteristics()
{
    return read_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,DllCharacteristics));
}

qint64 XPE::getOptionalHeader_SizeOfStackReserve()
{
    qint64 nResult=0;

    if(is64())
    {
        nResult=read_uint64(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,SizeOfStackReserve));
    }
    else
    {
        nResult=read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfStackReserve));
    }

    return nResult;
}

qint64 XPE::getOptionalHeader_SizeOfStackCommit()
{
    qint64 nResult=0;

    if(is64())
    {
        nResult=read_uint64(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,SizeOfStackCommit));
    }
    else
    {
        nResult=read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfStackCommit));
    }

    return nResult;
}

qint64 XPE::getOptionalHeader_SizeOfHeapReserve()
{
    qint64 nResult=0;

    if(is64())
    {
        nResult=read_uint64(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,SizeOfHeapReserve));
    }
    else
    {
        nResult=read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfHeapReserve));
    }

    return nResult;
}

qint64 XPE::getOptionalHeader_SizeOfHeapCommit()
{
    qint64 nResult=0;

    if(is64())
    {
        nResult=read_uint64(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,SizeOfHeapCommit));
    }
    else
    {
        nResult=read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfHeapCommit));
    }

    return nResult;
}

quint32 XPE::getOptionalHeader_LoaderFlags()
{
    quint32 nResult=0;

    if(is64())
    {
        nResult=read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,LoaderFlags));
    }
    else
    {
        nResult=read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,LoaderFlags));
    }

    return nResult;
}

quint32 XPE::getOptionalHeader_NumberOfRvaAndSizes()
{
    quint32 nResult=0;

    if(is64())
    {
        nResult=read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,NumberOfRvaAndSizes));
    }
    else
    {
        nResult=read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,NumberOfRvaAndSizes));
    }

    return nResult;
}

void XPE::setOptionalHeader_Magic(quint16 nValue)
{
    write_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,Magic),nValue);
}

void XPE::setOptionalHeader_MajorLinkerVersion(quint8 nValue)
{
    write_uint8(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MajorLinkerVersion),nValue);
}

void XPE::setOptionalHeader_MinorLinkerVersion(quint8 nValue)
{
    write_uint8(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MinorLinkerVersion),nValue);
}

void XPE::setOptionalHeader_SizeOfCode(quint32 nValue)
{
    write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfCode),nValue);
}

void XPE::setOptionalHeader_SizeOfInitializedData(quint32 nValue)
{
    write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfInitializedData),nValue);
}

void XPE::setOptionalHeader_SizeOfUninitializedData(quint32 nValue)
{
    write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfUninitializedData),nValue);
}

void XPE::setOptionalHeader_AddressOfEntryPoint(quint32 nValue)
{
    write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,AddressOfEntryPoint),nValue);
}

void XPE::setOptionalHeader_BaseOfCode(quint32 nValue)
{
    write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,BaseOfCode),nValue);
}

void XPE::setOptionalHeader_BaseOfData(quint32 nValue)
{
    if(is64()) // There is no BaseOfData for PE64
    {
        // TODO error string
    }
    else
    {
        write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,BaseOfData),nValue);
    }
}

void XPE::setOptionalHeader_ImageBase(quint64 nValue)
{
    if(is64())
    {
        write_uint64(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,ImageBase),nValue);
    }
    else
    {
        write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,ImageBase),nValue);
    }
}

void XPE::setOptionalHeader_SectionAlignment(quint32 nValue)
{
    write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SectionAlignment),nValue);
}

void XPE::setOptionalHeader_FileAlignment(quint32 nValue)
{
    write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,FileAlignment),nValue);
}

void XPE::setOptionalHeader_MajorOperatingSystemVersion(quint16 nValue)
{
    write_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MajorOperatingSystemVersion),nValue);
}

void XPE::setOptionalHeader_MinorOperatingSystemVersion(quint16 nValue)
{
    write_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MinorOperatingSystemVersion),nValue);
}

void XPE::setOptionalHeader_MajorImageVersion(quint16 nValue)
{
    write_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MajorImageVersion),nValue);
}

void XPE::setOptionalHeader_MinorImageVersion(quint16 nValue)
{
    write_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MinorImageVersion),nValue);
}

void XPE::setOptionalHeader_MajorSubsystemVersion(quint16 nValue)
{
    write_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MajorSubsystemVersion),nValue);
}

void XPE::setOptionalHeader_MinorSubsystemVersion(quint16 nValue)
{
    write_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MinorSubsystemVersion),nValue);
}

void XPE::setOptionalHeader_Win32VersionValue(quint32 nValue)
{
    write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,Win32VersionValue),nValue);
}

void XPE::setOptionalHeader_SizeOfImage(quint32 nValue)
{
    write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfImage),nValue);
}

void XPE::setOptionalHeader_SizeOfHeaders(quint32 nValue)
{
    write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfHeaders),nValue);
}

void XPE::setOptionalHeader_CheckSum(quint32 nValue)
{
    write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,CheckSum),nValue);
}

void XPE::setOptionalHeader_Subsystem(quint16 nValue)
{
    write_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,Subsystem),nValue);
}

void XPE::setOptionalHeader_DllCharacteristics(quint16 nValue)
{
    write_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,DllCharacteristics),nValue);
}

void XPE::setOptionalHeader_SizeOfStackReserve(quint64 nValue)
{
    if(is64())
    {
        write_uint64(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,SizeOfStackReserve),nValue);
    }
    else
    {
        write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfStackReserve),nValue);
    }
}

void XPE::setOptionalHeader_SizeOfStackCommit(quint64 nValue)
{
    if(is64())
    {
        write_uint64(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,SizeOfStackCommit),nValue);
    }
    else
    {
        write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfStackCommit),nValue);
    }
}

void XPE::setOptionalHeader_SizeOfHeapReserve(quint64 nValue)
{
    if(is64())
    {
        write_uint64(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,SizeOfHeapReserve),nValue);
    }
    else
    {
        write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfHeapReserve),nValue);
    }
}

void XPE::setOptionalHeader_SizeOfHeapCommit(quint64 nValue)
{
    if(is64())
    {
        write_uint64(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,SizeOfHeapCommit),nValue);
    }
    else
    {
        write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfHeapCommit),nValue);
    }
}

void XPE::setOptionalHeader_LoaderFlags(quint32 nValue)
{
    if(is64())
    {
        write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,LoaderFlags),nValue);
    }
    else
    {
        write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,LoaderFlags),nValue);
    }
}

void XPE::setOptionalHeader_NumberOfRvaAndSizes(quint32 nValue)
{
    if(is64())
    {
        write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,NumberOfRvaAndSizes),nValue);
    }
    else
    {
        write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,NumberOfRvaAndSizes),nValue);
    }
}

XPE_DEF::IMAGE_DATA_DIRECTORY XPE::read_IMAGE_DATA_DIRECTORY(qint64 nOffset)
{
    XPE_DEF::IMAGE_DATA_DIRECTORY result={};

    result.VirtualAddress=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_DATA_DIRECTORY,VirtualAddress));
    result.Size=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_DATA_DIRECTORY,Size));

    return result;
}

void XPE::write_IMAGE_DATA_DIRECTORY(qint64 nOffset, XPE_DEF::IMAGE_DATA_DIRECTORY *pDataDirectory)
{
    write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_DATA_DIRECTORY,VirtualAddress),pDataDirectory->VirtualAddress);
    write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_DATA_DIRECTORY,Size),pDataDirectory->Size);
}

XPE_DEF::IMAGE_DATA_DIRECTORY XPE::getOptionalHeader_DataDirectory(quint32 nNumber)
{
    XPE_DEF::IMAGE_DATA_DIRECTORY result={};

    //    if(nNumber<getOptionalHeader_NumberOfRvaAndSizes()) // There are some protectors with false NumberOfRvaAndSizes
    if(nNumber<16)
    {
        if(is64())
        {
            result=read_IMAGE_DATA_DIRECTORY(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,DataDirectory)+nNumber*sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY));
        }
        else
        {
            result=read_IMAGE_DATA_DIRECTORY(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,DataDirectory)+nNumber*sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY));
        }
    }

    return result;
}

void XPE::setOptionalHeader_DataDirectory(quint32 nNumber,XPE_DEF::IMAGE_DATA_DIRECTORY *pDataDirectory)
{
    //    if(nNumber<16)
    if(nNumber<getOptionalHeader_NumberOfRvaAndSizes()) // TODO Check!!!
    {
        if(is64())
        {
            write_IMAGE_DATA_DIRECTORY(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,DataDirectory)+nNumber*sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY),pDataDirectory);
        }
        else
        {
            write_IMAGE_DATA_DIRECTORY(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,DataDirectory)+nNumber*sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY),pDataDirectory);
        }
    }
}

void XPE::setOptionalHeader_DataDirectory_VirtualAddress(quint32 nNumber, quint32 nValue)
{
    if(nNumber<getOptionalHeader_NumberOfRvaAndSizes())
    {
        if(is64())
        {
            write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,DataDirectory)+nNumber*sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY)+offsetof(XPE_DEF::IMAGE_DATA_DIRECTORY,VirtualAddress),nValue);
        }
        else
        {
            write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,DataDirectory)+nNumber*sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY)+offsetof(XPE_DEF::IMAGE_DATA_DIRECTORY,VirtualAddress),nValue);
        }
    }
}

void XPE::setOptionalHeader_DataDirectory_Size(quint32 nNumber, quint32 nValue)
{
    if(nNumber<getOptionalHeader_NumberOfRvaAndSizes())
    {
        if(is64())
        {
            write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,DataDirectory)+nNumber*sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY)+offsetof(XPE_DEF::IMAGE_DATA_DIRECTORY,Size),nValue);
        }
        else
        {
            write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,DataDirectory)+nNumber*sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY)+offsetof(XPE_DEF::IMAGE_DATA_DIRECTORY,Size),nValue);
        }
    }
}

void XPE::clearOptionalHeader_DataDirectory(quint32 nNumber)
{
    XPE_DEF::IMAGE_DATA_DIRECTORY dd={};

    setOptionalHeader_DataDirectory(nNumber,&dd);
}

bool XPE::isOptionalHeader_DataDirectoryPresent(quint32 nNumber)
{
    XPE_DEF::IMAGE_DATA_DIRECTORY dd=getOptionalHeader_DataDirectory(nNumber);

    //    return (dd.Size)&&(dd.VirtualAddress)&&(isAddressValid(dd.VirtualAddress+getBaseAddress())); // TODO Check
    //    return (dd.Size)&&(dd.VirtualAddress);
    // TODO more checks
    return (dd.VirtualAddress);
}

QList<XPE_DEF::IMAGE_DATA_DIRECTORY> XPE::getDirectories()
{
    QList<XPE_DEF::IMAGE_DATA_DIRECTORY> listResult;

    int nNumberNumberOfRvaAndSizes=getOptionalHeader_NumberOfRvaAndSizes();
    nNumberNumberOfRvaAndSizes=qMin(nNumberNumberOfRvaAndSizes,16);

    qint64 nDirectoriesOffset=getOptionalHeaderOffset();

    if(is64())
    {
        nDirectoriesOffset+=offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,DataDirectory);
    }
    else
    {
        nDirectoriesOffset+=offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,DataDirectory);
    }

    for(int i=0; i<nNumberNumberOfRvaAndSizes; i++)
    {
        XPE_DEF::IMAGE_DATA_DIRECTORY record={};

        read_array(nDirectoriesOffset+i*sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY),(char *)&record,sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY));

        listResult.append(record);
    }

    return listResult;
}

void XPE::setDirectories(QList<XPE_DEF::IMAGE_DATA_DIRECTORY> *pListDirectories)
{
    int nNumberOfRvaAndSizes=getOptionalHeader_NumberOfRvaAndSizes();
    nNumberOfRvaAndSizes=qMin(nNumberOfRvaAndSizes,16);

    qint64 nDirectoriesOffset=getOptionalHeaderOffset();

    if(is64())
    {
        nDirectoriesOffset+=offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,DataDirectory);
    }
    else
    {
        nDirectoriesOffset+=offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,DataDirectory);
    }

    for(int i=0; i<nNumberOfRvaAndSizes; i++)
    {
        write_array(nDirectoriesOffset+i*sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY),(char *)&(pListDirectories->at(i)),sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY));
    }
}

qint64 XPE::getDataDirectoryOffset(quint32 nNumber)
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getDataDirectoryOffset(&memoryMap,nNumber);
}

qint64 XPE::getDataDirectoryOffset(XBinary::_MEMORY_MAP *pMemoryMap, quint32 nNumber)
{
    qint64 nResult=-1;

    XPE_DEF::IMAGE_DATA_DIRECTORY dataResources=getOptionalHeader_DataDirectory(nNumber);

    if(dataResources.VirtualAddress)
    {
        nResult=addressToOffset(pMemoryMap,dataResources.VirtualAddress+pMemoryMap->nBaseAddress);
    }

    return nResult;
}

QByteArray XPE::getDataDirectory(quint32 nNumber)
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getDataDirectory(&memoryMap,nNumber);
}

QByteArray XPE::getDataDirectory(XBinary::_MEMORY_MAP *pMemoryMap, quint32 nNumber)
{
    QByteArray baResult;

    XPE_DEF::IMAGE_DATA_DIRECTORY dataDirectory=getOptionalHeader_DataDirectory(nNumber);

    if(dataDirectory.VirtualAddress)
    {
        qint64 nOffset=addressToOffset(pMemoryMap,dataDirectory.VirtualAddress+pMemoryMap->nBaseAddress);

        if(nOffset!=-1)
        {
            baResult=read_array(nOffset,dataDirectory.Size);
        }
    }

    return baResult;
}

qint64 XPE::getSectionsTableOffset()
{
    qint64 nResult=-1;

    qint64 nOptionalHeaderOffset=getOptionalHeaderOffset();

    if(nOptionalHeaderOffset!=-1)
    {
        nResult=nOptionalHeaderOffset+getFileHeader_SizeOfOptionalHeader();
    }

    return nResult;
}

qint64 XPE::getSectionHeaderOffset(quint32 nNumber)
{
    qint64 nResult=-1;

    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        qint64 nSectionsTableOffset=getSectionsTableOffset();

        if(nSectionsTableOffset!=-1)
        {
            nResult=nSectionsTableOffset+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER);
        }
    }

    return nResult;
}

qint64 XPE::getSectionHeaderSize()
{
    return sizeof(XPE_DEF::IMAGE_SECTION_HEADER);
}

bool XPE::isSectionsTablePresent()
{
    return (bool)getFileHeader_NumberOfSections();
}

XPE_DEF::IMAGE_SECTION_HEADER XPE::getSectionHeader(quint32 nNumber)
{
    // TODO
    XPE_DEF::IMAGE_SECTION_HEADER result={};

    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        // TODO
        read_array(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER),(char *)&result,sizeof(XPE_DEF::IMAGE_SECTION_HEADER));
    }

    return result;
}

void XPE::setSectionHeader(quint32 nNumber, XPE_DEF::IMAGE_SECTION_HEADER *pSectionHeader)
{
    // TODO
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        write_array(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER),(char *)pSectionHeader,sizeof(XPE_DEF::IMAGE_SECTION_HEADER));
    }
}

QList<XPE_DEF::IMAGE_SECTION_HEADER> XPE::getSectionHeaders()
{
    QList<XPE_DEF::IMAGE_SECTION_HEADER> listResult;

    quint32 nNumberOfSections=getFileHeader_NumberOfSections();
    qint64 nSectionOffset=getSectionsTableOffset();

    // Fix
    if(nNumberOfSections>100)  // TODO const
    {
        nNumberOfSections=100;
    }

    for(int i=0; i<(int)nNumberOfSections; i++)
    {
        XPE_DEF::IMAGE_SECTION_HEADER record={};

        read_array(nSectionOffset+i*sizeof(XPE_DEF::IMAGE_SECTION_HEADER),(char *)&record,sizeof(XPE_DEF::IMAGE_SECTION_HEADER));

        listResult.append(record);
    }

    return listResult;
}

QList<XPE::SECTION_RECORD> XPE::getSectionRecords(QList<XPE_DEF::IMAGE_SECTION_HEADER> *pListSectionHeaders, bool bIsImage)
{
    QList<SECTION_RECORD> listResult;

    int nNumberOfSections=pListSectionHeaders->count();

    for(int i=0; i<nNumberOfSections; i++)
    {
        SECTION_RECORD record={};

        record.sName=QString((char *)pListSectionHeaders->at(i).Name);
        record.sName.resize(qMin(record.sName.length(),XPE_DEF::S_IMAGE_SIZEOF_SHORT_NAME));

        if(bIsImage)
        {
            record.nOffset=pListSectionHeaders->at(i).VirtualAddress;
        }
        else
        {
            record.nOffset=pListSectionHeaders->at(i).PointerToRawData;
        }
        record.nRVA=pListSectionHeaders->at(i).VirtualAddress;

        record.nSize=pListSectionHeaders->at(i).SizeOfRawData;
        record.nCharacteristics=pListSectionHeaders->at(i).Characteristics;

        listResult.append(record);
    }

    return listResult;
}

QList<QString> XPE::getSectionNames(QList<XPE::SECTION_RECORD> *pListSectionRecords)
{
    QList<QString> listResult;

    int nNumberOfSections=pListSectionRecords->count();

    for(int i=0; i<nNumberOfSections; i++)
    {
        listResult.append(pListSectionRecords->at(i).sName);
    }

    return listResult;
}

QList<XPE::SECTIONRVA_RECORD> XPE::getSectionRVARecords()
{
    QList<SECTIONRVA_RECORD> listResult;

    QList<XPE_DEF::IMAGE_SECTION_HEADER> listSH=getSectionHeaders();
    qint32 nSectionAlignment=getOptionalHeader_SectionAlignment();

    int nNumberOfSections=listSH.count();

    for(int i=0; i<nNumberOfSections; i++)
    {
        SECTIONRVA_RECORD record={};

        record.nRVA=listSH.at(i).VirtualAddress;
        record.nSize=S_ALIGN_UP(listSH.at(i).Misc.VirtualSize,nSectionAlignment);
        record.nCharacteristics=listSH.at(i).Characteristics;

        listResult.append(record);
    }

    return listResult;
}

QString XPE::getSection_NameAsString(quint32 nNumber)
{
    QString sResult;

    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    char cBuffer[9]={0};

    if(nNumber<nNumberOfSections)
    {
        XBinary::read_array(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,Name),cBuffer,8);
    }

    sResult.append(cBuffer);

    return sResult;
}

quint32 XPE::getSection_VirtualSize(quint32 nNumber)
{
    quint32 nResult=0;

    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        nResult=read_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,Misc.VirtualSize));
    }

    return nResult;
}

quint32 XPE::getSection_VirtualAddress(quint32 nNumber)
{
    quint32 nResult=0;

    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        nResult=read_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,VirtualAddress));
    }

    return nResult;
}

quint32 XPE::getSection_SizeOfRawData(quint32 nNumber)
{
    quint32 nResult=0;

    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        nResult=read_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,SizeOfRawData));
    }

    return nResult;
}

quint32 XPE::getSection_PointerToRawData(quint32 nNumber)
{
    quint32 nResult=0;

    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        nResult=read_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,PointerToRawData));
    }

    return nResult;
}

quint32 XPE::getSection_PointerToRelocations(quint32 nNumber)
{
    quint32 nResult=0;

    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        nResult=read_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,PointerToRelocations));
    }

    return nResult;
}

quint32 XPE::getSection_PointerToLinenumbers(quint32 nNumber)
{
    quint32 nResult=0;

    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        nResult=read_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,PointerToLinenumbers));
    }

    return nResult;
}

quint16 XPE::getSection_NumberOfRelocations(quint32 nNumber)
{
    quint16 nResult=0;

    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        nResult=read_uint16(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,NumberOfRelocations));
    }

    return nResult;
}

quint16 XPE::getSection_NumberOfLinenumbers(quint32 nNumber)
{
    quint16 nResult=0;

    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        nResult=read_uint16(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,NumberOfLinenumbers));
    }

    return nResult;
}

quint32 XPE::getSection_Characteristics(quint32 nNumber)
{
    quint32 nResult=0;

    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        nResult=read_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,Characteristics));
    }

    return nResult;
}

void XPE::setSection_NameAsString(quint32 nNumber, QString sName)
{
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    char cBuffer[9]={0};

    sName.resize(8);

    strcpy(cBuffer,sName.toLatin1().data());

    if(nNumber<nNumberOfSections)
    {
        XBinary::write_array(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,Name),cBuffer,8);
    }
}

void XPE::setSection_VirtualSize(quint32 nNumber, quint32 nValue)
{
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        write_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,Misc.VirtualSize),nValue);
    }
}

void XPE::setSection_VirtualAddress(quint32 nNumber, quint32 nValue)
{
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        write_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,VirtualAddress),nValue);
    }
}

void XPE::setSection_SizeOfRawData(quint32 nNumber, quint32 nValue)
{
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        write_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,SizeOfRawData),nValue);
    }
}

void XPE::setSection_PointerToRawData(quint32 nNumber, quint32 nValue)
{
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        write_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,PointerToRawData),nValue);
    }
}

void XPE::setSection_PointerToRelocations(quint32 nNumber, quint32 nValue)
{
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        write_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,PointerToRelocations),nValue);
    }
}

void XPE::setSection_PointerToLinenumbers(quint32 nNumber, quint32 nValue)
{
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        write_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,PointerToLinenumbers),nValue);
    }
}

void XPE::setSection_NumberOfRelocations(quint32 nNumber, quint16 nValue)
{
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        write_uint16(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,NumberOfRelocations),nValue);
    }
}

void XPE::setSection_NumberOfLinenumbers(quint32 nNumber, quint16 nValue)
{
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        write_uint16(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,NumberOfLinenumbers),nValue);
    }
}

void XPE::setSection_Characteristics(quint32 nNumber, quint32 nValue)
{
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        write_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,Characteristics),nValue);
    }
}

QString XPE::getSection_NameAsString(quint32 nNumber, QList<QString> *pListSectionNameStrings)
{
    QString sResult;

    if(nNumber<(quint32)pListSectionNameStrings->count())
    {
        sResult=pListSectionNameStrings->at(nNumber);
    }

    return sResult;
}

quint32 XPE::getSection_VirtualSize(quint32 nNumber, QList<XPE_DEF::IMAGE_SECTION_HEADER> *pListSectionHeaders)
{
    quint32 nResult=0;

    if(nNumber<(quint32)pListSectionHeaders->count())
    {
        nResult=pListSectionHeaders->at(nNumber).Misc.VirtualSize;
    }

    return nResult;
}

quint32 XPE::getSection_VirtualAddress(quint32 nNumber, QList<XPE_DEF::IMAGE_SECTION_HEADER> *pListSectionHeaders)
{
    quint32 nResult=0;

    if(nNumber<(quint32)pListSectionHeaders->count())
    {
        nResult=pListSectionHeaders->at(nNumber).VirtualAddress;
    }

    return nResult;
}

quint32 XPE::getSection_SizeOfRawData(quint32 nNumber, QList<XPE_DEF::IMAGE_SECTION_HEADER> *pListSectionHeaders)
{
    quint32 nResult=0;

    if(nNumber<(quint32)pListSectionHeaders->count())
    {
        nResult=pListSectionHeaders->at(nNumber).SizeOfRawData;
    }

    return nResult;
}

quint32 XPE::getSection_PointerToRawData(quint32 nNumber, QList<XPE_DEF::IMAGE_SECTION_HEADER> *pListSectionHeaders)
{
    quint32 nResult=0;

    if(nNumber<(quint32)pListSectionHeaders->count())
    {
        nResult=pListSectionHeaders->at(nNumber).PointerToRawData;
    }

    return nResult;
}

quint32 XPE::getSection_PointerToRelocations(quint32 nNumber, QList<XPE_DEF::IMAGE_SECTION_HEADER> *pListSectionHeaders)
{
    quint32 nResult=0;

    if(nNumber<(quint32)pListSectionHeaders->count())
    {
        nResult=pListSectionHeaders->at(nNumber).PointerToRelocations;
    }

    return nResult;
}

quint32 XPE::getSection_PointerToLinenumbers(quint32 nNumber, QList<XPE_DEF::IMAGE_SECTION_HEADER> *pListSectionHeaders)
{
    quint32 nResult=0;

    if(nNumber<(quint32)pListSectionHeaders->count())
    {
        nResult=pListSectionHeaders->at(nNumber).PointerToLinenumbers;
    }

    return nResult;
}

quint16 XPE::getSection_NumberOfRelocations(quint32 nNumber, QList<XPE_DEF::IMAGE_SECTION_HEADER> *pListSectionHeaders)
{
    quint16 nResult=0;

    if(nNumber<(quint32)pListSectionHeaders->count())
    {
        nResult=pListSectionHeaders->at(nNumber).NumberOfRelocations;
    }

    return nResult;
}

quint16 XPE::getSection_NumberOfLinenumbers(quint32 nNumber, QList<XPE_DEF::IMAGE_SECTION_HEADER> *pListSectionHeaders)
{
    quint16 nResult=0;

    if(nNumber<(quint32)pListSectionHeaders->count())
    {
        nResult=pListSectionHeaders->at(nNumber).NumberOfLinenumbers;
    }

    return nResult;
}

quint32 XPE::getSection_Characteristics(quint32 nNumber, QList<XPE_DEF::IMAGE_SECTION_HEADER> *pListSectionHeaders)
{
    quint32 nResult=0;

    if(nNumber<(quint32)pListSectionHeaders->count())
    {
        nResult=pListSectionHeaders->at(nNumber).Characteristics;
    }

    return nResult;
}

bool XPE::isSectionNamePresent(QString sSectionName)
{
    QList<XPE_DEF::IMAGE_SECTION_HEADER> listSectionHeaders=getSectionHeaders();

    return isSectionNamePresent(sSectionName,&listSectionHeaders);
}

bool XPE::isSectionNamePresent(QString sSectionName, QList<XPE_DEF::IMAGE_SECTION_HEADER> *pListSectionHeaders)
{
    bool bResult=false;

    int nNumberOfSections=pListSectionHeaders->count();

    for(int i=0; i<nNumberOfSections; i++)
    {
        QString _sSectionName=QString((char *)pListSectionHeaders->at(i).Name);
        _sSectionName.resize(qMin(_sSectionName.length(),XPE_DEF::S_IMAGE_SIZEOF_SHORT_NAME));

        if(_sSectionName==sSectionName)
        {
            bResult=true;
            break;
        }
    }

    return bResult;
}

XPE_DEF::IMAGE_SECTION_HEADER XPE::getSectionByName(QString sSectionName, QList<XPE_DEF::IMAGE_SECTION_HEADER> *pListSectionHeaders)
{
    XPE_DEF::IMAGE_SECTION_HEADER result={};

    int nNumberOfSections=pListSectionHeaders->count();

    for(int i=0; i<nNumberOfSections; i++)
    {
        QString _sSectionName=QString((char *)pListSectionHeaders->at(i).Name);
        _sSectionName.resize(qMin(_sSectionName.length(),XPE_DEF::S_IMAGE_SIZEOF_SHORT_NAME));

        if(_sSectionName==sSectionName)
        {
            result=pListSectionHeaders->at(i);

            break;
        }
    }

    return result;
}

qint32 XPE::getSectionNumber(QString sSectionName)
{
    QList<XPE_DEF::IMAGE_SECTION_HEADER> listSectionHeaders=getSectionHeaders();

    return getSectionNumber(sSectionName,&listSectionHeaders);
}

qint32 XPE::getSectionNumber(QString sSectionName, QList<XPE_DEF::IMAGE_SECTION_HEADER> *pListSectionHeaders)
{
    qint32 nResult=-1;

    int nNumberOfSections=pListSectionHeaders->count();

    for(int i=0; i<nNumberOfSections; i++)
    {
        QString _sSectionName=QString((char *)pListSectionHeaders->at(i).Name);
        _sSectionName.resize(qMin(_sSectionName.length(),XPE_DEF::S_IMAGE_SIZEOF_SHORT_NAME));

        if(_sSectionName==sSectionName)
        {
            nResult=i;
            break;
        }
    }

    return nResult;
}

bool XPE::isImportPresent()
{
    return isOptionalHeader_DataDirectoryPresent(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_IMPORT);
}

XBinary::_MEMORY_MAP XPE::getMemoryMap()
{
    _MEMORY_MAP result={};

    qint32 nIndex=0;

    result.mode=getMode();

    if(result.mode==MODE_64)
    {
        result.fileType=FT_PE64;
    }
    else
    {
        result.fileType=FT_PE32;
    }

    result.sArch=getArch();
    result.bIsBigEndian=isBigEndian();
    result.sType=getTypeAsString();

    result.nBaseAddress=_getBaseAddress();
    result.nRawSize=getSize();
    result.nImageSize=S_ALIGN_UP(getOptionalHeader_SizeOfImage(),0x1000);

    quint32 nNumberOfSections=qMin((int)getFileHeader_NumberOfSections(),100); // TODO const
    quint32 nFileAlignment=getOptionalHeader_FileAlignment();
    quint32 nSectionAlignment=getOptionalHeader_SectionAlignment();
    //qint64 nBaseAddress=getOptionalHeader_ImageBase();
    quint32 nHeadersSize=getOptionalHeader_SizeOfHeaders(); // mb TODO calc for UPX

    if(nFileAlignment>0x10000) // Invalid file
    {
        nFileAlignment=0x200;
    }

    if(nSectionAlignment>0x10000) // Invalid file
    {
        nSectionAlignment=0x1000;
    }

    if(nHeadersSize>(quint64)getSize())
    {
        nHeadersSize=(quint32)getSize();
    }

    if(nFileAlignment==nSectionAlignment)
    {
        nFileAlignment=1;
    }

    quint32 nVirtualSizeofHeaders=S_ALIGN_UP(nHeadersSize,nSectionAlignment);
    qint64 nMaxOffset=0;

    // Check Format
    bool bValid=false;

    if(nHeadersSize!=0)
    {
        bValid=true;
    }

    if(bValid)
    {
        _MEMORY_RECORD recordHeaderRaw={};

        QString sHeaderName=QString("PE %1").arg(tr("Header"));

        if(!isImage())
        {
            recordHeaderRaw.type=MMT_HEADER;
            recordHeaderRaw.nAddress=result.nBaseAddress;
            recordHeaderRaw.segment=ADDRESS_SEGMENT_FLAT;
            recordHeaderRaw.nOffset=0;
            recordHeaderRaw.nSize=nHeadersSize;
            recordHeaderRaw.sName=sHeaderName;
            recordHeaderRaw.nIndex=nIndex++;

            result.listRecords.append(recordHeaderRaw);

            if(nVirtualSizeofHeaders-nHeadersSize)
            {
                _MEMORY_RECORD record={};
                record.type=MMT_HEADER;
                record.bIsVirtual=true;

                record.nAddress=result.nBaseAddress+nHeadersSize;
                recordHeaderRaw.segment=ADDRESS_SEGMENT_FLAT;
                record.nOffset=-1;
                record.nSize=nVirtualSizeofHeaders-nHeadersSize;
                record.sName=sHeaderName;
                record.nIndex=nIndex++;

                result.listRecords.append(record);
            }
        }
        else
        {
            recordHeaderRaw.type=MMT_HEADER;
            recordHeaderRaw.nAddress=result.nBaseAddress;
            recordHeaderRaw.segment=ADDRESS_SEGMENT_FLAT;
            recordHeaderRaw.nOffset=0;
            recordHeaderRaw.nSize=nVirtualSizeofHeaders;
            recordHeaderRaw.sName=sHeaderName;
            recordHeaderRaw.nIndex=nIndex++;

            result.listRecords.append(recordHeaderRaw);
        }

        nMaxOffset=recordHeaderRaw.nSize;

        for(quint32 i=0; i<nNumberOfSections; i++)
        {
            XPE_DEF::IMAGE_SECTION_HEADER section=getSectionHeader(i);

            // TODO for corrupted files
            if(section.SizeOfRawData>result.nRawSize)
            {
                break; // TODO
            }

            qint64 nFileOffset=section.PointerToRawData;
            //
            nFileOffset=S_ALIGN_DOWN(nFileOffset,nFileAlignment);
            //        qint64 nFileSize=__ALIGN_UP(section.SizeOfRawData,nFileAlignment);
            qint64 nFileSize=section.SizeOfRawData+(section.PointerToRawData-nFileOffset);
            qint64 nVirtualAddress=result.nBaseAddress+section.VirtualAddress;
            qint64 nVirtualSize=S_ALIGN_UP(section.Misc.VirtualSize,nSectionAlignment);

            if(!isImage())
            {
                if(nFileSize)
                {
                    nMaxOffset=qMax(nMaxOffset,(qint64)(nFileOffset+nFileSize));
                }
            }
            else
            {
                if(nVirtualSize)
                {
                    nMaxOffset=qMax(nMaxOffset,(qint64)(nVirtualAddress+nVirtualSize));
                }
            }

            QString _sSectionName=QString((char *)section.Name);

            if(_sSectionName.size()>8)
            {
                _sSectionName.resize(8);
            }

            QString sSectionName=QString("%1(%2)['%3']").arg(tr("Section")).arg(i).arg(_sSectionName);

            if(!isImage())
            {
                if(nFileSize)
                {
                    _MEMORY_RECORD record={};

                    record.type=MMT_LOADSECTION;
                    record.nLoadSection=i;
                    record.segment=ADDRESS_SEGMENT_FLAT;
                    record.nAddress=nVirtualAddress;
                    record.nOffset=nFileOffset;
                    record.nSize=nFileSize;
                    record.sName=sSectionName;
                    record.nIndex=nIndex++;

                    result.listRecords.append(record);
                }

                if(nVirtualSize-nFileSize)
                {
                    _MEMORY_RECORD record={};
                    record.bIsVirtual=true;

                    record.type=MMT_LOADSECTION;
                    record.nLoadSection=i;
                    record.segment=ADDRESS_SEGMENT_FLAT;
                    record.nAddress=nVirtualAddress+nFileSize;
                    record.nOffset=-1;
                    record.nSize=nVirtualSize-nFileSize;
                    record.sName=sSectionName;
                    record.nIndex=nIndex++;

                    result.listRecords.append(record);
                }
            }
            else
            {
                _MEMORY_RECORD record={};

                record.type=MMT_LOADSECTION;
                record.nLoadSection=i;
                record.segment=ADDRESS_SEGMENT_FLAT;
                record.nAddress=nVirtualAddress;
                record.nOffset=nVirtualAddress-result.nBaseAddress;
                record.nSize=nVirtualSize;
                record.sName=sSectionName;
                record.nIndex=nIndex++;

                result.listRecords.append(record);
            }
        }

        if(!isImage())
        {
            // Overlay;
            _MEMORY_RECORD record={};

            record.type=MMT_OVERLAY;

            record.nAddress=-1;
            record.segment=ADDRESS_SEGMENT_UNKNOWN;
            record.nOffset=nMaxOffset;

            record.nSize=qMax(getSize()-nMaxOffset,(qint64)0);
            record.sName=tr("Overlay");
            record.nIndex=nIndex++;

            if(record.nSize)
            {
                result.listRecords.append(record);
            }
        }
    }

    return result;
}

qint64 XPE::getBaseAddress()
{
    return (qint64)getOptionalHeader_ImageBase();
}

void XPE::setBaseAddress(qint64 nBaseAddress)
{
    setOptionalHeader_ImageBase(nBaseAddress);
}

qint64 XPE::getEntryPointOffset(_MEMORY_MAP *pMemoryMap)
{
    return addressToOffset(pMemoryMap,pMemoryMap->nBaseAddress+getOptionalHeader_AddressOfEntryPoint());
}

void XPE::setEntryPointOffset(qint64 nEntryPointOffset)
{
    setOptionalHeader_AddressOfEntryPoint(offsetToAddress(nEntryPointOffset)-_getBaseAddress());
}

QList<XPE::IMPORT_RECORD> XPE::getImportRecords()
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getImportRecords(&memoryMap);
}

QList<XPE::IMPORT_RECORD> XPE::getImportRecords(_MEMORY_MAP *pMemoryMap)
{
    QList<IMPORT_RECORD> listResult;

    qint64 nImportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_IMPORT);

    if(nImportOffset!=-1)
    {
        bool bIs64=is64(pMemoryMap);

        while(true)
        {
            XPE_DEF::IMAGE_IMPORT_DESCRIPTOR iid=read_IMAGE_IMPORT_DESCRIPTOR(nImportOffset);

            QString sLibrary;

            if((iid.Characteristics==0)&&(iid.Name==0))
            {
                break;
            }

            qint64 nOffset=addressToOffset(pMemoryMap,iid.Name+pMemoryMap->nBaseAddress);

            if(nOffset!=-1)
            {
                sLibrary=read_ansiString(nOffset);

                if(sLibrary=="")
                {
                    break;
                }
            }
            else
            {
                break; // corrupted
            }

            qint64 nThunksOffset=-1;
            qint64 nRVA=0;

            if(iid.OriginalFirstThunk)
            {
                nThunksOffset=addressToOffset(pMemoryMap,iid.OriginalFirstThunk+pMemoryMap->nBaseAddress);
                //                nRVA=iid.OriginalFirstThunk;
            }
            else if((iid.FirstThunk))
            {
                nThunksOffset=addressToOffset(pMemoryMap,iid.FirstThunk+pMemoryMap->nBaseAddress);
                //                nRVA=iid.FirstThunk;
            }

            nRVA=iid.FirstThunk;

            if(nThunksOffset==-1)
            {
                break;
            }

            while(true)
            {
                QString sFunction;

                if(bIs64)
                {
                    qint64 nThunk64=read_uint64(nThunksOffset);

                    if(nThunk64==0)
                    {
                        break;
                    }

                    if(!(nThunk64&0x8000000000000000))
                    {
                        qint64 nOffset=addressToOffset(pMemoryMap,nThunk64+pMemoryMap->nBaseAddress);

                        if(nOffset!=-1)
                        {
                            sFunction=read_ansiString(nOffset+2);

                            if(sFunction=="")
                            {
                                break;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        sFunction=QString("%1").arg(nThunk64&0x7FFFFFFFFFFFFFFF);
                    }
                }
                else
                {
                    qint64 nThunk32=read_uint32(nThunksOffset);

                    if(nThunk32==0)
                    {
                        break;
                    }

                    if(!(nThunk32&0x80000000))
                    {
                        qint64 nOffset=addressToOffset(pMemoryMap,nThunk32+pMemoryMap->nBaseAddress);

                        if(nOffset!=-1)
                        {
                            sFunction=read_ansiString(nOffset+2);

                            if(sFunction=="")
                            {
                                break;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        sFunction=QString("%1").arg(nThunk32&0x7FFFFFFF);
                    }
                }

                IMPORT_RECORD record;

                record.nOffset=nThunksOffset;
                record.nRVA=nRVA;
                record.sLibrary=sLibrary;
                record.sFunction=sFunction;

                listResult.append(record);

                if(bIs64)
                {
                    nThunksOffset+=8;
                    nRVA+=8; // quint64
                }
                else
                {
                    nThunksOffset+=4;
                    nRVA+=4; // quint32
                }
            }

            nImportOffset+=sizeof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR);
        }
    }

    return listResult;
}

quint64 XPE::getImportHash64(_MEMORY_MAP *pMemoryMap)
{
    quint64 nResult=0;

    QList<IMPORT_RECORD> listImportRecords=getImportRecords(pMemoryMap);

    int nNumberOfImports=listImportRecords.count();

    for(int i=0;i<nNumberOfImports; i++)
    {
        QString sRecord=listImportRecords.at(i).sLibrary+" "+listImportRecords.at(i).sFunction;

        nResult+=getStringCustomCRC32(sRecord);
    }

    return nResult;
}

quint32 XPE::getImportHash32(_MEMORY_MAP *pMemoryMap)
{
    quint64 nResult=0;

    QList<IMPORT_RECORD> listImportRecords=getImportRecords(pMemoryMap);

    int nNumberOfImports=listImportRecords.count();

    QString sRecord;

    for(int i=0;i<nNumberOfImports; i++)
    {
        sRecord+=listImportRecords.at(i).sLibrary+listImportRecords.at(i).sFunction;
    }

    nResult=getStringCustomCRC32(sRecord);

    return nResult;
}

qint64 XPE::getImportDescriptorOffset(quint32 nNumber)
{
    qint64 nResult=-1;

    qint64 nImportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_IMPORT);

    if(nImportOffset!=-1)
    {
        nResult=nImportOffset+nNumber*sizeof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR);
    }

    return nResult;
}

qint64 XPE::getImportDescriptorSize()
{
    return sizeof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR);
}

QList<XPE_DEF::IMAGE_IMPORT_DESCRIPTOR> XPE::getImportDescriptors()
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getImportDescriptors(&memoryMap);
}

QList<XPE_DEF::IMAGE_IMPORT_DESCRIPTOR> XPE::getImportDescriptors(XBinary::_MEMORY_MAP *pMemoryMap)
{
    QList<XPE_DEF::IMAGE_IMPORT_DESCRIPTOR> listResult;

    qint64 nImportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_IMPORT);

    if(nImportOffset!=-1)
    {
        while(true)
        {
            XPE_DEF::IMAGE_IMPORT_DESCRIPTOR iid=read_IMAGE_IMPORT_DESCRIPTOR(nImportOffset);

            if((iid.Characteristics==0)&&(iid.Name==0))
            {
                break;
            }

            qint64 nOffset=addressToOffset(pMemoryMap,iid.Name+pMemoryMap->nBaseAddress);

            if(nOffset!=-1)
            {
                QString sName=read_ansiString(nOffset);

                if(sName=="")
                {
                    break;
                }
            }
            else
            {
                break; // corrupted
            }

            listResult.append(iid);

            nImportOffset+=sizeof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR);
        }
    }

    return listResult;
}

QList<XPE::IMAGE_IMPORT_DESCRIPTOR_EX> XPE::getImportDescriptorsEx()
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getImportDescriptorsEx(&memoryMap);
}

QList<XPE::IMAGE_IMPORT_DESCRIPTOR_EX> XPE::getImportDescriptorsEx(XBinary::_MEMORY_MAP *pMemoryMap)
{
    QList<IMAGE_IMPORT_DESCRIPTOR_EX> listResult;

    qint64 nImportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_IMPORT);

    if(nImportOffset!=-1)
    {
        while(true)
        {
            IMAGE_IMPORT_DESCRIPTOR_EX record={};
            XPE_DEF::IMAGE_IMPORT_DESCRIPTOR iid=read_IMAGE_IMPORT_DESCRIPTOR(nImportOffset);

            if((iid.Characteristics==0)&&(iid.Name==0))
            {
                break;
            }

            qint64 nOffset=addressToOffset(pMemoryMap,iid.Name+pMemoryMap->nBaseAddress);

            if(nOffset!=-1)
            {
                record.sLibrary=read_ansiString(nOffset);

                if(record.sLibrary=="")
                {
                    break;
                }
            }
            else
            {
                break; // corrupted
            }

            record.Characteristics=iid.Characteristics;
            record.FirstThunk=iid.FirstThunk;
            record.ForwarderChain=iid.ForwarderChain;
            record.Name=iid.Name;
            record.OriginalFirstThunk=iid.OriginalFirstThunk;
            record.TimeDateStamp=iid.TimeDateStamp;

            listResult.append(record);

            nImportOffset+=sizeof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR);
        }
    }

    return listResult;
}

XPE_DEF::IMAGE_IMPORT_DESCRIPTOR XPE::getImportDescriptor(quint32 nNumber)
{
    XPE_DEF::IMAGE_IMPORT_DESCRIPTOR result={};

    qint64 nImportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_IMPORT);

    if(nImportOffset!=-1)
    {
        nImportOffset+=nNumber*sizeof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR);

        result=read_IMAGE_IMPORT_DESCRIPTOR(nImportOffset);
    }

    return result;
}

void XPE::setImportDescriptor(quint32 nNumber, XPE_DEF::IMAGE_IMPORT_DESCRIPTOR *pImportDescriptor)
{
    qint64 nImportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_IMPORT);

    if(nImportOffset!=-1)
    {
        nImportOffset+=nNumber*sizeof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR);

        write_IMAGE_IMPORT_DESCRIPTOR(nImportOffset,*pImportDescriptor);
    }
}

void XPE::setImportDescriptor_OriginalFirstThunk(quint32 nNumber, quint32 nValue)
{
    qint64 nOffset=getImportDescriptorOffset(nNumber);

    write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR,OriginalFirstThunk),nValue);
}

void XPE::setImportDescriptor_TimeDateStamp(quint32 nNumber, quint32 nValue)
{
    qint64 nOffset=getImportDescriptorOffset(nNumber);

    write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR,TimeDateStamp),nValue);
}

void XPE::setImportDescriptor_ForwarderChain(quint32 nNumber, quint32 nValue)
{
    qint64 nOffset=getImportDescriptorOffset(nNumber);

    write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR,ForwarderChain),nValue);
}

void XPE::setImportDescriptor_Name(quint32 nNumber, quint32 nValue)
{
    qint64 nOffset=getImportDescriptorOffset(nNumber);

    write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR,Name),nValue);
}

void XPE::setImportDescriptor_FirstThunk(quint32 nNumber, quint32 nValue)
{
    qint64 nOffset=getImportDescriptorOffset(nNumber);

    write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR,FirstThunk),nValue);
}

QList<XPE::IMPORT_HEADER> XPE::getImports()
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getImports(&memoryMap);
}

QList<XPE::IMPORT_HEADER> XPE::getImports(XBinary::_MEMORY_MAP *pMemoryMap)
{
    QList<IMPORT_HEADER> listResult;

    XPE_DEF::IMAGE_DATA_DIRECTORY dataResources=getOptionalHeader_DataDirectory(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_IMPORT);

    qint64 nBaseAddress=_getBaseAddress();
    qint64 nImportOffset=-1;
    qint64 nImportOffsetTest=-1;

    if(dataResources.VirtualAddress)
    {
        nImportOffset=addressToOffset(pMemoryMap,dataResources.VirtualAddress+nBaseAddress);
        nImportOffsetTest=addressToOffset(pMemoryMap,dataResources.VirtualAddress+nBaseAddress+sizeof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR)-2); // Test for some (Win)Upack stubs
    }

    if(nImportOffset!=-1)
    {
        while(true)
        {
            XPE_DEF::IMAGE_IMPORT_DESCRIPTOR iid=read_IMAGE_IMPORT_DESCRIPTOR(nImportOffset);

            IMPORT_HEADER importHeader={};

            if(nImportOffsetTest==-1)
            {
                iid.FirstThunk&=0x0000FFFF;
            }

            if((iid.Characteristics==0)&&(iid.Name==0))
            {
                break;
            }

            qint64 nOffset=addressToOffset(pMemoryMap,iid.Name+nBaseAddress);

            if(nOffset!=-1)
            {
                importHeader.sName=read_ansiString(nOffset);

                if(importHeader.sName=="")
                {
                    break;
                }
            }
            else
            {
                break; // corrupted
            }

            qint64 nThunksOffset=-1;
            qint64 nThunksRVA=0;
            qint64 nThunksOriginalRVA=0;
//          qint64 nThunksOriginalOffset=0;

            if(iid.OriginalFirstThunk)
            {
                nThunksRVA=iid.OriginalFirstThunk;
                //                nRVA=iid.OriginalFirstThunk;
            }
            else if((iid.FirstThunk))
            {
                nThunksRVA=iid.FirstThunk;
                //                nRVA=iid.FirstThunk;
            }

            nThunksOriginalRVA=iid.FirstThunk;

            nThunksOffset=addressToOffset(pMemoryMap,nThunksRVA+nBaseAddress);
//            nThunksOriginalOffset=addressToOffset(pMemoryMap,nThunksOriginalRVA+nBaseAddress);

            if(nThunksOffset!=-1)
            {
                importHeader.listPositions=_getImportPositions(pMemoryMap,nThunksRVA,nThunksOriginalRVA);
            }

            listResult.append(importHeader);

            nImportOffset+=sizeof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR);
        }
    }

    return listResult;
}

QList<XPE::IMPORT_POSITION> XPE::_getImportPositions(XBinary::_MEMORY_MAP *pMemoryMap, qint64 nThunksRVA,qint64 nRVA)
{
    QList<IMPORT_POSITION> listResult;

    qint64 nThunksOffset=XBinary::relAddressToOffset(pMemoryMap,nThunksRVA);

    bool bIs64=is64(pMemoryMap);

    while(true)
    {
        IMPORT_POSITION importPosition={};
        importPosition.nThunkOffset=nThunksOffset;
        importPosition.nThunkRVA=nThunksRVA;

        if(bIs64)
        {
            importPosition.nThunkValue=read_uint64(nThunksOffset);

            if(importPosition.nThunkValue==0)
            {
                break;
            }

            if(!(importPosition.nThunkValue&0x8000000000000000))
            {
                qint64 nOffset=addressToOffset(pMemoryMap,importPosition.nThunkValue+pMemoryMap->nBaseAddress);

                if(nOffset!=-1)
                {
                    importPosition.nHint=read_uint16(nOffset);
                    importPosition.sName=read_ansiString(nOffset+2);

                    if(importPosition.sName=="")
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
            else
            {
                importPosition.nOrdinal=importPosition.nThunkValue&0x7FFFFFFFFFFFFFFF;
            }
        }
        else
        {
            importPosition.nThunkValue=read_uint32(nThunksOffset);

            if(importPosition.nThunkValue==0)
            {
                break;
            }

            if(!(importPosition.nThunkValue&0x80000000))
            {
                qint64 nOffset=addressToOffset(pMemoryMap,importPosition.nThunkValue+pMemoryMap->nBaseAddress);

                if(nOffset!=-1)
                {
                    importPosition.nHint=read_uint16(nOffset);
                    importPosition.sName=read_ansiString(nOffset+2);

                    if(importPosition.sName=="")
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
            else
            {
                importPosition.nOrdinal=importPosition.nThunkValue&0x7FFFFFFF;
            }
        }

        if(importPosition.nOrdinal==0)
        {
            importPosition.sFunction=importPosition.sName;
        }
        else
        {
            importPosition.sFunction=QString("%1").arg(importPosition.nOrdinal);
        }

        if(bIs64)
        {
            nThunksRVA+=8;
            nThunksOffset+=8;
            nRVA+=8;
        }
        else
        {
            nThunksRVA+=4;
            nThunksOffset+=4;
            nRVA+=4;
        }

        listResult.append(importPosition);
    }

    return listResult;
}

QList<XPE::IMPORT_POSITION> XPE::getImportPositions(int nIndex)
{
    QList<IMPORT_POSITION> listResult;

    qint64 nImportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_IMPORT);

    if(nImportOffset!=-1)
    {
        _MEMORY_MAP memoryMap=getMemoryMap();

        int _nIndex=0;

        while(true)
        {
            IMPORT_HEADER importHeader={};
            XPE_DEF::IMAGE_IMPORT_DESCRIPTOR iid=read_IMAGE_IMPORT_DESCRIPTOR(nImportOffset);

            if((iid.Characteristics==0)&&(iid.Name==0))
            {
                break;
            }

            qint64 nOffset=addressToOffset(&memoryMap,iid.Name+memoryMap.nBaseAddress);

            if(nOffset!=-1)
            {
                importHeader.sName=read_ansiString(nOffset);

                if(importHeader.sName=="")
                {
                    break;
                }
            }
            else
            {
                break; // corrupted
            }

            qint64 nThunksOffset=-1;
            qint64 nRVA=0;
            qint64 nThunksRVA=-1;

            if(iid.OriginalFirstThunk)
            {
                nThunksRVA=iid.OriginalFirstThunk;
            }
            else if((iid.FirstThunk))
            {
                nThunksRVA=iid.FirstThunk;
            }

            nRVA=iid.FirstThunk;
            nThunksOffset=relAddressToOffset(&memoryMap,nThunksRVA);

            if(nThunksOffset==-1)
            {
                break;
            }

            if(_nIndex==nIndex)
            {
                listResult=_getImportPositions(&memoryMap,nThunksRVA,nRVA);

                break;
            }

            nImportOffset+=sizeof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR);
            _nIndex++;
        }
    }

    return listResult;
}

QList<quint32> XPE::getImportPositionHashes(_MEMORY_MAP *pMemoryMap)
{
    QList<quint32> listResult;

    QList<IMPORT_HEADER> listImportHeaders=getImports(pMemoryMap);

    int nNumberOfImports=listImportHeaders.count();

    for(int i=0;i<nNumberOfImports;i++)
    {
        IMPORT_HEADER record=listImportHeaders.at(i);

        int nNumberOfPositions=record.listPositions.count();

        QString sString;

        for(int j=0;j<nNumberOfPositions;j++)
        {
            sString+=record.sName;
            sString+=record.listPositions.at(j).sFunction;
        }

        listResult.append(getStringCustomCRC32(sString));
    }

    return listResult;
}

bool XPE::isImportLibraryPresentI(QString sLibrary)
{
    QList<IMPORT_HEADER> listImportHeaders=getImports();

    return isImportLibraryPresentI(sLibrary,&listImportHeaders);
}

bool XPE::isImportLibraryPresentI(QString sLibrary, QList<XPE::IMPORT_HEADER> *pListImportHeaders)
{
    bool bResult=false;

    int nNumberOfImports=pListImportHeaders->count();

    for(int i=0; i<nNumberOfImports; i++)
    {
        if(pListImportHeaders->at(i).sName.toUpper()==sLibrary.toUpper())
        {
            bResult=true;
            break;
        }
    }

    return bResult;
}

bool XPE::isImportFunctionPresentI(QString sLibrary, QString sFunction)
{
    QList<IMPORT_HEADER> listImportHeaders=getImports();

    return isImportFunctionPresentI(sLibrary,sFunction,&listImportHeaders);
}

bool XPE::isImportFunctionPresentI(QString sLibrary, QString sFunction, QList<XPE::IMPORT_HEADER> *pListImportHeaders)
{
    bool bResult=false;

    // TODO Optimize!

    int nNumberOfImports=pListImportHeaders->count();

    for(int i=0; i<nNumberOfImports; i++)
    {
        if(pListImportHeaders->at(i).sName.toUpper()==sLibrary.toUpper())
        {
            int nNumberOfPositions=pListImportHeaders->at(i).listPositions.count();

            for(int j=0;j<nNumberOfPositions;j++)
            {
                if(pListImportHeaders->at(i).listPositions.at(j).sFunction==sFunction)
                {
                    bResult=true;
                    break;
                }
            }
        }
    }

    return bResult;
}

bool XPE::setImports(QList<XPE::IMPORT_HEADER> *pListImportHeaders)
{
    return setImports(getDevice(),isImage(),pListImportHeaders);
}

bool XPE::setImports(QIODevice *pDevice, bool bIsImage, QList<XPE::IMPORT_HEADER> *pListImportHeaders)
{
    bool bResult=false;

    if(isResizeEnable(pDevice))
    {
        XPE pe(pDevice,bIsImage);

        if(pe.isValid())
        {
            int nAddressSize=4;

            if(pe.is64())
            {
                nAddressSize=8;
            }
            else
            {
                nAddressSize=4;
            }

            QByteArray baImport;
            QList<qint64> listPatches; // Addresses for patch
            //    QMap<qint64,qint64> mapMove;

            // Calculate
            quint32 nIATSize=0;
            quint32 nImportTableSize=(pListImportHeaders->count()+1)*sizeof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR);
            quint32 nAnsiDataSize=0;

            int nNumberOfHeaders=pListImportHeaders->count();

            for(int i=0; i<nNumberOfHeaders; i++)
            {
                // TODO 64
                int nNumberOfPositions=pListImportHeaders->at(i).listPositions.count();

                nIATSize+=(nNumberOfPositions+1)*nAddressSize;
                nAnsiDataSize+=pListImportHeaders->at(i).sName.length()+3;

                for(int j=0; j<nNumberOfPositions; j++)
                {
                    if(pListImportHeaders->at(i).listPositions.at(j).sName!="")
                    {
                        nAnsiDataSize+=2+pListImportHeaders->at(i).listPositions.at(j).sName.length()+1;
                    }
                }
            }

            nImportTableSize=S_ALIGN_UP(nImportTableSize,16);
            nIATSize=S_ALIGN_UP(nIATSize,16);
            nAnsiDataSize=S_ALIGN_UP(nAnsiDataSize,16);

            baImport.resize(nIATSize+nImportTableSize+nIATSize+nAnsiDataSize);
            baImport.fill(0);

            char *pDataOffset=baImport.data();
            char *pIAT=pDataOffset;
            XPE_DEF::IMAGE_IMPORT_DESCRIPTOR *pIID=(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR *)(pDataOffset+nIATSize);
            char *pOIAT=pDataOffset+nIATSize+nImportTableSize;
            char *pAnsiData=pDataOffset+nIATSize+nImportTableSize+nIATSize;

            nNumberOfHeaders=pListImportHeaders->count();

            for(int i=0; i<nNumberOfHeaders; i++)
            {
                pIID->FirstThunk=pIAT-pDataOffset;
                listPatches.append((char *)pIID-pDataOffset+offsetof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR,FirstThunk));

                pIID->Name=pAnsiData-pDataOffset;
                listPatches.append((char *)pIID-pDataOffset+offsetof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR,Name));

                pIID->OriginalFirstThunk=pOIAT-pDataOffset;
                listPatches.append((char *)pIID-pDataOffset+offsetof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR,OriginalFirstThunk));

                strcpy(pAnsiData,pListImportHeaders->at(i).sName.toLatin1().data());
                pAnsiData+=pListImportHeaders->at(i).sName.length()+3;

                int nNumberOfPositions=pListImportHeaders->at(i).listPositions.count();

                for(int j=0; j<nNumberOfPositions; j++)
                {
                    if(pListImportHeaders->at(i).listPositions.at(j).sName!="")
                    {
                        *((quint32 *)pOIAT)=pAnsiData-pDataOffset;
                        *((quint32 *)pIAT)=*((quint32 *)pOIAT);

                        listPatches.append(pOIAT-pDataOffset);
                        listPatches.append(pIAT-pDataOffset);

                        *((quint16 *)pAnsiData)=pListImportHeaders->at(i).listPositions.at(j).nHint;
                        pAnsiData+=2;

                        strcpy(pAnsiData,pListImportHeaders->at(i).listPositions.at(j).sName.toLatin1().data());

                        pAnsiData+=pListImportHeaders->at(i).listPositions.at(j).sName.length()+1;
                    }
                    else
                    {
                        // TODO 64
                        if(nAddressSize==4)
                        {
                            *((quint32 *)pOIAT)=pListImportHeaders->at(i).listPositions.at(j).nOrdinal+0x80000000;
                            *((quint32 *)pIAT)=*((quint32 *)pOIAT);
                        }
                        else
                        {
                            *((quint64 *)pOIAT)=pListImportHeaders->at(i).listPositions.at(j).nOrdinal+0x8000000000000000;
                            *((quint64 *)pIAT)=*((quint64 *)pOIAT);
                        }
                    }

                    //            if(pListHeaders->at(i).nFirstThunk)
                    //            {
                    //                mapMove.insert(pListHeaders->at(i).listPositions.at(j).nThunkRVA,pIAT-pDataOffset);
                    //            }

                    pIAT+=nAddressSize;
                    pOIAT+=nAddressSize;
                }

                pIAT+=nAddressSize;
                pOIAT+=nAddressSize;
                pIID++;
            }

            XPE_DEF::IMAGE_SECTION_HEADER ish={};

            ish.Characteristics=0xc0000040;

            // TODO section name!!!
            if(addSection(pDevice,bIsImage,&ish,baImport.data(),baImport.size()))
            {
                _MEMORY_MAP memoryMap=pe.getMemoryMap();

                XPE_DEF::IMAGE_DATA_DIRECTORY iddIAT={};
                iddIAT.VirtualAddress=ish.VirtualAddress;
                iddIAT.Size=nIATSize;
                XPE_DEF::IMAGE_DATA_DIRECTORY iddImportTable={};
                iddImportTable.VirtualAddress=nIATSize+ish.VirtualAddress;
                iddImportTable.Size=nImportTableSize;

                pe.setOptionalHeader_DataDirectory(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_IAT,&iddIAT);
                pe.setOptionalHeader_DataDirectory(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_IMPORT,&iddImportTable);

                int nNumberOfPatches=listPatches.count();

                for(int i=0; i<nNumberOfPatches; i++)
                {
                    // TODO 64
                    qint64 nCurrentOffset=ish.PointerToRawData+listPatches.at(i);
                    quint32 nValue=pe.read_uint32(nCurrentOffset);
                    pe.write_uint32(nCurrentOffset,nValue+ish.VirtualAddress);
                }

                int _nNumberOfHeaders=pListImportHeaders->count();

                for(int i=0; i<_nNumberOfHeaders; i++)
                {
                    if(pListImportHeaders->at(i).nFirstThunk)
                    {
                        XPE_DEF::IMAGE_IMPORT_DESCRIPTOR iid=pe.getImportDescriptor(i);

                        //                        qDebug("pListHeaders->at(i).nFirstThunk(%d): %x",i,(quint32)pListHeaders->at(i).nFirstThunk);
                        //                        qDebug("FirstThunk(%d): %x",i,(quint32)iid.FirstThunk);
                        //                        qDebug("Import offset(%d): %x",i,(quint32)pe.getDataDirectoryOffset(XPE_DEF::IMAGE_DIRECTORY_ENTRY_IMPORT));

                        qint64 nSrcOffset=pe.addressToOffset(&memoryMap,iid.FirstThunk+memoryMap.nBaseAddress);
                        qint64 nDstOffset=pe.addressToOffset(&memoryMap,pListImportHeaders->at(i).nFirstThunk+memoryMap.nBaseAddress);

                        //                        qDebug("src: %x",(quint32)nSrcOffset);
                        //                        qDebug("dst: %x",(quint32)nDstOffset);

                        if((nSrcOffset!=-1)&&(nDstOffset!=-1))
                        {
                            // TODO 64 ????
                            while(true)
                            {
                                quint32 nValue=pe.read_uint32(nSrcOffset);

                                pe.write_uint32(nDstOffset,nValue);

                                if(nValue==0)
                                {
                                    break;
                                }

                                nSrcOffset+=nAddressSize;
                                nDstOffset+=nAddressSize;
                            }

                            //                            iid.OriginalFirstThunk=0;
                            iid.FirstThunk=pListImportHeaders->at(i).nFirstThunk;

                            pe.setImportDescriptor(i,&iid);
                        }
                    }
                }

                bResult=true;
            }
        }
    }

    return bResult;
}

bool XPE::setImports(QString sFileName, bool bIsImage, QList<XPE::IMPORT_HEADER> *pListImportHeaders)
{
    bool bResult=false;

    QFile file(sFileName);

    if(file.open(QIODevice::ReadWrite))
    {
        bResult=setImports(&file,bIsImage,pListImportHeaders);

        file.close();
    }

    return bResult;
}

QString XPE::getImportFunctionName(quint32 nImport, quint32 nFunctionNumber, QList<XPE::IMPORT_HEADER> *pListImportHeaders)
{
    QString sResult;

    if(nImport<(quint32)pListImportHeaders->count())
    {
        if(nFunctionNumber<(quint32)pListImportHeaders->at(nImport).listPositions.count())
        {
            sResult=pListImportHeaders->at(nImport).listPositions.at(nFunctionNumber).sFunction;
        }
    }

    return sResult;
}

XPE::RESOURCE_HEADER XPE::getResourceHeader()
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getResourceHeader(&memoryMap);
}

XPE::RESOURCE_HEADER XPE::getResourceHeader(_MEMORY_MAP *pMemoryMap)
{
    RESOURCE_HEADER result={};

    qint64 nResourceOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_RESOURCE);

    if(nResourceOffset!=-1)
    {
        qint64 nOffset=nResourceOffset;

        result.nOffset=nOffset;
        result.directory=read_IMAGE_RESOURCE_DIRECTORY(nOffset);

        if((result.directory.NumberOfIdEntries+result.directory.NumberOfNamedEntries<=100)&&(result.directory.Characteristics==0)) // check corrupted
        {
            nOffset+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY);

            for(int i=0; i<result.directory.NumberOfIdEntries+result.directory.NumberOfNamedEntries; i++)
            {
                RESOURCE_POSITION rp=_getResourcePosition(pMemoryMap,pMemoryMap->nBaseAddress,nResourceOffset,nOffset,0);

                if(!rp.bIsValid)
                {
                    break;
                }

                result.listPositions.append(rp);
                nOffset+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY);
            }
        }
    }

    return result;
}

QList<XPE::RESOURCE_RECORD> XPE::getResources()
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getResources(&memoryMap);
}

QList<XPE::RESOURCE_RECORD> XPE::getResources(XBinary::_MEMORY_MAP *pMemoryMap)
{
    // TODO BE LE
    QList<RESOURCE_RECORD> listResources;

    qint64 nResourceOffset=getDataDirectoryOffset(pMemoryMap,XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_RESOURCE);

    if(nResourceOffset!=-1)
    {
        qint64 nBaseAddress=_getBaseAddress();
        RESOURCE_RECORD record={};

        qint64 nOffsetLevel[3]={};
        XPE_DEF::IMAGE_RESOURCE_DIRECTORY rd[3]={};
        XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY rde[3]={};

#if (QT_VERSION_MAJOR>=5)&&(QT_VERSION_MINOR>=10)
        RESOURCES_ID_NAME irin[3]={};
#else
        RESOURCES_ID_NAME irin[3]={0}; // MinGW 4.9 bug?
#endif

        nOffsetLevel[0]=nResourceOffset;
        rd[0]=read_IMAGE_RESOURCE_DIRECTORY(nOffsetLevel[0]);

        if((rd[0].NumberOfIdEntries+rd[0].NumberOfNamedEntries<=100)&&(rd[0].Characteristics==0)) // check corrupted  TODO const
        {
            nOffsetLevel[0]+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY);

            for(int i=0; i<rd[0].NumberOfIdEntries+rd[0].NumberOfNamedEntries; i++)
            {
                rde[0]=read_IMAGE_RESOURCE_DIRECTORY_ENTRY(nOffsetLevel[0]);

                irin[0]=getResourcesIDName(nResourceOffset,rde[0].Name);
                record.irin[0]=irin[0];

                nOffsetLevel[1]=nResourceOffset+rde[0].OffsetToDirectory;

                rd[1]=read_IMAGE_RESOURCE_DIRECTORY(nOffsetLevel[1]);

                if(rd[1].Characteristics!=0)
                {
                    break;
                }

                nOffsetLevel[1]+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY);

                if(rd[1].NumberOfIdEntries+rd[1].NumberOfNamedEntries<=100)
                {
                    for(int j=0; j<rd[1].NumberOfIdEntries+rd[1].NumberOfNamedEntries; j++)
                    {
                        rde[1]=read_IMAGE_RESOURCE_DIRECTORY_ENTRY(nOffsetLevel[1]);

                        irin[1]=getResourcesIDName(nResourceOffset,rde[1].Name);
                        record.irin[1]=irin[1];

                        nOffsetLevel[2]=nResourceOffset+rde[1].OffsetToDirectory;

                        rd[2]=read_IMAGE_RESOURCE_DIRECTORY(nOffsetLevel[2]);

                        if(rd[2].Characteristics!=0)
                        {
                            break;
                        }

                        nOffsetLevel[2]+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY);

                        if(rd[2].NumberOfIdEntries+rd[2].NumberOfNamedEntries<=100)
                        {
                            for(int k=0; k<rd[2].NumberOfIdEntries+rd[2].NumberOfNamedEntries; k++)
                            {
                                rde[2]=read_IMAGE_RESOURCE_DIRECTORY_ENTRY(nOffsetLevel[2]);

                                irin[2]=getResourcesIDName(nResourceOffset,rde[2].Name);
                                record.irin[2]=irin[2];

                                record.nIRDEOffset=rde[2].OffsetToData;
                                XPE_DEF::IMAGE_RESOURCE_DATA_ENTRY irde=read_IMAGE_RESOURCE_DATA_ENTRY(nResourceOffset+record.nIRDEOffset);
                                record.nRVA=irde.OffsetToData;
                                record.nAddress=irde.OffsetToData+nBaseAddress;
                                record.nOffset=addressToOffset(pMemoryMap,record.nAddress);
                                record.nSize=irde.Size;

                                listResources.append(record);

                                nOffsetLevel[2]+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY);
                            }
                        }

                        nOffsetLevel[1]+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY);
                    }
                }

                nOffsetLevel[0]+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY);
            }
        }
    }

    return listResources;
}

XPE::RESOURCE_RECORD XPE::getResourceRecord(quint32 nID1, quint32 nID2, QList<XPE::RESOURCE_RECORD> *pListResourceRecords)
{
    RESOURCE_RECORD result={};

    result.nOffset=-1;

    int nNumberOfResources=pListResourceRecords->count();

    for(int i=0; i<nNumberOfResources; i++)
    {
        if(pListResourceRecords->at(i).irin[0].nID==nID1)
        {
            if((pListResourceRecords->at(i).irin[1].nID==nID2)||(nID2==(quint32)-1))
            {
                result=pListResourceRecords->at(i);

                break;
            }
        }
    }

    return result;
}

XPE::RESOURCE_RECORD XPE::getResourceRecord(quint32 nID1, QString sName2, QList<XPE::RESOURCE_RECORD> *pListResourceRecords)
{
    RESOURCE_RECORD result={};

    result.nOffset=-1;

    int nNumberOfResources=pListResourceRecords->count();

    for(int i=0; i<nNumberOfResources; i++)
    {
        if((pListResourceRecords->at(i).irin[0].nID==nID1)&&(pListResourceRecords->at(i).irin[1].sName==sName2))
        {
            result=pListResourceRecords->at(i);

            break;
        }
    }

    return result;
}

XPE::RESOURCE_RECORD XPE::getResourceRecord(QString sName1, quint32 nID2, QList<XPE::RESOURCE_RECORD> *pListResourceRecords)
{
    RESOURCE_RECORD result={};

    result.nOffset=-1;

    int nNumberOfResources=pListResourceRecords->count();

    for(int i=0; i<nNumberOfResources; i++)
    {
        if(pListResourceRecords->at(i).irin[0].sName==sName1)
        {
            if((pListResourceRecords->at(i).irin[0].nID==nID2)||(nID2==(quint32)-1))
            {
                result=pListResourceRecords->at(i);

                break;
            }
        }
    }

    return result;
}

XPE::RESOURCE_RECORD XPE::getResourceRecord(QString sName1, QString sName2, QList<XPE::RESOURCE_RECORD> *pListResourceRecords)
{
    RESOURCE_RECORD result={};

    result.nOffset=-1;

    int nNumberOfResources=pListResourceRecords->count();

    for(int i=0; i<nNumberOfResources; i++)
    {
        if((pListResourceRecords->at(i).irin[0].sName==sName1)&&(pListResourceRecords->at(i).irin[1].sName==sName2))
        {
            result=pListResourceRecords->at(i);

            break;
        }
    }

    return result;
}

bool XPE::isResourcePresent(quint32 nID1, quint32 nID2, QList<XPE::RESOURCE_RECORD> *pListResourceRecords)
{
    return (getResourceRecord(nID1,nID2,pListResourceRecords).nSize);
}

bool XPE::isResourcePresent(quint32 nID1, QString sName2, QList<XPE::RESOURCE_RECORD> *pListResourceRecords)
{
    return (getResourceRecord(nID1,sName2,pListResourceRecords).nSize);
}

bool XPE::isResourcePresent(QString sName1, quint32 nID2, QList<XPE::RESOURCE_RECORD> *pListResourceRecords)
{
    return (getResourceRecord(sName1,nID2,pListResourceRecords).nSize);
}

bool XPE::isResourcePresent(QString sName1, QString sName2, QList<XPE::RESOURCE_RECORD> *pListResourceRecords)
{
    return (getResourceRecord(sName1,sName2,pListResourceRecords).nSize);
}

bool XPE::isResourceManifestPresent()
{
    QList<RESOURCE_RECORD> listResources=getResources();

    return isResourceManifestPresent(&listResources);
}

bool XPE::isResourceManifestPresent(QList<XPE::RESOURCE_RECORD> *pListResourceRecords)
{
    return isResourcePresent(XPE_DEF::S_RT_MANIFEST,-1,pListResourceRecords);
}

QString XPE::getResourceManifest()
{
    QList<RESOURCE_RECORD> listResources=getResources();

    return getResourceManifest(&listResources);
}

QString XPE::getResourceManifest(QList<XPE::RESOURCE_RECORD> *pListResourceRecords)
{
    QString sResult;

    RESOURCE_RECORD rh=getResourceRecord(XPE_DEF::S_RT_MANIFEST,-1,pListResourceRecords);

    if(rh.nOffset!=-1)
    {
        rh.nSize=qMin(rh.nSize,qint64(4000));
        sResult=read_ansiString(rh.nOffset,rh.nSize);
    }

    return sResult;
}

bool XPE::isResourceVersionPresent()
{
    QList<RESOURCE_RECORD> listResources=getResources();

    return isResourceVersionPresent(&listResources);
}

bool XPE::isResourceVersionPresent(QList<XPE::RESOURCE_RECORD> *pListResourceRecords)
{
    return isResourcePresent(XPE_DEF::S_RT_VERSION,-1,pListResourceRecords);
}

XPE_DEF::S_VS_VERSION_INFO XPE::readVS_VERSION_INFO(qint64 nOffset)
{
    XPE_DEF::S_VS_VERSION_INFO result={};

    result.wLength=read_uint16(nOffset+offsetof(XPE_DEF::S_VS_VERSION_INFO,wLength));
    result.wValueLength=read_uint16(nOffset+offsetof(XPE_DEF::S_VS_VERSION_INFO,wValueLength));
    result.wType=read_uint16(nOffset+offsetof(XPE_DEF::S_VS_VERSION_INFO,wType));

    read_array(nOffset,(char *)&result,sizeof(XPE_DEF::S_VS_VERSION_INFO));

    return result;
}

quint32 XPE::__getResourceVersion(XPE::RESOURCE_VERSION *pResourceVersionResult, qint64 nOffset, qint64 nSize, QString sPrefix, int nLevel)
{
    quint32 nResult=0;

    if((quint32)nSize>=sizeof(XPE_DEF::S_VS_VERSION_INFO))
    {
        XPE_DEF::S_VS_VERSION_INFO vi=readVS_VERSION_INFO(nOffset);

        if(vi.wLength<=nSize)
        {
            if(vi.wValueLength<vi.wLength)
            {
                QString sTitle=read_unicodeString(nOffset+sizeof(XPE_DEF::S_VS_VERSION_INFO));

                qint32 nDelta=sizeof(XPE_DEF::S_VS_VERSION_INFO);
                nDelta+=(sTitle.length()+1)*sizeof(quint16);
                nDelta=S_ALIGN_UP(nDelta,4);

                if(sPrefix!="")
                {
                    sPrefix+=".";
                }

                sPrefix+=sTitle;

                if(sPrefix=="VS_VERSION_INFO")
                {
                    if(vi.wValueLength>=sizeof(XPE_DEF::S_tagVS_FIXEDFILEINFO))
                    {
                        pResourceVersionResult->nFixedFileInfoOffset=nOffset+nDelta;
                        // TODO Check Signature?
                        pResourceVersionResult->fileInfo.dwSignature=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwSignature));
                        pResourceVersionResult->fileInfo.dwStrucVersion=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwStrucVersion));
                        pResourceVersionResult->fileInfo.dwFileVersionMS=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileVersionMS));
                        pResourceVersionResult->fileInfo.dwFileVersionLS=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileVersionLS));
                        pResourceVersionResult->fileInfo.dwProductVersionMS=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwProductVersionMS));
                        pResourceVersionResult->fileInfo.dwProductVersionLS=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwProductVersionLS));
                        pResourceVersionResult->fileInfo.dwFileFlagsMask=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileFlagsMask));
                        pResourceVersionResult->fileInfo.dwFileFlags=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileFlags));
                        pResourceVersionResult->fileInfo.dwFileOS=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileOS));
                        pResourceVersionResult->fileInfo.dwFileType=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileType));
                        pResourceVersionResult->fileInfo.dwFileSubtype=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileSubtype));
                        pResourceVersionResult->fileInfo.dwFileDateMS=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileDateMS));
                        pResourceVersionResult->fileInfo.dwFileDateLS=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileDateLS));
                    }
                }

                if(nLevel==3)
                {
                    QString sValue=read_unicodeString(nOffset+nDelta);
                    sPrefix+=QString(":%1").arg(sValue);

                    pResourceVersionResult->listRecords.append(sPrefix);
                }

                if(sPrefix=="VS_VERSION_INFO.VarFileInfo.Translation")
                {
                    if(vi.wValueLength==4)
                    {
                        quint32 nValue=read_uint32(nOffset+nDelta);
                        QString sValue=XBinary::valueToHex(nValue);
                        sPrefix+=QString(":%1").arg(sValue);

                        pResourceVersionResult->listRecords.append(sPrefix);
                    }
                }

                nDelta+=vi.wValueLength;

                qint32 _nSize=vi.wLength-nDelta;

                if(nLevel<3)
                {
                    while(_nSize>0)
                    {
                        qint32 _nDelta=__getResourceVersion(pResourceVersionResult,nOffset+nDelta,vi.wLength-nDelta,sPrefix,nLevel+1);

                        if(_nDelta==0)
                        {
                            break;
                        }

                        _nDelta=S_ALIGN_UP(_nDelta,4);

                        nDelta+=_nDelta;
                        _nSize-=_nDelta;
                    }
                }

                nResult=vi.wLength;
            }
        }
    }

    return nResult;
}

XPE::RESOURCE_VERSION XPE::getResourceVersion()
{
    QList<RESOURCE_RECORD> listResourceRecords=getResources();

    return getResourceVersion(&listResourceRecords);
}

XPE::RESOURCE_VERSION XPE::getResourceVersion(QList<XPE::RESOURCE_RECORD> *pListResourceRecords)
{
    RESOURCE_VERSION result={};
    result.nFixedFileInfoOffset=-1;

    RESOURCE_RECORD resourceRecord=getResourceRecord(XPE_DEF::S_RT_VERSION,-1,pListResourceRecords);

    if(resourceRecord.nOffset!=-1)
    {
        __getResourceVersion(&result,resourceRecord.nOffset,resourceRecord.nSize,"",0);
    }

    return result;
}

QString XPE::getFileVersion()
{
    RESOURCE_VERSION resourveVersion=getResourceVersion();

    return getFileVersion(&resourveVersion);
}

QString XPE::getFileVersion(RESOURCE_VERSION *pResourceVersion)
{
    return QString("%1.%2").arg(get_uint32_version(pResourceVersion->fileInfo.dwFileVersionMS)).arg(get_uint32_version(pResourceVersion->fileInfo.dwFileVersionLS));
}

void XPE::setFixedFileInfo_dwSignature(quint32 nValue)
{
    qint64 nOffset=getResourceVersion().nFixedFileInfoOffset;

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwSignature),nValue);
    }
}

void XPE::setFixedFileInfo_dwStrucVersion(quint32 nValue)
{
    qint64 nOffset=getResourceVersion().nFixedFileInfoOffset;

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwStrucVersion),nValue);
    }
}

void XPE::setFixedFileInfo_dwFileVersionMS(quint32 nValue)
{
    qint64 nOffset=getResourceVersion().nFixedFileInfoOffset;

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileVersionMS),nValue);
    }
}

void XPE::setFixedFileInfo_dwFileVersionLS(quint32 nValue)
{
    qint64 nOffset=getResourceVersion().nFixedFileInfoOffset;

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileVersionLS),nValue);
    }
}

void XPE::setFixedFileInfo_dwProductVersionMS(quint32 nValue)
{
    qint64 nOffset=getResourceVersion().nFixedFileInfoOffset;

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwProductVersionMS),nValue);
    }
}

void XPE::setFixedFileInfo_dwProductVersionLS(quint32 nValue)
{
    qint64 nOffset=getResourceVersion().nFixedFileInfoOffset;

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwProductVersionLS),nValue);
    }
}

void XPE::setFixedFileInfo_dwFileFlagsMask(quint32 nValue)
{
    qint64 nOffset=getResourceVersion().nFixedFileInfoOffset;

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileFlagsMask),nValue);
    }
}

void XPE::setFixedFileInfo_dwFileFlags(quint32 nValue)
{
    qint64 nOffset=getResourceVersion().nFixedFileInfoOffset;

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileFlags),nValue);
    }
}

void XPE::setFixedFileInfo_dwFileOS(quint32 nValue)
{
    qint64 nOffset=getResourceVersion().nFixedFileInfoOffset;

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileOS),nValue);
    }
}

void XPE::setFixedFileInfo_dwFileType(quint32 nValue)
{
    qint64 nOffset=getResourceVersion().nFixedFileInfoOffset;

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileType),nValue);
    }
}

void XPE::setFixedFileInfo_dwFileSubtype(quint32 nValue)
{
    qint64 nOffset=getResourceVersion().nFixedFileInfoOffset;

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileSubtype),nValue);
    }
}

void XPE::setFixedFileInfo_dwFileDateMS(quint32 nValue)
{
    qint64 nOffset=getResourceVersion().nFixedFileInfoOffset;

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileDateMS),nValue);
    }
}

void XPE::setFixedFileInfo_dwFileDateLS(quint32 nValue)
{
    qint64 nOffset=getResourceVersion().nFixedFileInfoOffset;

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileDateLS),nValue);
    }
}

QString XPE::getResourceVersionValue(QString sKey)
{
    QList<RESOURCE_RECORD> listResourceRecords=getResources();
    RESOURCE_VERSION resVersion=getResourceVersion(&listResourceRecords);

    return getResourceVersionValue(sKey,&resVersion);
}

QString XPE::getResourceVersionValue(QString sKey, XPE::RESOURCE_VERSION *pResourceVersion)
{
    QString sResult;

    int nNumberOfRecords=pResourceVersion->listRecords.count();

    for(int i=0; i<nNumberOfRecords; i++)
    {
        QString sRecord=pResourceVersion->listRecords.at(i).section(".",3,-1);
        QString _sKey=sRecord.section(":",0,0);

        if(_sKey==sKey)
        {
            sResult=sRecord.section(":",1,-1);

            break;
        }
    }

    return sResult;
}

quint32 XPE::getResourceIdByNumber(quint32 nNumber)
{
    QList<RESOURCE_RECORD> listResources=getResources();

    return getResourceIdByNumber(nNumber,&listResources);
}

quint32 XPE::getResourceIdByNumber(quint32 nNumber, QList<XPE::RESOURCE_RECORD> *pListResourceRecords)
{
    quint32 nResult=0;

    if((qint32)nNumber<pListResourceRecords->count())
    {
        nResult=pListResourceRecords->at(nNumber).irin[1].nID;
    }

    return nResult;
}

QString XPE::getResourceNameByNumber(quint32 nNumber)
{
    QList<RESOURCE_RECORD> listResources=getResources();

    return getResourceNameByNumber(nNumber,&listResources);
}

QString XPE::getResourceNameByNumber(quint32 nNumber, QList<XPE::RESOURCE_RECORD> *pListResourceRecords)
{
    QString sResult;

    if((qint32)nNumber<pListResourceRecords->count())
    {
        sResult=pListResourceRecords->at(nNumber).irin[1].sName;
    }

    return sResult;
}

qint64 XPE::getResourceOffsetByNumber(quint32 nNumber)
{
    QList<RESOURCE_RECORD> listResources=getResources();

    return getResourceOffsetByNumber(nNumber,&listResources);
}

qint64 XPE::getResourceOffsetByNumber(quint32 nNumber, QList<XPE::RESOURCE_RECORD> *pListResourceRecords)
{
    qint64 nResult=-1;

    if((qint32)nNumber<pListResourceRecords->count())
    {
        nResult=pListResourceRecords->at(nNumber).nOffset;
    }

    return nResult;
}

qint64 XPE::getResourceSizeByNumber(quint32 nNumber)
{
    QList<RESOURCE_RECORD> listResources=getResources();

    return getResourceSizeByNumber(nNumber,&listResources);
}

qint64 XPE::getResourceSizeByNumber(quint32 nNumber, QList<XPE::RESOURCE_RECORD> *pListResourceRecords)
{
    qint64 nResult=0;

    if((qint32)nNumber<pListResourceRecords->count())
    {
        nResult=pListResourceRecords->at(nNumber).nSize;
    }

    return nResult;
}

quint32 XPE::getResourceTypeByNumber(quint32 nNumber)
{
    QList<RESOURCE_RECORD> listResources=getResources();

    return getResourceTypeByNumber(nNumber,&listResources);
}

quint32 XPE::getResourceTypeByNumber(quint32 nNumber, QList<XPE::RESOURCE_RECORD> *pListResourceRecords)
{
    qint64 nResult=0;

    if((qint32)nNumber<pListResourceRecords->count())
    {
        nResult=pListResourceRecords->at(nNumber).irin[0].nID;
    }

    return nResult;
}

qint64 XPE::getResourceNameOffset(QString sName)
{
    QList<RESOURCE_RECORD> listResources=getResources();

    return getResourceNameOffset(sName,&listResources);
}

qint64 XPE::getResourceNameOffset(QString sName, QList<XPE::RESOURCE_RECORD> *pListResourceRecords)
{
    qint64 nResult=-1;

    int nNumberOfResources=pListResourceRecords->count();

    for(int i=0;i<nNumberOfResources;i++)
    {
        if(pListResourceRecords->at(i).irin[1].sName==sName)
        {
            nResult=pListResourceRecords->at(i).nOffset;
            break;
        }
    }

    return nResult;
}

bool XPE::isResourceNamePresent(QString sName)
{
    QList<RESOURCE_RECORD> listResources=getResources();

    return isResourceNamePresent(sName,&listResources);
}

bool XPE::isResourceNamePresent(QString sName, QList<XPE::RESOURCE_RECORD> *pListResourceRecords)
{
    return (getResourceNameOffset(sName,pListResourceRecords)!=-1);
}

XPE_DEF::IMAGE_IMPORT_DESCRIPTOR XPE::read_IMAGE_IMPORT_DESCRIPTOR(qint64 nOffset)
{
    XPE_DEF::IMAGE_IMPORT_DESCRIPTOR result={};

    result.OriginalFirstThunk=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR,OriginalFirstThunk));
    result.TimeDateStamp=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR,TimeDateStamp));
    result.ForwarderChain=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR,ForwarderChain));
    result.Name=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR,Name));
    result.FirstThunk=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR,FirstThunk));

    return result;
}

void XPE::write_IMAGE_IMPORT_DESCRIPTOR(qint64 nOffset, XPE_DEF::IMAGE_IMPORT_DESCRIPTOR idd)
{
    // TODO !!
    write_array(nOffset,(char *)&idd,sizeof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR));
}

XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR XPE::_read_IMAGE_DELAYLOAD_DESCRIPTOR(qint64 nOffset)
{
    XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR result={};

    result.AllAttributes=read_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR,AllAttributes));
    result.DllNameRVA=read_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR,DllNameRVA));
    result.ModuleHandleRVA=read_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR,ModuleHandleRVA));
    result.ImportAddressTableRVA=read_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR,ImportAddressTableRVA));
    result.ImportNameTableRVA=read_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR,ImportNameTableRVA));
    result.BoundImportAddressTableRVA=read_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR,BoundImportAddressTableRVA));
    result.UnloadInformationTableRVA=read_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR,UnloadInformationTableRVA));
    result.TimeDateStamp=read_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR,TimeDateStamp));

    return result;
}

bool XPE::isExportPresent()
{
    return isOptionalHeader_DataDirectoryPresent(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);
}

XPE::EXPORT_HEADER XPE::getExport()
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getExport(&memoryMap);
}

XPE::EXPORT_HEADER XPE::getExport(_MEMORY_MAP *pMemoryMap)
{
    EXPORT_HEADER result={};

    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        read_array(nExportOffset,(char *)&result.directory,sizeof(XPE_DEF::IMAGE_EXPORT_DIRECTORY));

        qint64 nNameOffset=addressToOffset(pMemoryMap,result.directory.Name+pMemoryMap->nBaseAddress);

        if(nNameOffset!=-1)
        {
            result.sName=read_ansiString(nNameOffset);
        }

        qint64 nAddressOfFunctionsOffset=addressToOffset(pMemoryMap,result.directory.AddressOfFunctions+pMemoryMap->nBaseAddress);
        qint64 nAddressOfNamesOffset=addressToOffset(pMemoryMap,result.directory.AddressOfNames+pMemoryMap->nBaseAddress);
        qint64 nAddressOfNameOrdinalsOffset=addressToOffset(pMemoryMap,result.directory.AddressOfNameOrdinals+pMemoryMap->nBaseAddress);

        if(result.directory.NumberOfFunctions<0xFFFF)
        {
            if((nAddressOfFunctionsOffset!=-1)&&(nAddressOfNamesOffset!=-1)&&(nAddressOfNameOrdinalsOffset!=-1))
            {
                QMap<quint16,EXPORT_POSITION> mapNames;

                for(int i=0; i<(int)result.directory.NumberOfNames; i++)
                {
                    EXPORT_POSITION position={};

                    int nIndex=read_uint16(nAddressOfNameOrdinalsOffset+2*i);
                    position.nOrdinal=nIndex+result.directory.Base;
                    position.nRVA=read_uint32(nAddressOfFunctionsOffset+4*nIndex);
                    position.nAddress=position.nRVA+pMemoryMap->nBaseAddress;
                    position.nNameRVA=read_uint32(nAddressOfNamesOffset+4*i);

                    qint64 nFunctionNameOffset=addressToOffset(pMemoryMap,position.nNameRVA+pMemoryMap->nBaseAddress);

                    if(nFunctionNameOffset!=-1)
                    {
                        position.sFunctionName=read_ansiString(nFunctionNameOffset);
                    }

                    mapNames.insert(position.nOrdinal,position);
                }

                for(int i=0; i<(int)result.directory.NumberOfFunctions; i++)
                {
                    EXPORT_POSITION position={};

                    int nIndex=i;
                    position.nOrdinal=nIndex+result.directory.Base;

                    if(mapNames.contains(position.nOrdinal))
                    {
                        position=mapNames.value(position.nOrdinal);
                    }
                    else
                    {
                        position.nRVA=read_uint32(nAddressOfFunctionsOffset+4*nIndex);
                        position.nAddress=position.nRVA+pMemoryMap->nBaseAddress;
                    }

                    result.listPositions.append(position);
                }
            }
        }
    }

    return result;
}

QList<QString> XPE::getExportFunctionsList(EXPORT_HEADER *pExportHeader)
{
    QList<QString> listResult;

    int nNumberOfPositions=pExportHeader->listPositions.count();

    for(int i=0;i<nNumberOfPositions;i++)
    {
        listResult.append(pExportHeader->listPositions.at(i).sFunctionName);
    }

    return listResult;
}

XPE_DEF::IMAGE_EXPORT_DIRECTORY XPE::getExportDirectory()
{
    XPE_DEF::IMAGE_EXPORT_DIRECTORY result={};

    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        read_array(nExportOffset,(char *)&result,sizeof(XPE_DEF::IMAGE_EXPORT_DIRECTORY));
    }

    return result;
}

void XPE::setExportDirectory(XPE_DEF::IMAGE_EXPORT_DIRECTORY *pExportDirectory)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_array(nExportOffset,(char *)pExportDirectory,sizeof(XPE_DEF::IMAGE_EXPORT_DIRECTORY));
    }
}

void XPE::setExportDirectory_Characteristics(quint32 nValue)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_uint32(nExportOffset+offsetof(XPE_DEF::IMAGE_EXPORT_DIRECTORY,Characteristics),nValue);
    }
}

void XPE::setExportDirectory_TimeDateStamp(quint32 nValue)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_uint32(nExportOffset+offsetof(XPE_DEF::IMAGE_EXPORT_DIRECTORY,TimeDateStamp),nValue);
    }
}

void XPE::setExportDirectory_MajorVersion(quint16 nValue)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_uint16(nExportOffset+offsetof(XPE_DEF::IMAGE_EXPORT_DIRECTORY,MajorVersion),nValue);
    }
}

void XPE::setExportDirectory_MinorVersion(quint16 nValue)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_uint16(nExportOffset+offsetof(XPE_DEF::IMAGE_EXPORT_DIRECTORY,MinorVersion),nValue);
    }
}

void XPE::setExportDirectory_Name(quint32 nValue)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_uint32(nExportOffset+offsetof(XPE_DEF::IMAGE_EXPORT_DIRECTORY,Name),nValue);
    }
}

void XPE::setExportDirectory_Base(quint32 nValue)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_uint32(nExportOffset+offsetof(XPE_DEF::IMAGE_EXPORT_DIRECTORY,Base),nValue);
    }
}

void XPE::setExportDirectory_NumberOfFunctions(quint32 nValue)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_uint32(nExportOffset+offsetof(XPE_DEF::IMAGE_EXPORT_DIRECTORY,NumberOfFunctions),nValue);
    }
}

void XPE::setExportDirectory_NumberOfNames(quint32 nValue)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_uint32(nExportOffset+offsetof(XPE_DEF::IMAGE_EXPORT_DIRECTORY,NumberOfNames),nValue);
    }
}

void XPE::setExportDirectory_AddressOfFunctions(quint32 nValue)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_uint32(nExportOffset+offsetof(XPE_DEF::IMAGE_EXPORT_DIRECTORY,AddressOfFunctions),nValue);
    }
}

void XPE::setExportDirectory_AddressOfNames(quint32 nValue)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_uint32(nExportOffset+offsetof(XPE_DEF::IMAGE_EXPORT_DIRECTORY,AddressOfNames),nValue);
    }
}

void XPE::setExportDirectory_AddressOfNameOrdinals(quint32 nValue)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_uint32(nExportOffset+offsetof(XPE_DEF::IMAGE_EXPORT_DIRECTORY,AddressOfNameOrdinals),nValue);
    }
}

QByteArray XPE::getHeaders()
{
    // TODO Check
    QByteArray baResult;

    int nSizeOfHeaders=getOptionalHeader_SizeOfHeaders();

    if(isImage())
    {
        quint32 nSectionAlignment=getOptionalHeader_SectionAlignment();
        nSizeOfHeaders=S_ALIGN_UP(nSizeOfHeaders,nSectionAlignment);
    }

    baResult=read_array(0,nSizeOfHeaders);

    if(baResult.size()!=nSizeOfHeaders)
    {
        baResult.resize(0);
    }

    return baResult;
}

XBinary::OFFSETSIZE XPE::__getSectionOffsetAndSize(quint32 nSection)
{
    OFFSETSIZE result={};

    XPE_DEF::IMAGE_SECTION_HEADER sectionHeader=getSectionHeader(nSection);
    quint32 nSectionAlignment=getOptionalHeader_SectionAlignment();
    quint32 nFileAlignment=getOptionalHeader_FileAlignment();

    if(nFileAlignment==nSectionAlignment)
    {
        nFileAlignment=1;
    }

    bool bIsSectionValid=false;

    if(!isImage())
    {
        bIsSectionValid=(bool)(sectionHeader.SizeOfRawData!=0);
    }
    else
    {
        bIsSectionValid=(bool)(sectionHeader.Misc.VirtualSize!=0);
    }

    if(bIsSectionValid)
    {
        qint64 nSectionOffset=0;
        qint64 nSectionSize=0;

        if(!isImage())
        {
            nSectionOffset=sectionHeader.PointerToRawData;
            nSectionOffset=S_ALIGN_DOWN(nSectionOffset,nFileAlignment);
            nSectionSize=sectionHeader.SizeOfRawData+(sectionHeader.PointerToRawData-nSectionOffset);
        }
        else
        {
            nSectionOffset=sectionHeader.VirtualAddress;
            nSectionSize=sectionHeader.Misc.VirtualSize;
        }

        result=convertOffsetAndSize(nSectionOffset,nSectionSize);
    }
    else
    {
        result.nOffset=-1;
    }

    return result;
}

QByteArray XPE::getSection(quint32 nSection)
{
    QByteArray baResult;

    OFFSETSIZE offsetSize=__getSectionOffsetAndSize(nSection);

    if(offsetSize.nOffset!=-1)
    {
        baResult=read_array(offsetSize.nOffset,offsetSize.nSize);

        if(baResult.size()!=offsetSize.nSize) // TODO check???
        {
            baResult.resize(0);
        }
    }

    return baResult;
}

QString XPE::getSectionHash(HASH hash,quint32 nSection)
{
    QString sResult;

    OFFSETSIZE offsetSize=__getSectionOffsetAndSize(nSection);

    if(offsetSize.nOffset!=-1)
    {
        sResult=getHash(hash,offsetSize.nOffset,offsetSize.nSize);
    }

    return sResult;
}

double XPE::getSectionEntropy(quint32 nSection)
{
    double dResult=0;

    OFFSETSIZE offsetSize=__getSectionOffsetAndSize(nSection);

    if(offsetSize.nOffset!=-1)
    {
        dResult=getEntropy(offsetSize.nOffset,offsetSize.nSize);
    }

    return dResult;
}

bool XPE::addImportSection(QMap<qint64, QString> *pMapIAT)
{
    return addImportSection(getDevice(),isImage(),pMapIAT);
}

bool XPE::addImportSection(QIODevice *pDevice, bool bIsImage, QMap<qint64, QString> *pMapIAT)
{
#ifdef QT_DEBUG
    QElapsedTimer timer;
    timer.start();
    qDebug("addImportSection");
#endif

    bool bResult=false;

    if(isResizeEnable(pDevice))
    {
        XPE pe(pDevice,bIsImage);

        if(pe.isValid())
        {
            QList<IMPORT_HEADER> listImportHeaders=mapIATToList(pMapIAT,pe.is64());
#ifdef QT_DEBUG
            qDebug("addImportSection:mapIATToList: %lld msec",timer.elapsed());
#endif
            bResult=setImports(pDevice,bIsImage,&listImportHeaders);
#ifdef QT_DEBUG
            qDebug("addImportSection:setImports: %lld msec",timer.elapsed());
#endif
        }
    }

#ifdef QT_DEBUG
    qDebug("addImportSection: %lld msec",timer.elapsed());
#endif

    return bResult;
}

bool XPE::addImportSection(QString sFileName,bool bIsImage, QMap<qint64, QString> *pMapIAT)
{
    bool bResult=false;

    QFile file(sFileName);

    if(file.open(QIODevice::ReadWrite))
    {
        bResult=addImportSection(&file,bIsImage,pMapIAT);

        file.close();
    }

    return bResult;
}

QList<XPE::IMPORT_HEADER> XPE::mapIATToList(QMap<qint64, QString> *pMapIAT,bool bIs64)
{
    QList<IMPORT_HEADER> listResult;

    IMPORT_HEADER record={};

    quint64 nCurrentRVA=0;

    quint32 nStep=0;

    if(bIs64)
    {
        nStep=8;
    }
    else
    {
        nStep=4;
    }

    QMapIterator<qint64, QString> i(*pMapIAT);

    while(i.hasNext())
    {
        i.next();

        QString sLibrary=i.value().section("#",0,0);
        QString sFunction=i.value().section("#",1,1);

        if(((qint64)(nCurrentRVA+nStep)!=i.key())||((record.sName!="")&&(record.sName!=sLibrary)))
        {
            if(record.sName!="")
            {
                listResult.append(record);
            }

            record.sName=sLibrary;
            record.nFirstThunk=i.key();
            record.listPositions.clear();
        }

        nCurrentRVA=i.key();

        IMPORT_POSITION position={};

        position.nHint=0;

        if(sFunction.toInt())
        {
            position.nOrdinal=sFunction.toInt();
        }
        else
        {
            position.sName=sFunction;
        }

        position.nThunkRVA=i.key();

        record.listPositions.append(position);

        if(!i.hasNext())
        {
            if(record.sName!="")
            {
                listResult.append(record);
            }
        }
    }

    return listResult;
}

quint32 XPE::calculateCheckSum()
{
    quint32 nCalcSum=_checkSum(0,getSize());
    quint32 nHdrSum=getOptionalHeader_CheckSum();

    if(S_LOWORD(nCalcSum)>=S_LOWORD(nHdrSum))
    {
        nCalcSum-=S_LOWORD(nHdrSum);
    }
    else
    {
        nCalcSum=((S_LOWORD(nCalcSum)-S_LOWORD(nHdrSum))&0xFFFF)-1;
    }

    if(S_LOWORD(nCalcSum)>=S_HIWORD(nHdrSum)) //!!!!!
    {
        nCalcSum-=S_HIWORD(nHdrSum);
    }
    else
    {
        nCalcSum=((S_LOWORD(nCalcSum)-S_HIWORD(nHdrSum))&0xFFFF)-1;
    }

    nCalcSum+=getSize();

    return nCalcSum;
}

bool XPE::addSection(QString sFileName,bool bIsImage, XPE_DEF::IMAGE_SECTION_HEADER *pSectionHeader, char *pData, qint64 nDataSize)
{
    bool bResult=false;

    QFile file(sFileName);

    if(file.open(QIODevice::ReadWrite))
    {
        bResult=addSection(&file,bIsImage,pSectionHeader,pData,nDataSize);

        file.close();
    }
    else
    {
        emit errorMessage(QString("%1: %2").arg(tr("Cannot open file")).arg(sFileName));
    }

    return bResult;
}

bool XPE::addSection(QIODevice *pDevice, bool bIsImage, XPE_DEF::IMAGE_SECTION_HEADER *pSectionHeader, char *pData, qint64 nDataSize)
{
    bool bResult=false;

    if(isResizeEnable(pDevice))
    {
        XPE pe(pDevice,bIsImage);

        if(pe.isValid())
        {
            qint64 nHeadersSize=pe._fixHeadersSize();
            qint64 nNewHeadersSize=pe._calculateHeadersSize(pe.getSectionsTableOffset(),pe.getFileHeader_NumberOfSections()+1);
            quint32 nFileAlignment=pe.getOptionalHeader_FileAlignment();
            quint32 nSectionAlignment=pe.getOptionalHeader_SectionAlignment();

            if(pSectionHeader->PointerToRawData==0)
            {
                pSectionHeader->PointerToRawData=pe._calculateRawSize();
            }

            if(pSectionHeader->SizeOfRawData==0)
            {
                pSectionHeader->SizeOfRawData=S_ALIGN_UP(nDataSize,nFileAlignment);
            }

            if(pSectionHeader->VirtualAddress==0)
            {
                pSectionHeader->VirtualAddress=S_ALIGN_UP(pe.getOptionalHeader_SizeOfImage(),nSectionAlignment);
            }

            if(pSectionHeader->Misc.VirtualSize==0)
            {
                pSectionHeader->Misc.VirtualSize=S_ALIGN_UP(nDataSize,nSectionAlignment);
            }

            qint64 nDelta=nNewHeadersSize-nHeadersSize;
            qint64 nFileSize=pDevice->size();

            if(nDelta>0)
            {
                resize(pDevice,nFileSize+nDelta);
                pe.moveMemory(nHeadersSize,nNewHeadersSize,nFileSize-nHeadersSize);
            }
            else if(nDelta<0)
            {
                pe.moveMemory(nHeadersSize,nNewHeadersSize,nFileSize-nHeadersSize);
                resize(pDevice,nFileSize+nDelta);
            }

            pe._fixFileOffsets(nDelta);

            pSectionHeader->PointerToRawData+=nDelta;
            nFileSize+=nDelta;

            if(nFileSize<nHeadersSize)
            {
                nFileSize=nHeadersSize;
            }

            // TODO!!!
            resize(pDevice,nFileSize+pSectionHeader->SizeOfRawData);

            quint32 nNumberOfSections=pe.getFileHeader_NumberOfSections();
            pe.setFileHeader_NumberOfSections(nNumberOfSections+1);
            pe.setSectionHeader(nNumberOfSections,pSectionHeader);

            // Overlay
            if(pe.isOverlayPresent())
            {
                qint64 nOverlayOffset=pe.getOverlayOffset();
                qint64 nOverlaySize=pe.getOverlaySize();
                pe.moveMemory(nOverlayOffset-pSectionHeader->SizeOfRawData,nOverlayOffset,nOverlaySize);
            }

            pe.write_array(pSectionHeader->PointerToRawData,pData,nDataSize);

            pe.zeroFill(pSectionHeader->PointerToRawData+nDataSize,(pSectionHeader->SizeOfRawData)-nDataSize);

            qint64 nNewImageSize=S_ALIGN_UP(pSectionHeader->VirtualAddress+pSectionHeader->Misc.VirtualSize,nSectionAlignment);
            pe.setOptionalHeader_SizeOfImage(nNewImageSize);

            // TODO flag
            pe.fixCheckSum();

            bResult=true;
        }
    }

    return bResult;
}

bool XPE::removeLastSection()
{
    return removeLastSection(getDevice(),isImage());
}

bool XPE::removeLastSection(QIODevice *pDevice,bool bIsImage)
{
    // TODO Check
    bool bResult=false;

    if(isResizeEnable(pDevice))
    {
        XPE pe(pDevice,bIsImage);

        if(pe.isValid())
        {
            int nNumberOfSections=pe.getFileHeader_NumberOfSections();

            if(nNumberOfSections)
            {
                qint64 nHeadersSize=pe._fixHeadersSize();
                qint64 nNewHeadersSize=pe._calculateHeadersSize(pe.getSectionsTableOffset(),nNumberOfSections-1);
                quint32 nFileAlignment=pe.getOptionalHeader_FileAlignment();
                quint32 nSectionAlignment=pe.getOptionalHeader_SectionAlignment();
                bool bIsOverlayPresent=pe.isOverlayPresent();
                qint64 nOverlayOffset=pe.getOverlayOffset();
                qint64 nOverlaySize=pe.getOverlaySize();

                XPE_DEF::IMAGE_SECTION_HEADER ish=pe.getSectionHeader(nNumberOfSections-1);
                XPE_DEF::IMAGE_SECTION_HEADER ish0={};
                pe.setSectionHeader(nNumberOfSections-1,&ish0);
                pe.setFileHeader_NumberOfSections(nNumberOfSections-1);

                ish.SizeOfRawData=S_ALIGN_UP(ish.SizeOfRawData,nFileAlignment);
                ish.Misc.VirtualSize=S_ALIGN_UP(ish.Misc.VirtualSize,nSectionAlignment);

                qint64 nDelta=nNewHeadersSize-nHeadersSize;
                qint64 nFileSize=pDevice->size();

                if(nDelta>0)
                {
                    resize(pDevice,nFileSize+nDelta);
                    pe.moveMemory(nHeadersSize,nNewHeadersSize,nFileSize-nHeadersSize);
                }
                else if(nDelta<0)
                {
                    pe.moveMemory(nHeadersSize,nNewHeadersSize,nFileSize-nHeadersSize);
                    resize(pDevice,nFileSize+nDelta);
                }

                pe._fixFileOffsets(nDelta);

                nFileSize+=nDelta;
                nOverlayOffset+=nDelta;

                if(bIsOverlayPresent)
                {
                    pe.moveMemory(nOverlayOffset,nOverlayOffset-ish.SizeOfRawData,nOverlaySize);
                }

                resize(pDevice,nFileSize-ish.SizeOfRawData);

                qint64 nNewImageSize=S_ALIGN_UP(ish.VirtualAddress,nSectionAlignment);
                pe.setOptionalHeader_SizeOfImage(nNewImageSize);

                pe.fixCheckSum();

                bResult=true;
            }
        }
    }

    return bResult;
}

bool XPE::removeLastSection(QString sFileName, bool bIsImage)
{
    bool bResult=false;

    QFile file(sFileName);

    if(file.open(QIODevice::ReadWrite))
    {
        bResult=removeLastSection(&file,bIsImage);

        file.close();
    }

    return bResult;
}

bool XPE::addSection(XPE_DEF::IMAGE_SECTION_HEADER *pSectionHeader, char *pData, qint64 nDataSize)
{
    return addSection(getDevice(),isImage(),pSectionHeader,pData,nDataSize);
}

XPE::RESOURCE_POSITION XPE::_getResourcePosition(XBinary::_MEMORY_MAP *pMemoryMap,qint64 nBaseAddress, qint64 nResourceOffset, qint64 nOffset, quint32 nLevel)
{
    RESOURCE_POSITION result={};

    result.nOffset=nOffset;
    result.nLevel=nLevel;
    result.dirEntry=read_IMAGE_RESOURCE_DIRECTORY_ENTRY(nOffset);
    result.rin=getResourcesIDName(nResourceOffset,result.dirEntry.Name);
    result.bIsDataDirectory=result.dirEntry.DataIsDirectory;

    if(result.bIsDataDirectory)
    {
        qint64 nDirectoryOffset=nResourceOffset+result.dirEntry.OffsetToDirectory;
        result.directory=read_IMAGE_RESOURCE_DIRECTORY(nDirectoryOffset);
        nDirectoryOffset+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY);

        if((result.directory.NumberOfIdEntries+result.directory.NumberOfNamedEntries<=1000)&&(result.directory.Characteristics==0)) // check corrupted
        {
            result.bIsValid=true;

            for(int i=0; i<result.directory.NumberOfIdEntries+result.directory.NumberOfNamedEntries; i++)
            {
                RESOURCE_POSITION rp=_getResourcePosition(pMemoryMap,nBaseAddress,nResourceOffset,nDirectoryOffset,nLevel+1);

                if(!rp.bIsValid)
                {
                    break;
                }

                result.listPositions.append(rp);
                nDirectoryOffset+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY);
            }
        }
    }
    else
    {
        result.bIsValid=true;
        result.dataEntry=read_IMAGE_RESOURCE_DATA_ENTRY(nResourceOffset+result.dirEntry.OffsetToData);
        result.nDataAddress=nBaseAddress+result.dataEntry.OffsetToData;
        result.nDataOffset=addressToOffset(pMemoryMap,result.nDataAddress);
    }

    return result;
}

void XPE::fixCheckSum()
{
    setOptionalHeader_CheckSum(calculateCheckSum());
}

QList<XPE_DEF::IMAGE_SECTION_HEADER> XPE::splitSection(QByteArray *pbaData, XPE_DEF::IMAGE_SECTION_HEADER sectionHeaderOriginal, quint32 nBlockSize)
{
    QList<XPE_DEF::IMAGE_SECTION_HEADER> listResult;
    //    int nBlockSize=0x1000;
    int nSize=pbaData->size();
    char *pOffset=pbaData->data();
    char *pOffsetStart=pOffset;
    int nCount=nSize/nBlockSize;
//    quint64 nVirtualAddress=shOriginal.VirtualAddress;
    qint64 nRelVirtualStart=0;
    qint64 nRelVirtualEnd=S_ALIGN_UP(sectionHeaderOriginal.Misc.VirtualSize,nBlockSize);
    qint64 nRelCurrent=nRelVirtualStart;

    if(nCount>1)
    {
        // Check the first block
        while(isEmptyData(pOffset,nBlockSize))
        {
            pOffset+=nBlockSize;
            nRelCurrent+=nBlockSize;
            nCount--;

            if(pOffset>=pOffsetStart+nSize)
            {
                break;
            }
        }

        if(pOffset!=pOffsetStart)
        {
            XPE_DEF::IMAGE_SECTION_HEADER sectionHeader=sectionHeaderOriginal;
//            sh.VirtualAddress=nVirtualAddress;
            //            sh.Misc.VirtualSize=pOffset-pOffsetStart;
            sectionHeader.Misc.VirtualSize=nRelCurrent-nRelVirtualStart;
            sectionHeader.SizeOfRawData=(quint32)XBinary::getPhysSize(pOffsetStart,sectionHeader.Misc.VirtualSize);
            listResult.append(sectionHeader);

//            nVirtualAddress+=sh.Misc.VirtualSize;
        }

        bool bNew=false;
        pOffsetStart=pOffset;
        nRelVirtualStart=nRelCurrent;

        while(nCount>0)
        {
            if(isEmptyData(pOffset,nBlockSize))
            {
                bNew=true;
            }
            else
            {
                if(bNew)
                {
                    XPE_DEF::IMAGE_SECTION_HEADER sectionHeader=sectionHeaderOriginal;
//                    sh.VirtualAddress=nVirtualAddress;
                    //                    sh.Misc.VirtualSize=pOffset-pOffsetStart;
                    sectionHeader.Misc.VirtualSize=nRelCurrent-nRelVirtualStart;
                    sectionHeader.SizeOfRawData=(quint32)XBinary::getPhysSize(pOffsetStart,sectionHeader.Misc.VirtualSize);
                    listResult.append(sectionHeader);

//                    nVirtualAddress+=sh.Misc.VirtualSize;

                    pOffsetStart=pOffset;
                    nRelVirtualStart=nRelCurrent;
                    bNew=false;
                }
            }

            pOffset+=nBlockSize;
            nRelCurrent+=nBlockSize;
            nCount--;
        }

        if(pOffset!=pOffsetStart)
        {
            XPE_DEF::IMAGE_SECTION_HEADER sectionHeader=sectionHeaderOriginal;
//            sh.VirtualAddress=nVirtualAddress;
            //            sh.Misc.VirtualSize=pOffset-pOffsetStart;
            sectionHeader.Misc.VirtualSize=nRelVirtualEnd-nRelVirtualStart;
            sectionHeader.SizeOfRawData=(quint32)XBinary::getPhysSize(pOffsetStart,nSize-(pOffsetStart-pbaData->data()));

            if(sectionHeader.Misc.VirtualSize)
            {
                listResult.append(sectionHeader);
            }

//            nVirtualAddress+=sh.Misc.VirtualSize;
        }
    }
    else
    {
        listResult.append(sectionHeaderOriginal);
    }

    return listResult;
}

QByteArray XPE::createHeaderStub(HEADER_OPTIONS *pHeaderOptions) // TODO options
{
    QByteArray baResult;

    baResult.resize(0x200); // TODO const
    baResult.fill(0);

    QBuffer buffer(&baResult);

    if(buffer.open(QIODevice::ReadWrite))
    {
        XPE pe(&buffer);

        pe.set_e_magic(XMSDOS_DEF::S_IMAGE_DOS_SIGNATURE_MZ);
        pe.set_e_lfanew(0x40);
        pe.setNtHeaders_Signature(XPE_DEF::S_IMAGE_NT_SIGNATURE);
        pe.setFileHeader_SizeOfOptionalHeader(0xE0); // TODO
        pe.setFileHeader_Machine(pHeaderOptions->nMachine);
        pe.setFileHeader_Characteristics(pHeaderOptions->nCharacteristics);
        pe.setOptionalHeader_Magic(pHeaderOptions->nMagic);
        pe.setOptionalHeader_ImageBase(pHeaderOptions->nImagebase);
        pe.setOptionalHeader_DllCharacteristics(pHeaderOptions->nDllcharacteristics);
        pe.setOptionalHeader_Subsystem(pHeaderOptions->nSubsystem);
        pe.setOptionalHeader_MajorOperatingSystemVersion(pHeaderOptions->nMajorOperationSystemVersion);
        pe.setOptionalHeader_MinorOperatingSystemVersion(pHeaderOptions->nMinorOperationSystemVersion);
        pe.setOptionalHeader_FileAlignment(pHeaderOptions->nFileAlignment);
        pe.setOptionalHeader_SectionAlignment(pHeaderOptions->nSectionAlignment);
        pe.setOptionalHeader_AddressOfEntryPoint(pHeaderOptions->nAddressOfEntryPoint);
        pe.setOptionalHeader_NumberOfRvaAndSizes(0x10);

        pe.setOptionalHeader_DataDirectory_VirtualAddress(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_RESOURCE,pHeaderOptions->nResourceRVA);
        pe.setOptionalHeader_DataDirectory_Size(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_RESOURCE,pHeaderOptions->nResourceSize);

        buffer.close();
    }

    return baResult;
}

XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32 XPE::getLoadConfigDirectory32()
{
    XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32 result={};

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        result.Size=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,Size));
        result.TimeDateStamp=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,TimeDateStamp));
        result.MajorVersion=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,MajorVersion));
        result.MinorVersion=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,MinorVersion));
        result.GlobalFlagsClear=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GlobalFlagsClear));
        result.GlobalFlagsSet=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GlobalFlagsSet));
        result.CriticalSectionDefaultTimeout=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,CriticalSectionDefaultTimeout));
        result.DeCommitFreeBlockThreshold=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,DeCommitFreeBlockThreshold));
        result.DeCommitTotalFreeThreshold=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,DeCommitTotalFreeThreshold));
        result.LockPrefixTable=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,LockPrefixTable));
        result.MaximumAllocationSize=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,MaximumAllocationSize));
        result.VirtualMemoryThreshold=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,VirtualMemoryThreshold));
        result.ProcessAffinityMask=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,ProcessAffinityMask));
        result.CSDVersion=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,CSDVersion));
        result.DependentLoadFlags=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,DependentLoadFlags));
        result.EditList=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,EditList));
        result.SecurityCookie=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,SecurityCookie));
        result.SEHandlerTable=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,SEHandlerTable));
        result.SEHandlerCount=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,SEHandlerCount));
        // Extra
        result.GuardCFCheckFunctionPointer=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardCFCheckFunctionPointer));
        result.GuardCFDispatchFunctionPointer=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardCFDispatchFunctionPointer));
        result.GuardCFFunctionTable=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardCFFunctionTable));
        result.GuardCFFunctionCount=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardCFFunctionCount));
        result.GuardFlags=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardFlags));
        result.CodeIntegrity.Flags=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,CodeIntegrity.Flags));
        result.CodeIntegrity.Catalog=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,CodeIntegrity.Catalog));
        result.CodeIntegrity.CatalogOffset=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,CodeIntegrity.CatalogOffset));
        result.CodeIntegrity.Reserved=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,CodeIntegrity.Reserved));
        result.GuardAddressTakenIatEntryTable=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardAddressTakenIatEntryTable));
        result.GuardAddressTakenIatEntryCount=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardAddressTakenIatEntryCount));
        result.GuardLongJumpTargetTable=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardLongJumpTargetTable));
        result.GuardLongJumpTargetCount=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardLongJumpTargetCount));
        result.DynamicValueRelocTable=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,DynamicValueRelocTable));
        result.CHPEMetadataPointer=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,CHPEMetadataPointer));
        result.GuardRFFailureRoutine=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardRFFailureRoutine));
        result.GuardRFFailureRoutineFunctionPointer=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardRFFailureRoutineFunctionPointer));
        result.DynamicValueRelocTableOffset=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,DynamicValueRelocTableOffset));
        result.DynamicValueRelocTableSection=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,DynamicValueRelocTableSection));
        result.Reserved2=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,Reserved2));
        result.GuardRFVerifyStackPointerFunctionPointer=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardRFVerifyStackPointerFunctionPointer));
        result.HotPatchTableOffset=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,HotPatchTableOffset));
        result.Reserved3=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,Reserved3));
        result.EnclaveConfigurationPointer=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,EnclaveConfigurationPointer));
        result.VolatileMetadataPointer=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,VolatileMetadataPointer));
    }

    return result;
}

XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64 XPE::getLoadConfigDirectory64()
{
    XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64 result={};

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        result.Size=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,Size));
        result.TimeDateStamp=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,TimeDateStamp));
        result.MajorVersion=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,MajorVersion));
        result.MinorVersion=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,MinorVersion));
        result.GlobalFlagsClear=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GlobalFlagsClear));
        result.GlobalFlagsSet=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GlobalFlagsSet));
        result.CriticalSectionDefaultTimeout=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,CriticalSectionDefaultTimeout));
        result.DeCommitFreeBlockThreshold=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,DeCommitFreeBlockThreshold));
        result.DeCommitTotalFreeThreshold=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,DeCommitTotalFreeThreshold));
        result.LockPrefixTable=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,LockPrefixTable));
        result.MaximumAllocationSize=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,MaximumAllocationSize));
        result.VirtualMemoryThreshold=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,VirtualMemoryThreshold));
        result.ProcessAffinityMask=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,ProcessAffinityMask));
        result.CSDVersion=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,CSDVersion));
        result.DependentLoadFlags=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,DependentLoadFlags));
        result.EditList=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,EditList));
        result.SecurityCookie=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,SecurityCookie));
        result.SEHandlerTable=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,SEHandlerTable));
        result.SEHandlerCount=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,SEHandlerCount));
        // Extra
        result.GuardCFCheckFunctionPointer=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardCFCheckFunctionPointer));
        result.GuardCFDispatchFunctionPointer=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardCFDispatchFunctionPointer));
        result.GuardCFFunctionTable=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardCFFunctionTable));
        result.GuardCFFunctionCount=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardCFFunctionCount));
        result.GuardFlags=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardFlags));
        result.CodeIntegrity.Flags=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,CodeIntegrity.Flags));
        result.CodeIntegrity.Catalog=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,CodeIntegrity.Catalog));
        result.CodeIntegrity.CatalogOffset=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,CodeIntegrity.CatalogOffset));
        result.CodeIntegrity.Reserved=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,CodeIntegrity.Reserved));
        result.GuardAddressTakenIatEntryTable=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardAddressTakenIatEntryTable));
        result.GuardAddressTakenIatEntryCount=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardAddressTakenIatEntryCount));
        result.GuardLongJumpTargetTable=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardLongJumpTargetTable));
        result.GuardLongJumpTargetCount=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardLongJumpTargetCount));
        result.DynamicValueRelocTable=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,DynamicValueRelocTable));
        result.CHPEMetadataPointer=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,CHPEMetadataPointer));
        result.GuardRFFailureRoutine=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardRFFailureRoutine));
        result.GuardRFFailureRoutineFunctionPointer=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardRFFailureRoutineFunctionPointer));
        result.DynamicValueRelocTableOffset=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,DynamicValueRelocTableOffset));
        result.DynamicValueRelocTableSection=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,DynamicValueRelocTableSection));
        result.Reserved2=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,Reserved2));
        result.GuardRFVerifyStackPointerFunctionPointer=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardRFVerifyStackPointerFunctionPointer));
        result.HotPatchTableOffset=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,HotPatchTableOffset));
        result.Reserved3=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,Reserved3));
        result.EnclaveConfigurationPointer=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,EnclaveConfigurationPointer));
        result.VolatileMetadataPointer=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,VolatileMetadataPointer));
    }

    return result;
}

qint64 XPE::getLoadConfigDirectoryOffset()
{
    return getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);
}

qint64 XPE::getLoadConfigDirectorySize()
{
    return getLoadConfig_Size();
}

quint32 XPE::getLoadConfig_Size()
{
    quint32 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,Size));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,Size));
        }
    }

    return nResult;
}

quint32 XPE::getLoadConfig_TimeDateStamp()
{
    quint32 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,TimeDateStamp));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,TimeDateStamp));
        }
    }

    return nResult;
}

quint16 XPE::getLoadConfig_MajorVersion()
{
    quint16 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,MajorVersion));
        }
        else
        {
            nResult=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,MajorVersion));
        }
    }

    return nResult;
}

quint16 XPE::getLoadConfig_MinorVersion()
{
    quint16 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,MinorVersion));
        }
        else
        {
            nResult=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,MinorVersion));
        }
    }

    return nResult;
}

quint32 XPE::getLoadConfig_GlobalFlagsClear()
{
    quint32 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GlobalFlagsClear));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GlobalFlagsClear));
        }
    }

    return nResult;
}

quint32 XPE::getLoadConfig_GlobalFlagsSet()
{
    quint32 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GlobalFlagsSet));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GlobalFlagsSet));
        }
    }

    return nResult;
}

quint32 XPE::getLoadConfig_CriticalSectionDefaultTimeout()
{
    quint32 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,CriticalSectionDefaultTimeout));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,CriticalSectionDefaultTimeout));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_DeCommitFreeBlockThreshold()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,DeCommitFreeBlockThreshold));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,DeCommitFreeBlockThreshold));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_DeCommitTotalFreeThreshold()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,DeCommitTotalFreeThreshold));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,DeCommitTotalFreeThreshold));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_LockPrefixTable()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,LockPrefixTable));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,LockPrefixTable));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_MaximumAllocationSize()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,MaximumAllocationSize));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,MaximumAllocationSize));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_VirtualMemoryThreshold()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,VirtualMemoryThreshold));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,VirtualMemoryThreshold));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_ProcessAffinityMask()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,ProcessAffinityMask));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,ProcessAffinityMask));
        }
    }

    return nResult;
}

quint16 XPE::getLoadConfig_CSDVersion()
{
    quint16 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,CSDVersion));
        }
        else
        {
            nResult=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,CSDVersion));
        }
    }

    return nResult;
}

quint16 XPE::getLoadConfig_DependentLoadFlags()
{
    quint16 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,DependentLoadFlags));
        }
        else
        {
            nResult=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,DependentLoadFlags));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_EditList()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,EditList));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,EditList));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_SecurityCookie()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,SecurityCookie));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,SecurityCookie));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_SEHandlerTable()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,SEHandlerTable));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,SEHandlerTable));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_SEHandlerCount()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,SEHandlerCount));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,SEHandlerCount));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_GuardCFCheckFunctionPointer()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardCFCheckFunctionPointer));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardCFCheckFunctionPointer));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_GuardCFDispatchFunctionPointer()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardCFDispatchFunctionPointer));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardCFDispatchFunctionPointer));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_GuardCFFunctionTable()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardCFFunctionTable));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardCFFunctionTable));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_GuardCFFunctionCount()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardCFFunctionCount));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardCFFunctionCount));
        }
    }

    return nResult;
}

quint32 XPE::getLoadConfig_GuardFlags()
{
    quint32 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardFlags));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardFlags));
        }
    }

    return nResult;
}

quint16 XPE::getLoadConfig_CodeIntegrity_Flags()
{
    quint16 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,CodeIntegrity.Flags));
        }
        else
        {
            nResult=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,CodeIntegrity.Flags));
        }
    }

    return nResult;
}

quint16 XPE::getLoadConfig_CodeIntegrity_Catalog()
{
    quint16 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,CodeIntegrity.Catalog));
        }
        else
        {
            nResult=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,CodeIntegrity.Catalog));
        }
    }

    return nResult;
}

quint32 XPE::getLoadConfig_CodeIntegrity_CatalogOffset()
{
    quint32 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,CodeIntegrity.CatalogOffset));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,CodeIntegrity.CatalogOffset));
        }
    }

    return nResult;
}

quint32 XPE::getLoadConfig_CodeIntegrity_Reserved()
{
    quint32 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,CodeIntegrity.Reserved));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,CodeIntegrity.Reserved));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_GuardAddressTakenIatEntryTable()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardAddressTakenIatEntryTable));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardAddressTakenIatEntryTable));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_GuardAddressTakenIatEntryCount()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardAddressTakenIatEntryCount));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardAddressTakenIatEntryCount));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_GuardLongJumpTargetTable()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardLongJumpTargetTable));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardLongJumpTargetTable));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_GuardLongJumpTargetCount()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardLongJumpTargetCount));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardLongJumpTargetCount));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_DynamicValueRelocTable()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,DynamicValueRelocTable));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,DynamicValueRelocTable));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_CHPEMetadataPointer()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,CHPEMetadataPointer));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,CHPEMetadataPointer));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_GuardRFFailureRoutine()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardRFFailureRoutine));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardRFFailureRoutine));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_GuardRFFailureRoutineFunctionPointer()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardRFFailureRoutineFunctionPointer));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardRFFailureRoutineFunctionPointer));
        }
    }

    return nResult;
}

quint32 XPE::getLoadConfig_DynamicValueRelocTableOffset()
{
    quint32 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,DynamicValueRelocTableOffset));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,DynamicValueRelocTableOffset));
        }
    }

    return nResult;
}

quint16 XPE::getLoadConfig_DynamicValueRelocTableSection()
{
    quint16 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,DynamicValueRelocTableSection));
        }
        else
        {
            nResult=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,DynamicValueRelocTableSection));
        }
    }

    return nResult;
}

quint16 XPE::getLoadConfig_Reserved2()
{
    quint16 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,Reserved2));
        }
        else
        {
            nResult=read_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,Reserved2));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_GuardRFVerifyStackPointerFunctionPointer()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardRFVerifyStackPointerFunctionPointer));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardRFVerifyStackPointerFunctionPointer));
        }
    }

    return nResult;
}

quint32 XPE::getLoadConfig_HotPatchTableOffset()
{
    quint32 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,HotPatchTableOffset));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,HotPatchTableOffset));
        }
    }

    return nResult;
}

quint32 XPE::getLoadConfig_Reserved3()
{
    quint32 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,Reserved3));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,Reserved3));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_EnclaveConfigurationPointer()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,EnclaveConfigurationPointer));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,EnclaveConfigurationPointer));
        }
    }

    return nResult;
}

quint64 XPE::getLoadConfig_VolatileMetadataPointer()
{
    quint64 nResult=0;

    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,VolatileMetadataPointer));
        }
        else
        {
            nResult=read_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,VolatileMetadataPointer));
        }
    }

    return nResult;
}

void XPE::setLoadConfig_Size(quint32 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,Size),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,Size),nValue);
        }
    }
}

void XPE::setLoadConfig_TimeDateStamp(quint32 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,TimeDateStamp),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,TimeDateStamp),nValue);
        }
    }
}

void XPE::setLoadConfig_MajorVersion(quint16 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,MajorVersion),nValue);
        }
        else
        {
            write_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,MajorVersion),nValue);
        }
    }
}

void XPE::setLoadConfig_MinorVersion(quint16 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,MinorVersion),nValue);
        }
        else
        {
            write_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,MinorVersion),nValue);
        }
    }
}

void XPE::setLoadConfig_GlobalFlagsClear(quint32 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GlobalFlagsClear),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GlobalFlagsClear),nValue);
        }
    }
}

void XPE::setLoadConfig_GlobalFlagsSet(quint32 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GlobalFlagsSet),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GlobalFlagsSet),nValue);
        }
    }
}

void XPE::setLoadConfig_CriticalSectionDefaultTimeout(quint32 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,CriticalSectionDefaultTimeout),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,CriticalSectionDefaultTimeout),nValue);
        }
    }
}

void XPE::setLoadConfig_DeCommitFreeBlockThreshold(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,DeCommitFreeBlockThreshold),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,DeCommitFreeBlockThreshold),nValue);
        }
    }
}

void XPE::setLoadConfig_DeCommitTotalFreeThreshold(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,DeCommitTotalFreeThreshold),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,DeCommitTotalFreeThreshold),nValue);
        }
    }
}

void XPE::setLoadConfig_LockPrefixTable(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,LockPrefixTable),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,LockPrefixTable),nValue);
        }
    }
}

void XPE::setLoadConfig_MaximumAllocationSize(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,MaximumAllocationSize),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,MaximumAllocationSize),nValue);
        }
    }
}

void XPE::setLoadConfig_VirtualMemoryThreshold(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,VirtualMemoryThreshold),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,VirtualMemoryThreshold),nValue);
        }
    }
}

void XPE::setLoadConfig_ProcessAffinityMask(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,ProcessAffinityMask),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,ProcessAffinityMask),nValue);
        }
    }
}

void XPE::setLoadConfig_CSDVersion(quint16 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,CSDVersion),nValue);
        }
        else
        {
            write_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,CSDVersion),nValue);
        }
    }
}

void XPE::setLoadConfig_DependentLoadFlags(quint16 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,DependentLoadFlags),nValue);
        }
        else
        {
            write_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,DependentLoadFlags),nValue);
        }
    }
}

void XPE::setLoadConfig_EditList(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,EditList),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,EditList),nValue);
        }
    }
}

void XPE::setLoadConfig_SecurityCookie(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,SecurityCookie),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,SecurityCookie),nValue);
        }
    }
}

void XPE::setLoadConfig_SEHandlerTable(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,SEHandlerTable),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,SEHandlerTable),nValue);
        }
    }
}

void XPE::setLoadConfig_SEHandlerCount(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,SEHandlerCount),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,SEHandlerCount),nValue);
        }
    }
}

void XPE::setLoadConfig_GuardCFCheckFunctionPointer(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardCFCheckFunctionPointer),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardCFCheckFunctionPointer),nValue);
        }
    }
}

void XPE::setLoadConfig_GuardCFDispatchFunctionPointer(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardCFDispatchFunctionPointer),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardCFDispatchFunctionPointer),nValue);
        }
    }
}

void XPE::setLoadConfig_GuardCFFunctionTable(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardCFFunctionTable),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardCFFunctionTable),nValue);
        }
    }
}

void XPE::setLoadConfig_GuardCFFunctionCount(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardCFFunctionCount),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardCFFunctionCount),nValue);
        }
    }
}

void XPE::setLoadConfig_GuardFlags(quint32 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardFlags),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardFlags),nValue);
        }
    }
}

void XPE::setLoadConfig_CodeIntegrity_Flags(quint16 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,CodeIntegrity.Flags),nValue);
        }
        else
        {
            write_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,CodeIntegrity.Flags),nValue);
        }
    }
}

void XPE::setLoadConfig_CodeIntegrity_Catalog(quint16 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,CodeIntegrity.Catalog),nValue);
        }
        else
        {
            write_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,CodeIntegrity.Catalog),nValue);
        }
    }
}

void XPE::setLoadConfig_CodeIntegrity_CatalogOffset(quint32 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,CodeIntegrity.CatalogOffset),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,CodeIntegrity.CatalogOffset),nValue);
        }
    }
}

void XPE::setLoadConfig_CodeIntegrity_Reserved(quint32 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,CodeIntegrity.Reserved),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,CodeIntegrity.Reserved),nValue);
        }
    }
}

void XPE::setLoadConfig_GuardAddressTakenIatEntryTable(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardAddressTakenIatEntryTable),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardAddressTakenIatEntryTable),nValue);
        }
    }
}

void XPE::setLoadConfig_GuardAddressTakenIatEntryCount(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardAddressTakenIatEntryCount),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardAddressTakenIatEntryCount),nValue);
        }
    }
}

void XPE::setLoadConfig_GuardLongJumpTargetTable(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardLongJumpTargetTable),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardLongJumpTargetTable),nValue);
        }
    }
}

void XPE::setLoadConfig_GuardLongJumpTargetCount(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardLongJumpTargetTable),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardLongJumpTargetTable),nValue);
        }
    }
}

void XPE::setLoadConfig_DynamicValueRelocTable(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,DynamicValueRelocTable),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,DynamicValueRelocTable),nValue);
        }
    }
}

void XPE::setLoadConfig_CHPEMetadataPointer(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,CHPEMetadataPointer),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,CHPEMetadataPointer),nValue);
        }
    }
}

void XPE::setLoadConfig_GuardRFFailureRoutine(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardRFFailureRoutine),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardRFFailureRoutine),nValue);
        }
    }
}

void XPE::setLoadConfig_GuardRFFailureRoutineFunctionPointer(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardRFFailureRoutineFunctionPointer),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardRFFailureRoutineFunctionPointer),nValue);
        }
    }
}

void XPE::setLoadConfig_DynamicValueRelocTableOffset(quint32 nValue)
{
    qint32 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,DynamicValueRelocTableOffset),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,DynamicValueRelocTableOffset),nValue);
        }
    }
}

void XPE::setLoadConfig_DynamicValueRelocTableSection(quint16 nValue)
{
    qint32 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,DynamicValueRelocTableSection),nValue);
        }
        else
        {
            write_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,DynamicValueRelocTableSection),nValue);
        }
    }
}

void XPE::setLoadConfig_Reserved2(quint16 nValue)
{
    qint32 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,Reserved2),nValue);
        }
        else
        {
            write_uint16(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,Reserved2),nValue);
        }
    }
}

void XPE::setLoadConfig_GuardRFVerifyStackPointerFunctionPointer(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,GuardRFVerifyStackPointerFunctionPointer),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,GuardRFVerifyStackPointerFunctionPointer),nValue);
        }
    }
}

void XPE::setLoadConfig_HotPatchTableOffset(quint32 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,HotPatchTableOffset),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,HotPatchTableOffset),nValue);
        }
    }
}

void XPE::setLoadConfig_Reserved3(quint32 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,Reserved3),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,Reserved3),nValue);
        }
    }
}

void XPE::setLoadConfig_EnclaveConfigurationPointer(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,EnclaveConfigurationPointer),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,EnclaveConfigurationPointer),nValue);
        }
    }
}

void XPE::setLoadConfig_VolatileMetadataPointer(quint64 nValue)
{
    qint64 nLoadConfigOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);

    if(nLoadConfigOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY64,VolatileMetadataPointer),nValue);
        }
        else
        {
            write_uint32(nLoadConfigOffset+offsetof(XPE_DEF::S_IMAGE_LOAD_CONFIG_DIRECTORY32,VolatileMetadataPointer),nValue);
        }
    }
}

XPE_DEF::S_IMAGE_RUNTIME_FUNCTION_ENTRY XPE::_read_IMAGE_RUNTIME_FUNCTION_ENTRY(qint64 nOffset)
{
    XPE_DEF::S_IMAGE_RUNTIME_FUNCTION_ENTRY result={};

    result.BeginAddress=read_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_RUNTIME_FUNCTION_ENTRY,BeginAddress));
    result.EndAddress=read_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_RUNTIME_FUNCTION_ENTRY,EndAddress));
    result.UnwindInfoAddress=read_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_RUNTIME_FUNCTION_ENTRY,UnwindInfoAddress));

    return result;
}

XPE_DEF::S_IMAGE_RUNTIME_FUNCTION_ENTRY XPE::_getException(qint32 nNumber)
{
    XPE_DEF::S_IMAGE_RUNTIME_FUNCTION_ENTRY result={};

    qint64 nExceptionOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXCEPTION);

    if(nExceptionOffset!=-1)
    {
        result=_read_IMAGE_RUNTIME_FUNCTION_ENTRY(nExceptionOffset+nNumber*sizeof(XPE_DEF::S_IMAGE_RUNTIME_FUNCTION_ENTRY));
    }

    return result;
}

void XPE::setException_BeginAddress(qint32 nNumber, quint32 nValue)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXCEPTION+nNumber*sizeof(XPE_DEF::S_IMAGE_RUNTIME_FUNCTION_ENTRY));

    write_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_RUNTIME_FUNCTION_ENTRY,BeginAddress),nValue);
}

void XPE::setException_EndAddress(qint32 nNumber, quint32 nValue)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXCEPTION+nNumber*sizeof(XPE_DEF::S_IMAGE_RUNTIME_FUNCTION_ENTRY));

    write_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_RUNTIME_FUNCTION_ENTRY,EndAddress),nValue);
}

void XPE::setException_UnwindInfoAddress(qint32 nNumber, quint32 nValue)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXCEPTION+nNumber*sizeof(XPE_DEF::S_IMAGE_RUNTIME_FUNCTION_ENTRY));

    write_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_RUNTIME_FUNCTION_ENTRY,UnwindInfoAddress),nValue);
}

qint64 XPE::getExceptionRecordOffset(qint32 nNumber)
{
    qint64 nResult=-1;

    qint64 nExceptionOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXCEPTION);

    if(nExceptionOffset!=-1)
    {
        nResult=nExceptionOffset+nNumber*sizeof(XPE_DEF::S_IMAGE_RUNTIME_FUNCTION_ENTRY);
    }

    return nResult;
}

qint64 XPE::getExceptionRecordSize()
{
    return sizeof(XPE_DEF::S_IMAGE_RUNTIME_FUNCTION_ENTRY);
}

QList<XPE_DEF::S_IMAGE_RUNTIME_FUNCTION_ENTRY> XPE::getExceptionsList()
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getExceptionsList(&memoryMap);
}

QList<XPE_DEF::S_IMAGE_RUNTIME_FUNCTION_ENTRY> XPE::getExceptionsList(XBinary::_MEMORY_MAP *pMemoryMap)
{
    QList<XPE_DEF::S_IMAGE_RUNTIME_FUNCTION_ENTRY> listResult;

    qint64 nExceptionOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXCEPTION);

    if(nExceptionOffset!=-1)
    {
        while(true)
        {
            XPE_DEF::S_IMAGE_RUNTIME_FUNCTION_ENTRY record=_read_IMAGE_RUNTIME_FUNCTION_ENTRY(nExceptionOffset);

            if( record.BeginAddress&&
                record.EndAddress&&
                isAddressValid(pMemoryMap,pMemoryMap->nBaseAddress+record.BeginAddress)&&
                isAddressValid(pMemoryMap,pMemoryMap->nBaseAddress+record.EndAddress))
            {
                listResult.append(record);
            }
            else
            {
                break;
            }

            nExceptionOffset+=sizeof(XPE_DEF::S_IMAGE_RUNTIME_FUNCTION_ENTRY);
        }
    }

    return listResult;
}

XPE_DEF::S_IMAGE_DEBUG_DIRECTORY XPE::_read_IMAGE_DEBUG_DIRECTORY(qint64 nOffset)
{
    XPE_DEF::S_IMAGE_DEBUG_DIRECTORY result={};

    result.Characteristics=read_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DEBUG_DIRECTORY,Characteristics));
    result.TimeDateStamp=read_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DEBUG_DIRECTORY,TimeDateStamp));
    result.MajorVersion=read_uint16(nOffset+offsetof(XPE_DEF::S_IMAGE_DEBUG_DIRECTORY,MajorVersion));
    result.MinorVersion=read_uint16(nOffset+offsetof(XPE_DEF::S_IMAGE_DEBUG_DIRECTORY,MinorVersion));
    result.Type=read_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DEBUG_DIRECTORY,Type));
    result.SizeOfData=read_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DEBUG_DIRECTORY,SizeOfData));
    result.AddressOfRawData=read_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DEBUG_DIRECTORY,AddressOfRawData));
    result.PointerToRawData=read_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DEBUG_DIRECTORY,PointerToRawData));

    return result;
}

QList<XPE_DEF::S_IMAGE_DEBUG_DIRECTORY> XPE::getDebugList()
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getDebugList(&memoryMap);
}

QList<XPE_DEF::S_IMAGE_DEBUG_DIRECTORY> XPE::getDebugList(XBinary::_MEMORY_MAP *pMemoryMap)
{
    QList<XPE_DEF::S_IMAGE_DEBUG_DIRECTORY>  listResult;

    qint64 nDebugOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_DEBUG);

    if(nDebugOffset!=-1)
    {
        while(true)
        {
            XPE_DEF::S_IMAGE_DEBUG_DIRECTORY record=_read_IMAGE_DEBUG_DIRECTORY(nDebugOffset);

            if( record.AddressOfRawData&&
                record.PointerToRawData&&
                isAddressValid(pMemoryMap,pMemoryMap->nBaseAddress+record.AddressOfRawData)&&
                isOffsetValid(pMemoryMap,record.PointerToRawData))
            {
                listResult.append(record);
            }
            else
            {
                break;
            }

            nDebugOffset+=sizeof(XPE_DEF::S_IMAGE_DEBUG_DIRECTORY);
        }
    }

    return listResult;
}

qint64 XPE::getDebugHeaderOffset(quint32 nNumber)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_DEBUG);

    nOffset+=sizeof(XPE_DEF::S_IMAGE_DEBUG_DIRECTORY)*nNumber;

    return nOffset;
}

qint64 XPE::getDebugHeaderSize()
{
    return sizeof(XPE_DEF::S_IMAGE_DEBUG_DIRECTORY);
}

XPE_DEF::S_IMAGE_DEBUG_DIRECTORY XPE::getDebugHeader(quint32 nNumber)
{
    XPE_DEF::S_IMAGE_DEBUG_DIRECTORY result={};

    // TODO Check number of headers

    qint64 nDebugOffset=getDebugHeaderOffset(nNumber);

    result=_read_IMAGE_DEBUG_DIRECTORY(nDebugOffset);

    return result;
}

void XPE::setDebugHeader_Characteristics(quint32 nNumber, quint32 nValue)
{
    qint64 nOffset=getDebugHeaderOffset(nNumber);

    write_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DEBUG_DIRECTORY,Characteristics),nValue);
}

void XPE::setDebugHeader_TimeDateStamp(quint32 nNumber, quint32 nValue)
{
    qint64 nOffset=getDebugHeaderOffset(nNumber);

    write_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DEBUG_DIRECTORY,TimeDateStamp),nValue);
}

void XPE::setDebugHeader_MajorVersion(quint32 nNumber, quint16 nValue)
{
    qint64 nOffset=getDebugHeaderOffset(nNumber);

    write_uint16(nOffset+offsetof(XPE_DEF::S_IMAGE_DEBUG_DIRECTORY,MajorVersion),nValue);
}

void XPE::setDebugHeader_MinorVersion(quint32 nNumber, quint16 nValue)
{
    qint64 nOffset=getDebugHeaderOffset(nNumber);

    write_uint16(nOffset+offsetof(XPE_DEF::S_IMAGE_DEBUG_DIRECTORY,MinorVersion),nValue);
}

void XPE::setDebugHeader_Type(quint32 nNumber, quint32 nValue)
{
    qint64 nOffset=getDebugHeaderOffset(nNumber);

    write_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DEBUG_DIRECTORY,Type),nValue);
}

void XPE::setDebugHeader_SizeOfData(quint32 nNumber, quint32 nValue)
{
    qint64 nOffset=getDebugHeaderOffset(nNumber);

    write_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DEBUG_DIRECTORY,SizeOfData),nValue);
}

void XPE::setDebugHeader_AddressOfRawData(quint32 nNumber, quint32 nValue)
{
    qint64 nOffset=getDebugHeaderOffset(nNumber);

    write_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DEBUG_DIRECTORY,AddressOfRawData),nValue);
}

void XPE::setDebugHeader_PointerToRawData(quint32 nNumber, quint32 nValue)
{
    qint64 nOffset=getDebugHeaderOffset(nNumber);

    write_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DEBUG_DIRECTORY,PointerToRawData),nValue);
}

qint64 XPE::getDelayImportRecordOffset(qint32 nNumber)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);

    nOffset+=sizeof(XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR)*nNumber;

    return nOffset;
}

qint64 XPE::getDelayImportRecordSize()
{
    return sizeof(XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR);
}

QList<XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR> XPE::getDelayImportsList()
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getDelayImportsList(&memoryMap);
}

QList<XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR> XPE::getDelayImportsList(XBinary::_MEMORY_MAP *pMemoryMap)
{
    QList<XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR>  listResult;

    qint64 nDelayImportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);

    if(nDelayImportOffset!=-1)
    {
        while(true)
        {
            XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR record=_read_IMAGE_DELAYLOAD_DESCRIPTOR(nDelayImportOffset);

            if( record.DllNameRVA&&
                isAddressValid(pMemoryMap,pMemoryMap->nBaseAddress+record.DllNameRVA))
            {
                listResult.append(record);
            }
            else
            {
                break;
            }

            nDelayImportOffset+=sizeof(XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR);
        }
    }

    return listResult;
}

void XPE::setDelayImport_AllAttributes(quint32 nNumber, quint32 nValue)
{
    qint64 nOffset=getDelayImportRecordOffset(nNumber);

    write_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR,AllAttributes),nValue);
}

void XPE::setDelayImport_DllNameRVA(quint32 nNumber, quint32 nValue)
{
    qint64 nOffset=getDelayImportRecordOffset(nNumber);

    write_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR,DllNameRVA),nValue);
}

void XPE::setDelayImport_ModuleHandleRVA(quint32 nNumber, quint32 nValue)
{
    qint64 nOffset=getDelayImportRecordOffset(nNumber);

    write_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR,ModuleHandleRVA),nValue);
}

void XPE::setDelayImport_ImportAddressTableRVA(quint32 nNumber, quint32 nValue)
{
    qint64 nOffset=getDelayImportRecordOffset(nNumber);

    write_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR,ImportAddressTableRVA),nValue);
}

void XPE::setDelayImport_ImportNameTableRVA(quint32 nNumber, quint32 nValue)
{
    qint64 nOffset=getDelayImportRecordOffset(nNumber);

    write_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR,ImportNameTableRVA),nValue);
}

void XPE::setDelayImport_BoundImportAddressTableRVA(quint32 nNumber, quint32 nValue)
{
    qint64 nOffset=getDelayImportRecordOffset(nNumber);

    write_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR,BoundImportAddressTableRVA),nValue);
}

void XPE::setDelayImport_UnloadInformationTableRVA(quint32 nNumber, quint32 nValue)
{
    qint64 nOffset=getDelayImportRecordOffset(nNumber);

    write_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR,UnloadInformationTableRVA),nValue);
}

void XPE::setDelayImport_TimeDateStamp(quint32 nNumber, quint32 nValue)
{
    qint64 nOffset=getDelayImportRecordOffset(nNumber);

    write_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR,TimeDateStamp),nValue);
}

QList<XPE::DELAYIMPORT_POSITION> XPE::getDelayImportPositions(int nIndex)
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getDelayImportPositions(&memoryMap,nIndex);
}

QList<XPE::DELAYIMPORT_POSITION> XPE::getDelayImportPositions(XBinary::_MEMORY_MAP *pMemoryMap, int nIndex)
{
    QList<DELAYIMPORT_POSITION> listResult;

    qint64 nDelayImportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);

    if(nDelayImportOffset!=-1)
    {
        nDelayImportOffset+=sizeof(XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR)*nIndex;

        XPE_DEF::S_IMAGE_DELAYLOAD_DESCRIPTOR idd=_read_IMAGE_DELAYLOAD_DESCRIPTOR(nDelayImportOffset);

        qint64 nNameThunksRVA=idd.ImportNameTableRVA;
        qint64 nAddressThunksRVA=idd.ImportAddressTableRVA;
        qint64 nBoundThunksRVA=idd.BoundImportAddressTableRVA;

        qint64 nNameThunksOffset=XBinary::relAddressToOffset(pMemoryMap,nNameThunksRVA);
        qint64 nAddressThunksOffset=XBinary::relAddressToOffset(pMemoryMap,nAddressThunksRVA);
        qint64 nBoundThunksOffset=XBinary::relAddressToOffset(pMemoryMap,nBoundThunksRVA);

        bool bIs64=is64(pMemoryMap);

        while(true)
        {
            DELAYIMPORT_POSITION importPosition={};
            importPosition.nNameThunkOffset=nNameThunksOffset;
            importPosition.nNameThunkRVA=nNameThunksRVA;
            importPosition.nAddressThunkOffset=nNameThunksOffset;
            importPosition.nAddressThunkRVA=nAddressThunksOffset;
            importPosition.nBoundThunkOffset=nBoundThunksOffset;
            importPosition.nBoundThunkRVA=nBoundThunksRVA;

            if(bIs64)
            {
                importPosition.nNameThunkValue=read_uint64(nNameThunksOffset);
                importPosition.nAddressThunkValue=read_uint64(nAddressThunksOffset);
                importPosition.nBoundThunkValue=read_uint64(nBoundThunksOffset);

                if(importPosition.nNameThunkValue==0)
                {
                    break;
                }

                // mb TODO check importPosition.nAddressThunkValue

                if(!(importPosition.nNameThunkValue&0x8000000000000000))
                {
                    qint64 nOffset=addressToOffset(pMemoryMap,importPosition.nNameThunkValue+pMemoryMap->nBaseAddress);

                    if(nOffset!=-1)
                    {
                        importPosition.nHint=read_uint16(nOffset);
                        importPosition.sName=read_ansiString(nOffset+2);

                        if(importPosition.sName=="")
                        {
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    importPosition.nOrdinal=importPosition.nNameThunkValue&0x7FFFFFFFFFFFFFFF;
                }
            }
            else
            {
                importPosition.nNameThunkValue=read_uint32(nNameThunksOffset);
                importPosition.nAddressThunkValue=read_uint32(nAddressThunksOffset);
                importPosition.nBoundThunkValue=read_uint32(nBoundThunksOffset);

                if(importPosition.nNameThunkValue==0)
                {
                    break;
                }

                if(!(importPosition.nNameThunkValue&0x80000000))
                {
                    qint64 nOffset=addressToOffset(pMemoryMap,importPosition.nNameThunkValue+pMemoryMap->nBaseAddress);

                    if(nOffset!=-1)
                    {
                        importPosition.nHint=read_uint16(nOffset);
                        importPosition.sName=read_ansiString(nOffset+2);

                        if(importPosition.sName=="")
                        {
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    importPosition.nOrdinal=importPosition.nNameThunkValue&0x7FFFFFFF;
                }
            }

            if(importPosition.nOrdinal==0)
            {
                importPosition.sFunction=importPosition.sName;
            }
            else
            {
                importPosition.sFunction=QString("%1").arg(importPosition.nOrdinal);
            }

            if(bIs64)
            {
                nNameThunksRVA+=8;
                nNameThunksOffset+=8;
                nAddressThunksRVA+=8;
                nAddressThunksOffset+=8;
                nBoundThunksRVA+=8;
                nBoundThunksOffset+=8;
            }
            else
            {
                nNameThunksRVA+=4;
                nNameThunksOffset+=4;
                nAddressThunksRVA+=4;
                nAddressThunksOffset+=4;
                nBoundThunksRVA+=4;
                nBoundThunksOffset+=4;
            }

            listResult.append(importPosition);
        }
    }

    return listResult;
}

QList<XPE::BOUND_IMPORT_POSITION> XPE::getBoundImportPositions()
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getBoundImportPositions(&memoryMap);
}

QList<XPE::BOUND_IMPORT_POSITION> XPE::getBoundImportPositions(XBinary::_MEMORY_MAP *pMemoryMap)
{
    Q_UNUSED(pMemoryMap)

    QList<BOUND_IMPORT_POSITION> listResult;

    qint64 nBoundImportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT);

    if(nBoundImportOffset!=-1)
    {
        qint64 nOffset=nBoundImportOffset;

        while(true)
        {
            BOUND_IMPORT_POSITION record={};

            record.descriptor=_read_IMAGE_BOUND_IMPORT_DESCRIPTOR(nOffset);

            if((record.descriptor.TimeDateStamp)&&(record.descriptor.OffsetModuleName))
            {
                record.sName=read_ansiString(nBoundImportOffset+record.descriptor.OffsetModuleName);

                listResult.append(record);
            }
            else
            {
                break;
            }

            nOffset+=sizeof(XPE_DEF::S_IMAGE_BOUND_IMPORT_DESCRIPTOR);
        }
    }

    return listResult;
}

XPE_DEF::S_IMAGE_BOUND_IMPORT_DESCRIPTOR XPE::_read_IMAGE_BOUND_IMPORT_DESCRIPTOR(qint64 nOffset)
{
    XPE_DEF::S_IMAGE_BOUND_IMPORT_DESCRIPTOR result={};

    result.TimeDateStamp=read_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_BOUND_IMPORT_DESCRIPTOR,TimeDateStamp));
    result.OffsetModuleName=read_uint16(nOffset+offsetof(XPE_DEF::S_IMAGE_BOUND_IMPORT_DESCRIPTOR,OffsetModuleName));
    result.NumberOfModuleForwarderRefs=read_uint16(nOffset+offsetof(XPE_DEF::S_IMAGE_BOUND_IMPORT_DESCRIPTOR,NumberOfModuleForwarderRefs));

    return result;
}

qint64 XPE::getBoundImportRecordOffset(qint32 nNumber)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT);

    nOffset+=sizeof(XPE_DEF::S_IMAGE_BOUND_IMPORT_DESCRIPTOR)*nNumber;

    return nOffset;
}

qint64 XPE::getBoundImportRecordSize()
{
    return sizeof(XPE_DEF::S_IMAGE_BOUND_IMPORT_DESCRIPTOR);
}

void XPE::setBoundImport_TimeDateStamp(quint32 nNumber, quint32 nValue)
{
    qint64 nOffset=getBoundImportRecordOffset(nNumber);

    write_uint32(nOffset+offsetof(XPE_DEF::S_IMAGE_BOUND_IMPORT_DESCRIPTOR,TimeDateStamp),nValue);
}

void XPE::setBoundImport_OffsetModuleName(quint32 nNumber, quint16 nValue)
{
    qint64 nOffset=getBoundImportRecordOffset(nNumber);

    write_uint16(nOffset+offsetof(XPE_DEF::S_IMAGE_BOUND_IMPORT_DESCRIPTOR,OffsetModuleName),nValue);
}

void XPE::setBoundImport_NumberOfModuleForwarderRefs(quint32 nNumber, quint16 nValue)
{
    qint64 nOffset=getBoundImportRecordOffset(nNumber);

    write_uint16(nOffset+offsetof(XPE_DEF::S_IMAGE_BOUND_IMPORT_DESCRIPTOR,NumberOfModuleForwarderRefs),nValue);
}

qint32 XPE::getNumberOfImports()
{
    QList<XPE_DEF::IMAGE_IMPORT_DESCRIPTOR> listImports=getImportDescriptors();

    return listImports.count();
}

QString XPE::getImportLibraryName(quint32 nNumber)
{
    QString sResult;

    QList<IMAGE_IMPORT_DESCRIPTOR_EX> listImports=getImportDescriptorsEx(); // TODO Check

    if(nNumber<(quint32)listImports.count())
    {
        sResult=listImports.at(nNumber).sLibrary;
    }

    return sResult;
}

QString XPE::getImportLibraryName(quint32 nNumber, QList<XPE::IMPORT_HEADER> *pListImport)
{
    QString sResult;

    if((qint32)nNumber<pListImport->count())
    {
        sResult=pListImport->at(nNumber).sName;
    }

    return sResult;
}

qint32 XPE::getNumberOfImportThunks(quint32 nNumber)
{
    QList<IMPORT_HEADER> listImportHeaders=getImports();

    return getNumberOfImportThunks(nNumber,&listImportHeaders);
}

qint32 XPE::getNumberOfImportThunks(quint32 nNumber, QList<XPE::IMPORT_HEADER> *pListImport)
{
    qint32 nResult=0;

    if(nNumber<(quint32)pListImport->count())
    {
        nResult=pListImport->at(nNumber).listPositions.count();
    }

    return nResult;
}

qint64 XPE::getNetHeaderOffset()
{
    return getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);
}

qint64 XPE::getNetHeaderSize()
{
    qint64 nResult=0;

    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        nResult=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,cb));
    }

    return nResult;
}

XPE_DEF::IMAGE_COR20_HEADER XPE::_read_IMAGE_COR20_HEADER(qint64 nOffset)
{
    XPE_DEF::IMAGE_COR20_HEADER result={};

    result.cb=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,cb));
    result.MajorRuntimeVersion=read_uint16(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,MajorRuntimeVersion));
    result.MinorRuntimeVersion=read_uint16(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,MinorRuntimeVersion));
    result.MetaData=read_IMAGE_DATA_DIRECTORY(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,MetaData));
    result.Flags=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,Flags));
    result.EntryPointRVA=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,EntryPointRVA));
    result.Resources=read_IMAGE_DATA_DIRECTORY(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,Resources));
    result.StrongNameSignature=read_IMAGE_DATA_DIRECTORY(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,StrongNameSignature));
    result.CodeManagerTable=read_IMAGE_DATA_DIRECTORY(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,CodeManagerTable));
    result.VTableFixups=read_IMAGE_DATA_DIRECTORY(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,VTableFixups));
    result.ExportAddressTableJumps=read_IMAGE_DATA_DIRECTORY(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,ExportAddressTableJumps));
    result.ManagedNativeHeader=read_IMAGE_DATA_DIRECTORY(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,ManagedNativeHeader));

    return result;
}

XPE_DEF::IMAGE_COR20_HEADER XPE::getNetHeader()
{
    XPE_DEF::IMAGE_COR20_HEADER result={};

    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        result=_read_IMAGE_COR20_HEADER(nOffset);
    }

    return result;
}

quint32 XPE::getNetHeader_cb()
{
    quint32 nResult=0;

    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        nResult=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,cb));
    }

    return nResult;
}

quint16 XPE::getNetHeader_MajorRuntimeVersion()
{
    quint16 nResult=0;

    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        nResult=read_uint16(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,MajorRuntimeVersion));
    }

    return nResult;
}

quint16 XPE::getNetHeader_MinorRuntimeVersion()
{
    quint16 nResult=0;

    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        nResult=read_uint16(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,MinorRuntimeVersion));
    }

    return nResult;
}

quint32 XPE::getNetHeader_MetaData_Address()
{
    quint32 nResult=0;

    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        nResult=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,MetaData.VirtualAddress));
    }

    return nResult;
}

quint32 XPE::getNetHeader_MetaData_Size()
{
    quint32 nResult=0;

    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        nResult=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,MetaData.Size));
    }

    return nResult;
}

quint32 XPE::getNetHeader_Flags()
{
    quint32 nResult=0;

    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        nResult=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,Flags));
    }

    return nResult;
}

quint32 XPE::getNetHeader_EntryPoint()
{
    quint32 nResult=0;

    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        nResult=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,EntryPointRVA));
    }

    return nResult;
}

quint32 XPE::getNetHeader_Resources_Address()
{
    quint32 nResult=0;

    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        nResult=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,Resources.VirtualAddress));
    }

    return nResult;
}

quint32 XPE::getNetHeader_Resources_Size()
{
    quint32 nResult=0;

    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        nResult=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,Resources.Size));
    }

    return nResult;
}

quint32 XPE::getNetHeader_StrongNameSignature_Address()
{
    quint32 nResult=0;

    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        nResult=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,StrongNameSignature.VirtualAddress));
    }

    return nResult;
}

quint32 XPE::getNetHeader_StrongNameSignature_Size()
{
    quint32 nResult=0;

    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        nResult=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,StrongNameSignature.Size));
    }

    return nResult;
}

quint32 XPE::getNetHeader_CodeManagerTable_Address()
{
    quint32 nResult=0;

    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        nResult=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,CodeManagerTable.VirtualAddress));
    }

    return nResult;
}

quint32 XPE::getNetHeader_CodeManagerTable_Size()
{
    quint32 nResult=0;

    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        nResult=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,CodeManagerTable.Size));
    }

    return nResult;
}

quint32 XPE::getNetHeader_VTableFixups_Address()
{
    quint32 nResult=0;

    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        nResult=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,VTableFixups.VirtualAddress));
    }

    return nResult;
}

quint32 XPE::getNetHeader_VTableFixups_Size()
{
    quint32 nResult=0;

    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        nResult=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,VTableFixups.Size));
    }

    return nResult;
}

quint32 XPE::getNetHeader_ExportAddressTableJumps_Address()
{
    quint32 nResult=0;

    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        nResult=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,ExportAddressTableJumps.VirtualAddress));
    }

    return nResult;
}

quint32 XPE::getNetHeader_ExportAddressTableJumps_Size()
{
    quint32 nResult=0;

    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        nResult=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,ExportAddressTableJumps.Size));
    }

    return nResult;
}

quint32 XPE::getNetHeader_ManagedNativeHeader_Address()
{
    quint32 nResult=0;

    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        nResult=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,ManagedNativeHeader.VirtualAddress));
    }

    return nResult;
}

quint32 XPE::getNetHeader_ManagedNativeHeader_Size()
{
    quint32 nResult=0;

    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        nResult=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,ManagedNativeHeader.Size));
    }

    return nResult;
}

void XPE::setNetHeader_cb(quint32 nValue)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,cb),nValue);
    }
}

void XPE::setNetHeader_MajorRuntimeVersion(quint16 nValue)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        write_uint16(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,MajorRuntimeVersion),nValue);
    }
}

void XPE::setNetHeader_MinorRuntimeVersion(quint16 nValue)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        write_uint16(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,MinorRuntimeVersion),nValue);
    }
}

void XPE::setNetHeader_MetaData_Address(quint32 nValue)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,MetaData.VirtualAddress),nValue);
    }
}

void XPE::setNetHeader_MetaData_Size(quint32 nValue)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,MetaData.Size),nValue);
    }
}

void XPE::setNetHeader_Flags(quint32 nValue)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,Flags),nValue);
    }
}

void XPE::setNetHeader_EntryPoint(quint32 nValue)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,EntryPointRVA),nValue);
    }
}

void XPE::setNetHeader_Resources_Address(quint32 nValue)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,Resources.VirtualAddress),nValue);
    }
}

void XPE::setNetHeader_Resources_Size(quint32 nValue)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,Resources.Size),nValue);
    }
}

void XPE::setNetHeader_StrongNameSignature_Address(quint32 nValue)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,StrongNameSignature.VirtualAddress),nValue);
    }
}

void XPE::setNetHeader_StrongNameSignature_Size(quint32 nValue)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,StrongNameSignature.Size),nValue);
    }
}

void XPE::setNetHeader_CodeManagerTable_Address(quint32 nValue)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,CodeManagerTable.VirtualAddress),nValue);
    }
}

void XPE::setNetHeader_CodeManagerTable_Size(quint32 nValue)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,CodeManagerTable.Size),nValue);
    }
}

void XPE::setNetHeader_VTableFixups_Address(quint32 nValue)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,VTableFixups.VirtualAddress),nValue);
    }
}

void XPE::setNetHeader_VTableFixups_Size(quint32 nValue)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,VTableFixups.Size),nValue);
    }
}

void XPE::setNetHeader_ExportAddressTableJumps_Address(quint32 nValue)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,ExportAddressTableJumps.VirtualAddress),nValue);
    }
}

void XPE::setNetHeader_ExportAddressTableJumps_Size(quint32 nValue)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,ExportAddressTableJumps.Size),nValue);
    }
}

void XPE::setNetHeader_ManagedNativeHeader_Address(quint32 nValue)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,ManagedNativeHeader.VirtualAddress),nValue);
    }
}

void XPE::setNetHeader_ManagedNativeHeader_Size(quint32 nValue)
{
    qint64 nOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

    if(nOffset!=-1)
    {
        write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_COR20_HEADER,ManagedNativeHeader.Size),nValue);
    }
}

qint64 XPE::calculateHeadersSize()
{
    return _calculateHeadersSize(getSectionsTableOffset(),getFileHeader_NumberOfSections());
}

qint64 XPE::_calculateHeadersSize(qint64 nSectionsTableOffset, quint32 nNumberOfSections)
{
    qint64 nHeadersSize=nSectionsTableOffset+sizeof(XPE_DEF::IMAGE_SECTION_HEADER)*nNumberOfSections;
    quint32 nFileAlignment=getOptionalHeader_FileAlignment();
    nHeadersSize=S_ALIGN_UP(nHeadersSize,nFileAlignment);

    return nHeadersSize;
}

bool XPE::isDll()
{
    return (getType()==TYPE_DLL);
}

bool XPE::isDll(QString sFileName)
{
    bool bResult=false;

    QFile file;
    file.setFileName(sFileName);

    if(file.open(QIODevice::ReadOnly))
    {
        XPE pe(&file);

        if(pe.isValid())
        {
            bResult=pe.isDll();
        }

        file.close();
    }

    return bResult;
}

bool XPE::isConsole()
{
    return (getType()==TYPE_CONSOLE);
}

bool XPE::isDriver()
{
    return (getType()==TYPE_DRIVER);
}

bool XPE::isNETPresent()
{
    // TODO more checks
    return isOptionalHeader_DataDirectoryPresent(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);
}

XPE::CLI_INFO XPE::getCliInfo(bool bFindHidden)
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getCliInfo(bFindHidden,&memoryMap);
}

XPE::CLI_INFO XPE::getCliInfo(bool bFindHidden, XBinary::_MEMORY_MAP *pMemoryMap)
{
    CLI_INFO result={};

    if(isNETPresent()||bFindHidden)
    {
        qint64 nBaseAddress=pMemoryMap->nBaseAddress;

        qint64 nCLIHeaderOffset=-1;

        if(isNETPresent())
        {
            XPE_DEF::IMAGE_DATA_DIRECTORY _idd=getOptionalHeader_DataDirectory(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

            nCLIHeaderOffset=addressToOffset(pMemoryMap,nBaseAddress+_idd.VirtualAddress);
        }
        else
        {
            // mb TODO
            // TODO Check!
            nCLIHeaderOffset=addressToOffset(pMemoryMap,nBaseAddress+0x2008);
            result.bHidden=true;
        }

        if(nCLIHeaderOffset!=-1)
        {
            result.nHeaderOffset=nCLIHeaderOffset;

            result.header=_read_IMAGE_COR20_HEADER(result.nHeaderOffset);

            if((result.header.cb==0x48)&&result.header.MetaData.VirtualAddress&&result.header.MetaData.Size)
            {
                result.bValid=true;

                result.metaData.nEntryPointSize=0;
                result.metaData.nEntryPoint=result.header.EntryPointRVA;

                result.nMetaDataOffset=addressToOffset(pMemoryMap,nBaseAddress+result.header.MetaData.VirtualAddress);

                if(result.nMetaDataOffset!=-1)
                {
                    result.metaData.header=_read_MetadataHeader(result.nMetaDataOffset);

                    if(result.metaData.header.nSignature==0x424a5342)
                    {
                        // result.bInit=true;
                        qint64 nOffset=result.nMetaDataOffset+20+result.metaData.header.nVersionStringLength;

                        for(int i=0; i<result.metaData.header.nStreams; i++)
                        {
                            CLI_METADATA_STREAM stream={};

                            stream.nOffset=read_uint32(nOffset+0)+result.nMetaDataOffset;
                            stream.nSize=read_uint32(nOffset+4);
                            stream.sName=read_ansiString(nOffset+8);

                            result.metaData.listStreams.append(stream);

                            if(result.metaData.listStreams.at(i).sName=="#~")
                            {
                                result.metaData.nTablesHeaderOffset=result.metaData.listStreams.at(i).nOffset;
                                result.metaData.nTablesSize=result.metaData.listStreams.at(i).nSize;
                            }
                            else if(result.metaData.listStreams.at(i).sName=="#Strings")
                            {
                                result.metaData.nStringsOffset=result.metaData.listStreams.at(i).nOffset;
                                result.metaData.nStringsSize=result.metaData.listStreams.at(i).nSize;

                                QByteArray baStrings=read_array(result.metaData.nStringsOffset,result.metaData.nStringsSize);

                                char *_pOffset=baStrings.data();
                                int _nSize=baStrings.size();

                                for(int i=1; i<_nSize; i++)
                                {
                                    _pOffset++;
                                    QString sTemp=_pOffset;
                                    result.metaData.listAnsiStrings.append(sTemp);

                                    _pOffset+=sTemp.size();
                                    i+=sTemp.size();
                                }
                            }
                            else if(result.metaData.listStreams.at(i).sName=="#US")
                            {
                                result.metaData.nUSOffset=result.metaData.listStreams.at(i).nOffset;
                                result.metaData.nUSSize=result.metaData.listStreams.at(i).nSize;

                                QByteArray baStrings=read_array(result.metaData.nUSOffset,result.metaData.nUSSize);

                                char *pStringOffset=baStrings.data();
                                char *pStringCurrentOffsetOffset=pStringOffset;
                                int _nSize=baStrings.size();

                                pStringCurrentOffsetOffset++;

                                for(int i=1; i<_nSize; i++)
                                {
                                    int nStringSize=(*((unsigned char *)pStringCurrentOffsetOffset));

                                    if(nStringSize==0x80)
                                    {
                                        nStringSize=0;
                                    }

                                    if(nStringSize>_nSize-i)
                                    {
                                        break;
                                    }

                                    pStringCurrentOffsetOffset++;

                                    if(pStringCurrentOffsetOffset>pStringOffset+_nSize)
                                    {
                                        break;
                                    }

                                    QString sTemp=QString::fromUtf16((ushort *)pStringCurrentOffsetOffset,nStringSize/2);

                                    result.metaData.listUnicodeStrings.append(sTemp);

                                    pStringCurrentOffsetOffset+=nStringSize;
                                    i+=nStringSize;
                                }
                            }
                            else if(result.metaData.listStreams.at(i).sName=="#Blob")
                            {
                                result.metaData.nBlobOffset=result.metaData.listStreams.at(i).nOffset;
                                result.metaData.nBlobSize=result.metaData.listStreams.at(i).nSize;
                            }
                            else if(result.metaData.listStreams.at(i).sName=="#GUID")
                            {
                                result.metaData.nGUIDOffset=result.metaData.listStreams.at(i).nOffset;
                                result.metaData.nGUIDSize=result.metaData.listStreams.at(i).nSize;
                            }

                            nOffset+=8;
                            nOffset+=S_ALIGN_UP((result.metaData.listStreams.at(i).sName.length()+1),4);
                        }

                        if(result.metaData.nTablesHeaderOffset)
                        {
                            result.metaData.nTables_Reserved1=read_uint32(result.metaData.nTablesHeaderOffset);
                            result.metaData.cTables_MajorVersion=read_uint8(result.metaData.nTablesHeaderOffset+4);
                            result.metaData.cTables_MinorVersion=read_uint8(result.metaData.nTablesHeaderOffset+5);
                            result.metaData.cTables_HeapOffsetSizes=read_uint8(result.metaData.nTablesHeaderOffset+6);
                            result.metaData.cTables_Reserved2=read_uint8(result.metaData.nTablesHeaderOffset+7);
                            result.metaData.nTables_Valid=read_uint64(result.metaData.nTablesHeaderOffset+8);
                            result.metaData.nTables_Sorted=read_uint64(result.metaData.nTablesHeaderOffset+16);

                            quint64 nValid=result.metaData.nTables_Valid;

                            quint32 nTemp=0;

                            for(nTemp=0; nValid; nTemp++)
                            {
                                nValid&=(nValid-1);
                            }

                            result.metaData.nTables_Valid_NumberOfRows=nTemp;

                            nOffset=result.metaData.nTablesHeaderOffset+24;

                            for(int i=0; i<64; i++)
                            {
                                if(result.metaData.nTables_Valid&((unsigned long long)1<<i))
                                {
                                    result.metaData.Tables_TablesNumberOfIndexes[i]=read_uint32(nOffset);
                                    nOffset+=4;
                                }
                            }

                            quint32 nSize=0;
                            int nStringIndexSize=2;
                            int nGUIDIndexSize=2;
                            int nBLOBIndexSize=2;
                            int nResolutionScope=2;
                            int nTypeDefOrRef=2;
                            int nField=2;
                            int nMethodDef=2;
                            int nParamList=2;

                            quint8 cHeapOffsetSizes=result.metaData.cTables_HeapOffsetSizes;

                            if(cHeapOffsetSizes&0x01)
                            {
                                nStringIndexSize=4;
                            }

                            if(cHeapOffsetSizes&0x02)
                            {
                                nGUIDIndexSize=4;
                            }

                            if(cHeapOffsetSizes&0x04)
                            {
                                nBLOBIndexSize=4;
                            }

                            // TODO !!!
                            if(result.metaData.Tables_TablesNumberOfIndexes[0]>0x3FFF)
                            {
                                nResolutionScope=4;
                            }

                            if(result.metaData.Tables_TablesNumberOfIndexes[26]>0x3FFF)
                            {
                                nResolutionScope=4;
                            }

                            if(result.metaData.Tables_TablesNumberOfIndexes[35]>0x3FFF)
                            {
                                nResolutionScope=4;
                            }

                            if(result.metaData.Tables_TablesNumberOfIndexes[1]>0x3FFF)
                            {
                                nResolutionScope=4;
                            }

                            if(result.metaData.Tables_TablesNumberOfIndexes[1]>0x3FFF)
                            {
                                nTypeDefOrRef=4;
                            }

                            if(result.metaData.Tables_TablesNumberOfIndexes[2]>0x3FFF)
                            {
                                nTypeDefOrRef=4;
                            }

                            if(result.metaData.Tables_TablesNumberOfIndexes[27]>0x3FFF)
                            {
                                nTypeDefOrRef=4;
                            }

                            if(result.metaData.Tables_TablesNumberOfIndexes[4]>0xFFFF)
                            {
                                nField=4;
                            }

                            if(result.metaData.Tables_TablesNumberOfIndexes[6]>0xFFFF)
                            {
                                nMethodDef=4;
                            }

                            if(result.metaData.Tables_TablesNumberOfIndexes[8]>0xFFFF)
                            {
                                nParamList=4;
                            }

                            nSize=0;
                            nSize+=2;
                            nSize+=nStringIndexSize;
                            nSize+=nGUIDIndexSize;
                            nSize+=nGUIDIndexSize;
                            nSize+=nGUIDIndexSize;
                            result.metaData.Tables_TablesSizes[0]=nSize;
                            nSize=0;
                            nSize+=nResolutionScope;
                            nSize+=nStringIndexSize;
                            nSize+=nStringIndexSize;
                            result.metaData.Tables_TablesSizes[1]=nSize;
                            nSize=0;
                            nSize+=4;
                            nSize+=nStringIndexSize;
                            nSize+=nStringIndexSize;
                            nSize+=nTypeDefOrRef;
                            nSize+=nField;
                            nSize+=nMethodDef;
                            result.metaData.Tables_TablesSizes[2]=nSize;
                            nSize=0;
                            result.metaData.Tables_TablesSizes[3]=nSize;
                            nSize=0;
                            nSize+=2;
                            nSize+=nStringIndexSize;
                            nSize+=nBLOBIndexSize;
                            result.metaData.Tables_TablesSizes[4]=nSize;
                            nSize=0;
                            result.metaData.Tables_TablesSizes[5]=nSize;
                            nSize=0;
                            nSize+=4;
                            nSize+=2;
                            nSize+=2;
                            nSize+=nStringIndexSize;
                            nSize+=nBLOBIndexSize;
                            nSize+=nParamList;
                            result.metaData.Tables_TablesSizes[6]=nSize;

                            for(int i=0; i<64; i++)
                            {
                                if(result.metaData.Tables_TablesNumberOfIndexes[i])
                                {
                                    result.metaData.Tables_TablesOffsets[i]=nOffset;
                                    nOffset+=result.metaData.Tables_TablesSizes[i]*result.metaData.Tables_TablesNumberOfIndexes[i];
                                }
                            }

                            if(!(result.header.Flags&XPE_DEF::COMIMAGE_FLAGS_NATIVE_ENTRYPOINT))
                            {
                                if(((result.metaData.nEntryPoint&0xFF000000)>>24)==6)
                                {
                                    unsigned int nIndex=result.metaData.nEntryPoint&0xFFFFFF;

                                    if(nIndex<=result.metaData.Tables_TablesNumberOfIndexes[6])
                                    {
                                        nOffset=result.metaData.Tables_TablesOffsets[6];
                                        nOffset+=result.metaData.Tables_TablesSizes[6]*(nIndex-1);

                                        result.metaData.nEntryPoint=read_uint32(nOffset);
                                    }
                                    else
                                    {
                                        result.metaData.nEntryPoint=0;
                                    }
                                }
                                else
                                {
                                    result.metaData.nEntryPoint=0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    //    emit appendError(".NET is not present");
    return result;
}

XBinary::OFFSETSIZE XPE::getNet_MetadataOffsetSize()
{
    OFFSETSIZE result={};
    result.nOffset=-1;

    _MEMORY_MAP memoryMap=getMemoryMap();

    qint64 nCLIHeaderOffset=-1;

    if(isNETPresent())
    {
        XPE_DEF::IMAGE_DATA_DIRECTORY _idd=getOptionalHeader_DataDirectory(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

        nCLIHeaderOffset=relAddressToOffset(&memoryMap,_idd.VirtualAddress);
    }
    else
    {
        // mb TODO
        // TODO Check!
        nCLIHeaderOffset=addressToOffset(&memoryMap,memoryMap.nBaseAddress+0x2008);
    }

    if(nCLIHeaderOffset!=-1)
    {
        XPE_DEF::IMAGE_COR20_HEADER header=_read_IMAGE_COR20_HEADER(nCLIHeaderOffset);

        if((header.cb==0x48)&&header.MetaData.VirtualAddress&&header.MetaData.Size)
        {
            result.nOffset=relAddressToOffset(&memoryMap,header.MetaData.VirtualAddress);
            result.nSize=header.MetaData.VirtualAddress;
        }
    }

    return result;
}

XPE::CLI_METADATA_HEADER XPE::_read_MetadataHeader(qint64 nOffset)
{
    CLI_METADATA_HEADER result={};

    result.nSignature=read_uint32(nOffset);
    result.nMajorVersion=read_uint16(nOffset+4);
    result.nMinorVersion=read_uint16(nOffset+6);
    result.nReserved=read_uint32(nOffset+8);
    result.nVersionStringLength=read_uint32(nOffset+12);
    result.sVersion=read_ansiString(nOffset+16,result.nVersionStringLength);
    result.nFlags=read_uint16(nOffset+16+result.nVersionStringLength);
    result.nStreams=read_uint16(nOffset+16+result.nVersionStringLength+2);

    return result;
}

void XPE::setMetadataHeader_Signature(quint32 nValue)
{
    qint64 nOffset=getNet_MetadataOffsetSize().nOffset;

    if(nOffset!=-1)
    {
        write_uint32(nOffset+0,nValue);
    }
}

void XPE::setMetadataHeader_MajorVersion(quint16 nValue)
{
    qint64 nOffset=getNet_MetadataOffsetSize().nOffset;

    if(nOffset!=-1)
    {
        write_uint16(nOffset+4,nValue);
    }
}

void XPE::setMetadataHeader_MinorVersion(quint16 nValue)
{
    qint64 nOffset=getNet_MetadataOffsetSize().nOffset;

    if(nOffset!=-1)
    {
        write_uint16(nOffset+6,nValue);
    }
}

void XPE::setMetadataHeader_Reserved(quint32 nValue)
{
    qint64 nOffset=getNet_MetadataOffsetSize().nOffset;

    if(nOffset!=-1)
    {
        write_uint32(nOffset+8,nValue);
    }
}

void XPE::setMetadataHeader_VersionStringLength(quint32 nValue)
{
    qint64 nOffset=getNet_MetadataOffsetSize().nOffset;

    if(nOffset!=-1)
    {
        write_uint32(nOffset+12,nValue);
    }
}

void XPE::setMetadataHeader_Version(QString sValue)
{
    qint64 nOffset=getNet_MetadataOffsetSize().nOffset;

    if(nOffset!=-1)
    {
        quint32 nVersionStringLength=read_uint32(nOffset+12);

        if(sValue.size()>(qint32)nVersionStringLength)
        {
            sValue.resize(nVersionStringLength);
        }

        write_ansiString(nOffset+16,sValue);
    }
}

void XPE::setMetadataHeader_Flags(quint16 nValue)
{
    qint64 nOffset=getNet_MetadataOffsetSize().nOffset;

    if(nOffset!=-1)
    {
        quint32 nVersionStringLength=read_uint32(nOffset+12);

        write_uint16(nOffset+16+nVersionStringLength,nValue);
    }
}

void XPE::setMetadataHeader_Streams(quint16 nValue)
{
    qint64 nOffset=getNet_MetadataOffsetSize().nOffset;

    if(nOffset!=-1)
    {
        quint32 nVersionStringLength=read_uint32(nOffset+12);

        write_uint16(nOffset+16+nVersionStringLength+2,nValue);
    }
}

bool XPE::isDataDirectoryValid(XPE_DEF::IMAGE_DATA_DIRECTORY *pDataDirectory)
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return isDataDirectoryValid(pDataDirectory,&memoryMap);
}

bool XPE::isDataDirectoryValid(XPE_DEF::IMAGE_DATA_DIRECTORY *pDataDirectory, XBinary::_MEMORY_MAP *pMemoryMap)
{
    bool bResult=false;

    bResult=isRelAddressValid(pMemoryMap,pDataDirectory->VirtualAddress);

    // TODO more checks

    return bResult;
}

bool XPE::isNetMetadataPresent()
{
    _MEMORY_MAP memoryMap=getMemoryMap();
    CLI_INFO cliInfo=getCliInfo(true,&memoryMap);

    return isNetMetadataPresent(&cliInfo,&memoryMap);
}

bool XPE::isNetMetadataPresent(XPE::CLI_INFO *pCliInfo, XBinary::_MEMORY_MAP *pMemoryMap)
{
    return isDataDirectoryValid(&(pCliInfo->header.MetaData),pMemoryMap);
}

bool XPE::isNETAnsiStringPresent(QString sString)
{
    CLI_INFO cliInfo=getCliInfo(true);

    return isNETAnsiStringPresent(sString,&cliInfo);
}

bool XPE::isNETAnsiStringPresent(QString sString, XPE::CLI_INFO *pCliInfo)
{
    return pCliInfo->metaData.listAnsiStrings.contains(sString);
}

bool XPE::isNETUnicodeStringPresent(QString sString)
{
    CLI_INFO cliInfo=getCliInfo(true);

    return isNETUnicodeStringPresent(sString,&cliInfo);
}

bool XPE::isNETUnicodeStringPresent(QString sString, XPE::CLI_INFO *pCliInfo)
{
    return pCliInfo->metaData.listUnicodeStrings.contains(sString);
}

int XPE::getEntryPointSection()
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getEntryPointSection(&memoryMap);
}

int XPE::getEntryPointSection(_MEMORY_MAP *pMemoryMap)
{
    int nResult=-1;

    qint64 nAddressOfEntryPoint=getOptionalHeader_AddressOfEntryPoint();

    if(nAddressOfEntryPoint)
    {
        nResult=addressToLoadSection(pMemoryMap,_getBaseAddress()+nAddressOfEntryPoint);
    }

    return nResult;
}

int XPE::getImportSection()
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getImportSection(&memoryMap);
}

int XPE::getImportSection(_MEMORY_MAP *pMemoryMap)
{
    int nResult=-1;

    qint64 nAddressOfImport=getOptionalHeader_DataDirectory(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_IMPORT).VirtualAddress;

    if(nAddressOfImport)
    {
        nResult=addressToLoadSection(pMemoryMap,_getBaseAddress()+nAddressOfImport);
    }

    return nResult;
}

int XPE::getExportSection()
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getExportSection(&memoryMap);
}

int XPE::getExportSection(_MEMORY_MAP *pMemoryMap)
{
    int nResult=-1;

    qint64 nAddressOfExport=getOptionalHeader_DataDirectory(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT).VirtualAddress;

    if(nAddressOfExport)
    {
        nResult=addressToLoadSection(pMemoryMap,_getBaseAddress()+nAddressOfExport);
    }

    return nResult;
}

int XPE::getTLSSection()
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getTLSSection(&memoryMap);
}

int XPE::getTLSSection(_MEMORY_MAP *pMemoryMap)
{
    int nResult=-1;

    qint64 nAddressOfTLS=getOptionalHeader_DataDirectory(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS).VirtualAddress;

    if(nAddressOfTLS)
    {
        nResult=addressToLoadSection(pMemoryMap,_getBaseAddress()+nAddressOfTLS);
    }

    return nResult;
}

int XPE::getResourcesSection()
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getResourcesSection(&memoryMap);
}

int XPE::getResourcesSection(_MEMORY_MAP *pMemoryMap)
{
    int nResult=-1;

    qint64 nAddressOfResources=getOptionalHeader_DataDirectory(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_RESOURCE).VirtualAddress;

    if(nAddressOfResources)
    {
        nResult=addressToLoadSection(pMemoryMap,_getBaseAddress()+nAddressOfResources);
    }

    return nResult;
}

int XPE::getRelocsSection()
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getRelocsSection(&memoryMap);
}

int XPE::getRelocsSection(_MEMORY_MAP *pMemoryMap)
{
    int nResult=-1;

    qint64 nAddressOfRelocs=getOptionalHeader_DataDirectory(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_BASERELOC).VirtualAddress;

    if(nAddressOfRelocs)
    {
        nResult=addressToLoadSection(pMemoryMap,_getBaseAddress()+nAddressOfRelocs);
    }

    return nResult;
}

int XPE::getNormalCodeSection()
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getNormalCodeSection(&memoryMap);
}

int XPE::getNormalCodeSection(_MEMORY_MAP *pMemoryMap)
{
    int nResult=-1;
    // TODO opimize

    QList<XPE_DEF::IMAGE_SECTION_HEADER> listSections=getSectionHeaders();
    int nNumberOfSections=listSections.count();
    nNumberOfSections=qMin(nNumberOfSections,2);

    for(int i=0; i<nNumberOfSections; i++)
    {
        QString sSectionName=QString((char *)listSections.at(i).Name);
        sSectionName.resize(qMin(sSectionName.length(),XPE_DEF::S_IMAGE_SIZEOF_SHORT_NAME));
        quint32 nSectionCharacteristics=listSections.at(i).Characteristics;
        nSectionCharacteristics&=0xFF0000FF;

        // .textbss
        // 0x60500060 mingw
        if((((sSectionName=="CODE")||sSectionName==".text"))&&
                (nSectionCharacteristics==0x60000020)&&
                (listSections.at(i).SizeOfRawData))
        {
            nResult=addressToLoadSection(pMemoryMap,_getBaseAddress()+listSections.at(i).VirtualAddress);
            break;
        }
    }

    if(nResult==-1)
    {
        if(nNumberOfSections>0)
        {
            if(listSections.at(0).SizeOfRawData)
            {
                nResult=addressToLoadSection(pMemoryMap,_getBaseAddress()+listSections.at(0).VirtualAddress);
            }
        }
    }

    return nResult;
}

int XPE::getNormalDataSection()
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getNormalDataSection(&memoryMap);
}

int XPE::getNormalDataSection(_MEMORY_MAP *pMemoryMap)
{
    int nResult=-1;
    // TODO opimize

    QList<XPE_DEF::IMAGE_SECTION_HEADER> listSections=getSectionHeaders();
    int nNumberOfSections=listSections.count();

    int nImportSection=getImportSection(pMemoryMap);

    for(int i=1; i<nNumberOfSections; i++)
    {
        // 0xc0700040 MinGW
        // 0xc0600040 MinGW
        // 0xc0300040 MinGW
        QString sSectionName=QString((char *)listSections.at(i).Name);
        sSectionName.resize(qMin(sSectionName.length(),XPE_DEF::S_IMAGE_SIZEOF_SHORT_NAME));
        quint32 nSectionCharacteristics=listSections.at(i).Characteristics;
        nSectionCharacteristics&=0xFF0000FF;

        if( (((sSectionName=="DATA")||sSectionName==".data"))&&
            (nSectionCharacteristics==0xC0000040)&&
            (listSections.at(i).SizeOfRawData)&&
            (nImportSection!=i))
        {
            nResult=addressToLoadSection(pMemoryMap,_getBaseAddress()+listSections.at(i).VirtualAddress);
            break;
        }
    }

    if(nResult==-1)
    {
        for(int i=1; i<nNumberOfSections; i++)
        {
            if( listSections.at(i).SizeOfRawData&&(nImportSection!=i)&&
                (listSections.at(i).Characteristics!=0x60000020)&&
                (listSections.at(i).Characteristics!=0x40000040))
            {
                nResult=addressToLoadSection(pMemoryMap,_getBaseAddress()+listSections.at(i).VirtualAddress);
                break;
            }
        }
    }

    return nResult;
}

int XPE::getConstDataSection()
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getConstDataSection(&memoryMap);
}

int XPE::getConstDataSection(_MEMORY_MAP *pMemoryMap)
{
    int nResult=-1;
    // TODO opimize

    QList<XPE_DEF::IMAGE_SECTION_HEADER> listSections=getSectionHeaders();
    int nNumberOfSections=listSections.count();

    for(int i=1; i<nNumberOfSections; i++)
    {
        // 0x40700040 MinGW
        // 0x40600040 MinGW
        // 0x40300040 MinGW
        QString sSectionName=QString((char *)listSections.at(i).Name);
        sSectionName.resize(qMin(sSectionName.length(),XPE_DEF::S_IMAGE_SIZEOF_SHORT_NAME));
        quint32 nSectionCharacteristics=listSections.at(i).Characteristics;
        nSectionCharacteristics&=0xFF0000FF;

        if((sSectionName==".rdata")&&
                (nSectionCharacteristics==0x40000040)&&
                (listSections.at(i).SizeOfRawData))
        {
            nResult=addressToLoadSection(pMemoryMap,_getBaseAddress()+listSections.at(i).VirtualAddress);
            break;
        }
    }

    return nResult;
}

bool XPE::rebuildDump(QString sResultFile,REBUILD_OPTIONS *pRebuildOptions)
{
    // TODO rework!
#ifdef QT_DEBUG
    QElapsedTimer timer;
    timer.start();
    qDebug("XPE::rebuildDump");
#endif
    bool bResult=false;

    if(sResultFile!="")
    {
        quint32 nTotalSize=0;
        quint32 nHeaderSize=0;
        QList<quint32> listSectionsSize;
        QList<quint32> listSectionsOffsets;

        quint32 nFileAlignment=getOptionalHeader_FileAlignment();

        quint32 nSectionAlignment=getOptionalHeader_SectionAlignment();

        if(pRebuildOptions->bOptimize)
        {
            QByteArray baHeader=getHeaders();
            int nNumberOfSections=getFileHeader_NumberOfSections();

//            if(pRebuildOptions->bClearHeader)
//            {
//                nHeaderSize=(qint32)getSectionsTableOffset()+nNumberOfSections*sizeof(XPE_DEF::IMAGE_SECTION_HEADER);
//            }
//            else
//            {
//                nHeaderSize=(quint32)XBinary::getPhysSize(baHeader.data(),baHeader.size());
//            }

            nHeaderSize=(quint32)XBinary::getPhysSize(baHeader.data(),baHeader.size());

            for(int i=0; i<nNumberOfSections; i++)
            {
                QByteArray baSection=read_array(getSection_VirtualAddress(i),getSection_VirtualSize(i));
                quint32 nSectionSize=(quint32)XBinary::getPhysSize(baSection.data(),baSection.size());
                listSectionsSize.append(nSectionSize);
            }

            nTotalSize+=S_ALIGN_UP(nHeaderSize,nFileAlignment);

            for(int i=0; i<listSectionsSize.size(); i++)
            {
                listSectionsOffsets.append(nTotalSize);

                if(listSectionsSize.at(i))
                {
                    nTotalSize+=S_ALIGN_UP(listSectionsSize.at(i),nFileAlignment);
                }
            }
        }
        else
        {
            nTotalSize=getSize();
        }
#ifdef QT_DEBUG
        qDebug("XPE::rebuildDump:totalsize: %lld msec",timer.elapsed());
#endif
        QByteArray baBuffer;
        baBuffer.resize(nTotalSize);
        baBuffer.fill(0);
        QBuffer buffer;
        buffer.setBuffer(&baBuffer);

        if(buffer.open(QIODevice::ReadWrite))
        {
            XPE bufPE(&buffer,false);

            if(pRebuildOptions->bOptimize)
            {
                XBinary::copyDeviceMemory(getDevice(),0,&buffer,0,nHeaderSize);
                bufPE.setOptionalHeader_SizeOfHeaders(S_ALIGN_UP(nHeaderSize,nFileAlignment));
            }
            else
            {
                XBinary::copyDeviceMemory(getDevice(),0,&buffer,0,nTotalSize);
            }
#ifdef QT_DEBUG
            qDebug("XPE::rebuildDump:copy: %lld msec",timer.elapsed());
#endif
            int nNumberOfSections=getFileHeader_NumberOfSections();

            for(int i=0; i<nNumberOfSections; i++)
            {
                if(pRebuildOptions->bOptimize)
                {
                    XBinary::copyDeviceMemory(getDevice(),getSection_VirtualAddress(i),&buffer,listSectionsOffsets.at(i),listSectionsSize.at(i));
                    bufPE.setSection_PointerToRawData(i,listSectionsOffsets.at(i));
                    bufPE.setSection_SizeOfRawData(i,S_ALIGN_UP(listSectionsSize.at(i),nFileAlignment));
                }
                else
                {
                    quint32 nSectionAddress=getSection_VirtualAddress(i);
                    quint32 nSectionSize=getSection_VirtualSize(i);
                    bufPE.setSection_SizeOfRawData(i,S_ALIGN_UP(nSectionSize,nSectionAlignment));
                    bufPE.setSection_PointerToRawData(i,nSectionAddress);
                }

                bufPE.setSection_Characteristics(i,0xe0000020); // !!!
            }
#ifdef QT_DEBUG
            qDebug("XPE::rebuildDump:copysections: %lld msec",timer.elapsed());
#endif
            bResult=true;

            buffer.close();
        }

        QFile file;
        file.setFileName(sResultFile);

        if(file.open(QIODevice::ReadWrite))
        {
#ifdef QT_DEBUG
            qDebug("XPE::rebuildDump:write:start: %lld msec",timer.elapsed());
#endif
            file.resize(baBuffer.size());
            file.write(baBuffer.data(),baBuffer.size());
            file.close();
#ifdef QT_DEBUG
            qDebug("XPE::rebuildDump:write: %lld msec",timer.elapsed());
#endif
            bResult=true;
        }
    }

    if(bResult)
    {
        bResult=false;

        QFile file;
        file.setFileName(sResultFile);

        if(file.open(QIODevice::ReadWrite))
        {
            XPE _pe(&file,false);

            if(_pe.isValid())
            {
                //                if(pRebuildOptions->bRemoveLastSection)
                //                {
                //                    _pe.removeLastSection();
                //                }
                //            #ifdef QT_DEBUG
                //                qDebug("XPE::rebuildDump:removelastsection: %lld msec",timer.elapsed());
                //            #endif
                if(!pRebuildOptions->mapPatches.empty())
                {
                    _MEMORY_MAP memoryMap=getMemoryMap();

                    QMapIterator<qint64, quint64> i(pRebuildOptions->mapPatches);

                    while(i.hasNext())
                    {
                        i.next();

                        qint64 nAddress=i.key();
                        quint64 nValue=i.value();

                        quint64 nOffset=_pe.addressToOffset(&memoryMap,nAddress);

                        if(_pe.is64())
                        {
                            _pe.write_uint64(nOffset,nValue);
                        }
                        else
                        {
                            _pe.write_uint32(nOffset,(quint32)nValue);
                        }
                    }
                }

#ifdef QT_DEBUG
                qDebug("XPE::rebuildDump:mapPatches: %lld msec",timer.elapsed());
#endif
                if(pRebuildOptions->bSetEntryPoint)
                {
                    _pe.setOptionalHeader_AddressOfEntryPoint(pRebuildOptions->nEntryPoint);
                }

#ifdef QT_DEBUG
                qDebug("XPE::rebuildDump:setentrypoint: %lld msec",timer.elapsed());
#endif
                if(!pRebuildOptions->mapIAT.isEmpty())
                {
                    if(!_pe.addImportSection(&(pRebuildOptions->mapIAT)))
                    {
//                        _errorMessage(tr("Cannot add import section"));
                    }
                }

#ifdef QT_DEBUG
                qDebug("XPE::rebuildDump:addimportsection: %lld msec",timer.elapsed());
#endif
                if(pRebuildOptions->bRenameSections)
                {
                    int nNumberOfSections=_pe.getFileHeader_NumberOfSections();

                    for(int i=0; i<nNumberOfSections; i++)
                    {
                        QString sSection=_pe.getSection_NameAsString(i);

                        if(sSection!=".rsrc")
                        {
                            _pe.setSection_NameAsString(i,pRebuildOptions->sSectionName);
                        }
                    }
                }

#ifdef QT_DEBUG
                qDebug("XPE::rebuildDump:renamesections: %lld msec",timer.elapsed());
#endif
                if(pRebuildOptions->listRelocsRVAs.count())
                {
                    _pe.addRelocsSection(&(pRebuildOptions->listRelocsRVAs));
                }
#ifdef QT_DEBUG
                qDebug("XPE::rebuildDump:addrelocssection: %lld msec",timer.elapsed());
#endif
                if(pRebuildOptions->bFixChecksum)
                {
                    _pe.fixCheckSum();
                }
#ifdef QT_DEBUG
                qDebug("XPE::rebuildDump:fixchecksum: %lld msec",timer.elapsed());
#endif
            }

            bResult=true;

            file.close();
        }
    }
#ifdef QT_DEBUG
    qDebug("XPE::rebuildDump: %lld msec",timer.elapsed());
#endif

    return bResult;
}

bool XPE::rebuildDump(QString sInputFile, QString sResultFile,REBUILD_OPTIONS *pRebuildOptions)
{
    // TODO rework!
    bool bResult=false;

    QFile file;
    file.setFileName(sInputFile);

    if(file.open(QIODevice::ReadOnly))
    {
        XPE pe(&file,false);

        if(pe.isValid())
        {
            bResult=pe.rebuildDump(sResultFile,pRebuildOptions);
        }

        file.close();
    }

    return bResult;
}

bool XPE::fixCheckSum(QString sFileName, bool bIsImage)
{
    bool bResult=false;

    QFile file;
    file.setFileName(sFileName);

    if(file.open(QIODevice::ReadWrite))
    {
        XPE pe(&file,bIsImage);

        if(pe.isValid())
        {
            pe.fixCheckSum();
            bResult=true;
        }

        file.close();
    }

    return bResult;
}

qint64 XPE::_fixHeadersSize()
{
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();
    qint64 nSectionsTableOffset=getSectionsTableOffset();
    qint64 nHeadersSize=_calculateHeadersSize(nSectionsTableOffset,nNumberOfSections);

    // MB TODO
    setOptionalHeader_SizeOfHeaders(nHeadersSize);

    return nHeadersSize;
}

qint64 XPE::_getMinSectionOffset()
{
    qint64 nResult=-1;

    _MEMORY_MAP memoryMap=getMemoryMap();

    int nNumberOfRecords=memoryMap.listRecords.count();

    for(int i=0; i<nNumberOfRecords; i++)
    {
        if(memoryMap.listRecords.at(i).type==MMT_LOADSECTION)
        {
            if(nResult==-1)
            {
                nResult=memoryMap.listRecords.at(i).nOffset;
            }
            else
            {
                nResult=qMin(nResult,memoryMap.listRecords.at(i).nOffset);
            }
        }
    }

    return nResult;
}

void XPE::_fixFileOffsets(qint64 nDelta)
{
    if(nDelta)
    {
        setOptionalHeader_SizeOfHeaders(getOptionalHeader_SizeOfHeaders()+nDelta);
        quint32 nNumberOfSections=getFileHeader_NumberOfSections();

        for(quint32 i=0; i<nNumberOfSections; i++)
        {
            quint32 nFileOffset=getSection_PointerToRawData(i);
            setSection_PointerToRawData(i,nFileOffset+nDelta);
        }
    }
}

quint16 XPE::_checkSum(qint64 nStartValue,qint64 nDataSize)
{
    // TODO Check
    // TODO Optimize
    const int BUFFER_SIZE=0x1000;
    int nSum=(int)nStartValue;
    unsigned int nTemp=0;
    char *pBuffer=new char[BUFFER_SIZE];
    char *pOffset;

    while(nDataSize>0)
    {
        nTemp=qMin((qint64)BUFFER_SIZE,nDataSize);

        if(!read_array(nStartValue,pBuffer,nTemp))
        {
            delete[] pBuffer;

            return 0;
        }

        pOffset=pBuffer;

        for(unsigned int i=0; i<(nTemp+1)/2; i++)
        {
            nSum+=*((unsigned short *)pOffset);

            if(S_HIWORD(nSum)!=0)
            {
                nSum=S_LOWORD(nSum)+S_HIWORD(nSum);
            }

            pOffset+=2;
        }

        nDataSize-=nTemp;
        nStartValue+=nTemp;
    }

    delete [] pBuffer;

    return (unsigned short)(S_LOWORD(nSum)+S_HIWORD(nSum));
}

XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY XPE::read_IMAGE_RESOURCE_DIRECTORY_ENTRY(qint64 nOffset)
{
    XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY result={};

    read_array(nOffset,(char *)&result,sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY));

    return result;
}

XPE_DEF::IMAGE_RESOURCE_DIRECTORY XPE::read_IMAGE_RESOURCE_DIRECTORY(qint64 nOffset)
{
    XPE_DEF::IMAGE_RESOURCE_DIRECTORY result={};

    read_array(nOffset,(char *)&result,sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY));

    return result;
}

XPE_DEF::IMAGE_RESOURCE_DATA_ENTRY XPE::read_IMAGE_RESOURCE_DATA_ENTRY(qint64 nOffset)
{
    XPE_DEF::IMAGE_RESOURCE_DATA_ENTRY result={};

    read_array(nOffset,(char *)&result,sizeof(XPE_DEF::IMAGE_RESOURCE_DATA_ENTRY));

    return result;
}

XPE::RESOURCES_ID_NAME XPE::getResourcesIDName(qint64 nResourceOffset, quint32 nValue)
{
    RESOURCES_ID_NAME result={};

    if(nValue&0x80000000)
    {
        result.bIsName=true;
        nValue&=0x7FFFFFFF;
        result.nNameOffset=nValue;
        result.nID=0;
        quint16 nStringLength=read_uint16(nResourceOffset+nValue);

        nStringLength=qMin((quint16)1024,nStringLength);

        QByteArray baName=read_array(nResourceOffset+nValue+2,nStringLength*2);
        result.sName=QString::fromUtf16((quint16 *)(baName.data()),nStringLength);
    }
    else
    {
        result.nID=nValue;
        result.sName="";
        result.nNameOffset=0;
    }

    return result;
}

QList<qint64> XPE::getRelocsAsRVAList()
{
    QSet<qint64> stResult;

    // TODO 64
    qint64 nRelocsOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_BASERELOC);

    if(nRelocsOffset!=-1)
    {
        while(true)
        {
            XPE_DEF::IMAGE_BASE_RELOCATION ibr=_readIMAGE_BASE_RELOCATION(nRelocsOffset);

            if((ibr.VirtualAddress==0)||(ibr.SizeOfBlock==0))
            {
                break;
            }

            if(ibr.VirtualAddress&0xFFF)
            {
                break;
            }

            nRelocsOffset+=sizeof(XPE_DEF::IMAGE_BASE_RELOCATION);

            int nNumberOfBlocks=(ibr.SizeOfBlock-sizeof(XPE_DEF::IMAGE_BASE_RELOCATION))/sizeof(quint16);

            nNumberOfBlocks=qMin(nNumberOfBlocks,(int)0xFFFF);

            for(int i=0; i<nNumberOfBlocks; i++)
            {
                quint16 nRecord=read_uint16(nRelocsOffset);

                if(nRecord)
                {
                    nRecord=nRecord&0x0FFF;
                    stResult.insert(ibr.VirtualAddress+nRecord);
                }

                nRelocsOffset+=sizeof(quint16);
            }
        }
    }

    return stResult.values();
}

QList<XPE::RELOCS_HEADER> XPE::getRelocsHeaders()
{
    QList<RELOCS_HEADER> listResult;

    qint64 nRelocsOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_BASERELOC);

    if(nRelocsOffset!=-1)
    {
        while(true)
        {
            RELOCS_HEADER record={0};

            record.nOffset=nRelocsOffset;

            record.baseRelocation=_readIMAGE_BASE_RELOCATION(nRelocsOffset);

            if((record.baseRelocation.VirtualAddress==0)||(record.baseRelocation.SizeOfBlock==0))
            {
                break;
            }

            if(record.baseRelocation.VirtualAddress&0xFFF)
            {
                break;
            }

            nRelocsOffset+=sizeof(XPE_DEF::IMAGE_BASE_RELOCATION);

            record.nCount=(record.baseRelocation.SizeOfBlock-sizeof(XPE_DEF::IMAGE_BASE_RELOCATION))/sizeof(quint16);

            nRelocsOffset+=sizeof(quint16)*record.nCount;

            listResult.append(record);
        }
    }

    return listResult;
}

QList<XPE::RELOCS_POSITION> XPE::getRelocsPositions(qint64 nOffset)
{
    QList<RELOCS_POSITION> listResult;

    XPE_DEF::IMAGE_BASE_RELOCATION ibr=_readIMAGE_BASE_RELOCATION(nOffset);

    if((ibr.VirtualAddress)&&(ibr.SizeOfBlock))
    {
        nOffset+=sizeof(XPE_DEF::IMAGE_BASE_RELOCATION);

        int nCount=(ibr.SizeOfBlock-sizeof(XPE_DEF::IMAGE_BASE_RELOCATION))/sizeof(quint16);

        nCount&=0xFFFF;

        for(int i=0; i<nCount; i++)
        {
            RELOCS_POSITION record={0};

            quint16 nRecord=read_uint16(nOffset);

            record.nTypeOffset=nRecord;
            record.nAddress=ibr.VirtualAddress+(nRecord&0x0FFF);
            record.nType=nRecord>>12;

            listResult.append(record);

            nOffset+=sizeof(quint16);
        }
    }

    return listResult;
}

XPE_DEF::IMAGE_BASE_RELOCATION XPE::_readIMAGE_BASE_RELOCATION(qint64 nOffset)
{
    XPE_DEF::IMAGE_BASE_RELOCATION result={};

    result.VirtualAddress=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_BASE_RELOCATION,VirtualAddress));
    result.SizeOfBlock=read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_BASE_RELOCATION,SizeOfBlock));

    return result;
}

quint32 XPE::getRelocsVirtualAddress(qint64 nOffset)
{
    return read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_BASE_RELOCATION,VirtualAddress));
}

quint32 XPE::getRelocsSizeOfBlock(qint64 nOffset)
{
    return read_uint32(nOffset+offsetof(XPE_DEF::IMAGE_BASE_RELOCATION,SizeOfBlock));
}

void XPE::setRelocsVirtualAddress(qint64 nOffset, quint32 nValue)
{
    write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_BASE_RELOCATION,VirtualAddress),nValue);
}

void XPE::setRelocsSizeOfBlock(qint64 nOffset, quint32 nValue)
{
    write_uint32(nOffset+offsetof(XPE_DEF::IMAGE_BASE_RELOCATION,SizeOfBlock),nValue);
}

bool XPE::addRelocsSection(QList<qint64> *pList)
{
    return addRelocsSection(getDevice(),isImage(),pList);
}

bool XPE::addRelocsSection(QIODevice *pDevice, bool bIsImage, QList<qint64> *pListRelocs)
{
    bool bResult=false;

    if((isResizeEnable(pDevice))&&(pListRelocs->count()))
    {
        XPE pe(pDevice,bIsImage);

        if(pe.isValid())
        {
            // Check valid
            _MEMORY_MAP memoryMap=pe.getMemoryMap();

            QList<qint64> listRVAs;

            int nNumberOfRelocs=pListRelocs->count();

            for(int i=0; i<nNumberOfRelocs; i++)
            {
                if(pe.isAddressValid(&memoryMap,pListRelocs->at(i)+memoryMap.nBaseAddress))
                {
                    listRVAs.append(pListRelocs->at(i));
                }
            }

            QByteArray baRelocs=relocsAsRVAListToByteArray(&listRVAs,pe.is64());

            XPE_DEF::IMAGE_SECTION_HEADER ish={};

            ish.Characteristics=0x42000040;
            QString sSectionName=".reloc";
            XBinary::_copyMemory((char *)&ish.Name,sSectionName.toLatin1().data(),qMin(XPE_DEF::S_IMAGE_SIZEOF_SHORT_NAME,sSectionName.length()));

            bResult=addSection(pDevice,bIsImage,&ish,baRelocs.data(),baRelocs.size());

            if(bResult)
            {
                XPE_DEF::IMAGE_DATA_DIRECTORY dd={};

                dd.VirtualAddress=ish.VirtualAddress;
                dd.Size=ish.Misc.VirtualSize;

                pe.setOptionalHeader_DataDirectory(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_BASERELOC,&dd);

                bResult=true;
            }
        }
    }

    return bResult;
}

bool XPE::addRelocsSection(QString sFileName, bool bIsImage, QList<qint64> *pListRelocs)
{
    bool bResult=false;

    QFile file(sFileName);

    if(file.open(QIODevice::ReadWrite))
    {
        bResult=addRelocsSection(&file,bIsImage,pListRelocs);

        file.close();
    }

    return bResult;
}

QByteArray XPE::relocsAsRVAListToByteArray(QList<qint64> *pListRelocs, bool bIs64)
{
    QByteArray baResult;
    // GetHeaders
    // pList must be sorted!

    qint32 nBaseAddress=-1;
    quint32 nSize=0;

    int nNumberOfRelocs=pListRelocs->count();

    for(int i=0; i<nNumberOfRelocs; i++)
    {
        qint32 _nBaseAddress=S_ALIGN_DOWN(pListRelocs->at(i),0x1000);

        if(nBaseAddress!=_nBaseAddress)
        {
            nBaseAddress=_nBaseAddress;
            nSize=S_ALIGN_UP(nSize,4);
            nSize+=sizeof(XPE_DEF::IMAGE_BASE_RELOCATION);
        }

        nSize+=2;
    }

    nSize=S_ALIGN_UP(nSize,4);

    baResult.resize(nSize);

    nBaseAddress=-1;
    quint32 nOffset=0;
    char *pData=baResult.data();
    char *pVirtualAddress=0;
    char *pSizeOfBlock=0;
    quint32 nCurrentBlockSize=0;

    nNumberOfRelocs=pListRelocs->count();

    for(int i=0; i<nNumberOfRelocs; i++)
    {
        qint32 _nBaseAddress=S_ALIGN_DOWN(pListRelocs->at(i),0x1000);

        if(nBaseAddress!=_nBaseAddress)
        {
            nBaseAddress=_nBaseAddress;
            quint32 _nOffset=S_ALIGN_UP(nOffset,4);

            if(nOffset!=_nOffset)
            {
                nCurrentBlockSize+=2;
                XBinary::_write_uint32(pSizeOfBlock,nCurrentBlockSize);
                XBinary::_write_uint16(pData+nOffset,0);
                nOffset=_nOffset;
            }

            pVirtualAddress=pData+nOffset+offsetof(XPE_DEF::IMAGE_BASE_RELOCATION,VirtualAddress);
            pSizeOfBlock=pData+nOffset+offsetof(XPE_DEF::IMAGE_BASE_RELOCATION,SizeOfBlock);
            XBinary::_write_uint32(pVirtualAddress,nBaseAddress);
            nCurrentBlockSize=sizeof(XPE_DEF::IMAGE_BASE_RELOCATION);
            XBinary::_write_uint32(pSizeOfBlock,nCurrentBlockSize);

            nOffset+=sizeof(XPE_DEF::IMAGE_BASE_RELOCATION);
        }

        nCurrentBlockSize+=2;
        XBinary::_write_uint32(pSizeOfBlock,nCurrentBlockSize);

        if(!bIs64)
        {
            XBinary::_write_uint16(pData+nOffset,pListRelocs->at(i)-nBaseAddress+0x3000);
        }
        else
        {
            XBinary::_write_uint16(pData+nOffset,pListRelocs->at(i)-nBaseAddress+0xA000);
        }

        nOffset+=2;
    }

    quint32 _nOffset=S_ALIGN_UP(nOffset,4);

    if(nOffset!=_nOffset)
    {
        nCurrentBlockSize+=2;
        XBinary::_write_uint32(pSizeOfBlock,nCurrentBlockSize);
        XBinary::_write_uint16(pData+nOffset,0);
    }

    return baResult;
}

bool XPE::isResourcesPresent()
{
    return isOptionalHeader_DataDirectoryPresent(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_RESOURCE);
}

bool XPE::isRelocsPresent()
{
    return isOptionalHeader_DataDirectoryPresent(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_BASERELOC);
}

bool XPE::isDebugPresent()
{
    return isOptionalHeader_DataDirectoryPresent(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_DEBUG);
}

bool XPE::isTLSPresent()
{
    return isOptionalHeader_DataDirectoryPresent(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);
}

bool XPE::isSignPresent()
{
    return isOptionalHeader_DataDirectoryPresent(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_SECURITY);
}

bool XPE::isExceptionPresent()
{
    return isOptionalHeader_DataDirectoryPresent(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXCEPTION);
}

bool XPE::isLoadConfigPresent()
{
    return isOptionalHeader_DataDirectoryPresent(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);
}

bool XPE::isBoundImportPresent()
{
    return isOptionalHeader_DataDirectoryPresent(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT);
}

bool XPE::isDelayImportPresent()
{
    return isOptionalHeader_DataDirectoryPresent(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
}

qint64 XPE::getTLSHeaderOffset()
{
    return getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);
}

qint64 XPE::getTLSHeaderSize()
{
    qint64 nResult=0;

    if(is64())
    {
        nResult=sizeof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64);
    }
    else
    {
        nResult=sizeof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32);
    }

    return nResult;
}

XPE_DEF::S_IMAGE_TLS_DIRECTORY32 XPE::getTLSDirectory32()
{
    XPE_DEF::S_IMAGE_TLS_DIRECTORY32 result={};

    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        // TODO read function!!!
        read_array(nTLSOffset,(char *)&result,sizeof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32));
    }

    return result;
}

XPE_DEF::S_IMAGE_TLS_DIRECTORY64 XPE::getTLSDirectory64()
{
    XPE_DEF::S_IMAGE_TLS_DIRECTORY64 result={};

    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        // TODO read function!!!
        read_array(nTLSOffset,(char *)&result,sizeof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64));
    }

    return result;
}

quint64 XPE::getTLS_StartAddressOfRawData()
{
    quint64 nResult=0;

    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,StartAddressOfRawData));
        }
        else
        {
            nResult=read_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,StartAddressOfRawData));
        }
    }

    return nResult;
}

quint64 XPE::getTLS_EndAddressOfRawData()
{
    quint64 nResult=0;

    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,EndAddressOfRawData));
        }
        else
        {
            nResult=read_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,EndAddressOfRawData));
        }
    }

    return nResult;
}

quint64 XPE::getTLS_AddressOfIndex()
{
    quint64 nResult=0;

    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,AddressOfIndex));
        }
        else
        {
            nResult=read_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,AddressOfIndex));
        }
    }

    return nResult;
}

quint64 XPE::getTLS_AddressOfCallBacks()
{
    quint64 nResult=0;

    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,AddressOfCallBacks));
        }
        else
        {
            nResult=read_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,AddressOfCallBacks));
        }
    }

    return nResult;
}

quint32 XPE::getTLS_SizeOfZeroFill()
{
    quint32 nResult=0;

    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,SizeOfZeroFill));
        }
        else
        {
            nResult=read_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,SizeOfZeroFill));
        }
    }

    return nResult;
}

quint32 XPE::getTLS_Characteristics()
{
    quint32 nResult=0;

    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,Characteristics));
        }
        else
        {
            nResult=read_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,Characteristics));
        }
    }

    return nResult;
}

void XPE::setTLS_StartAddressOfRawData(quint64 nValue)
{
    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,StartAddressOfRawData),nValue);
        }
        else
        {
            write_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,StartAddressOfRawData),nValue);
        }
    }
}

void XPE::setTLS_EndAddressOfRawData(quint64 nValue)
{
    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,EndAddressOfRawData),nValue);
        }
        else
        {
            write_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,EndAddressOfRawData),nValue);
        }
    }
}

void XPE::setTLS_AddressOfIndex(quint64 nValue)
{
    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,AddressOfIndex),nValue);
        }
        else
        {
            write_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,AddressOfIndex),nValue);
        }
    }
}

void XPE::setTLS_AddressOfCallBacks(quint64 nValue)
{
    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,AddressOfCallBacks),nValue);
        }
        else
        {
            write_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,AddressOfCallBacks),nValue);
        }
    }
}

void XPE::setTLS_SizeOfZeroFill(quint32 nValue)
{
    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,SizeOfZeroFill),nValue);
        }
        else
        {
            write_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,SizeOfZeroFill),nValue);
        }
    }
}

void XPE::setTLS_Characteristics(quint32 nValue)
{
    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,Characteristics),nValue);
        }
        else
        {
            write_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,Characteristics),nValue);
        }
    }
}

QList<qint64> XPE::getTLS_CallbacksList()
{
    _MEMORY_MAP memoryMap=getMemoryMap();

    return getTLS_CallbacksList(&memoryMap);
}

QList<qint64> XPE::getTLS_CallbacksList(XBinary::_MEMORY_MAP *pMemoryMap)
{
    QList<qint64> listResult;

    qint64 nOffset=addressToOffset(pMemoryMap,(qint64)getTLS_AddressOfCallBacks());

    if(nOffset!=-1)
    {
        for(int i=0;i<100;i++) // TODO const
        {
            qint64 nAddress=0;

            if(is64())
            {
                nAddress=read_uint64(nOffset);

                nOffset+=8;
            }
            else
            {
                nAddress=read_uint32(nOffset);

                nOffset+=4;
            }

            if(nAddress&&isAddressValid(pMemoryMap,nAddress))
            {
                listResult.append(nAddress);
            }
            else
            {
                break;
            }
        }
    }

    return listResult;
}

XPE::TLS_HEADER XPE::getTLSHeader()
{
    TLS_HEADER result={};

    if(isTLSPresent())
    {
        if(is64())
        {
            XPE_DEF::S_IMAGE_TLS_DIRECTORY64 tls64=getTLSDirectory64();

            result.AddressOfCallBacks=tls64.AddressOfCallBacks;
            result.AddressOfIndex=tls64.AddressOfIndex;
            result.Characteristics=tls64.Characteristics;
            result.EndAddressOfRawData=tls64.EndAddressOfRawData;
            result.SizeOfZeroFill=tls64.SizeOfZeroFill;
            result.StartAddressOfRawData=tls64.StartAddressOfRawData;
        }
        else
        {
            XPE_DEF::S_IMAGE_TLS_DIRECTORY32 tls32=getTLSDirectory32();

            result.AddressOfCallBacks=tls32.AddressOfCallBacks;
            result.AddressOfIndex=tls32.AddressOfIndex;
            result.Characteristics=tls32.Characteristics;
            result.EndAddressOfRawData=tls32.EndAddressOfRawData;
            result.SizeOfZeroFill=tls32.SizeOfZeroFill;
            result.StartAddressOfRawData=tls32.StartAddressOfRawData;
        }
    }

    return result;
}

QMap<quint64, QString> XPE::getImageNtHeadersSignatures()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0x00004550,"IMAGE_NT_SIGNATURE");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageNtHeadersSignaturesS()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0x00004550,"NT_SIGNATURE");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageMagics()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0x5A4D,"IMAGE_DOS_SIGNATURE");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageMagicsS()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0x5A4D,"DOS_SIGNATURE");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageFileHeaderMachines()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0,"IMAGE_FILE_MACHINE_UNKNOWN");
    mapResult.insert(0x014c,"IMAGE_FILE_MACHINE_I386");
    mapResult.insert(0x0162,"IMAGE_FILE_MACHINE_R3000");
    mapResult.insert(0x0166,"IMAGE_FILE_MACHINE_R4000");
    mapResult.insert(0x0168,"IMAGE_FILE_MACHINE_R10000");
    mapResult.insert(0x0169,"IMAGE_FILE_MACHINE_WCEMIPSV2");
    mapResult.insert(0x0184,"IMAGE_FILE_MACHINE_ALPHA");
    mapResult.insert(0x01a2,"IMAGE_FILE_MACHINE_SH3");
    mapResult.insert(0x01a3,"IMAGE_FILE_MACHINE_SH3DSP");
    mapResult.insert(0x01a4,"IMAGE_FILE_MACHINE_SH3E");
    mapResult.insert(0x01a6,"IMAGE_FILE_MACHINE_SH4");
    mapResult.insert(0x01a8,"IMAGE_FILE_MACHINE_SH5");
    mapResult.insert(0x01c0,"IMAGE_FILE_MACHINE_ARM");
    mapResult.insert(0x01c2,"IMAGE_FILE_MACHINE_THUMB");
    mapResult.insert(0x01c4,"IMAGE_FILE_MACHINE_ARMNT");
    mapResult.insert(0x01d3,"IMAGE_FILE_MACHINE_AM33");
    mapResult.insert(0x01F0,"IMAGE_FILE_MACHINE_POWERPC");
    mapResult.insert(0x01f1,"IMAGE_FILE_MACHINE_POWERPCFP");
    mapResult.insert(0x0200,"IMAGE_FILE_MACHINE_IA64");
    mapResult.insert(0x0266,"IMAGE_FILE_MACHINE_MIPS16");
    mapResult.insert(0x0284,"IMAGE_FILE_MACHINE_ALPHA64");
    mapResult.insert(0x0366,"IMAGE_FILE_MACHINE_MIPSFPU");
    mapResult.insert(0x0466,"IMAGE_FILE_MACHINE_MIPSFPU16");
    mapResult.insert(0x0520,"IMAGE_FILE_MACHINE_TRICORE");
    mapResult.insert(0x0CEF,"IMAGE_FILE_MACHINE_CEF");
    mapResult.insert(0x0EBC,"IMAGE_FILE_MACHINE_EBC");
    mapResult.insert(0x8664,"IMAGE_FILE_MACHINE_AMD64");
    mapResult.insert(0x9041,"IMAGE_FILE_MACHINE_M32R");
    mapResult.insert(0xC0EE,"IMAGE_FILE_MACHINE_CEE");
    mapResult.insert(0xAA64,"IMAGE_FILE_MACHINE_ARM64");
    mapResult.insert(0x5032,"IMAGE_FILE_MACHINE_RISCV32");
    mapResult.insert(0x5064,"IMAGE_FILE_MACHINE_RISCV64");
    mapResult.insert(0x5128,"IMAGE_FILE_MACHINE_RISCV128");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageFileHeaderMachinesS()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0,"UNKNOWN");
    mapResult.insert(0x014c,"I386");
    mapResult.insert(0x0162,"R3000");
    mapResult.insert(0x0166,"R4000");
    mapResult.insert(0x0168,"R10000");
    mapResult.insert(0x0169,"WCEMIPSV2");
    mapResult.insert(0x0184,"ALPHA");
    mapResult.insert(0x01a2,"SH3");
    mapResult.insert(0x01a3,"SH3DSP");
    mapResult.insert(0x01a4,"SH3E");
    mapResult.insert(0x01a6,"SH4");
    mapResult.insert(0x01a8,"SH5");
    mapResult.insert(0x01c0,"ARM");
    mapResult.insert(0x01c2,"THUMB");
    mapResult.insert(0x01c4,"ARMNT");
    mapResult.insert(0x01d3,"AM33");
    mapResult.insert(0x01F0,"POWERPC");
    mapResult.insert(0x01f1,"POWERPCFP");
    mapResult.insert(0x0200,"IA64");
    mapResult.insert(0x0266,"MIPS16");
    mapResult.insert(0x0284,"ALPHA64");
    mapResult.insert(0x0366,"MIPSFPU");
    mapResult.insert(0x0466,"MIPSFPU16");
    mapResult.insert(0x0520,"TRICORE");
    mapResult.insert(0x0CEF,"CEF");
    mapResult.insert(0x0EBC,"EBC");
    mapResult.insert(0x8664,"AMD64");
    mapResult.insert(0x9041,"M32R");
    mapResult.insert(0xC0EE,"CEE");
    mapResult.insert(0xAA64,"ARM64");
    mapResult.insert(0x5032,"RISCV32");
    mapResult.insert(0x5064,"RISCV64");
    mapResult.insert(0x5128,"RISCV128");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageFileHeaderCharacteristics()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0x0001,"IMAGE_FILE_RELOCS_STRIPPED");
    mapResult.insert(0x0002,"IMAGE_FILE_EXECUTABLE_IMAGE");
    mapResult.insert(0x0004,"IMAGE_FILE_LINE_NUMS_STRIPPED");
    mapResult.insert(0x0008,"IMAGE_FILE_LOCAL_SYMS_STRIPPED");
    mapResult.insert(0x0010,"IMAGE_FILE_AGGRESIVE_WS_TRIM");
    mapResult.insert(0x0020,"IMAGE_FILE_LARGE_ADDRESS_AWARE");
    mapResult.insert(0x0080,"IMAGE_FILE_BYTES_REVERSED_LO");
    mapResult.insert(0x0100,"IMAGE_FILE_32BIT_MACHINE");
    mapResult.insert(0x0200,"IMAGE_FILE_DEBUG_STRIPPED");
    mapResult.insert(0x0400,"IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP");
    mapResult.insert(0x0800,"IMAGE_FILE_NET_RUN_FROM_SWAP");
    mapResult.insert(0x1000,"IMAGE_FILE_SYSTEM");
    mapResult.insert(0x2000,"IMAGE_FILE_DLL");
    mapResult.insert(0x4000,"IMAGE_FILE_UP_SYSTEM_ONLY");
    mapResult.insert(0x8000,"IMAGE_FILE_BYTES_REVERSED_HI");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageFileHeaderCharacteristicsS()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0x0001,"RELOCS_STRIPPED");
    mapResult.insert(0x0002,"EXECUTABLE_IMAGE");
    mapResult.insert(0x0004,"LINE_NUMS_STRIPPED");
    mapResult.insert(0x0008,"LOCAL_SYMS_STRIPPED");
    mapResult.insert(0x0010,"AGGRESIVE_WS_TRIM");
    mapResult.insert(0x0020,"LARGE_ADDRESS_AWARE");
    mapResult.insert(0x0080,"BYTES_REVERSED_LO");
    mapResult.insert(0x0100,"32BIT_MACHINE");
    mapResult.insert(0x0200,"DEBUG_STRIPPED");
    mapResult.insert(0x0400,"REMOVABLE_RUN_FROM_SWAP");
    mapResult.insert(0x0800,"NET_RUN_FROM_SWAP");
    mapResult.insert(0x1000,"SYSTEM");
    mapResult.insert(0x2000,"DLL");
    mapResult.insert(0x4000,"UP_SYSTEM_ONLY");
    mapResult.insert(0x8000,"BYTES_REVERSED_HI");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageOptionalHeaderMagic()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0x10b,"IMAGE_NT_OPTIONAL_HDR32_MAGIC");
    mapResult.insert(0x20b,"IMAGE_NT_OPTIONAL_HDR64_MAGIC");
    mapResult.insert(0x107,"IMAGE_ROM_OPTIONAL_HDR_MAGIC");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageOptionalHeaderMagicS()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0x10b,"NT_HDR32_MAGIC");
    mapResult.insert(0x20b,"NT_HDR64_MAGIC");
    mapResult.insert(0x107,"ROM_HDR_MAGIC");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageOptionalHeaderSubsystem()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0,"IMAGE_SUBSYSTEM_UNKNOWN");
    mapResult.insert(1,"IMAGE_SUBSYSTEM_NATIVE");
    mapResult.insert(2,"IMAGE_SUBSYSTEM_WINDOWS_GUI");
    mapResult.insert(3,"IMAGE_SUBSYSTEM_WINDOWS_CUI");
    mapResult.insert(5,"IMAGE_SUBSYSTEM_OS2_CUI");
    mapResult.insert(7,"IMAGE_SUBSYSTEM_POSIX_CUI");
    mapResult.insert(8,"IMAGE_SUBSYSTEM_NATIVE_WINDOWS");
    mapResult.insert(9,"IMAGE_SUBSYSTEM_WINDOWS_CE_GUI");
    mapResult.insert(10,"IMAGE_SUBSYSTEM_EFI_APPLICATION");
    mapResult.insert(11,"IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER");
    mapResult.insert(12,"IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER");
    mapResult.insert(13,"IMAGE_SUBSYSTEM_EFI_ROM");
    mapResult.insert(14,"IMAGE_SUBSYSTEM_XBOX");
    mapResult.insert(16,"IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageOptionalHeaderSubsystemS()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0,"UNKNOWN");
    mapResult.insert(1,"NATIVE");
    mapResult.insert(2,"WINDOWS_GUI");
    mapResult.insert(3,"WINDOWS_CUI");
    mapResult.insert(5,"OS2_CUI");
    mapResult.insert(7,"POSIX_CUI");
    mapResult.insert(8,"NATIVE_WINDOWS");
    mapResult.insert(9,"WINDOWS_CE_GUI");
    mapResult.insert(10,"EFI_APPLICATION");
    mapResult.insert(11,"EFI_BOOT_SERVICE_DRIVER");
    mapResult.insert(12,"EFI_RUNTIME_DRIVER");
    mapResult.insert(13,"EFI_ROM");
    mapResult.insert(14,"XBOX");
    mapResult.insert(16,"WINDOWS_BOOT_APPLICATION");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageOptionalHeaderDllCharacteristics()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0x0020,"IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA");
    mapResult.insert(0x0040,"IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE");
    mapResult.insert(0x0080,"IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY");
    mapResult.insert(0x0100,"IMAGE_DLLCHARACTERISTICS_NX_COMPAT");
    mapResult.insert(0x0200,"IMAGE_DLLCHARACTERISTICS_NO_ISOLATION");
    mapResult.insert(0x0400,"IMAGE_DLLCHARACTERISTICS_NO_SEH");
    mapResult.insert(0x0800,"IMAGE_DLLCHARACTERISTICS_NO_BIND");
    mapResult.insert(0x1000,"IMAGE_DLLCHARACTERISTICS_APPCONTAINER");
    mapResult.insert(0x2000,"IMAGE_DLLCHARACTERISTICS_WDM_DRIVER");
    mapResult.insert(0x4000,"IMAGE_DLLCHARACTERISTICS_GUARD_CF");
    mapResult.insert(0x8000,"IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageOptionalHeaderDllCharacteristicsS()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0x0020,"HIGH_ENTROPY_VA");
    mapResult.insert(0x0040,"DYNAMIC_BASE");
    mapResult.insert(0x0080,"FORCE_INTEGRITY");
    mapResult.insert(0x0100,"NX_COMPAT");
    mapResult.insert(0x0200,"NO_ISOLATION");
    mapResult.insert(0x0400,"NO_SEH");
    mapResult.insert(0x0800,"NO_BIND");
    mapResult.insert(0x1000,"APPCONTAINER");
    mapResult.insert(0x2000,"WDM_DRIVER");
    mapResult.insert(0x4000,"GUARD_CF");
    mapResult.insert(0x8000,"TERMINAL_SERVER_AWARE");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageOptionalHeaderDataDirectory()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0,"IMAGE_DIRECTORY_ENTRY_EXPORT");
    mapResult.insert(1,"IMAGE_DIRECTORY_ENTRY_IMPORT");
    mapResult.insert(2,"IMAGE_DIRECTORY_ENTRY_RESOURCE");
    mapResult.insert(3,"IMAGE_DIRECTORY_ENTRY_EXCEPTION");
    mapResult.insert(4,"IMAGE_DIRECTORY_ENTRY_SECURITY");
    mapResult.insert(5,"IMAGE_DIRECTORY_ENTRY_BASERELOC");
    mapResult.insert(6,"IMAGE_DIRECTORY_ENTRY_DEBUG");
    mapResult.insert(7,"IMAGE_DIRECTORY_ENTRY_ARCHITECTURE");
    mapResult.insert(8,"IMAGE_DIRECTORY_ENTRY_GLOBALPTR");
    mapResult.insert(9,"IMAGE_DIRECTORY_ENTRY_TLS");
    mapResult.insert(10,"IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG");
    mapResult.insert(11,"IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT");
    mapResult.insert(12,"IMAGE_DIRECTORY_ENTRY_IAT");
    mapResult.insert(13,"IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT");
    mapResult.insert(14,"IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR");
    mapResult.insert(15,"RESERVED");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageOptionalHeaderDataDirectoryS()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0,"EXPORT");
    mapResult.insert(1,"IMPORT");
    mapResult.insert(2,"RESOURCE");
    mapResult.insert(3,"EXCEPTION");
    mapResult.insert(4,"SECURITY");
    mapResult.insert(5,"BASERELOC");
    mapResult.insert(6,"DEBUG");
    mapResult.insert(7,"ARCHITECTURE");
    mapResult.insert(8,"GLOBALPTR");
    mapResult.insert(9,"TLS");
    mapResult.insert(10,"LOAD_CONFIG");
    mapResult.insert(11,"BOUND_IMPORT");
    mapResult.insert(12,"IAT");
    mapResult.insert(13,"DELAY_IMPORT");
    mapResult.insert(14,"COM_DESCRIPTOR");
    mapResult.insert(15,"RESERVED");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageSectionHeaderFlags()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0x00000008,"IMAGE_SCN_TYPE_NO_PAD");
    mapResult.insert(0x00000020,"IMAGE_SCN_CNT_CODE");
    mapResult.insert(0x00000040,"IMAGE_SCN_CNT_INITIALIZED_DATA");
    mapResult.insert(0x00000080,"IMAGE_SCN_CNT_UNINITIALIZED_DATA");
    mapResult.insert(0x00000100,"IMAGE_SCN_LNK_OTHER");
    mapResult.insert(0x00000200,"IMAGE_SCN_LNK_INFO");
    mapResult.insert(0x00000800,"IMAGE_SCN_LNK_REMOVE");
    mapResult.insert(0x00001000,"IMAGE_SCN_LNK_COMDAT");
    mapResult.insert(0x00004000,"IMAGE_SCN_NO_DEFER_SPEC_EXC");
    mapResult.insert(0x00008000,"IMAGE_SCN_GPREL");
    mapResult.insert(0x00020000,"IMAGE_SCN_MEM_PURGEABLE");
    mapResult.insert(0x00020000,"IMAGE_SCN_MEM_16BIT");
    mapResult.insert(0x00040000,"IMAGE_SCN_MEM_LOCKED");
    mapResult.insert(0x00080000,"IMAGE_SCN_MEM_PRELOAD");
    mapResult.insert(0x01000000,"IMAGE_SCN_LNK_NRELOC_OVFL");
    mapResult.insert(0x02000000,"IMAGE_SCN_MEM_DISCARDABLE");
    mapResult.insert(0x04000000,"IMAGE_SCN_MEM_NOT_CACHED");
    mapResult.insert(0x08000000,"IMAGE_SCN_MEM_NOT_PAGED");
    mapResult.insert(0x10000000,"IMAGE_SCN_MEM_SHARED");
    mapResult.insert(0x20000000,"IMAGE_SCN_MEM_EXECUTE");
    mapResult.insert(0x40000000,"IMAGE_SCN_MEM_READ");
    mapResult.insert(0x80000000,"IMAGE_SCN_MEM_WRITE");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageSectionHeaderFlagsS()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0x00000008,"TYPE_NO_PAD");
    mapResult.insert(0x00000020,"CNT_CODE");
    mapResult.insert(0x00000040,"CNT_INITIALIZED_DATA");
    mapResult.insert(0x00000080,"CNT_UNINITIALIZED_DATA");
    mapResult.insert(0x00000100,"LNK_OTHER");
    mapResult.insert(0x00000200,"LNK_INFO");
    mapResult.insert(0x00000800,"LNK_REMOVE");
    mapResult.insert(0x00001000,"LNK_COMDAT");
    mapResult.insert(0x00004000,"NO_DEFER_SPEC_EXC");
    mapResult.insert(0x00008000,"GPREL");
    mapResult.insert(0x00020000,"MEM_PURGEABLE");
    mapResult.insert(0x00020000,"MEM_16BIT");
    mapResult.insert(0x00040000,"MEM_LOCKED");
    mapResult.insert(0x00080000,"MEM_PRELOAD");
    mapResult.insert(0x01000000,"LNK_NRELOC_OVFL");
    mapResult.insert(0x02000000,"MEM_DISCARDABLE");
    mapResult.insert(0x04000000,"MEM_NOT_CACHED");
    mapResult.insert(0x08000000,"MEM_NOT_PAGED");
    mapResult.insert(0x10000000,"MEM_SHARED");
    mapResult.insert(0x20000000,"MEM_EXECUTE");
    mapResult.insert(0x40000000,"MEM_READ");
    mapResult.insert(0x80000000,"MEM_WRITE");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageSectionHeaderAligns()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0x00100000,"IMAGE_SCN_ALIGN_1BYTES");
    mapResult.insert(0x00200000,"IMAGE_SCN_ALIGN_2BYTES");
    mapResult.insert(0x00300000,"IMAGE_SCN_ALIGN_4BYTES");
    mapResult.insert(0x00400000,"IMAGE_SCN_ALIGN_8BYTES");
    mapResult.insert(0x00500000,"IMAGE_SCN_ALIGN_16BYTES");
    mapResult.insert(0x00600000,"IMAGE_SCN_ALIGN_32BYTES");
    mapResult.insert(0x00700000,"IMAGE_SCN_ALIGN_64BYTES");
    mapResult.insert(0x00800000,"IMAGE_SCN_ALIGN_128BYTES");
    mapResult.insert(0x00900000,"IMAGE_SCN_ALIGN_256BYTES");
    mapResult.insert(0x00A00000,"IMAGE_SCN_ALIGN_512BYTES");
    mapResult.insert(0x00B00000,"IMAGE_SCN_ALIGN_1024BYTES");
    mapResult.insert(0x00C00000,"IMAGE_SCN_ALIGN_2048BYTES");
    mapResult.insert(0x00D00000,"IMAGE_SCN_ALIGN_4096BYTES");
    mapResult.insert(0x00E00000,"IMAGE_SCN_ALIGN_8192BYTES");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageSectionHeaderAlignsS()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0x00100000,"1BYTES");
    mapResult.insert(0x00200000,"2BYTES");
    mapResult.insert(0x00300000,"4BYTES");
    mapResult.insert(0x00400000,"8BYTES");
    mapResult.insert(0x00500000,"16BYTES");
    mapResult.insert(0x00600000,"32BYTES");
    mapResult.insert(0x00700000,"64BYTES");
    mapResult.insert(0x00800000,"128BYTES");
    mapResult.insert(0x00900000,"256BYTES");
    mapResult.insert(0x00A00000,"512BYTES");
    mapResult.insert(0x00B00000,"1024BYTES");
    mapResult.insert(0x00C00000,"2048BYTES");
    mapResult.insert(0x00D00000,"4096BYTES");
    mapResult.insert(0x00E00000,"8192BYTES");

    return mapResult;
}

QMap<quint64, QString> XPE::getResourceTypes()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(1,"RT_CURSOR");
    mapResult.insert(2,"RT_BITMAP");
    mapResult.insert(3,"RT_ICON");
    mapResult.insert(4,"RT_MENU");
    mapResult.insert(5,"RT_DIALOG");
    mapResult.insert(6,"RT_STRING");
    mapResult.insert(7,"RT_FONTDIR");
    mapResult.insert(8,"RT_FONT");
    mapResult.insert(9,"RT_ACCELERATORS");
    mapResult.insert(10,"RT_RCDATA");
    mapResult.insert(11,"RT_MESSAGETABLE");
    mapResult.insert(12,"RT_GROUP_CURSOR");
    mapResult.insert(14,"RT_GROUP_ICON");
    mapResult.insert(16,"RT_VERSION");
    mapResult.insert(24,"RT_MANIFEST");
    mapResult.insert(0x2000+2,"RT_NEWBITMAP");
    mapResult.insert(0x2000+4,"RT_NEWMENU");
    mapResult.insert(0x2000+5,"RT_NEWDIALOG");

    return mapResult;
}

QMap<quint64, QString> XPE::getResourceTypesS()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(1,"CURSOR");
    mapResult.insert(2,"BITMAP");
    mapResult.insert(3,"ICON");
    mapResult.insert(4,"MENU");
    mapResult.insert(5,"DIALOG");
    mapResult.insert(6,"STRING");
    mapResult.insert(7,"FONTDIR");
    mapResult.insert(8,"FONT");
    mapResult.insert(9,"ACCELERATORS");
    mapResult.insert(10,"RCDATA");
    mapResult.insert(11,"MESSAGETABLE");
    mapResult.insert(12,"GROUP_CURSOR");
    mapResult.insert(14,"GROUP_ICON");
    mapResult.insert(16,"VERSION");
    mapResult.insert(24,"MANIFEST");
    mapResult.insert(0x2000+2,"NEWBITMAP");
    mapResult.insert(0x2000+4,"NEWMENU");
    mapResult.insert(0x2000+5,"NEWDIALOG");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageRelBased()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0,"IMAGE_REL_BASED_ABSOLUTE");
    mapResult.insert(1,"IMAGE_REL_BASED_HIGH");
    mapResult.insert(2,"IMAGE_REL_BASED_LOW");
    mapResult.insert(3,"IMAGE_REL_BASED_HIGHLOW");
    mapResult.insert(4,"IMAGE_REL_BASED_HIGHADJ");
    mapResult.insert(5,"IMAGE_REL_BASED_MACHINE_SPECIFIC_5");
    mapResult.insert(6,"IMAGE_REL_BASED_RESERVED");
    mapResult.insert(7,"IMAGE_REL_BASED_MACHINE_SPECIFIC_7");
    mapResult.insert(8,"IMAGE_REL_BASED_MACHINE_SPECIFIC_8");
    mapResult.insert(9,"IMAGE_REL_BASED_MACHINE_SPECIFIC_9");
    mapResult.insert(10,"IMAGE_REL_BASED_DIR64");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageRelBasedS()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0,"ABSOLUTE");
    mapResult.insert(1,"HIGH");
    mapResult.insert(2,"LOW");
    mapResult.insert(3,"HIGHLOW");
    mapResult.insert(4,"HIGHADJ");
    mapResult.insert(5,"MACHINE_SPECIFIC_5");
    mapResult.insert(6,"RESERVED");
    mapResult.insert(7,"MACHINE_SPECIFIC_7");
    mapResult.insert(8,"MACHINE_SPECIFIC_8");
    mapResult.insert(9,"MACHINE_SPECIFIC_9");
    mapResult.insert(10,"DIR64");

    return mapResult;
}

QMap<quint64, QString> XPE::getComImageFlags()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0x00000001,"COMIMAGE_FLAGS_ILONLY");
    mapResult.insert(0x00000002,"COMIMAGE_FLAGS_32BITREQUIRED");
    mapResult.insert(0x00000004,"COMIMAGE_FLAGS_IL_LIBRARY");
    mapResult.insert(0x00000008,"COMIMAGE_FLAGS_STRONGNAMESIGNED");
    mapResult.insert(0x00000010,"COMIMAGE_FLAGS_NATIVE_ENTRYPOINT");
    mapResult.insert(0x00010000,"COMIMAGE_FLAGS_TRACKDEBUGDATA");

    return mapResult;
}

QMap<quint64, QString> XPE::getComImageFlagsS()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0x00000001,"ILONLY");
    mapResult.insert(0x00000002,"32BITREQUIRED");
    mapResult.insert(0x00000004,"IL_LIBRARY");
    mapResult.insert(0x00000008,"STRONGNAMESIGNED");
    mapResult.insert(0x00000010,"NATIVE_ENTRYPOINT");
    mapResult.insert(0x00010000,"TRACKDEBUGDATA");

    return mapResult;
}

QMap<quint64, QString> XPE::getDebugTypes()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0,"IMAGE_DEBUG_TYPE_UNKNOWN");
    mapResult.insert(1,"IMAGE_DEBUG_TYPE_COFF");
    mapResult.insert(2,"IMAGE_DEBUG_TYPE_CODEVIEW");
    mapResult.insert(3,"IMAGE_DEBUG_TYPE_FPO");
    mapResult.insert(4,"IMAGE_DEBUG_TYPE_MISC");
    mapResult.insert(5,"IMAGE_DEBUG_TYPE_EXCEPTION");
    mapResult.insert(6,"IMAGE_DEBUG_TYPE_FIXUP");
    mapResult.insert(7,"IMAGE_DEBUG_TYPE_OMAP_TO_SRC");
    mapResult.insert(8,"IMAGE_DEBUG_TYPE_OMAP_FROM_SRC");
    mapResult.insert(9,"IMAGE_DEBUG_TYPE_BORLAND");
    mapResult.insert(10,"IMAGE_DEBUG_TYPE_RESERVED10");
    mapResult.insert(11,"IMAGE_DEBUG_TYPE_CLSID");
    mapResult.insert(12,"IMAGE_DEBUG_TYPE_VC_FEATURE");
    mapResult.insert(13,"IMAGE_DEBUG_TYPE_POGO");
    mapResult.insert(14,"IMAGE_DEBUG_TYPE_ILTCG");
    mapResult.insert(15,"IMAGE_DEBUG_TYPE_MPX");
    mapResult.insert(16,"IMAGE_DEBUG_TYPE_REPRO");
    mapResult.insert(20,"IMAGE_DEBUG_TYPE_EX_DLLCHARACTERISTICS");

    return mapResult;
}

QMap<quint64, QString> XPE::getDebugTypesS()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0,"UNKNOWN");
    mapResult.insert(1,"COFF");
    mapResult.insert(2,"CODEVIEW");
    mapResult.insert(3,"FPO");
    mapResult.insert(4,"MISC");
    mapResult.insert(5,"EXCEPTION");
    mapResult.insert(6,"FIXUP");
    mapResult.insert(7,"OMAP_TO_SRC");
    mapResult.insert(8,"OMAP_FROM_SRC");
    mapResult.insert(9,"BORLAND");
    mapResult.insert(10,"RESERVED10");
    mapResult.insert(11,"CLSID");
    mapResult.insert(12,"VC_FEATURE");
    mapResult.insert(13,"POGO");
    mapResult.insert(14,"ILTCG");
    mapResult.insert(15,"MPX");
    mapResult.insert(16,"REPRO");
    mapResult.insert(20,"EX_DLLCHARACTERISTICS");

    return mapResult;
}
