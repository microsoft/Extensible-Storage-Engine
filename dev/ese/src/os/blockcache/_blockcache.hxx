// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "tcconst.hxx"
#include "checksum.hxx"

#ifdef ESENT
#include "jetmsg.h"
#else
#include "jetmsgex.h"
#endif


#include "blockcache\_common.hxx"
#include "blockcache\_headerhelpers.hxx"
#include "blockcache\_cachetelemetry.hxx"
#include "blockcache\_cachedfiletableentrybase.hxx"
#include "blockcache\_cachedfilehash.hxx"
#include "blockcache\_cacheheader.hxx"
#include "blockcache\_cachethreadlocalstoragebase.hxx"
#include "blockcache\_cachethreadlocalstoragehash.hxx"
#include "blockcache\_cachethreadlocalstorage.hxx"
#include "blockcache\_cachebase.hxx"
#include "blockcache\_passthroughcachedfiletableentry.hxx"
#include "blockcache\_passthroughcache.hxx"
#include "blockcache\_journalregion.hxx"
#include "blockcache\_journalsegmentheader.hxx"
#include "blockcache\_journalsegment.hxx"
#include "blockcache\_journalsegmentmanager.hxx"
#include "blockcache\_journalentryfragment.hxx"
#include "blockcache\_journalentry.hxx"
#include "blockcache\_journal.hxx"
#include "blockcache\_hashedlrukcacheheader.hxx"
#include "blockcache\_hashedlrukcachedfiletableentry.hxx"
#include "blockcache\_hashedlrukcachestate.hxx"
#include "blockcache\_hashedlrukcache.hxx"
#include "blockcache\_cachefactory.hxx"
#include "blockcache\_cachewrapper.hxx"
#include "blockcache\_cacherepository.hxx"
#include "blockcache\_fileidentification.hxx"
#include "blockcache\_filewrapper.hxx"
#include "blockcache\_cachedfileheader.hxx"
#include "blockcache\_filefilter.hxx"
#include "blockcache\_fswrapper.hxx"
#include "blockcache\_fsfilter.hxx"
