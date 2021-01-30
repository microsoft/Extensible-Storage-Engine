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
                public ref class FileFilterBase : public FileBase<TM,TN,TW>, IFileFilter
                {
                    public:

                        FileFilterBase( TM^ ff ) : FileBase( ff ) { }

                        FileFilterBase( TN** const ppff ) : FileBase( ppff ) { }

                    public:

                        virtual void GetPhysicalId(
                            [Out] VolumeId% volumeid,
                            [Out] FileId% fileid, 
                            [Out] FileSerial% fileserial );

                        virtual void Read(
                            Int64 offsetInBytes,
                            ArraySegment<byte> data,
                            FileQOS fileQOS,
                            IOMode ioMode,
                            IFile::IOComplete^ ioComplete,
                            IFile::IOHandoff^ ioHandoff );

                        virtual void Write(
                            Int64 offsetInBytes,
                            ArraySegment<byte> data,
                            FileQOS fileQOS,
                            IOMode ioMode,
                            IFile::IOComplete^ ioComplete,
                            IFile::IOHandoff^ ioHandoff );

                        virtual void Issue( IOMode ioMode );
                };

                template< class TM, class TN, class TW >
                inline void FileFilterBase<TM, TN, TW>::GetPhysicalId(
                    [Out] VolumeId% volumeid,
                    [Out] FileId% fileid,
                    [Out] FileSerial% fileserial )
                {
                    ERR             err                 = JET_errSuccess;
                    ::VolumeId      volumeidNative      = ::volumeidInvalid;
                    ::FileId        fileidNative        = ::fileidInvalid;
                    ::FileSerial    fileserialNative    = ::fileserialInvalid;

                    volumeid = VolumeId::Invalid;
                    fileid = FileId::Invalid;
                    fileserial = FileSerial::Invalid;

                    Call( Pi->ErrGetPhysicalId( &volumeidNative, &fileidNative, &fileserialNative ) );

                    volumeid = (VolumeId)volumeidNative;
                    fileid = (FileId)(Int64)fileidNative;
                    fileserial = (FileSerial)fileserialNative;

                    return;

                HandleError:
                    volumeid = VolumeId::Invalid;
                    fileid = FileId::Invalid;
                    fileserial = FileSerial::Invalid;
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void FileFilterBase<TM,TN,TW>::Read(
                    Int64 offsetInBytes,
                    ArraySegment<byte> data,
                    FileQOS fileQOS,
                    IOMode ioMode,
                    IFile::IOComplete^ ioComplete,
                    IFile::IOHandoff^ ioHandoff )
                {
                    ERR                 err             = JET_errSuccess;
                    const DWORD         cbBlock         = OSMemoryPageCommitGranularity();
                    DWORD               cbBuffer        = ( ( data.Count - 1 + cbBlock ) / cbBlock ) * cbBlock;
                    VOID*               pvBuffer        = NULL;
                    BOOL                fReleaseBuffer  = fTrue;
                    TraceContextScope   tcScope;
                    CIOComplete*        piocomplete     = NULL;

                    Alloc( pvBuffer = PvOSMemoryPageAlloc( cbBuffer, NULL ) );

                    if ( ioComplete != nullptr || ioHandoff != nullptr )
                    {
                        if ( ioComplete != nullptr )
                        {
                            Alloc( piocomplete = new CIOComplete( fTrue, this, fFalse, data.Array, data.Offset, data.Count, ioComplete, ioHandoff, pvBuffer ) );
                        }
                        else
                        {
                            piocomplete = new( _alloca( sizeof( CIOComplete ) ) ) CIOComplete( fFalse, this, fFalse, data.Array, data.Offset, data.Count, ioComplete, ioHandoff, pvBuffer );
                        }

                        fReleaseBuffer = fFalse;
                    }

                    pin_ptr<const byte> rgbDataIn = &data.Array[ data.Offset ];
                    UtilMemCpy( pvBuffer, (const BYTE*)rgbDataIn, data.Count );

                    Call( Pi->ErrRead(  *tcScope,
                                        offsetInBytes,
                                        data.Count,
                                        (BYTE*)pvBuffer,
                                        (OSFILEQOS)fileQOS,
                                        (::IFileFilter::IOMode)ioMode,
                                        ioComplete != nullptr ? IFileAPI::PfnIOComplete( CIOComplete::Complete ) : NULL,
                                        DWORD_PTR( piocomplete ),
                                        piocomplete ? IFileAPI::PfnIOHandoff( CIOComplete::Handoff ) : NULL,
                                        NULL ) );

                HandleError:
                    if ( ioComplete == nullptr )
                    {
                        pin_ptr<byte> rgbData = &data.Array[ data.Offset ];
                        UtilMemCpy( (BYTE*)rgbData, pvBuffer, data.Count );
                    }
                    if ( piocomplete && ( ioComplete == nullptr || err < JET_errSuccess ) )
                    {
                        piocomplete->Release();
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
                inline void FileFilterBase<TM,TN,TW>::Write(
                    Int64 offsetInBytes,
                    ArraySegment<byte> data,
                    FileQOS fileQOS,
                    IOMode ioMode,
                    IFile::IOComplete^ ioComplete,
                    IFile::IOHandoff^ ioHandoff )
                {
                    ERR                 err             = JET_errSuccess;
                    const DWORD         cbBlock         = OSMemoryPageCommitGranularity();
                    DWORD               cbBuffer        = ( ( data.Count - 1 + cbBlock ) / cbBlock ) * cbBlock;
                    VOID*               pvBuffer        = NULL;
                    BOOL                fReleaseBuffer  = fTrue;
                    TraceContextScope   tcScope;
                    CIOComplete*        piocomplete     = NULL;

                    Alloc( pvBuffer = PvOSMemoryPageAlloc( cbBuffer, NULL ) );

                    if ( ioComplete != nullptr || ioHandoff != nullptr )
                    {
                        if ( ioComplete != nullptr )
                        {
                            Alloc( piocomplete = new CIOComplete( fTrue, this, fTrue, data.Array, data.Offset, data.Count, ioComplete, ioHandoff, pvBuffer ) );
                        }
                        else
                        {
                            piocomplete = new( _alloca( sizeof( CIOComplete ) ) ) CIOComplete( fFalse, this, fTrue, data.Array, data.Offset, data.Count, ioComplete, ioHandoff, pvBuffer );
                        }

                        fReleaseBuffer = fFalse;
                    }

                    pin_ptr<const byte> rgbData = &data.Array[ data.Offset ];
                    UtilMemCpy( pvBuffer, (const BYTE*)rgbData, data.Count );

                    Call( Pi->ErrWrite( *tcScope,
                                        offsetInBytes,
                                        data.Count,
                                        (const BYTE*)pvBuffer,
                                        (OSFILEQOS)fileQOS,
                                        (::IFileFilter::IOMode)ioMode,
                                        ioComplete != nullptr ? IFileAPI::PfnIOComplete( CIOComplete::Complete ) : NULL,
                                        DWORD_PTR( piocomplete ),
                                        piocomplete ? IFileAPI::PfnIOHandoff( CIOComplete::Handoff ) : NULL ) );

                HandleError:
                    if ( piocomplete && ( ioComplete == nullptr || err < JET_errSuccess ) )
                    {
                        piocomplete->Release();
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
                inline void FileFilterBase<TM, TN, TW>::Issue( IOMode ioMode )
                {
                    ERR err = JET_errSuccess;

                    Call( Pi->ErrIssue( (::IFileFilter::IOMode)ioMode ) );

                    return;

                HandleError:
                    throw EseException( err );
                }
            }
        }
    }
}
