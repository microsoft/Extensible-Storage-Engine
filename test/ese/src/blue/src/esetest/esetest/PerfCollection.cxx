// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//================================================================
// Performance collection functions.
//================================================================
//

#include <ese_common.hxx>
#include <strsafe.h>
#include <stdlib.h>

typedef struct
{
    //  Actual counters.
    const PerfCollectorCounter* ctrAvg;
    size_t cctrAvg;
    const PerfCollectorCounter* ctrFormatted;
    size_t cctrFormatted;
    const PerfCollectorCounter* ctrRaw;
    size_t cctrRaw;
    const PerfCollectorCustomCounter* ctrCustom;
    size_t cctrCustom;

    //  Queries.
    HANDLE qAvg;
    HANDLE qFormatted;
    HANDLE qRaw;

    //  Internal buffers.
    double* dblAvgMin;
    double* dblAvgMax;
    double* dblAvgAvg;
    double* dblAvgVar;
    double* dblFormatted;
    ULONG64* qwRaw1;
    ULONG64* qwRaw2;

    //  Other states.
    void* pvContext;
} PerfCollector;

char* SzFilterInvalidChars( char* const szBuffer, const char* const sz )
{
    bool fUpper             = true;
    const char szValid[]    = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-";
    size_t iBuffer = 0;

    for ( size_t i = 0 ; sz[ i ] ; i++ )
    {
        char chToAppend = sz[ i ];
        if ( strchr( szValid, chToAppend ) )
        {
            if ( fUpper )
            {
                chToAppend = ( char )toupper( chToAppend );
                fUpper = false;
            }
            szBuffer[iBuffer++] = chToAppend;
        }
        else
        {
            fUpper = true;
        }
    }
    szBuffer[iBuffer] = '\0';

    return szBuffer;
}

void PerfCollectorInitStates( PerfCollector* const pPerfCollector,
                                const PerfCollectorCounter* const ctrAvg, const size_t cctrAvg,
                                const PerfCollectorCounter* const ctrFormatted, const size_t cctrFormatted,
                                const PerfCollectorCounter* const ctrRaw, const size_t cctrRaw,
                                const PerfCollectorCustomCounter* const ctrCustom, const size_t cctrCustom,
                                void* const pvContext )
{
    pPerfCollector->ctrAvg = ctrAvg;
    pPerfCollector->cctrAvg = cctrAvg;
    pPerfCollector->ctrFormatted = ctrFormatted;
    pPerfCollector->cctrFormatted = cctrFormatted;
    pPerfCollector->ctrRaw = ctrRaw;
    pPerfCollector->cctrRaw = cctrRaw;
    pPerfCollector->ctrCustom = ctrCustom;
    pPerfCollector->cctrCustom = cctrCustom;
    pPerfCollector->qAvg = NULL;
    pPerfCollector->qFormatted = NULL;
    pPerfCollector->qRaw = NULL;
    pPerfCollector->dblAvgMin = NULL;
    pPerfCollector->dblAvgMax = NULL;
    pPerfCollector->dblAvgAvg = NULL;
    pPerfCollector->dblAvgVar = NULL;
    pPerfCollector->dblFormatted = NULL;
    pPerfCollector->qwRaw1 = NULL;
    pPerfCollector->qwRaw2 = NULL;
    pPerfCollector->pvContext = pvContext;
}


