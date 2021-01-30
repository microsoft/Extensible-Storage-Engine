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
                class CJournalSegmentWrapper : public CWrapper<TM, TN>
                {
                    public:

                        CJournalSegmentWrapper( TM^ js ) : CWrapper( js ) { }

                    public:

                        ERR ErrGetPhysicalId( _Out_ QWORD* const pib ) override;

                        ERR ErrGetProperties(   _Out_opt_ ::SegmentPosition* const  pspos,
                                                _Out_opt_ DWORD* const              pdwUniqueId,
                                                _Out_opt_ DWORD* const              pdwUniqueIdPrev,
                                                _Out_opt_ ::SegmentPosition* const  psposReplay,
                                                _Out_opt_ ::SegmentPosition* const  psposDurable,
                                                _Out_opt_ BOOL* const               pfSealed ) override;

                        ERR ErrVisitRegions(    _In_ const ::IJournalSegment::PfnVisitRegion    pfnVisitRegion,
                                                _In_ const DWORD_PTR                            keyVisitRegion ) override;

                        ERR ErrAppendRegion(    _In_                const size_t            cjb,
                                                _In_reads_( cjb )   CJournalBuffer* const   rgjb,
                                                _In_                const DWORD             cbMin,
                                                _Out_               ::RegionPosition* const prpos,
                                                _Out_               DWORD* const            pcbActual ) override;

                        ERR ErrSeal( _In_opt_ ::IJournalSegment::PfnSealed pfnSealed, _In_ const DWORD_PTR keySealed ) override;
                };

                template< class TM, class TN >
                inline ERR CJournalSegmentWrapper<TM, TN>::ErrGetPhysicalId( _Out_ QWORD* const pib )
                {
                    ERR     err = JET_errSuccess;
                    Int64   ib  = 0;

                    *pib = 0;

                    ExCall( ib = I()->GetPhysicalId() );

                    *pib = ib;

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        *pib = 0;
                    }
                    return err;
                }

                template<class TM, class TN>
                inline ERR CJournalSegmentWrapper<TM, TN>::ErrGetProperties(    _Out_opt_::SegmentPosition* const   pspos,
                                                                                _Out_opt_ DWORD* const              pdwUniqueId,
                                                                                _Out_opt_ DWORD* const              pdwUniqueIdPrev,
                                                                                _Out_opt_::SegmentPosition* const   psposReplay,
                                                                                _Out_opt_::SegmentPosition* const   psposDurable,
                                                                                _Out_opt_ BOOL* const               pfSealed )
                {
                    ERR             err                     = JET_errSuccess;
                    SegmentPosition segmentPosition         = SegmentPosition::Invalid;
                    Int32           uniqueId                = 0;
                    Int32           uniqueIdPrevious        = 0;
                    SegmentPosition segmentPositionReplay   = SegmentPosition::Invalid;
                    SegmentPosition segmentPositionDurable  = SegmentPosition::Invalid;
                    bool            isSealed                = false;

                    if ( pspos )
                    {
                        *pspos = ::sposInvalid;
                    }
                    if ( pdwUniqueId )
                    {
                        *pdwUniqueId = 0;
                    }
                    if ( pdwUniqueIdPrev )
                    {
                        *pdwUniqueIdPrev = 0;
                    }
                    if ( psposReplay )
                    {
                        *psposReplay = ::sposInvalid;
                    }
                    if ( psposDurable )
                    {
                        *psposDurable = ::sposInvalid;
                    }
                    if ( pfSealed )
                    {
                        *pfSealed = fFalse;
                    }

                    ExCall( I()->GetProperties( segmentPosition,
                                                uniqueId,
                                                uniqueIdPrevious, 
                                                segmentPositionReplay,
                                                segmentPositionDurable,
                                                isSealed ) );

                    if ( pspos )
                    {
                        *pspos = (::SegmentPosition)segmentPosition;
                    }
                    if ( pdwUniqueId )
                    {
                        *pdwUniqueId = uniqueId;
                    }
                    if ( pdwUniqueIdPrev )
                    {
                        *pdwUniqueIdPrev = uniqueIdPrevious;
                    }
                    if ( psposReplay )
                    {
                        *psposReplay = (::SegmentPosition)segmentPositionReplay;
                    }
                    if ( psposDurable )
                    {
                        *psposDurable = (::SegmentPosition)segmentPositionDurable;
                    }
                    if ( pfSealed )
                    {
                        *pfSealed = isSealed ? fTrue : fFalse;
                    }

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        if ( pspos )
                        {
                            *pspos = ::sposInvalid;
                        }
                        if ( pdwUniqueId )
                        {
                            *pdwUniqueId = 0;
                        }
                        if ( pdwUniqueIdPrev )
                        {
                            *pdwUniqueIdPrev = 0;
                        }
                        if ( psposReplay )
                        {
                            *psposReplay = ::sposInvalid;
                        }
                        if ( psposDurable )
                        {
                            *psposDurable = ::sposInvalid;
                        }
                        if ( pfSealed )
                        {
                            *pfSealed = fFalse;
                        }
                    }
                    return err;
                }

                template<class TM, class TN>
                inline ERR CJournalSegmentWrapper<TM, TN>::ErrVisitRegions( _In_ const ::IJournalSegment::PfnVisitRegion    pfnVisitRegion,
                                                                            _In_ const DWORD_PTR                            keyVisitRegion )
                {
                    ERR             err         = JET_errSuccess;
                    VisitRegion^    visitregion = gcnew VisitRegion( pfnVisitRegion, keyVisitRegion );

                    ExCall( I()->VisitRegions( gcnew Internal::Ese::BlockCache::Interop::IJournalSegment::VisitRegion( visitregion, &VisitRegion::VisitRegion_ ) ) );

                HandleError:
                    return err;
                }

                template<class TM, class TN>
                inline ERR CJournalSegmentWrapper<TM, TN>::ErrAppendRegion( _In_                const size_t            cjb,
                                                                            _In_reads_( cjb )   CJournalBuffer* const   rgjb,
                                                                            _In_                const DWORD             cbMin,
                                                                            _Out_               ::RegionPosition* const prpos,
                                                                            _Out_               DWORD* const            pcbActual )
                {
                    ERR                         err                     = JET_errSuccess;
                    array<ArraySegment<byte>>^  payload                 = rgjb ? gcnew array<ArraySegment<byte>>( cjb ) : nullptr;
                    RegionPosition              regionPosition          = RegionPosition::Invalid;
                    Int32                       payloadAppendedInBytes  = 0;

                    *prpos = ::rposInvalid;
                    *pcbActual = 0;

                    for ( int ijb = 0; ijb < cjb; ijb++ )
                    {
                        const size_t cb = rgjb[ ijb ].Cb();
                        array<byte>^ data = rgjb[ ijb ].Rgb() ? gcnew array<byte>( cb ) : nullptr;
                        if ( cb )
                        {
                            pin_ptr<byte> rgbData = &data[ 0 ];
                            UtilMemCpy( (BYTE*)rgbData, rgjb[ ijb ].Rgb(), cb );
                        }
                        if ( data != nullptr )
                        {
                            payload[ ijb ] = ArraySegment<byte>( data );
                        }
                    }

                    ExCall( I()->AppendRegion(  payload,
                                                cbMin,
                                                regionPosition,
                                                payloadAppendedInBytes) );

                    *prpos = (::RegionPosition)regionPosition;
                    *pcbActual = payloadAppendedInBytes;

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        *prpos = ::rposInvalid;
                        *pcbActual = 0;
                    }
                    return err;
                }

                template<class TM, class TN>
                inline ERR CJournalSegmentWrapper<TM, TN>::ErrSeal( _In_opt_    ::IJournalSegment::PfnSealed    pfnSealed, 
                                                                    _In_        const DWORD_PTR                 keySealed )
                {
                    ERR     err     = JET_errSuccess;
                    Sealed^ sealed  = pfnSealed ? gcnew Sealed( pfnSealed, keySealed ) : nullptr;

                    ExCall( I()->Seal( pfnSealed ? gcnew Internal::Ese::BlockCache::Interop::IJournalSegment::Sealed( sealed, &Sealed::Sealed_ ) : nullptr ) );

                HandleError:
                    return err;
                }
            }
        }
    }
}
