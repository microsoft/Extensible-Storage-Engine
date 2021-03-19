// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef ITERQUERY_HXX_INCLUDED
#define ITERQUERY_HXX_INCLUDED

// ---------------------------------------------------------------------------
//
//  Iteration Query support
//
//  Provides a simple iterative query against a set of simple structures, applying 
//  a predicate query to each entry first, and then applying an action to print,
//  pick, min, max, collate in some way the results (yes, it supports histograms).


// ---------------------------------------------------------------------------
//  Using Iteration Query facilities
//
//  1) #define any of the "Basic Facilities Extensions" points you want.
//  2) #include "iterquery.hxx" in your .cxx file (NYI for headers).
//  3) Define your IEntryDescriptor class 
//      A) And an array of CMemberDescriptor member infos therein.
//  4) Initialize the query with ErrIQCreateIterQuery[Count]().
//  5) In your iterator loop, make use of:
//      A) Call pic->FEvaluatePredicate( qwAddr, pcvEntry )
//      B) And if returns true, call pic->PerformAction( qwAddr, pcvEntry );
//  6) Sorry this is so many steps, SOMEONE.

// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------



// -------------------------------------------------------------
//
//  Basic Facilities Extensions
//

//  The including app may want to specify custom malloc/free/warning facilities
//  that work better in their scole.


// In the meant time, for these facilities:
//    If you can not afford a lawyer, one will be provided for you.

#ifndef FITQUDebugMode
#define FITQUDebugMode            fTrue
#endif

#ifndef PvITQUAlloc
#define PvITQUAlloc( cb )         malloc( cb )
#endif

#ifndef ITQUFree
#define ITQUFree( p )             free( p )
#endif

#ifndef ITQUAddWarning
#define ITQUAddWarning( sz )      printf( sz )
#endif

#ifndef ITQUPrintf
#define ITQUPrintf( szFmt, ... )  printf( szFmt, __VA_ARGS__ )
#endif

#ifndef ITQUAssert
#define ITQUAssert                Assert
#endif

#ifndef ITQUAssertSz
#define ITQUAssertSz              AssertSz
#endif

#ifndef ITQUSetExprCxt
#define ITQUSetExprCxt( pentry )  
#endif


// -------------------------------------------------------------
//
//  Constants and Errors
//

enum IQERR
{
    errSuccess           = 0,

    errInternalError     = -1,
    errInvalidParameter  = -2,
    errOutOfMemory       = -3,

};

#define ErrFromIqerr( iqerr )   \
            ( ( iqerr == IQERR::errSuccess ) ?              \
                  JET_errSuccess :                          \
              ( iqerr == IQERR::errInternalError ) ?        \
                  JET_errInternalError :                    \
              ( iqerr == IQERR::errInvalidParameter ) ?     \
                  JET_errInternalError :                    \
              ( iqerr == IQERR::errOutOfMemory ) ?          \
                  JET_errOutOfMemory :                      \
                  JET_wrnNyi                                \
            )

#define ITQUCall( f )                        \
            err = ( f );                     \
            if ( err < IQERR::errSuccess )   \
            {                                \
                goto HandleError;            \
            }


// -------------------------------------------------------------
//
//  Abstract Types the query processes
//

typedef void *        PvEntry;
typedef const void *  PcvEntry;

typedef QWORD         QwEntryAddr;

//
//  Function types for evaluating fields of the Entries.
//
//  pVal1 = retrieved member / element (from loop), pVal2 = value from original query string
typedef INT (*PfnExprEval)( void * pVal1, void * pVal2 );
typedef BOOL (*PfnFReadVal)( const char * szArg, void ** ppvValue );
typedef void (*PfnPrintVal)( const void * pVal );

typedef ERR (*PfnErrMemberFromEntry)( size_t iCtx1, size_t iCtx2, QwEntryAddr qwEntryAddr, PcvEntry pcvEntry, ULONGLONG * pullValue );

class IEntryDescriptor;
typedef ERR (*PfnQueryVisitAction)( const IEntryDescriptor * const pied, QwEntryAddr qwEntryAddr, PcvEntry pcvEntry, void * pvContext );
typedef ERR (*PfnQueryFinalAction)( const IEntryDescriptor * const pied, void * pContext );
//  Note: while the PfnQuery*Action return an error, these errors are currently ignored ...


// -------------------------------------------------------------
//
//  Flags for Histo Printing Control 
//

// QueryTypeHistoInfo 
//
#define eNoHistoSupport         (0)
#define ePerfectHisto           (0x80000000)
#define ePartialHisto           (0x40000000)
#define eFlagsHisto             (0x20000000)
#define eAttemptContinousPrint  (0x10000000)
#define mskHistoType            (ePerfectHisto|ePartialHisto|eFlagsHisto)
#define mskHistoOptions         (eAttemptContinousPrint)
#define mskHistoSection         (~(mskHistoType|mskHistoOptions))

// -------------------------------------------------------------
//
//  Type Processing (Reading, Comparing, Printing) Functions
//

//  First some generic helper functions

// Shamefully copied from edbg.cxx, and renamed.
BOOL FITQUUlFromSz( const char* const sz, ULONG* const pul, const INT base = 16 )
{
    if( sz && *sz )
    {
        char* pchEnd;
        *pul = strtoul( sz, &pchEnd, base );
        return !( *pchEnd );
    }
    return fFalse;
}

// Shamefully copied from eseioutil.cxx, and renamed.  
// Note: Also where it was passing base 10, instead of base 16 ... which I find confusing
// for a QWORD.
// Also it is wchar there, here it is char.  So need rationalization there too.
// Note this purposely does not init pnArg, unless it reads an integer.
BOOL FITQUAddressFromSz( int argc, __in_ecount(argc) const CHAR ** const argv, int iarg, QWORD * pnArg )
    {
    if ( iarg >= argc )
    {
        return fFalse;
    }
    if ( NULL == argv[iarg] )
    {
        return fFalse;
    }

    CHAR * pchEnd;
    QWORD qw = _strtoui64( argv[iarg], &pchEnd, 16 );

    if ( pchEnd == argv[iarg] )
    {
        return fFalse;
    }

    if ( *pchEnd == '`' && qw < (QWORD)ulMax )
    {
        //  This is a weird debugger pointer var, like: 0x0000017b`d118a000
        CHAR * pchMid = pchEnd;
        QWORD qwLow = _strtoui64( pchMid + 1, &pchEnd, 16 );
        
        if ( pchEnd == pchMid )
        {
            return fTrue; // guess it's just a small 64-bit value followed by a back-tick
        }

        qw = ( qw << 32 ) | qwLow;
    }

    // we managed to read something like a number from the string ...
    if ( pnArg )
    {
        *pnArg = qw;
    }
    return fTrue;
    }

//  Type specific functions

BOOL BoolReadVal( const char * szBool, void ** ppvValue )
{
    *ppvValue = PvITQUAlloc(sizeof(BOOL));
    if ( *ppvValue == NULL )
    {
        ITQUPrintf( "Allocation failure.\n" );
        return fFalse;
    }

    if ( ( 0 == _stricmp( szBool, "true" ) ) ||
        ( 0 == _stricmp( szBool, "fTrue" ) ) ||
        ( 0 == _stricmp( szBool, "1" ) )
        )
    {
        *((BOOL*)(*ppvValue)) = fTrue;
        return fTrue;
    }
    else if ( ( 0 == _stricmp( szBool, "false" ) ) ||
        ( 0 == _stricmp( szBool, "fFalse" ) ) ||
        ( 0 == _stricmp( szBool, "0" ) )
        )
    {
        *((BOOL*)(*ppvValue)) = fFalse;
        return fTrue;
    }

    ITQUFree( *ppvValue );
    ITQUPrintf( "Could not read BOOL value from arg: %s\n", szBool );
    return fFalse;
}

INT BoolExprEval( void * pVal1, void * pVal2 )
{
    return *((BOOL*)pVal1) - *((BOOL*)pVal2);
}

void BoolPrintVal( const void * pVal )
{
    BOOL f = *(ULONG*)pVal;
#ifndef CACHE_QUERY_PRINT_RAW_VALUES    // non-logical values
    if ( f )
    {
        ITQUPrintf( "  fTrue" );
    }
    else
    {
        ITQUPrintf( " fFalse" );
    }
#else
    ITQUPrintf( " %d", f );
#endif
}

