// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  Journal Entry Type

//PERSISTED
enum class JournalEntryType : BYTE  // jetyp
{
    jetypInvalid,
    jetypCompressed,
    jetypCacheUpdate,
    jetypFlush,
};

constexpr JournalEntryType jetypInvalid = JournalEntryType::jetypInvalid;
constexpr JournalEntryType jetypCompressed = JournalEntryType::jetypCompressed;
constexpr JournalEntryType jetypCacheUpdate = JournalEntryType::jetypCacheUpdate;
constexpr JournalEntryType jetypFlush = JournalEntryType::jetypFlush;

//  Specialize the common journal entry implementation for our journal

typedef TJournalEntry<JournalEntryType> CJournalEntry;
typedef TCompressedJournalEntry<JournalEntryType, jetypCompressed> CCompressedJournalEntry;

//  Cached Block Update

#pragma warning (push)
#pragma warning (disable: 4315)  //  '': 'this' pointer for member '' may not be aligned X as expected by the constructor

#include <pshpack1.h>

//PERSISTED
class CCachedBlockUpdate : public CCachedBlockSlot  //  cbu
{
    public:

        CCachedBlockUpdate()
            :   m_fSlotUpdated( fFalse ),
                m_fClusterReference( fFalse ),
                m_rgbitReserved( 0 )
        {
            C_ASSERT( 56 == sizeof( CCachedBlockUpdate ) );
        }

        CCachedBlockUpdate( _In_ const CCachedBlockSlotState& slotst )
            :   CCachedBlockSlot( slotst ),
                m_fSlotUpdated( slotst.FSlotUpdated() ? 1 : 0 ),
                m_fClusterReference( slotst.FClusterUpdated() ? 1 : 0 ),
                m_rgbitReserved( 0 )
        {
        }

        CCachedBlockUpdate& operator=( const CCachedBlockUpdate& other )
        {
            UtilMemCpy( this, &other, sizeof( *this ) );

            return *this;
        }

        BOOL FSlotUpdated() const { return m_fSlotUpdated != 0; }
        BOOL FClusterReference() const { return m_fClusterReference != 0; }

    private:

        const BYTE  m_fSlotUpdated : 1;         //  the slot was updated
        const BYTE  m_fClusterReference : 1;    //  the associated cluster was updated and is part of the journal
        const BYTE  m_rgbitReserved : 6;        //  reserved; always zero
};

#include <poppack.h>

#pragma warning (pop)

//  Cache Update

#pragma warning (push)
#pragma warning (disable: 4315)  //  '': 'this' pointer for member '' may not be aligned X as expected by the constructor

#include <pshpack1.h>

//PERSISTED
class CCacheUpdateJournalEntry : public TJournalEntryBase<JournalEntryType, jetypCacheUpdate>  //  cuje
{
    public:

        static ERR ErrCreate(   _In_                const ULONG                         ccbu, 
                                _In_reads_( ccbu )  const CCachedBlockUpdate* const     rgcbu, 
                                _Out_               CCacheUpdateJournalEntry** const    ppcuje )
        {
            ERR                         err     = JET_errSuccess;
            void*                       pv      = NULL;
            CCacheUpdateJournalEntry*   pcuje   = NULL;

            *ppcuje = NULL;

            Alloc( pv = PvOSMemoryHeapAlloc( sizeof( CCacheUpdateJournalEntry ) + ccbu * sizeof( CCachedBlockUpdate ) ) );

            pcuje = new( pv ) CCacheUpdateJournalEntry( ccbu, rgcbu );
            pv = NULL;

            *ppcuje = pcuje;
            pcuje = NULL;

        HandleError:
            delete pcuje;
            delete pv;
            if ( err < JET_errSuccess )
            {
                delete *ppcuje;
                *ppcuje = NULL;
            }
            return err;
        }

        ULONG Ccbu() const { return ( Cb() - sizeof( CCacheUpdateJournalEntry ) ) / sizeof( CCachedBlockUpdate ); }
        const CCachedBlockUpdate* Pcbu( _In_ const size_t icbu ) const 
        {
            return icbu < Ccbu() ? &m_rgcbu[ icbu ] : NULL;
        }

    private:

        CCacheUpdateJournalEntry( _In_ const ULONG ccbu, _In_reads_( ccbu ) const CCachedBlockUpdate* const rgcbu )
            :   TJournalEntryBase( sizeof( CCacheUpdateJournalEntry ) + ccbu * sizeof( CCachedBlockUpdate ) ),
                m_rgcbu { }
        {
            UtilMemCpy( m_rgcbu, rgcbu, ccbu * sizeof( rgcbu[ 0 ] ) );
        }

        CCacheUpdateJournalEntry() = delete;

    private:

        CCachedBlockUpdate                  m_rgcbu[ 0 ];   //  cached block updates
};

#include <poppack.h>

#pragma warning (pop)

//  Flush

#include <pshpack1.h>

//PERSISTED
class CFlushJournalEntry : public TJournalEntryBase<JournalEntryType, jetypFlush>  //  fje
{
    public:

        static ERR ErrCreate( _Out_ CFlushJournalEntry** const ppfje )
        {
            ERR                 err     = JET_errSuccess;
            CFlushJournalEntry* pfje    = NULL;

            *ppfje = NULL;

            Alloc( pfje = new CFlushJournalEntry() );

            *ppfje = pfje;
            pfje = NULL;

        HandleError:
            delete pfje;
            if ( err < JET_errSuccess )
            {
                delete *ppfje;
                *ppfje = NULL;
            }
            return err;
        }

    private:

        CFlushJournalEntry()
            :   TJournalEntryBase( sizeof( CFlushJournalEntry ) )
        {
        }
};

#include <poppack.h>
