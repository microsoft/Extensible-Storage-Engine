// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "stdafx.h"

#pragma once

namespace Internal
{
    namespace Ese
    {
        namespace BlockCache
        {
            namespace Interop
            {
                template< class TM, class TN >
                class CCachedBlockSlabWrapper : public CWrapper<TM, TN>
                {
                    public:

                        CCachedBlockSlabWrapper( TM^ cbs ) : CWrapper( cbs ) { }

                    public:

                        ERR ErrGetPhysicalId( _Out_ QWORD* const pib ) override;

                        ERR ErrGetSlotForCache( _In_                const ::CCachedBlockId&     cbid,
                                                _In_                const size_t                cb,
                                                _In_reads_( cb )    const BYTE* const           rgb,
                                                _Out_               ::CCachedBlockSlot* const   pslot ) override;

                        ERR ErrGetSlotForWrite( _In_                    const ::CCachedBlockId&     cbid,
                                                _In_                    const size_t                cb,
                                                _In_reads_opt_( cb )    const BYTE* const           rgb,
                                                _Out_                   ::CCachedBlockSlot* const   pslot ) override;

                        ERR ErrGetSlotForRead(  _In_    const ::CCachedBlockId&     cbid,
                                                _Out_   ::CCachedBlockSlot* const   pslot ) override;

                        ERR ErrWriteCluster(    _In_                const ::CCachedBlockSlot&                   slot,
                                                _In_                const size_t                                cb,
                                                _In_reads_( cb )    const BYTE* const                           rgb,
                                                _In_opt_            const ::ICachedBlockSlab::PfnClusterWritten pfnClusterWritten,
                                                _In_opt_            const DWORD_PTR                             keyClusterWritten,
                                                _In_opt_            const ::ICachedBlockSlab::PfnClusterHandoff pfnClusterHandoff ) override;

                        ERR ErrUpdateSlot( _In_ const ::CCachedBlockSlot& slotNew ) override;
        
                        ERR ErrReadCluster( _In_                const ::CCachedBlockSlot&                   slot,
                                            _In_                const size_t                                cb,
                                            _Out_writes_( cb )  BYTE* const                                 rgb,
                                            _In_opt_            const ::ICachedBlockSlab::PfnClusterRead    pfnClusterRead,
                                            _In_opt_            const DWORD_PTR                             keyClusterRead,
                                            _In_opt_            const ::ICachedBlockSlab::PfnClusterHandoff pfnClusterHandoff ) override;
        
                        ERR ErrVerifyCluster(   _In_                const ::CCachedBlockSlot&   slot,
                                                _In_                const size_t                cb,
                                                _In_reads_( cb )    const BYTE* const           rgb ) override;

                        ERR ErrInvalidateSlots( _In_ const ::ICachedBlockSlab::PfnConsiderUpdate    pfnConsiderUpdate,
                                                _In_ const DWORD_PTR                                keyConsiderUpdate ) override;

                        ERR ErrCleanSlots(  _In_ const ::ICachedBlockSlab::PfnConsiderUpdate    pfnConsiderUpdate,
                                            _In_ const DWORD_PTR                                keyConsiderUpdate ) override;

                        ERR ErrEvictSlots(  _In_ const ::ICachedBlockSlab::PfnConsiderUpdate    pfnConsiderUpdate,
                                            _In_ const DWORD_PTR                                keyConsiderUpdate ) override;

                        ERR ErrVisitSlots(  _In_ const ::ICachedBlockSlab::PfnVisitSlot pfnVisitSlot,
                                            _In_ const DWORD_PTR                        keyVisitSlot ) override;

                        BOOL FUpdated() override;

                        ERR ErrAcceptUpdates() override;

                        void RevertUpdates() override;

                        BOOL FDirty() override;

                        ERR ErrSave(    _In_opt_    const ::ICachedBlockSlab::PfnSlabSaved  pfnSlabSaved, 
                                        _In_opt_    const DWORD_PTR                         keySlabSaved ) override;
                };

