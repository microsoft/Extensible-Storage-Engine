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
                public ref class CacheConfigurationBase : public Base<TM, TN, TW>, ICacheConfiguration
                {
                    public:

                        CacheConfigurationBase( TM^ cconfig ) : Base( cconfig ) { }

                        CacheConfigurationBase( TN** const ppcconfig ) : Base( ppcconfig ) {}

                        CacheConfigurationBase( TN* const pcconfig ) : Base( pcconfig ) {}

                    public:

                        virtual bool IsCacheEnabled();

                        virtual Guid CacheType();

                        virtual String^ Path();

                        virtual Int64 MaximumSize();

                        virtual double PercentWrite();
                };

                template< class TM, class TN, class TW >
                inline bool CacheConfigurationBase<TM, TN, TW>::IsCacheEnabled()
                {
                    return Pi->FCacheEnabled() ? true : false;
                }

                template< class TM, class TN, class TW >
                inline Guid CacheConfigurationBase<TM, TN, TW>::CacheType()
                {
                    GUID guidNative = { 0 };

                    Pi->CacheType( (BYTE*)&guidNative );

                    return Guid(    guidNative.Data1,
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
                }

                template< class TM, class TN, class TW >
                inline String^ CacheConfigurationBase<TM, TN, TW>::Path()
                {
                    WCHAR   wszPath[ OSFSAPI_MAX_PATH ] = { 0 };

                    Pi->Path( wszPath );

                    return gcnew String( wszPath );
                }

                template< class TM, class TN, class TW >
                inline Int64 CacheConfigurationBase<TM, TN, TW>::MaximumSize()
                {
                    return Pi->CbMaximumSize();
                }

                template< class TM, class TN, class TW >
                inline double CacheConfigurationBase<TM, TN, TW>::PercentWrite()
                {
                    return Pi->PctWrite();
                }
            }
        }
    }
}
