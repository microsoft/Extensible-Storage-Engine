// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  TFileFilterWrapper:  wrapper of an implementation of IFileFilter or its derivatives.

template< class I >
class TFileFilterWrapper  //  cff
    :   public TFileWrapper<I>
{
    public:  //  specialized API

        TFileFilterWrapper( _Inout_ I** const ppi, _In_ const IFileFilter::IOMode iom )
            :   TFileWrapper<I>( ppi ),
                m_iom( iom )
        {
        }

        TFileFilterWrapper( _In_ I* const pi, _In_ const IFileFilter::IOMode iom )
            :   TFileWrapper<I>( pi ),
                m_iom( iom )
        {
        }

    public:  //  IFileAPI

        ERR ErrIORead(  _In_                    const TraceContext&             tc,
                        _In_                    const QWORD                     ibOffset,
                        _In_                    const DWORD                     cbData,
                        _Out_writes_( cbData )  BYTE* const                     pbData,
                        _In_                    const OSFILEQOS                 grbitQOS,
                        _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                        _In_opt_                const DWORD_PTR                 keyIOComplete,
                        _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                        _In_opt_                const VOID *                    pioreq ) override;
        ERR ErrIOWrite( _In_                    const TraceContext&             tc,
                        _In_                    const QWORD                     ibOffset,
                        _In_                    const DWORD                     cbData,
                        _In_reads_( cbData )    const BYTE* const               pbData,
                        _In_                    const OSFILEQOS                 grbitQOS,
                        _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                        _In_opt_                const DWORD_PTR                 keyIOComplete,
                        _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff ) override;
        ERR ErrIOIssue() override;
        ERR ErrFlushFileBuffers( _In_ const IOFLUSHREASON iofr ) override;

    public:  //  IFileFilter

        ERR ErrGetPhysicalId(   _Out_ VolumeId* const   pvolumeid,
                                _Out_ FileId* const     pfileid,
                                _Out_ FileSerial* const pfileserial ) override;

        ERR ErrRead(    _In_                    const TraceContext&             tc,
                        _In_                    const QWORD                     ibOffset,
                        _In_                    const DWORD                     cbData,
                        _Out_writes_( cbData )  BYTE* const                     pbData,
                        _In_                    const OSFILEQOS                 grbitQOS,
                        _In_                    const IFileFilter::IOMode       iom,
                        _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                        _In_opt_                const DWORD_PTR                 keyIOComplete,
                        _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                        _In_opt_                const VOID *                    pioreq ) override;
        ERR ErrWrite(   _In_                    const TraceContext&             tc,
                        _In_                    const QWORD                     ibOffset,
                        _In_                    const DWORD                     cbData,
                        _In_reads_( cbData )    const BYTE* const               pbData,
                        _In_                    const OSFILEQOS                 grbitQOS,
                        _In_                    const IFileFilter::IOMode       iom,
                        _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                        _In_opt_                const DWORD_PTR                 keyIOComplete,
                        _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff ) override;
        ERR ErrIssue( _In_ const IFileFilter::IOMode iom ) override;
        ERR ErrFlush( _In_ const IOFLUSHREASON iofr, _In_ const IFileFilter::IOMode iom ) override;

    private:

        const IFileFilter::IOMode m_iom;
};

template< class I >
ERR TFileFilterWrapper<I>::ErrGetPhysicalId(    _Out_ VolumeId* const   pvolumeid,
                                                _Out_ FileId* const     pfileid,
                                                _Out_ FileSerial* const pfileserial )
{
    return m_piInner->ErrGetPhysicalId( pvolumeid, pfileid, pfileserial );
}

