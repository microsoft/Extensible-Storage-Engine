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

                        BOOL FCacheEnabled() override
                        {
                            return I()->IsCacheEnabled() ? fTrue : fFalse;
                        }

                        VOID CacheType( _Out_writes_( cbGuid ) BYTE* const rgbCacheType ) override
                        {
                            Guid cacheType = I()->CacheType();

                            array<Byte>^ cacheTypeBytes = cacheType.ToByteArray();
                            pin_ptr<Byte> cacheTypeBytesT = &cacheTypeBytes[ 0 ];
                            UtilMemCpy( rgbCacheType, cacheTypeBytesT, cbGuid );
                        }

                        VOID Path( __out_bcount( cbOSFSAPI_MAX_PATHW ) WCHAR* const wszAbsPath ) override
                        {
                            String^ absPath = I()->Path();
                            pin_ptr<const Char> wszAbsPathT = PtrToStringChars( absPath );
                            OSStrCbCopyW( wszAbsPath, cbOSFSAPI_MAX_PATHW, (STRSAFE_LPCWSTR)wszAbsPathT );
                        }

                        QWORD CbMaximumSize() override
                        {
                            return I()->MaximumSize();
                        }

                        double PctWrite() override
                        {
                            return I()->PercentWrite();
                        }

                        QWORD CbJournalSegmentsMaximumSize() override
                        {
                            return I()->JournalSegmentsMaximumSize();
                        }

                        double PctJournalSegmentsInUse() override
                        {
                            return I()->PercentJournalSegmentsInUse();
                        }

                        QWORD CbJournalSegmentsMaximumCacheSize() override
                        {
                            return I()->JournalSegmentsMaximumCacheSize();
                        }

                        QWORD CbJournalClustersMaximumSize() override
                        {
                            return I()->JournalClustersMaximumSize();
                        }

                        QWORD CbCachingFilePerSlab() override
                        {
                            return I()->CachingFilePerSlab();
                        }

                        QWORD CbCachedFilePerSlab() override
                        {
                            return I()->CachedFilePerSlab();
                        }

                        QWORD CbSlabMaximumCacheSize() override
                        {
                            return I()->SlabMaximumCacheSize();
                        }

                        BOOL FAsyncWriteBackEnabled() override
                        {
                            return I()->IsAsyncWriteBackEnabled() ? fTrue : fFalse;
                        }

                        ULONG CIOMaxOutstandingDestage()
                        {
                            return I()->MaxConcurrentIODestage();
                        }

                        ULONG CbIOMaxOutstandingDestage()
                        {
                            return I()->MaxConcurrentIOSizeDestage();
                        }
                };
            }
        }
    }
}