BOOL ShortReadVal( const char * szShort, void ** ppvValue )
{
    ULONG ul;
    if ( !FITQUUlFromSz( szShort, &ul, 10 ) ||
        ( ((INT)ul) != ((SHORT)ul) ) )
    {
        ITQUPrintf( "Could not read SHORT value from arg: %s.\n", szShort );
        return fFalse;
    }
    *ppvValue = PvITQUAlloc( sizeof(SHORT) );
    if ( *ppvValue == NULL )
    {
        ITQUPrintf( "Allocation failure.\n" );
        return fFalse;
    }
    *((SHORT*)*ppvValue) = (SHORT)ul;
    return fTrue;
}

INT ShortExprEval( void * pVal1, void * pVal2 )
{
    return (*(SHORT*)pVal1) - (*(SHORT*)pVal2);
}

void ShortPrintVal( const void * pVal )
{
    ITQUPrintf(" %05hd", (*(SHORT*)pVal) );
}

BOOL UshortReadVal( const char * szUshort, void ** ppvValue )
{
    ULONG ul;
    if ( !FITQUUlFromSz( szUshort, &ul, 10 ) ||
        ( ((INT)ul) != ((USHORT)ul) ) )
    {
        ITQUPrintf( "Could not read USHORT value from arg: %s.\n", szUshort );
        return fFalse;
    }
    *ppvValue = PvITQUAlloc( sizeof(USHORT) );
    if ( *ppvValue == NULL )
    {
        ITQUPrintf( "Allocation failure.\n" );
        return fFalse;
    }
    *((USHORT*)*ppvValue) = (USHORT)ul;
    return fTrue;
}

INT UshortExprEval( void * pVal1, void * pVal2 )
{
    return (*(USHORT*)pVal1) - (*(USHORT*)pVal2);
}

void UshortPrintVal( const void * pVal )
{
    ITQUPrintf(" %05hu", (*(USHORT*)pVal) );
}

BOOL UlongReadVal( const char * szUlong, void ** ppvValue )
{
    ULONG ul;
    if ( !FITQUUlFromSz( szUlong, &ul, 10 ) )
    {
        ITQUPrintf( "Could not read ULONG value from arg: %s\n", szUlong );
        return fFalse;
    }
    *ppvValue = PvITQUAlloc( sizeof(ULONG) );
    if ( *ppvValue == NULL )
    {
        ITQUPrintf( "Allocation failure.\n" );
        return fFalse;
    }
    *((ULONG*)*ppvValue) = ul;

    return fTrue;
}

INT UlongExprEval( void * pVal1, void * pVal2 )
{
    return (*(ULONG*)pVal1) - (*(ULONG*)pVal2);
}

void UlongPrintVal( const void * pVal )
{
    ITQUPrintf(" %08d", *(ULONG*)pVal );
}


BOOL DwordReadVal( const char * szDword, void ** ppvValue )
{
    DWORD ul;
    // Amazingly, and disturbingly this consumes "0x1" properly. Also processes "-1".
    if ( !FITQUUlFromSz( szDword, &ul, 16 ) )
    {
        ITQUPrintf( "Could not read DWORD value from arg: %s\n", szDword );
        return fFalse;
    }
    *ppvValue = PvITQUAlloc( sizeof(DWORD) );
    if ( *ppvValue == NULL )
    {
        ITQUPrintf( "Allocation failure.\n" );
        return fFalse;
    }
    *((DWORD*)*ppvValue) = ul;

    return fTrue;
}

INT DwordExprEval( void * pVal1, void * pVal2 )
{
    return (*(DWORD*)pVal1) - (*(DWORD*)pVal2);
}

void DwordPrintVal( const void * pVal )
{
    ITQUPrintf(" 0x%08x", *(DWORD*)pVal );
}

BOOL QwordReadVal( const char * szQword, void ** ppvValue )
{
    QWORD qw;
    // Amazingly, and disturbingly this consumes "0x1" properly. Also processes "-1".
    if ( 0 == _stricmp( szQword, "NULL" ) ||
        0 == _stricmp( szQword, "0x0" ) ||
        0 == _stricmp( szQword, "0" ) )
    {
        *ppvValue = PvITQUAlloc( sizeof(QWORD) );
        *((void**)*ppvValue) = 0x0;
        return fTrue;
    }

    QWORD qwT;
    const CHAR * rgsz[1] = { szQword };
    if ( !FITQUAddressFromSz( 1, rgsz, 0, &qwT ) )
    {
        ITQUPrintf( "Could not read QWORD value from arg: %s\n", szQword );
        return fFalse;
    }
    qw = (QWORD)qwT;

    *ppvValue = PvITQUAlloc( sizeof(QWORD) );
    if ( *ppvValue == NULL )
    {
        ITQUPrintf( "Allocation failure.\n" );
        return fFalse;
    }
    *((QWORD*)*ppvValue) = qw;

    return fTrue;
}

INT QwordExprEval( void * pVal1, void * pVal2 )
{
    __int64 i = ( (*(__int64*)pVal1) - (*(__int64*)pVal2) );
    if ( i < 0 )
    {
        return -1;
    }
    else if ( i > 0 )
    {
        return 1;
    }
    return 0;
}

void QwordPrintVal( const void * pVal )
{
    ITQUPrintf(" 0x%016I64x", *(QWORD*)pVal );
}

BOOL PtrReadVal( const char * szPtr, void ** ppvValue )
{
    void* pv;
    //  Short circuit "NULL" processing 
    if ( 0 == _stricmp( szPtr, "NULL" ) ||
        0 == _stricmp( szPtr, "0x0" ) ||
        0 == _stricmp( szPtr, "0" ) )
    {
        *ppvValue = PvITQUAlloc( sizeof(UNSIGNED_PTR) );
        *((void**)*ppvValue) = 0x0;
        return fTrue;
    }
    QWORD qwT = 0;
    if ( !FITQUAddressFromSz( 1, &szPtr, 0, &qwT ) )
    {
        return fFalse;
    }
    pv = (void*)qwT;
    *ppvValue = PvITQUAlloc( sizeof(UNSIGNED_PTR) );
    *((void**)*ppvValue) = pv;
    return fTrue;
}

INT PtrExprEval( void * pVal1, void * pVal2 )
{
    __int64 i = ( (*(UNSIGNED_PTR*)pVal1) - (*(UNSIGNED_PTR*)pVal2) );
    if ( i < 0 )
    {
        return -1;
    }
    else if ( i > 0 )
    {
        return 1;
    }
    return 0;
}

void PtrPrintVal( const void * pVal )
{
#ifdef _WIN64
    QwordPrintVal( pVal );
#else
    DwordPrintVal( pVal );
#endif
}


// -------------------------------------------------------------
//
//  Member Descriptor
//

class CMemberDescriptor {

public:
    char *                 szMember;

    //
    //  Callback/Data for cracking this member from the structure.
    //
    PfnErrMemberFromEntry  m_pfnErrMemberFromEntry;
    size_t                 m_cbOffset;
    size_t                 m_cbSize;

    DWORD                  m_HistoInfo;    // combo of CMemberDescriptorHistoInfo and low data.

    PfnExprEval            m_pfnExprEval;
    void *                 m_pvReservedWasStatsEval;
    PfnFReadVal            m_pfnFReadVal;
    PfnPrintVal            m_pfnPrintVal;

    ERR ErrMemberFromEntry( QwEntryAddr qwEntryAddr, PcvEntry pcvEntry, ULONGLONG * pullValue ) const
    {
        ITQUAssert( m_pfnErrMemberFromEntry );
        return m_pfnErrMemberFromEntry( m_cbOffset, m_cbSize, qwEntryAddr, pcvEntry, pullValue );
    }

};

class IQPredicateSubClause  //  psc
{

    enum eCompOp {  // rename CompareOperMemberPredicate?  compAll, compGT, compLT ;-
        eCompOpNone = 0,
        eCompOpAll = 1,
        eCompOpMin,
        eCompOpMax,
        eCompOpGT,
        eCompOpLT,
        eCompOpGE,
        eCompOpLE,
        eCompOpEQ,
        eCompOpNE,
        eCompOpAnd,
    };

public:

    const CMemberDescriptor *    pmd;
    enum eCompOp                 m_eCompOp;
    void *                       pvValue;

    ERR ErrSetCompOp( __in_z const CHAR * szComparisonArg )
    {
        if ( 0 == _stricmp( szComparisonArg, "*" ) )
        {
            m_eCompOp = eCompOpAll;
        }
        else if ( 0 == _stricmp( szComparisonArg, ">" ) )
        {
            m_eCompOp = eCompOpGT;
        }
        else if ( 0 == _stricmp( szComparisonArg, "<" ) )
        {
            m_eCompOp = eCompOpLT;
        }
        else if ( 0 == _stricmp( szComparisonArg, ">=" ) )
        {
            m_eCompOp = eCompOpGE;
        }
        else if ( 0 == _stricmp( szComparisonArg, "<=" ) )
        {
            m_eCompOp = eCompOpLE;
        }
        else if ( 0 == _stricmp( szComparisonArg, "==" ) )
        {
            m_eCompOp = eCompOpEQ;
        }
        else if ( 0 == _stricmp( szComparisonArg, "!=" ) )
        {
            m_eCompOp = eCompOpNE;
        }
        else if ( 0 == _stricmp( szComparisonArg, "&" ) )
        {
            m_eCompOp = eCompOpAnd;
        }
        else
        {
            return IQERR::errInvalidParameter;
        }

        return IQERR::errSuccess;
    }

    BOOL FEvalQueryMember( QwEntryAddr qwEntryAddr, PcvEntry pcvEntry, void * pVal2 )
    {
        BOOL fRet = fFalse;
        INT iEval = 0;

        ULONGLONG ullValue1;

        if ( eCompOpAll == m_eCompOp )
        {
            // weird, I tried "cachequery * print:ifmp,pgno" and this line didn't fire... what evaluates "*"?
            //  something else, that's clear. Should've come down here, that would've been better, but its
            //  clearly shortcut somewhere
            fRet = fTrue;
            return fRet;
        }

        ITQUAssert( pmd );

        const ERR err = pmd->ErrMemberFromEntry( qwEntryAddr, pcvEntry, &ullValue1 );
        if ( IQERR::errSuccess != err )
        {
            ITQUAddWarning( "WARNING: Failure to retrieve entry member or sub-member, failing query eval (results may be wrong)\n" );
            return fFalse;
        }

        if ( m_eCompOp == eCompOpAnd )
        {
            //  Bit-Wise AND evaluated separately ...
            QWORD qwMask = *(QWORD*)pVal2;
            return ( qwMask & ullValue1 ) ? fTrue : fFalse;
        }

        ITQUSetExprCxt( pcvEntry );
        
        iEval = pmd->m_pfnExprEval( &ullValue1, pVal2 );

        switch ( m_eCompOp )
        {
            case eCompOpGT:
                fRet = iEval > 0;
                break;
            case eCompOpLT:
                fRet = iEval < 0;
                break;
            case eCompOpGE:
                fRet = iEval >= 0;
                break;
            case eCompOpLE:
                fRet = iEval <= 0;
                break;
            case eCompOpEQ:
                fRet = iEval == 0;
                break;
            case eCompOpNE:
                fRet = iEval != 0;
                break;
            default:
                ITQUAssertSz( fFalse, "Unknown equality argument" );
        }

        return fRet;
    }

};


// -------------------------------------------------------------
//
//  Entry Descriptor Interface
//

class IEntryDescriptor // ied
    {
public:
    // Not needed.  Should we define anyways?
    //virtual IEntryDescriptor() = 0;
    //virtual ~IEntryDescriptor() = 0;

    //?? create a name for the entry type?
    //virtual const WCHAR * WszEntryName() const = 0;

    virtual ULONG CbEntry( PcvEntry pcvEntry ) const = 0;

    virtual const CHAR * SzDefaultPrintList() const = 0;
    virtual void DumpEntry( QwEntryAddr qwAddr, PcvEntry pcvEntry ) const = 0;

    virtual const CMemberDescriptor * PmdMatch( const CHAR * szTargetEntry, const CHAR chDelim = '\0' ) const = 0;

    #define SzIQFirstMember ( (CHAR*)0x1 )
    #define SzIQLastMember  ( (CHAR*)0x2 )

    // We can use PmdMatch( SzFirstMember ) and CMembers() to maintain compat with old code.
    virtual ULONG CMembers() const = 0;

};  //  IEntryDescriptor


const CMemberDescriptor * PmdMemberDescriptorLookupHelper(
    const CMemberDescriptor * const  prgqt,
    const ULONG              cqt,
    const char *             szTarget,
    const CHAR               chDelim )
{
    if ( szTarget == SzIQFirstMember )
    {
        return &( prgqt[ 0 ] );
    }
    if ( szTarget == SzIQLastMember )
    {
        return &( prgqt[ cqt - 1 ] );
    }

    ULONG cchMember = strchr( szTarget, chDelim ) ?
                        ( strchr( szTarget, chDelim ) - szTarget ) :
                        ( strlen( szTarget ) );

    ULONG iMember;
    for( iMember = 0; iMember < cqt; iMember++ )
    {
        if ( 0 == _strnicmp( prgqt[iMember].szMember, szTarget, cchMember ) &&
                cchMember == strlen( prgqt[iMember].szMember ) )
        {
            return &prgqt[iMember];
        }
    }

    return NULL;
}


// -------------------------------------------------------------
//
//  Member Descriptor Generic Accessor
//

