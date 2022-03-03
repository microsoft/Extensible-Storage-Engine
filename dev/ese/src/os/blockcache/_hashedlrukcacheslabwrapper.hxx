// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  TCachedBlockSlabWrapper:  wrapper of an implementation of ICachedBlockSlab or its derivatives.
//
//  This is a utility class used to enable test dependency injection and as the basis for exposing handles to slabs.

template< class I >
class TCachedBlockSlabWrapper  //  fw
    :   public I
{
    public:  //  specialized API

        TCachedBlockSlabWrapper( _In_ I* const pi )
            :   m_piInner( pi ),
                m_fReleaseOnClose( fFalse )
        {
        }

        TCachedBlockSlabWrapper( _Inout_ I** const ppi )
            :   m_piInner( *ppi ),
                m_fReleaseOnClose( fTrue )
        {
            *ppi = NULL;
        }


        virtual ~TCachedBlockSlabWrapper()
        {
            if ( m_fReleaseOnClose )
            {
                delete m_piInner;
            }
        }

    public:  //  ICachedBlockSlab

        ERR ErrGetPhysicalId( _Out_ QWORD* const pib ) override
        {
            return m_piInner->ErrGetPhysicalId( pib );
        }

        ERR ErrGetSlotForCache( _In_                const CCachedBlockId&   cbid,
                                _In_                const size_t            cb,
                                _In_reads_( cb )    const BYTE* const       rgb,
                                _Out_               CCachedBlockSlot* const pslot ) override
        {
            return m_piInner->ErrGetSlotForCache( cbid, cb, rgb, pslot );
        }

        ERR ErrGetSlotForWrite( _In_                    const CCachedBlockId&   cbid,
                                _In_                    const size_t            cb,
                                _In_reads_opt_( cb )    const BYTE* const       rgb,
                                _Out_                   CCachedBlockSlot* const pslot ) override
        {
            return m_piInner->ErrGetSlotForWrite( cbid, cb, rgb, pslot );
        }

        ERR ErrGetSlotForRead(  _In_    const CCachedBlockId&   cbid,
                                _Out_   CCachedBlockSlot* const pslot ) override
        {
            return m_piInner->ErrGetSlotForRead( cbid, pslot );
        }

        ERR ErrWriteCluster(    _In_                const CCachedBlockSlot&                     slot,
                                _In_                const size_t                                cb,
                                _In_reads_( cb )    const BYTE* const                           rgb,
                                _In_opt_            const ICachedBlockSlab::PfnClusterWritten   pfnClusterWritten,
                                _In_opt_            const DWORD_PTR                             keyClusterWritten,
                                _In_opt_            const ICachedBlockSlab::PfnClusterHandoff   pfnClusterHandoff ) override
        {
            return m_piInner->ErrWriteCluster( slot, cb, rgb, pfnClusterWritten, keyClusterWritten, pfnClusterHandoff );
        }

        ERR ErrUpdateSlot( _In_ const CCachedBlockSlot& slotNew ) override
        {
            return m_piInner->ErrUpdateSlot( slotNew );
        }
        
        ERR ErrReadCluster( _In_                const CCachedBlockSlot&                     slot,
                            _In_                const size_t                                cb,
                            _Out_writes_( cb )  BYTE* const                                 rgb,
                            _In_opt_            const ICachedBlockSlab::PfnClusterRead      pfnClusterRead,
                            _In_opt_            const DWORD_PTR                             keyClusterRead,
                            _In_opt_            const ICachedBlockSlab::PfnClusterHandoff   pfnClusterHandoff ) override
        {
            return m_piInner->ErrReadCluster( slot, cb, rgb, pfnClusterRead, keyClusterRead, pfnClusterHandoff );
        }
        
        ERR ErrVerifyCluster(   _In_                const CCachedBlockSlot& slot,
                                _In_                const size_t            cb,
                                _In_reads_( cb )    const BYTE* const       rgb ) override
        {
            return m_piInner->ErrVerifyCluster( slot, cb, rgb );
        }

        ERR ErrInvalidateSlots( _In_ const ICachedBlockSlab::PfnConsiderUpdate  pfnConsiderUpdate,
                                _In_ const DWORD_PTR                            keyConsiderUpdate ) override
        {
            return m_piInner->ErrInvalidateSlots( pfnConsiderUpdate, keyConsiderUpdate );
        }

        ERR ErrCleanSlots(  _In_ const ICachedBlockSlab::PfnConsiderUpdate  pfnConsiderUpdate,
                            _In_ const DWORD_PTR                            keyConsiderUpdate ) override
        {
            return m_piInner->ErrCleanSlots( pfnConsiderUpdate, keyConsiderUpdate );
        }

        ERR ErrEvictSlots(  _In_ const ICachedBlockSlab::PfnConsiderUpdate  pfnConsiderUpdate,
                            _In_ const DWORD_PTR                            keyConsiderUpdate ) override
        {
            return m_piInner->ErrEvictSlots( pfnConsiderUpdate, keyConsiderUpdate );
        }

        ERR ErrVisitSlots(  _In_ const ICachedBlockSlab::PfnVisitSlot   pfnVisitSlot,
                            _In_ const DWORD_PTR                        keyVisitSlot ) override
        {
            return m_piInner->ErrVisitSlots( pfnVisitSlot, keyVisitSlot );
        }

        BOOL FUpdated() override
        {
            return m_piInner->FUpdated();
        }

        ERR ErrAcceptUpdates() override
        {
            return m_piInner->ErrAcceptUpdates();
        }

        void RevertUpdates() override
        {
            m_piInner->RevertUpdates();
        }

        BOOL FDirty() override
        {
            return m_piInner->FDirty();
        }

        ERR ErrSave(    _In_opt_    const ICachedBlockSlab::PfnSlabSaved    pfnSlabSaved, 
                        _In_opt_    const DWORD_PTR                         keySlabSaved ) override
        {
            return m_piInner->ErrSave( pfnSlabSaved, keySlabSaved );
        }

    protected:

        I* const    m_piInner;
        const BOOL  m_fReleaseOnClose;
};

//  CCachedBlockSlabWrapper:  concrete TCachedBlockSlabWrapper<ICachedBlockSlab>.

class CCachedBlockSlabWrapper : public TCachedBlockSlabWrapper<ICachedBlockSlab>
{
    public:  //  specialized API

        CCachedBlockSlabWrapper( _In_ ICachedBlockSlab* const pcbs )
            :   TCachedBlockSlabWrapper<ICachedBlockSlab>( pcbs )
        {
        }

        CCachedBlockSlabWrapper( _Inout_ ICachedBlockSlab** const ppcbs )
            :   TCachedBlockSlabWrapper<ICachedBlockSlab>( ppcbs )
        {
        }

        virtual ~CCachedBlockSlabWrapper() {}
};
