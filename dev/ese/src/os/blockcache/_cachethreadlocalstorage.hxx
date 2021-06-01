// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  TCacheThreadLocalStorage:  provides thread local storage.
//
//  CTLS:  CCacheThreadLocalStorageBase or derivative

template< class CTLS >
class TCacheThreadLocalStorage
{
    public:

        TCacheThreadLocalStorage()
            :   m_cacheThreadLocalStorage( rankCacheThreadLocalStorage ),
                m_critThreadLocalStorage( CLockBasicInfo( CSyncBasicInfo( "TCacheThreadLocalStorage<CTLS>::m_critThreadLocalStorage" ), rankCacheThreadLocalStorage, 0 ) ),
                m_pctlsCleanup( NULL )
        {
        }

        ~TCacheThreadLocalStorage()
        {
            TermThreadLocalStorageTable();
        }

        ERR ErrGetThreadLocalStorage( _Out_ CTLS** const ppctls )
        {
            return ErrGetThreadLocalStorageInternal( ppctls );
        }

    private:

        BOOL FThreadLocalStorageTableIsInit() const { return m_initOnceThreadLocalStorageTable.FIsInit(); }
        ERR ErrEnsureInitThreadLocalStorageTable() { return m_initOnceThreadLocalStorageTable.Init( ErrInitThreadLocalStorageTable_, this ); };
        ERR ErrInitThreadLocalStorageTable();
        static ERR ErrInitThreadLocalStorageTable_( _In_ TCacheThreadLocalStorage* const pc ) { return pc->ErrInitThreadLocalStorageTable(); }
        void TermThreadLocalStorageTable();

        ERR ErrGetThreadLocalStorageInternal( _Out_ CTLS** const ppctls );
        ERR ErrGetThreadLocalStorageSlowly( _Out_ CTLS** const ppctls );
        void TryReleaseThreadLocalStorage( _In_ const DWORD dwTid );
        DWORD DwGetCurrentThreadId();
        BOOL IsThreadAlive( _In_ const DWORD dwTid );

    private:

        typedef CInitOnce<ERR, decltype( &ErrInitThreadLocalStorageTable_ ), TCacheThreadLocalStorage* const> CInitOnceThreadLocalStorageTable;

        CInitOnceThreadLocalStorageTable                                        m_initOnceThreadLocalStorageTable;
        CCacheThreadLocalStorageHash                                            m_cacheThreadLocalStorage;
        CCriticalSection                                                        m_critThreadLocalStorage;
        typename CInvasiveList<CTLS, CCacheThreadLocalStorageBase::OffsetOfILE> m_ilThreadLocalStorage;
        CTLS*                                                                   m_pctlsCleanup;
};

template< class CTLS >
ERR TCacheThreadLocalStorage<CTLS>::ErrInitThreadLocalStorageTable()
{
    ERR err = JET_errSuccess;

    if ( m_cacheThreadLocalStorage.ErrInit( 5.0, 1.0 ) == CCacheThreadLocalStorageHash::ERR::errOutOfMemory )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

HandleError:
    return err;
}

template< class CTLS >
void TCacheThreadLocalStorage<CTLS>::TermThreadLocalStorageTable()
{
    if ( FThreadLocalStorageTableIsInit() )
    {
        CCacheThreadLocalStorageHash::CLock lock;
        CCacheThreadLocalStorageEntry       entry;
        CCacheThreadLocalStorageHash::ERR   errCacheThreadLocalStorageHash;

        m_cacheThreadLocalStorage.BeginHashScan( &lock );

        while ( ( errCacheThreadLocalStorageHash = m_cacheThreadLocalStorage.ErrMoveNext( &lock ) ) != CCacheThreadLocalStorageHash::ERR::errNoCurrentEntry )
        {
            errCacheThreadLocalStorageHash = m_cacheThreadLocalStorage.ErrRetrieveEntry( &lock, &entry );
            Assert( errCacheThreadLocalStorageHash == CCacheThreadLocalStorageHash::ERR::errSuccess );

            CTLS* pctlsExisting = static_cast<CTLS*>( entry.Pctls() );
            CTLS::Release( &pctlsExisting );
        }

        m_cacheThreadLocalStorage.EndHashScan( &lock );

        m_cacheThreadLocalStorage.Term();
    }
}

