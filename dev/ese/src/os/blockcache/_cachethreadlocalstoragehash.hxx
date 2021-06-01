// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  Cache Thread Local Storage key.

class CCacheThreadLocalStorageKey
{
    public:

        CCacheThreadLocalStorageKey()
            :   m_dwTid( 0 )
        {
        }

        CCacheThreadLocalStorageKey( _In_ const DWORD dwTid )
            :   m_dwTid( dwTid )
        {
        }

        CCacheThreadLocalStorageKey( _In_ CCacheThreadLocalStorageKey* const pctls )
            : CCacheThreadLocalStorageKey( pctls->DwTid() )
        {
        }

        CCacheThreadLocalStorageKey( _In_ const CCacheThreadLocalStorageKey& src )
        {
            *this = src;
        }

        const CCacheThreadLocalStorageKey& operator=( _In_ const CCacheThreadLocalStorageKey& src )
        {
            m_dwTid = src.m_dwTid;

            return *this;
        }

        DWORD DwTid() const { return m_dwTid; }
        UINT UiHash() const { return CCacheThreadLocalStorageBase::UiHash( m_dwTid ); }

    private:

        DWORD m_dwTid;
};

//  Cache Thread Local Storage entry.

class CCacheThreadLocalStorageEntry
{
    public:

        CCacheThreadLocalStorageEntry()
            :   m_pctls( NULL ),
                m_uiHash( 0 )
        {
        }

        CCacheThreadLocalStorageEntry( _In_ CCacheThreadLocalStorageBase* const pctls )
            :   m_pctls( pctls ),
                m_uiHash( pctls->UiHash() )
        {
        }

        CCacheThreadLocalStorageEntry( _In_ const CCacheThreadLocalStorageEntry& src )
        {
            *this = src;
        }

        const CCacheThreadLocalStorageEntry& operator=( _In_ const CCacheThreadLocalStorageEntry& src )
        {
            m_pctls = src.m_pctls;
            m_uiHash = src.m_uiHash;

            return *this;
        }

        CCacheThreadLocalStorageBase* Pctls() const { return m_pctls; }
        UINT UiHash() const { return m_uiHash; }

    private:

        CCacheThreadLocalStorageBase*   m_pctls;
        UINT                            m_uiHash;
};

//  Cache Thread Local Storage table.

typedef CDynamicHashTable<CCacheThreadLocalStorageKey, CCacheThreadLocalStorageEntry> CCacheThreadLocalStorageHash;

INLINE typename CCacheThreadLocalStorageHash::NativeCounter CCacheThreadLocalStorageHash::CKeyEntry::Hash( _In_ const CCacheThreadLocalStorageKey& key )
{
    return CCacheThreadLocalStorageHash::NativeCounter( key.UiHash() );
}

INLINE typename CCacheThreadLocalStorageHash::NativeCounter CCacheThreadLocalStorageHash::CKeyEntry::Hash() const
{
    return CCacheThreadLocalStorageHash::NativeCounter( m_entry.UiHash() );
}

INLINE BOOL CCacheThreadLocalStorageHash::CKeyEntry::FEntryMatchesKey( _In_ const CCacheThreadLocalStorageKey& key ) const
{
    if ( m_entry.UiHash() != key.UiHash() )
    {
        return fFalse;
    }

    if ( m_entry.Pctls()->DwTid() != key.DwTid() )
    {
        return fFalse;
    }

    return fTrue;
}

INLINE void CCacheThreadLocalStorageHash::CKeyEntry::SetEntry( _In_ const CCacheThreadLocalStorageEntry& entry )
{
    m_entry = entry;
}

INLINE void CCacheThreadLocalStorageHash::CKeyEntry::GetEntry( _In_ CCacheThreadLocalStorageEntry* const pentry ) const
{
    *pentry = m_entry;
}
