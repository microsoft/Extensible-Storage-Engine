// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once


//  TCacheBase:  base implementation of ICache and its derivatives.
//
//  I:  ICache or derivative
//  CFTE:  CCachedFileTableEntryBase or derivative

template< class I, class CFTE >
class TCacheBase  //  c
    :   public I
{
    public:

        virtual ~TCacheBase();

    public:  //  ICache

        ERR ErrGetCacheType( _Out_writes_( cbGuid ) BYTE* const rgbCacheType ) override;

        ERR ErrGetPhysicalId(   _Out_                   VolumeId* const pvolumeid,
                                _Out_                   FileId* const   pfileid,
                                _Out_writes_( cbGuid )  BYTE* const     rgbUniqueId ) override;
        
        ERR ErrClose(   _In_ const VolumeId     volumeid,
                        _In_ const FileId       fileid,
                        _In_ const FileSerial   fileserial ) override;

    protected:
        
        TCacheBase( _In_    IFileSystemFilter* const            pfsf,
                    _In_    IFileIdentification* const          pfident,
                    _In_    IFileSystemConfiguration* const     pfsconfig, 
                    _Inout_ IBlockCacheConfiguration** const    ppbcconfig,
                    _Inout_ ICacheConfiguration** const         ppcconfig,
                    _In_    ICacheTelemetry* const              pctm,
                    _Inout_ IFileFilter** const                 ppffCaching,
                    _Inout_ CCacheHeader** const                ppch );

        IFileSystemFilter* Pfsf() const { return m_pfsf; }
        IFileIdentification* Pfident() const { return m_pfident; }
        IFileSystemConfiguration* Pfsconfig() const { return m_pfsconfig; }
        IBlockCacheConfiguration* Pbcconfig() const { return m_pbcconfig; }
        ICacheConfiguration* Pcconfig() const { return m_pcconfig; }
        IFileFilter* PffCaching() const { return m_pffCaching; }
        
        ERR ErrGetCachedFile(   _In_    const VolumeId      volumeid,
                                _In_    const FileId        fileid,
                                _In_    const FileSerial    fileserial,
                                _In_    const BOOL          fOnlyIfAlreadyOpen,
                                _Out_   CFTE** const        ppcfte );
        VOID ReleaseCachedFile( _In_opt_ CFTE* const        pcfte,
                                _In_opt_ CSemaphore* const  psem    = NULL);

        //  Reports a cache miss for a file/block and if that block should be cached.

        void ReportMiss(    _In_ CFTE* const    pcfte,
                            _In_ const QWORD    ibOffset,
                            _In_ const DWORD    cbData,
                            _In_ const BOOL     fRead,
                            _In_ const BOOL     fCacheIfPossible );

        //  Reports a cache hit for a file/block and if that block should be cached.

        void ReportHit( _In_ CFTE* const    pcfte,
                        _In_ const QWORD    ibOffset,
                        _In_ const DWORD    cbData,
                        _In_ const BOOL     fRead,
                        _In_ const BOOL     fCacheIfPossible );

        //  Reports an update of a file/block.

        void ReportUpdate(  _In_ CFTE* const    pcfte,
                            _In_ const QWORD    ibOffset,
                            _In_ const DWORD    cbData );

        //  Reports a write of a file/block and if that write was due to limited resources.

        void ReportWrite(   _In_ CFTE* const    pcfte,
                            _In_ const QWORD    ibOffset,
                            _In_ const DWORD    cbData,
                            _In_ const BOOL     fReplacementPolicy );

        //  Reports the removal of a file/block from the cache and if that removal was due to limited resources.

        void ReportEvict(   _In_ CFTE* const    pcfte,
                            _In_ const QWORD    ibOffset,
                            _In_ const DWORD    cbData,
                            _In_ const BOOL     fReplacementPolicy );

    private:

        ERR ErrEnsureInitCachedFileTable() { return m_initOnceCachedFileTable.Init( ErrInitCachedFileTable_, this ); };
        ERR ErrInitCachedFileTable();
        static ERR ErrInitCachedFileTable_( _In_ TCacheBase<I, CFTE>* const pc ) { return pc->ErrInitCachedFileTable(); }
        void TermCachedFileTable();

        ERR ErrLockCachedFile(  _In_    const VolumeId      volumeid,
                                _In_    const FileId        fileid,
                                _In_    const FileSerial    fileserial,
                                _In_    CSemaphore* const   psem,
                                _Out_   CFTE** const        ppcfte );

    protected:

        //  Completion context.

        class CComplete
        {
            public:

                CComplete(  _In_                    const VolumeId              volumeid,
                            _In_                    const FileId                fileid,
                            _In_                    const FileSerial            fileserial,
                            _In_                    const QWORD                 ibOffset,
                            _In_                    const DWORD                 cbData,
                            _In_reads_( cbData )    const BYTE* const           pbData,
                            _In_                    const ICache::PfnComplete   pfnComplete,
                            _In_                    const DWORD_PTR             keyComplete)
                    :   m_volumeid( volumeid ),
                        m_fileid( fileid ),
                        m_fileserial( fileserial ),
                        m_ibOffset( ibOffset ),
                        m_cbData( cbData ),
                        m_pbData( pbData ),
                        m_pfnComplete( pfnComplete ),
                        m_keyComplete( keyComplete ),
                        m_cref( 1 ),
                        m_err( JET_errSuccess )
                {
                }

            public:

                void Release(   _In_ const ERR              err,
                                _In_ const TraceContext&    tc,
                                _In_ const OSFILEQOS        grbitQOS )
                {
                    RecordResult( err );

                    if ( FRelease() )
                    {
                        FullTraceContext ftc;
                        GetCurrUserTraceContext getutc;
                        ftc.DeepCopy( getutc.Utc(), tc );
                        Complete( ftc, grbitQOS );
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

                    CComplete* const pcomplete = (CComplete*)keyIOComplete;
                    pcomplete->Finish( err, tc, grbitQOS );
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

                    CComplete* const pcomplete = (CComplete*)keyIOComplete;
                    pcomplete->Start( err, tc, grbitQOS );
                }

            private:

                ~CComplete()
                {
                }

                CComplete( const CComplete& other );

                void AddRef() { AtomicIncrement( (LONG*)&m_cref ); }
                BOOL FRelease() { return AtomicDecrement( (LONG*)&m_cref ) == 0; }
                                
                void Start( _In_ const ERR                  err,
                            _In_ const FullTraceContext&    tc,
                            _In_ const OSFILEQOS            grbitQOS )
                {
                    AddRef();
                }

                void Finish(    _In_ const ERR                  err,
                                _In_ const FullTraceContext&    tc,
                                _In_ const OSFILEQOS            grbitQOS )
                {
                    RecordResult( err );

                    if ( FRelease() )
                    {
                        Complete( tc, grbitQOS );
                    }
                }

                void RecordResult( _In_ const ERR err )
                {
                    AtomicCompareExchange( (LONG*)&m_err, JET_errSuccess, err );
                }

                void Complete(  _In_ const FullTraceContext&    tc,
                                _In_ const OSFILEQOS            grbitQOS )
                {
                    m_pfnComplete(  m_err,
                                    m_volumeid,
                                    m_fileid,
                                    m_fileserial,
                                    tc,
                                    grbitQOS,
                                    m_ibOffset,
                                    m_cbData,
                                    m_pbData,
                                    m_keyComplete );

                    delete this;
                }


            private:

                const VolumeId              m_volumeid;
                const FileId                m_fileid;
                const FileSerial            m_fileserial;
                const QWORD                 m_ibOffset;
                const DWORD                 m_cbData;
                const BYTE* const           m_pbData;
                const ICache::PfnComplete   m_pfnComplete;
                const DWORD_PTR             m_keyComplete;
                int                         m_cref;
                ERR                         m_err;
        };

    private:

        IFileSystemFilter* const                                                            m_pfsf;
        IFileIdentification* const                                                          m_pfident;
        IFileSystemConfiguration* const                                                     m_pfsconfig;
        IBlockCacheConfiguration* const                                                     m_pbcconfig;
        ICacheConfiguration* const                                                          m_pcconfig;
        ICacheTelemetry* const                                                              m_pctm;
        IFileFilter* const                                                                  m_pffCaching;
        CCacheHeader* const                                                                 m_pch;
        CInitOnce<ERR, decltype( &ErrInitCachedFileTable_ ), TCacheBase<I, CFTE>* const>    m_initOnceCachedFileTable;
        CCachedFileHash                                                                     m_cachedFileHash;
};

template< class I, class CFTE >
TCacheBase<I, CFTE>::~TCacheBase()
{
    delete m_pch;
    delete m_pffCaching;
    delete m_pcconfig;
    delete m_pbcconfig;
    TermCachedFileTable();
}

template< class I, class CFTE >
ERR TCacheBase<I, CFTE>::ErrGetCacheType( _Out_writes_( cbGuid ) BYTE* const rgbCacheType )
{
    memcpy( rgbCacheType, m_pch->RgbCacheType(), cbGuid );

    return JET_errSuccess;
}

template< class I, class CFTE >
ERR TCacheBase<I, CFTE>::ErrGetPhysicalId(  _Out_                   VolumeId* const pvolumeid,
                                            _Out_                   FileId* const   pfileid,
                                            _Out_writes_( cbGuid )  BYTE* const     rgbUniqueId )
{
    *pvolumeid = m_pch->Volumeid();
    *pfileid = m_pch->Fileid();
    memcpy( rgbUniqueId, m_pch->RgbUniqueId(), cbGuid );

    return JET_errSuccess;
}
                
template< class I, class CFTE >
ERR TCacheBase<I, CFTE>::ErrClose(  _In_ const VolumeId     volumeid,
                                    _In_ const FileId       fileid,
                                    _In_ const FileSerial   fileserial ) 
{
    ERR     err     = JET_errSuccess;
    CFTE*   pcfte   = NULL;

    //  get the cached file if it is already open

    Call( ErrGetCachedFile( volumeid, fileid, fileserial, fTrue, &pcfte ) );

    //  release the cached file if it was open.  this will close the file because we only reference it once for all
    //  opens.  see ErrGetCachedFile

HandleError:
    ReleaseCachedFile( pcfte );
    return err;
}

template< class I, class CFTE >
TCacheBase<I, CFTE>::TCacheBase(    _In_    IFileSystemFilter* const            pfsf,
                                    _In_    IFileIdentification* const          pfident,
                                    _In_    IFileSystemConfiguration* const     pfsconfig, 
                                    _Inout_ IBlockCacheConfiguration** const    ppbcconfig,
                                    _Inout_ ICacheConfiguration** const         ppcconfig,
                                    _In_    ICacheTelemetry* const              pctm,
                                    _Inout_ IFileFilter** const                 ppffCaching,
                                    _Inout_ CCacheHeader** const                ppch )
    :   m_pfsf( pfsf ),
        m_pfident( pfident ),
        m_pfsconfig( pfsconfig ),
        m_pbcconfig( *ppbcconfig ),
        m_pcconfig( *ppcconfig ),
        m_pctm( pctm ),
        m_pffCaching( *ppffCaching ),
        m_pch( *ppch ),
        m_cachedFileHash( 0 )
{
    *ppbcconfig = NULL;
    *ppcconfig = NULL;
    *ppffCaching = NULL;
    *ppch = NULL;
}
        
template< class I, class CFTE >
ERR TCacheBase<I, CFTE>::ErrGetCachedFile(  _In_    const VolumeId      volumeid,
                                            _In_    const FileId        fileid,
                                            _In_    const FileSerial    fileserial,
                                            _In_    const BOOL          fOnlyIfAlreadyOpen,
                                            _Out_   CFTE** const        ppcfte )
{
    ERR         err     = JET_errSuccess;
    CSemaphore  sem( CSyncBasicInfo( "TCacheBase<I, CFTE>::ErrGetCachedFile" ) );
    CFTE*       pcfte   = NULL;

    *ppcfte = NULL;

    //  wait to lock the entry for this cached file

    Call( ErrLockCachedFile( volumeid, fileid, fileserial, &sem, &pcfte ) );

    //  if the cached file isn't open and we want to open it then do so

    if ( !pcfte->FOpen() && !fOnlyIfAlreadyOpen )
    {
        //  open the cached file

        Call( pcfte->ErrOpenCachedFile( m_pfsf, m_pfident, m_pbcconfig, m_pffCaching ) );

        //  reference the cached file once for all opens.  this will be released by ICache::ErrClose

        pcfte->AddRef();
    }

    //  return the opened cached file

    *ppcfte = pcfte->FOpen() ? pcfte : NULL;

HandleError:
    ReleaseCachedFile( pcfte, &sem );
    if ( err < JET_errSuccess )
    {
        *ppcfte = NULL;
    }
    return err;
}

template< class I, class CFTE >
VOID TCacheBase<I, CFTE>::ReleaseCachedFile(    _In_opt_ CFTE* const   pcfte,
                                                _In_opt_ CSemaphore* const              psem )
{
    BOOL fDeleteCachedFileTableEntry = fFalse;

    if ( !pcfte )
    {
        return;
    }

    CCachedFileKey          key( pcfte );
    CCachedFileHash::CLock  lock;
    m_cachedFileHash.WriteLockKey( key, &lock );

    pcfte->RemoveAsOwnerOrWaiter( psem );

    if ( fDeleteCachedFileTableEntry = pcfte->FRelease() )
    {
        CCachedFileHash::ERR errCachedFileHash = m_cachedFileHash.ErrDeleteEntry( &lock );
        Assert( errCachedFileHash == CCachedFileHash::ERR::errSuccess );
    }
    else
    {
        pcfte->AddNextOwner();
    }

    m_cachedFileHash.WriteUnlockKey( &lock );

    if ( fDeleteCachedFileTableEntry )
    {
        delete pcfte;
    }
}

template< class I, class CFTE >
void TCacheBase<I, CFTE>::ReportMiss(   _In_ CFTE* const    pcfte,
                                        _In_ const QWORD    ibOffset,
                                        _In_ const DWORD    cbData,
                                        _In_ const BOOL     fRead,
                                        _In_ const BOOL     fCacheIfPossible )
{
    const ICacheTelemetry::FileNumber filenumber = pcfte->Filenumber();
    const ICacheTelemetry::BlockNumber blocknumberMax = pcfte->Blocknumber( ibOffset + cbData + pcfte->CbBlockSize() - 1 );
    for (   ICacheTelemetry::BlockNumber blocknumber = pcfte->Blocknumber( ibOffset );
            blocknumber < blocknumberMax;
            blocknumber = ( ICacheTelemetry::BlockNumber )( (QWORD)blocknumber + 1 ) )
    {
        m_pctm->Miss( filenumber, blocknumber, fRead, fCacheIfPossible );
    }
}

template< class I, class CFTE >
void TCacheBase<I, CFTE>::ReportHit(    _In_ CFTE* const    pcfte,
                                        _In_ const QWORD    ibOffset,
                                        _In_ const DWORD    cbData,
                                        _In_ const BOOL     fRead,
                                        _In_ const BOOL     fCacheIfPossible )
{
    const ICacheTelemetry::FileNumber filenumber = pcfte->Filenumber();
    const ICacheTelemetry::BlockNumber blocknumberMax = pcfte->Blocknumber( ibOffset + cbData + pcfte->CbBlockSize() - 1 );
    for (   ICacheTelemetry::BlockNumber blocknumber = pcfte->Blocknumber( ibOffset );
            blocknumber < blocknumberMax;
            blocknumber = ( ICacheTelemetry::BlockNumber )( (QWORD)blocknumber + 1 ) )
    {
        m_pctm->Hit( filenumber, blocknumber, fRead, fCacheIfPossible );
    }
}

template< class I, class CFTE >
void TCacheBase<I, CFTE>::ReportUpdate( _In_ CFTE* const    pcfte,
                                        _In_ const QWORD    ibOffset,
                                        _In_ const DWORD    cbData )
{
    const ICacheTelemetry::FileNumber filenumber = pcfte->Filenumber();
    const ICacheTelemetry::BlockNumber blocknumberMax = pcfte->Blocknumber( ibOffset + cbData + pcfte->CbBlockSize() - 1 );
    for (   ICacheTelemetry::BlockNumber blocknumber = pcfte->Blocknumber( ibOffset );
            blocknumber < blocknumberMax;
            blocknumber = ( ICacheTelemetry::BlockNumber )( (QWORD)blocknumber + 1 ) )
    {
        m_pctm->Update( filenumber, blocknumber );
    }
}

template< class I, class CFTE >
void TCacheBase<I, CFTE>::ReportWrite(  _In_ CFTE* const    pcfte,
                                        _In_ const QWORD    ibOffset,
                                        _In_ const DWORD    cbData,
                                        _In_ const BOOL     fReplacementPolicy )
{
    const ICacheTelemetry::FileNumber filenumber = pcfte->Filenumber();
    const ICacheTelemetry::BlockNumber blocknumberMax = pcfte->Blocknumber( ibOffset + cbData + pcfte->CbBlockSize() - 1 );
    for (   ICacheTelemetry::BlockNumber blocknumber = pcfte->Blocknumber( ibOffset );
            blocknumber < blocknumberMax;
            blocknumber = ( ICacheTelemetry::BlockNumber )( (QWORD)blocknumber + 1 ) )
    {
        m_pctm->Write( filenumber, blocknumber, fReplacementPolicy );
    }
}

template< class I, class CFTE >
void TCacheBase<I, CFTE>::ReportEvict(  _In_ CFTE* const    pcfte,
                                        _In_ const QWORD    ibOffset,
                                        _In_ const DWORD    cbData,
                                        _In_ const BOOL     fReplacementPolicy )
{
    const ICacheTelemetry::FileNumber filenumber = pcfte->Filenumber();
    const ICacheTelemetry::BlockNumber blocknumberMax = pcfte->Blocknumber( ibOffset + cbData + pcfte->CbBlockSize() - 1 );
    for (   ICacheTelemetry::BlockNumber blocknumber = pcfte->Blocknumber( ibOffset );
            blocknumber < blocknumberMax;
            blocknumber = ( ICacheTelemetry::BlockNumber )( (QWORD)blocknumber + 1 ) )
    {
        m_pctm->Evict( filenumber, blocknumber, fReplacementPolicy );
    }
}

template< class I, class CFTE >
ERR TCacheBase<I, CFTE>::ErrInitCachedFileTable()
{
    ERR err = JET_errSuccess;

    if ( m_cachedFileHash.ErrInit( 5.0, 1.0 ) == CCachedFileHash::ERR::errOutOfMemory )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

HandleError:
    return err;
}

template< class I, class CFTE >
void TCacheBase<I, CFTE>::TermCachedFileTable()
{
    if ( m_initOnceCachedFileTable.FIsInit() )
    {
        CCachedFileHash::CLock  lock;
        CCachedFileEntry        entry;
        CCachedFileHash::ERR    errCachedFileHash;

        m_cachedFileHash.BeginHashScan( &lock );

        while ( ( errCachedFileHash = m_cachedFileHash.ErrMoveNext( &lock ) ) != CCachedFileHash::ERR::errNoCurrentEntry )
        {
            errCachedFileHash = m_cachedFileHash.ErrRetrieveEntry( &lock, &entry );
            Assert( errCachedFileHash == CCachedFileHash::ERR::errSuccess );

            delete entry.Pcfte();
        }

        m_cachedFileHash.EndHashScan( &lock );

        m_cachedFileHash.Term();
    }
}

template< class I, class CFTE >
ERR TCacheBase<I, CFTE>::ErrLockCachedFile( _In_    const VolumeId      volumeid,
                                            _In_    const FileId        fileid,
                                            _In_    const FileSerial    fileserial,
                                            _In_    CSemaphore* const   psem,
                                            _Out_   CFTE** const        ppcfte )
{
    ERR                     err                 = JET_errSuccess;
    CCachedFileKey          key( volumeid, fileid, fileserial );
    CCachedFileEntry        entry;
    CCachedFileHash::CLock  lock;
    CCachedFileHash::ERR    errCachedFileHash   = CCachedFileHash::ERR::errSuccess;
    BOOL                    fReadLocked         = fFalse;
    CFTE*                   pcfteExisting       = NULL;
    BOOL                    fWriteLocked        = fFalse;
    CFTE*                   pcfteNew            = NULL;
    BOOL                    fRemove             = fFalse;

    *ppcfte = NULL;

    Call( ErrEnsureInitCachedFileTable() );

    m_cachedFileHash.ReadLockKey( key, &lock );
    fReadLocked = fTrue;

    errCachedFileHash = m_cachedFileHash.ErrRetrieveEntry( &lock, &entry );
    if ( errCachedFileHash == CCachedFileHash::ERR::errSuccess )
    {
        pcfteExisting = static_cast<CFTE*>( entry.Pcfte() );
    }
    else
    {
        Assert( errCachedFileHash == CCachedFileHash::ERR::errEntryNotFound );

        m_cachedFileHash.ReadUnlockKey( &lock );
        fReadLocked = fFalse;
        m_cachedFileHash.WriteLockKey( key, &lock );
        fWriteLocked = fTrue;

        errCachedFileHash = m_cachedFileHash.ErrRetrieveEntry( &lock, &entry );
        if ( errCachedFileHash == CCachedFileHash::ERR::errSuccess )
        {
            pcfteExisting = static_cast<CFTE*>( entry.Pcfte() );
        }
        else
        {
            Assert( errCachedFileHash == CCachedFileHash::ERR::errEntryNotFound );

            Alloc( pcfteExisting = pcfteNew = new CFTE( volumeid, fileid, fileserial ) );

            entry = CCachedFileEntry( pcfteNew );
            errCachedFileHash = m_cachedFileHash.ErrInsertEntry( &lock, entry );
            if ( errCachedFileHash == CCachedFileHash::ERR::errOutOfMemory )
            {
                Call( ErrERRCheck( JET_errOutOfMemory ) );
            }
            Assert( errCachedFileHash == CCachedFileHash::ERR::errSuccess );
            fRemove = fTrue;
        }
    }

    Call( pcfteExisting->ErrAddAsOwnerOrWaiter( psem ) );

    pcfteExisting->AddRef();
    fRemove = fFalse;

    if ( fReadLocked )
    {
        m_cachedFileHash.ReadUnlockKey( &lock );
        fReadLocked = fFalse;
    }
    if ( fWriteLocked )
    {
        m_cachedFileHash.WriteUnlockKey( &lock );
        fWriteLocked = fFalse;
    }

    *ppcfte = pcfteExisting;
    psem->Wait();

HandleError:
    if ( fRemove )
    {
        errCachedFileHash = m_cachedFileHash.ErrDeleteEntry( &lock );
        Assert( errCachedFileHash == CCachedFileHash::ERR::errSuccess );
        delete pcfteNew;
    }
    if ( fReadLocked )
    {
        m_cachedFileHash.ReadUnlockKey( &lock );
    }
    if ( fWriteLocked )
    {
        m_cachedFileHash.WriteUnlockKey( &lock );
    }
    if ( err < JET_errSuccess )
    {
        *ppcfte = NULL;
    }
    return err;
}