template< class CTLS >
ERR TCacheThreadLocalStorage<CTLS>::ErrGetThreadLocalStorageInternal( _Out_ CTLS** const ppctls )
{
    ERR                                 err                             = JET_errSuccess;
    CCacheThreadLocalStorageKey         key( DwGetCurrentThreadId() );
    CCacheThreadLocalStorageEntry       entry;
    CCacheThreadLocalStorageHash::CLock lock;
    CCacheThreadLocalStorageHash::ERR   errCacheThreadLocalStorageHash  = CCacheThreadLocalStorageHash::ERR::errSuccess;
    BOOL                                fReadLocked                     = fFalse;
    CTLS*                               pctlsExisting                   = NULL;

    *ppctls = NULL;

    Call( ErrEnsureInitThreadLocalStorageTable() );

    m_cacheThreadLocalStorage.ReadLockKey( key, &lock );
    fReadLocked = fTrue;

    errCacheThreadLocalStorageHash = m_cacheThreadLocalStorage.ErrRetrieveEntry( &lock, &entry );
    if ( errCacheThreadLocalStorageHash == CCacheThreadLocalStorageHash::ERR::errSuccess )
    {
        pctlsExisting = static_cast<CTLS*>( entry.Pctls() );
        pctlsExisting->AddRef();
    }
    else
    {
        Assert( errCacheThreadLocalStorageHash == CCacheThreadLocalStorageHash::ERR::errEntryNotFound );
    }

    if ( fReadLocked )
    {
        m_cacheThreadLocalStorage.ReadUnlockKey( &lock );
        fReadLocked = fFalse;
    }

    *ppctls = pctlsExisting;
    pctlsExisting = NULL;

    if ( !(*ppctls) )
    {
        Call( ErrGetThreadLocalStorageSlowly( ppctls ) );
    }

HandleError:
    if ( fReadLocked )
    {
        m_cacheThreadLocalStorage.ReadUnlockKey( &lock );
    }
    if ( err < JET_errSuccess )
    {
        CTLS::Release( ppctls );
    }
    return err;
}

template< class CTLS >
ERR TCacheThreadLocalStorage<CTLS>::ErrGetThreadLocalStorageSlowly( _Out_ CTLS** const ppctls )
{
    ERR                                 err                                 = JET_errSuccess;
    CCacheThreadLocalStorageKey         key( DwGetCurrentThreadId() );
    CCacheThreadLocalStorageEntry       entry;
    CCacheThreadLocalStorageHash::CLock lock;
    CCacheThreadLocalStorageHash::ERR   errCacheThreadLocalStorageHash      = CCacheThreadLocalStorageHash::ERR::errSuccess;
    CTLS*                               pctlsExisting                       = NULL;
    BOOL                                fWriteLocked                        = fFalse;
    CTLS*                               pctlsNew                            = NULL;
    const int                           cdwTidCleanupMax                    = 2;
    DWORD                               rgdwTidCleanup[ cdwTidCleanupMax ]  = { };
    int                                 cdwTidCleanup                       = 0;

    *ppctls = NULL;

    m_cacheThreadLocalStorage.WriteLockKey( key, &lock );
    fWriteLocked = fTrue;

    errCacheThreadLocalStorageHash = m_cacheThreadLocalStorage.ErrRetrieveEntry( &lock, &entry );
    if ( errCacheThreadLocalStorageHash == CCacheThreadLocalStorageHash::ERR::errSuccess )
    {
        pctlsExisting = static_cast<CTLS*>( entry.Pctls() );
    }
    else
    {
        Assert( errCacheThreadLocalStorageHash == CCacheThreadLocalStorageHash::ERR::errEntryNotFound );

        Alloc( pctlsNew = new CTLS( key.DwTid() ) );
        pctlsExisting = pctlsNew;

        entry = CCacheThreadLocalStorageEntry( pctlsNew );
        errCacheThreadLocalStorageHash = m_cacheThreadLocalStorage.ErrInsertEntry( &lock, entry );
        if ( errCacheThreadLocalStorageHash == CCacheThreadLocalStorageHash::ERR::errOutOfMemory )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }
        Assert( errCacheThreadLocalStorageHash == CCacheThreadLocalStorageHash::ERR::errSuccess );
    }

    pctlsNew = NULL;
    pctlsExisting->AddRef();

    if ( fWriteLocked )
    {
        m_cacheThreadLocalStorage.WriteUnlockKey( &lock );
        fWriteLocked = fFalse;
    }

    if ( !m_ilThreadLocalStorage.FMember( pctlsExisting ) )
    {
        m_critThreadLocalStorage.Enter();

        for (   CTLS* pctlsCleanup = m_pctlsCleanup ? m_pctlsCleanup : m_ilThreadLocalStorage.PrevMost();
                pctlsCleanup && cdwTidCleanup < cdwTidCleanupMax;
                pctlsCleanup = m_pctlsCleanup )
        {
            m_pctlsCleanup = m_ilThreadLocalStorage.Next( pctlsCleanup );
            rgdwTidCleanup[cdwTidCleanup++] = pctlsCleanup->DwTid();
        }

        m_ilThreadLocalStorage.InsertAsNextMost( pctlsExisting );

        m_critThreadLocalStorage.Leave();
    }

    for ( int idwTidCleanup = 0; idwTidCleanup < cdwTidCleanup; idwTidCleanup++ )
    {
        TryReleaseThreadLocalStorage( rgdwTidCleanup[ idwTidCleanup ] );
    }

    *ppctls = pctlsExisting;

