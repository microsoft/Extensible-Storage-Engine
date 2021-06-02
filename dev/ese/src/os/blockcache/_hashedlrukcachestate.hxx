// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#pragma warning (push)
#pragma warning (disable: 4315)  //  'CCachedBlock': 'this' pointer for member 'CCachedBlock::m_le_fileid' may not be aligned 8 as expected by the constructor

//  Cluster Number

//PERSISTED
enum class ClusterNumber : DWORD  //  clno
{
    clnoInvalid = dwMax,
};

//  Block Number

//PERSISTED
enum class BlockNumber : DWORD  //  blno
{
    blnoInvalid = dwMax,
};

//  Cluster Operation

//PERSISTED
enum class ClusterOperation : BYTE  //  clop
{
    clopUpdate = 1,
    clopWriteBack = 2,
    clopInvalidate = 3,
    clopAccess = 4,
};

//  Touch Number

//PERSISTED
enum class TouchNumber : DWORD  //  tono
{
};

//  Cached Block

#include <pshpack1.h>

//PERSISTED
class CCachedBlock  //  cbl
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

        const UnalignedLittleEndian<VolumeId>       m_le_volumeid;      //  the volume id of the cached file
        const UnalignedLittleEndian<FileId>         m_le_fileid;        //  the file id of the cached file
        const UnalignedLittleEndian<FileSerial>     m_le_fileserial;    //  the serial number of the cached file
        const UnalignedLittleEndian<BlockNumber>    m_le_blno;          //  the block number of the cached file for the data
};

#include <poppack.h>

//  Cluster State

#include <pshpack1.h>

//PERSISTED
class CClusterState  //  clst
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

        const CCachedBlock                              m_cbl;              //  the cached block
        const UnalignedLittleEndian<ClusterNumber>      m_le_clno;          //  the cluster number of the caching file holding the cached block
        const UnalignedLittleEndian<DWORD>              m_le_ecc;           //  ECC of the block's contents
        const UnalignedLittleEndian<TouchNumber>        m_le_rgtono[ 2 ];   //  touch numbers for the replacement policy
        const UnalignedLittleEndian<ClusterOperation>   m_le_clopLast;      //  the last operation on this cluster
        const BYTE                                      m_rgbReserved[ 3 ]; //  reserved; always zero
};

#include <poppack.h>

//  Journal Entry Cluster Update

#include <pshpack1.h>

//PERSISTED
class CJournalEntryClusterUpdate  //  jentcu
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

        const UnalignedLittleEndian<ClusterNumber>  m_le_clno;  //  cluster number
        CClusterState                               m_clst;     //  cluster state
};

#include <poppack.h>

//  Journal Entry Header

#include <pshpack1.h>

//PERSISTED
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

        const UnalignedLittleEndian<ULONG>  m_le_cClusterUpdate;    //  count of cluster updates in this entry
        CJournalEntryClusterUpdate          m_rgjentcu[ 0 ];        //  cluster updates
};

#include <poppack.h>

#pragma warning (pop)
