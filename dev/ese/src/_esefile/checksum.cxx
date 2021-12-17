// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "esefile.hxx"
#include "xsum.hxx"

//  Checksum a file in ESE format
//


INT g_cbPage;
INT g_cpagesPerBlock;

static const INT cblocks            = g_cbBufferTotal / g_cbReadBuffer; //  number of read chunks


struct CHECKSUM_STATS
{
    //  progress variables

    DWORD               cpagesSeen;
    DWORD               cpagesBadChecksum;
    DWORD               cpagesCorrectableChecksum;
    DWORD               cpagesUninit;
    DWORD               cpagesWrongPgno;
    unsigned __int64    dbtimeHighest;
    DWORD               pgnoDbtimeHighest;
    DWORD               dwFiller;

    //  I/O latency timing

    LONG                cIOs;           //  total number of read IOs that were performed
    LONG                dtickTotal;     //  total tickcount for all IOs
    LONG                dtickMax;       //  maximum tickcount (slowest IO)
    LONG                dtickMin;       //  minimum elapsed tickcount (fastest IO)

    JET_ERR             errReadFileError;
};

static LONG     cpageMax;
static LONG     cblockMax;
static LONG     cblockCurr;

typedef DWORD (*PFNCHECKSUM)( const BYTE * const, const DWORD );


//  ================================================================
struct JETPAGE
//  ================================================================
{
    DWORD               dwChecksum;
    DWORD               pgno;
    unsigned __int64    dbtime;
//  BYTE                rgbRestOfPage[];
};


//  ================================================================
class CChecksumContext
//  ================================================================
{
    public:

        CChecksumContext(   CTaskManager* const     ptaskmgrIn,
                            CHECKSUM_STATS* const   pchecksumstatsIn,
                            CSemaphore* const       psemIn,
                            IFileAPI* const         pfapiIn,
                            BYTE* const             pbDataIn )
            :   ptaskmgr( ptaskmgrIn ),
                pchecksumstats( pchecksumstatsIn ),
                psem( psemIn ),
                pfapi( pfapiIn ),
                pbData( pbDataIn ),
                ibOffset( 0 ),
                cbData( 0 ),
                hrtStart( 0 )
        {
        }

        ~CChecksumContext()
        {
            OSMemoryPageFree( pbData );
        }

        VOID Complete() { psem->Release(); }

    public:

        CTaskManager* const     ptaskmgr;
        CHECKSUM_STATS* const   pchecksumstats;
        CSemaphore* const       psem;
        IFileAPI* const         pfapi;
        BYTE* const             pbData;
        QWORD                   ibOffset;
        DWORD                   cbData;
        QWORD                   hrtStart;
};


//  ================================================================
static void PrintChecksumStats(
    const QWORD                     dtickElapsed,
    const CHECKSUM_STATS * const    pchecksumstats )
//  ================================================================
{
    (void)wprintf( L"\r\n\r\n" );
    (void)wprintf( L"%d pages seen\r\n", pchecksumstats->cpagesSeen );
    (void)wprintf( L"%d bad checksums\r\n", pchecksumstats->cpagesBadChecksum );
    (void)wprintf( L"%d correctable checksums\r\n", pchecksumstats->cpagesCorrectableChecksum );
    (void)wprintf( L"%d uninitialized pages\r\n", pchecksumstats->cpagesUninit );
    (void)wprintf( L"%d wrong page numbers\r\n", pchecksumstats->cpagesWrongPgno );
    (void)wprintf( L"0x%I64x highest dbtime (pgno 0x%x)\r\n", pchecksumstats->dbtimeHighest, pchecksumstats->pgnoDbtimeHighest );
    (void)wprintf( L"\r\n" );
    (void)wprintf( L"%d reads performed\r\n", pchecksumstats->cIOs );

    const DWORD mbRead          = pchecksumstats->cpagesSeen / ( ( 1024 * 1024 ) / g_cbPage );
    const DWORD csecElapsed     = (DWORD)max( dtickElapsed / 1000, 1 );
    const DWORD mbPerSec        = mbRead / csecElapsed;
    DWORD cmsecPerRead          = 0;
    DWORD cmsecSlowestRead      = 0;
    DWORD cmsecFastestRead      = 0;
    if ( pchecksumstats->cIOs )
    {
        cmsecPerRead = pchecksumstats->dtickTotal / pchecksumstats->cIOs;
        cmsecSlowestRead = pchecksumstats->dtickMax;
        cmsecFastestRead = pchecksumstats->dtickMin;
    }
    (void)wprintf( L"%d MB read\r\n", mbRead );
    (void)wprintf( L"%d seconds taken\r\n", csecElapsed );
    (void)wprintf( L"%d MB/second\r\n", mbPerSec );
    (void)wprintf( L"%d milliseconds used\r\n", pchecksumstats->dtickTotal );
    (void)wprintf( L"%d milliseconds per read\r\n", cmsecPerRead );
    (void)wprintf( L"%d milliseconds for the slowest read\r\n", cmsecSlowestRead );
    (void)wprintf( L"%d milliseconds for the fastest read\r\n", cmsecFastestRead );
    (void)wprintf( L"\r\n" );
}


