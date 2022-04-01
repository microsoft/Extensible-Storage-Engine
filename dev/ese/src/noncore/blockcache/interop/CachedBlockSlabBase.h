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
                template< class TM, class TN, class TW >
                public ref class CachedBlockSlabBase : Base<TM, TN, TW>, ICachedBlockSlab
                {
                    public:

                        CachedBlockSlabBase( TM^ cbs ) : Base( cbs ) { }

                        CachedBlockSlabBase( TN** const ppcbs ) : Base( ppcbs ) {}

                        CachedBlockSlabBase( TN* const pcbs ) : Base( pcbs ) {}

                    public:

                        virtual UInt64 GetPhysicalId();

                        virtual CachedBlockSlot^ GetSlotForCache( CachedBlockId^ cachedBlockId, MemoryStream^ data );

                        virtual CachedBlockSlot^ GetSlotForWrite( CachedBlockId^ cachedBlockId, MemoryStream^ data );

                        virtual CachedBlockSlot^ GetSlotForRead( CachedBlockId^ cachedBlockId );

                        virtual void WriteCluster(
                            CachedBlockSlot^ slot,
                            MemoryStream^ data,
                            ICachedBlockSlab::ClusterWritten^ clusterWritten,
                            ICachedBlockSlab::ClusterHandoff^ clusterHandoff );

                        virtual void UpdateSlot( CachedBlockSlot^ slot );

                        virtual void ReadCluster(
                            CachedBlockSlot^ slot,
                            MemoryStream^ data,
                            ICachedBlockSlab::ClusterRead^ clusterRead,
                            ICachedBlockSlab::ClusterHandoff^ clusterHandoff );

                        virtual void VerifyCluster( CachedBlockSlot^ slot, MemoryStream^ data );

                        virtual void InvalidateSlots( ICachedBlockSlab::ConsiderUpdate^ considerUpdate );

                        virtual void CleanSlots( ICachedBlockSlab::ConsiderUpdate^ considerUpdate );

                        virtual void EvictSlots( ICachedBlockSlab::ConsiderUpdate^ considerUpdate );

                        virtual void VisitSlots( ICachedBlockSlab::VisitSlot^ visitSlot );

                        virtual bool IsUpdated();

                        virtual void AcceptUpdates();

                        virtual void RevertUpdates();

                        virtual bool IsDirty();

                        virtual void Save( ICachedBlockSlab::SlabSaved^ slabSaved );
                };

                template<class TM, class TN, class TW>
                inline UInt64 CachedBlockSlabBase<TM, TN, TW>::GetPhysicalId()
                {
                    ERR     err     = JET_errSuccess;
                    QWORD   ibSlab  = 0;

                    Call( Pi->ErrGetPhysicalId( &ibSlab ) );

                    return ibSlab;

                HandleError:
                    throw EseException( err );
                }

                template<class TM, class TN, class TW>
                inline CachedBlockSlot^ CachedBlockSlabBase<TM, TN, TW>::GetSlotForCache(
                    CachedBlockId^ cachedBlockId,
                    MemoryStream^ data )
                {
                    ERR                 err         = JET_errSuccess;
                    const DWORD         cbData      = data == nullptr ? 0 : (DWORD)data->Length;
                    VOID*               pvBuffer    = NULL;
                    ::CCachedBlockSlot* pslot       = NULL;

                    if ( cbData )
                    {
                        Alloc( pvBuffer = PvOSMemoryPageAlloc( roundup( cbData, OSMemoryPageCommitGranularity() ), NULL ) );
                        array<BYTE>^ bytes = data->ToArray();
                        pin_ptr<const BYTE> rgbData = &bytes[ 0 ];
                        UtilMemCpy( pvBuffer, (const BYTE*)rgbData, cbData );
                    }

                    Alloc( pslot = new ::CCachedBlockSlot() );

                    Call( Pi->ErrGetSlotForCache(   *cachedBlockId->Pcbid(),
                                                    cbData,
                                                    (const BYTE*)pvBuffer,
                                                    pslot ) );

                    OSMemoryPageFree( pvBuffer );
                    return gcnew CachedBlockSlot( &pslot );

                HandleError:
                    delete pslot;
                    OSMemoryPageFree( pvBuffer );
                    throw EseException( err );
                }

                template<class TM, class TN, class TW>
                inline CachedBlockSlot^ CachedBlockSlabBase<TM, TN, TW>::GetSlotForWrite(
                    CachedBlockId^ cachedBlockId,
                    MemoryStream^ data )
                {
                    ERR                 err         = JET_errSuccess;
                    const DWORD         cbData      = data == nullptr ? 0 : (DWORD)data->Length;
                    VOID*               pvBuffer    = NULL;
                    ::CCachedBlockSlot* pslot       = NULL;

                    if ( cbData )
                    {
                        Alloc( pvBuffer = PvOSMemoryPageAlloc( roundup( cbData, OSMemoryPageCommitGranularity() ), NULL ) );
                        array<BYTE>^ bytes = data->ToArray();
                        pin_ptr<const BYTE> rgbData = &bytes[ 0 ];
                        UtilMemCpy( pvBuffer, (const BYTE*)rgbData, cbData );
                    }

                    Alloc( pslot = new ::CCachedBlockSlot() );

                    Call( Pi->ErrGetSlotForWrite(   *cachedBlockId->Pcbid(),
                                                    cbData,
                                                    (const BYTE*)pvBuffer,
                                                    pslot ) );

                    OSMemoryPageFree( pvBuffer );
                    return gcnew CachedBlockSlot( &pslot );

                HandleError:
                    delete pslot;
                    OSMemoryPageFree( pvBuffer );
                    throw EseException( err );
                }

                template<class TM, class TN, class TW>
                inline CachedBlockSlot^ CachedBlockSlabBase<TM, TN, TW>::GetSlotForRead( CachedBlockId^ cachedBlockId )
                {
                    ERR                 err     = JET_errSuccess;
                    ::CCachedBlockSlot* pslot   = NULL;

                    Alloc( pslot = new ::CCachedBlockSlot() );

                    Call( Pi->ErrGetSlotForRead( *cachedBlockId->Pcbid(), pslot ) );

                    return gcnew CachedBlockSlot( &pslot );

                HandleError:
                    delete pslot;
                    throw EseException( err );
                }

                template<class TM, class TN, class TW>
                inline void CachedBlockSlabBase<TM, TN, TW>::WriteCluster(
                    CachedBlockSlot^ slot, 
                    MemoryStream^ data,
                    ICachedBlockSlab::ClusterWritten^ clusterWritten, 
                    ICachedBlockSlab::ClusterHandoff^ clusterHandoff )
                {
                    ERR                     err                     = JET_errSuccess;
                    const DWORD             cbData                  = data == nullptr ? 0 : (DWORD)data->Length;
                    VOID*                   pvBuffer                = NULL;
                    ClusterWrittenInverse^  clusterWrittenInverse   = nullptr;

                    if ( cbData )
                    {
                        Alloc( pvBuffer = PvOSMemoryPageAlloc( roundup( cbData, OSMemoryPageCommitGranularity() ), NULL ) );
                        array<BYTE>^ bytes = data->ToArray();
                        pin_ptr<const BYTE> rgbData = &bytes[ 0 ];
                        UtilMemCpy( pvBuffer, (const BYTE*)rgbData, cbData );
                    }

                    if ( clusterWritten != nullptr || clusterHandoff != nullptr )
                    {
                        clusterWrittenInverse = gcnew ClusterWrittenInverse( clusterWritten, clusterHandoff, &pvBuffer );
                    }

                    Call( Pi->ErrWriteCluster(  *slot->Pslot(),
                                                cbData,
                                                (const BYTE*)( clusterWrittenInverse == nullptr ? pvBuffer : clusterWrittenInverse->PvBuffer ),
                                                clusterWrittenInverse == nullptr ? NULL : clusterWrittenInverse->PfnClusterWritten,
                                                clusterWrittenInverse == nullptr ? NULL : clusterWrittenInverse->KeyClusterWritten,
                                                clusterWrittenInverse == nullptr ? NULL : clusterWrittenInverse->PfnClusterHandoff ) );

                HandleError:
                    OSMemoryPageFree( pvBuffer );
                    if ( clusterWritten == nullptr )
                    {
                        delete clusterWrittenInverse;
                    }
                    if ( err < JET_errSuccess )
                    {
                        delete clusterWrittenInverse;
                        throw EseException( err );
                    }
                }

                template<class TM, class TN, class TW>
                inline void CachedBlockSlabBase<TM, TN, TW>::UpdateSlot( CachedBlockSlot^ slot )
                {
                    ERR err = JET_errSuccess;

                    Call( Pi->ErrUpdateSlot( *slot->Pslot() ) );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        throw EseException( err );
                    }
                }

                template<class TM, class TN, class TW>
                inline void CachedBlockSlabBase<TM, TN, TW>::ReadCluster(
                    CachedBlockSlot^ slot,
                    MemoryStream^ data,
                    ICachedBlockSlab::ClusterRead^ clusterRead,
                    ICachedBlockSlab::ClusterHandoff^ clusterHandoff )
                {
                    ERR                 err                 = JET_errSuccess;
                    const DWORD         cbData              = data == nullptr ? 0 : (DWORD)data->Length;
                    VOID*               pvBuffer            = NULL;
                    ClusterReadInverse^ clusterReadInverse  = nullptr;

                    if ( cbData )
                    {
                        Alloc( pvBuffer = PvOSMemoryPageAlloc( roundup( cbData, OSMemoryPageCommitGranularity() ), NULL ) );
                        array<BYTE>^ bytes = data->ToArray();
                        pin_ptr<const BYTE> rgbData = &bytes[ 0 ];
                        UtilMemCpy( pvBuffer, (const BYTE*)rgbData, cbData );
                    }

                    if ( clusterRead != nullptr || clusterHandoff != nullptr )
                    {
                        clusterReadInverse = gcnew ClusterReadInverse( data, clusterRead, clusterHandoff, &pvBuffer );
                    }

                    Call( Pi->ErrReadCluster(  *slot->Pslot(),
                                                cbData,
                                                (BYTE*)( clusterReadInverse == nullptr ? pvBuffer : clusterReadInverse->PvBuffer ),
                                                clusterReadInverse == nullptr ? NULL : clusterReadInverse->PfnClusterRead,
                                                clusterReadInverse == nullptr ? NULL : clusterReadInverse->KeyClusterRead,
                                                clusterReadInverse == nullptr ? NULL : clusterReadInverse->PfnClusterHandoff ) );

                HandleError:
                    if ( clusterRead == nullptr && cbData )
                    {
                        array<BYTE>^ bytes = gcnew array<BYTE>( cbData );
                        pin_ptr<const BYTE> rgbData = &bytes[ 0 ];
                        UtilMemCpy( (BYTE*)rgbData, clusterReadInverse == nullptr ? pvBuffer : clusterReadInverse->PvBuffer, cbData );
                        data->Position = 0;
                        data->Write( bytes, 0, bytes->Length );
                    }
                    OSMemoryPageFree( pvBuffer );
                    if ( clusterRead == nullptr )
                    {
                        delete clusterReadInverse;
                    }
                    if ( err < JET_errSuccess )
                    {
                        delete clusterReadInverse;
                        throw EseException( err );
                    }
                }

                template<class TM, class TN, class TW>
                inline void CachedBlockSlabBase<TM, TN, TW>::VerifyCluster( CachedBlockSlot^ slot, MemoryStream^ data )
                {
                    ERR         err         = JET_errSuccess;
                    const DWORD cbData      = data == nullptr ? 0 : (DWORD)data->Length;
                    VOID*       pvBuffer    = NULL;

                    if ( cbData )
                    {
                        Alloc( pvBuffer = PvOSMemoryPageAlloc( roundup( cbData, OSMemoryPageCommitGranularity() ), NULL ) );
                        array<BYTE>^ bytes = data->ToArray();
                        pin_ptr<const BYTE> rgbData = &bytes[ 0 ];
                        UtilMemCpy( pvBuffer, (const BYTE*)rgbData, cbData );
                    }

                    Call( Pi->ErrVerifyCluster( *slot->Pslot(), cbData, (const BYTE*)pvBuffer ) );

                HandleError:
                    OSMemoryPageFree( pvBuffer );
                    if ( err < JET_errSuccess )
                    {
                        throw EseException( err );
                    }
                }

                template<class TM, class TN, class TW>
                inline void CachedBlockSlabBase<TM, TN, TW>::InvalidateSlots(
                    ICachedBlockSlab::ConsiderUpdate^ considerUpdate )
                {
                    ERR                     err                     = JET_errSuccess;
                    ConsiderUpdateInverse^  considerUpdateInverse   = nullptr;

                    if ( considerUpdate != nullptr )
                    {
                        considerUpdateInverse = gcnew ConsiderUpdateInverse( considerUpdate );
                    }

                    Call( Pi->ErrInvalidateSlots(   considerUpdateInverse == nullptr ? NULL : considerUpdateInverse->PfnConsiderUpdate,
                                                    considerUpdateInverse == nullptr ? NULL : considerUpdateInverse->KeyConsiderUpdate ) );

                HandleError:
                    delete considerUpdateInverse;
                    if ( err < JET_errSuccess )
                    {
                        throw EseException( err );
                    }
                }

                template<class TM, class TN, class TW>
                inline void CachedBlockSlabBase<TM, TN, TW>::CleanSlots(
                    ICachedBlockSlab::ConsiderUpdate^ considerUpdate )
                {
                    ERR                     err                     = JET_errSuccess;
                    ConsiderUpdateInverse^  considerUpdateInverse   = nullptr;

                    if ( considerUpdate != nullptr )
                    {
                        considerUpdateInverse = gcnew ConsiderUpdateInverse( considerUpdate );
                    }

                    Call( Pi->ErrCleanSlots(    considerUpdateInverse == nullptr ? NULL : considerUpdateInverse->PfnConsiderUpdate,
                                                considerUpdateInverse == nullptr ? NULL : considerUpdateInverse->KeyConsiderUpdate ) );

                HandleError:
                    delete considerUpdateInverse;
                    if ( err < JET_errSuccess )
                    {
                        throw EseException( err );
                    }
                }

                template<class TM, class TN, class TW>
                inline void CachedBlockSlabBase<TM, TN, TW>::EvictSlots(
                    ICachedBlockSlab::ConsiderUpdate^ considerUpdate )
                {
                    ERR                     err                     = JET_errSuccess;
                    ConsiderUpdateInverse^  considerUpdateInverse   = nullptr;

                    if ( considerUpdate != nullptr )
                    {
                        considerUpdateInverse = gcnew ConsiderUpdateInverse( considerUpdate );
                    }

                    Call( Pi->ErrEvictSlots(    considerUpdateInverse == nullptr ? NULL : considerUpdateInverse->PfnConsiderUpdate,
                                                considerUpdateInverse == nullptr ? NULL : considerUpdateInverse->KeyConsiderUpdate ) );

                HandleError:
                    delete considerUpdateInverse;
                    if ( err < JET_errSuccess )
                    {
                        throw EseException( err );
                    }
                }

                template<class TM, class TN, class TW>
                inline void CachedBlockSlabBase<TM, TN, TW>::VisitSlots(
                    ICachedBlockSlab::VisitSlot^ visitSlot )
                {
                    ERR                 err                 = JET_errSuccess;
                    VisitSlotInverse^   visitSlotInverse    = nullptr;

                    if ( visitSlot != nullptr )
                    {
                        visitSlotInverse = gcnew VisitSlotInverse( visitSlot );
                    }

                    Call( Pi->ErrVisitSlots(    visitSlotInverse == nullptr ? NULL : visitSlotInverse->PfnVisitSlot,
                                                visitSlotInverse == nullptr ? NULL : visitSlotInverse->KeyVisitSlot ) );

                HandleError:
                    delete visitSlotInverse;
                    if ( err < JET_errSuccess )
                    {
                        throw EseException( err );
                    }
                }

                template<class TM, class TN, class TW>
                inline bool CachedBlockSlabBase<TM, TN, TW>::IsUpdated()
                {
                    return Pi->FUpdated() ? true : false;
                }

                template<class TM, class TN, class TW>
                inline void CachedBlockSlabBase<TM, TN, TW>::AcceptUpdates()
                {
                    ERR err = JET_errSuccess;
                    
                    Call( Pi->ErrAcceptUpdates() );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        throw EseException( err );
                    }
                }

                template<class TM, class TN, class TW>
                inline void CachedBlockSlabBase<TM, TN, TW>::RevertUpdates()
                {
                    Pi->RevertUpdates();
                }

                template<class TM, class TN, class TW>
                inline bool CachedBlockSlabBase<TM, TN, TW>::IsDirty()
                {
                    return Pi->FDirty() ? true : false;
                }

                template<class TM, class TN, class TW>
                inline void CachedBlockSlabBase<TM, TN, TW>::Save( ICachedBlockSlab::SlabSaved^ slabSaved )
                {
                    ERR                 err                 = JET_errSuccess;
                    SlabSavedInverse^   slabSavedInverse    = nullptr;

                    if ( slabSaved != nullptr )
                    {
                        slabSavedInverse = gcnew SlabSavedInverse( slabSaved );
                    }

                    Call( Pi->ErrSave(  slabSavedInverse == nullptr ? NULL : slabSavedInverse->PfnSlabSaved,
                                        slabSavedInverse == nullptr ? NULL : slabSavedInverse->KeySlabSaved ) );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        delete slabSavedInverse;
                        throw EseException( err );
                    }
                }
            }
        }
    }
}