void PerfCollectorFreeResources( PerfCollector* const pPerfCollector )
{
    CallVTrue( PerfCountersDestroyQuery( pPerfCollector->qAvg ) );
    CallVTrue( PerfCountersDestroyQuery( pPerfCollector->qRaw ) );
    CallVTrue( PerfCountersDestroyQuery( pPerfCollector->qFormatted ) );
    pPerfCollector->qAvg = NULL;
    pPerfCollector->qRaw = NULL;
    pPerfCollector->qFormatted = NULL;
    delete[] pPerfCollector->dblAvgMin;
    delete[] pPerfCollector->dblAvgMax;
    delete[] pPerfCollector->dblAvgAvg;
    delete[] pPerfCollector->dblAvgVar;
    delete[] pPerfCollector->dblFormatted;
    delete[] pPerfCollector->qwRaw1;
    delete[] pPerfCollector->qwRaw2;
    pPerfCollector->dblAvgMin = NULL;
    pPerfCollector->dblAvgMax = NULL;
    pPerfCollector->dblAvgAvg = NULL;
    pPerfCollector->dblAvgVar = NULL;
    pPerfCollector->dblFormatted = NULL;
    pPerfCollector->qwRaw1 = NULL;
    pPerfCollector->qwRaw2 = NULL;
}

HANDLE PerfCollectorCreate( const PerfCollectorCounter* const ctrAvg, const size_t cctrAvg,
                            const PerfCollectorCounter* const ctrFormatted, const size_t cctrFormatted,
                            const PerfCollectorCounter* const ctrRaw, const size_t cctrRaw,
                            const PerfCollectorCustomCounter* const ctrCustom, const size_t cctrCustom,
                            void* const pvContext )
{
    PerfCollector* const pPerfCollector = new PerfCollector;
    
    PerfCollectorInitStates( pPerfCollector, ctrAvg, cctrAvg, ctrFormatted, cctrFormatted, ctrRaw, cctrRaw, ctrCustom, cctrCustom, pvContext );

    return (HANDLE)pPerfCollector;
}

void PerfCollectorDestroy( HANDLE hCollector )
{
    PerfCollector* const pPerfCollector = (PerfCollector*)hCollector;

    delete pPerfCollector;
}

