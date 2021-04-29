// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  Journal Entry

#pragma warning (push)
#pragma warning (disable: 4315)  //  '': 'this' pointer for member '' may not be aligned X as expected by the constructor

#include <pshpack1.h>

//PERSISTED
template<class T>
class TJournalEntry  //  je
{
    public:

        static ERR ErrValidate( _In_ const CJournalBuffer jb )
        {
            ERR                         err = JET_errSuccess;
            const TJournalEntry* const  pje = (const TJournalEntry*)jb.Rgb();

            //  fail if the journal entry buffer is too small to contain a journal entry

            if ( jb.Cb() < sizeof( TJournalEntry ) )
            {
                Error( ErrERRCheck( JET_errInternalError ) );
            }

            //  fail if this journal entry buffer is smaller than the declared size of the journal entry

            if ( jb.Cb() < pje->Cb() )
            {
                Error( ErrERRCheck( JET_errInternalError ) );
            }

        HandleError:
            return err;
        }

        T Jetyp() const { return m_le_jetyp; }
        ULONG Cb() const { return m_le_cb; }

    protected:

        TJournalEntry( _In_ const T jetyp, _In_ const ULONG cb )
            :   m_le_jetyp( jetyp ),
                m_le_cb( cb )
        {
            C_ASSERT( offsetof( TJournalEntry, m_le_jetyp ) == 0 );
        }

    private:

        TJournalEntry() = delete;

        const UnalignedLittleEndian<T>      m_le_jetyp;
        const UnalignedLittleEndian<ULONG>  m_le_cb;
};

#pragma warning (pop) 

#include <poppack.h>

template<class T, T JETYP>
class TJournalEntryBase : public TJournalEntry<T>
{
    protected:

        TJournalEntryBase( _In_ const ULONG cb )
            : TJournalEntry( JETYP, cb )
        {
        }

    private:

        TJournalEntryBase() = delete;
};

//  Compressed Journal Entry

#pragma warning (push)
#pragma warning (disable: 4315)  //  '': 'this' pointer for member '' may not be aligned X as expected by the constructor

#include <pshpack1.h>

typedef __success( return >= 0 ) LONG NTSTATUS;

NTSTATUS
NTAPI
RtlGetCompressionWorkSpaceSize(
    _In_ USHORT CompressionFormatAndEngine,
    _Out_ PULONG CompressBufferWorkSpaceSize,
    _Out_ PULONG CompressFragmentWorkSpaceSize
);

static NTOSFuncNtStd( g_pfnRtlGetCompressionWorkSpaceSize, g_mwszzNtdllLibs, RtlGetCompressionWorkSpaceSize, oslfExpectedOnWin5x );

NTSTATUS
NTAPI
RtlCompressBuffer(
    _In_ USHORT CompressionFormatAndEngine,
    _In_reads_bytes_( UncompressedBufferSize ) PUCHAR UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _Out_writes_bytes_to_( CompressedBufferSize, *FinalCompressedSize ) PUCHAR CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _In_ ULONG UncompressedChunkSize,
    _Out_ PULONG FinalCompressedSize,
    _In_ PVOID WorkSpace
);

static NTOSFuncNtStd( g_pfnRtlCompressBuffer, g_mwszzNtdllLibs, RtlCompressBuffer, oslfExpectedOnWin5x );

NTSTATUS
NTAPI
RtlDecompressBuffer(
    _In_ USHORT CompressionFormat,
    _Out_writes_bytes_to_( UncompressedBufferSize, *FinalUncompressedSize ) PUCHAR UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _In_reads_bytes_( CompressedBufferSize ) PUCHAR CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _Out_ PULONG FinalUncompressedSize
);

static NTOSFuncNtStd( g_pfnRtlDecompressBuffer, g_mwszzNtdllLibs, RtlDecompressBuffer, oslfExpectedOnWin5x );

//PERSISTED
template<class T, T JETYPCOMPRESSED>
class TCompressedJournalEntry : public TJournalEntryBase<T, JETYPCOMPRESSED>  //  cje
{
    public:

        static ERR ErrCreate(   _In_    const TJournalEntry<T>*         pje,
                                _Out_   const TJournalEntry<T>** const  ppjeCompressed );

