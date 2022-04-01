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
                ref class CachedBlockSlabRemotable : Remotable, ICachedBlockSlab
                {
                    public:

                        CachedBlockSlabRemotable( ICachedBlockSlab^ target )
                            :   target( target )
                        {
                        }

                        ~CachedBlockSlabRemotable() {}

                    public:

                        virtual UInt64 GetPhysicalId()
                        {
                            return this->target->GetPhysicalId();
                        }

                        virtual CachedBlockSlot^ GetSlotForCache( CachedBlockId^ cachedBlockId, MemoryStream^ data )
                        {
                            return this->target->GetSlotForCache( cachedBlockId, data );
                        }

                        virtual CachedBlockSlot^ GetSlotForWrite( CachedBlockId^ cachedBlockId, MemoryStream^ data )
                        {
                            return this->target->GetSlotForWrite( cachedBlockId, data );
                        }

                        virtual CachedBlockSlot^ GetSlotForRead( CachedBlockId^ cachedBlockId )
                        {
                            return this->target->GetSlotForRead( cachedBlockId );
                        }

                        virtual void WriteCluster(
                            CachedBlockSlot^ slot,
                            MemoryStream^ data,
                            ICachedBlockSlab::ClusterWritten^ clusterWritten,
                            ICachedBlockSlab::ClusterHandoff^ clusterHandoff )
                        {
                            return this->target->WriteCluster( slot, data, clusterWritten, clusterHandoff );
                        }

                        virtual void UpdateSlot( CachedBlockSlot^ slot )
                        {
                            return this->target->UpdateSlot( slot );
                        }

                        virtual void ReadCluster(
                            CachedBlockSlot^ slot,
                            MemoryStream^ data,
                            ICachedBlockSlab::ClusterRead^ clusterRead,
                            ICachedBlockSlab::ClusterHandoff^ clusterHandoff )
                        {
                            return this->target->ReadCluster( slot, data, clusterRead, clusterHandoff );
                        }

                        virtual void VerifyCluster( CachedBlockSlot^ slot, MemoryStream^ data )
                        {
                            return this->target->VerifyCluster( slot, data );
                        }

                        virtual void InvalidateSlots( ICachedBlockSlab::ConsiderUpdate^ considerUpdate )
                        {
                            return this->target->InvalidateSlots( considerUpdate );
                        }

                        virtual void CleanSlots( ICachedBlockSlab::ConsiderUpdate^ considerUpdate )
                        {
                            return this->target->CleanSlots( considerUpdate );
                        }

                        virtual void EvictSlots( ICachedBlockSlab::ConsiderUpdate^ considerUpdate )
                        {
                            return this->target->EvictSlots( considerUpdate );
                        }

                        virtual void VisitSlots( ICachedBlockSlab::VisitSlot^ visitSlot )
                        {
                            return this->target->VisitSlots( visitSlot );
                        }

                        virtual bool IsUpdated()
                        {
                            return this->target->IsUpdated();
                        }

                        virtual void AcceptUpdates()
                        {
                            return this->target->AcceptUpdates();
                        }

                        virtual void RevertUpdates()
                        {
                            return this->target->RevertUpdates();
                        }

                        virtual bool IsDirty()
                        {
                            return this->target->IsDirty();
                        }

                        virtual void Save( ICachedBlockSlab::SlabSaved^ slabSaved )
                        {
                            return this->target->Save( slabSaved );
                        }

                    private:

                        ICachedBlockSlab^ target;
                };
            }
        }
    }
}