void PerfCollectorStart( HANDLE hCollector, const DWORD cmsecSampling )
{
    PerfCollector* const pPerfCollector = (PerfCollector*)hCollector;

    CallVTrue( pPerfCollector->qAvg == NULL );
    CallVTrue( pPerfCollector->qFormatted == NULL );
    CallVTrue( pPerfCollector->qRaw == NULL );
    CallVTrue( pPerfCollector->dblAvgMin == NULL );
    CallVTrue( pPerfCollector->dblAvgMax == NULL );
    CallVTrue( pPerfCollector->dblAvgAvg == NULL );
    CallVTrue( pPerfCollector->dblAvgVar == NULL );
    CallVTrue( pPerfCollector->dblFormatted == NULL );
    CallVTrue( pPerfCollector->qwRaw1 == NULL );
    CallVTrue( pPerfCollector->qwRaw2 == NULL );

    pPerfCollector->dblAvgMin = new double[ pPerfCollector->cctrAvg ];
    pPerfCollector->dblAvgMax = new double[ pPerfCollector->cctrAvg ];
    pPerfCollector->dblAvgAvg = new double[ pPerfCollector->cctrAvg ];
    pPerfCollector->dblAvgVar = new double[ pPerfCollector->cctrAvg ];
    pPerfCollector->dblFormatted = new double[ pPerfCollector->cctrFormatted ];
    pPerfCollector->qwRaw1 = new ULONG64[ pPerfCollector->cctrRaw ];
    pPerfCollector->qwRaw2 = new ULONG64[ pPerfCollector->cctrRaw ];
    CallVTrue( pPerfCollector->dblAvgMin != NULL || pPerfCollector->cctrAvg == 0 );
    CallVTrue( pPerfCollector->dblAvgMax != NULL || pPerfCollector->cctrAvg == 0 );
    CallVTrue( pPerfCollector->dblAvgAvg != NULL || pPerfCollector->cctrAvg == 0 );
    CallVTrue( pPerfCollector->dblAvgVar != NULL || pPerfCollector->cctrAvg == 0 );
    CallVTrue( pPerfCollector->dblFormatted != NULL || pPerfCollector->cctrFormatted == 0 );
    CallVTrue( pPerfCollector->qwRaw1 != NULL || pPerfCollector->cctrRaw == 0 );
    CallVTrue( pPerfCollector->qwRaw2 != NULL || pPerfCollector->cctrRaw == 0 );

    //  Avg.
    pPerfCollector->qAvg = PerfCountersCreateQuery();
    CallVTrue( pPerfCollector->qAvg != NULL );
    JET_GRBIT* grbit = new JET_GRBIT[ pPerfCollector->cctrAvg ];
    CallVTrue( grbit != NULL || pPerfCollector->cctrAvg == 0 );
    for ( size_t i = 0 ; i < pPerfCollector->cctrAvg ; i++ )
    {
        CallVTrue( PerfCountersAddLnCounterA( pPerfCollector->qAvg, NULL, pPerfCollector->ctrAvg[ i ].szObject, pPerfCollector->ctrAvg[ i ].szName, pPerfCollector->ctrAvg[ i ].szInstance ) );
        grbit[ i ] = pPerfCollector->ctrAvg[ i ].grbitStats;
    }

    //  Formatted.
    pPerfCollector->qFormatted = PerfCountersCreateQuery();
    CallVTrue( pPerfCollector->qFormatted != NULL );
    for ( size_t i = 0 ; i < pPerfCollector->cctrFormatted ; i++ )
    {
        CallVTrue( PerfCountersAddLnCounterA( pPerfCollector->qFormatted, NULL, pPerfCollector->ctrFormatted[ i ].szObject, pPerfCollector->ctrFormatted[ i ].szName, pPerfCollector->ctrFormatted[ i ].szInstance ) );
    }

    //  Raw.
    pPerfCollector->qRaw = PerfCountersCreateQuery();
    CallVTrue( pPerfCollector->qRaw != NULL );
    for ( size_t i = 0 ; i < pPerfCollector->cctrRaw ; i++ )
    {
        CallVTrue( PerfCountersAddLnCounterA( pPerfCollector->qRaw, NULL, pPerfCollector->ctrRaw[ i ].szObject, pPerfCollector->ctrRaw[ i ].szName, pPerfCollector->ctrRaw[ i ].szInstance ) );
    }

    //  Custom.
    for ( size_t i = 0 ; i < pPerfCollector->cctrCustom ; i++ )
    {
        pPerfCollector->ctrCustom[ i ].pfnStartStop( hCollector, pPerfCollector->pvContext, &pPerfCollector->ctrCustom[ i ], true );
    }

    if ( pPerfCollector->cctrFormatted > 0 )
    {
        CallVTrue( PerfCountersGetCounterValues( pPerfCollector->qFormatted, pPerfCollector->dblFormatted ) );
    }
    if ( pPerfCollector->cctrRaw > 0 )
    {
        CallVTrue( PerfCountersGetCounterValuesRaw( pPerfCollector->qRaw, pPerfCollector->qwRaw1, NULL ) );
    }   
    if ( pPerfCollector->cctrAvg > 0 )
    {
        CallVTrue( PerfCountersStartCollectingStatsA( pPerfCollector->qAvg, cmsecSampling, grbit, NULL ) );
    }

    delete[] grbit;
}

void PerfCollectorCancel( HANDLE hCollector )
{
    PerfCollector* const pPerfCollector = (PerfCollector*)hCollector;

    if ( pPerfCollector->cctrAvg > 0 )
    {
        CallVTrue( PerfCountersStopCollectingStats( pPerfCollector->qAvg ) );
    }

    PerfCollectorFreeResources( pPerfCollector );
    PerfCollectorInitStates( pPerfCollector, pPerfCollector->ctrAvg, pPerfCollector->cctrAvg, pPerfCollector->ctrFormatted, pPerfCollector->cctrFormatted, pPerfCollector->ctrRaw, pPerfCollector->cctrRaw, pPerfCollector->ctrCustom, pPerfCollector->cctrCustom, pPerfCollector->pvContext );
}


