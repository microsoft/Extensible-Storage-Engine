// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  Slab Write Back context

class CSlabWriteBack  //  swb
{
    public:

        typedef void (*PfnSaved)(   _In_ const ERR                  errSave,
                                    _In_ CSlabWriteBack* const      pswb,
                                    _In_ ICachedBlockSlab* const    pcbs,
                                    _In_ const BOOL                 fReleaseSlabOnSave,
                                    _In_ const DWORD_PTR            keySaved );

        CSlabWriteBack( _In_ const QWORD ibSlab )
            :   m_ibSlab( ibSlab ),
                m_jposMin( jposInvalid ),
                m_jposLast( jposInvalid ),
                m_jposEndLast( jposInvalid ),
                m_pfnSaved( NULL ),
                m_keySaved( NULL ),
                m_pcbsSave( NULL ),
                m_fReleaseSlabOnSave( fFalse ),
                m_sigSave( CSyncBasicInfo( "THashedLRUKCache<I>::CSlabWriteBack::m_sigSave" ) )
        {
            m_sigSave.Set();
        }

        ~CSlabWriteBack()
        {
            m_sigSave.Wait();
            if ( m_fReleaseSlabOnSave )
            {
                delete m_pcbsSave;
            }
        }

        QWORD IbSlab() const { return m_ibSlab; }
        BOOL FDirty() const { return m_jposMin != jposInvalid; }
        JournalPosition JposMin() const { return m_jposMin; }
        JournalPosition JposLast() const { return m_jposLast; }
        JournalPosition JposEndLast() const { return m_jposEndLast; }
        BOOL FSavePending() const { return !m_ileSave.FUninitialized(); }

        void RegisterWrite( _In_ ICachedBlockSlab* const    pcbs,
                            _In_ const JournalPosition      jpos,
                            _In_ const JournalPosition      jposEnd )
        {
            Assert( jpos < jposEnd );

            m_jposMin = m_jposMin == jposInvalid ? jpos : min( m_jposMin, jpos );
            m_jposLast = m_jposLast == jposInvalid ? jpos : max( m_jposLast, jpos );
            m_jposEndLast = m_jposEndLast == jposInvalid ? jposEnd : max( m_jposEndLast, jposEnd );
        }

        ERR ErrSave(    _In_    const CSlabWriteBack::PfnSaved  pfnSaved,
                        _In_    const DWORD_PTR                 keySaved,
                        _Inout_ ICachedBlockSlab** const        ppcbs,
                        _In_    const BOOL                      fReleaseSlabOnSave );

        static SIZE_T OffsetOfILEJposMin() { return OffsetOf( CSlabWriteBack, m_ileJposMin ); }
        static SIZE_T OffsetOfILESave() { return OffsetOf( CSlabWriteBack, m_ileSave ); }

    private:

        static void Saved_( _In_ const ERR errSave, _In_ const DWORD_PTR keySaved )
        {
            ICachedBlockSlab::PfnSlabSaved pfnSlabSaved = Saved_;
            Unused( pfnSlabSaved );

            CSlabWriteBack* const pswb = (CSlabWriteBack*)keySaved;
            pswb->Saved( errSave );
        }

        void Saved( _In_ const ERR errSave );

    private:

        typename CCountedInvasiveList<CSlabWriteBack, OffsetOfILEJposMin>::CElement m_ileJposMin;
        typename CCountedInvasiveList<CSlabWriteBack, OffsetOfILESave>::CElement    m_ileSave;
        const QWORD                                                                 m_ibSlab;
        JournalPosition                                                             m_jposMin;
        JournalPosition                                                             m_jposLast;
        JournalPosition                                                             m_jposEndLast;
        PfnSaved                                                                    m_pfnSaved;
        DWORD_PTR                                                                   m_keySaved;
        ICachedBlockSlab*                                                           m_pcbsSave;
        BOOL                                                                        m_fReleaseSlabOnSave;
        CManualResetSignal                                                          m_sigSave;
};

