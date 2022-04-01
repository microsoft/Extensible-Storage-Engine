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
                public ref class JournalBase : Base<TM, TN, TW>, IJournal
                {
                    public:

                        JournalBase( TM^ j ) : Base( j ) { }

                        JournalBase( TN** const ppj ) : Base( ppj ) {}

                    public:

                        virtual void GetProperties(
                            [Out] JournalPosition% journalPositionReplay,
                            [Out] JournalPosition% journalPositionDurableForWriteBack,
                            [Out] JournalPosition% journalPositionDurable,
                            [Out] JournalPosition% journalPositionAppend,
                            [Out] JournalPosition% journalPositionFull );

                        virtual void VisitEntries( IJournal::VisitEntry^ visitEntry );

                        virtual JournalPosition Repair( JournalPosition journalPositionInvalidate );

                        virtual JournalPosition AppendEntry(
                            array<ArraySegment<BYTE>>^ payload,
                            [Out] JournalPosition% journalPositionEnd );

                        virtual void Flush();

                        virtual void Truncate( JournalPosition journalPositionReplay );
                };

                template< class TM, class TN, class TW >
                inline void JournalBase<TM, TN, TW>::GetProperties(
                    [Out] JournalPosition% journalPositionReplay,
                    [Out] JournalPosition% journalPositionDurableForWriteBack,
                    [Out] JournalPosition% journalPositionDurable,
                    [Out] JournalPosition% journalPositionAppend,
                    [Out] JournalPosition% journalPositionFull )
                {
                    ERR                 err                     = JET_errSuccess;
                    ::JournalPosition   jposReplay              = ::jposInvalid;
                    ::JournalPosition   jposDurableForWriteBack = ::jposInvalid;
                    ::JournalPosition   jposDurable             = ::jposInvalid;
                    ::JournalPosition   jposAppend              = ::jposInvalid;
                    ::JournalPosition   jposFull                = ::jposInvalid;

                    journalPositionReplay = JournalPosition::Invalid;
                    journalPositionDurableForWriteBack = JournalPosition::Invalid;
                    journalPositionDurable = JournalPosition::Invalid;
                    journalPositionAppend = JournalPosition::Invalid;
                    journalPositionFull = JournalPosition::Invalid;

                    Call( Pi->ErrGetProperties( &jposReplay, 
                                                &jposDurableForWriteBack,
                                                &jposDurable, 
                                                &jposAppend, 
                                                &jposFull ) );

                    journalPositionReplay = (JournalPosition)jposReplay;
                    journalPositionDurableForWriteBack = (JournalPosition)jposDurableForWriteBack;
                    journalPositionDurable = (JournalPosition)jposDurable;
                    journalPositionAppend = (JournalPosition)jposAppend;
                    journalPositionFull = (JournalPosition)jposFull;

                    return;

                HandleError:
                    journalPositionReplay = JournalPosition::Invalid;
                    journalPositionDurableForWriteBack = JournalPosition::Invalid;
                    journalPositionDurable = JournalPosition::Invalid;
                    journalPositionAppend = JournalPosition::Invalid;
                    journalPositionFull = JournalPosition::Invalid;
                    throw EseException( err );
                }

                template<class TM, class TN, class TW>
                inline void JournalBase<TM, TN, TW>::VisitEntries( IJournal::VisitEntry^ visitEntry )
                {
                    ERR                 err                 = JET_errSuccess;
                    VisitEntryInverse^  visitEntryInverse   = nullptr;

                    if ( visitEntry != nullptr )
                    {
                        visitEntryInverse = gcnew VisitEntryInverse( visitEntry );
                    }

                    Call( Pi->ErrVisitEntries(  visitEntryInverse == nullptr ? NULL : visitEntryInverse->PfnVisitEntry,
                                                visitEntryInverse == nullptr ? NULL : visitEntryInverse->KeyVisitEntry ) );

                HandleError:
                    delete visitEntryInverse;
                    if ( err < JET_errSuccess )
                    {
                        throw EseException( err );
                    }
                }

                template<class TM, class TN, class TW>
                inline JournalPosition JournalBase<TM, TN, TW>::Repair( JournalPosition journalPositionInvalidate )
                {
                    ERR                 err             = JET_errSuccess;
                    ::JournalPosition   jposInvalidated = ::jposInvalid;

                    Call( Pi->ErrRepair( (::JournalPosition)journalPositionInvalidate, &jposInvalidated ) );

                    return (JournalPosition)jposInvalidated;

                HandleError:
                    throw EseException( err );
                }

                template<class TM, class TN, class TW>
                inline JournalPosition JournalBase<TM, TN, TW>::AppendEntry( 
                    array<ArraySegment<BYTE>>^ payload,
                    [Out] JournalPosition% journalPositionEnd )
                {
                    ERR                 err     = JET_errSuccess;
                    const size_t        cjb     = payload == nullptr ? 0 : payload->Length;
                    CJournalBuffer*     rgjb    = NULL;
                    ::JournalPosition   jpos    = ::jposInvalid;
                    ::JournalPosition   jposEnd = ::jposInvalid;

                    journalPositionEnd = JournalPosition::Invalid;

                    if ( payload != nullptr )
                    {
                        Alloc( rgjb = new CJournalBuffer[ cjb + 1 ] );
                    }

                    for ( int ijb = 0; ijb < cjb; ijb++ )
                    {
                        const size_t    cb  = payload[ ijb ].Count;
                        BYTE*           rgb = NULL;

                        if ( payload[ ijb ].Array != nullptr )
                        {
                            Alloc( rgb = new BYTE[ cb + 1 ] );
                        }

                        rgjb[ ijb ] = CJournalBuffer( cb, rgb );

                        if ( cb )
                        {
                            pin_ptr<BYTE> rgbIn = &payload[ ijb ].Array[ payload[ ijb ].Offset ];
                            UtilMemCpy( rgb, (BYTE*)rgbIn, cb );
                        }
                    }

                    Call( Pi->ErrAppendEntry( cjb, rgjb, &jpos, &jposEnd ) );

                    journalPositionEnd = (JournalPosition)jposEnd;
                    
                HandleError:
                    if ( rgjb )
                    {
                        for ( int ijb = 0; ijb < cjb; ijb++ )
                        {
                            delete[] rgjb[ ijb ].Rgb();
                        }
                    }
                    delete[] rgjb;
                    if ( err < JET_errSuccess )
                    {
                        journalPositionEnd = JournalPosition::Invalid;
                        throw EseException( err );
                    }
                    return (JournalPosition)jpos;
                }

                template<class TM, class TN, class TW>
                inline void JournalBase<TM, TN, TW>::Flush()
                {
                    ERR err = JET_errSuccess;

                    Call( Pi->ErrFlush() );

                    return;

                HandleError:
                    throw EseException( err );
                }

                template<class TM, class TN, class TW>
                inline void JournalBase<TM, TN, TW>::Truncate( JournalPosition journalPositionReplay )
                {
                    ERR err = JET_errSuccess;

                    Call( Pi->ErrTruncate( (::JournalPosition)journalPositionReplay ) );

                    return;

                HandleError:
                    throw EseException( err );
                }
            }
        }
    }
}
