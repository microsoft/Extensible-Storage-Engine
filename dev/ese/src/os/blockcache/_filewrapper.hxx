// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  TFileWrapper:  wrapper of an implementation of IFileAPI or its derivatives.
//
//  This is a utility class used to enable test dependency injection and as the basis for building the file system
//  filter used to implement the block cache.

template< class I >
class TFileWrapper  //  fw
    :   public I
{
    public:  //  specialized API

        TFileWrapper( _In_ I* const pi );
        TFileWrapper( _Inout_ I** const ppi );

        virtual ~TFileWrapper();

    public:  //  IFileAPI

        IFileAPI::FileModeFlags Fmf() const;

        ERR ErrFlushFileBuffers( const IOFLUSHREASON iofr );
        void SetNoFlushNeeded();

        ERR ErrPath( _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) WCHAR* const wszAbsPath );
        ERR ErrSize(    _Out_ QWORD* const              pcbSize,
                        _In_ const IFileAPI::FILESIZE   filesize );
        ERR ErrIsReadOnly( BOOL* const pfReadOnly );

        LONG CLogicalCopies();

        ERR ErrSetSize( const TraceContext& tc,
                        const QWORD         cbSize,
                        const BOOL          fZeroFill,
                        const OSFILEQOS     grbitQOS );

        ERR ErrRename(  const WCHAR* const  wszPathDest,
                        const BOOL          fOverwriteExisting = fFalse );

        ERR ErrSetSparseness();

        ERR ErrIOTrim(  const TraceContext& tc,
                        const QWORD         ibOffset,
                        const QWORD         cbToFree );

        ERR ErrRetrieveAllocatedRegion( const QWORD         ibOffsetToQuery,
                                        _Out_ QWORD* const  pibStartTrimmedRegion,
                                        _Out_ QWORD* const  pcbTrimmed );

        ERR ErrIOSize( DWORD* const pcbSize );

        ERR ErrSectorSize( DWORD* const pcbSize );
 
        ERR ErrReserveIOREQ(    const QWORD     ibOffset,
                                const DWORD     cbData,
                                const OSFILEQOS grbitQOS,
                                VOID **         ppioreq );
        VOID ReleaseUnusedIOREQ( VOID * pioreq );

        ERR ErrIORead(  const TraceContext&                 tc,
                        const QWORD                         ibOffset,
                        const DWORD                         cbData,
                        __out_bcount( cbData ) BYTE* const  pbData,
                        const OSFILEQOS                     grbitQOS,
                        const IFileAPI::PfnIOComplete       pfnIOComplete,
                        const DWORD_PTR                     keyIOComplete,
                        const IFileAPI::PfnIOHandoff        pfnIOHandoff,
                        const VOID *                        pioreq );
        ERR ErrIOWrite( const TraceContext&             tc,
                        const QWORD                     ibOffset,
                        const DWORD                     cbData,
                        const BYTE* const               pbData,
                        const OSFILEQOS                 grbitQOS,
                        const IFileAPI::PfnIOComplete   pfnIOComplete,
                        const DWORD_PTR                 keyIOComplete,
                        const IFileAPI::PfnIOHandoff    pfnIOHandoff );
        ERR ErrIOIssue();

        ERR ErrMMRead(  const QWORD     ibOffset,
                        const QWORD     cbSize,
                        void** const    ppvMap );
        ERR ErrMMCopy(  const QWORD     ibOffset,
                        const QWORD     cbSize,
                        void** const    ppvMap );
        ERR ErrMMIORead(    _In_ const QWORD                        ibOffset,
                            _Out_writes_bytes_(cb) BYTE * const     pb,
                            _In_ ULONG                              cb,
                            _In_ const IFileAPI::FileMmIoReadFlag   fmmiorf );
        ERR ErrMMRevert(    _In_ const QWORD                    ibOffset,
                            _In_reads_bytes_(cbSize)void* const pvMap,
                            _In_ const QWORD                    cbSize );
        ERR ErrMMFree( void* const pvMap );

        VOID RegisterIFilePerfAPI( IFilePerfAPI * const pfpapi );

        VOID UpdateIFilePerfAPIEngineFileTypeId(    _In_ const DWORD    dwEngineFileType,
                                                    _In_ const QWORD    qwEngineFileId );

        ERR ErrNTFSAttributeListSize( QWORD* const pcbSize );

        ERR ErrDiskId( ULONG_PTR* const pulDiskId ) const;

        LONG64 CioNonFlushed() const;

        BOOL FSeekPenalty() const;