void PerfCollectorStopAndReport( HANDLE hCollector, const char* const szLabel )
{
    PerfCollector* const pPerfCollector = (PerfCollector*)hCollector;
    char szCounterName[ MAX_PATH ] = { 0 };
    char szCounterReportName[ MAX_PATH ] = { 0 };

    if ( pPerfCollector->cctrAvg > 0 )
    {
        CallVTrue( PerfCountersRetrieveStats( pPerfCollector->qAvg, NULL, pPerfCollector->dblAvgMin, NULL, pPerfCollector->dblAvgMax, NULL, pPerfCollector->dblAvgAvg, pPerfCollector->dblAvgVar, NULL ) );
        CallVTrue( PerfCountersStopCollectingStats( pPerfCollector->qAvg ) );
    }
    if ( pPerfCollector->cctrRaw > 0 )
    {
        CallVTrue( PerfCountersGetCounterValuesRaw( pPerfCollector->qRaw, pPerfCollector->qwRaw2, NULL ) );
    }
    if ( pPerfCollector->cctrFormatted > 0 )
    {
        CallVTrue( PerfCountersGetCounterValues( pPerfCollector->qFormatted, pPerfCollector->dblFormatted ) );
    }

    //  Report.
    HANDLE hFile = PerfReportingCreateFileA( SzEsetestPerfSummaryXml() );
    CallVTrue( hFile != NULL );
    for ( size_t i = 0 ; i < pPerfCollector->cctrAvg ; i++ )
    {
        if ( pPerfCollector->ctrAvg[ i ].grbitStats & ESETEST_bitPCHelperCollectMin )
        {
            StringCchPrintfA( szCounterReportName,
                                _countof( szCounterReportName ),
                                "%s.%s%s",
                                szLabel,
                                SzFilterInvalidChars( szCounterName, pPerfCollector->ctrAvg[ i ].szName ),
                                "Min" );
            CallVTrue( PerfReportingReportValueA( hFile,
                                                    szCounterReportName,
                                                    pPerfCollector->ctrAvg[ i ].szUnit,
                                                    pPerfCollector->ctrAvg[ i ].szFormat,
                                                    pPerfCollector->ctrAvg[ i ].fHigherIsBetter,
                                                    pPerfCollector->dblAvgMin[ i ] ) );
        }
        if ( pPerfCollector->ctrAvg[ i ].grbitStats & ESETEST_bitPCHelperCollectMax )
        {
            StringCchPrintfA( szCounterReportName,
                                _countof( szCounterReportName ),
                                "%s.%s%s",
                                szLabel,
                                SzFilterInvalidChars( szCounterName,  pPerfCollector->ctrAvg[ i ].szName ),
                                "Max" );
            CallVTrue( PerfReportingReportValueA( hFile,
                                                    szCounterReportName,
                                                    pPerfCollector->ctrAvg[ i ].szUnit,
                                                    pPerfCollector->ctrAvg[ i ].szFormat,
                                                    pPerfCollector->ctrAvg[ i ].fHigherIsBetter,
                                                    pPerfCollector->dblAvgMax[ i ] ) );
        }

        if ( pPerfCollector->ctrAvg[ i ].grbitStats & ESETEST_bitPCHelperCollectAvg )
        {
            StringCchPrintfA( szCounterReportName,
                                _countof( szCounterReportName ),
                                "%s.%s%s",
                                szLabel,
                                SzFilterInvalidChars( szCounterName,  pPerfCollector->ctrAvg[ i ].szName ),
                                "Avg" );
            CallVTrue( PerfReportingReportValueA( hFile,
                                                    szCounterReportName,
                                                    pPerfCollector->ctrAvg[ i ].szUnit,
                                                    pPerfCollector->ctrAvg[ i ].szFormat,
                                                    pPerfCollector->ctrAvg[ i ].fHigherIsBetter,
                                                    pPerfCollector->dblAvgAvg[ i ] ) );
        }

        if ( pPerfCollector->ctrAvg[ i ].grbitStats & ESETEST_bitPCHelperCollectVar )
        {
            StringCchPrintfA( szCounterReportName,
                                _countof( szCounterReportName ),
                                "%s.%s%s",
                                szLabel,
                                SzFilterInvalidChars( szCounterName,  pPerfCollector->ctrAvg[ i ].szName ),
                                "Var" );
            CallVTrue( PerfReportingReportValueA( hFile,
                                                    szCounterReportName,
                                                    pPerfCollector->ctrAvg[ i ].szUnit,
                                                    pPerfCollector->ctrAvg[ i ].szFormat,
                                                    pPerfCollector->ctrAvg[ i ].fHigherIsBetter,
                                                    pPerfCollector->dblAvgVar[ i ] ) );
        }
    }
    for ( size_t i = 0 ; i < pPerfCollector->cctrFormatted ; i++ )
    {
        StringCchPrintfA( szCounterReportName,
                            _countof( szCounterReportName ),
                            "%s.%s",
                            szLabel,
                            SzFilterInvalidChars( szCounterName,  pPerfCollector->ctrFormatted[ i ].szName ) );
        CallVTrue( PerfReportingReportValueA( hFile,
                                                szCounterReportName,
                                                pPerfCollector->ctrFormatted[ i ].szUnit,
                                                pPerfCollector->ctrFormatted[ i ].szFormat,
                                                pPerfCollector->ctrFormatted[ i ].fHigherIsBetter,
                                                pPerfCollector->dblFormatted[ i ] ) );
    }
    for ( size_t i = 0 ; i < pPerfCollector->cctrRaw ; i++ )
    {
        StringCchPrintfA( szCounterReportName,
                            _countof( szCounterReportName ),
                            "%s.%s%s",
                            szLabel,
                            SzFilterInvalidChars( szCounterName,  pPerfCollector->ctrRaw[ i ].szName ),
                            "Raw" );
        CallVTrue( PerfReportingReportValueA( hFile,
                                                szCounterReportName,
                                                pPerfCollector->ctrRaw[ i ].szUnit,
                                                pPerfCollector->ctrRaw[ i ].szFormat,
                                                pPerfCollector->ctrRaw[ i ].fHigherIsBetter,
                                                (double)( pPerfCollector->qwRaw2[ i ] - pPerfCollector->qwRaw1[ i ] ) ) );
    }
    for ( size_t i = 0 ; i < pPerfCollector->cctrCustom ; i++ )
    {
        StringCchPrintfA( szCounterReportName,
                            _countof( szCounterReportName ),
                            "%s.%s",
                            szLabel,
                            SzFilterInvalidChars( szCounterName,  pPerfCollector->ctrCustom[ i ].szName ) );
        CallVTrue( PerfReportingReportValueA( hFile,
                                                szCounterReportName,
                                                pPerfCollector->ctrCustom[ i ].szUnit,
                                                pPerfCollector->ctrCustom[ i ].szFormat,
                                                pPerfCollector->ctrCustom[ i ].fHigherIsBetter,
                                                pPerfCollector->ctrCustom[ i ].pfnStartStop( hCollector, pPerfCollector->pvContext, &pPerfCollector->ctrCustom[ i ], false ) ) );
    }
    CallVTrue( PerfReportingCloseFile( hFile ) );
    
    PerfCollectorFreeResources( pPerfCollector );
    PerfCollectorInitStates( pPerfCollector, pPerfCollector->ctrAvg, pPerfCollector->cctrAvg, pPerfCollector->ctrFormatted, pPerfCollector->cctrFormatted, pPerfCollector->ctrRaw, pPerfCollector->cctrRaw, pPerfCollector->ctrCustom, pPerfCollector->cctrCustom, pPerfCollector->pvContext );
}
