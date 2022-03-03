// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  Cluster reference table key.

class CClusterReferenceKey
{
    public:

        CClusterReferenceKey()
            :   m_clno( clnoInvalid )
        {
        }

        CClusterReferenceKey( _In_ const ClusterNumber clno )
            :   m_clno( clno )
        {
        }

        CClusterReferenceKey( _In_ const CClusterReferenceKey& src )
        {
            *this = src;
        }

        const CClusterReferenceKey& operator=( _In_ const CClusterReferenceKey& src )
        {
            m_clno = src.m_clno;

            return *this;
        }

        ClusterNumber Clno() const { return m_clno; }
        UINT UiHash() const { return UiHash( m_clno ); }

        static UINT UiHash( _In_ const ClusterNumber clno ) { return (UINT)clno; }

    private:

        ClusterNumber   m_clno;
};

//  Cluster reference table entry.

class CClusterReferenceEntry
{
    public:

        CClusterReferenceEntry()
            :   m_clno( clnoInvalid ),
                m_err( JET_errSuccess ),
                m_jpos( jposInvalid )
        {
        }

        CClusterReferenceEntry( _In_ const CClusterReferenceKey key,
                                _In_ const ERR                  err,
                                _In_ const JournalPosition      jpos )
            :   m_clno( key.Clno() ),
                m_err( err ),
                m_jpos( jpos )
        {
        }

        CClusterReferenceEntry( _In_ const CClusterReferenceEntry& src )
        {
            *this = src;
        }

        const CClusterReferenceEntry& operator=( _In_ const CClusterReferenceEntry& src )
        {
            m_clno = src.m_clno;
            m_err = src.m_err;
            m_jpos = src.m_jpos;

            return *this;
        }

        ClusterNumber Clno() const { return m_clno; }
        ERR Err() const { return m_err; }
        JournalPosition Jpos() const { return m_jpos; }
        UINT UiHash() const { return CClusterReferenceKey::UiHash( m_clno ); }

    private:

        ClusterNumber   m_clno;
        ERR             m_err;
        JournalPosition m_jpos;
};

//  Cluster reference hash table.

typedef CDynamicHashTable<CClusterReferenceKey, CClusterReferenceEntry> CClusterReferenceHash;

INLINE typename CClusterReferenceHash::NativeCounter CClusterReferenceHash::CKeyEntry::Hash( _In_ const CClusterReferenceKey& key )
{
    return CClusterReferenceHash::NativeCounter( key.UiHash() );
}

INLINE typename CClusterReferenceHash::NativeCounter CClusterReferenceHash::CKeyEntry::Hash() const
{
    return CClusterReferenceHash::NativeCounter( m_entry.UiHash() );
}

INLINE BOOL CClusterReferenceHash::CKeyEntry::FEntryMatchesKey( _In_ const CClusterReferenceKey& key ) const
{
    if ( m_entry.Clno() != key.Clno() )
    {
        return fFalse;
    }

    return fTrue;
}

INLINE void CClusterReferenceHash::CKeyEntry::SetEntry( _In_ const CClusterReferenceEntry& entry )
{
    m_entry = entry;
}

INLINE void CClusterReferenceHash::CKeyEntry::GetEntry( _In_ CClusterReferenceEntry* const pentry ) const
{
    *pentry = m_entry;
}
