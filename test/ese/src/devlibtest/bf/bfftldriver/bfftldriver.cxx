// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#include <tchar.h>
#include "os.hxx"

#ifdef BUILD_ENV_IS_NT
#include "esent_x.h"
#else
#include "jet.h"
#endif

#include "bfreqs.hxx"       // requisite thunked out types
#include "_bfconst.hxx"     // BF constants and simple types we need
#include "bfftl.hxx"        // BF trace data and IDs
#include "bfftldriver.hxx"  // header for this library

IOREASON g_iorThunk( (IOREASONPRIMARY) 1 );

//  Terminates the FTL reader.

void BFFTLITermFTLReader( _In_ BFFTLContext * const pbfftlc )
{
    Assert( !pbfftlc->pftlr || pbfftlc->pftl );

    if ( pbfftlc->pftlr )
    {
        //  note FTLTerm destroys pbfftlc->pftlr as well
        pbfftlc->pftl->FTLTerm();
        pbfftlc->pftlr = NULL;
    }

    if ( pbfftlc->pftl )
    {
        delete pbfftlc->pftl;
        pbfftlc->pftl = NULL;
    }
}

class CFileSystemConfiguration : public CDefaultFileSystemConfiguration
{
    public:
        CFileSystemConfiguration()
        {
            m_dtickAccessDeniedRetryPeriod = 2000;
            m_cbMaxReadSize = 512 * 1024;
            m_cbMaxWriteSize = 384 * 1024;
        }
} g_fsconfigFTL;

//  Initializes the FTL reader.

ERR ErrBFFTLIInitFTLReader( _In_ BFFTLContext * const pbfftlc, _In_ const WCHAR * const wszTraceLogFile )
{
    ERR err = JET_errSuccess;

    Assert( pbfftlc );
    Assert( !pbfftlc->pftl );
    Assert( !pbfftlc->pftlr );

    Alloc( pbfftlc->pftl = new CFastTraceLog( NULL, &g_fsconfigFTL ) );
    Call( pbfftlc->pftl->ErrFTLInitReader( wszTraceLogFile, 
                                           &g_iorThunk, 
                                           ( pbfftlc->grbit & fBFFTLDriverCollectFTLStats ) ? CFastTraceLog::ftlifKeepStats : CFastTraceLog::ftlifNone, 
                                           &( pbfftlc->pftlr ) ) );

HandleError:
    if ( err < JET_errSuccess )
    {
        BFFTLITermFTLReader( pbfftlc );
    }

    return err;
}

//  Initializes the BF trace driver's handle / context

