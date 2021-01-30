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
                class CJournalWrapper : public CWrapper<TM, TN>
                {
                    public:

                        CJournalWrapper( TM^ j ) : CWrapper( j ) { }

                    public:

                        ERR ErrGetProperties(   _Out_opt_ ::JournalPosition* const  pjposReplay,
                                                _Out_opt_ ::JournalPosition* const  pjposDurableForWriteBack,
                                                _Out_opt_ ::JournalPosition* const  pjposDurable ) override;

                        ERR ErrVisitEntries(    _In_ const ::IJournal::PfnVisitEntry    pfnVisitEntry,
                                                _In_ const DWORD_PTR                    keyVisitEntry ) override;

                        ERR ErrRepair( _In_ const ::JournalPosition jposLast ) override;

                        ERR ErrAppendEntry( _In_                const size_t                cjb,
                                            _In_reads_( cjb )   CJournalBuffer* const       rgjb,
                                            _Out_               ::JournalPosition* const    pjpos ) override;

                        ERR ErrFlush() override;

                        ERR ErrTruncate( _In_ const ::JournalPosition jposReplay ) override;
                };

                template<class TM, class TN>
                inline ERR CJournalWrapper<TM, TN>::ErrGetProperties(   _Out_opt_ ::JournalPosition* const  pjposReplay,
                                                                        _Out_opt_ ::JournalPosition* const  pjposDurableForWriteBack,
                                                                        _Out_opt_ ::JournalPosition* const  pjposDurable )
                {
                    ERR             err                                 = JET_errSuccess;
                    JournalPosition journalPositionReplay               = JournalPosition::Invalid;
                    JournalPosition journalPositionDurableForWriteBack  = JournalPosition::Invalid;
                    JournalPosition journalPositionDurable              = JournalPosition::Invalid;

                    if ( pjposReplay )
                    {
                        *pjposReplay = ::jposInvalid;
                    }
                    if ( pjposDurableForWriteBack )
                    {
                        *pjposDurableForWriteBack = ::jposInvalid;
                    }
                    if ( pjposDurable )
                    {
                        *pjposDurable = ::jposInvalid;
                    }

                    ExCall( I()->GetProperties( journalPositionReplay,
                                                journalPositionDurableForWriteBack,
                                                journalPositionDurable ) );

                    if ( pjposReplay )
                    {
                        *pjposReplay = (::JournalPosition)journalPositionReplay;
                    }
                    if ( pjposDurableForWriteBack )
                    {
                        *pjposDurableForWriteBack = (::JournalPosition)journalPositionDurableForWriteBack;
                    }
                    if ( pjposDurable )
                    {
                        *pjposDurable = (::JournalPosition)journalPositionDurable;
                    }

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        if ( pjposReplay )
                        {
                            *pjposReplay = ::jposInvalid;
                        }
                        if ( pjposDurableForWriteBack )
                        {
                            *pjposDurableForWriteBack = ::jposInvalid;
                        }
                        if ( pjposDurable )
                        {
                            *pjposDurable = ::jposInvalid;
                        }
                    }
                    return err;
                }

                template<class TM, class TN>
                inline ERR CJournalWrapper<TM, TN>::ErrVisitEntries(    _In_ const ::IJournal::PfnVisitEntry    pfnVisitEntry,
                                                                        _In_ const DWORD_PTR                    keyVisitEntry )
                {
                    ERR         err         = JET_errSuccess;
                    VisitEntry^ visitentry  = gcnew VisitEntry( pfnVisitEntry, keyVisitEntry );

                    ExCall( I()->VisitEntries( gcnew Internal::Ese::BlockCache::Interop::IJournal::VisitEntry( visitentry, &VisitEntry::VisitEntry_ ) ) );

                HandleError:
                    return err;
                }

                template<class TM, class TN>
                inline ERR CJournalWrapper<TM, TN>::ErrRepair( _In_ const ::JournalPosition jposLast )
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->Repair( (JournalPosition)jposLast ) );

                HandleError:
                    return err;
                }

                template<class TM, class TN>
                inline ERR CJournalWrapper<TM, TN>::ErrAppendEntry( _In_                const size_t                cjb,
                                                                    _In_reads_( cjb )   CJournalBuffer* const       rgjb,
                                                                    _Out_               ::JournalPosition* const    pjpos )
                {
                    ERR                         err             = JET_errSuccess;
                    array<ArraySegment<byte>>^  payload         = rgjb ? gcnew array<ArraySegment<byte>>( cjb ) : nullptr;
                    JournalPosition             journalPosition = JournalPosition::Invalid;

                    *pjpos = ::jposInvalid;

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

                    ExCall( journalPosition = I()->AppendEntry( payload ) );

                    *pjpos = (::JournalPosition)journalPosition;

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        *pjpos = ::jposInvalid;
                    }
                    return err;
                }

                template<class TM, class TN>
                inline ERR CJournalWrapper<TM, TN>::ErrFlush()
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->Flush() );

                HandleError:
                    return err;
                }

                template<class TM, class TN>
                inline ERR CJournalWrapper<TM, TN>::ErrTruncate( _In_ const ::JournalPosition jposReplay )
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->Truncate( (JournalPosition)jposReplay ) );

                HandleError:
                    return err;
                }
            }
        }
    }
}