#ifdef DEBUG
        DWORD DwEngineFileType() const;
        QWORD QwEngineFileId() const;
#endif

        TICK DtickIOElapsed( void* const pvIOContext ) const;

    protected:

        //  IO completion context for an IFileAPI implementation.

        class CIOComplete
        {
            public:

                CIOComplete(    const BOOL                      fIsHeapAlloc,
                                IFileAPI* const                 pfapi,
                                const QWORD                     ibOffset,
                                const DWORD                     cbData,
                                const BYTE* const               pbData,
                                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                const DWORD_PTR                 keyIOComplete)
                    :   m_fIsHeapAlloc( fIsHeapAlloc ),
                        m_pfapi( pfapi ),
                        m_ibOffset( ibOffset ),
                        m_cbData( cbData ),
                        m_pbData( pbData ),
                        m_pfnIOComplete( pfnIOComplete ),
                        m_pfnIOHandoff( pfnIOHandoff ),
                        m_keyIOComplete( keyIOComplete ),
                        m_pfapiInner( NULL ),
                        m_pvIOContext( NULL ),
                        m_hrtStart( HrtHRTCount() ),
                        m_fIOHandoffCalled( fFalse ),
                        m_fIOCompleteCalled( fFalse ),
                        m_cref( 1 )
                {
                    s_ncrit.Enter();
                    s_il.InsertAsNextMost( this );
                    s_ncrit.Leave();
                }

            protected:

                virtual ~CIOComplete()
                {
                    s_ncrit.Enter();
                    s_il.Remove( this );
                    s_ncrit.Leave();
                }

                virtual void CleanupBeforeAsyncIOCompletion()
                {
                }

            public:

                TICK DtickIOElapsed() const
                { 
                    s_ncrit.Enter();

                    //  determine if this is still a valid io context.  this covers for a design flaw in pfnIOHandoff
                    //  where the interface presumes that the value assigned to pvIOContext will exist forever.  note
                    //  that this same flaw can cause us to accidentally look at a pvIOContext that has already been
                    //  reused
                    //
                    //  NOTE:  we cannot use "s_il.FMember( this )" here because it could touch freed memory

                    CIOComplete* piocompleteT = NULL;
                    for (   piocompleteT = s_il.PrevMost();
                            piocompleteT != NULL && piocompleteT != this;
                            piocompleteT = s_il.Next( piocompleteT ) )
                    {
                    }

                    //  get the elapsed time only if this is still a valid io context

                    TICK dtick = 0;
                    
                    if ( piocompleteT == this )
                    {
                        //  if the pvIOContext is null then we are the source of the information, otherwise call down
                        //  to the inner IFileAPI implementation

                        if ( !m_pvIOContext )
                        {
                            dtick = (DWORD)min( lMax, CmsecHRTFromDhrt( m_hrtStart ) );
                        }
                        else
                        {
                            dtick = m_pfapiInner->DtickIOElapsed( m_pvIOContext );
                        }
                    }

                    s_ncrit.Leave();

                    return dtick;
                }

                VOID Release(   _In_ const ERR              err,
                                _In_ const TraceContext&    tc,
                                _In_ const OSFILEQOS        grbitQOS )
                {
                    //  make sure we have signaled a handoff if this was a success

                    if ( err >= JET_errSuccess )
                    {
                        IOHandoff( err, tc, grbitQOS );
                    }

                    //  if this context is for a sync IO with a successful handoff then make sure we complete

                    if ( !m_pfnIOComplete && m_fIOHandoffCalled )
                    {
                        IOComplete( err, tc, grbitQOS );
                    }

                    //  if this context is on the stack then we had better only have one reference left now

                    Assert( m_fIsHeapAlloc || m_cref == 1 );

                    //  release our initial reference on the context

                    ReleaseInternal();
                }

                void IOComplete(    _In_ const ERR              err,
                                    _In_ const TraceContext&    tc,
                                    _In_ const OSFILEQOS        grbitQOS )
                {
                    if ( FIOComplete() )
                    {
                        FullTraceContext ftc;
                        if ( m_pfnIOComplete )
                        {
                            ftc.DeepCopy( GetCurrUserTraceContext().Utc(), tc );
                        }

                        IOComplete( err, ftc, grbitQOS );
                    }
                }

                void IOComplete(    _In_ const ERR                  err,
                                    _In_ const FullTraceContext&    tc,
                                    _In_ const OSFILEQOS            grbitQOS )
                {
                    if ( FIOHandoff() )
                    {
                        IOHandoff( err, tc, grbitQOS );
                    }

                    if ( FIOComplete() )
                    {
                        if ( m_pfnIOComplete )
                        {
                            CleanupBeforeAsyncIOCompletion();

                            m_pfnIOComplete(   err,
                                               m_pfapi,
                                               tc,
                                               grbitQOS,
                                               m_ibOffset,
                                               m_cbData,
                                               m_pbData,
                                               m_keyIOComplete );
                        }

                        m_fIOCompleteCalled = fTrue;

                        ReleaseInternal();
                    }
                }

                static void IOComplete_(    _In_                    const ERR               err,
                                            _In_                    IFileAPI* const         pfapi,
                                            _In_                    const FullTraceContext& tc,
                                            _In_                    const OSFILEQOS         grbitQOS,
                                            _In_                    const QWORD             ibOffset,
                                            _In_                    const DWORD             cbData,
                                            _In_reads_( cbData )    const BYTE* const       pbData,
                                            _In_                    const DWORD_PTR         keyIOComplete )
                {
                    IFileAPI::PfnIOComplete pfnIOComplete = IOComplete_;
                    Unused( pfnIOComplete );

                    CIOComplete* const piocomplete = (CIOComplete*)keyIOComplete;
                    piocomplete->IOComplete( err, tc, grbitQOS );
                }
                
                void IOHandoff( _In_ const ERR              err,
                                _In_ const TraceContext&    tc,
                                _In_ const OSFILEQOS        grbitQOS )
                {
                    if ( FIOHandoff() )
                    {
                        FullTraceContext ftc;
                        if ( m_pfnIOHandoff )
                        {
                            ftc.DeepCopy( GetCurrUserTraceContext().Utc(), tc );
                        }
                        IOHandoff( err, ftc, grbitQOS );
                    }
                }
                
                void IOHandoff( _In_ const ERR                  err,
                                _In_ const FullTraceContext&    tc,
                                _In_ const OSFILEQOS            grbitQOS )
                {
                    if ( FIOHandoff() )
                    {
                        if ( m_pfnIOHandoff )
                        {
                            m_pfnIOHandoff( err,
                                            m_pfapi,
                                            tc,
                                            grbitQOS,
                                            m_ibOffset,
                                            m_cbData,
                                            m_pbData,
                                            m_keyIOComplete,
                                            this );
                        }

                        m_fIOHandoffCalled = fTrue;

                        AddRef();
                    }
                }

                static void IOHandoff_( _In_                    const ERR               err,
                                        _In_                    IFileAPI* const         pfapi,
                                        _In_                    const FullTraceContext& tc,
                                        _In_                    const OSFILEQOS         grbitQOS,
                                        _In_                    const QWORD             ibOffset,
                                        _In_                    const DWORD             cbData,
                                        _In_reads_( cbData )    const BYTE* const       pbData,
                                        _In_                    const DWORD_PTR         keyIOComplete,
                                        _In_                    void* const             pvIOContext )
                {
                    IFileAPI::PfnIOHandoff pfnIOHandoff = IOHandoff_;
                    Unused( pfnIOHandoff );

                    CIOComplete* const piocomplete = (CIOComplete*)keyIOComplete;
                    piocomplete->m_pfapiInner = pfapi;
                    piocomplete->m_pvIOContext = pvIOContext;
                    piocomplete->IOHandoff( err, tc, grbitQOS );
                }

            private:

                CIOComplete( const CIOComplete& other );

                static SIZE_T OffsetOfILE() { return OffsetOf( CIOComplete, m_ile ); }

                BOOL FIOHandoff() const { return !m_fIOHandoffCalled; }
                BOOL FIOComplete() const { return !m_fIOCompleteCalled; }

                void AddRef() { AtomicIncrement( (LONG*)&m_cref ); }
                BOOL FRelease() { return AtomicDecrement( (LONG*)&m_cref ) == 0; }

                void ReleaseInternal()
                {
                    if ( FRelease() )
                    {
                        if ( m_fIsHeapAlloc )
                        {
                            delete this;
                        }
                        else
                        {
                            this->~CIOComplete();
                        }
                    }
                }

            private:

                static CNestableCriticalSection                             s_ncrit;
                static CInvasiveList<CIOComplete, CIOComplete::OffsetOfILE> s_il;

                typename CInvasiveList<CIOComplete, OffsetOfILE>::CElement  m_ile;
                const BOOL                                                  m_fIsHeapAlloc;
                IFileAPI* const                                             m_pfapi;
                const QWORD                                                 m_ibOffset;
                const DWORD                                                 m_cbData;
                const BYTE* const                                           m_pbData;
                const IFileAPI::PfnIOComplete                               m_pfnIOComplete;
                const IFileAPI::PfnIOHandoff                                m_pfnIOHandoff;
                const DWORD_PTR                                             m_keyIOComplete;
                IFileAPI*                                                   m_pfapiInner;
                void*                                                       m_pvIOContext;
                const HRT                                                   m_hrtStart;
                BOOL                                                        m_fIOHandoffCalled;
                BOOL                                                        m_fIOCompleteCalled;
                int                                                         m_cref;
        };

    protected:

        ERR HandleReservedIOREQ(    _In_                    const TraceContext&             tc,
                                    _In_                    const QWORD                     ibOffset,
                                    _In_                    const DWORD                     cbData,
                                    _Out_writes_( cbData )  BYTE* const                     pbData,
                                    _In_                    const OSFILEQOS                 grbitQOS,
                                    _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                                    _In_                    const DWORD_PTR                 keyIOComplete,
                                    _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                    _In_                    const VOID *                    pioreq,
                                    _In_                    const BOOL                      fIOREQUsed,
                                    _In_                    const ERR                       errOriginal,
                                    _In_                    CIOComplete* const              piocompleteOriginal = NULL );

    protected:

        I* const    m_piInner;
        const BOOL  m_fReleaseOnClose;
};