ERR ErrBFFTLInit( _In_ const void * const pvTraceDataLog, _In_ const DWORD grbit, _Out_ BFFTLContext ** ppbfftlc )
{
    ERR err = JET_errSuccess;

    Alloc( *ppbfftlc = new BFFTLContext() );
    BFFTLContext * const pbfftlc = *ppbfftlc;

    memset( pbfftlc, 0, sizeof(BFFTLContext) );

    pbfftlc->grbit = grbit;

    if ( grbit & fBFFTLDriverTestMode )
    {
        pbfftlc->rgFakeTraces = (BFTRACE*)pvTraceDataLog;
    }
    else
    {
        // Deal with real trace log(s)
        //

        // Count how many logs we need to process.
        ULONG cTraceLogFiles = 0;
        const WCHAR * wszTraceLogFiles = (WCHAR*)pvTraceDataLog;
        while ( ( wszTraceLogFiles != NULL ) && ( wszTraceLogFiles[0] != L'\0' ) )
        {
            if ( wszTraceLogFiles[0] == L',' )
            {
                wszTraceLogFiles++;
                continue;
            }

            cTraceLogFiles++;
            wszTraceLogFiles = wcsstr( wszTraceLogFiles, L"," );
        }
        if ( cTraceLogFiles == 0 )
        {
            Error( ErrERRCheck( JET_errInvalidPath ) );
        }
        if ( ( grbit & fBFFTLDriverCollectFTLStats ) && ( cTraceLogFiles > 1 ) )
        {
            // Not supported when working with multiple files.
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }

        // Allocate space for all file paths.
        Alloc( pbfftlc->wszTraceLogFiles = new WCHAR*[ cTraceLogFiles ] );
        memset( pbfftlc->wszTraceLogFiles, 0, cTraceLogFiles * sizeof( WCHAR* ) );
        pbfftlc->cTraceLogFiles = cTraceLogFiles;
        pbfftlc->iTraceLogFile = 0;

        // Collect all paths to our array of paths.
        ULONG iTraceLogFile = 0;
        wszTraceLogFiles = (WCHAR*)pvTraceDataLog;
        while ( ( wszTraceLogFiles != NULL ) && ( wszTraceLogFiles[0] != L'\0' ) )
        {
            if ( wszTraceLogFiles[0] == L',' )
            {
                wszTraceLogFiles++;
                continue;
            }

            const WCHAR * const wszTraceLogFileBegin = wszTraceLogFiles;
            wszTraceLogFiles = wcsstr( wszTraceLogFiles, L"," );
            const WCHAR * const wszTraceLogFileEnd = ( wszTraceLogFiles == NULL ) ? ( wszTraceLogFileBegin + LOSStrLengthW( wszTraceLogFileBegin ) ) : wszTraceLogFiles;

            const ULONG cchTraceLogFile = (ULONG)( wszTraceLogFileEnd - wszTraceLogFileBegin );
            Alloc( pbfftlc->wszTraceLogFiles[iTraceLogFile] = new WCHAR[cchTraceLogFile + 1] );
            memcpy( pbfftlc->wszTraceLogFiles[iTraceLogFile], wszTraceLogFileBegin, cchTraceLogFile * sizeof(WCHAR) );
            ( pbfftlc->wszTraceLogFiles[iTraceLogFile] )[cchTraceLogFile] = L'\0';

            iTraceLogFile++;
        }
        Assert( cTraceLogFiles == iTraceLogFile );

        // Collect max ifmp and pgnoMax from the multiple trace files.
        ULONG cifmpMin = ulMax, cifmpMax = 0;
        for ( iTraceLogFile = 0; iTraceLogFile < pbfftlc->cTraceLogFiles; iTraceLogFile++ )
        {
            Call( ErrBFFTLIInitFTLReader( pbfftlc, pbfftlc->wszTraceLogFiles[iTraceLogFile] ) );

            CFastTraceLog::BFFTLFilePostProcessHeader* const pPostProcessHeader = (CFastTraceLog::BFFTLFilePostProcessHeader *)( pbfftlc->pftl->PvFTLPrivatePostHeader() );

            const ULONG cifmp = pPostProcessHeader->le_cifmp;
            Assert( cifmp <= ifmpMax );
            cifmpMin = min( cifmpMin, cifmp );
            cifmpMax = max( cifmpMax, cifmp );

            BFFTLITermFTLReader( pbfftlc );
        }

        // If any of the traces was not processed (0 ifmps), force the client to process them by
        // returning 0 in the IFMP count. Should we error out instead?
        if ( cifmpMin == 0 )
        {
            pbfftlc->cIFMP = cifmpMin;
        }
        else
        {
            pbfftlc->cIFMP = cifmpMax;
        }

        // Max all pgnoMaxes.
        if ( pbfftlc->cIFMP != 0 )
        {
            for ( iTraceLogFile = 0; iTraceLogFile < pbfftlc->cTraceLogFiles; iTraceLogFile++ )
            {
                Call( ErrBFFTLIInitFTLReader( pbfftlc, pbfftlc->wszTraceLogFiles[iTraceLogFile] ) );

                CFastTraceLog::BFFTLFilePostProcessHeader* const pPostProcessHeader = (CFastTraceLog::BFFTLFilePostProcessHeader *)( pbfftlc->pftl->PvFTLPrivatePostHeader() );

                const ULONG cifmp = pPostProcessHeader->le_cifmp;
                Assert( cifmp != 0 );

                for ( ULONG ifmp = 0; ifmp < cifmp; ifmp++ )
                {
                    pbfftlc->rgpgnoMax[ifmp] = max( (ULONG)pbfftlc->rgpgnoMax[ifmp], pPostProcessHeader->le_mpifmpcpg[ifmp] );
                }

                BFFTLITermFTLReader( pbfftlc );
            }
        }

        // FTL reader (open first file).
        Assert( pbfftlc->cTraceLogFiles > 0 );
        Assert( pbfftlc->iTraceLogFile == 0 );
        Call( ErrBFFTLIInitFTLReader( pbfftlc, pbfftlc->wszTraceLogFiles[pbfftlc->iTraceLogFile] ) );
    }

HandleError:

    return err;
}

//  Shuts down and frees resources maintained by the BFFTLContext.

