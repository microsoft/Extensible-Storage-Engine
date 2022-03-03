// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  Cached block slab table entry.

class CCachedBlockSlabTableEntry //  ste
{
    public:

        CCachedBlockSlabTableEntry( _In_ const QWORD ibSlab )
            :   m_ibSlab( ibSlab ),
                m_cref( 0 ),
                m_fOwned( fFalse ),
                m_pcbs( NULL )
        {
        }

        ~CCachedBlockSlabTableEntry()
        {
            Assert( !m_cref );
            Assert( !m_fOwned );
            Assert( m_ilContextWaiters.FEmpty() );
            delete m_pcbs;
            m_pcbs = NULL;
        }

        QWORD IbSlab() const { return m_ibSlab; }
        UINT UiHash() const { return CCachedBlockSlabTableEntry::UiHash( m_ibSlab ); }
        BOOL FOwned() const { return m_fOwned; }

        CCachedBlockSlab* Pcbs() const { return m_pcbs; }

        void OpenSlab( _Inout_ CCachedBlockSlab** const ppcbs )
        {
            m_pcbs = *ppcbs;
            *ppcbs = NULL;
        }

        static UINT UiHash( _In_ const QWORD ibSlab ) { return (UINT)( ibSlab / cbCachedBlock ); }

    public:

        //  Context for acquiring the slab.

        class CContext
        {
            public:

                CContext()
                    :   m_sem( CSyncBasicInfo( "CCachedBlockSlabTableEntry::CContext::m_sem" ) )
                {
                }

                void Wait() { m_sem.Acquire(); }
                void Release() { m_sem.Release(); }

                static SIZE_T OffsetOfILE() { return OffsetOf( CContext, m_ile ); }

            private:

                typename CInvasiveList<CContext, OffsetOfILE>::CElement m_ile;
                CSemaphore                                              m_sem;
        };

        void AddAsOwnerOrWaiter( _In_ CContext* const pcontext )
        {
            if ( !m_fOwned )
            {
                m_fOwned = fTrue;
                pcontext->Release();
            }
            else
            {
                m_ilContextWaiters.InsertAsNextMost( pcontext );
            }

            m_cref++;
        }

        BOOL FRemoveAsOwner()
        {
            CContext* const pcontextNext = m_ilContextWaiters.PrevMost();
            if ( pcontextNext != NULL )
            {
                m_ilContextWaiters.Remove( pcontextNext );
                pcontextNext->Release();
            }

            m_fOwned = pcontextNext != NULL;

            m_cref--;

            //  always retain dirty slabs in the slab table

            if ( m_cref == 0 && m_pcbs && m_pcbs->FDirty() )
            {
                return fFalse;
            }

            return m_cref == 0;
        }

    private:

        const QWORD                                     m_ibSlab;
        int                                             m_cref;
        BOOL                                            m_fOwned;
        CInvasiveList<CContext, CContext::OffsetOfILE>  m_ilContextWaiters;
        CCachedBlockSlab*                               m_pcbs;
};

//  Cached block slab table key.

class CCachedBlockSlabKey
{
    public:

        CCachedBlockSlabKey()
            :   m_ibSlab( 0 )
        {
        }

        CCachedBlockSlabKey( _In_ const QWORD ibSlab )
            :   m_ibSlab( ibSlab )
        {
        }

        CCachedBlockSlabKey( _In_ CCachedBlockSlabTableEntry* const pste )
            :   CCachedBlockSlabKey( pste->IbSlab() )
        {
        }

        CCachedBlockSlabKey( _In_ const CCachedBlockSlabKey& src )
        {
            *this = src;
        }

        const CCachedBlockSlabKey& operator=( _In_ const CCachedBlockSlabKey& src )
        {
            m_ibSlab = src.m_ibSlab;

            return *this;
        }

        QWORD IbSlab() const { return m_ibSlab; }
        UINT UiHash() const { return CCachedBlockSlabTableEntry::UiHash( m_ibSlab ); }

    private:

        QWORD m_ibSlab;
};

//  Cached block slab table entry.

class CCachedBlockSlabEntry
{
    public:

        CCachedBlockSlabEntry()
            :   m_pste( NULL ),
                m_uiHash( 0 )
        {
        }

        CCachedBlockSlabEntry( _In_ CCachedBlockSlabTableEntry* const pste )
            :   m_pste( pste ),
                m_uiHash( pste->UiHash() )
        {
        }

        CCachedBlockSlabEntry( _In_ const CCachedBlockSlabEntry& src )
        {
            *this = src;
        }

        const CCachedBlockSlabEntry& operator=( _In_ const CCachedBlockSlabEntry& src )
        {
            m_pste = src.m_pste;
            m_uiHash = src.m_uiHash;

            return *this;
        }

        CCachedBlockSlabTableEntry* Pste() const { return m_pste; }
        UINT UiHash() const { return m_uiHash; }

    private:

        CCachedBlockSlabTableEntry* m_pste;
        UINT                        m_uiHash;
};

//  Cached file hash table.

typedef CDynamicHashTable<CCachedBlockSlabKey, CCachedBlockSlabEntry> CCachedBlockSlabHash;

INLINE typename CCachedBlockSlabHash::NativeCounter CCachedBlockSlabHash::CKeyEntry::Hash( _In_ const CCachedBlockSlabKey& key )
{
    return CCachedBlockSlabHash::NativeCounter( key.UiHash() );
}

INLINE typename CCachedBlockSlabHash::NativeCounter CCachedBlockSlabHash::CKeyEntry::Hash() const
{
    return CCachedBlockSlabHash::NativeCounter( m_entry.UiHash() );
}

INLINE BOOL CCachedBlockSlabHash::CKeyEntry::FEntryMatchesKey( _In_ const CCachedBlockSlabKey& key ) const
{
    if ( m_entry.UiHash() != key.UiHash() )
    {
        return fFalse;
    }

    if ( m_entry.Pste()->IbSlab() != key.IbSlab() )
    {
        return fFalse;
    }

    return fTrue;
}

INLINE void CCachedBlockSlabHash::CKeyEntry::SetEntry( _In_ const CCachedBlockSlabEntry& entry )
{
    m_entry = entry;
}

INLINE void CCachedBlockSlabHash::CKeyEntry::GetEntry( _In_ CCachedBlockSlabEntry* const pentry ) const
{
    *pentry = m_entry;
}