template< class I >
CNestableCriticalSection TFileWrapper<I>::CIOComplete::s_ncrit( CLockBasicInfo( CSyncBasicInfo( "TFileWrapper<I>::CIOComplete::s_ncrit" ), rankFileWrapperIOComplete, 0 ) );

template< class I >
CInvasiveList<typename TFileWrapper<I>::CIOComplete, TFileWrapper<I>::CIOComplete::OffsetOfILE> TFileWrapper<I>::CIOComplete::s_il;

template< class I >
TFileWrapper<I>::TFileWrapper( _In_ I* const pi )
    :   m_piInner( pi ),
        m_fReleaseOnClose( fFalse )
{
}

template< class I >
TFileWrapper<I>::TFileWrapper( _Inout_ I** const ppi )
    :   m_piInner( *ppi ),
        m_fReleaseOnClose( fTrue )
{
    *ppi = NULL;
}

template< class I >
TFileWrapper<I>::~TFileWrapper()
{
    if ( m_fReleaseOnClose )
    {
        delete m_piInner;
    }
}

template< class I >
IFileAPI::FileModeFlags TFileWrapper<I>::Fmf() const
{
    return m_piInner->Fmf();
}

template< class I >
ERR TFileWrapper<I>::ErrFlushFileBuffers( const IOFLUSHREASON iofr )
{
    return m_piInner->ErrFlushFileBuffers( iofr );
}