void BFFTLTerm( _Out_ BFFTLContext * pbfftlc )
{
    if ( !pbfftlc )
    {
        return;
    }

    // FTL reader.
    BFFTLITermFTLReader( pbfftlc );

    // Trace file path strings.
    if ( pbfftlc->wszTraceLogFiles )
    {
        for ( ULONG iTraceLogFile = 0; iTraceLogFile < pbfftlc->cTraceLogFiles; iTraceLogFile++ )
        {
            if ( !pbfftlc->wszTraceLogFiles[iTraceLogFile] )
            {
                continue;
            }

            delete[] pbfftlc->wszTraceLogFiles[iTraceLogFile];
            pbfftlc->wszTraceLogFiles[iTraceLogFile] = NULL;
        }

        delete[] pbfftlc->wszTraceLogFiles;
        pbfftlc->wszTraceLogFiles = NULL;
    }

    delete pbfftlc;
}

/*


BOOL FBFFTLPostProcessed( _In_ const CFastTraceLog * pftl, const ULONG ulPostProcessVersionMajor )
{
    AssertSz( fFalse, "FBFFTLPostProcessed() NYI!" );

    return fFalse;
}

ERR ErrBFFTLPostProcess( __inout CFastTraceLog * pftl )
{
    ERR err = JET_errSuccess;

    AssertSz( fFalse, "ErrBFFTLPostProcess() NYI!" );
    
    return err;
}

PGNO CpgBFFTLHighPgnoRef( _In_ const IFMP ifmp )
{
    AssertSz( fFalse, "CpgBFFTLHighPgnoRef() NYI!" );

    return -1;
}

*/  

