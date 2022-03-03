// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "stdafx.h"

#pragma once

#pragma managed

namespace Internal
{
    namespace Ese
    {
        namespace BlockCache
        {
            namespace Interop
            {
                /// <summary>
                /// Cached Block Slot State.
                /// </summary>
                public ref class CachedBlockSlotState : public IEquatable<CachedBlockSlotState^>
                {
                    internal:

                        /// <summary>
                        /// Creates a Cached Block Slot State.
                        /// </summary>
                        /// <param name="cachedBlockIn">The native cached block.</param>
                        CachedBlockSlotState( _Inout_ ::CCachedBlockSlotState** const ppslotst )
                            :   pslotst( *ppslotst )
                        {
                            *ppslotst = NULL;
                        }

                        /// <summary>
                        /// Creates a Cached Block Slot State.
                        /// </summary>
                        /// <param name="cachedBlockIn">The native cached block.</param>
                        CachedBlockSlotState( _Inout_ const ::CCachedBlockSlotState* const pslotst )
                            :   pslotst( new ::CCachedBlockSlotState() )
                        {
                            if ( !this->pslotst )
                            {
                                throw gcnew OutOfMemoryException();
                            }

                            UtilMemCpy( (void*)this->pslotst, pslotst, sizeof( *this->pslotst ) );
                        }

                        const ::CCachedBlockSlotState* Pslotst() { return this->pslotst; }

                    public:

                        ~CachedBlockSlotState()
                        {
                            this->!CachedBlockSlotState();
                        }

                        !CachedBlockSlotState()
                        {
                            delete this->pslotst;
                            this->pslotst = NULL;
                        }

                        /// <summary>
                        /// Cached Block Slot.
                        /// </summary>
                        property Internal::Ese::BlockCache::Interop::CachedBlockSlot^ CachedBlockSlot
                        {
                            Internal::Ese::BlockCache::Interop::CachedBlockSlot^ get()
                            {
                                const ::CCachedBlockSlot* pslotT = this->pslotst;
                                Unused( pslotT );

                                ::CCachedBlockSlot* pslot = new ::CCachedBlockSlot();
                                if ( !pslot )
                                {
                                    throw gcnew OutOfMemoryException();
                                }

                                UtilMemCpy( pslot, this->pslotst, sizeof( *pslot ) );

                                return gcnew Internal::Ese::BlockCache::Interop::CachedBlockSlot( &pslot );
                            }
                        }

                        /// <summary>
                        /// Indicates if the slab has updates that have not yet been accepted.
                        /// </summary>
                        property bool IsSlabUpdated
                        {
                            bool get()
                            {
                                return this->pslotst->FSlabUpdated() ? true : false;
                            }
                        }

                        /// <summary>
                        /// Indicates if the chunk has updates that have not yet been accepted.
                        /// </summary>
                        property bool IsChunkUpdated
                        {
                            bool get()
                            {
                                return this->pslotst->FChunkUpdated() ? true : false;
                            }
                        }

                        /// <summary>
                        /// Indicates if the slot has updates that have not yet been accepted.
                        /// </summary>
                        property bool IsSlotUpdated
                        {
                            bool get()
                            {
                                return this->pslotst->FSlotUpdated() ? true : false;
                            }
                        }

                        /// <summary>
                        /// Indicates if the cluster has updates that have not yet been accepted.
                        /// </summary>
                        property bool IsClusterUpdated
                        {
                            bool get()
                            {
                                return this->pslotst->FClusterUpdated() ? true : false;
                            }
                        }

                        /// <summary>
                        /// Indicates if the cached block held in the slot does not contain the current image of that cached block.
                        /// </summary>
                        property bool IsSuperceded
                        {
                            bool get()
                            {
                                return this->pslotst->FSuperceded() ? true : false;
                            }
                        }

                        /// <inheritdoc/>
                        static bool operator==( CachedBlockSlotState^ a, CachedBlockSlotState^ b )
                        {
                            return a->Equals( b );
                        }

                        /// <inheritdoc/>
                        static bool operator!=( CachedBlockSlotState^ a, CachedBlockSlotState^ b )
                        {
                            return !a->Equals( b );
                        }

                        /// <inheritdoc/>
                        virtual bool Equals( Object^ obj ) override
                        {
                            bool result = false;

                            if ( obj != nullptr && obj->GetType() == CachedBlockSlotState::typeid )
                            {
                                result = this->Equals( safe_cast<CachedBlockSlotState^>( obj ) );
                            }

                            return result;
                        }

                        /// <inheritdoc/>
                        virtual int GetHashCode() override
                        {
                            return this->CachedBlockSlot->GetHashCode()
                                ^ this->IsSlabUpdated.GetHashCode()
                                ^ this->IsChunkUpdated.GetHashCode()
                                ^ this->IsSlotUpdated.GetHashCode()
                                ^ this->IsClusterUpdated.GetHashCode()
                                ^ this->IsSuperceded.GetHashCode();
                        }

                        /// <inheritdoc/>
                        virtual bool Equals( CachedBlockSlotState^ other )
                        {
                            if ( this->CachedBlockSlot != other->CachedBlockSlot )
                            {
                                return false;
                            }

                            if ( this->IsSlabUpdated != other->IsSlabUpdated )
                            {
                                return false;
                            }

                            if ( this->IsChunkUpdated != other->IsChunkUpdated )
                            {
                                return false;
                            }

                            if ( this->IsSlotUpdated != other->IsSlotUpdated )
                            {
                                return false;
                            }

                            if ( this->IsClusterUpdated != other->IsClusterUpdated )
                            {
                                return false;
                            }

                            if ( this->IsSuperceded != other->IsSuperceded )
                            {
                                return false;
                            }

                            return true;
                        }

                    private:

                        const ::CCachedBlockSlotState*  pslotst;
                        bool                            isOwner;
                };
            }
        }
    }
}