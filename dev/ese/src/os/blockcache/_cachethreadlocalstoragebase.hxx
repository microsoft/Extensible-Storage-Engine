// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  Cache Thread Id.

enum class CacheThreadId  //  ctid
{
    ctidInvalid = 0,
};

constexpr CacheThreadId ctidInvalid = CacheThreadId::ctidInvalid;

//  A cache thread local storage base class.

class CCacheThreadLocalStorageBase  //  ctls
{
    public:

        CCacheThreadLocalStorageBase( _In_ const CacheThreadId ctid )
            :   m_ctid( ctid ),
                m_cref( 1 )
        {
        }

        CacheThreadId Ctid() const { return m_ctid; }
        UINT UiHash() const { return CCacheThreadLocalStorageBase::UiHash( m_ctid ); }

        BOOL FOwner() { return Ctid() == CtidCurrentThread(); }

        void AddRef() { AddRefInternal(); }

        static void Release( _Inout_ CCacheThreadLocalStorageBase** const ppctls )
        {
            CCacheThreadLocalStorageBase* const pctls = *ppctls;
            
            *ppctls = NULL;

            if ( pctls )
            {
                if ( pctls->FReleaseInternal() )
                {
                    delete pctls;
                }
            }
        }

        static CacheThreadId CtidCurrentThread();
        static BOOL IsThreadAlive( _In_ const CacheThreadId ctid );

        static UINT UiHash( _In_ const CacheThreadId ctid ) { return (UINT)( ctid ); }

        static SIZE_T OffsetOfILE() { return OffsetOf( CCacheThreadLocalStorageBase, m_ile ); }

    protected:

        virtual ~CCacheThreadLocalStorageBase()
        {
            Assert( m_cref == 0 );
        }

    private:

        void AddRefInternal() { AtomicIncrement( (LONG*)&m_cref ); }
        BOOL FReleaseInternal() { return AtomicDecrement( (LONG*)&m_cref ) == 0; }

    private:

        typename CInvasiveList<CCacheThreadLocalStorageBase, OffsetOfILE>::CElement m_ile;
        const CacheThreadId                                                         m_ctid;
        volatile int                                                                m_cref;
};

INLINE CacheThreadId CCacheThreadLocalStorageBase::CtidCurrentThread()
{
    const DWORD         dwTid = GetCurrentThreadId();
    const CacheThreadId ctid = (CacheThreadId)dwTid;

    Assert( ctid != ctidInvalid );

    return ctid;
}

INLINE BOOL CCacheThreadLocalStorageBase::IsThreadAlive( _In_ const CacheThreadId ctid )
{
    const DWORD dwTid = (DWORD)ctid;
    HANDLE      hThread = INVALID_HANDLE_VALUE;
    BOOL        fAlive = fFalse;

    hThread = OpenThread( SYNCHRONIZE, FALSE, dwTid );
    fAlive = hThread && hThread != INVALID_HANDLE_VALUE && WaitForSingleObject( hThread, 0 ) == WAIT_TIMEOUT;

    if ( hThread && hThread != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hThread );
    }

    return fAlive;
}
