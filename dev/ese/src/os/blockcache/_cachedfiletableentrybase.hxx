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
        BOOL FExternalOpen() const { return m_fExternalOpen; }
        IFileFilter* Pff() const { return m_pff; }
        ICachedFileConfiguration* Pcfconfig() const { return m_pcfconfig; }
        UINT UiHash() const { return CCachedFileTableEntryBase::UiHash( m_volumeid, m_fileid, m_fileserial ); }

        void AddRef() { AtomicIncrement( (LONG*)&m_cref ); }
        BOOL FRelease() { return AtomicDecrement( (LONG*)&m_cref ) == 0; }
        BOOL FReleaseIfNotLastReference();

        void SetExternalOpen();
        BOOL FResetExternalOpen();

        class CWaiter;
        void AddAsWaiterForExternalClose( _In_ CWaiter* const pwaiter );
        BOOL FWaitersForExternalClose() const { return !m_ilWaitersForExternalClose.FEmpty(); }
        void ReleaseWaitersForExternalClose();

        ICacheTelemetry::FileNumber Filenumber() const { return m_filenumber; }
        DWORD CbBlockSize() const { return m_cbBlockSize; }
        ICacheTelemetry::BlockNumber Blocknumber( const QWORD ibOffset ) const
        {
            return ( ICacheTelemetry::BlockNumber )( ibOffset / CbBlockSize() );
        }

        static UINT UiHash( _In_ const VolumeId     volumeid,
                            _In_ const FileId       fileid,
                            _In_ const FileSerial   fileserial );

        ERR ErrEnsureOpenCacheFile( _In_ IFileSystemFilter* const           pfsf,
                                    _In_ IFileIdentification* const         pfident,
                                    _In_ IBlockCacheConfiguration* const    pbcconfig,
                                    _In_ IFileFilter* const                 pffCaching)
        {
            return m_initOnceCachedFile.Init( ErrOpenCachedFile_, this, pfsf, pfident, pbcconfig, pffCaching );
        };

    protected:

        virtual ERR ErrOpenCachedFile(  _In_ IFileSystemFilter* const           pfsf,
                                        _In_ IFileIdentification* const         pfident,
                                        _In_ IBlockCacheConfiguration* const    pbcconfig,
                                        _In_ IFileFilter* const                 pffCaching );

    private:

        static ERR ErrOpenCachedFile_(  _In_ CCachedFileTableEntryBase* const   pcfte,
                                        _In_ IFileSystemFilter* const           pfsf,
                                        _In_ IFileIdentification* const         pfident,
                                        _In_ IBlockCacheConfiguration* const    pbcconfig,
                                        _In_ IFileFilter* const                 pffCaching )
        { 
            return pcfte->ErrOpenCachedFile( pfsf, pfident, pbcconfig, pffCaching );
        }

    public:

        //  Wait context for external close.

        class CWaiter
        {
            public:

                CWaiter()
                    :   m_sem( CSyncBasicInfo( "CCachedFileTableEntryBase::CWaiter::m_sem" ) )
                {
                }

                void Wait() { m_sem.Acquire(); }
                void Release() { m_sem.Release(); }

                static SIZE_T OffsetOfILE() { return OffsetOf( CWaiter, m_ile ); }

            private:

                typename CInvasiveList< CWaiter, OffsetOfILE >::CElement    m_ile;
                CSemaphore                                                  m_sem;
        };

    private:

        typedef CInitOnce< ERR, decltype( &ErrOpenCachedFile_ ), CCachedFileTableEntryBase* const, IFileSystemFilter* const, IFileIdentification* const, IBlockCacheConfiguration* const, IFileFilter* const > CInitOnceCachedFile;

        const VolumeId                                  m_volumeid;
        const FileId                                    m_fileid;
        const FileSerial                                m_fileserial;
        volatile int                                    m_cref;
        volatile BOOL                                   m_fExternalOpen;
        CInvasiveList<CWaiter, CWaiter::OffsetOfILE>    m_ilWaitersForExternalClose;
        IFileFilter*                                    m_pff;
        ICachedFileConfiguration*                       m_pcfconfig;
        ICacheTelemetry::FileNumber                     m_filenumber;
        ULONG                                           m_cbBlockSize;
        CInitOnceCachedFile                             m_initOnceCachedFile;
};

INLINE CCachedFileTableEntryBase::CCachedFileTableEntryBase(    _In_ const VolumeId     volumeid,
                                                                _In_ const FileId       fileid,
                                                                _In_ const FileSerial   fileserial )
    :   m_volumeid( volumeid ),
        m_fileid( fileid ),
        m_fileserial( fileserial ),
        m_cref( 0 ),
        m_fExternalOpen( fFalse ),
        m_pff( NULL ),
        m_pcfconfig( NULL ),
        m_filenumber( filenumberInvalid ),
        m_cbBlockSize( 0 )
{
}

INLINE CCachedFileTableEntryBase::~CCachedFileTableEntryBase()
{
    Assert( m_cref == 0 || m_cref == 1 );

    delete m_pff;
    delete m_pcfconfig;

    ReleaseWaitersForExternalClose();
}

INLINE UINT CCachedFileTableEntryBase::UiHash(  _In_ const VolumeId     volumeid,
                                                _In_ const FileId       fileid,
                                                _In_ const FileSerial   fileserial )
{
    return (UINT)( (DWORD)volumeid + ( (QWORD)fileid >> 32 ) + ( (QWORD)fileid & dwMax ) + (DWORD)fileserial );
}

INLINE BOOL CCachedFileTableEntryBase::FReleaseIfNotLastReference()
{
    OSSYNC_FOREVER
    {
        const LONG crefInitial = AtomicRead( (LONG*)&m_cref );
        const LONG crefValue = crefInitial - 1;

        if ( crefValue <= 0 )
        {
            return fFalse;
        }

        if ( AtomicCompareExchange( (LONG*)&m_cref, crefInitial, crefValue ) == crefInitial )
        {
            return fTrue;
        }
    }
}

INLINE void CCachedFileTableEntryBase::SetExternalOpen()
{ 
    if ( !AtomicExchange( (LONG*)&m_fExternalOpen, fTrue ) )
    {
        AddRef();
    }
}
        
INLINE BOOL CCachedFileTableEntryBase::FResetExternalOpen()
{
    BOOL fRelease = fFalse;

    if ( AtomicExchange( (LONG*)&m_fExternalOpen, fFalse ) )
    {
        fRelease = FRelease();
    }

    return fRelease;
}

INLINE void CCachedFileTableEntryBase::AddAsWaiterForExternalClose( _In_ CWaiter* const pwaiter )
{
    m_ilWaitersForExternalClose.InsertAsNextMost( pwaiter );
}

INLINE void CCachedFileTableEntryBase::ReleaseWaitersForExternalClose()
{
    while ( m_ilWaitersForExternalClose.PrevMost() )
    {
        CWaiter* const pwaiter = m_ilWaitersForExternalClose.PrevMost();
        m_ilWaitersForExternalClose.Remove( pwaiter );
        pwaiter->Release();
    }
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
