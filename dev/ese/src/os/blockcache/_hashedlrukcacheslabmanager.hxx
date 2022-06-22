// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  TCachedBlockSlabManager:  implementation of ICachedBlockSlabManager and its derivatives.
//
//  I:  ICachedBlockSlabManager or derivative
//
//  This class manages the set of slabs that belong to the cache.  Slabs can be addressed directly by offset for
//  journal replay or they can be addressed logically by cached block id.  Cached blocks are distributed among the
//  slabs by a hash function.  This hash function is key to the overall performance of the cache.
//
//  Slabs can be acquired for exclusive access only.  They must be released via ICachedBlockSlab::Release when no
//  longer in use.  Any changes made to the slab are made temporarily in memory.  If a slab is released without
//  accepting these changes via ICachedBlockSlab::AcceptChanges then those changes will be lost.
//
//  A modified slab is retained in memory as long as its accepted changes have not yet been written back to storage via
//  ICachedBlockSlab::ErrSave.
//
//  A slab is generally not retained in memory for any other reason.  This is because of the nature of the persistent
//  cache it supports.  We have obviously already decided that the data we are caching is not important enough to hold
//  in memory.  We also consider its metadata too cold to retain in memory.
// 
//  The slab manager is responsible for defer initializing a slab with its initial state on load.  This happens when a
//  slab is read and any chunk of that slab is confirmed to be uninitialized via its checksum and it has never been
//  written according to the write counters (CCachedBlockWriteCountsManager).
//
//  The slab manager is actually used twice:  once for the main cache state and again to manage the avail pool.  The
//  avail pool is the set of clusters we keep to support writing new data into the cache as a virtual part of the
//  journal.  The avail pool slabs are only accessed by offset because they contain no cached data.

class CCachedBlockSlabReference;

template< class I >
class TCachedBlockSlabManager  //  cbsm
    :   public I
{
    public:  //  specialized API

        ~TCachedBlockSlabManager()
        {
            TermSlabTable();
        }

        void RemoveSlabReference( _Inout_ CCachedBlockSlabTableEntry** const ppste )
        {
            ReleaseSlab( ppste );
        }

    public:  //  ICachedBlockSlabManager

        ERR ErrGetSlabForCachedBlock(   _In_    const CCachedBlockId&   cbid,
                                        _Out_   QWORD* const            pib ) override;

        ERR ErrGetSlab( _In_    const QWORD                 ib,
                        _In_    const BOOL                  fWait,
                        _In_    const BOOL                  fIgnoreVerificationErrors,
                        _Out_   ICachedBlockSlab** const    ppcbs ) override;

        ERR ErrVisitSlabs(  _In_ const ICachedBlockSlabManager::PfnVisitSlab    pfnVisitSlab,
                            _In_ const DWORD_PTR                                keyVisitSlab ) override;

    protected:

        TCachedBlockSlabManager(    _In_    IFileFilter* const                      pff,
                                    _In_    const QWORD                             cbCachingFilePerSlab,
                                    _In_    const QWORD                             cbCachedFilePerSlab,
                                    _In_    const QWORD                             ibChunk,
                                    _In_    const QWORD                             cbChunk,
                                    _In_    ICachedBlockWriteCountsManager* const   pcbwcm,
                                    _In_    const QWORD                             icbwcBase,
                                    _In_    const ClusterNumber                     clnoMin,
                                    _In_    const ClusterNumber                     clnoMax );

    private:

        ERR ErrLockSlab(    _In_    const QWORD                         ibSlab,
                            _In_    const BOOL                          fWait,
                            _Out_   CCachedBlockSlabTableEntry** const  ppste );
        void ReleaseSlab( _Inout_ CCachedBlockSlabTableEntry** const ppste );

        BOOL FValidCachedBlockId( _In_ const CCachedBlockId& cbid );
        QWORD IbSlabOfCachedBlockId( _In_ const CCachedBlockId& cbid );

        ERR ErrEnsureInitSlabTable() { return m_initOnceSlabTable.Init( ErrInitSlabTable_, this ); };
        ERR ErrInitSlabTable();
        static ERR ErrInitSlabTable_( _In_ TCachedBlockSlabManager<I>* const pcbsm ) { return pcbsm->ErrInitSlabTable(); }
        void TermSlabTable();

        ERR ErrBlockCacheInternalError( _In_ const char* const szTag )
        {
            return ::ErrBlockCacheInternalError( m_pff, szTag );
        }

    private:

        IFileFilter* const                                                                  m_pff;
        const QWORD                                                                         m_cbCachingFilePerSlab;
        const QWORD                                                                         m_cbCachedFilePerSlab;
        const QWORD                                                                         m_ibChunk;
        const QWORD                                                                         m_cbChunk;
        const QWORD                                                                         m_cbSlab;
        const QWORD                                                                         m_cSlab;
        const QWORD                                                                         m_cCachedFileBlockPerSlab;
        ICachedBlockWriteCountsManager* const                                               m_pcbwcm;
        const QWORD                                                                         m_icbwcBase;
        const ClusterNumber                                                                 m_clnoMin;
        const ClusterNumber                                                                 m_clnoMax;
        CInitOnce< ERR, decltype( &ErrInitSlabTable_ ), TCachedBlockSlabManager<I>* const > m_initOnceSlabTable;
        CCachedBlockSlabHash                                                                m_slabHash;
};