//  Normal field types ... can be done by size...
#define QEF(  type, member, histoinfo, expreval, statseval, readval, printval ) \
    { #member, ErrMemberFromStructIbCb, (size_t)&(((type*)NULL)->member), sizeof((((type*)NULL)->member)), histoinfo, expreval, statseval, readval, printval },


ERR ErrMemberFromStructIbCb( size_t ibOffset, size_t cbSize, QwEntryAddr qwEntryAddr, PcvEntry pcvEntry, ULONGLONG * pullValue )
{
    ULONGLONG ullValue1 = 0x0;

    const BYTE * pb = (const BYTE*)pcvEntry;

    ITQUAssert( pullValue );
    *pullValue = 0xBAADF00D;

    ITQUAssert( ibOffset != -1 );

    switch( cbSize )
    {
        case 1:
            ullValue1 = (ULONGLONG) ( *( (pb + ibOffset) ) );
            break;
        case 2:
            ullValue1 = (ULONGLONG) ( *( (USHORT*) (pb + ibOffset) ) );
            break;
        case 4:
            ullValue1 = (ULONGLONG) ( *( (ULONG*) (pb + ibOffset) ) );
            break;
        case 8:
            ullValue1 = (ULONGLONG) ( *( (ULONGLONG*) (pb + ibOffset) ) );
            break;
        default:
            ITQUAssertSz( fFalse, "Huh, unrecognized cbSize / iCtx2 - Not really sure what this code path will really do." );
            return IQERR::errInternalError;
    }

    // FITQUDebugMode is too verbose, and it was: g_eDebugMode >= eDebugModeMicroOps - but didn't want to thunk that
    // out IterationQuery was extricated and generalized from edbg.cxx / CacheQuery.
    if( fFalse )
    {
        ITQUPrintf( " ErrMemberFromStructIbCb()-> val-lu: %lu\n", ullValue1 );
        ITQUPrintf( " ErrMemberFromStructIbCb()-> val-I64u: %I64u\n", ullValue1 );
    }

    *pullValue = ullValue1;

    return IQERR::errSuccess;
}


// -------------------------------------------------------------
//
//  Query Actions
//

typedef struct
{
    ULONG       cPrintedMembers;
    CMemberDescriptor * rgpmdPrintedMembers[1];
} PRINT_ACTION_CONTEXT;

ERR PrintAction( const IEntryDescriptor * const pied, QwEntryAddr qwEntryAddr, PcvEntry pcvEntry, void * pContext )
{
    const PRINT_ACTION_CONTEXT * ppac = (PRINT_ACTION_CONTEXT*)pContext;

    ITQUPrintf( " pcvEntry=%p: ", qwEntryAddr );

    for( ULONG iP = 0; iP < ppac->cPrintedMembers; iP++ )
    {
        unsigned __int64 ullValueT;
        const ERR err = ppac->rgpmdPrintedMembers[iP]->ErrMemberFromEntry( qwEntryAddr, pcvEntry, &ullValueT );
        if ( IQERR::errSuccess == err )
        {
            ppac->rgpmdPrintedMembers[iP]->m_pfnPrintVal( (void *) &ullValueT );
        }
        else
        {
            ITQUPrintf( " #error#" );
        }
        ITQUPrintf( "," );
    }
    ITQUPrintf("\n" );

    return IQERR::errSuccess;
}

ERR CountAction( const IEntryDescriptor * const pied, QwEntryAddr qwEntryAddr, PcvEntry pcvEntry, void * pContext )
{
    *(__int64*)pContext = (*(__int64*)pContext) + 1;
    return IQERR::errSuccess;
}

ERR CountFinalAction( const IEntryDescriptor * const pied, void * pContext )
{
    __int64 cAccum = * (__int64*) pContext;
    ITQUPrintf(" cCount = %I64d\n", cAccum );
    return IQERR::errSuccess;
}

typedef struct
{
    const CMemberDescriptor *   pmd;
    __int64 *                   pcAccum;
    __int64                     cAccumImplicit; // do not use directly, see pcAccum usage
} ITQU_ACCUM_CONTEXT;

ERR AccumAction( const IEntryDescriptor * const pied, QwEntryAddr qwEntryAddr, PcvEntry pcvEntry, void * pContext )
{
    ITQU_ACCUM_CONTEXT * pC = (ITQU_ACCUM_CONTEXT*)pContext;

    ULONGLONG ullValue1;
    const ERR err = pC->pmd->ErrMemberFromEntry( qwEntryAddr, pcvEntry, &ullValue1 );
    if ( IQERR::errSuccess != err )
    {
        ITQUAddWarning( "WARNING: Could not retrieve member during AccumAction.\n" );
        return IQERR::errInvalidParameter;
    }
    
    *(pC->pcAccum) = *(pC->pcAccum) + ullValue1;
    return IQERR::errSuccess;
}

ERR AccumFinalAction( const IEntryDescriptor * const pied, void * pContext )
{
    ITQU_ACCUM_CONTEXT * pC = (ITQU_ACCUM_CONTEXT*)pContext;

    ITQUPrintf(" cCount = %I64d\n", *(pC->pcAccum) );
    return IQERR::errSuccess;
}

ERR DumpAction( const IEntryDescriptor * const pied, QwEntryAddr qwEntryAddr, PcvEntry pcvEntry, void * pContext )
{
    pied->DumpEntry( qwEntryAddr, pcvEntry );
    return IQERR::errSuccess;
}

typedef struct
{
    const CMemberDescriptor *   pmd;
    BOOL                        fMin;   // otherwise max
    ULONGLONG                   ullTarget;
    BOOL                        fDup;
    QwEntryAddr                 qwTargetAddr;
    PvEntry                     pvTargetEntry;
} ITQU_MINMAX_TARGET_CONTEXT;

ERR MinMaxTargetAction( const IEntryDescriptor * const pied, QwEntryAddr qwEntryAddr, PcvEntry pcvEntry, void * pContext )
{
    ITQU_MINMAX_TARGET_CONTEXT * pC = (ITQU_MINMAX_TARGET_CONTEXT *)pContext;

    ULONGLONG ullValue1;
    const ERR err = pC->pmd->ErrMemberFromEntry( qwEntryAddr, pcvEntry, &ullValue1 );
    if ( IQERR::errSuccess != err )
    {
        ITQUAddWarning( "WARNING: Could not retrieve member during MinMaxTargetAction.\n" );
        return IQERR::errInternalError;
    }

    INT iEval = pC->pmd->m_pfnExprEval( &ullValue1, &(pC->ullTarget) );

    const bool fMatches = ( ( pC->fMin && iEval < 0 ) ||
                            ( !pC->fMin && iEval > 0 ) ||
                            ( iEval == 0 && pC->qwTargetAddr == NULL ) );

    if ( fMatches )
    {
        pC->ullTarget = ullValue1;
        pC->fDup = fFalse;
        if ( pC->pvTargetEntry == NULL )
        {
            pC->pvTargetEntry = PvITQUAlloc( pied->CbEntry( pcvEntry ) );
            if ( pC->pvTargetEntry == NULL )
            {
                ITQUAddWarning( "Allocation failure in MinMaxTargetAction" );
                return IQERR::errInternalError;
            }
        }
        memcpy( pC->pvTargetEntry, pcvEntry, pied->CbEntry( pcvEntry ) );
        pC->qwTargetAddr = qwEntryAddr;
    }
    else if ( iEval == 0 )
    {
        pC->fDup = fTrue;
    }
    
    return IQERR::errSuccess;
}

ERR MinMaxTargetFinalAction( const IEntryDescriptor * const pied, void * pContext )
{
    ITQU_MINMAX_TARGET_CONTEXT * pC = (ITQU_MINMAX_TARGET_CONTEXT *)pContext;

    if ( pC->qwTargetAddr == NULL )
    {
        ITQUPrintf( " No best match found.\n" );
    }
    else
    {
        ITQUPrintf( " TargetAddr = 0x%p, %s =", pC->qwTargetAddr, pC->pmd->szMember );
        unsigned __int64 ullValueT;
        const ERR err = pC->pmd->ErrMemberFromEntry( pC->qwTargetAddr, pC->pvTargetEntry, &ullValueT );
        if ( IQERR::errSuccess != err )
        {
            ITQUPrintf( "ERROR: Trouble getting final match member!\n" );
            return IQERR::errInternalError;
        }

        pC->pmd->m_pfnPrintVal( (void *) &ullValueT );
        if ( pC->fDup )
        {
            ITQUPrintf( "   (Note: Other matches of same value exist)\n" );
        }
        else
        {
            ITQUPrintf( "   (Note: Unique match)\n" );
        }

    }
    return IQERR::errSuccess;
}


typedef struct
{
    const CMemberDescriptor *   pmd;
    CStats *                    phisto;
    DWORD                       cbucketZeroSkipThreshold;

    //  Just to ensure alignment for histogram allocated after this structure.
    void *                      pvAlignment;
    // Why did I need to go from "[]" to "[1]" when i compiled it in iterqueryunit.exe and not in ese.dll?
    BYTE                        rgbHistoImplicit[1];
} ITQU_HISTO_CONTEXT;

ERR HistoAction( const IEntryDescriptor * const pied, QwEntryAddr qwEntryAddr, PcvEntry pcvEntry, void * pContext )
{
    ITQU_HISTO_CONTEXT * pCtx = (ITQU_HISTO_CONTEXT*)pContext;

    QWORD qwValue;
    const ERR err = pCtx->pmd->ErrMemberFromEntry( qwEntryAddr, pcvEntry, &qwValue );

    if ( IQERR::errSuccess != err )
    {
        ITQUAddWarning( "WARNING: Could not retrieve member during HistoAction.\n" );
        return IQERR::errInternalError;
    }

    const CStats::ERR errStats = pCtx->phisto->ErrAddSample( qwValue );

    if ( CStats::ERR::errOutOfMemory == errStats )
    {
        ITQUAddWarning( "WARNING: Out of memory trying to add stat value during HistoAction.\n" );
        return IQERR::errOutOfMemory;
    }
    else if ( errStats != CStats::ERR::errSuccess )
    {
        ITQUAddWarning( "WARNING: Could not add stat value during HistoAction.\n" );
        return IQERR::errInternalError;
    }

    return IQERR::errSuccess;
}

inline void IQPrintStats( CStats * pCS, PfnPrintVal pfnPrintValue, const SAMPLE sampleBucketSize, const DWORD cbTypeSize, const DWORD cbucketZeroSkipThreshold, const bool fSummary )
{
    CStats::ERR csErr;

    SAMPLE sampleMax;
    switch( cbTypeSize )
    {
        case 1:
            sampleMax = 0xff;
            break;
        case 2:
            sampleMax = 0xffff;
            break;
        case 4:
            sampleMax = 0xffffffff;
            break;

        default:
            __fallthrough;
        case 8:
            sampleMax = 0xffffffffffffffff;
            break;
    }

    if ( pCS == NULL )
    {
        ITQUPrintf( "   NULL pointer to object of class CStats!\n" );
        return;
    }
    
    if ( pCS->C() == 0 )
    {
        ITQUPrintf( "   No samples!\n" );
        return;
    }

    if ( CStats::ERR::errSuccess != ( csErr = pCS->ErrReset() ) )
    {
        ITQUPrintf( "   ERROR: %d reseting stat class.\n", csErr );
        return;
    }

    SAMPLE sample;
    SAMPLE sampleLast = 0;
    ULONG cHitMax = 0;
    while( CStats::ERR::errSuccess == ( csErr = pCS->ErrGetSampleValues( &sample ) ) )
    {
        CHITS hits;
        //  We collect samples at higher than sampleMax, b/c if it's a partial histo we won't get a good read of everything
        //  through the end b/c of the bucketing.  After we collect we truncate sample down to sampleMax, and assert the 
        //  next go turns up empty with the cHitMax (otherwise we've bailed early and we're mis-reporting things).
        csErr = pCS->ErrGetSampleHits( sample, &hits );
        if ( csErr != CStats::ERR::errSuccess )
        {
            break;
        }
        if ( sample >= sampleMax )
        {
            sample = sampleMax;
            cHitMax++;
        }
        ITQUAssertSz( cHitMax <= 1, "Should only be able to hit the max of the type once, something is wrong." );
        if ( cHitMax > 1 )  // defense in depth, we should not spin forever
        {
            break;
        }

        if ( sampleBucketSize )
        {
            //  if it's a partial histo (i.e. sampleBucketSize != 0) we should have buckets like this ...
            ITQUAssert( ( ( sample - sampleLast ) % sampleBucketSize ) == 0 || cHitMax );
 
            SAMPLE sampleSkip = sample - sampleLast;
            ITQUAssert( sampleSkip != 0 || sampleLast == 0 );

            if ( sampleSkip == sampleBucketSize )
            {
                //  this is the normal or hopeful case, some hits in every bucket ...
            }
            else if ( ( ( sampleSkip ) / sampleBucketSize ) < cbucketZeroSkipThreshold )
            {
                while( sampleLast < ( sample - sampleBucketSize ) )
                {
                    sampleLast += sampleBucketSize;
                    if ( sampleLast >= sampleMax )
                    {
                        sampleLast = sampleMax;
                    }
                    ITQUPrintf( "         " );
                    pfnPrintValue( &sampleLast );
                    ITQUPrintf( " = %I64u\n", (CHITS)0 );
                }
            }
            else
            {
                Assert( sampleSkip );
                if ( sampleSkip )
                {
                    ITQUAssert( sampleSkip > sampleBucketSize )
                    ITQUPrintf( "Warning: " );
                    pfnPrintValue( &sampleLast );
                    ITQUPrintf( " = %I64u (Begin Skip)\n", (CHITS)0 );
                    ITQUPrintf( "Warning: " );
                    sampleLast = sample - sampleBucketSize;
                    pfnPrintValue( &sampleLast );
                    ITQUPrintf( " = %I64u (End Skip, delta = %I64u (buckets = %I64u))\n", (CHITS)0, sampleSkip, ( sampleSkip ) / sampleBucketSize );
//                  ITQUPrintf( "Warning:    Skipping giant hole in histogram: %I64d buckets, %I64d value long\n", sampleLast - sample, ( sampleLast - sample ) / sampleBucketSize );
                }
            }
        }

        ITQUPrintf( "         " );
        pfnPrintValue( &sample );
        ITQUPrintf( " = %I64u\n", hits );

        sampleLast = sample;
    }

    if ( csErr != CStats::ERR::wrnOutOfSamples )
    {
        ITQUPrintf( "   ERROR: %d enumerating samples.\n", csErr );
    }

    if ( fSummary )
    {
        ITQUPrintf( "   C,Min,Ave(DblAve),Max,Range,Total:      %I64u,%I64u,%I64u(%f),%I64u,%I64u,%I64u\n",
                pCS->C(), pCS->Min(), pCS->Ave(), pCS->DblAve(), pCS->Max(), pCS->Max() - pCS->Min() , pCS->Total() );
    }

    (void)pCS->ErrReset();
}

ERR HistoFinalAction( const IEntryDescriptor * const pied, void * pContext )
{
    ITQU_HISTO_CONTEXT * pCtx = (ITQU_HISTO_CONTEXT*)pContext;

    IQPrintStats( pCtx->phisto,
                    pCtx->pmd->m_pfnPrintVal,
                    pCtx->pmd->m_HistoInfo & mskHistoSection,
                    pCtx->pmd->m_cbSize,
                    pCtx->cbucketZeroSkipThreshold,
                    true );

    return IQERR::errSuccess;
}

// -------------------------------------------------------------
//
//  Query Predicate Evaluator
//


class CIterQuery  //  piq-> / "pick"
{

private:

    //  Descriptor of the Entries we will be processing.
    //
    IEntryDescriptor *      m_pied;

    //  The Query's structure and Predicate Sub-Clauses.
    //

    const static int        s_cPscMax = 20;
    ULONG                   m_cPsc; // # of predicate sub-clauses in use
    IQPredicateSubClause    m_rgpsc[ s_cPscMax ];
    BOOL                    m_rgPscOrClause[ s_cPscMax ];

    //  The Action Framework.
    //

    PfnQueryVisitAction     m_pfnAction;
    PfnQueryFinalAction     m_pfnFinalAction;
    void *                  m_pActionContext;

public:

    //  .ctor

    CIterQuery( IEntryDescriptor * pied )
    {
        memset( this, 0, sizeof( *this ) );
        m_pied = pied;
    }

    //  Entry Descriptor accessor

    IEntryDescriptor * Pied() const { return m_pied; }

    //  Query Setup

    ULONG CAddQuerySubClause( const char * prgArg [], BOOL fOrClause )
    {

        if ( FITQUDebugMode )
        {
            //  Somewhat unsafe, b/c we check these conditions below, but generally
            //  we expect at least prgArg[0] and prgArg[1] to be non-null.  And note
            //  prgArg[2] may not even relate to this evaluation if prgArg[1] is a
            //  unary operator
            ITQUPrintf("CAddQuerySubClause() - Adding clause: fAndOrOr=%d, Args %s, %s, %s\n",
                    fOrClause, prgArg[0],
                    prgArg[1] ? prgArg[1] : "<null>",
                    ( prgArg[1] && prgArg[2] ) ? prgArg[2] : "<null>" );
        }

        //  this is the closest thing to a constructor, so putting some basic compile
        //  type asserts here...
        C_ASSERT( sizeof(m_rgpsc)/sizeof(m_rgpsc[0]) == sizeof(m_rgPscOrClause)/sizeof(m_rgPscOrClause[0]) );

        //
        //  Use the next Query Predicate Sub-Clause in the array ...
        //
        if ( m_cPsc >= _countof(m_rgpsc) )
        {
            ITQUPrintf("CAddQuerySubClause() - Adding too many query clauses, only hard compiled to handle %d clauses\n", _countof(m_rgpsc) );
            return 0;
        }
        IQPredicateSubClause * ppsc = &(m_rgpsc[m_cPsc]);
        m_rgPscOrClause[m_cPsc] = fOrClause;

        //
        //  Evaluate the element the caller wants to query against.
        //

        //  Bail early if this is the match all query ...
        if ( 0 == _stricmp( prgArg[0], "*" ) )
        {
            ppsc->pmd = NULL;
            ppsc->ErrSetCompOp( "*" );
            ppsc->pvValue = NULL;
            return 1;   // only one arg ...
        }

        //  Select a query type ...
        ppsc->pmd = m_pied->PmdMatch( prgArg[0] );
        if ( ppsc->pmd  == NULL )
        {
            ITQUPrintf( "Must specify a entry member or sub-member for the 2nd arg, %s doesn't seem to be.\n", prgArg[0] );
            goto HandleError;
        }

        if ( prgArg[1] == NULL )
        {
            ITQUPrintf( "Must specify a equality operator { ==, >=, >, <, etc }.\n" );
            goto HandleError;
        }

        ERR err = ppsc->ErrSetCompOp( prgArg[1] );
        ITQUAssert( ppsc->pmd && ppsc->pmd->m_pfnFReadVal );
        if ( err )
        {
            ITQUPrintf( "Must specify a equality operator that we recognize { ==, >=, >, <, etc }.\n" );
            goto HandleError;
        }

        if ( prgArg[2] == NULL ||
            !ppsc->pmd->m_pfnFReadVal( prgArg[2], &(ppsc->pvValue) ) )
        {
            ITQUPrintf( "Must specify an argument to match the operator.\n" );
            goto HandleError;
        }
        ITQUAssert( ppsc->pvValue );
        m_cPsc++;
        return 3;

    HandleError:
    
        ITQUPrintf( "Could not parse query arg!\n" );
        return 0;
    }

    //  Action Setup

    void SetCallback( PfnQueryVisitAction pfnAction, PfnQueryFinalAction pfnFinalAction, void * pContext )
    {
        m_pfnAction = pfnAction;
        m_pfnFinalAction = pfnFinalAction;
        m_pActionContext = pContext;
    }

    //  Tear Down

    ~CIterQuery()
    {
        for( ULONG ipsc = 0; ipsc < m_cPsc; ipsc++ )
        {
            if ( m_rgpsc[ipsc].pvValue )
            {
                ITQUFree( m_rgpsc[ipsc].pvValue );
            }
        }
    }

    //  Query evaluation functions

    bool FPrints( void )
    {
        return ( m_pfnAction == PrintAction ||
                    m_pfnAction == DumpAction );
    }

    BOOL FEvaluatePredicate( QwEntryAddr qwEntryAddr, PcvEntry pcvEntry )
    {
        BOOL fRet = fTrue;

        //
        //  We only support "and" or "or" clauses for now ...
        //
        for ( ULONG iPsc = 0; iPsc < m_cPsc; iPsc++ )
        {
            if ( m_rgPscOrClause[iPsc] == fTrue )
            {
                //  This means we have an "or" in front of this clause, so we
                //  need to see if we've already evaluated to true, and break.
                //  if we haven't gotten a true at this point, we need to reset 
                //  fRet so we return true if this clause is true ...
                if ( fRet )
                {
                    //  first clause before this || evaluated to true, break
                    //  now, we're done ...
                    //  Note: This does effectively performs partial / short
                    //  circuit evaluation for "or" clauses ...
                    break;
                }
                //  first clause before this || evaulated to false, reset fRet
                //  and give second clause after this || to evaluate to true ...
                fRet = fTrue;
            }
            //  Note: This does affect partial evaluation for "and"
            //  clauses ... so should be useful for things like:
            fRet = fRet && m_rgpsc[iPsc].FEvalQueryMember( qwEntryAddr, pcvEntry, m_rgpsc[iPsc].pvValue );

        }

        return fRet;
    }

    void PerformAction( QwEntryAddr qwAddr, PcvEntry pcvEntry )
    {
        if ( m_pfnAction )
        {
            m_pfnAction( m_pied, qwAddr, pcvEntry, m_pActionContext );
        }
    }

    void FinalAction( void )
    {
        if ( m_pfnFinalAction )
        {
            m_pfnFinalAction( m_pied, m_pActionContext );
        }
    }

    //  Diagnostic helpers

    void PrintQuery( void )
    {
        ITQUPrintf("Printing CIterQuery - %p\n  Query:\n    ", this );
        //
        //  We only support "and" and "or" clauses for now ...
        //
        for ( ULONG iPsc = 0; iPsc < m_cPsc; iPsc++ )
        {
            if ( m_rgPscOrClause[iPsc] )
            {
                ITQUPrintf("||\n    ");
            }
            else
            {
                if ( iPsc )   // first && is implicit
                {
                    ITQUPrintf("&&");
                }
            }

            if ( m_rgpsc[iPsc].pmd && m_rgpsc[iPsc].pmd->szMember && m_rgpsc[iPsc].m_eCompOp && m_rgpsc[iPsc].pvValue )
            {
                ITQUPrintf(" m_rgpsc[%d]{ %s, %d, %p } ", iPsc, m_rgpsc[iPsc].pmd->szMember, m_rgpsc[iPsc].m_eCompOp, m_rgpsc[iPsc].pvValue );
            }
            else
            {
                ITQUPrintf(" -At m_rgpsc[%d], either pmd, or member of is NULL!- ", iPsc);
            }
            
        }

        ITQUPrintf("\n  Action:    ");
        ITQUPrintf(" %p, %p ", m_pfnAction, m_pfnFinalAction );
        if ( m_pfnAction == PrintAction )
        {

            ULONG * piMemberPrint = (ULONG *)m_pActionContext;

            ITQUPrintf( "{ " );

            for( ULONG iP = 0; iP < piMemberPrint[0]; iP++ )
            {
                ITQUPrintf( "[%d]=%d, ", iP, piMemberPrint[1+iP] );
            }

            ITQUPrintf( "}" );
        }
        ITQUPrintf("\n");

    }

};


// -------------------------------------------------------------
//
//  Generic Arg processing helpers
//

void ConsumeArgs( const char ** prgArg, ULONG * pcArg, ULONG cNum )
{
    if ( cNum == 0 )
    {
        ITQUPrintf("Error: Trying to consume zero args.");
        return;
    }
    if ( *pcArg < cNum )
    {
        ITQUPrintf("Error: trying to consume more args than exist.");
        return;
    }

    memmove( &(prgArg[0]), &(prgArg[cNum]), sizeof(*prgArg)*(*pcArg-cNum) );

    *pcArg = *pcArg - cNum;
    prgArg[*pcArg] = NULL;
}

#define DumpArgs( arg, c )                      \
    ITQUPrintf( "arg = %p\n", arg );               \
    for ( ULONG iT = 0; iT < (ULONG)c; iT++ )   \
    {                                           \
        ITQUPrintf( "arg[%d] = %s\n", iT, arg[iT] != NULL ? arg[iT] : "<NULL>" );  \
    }

//  This creates a duplicate array of args so that they can be "trimmed".  Note
//  this doesn't unencumber the original args, we still have references from this
//  array to those string.
//
const char ** LocalArgDup( INT argc, const char * const argv [] )
{
    const char **   prgArg = NULL;

    INT cbArgSize = (INT)sizeof(char*) * ( argc + 1 );
    if ( argc < 0 || cbArgSize/((INT)sizeof(char*)) != ( argc + 1 ) )
    {
        ITQUPrintf("Error: argument count %d is negative or too big.", argc);
        return NULL;
    }
    
    prgArg = (const char**)PvITQUAlloc( (ULONG)cbArgSize );
    if ( prgArg == NULL )
    {
        return NULL;
    }

    for ( INT i = 0; i < argc; i++ )
    {
        if ( FITQUDebugMode )
        {
            ITQUPrintf( "arg[%d] = %s\n", i, argv[i] );
        }
        prgArg[i] = argv[i];
    }
    prgArg[argc] = NULL; // as is convention NULL ptr after last arg.

    return prgArg;
}


// -------------------------------------------------------------
//
//  Predicate Arg to Query Creator
//

IQERR ErrIQConsumePredicateArgs(
        IEntryDescriptor *      pied,
        const char **           prgArg,
        ULONG *                 pcArg,
        _Out_ CIterQuery **     ppiq )
//  ================================================================
{
    IQERR          err = IQERR::errSuccess;
    CIterQuery *   piq = NULL;

    //
    //  Allocate a new empty CIterQuery.
    //
    piq = new CIterQuery( pied );
    if ( piq == NULL )
    {
        ITQUPrintf( "Couldn't allocate %d bytes\n", sizeof(CIterQuery) );
        return IQERR::errOutOfMemory;
    }

    //
    //  Actually start parsing the args.
    //

    if ( FITQUDebugMode )
    {
        ITQUPrintf("ErrIQConsumePredicateArgs() processing these args:\n");
        DumpArgs( prgArg, *pcArg );
    }

    if ( prgArg[0] == NULL )
    {
        ITQUPrintf( "Must specify a entry member or sub-member for the 2nd arg.\n" );
        ITQUCall( IQERR::errInvalidParameter );
    }

    //
    //  Evaluate and add each set of clauses to the CIterQuery ... 
    //
    BOOL fEvaluateMore = fFalse;
    BOOL fOrClause = fFalse;
    do {

        //
        //  Add the 2 or 3 part clause to the CIterQuery, and consume the args...
        //
        ULONG cConsume = piq->CAddQuerySubClause( prgArg, fOrClause );
        if ( 0 == cConsume )
        {
            ITQUPrintf( "Couldn't process query arg.\n");
            ITQUCall( IQERR::errInvalidParameter );
        }
        ConsumeArgs( prgArg, pcArg, cConsume );

        if ( NULL == prgArg[0] )
        {
            fEvaluateMore = fFalse;
        }
        else
        {
            //  Check to see if the next argument is an "and" or "or" clause
            //  and thus we have more query clauses to evaluate.
            fEvaluateMore = _stricmp( prgArg[0], "&&" ) == 0 ||
                            _stricmp( prgArg[0], "||" ) == 0;
            if ( fEvaluateMore )
            {
                fOrClause = _stricmp( prgArg[0], "||" ) == 0;
                ConsumeArgs( prgArg, pcArg, 1 );
            }
        }
    }
    while ( fEvaluateMore );

    if ( FITQUDebugMode )
    {
        piq->PrintQuery();
        ITQUPrintf( "\n" );
    }

    *ppiq = piq;
    
    piq = NULL;     // so not freed
    err = IQERR::errSuccess;

HandleError:

    if ( piq )
    {
        delete piq;
    }

    return err;
}

// -------------------------------------------------------------
//
//  Action Arg Processing
//

IQERR ErrIQAddAction(
    __inout CIterQuery * const  piq,
    const char **               prgArg,
    ULONG *                     pcArg,
    void *                      pvResult = NULL );

IQERR ErrIQAddAction(
    __inout CIterQuery * const  piq,
    const char **               prgArg,
    ULONG *                     pcArg,
    void *                      pvResult )
{
    IQERR err               = IQERR::errSuccess;
    ULONG   cConsumeAction  = 0;

    void *  pContext = NULL;

    if ( FITQUDebugMode )
    {
        ITQUPrintf("ErrIQAddAction() processing these args:\n");
        DumpArgs( prgArg, *pcArg );
    }

    const char * szAction = prgArg[0];

    if ( szAction == NULL )
    {
        ITQUPrintf( "no action defined!\n" );
        ITQUCall( IQERR::errInvalidParameter );
    }


    if ( ( 0 == _stricmp( szAction, "print" ) ) ||
         ( 0 == _strnicmp( szAction, "print:", 6 ) ) )
    {
        const CMemberDescriptor * rgpmdPrintMembers[ 30 ];
        ULONG cPrintedMembers = 0;

        if ( strlen( szAction ) > 6 )
        {
            const char * szCurr = strchr( szAction, ':' );
            // assert(szCurr)

            while ( szCurr && *szCurr != '\0' )
            {
                szCurr++;   // move past the delimiter ...

                if ( cPrintedMembers >= _countof(rgpmdPrintMembers) )
                {
                    ITQUPrintf("Tried to print too many members, increase print members array and recompile: %d\n", cPrintedMembers );
                    break;
                }

                rgpmdPrintMembers[ cPrintedMembers ] = piq->Pied()->PmdMatch( szCurr, ',' );
                if ( rgpmdPrintMembers[ cPrintedMembers ] == NULL )
                {
                    ITQUPrintf("Couldn't find member starting with %s\n", szCurr );
                    break;
                }
                cPrintedMembers++;

                //  Move to the next member we want to print.
                szCurr = strchr( szCurr, ',' );
            }

            if ( szCurr && *szCurr != '\0' )
            {
                ITQUCall( IQERR::errInvalidParameter );
            }

            // Note: This overallocates the array size by one slot / extra CMemberDescriptor * ptr (b/c context
            // array has size of 1 already).
            pContext = PvITQUAlloc( sizeof( PRINT_ACTION_CONTEXT ) + ( cPrintedMembers * sizeof( CMemberDescriptor * ) ) );
            if ( pContext == NULL )
            {
                ITQUPrintf( "Out of memory.\n" );
                ITQUCall( IQERR::errInvalidParameter );
            }
            // success
            ((PRINT_ACTION_CONTEXT*)pContext)->cPrintedMembers = cPrintedMembers;
            memcpy( ((PRINT_ACTION_CONTEXT*)pContext)->rgpmdPrintedMembers, rgpmdPrintMembers, cPrintedMembers * sizeof(rgpmdPrintMembers[0]) );

            piq->SetCallback( PrintAction, NULL, pContext );

            cConsumeAction = 1;
        }
        else
        {
            const char * rgszDefaultPrint [] = { piq->Pied()->SzDefaultPrintList(), NULL };
            ULONG cArgsT = _countof( rgszDefaultPrint );
            ERR errT = ErrIQAddAction( piq, rgszDefaultPrint, &cArgsT );
            Assert( errT == IQERR::errSuccess );
            Assert( 0 == cArgsT );

            cConsumeAction = 1;
        }

    }
    else if ( 0 == _stricmp( szAction, "count" ) )
    {
        const ULONG cbContext = sizeof(__int64);
        pContext = PvITQUAlloc( cbContext );
        if ( pContext == NULL )
        {
            ITQUPrintf( "Out of memory.\n" );
            ITQUCall( IQERR::errInvalidParameter );
        }
        memset( pContext, 0, cbContext );

        __int64 * const pcAccum = (__int64 *)pvResult;

        if ( pcAccum )
        {
            //  External accumulator, no final / "printing" action
            piq->SetCallback( CountAction, NULL, pcAccum );
        }
        else
        {
            piq->SetCallback( CountAction, CountFinalAction, pContext );
        }

        cConsumeAction = 1;
    }
    else if ( ( 0 == _strnicmp( szAction, "accum:", 6 ) ) )
    {
        const ULONG cbContext = sizeof(ITQU_ACCUM_CONTEXT);
        ITQU_ACCUM_CONTEXT * pC;
        pContext = pC = (ITQU_ACCUM_CONTEXT *)PvITQUAlloc( cbContext );
        if ( pContext == NULL )
        {
            ITQUPrintf( "Out of memory.\n" );
            ITQUCall( IQERR::errInvalidParameter );
        }
        memset( pContext, 0, cbContext );

        pC->pmd = piq->Pied()->PmdMatch( &(szAction[6]) );
        if ( pC->pmd == NULL )
        {
            ITQUPrintf( "Must specify a entry member or sub-member for the 2nd arg, %s doesn't seem to be.\n", prgArg[0] );
            ITQUCall( IQERR::errInvalidParameter );
        }

        __int64 * const pcAccum = (__int64 *)pvResult;

        if ( pcAccum )
        {
            pC->pcAccum = pcAccum;
        }
        else
        {
            pC->pcAccum = &(pC->cAccumImplicit);
        }

        //  if there is an external accumulator, there is no final / "printing" action
        piq->SetCallback( AccumAction, pcAccum ? NULL : AccumFinalAction, pContext );

        cConsumeAction = 1;
    }

    else if ( ( 0 == _strnicmp( szAction, "histo:", 6 ) ) )
    {
        const CMemberDescriptor * pmd = piq->Pied()->PmdMatch( &(szAction[6]) );
        if ( pmd == NULL )
        {
            ITQUPrintf( "Must specify a entry member or sub-member for the 2nd arg, %s doesn't seem to be.\n", prgArg[0] );
            ITQUCall( IQERR::errInvalidParameter );
        }

        ULONG cSection = 0;
        ULONG cZeroBucketSkipThreshold = 0;
        
        ULONG cbContext = sizeof(ITQU_HISTO_CONTEXT);
        switch( pmd->m_HistoInfo & mskHistoType )
        {
            case ePerfectHisto:
                cbContext += sizeof(CPerfectHistogramStats);
                break;
            case ePartialHisto:
                cSection = mskHistoSection & pmd->m_HistoInfo;
                cbContext += sizeof(CLinearHistogramStats);
                break;
            default:
                ITQUPrintf( "This particular property, %s doesn't seem to allow histogram support.\n", prgArg[0] );
                ITQUCall( IQERR::errInvalidParameter );
                break;
        }

        if ( mskHistoOptions & pmd->m_HistoInfo )
        {
            if ( eAttemptContinousPrint & pmd->m_HistoInfo )
            {
                //  For now, 100 empty buckets seems like a good limitation, but someday
                //  we may want this to be independently tunable.
                cZeroBucketSkipThreshold = 100;
            }
            else
            {
                ITQUPrintf( "This particular property, %s has an unknown histogram option.\n", prgArg[0] );
                ITQUCall( IQERR::errInvalidParameter );
            }
        }

        ITQU_HISTO_CONTEXT * pCtx;
        pContext = pCtx = (ITQU_HISTO_CONTEXT *)PvITQUAlloc( cbContext );
        if ( pContext == NULL )
        {
            ITQUPrintf( "Out of memory.\n" );
            ITQUCall( IQERR::errInvalidParameter );
        }
        memset( pContext, 0, cbContext );

        switch( pmd->m_HistoInfo & mskHistoType )
        {
            case ePerfectHisto:
            {
                CPerfectHistogramStats * phistoPerfect = (CPerfectHistogramStats*)pCtx->rgbHistoImplicit;
                new( phistoPerfect ) CPerfectHistogramStats();
                pCtx->phisto = phistoPerfect;
            }
                break;
            case ePartialHisto:
            {
                CLinearHistogramStats * phistoLinear = (CLinearHistogramStats*)pCtx->rgbHistoImplicit;
                ITQUAssert( cSection );
                new( phistoLinear ) CLinearHistogramStats( cSection );
                pCtx->phisto = phistoLinear;
                pCtx->cbucketZeroSkipThreshold = cZeroBucketSkipThreshold;
            }
                break;
            default:
                ITQUAssertSz( fFalse, "Need to initialized a type of histo we shouldn't have allowed through." );
                ITQUCall( IQERR::errInvalidParameter );
                break;
        }

        pCtx->pmd = pmd;

        if ( pvResult )
        {
            pCtx->phisto = ((CStats*)pvResult);
        }
        else
        {
            ITQUAssert( pCtx->phisto );
        }

        //  if there is an external histogram, there is no final / "printing" action
        piq->SetCallback( HistoAction, pvResult ? NULL : HistoFinalAction, pContext );

        cConsumeAction = 1;
    }
    else if ( ( 0 == _strnicmp( szAction, "min:", 4 ) ) ||
                ( 0 == _strnicmp( szAction, "max:", 4 ) ) )
    {
        const ULONG cbContext = sizeof(ITQU_MINMAX_TARGET_CONTEXT);
        ITQU_MINMAX_TARGET_CONTEXT * pC;
        pContext = pC = (ITQU_MINMAX_TARGET_CONTEXT *) PvITQUAlloc( cbContext );
        if ( pContext == NULL )
        {
            ITQUPrintf( "Out of memory.\n" );
            ITQUCall( IQERR::errInvalidParameter );
        }
        memset( pContext, 0, cbContext );
        if ( ( 0 == _strnicmp( szAction, "min:", 4 ) ) ||
            ( 0 == _strnicmp( szAction, "max:", 4 ) ) )
        {

            pC->fMin = 0 == _strnicmp( szAction, "min:", 4 );

            pC->pmd = piq->Pied()->PmdMatch( &(szAction[4]) );
            if ( pC->pmd == NULL )
            {
                ITQUPrintf( "Must specify a entry member or sub-member for the 2nd arg, %s doesn't seem to be.\n", prgArg[0] );
                ITQUCall( IQERR::errInvalidParameter );
            }

            if ( pC->pmd->m_cbSize > 4 )
            {
                ITQUAddWarning( "Min/max actions probably don't work with large types yet.\n" );
            }
            if ( pC->fMin )
            {
                pC->ullTarget = 0x80000000;
            }
            else
            {
                pC->ullTarget = 0;
            }

            piq->SetCallback( MinMaxTargetAction, MinMaxTargetFinalAction, pContext );

            cConsumeAction = 1;
        }
        else
        {
            ITQUCall( IQERR::errInvalidParameter );
        }
    }
    else if ( 0 == _stricmp( szAction, "dump" ) )
    {

        piq->SetCallback( DumpAction, NULL, NULL );

        cConsumeAction = 1;
    }
    else
    {
        ITQUPrintf("Error: Unknown action: %s\n", szAction );
        ITQUCall( IQERR::errInvalidParameter );
    }

HandleError:

    if ( err < IQERR::errSuccess )
    {
        ITQUFree( pContext );
        pContext = NULL;
        ITQUAssert( 0 == cConsumeAction );
    }
    else
    {
        ITQUAssert( cConsumeAction );
        ITQUAssert( pContext );
        ConsumeArgs( prgArg, pcArg, cConsumeAction );
    }

    return err;
}

// -------------------------------------------------------------
//
//  Main Arg Processing Functions (combines Predicate and Action arg processing)
//

ERR ErrIQCreateIterQuery(
        IEntryDescriptor * pied,
        const INT argc,
        const CHAR * const argv[],
        _Out_ CIterQuery ** ppiq )
{
    IQERR            err = IQERR::errSuccess;
    const char **   prgArg = NULL;
    CIterQuery *       piq = NULL;

    //  Create a copy of the arg ptr array, so that we can "consume" args
    //  for easy arg parsing ...
    prgArg = LocalArgDup( argc, argv );
    if ( prgArg == NULL )
    {
        ITQUPrintf( "Couldn't allocate memory for a 2nd copy of the args.\n" );
        ITQUCall( IQERR::errInvalidParameter );
    }
    ULONG cArgs = argc;

    //
    //  Create the query with appropriate evaluation clauses.
    //
    ITQUCall( ErrIQConsumePredicateArgs( pied, prgArg, &cArgs, ppiq ) );
    piq = *ppiq;

    //
    //  Now add the action clause.
    //
    ITQUCall( ErrIQAddAction( piq, prgArg, &cArgs ) );

    //
    //  If not all args process, invalid syntax ...
    //
    if ( 0 != cArgs )
    {
        ITQUPrintf( "Couldn't process all the args.  Remaining:\n" );
        DumpArgs( prgArg, cArgs );
        ITQUCall( IQERR::errInvalidParameter );
    }

    if ( FITQUDebugMode )
    {
        piq->PrintQuery();
        ITQUPrintf( "\n" );
    }

    piq = NULL;     // so not freed
    err = IQERR::errSuccess;

HandleError:

    if ( piq )
    {
        delete piq;
        *ppiq = NULL;
    }
    if ( prgArg )
    {
        ITQUFree( prgArg );
    }

    return err;
}

ERR ErrIQCreateIterQueryCount(
        IEntryDescriptor *   pied,
        const INT            argc,
        const CHAR * const   argv[],
        void *               pvResult,
        _Out_ CIterQuery **  ppiq )
{
    IQERR              err = IQERR::errSuccess;
    const char **   prgArg = NULL;
    CIterQuery *       piq = NULL;

    //  Create a copy of the arg ptr array, so that we can "consume" args
    //  for easy arg parsing ...
    prgArg = LocalArgDup( argc, argv );
    if ( prgArg == NULL )
    {
        ITQUPrintf( "Couldn't allocate memory for a 2nd copy of the args.\n" );
        ITQUCall( IQERR::errInvalidParameter );
    }
    ULONG cArgs = argc;

    //
    //  Create the query with appropriate evaluation clauses.
    //
    ITQUCall( ErrIQConsumePredicateArgs( pied, prgArg, &cArgs, ppiq ) );
    piq = *ppiq;

    //
    //  Now add the action clause.
    //
    ITQUCall( ErrIQAddAction( piq, prgArg, &cArgs, pvResult ) );

    //
    //  If not all args process, invalid syntax ...
    //
    if ( 0 != cArgs )
    {
        ITQUPrintf( "Couldn't process all the args.  Remaining:\n" );
        DumpArgs( prgArg, cArgs );
        ITQUCall( IQERR::errInvalidParameter );
    }

    if ( FITQUDebugMode )
    {
        piq->PrintQuery();
        ITQUPrintf( "\n" );
    }

    piq = NULL;     // so not freed
    err = IQERR::errSuccess;

HandleError:

    if ( piq )
    {
        delete piq;
        *ppiq = NULL;
    }
    if ( prgArg )
    {
        ITQUFree( prgArg );
    }

    return err;
}

#endif // ITERQUERY_HXX_INCLUDED

