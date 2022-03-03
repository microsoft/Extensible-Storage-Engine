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
                /// Cached Block.
                /// </summary>
                public ref class CachedBlock : public IEquatable<CachedBlock^>
                {
                    internal:

                        /// <summary>
                        /// Creates a Cached Block.
                        /// </summary>
                        /// <param name="cachedBlockIn">The native cached block.</param>
                        CachedBlock( _Inout_ ::CCachedBlock** const ppcbl )
                            :   pcbl( *ppcbl )
                        {
                            *ppcbl = NULL;
                        }

                        /// <summary>
                        /// Creates a Cached Block.
                        /// </summary>
                        /// <param name="cachedBlockIn">The native cached block.</param>
                        CachedBlock( _Inout_ const ::CCachedBlock* const pcbl )
                            :   pcbl( new ::CCachedBlock() )
                        {
                            if ( !this->pcbl )
                            {
                                throw gcnew OutOfMemoryException();
                            }

                            UtilMemCpy( (void*)this->pcbl, pcbl, sizeof( *this->pcbl ) );
                        }

                        const ::CCachedBlock* Pcbl() { return this->pcbl; }

                    public:

                        ~CachedBlock()
                        {
                            this->!CachedBlock();
                        }

                        !CachedBlock()
                        {
                            delete this->pcbl;
                            this->pcbl = NULL;
                        }

                        /// <summary>
                        /// Cached Block ID.
                        /// </summary>
                        property Internal::Ese::BlockCache::Interop::CachedBlockId^ CachedBlockId
                        {
                            Internal::Ese::BlockCache::Interop::CachedBlockId^ get()
                            {
                                ::CCachedBlockId* pcbid = new ::CCachedBlockId();
                                if ( !pcbid )
                                {
                                    throw gcnew OutOfMemoryException();
                                }

                                UtilMemCpy( pcbid, &this->pcbl->Cbid(), sizeof( *pcbid ) );

                                return gcnew Internal::Ese::BlockCache::Interop::CachedBlockId( &pcbid );
                            }
                        }

                        /// <summary>
                        /// Cluster Number.
                        /// </summary>
                        property Internal::Ese::BlockCache::Interop::ClusterNumber ClusterNumber
                        {
                            Internal::Ese::BlockCache::Interop::ClusterNumber get()
                            {
                                return (Internal::Ese::BlockCache::Interop::ClusterNumber)this->pcbl->Clno();
                            }
                        }

                        /// <summary>
                        /// Indicates if the slot contains valid data.
                        /// </summary>
                        property bool IsValid
                        {
                            bool get()
                            {
                                return this->pcbl->FValid() ? true : false;
                            }
                        }

                        /// <summary>
                        /// Indicates if the slot contains data that may not be written back.
                        /// </summary>
                        property bool IsPinned
                        {
                            bool get()
                            {
                                return this->pcbl->FPinned() ? true : false;
                            }
                        }

                        /// <summary>
                        /// Indicates if the slot contains data that needs to be written back to the caching file.
                        /// </summary>
                        property bool IsDirty
                        {
                            bool get()
                            {
                                return this->pcbl->FDirty() ? true : false;
                            }
                        }

                        /// <summary>
                        /// Indicates if the slot contains data that has ever needed to be written back to the caching
                        /// file.
                        /// </summary>
                        property bool WasEverDirty
                        {
                            bool get()
                            {
                                return this->pcbl->FEverDirty() ? true : false;
                            }
                        }

                        /// <summary>
                        /// Indicates if the slot contained data that was invalidated.
                        /// </summary>
                        property bool WasPurged
                        {
                            bool get()
                            {
                                return this->pcbl->FPurged() ? true : false;
                            }
                        }

                        /// <inheritdoc/>
                        static bool operator==( CachedBlock^ a, CachedBlock^ b )
                        {
                            return a->Equals( b );
                        }

                        /// <inheritdoc/>
                        static bool operator!=( CachedBlock^ a, CachedBlock^ b )
                        {
                            return !a->Equals( b );
                        }

                        /// <inheritdoc/>
                        virtual bool Equals( Object^ obj ) override
                        {
                            bool result = false;

                            if ( obj != nullptr && obj->GetType() == CachedBlock::typeid )
                            {
                                result = this->Equals( safe_cast<CachedBlock^>( obj ) );
                            }

                            return result;
                        }

                        /// <inheritdoc/>
                        virtual int GetHashCode() override
                        {
                            return this->CachedBlockId->GetHashCode()
                                ^ this->ClusterNumber.GetHashCode()
                                ^ this->IsValid.GetHashCode()
                                ^ this->IsPinned.GetHashCode()
                                ^ this->IsDirty.GetHashCode()
                                ^ this->WasEverDirty.GetHashCode()
                                ^ this->WasPurged.GetHashCode();
                        }

                        /// <inheritdoc/>
                        virtual bool Equals( CachedBlock^ other )
                        {
                            if ( this->CachedBlockId != other->CachedBlockId )
                            {
                                return false;
                            }

                            if ( this->ClusterNumber != other->ClusterNumber )
                            {
                                return false;
                            }

                            if ( this->IsValid != other->IsValid )
                            {
                                return false;
                            }

                            if ( this->IsPinned != other->IsPinned )
                            {
                                return false;
                            }

                            if ( this->IsDirty != other->IsDirty )
                            {
                                return false;
                            }

                            if ( this->WasEverDirty != other->WasEverDirty )
                            {
                                return false;
                            }

                            if ( this->WasPurged != other->WasPurged )
                            {
                                return false;
                            }

                            return true;
                        }

                    private:

                        const ::CCachedBlock*   pcbl;
                        bool                    isOwner;
                };
            }
        }
    }
}