template< class I >
ERR TFileFilterWrapper<I>::ErrIORead(   _In_                    const TraceContext&                 tc,
                                        _In_                    const QWORD                         ibOffset,
                                        _In_                    const DWORD                         cbData,
                                        _Out_writes_( cbData )  __out_bcount( cbData ) BYTE* const  pbData,
                                        _In_                    const OSFILEQOS                     grbitQOS,
                                        _In_opt_                const IFileAPI::PfnIOComplete       pfnIOComplete,
                                        _In_opt_                const DWORD_PTR                     keyIOComplete,
                                        _In_opt_                const IFileAPI::PfnIOHandoff        pfnIOHandoff,
                                        _In_opt_                const VOID *                        pioreq )
{
    ERR             err         = JET_errSuccess;
    CIOComplete*    piocomplete = NULL;

    if ( pfnIOComplete || pfnIOHandoff )
    {
        const BOOL fHeap = pfnIOComplete != NULL;
        Alloc( piocomplete = new( fHeap ? new Buffer<CIOComplete>() : _malloca( sizeof( CIOComplete ) ) )
            CIOComplete(    fHeap,
                            this,
                            ibOffset,
                            cbData, 
                            pbData, 
                            pfnIOComplete,
                            pfnIOHandoff, 
                            keyIOComplete ) );
    }

    err = m_piInner->ErrIORead( tc,
                                ibOffset,
                                cbData,
                                pbData,
                                grbitQOS,
                                pfnIOComplete ? CIOComplete::IOComplete_ : NULL,
                                DWORD_PTR( piocomplete ),
                                piocomplete ? CIOComplete::IOHandoff_ : NULL,
                                pioreq );
    pioreq = NULL;
    Call( err );

HandleError:
    err = HandleReservedIOREQ(  tc, 
                                ibOffset, 
                                cbData, 
                                pbData,
                                grbitQOS,
                                pfnIOComplete,
                                keyIOComplete, 
                                pfnIOHandoff,
                                pioreq,
                                err,
                                piocomplete );
    if ( piocomplete )
    {
        piocomplete->Release( err, tc, grbitQOS );
    }
    return err;
}

template< class I >
ERR TFileFilterWrapper<I>::ErrIOWrite(  _In_                    const TraceContext&             tc,
                                        _In_                    const QWORD                     ibOffset,
                                        _In_                    const DWORD                     cbData,
                                        _In_reads_( cbData )    const BYTE* const               pbData,
                                        _In_                    const OSFILEQOS                 grbitQOS,
                                        _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                        _In_opt_                const DWORD_PTR                 keyIOComplete,
                                        _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff )
{
    ERR             err         = JET_errSuccess;
    CIOComplete*    piocomplete = NULL;

    if ( pfnIOComplete || pfnIOHandoff )
    {
        const BOOL fHeap = pfnIOComplete != NULL;
        Alloc( piocomplete = new( fHeap ? new Buffer<CIOComplete>() : _malloca( sizeof( CIOComplete ) ) )
            CIOComplete(    fHeap,
                            this, 
                            ibOffset,
                            cbData,
                            pbData,
                            pfnIOComplete, 
                            pfnIOHandoff, 
                            keyIOComplete ) );
    }

    Call( m_piInner->ErrIOWrite(    tc,
                                    ibOffset, 
                                    cbData,
                                    pbData,
                                    grbitQOS, 
                                    pfnIOComplete ? CIOComplete::IOComplete_ : NULL,
                                    DWORD_PTR( piocomplete ),
                                    piocomplete ? CIOComplete::IOHandoff_ : NULL ) );

HandleError:
    if ( piocomplete )
    {
        piocomplete->Release( err, tc, grbitQOS );
    }
    return err;
}

template< class I >
ERR TFileFilterWrapper<I>::ErrIOIssue()
{
    return ErrIssue( m_iom );
}

template< class I >
ERR TFileFilterWrapper<I>::ErrFlushFileBuffers( _In_ const IOFLUSHREASON iofr )
{
    return m_piInner->ErrFlushFileBuffers( iofr );
}