        static ERR ErrExtract( _In_ const CJournalBuffer jb, _Out_ const TJournalEntry<T>** const ppje );

    private:

        //PERSISTED
        enum class CompressionAlgorithm : BYTE  //  ca
        {
            caInvalid = 0,
            caLegacyXpressHuffman = 1,
        };

        static const CompressionAlgorithm caInvalid = CompressionAlgorithm::caInvalid;
        static const CompressionAlgorithm caLegacyXpressHuffman = CompressionAlgorithm::caLegacyXpressHuffman;

        TCompressedJournalEntry(    _In_ const TJournalEntry<T>* const  pje,
                                    _In_ const CompressionAlgorithm     ca,
                                    _In_ const ULONG                    cbCompressed );

        TCompressedJournalEntry() = delete;

        ULONG CbCompressed() const { return Cb() - sizeof( CCompressedJournalEntry ); }

    private:

        const UnalignedLittleEndian<CompressionAlgorithm>   m_le_ca;                //  compression algorithm
        const UnalignedLittleEndian<ULONG>                  m_le_cbUncompressed;    //  uncompressed size of the journal entry
        const UnalignedLittleEndian<ULONG>                  m_le_crc32Uncompressed; //  crc32 of uncompressed journal entry
        const BYTE                                          m_rgbCompressed[ 0 ];   //  compressed journal entry
};

#include <poppack.h>

template<class T, T JETYPCOMPRESSED>
INLINE ERR TCompressedJournalEntry<T, JETYPCOMPRESSED>::ErrCreate(  _In_    const TJournalEntry<T>*         pje,
                                                                    _Out_   const TJournalEntry<T>** const  ppjeCompressed )
{
    ERR                     err                 = JET_errSuccess;
    void*                   pv                  = NULL;
    CompressionAlgorithm    ca                  = caInvalid;
    USHORT                  compressionFormat   = COMPRESSION_FORMAT_NONE;
    NTSTATUS                status              = 0;
    ULONG                   cbWorkspace         = 0;
    ULONG                   cbUnused            = 0;
    BYTE*                   rgbWorkspace        = NULL;
    ULONG                   cbCompressed        = 0;
    BOOL                    fCompressed         = fTrue;
    const TJournalEntry<T>* pjeCompressed       = NULL;

    *ppjeCompressed = NULL;

    //  allocate worst case memory for the compressed journal entry

    Alloc( pv = PvOSMemoryHeapAlloc( sizeof( CCompressedJournalEntry ) + pje->Cb() ) );

    //  determine our compression algorithm

    ca = caLegacyXpressHuffman;
    compressionFormat = COMPRESSION_FORMAT_XPRESS_HUFF | COMPRESSION_ENGINE_STANDARD;

    //  allocate our workspace for compression

    status = g_pfnRtlGetCompressionWorkSpaceSize( compressionFormat, &cbWorkspace, &cbUnused );
    if ( status >= 0 )
    {
        Alloc( rgbWorkspace = new BYTE[ cbWorkspace ] );
    }

    //  try to compress the journal entry

    if ( status >= 0 )
    {
        status = g_pfnRtlCompressBuffer( compressionFormat,
                                            (PUCHAR)pje,
                                            pje->Cb(),
                                            (PUCHAR)((CCompressedJournalEntry*)pv)->m_rgbCompressed,
                                            pje->Cb(),
                                            4096,
                                            &cbCompressed,
                                            rgbWorkspace );
    }

    //  determine if we successfully compressed the journal entry

    fCompressed = fCompressed && status >= 0;
    fCompressed = fCompressed && sizeof( CCompressedJournalEntry ) + cbCompressed < pje->Cb();
    fCompressed = fCompressed && pje->Jetyp() != JETYPCOMPRESSED;

    //  if we succeeded then we will return a compressed journal entry

    if ( fCompressed )
    {
        pjeCompressed = new( pv ) CCompressedJournalEntry( pje, ca, cbCompressed );
        pv = NULL;
    }

    //  if we failed to compress then we will return a copy of the original uncompressed journal entry

    else
    {
        memcpy( pv, (void*)pje, pje->Cb() );
        pjeCompressed = (TJournalEntry<T>*)pv;
        pv = NULL;
    }

    //  return the compressed journal entry

    *ppjeCompressed = pjeCompressed;
    pjeCompressed = NULL;

HandleError:
    delete pjeCompressed;
    delete[] rgbWorkspace;
    OSMemoryHeapFree( pv );
    if ( err < JET_errSuccess )
    {
        delete *ppjeCompressed;
        *ppjeCompressed = NULL;
    }
    return err;
}