template< class I  >
INLINE ERR TCachedBlockSlabManager<I>::ErrGetSlabForCachedBlock(    _In_    const CCachedBlockId&   cbid,
                                                                    _Out_   QWORD* const            pib )
{
    ERR err = JET_errSuccess;

    *pib = 0;

    //  we do not accept invalid cached block ids

    if ( !FValidCachedBlockId( cbid ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  return the slab id

    *pib = IbSlabOfCachedBlockId( cbid );

HandleError:
    if ( err < JET_errSuccess )
    {
        *pib = 0;
    }
    return err;
}

template< class I  >
INLINE ERR TCachedBlockSlabManager<I>::ErrGetSlab(  _In_    const QWORD                 ib,
                                                    _In_    const BOOL                  fWait,
                                                    _In_    const BOOL                  fIgnoreVerificationErrors,
                                                    _Out_   ICachedBlockSlab** const    ppcbs )
{
    ERR                         err         = JET_errSuccess;
    CCachedBlockSlabTableEntry* pste        = NULL;
    CCachedBlockSlab*           pcbs        = NULL;
    ERR                         errSlab     = JET_errSuccess;
    CCachedBlockSlabReference*  pcbsr       = NULL;

    *ppcbs = NULL;

    //  we must be asking for a slab by a legal offset

    if ( m_ibChunk > ib || ib >= m_ibChunk + m_cbChunk )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( ib % sizeof( CCachedBlockChunk ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  create/open the entry for this slab by its offset

    Call( ErrLockSlab( ib, fWait, &pste ) );

    if ( !pste )
    {
        Error( JET_errSuccess );
    }

    //  if the slab isn't already loaded then load it

    if ( !pste->Pcbs() )
    {
        //  load the slab.  this will still work even if there are verification errors

        const QWORD iChunk          = ( ib - m_ibChunk ) / sizeof( CCachedBlockChunk );
        const QWORD icbwcBase       = m_icbwcBase + iChunk;
        const QWORD iSlab           = ( ib - m_ibChunk ) / m_cbSlab;

        errSlab = CCachedBlockSlab::ErrLoad(    m_pff, 
                                                ib, 
                                                m_cbSlab, 
                                                m_pcbwcm, 
                                                icbwcBase, 
                                                m_clnoMin,
                                                m_clnoMax,
                                                fIgnoreVerificationErrors,
                                                &pcbs );

        //  fail only if we are not supposed to ignore verification errors

        Call( fIgnoreVerificationErrors ? ErrIgnoreVerificationErrors( errSlab ) : errSlab );

        //  verify that the slab error status matches what was returned

        if ( errSlab != pcbs->ErrStatus() )
        {
            Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlabManagerMismatchedErr" ) );
        }

        //  keep the slab in the slab entry

        pste->OpenSlab( &pcbs );
    }

    //  verify that the slab validation state meets our expectations

    errSlab = pste->Pcbs()->ErrStatus();
    Call( fIgnoreVerificationErrors ? ErrIgnoreVerificationErrors( errSlab ) : errSlab );

    //  provide a wrapper for the slab that will manage its lifecycle

    Alloc( pcbsr = new CCachedBlockSlabReference( this, &pste, pste->Pcbs(), fIgnoreVerificationErrors ) );

    //  return the slab

    *ppcbs = pcbsr;
    pcbsr = NULL;

    err = errSlab;

HandleError:
    delete pcbsr;
    delete pcbs;
    ReleaseSlab( &pste );
    if ( ( fIgnoreVerificationErrors ? ErrIgnoreVerificationErrors( err ) : err ) < JET_errSuccess )
    {
        delete *ppcbs;
        *ppcbs = NULL;
    }
    return err;
}

template< class I  >
INLINE ERR TCachedBlockSlabManager<I>::ErrVisitSlabs(   _In_ const ICachedBlockSlabManager::PfnVisitSlab    pfnVisitSlab,
                                                        _In_ const DWORD_PTR                                keyVisitSlab )
{
    ERR                 err     = JET_errSuccess;
    ERR                 errSlab = JET_errSuccess;
    ICachedBlockSlab*   pcbs    = NULL;

    //  we expect a callback

    if ( !pfnVisitSlab )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  iterate through all the slabs we own

    for ( QWORD ibSlab = m_ibChunk; ibSlab < m_ibChunk + m_cbChunk; ibSlab += m_cbSlab )
    {
        //  close any previously opened slab
        
        delete pcbs;
        pcbs = NULL;

        //  open this slab, ignoring verification errors

        errSlab = ErrGetSlab( ibSlab, fTrue, fTrue, &pcbs );
        Call( ErrIgnoreVerificationErrors( errSlab ) );

        //  visit this slab

        if ( !pfnVisitSlab( ibSlab, errSlab, pcbs, keyVisitSlab ) )
        {
            break;
        }
    }

HandleError:
    delete pcbs;
    return err;
}

template< class I  >
INLINE TCachedBlockSlabManager<I>::TCachedBlockSlabManager( _In_    IFileFilter* const                      pff,
                                                            _In_    const QWORD                             cbCachingFilePerSlab,
                                                            _In_    const QWORD                             cbCachedFilePerSlab,
                                                            _In_    const QWORD                             ibChunk,
                                                            _In_    const QWORD                             cbChunk,
                                                            _In_    ICachedBlockWriteCountsManager* const   pcbwcm,
                                                            _In_    const QWORD                             icbwcBase,
                                                            _In_    const ClusterNumber                     clnoMin,
                                                            _In_    const ClusterNumber                     clnoMax )
    :   m_pff( pff ),
        m_cbCachingFilePerSlab( cbCachingFilePerSlab ),
        m_cbCachedFilePerSlab( cbCachedFilePerSlab  ),
        m_ibChunk( ibChunk ),
        m_cbChunk( cbChunk ),
        m_cbSlab( ( m_cbCachingFilePerSlab / ( CCachedBlockChunk::Ccbl() * cbCachedBlock ) ) * sizeof( CCachedBlockChunk ) ),
        m_cSlab( m_cbChunk / m_cbSlab ),
        m_cCachedFileBlockPerSlab( m_cbCachedFilePerSlab / cbCachedBlock ),
        m_pcbwcm( pcbwcm ),
        m_icbwcBase( icbwcBase ),
        m_clnoMin( clnoMin ),
        m_clnoMax( clnoMax ),
        m_slabHash( rankSlabHash )
{
}

template< class I >
ERR TCachedBlockSlabManager<I>::ErrLockSlab(    _In_    const QWORD                         ibSlab,
                                                _In_    const BOOL                          fWait,
                                                _Out_   CCachedBlockSlabTableEntry** const  ppste )
{
    ERR                                     err                     = JET_errSuccess;
    CCachedBlockSlabKey                     key( ibSlab );
    CCachedBlockSlabEntry                   entry;
    CCachedBlockSlabHash::CLock             lock;
    CCachedBlockSlabHash::ERR               errCachedBlockSlabHash  = CCachedBlockSlabHash::ERR::errSuccess;
    BOOL                                    fLocked                 = fFalse;
    CCachedBlockSlabTableEntry*             psteExisting            = NULL;
    CCachedBlockSlabTableEntry*             psteNew                 = NULL;
    BOOL                                    fDeleteEntry            = fFalse;
    CCachedBlockSlabTableEntry::CContext    context;

    *ppste = NULL;

    Call( ErrEnsureInitSlabTable() );

    m_slabHash.WriteLockKey( key, &lock );
    fLocked = fTrue;

    errCachedBlockSlabHash = m_slabHash.ErrRetrieveEntry( &lock, &entry );
    if ( errCachedBlockSlabHash == CCachedBlockSlabHash::ERR::errSuccess )
    {
        psteExisting = entry.Pste();
    }
    else
    {
        Assert( errCachedBlockSlabHash == CCachedBlockSlabHash::ERR::errEntryNotFound );

        Alloc( psteExisting = psteNew = new CCachedBlockSlabTableEntry( ibSlab ) );

        entry = CCachedBlockSlabEntry( psteNew );
        errCachedBlockSlabHash = m_slabHash.ErrInsertEntry( &lock, entry );
        if ( errCachedBlockSlabHash == CCachedBlockSlabHash::ERR::errOutOfMemory )
        {
            Error( ErrERRCheck( JET_errOutOfMemory ) );
        }
        Assert( errCachedBlockSlabHash == CCachedBlockSlabHash::ERR::errSuccess );
        fDeleteEntry = fTrue;
    }

    if ( fWait || !psteExisting->FOwned() )
    {
        psteExisting->AddAsOwnerOrWaiter( &context );
        fDeleteEntry = fFalse;
        psteNew = NULL;

        m_slabHash.WriteUnlockKey( &lock );
        fLocked = fFalse;

        context.Wait();

        *ppste = psteExisting;
    }

HandleError:
    if ( fDeleteEntry )
    {
        errCachedBlockSlabHash = m_slabHash.ErrDeleteEntry( &lock );
        Assert( errCachedBlockSlabHash == CCachedBlockSlabHash::ERR::errSuccess );
    }
    delete psteNew;
    if ( fLocked )
    {
        m_slabHash.WriteUnlockKey( &lock );
    }
    if ( err < JET_errSuccess )
    {
        *ppste = NULL;
    }
    return err;
}

template< class I >
INLINE void TCachedBlockSlabManager<I>::ReleaseSlab( _Inout_ CCachedBlockSlabTableEntry** const ppste )
{
    CCachedBlockSlabTableEntry* const   pste            = *ppste;
    BOOL                                fDeleteEntry    = fFalse;

    *ppste = NULL;

    if ( !pste )
    {
        return;
    }

    CCachedBlockSlabHash::CLock lock;
    m_slabHash.WriteLockKey( CCachedBlockSlabKey( pste ), &lock );

    if ( fDeleteEntry = pste->FRemoveAsOwner() )
    {
        CCachedBlockSlabHash::ERR errCachedBlockSlabHash = m_slabHash.ErrDeleteEntry( &lock );
        Assert( errCachedBlockSlabHash == CCachedBlockSlabHash::ERR::errSuccess );
    }

    m_slabHash.WriteUnlockKey( &lock );

    if ( fDeleteEntry )
    {
        delete pste;
    }
}

template< class I >
INLINE BOOL TCachedBlockSlabManager<I>::FValidCachedBlockId( _In_ const CCachedBlockId& cbid )
{
    if ( cbid.Volumeid() == volumeidInvalid )
    {
        return fFalse;
    }
    if ( cbid.Fileid() == fileidInvalid )
    {
        return fFalse;
    }
    if ( cbid.Fileserial() == fileserialInvalid )
    {
        return fFalse;
    }
    if ( cbid.Cbno() == cbnoInvalid )
    {
        return fFalse;
    }
    return fTrue;
}

template< class I >
INLINE QWORD TCachedBlockSlabManager<I>::IbSlabOfCachedBlockId( _In_ const CCachedBlockId& cbid )
{
    //  NOTE:  PERSISTED:  this hash function is used to locate persistent data in the cache.  changes to this hash
    //  function or its configuration can result in data loss!

    //  we will hash the CachedBlockNumber aligned to the number of sequential cached file blocks that should land in
    //  the same slab

    const DWORD dwHashInput = (DWORD)cbid.Cbno() / (DWORD)m_cCachedFileBlockPerSlab;

    //  use a mixed tabulation hash based on cryptographically random numbers to hash the CachedBlockNumber.  this
    //  allows us to get a good hash function though the input is an integer with non-uniformly used values that are
    //  close to zero.  the quality of this hash function is crucial because it is providing the load balancing for the
    //  cache by distributing the cache request load as evenly as possible over the slabs and because we only allow a
    //  given cached block to be placed in the slab that is selected.

    DWORD dwHash = CMixedTabulationHash::DwHash( dwHashInput );

    //  combine the hash with the FileSerial which is a cryptographically random number

    dwHash ^= (DWORD)cbid.Fileserial();

    //  map the hash into our slab count

    const QWORD iSlab = dwHash % m_cSlab;

    //  convert the hash to a slab offset

    const QWORD ibSlab = m_ibChunk + iSlab * m_cbSlab;

    //  return the target slab

    return ibSlab;
}

template< class I >
ERR TCachedBlockSlabManager<I>::ErrInitSlabTable()
{
    ERR err = JET_errSuccess;

    if ( m_slabHash.ErrInit( 5.0, 1.0 ) == CCachedBlockSlabHash::ERR::errOutOfMemory )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

HandleError:
    return err;
}

template< class I >
void TCachedBlockSlabManager<I>::TermSlabTable()
{
    if ( m_initOnceSlabTable.FIsInit() )
    {
        CCachedBlockSlabHash::CLock lock;
        CCachedBlockSlabEntry       entry;
        CCachedBlockSlabHash::ERR   errSlabHash;

        m_slabHash.BeginHashScan( &lock );

        while ( ( errSlabHash = m_slabHash.ErrMoveNext( &lock ) ) != CCachedBlockSlabHash::ERR::errNoCurrentEntry )
        {
            errSlabHash = m_slabHash.ErrRetrieveEntry( &lock, &entry );
            Assert( errSlabHash == CCachedBlockSlabHash::ERR::errSuccess );

            delete entry.Pste();
        }

        m_slabHash.EndHashScan( &lock );

        m_slabHash.Term();
    }
}


//  CCachedBlockSlabManager:  concrete TCachedBlockSlabManager<ICachedBlockSlabManager>

class CCachedBlockSlabManager  //  cbsm
    :   public TCachedBlockSlabManager<ICachedBlockSlabManager>
{
    public:  //  specialized API

        static ERR ErrInit( _In_    IFileFilter* const                      pff,
                            _In_    const QWORD                             cbCachingFilePerSlab,
                            _In_    const QWORD                             cbCachedFilePerSlab,
                            _In_    const QWORD                             ibChunk,
                            _In_    const QWORD                             cbChunk,
                            _In_    ICachedBlockWriteCountsManager* const   pcbwcm,
                            _In_    const QWORD                             icbwcBase,
                            _In_    const ClusterNumber                     clnoMin,
                            _In_    const ClusterNumber                     clnoMax,
                            _Out_   ICachedBlockSlabManager** const         ppcbsm )
        {
            ERR                         err     = JET_errSuccess;
            CCachedBlockSlabManager*    pcbsm   = NULL;

            *ppcbsm = NULL;

            if ( ibChunk % sizeof( CCachedBlockChunk ) )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            if ( !cbChunk )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            if ( cbChunk % sizeof( CCachedBlockChunk ) )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            //  create the slab manager

            Alloc( pcbsm = new CCachedBlockSlabManager( pff, 
                                                        cbCachingFilePerSlab, 
                                                        cbCachedFilePerSlab, 
                                                        ibChunk, 
                                                        cbChunk,
                                                        pcbwcm,
                                                        icbwcBase,
                                                        clnoMin,
                                                        clnoMax ) );

            //  return the slab manager

            *ppcbsm = pcbsm;
            pcbsm = NULL;

        HandleError:
            delete pcbsm;
            if ( err < JET_errSuccess )
            {
                delete *ppcbsm;
                *ppcbsm = NULL;
            }
            return err;
        }

    private:

        CCachedBlockSlabManager(    _In_    IFileFilter* const                      pff,
                                    _In_    const QWORD                             cbCachingFilePerSlab,
                                    _In_    const QWORD                             cbCachedFilePerSlab,
                                    _In_    const QWORD                             ibChunk,
                                    _In_    const QWORD                             cbChunk,
                                    _In_    ICachedBlockWriteCountsManager* const   pcbwcm,
                                    _In_    const QWORD                             icbwcBase,
                                    _In_    const ClusterNumber                     clnoMin,
                                    _In_    const ClusterNumber                     clnoMax )
            :   TCachedBlockSlabManager<ICachedBlockSlabManager>(   pff,
                                                                    cbCachingFilePerSlab, 
                                                                    cbCachedFilePerSlab,
                                                                    ibChunk,
                                                                    cbChunk,
                                                                    pcbwcm, 
                                                                    icbwcBase,
                                                                    clnoMin,
                                                                    clnoMax )
        {
        }
};


//  CCachedBlockSlabReference:  a reference to an ICachedBlockSlab implementation.

class CCachedBlockSlabReference : public CCachedBlockSlabWrapper
{
    public:  //  specialized API

        CCachedBlockSlabReference(  _In_    TCachedBlockSlabManager<ICachedBlockSlabManager>* const pcbsm,
                                    _Inout_ CCachedBlockSlabTableEntry** const                      ppste,
                                    _In_    CCachedBlockSlab* const                                 pcbs,
                                    _In_    const BOOL                                              fIgnoreVerificationErrors )
            :   CCachedBlockSlabWrapper( pcbs ),
                m_pcbsm( pcbsm ),
                m_pste( *ppste ),
                m_pcbs( pcbs ),
                m_fIgnoreVerificationErrors( fIgnoreVerificationErrors )
        {
            *ppste = NULL;
            m_pcbs->SetIgnoreVerificationErrors( fIgnoreVerificationErrors );
        }

        virtual ~CCachedBlockSlabReference()
        {
            RevertUpdates();
            m_pcbs->SetIgnoreVerificationErrors( fFalse );
            m_pcbsm->RemoveSlabReference( &m_pste );
        }

    private:

        TCachedBlockSlabManager<ICachedBlockSlabManager>* const m_pcbsm;
        CCachedBlockSlabTableEntry*                             m_pste;
        CCachedBlockSlab* const                                 m_pcbs;
        const BOOL                                              m_fIgnoreVerificationErrors;
};
