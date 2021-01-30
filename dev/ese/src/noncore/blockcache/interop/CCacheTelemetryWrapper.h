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
                class CCacheTelemetryWrapper : public CWrapper<TM, TN>
                {
                    public:

                        CCacheTelemetryWrapper( TM^ ctm ) : CWrapper( ctm ) { }

                    public:

                        void Miss(  _In_ const ::ICacheTelemetry::FileNumber    filenumber, 
                                    _In_ const ::ICacheTelemetry::BlockNumber   blocknumber,
                                    _In_ const BOOL                             fRead,
                                    _In_ const BOOL                             fCacheIfPossible ) override;

                        void Hit(   _In_ const ::ICacheTelemetry::FileNumber    filenumber,
                                    _In_ const ::ICacheTelemetry::BlockNumber   blocknumber,
                                    _In_ const BOOL                             fRead,
                                    _In_ const BOOL                             fCacheIfPossible ) override;

                        void Update(    _In_ const ::ICacheTelemetry::FileNumber    filenumber, 
                                        _In_ const ::ICacheTelemetry::BlockNumber   blocknumber ) override;

                        void Write( _In_ const ::ICacheTelemetry::FileNumber    filenumber, 
                                    _In_ const ::ICacheTelemetry::BlockNumber   blocknumber,
                                    _In_ const BOOL                             fReplacementPolicy ) override;

                        void Evict( _In_ const ::ICacheTelemetry::FileNumber    filenumber, 
                                    _In_ const ::ICacheTelemetry::BlockNumber   blocknumber,
                                    _In_ const BOOL                             fReplacementPolicy ) override;
                };

                template< class TM, class TN >
                inline void CCacheTelemetryWrapper<TM, TN>::Miss(   _In_ const ::ICacheTelemetry::FileNumber    filenumber,
                                                                    _In_ const ::ICacheTelemetry::BlockNumber   blocknumber,
                                                                    _In_ const BOOL                             fRead,
                                                                    _In_ const BOOL                             fCacheIfPossible )
                {
                    I()->Miss(  (Interop::ICacheTelemetry::FileNumber)filenumber,
                                (Interop::ICacheTelemetry::BlockNumber)blocknumber,
                                fRead ? true : false, 
                                fCacheIfPossible ? true : false );
                }

                template< class TM, class TN >
                inline void CCacheTelemetryWrapper<TM, TN>::Hit(    _In_ const ::ICacheTelemetry::FileNumber    filenumber,
                                                                    _In_ const ::ICacheTelemetry::BlockNumber   blocknumber,
                                                                    _In_ const BOOL                             fRead,
                                                                    _In_ const BOOL                             fCacheIfPossible )
                {
                    I()->Hit(   (Interop::ICacheTelemetry::FileNumber)filenumber,
                                (Interop::ICacheTelemetry::BlockNumber)blocknumber, 
                                fRead ? true : false, 
                                fCacheIfPossible ? true : false );
                }

                template< class TM, class TN >
                inline void CCacheTelemetryWrapper<TM, TN>::Update( _In_ const ::ICacheTelemetry::FileNumber    filenumber,
                                                                    _In_ const ::ICacheTelemetry::BlockNumber   blocknumber )
                {
                    I()->Update( (Interop::ICacheTelemetry::FileNumber)filenumber,
                                (Interop::ICacheTelemetry::BlockNumber)blocknumber );
                }

                template< class TM, class TN >
                inline void CCacheTelemetryWrapper<TM, TN>::Write(  _In_ const ::ICacheTelemetry::FileNumber    filenumber,
                                                                    _In_ const ::ICacheTelemetry::BlockNumber   blocknumber,
                                                                    _In_ const BOOL                             fReplacementPolicy )
                {
                    I()->Write( (Interop::ICacheTelemetry::FileNumber)filenumber,
                                (Interop::ICacheTelemetry::BlockNumber)blocknumber,
                                fReplacementPolicy ? true : false );
                }

                template< class TM, class TN >
                inline void CCacheTelemetryWrapper<TM, TN>::Evict(  _In_ const ::ICacheTelemetry::FileNumber    filenumber,
                                                                _In_ const ::ICacheTelemetry::BlockNumber   blocknumber,
                                                                _In_ const BOOL                             fReplacementPolicy )
                {
                    I()->Evict( (Interop::ICacheTelemetry::FileNumber)filenumber,
                                (Interop::ICacheTelemetry::BlockNumber)blocknumber,
                                fReplacementPolicy ? true : false );
                }
            }
        }
    }
}
