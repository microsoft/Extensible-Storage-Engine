// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  CIOComplete table key.

class CIOCompleteKey
{
    public:

        CIOCompleteKey()
            :   m_piocomplete( NULL )
        {
        }

        CIOCompleteKey( _In_ const void* const piocomplete )
            :   m_piocomplete( piocomplete )
        {
        }

        CIOCompleteKey( _In_ const CIOCompleteKey& src )
        {
            *this = src;
        }

        const CIOCompleteKey& operator=( _In_ const CIOCompleteKey& src )
        {
            m_piocomplete = src.m_piocomplete;

            return *this;
        }

        const void* Piocomplete() const { return m_piocomplete; }
        UINT UiHash() const { return CIOCompleteKey::UiHash( m_piocomplete ); }

        static UINT UiHash( _In_ const void* const piocomplete ) { return (UINT)( (UINT_PTR)piocomplete / sizeof( UINT_PTR ) ); }

    private:

        const void* m_piocomplete;
};

//  Cached block slab table entry.

class CIOCompleteEntry
{
    public:

        CIOCompleteEntry()
            :   m_piocomplete( NULL )
        {
        }

        CIOCompleteEntry( _In_ const void* const piocomplete )
            :   m_piocomplete( piocomplete )
        {
        }

        CIOCompleteEntry( _In_ const CIOCompleteEntry& src )
        {
            *this = src;
        }

        const CIOCompleteEntry& operator=( _In_ const CIOCompleteEntry& src )
        {
            m_piocomplete = src.m_piocomplete;

            return *this;
        }

        const void* Piocomplete() const { return m_piocomplete; }
        UINT UiHash() const { return CIOCompleteKey::UiHash( m_piocomplete ); }

    private:

        const void* m_piocomplete;
};

//  Cached file hash table.

typedef CDynamicHashTable<CIOCompleteKey, CIOCompleteEntry> CIOCompleteHash;

INLINE CIOCompleteHash::NativeCounter CIOCompleteHash::CKeyEntry::Hash( _In_ const CIOCompleteKey& key )
{
    return CIOCompleteHash::NativeCounter( key.UiHash() );
}

INLINE CIOCompleteHash::NativeCounter CIOCompleteHash::CKeyEntry::Hash() const
{
    return CIOCompleteHash::NativeCounter( m_entry.UiHash() );
}

INLINE BOOL CIOCompleteHash::CKeyEntry::FEntryMatchesKey( _In_ const CIOCompleteKey& key ) const
{
    if ( m_entry.Piocomplete() != key.Piocomplete() )
    {
        return fFalse;
    }

    return fTrue;
}

INLINE void CIOCompleteHash::CKeyEntry::SetEntry( _In_ const CIOCompleteEntry& entry )
{
    m_entry = entry;
}

INLINE void CIOCompleteHash::CKeyEntry::GetEntry( _In_ CIOCompleteEntry* const pentry ) const
{
    *pentry = m_entry;
}

template<>
INLINE ERR ErrToErr<CIOCompleteHash>( _In_ const typename CIOCompleteHash::ERR err )
{
    switch ( err )
    {
        case CIOCompleteHash::ERR::errSuccess:
            return JET_errSuccess;
        case CIOCompleteHash::ERR::errOutOfMemory:
            return ErrERRCheck( JET_errOutOfMemory );
        case CIOCompleteHash::ERR::errInvalidParameter:
            return ErrERRCheck( JET_errInvalidParameter );
        case CIOCompleteHash::ERR::errEntryNotFound:
            return ErrERRCheck( JET_errRecordNotFound );
        case CIOCompleteHash::ERR::errNoCurrentEntry:
            return ErrERRCheck( JET_errNoCurrentRecord );
        case CIOCompleteHash::ERR::errKeyDuplicate:
            return ErrERRCheck( JET_errKeyDuplicate );
        default:
            Assert( fFalse );
            return ErrERRCheck( JET_errInternalError );
    }
}