                template<class TM, class TN>
                inline ERR CCachedBlockSlabWrapper<TM, TN>::ErrGetPhysicalId( _Out_ QWORD* const pib )
                {
                    ERR     err = JET_errSuccess;
                    Int64   ib  = 0;

                    *pib = 0;

                    ExCall( ib = I()->GetPhysicalId() );

                    *pib = ib;

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        *pib = 0;
                    }
                    return err;
                }

                template<class TM, class TN>
                inline ERR CCachedBlockSlabWrapper<TM, TN>::ErrGetSlotForCache( _In_                const ::CCachedBlockId&     cbid,
                                                                                _In_                const size_t                cb,
                                                                                _In_reads_( cb )    const BYTE* const           rgb,
                                                                                _Out_               ::CCachedBlockSlot* const   pslot )
                {
                    ERR                 err     = JET_errSuccess;
                    ::CCachedBlockId*   pcbid   = NULL;
                    array<byte>^        buffer  = !rgb ? nullptr : gcnew array<byte>( cb );
                    pin_ptr<byte>       rgbData = ( !rgb || !cb ) ? nullptr : &buffer[ 0 ];
                    MemoryStream^       stream  = !rgb ? nullptr : gcnew MemoryStream( buffer, false );
                    CachedBlockSlot^    slot    = nullptr;

                    new( pslot ) ::CCachedBlockSlot();

                    Alloc( pcbid = new ::CCachedBlockId() );
                    UtilMemCpy( pcbid, &cbid, sizeof( *pcbid ) );

                    if ( cb )
                    {
                        UtilMemCpy( (BYTE*)rgbData, rgb, cb );
                    }

                    ExCall( slot = I()->GetSlotForCache( gcnew CachedBlockId( &pcbid ), stream ) );

                    if ( !System::Object::ReferenceEquals(slot, nullptr ) )
                    {
                        UtilMemCpy( pslot, slot->Pslot(), sizeof( *pslot ) );
                    }

                HandleError:
                    delete pcbid;
                    if ( err < JET_errSuccess )
                    {
                        new( pslot ) ::CCachedBlockSlot();
                    }
                    return err;
                }

                template<class TM, class TN>
                inline ERR CCachedBlockSlabWrapper<TM, TN>::ErrGetSlotForWrite( _In_                    const ::CCachedBlockId&     cbid,
                                                                                _In_                    const size_t                cb,
                                                                                _In_reads_opt_( cb )    const BYTE* const           rgb,
                                                                                _Out_                   ::CCachedBlockSlot* const   pslot )
                {
                    ERR                 err     = JET_errSuccess;
                    ::CCachedBlockId*   pcbid   = NULL;
                    array<byte>^        buffer  = !rgb ? nullptr : gcnew array<byte>( cb );
                    pin_ptr<byte>       rgbData = ( !rgb || !cb ) ? nullptr : &buffer[ 0 ];
                    MemoryStream^       stream  = !rgb ? nullptr : gcnew MemoryStream( buffer, false );
                    CachedBlockSlot^    slot    = nullptr;

                    new( pslot ) ::CCachedBlockSlot();

                    Alloc( pcbid = new ::CCachedBlockId() );
                    UtilMemCpy( pcbid, &cbid, sizeof( *pcbid ) );

                    if ( cb )
                    {
                        UtilMemCpy( (BYTE*)rgbData, rgb, cb );
                    }

                    ExCall( slot = I()->GetSlotForWrite( gcnew CachedBlockId( &pcbid ), stream ) );

                    if ( !System::Object::ReferenceEquals( slot, nullptr ) )
                    {
                        UtilMemCpy( pslot, slot->Pslot(), sizeof( *pslot ) );
                    }

                HandleError:
                    delete pcbid;
                    if ( err < JET_errSuccess )
                    {
                        new( pslot ) ::CCachedBlockSlot();
                    }
                    return err;
                }

