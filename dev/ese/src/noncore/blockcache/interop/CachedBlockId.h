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
                /// Cached Block Id.
                /// </summary>
                public ref class CachedBlockId : public IEquatable<CachedBlockId^>
                {
                    public:

                        /// <summary>
                        /// Creates a Cached Block ID.
                        /// </summary>
                        /// <param name="volumeid">Volume ID.</param>
                        /// <param name="fileid">File ID.</param>
                        /// <param name="fileserial">File Serial Number.</param>
                        /// <param name="cachedBlockNumber">Cached Block Number.</param>
                        CachedBlockId(
                            VolumeId volumeid,
                            FileId fileid,
                            FileSerial fileserial,
                            CachedBlockNumber cachedBlockNumber )
                        {
                            ERR err = JET_errSuccess;

                            Alloc( pcbid = new ::CCachedBlockId(    (::VolumeId)volumeid,
                                                                    (::FileId)fileid, 
                                                                    (::FileSerial)fileserial, 
                                                                    (::CachedBlockNumber)cachedBlockNumber ) );

                        HandleError:
                            if ( err < JET_errSuccess )
                            {
                                throw EseException( err );
                            }
                        }

                    internal:

                        /// <summary>
                        /// Creates a Cached Block ID.
                        /// </summary>
                        /// <param name="cachedBlockIn">The native cached block id.</param>
                        CachedBlockId( _Inout_ ::CCachedBlockId** const ppcbid )
                        {
                            pcbid = *ppcbid;
                            *ppcbid = NULL;
                        }

                        const ::CCachedBlockId* Pcbid() { return pcbid; }

                    public:

                        ~CachedBlockId()
                        {
                            this->!CachedBlockId();
                        }

                        !CachedBlockId()
                        {
                            delete pcbid;
                            pcbid = NULL;
                        }

                        /// <summary>
                        /// Volume ID.
                        /// </summary>
                        property Internal::Ese::BlockCache::Interop::VolumeId VolumeId
                        {
                            Internal::Ese::BlockCache::Interop::VolumeId get()
                            {
                                return (Internal::Ese::BlockCache::Interop::VolumeId)pcbid->Volumeid();
                            }
                        }

                        /// <summary>
                        /// File ID.
                        /// </summary>
                        property Internal::Ese::BlockCache::Interop::FileId FileId
                        {
                            Internal::Ese::BlockCache::Interop::FileId get()
                            {
                                return (Internal::Ese::BlockCache::Interop::FileId)pcbid->Fileid();
                            }
                        }

                        /// <summary>
                        /// File Serial Number.
                        /// </summary>
                        property Internal::Ese::BlockCache::Interop::FileSerial FileSerial
                        {
                            Internal::Ese::BlockCache::Interop::FileSerial get()
                            {
                                return (Internal::Ese::BlockCache::Interop::FileSerial)pcbid->Fileserial();
                            }
                        }

                        /// <summary>
                        /// Cached Block Number.
                        /// </summary>
                        property Internal::Ese::BlockCache::Interop::CachedBlockNumber CachedBlockNumber
                        {
                            Internal::Ese::BlockCache::Interop::CachedBlockNumber get()
                            {
                                return (Internal::Ese::BlockCache::Interop::CachedBlockNumber)pcbid->Cbno();
                            }
                        }

                        /// <inheritdoc/>
                        static bool operator==( CachedBlockId^ a, CachedBlockId^ b )
                        {
                            return a->Equals( b );
                        }

                        /// <inheritdoc/>
                        static bool operator!=( CachedBlockId^ a, CachedBlockId^ b )
                        {
                            return !a->Equals( b );
                        }

                        /// <inheritdoc/>
                        virtual bool Equals( Object^ obj ) override
                        {
                            bool result = false;

                            if ( obj != nullptr && obj->GetType() == CachedBlockId::typeid )
                            {
                                result = Equals( safe_cast<CachedBlockId^>( obj ) );
                            }

                            return result;
                        }

                        /// <inheritdoc/>
                        virtual int GetHashCode() override
                        {
                            return CachedBlockNumber.GetHashCode()
                                ^ FileSerial.GetHashCode()
                                ^ FileId.GetHashCode()
                                ^ VolumeId.GetHashCode();
                        }

                        /// <inheritdoc/>
                        virtual bool Equals( CachedBlockId^ other )
                        {
                            if ( CachedBlockNumber != other->CachedBlockNumber )
                            {
                                return false;
                            }

                            if ( FileSerial != other->FileSerial )
                            {
                                return false;
                            }

                            if ( FileId != other->FileId )
                            {
                                return false;
                            }

                            if ( VolumeId != other->VolumeId )
                            {
                                return false;
                            }

                            return true;
                        }

                    private:

                        ::CCachedBlockId* pcbid;
                };
            }
        }
    }
}