// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "resmgrreplay.hxx"

const WCHAR * WszLatchType( BFLatchType bflt )
{
    return ( ( bflt == bfltShared ) ?
                L"bfltShared" :
                ( ( bflt == bfltExclusive ) ?
                    L"bfltExclusive" :
                    ( ( bflt == bfltWrite ) ?
                        L"bfltWrite" :
                        L"bfltPreReadOrVersion" ) ) );
}


ERR ErrResMgrAccumFtlStats( __in BFFTLContext * const pbfftlc, __in const BOOL fDump )
{
    ERR err = JET_errSuccess;

    pbfftlc->pftl->ErrFTLDumpHeader();
    wprintf( L"\n" );

    wprintf( L"Begin accumulate stats run ...\n" );

    BFTRACE bftrace;

    while ( ( err = ErrBFFTLGetNext( pbfftlc, &bftrace ) ) >= JET_errSuccess )
    {
        if ( fDump )
        {
            QWORD ibBookmark = IbBFFTLBookmark( pbfftlc );
            wprintf( L"%010I64x: Trace: %3d @ %d ", ibBookmark, (ULONG)bftrace.traceid, bftrace.tick );

            switch( (ULONG)bftrace.traceid )
            {
                case bftidSysResMgrInit:
                    wprintf( L"SysResMgrInit( k=%d corr=%f timeout=%f uncertainty=%f load=%f unif=%f speedsize=%f )\n",
                                bftrace.bfinit.K, 
                                bftrace.bfinit.csecCorrelatedTouch, bftrace.bfinit.csecTimeout, bftrace.bfinit.csecUncertainty,
                                bftrace.bfinit.dblHashLoadFactor, bftrace.bfinit.dblHashUniformity, bftrace.bfinit.dblSpeedSizeTradeoff );
                    break;

                case bftidCache:
                    wprintf( L"Cache: { %u:%u } %ws, %u%%, %ws\n", 
                                (ULONG)bftrace.bfcache.ifmp, bftrace.bfcache.pgno,
                                bftrace.bfcache.fUseHistory ? L"fTrue" : L"fFalse",
                                bftrace.bfcache.pctPri, 
                                WszLatchType( (BFLatchType)bftrace.bfcache.bflt ) );
                    break;

                case bftidVerifyInfo:
                    wprintf( L"Verif: { %u:%u }\n",
                                (ULONG)bftrace.bfverify.ifmp, bftrace.bfverify.pgno );
                    break;

                case bftidTouchLong:
                case bftidTouch:
                    wprintf( L"Touch: { %u:%u, %ws, %u%%, %ws }\n",
                                (ULONG)bftrace.bftouch.ifmp, bftrace.bftouch.pgno,
                                bftrace.bfcache.fUseHistory ? L"fTrue" : L"fFalse",
                                bftrace.bftouch.pctPri, 
                                WszLatchType( (BFLatchType)bftrace.bftouch.bflt ) );
                    break;

                case bftidDirty:
                    wprintf( L"Dirty: { %u:%u, %u, %X:%lX:%lX }\n",
                                (ULONG)bftrace.bfdirty.ifmp, bftrace.bfdirty.pgno, 
                                (ULONG)bftrace.bfdirty.bfdf,
                                bftrace.bfdirty.lgenModify, bftrace.bfdirty.isecModify, bftrace.bfdirty.ibModify );
                    break;

                case bftidWrite:
                    wprintf( L"Write: { %u:%u, %u, %u }\n",
                                (ULONG)bftrace.bfwrite.ifmp, bftrace.bfwrite.pgno, 
                                (ULONG)bftrace.bfwrite.bfdf, (ULONG)bftrace.bfwrite.iorp );
                    break;

                case bftidSetLgposModify:
                    wprintf( L"SetLgposModify: { %u:%u, %X:%lX:%lX }\n",
                                (ULONG)bftrace.bfsetlgposmodify.ifmp, bftrace.bfsetlgposmodify.pgno, 
                                bftrace.bfsetlgposmodify.lgenModify, bftrace.bfsetlgposmodify.isecModify, bftrace.bfsetlgposmodify.ibModify );
                    break;

                case bftidEvict:
                    wprintf( L"Evict: { %u:%u, %hs, %u }\n",
                                (ULONG)bftrace.bfevict.ifmp, bftrace.bfevict.pgno, 
                                bftrace.bfevict.fCurrentVersion ? "Current" : "Older",
                                bftrace.bfevict.bfef );
                    break;

                case bftidSysResMgrTerm:
                {
                    WCHAR wszDate[60];
                    WCHAR wszTime[60];
                    size_t  cbCrud;
                    Call( ErrUtilFormatFileTimeAsDate( bftrace.bfterm.ftTerm, wszDate, _countof(wszDate), &cbCrud ) );
                    Call( ErrUtilFormatFileTimeAsTimeWithSeconds( bftrace.bfterm.ftTerm, wszTime, _countof(wszTime), &cbCrud ) );
                    wprintf( L"Term: Time=%ws %ws\n", wszDate, wszTime );
                }
                    break;

                default:
                    AssertSz( fFalse, "Unknown BF FTL Trace (%d)!\n", (ULONG)bftrace.traceid );
                    wprintf( L"{ Unknown BF FTL Trace (%d) }\n", bftrace.traceid );
            }
        }
    }
    if ( err == errNotFound )
    {
        err = JET_errSuccess;
    }
    Call( err );

    wprintf( L"Done accumulating stats.\n" );
    wprintf( L"\n" );
    wprintf( L"\n" );

HandleError:

    if ( err < JET_errSuccess )
    {
        wprintf( L"Failed accumulating stats!  err=%d\n", err );
    }

    return err;
}



ERR ErrResMgrAccumEtlStats( __in BFETLContext * const pbfetlc )
{
    ERR err = JET_errSuccess;
    __int64 cTraces = 0;

    wprintf( L"Accumulating stats...\r\n" );

    BFTRACE bftrace;


    while ( ( err = ErrBFETLGetNext( pbfetlc, &bftrace ) ) >= JET_errSuccess )
    {
        cTraces++;

        if ( ( cTraces % 100000 ) == 0 )
        {
            printf( "." );
        }
    }

    wprintf( L"\r\n" );

    if ( err == errNotFound )
    {
        err = JET_errSuccess;
    }

    Call( err );

    wprintf( L"Done accumulating stats.\r\n" );
    wprintf( L"\r\n" );

HandleError:

    if ( err < JET_errSuccess )
    {
        wprintf( L"Failed accumulating stats! err=%d\r\n", err );
    }

    return err;
}