                template<class TM, class TN>
                inline ERR CCachedBlockSlabWrapper<TM, TN>::ErrGetSlotForRead(  _In_    const ::CCachedBlockId&     cbid,
                                                                                _Out_   ::CCachedBlockSlot* const   pslot )
                {
                    ERR                 err     = JET_errSuccess;
                    ::CCachedBlockId*   pcbid   = NULL;
                    CachedBlockSlot^    slot    = nullptr;

                    new( pslot ) ::CCachedBlockSlot();

                    Alloc( pcbid = new ::CCachedBlockId() );
                    UtilMemCpy( pcbid, &cbid, sizeof( *pcbid ) );

                    ExCall( slot = I()->GetSlotForRead( gcnew CachedBlockId( &pcbid ) ) );

                    if ( !System::Object::ReferenceEquals( slot, nullptr ) )
                    {
                        UtilMemCpy( pslot, slot->Pslot(), sizeof( *pslot ) );
                    }

                HandleError:
                    delete pcbid;
                    if ( err < JET_errSuccess )
                    {
                        new( pslot ) ::CCachedBlockSlot();
                    }
                    return err;
                }

                template<class TM, class TN>
                inline ERR CCachedBlockSlabWrapper<TM, TN>::ErrWriteCluster(    _In_                const ::CCachedBlockSlot&                   slot,
                                                                                _In_                const size_t                                cb,
                                                                                _In_reads_( cb )    const BYTE* const                           rgb,
                                                                                _In_opt_            const ::ICachedBlockSlab::PfnClusterWritten pfnClusterWritten,
                                                                                _In_opt_            const DWORD_PTR                             keyClusterWritten,
                                                                                _In_opt_            const ::ICachedBlockSlab::PfnClusterHandoff pfnClusterHandoff )
                {
                    ERR                 err             = JET_errSuccess;
                    ::CCachedBlockSlot* pslot           = NULL;
                    array<byte>^        buffer          = !rgb ? nullptr : gcnew array<byte>( cb );
                    pin_ptr<byte>       rgbData         = ( !rgb || !cb ) ? nullptr : &buffer[ 0 ];
                    MemoryStream^       stream          = !rgb ? nullptr : gcnew MemoryStream( buffer, false );
                    ClusterWritten^     clusterWritten  = ( pfnClusterWritten || pfnClusterHandoff ) ?
                                                                gcnew ClusterWritten(   pfnClusterWritten, 
                                                                                        keyClusterWritten, 
                                                                                        pfnClusterHandoff ) :
                                                                nullptr;

                    Alloc( pslot = new ::CCachedBlockSlot() );
                    UtilMemCpy( pslot, &slot, sizeof( *pslot ) );

                    if ( cb )
                    {
                        UtilMemCpy( (BYTE*)rgbData, rgb, cb );
                    }

                    ExCall( I()->WriteCluster(  gcnew CachedBlockSlot( &pslot ),
                                                stream,
                                                pfnClusterWritten ?
                                                    gcnew Internal::Ese::BlockCache::Interop::ICachedBlockSlab::ClusterWritten( clusterWritten, &ClusterWritten::ClusterWritten_ ) :
                                                    nullptr,
                                                clusterWritten != nullptr ?
                                                    gcnew Internal::Ese::BlockCache::Interop::ICachedBlockSlab::ClusterHandoff( clusterWritten, &ClusterWritten::ClusterHandoff_ ) :
                                                    nullptr ) );

                HandleError:
                    delete pslot;
                    return err;
                }

                template<class TM, class TN>
                inline ERR CCachedBlockSlabWrapper<TM, TN>::ErrUpdateSlot( _In_ const CCachedBlockSlot& slotNew )
                {
                    ERR                 err     = JET_errSuccess;
                    ::CCachedBlockSlot* pslot   = NULL;

                    Alloc( pslot = new ::CCachedBlockSlot() );
                    UtilMemCpy( pslot, &slotNew, sizeof( *pslot ) );

                    ExCall( I()->UpdateSlot( gcnew CachedBlockSlot( &pslot ) ) );

                HandleError:
                    delete pslot;
                    return err;
                }
        