template< class I >
void TFileWrapper<I>::SetNoFlushNeeded()
{
    m_piInner->SetNoFlushNeeded();
}

template< class I >
ERR TFileWrapper<I>::ErrPath( _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) WCHAR* const wszAbsPath )
{
    return m_piInner->ErrPath( wszAbsPath );
}

template< class I >
ERR TFileWrapper<I>::ErrSize(   _Out_ QWORD* const              pcbSize,
                                _In_ const IFileAPI::FILESIZE   filesize )
{
    return m_piInner->ErrSize( pcbSize, filesize );
}

template< class I >
ERR TFileWrapper<I>::ErrIsReadOnly( BOOL* const pfReadOnly )
{
    return m_piInner->ErrIsReadOnly( pfReadOnly );
}

template< class I >
LONG TFileWrapper<I>::CLogicalCopies()
{
    return m_piInner->CLogicalCopies();
}

template< class I >
ERR TFileWrapper<I>::ErrSetSize(    const TraceContext& tc,
                                    const QWORD         cbSize,
                                    const BOOL          fZeroFill,
                                    const OSFILEQOS     grbitQOS )
{
    return m_piInner->ErrSetSize( tc, cbSize, fZeroFill, grbitQOS );
}

template< class I >
ERR TFileWrapper<I>::ErrRename( const WCHAR* const  wszPathDest,
                                const BOOL          fOverwriteExisting )
{
    return m_piInner->ErrRename( wszPathDest, fOverwriteExisting );
}

