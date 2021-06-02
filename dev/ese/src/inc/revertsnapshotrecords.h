// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include <pshpack1.h>

// Addition of any new record type should have a corresponding update in -
//      -   SzRBSRec    ( to return corresponding RBS record tag )
//      -   RBSRecToSz  ( to return string representation of the record )
//
//
#define     rbsrectypeNOP               0
#define     rbsrectypeDbHdr             1
#define     rbsrectypeDbAttach          2
#define     rbsrectypeDbPage            3
#define     rbsrectypeFragBegin         4
#define     rbsrectypeFragContinue      5
#define     rbsrectypeDbNewPage         6
#define     rbsrectypeMax               7

PERSISTED
struct RBSRecord
{
    BYTE                            m_bRecType;
    UnalignedLittleEndian<USHORT>   m_usRecLength;
};

PERSISTED
struct RBSDbHdrRecord : public RBSRecord
{
    UnalignedLittleEndian<DBID>     m_dbid;
    BYTE                            m_rgbHeader[0];
};

PERSISTED
struct RBSDbAttachRecord : public RBSRecord
{
    UnalignedLittleEndian<DBID>     m_dbid;
    UnalignedLittleEndian<WCHAR>    m_wszDbName[0];
};

PERSISTED const ULONG fRBSPreimageDehydrated = 0x1;
PERSISTED const ULONG fRBSPreimageXpress = 0x2;

PERSISTED
struct RBSDbPageRecord : public RBSRecord
{
    UnalignedLittleEndian<DBID>     m_dbid;
    UnalignedLittleEndian<PGNO>     m_pgno;
    UnalignedLittleEndian<ULONG>    m_fFlags;
    BYTE                            m_rgbData[0];
};

PERSISTED
struct RBSDbNewPageRecord : public RBSRecord
{
    UnalignedLittleEndian<DBID>     m_dbid;
    UnalignedLittleEndian<PGNO>     m_pgno;
};

ERR ErrRBSDecompressPreimage(
    DATA &data,
    const LONG cbPage,
    BYTE *pbDataDecompressed,
    PGNO pgno,
    ULONG fFlags );

PERSISTED
struct RBSFragBegin : public RBSRecord
{
    UnalignedLittleEndian<USHORT>   m_usTotalRecLength;
};

PERSISTED
struct RBSFragContinue : public RBSRecord
{
};

INLINE USHORT
CbRBSRecFixed( BYTE bRecType )
{
    switch ( bRecType )
    {
        case rbsrectypeDbHdr:
            return sizeof( RBSDbHdrRecord );
        case rbsrectypeDbAttach:
            return sizeof( RBSDbAttachRecord );
        case rbsrectypeDbPage:
            return sizeof( RBSDbPageRecord );
        case rbsrectypeFragBegin:
            return sizeof( RBSFragBegin );
        case rbsrectypeFragContinue:
            return sizeof( RBSFragContinue );
        case rbsrectypeDbNewPage:
            return sizeof( RBSDbNewPageRecord );
        default:
           Assert( fFalse );
        case rbsrectypeNOP:
            return 0;
    }
}

#include <poppack.h>