HandleError:
    CTLS::Release( &pctlsNew );
    if ( fWriteLocked )
    {
        m_cacheThreadLocalStorage.WriteUnlockKey( &lock );
    }
    if ( err < JET_errSuccess )
    {
        CTLS::Release( ppctls );
    }
    return err;
}

template< class CTLS >
void TCacheThreadLocalStorage<CTLS>::TryReleaseThreadLocalStorage( _In_ const DWORD dwTid )
{
    CCacheThreadLocalStorageKey         key( dwTid );
    CCacheThreadLocalStorageEntry       entry;
    CCacheThreadLocalStorageHash::CLock lock;
    CCacheThreadLocalStorageHash::ERR   errCacheThreadLocalStorageHash = CCacheThreadLocalStorageHash::ERR::errSuccess;
    CTLS*                               pctlsExisting                   = NULL;
    BOOL                                fRelease                        = fFalse;

    if ( IsThreadAlive( key.DwTid() ) )
    {
        return;
    }

    m_cacheThreadLocalStorage.WriteLockKey( key, &lock );

    errCacheThreadLocalStorageHash = m_cacheThreadLocalStorage.ErrRetrieveEntry( &lock, &entry );
    if ( errCacheThreadLocalStorageHash == CCacheThreadLocalStorageHash::ERR::errSuccess )
    {
        pctlsExisting = static_cast<CTLS*>( entry.Pctls() );

        fRelease = !IsThreadAlive( pctlsExisting->DwTid() );

        if ( fRelease )
        {
            errCacheThreadLocalStorageHash = m_cacheThreadLocalStorage.ErrDeleteEntry( &lock );
            Assert( errCacheThreadLocalStorageHash == CCacheThreadLocalStorageHash::ERR::errSuccess );
        }
    }
    else
    {
        Assert( errCacheThreadLocalStorageHash == CCacheThreadLocalStorageHash::ERR::errEntryNotFound );
    }

    m_cacheThreadLocalStorage.WriteUnlockKey( &lock );

    if ( fRelease )
    {
        m_critThreadLocalStorage.Enter();
        m_pctlsCleanup = m_pctlsCleanup == pctlsExisting ? m_ilThreadLocalStorage.Next( pctlsExisting ) : m_pctlsCleanup;
        m_ilThreadLocalStorage.Remove( pctlsExisting );
        m_critThreadLocalStorage.Leave();

        CTLS::Release( &pctlsExisting );
    }
}

template< class CTLS >
DWORD TCacheThreadLocalStorage<CTLS>::DwGetCurrentThreadId()
{
    return GetCurrentThreadId();
}

template< class CTLS >
BOOL TCacheThreadLocalStorage<CTLS>::IsThreadAlive( _In_ const DWORD dwTid )
{
    HANDLE  hThread = INVALID_HANDLE_VALUE;
    BOOL    fAlive  = fFalse;

    hThread = OpenThread( SYNCHRONIZE, FALSE, dwTid );
    fAlive = hThread != INVALID_HANDLE_VALUE && WaitForSingleObject( hThread, 0 ) == WAIT_TIMEOUT;

    if ( hThread != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hThread );
    }

    return fAlive;
}
