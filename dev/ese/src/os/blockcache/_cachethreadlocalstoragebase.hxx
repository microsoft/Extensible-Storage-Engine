// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  A cache thread local storage base class.

class CCacheThreadLocalStorageBase  //  ctls
{
    public:

        CCacheThreadLocalStorageBase( _In_ const DWORD dwTid )
            :   m_dwTid( dwTid ),
                m_cref( 1 )
        {
        }

        DWORD DwTid() const { return m_dwTid; }
        UINT UiHash() const { return CCacheThreadLocalStorageBase::UiHash( m_dwTid ); }

        void AddRef() { AddRefInternal(); }

        static void Release( _Inout_ CCacheThreadLocalStorageBase** const ppctls )
        {
            CCacheThreadLocalStorageBase* const pctls = *ppctls;
            
            *ppctls = NULL;

            if ( pctls && pctls->FReleaseInternal() )
            {
                delete pctls;
            }
        }

        static UINT UiHash( _In_ const DWORD dwTid ) { return (UINT)( dwTid ); }

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
        const DWORD                                                                 m_dwTid;
        volatile int                                                                m_cref;
};