template< class I >
ERR TFileWrapper<I>::ErrSetSparseness()
{
    return m_piInner->ErrSetSparseness();
}

template< class I >
ERR TFileWrapper<I>::ErrIOTrim( const TraceContext& tc,
                                const QWORD         ibOffset,
                                const QWORD         cbToFree )
{
    return m_piInner->ErrIOTrim( tc, ibOffset, cbToFree );
}

template< class I >
ERR TFileWrapper<I>::ErrRetrieveAllocatedRegion(    const QWORD         ibOffsetToQuery,
                                                    _Out_ QWORD* const  pibStartTrimmedRegion,
                                                    _Out_ QWORD* const  pcbTrimmed )
{
    return m_piInner->ErrRetrieveAllocatedRegion( ibOffsetToQuery, pibStartTrimmedRegion, pcbTrimmed );
}

template< class I >
ERR TFileWrapper<I>::ErrIOSize( DWORD* const pcbSize )
{
    return m_piInner->ErrIOSize( pcbSize );
}

template< class I >
ERR TFileWrapper<I>::ErrSectorSize( DWORD* const pcbSize )
{
    return m_piInner->ErrSectorSize( pcbSize );
}
 
template< class I >
ERR TFileWrapper<I>::ErrReserveIOREQ(   const QWORD     ibOffset,
                                        const DWORD     cbData,
                                        const OSFILEQOS grbitQOS,
                                        VOID **         ppioreq )
{
    return m_piInner->ErrReserveIOREQ( ibOffset, cbData, grbitQOS, ppioreq );
}

template< class I >
VOID TFileWrapper<I>::ReleaseUnusedIOREQ( VOID * pioreq )
{
    return m_piInner->ReleaseUnusedIOREQ( pioreq );
}

template< class I >
ERR TFileWrapper<I>::ErrIORead( const TraceContext&                 tc,
                                const QWORD                         ibOffset,
                                const DWORD                         cbData,
                                __out_bcount( cbData ) BYTE* const  pbData,
                                const OSFILEQOS                     grbitQOS,
                                const IFileAPI::PfnIOComplete       pfnIOComplete,
                                const DWORD_PTR                     keyIOComplete,
                                const IFileAPI::PfnIOHandoff        pfnIOHandoff,
                                const VOID *                        pioreq )
{
    ERR             err         = JET_errSuccess;
    BOOL            fIOREQUsed  = fFalse;
    CIOComplete*    piocomplete = NULL;

    if ( pfnIOComplete || pfnIOHandoff )
    {
        if ( pfnIOComplete )
        {
            Alloc( piocomplete = new CIOComplete(   fTrue,
                                                    this,
                                                    ibOffset,
                                                    cbData, 
                                                    pbData,
                                                    pfnIOComplete,
                                                    pfnIOHandoff,
                                                    keyIOComplete ) );
        }
        else
        {
            piocomplete = new( _alloca( sizeof( CIOComplete ) ) ) CIOComplete(  fFalse,
                                                                                this,
                                                                                ibOffset,
                                                                                cbData, 
                                                                                pbData, 
                                                                                pfnIOComplete,
                                                                                pfnIOHandoff, 
                                                                                keyIOComplete );
        }
    }

    fIOREQUsed = fTrue;
    Call( m_piInner->ErrIORead( tc,
                                ibOffset,
                                cbData,
                                pbData,
                                grbitQOS,
                                pfnIOComplete ? (IFileAPI::PfnIOComplete)CIOComplete::IOComplete_ : NULL,
                                DWORD_PTR( piocomplete ),
                                piocomplete ? (IFileAPI::PfnIOHandoff)CIOComplete::IOHandoff_ : NULL,
                                pioreq ) );

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
                                fIOREQUsed,
                                err,
                                piocomplete );
    if ( piocomplete )
    {
        piocomplete->Release( err, tc, grbitQOS );
    }
    return err;
}

