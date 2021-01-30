// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once


template< class I >
class TCacheWrapper
    :   public I
{
    public:

        TCacheWrapper( _In_ I* const pi );
        TCacheWrapper( _Inout_ I** const ppi );

        virtual ~TCacheWrapper();

    public:

        ERR ErrCreate() override;

        ERR ErrMount() override;

        ERR ErrDump( _In_ CPRINTF* const pcprintf ) override;

        ERR ErrGetCacheType( _Out_writes_( cbGuid ) BYTE* const rgbCacheType ) override;

        ERR ErrGetPhysicalId(   _Out_                   VolumeId* const pvolumeid,
                                _Out_                   FileId* const   pfileid,
                                _Out_writes_( cbGuid )  BYTE* const     rgbUniqueId ) override;
                        
        ERR ErrClose(   _In_ const VolumeId     volumeid,
                        _In_ const FileId       fileid,
                        _In_ const FileSerial   fileserial ) override;

        ERR ErrFlush(   _In_ const VolumeId     volumeid,
                        _In_ const FileId       fileid,
                        _In_ const FileSerial   fileserial ) override;

        ERR ErrInvalidate(  _In_ const VolumeId     volumeid,
                            _In_ const FileId       fileid,
                            _In_ const FileSerial   fileserial,
                            _In_ const QWORD        ibOffset,
                            _In_ const QWORD        cbData ) override;

        ERR ErrRead(    _In_                    const TraceContext&         tc,
                        _In_                    const VolumeId              volumeid,
                        _In_                    const FileId                fileid,
                        _In_                    const FileSerial            fileserial,
                        _In_                    const QWORD                 ibOffset,
                        _In_                    const DWORD                 cbData,
                        _Out_writes_( cbData )  BYTE* const                 pbData,
                        _In_                    const OSFILEQOS             grbitQOS,
                        _In_                    const ICache::CachingPolicy cp,
                        _In_opt_                const ICache::PfnComplete   pfnComplete,
                        _In_                    const DWORD_PTR             keyComplete ) override;

        ERR ErrWrite(   _In_                    const TraceContext&         tc,
                        _In_                    const VolumeId              volumeid,
                        _In_                    const FileId                fileid,
                        _In_                    const FileSerial            fileserial,
                        _In_                    const QWORD                 ibOffset,
                        _In_                    const DWORD                 cbData,
                        _In_reads_( cbData )    const BYTE* const           pbData,
                        _In_                    const OSFILEQOS             grbitQOS,
                        _In_                    const ICache::CachingPolicy cp,
                        _In_opt_                const ICache::PfnComplete   pfnComplete,
                        _In_                    const DWORD_PTR             keyComplete ) override;

    protected:

        I* const    m_piInner;
        const BOOL  m_fReleaseOnClose;
};

template< class I >
TCacheWrapper<I>::TCacheWrapper( _In_ I* const pi )
    :   m_piInner( pi ),
        m_fReleaseOnClose( fFalse )
{
}

template< class I >
TCacheWrapper<I>::TCacheWrapper( _Inout_ I** const ppi )
    :   m_piInner( *ppi ),
        m_fReleaseOnClose( fTrue )
{
    *ppi = NULL;
}

template< class I >
TCacheWrapper<I>::~TCacheWrapper()
{
    if ( m_fReleaseOnClose )
    {
        delete m_piInner;
    }
}

template< class I >
ERR TCacheWrapper<I>::ErrCreate()
{
    return m_piInner->ErrCreate();
}

template< class I >
ERR TCacheWrapper<I>::ErrMount()
{
    return m_piInner->ErrMount();
}

template< class I >
ERR TCacheWrapper<I>::ErrDump( _In_ CPRINTF* const pcprintf )
{
    return m_piInner->ErrDump( pcprintf );
}

template< class I >
ERR TCacheWrapper<I>::ErrGetCacheType( _Out_writes_( cbGuid ) BYTE* const rgbCacheType )
{
    return m_piInner->ErrGetCacheType( rgbCacheType );
}


template< class I >
ERR TCacheWrapper<I>::ErrGetPhysicalId( _Out_                   VolumeId* const pvolumeid,
                                        _Out_                   FileId* const   pfileid,
                                        _Out_writes_( cbGuid )  BYTE* const     rgbUniqueId )
{
    return m_piInner->ErrGetPhysicalId( pvolumeid, pfileid, rgbUniqueId );
}
        
template< class I >
ERR TCacheWrapper<I>::ErrClose( _In_ const VolumeId     volumeid,
                                _In_ const FileId       fileid,
                                _In_ const FileSerial   fileserial )
{
    return m_piInner->ErrClose( volumeid, fileid, fileserial );
}
        
template< class I >
ERR TCacheWrapper<I>::ErrFlush( _In_ const VolumeId     volumeid,
                                _In_ const FileId       fileid,
                                _In_ const FileSerial   fileserial )
{
    return m_piInner->ErrFlush( volumeid, fileid, fileserial );
}

template< class I >
ERR TCacheWrapper<I>::ErrInvalidate(    _In_ const VolumeId     volumeid,
                                        _In_ const FileId       fileid,
                                        _In_ const FileSerial   fileserial,
                                        _In_ const QWORD        ibOffset,
                                        _In_ const QWORD        cbData )
{
    return m_piInner->ErrInvalidate( volumeid, fileid, fileserial, ibOffset, cbData );
}

template< class I >
ERR TCacheWrapper<I>::ErrRead(  _In_                    const TraceContext&         tc,
                                _In_                    const VolumeId              volumeid,
                                _In_                    const FileId                fileid,
                                _In_                    const FileSerial            fileserial,
                                _In_                    const QWORD                 ibOffset,
                                _In_                    const DWORD                 cbData,
                                _Out_writes_( cbData )  BYTE* const                 pbData,
                                _In_                    const OSFILEQOS             grbitQOS,
                                _In_                    const ICache::CachingPolicy cp,
                                _In_                    const ICache::PfnComplete   pfnComplete,
                                _In_                    const DWORD_PTR             keyComplete )
{
    return m_piInner->ErrRead( tc, volumeid, fileid, fileserial, ibOffset, cbData, pbData, grbitQOS, cp, pfnComplete, keyComplete );
}

template< class I >
ERR TCacheWrapper<I>::ErrWrite( _In_                    const TraceContext&         tc,
                                _In_                    const VolumeId              volumeid,
                                _In_                    const FileId                fileid,
                                _In_                    const FileSerial            fileserial,
                                _In_                    const QWORD                 ibOffset,
                                _In_                    const DWORD                 cbData,
                                _In_reads_( cbData )    const BYTE* const           pbData,
                                _In_                    const OSFILEQOS             grbitQOS,
                                _In_                    const ICache::CachingPolicy cp,
                                _In_                    const ICache::PfnComplete   pfnComplete,
                                _In_                    const DWORD_PTR             keyComplete )
{
    return m_piInner->ErrWrite( tc, volumeid, fileid, fileserial, ibOffset, cbData, pbData, grbitQOS, cp, pfnComplete, keyComplete );
}


class CCacheWrapper : public TCacheWrapper<ICache>
{
    public:

        CCacheWrapper( _Inout_ ICache** const ppc )
            : TCacheWrapper<ICache>( ppc )
        {
        }

        virtual ~CCacheWrapper() {}
};