INLINE ERR CSlabWriteBack::ErrSave( _In_    const CSlabWriteBack::PfnSaved  pfnSaved,
                                    _In_    const DWORD_PTR                 keySaved,
                                    _Inout_ ICachedBlockSlab** const        ppcbs,
                                    _In_    const BOOL                      fReleaseSlabOnSave )
{
    ERR                 err         = JET_errSuccess;
    ICachedBlockSlab*   pcbs        = *ppcbs;
    BOOL                fStarted    = fFalse;

    *ppcbs = NULL;

    //  capture our completion

    m_pfnSaved = pfnSaved;
    m_keySaved = keySaved;

    //  capture the slab

    m_pcbsSave = pcbs;
    pcbs = NULL;
    m_fReleaseSlabOnSave = fReleaseSlabOnSave;

    //  save the slab

    m_sigSave.Reset();
    Call( m_pcbsSave->ErrSave( Saved_, (DWORD_PTR)this ) );
    fStarted = fTrue;

HandleError:
    delete pcbs;
    if ( !fStarted && err < JET_errSuccess )
    {
        //  ensure a completion is fired

        Saved( err );
    }
    return err;
}

INLINE void CSlabWriteBack::Saved( _In_ const ERR errSave )
{
    m_pfnSaved( errSave, this, m_pcbsSave, m_fReleaseSlabOnSave, m_keySaved );
    m_pcbsSave = NULL;

    m_sigSave.Set();
}

//  Slab Write Back table key.

class CSlabWriteBackKey
{
    public:

        CSlabWriteBackKey()
            :   m_ibSlab( qwMax )
        {
        }

        CSlabWriteBackKey( _In_ const QWORD ibSlab )
            :   m_ibSlab( ibSlab )
        {
        }

        CSlabWriteBackKey( _In_ const CSlabWriteBackKey& src )
        {
            *this = src;
        }

        const CSlabWriteBackKey& operator=( _In_ const CSlabWriteBackKey& src )
        {
            m_ibSlab = src.m_ibSlab;

            return *this;
        }

        QWORD IbSlab() const { return m_ibSlab; }
        UINT UiHash() const { return UiHash( m_ibSlab ); }

        static UINT UiHash( _In_ const QWORD ibSlab ) { return (UINT)( ibSlab / sizeof( CCachedBlockChunk ) ); }

    private:

        QWORD   m_ibSlab;
};

//  Slab Write Back table entry.

class CSlabWriteBackEntry
{
    public:

        CSlabWriteBackEntry()
            :   m_ibSlab( qwMax ),
                m_pswb( NULL )
        {
        }

        CSlabWriteBackEntry( _In_ const QWORD ibSlab, _In_ CSlabWriteBack* const pswb )
            :   m_ibSlab( ibSlab ),
                m_pswb( pswb )
        {
        }

        CSlabWriteBackEntry( _In_ const CSlabWriteBackEntry& src )
        {
            *this = src;
        }

        const CSlabWriteBackEntry& operator=( _In_ const CSlabWriteBackEntry& src )
        {
            m_ibSlab = src.m_ibSlab;
            m_pswb = src.m_pswb;

            return *this;
        }

        QWORD IbSlab() const { return m_ibSlab; }
        CSlabWriteBack* Pswb() const { return m_pswb; }
        UINT UiHash() const { return CSlabWriteBackKey::UiHash( m_ibSlab ); }

    private:

        QWORD           m_ibSlab;
        CSlabWriteBack* m_pswb;
};

//  Slab Write Back hash table.

typedef CDynamicHashTable<CSlabWriteBackKey, CSlabWriteBackEntry> CSlabWriteBackHash;

INLINE typename CSlabWriteBackHash::NativeCounter CSlabWriteBackHash::CKeyEntry::Hash( _In_ const CSlabWriteBackKey& key )
{
    return CSlabWriteBackHash::NativeCounter( key.UiHash() );
}

INLINE typename CSlabWriteBackHash::NativeCounter CSlabWriteBackHash::CKeyEntry::Hash() const
{
    return CSlabWriteBackHash::NativeCounter( m_entry.UiHash() );
}

INLINE BOOL CSlabWriteBackHash::CKeyEntry::FEntryMatchesKey( _In_ const CSlabWriteBackKey& key ) const
{
    if ( m_entry.IbSlab() != key.IbSlab() )
    {
        return fFalse;
    }

    return fTrue;
}

INLINE void CSlabWriteBackHash::CKeyEntry::SetEntry( _In_ const CSlabWriteBackEntry& entry )
{
    m_entry = entry;
}

INLINE void CSlabWriteBackHash::CKeyEntry::GetEntry( _In_ CSlabWriteBackEntry* const pentry ) const
{
    *pentry = m_entry;
}
