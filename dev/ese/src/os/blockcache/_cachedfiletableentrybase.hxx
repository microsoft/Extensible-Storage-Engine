// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  A cached file table entry base class.

class CCachedFileTableEntryBase  //  cfte
{
    public:

        CCachedFileTableEntryBase(  _In_ const VolumeId     volumeid,
                                    _In_ const FileId       fileid,
                                    _In_ const FileSerial   fileserial );

        virtual ~CCachedFileTableEntryBase();

        VolumeId Volumeid() const { return m_volumeid; }
        FileId Fileid() const { return m_fileid; }
        FileSerial Fileserial() const { return m_fileserial; }
        BOOL FOpen() const { return Pff() != NULL; }
        IFileFilter* Pff() const { return m_pff; }
        ICachedFileConfiguration* Pcfconfig() const { return m_pcfconfig; }
        UINT UiHash() const { return CCachedFileTableEntryBase::UiHash( m_volumeid, m_fileid, m_fileserial ); }

        void AddRef() { AtomicIncrement( (LONG*)&m_cref ); }
        BOOL FRelease() { return AtomicDecrement( (LONG*)&m_cref ) == 0; }

        ERR ErrAddAsOwnerOrWaiter( _In_ CSemaphore* const psem );
        void AddNextOwner();
        void RemoveAsOwnerOrWaiter( _In_opt_ CSemaphore* const psem );

        ICacheTelemetry::FileNumber Filenumber() const { return m_filenumber; }
        DWORD CbBlockSize() const { return m_cbBlockSize; }
        ICacheTelemetry::BlockNumber Blocknumber( const QWORD ibOffset ) const
        {
            return ( ICacheTelemetry::BlockNumber )( ibOffset / CbBlockSize() );
        }

        static UINT UiHash( _In_ const VolumeId     volumeid,
                            _In_ const FileId       fileid,
                            _In_ const FileSerial   fileserial );

        virtual ERR ErrOpenCachedFile(  _In_ IFileSystemFilter* const           pfsf,
                                        _In_ IFileIdentification* const         pfident,
                                        _In_ IBlockCacheConfiguration* const    pbcconfig,
                                        _In_ IFileFilter* const                 pffCaching );

    private:

        //  Wait context for cache miss collisions on the cached file table.

        class CWaiter
        {
            public:

                CWaiter( _In_ CSemaphore* const psem )
                    :   m_psem( psem )
                {
                }

                CSemaphore* Psem() { return m_psem; }

                static SIZE_T OffsetOfILE() { return OffsetOf( CWaiter, m_ile ); }

            private:

                typename CInvasiveList< CWaiter, OffsetOfILE >::CElement    m_ile;
                CSemaphore* const                                           m_psem;
        };

    private:

        const VolumeId                                  m_volumeid;
        const FileId                                    m_fileid;
        const FileSerial                                m_fileserial;
        volatile int                                    m_cref;
        CSemaphore*                                     m_psemOwner;
        CInvasiveList<CWaiter, CWaiter::OffsetOfILE>    m_ilWaiters;
        IFileFilter*                                    m_pff;
        ICachedFileConfiguration*                       m_pcfconfig;
        ICacheTelemetry::FileNumber                     m_filenumber;
        ULONG                                           m_cbBlockSize;
};

INLINE CCachedFileTableEntryBase::CCachedFileTableEntryBase(    _In_ const VolumeId     volumeid,
                                                                _In_ const FileId       fileid,
                                                                _In_ const FileSerial   fileserial )
    :   m_volumeid( volumeid ),
        m_fileid( fileid ),
        m_fileserial( fileserial ),
        m_cref( 0 ),
        m_psemOwner( NULL ),
        m_pff( NULL ),
        m_pcfconfig( NULL ),
        m_filenumber( filenumberInvalid ),
        m_cbBlockSize( 0 )
{
}

INLINE CCachedFileTableEntryBase::~CCachedFileTableEntryBase()
{
    Assert( m_cref == 0 || m_cref == 1 );
    Assert( !m_psemOwner );
    Assert( m_ilWaiters.FEmpty() );

    delete m_pff;
    delete m_pcfconfig;
}

