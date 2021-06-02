// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  SOMEONE says, you can't boil the ocean.  Let's find out.

//  This file is the start of an effort to remove global and inst specific page sizes.  All page size 
//  references should be at the natural scope of a FMP (i.e. a DB) when done.

//  Further we will be using this .hxx file for forcing file by file to move to non-global PageSize
//  routines, by undefining / redefining the verbotten global page size functions.
//
//	NO BACK SLIDING!
//
//  If you are here, it is because the file is marked clean, and you must find a way to get to any of:
//     - FMP / ->CbPage()
//         - Note also:  pfucb->ifmp, pfcb->Ifmp() can help.
//     - CPAGE / ->CbPage()
//     - BF / .icbPage, maps to array of page sizes.
//  instead of using the functions that are undefined at the end of this file.
//     
//  Once g_cbPage and ancillary functions have been removed, this file and all #include references will
//  be removed as well.

/*  Project notes

  Current references:
     g_cbPage      419       (468 - originally ... well between 468 - 535, excluding lines with g_cbPageMin,Max, or Default 481)
     param         57        (78)
     ancillary     148       (157)
     total         584 / 64  (672 hits / 86 files - originally)


  Staged work:
     x Week  1 - marks clean: bt.cxx, comp.cxx, dbshrink.cxx, ver.cxx, and some misc no-hit files.
                   x Splits and fixes most CbRecordMost() uses.
     x Week  2 - marks clean: logdiff.cxx, rec.cxx, node.cxx, logwrite.cxx, dbtask.cxx, info.cxx, prl.cxx, upgrade.cxx.
                   x Convert CpgOfCb, CbOfCpg to be FMP-based; pass cbPage to CpgOfUnalignedCb and fallout to SpaceHint functions.
                   x Also removed eseex_x.h from consideration, because it is generated.
                   x Added FSmallPageDb as replacement for FIsSmallPage()
                   x Add CbSYSMaxPageSize() and CbINSTMaxPageSize() for the worst case page globally and per INST.
     o Week  3 - cleaned up much of edbg.cxx hits.
                   x Involved indirecting most page size references
     o Week  4 - TBD lv.cxx and backup.cxx looks easy ... 
                   o Rationalize CbFileSizeOfPgnoLast, CbFileSizeOfCpg
                   o Deal with ErrPKInitCompression (getting g_cbPage)
                       o Remove g_compressionBufferCache, and make it use BFAlloc()
                       o Move g_dataCompressor to INST.  Make JET_paramMinDataForXpress inst-level.
                       o Remove ErrPKInitCompression in tm.cxx, move to InstInit.
                       o And probably easy to fix g_cbPage hits in compression.cxx at this time.
                       o Consider embeddedunittest.cxx
                            OSTestCall( JetSetSystemParameterW( pinst, JET_sesidNil, JET_paramDatabasePageSize, 4 * 1024, NULL ) );
                   o 
     o Week  5 - TBD
                   o Figure out unittesting ... maybe make it run cbPage's in parallel?
                       o See incidents of cbSpaceHintsTestPageSize in cat.cxx
                       o See cpage_test.cxx has lots of page size stuff.
     o Week  6 - TBD
                   o Convert and rationalize PgnoOfOffset and OffsetOfPgno to be FMP-based
     o Week  7 - TBD
     o Week  8 - TBD
     o Week  9 - TBD
                   o Remember to defend against someone setting:
                       o the INST-wide PageSize "Max" setting > global-max (global parma), 
                       o and setting the DB PageSize setting (for defrag) > INST-wide param
     o Week 10 - TBD
     o ...
     o Week 21 - TBD
                   o Clean up hits: findstr /snp FireWall *.c* *.h* | findstr "RecTooBig".
                   o Clean up hits: CbAssertGlobalPageSizeMatchRTL ... should be removed, and arg passed in should be used as page size.
     o Week 24 - TBD
                   o Ensure eseutil & edbg \ DBUTLDumpRec page size is plumbed all the way down, so we can remove EDBGDeprecatedSetGlobalPageSize.
                       o Work out how to fix deep issues like REC::CbRecordMostWithGlobalPageSize(), which is marked.
                   o More edbg.cxx work:
                       o Remove EDBGDeprecatedSetGlobalPageSize().
                       o Review and/or possibly remove hits of CbEDBGILoadSYSMaxPageSize_() and CbPageGlobalMax_().  Some hits may be ok.
                       o Review and/or possibly remove hits of CbPageOfInst_().  Some hits may be ok.
     o Week 49 - Test two instances with different pages sizes work.
                   o Test Global.JET_paramDatabasePageSize = 32, Inst1.JET_paramDatabasePageSize = 8, Inst1.JET_paramDatabasePageSize = 16,
     o Week 50 - Test inject (full test pass) that temp DB runs at 2 x page size of primary inst DB.
     o Week 51 - Test defrag can move from one DB page size to another DB page size (a larger one).
     o Week 52 - This file and all #include references should be removed when the project is complete.


  We will keep a list of hard work cases here:
     o REC::CbRecordMostWithGlobalPageSize() see definition, has four nasty hits.
     o TAGFIELDS::FIsValidTagfields() used twice in repair.cxx with no context that can translate to cbPage.


  The artic ocean only has 5000 Billion Gallons.  We only have to boil 96 Billion Gallons per week
  to boil this ocean.  SI units say one g_cbPage reference = 11 Billion Gallons of salt water, and
  so we only have to remove ~9 references per week to get this done by end of 2020.  IF we can melt
  the Greenland glacier, surely we can boil the Artic!

*/

//	#defines and functions we are preventing from usage due to inherent g_cbPage dependencies
//

#undef g_cbPage
#define g_cbPage g_cbPage_THIS_FILE_EVOLVED_PAST_GLOBAL_PAGE_SIZE_SEE_see_PageSizeCleanHxx

#undef JET_paramDatabasePageSize
#define JET_paramDatabasePageSize JET_paramDatabasePageSize_THIS_FILE_EVOLVED_PAST_GLOBAL_PAGE_SIZE_SEE_see_PageSizeCleanHxx

// Ancillary that have implicit refs to g_cbPage or JET_paramDatabasePageSize in them.
#define FIsSmallPage FIsSmallPage_THIS_FILE_EVOLVED_PAST_GLOBAL_PAGE_SIZE_FUNCS_see_PageSizeCleanHxx
#define CbRecordMostWithGlobalPageSize CbRecordMostWithGlobalPageSize_THIS_FILE_EVOLVED_PAST_GLOBAL_PAGE_SIZE_FUNCS_see_PageSizeCleanHxx

/* 

SOMEONE and I decided that it would be easier, and we will knock these out one at a time - rather than #define them away.

  x CbOfCpg
  x CpgOfCb
  x CpgOfUnalignedCb
  o CbKeyMostForPage - There is a good one that takes cbPage, and a bad one with g_cbPage / no argument.
  o PgnoOfOffset
  o CbFileSizeOfCpg
  o CbFileSizeOfPgnoLast

*/