template< class I >
ERR TFileWrapper<I>::ErrIOWrite(    const TraceContext&             tc,
                                    const QWORD                     ibOffset,
                                    const DWORD                     cbData,
                                    const BYTE* const               pbData,
                                    const OSFILEQOS                 grbitQOS,
                                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                                    const DWORD_PTR                 keyIOComplete,
                                    const IFileAPI::PfnIOHandoff    pfnIOHandoff )
{
    ERR             err         = JET_errSuccess;
    CIOComplete*    piocomplete = NULL;

    if ( pfnIOComplete || pfnIOHandoff )
    {
        if ( pfnIOComplete )
        {
            Alloc( piocomplete = new CIOComplete(   fTrue,
                                                    this, 
                                                    ibOffset, 
                                                    cbData,
                                                    pbData, 
                                                    pfnIOComplete, 
                                                    pfnIOHandoff,
                                                    keyIOComplete ) );
        }
        else
        {
            piocomplete = new( _alloca( sizeof( CIOComplete ) ) ) CIOComplete(  fFalse, 
                                                                                this, 
                                                                                ibOffset,
                                                                                cbData,
                                                                                pbData,
                                                                                pfnIOComplete, 
                                                                                pfnIOHandoff, 
                                                                                keyIOComplete );
        }
    }

    Call( m_piInner->ErrIOWrite(    tc,
                                    ibOffset, 
                                    cbData,
                                    pbData,
                                    grbitQOS, 
                                    pfnIOComplete ? (IFileAPI::PfnIOComplete)CIOComplete::IOComplete_ : NULL,
                                    DWORD_PTR( piocomplete ),
                                    piocomplete ? (IFileAPI::PfnIOHandoff)CIOComplete::IOHandoff_ : NULL ) );

HandleError:
    if ( piocomplete )
    {
        piocomplete->Release( err, tc, grbitQOS );
    }
    return err;
}

template< class I >
ERR TFileWrapper<I>::ErrIOIssue()
{
    return m_piInner->ErrIOIssue();
}

template< class I >
ERR TFileWrapper<I>::ErrMMRead( const QWORD     ibOffset,
                                const QWORD     cbSize,
                                void** const    ppvMap )
{
    return m_piInner->ErrMMRead( ibOffset, cbSize, ppvMap );
}

template< class I >
ERR TFileWrapper<I>::ErrMMCopy( const QWORD     ibOffset,
                                const QWORD     cbSize,
                                void** const    ppvMap )
{
    return m_piInner->ErrMMCopy( ibOffset, cbSize, ppvMap );
}

template< class I >
ERR TFileWrapper<I>::ErrMMIORead(   _In_ const QWORD                        ibOffset,
                                    _Out_writes_bytes_(cb) BYTE * const     pb,
                                    _In_ ULONG                              cb,
                                    _In_ const IFileAPI::FileMmIoReadFlag   fmmiorf )
{
    return m_piInner->ErrMMIORead( ibOffset, pb, cb, fmmiorf );
}

template< class I >
ERR TFileWrapper<I>::ErrMMRevert(   _In_ const QWORD                    ibOffset,
                                    _In_reads_bytes_(cbSize)void* const pvMap,
                                    _In_ const QWORD                    cbSize )
{
    return m_piInner->ErrMMRevert( ibOffset, pvMap, cbSize );
}

template< class I >
ERR TFileWrapper<I>::ErrMMFree( void* const pvMap )
{
    return m_piInner->ErrMMFree( pvMap );
}

template< class I >
VOID TFileWrapper<I>::RegisterIFilePerfAPI( IFilePerfAPI * const pfpapi )
{
    return m_piInner->RegisterIFilePerfAPI( pfpapi );
}

