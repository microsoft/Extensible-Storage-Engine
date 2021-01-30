// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#pragma warning (push)
#pragma warning (disable: 4315)


enum class ClusterNumber : DWORD
{
    clnoInvalid = dwMax,
};


enum class BlockNumber : DWORD
{
    blnoInvalid = dwMax,
};


enum class ClusterOperation : BYTE
{
    clopUpdate = 1,
    clopWriteBack = 2,
    clopInvalidate = 3,
    clopAccess = 4,
};


enum class TouchNumber : DWORD
{
};


#include <pshpack1.h>

class CCachedBlock
{
    public:

        CCachedBlock(   _In_ const VolumeId         volumeid,
                        _In_ const FileId           fileid,
                        _In_ const FileSerial       fileserial,
                        _In_ const BlockNumber      blno )
            :   m_le_volumeid( volumeid ),
                m_le_fileid( fileid ),
                m_le_fileserial( fileserial ),
                m_le_blno( blno )
        {
            C_ASSERT( 20 == sizeof( CCachedBlock ) );
        }

        VolumeId Volumeid() const { return m_le_volumeid; }
        FileId Fileid() const { return m_le_fileid; }
        FileSerial Fileserial() const { return m_le_fileserial; }
        BlockNumber Blno() const { return m_le_blno; }

    private:

        const UnalignedLittleEndian<VolumeId>       m_le_volumeid;
        const UnalignedLittleEndian<FileId>         m_le_fileid;
        const UnalignedLittleEndian<FileSerial>     m_le_fileserial;
        const UnalignedLittleEndian<BlockNumber>    m_le_blno;
};

#include <poppack.h>


#include <pshpack1.h>

class CClusterState
{
    public:

        CClusterState(  _In_ const CCachedBlock     cbl,
                        _In_ const ClusterNumber    clno,
                        _In_ const DWORD            ecc,
                        _In_ const TouchNumber      tono0,
                        _In_ const TouchNumber      tono1,
                        _In_ const ClusterOperation clop )
            :   m_cbl( cbl ),
                m_le_clno( clno ),
                m_le_ecc( ecc ),
                m_le_rgtono { tono0, tono1 },
                m_le_clopLast( clop ),
                m_rgbReserved{ 0 }
        {
            C_ASSERT( 40 == sizeof( CClusterState ) );
        }

        const CCachedBlock& Cbl() const { return m_cbl; }
        ClusterNumber Clno() const { return m_le_clno; }
        DWORD Ecc() const { return m_le_ecc; }
        TouchNumber Tono0() const { return m_le_rgtono[ 0 ]; }
        TouchNumber Tono1() const { return m_le_rgtono[ 1 ]; }
        ClusterOperation ClopLast() const { return m_le_clopLast; }

    private:

        const CCachedBlock                              m_cbl;
        const UnalignedLittleEndian<ClusterNumber>      m_le_clno;
        const UnalignedLittleEndian<DWORD>              m_le_ecc;
        const UnalignedLittleEndian<TouchNumber>        m_le_rgtono[ 2 ];
        const UnalignedLittleEndian<ClusterOperation>   m_le_clopLast;
        const BYTE                                      m_rgbReserved[ 3 ];
};

#include <poppack.h>


#include <pshpack1.h>

class CJournalEntryClusterUpdate
{
    public:

        CJournalEntryClusterUpdate( _In_ const ClusterNumber clno, _In_ const CClusterState clst )
            :   m_le_clno( clno ),
                m_clst( clst )
        {
            C_ASSERT( 0 == offsetof( CJournalEntryClusterUpdate, m_le_clno ) );
        }

        ClusterNumber Clno() const { return m_le_clno; }
        const CClusterState& Clst() const { return m_clst; }

    private:

        const UnalignedLittleEndian<ClusterNumber>  m_le_clno;
        CClusterState                               m_clst;
};

#include <poppack.h>


#include <pshpack1.h>

class CJournalEntryHeader
{
    public:

        CJournalEntryHeader( _In_ const ULONG cClusterUpdate )
            :   m_le_cClusterUpdate( cClusterUpdate ),
                m_rgjentcu { }
        {
            C_ASSERT( 0 == offsetof( CJournalEntryHeader, m_le_cClusterUpdate ) );
        }

        ULONG CClusterUpdate() const { return m_le_cClusterUpdate; }
        const CJournalEntryClusterUpdate* Pjentcu() const { return &m_rgjentcu[ 0 ]; }

    private:

        const UnalignedLittleEndian<ULONG>  m_le_cClusterUpdate;
        CJournalEntryClusterUpdate          m_rgjentcu[ 0 ];
};

#include <poppack.h>

#pragma warning (pop)
