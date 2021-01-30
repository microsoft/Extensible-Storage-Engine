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
                public ref class CacheTelemetryBase : public Base<TM, TN, TW>, ICacheTelemetry
                {
                    public:

                        CacheTelemetryBase( TM^ ctm ) : Base( ctm ) { }

                        CacheTelemetryBase( TN** const ppctm ) : Base( ppctm ) {}

                    public:

                        virtual void Miss(
                            ICacheTelemetry::FileNumber filenumber,
                            ICacheTelemetry::BlockNumber blocknumber,
                            bool isRead,
                            bool cacheIfPossible );

                        virtual void Hit(
                            ICacheTelemetry::FileNumber filenumber,
                            ICacheTelemetry::BlockNumber blocknumber,
                            bool isRead,
                            bool cacheIfPossible );

                        virtual void Update(
                            ICacheTelemetry::FileNumber filenumber,
                            ICacheTelemetry::BlockNumber blocknumber );

                        virtual void Write(
                            ICacheTelemetry::FileNumber filenumber,
                            ICacheTelemetry::BlockNumber blocknumber,
                            bool replacementPolicy );

                        virtual void Evict(
                            ICacheTelemetry::FileNumber filenumber, 
                            ICacheTelemetry::BlockNumber blocknumber,
                            bool replacementPolicy );
                };

                template< class TM, class TN, class TW >
                inline void CacheTelemetryBase<TM, TN, TW>::Miss(
                    ICacheTelemetry::FileNumber filenumber, 
                    ICacheTelemetry::BlockNumber blocknumber,
                    bool isRead,
                    bool cacheIfPossible )
                {
                    Pi->Miss(   (::ICacheTelemetry::FileNumber)filenumber,
                                (::ICacheTelemetry::BlockNumber)blocknumber,
                                isRead ? fTrue : fFalse,
                                cacheIfPossible ? fTrue : fFalse );
                }

                template< class TM, class TN, class TW >
                inline void CacheTelemetryBase<TM, TN, TW>::Hit(
                    ICacheTelemetry::FileNumber filenumber, 
                    ICacheTelemetry::BlockNumber blocknumber,
                    bool isRead,
                    bool cacheIfPossible )
                {
                    Pi->Hit(    (::ICacheTelemetry::FileNumber)filenumber,
                                (::ICacheTelemetry::BlockNumber)blocknumber,
                                isRead ? fTrue : fFalse,
                                cacheIfPossible ? fTrue : fFalse );
                }

                template< class TM, class TN, class TW >
                inline void CacheTelemetryBase<TM, TN, TW>::Update(
                    ICacheTelemetry::FileNumber filenumber, 
                    ICacheTelemetry::BlockNumber blocknumber )
                {
                    Pi->Update( (::ICacheTelemetry::FileNumber)filenumber,
                                (::ICacheTelemetry::BlockNumber)blocknumber );
                }

                template< class TM, class TN, class TW >
                inline void CacheTelemetryBase<TM, TN, TW>::Write(
                    ICacheTelemetry::FileNumber filenumber, 
                    ICacheTelemetry::BlockNumber blocknumber,
                    bool replacementPolicy )
                {
                    Pi->Write(  (::ICacheTelemetry::FileNumber)filenumber,
                                (::ICacheTelemetry::BlockNumber)blocknumber,
                                replacementPolicy ? fTrue : fFalse );
                }

                template< class TM, class TN, class TW >
                inline void CacheTelemetryBase<TM, TN, TW>::Evict(
                    ICacheTelemetry::FileNumber filenumber, 
                    ICacheTelemetry::BlockNumber blocknumber,
                    bool replacementPolicy )
                {
                    Pi->Evict(  (::ICacheTelemetry::FileNumber)filenumber,
                                (::ICacheTelemetry::BlockNumber)blocknumber,
                                replacementPolicy ? fTrue : fFalse );
                }
            }
        }
    }
}
