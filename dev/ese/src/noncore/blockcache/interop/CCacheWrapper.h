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
                class CCacheWrapper : public CWrapper<TM, TN>
                {
                    public:

                        CCacheWrapper( TM^ c ) : CWrapper( c ) { }

                    public:

                        ERR ErrCreate() override;

                        ERR ErrMount() override;

                        ERR ErrPrepareToDismount() override;

                        ERR ErrDump( _In_ CPRINTF* const pcprintf ) override;

                        BOOL FEnabled() override;

                        ERR ErrGetPhysicalId(   _Out_                   ::VolumeId* const   pvolumeid,
                                                _Out_                   ::FileId* const     pfileid,
                                                _Out_writes_( cbGuid )  BYTE* const         rgbUniqueId ) override;
                
                        ERR ErrClose(   _In_ const ::VolumeId   volumeid,
                                        _In_ const ::FileId     fileid,
                                        _In_ const ::FileSerial fileserial ) override;
                
                        ERR ErrFlush(   _In_ const ::VolumeId   volumeid,
                                        _In_ const ::FileId     fileid,
                                        _In_ const ::FileSerial fileserial ) override;
                
                        ERR ErrDestage( _In_        const ::VolumeId                    volumeid,
                                        _In_        const ::FileId                      fileid,
                                        _In_        const ::FileSerial                  fileserial,
                                        _In_opt_    const ::ICache::PfnDestageStatus    pfnDestageStatus,
                                        _In_opt_    const DWORD_PTR                     keyDestageStatus ) override;

                        ERR ErrInvalidate(  _In_ const ::VolumeId   volumeid,
                                            _In_ const ::FileId     fileid,
                                            _In_ const ::FileSerial fileserial,
                                            _In_ const QWORD        ibOffset,
                                            _In_ const QWORD        cbData ) override;

                        ERR ErrRead(    _In_                    const TraceContext&             tc,
                                        _In_                    const ::VolumeId                volumeid,
                                        _In_                    const ::FileId                  fileid,
                                        _In_                    const ::FileSerial              fileserial,
                                        _In_                    const QWORD                     ibOffset,
                                        _In_                    const DWORD                     cbData,
                                        _Out_writes_( cbData )  BYTE* const                     pbData,
                                        _In_                    const OSFILEQOS                 grbitQOS,
                                        _In_                    const ::ICache::CachingPolicy   cp,
                                        _In_opt_                const ::ICache::PfnComplete     pfnComplete,
                                        _In_opt_                const DWORD_PTR                 keyComplete ) override;

                        ERR ErrWrite(   _In_                    const TraceContext&             tc,
                                        _In_                    const ::VolumeId                volumeid,
                                        _In_                    const ::FileId                  fileid,
                                        _In_                    const ::FileSerial              fileserial,
                                        _In_                    const QWORD                     ibOffset,
                                        _In_                    const DWORD                     cbData,
                                        _In_reads_( cbData )    const BYTE* const               pbData,
                                        _In_                    const OSFILEQOS                 grbitQOS,
                                        _In_                    const ::ICache::CachingPolicy   cp,
                                        _In_opt_                const ::ICache::PfnComplete     pfnComplete,
                                        _In_opt_                const DWORD_PTR                 keyComplete ) override;

                        ERR ErrIssue(   _In_                    const ::VolumeId                volumeid,
                                        _In_                    const ::FileId                  fileid,
                                        _In_                    const ::FileSerial              fileserial ) override;
                };

                template< class TM, class TN >
                inline ERR CCacheWrapper<TM, TN>::ErrCreate()
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->Create() );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CCacheWrapper<TM, TN>::ErrMount()
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->Mount() );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CCacheWrapper<TM, TN>::ErrPrepareToDismount()
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->PrepareToDismount() );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CCacheWrapper<TM, TN>::ErrDump( _In_ CPRINTF* const pcprintf )
                {
                    return JET_errSuccess;
                }

                template< class TM, class TN >
                inline BOOL CCacheWrapper<TM, TN>::FEnabled()
                {
                    return I()->IsEnabled() ? fTrue : fFalse;
                }

                template< class TM, class TN >
                inline ERR CCacheWrapper<TM, TN>::ErrGetPhysicalId( _Out_                   ::VolumeId* const   pvolumeid,
                                                                    _Out_                   ::FileId* const     pfileid,
                                                                    _Out_writes_( cbGuid )  BYTE* const         rgbUniqueId )
                {
                    ERR         err         = JET_errSuccess;
                    VolumeId    volumeid    = VolumeId::Invalid;
                    FileId      fileid      = FileId::Invalid;
                    Guid        guid        = Guid::Empty;

                    *pvolumeid = ::volumeidInvalid;
                    *pfileid = ::fileidInvalid;
                    memset( rgbUniqueId, 0, cbGuid );

                    ExCall( I()->GetPhysicalId( volumeid, fileid, guid ) );

                    *pvolumeid = (::VolumeId)volumeid;
                    *pfileid = (::FileId)fileid;

                    array<BYTE>^ uniqueIdBytes = guid.ToByteArray();
                    pin_ptr<BYTE> uniqueId = &uniqueIdBytes[ 0 ];
                    UtilMemCpy( rgbUniqueId, uniqueId, cbGuid );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        *pvolumeid = ::volumeidInvalid;
                        *pfileid = ::fileidInvalid;
                        memset( rgbUniqueId, 0, cbGuid );
                    }
                    return err;
                }
                                
                template< class TM, class TN >
                inline ERR CCacheWrapper<TM, TN>::ErrClose( _In_ const ::VolumeId   volumeid,
                                                            _In_ const ::FileId     fileid,
                                                            _In_ const ::FileSerial fileserial )
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->Close( (VolumeId)volumeid, (FileId)fileid, (FileSerial)fileserial ) );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CCacheWrapper<TM, TN>::ErrFlush( _In_ const ::VolumeId   volumeid,
                                                            _In_ const ::FileId     fileid,
                                                            _In_ const ::FileSerial fileserial )
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->Flush( (VolumeId)volumeid, (FileId)fileid, (FileSerial)fileserial ) );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CCacheWrapper<TM, TN>::ErrDestage(   _In_        const ::VolumeId                    volumeid,
                                                                _In_        const ::FileId                      fileid,
                                                                _In_        const ::FileSerial                  fileserial,
                                                                _In_opt_    const ::ICache::PfnDestageStatus    pfnDestageStatus,
                                                                _In_opt_    const DWORD_PTR                     keyDestageStatus )
                {
                    ERR             err             = JET_errSuccess;
                    DestageStatus^  destageStatus   = pfnDestageStatus ?
                                                        gcnew DestageStatus( pfnDestageStatus, keyDestageStatus ) :
                                                        nullptr;

                    ExCall( I()->Destage(   (VolumeId)volumeid,
                                            (FileId)fileid,
                                            (FileSerial)fileserial,
                                            pfnDestageStatus ?
                                                gcnew Internal::Ese::BlockCache::Interop::ICache::DestageStatus( destageStatus, &DestageStatus::DestageStatus_ ) :
                                                nullptr ) );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CCacheWrapper<TM, TN>::ErrInvalidate(    _In_ const ::VolumeId   volumeid,
                                                                    _In_ const ::FileId     fileid,
                                                                    _In_ const ::FileSerial fileserial,
                                                                    _In_ const QWORD        ibOffset,
                                                                    _In_ const QWORD        cbData )
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->Invalidate( (VolumeId)volumeid, (FileId)fileid, (FileSerial)fileserial, ibOffset, cbData ) );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CCacheWrapper<TM, TN>::ErrRead(  _In_                    const TraceContext&             tc,
                                                            _In_                    const ::VolumeId                volumeid,
                                                            _In_                    const ::FileId                  fileid,
                                                            _In_                    const ::FileSerial              fileserial,
                                                            _In_                    const QWORD                     ibOffset,
                                                            _In_                    const DWORD                     cbData,
                                                            _Out_writes_( cbData )  BYTE* const                     pbData,
                                                            _In_                    const OSFILEQOS                 grbitQOS,
                                                            _In_                    const ::ICache::CachingPolicy   cp,
                                                            _In_opt_                const ::ICache::PfnComplete     pfnComplete,
                                                            _In_opt_                const DWORD_PTR                 keyComplete )
                {
                    ERR             err         = JET_errSuccess;
                    array<BYTE>^    buffer      = !pbData ? nullptr : gcnew array<BYTE>( cbData );
                    pin_ptr<BYTE>   rgbData     = ( !pbData || !cbData ) ? nullptr : &buffer[ 0 ];
                    MemoryStream^   stream      = !pbData ? nullptr : gcnew MemoryStream( buffer, true );
                    Complete^       complete    = pfnComplete ?
                                                        gcnew Complete( false, pbData, pfnComplete, keyComplete ) :
                                                        nullptr;

                    if ( cbData )
                    {
                        UtilMemCpy( (BYTE*)rgbData, pbData, cbData );
                    }

                    ExCall( I()->Read(  (VolumeId)volumeid,
                                        (FileId)fileid,
                                        (FileSerial)fileserial,
                                        ibOffset,
                                        stream,
                                        (FileQOS)grbitQOS,
                                        (Internal::Ese::BlockCache::Interop::CachingPolicy)cp,
                                        pfnComplete ?
                                            gcnew Internal::Ese::BlockCache::Interop::ICache::Complete( complete, &Complete::Complete_ ) :
                                            nullptr ) );

                HandleError:
                    if ( !pfnComplete && cbData )
                    {
                        UtilMemCpy( pbData, (const BYTE*)rgbData, cbData );
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CCacheWrapper<TM, TN>::ErrWrite( _In_                    const TraceContext&             tc,
                                                            _In_                    const ::VolumeId                volumeid,
                                                            _In_                    const ::FileId                  fileid,
                                                            _In_                    const ::FileSerial              fileserial,
                                                            _In_                    const QWORD                     ibOffset,
                                                            _In_                    const DWORD                     cbData,
                                                            _In_reads_( cbData )    const BYTE* const               pbData,
                                                            _In_                    const OSFILEQOS                 grbitQOS,
                                                            _In_                    const ::ICache::CachingPolicy   cp,
                                                            _In_opt_                const ::ICache::PfnComplete     pfnComplete,
                                                            _In_opt_                const DWORD_PTR                 keyComplete )
                {
                    ERR             err         = JET_errSuccess;
                    array<BYTE>^    buffer      = !pbData ? nullptr : gcnew array<BYTE>( cbData );
                    pin_ptr<BYTE>   rgbData     = ( !pbData || !cbData ) ? nullptr : &buffer[ 0 ];
                    MemoryStream^   stream      = !pbData ? nullptr : gcnew MemoryStream( buffer, false );
                    Complete^       complete    = pfnComplete ?
                                                        gcnew Complete( true, 
                                                                        const_cast<BYTE*>( pbData ), 
                                                                        pfnComplete, 
                                                                        keyComplete ) :
                                                        nullptr;

                    if ( cbData )
                    {
                        UtilMemCpy( (BYTE*)rgbData, pbData, cbData );
                    }

                    ExCall( I()->Write( (VolumeId)volumeid,
                                        (FileId)fileid,
                                        (FileSerial)fileserial,
                                        ibOffset,
                                        stream,
                                        (FileQOS)grbitQOS,
                                        (Internal::Ese::BlockCache::Interop::CachingPolicy)cp,
                                        pfnComplete ?
                                            gcnew Internal::Ese::BlockCache::Interop::ICache::Complete( complete, &Complete::Complete_ ) :
                                            nullptr ) );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CCacheWrapper<TM, TN>::ErrIssue( _In_                    const ::VolumeId                volumeid,
                                                            _In_                    const ::FileId                  fileid,
                                                            _In_                    const ::FileSerial              fileserial )
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->Issue( (VolumeId)volumeid, (FileId)fileid, (FileSerial)fileserial ) );

                HandleError:
                    return err;
                }
            }
        }
    }
}