ERR ErrBFFTLIUpdIfmpCpgStats( __inout BFFTLContext * pbfftlc, _In_ const IFMP ifmp, _In_ const PGNO pgno )
{
    if ( ifmp >= _countof( pbfftlc->rgpgnoMax ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    if ( ifmp >= pbfftlc->cIFMP )
    {
        pbfftlc->cIFMP = ifmp + 1;
    }
    if ( (INT)pgno > pbfftlc->rgpgnoMax[ifmp] )
    {
        pbfftlc->rgpgnoMax[ifmp] = pgno;
    }
    return JET_errSuccess;
}

ERR ErrBFFTLIAddSample( __inout BFFTLContext * pbfftlc, _Out_ BFTRACE * pbftrace )
{
    ERR err = JET_errSuccess;


    switch( pbftrace->traceid )
    {
        case bftidSysResMgrInit:
            pbfftlc->cSysResMgrInit++;
            break;

        case bftidSysResMgrTerm:
            pbfftlc->cSysResMgrTerm++;
            break;

        case bftidCache:
            CallR( ErrBFFTLIUpdIfmpCpgStats( pbfftlc, pbftrace->bfcache.ifmp, pbftrace->bfcache.pgno ) );
            pbfftlc->cCache++;
            break;
    
        case bftidVerifyInfo:
            pbfftlc->cVerify++;
            break;
    
        case bftidTouchLong:
        case bftidTouch:
            CallR( ErrBFFTLIUpdIfmpCpgStats( pbfftlc, pbftrace->bftouch.ifmp, pbftrace->bftouch.pgno ) );
            pbfftlc->cTouch++;
            break;

        case bftidDirty:
            CallR( ErrBFFTLIUpdIfmpCpgStats( pbfftlc, pbftrace->bfdirty.ifmp, pbftrace->bfdirty.pgno ) );
            pbfftlc->cDirty++;
            break;

        case bftidWrite:
            CallR( ErrBFFTLIUpdIfmpCpgStats( pbfftlc, pbftrace->bfwrite.ifmp, pbftrace->bfwrite.pgno ) );
            pbfftlc->cWrite++;
            break;
    
        case bftidSetLgposModify:
            CallR( ErrBFFTLIUpdIfmpCpgStats( pbfftlc, pbftrace->bfsetlgposmodify.ifmp, pbftrace->bfsetlgposmodify.pgno ) );
            pbfftlc->cSetLgposModify++;
            break;

        case bftidEvict:
            CallR( ErrBFFTLIUpdIfmpCpgStats( pbfftlc, pbftrace->bfevict.ifmp, pbftrace->bfevict.pgno ) );
            pbfftlc->cEvict++;
            break;

        case bftidSuperCold:
            pbfftlc->cSuperCold++;
            break;
    
        default:
            // test code uses bftidFTLReserved as discardable trace ... ugh
            AssertSz( pbftrace->traceid == bftidFTLReserved, "Unknown BF FTL Trace (%d)!\n", (ULONG)pbftrace->traceid );
            wprintf( L"T:%08x { Unknown BF FTL Trace }\n", pbftrace->tick );
    }

    return JET_errSuccess;
}


//  Gets the next trace record in the Buffer Manager FTL trace log file, returning success if we were
//  able to get the trace, errNotFound if we're done with the trace file, and an a specific error in
//  any case.

ERR ErrBFFTLGetNext( __inout BFFTLContext * pbfftlc, _Out_ BFTRACE * pbftrace )
{
    ERR err = JET_errSuccess;

    if ( pbfftlc->grbit & fBFFTLDriverTestMode )
    {
        if ( pbfftlc->rgFakeTraces[pbfftlc->iFake].traceid == bftidInvalid )
        {
            Error( ErrERRCheck( errNotFound ) );
        }

        *pbftrace = pbfftlc->rgFakeTraces[pbfftlc->iFake];

        pbfftlc->iFake++;
    }
    else
    {

        //  real traces: this loop will only normally execute once, unless we're switching files.

        while ( true )
        {
            const ERR errGetNext = pbfftlc->pftlr->ErrFTLGetNextTrace( &(pbfftlc->ftltraceCurrent) );
            if ( errGetNext == errNotFound )
            {
                Assert( pbfftlc->cTraceLogFiles > 0 );
                Assert( pbfftlc->iTraceLogFile < pbfftlc->cTraceLogFiles );

                //  switch to the next trace file, if any
                if ( ( pbfftlc->iTraceLogFile + 1 ) < pbfftlc->cTraceLogFiles )
                {
                    err = JET_errSuccess;

                    BFFTLITermFTLReader( pbfftlc );
                    pbfftlc->iTraceLogFile++;
                    Call( ErrBFFTLIInitFTLReader( pbfftlc, pbfftlc->wszTraceLogFiles[pbfftlc->iTraceLogFile] ) );
                    continue;
                }
                else
                {
                    err = errGetNext;
                }
            }

            Call( errGetNext );

            break;
        }

        pbftrace->traceid = pbfftlc->ftltraceCurrent.ftltid;
        pbftrace->tick = pbfftlc->ftltraceCurrent.tick;
        if ( ( sizeof(*pbftrace) - OffsetOf( BFTRACE_, bfinit ) ) < pbfftlc->ftltraceCurrent.cbTraceData )
        {
            AssertSz( fFalse, "Hmmm, we have a data chunk that doesn't fit in the BFTRACE?  Mismatched version?" );
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
        memcpy( &(pbftrace->bfinit), pbfftlc->ftltraceCurrent.pbTraceData, pbfftlc->ftltraceCurrent.cbTraceData );
    }

    Assert( pbftrace->traceid != 0 );

    //  accumulate statistics

    pbfftlc->cTracesProcessed++;

    (void)ErrBFFTLIAddSample( pbfftlc, pbftrace );

HandleError:

    //  once empty returns errNotFound

    return err;
}

QWORD IbBFFTLBookmark( _Out_ const BFFTLContext * pbfftlc )
{
    Assert( pbfftlc );
    if ( pbfftlc == NULL )
    {
        return 0;
    }
    return pbfftlc->ftltraceCurrent.ibBookmark;
}

ERR ErrBFFTLPostProcess( _In_ const BFFTLContext * pbfftlc )
{
    ERR err = JET_errSuccess;

    CFastTraceLog::BFFTLFilePostProcessHeader * pPostProcessHeader = (CFastTraceLog::BFFTLFilePostProcessHeader *) malloc( pbfftlc->pftl->CbPrivateHeader() );

    C_ASSERT( sizeof(CFastTraceLog::BFFTLFilePostProcessHeader) <= 1024 );
    Assert( sizeof(CFastTraceLog::BFFTLFilePostProcessHeader) <= pbfftlc->pftl->CbPrivateHeader() );

    if ( 0 == ( pbfftlc->grbit & fBFFTLDriverCollectBFStats ) )
    {
        //  if stats weren't collected, impossible to update post proc header
        return ErrERRCheck( JET_errInvalidParameter );
    }

    pPostProcessHeader->le_cifmp = (ULONG)pbfftlc->cIFMP;
    memcpy( pPostProcessHeader->le_mpifmpcpg, pbfftlc->rgpgnoMax, sizeof(CPG)*pbfftlc->cIFMP );

    CallR( pbfftlc->pftl->ErrFTLSetPostProcessHeader( pPostProcessHeader ) );

    return err;
}

#define DblPct( numerator, denominator )        ( ((double)numerator)/((double)denominator)*100.0 )
//  Dumps stats about the trace file processed
    
ERR ErrBFFTLDumpStats( _In_ const BFFTLContext * pbfftlc, _In_ const DWORD grbit )
{
    wprintf( L"Dumping pbfftlc = %p with grbit = 0x%x\n", pbfftlc, grbit );
    wprintf( L"\n" );

    //  Dump the IFMP + cpgMax map
    //
    CFastTraceLog::BFFTLFilePostProcessHeader * pPostProcessHeader = (CFastTraceLog::BFFTLFilePostProcessHeader *)( pbfftlc->pftl->PvFTLPrivatePostHeader() );
    wprintf( L"    Processed file that has %d | %d ifmps\n", pbfftlc->cIFMP, pPostProcessHeader->le_cifmp );
    ULONG ifmp;
    for ( ifmp = 0; ifmp < pbfftlc->cIFMP && ifmp < _countof(pPostProcessHeader->le_mpifmpcpg); ifmp++ )
    {
        wprintf( L"        [ifmp=%d].cpgMax = %d | %d\n", ifmp, pbfftlc->rgpgnoMax[ifmp], pPostProcessHeader->le_mpifmpcpg[ifmp] );
    }
    if ( ifmp == _countof(pPostProcessHeader->le_mpifmpcpg) )
    {
        wprintf( L"        OVERFLOW of pPostProcessHeader->le_mpifmpcpg array!\n" );
    }

    //  Dump FTL-level stats
    //
    if ( grbit & fBFFTLDriverCollectFTLStats )
    {
        CallS( pbfftlc->pftlr->ErrFTLDumpStats() );
        wprintf( L"\n" );
    }

    //  Dump BF-level stats
    //
    if ( grbit & fBFFTLDriverCollectBFStats )
    {
        wprintf( L"BF Stats:\n" );
        wprintf( L"    Traces:  %8I64d\n", pbfftlc->cTracesProcessed );
        wprintf( L"    bftidSysResMgrInit:  %8I64d\n", pbfftlc->cSysResMgrInit );
        wprintf( L"    bftidSysResMgrTerm:  %8I64d\n", pbfftlc->cSysResMgrTerm );
        wprintf( L"    bftidCache:          %8I64d (%6.3f%%)\n", pbfftlc->cCache, DblPct( pbfftlc->cCache, pbfftlc->cTracesProcessed ) );
        wprintf( L"    bftidVerify:         %8I64d (%6.3f%%)\n", pbfftlc->cVerify, DblPct( pbfftlc->cVerify, pbfftlc->cTracesProcessed ) );
        wprintf( L"    bftidTouch[Long]:    %8I64d (%6.3f%%, TC Ratio=%3.1f)\n", pbfftlc->cTouch, 
                        DblPct( pbfftlc->cTouch, pbfftlc->cTracesProcessed ), ( (double)pbfftlc->cTouch / (double)pbfftlc->cCache ) );
        wprintf( L"    bftidWrite:          %8I64d (%6.3f%%)\n", pbfftlc->cWrite, DblPct( pbfftlc->cWrite, pbfftlc->cTracesProcessed ) );
        wprintf( L"    bftidDirty:          %8I64d (%6.3f%%)\n", pbfftlc->cDirty, DblPct( pbfftlc->cDirty, pbfftlc->cTracesProcessed ) );
        wprintf( L"    bftidSetLgposModify: %8I64d (%6.3f%%)\n", pbfftlc->cSetLgposModify, DblPct( pbfftlc->cSetLgposModify, pbfftlc->cTracesProcessed ) );
        wprintf( L"    bftidEvict:          %8I64d (%6.3f%%)\n", pbfftlc->cEvict, DblPct( pbfftlc->cEvict, pbfftlc->cTracesProcessed ) );
        wprintf( L"    bftidSuperCold:      %8I64d (%6.3f%%)\n", pbfftlc->cSuperCold, DblPct( pbfftlc->cSuperCold, pbfftlc->cTracesProcessed ) );
    }
    
    return JET_errSuccess;
}


