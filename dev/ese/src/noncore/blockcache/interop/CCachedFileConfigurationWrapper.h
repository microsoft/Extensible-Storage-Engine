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
                class CCachedFileConfigurationWrapper : public CWrapper<TM, TN>
                {
                    public:

                        CCachedFileConfigurationWrapper( TM^ cfconfig ) : CWrapper( cfconfig ) { }

                    public:

                        BOOL FCachingEnabled() override;
                        VOID CachingFilePath( __out_bcount( cbOSFSAPI_MAX_PATHW ) WCHAR* const wszAbsPath ) override;
                        ULONG CbBlockSize() override;
                        ULONG CConcurrentBlockWriteBackMax() override;
                        ULONG LCacheTelemetryFileNumber() override;
                };

                template< class TM, class TN >
                inline BOOL CCachedFileConfigurationWrapper<TM, TN>::FCachingEnabled()
                {
                    return I()->IsCachingEnabled() ? fTrue : fFalse;
                }

                template< class TM, class TN >
                inline VOID CCachedFileConfigurationWrapper<TM, TN>::CachingFilePath( __out_bcount( cbOSFSAPI_MAX_PATHW ) WCHAR* const wszAbsPath )
                {
                    String^ absPath = I()->CachingFilePath();
                    pin_ptr<const Char> wszAbsPathT = PtrToStringChars( absPath );
                    OSStrCbCopyW( wszAbsPath, cbOSFSAPI_MAX_PATHW, (STRSAFE_LPCWSTR)wszAbsPathT );
                }

                template< class TM, class TN >
                inline ULONG CCachedFileConfigurationWrapper<TM, TN>::CbBlockSize()
                {
                    return I()->BlockSize();
                }

                template< class TM, class TN >
                inline ULONG CCachedFileConfigurationWrapper<TM, TN>::CConcurrentBlockWriteBackMax()
                {
                    return I()->MaxConcurrentBlockWriteBacks();
                }

                template< class TM, class TN >
                inline ULONG CCachedFileConfigurationWrapper<TM, TN>::LCacheTelemetryFileNumber()
                {
                    return I()->CacheTelemetryFileNumber();
                }
            }
        }
    }
}