//  ================================================================
static BOOL FChecksumCorrect( const CHECKSUM_STATS * const pchecksumstats )
//  ================================================================
{
    return ( 0 == pchecksumstats->cpagesBadChecksum &&
                0 == pchecksumstats->cpagesWrongPgno &&
                0 != pchecksumstats->cpagesSeen );
}

//  Check the whole page content against zero. If all zero, then not initialized; otherwise initialized
static BOOL FIsPageInitialized(
    _Readable_bytes_(cbPage) const BYTE* const pbPage,
    _In_range_(1, g_cbPage) ULONG cbPage )
{
    static const BYTE rgbZeroMem[256] = { 0 };
    const BYTE* pbAddrToCompare = pbPage;
    const BYTE* const pbPageEnd = pbPage + cbPage;
    const BYTE* const pbLastAddrToCompare = (BYTE*)( pbPageEnd - sizeof ( rgbZeroMem ) );
    for (; pbAddrToCompare <= pbLastAddrToCompare; pbAddrToCompare += sizeof ( rgbZeroMem ) )
    {
        if ( memcmp ( pbAddrToCompare, rgbZeroMem, sizeof ( rgbZeroMem ) ) != 0 )
        {
            return fTrue;
        }
    }
    //  There might be some bytes less than sizeof(rgbZeroMem) not compared in the loop
    if ( pbAddrToCompare < pbPageEnd )
    {
        return ( memcmp ( pbAddrToCompare, rgbZeroMem, pbPageEnd - pbAddrToCompare ) != 0 );
    }
    return fFalse;
}

//  ================================================================
static void ProcessESEPages(    const QWORD             ibOffset,
                                const DWORD             cbData,
                                const BYTE* const       pbData,
                                CHECKSUM_STATS* const   pchecksumstats )
