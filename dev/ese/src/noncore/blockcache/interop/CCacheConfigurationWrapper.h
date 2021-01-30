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
                class CCacheConfigurationWrapper : public CWrapper<TM, TN>
                {
                    public:

                        CCacheConfigurationWrapper( TM^ cconfig ) : CWrapper( cconfig ) { }

                    public:

                        BOOL FCacheEnabled() override;
                        VOID CacheType( _Out_writes_( cbGuid ) BYTE* const rgbCacheType ) override;
                        VOID Path( __out_bcount( cbOSFSAPI_MAX_PATHW ) WCHAR* const wszAbsPath ) override;
                        QWORD CbMaximumSize() override;
                        double PctWrite() override;
                };

                template< class TM, class TN >
                inline BOOL CCacheConfigurationWrapper<TM, TN>::FCacheEnabled()
                {
                    return I()->IsCacheEnabled() ? fTrue : fFalse;
                }

                template< class TM, class TN >
                inline VOID CCacheConfigurationWrapper<TM, TN>::CacheType( _Out_writes_( cbGuid ) BYTE* const rgbCacheType )
                {
                    Guid cacheType = I()->CacheType();

                    array<Byte>^ cacheTypeBytes = cacheType.ToByteArray();
                    pin_ptr<Byte> cacheTypeBytesT = &cacheTypeBytes[ 0 ];
                    memcpy( rgbCacheType, cacheTypeBytesT, cbGuid );
                }

                template< class TM, class TN >
                inline VOID CCacheConfigurationWrapper<TM, TN>::Path( __out_bcount( cbOSFSAPI_MAX_PATHW ) WCHAR* const wszAbsPath )
                {
                    String^ absPath = I()->Path();
                    pin_ptr<const Char> wszAbsPathT = PtrToStringChars( absPath );
                    OSStrCbCopyW( wszAbsPath, cbOSFSAPI_MAX_PATHW, (STRSAFE_LPCWSTR)wszAbsPathT );
                }

                template< class TM, class TN >
                inline QWORD CCacheConfigurationWrapper<TM, TN>::CbMaximumSize()
                {
                    return I()->MaximumSize();
                }

                template< class TM, class TN >
                inline double CCacheConfigurationWrapper<TM, TN>::PctWrite()
                {
                    return I()->PercentWrite();
                }
            }
        }
    }
}
