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
                class CFileFilterWrapper : public CFileWrapper<TM, TN>
                {
                    public:

                        CFileFilterWrapper( TM^ ff ) : CFileWrapper( ff ) { }

                    public:

                        ERR ErrGetPhysicalId(   _Out_ ::VolumeId* const     pvolumeid,
                                                _Out_ ::FileId* const       pfileid,
                                                _Out_ ::FileSerial* const   pfileserial ) override;

                        ERR ErrRead(    _In_                    const TraceContext&             tc,
                                        _In_                    const QWORD                     ibOffset,
                                        _In_                    const DWORD                     cbData,
                                        _Out_writes_( cbData )  BYTE* const                     pbData,
                                        _In_                    const OSFILEQOS                 grbitQOS,
                                        _In_                    const ::IFileFilter::IOMode     iom,
                                        _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                        _In_opt_                const DWORD_PTR                 keyIOComplete,
                                        _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                        _In_opt_                const VOID *                    pioreq ) override;

                        ERR ErrWrite(   _In_                    const TraceContext&             tc,
                                        _In_                    const QWORD                     ibOffset,
                                        _In_                    const DWORD                     cbData,
                                        _In_reads_( cbData )    const BYTE* const               pbData,
                                        _In_                    const OSFILEQOS                 grbitQOS,
                                        _In_                    const ::IFileFilter::IOMode     iom,
                                        _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                        _In_opt_                const DWORD_PTR                 keyIOComplete,
                                        _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff ) override;

                        ERR ErrIssue( _In_ const ::IFileFilter::IOMode iom ) override;

                        ERR ErrFlush( _In_ const IOFLUSHREASON iofr, _In_ const ::IFileFilter::IOMode iom ) override;
                };

                template< class TM, class TN >
                inline ERR CFileFilterWrapper<TM, TN>::ErrGetPhysicalId(    _Out_ ::VolumeId* const     pvolumeid,
                                                                            _Out_ ::FileId* const       pfileid,
                                                                            _Out_ ::FileSerial* const   pfileserial )
                {
                    ERR         err         = JET_errSuccess;
                    VolumeId    volumeid    = VolumeId::Invalid;
                    FileId      fileid      = FileId::Invalid;
                    FileSerial  fileserial  = FileSerial::Invalid;

                    *pvolumeid = ::volumeidInvalid;
                    *pfileid = ::fileidInvalid;
                    *pfileserial = ::fileserialInvalid;

                    ExCall( I()->GetPhysicalId( volumeid, fileid, fileserial ) );

                    *pvolumeid = (::VolumeId)volumeid;
                    *pfileid = (::FileId)fileid;
                    *pfileserial = (::FileSerial)fileserial;

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        *pvolumeid = ::volumeidInvalid;
                        *pfileid = ::fileidInvalid;
                        *pfileserial = ::fileserialInvalid;
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileFilterWrapper<TM, TN>::ErrRead( _In_                    const TraceContext&             tc,
                                                                _In_                    const QWORD                     ibOffset,
                                                                _In_                    const DWORD                     cbData,
                                                                _Out_writes_( cbData )  BYTE* const                     pbData,
                                                                _In_                    const OSFILEQOS                 grbitQOS,
                                                                _In_                    const ::IFileFilter::IOMode     iom,
                                                                _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                                                _In_opt_                const DWORD_PTR                 keyIOComplete,
                                                                _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                                                _In_opt_                const VOID *                    pioreq )
                {
                    ERR             err         = JET_errSuccess;
                    array<byte>^    buffer      = !pbData ? nullptr : gcnew array<byte>( cbData );
                    pin_ptr<byte>   rgbData     = ( !pbData || !cbData ) ? nullptr : &buffer[ 0 ];
                    MemoryStream^   stream      = !pbData ? nullptr : gcnew MemoryStream( buffer, true );
                    IOComplete^     iocomplete  = ( pfnIOComplete || pfnIOHandoff ) ?
                                                        gcnew IOComplete(   this,
                                                                            false, 
                                                                            pbData, 
                                                                            pfnIOComplete, 
                                                                            keyIOComplete, 
                                                                            pfnIOHandoff ) :
                                                        nullptr;

                    if ( cbData )
                    {
                        UtilMemCpy( (BYTE*)rgbData, pbData, cbData );
                    }

                    ExCall( I()->Read(  ibOffset,
                                        stream,
                                        (FileQOS)grbitQOS,
                                        (Internal::Ese::BlockCache::Interop::IOMode)iom,
                                        pfnIOComplete ?
                                            gcnew IFile::IOComplete( iocomplete, &IOComplete::Complete ) :
                                            nullptr,
                                        iocomplete != nullptr ?
                                            gcnew IFile::IOHandoff( iocomplete, &IOComplete::Handoff ) :
                                            nullptr ) );

                HandleError:
                    if ( !pfnIOComplete && cbData )
                    {
                        UtilMemCpy( pbData, (const BYTE*)rgbData, cbData );
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileFilterWrapper<TM, TN>::ErrWrite(    _In_                    const TraceContext&             tc,
                                                                    _In_                    const QWORD                     ibOffset,
                                                                    _In_                    const DWORD                     cbData,
                                                                    _In_reads_( cbData )    const BYTE* const               pbData,
                                                                    _In_                    const OSFILEQOS                 grbitQOS,
                                                                    _In_                    const ::IFileFilter::IOMode     iom,
                                                                    _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                                                    _In_opt_                const DWORD_PTR                 keyIOComplete,
                                                                    _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff )
                {
                    ERR             err         = JET_errSuccess;
                    array<byte>^    buffer      = !pbData ? nullptr : gcnew array<byte>( cbData );
                    pin_ptr<byte>   rgbData     = ( !pbData || !cbData ) ? nullptr : &buffer[ 0 ];
                    MemoryStream^   stream      = !pbData ? nullptr : gcnew MemoryStream( buffer, false );
                    IOComplete^     iocomplete  = ( pfnIOComplete || pfnIOHandoff ) ?
                                                        gcnew IOComplete(   this,
                                                                            true, 
                                                                            const_cast<BYTE*>( pbData ), 
                                                                            pfnIOComplete, 
                                                                            keyIOComplete, 
                                                                            pfnIOHandoff ) :
                                                        nullptr;

                    if ( cbData )
                    {
                        UtilMemCpy( (BYTE*)rgbData, pbData, cbData );
                    }

                    ExCall( I()->Write( ibOffset,
                                        stream,
                                        (FileQOS)grbitQOS,
                                        (Internal::Ese::BlockCache::Interop::IOMode)iom,
                                        pfnIOComplete ?
                                            gcnew IFile::IOComplete( iocomplete, &IOComplete::Complete ) :
                                            nullptr,
                                        iocomplete != nullptr ?
                                            gcnew IFile::IOHandoff( iocomplete, &IOComplete::Handoff ) :
                                            nullptr ) );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileFilterWrapper<TM, TN>::ErrIssue( _In_ const ::IFileFilter::IOMode iom )
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->Issue( (Internal::Ese::BlockCache::Interop::IOMode)iom ) );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileFilterWrapper<TM, TN>::ErrFlush(    _In_ const IOFLUSHREASON            iofr,
                                                                    _In_ const ::IFileFilter::IOMode    iom )
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->Flush( (Internal::Ese::BlockCache::Interop::IOMode)iom ) );

                HandleError:
                    return err;
                }
            }
        }
    }
}