//  ================================================================
{
    const JETPAGE * const       rgpage          = (JETPAGE*)pbData;
    const DWORD                 cpage           = cbData / g_cbPage;
    const DWORD                 ipageStart      = (DWORD)( ibOffset / g_cbPage );

    for( DWORD ipage = 0; ipage < cpage; ++ipage )
    {
        const JETPAGE * const   ppageCurr       = (JETPAGE *)( (BYTE *)rgpage + ( ipage * g_cbPage ) );

        pchecksumstats->cpagesSeen++;

        if ( !FIsPageInitialized( (const BYTE* const)ppageCurr, g_cbPage ) )
        {
            pchecksumstats->cpagesUninit++;
        }
        else
        {
            const DWORD             pgnoPhysical        = ipageStart + ipage + 1;
            const BOOL              fHeaderPage         = ( pgnoPhysical <= g_cpgDBReserved );
            BOOL                    fTrailerPage        = fFalse;
            const DWORD             pgnoReal            = ( fHeaderPage ? 0 : pgnoPhysical - g_cpgDBReserved );
            const INT               pgnoDisplay         = (INT)( pgnoPhysical - g_cpgDBReserved );
            const wchar_t* const    wszPageTypeDisplay  = fHeaderPage ? L" (header)" : L"" ;
            PAGECHECKSUM            checksumExpected;
            PAGECHECKSUM            checksumActual;
            BOOL                    fCorrectableError   = fFalse;
            INT                     ibitCorrupted       = 0;

            if ( pgnoPhysical == (DWORD)cpageMax )
            {
                //  last physical page in the database, see
                //  if it looks like a trailer page
                //
                const DWORD * const rgdwPage    = (DWORD *)ppageCurr;

                if ( 0 == rgdwPage[1]       //  checksum (high dword)
                    && 0 == rgdwPage[2]     //  dbtimeDirtied (low dword)
                    && 0 == rgdwPage[3]     //  dbtimeDirtied (high dword)
                    && 0 == rgdwPage[6] )   //  objidFDP
                {
                    fTrailerPage = fTrue;
                }
            }

            ChecksumAndPossiblyFixPage(
                    (void* const)ppageCurr,
                    ( UINT )g_cbPage,
                    ( fHeaderPage || fTrailerPage ? databaseHeader : databasePage ),
                    pgnoReal,
                    fTrue,
                    &checksumExpected,
                    &checksumActual,
                    &fCorrectableError,
                    &ibitCorrupted );

            if( checksumExpected != checksumActual )
            {
                // we should  Assert( !fCorrectableError ); but there is no Assert in eseutil.

                (void)fwprintf( stderr, L"ERROR: page %d%s checksum failed\r\n", pgnoDisplay, wszPageTypeDisplay );
                pchecksumstats->cpagesBadChecksum++;
            }
            else
            {
                if ( fCorrectableError )
                {
                    (void)fwprintf( stderr, L"WARNING: page %d%s checksum verification failed but the corruption ( bit %d ) can be corrected\r\n", pgnoDisplay, wszPageTypeDisplay, ibitCorrupted );
                    pchecksumstats->cpagesCorrectableChecksum++;
                }

                if ( !fHeaderPage )
                {
                    if ( ppageCurr->dbtime > pchecksumstats->dbtimeHighest )
                    {
                        pchecksumstats->dbtimeHighest = ppageCurr->dbtime;
                        pchecksumstats->pgnoDbtimeHighest = pgnoReal;
                    }
                }
            }

        }
    }
}


//  ================================================================
static void CollectStatistics(
    const QWORD                 hrtCompleted,
    const ERR                   err,
    CChecksumContext * const    pchecksumcontext )
//  ================================================================
{
    CHECKSUM_STATS * const  pchecksumstats  = pchecksumcontext->pchecksumstats;
    const DWORD             dtickElapsed    = (DWORD)CmsecHRTFromDhrt( hrtCompleted - pchecksumcontext->hrtStart );

    AtomicIncrement( &pchecksumstats->cIOs );
    AtomicExchangeAdd( &pchecksumstats->dtickTotal, dtickElapsed );
    AtomicExchangeMax( &pchecksumstats->dtickMax, dtickElapsed );
    AtomicExchangeMin( &pchecksumstats->dtickMin, dtickElapsed );

    if ( err < JET_errSuccess )
    {
        WCHAR wszError[ 512 ] = { 0 };

        (void)JetGetSystemParameterW( JET_instanceNil,
            JET_sesidNil,
            JET_paramErrorToString,
            (ULONG_PTR *)&err,
            wszError,
            sizeof( wszError ) );
        wprintf( L"Read at offset 0x%016llx for %d bytes failed with error %d (%s).\n",
            pchecksumcontext->ibOffset,
            pchecksumcontext->cbData,
            err,
            wszError );

        AtomicCompareExchange( &pchecksumstats->errReadFileError, JET_errSuccess, err );
    }
}

static ERR ErrIssueNextIO( CChecksumContext* const pchecksumcontext );

static VOID IOHandoff(  const ERR               errIO,
                        IFileAPI* const         pfapiInner,
                        const FullTraceContext& tc,
                        const OSFILEQOS         grbitQOS,
                        const QWORD             ibOffset,
                        const DWORD             cbData,
                        const BYTE* const       pbData,
                        CChecksumContext* const pchecksumcontext,
                        void* const             pvIOContext )
{
    ERR err = JET_errSuccess;

    Call( errIO );

HandleError:
    if ( err < JET_errSuccess )
    {
        pchecksumcontext->Complete();
    }
}