template< class I >
VOID TFileWrapper<I>::UpdateIFilePerfAPIEngineFileTypeId(   _In_ const DWORD    dwEngineFileType,
                                                            _In_ const QWORD    qwEngineFileId )
{
    return m_piInner->UpdateIFilePerfAPIEngineFileTypeId( dwEngineFileType, qwEngineFileId );
}

template< class I >
ERR TFileWrapper<I>::ErrNTFSAttributeListSize( QWORD* const pcbSize )
{
    return m_piInner->ErrNTFSAttributeListSize( pcbSize );
}

template< class I >
ERR TFileWrapper<I>::ErrDiskId( ULONG_PTR* const pulDiskId ) const
{
    return m_piInner->ErrDiskId( pulDiskId );
}

template< class I >
LONG64 TFileWrapper<I>::CioNonFlushed() const
{
    return m_piInner->CioNonFlushed();
}

template< class I >
BOOL TFileWrapper<I>::FSeekPenalty() const
{
    return m_piInner->FSeekPenalty();
}

#ifdef DEBUG
template< class I >
DWORD TFileWrapper<I>::DwEngineFileType() const
{
    return m_piInner->DwEngineFileType();
}

template< class I >
QWORD TFileWrapper<I>::QwEngineFileId() const
{
    return m_piInner->QwEngineFileId();
}
#endif

template< class I >
TICK TFileWrapper<I>::DtickIOElapsed( void* const pvIOContext ) const
{
    CIOComplete* const piocomplete = (CIOComplete*)pvIOContext;
    return piocomplete->DtickIOElapsed();
}

template<class I>
ERR TFileWrapper<I>::HandleReservedIOREQ(   _In_                    const TraceContext&             tc,
                                            _In_                    const QWORD                     ibOffset,
                                            _In_                    const DWORD                     cbData,
                                            _Out_writes_( cbData )  BYTE* const                     pbData,
                                            _In_                    const OSFILEQOS                 grbitQOS,
                                            _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                                            _In_                    const DWORD_PTR                 keyIOComplete,
                                            _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                            _In_                    const VOID *                    pioreq,
                                            _In_                    const BOOL                      fIOREQUsed,
                                            _In_                    const ERR                       errOriginal,
                                            _In_                    CIOComplete* const              piocompleteOriginal )
{
    ERR             err         = errOriginal;
    CIOComplete*    piocomplete = piocompleteOriginal;

    //  as for the COSFile::ErrIORead impl, you cannot pass a pioreq for a sync read

    Enforce( pfnIOComplete || !pioreq );

    if ( pioreq && !fIOREQUsed )
    {
        //  if we were given a reserved IOREQ and we failed before it could be passed on then we must release it

        ReleaseUnusedIOREQ( (void*)pioreq );

        //  if we were given a reserved IOREQ then it is expected that the IO request will succeed.  we must instead
        //  indicate a failure by accepting the IO and then by completing it with an error.  this IO completion cannot
        //  accept transient errors like errDiskTilt (which we shouldn't see anyway) or JET_errOutOfMemory.  we will
        //  transform these errors into a fatal IO error (JET_errDiskIO)

        Enforce( err != errDiskTilt );

        err = err == JET_errOutOfMemory ? ErrERRCheck( JET_errDiskIO ) : err;

        if ( !piocompleteOriginal )
        {
            piocomplete = new( _alloca( sizeof( CIOComplete ) ) ) CIOComplete(  fFalse,
                                                                                this,
                                                                                ibOffset,
                                                                                cbData,
                                                                                pbData,
                                                                                pfnIOComplete,
                                                                                pfnIOHandoff,
                                                                                keyIOComplete );
        }

        piocomplete->IOComplete( err, tc, grbitQOS );

        if ( !piocompleteOriginal )
        {
            piocomplete->Release( err, tc, grbitQOS );
        }

        err = JET_errSuccess;
    }

    return err;
}

//  CFileWrapper:  concrete TFileWrapper<IFileAPI>.

class CFileWrapper : public TFileWrapper<IFileAPI>
{
    public:  //  specialized API

        CFileWrapper( _Inout_ IFileAPI** const ppfapi )
            :   TFileWrapper<IFileAPI>( ppfapi )
        {
        }

        virtual ~CFileWrapper() {}
};