                template<class TM, class TN>
                inline ERR CCachedBlockSlabWrapper<TM, TN>::ErrReadCluster( _In_                const ::CCachedBlockSlot&                   slot,
                                                                            _In_                const size_t                                cb,
                                                                            _Out_writes_( cb )  BYTE* const                                 rgb,
                                                                            _In_opt_            const ::ICachedBlockSlab::PfnClusterRead    pfnClusterRead,
                                                                            _In_opt_            const DWORD_PTR                             keyClusterRead,
                                                                            _In_opt_            const ::ICachedBlockSlab::PfnClusterHandoff pfnClusterHandoff )
                {
                    ERR                 err         = JET_errSuccess;
                    ::CCachedBlockSlot* pslot       = NULL;
                    array<byte>^        buffer      = !rgb ? nullptr : gcnew array<byte>( cb );
                    pin_ptr<byte>       rgbData     = ( !rgb || !cb ) ? nullptr : &buffer[ 0 ];
                    MemoryStream^       stream      = !rgb ? nullptr : gcnew MemoryStream( buffer, true );
                    ClusterRead^        clusterRead = ( pfnClusterRead|| pfnClusterHandoff ) ?
                                                            gcnew ClusterRead(  cb,
                                                                                rgb, 
                                                                                pfnClusterRead, 
                                                                                keyClusterRead, 
                                                                                pfnClusterHandoff,  
                                                                                stream ) :
                                                            nullptr;

                    Alloc( pslot = new ::CCachedBlockSlot() );
                    UtilMemCpy( pslot, &slot, sizeof( *pslot ) );

                    if ( cb )
                    {
                        UtilMemCpy( (BYTE*)rgbData, rgb, cb );
                    }

                    ExCall( I()->ReadCluster(   gcnew CachedBlockSlot( &pslot ),
                                                stream,
                                                pfnClusterRead ?
                                                    gcnew Internal::Ese::BlockCache::Interop::ICachedBlockSlab::ClusterRead( clusterRead, &ClusterRead::ClusterRead_ ) :
                                                    nullptr,
                                                clusterRead != nullptr ?
                                                    gcnew Internal::Ese::BlockCache::Interop::ICachedBlockSlab::ClusterHandoff( clusterRead, &ClusterRead::ClusterHandoff_ ) :
                                                    nullptr ) );

                HandleError:
                    if ( !pfnClusterRead && cb )
                    {
                        UtilMemCpy( rgb, (const BYTE*)rgbData, cb );
                    }
                    delete pslot;
                    return err;
                }
        
                template<class TM, class TN>
                inline ERR CCachedBlockSlabWrapper<TM, TN>::ErrVerifyCluster(   _In_                const ::CCachedBlockSlot&   slot,
                                                                                _In_                const size_t                cb,
                                                                                _In_reads_( cb )    const BYTE* const           rgb )
                {
                    ERR                 err     = JET_errSuccess;
                    ::CCachedBlockSlot* pslot   = NULL;
                    array<byte>^        buffer      = !rgb ? nullptr : gcnew array<byte>( cb );
                    pin_ptr<byte>       rgbData     = ( !rgb || !cb ) ? nullptr : &buffer[ 0 ];
                    MemoryStream^       stream      = !rgb ? nullptr : gcnew MemoryStream( buffer, false );

                    Alloc( pslot = new ::CCachedBlockSlot() );
                    UtilMemCpy( pslot, &slot, sizeof( *pslot ) );

                    if ( cb )
                    {
                        UtilMemCpy( (BYTE*)rgbData, rgb, cb );
                    }

                    ExCall( I()->VerifyCluster( gcnew CachedBlockSlot( &pslot ), stream ) );

                HandleError:
                    delete pslot;
                    return err;
                }

                template<class TM, class TN>
                inline ERR CCachedBlockSlabWrapper<TM, TN>::ErrInvalidateSlots( _In_ const ::ICachedBlockSlab::PfnConsiderUpdate    pfnConsiderUpdate,
                                                                                _In_ const DWORD_PTR                                keyConsiderUpdate )
                {
                    ERR             err             = JET_errSuccess;
                    ConsiderUpdate^ considerUpdate  = pfnConsiderUpdate ? gcnew ConsiderUpdate( pfnConsiderUpdate, keyConsiderUpdate ) : nullptr;

                    ExCall( I()->InvalidateSlots( pfnConsiderUpdate ? gcnew Internal::Ese::BlockCache::Interop::ICachedBlockSlab::ConsiderUpdate( considerUpdate, &ConsiderUpdate::ConsiderUpdate_ ) : nullptr ) );

                HandleError:
                    return err;
                }