static VOID ProcessPages(   const DWORD             dwError,
                            const DWORD_PTR         dwThreadContext,
                            const DWORD             dwCompletionKey1,
                            CChecksumContext* const pchecksumcontext )
{
    ERR err = JET_errSuccess;

    ProcessESEPages( pchecksumcontext->ibOffset, pchecksumcontext->cbData, pchecksumcontext->pbData, pchecksumcontext->pchecksumstats );
    Call( ErrIssueNextIO( pchecksumcontext ) );
    CallS( pchecksumcontext->pfapi->ErrIOIssue() );

HandleError:
    if ( err < JET_errSuccess )
    {
        pchecksumcontext->Complete();
    }
}

static VOID IOComplete(       ERR               errIO,
                        IFileAPI* const         pfapi,
                        const FullTraceContext& tc,
                        const OSFILEQOS         grbitQOS,
                        const QWORD             ibOffset,
                        const DWORD             cbData,
                        const BYTE* const       pbData,
                        CChecksumContext* const pchecksumcontext )
{
    ERR err = JET_errSuccess;

    CollectStatistics( HrtHRTCount(), errIO, pchecksumcontext );
    Call( errIO );

    Call( pchecksumcontext->ptaskmgr->ErrTMPost( (CTaskManager::PfnCompletion)ProcessPages, 0, (DWORD_PTR)pchecksumcontext ) );

HandleError:
    if ( err < JET_errSuccess )
    {
        pchecksumcontext->Complete();
    }
}

static ERR ErrIssueNextIO( CChecksumContext* const pchecksumcontext )
{
    ERR                 err     = JET_errSuccess;
    TraceContextScope   tcScope( iorpDirectAccessUtil );

    const QWORD iblock = AtomicIncrement( &cblockCurr ) - 1;

    if ( iblock >= cblockMax )
    {
        Call( ErrERRCheck( JET_errFileIOBeyondEOF ) );
    }

    pchecksumcontext->ibOffset = g_cbPage * iblock * g_cpagesPerBlock;
    pchecksumcontext->cbData = (DWORD)( g_cbPage * min( g_cpagesPerBlock, cpageMax - ( iblock * g_cpagesPerBlock ) ) );
    pchecksumcontext->hrtStart = HrtHRTCount();

    Call( pchecksumcontext->pfapi->ErrIORead(   *tcScope,
                                                pchecksumcontext->ibOffset,
                                                pchecksumcontext->cbData,
                                                pchecksumcontext->pbData,
                                                qosIONormal,
                                                (IFileAPI::PfnIOComplete)IOComplete,
                                                (DWORD_PTR)pchecksumcontext,
                                                (IFileAPI::PfnIOHandoff)IOHandoff ) );

HandleError:
    return err;
}

//  ================================================================
BOOL FChecksumFile(
    const wchar_t * const   szFile,
    const ULONG             cbPage,
    const ULONG             cPagesToChecksum,
    BOOL * const            pfBadPagesDetected )
