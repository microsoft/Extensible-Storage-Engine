// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <msclr\auto_gcroot.h>
#include <vcclr.h>

#define _CRT_RAND_S

// needed for JET errors
#if defined(BUILD_ENV_IS_NT) || defined(BUILD_ENV_IS_WPHONE)
#include <esent_x.h>
#endif
#ifdef BUILD_ENV_IS_EX
#include <jet.h>
#endif

#include <windows.h>
#include <cstdio>
#include <stdlib.h>

#include <functional>


#include <tchar.h>
#include "os.hxx"

#include "tcconst.hxx"


using namespace System;
using namespace System::Runtime::InteropServices;

#using <Microsoft.Isam.Esent.Interop.dll> as_friend
using namespace Microsoft::Isam::Esent::Interop;

//  one last macro for old time's sake
#define ExCall( f )                             \
{                                               \
    try                                         \
    {                                           \
        (f);                                    \
        err = JET_errSuccess;                   \
    }                                           \
    catch( EsentErrorException^ ex )            \
    {                                           \
        err = (ERR)(int)ex->Error;              \
    }                                           \
    catch( Exception^ )                         \
    {                                           \
        Enforce( !"Unhandled C# exception" );   \
    }                                           \
    Call( err );                                \
}

INLINE EsentErrorException^ EseException( const JET_ERR err )
{
    return  err >= JET_errSuccess ? nullptr : EsentExceptionHelper::JetErrToException( (JET_err)err );
}

#include "CWrapper.h"
#include "Base.h"

#include "CachingPolicy.h"
#include "FileId.h"
#include "FileModeFlags.h"
#include "FileQOS.h"
#include "FileSerial.h"
#include "FileSize.h"
#include "IOMode.h"
#include "VolumeId.h"

#include "ICachedFileConfiguration.h"
#include "ICacheConfiguration.h"
#include "IBlockCacheConfiguration.h"

#include "IFileIdentification.h"
#include "IFile.h"
#include "IFileFind.h"
#include "IFileSystemConfiguration.h"
#include "IFileSystem.h"
#include "IFileSystemFilter.h"
#include "IFileFilter.h"

#include "ICache.h"
#include "ICacheRepository.h"

#include "CIOComplete.h"
#include "IOComplete.h"

#include "CFileIdentificationWrapper.h"
#include "FileIdentificationBase.h"
#include "FileIdentification.h"

#include "CFileWrapper.h"
#include "FileBase.h"
#include "File.h"

#include "CFileFilterWrapper.h"
#include "FileFilterBase.h"
#include "FileFilter.h"

#include "CFileFindWrapper.h"
#include "FileFindBase.h"
#include "FileFind.h"

#include "CCachedFileConfigurationWrapper.h"
#include "CachedFileConfigurationBase.h"
#include "CachedFileConfiguration.h"

#include "CCacheConfigurationWrapper.h"
#include "CacheConfigurationBase.h"
#include "CacheConfiguration.h"

#include "CBlockCacheConfigurationWrapper.h"
#include "BlockCacheConfigurationBase.h"
#include "BlockCacheConfiguration.h"

#include "CFileSystemConfigurationWrapper.h"
#include "FileSystemConfigurationBase.h"
#include "FileSystemConfiguration.h"

#include "CFileSystemWrapper.h"
#include "FileSystemBase.h"
#include "FileSystem.h"

#include "CFileSystemFilterWrapper.h"
#include "FileSystemFilterBase.h"
#include "FileSystemFilter.h"

#include "CComplete.h"
#include "Complete.h"

#include "CCacheWrapper.h"
#include "CacheBase.h"
#include "Cache.h"

#include "CCacheRepositoryWrapper.h"
#include "CacheRepositoryBase.h"
#include "CacheRepository.h"

#include "ICacheTelemetry.h"
#include "CCacheTelemetryWrapper.h"
#include "CacheTelemetryBase.h"
#include "CacheTelemetry.h"

#include "SegmentPosition.h"
#include "RegionPosition.h"
#include "JournalPosition.h"

#include "IJournalSegment.h"
#include "IJournalSegmentManager.h"
#include "IJournal.h"

#include "CVisitRegion.h"
#include "VisitRegion.h"

#include "CSealed.h"
#include "Sealed.h"

#include "CJournalSegmentWrapper.h"
#include "JournalSegmentBase.h"
#include "JournalSegment.h"

#include "CVisitSegment.h"
#include "VisitSegment.h"

#include "CJournalSegmentManagerWrapper.h"
#include "JournalSegmentManagerBase.h"
#include "JournalSegmentManager.h"

#include "CVisitEntry.h"
#include "VisitEntry.h"

#include "CJournalWrapper.h"
#include "JournalBase.h"
#include "Journal.h"
