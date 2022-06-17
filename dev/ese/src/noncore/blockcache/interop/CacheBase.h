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
                public ref class CacheBase : Base<TM, TN, TW>, ICache
                {
                    public:

                        CacheBase( TM^ c ) : Base( c ) { }

                        CacheBase( TN** const ppc ) : Base( ppc ) {}

                    public:

                        virtual void Create();

                        virtual void Mount();

                        virtual bool IsEnabled();

                        virtual void GetPhysicalId( [Out] VolumeId% volumeid, [Out] FileId% fileid, [Out] Guid% guid );

                        virtual void Close( VolumeId volumeid, FileId fileid, FileSerial fileserial );

                        virtual void Flush( VolumeId volumeid, FileId fileid, FileSerial fileserial );

                        virtual void Destage(
                            VolumeId volumeid, 
                            FileId fileid, 
                            FileSerial fileserial,
                            ICache::DestageStatus^ destageStatus );

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
                            MemoryStream^ data,
                            FileQOS fileQOS,
                            CachingPolicy cachingPolicy,
                            ICache::Complete^ complete );

                        virtual void Write(
                            VolumeId volumeid,
                            FileId fileid,
                            FileSerial fileserial,
                            Int64 offsetInBytes,
                            MemoryStream^ data,
                            FileQOS fileQOS,
                            CachingPolicy cachingPolicy,
                            ICache::Complete^ complete );

                        virtual void Issue(
                            VolumeId volumeid,
                            FileId fileid,
                            FileSerial fileserial );
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
                inline bool CacheBase<TM, TN, TW>::IsEnabled()
                {
                    return Pi->FEnabled() ? true : false;
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
                inline void CacheBase<TM, TN, TW>::Destage(
                    VolumeId volumeid, 
                    FileId fileid, 
                    FileSerial fileserial,
                    ICache::DestageStatus^ destageStatus )
                {
                    ERR                     err                     = JET_errSuccess;
                    DestageStatusInverse^   destageStatusInverse    = nullptr;

                    if ( destageStatus != nullptr )
                    {
                        destageStatusInverse = gcnew DestageStatusInverse( destageStatus );
                    }

                    Call( Pi->ErrDestage(   (::VolumeId)volumeid,
                                            (::FileId)fileid,
                                            (::FileSerial)fileserial,
                                            destageStatusInverse == nullptr ?
                                                NULL :
                                                destageStatusInverse->PfnDestageStatus,
                                            destageStatusInverse == nullptr ?
                                                NULL : 
                                                destageStatusInverse->KeyDestageStatus) );

                HandleError:
                    delete destageStatusInverse;
                    if ( err < JET_errSuccess )
                    {
                        throw EseException( err );
                    }
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
                    MemoryStream^ data,
                    FileQOS fileQOS,
                    CachingPolicy cachingPolicy,
                    ICache::Complete^ complete )
                {
                    ERR                 err             = JET_errSuccess;
                    const DWORD         cbData          = data == nullptr ? 0 : (DWORD)data->Length;
                    VOID*               pvBuffer        = NULL;
                    CompleteInverse^    completeInverse = nullptr;
                    TraceContextScope   tcScope( iorpBlockCache );

                    if ( cbData )
                    {
                        Alloc( pvBuffer = PvOSMemoryPageAlloc( roundup( cbData, OSMemoryPageCommitGranularity() ), NULL ) );
                        array<BYTE>^ bytes = data->ToArray();
                        pin_ptr<const BYTE> rgbData = &bytes[ 0 ];
                        UtilMemCpy( pvBuffer, (const BYTE*)rgbData, cbData );
                    }

                    if ( complete != nullptr )
                    {
                        completeInverse = gcnew CompleteInverse( false, data, complete, &pvBuffer );
                    }

                    Call( Pi->ErrRead(  *tcScope,
                                        (::VolumeId)volumeid,
                                        (::FileId)fileid,
                                        (::FileSerial)fileserial,
                                        offsetInBytes,
                                        cbData,
                                        (BYTE*)( completeInverse == nullptr ? pvBuffer : completeInverse->PvBuffer ),
                                        (OSFILEQOS)fileQOS,
                                        (::ICache::CachingPolicy)cachingPolicy,
                                        completeInverse == nullptr ? NULL : completeInverse->PfnComplete,
                                        completeInverse == nullptr ? NULL : completeInverse->KeyComplete ) );

                HandleError:
                    if ( completeInverse == nullptr && cbData )
                    {
                        array<BYTE>^ bytes = gcnew array<BYTE>( cbData );
                        pin_ptr<const BYTE> rgbData = &bytes[ 0 ];
                        UtilMemCpy( (BYTE*)rgbData, completeInverse == nullptr ? pvBuffer : completeInverse->PvBuffer, cbData );
                        data->Position = 0;
                        data->Write( bytes, 0, bytes->Length );
                    }
                    OSMemoryPageFree( pvBuffer );
                    if ( err < JET_errSuccess )
                    {
                        delete completeInverse;
                        throw EseException( err );
                    }
                }

                template< class TM, class TN, class TW >
                inline void CacheBase<TM, TN, TW>::Write(
                    VolumeId volumeid,
                    FileId fileid,
                    FileSerial fileserial,
                    Int64 offsetInBytes,
                    MemoryStream^ data,
                    FileQOS fileQOS,
                    CachingPolicy cachingPolicy,
                    ICache::Complete^ complete )
                {
                    ERR                 err             = JET_errSuccess;
                    const DWORD         cbData          = data == nullptr ? 0 : (DWORD)data->Length;
                    VOID*               pvBuffer        = NULL;
                    CompleteInverse^    completeInverse = nullptr;
                    TraceContextScope   tcScope( iorpBlockCache );

                    if ( cbData )
                    {
                        Alloc( pvBuffer = PvOSMemoryPageAlloc( roundup( cbData, OSMemoryPageCommitGranularity() ), NULL ) );
                        array<BYTE>^ bytes = data->ToArray();
                        pin_ptr<const BYTE> rgbData = &bytes[ 0 ];
                        UtilMemCpy( pvBuffer, (const BYTE*)rgbData, cbData );
                    }

                    if ( complete != nullptr )
                    {
                        completeInverse = gcnew CompleteInverse( true, data, complete, &pvBuffer );
                    }

                    Call( Pi->ErrWrite( *tcScope,
                                        (::VolumeId)volumeid,
                                        (::FileId)fileid,
                                        (::FileSerial)fileserial,
                                        offsetInBytes,
                                        cbData,
                                        (BYTE*)( completeInverse == nullptr ? pvBuffer : completeInverse->PvBuffer ),
                                        (OSFILEQOS)fileQOS,
                                        (::ICache::CachingPolicy)cachingPolicy,
                                        completeInverse == nullptr ? NULL : completeInverse->PfnComplete,
                                        completeInverse == nullptr ? NULL : completeInverse->KeyComplete ) );

                HandleError:
                    OSMemoryPageFree( pvBuffer );
                    if ( err < JET_errSuccess )
                    {
                        delete completeInverse;
                        throw EseException( err );
                    }
                }

                template< class TM, class TN, class TW >
                inline void CacheBase<TM, TN, TW>::Issue(
                    VolumeId volumeid,
                    FileId fileid,
                    FileSerial fileserial )
                {
                    ERR                 err             = JET_errSuccess;

                    Call( Pi->ErrIssue( (::VolumeId)volumeid,
                                        (::FileId)fileid,
                                        (::FileSerial)fileserial ) );

                HandleError:
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