//  ================================================================
{
    BOOL                fSuccess            = fFalse;
    ERR                 err                 = JET_errSuccess;

    CTaskManager        taskmgr;
    IFileSystemAPI*     pfsapi              = NULL;
    IFileAPI*           pfapi               = NULL;
    BYTE*               pbBlock             = NULL;
    CChecksumContext**  rgpchecksumcontext  = NULL;

    CHECKSUM_STATS      checksumstats       = { 0 };
    CSemaphore          sem( CSyncBasicInfo( "FChecksumFile" ) );

    INT                 iblockio            = 0;

    QWORD               cbSize              = 0;

    HRT                 hrtFirstIO;  //  the time the first IO started
    HRT                 hrtLastIO;   //  the time the last IO finished


    wprintf( L"File: %.64ls", ( iswascii( szFile[0] ) ? szFile : L"<unprintable>" ) );
    wprintf( L"\r\n\r\n" );

    //  init

    InitPageSize( cbPage );

    cpageMax                = 0;
    cblockMax               = 0;
    cblockCurr              = 0;

    //  create a file system with our desired IO characteristics

    class CFileSystemConfiguration : public CDefaultFileSystemConfiguration
    {
        public:

            CFileSystemConfiguration()
            {
                m_dtickAccessDeniedRetryPeriod = 0;
                m_cbMaxReadSize = g_cbReadBuffer;
                m_cbMaxWriteSize = g_cbReadBuffer;
                m_fBlockCacheEnabled = fTrue;
            }
    } fsconfig;

    Call( ErrOSFSCreate( &fsconfig, &pfsapi ) );

    //  create the task manager

    Call( taskmgr.ErrTMInit( cblocks, NULL, fTrue ) );

    //  open the file

    Call( pfsapi->ErrFileOpen( szFile, IFileAPI::fmfReadOnlyPermissive, &pfapi ) );

    //  get the size of the file

    Call( pfapi->ErrSize( &cbSize, IFileAPI::filesizeLogical ) );

    //  see if we're restricted to checking up to a specified size

    cbSize = cPagesToChecksum == 0 ? cbSize : min( cbSize, QWORD( g_cbPage ) * cPagesToChecksum );

    cpageMax    = (DWORD)( cbSize / g_cbPage );
    cblockMax   = ( cpageMax + g_cpagesPerBlock - 1 ) / g_cpagesPerBlock;

    //  checksum the file using async IO with a fixed queue depth

    hrtFirstIO = HrtHRTCount();

    Alloc( rgpchecksumcontext = new CChecksumContext*[ cblocks ] );
    for ( iblockio = 0; iblockio < cblocks; ++iblockio )
    {
        Alloc( pbBlock = (BYTE*)PvOSMemoryPageAlloc( g_cbReadBuffer, NULL ) );
        Alloc( rgpchecksumcontext[ iblockio ] = new CChecksumContext( &taskmgr, &checksumstats, &sem, pfapi, pbBlock ) );
        pbBlock = NULL;

        err = ErrIssueNextIO( rgpchecksumcontext[ iblockio ] );
        if ( err == JET_errFileIOBeyondEOF )
        {
            rgpchecksumcontext[ iblockio ]->Complete();
            err = JET_errSuccess;
        }
        Call( err );
    }

    Call( pfapi->ErrIOIssue() );

    InitStatus( L"Checksum Status (% complete)" );

    //  wait until the io is done
    //  wake up to update the status bar

    while ( iblockio > 0 )
    {
        const BOOL fAcquired = sem.FAcquire( 100 );
        if ( !fAcquired )
        {
            const INT  iPercentage  = ( cblockCurr * 100 ) / cblockMax;
            UpdateStatus( iPercentage );
        }
        else
        {
            iblockio--;
        }
    }

    hrtLastIO = HrtHRTCount();

    TermStatus();
    PrintChecksumStats( CmsecHRTFromDhrt( hrtLastIO - hrtFirstIO ), &checksumstats );

    fSuccess = FChecksumCorrect( &checksumstats );

    //  by default we return fFalse if a checksum mismatch was detected,
    //  but if a separate flag was passed in to detect checksum
    //  mismatches, then always return fTrue if we got this far,
    //  and report checksum mismatches in the flag
    //
    if ( NULL != pfBadPagesDetected )
    {
        *pfBadPagesDetected = !fSuccess;
        fSuccess = fTrue;
    }

HandleError:

    OSMemoryPageFree( pbBlock );
    if ( pfapi )
    {
        (VOID)pfapi->ErrIOIssue();
    }
    for ( ; iblockio > 0; iblockio-- )
    {
        sem.Acquire();
    }
    taskmgr.TMTerm();
    if ( rgpchecksumcontext )
    {
        for ( iblockio = 0; iblockio < cblocks; ++iblockio )
        {
            delete rgpchecksumcontext[ iblockio ];
        }
        delete[] rgpchecksumcontext;
    }
    delete pfapi;
    delete pfsapi;

    return fSuccess && err >= JET_errSuccess && checksumstats.errReadFileError >= JET_errSuccess;
}