INLINE ERR CCachedFileTableEntryBase::ErrAddAsOwnerOrWaiter( _In_ CSemaphore* const psem )
{
    ERR         err                     = JET_errSuccess;
    BOOL        fCleanUpStateSaved      = fFalse;
    BOOL        fRestoreCleanupState    = fFalse;
    CWaiter*    pwaiter                 = NULL;

    if ( !m_psemOwner )
    {
        m_psemOwner = psem;
        m_psemOwner->Release();
    }
    else
    {
        fCleanUpStateSaved = FOSSetCleanupState( fFalse );
        fRestoreCleanupState = fTrue;

        Alloc( pwaiter = new CWaiter( psem ) );

        m_ilWaiters.InsertAsNextMost( pwaiter );
        pwaiter = NULL;
    }

HandleError:
    if ( fRestoreCleanupState )
    {
        FOSSetCleanupState( fCleanUpStateSaved );
    }
    delete pwaiter;
    return err;
}

INLINE void CCachedFileTableEntryBase::AddNextOwner()
{
    CWaiter* pwaiter = m_ilWaiters.PrevMost();
    if ( pwaiter != NULL )
    {
        m_ilWaiters.Remove( pwaiter );
        m_psemOwner = pwaiter->Psem();
        m_psemOwner->Release();
        delete pwaiter;
    }
}

INLINE void CCachedFileTableEntryBase::RemoveAsOwnerOrWaiter( _In_opt_ CSemaphore* const psem )
{
    CWaiter* pwaiter = m_ilWaiters.PrevMost();
    while ( pwaiter != NULL )
    {
        CWaiter* const pwaiterNext = m_ilWaiters.Next( pwaiter );
        if ( pwaiter->Psem() == psem )
        {
            m_ilWaiters.Remove( pwaiter );
            delete pwaiter;
        }
        pwaiter = pwaiterNext;
    }

    if ( m_psemOwner == psem )
    {
        m_psemOwner = NULL;
    }
}

INLINE UINT CCachedFileTableEntryBase::UiHash(  _In_ const VolumeId     volumeid,
                                                _In_ const FileId       fileid,
                                                _In_ const FileSerial   fileserial )
{
    return (UINT)( (DWORD)volumeid + ( (QWORD)fileid >> 32 ) + ( (QWORD)fileid & dwMax ) + (DWORD)fileserial );
}

INLINE ERR CCachedFileTableEntryBase::ErrOpenCachedFile(    _In_ IFileSystemFilter* const           pfsf,
                                                            _In_ IFileIdentification* const         pfident,
                                                            _In_ IBlockCacheConfiguration* const    pbcconfig,
                                                            _In_ IFileFilter* const                 pffCaching )
{
    ERR                         err                                                 = JET_errSuccess;
    IFileFilter*                pff                                                 = NULL;
    WCHAR                       wszAbsCachedFilePath[ IFileSystemAPI::cchPathMax ]  = { 0 };
    WCHAR                       wszCachedFileKeyPath[ IFileSystemAPI::cchPathMax ]  = { 0 };
    ICachedFileConfiguration*   pcfconfig                                           = NULL;

    //  open the cached file by id

    Call( pfsf->ErrFileOpenById(    Volumeid(),
                                    Fileid(),
                                    Fileserial(), 
                                    IFileAPI::fmfStorageWriteBack,
                                    (IFileAPI**)&pff ) );

    //  get the configuration for the cached file

    Call( pff->ErrPath( wszAbsCachedFilePath ) );
    Call( pfident->ErrGetFileKeyPath( wszAbsCachedFilePath, wszCachedFileKeyPath ) );
    Call( pbcconfig->ErrGetCachedFileConfiguration( wszCachedFileKeyPath, &pcfconfig ) );

    //  cache commonly used configuration

    m_filenumber = (ICacheTelemetry::FileNumber)pcfconfig->LCacheTelemetryFileNumber();
    m_cbBlockSize = pcfconfig->CbBlockSize();

    //  make the cached file available for other opens

    m_pff = pff;
    m_pcfconfig = pcfconfig;

    pff = NULL;
    pcfconfig = NULL;

HandleError:
    delete pcfconfig;
    delete pff;
    return err;
}
