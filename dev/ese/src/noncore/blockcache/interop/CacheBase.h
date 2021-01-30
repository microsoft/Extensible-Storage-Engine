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
                public ref class CacheBase : public Base<TM, TN, TW>, ICache
                {
                    public:

                        CacheBase( TM^ c ) : Base( c ) { }

                        CacheBase( TN** const ppc ) : Base( ppc ) {}

                    public:

                        virtual void Create();

                        virtual void Mount();

                        virtual Guid GetCacheType();

                        virtual void GetPhysicalId( [Out] VolumeId% volumeid, [Out] FileId% fileid, [Out] Guid% guid );

                        virtual void Close( VolumeId volumeid, FileId fileid, FileSerial fileserial );

                        virtual void Flush( VolumeId volumeid, FileId fileid, FileSerial fileserial );

                        virtual void Invalidate(
                            VolumeId volumeid,
                            FileId fileid, 
                            FileSerial fileserial, 
                            Int64 offsetInBytes,
                            Int64 sizeInBytes );

                        virtual void Read(
                            VolumeId volumeid,
                            FileId fileid,
                            FileSerial fileserial,
                            Int64 offsetInBytes,
                            ArraySegment<byte> data,
                            FileQOS fileQOS,
                            CachingPolicy cachingPolicy,
                            ICache::Complete^ complete );

                        virtual void Write(
                            VolumeId volumeid,
                            FileId fileid,
                            FileSerial fileserial,
                            Int64 offsetInBytes,
                            ArraySegment<byte> data,
                            FileQOS fileQOS,
                            CachingPolicy cachingPolicy,
                            ICache::Complete^ complete );
                };

                template< class TM, class TN, class TW >
                inline void CacheBase<TM, TN, TW>::Create()
                {
                    ERR err = JET_errSuccess;

                    Call( Pi->ErrCreate() );

                    return;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void CacheBase<TM, TN, TW>::Mount()
                {
                    ERR err = JET_errSuccess;

                    Call( Pi->ErrMount() );

                    return;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline Guid CacheBase<TM, TN, TW>::GetCacheType()
                {
                    ERR     err             = JET_errSuccess;
                    GUID    cacheTypeNative = { 0 };

                    Call( Pi->ErrGetCacheType( (BYTE*)&cacheTypeNative ) );

                    return Guid(    cacheTypeNative.Data1,
                                    cacheTypeNative.Data2,
                                    cacheTypeNative.Data3,
                                    cacheTypeNative.Data4[ 0 ],
                                    cacheTypeNative.Data4[ 1 ],
                                    cacheTypeNative.Data4[ 2 ],
                                    cacheTypeNative.Data4[ 3 ],
                                    cacheTypeNative.Data4[ 4 ],
                                    cacheTypeNative.Data4[ 5 ],
                                    cacheTypeNative.Data4[ 6 ],
                                    cacheTypeNative.Data4[ 7 ] );

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void CacheBase<TM, TN, TW>::GetPhysicalId(
                    [Out] VolumeId% volumeid,
                    [Out] FileId% fileid,
                    [Out] Guid% guid )
                {
                    ERR         err             = JET_errSuccess;
                    ::VolumeId  volumeidNative  = ::volumeidInvalid;
                    ::FileId    fileidNative    = ::fileidInvalid;
                    GUID        guidNative      = { 0 };

                    volumeid = VolumeId::Invalid;
                    fileid = FileId::Invalid;
                    guid = Guid::Empty;

                    Call( Pi->ErrGetPhysicalId( &volumeidNative, &fileidNative, (BYTE*)&guidNative ) );

                    volumeid = (VolumeId)volumeidNative;
                    fileid = (FileId)(Int64)fileidNative;
                    guid = Guid(    guidNative.Data1,
                                    guidNative.Data2,
                                    guidNative.Data3,
                                    guidNative.Data4[ 0 ],
                                    guidNative.Data4[ 1 ],
                                    guidNative.Data4[ 2 ],
                                    guidNative.Data4[ 3 ],
                                    guidNative.Data4[ 4 ],
                                    guidNative.Data4[ 5 ],
                                    guidNative.Data4[ 6 ],
                                    guidNative.Data4[ 7 ] );

                    return;

                HandleError:
                    volumeid = VolumeId::Invalid;
                    fileid = FileId::Invalid;
                    guid = Guid::Empty;
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void CacheBase<TM, TN, TW>::Close( VolumeId volumeid, FileId fileid, FileSerial fileserial )
                {
                    ERR err = JET_errSuccess;

                    Call( Pi->ErrClose( (::VolumeId)volumeid, (::FileId)fileid, (::FileSerial)fileserial ) );

                    return;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void CacheBase<TM, TN, TW>::Flush( VolumeId volumeid, FileId fileid, FileSerial fileserial )
                {
                    ERR err = JET_errSuccess;

                    Call( Pi->ErrFlush( (::VolumeId)volumeid, (::FileId)fileid, (::FileSerial)fileserial ) );

                    return;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void CacheBase<TM, TN, TW>::Invalidate(
                    VolumeId volumeid,
                    FileId fileid, 
                    FileSerial fileserial,
                    Int64 offsetInBytes,
                    Int64 sizeInBytes )
                {
                    ERR err = JET_errSuccess;

                    Call( Pi->ErrInvalidate(
                        (::VolumeId)volumeid,
                        (::FileId)fileid,
                        (::FileSerial)fileserial,
                        offsetInBytes, 
                        sizeInBytes ) );

                    return;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void CacheBase<TM, TN, TW>::Read(
                    VolumeId volumeid,
                    FileId fileid,
                    FileSerial fileserial,
                    Int64 offsetInBytes,
                    ArraySegment<byte> data,
                    FileQOS fileQOS,
                    CachingPolicy cachingPolicy,
                    ICache::Complete^ complete )
                {
                    ERR                 err             = JET_errSuccess;
                    const DWORD         cbBlock         = OSMemoryPageCommitGranularity();
                    DWORD               cbBuffer        = ( ( data.Count - 1 + cbBlock ) / cbBlock ) * cbBlock;
                    VOID*               pvBuffer        = NULL;
                    BOOL                fReleaseBuffer  = fTrue;
                    CComplete*          pcomplete       = NULL;
                    TraceContextScope   tcScope;

                    Alloc( pvBuffer = PvOSMemoryPageAlloc( cbBuffer, NULL ) );

                    if ( complete != nullptr )
                    {
                        Alloc( pcomplete = new CComplete( fFalse, data.Array, data.Offset, data.Count, complete, pvBuffer ) );

                        fReleaseBuffer = fFalse;
                    }

                    pin_ptr<const byte> rgbDataIn = &data.Array[ data.Offset ];
                    UtilMemCpy( pvBuffer, (const BYTE*)rgbDataIn, data.Count );

                    Call( Pi->ErrRead(  *tcScope,
                                        (::VolumeId)volumeid,
                                        (::FileId)fileid,
                                        (::FileSerial)fileserial,
                                        offsetInBytes,
                                        data.Count,
                                        (BYTE*)pvBuffer,
                                        (OSFILEQOS)fileQOS,
                                        (::ICache::CachingPolicy)cachingPolicy,
                                        complete != nullptr ? ::ICache::PfnComplete( CComplete::Complete ) : NULL,
                                        DWORD_PTR( pcomplete ) ) );

                HandleError:
                    if ( complete == nullptr )
                    {
                        pin_ptr<byte> rgbData = &data.Array[ data.Offset ];
                        UtilMemCpy( (BYTE*)rgbData, pvBuffer, data.Count );
                    }
                    if ( err < JET_errSuccess )
                    {
                        delete pcomplete;
                    }
                    if ( fReleaseBuffer )
                    {
                        OSMemoryPageFree( pvBuffer );
                    }
                    if ( err >= JET_errSuccess )
                    {
                        return;
                    }
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void CacheBase<TM, TN, TW>::Write(
                    VolumeId volumeid,
                    FileId fileid,
                    FileSerial fileserial,
                    Int64 offsetInBytes,
                    ArraySegment<byte> data,
                    FileQOS fileQOS,
                    CachingPolicy cachingPolicy,
                    ICache::Complete^ complete )
                {
                    ERR                 err             = JET_errSuccess;
                    const DWORD         cbBlock         = OSMemoryPageCommitGranularity();
                    DWORD               cbBuffer        = ( ( data.Count - 1 + cbBlock ) / cbBlock ) * cbBlock;
                    VOID*               pvBuffer        = NULL;
                    BOOL                fReleaseBuffer  = fTrue;
                    CComplete*          pcomplete       = NULL;
                    TraceContextScope   tcScope;

                    Alloc( pvBuffer = PvOSMemoryPageAlloc( cbBuffer, NULL ) );

                    if ( complete != nullptr )
                    {
                        Alloc( pcomplete = new CComplete( fTrue, data.Array, data.Offset, data.Count, complete, pvBuffer ) );

                        fReleaseBuffer = fFalse;
                    }

                    pin_ptr<const byte> rgbData = &data.Array[ data.Offset ];
                    UtilMemCpy( pvBuffer, (const BYTE*)rgbData, data.Count );

                    Call( Pi->ErrWrite( *tcScope,
                                        (::VolumeId)volumeid,
                                        (::FileId)fileid,
                                        (::FileSerial)fileserial,
                                        offsetInBytes,
                                        data.Count,
                                        (const BYTE*)pvBuffer,
                                        (OSFILEQOS)fileQOS,
                                        (::ICache::CachingPolicy)cachingPolicy,
                                        complete != nullptr ? ::ICache::PfnComplete( CComplete::Complete ) : NULL,
                                        DWORD_PTR( pcomplete ) ) );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        delete pcomplete;
                    }
                    if ( fReleaseBuffer )
                    {
                        OSMemoryPageFree( pvBuffer );
                    }
                    if ( err >= JET_errSuccess )
                    {
                        return;
                    }
                    throw EseException( err );
                }
            }
        }
    }
}
