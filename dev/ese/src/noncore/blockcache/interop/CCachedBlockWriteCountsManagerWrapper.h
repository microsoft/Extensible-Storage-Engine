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
                class CCachedBlockWriteCountsManagerWrapper : public CWrapper<TM, TN>
                {
                    public:

                        CCachedBlockWriteCountsManagerWrapper( TM^ cbwcm ) : CWrapper( cbwcm ) { }

                    public:

                    ERR ErrGetWriteCount( _In_ const QWORD icbwc, _Out_ ::CachedBlockWriteCount* const pcbwc ) override;

                    ERR ErrSetWriteCount(   _In_ const QWORD                    icbwc,
                                            _In_ const ::CachedBlockWriteCount  cbwc ) override;

                    ERR ErrSave() override;
                };

                template<class TM, class TN>
                inline ERR CCachedBlockWriteCountsManagerWrapper<TM, TN>::ErrGetWriteCount( _In_    const QWORD                     icbwc,
                                                                                            _Out_   ::CachedBlockWriteCount* const  pcbwc )
                {
                    ERR                     err         = JET_errSuccess;
                    CachedBlockWriteCount   writeCount  = CachedBlockWriteCount::Unknown;

                    *pcbwc = ::cbwcUnknown;

                    ExCall( writeCount = I()->GetWriteCount( icbwc ) );

                    *pcbwc = (::CachedBlockWriteCount)writeCount;

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        *pcbwc = ::cbwcUnknown;
                    }
                    return err;
                }

                template<class TM, class TN>
                inline ERR CCachedBlockWriteCountsManagerWrapper<TM, TN>::ErrSetWriteCount( _In_ const QWORD                    icbwc,
                                                                                            _In_ const ::CachedBlockWriteCount  cbwc )
                {
                    ERR                     err         = JET_errSuccess;

                    ExCall( I()->SetWriteCount( icbwc, (CachedBlockWriteCount)cbwc ) );

                HandleError:
                    return err;
                }

                template<class TM, class TN>
                inline ERR CCachedBlockWriteCountsManagerWrapper<TM, TN>::ErrSave()
                {
                    ERR                     err         = JET_errSuccess;

                    ExCall( I()->Save() );

                HandleError:
                    return err;
                }
            }
        }
    }
}