template<class T, T JETYPCOMPRESSED>
INLINE ERR TCompressedJournalEntry<T, JETYPCOMPRESSED>::ErrExtract( _In_    const CJournalBuffer            jb,
                                                                    _Out_   const TJournalEntry<T>** const  ppje )
{
    ERR                                     err             = JET_errSuccess;
    const CCompressedJournalEntry* const    pcje            = (const CCompressedJournalEntry*)jb.Rgb();
    void*                                   pv              = NULL;
    NTSTATUS                                status          = 0;
    ULONG                                   cbUncompressed  = 0;

    *ppje = NULL;

    //  verify that this is a valid journal entry

    Call( ErrValidate( jb ) );

    //  if this is a compressed journal entry then decompress it

    if ( pcje->Jetyp() == JETYPCOMPRESSED )
    {
        //  allocate enough storage to decompress the journal entry

        Alloc( pv = PvOSMemoryHeapAlloc( pcje->m_le_cbUncompressed ) );

        //  decompress based on the algorithm

        if ( pcje->m_le_ca == caLegacyXpressHuffman )
        {
            status = g_pfnRtlDecompressBuffer( COMPRESSION_FORMAT_XPRESS_HUFF | COMPRESSION_ENGINE_STANDARD,
                (PUCHAR)pv,
                pcje->m_le_cbUncompressed,
                (PUCHAR)pcje->m_rgbCompressed,
                pcje->CbCompressed(),
                &cbUncompressed );

            if ( status < 0 )
            {
                Error( ErrERRCheck( JET_errInternalError ) );
            }
            if ( cbUncompressed != pcje->m_le_cbUncompressed )
            {
                Error( ErrERRCheck( JET_errInternalError ) );
            }
            if ( Crc32Checksum( (const BYTE*)pv, pcje->m_le_cbUncompressed ) != pcje->m_le_crc32Uncompressed )
            {
                Error( ErrERRCheck( JET_errInternalError ) );
            }
            Call( ErrValidate( CJournalBuffer( pcje->m_le_cbUncompressed, (const BYTE*)pv ) ) );
            if ( pcje->Jetyp() == JETYPCOMPRESSED )
            {
                Error( ErrERRCheck( JET_errInternalError ) );
            }
        }
        else
        {
            Error( ErrERRCheck( JET_errInternalError ) );
        }
    }

    //  if this is any other type of journal entry then just copy it

    else
    {
        Alloc( pv = PvOSMemoryHeapAlloc( jb.Cb() ) );
        memcpy( pv, jb.Rgb(), jb.Cb() );
    }

    //  return the decompressed journal entry

    *ppje = (const TJournalEntry<T>*)pv;
    pv = NULL;

HandleError:
    OSMemoryHeapFree( pv );
    if ( err < JET_errSuccess )
    {
        delete *ppje;
        *ppje = NULL;
    }
    return err;
}

template<class T, T JETYPCOMPRESSED>
INLINE TCompressedJournalEntry<T, JETYPCOMPRESSED>::TCompressedJournalEntry(    _In_ const TJournalEntry<T>* const  pje,
                                                                                _In_ const CompressionAlgorithm     ca,
                                                                                _In_ const ULONG                    cbCompressed )
    :   TJournalEntryBase( sizeof( CCompressedJournalEntry) + cbCompressed ),
        m_le_ca( ca ),
        m_le_cbUncompressed( pje->Cb() ),
        m_le_crc32Uncompressed( Crc32Checksum( (const BYTE*)pje, pje->Cb() ) ),
        m_rgbCompressed { }
{
}

#pragma warning (pop)