                template<class TM, class TN>
                inline ERR CCachedBlockSlabWrapper<TM, TN>::ErrCleanSlots(  _In_ const ::ICachedBlockSlab::PfnConsiderUpdate    pfnConsiderUpdate,
                                                                            _In_ const DWORD_PTR                                keyConsiderUpdate )
                {
                    ERR             err             = JET_errSuccess;
                    ConsiderUpdate^ considerUpdate  = pfnConsiderUpdate ? gcnew ConsiderUpdate( pfnConsiderUpdate, keyConsiderUpdate ) : nullptr;

                    ExCall( I()->CleanSlots( pfnConsiderUpdate ? gcnew Internal::Ese::BlockCache::Interop::ICachedBlockSlab::ConsiderUpdate( considerUpdate, &ConsiderUpdate::ConsiderUpdate_ ) : nullptr ) );

                HandleError:
                    return err;
                }

                template<class TM, class TN>
                inline ERR CCachedBlockSlabWrapper<TM, TN>::ErrEvictSlots(  _In_ const ::ICachedBlockSlab::PfnConsiderUpdate    pfnConsiderUpdate,
                                                                            _In_ const DWORD_PTR                                keyConsiderUpdate )
                {
                    ERR             err             = JET_errSuccess;
                    ConsiderUpdate^ considerUpdate  = pfnConsiderUpdate ? gcnew ConsiderUpdate( pfnConsiderUpdate, keyConsiderUpdate ) : nullptr;

                    ExCall( I()->EvictSlots( pfnConsiderUpdate ? gcnew Internal::Ese::BlockCache::Interop::ICachedBlockSlab::ConsiderUpdate( considerUpdate, &ConsiderUpdate::ConsiderUpdate_ ) : nullptr ) );

                HandleError:
                    return err;
                }

                template<class TM, class TN>
                inline ERR CCachedBlockSlabWrapper<TM, TN>::ErrVisitSlots(  _In_ const ::ICachedBlockSlab::PfnVisitSlot pfnVisitSlot,
                                                                            _In_ const DWORD_PTR                        keyVisitSlot )
                {
                    ERR         err         = JET_errSuccess;
                    VisitSlot^  visitSlot   = pfnVisitSlot ? gcnew VisitSlot( pfnVisitSlot, keyVisitSlot ) : nullptr;

                    ExCall( I()->VisitSlots( pfnVisitSlot ? gcnew Internal::Ese::BlockCache::Interop::ICachedBlockSlab::VisitSlot( visitSlot, &VisitSlot::VisitSlot_ ) : nullptr ) );

                HandleError:
                    return err;
                }

                template<class TM, class TN>
                inline BOOL CCachedBlockSlabWrapper<TM, TN>::FUpdated()
                {
                    return I()->IsUpdated() ? fTrue : fFalse;
                }

                template<class TM, class TN>
                inline ERR CCachedBlockSlabWrapper<TM, TN>::ErrAcceptUpdates()
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->AcceptUpdates() );

                HandleError:
                    return err;
                }

                template<class TM, class TN>
                inline void CCachedBlockSlabWrapper<TM, TN>::RevertUpdates()
                {
                    I()->RevertUpdates();
                }

                template<class TM, class TN>
                inline BOOL CCachedBlockSlabWrapper<TM, TN>::FDirty()
                {
                    return I()->IsDirty() ? fTrue : fFalse;
                }

                template<class TM, class TN>
                inline ERR CCachedBlockSlabWrapper<TM, TN>::ErrSave(    _In_opt_    const ::ICachedBlockSlab::PfnSlabSaved  pfnSlabSaved,
                                                                        _In_opt_    const DWORD_PTR                         keySlabSaved )
                {
                    ERR         err         = JET_errSuccess;
                    SlabSaved^  slabSaved   = pfnSlabSaved ? gcnew SlabSaved( pfnSlabSaved, keySlabSaved ) : nullptr;

                    ExCall( I()->Save( pfnSlabSaved ? gcnew Internal::Ese::BlockCache::Interop::ICachedBlockSlab::SlabSaved( slabSaved, &SlabSaved::SlabSaved_ ) : nullptr ) );

                HandleError:
                    return err;
                }
            }
        }
    }
}