template< class I >
ERR TFileFilterWrapper<I>::ErrRead( _In_                    const TraceContext&             tc,
                                    _In_                    const QWORD                     ibOffset,
                                    _In_                    const DWORD                     cbData,
                                    _Out_writes_( cbData )  BYTE* const                     pbData,
                                    _In_                    const OSFILEQOS                 grbitQOS,
                                    _In_                    const IFileFilter::IOMode       iom,
                                    _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                    _In_opt_                const DWORD_PTR                 keyIOComplete,
                                    _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                    _In_opt_                const VOID *                    pioreq )
{
    ERR             err         = JET_errSuccess;
    CIOComplete*    piocomplete = NULL;

    if ( pfnIOComplete || pfnIOHandoff )
    {
        const BOOL fHeap = pfnIOComplete != NULL;
        Alloc( piocomplete = new( fHeap ? new Buffer<CIOComplete>() : _malloca( sizeof( CIOComplete ) ) )
            CIOComplete(    fHeap,
                            this,
                            ibOffset,
                            cbData, 
                            pbData, 
                            pfnIOComplete,
                            pfnIOHandoff, 
                            keyIOComplete ) );
    }

    err = m_piInner->ErrRead(   tc,
                                ibOffset,
                                cbData,
                                pbData,
                                grbitQOS,
                                iom,
                                pfnIOComplete ? CIOComplete::IOComplete_ : NULL,
                                DWORD_PTR( piocomplete ),
                                piocomplete ? CIOComplete::IOHandoff_ : NULL,
                                pioreq );
    pioreq = NULL;
    Call( err );

HandleError:
    err = HandleReservedIOREQ(  tc, 
                                ibOffset, 
                                cbData, 
                                pbData,
                                grbitQOS,
                                pfnIOComplete,
                                keyIOComplete, 
                                pfnIOHandoff,
                                pioreq,
                                err,
                                piocomplete );
    if ( piocomplete )
    {
        piocomplete->Release( err, tc, grbitQOS );
    }
    return err;
}

template< class I >
ERR TFileFilterWrapper<I>::ErrWrite(    _In_                    const TraceContext&             tc,
                                        _In_                    const QWORD                     ibOffset,
                                        _In_                    const DWORD                     cbData,
                                        _In_reads_( cbData )    const BYTE* const               pbData,
                                        _In_                    const OSFILEQOS                 grbitQOS,
                                        _In_                    const IFileFilter::IOMode       iom,
                                        _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                        _In_opt_                const DWORD_PTR                 keyIOComplete,
                                        _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff )
{
    ERR             err         = JET_errSuccess;
    CIOComplete*    piocomplete = NULL;

    if ( pfnIOComplete || pfnIOHandoff )
    {
        const BOOL fHeap = pfnIOComplete != NULL;
        Alloc( piocomplete = new( fHeap ? new Buffer<CIOComplete>() : _malloca( sizeof( CIOComplete ) ) )
            CIOComplete(    fHeap,
                            this, 
                            ibOffset,
                            cbData,
                            pbData,
                            pfnIOComplete, 
                            pfnIOHandoff, 
                            keyIOComplete ) );
    }

    Call( m_piInner->ErrWrite(  tc,
                                ibOffset, 
                                cbData,
                                pbData,
                                grbitQOS, 
                                iom,
                                pfnIOComplete ? CIOComplete::IOComplete_ : NULL,
                                DWORD_PTR( piocomplete ),
                                piocomplete ? CIOComplete::IOHandoff_ : NULL ) );

HandleError:
    if ( piocomplete )
    {
        piocomplete->Release( err, tc, grbitQOS );
    }
    return err;
}

template< class I >
ERR TFileFilterWrapper<I>::ErrIssue( _In_ const IFileFilter::IOMode iom )
{
    return m_piInner->ErrIssue( iom );
}

template< class I >
ERR TFileFilterWrapper<I>::ErrFlush( _In_ const IOFLUSHREASON iofr, _In_ const IFileFilter::IOMode  iom )
{
    return m_piInner->ErrFlush( iofr, iom );
}

//  CFileFilterWrapper:  concrete TFileFilterWrapper<IFileFilter>.

class CFileFilterWrapper : public TFileFilterWrapper<IFileFilter>
{
    public:  //  specialized API

        CFileFilterWrapper( _Inout_ IFileFilter** const ppff, _In_ const IFileFilter::IOMode iom )
            :   TFileFilterWrapper<IFileFilter>( ppff, iom )
        {
        }

        CFileFilterWrapper( _In_ IFileFilter* const pff, _In_ const IFileFilter::IOMode iom )
            :   TFileFilterWrapper<IFileFilter>( pff, iom )
        {
        }
};


