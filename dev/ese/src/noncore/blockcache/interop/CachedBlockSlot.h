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
                /// Cached Block Slot.
                /// </summary>
                public ref class CachedBlockSlot : public IEquatable<CachedBlockSlot^>
                {
                    internal:

                        /// <summary>
                        /// Creates a Cached Block Slot.
                        /// </summary>
                        /// <param name="cachedBlockIn">The native cached block.</param>
                        CachedBlockSlot( _Inout_ ::CCachedBlockSlot** const ppslot )
                            :   pslot( *ppslot )
                        {
                            *ppslot = NULL;
                        }

                        /// <summary>
                        /// Creates a Cached Block Slot.
                        /// </summary>
                        /// <param name="cachedBlockIn">The native cached block.</param>
                        CachedBlockSlot( _Inout_ const ::CCachedBlockSlot* const pslot )
                            :   pslot( new ::CCachedBlockSlot() )
                        {
                            if ( !this->pslot )
                            {
                                throw gcnew OutOfMemoryException();
                            }

                            UtilMemCpy( (void*)this->pslot, pslot, sizeof( *this->pslot ) );
                        }

                        const ::CCachedBlockSlot* Pslot() { return this->pslot; }

                    public:

                        ~CachedBlockSlot()
                        {
                            this->!CachedBlockSlot();
                        }

                        !CachedBlockSlot()
                        {
                            delete this->pslot;
                            this->pslot = NULL;
                        }

                        /// <summary>
                        /// Cached Block.
                        /// </summary>
                        property Internal::Ese::BlockCache::Interop::CachedBlock^ CachedBlock
                        {
                            Internal::Ese::BlockCache::Interop::CachedBlock^ get()
                            {
                                const ::CCachedBlock* pcblT = this->pslot;
                                Unused( pcblT );

                                ::CCachedBlock* pcbl = new ::CCachedBlock();
                                if ( !pcbl )
                                {
                                    throw gcnew OutOfMemoryException();
                                }

                                UtilMemCpy( pcbl, this->pslot, sizeof( *pcbl ) );

                                return gcnew Internal::Ese::BlockCache::Interop::CachedBlock( &pcbl );
                            }
                        }

                        /// <summary>
                        /// Slab Id.
                        /// </summary>
                        property UInt64 Id
                        {
                            UInt64 get()
                            {
                                return this->pslot->IbSlab();
                            }
                        }

                        /// <summary>
                        /// Chunk Number.
                        /// </summary>
                        property Internal::Ese::BlockCache::Interop::ChunkNumber ChunkNumber
                        {
                            Internal::Ese::BlockCache::Interop::ChunkNumber get()
                            {
                                return (Internal::Ese::BlockCache::Interop::ChunkNumber)this->pslot->Chno();
                            }
                        }

                        /// <summary>
                        /// Slot Number.
                        /// </summary>
                        property Internal::Ese::BlockCache::Interop::SlotNumber SlotNumber
                        {
                            Internal::Ese::BlockCache::Interop::SlotNumber get()
                            {
                                return (Internal::Ese::BlockCache::Interop::SlotNumber)this->pslot->Slno();
                            }
                        }

                        /// <inheritdoc/>
                        static bool operator==( CachedBlockSlot^ a, CachedBlockSlot^ b )
                        {
                            return a->Equals( b );
                        }

                        /// <inheritdoc/>
                        static bool operator!=( CachedBlockSlot^ a, CachedBlockSlot^ b )
                        {
                            return !a->Equals( b );
                        }

                        /// <inheritdoc/>
                        virtual bool Equals( Object^ obj ) override
                        {
                            bool result = false;

                            if ( obj != nullptr && obj->GetType() == CachedBlockSlot::typeid )
                            {
                                result = this->Equals( safe_cast<CachedBlockSlot^>( obj ) );
                            }

                            return result;
                        }

                        /// <inheritdoc/>
                        virtual int GetHashCode() override
                        {
                            return this->CachedBlock->GetHashCode()
                                ^ this->ChunkNumber.GetHashCode()
                                ^ this->SlotNumber.GetHashCode();
                        }

                        /// <inheritdoc/>
                        virtual bool Equals( CachedBlockSlot^ other )
                        {
                            if ( this->CachedBlock != other->CachedBlock )
                            {
                                return false;
                            }

                            if ( this->ChunkNumber != other->ChunkNumber )
                            {
                                return false;
                            }

                            if ( this->SlotNumber != other->SlotNumber )
                            {
                                return false;
                            }

                            return true;
                        }

                    private:

                        const ::CCachedBlockSlot*   pslot;
                        bool                        isOwner;
                };
            }
        }
    }
}