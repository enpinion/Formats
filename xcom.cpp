// copyright (c) 2020-2021 hors<horsicq@gmail.com>
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
#include "xcom.h"

XCOM::XCOM(QIODevice *pDevice, bool bIsImage, qint64 nImageBase) : XBinary(pDevice,bIsImage,nImageBase)
{

}

XCOM::~XCOM()
{

}

bool XCOM::isValid()
{
    return true; // mb TODO
}

qint64 XCOM::getBaseAddress()
{
    return XCOM_DEF::S_ADDRESS_BEGIN;
}

XBinary::_MEMORY_MAP XCOM::getMemoryMap()
{
    _MEMORY_MAP result={};

    qint64 nTotalSize=getSize();

    result.nBaseAddress=_getBaseAddress();
    result.nRawSize=nTotalSize;
    result.nImageSize=getImageSize();
    result.fileType=FT_COM;
    result.mode=getMode();
    result.sArch=getArch();
    result.bIsBigEndian=isBigEndian();
    result.sType=getTypeAsString();

    qint64 nCodeSize=qMin(nTotalSize,(qint64)(XCOM_DEF::S_IMAGESIZE-XCOM_DEF::S_ADDRESS_BEGIN));

    _MEMORY_RECORD record={};
    record.nAddress=0;
    record.segment=ADDRESS_SEGMENT_FLAT;
    record.nOffset=-1;
    record.nSize=XCOM_DEF::S_ADDRESS_BEGIN;
    record.nIndex++;

    result.listRecords.append(record);

    _MEMORY_RECORD recordMain={};
    recordMain.nAddress=XCOM_DEF::S_ADDRESS_BEGIN;
    recordMain.segment=ADDRESS_SEGMENT_FLAT;
    recordMain.nOffset=0;
    recordMain.nSize=nCodeSize;
    recordMain.nIndex++;

    result.listRecords.append(recordMain);

    qint64 nVirtualSize=(qint64)(XCOM_DEF::S_IMAGESIZE-XCOM_DEF::S_ADDRESS_BEGIN)-nTotalSize;

    if(nVirtualSize>0)
    {
        _MEMORY_RECORD record={};
        record.nAddress=XCOM_DEF::S_ADDRESS_BEGIN+nCodeSize;
        record.segment=ADDRESS_SEGMENT_FLAT;
        record.nOffset=-1;
        record.nSize=nVirtualSize;
        record.nIndex++;
        record.bIsVirtual=true;

        result.listRecords.append(record);
    }

    if(nTotalSize>nCodeSize)
    {
        _MEMORY_RECORD recordOverlay={};
        recordOverlay.nAddress=-1;
        recordOverlay.nOffset=nCodeSize;
        recordOverlay.nSize=nTotalSize-nCodeSize;
        recordOverlay.nIndex++;
        recordOverlay.type=MMT_OVERLAY;

        result.listRecords.append(recordOverlay);
    }

    return result;
}

QString XCOM::getArch()
{
    return "8086";
}

XBinary::MODE XCOM::getMode()
{
    return MODE_16;
}

bool XCOM::isBigEndian()
{
    return false;
}

qint64 XCOM::getEntryPointOffset(_MEMORY_MAP *pMemoryMap)
{
    Q_UNUSED(pMemoryMap)

    return 0;
}

qint64 XCOM::getImageSize()
{
    return 0x10000;
}

int XCOM::getType()
{
    return TYPE_EXE;
}

QString XCOM::typeIdToString(int nType)
{
    QString sResult="Unknown"; // mb TODO translate

    switch(nType)
    {
        case TYPE_UNKNOWN:      sResult=QString("Unknown");     break; // mb TODO translate
        case TYPE_EXE:          sResult=QString("EXE");         break;
    }

    return sResult;
}
