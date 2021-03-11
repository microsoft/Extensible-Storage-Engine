// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#pragma warning( disable : 4127 )

#include "esetest.h"
#include <stdio.h>

#define mdelete( ptr )          \
if ( ptr != 0 ) {               \
  delete ptr;                   \
  ptr = 0;                      \
}

#define mdelete_array( ptr )    \
if ( ptr != 0 ) {               \
  delete[] ptr;                 \
  ptr = 0;                      \
}

#define WSZVERBOSE  L"\t[verbose] "
#define SZVERBOSE   "\t[verbose] "

#define VERBOSE( expr )     \
    if ( g_fVerbose ) {     \
        expr;               \
    }

#define VERBOSE2( expr )    \
    if ( g_fVerbose2 ) {    \
        expr;               \
    }

void ReportErr( long err, long expected, unsigned long ulLine, const char *szFileName, const char *szFuncCalled );
int ReportWarn( long warn, long expected, unsigned long ulLine, const char *szFileName, const char *szFuncCalled );
int ReportExpectedErr( JET_ERR expError );
int ReportReceivedErr( JET_ERR expError );

#define CallJ( fn, label, experr )                                          \
do {                                                                        \
    if ( ( experr != ( err = fn ) ) && ( err <= JET_errSuccess ) ) {        \
        ReportErr( err, experr, __LINE__, __FILE__, #fn );                  \
        if ( JET_errSuccess == err && experr != JET_errSuccess ) {          \
            err = JET_errTestError;                                         \
        }                                                                   \
        goto label;                                                         \
    }                                                                       \
    else if ( JET_errSuccess < err ) {                                      \
        ReportWarn( err, experr, __LINE__, __FILE__, #fn );                 \
    }                                                                       \
} while ( 0 )

#define CallJNoWarning( fn, label, experr )                                 \
do {                                                                        \
    if ( ( experr != ( err = fn ) ) ) {                                     \
        ReportErr( err, experr, __LINE__, __FILE__, #fn );                  \
        goto label;                                                         \
    }                                                                       \
} while ( 0 )

#define CallJSeh( fn, experr )                                                  \
    do {                                                                        \
        if ( ( experr != ( err = fn ) ) && ( err <= JET_errSuccess ) ) {        \
            ReportErr( err, experr, __LINE__, __FILE__, #fn );                  \
            if ( JET_errSuccess == err && experr != JET_errSuccess ) {          \
                err = JET_errTestError;                                         \
            }                                                                   \
            __leave;                                                            \
        }                                                                       \
        else if ( JET_errSuccess < err ) {                                      \
            ReportWarn( err, experr, __LINE__, __FILE__, #fn );                 \
        }                                                                       \
    } while ( 0 )

#define CallErr( fn, errExpected )                  \
do {                                                \
    ReportExpectedErr( errExpected );               \
    CallJ( fn, Cleanup, errExpected );              \
    if ( ( errExpected ) != err ) {                 \
        ReportReceivedErr( err );                   \
    }                                               \
} while ( 0 )

#define CallVJ( fn, fnExpected )                                                        \
do {                                                                                    \
    JET_ERR err = fn;                                                                   \
    JET_ERR errExpected = fnExpected;                                                   \
    if ( err != errExpected )                                                           \
    {                                                                                   \
        char sz[ MAX_PATH ];                                                            \
        sprintf_s( sz, _countof( sz ),                                                  \
                            "Expression: ( %s ) == ( %s )  -  Returned: %ld == %ld",    \
                             #fn, #fnExpected, err, errExpected );                      \
        tcprintf( ANSI_RED, "File %s, Line %lu: %s" CRLF, __FILE__, __LINE__, sz );     \
        VerifySz( err == errExpected, sz );                                             \
    }                                                                                   \
} while ( 0 )

#define CALLJET_SEH( func )                                                     \
do {                                                                            \
    jErr = ( func );                                                            \
    if ( jErr < JET_errSuccess ) {                                              \
        ReportErr( jErr, JET_errSuccess, __LINE__, __FILE__, #func );           \
        __leave;                                                                \
    }                                                                           \
} while ( 0 )

#define CALLJET_TERMINATE( func )                                               \
do {                                                                            \
    JET_ERR jErr = ( func );                                                    \
    if ( jErr < JET_errSuccess ) {                                              \
        ReportErr( jErr, JET_errSuccess, __LINE__, __FILE__, #func );           \
    }                                                                           \
} while ( 0 )

#define Call( fn )                      CallJ( fn, Cleanup, JET_errSuccess )
#define CallNoWarning( fn )             CallJNoWarning( fn, Cleanup, JET_errSuccess )
#define CallSeh( fn )                   CallJSeh( fn, JET_errSuccess )
#define CallVSuccess( fn )              CallVJ( fn, JET_errSuccess )
#define CallVErr( fn, fnExpected )      CallVJ( fn, fnExpected )
#define CallVTrue( fn )                 CallVJ( fn, true )

#define NO_GRBIT        0

#ifndef JET_bitNil
#define JET_bitNil      0
#endif
#define JET_colidNil        0

#define JET_errTestError    -64

#define CODE_PAGE_UNICODE       1200
#define CODE_PAGE_ANSI          1252
const DWORD LCID_USENGLISH = MAKELCID ( MAKELANGID ( LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT );

const DWORD LCID_NONE = MAKELCID( MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ), SORT_DEFAULT );

const DWORD LCID_INVARIANT = MAKELCID( MAKELANGID( LANG_INVARIANT, SUBLANG_NEUTRAL ), SORT_DEFAULT );

#define FailIf( x, y )      Call( ( y, x ) ? JET_errTestError : JET_errSuccess )
#define TestP( x, y )       Call( ( y, x ) ? JET_errSuccess : JET_errTestError )
template<typename T> inline void pdelete_v( T*& rpT ) { delete[ ] rpT; rpT = NULL; }
template<typename T> inline void pdelete( T*& rpT ) { delete rpT; rpT = NULL; }
#define CountedLoop( i, init, final, body )                                     \
do {                                                                            \
    for ( size_t i = ( init ); i < ( size_t )( final ); ++i ) { body; }                     \
} while ( 0 )
inline BOOL pclose( HANDLE& h )
{
    BOOL rc = !0;
    if ( NULL != h && INVALID_HANDLE_VALUE != h ) {
        rc = CloseHandle( h );
        h = NULL;
    }
    return rc;
}
#define HrCallJ( fn, label ) \
do { \
    if ( FAILED( hr = ( fn ) ) ) { \
        tcprintf( ANSI_RED, "%#X=%s" CRLF, hr, #fn ); \
        tcprintf( ANSI_RED, "[%s:%lu]" CRLF, __FILE__, ( long )__LINE__ ); \
        goto label; \
    } \
} while ( 0 )

#define HrCall( fn )    HrCallJ( fn, Cleanup )
#define sizeof2( x )    ( sizeof( x ) / sizeof( *( x ) ) )
JET_ERR JET_API JetOSSnapshotEnd( const JET_OSSNAPID snapId, const JET_GRBIT grbit );




const JET_API_PTR   g_esetestckbLogFile = 5120;


bool
FEsetestWidenParameters()
;

bool
FEsetestAlwaysNarrow()
;

HMODULE
HmodEsetestEseDll()
;

ULONG64
QWEsetestQueryPerformanceCounter()
;


#if 1
wchar_t*
EsetestWidenString(
    __in PSTR   szFunction,
    __in PCSTR  sz
)
;

JET_ERR
EsetestUnwidenString(
    __in PSTR   szFunction,
    __in PWSTR  wsz,
    __inout PSTR    sz
)
;


JET_ERR
EsetestCleanupWidenString(
    __in PSTR   szFunction,
    __inout PWSTR   wsz,
    __in PCSTR  sz
)
;

wchar_t*
EsetestWidenStringWithLength(
    __in PSTR   szFunction,
    __in PCSTR  sz,
    __in const unsigned long cchsz
)
;

JET_ERR
EsetestUnwidenStringWithLength(
    __in PSTR   szFunction,
    __in PWSTR  wsz,
    __in const unsigned long cchsz,
    __inout PSTR    sz
)
;

wchar_t*
EsetestWidenCbString(
    __in PSTR   szFunction,
    __in PCSTR  sz,
    __in const unsigned long cchsz
)
;

JET_ERR
EsetestUnwidenCbString(
    __in PSTR   szFunction,
    __in PWSTR  wsz,
    __in const unsigned long cchsz,
    __inout PCSTR   sz
)
;

JET_RSTMAP_W*
EsetestWidenJET_RSTMAP(
    __in PSTR   szFunction,
    __in_ecount( crstfilemap ) const JET_RSTMAP*    rgstmap,
    __in const long         crstfilemap
)
;

JET_ERR
EsetestUnwidenJET_RSTMAP(
    __in PSTR   szFunction,
    __in_ecount( crstfilemap ) JET_RSTMAP_W*        wrgstmap,
    __in const long crstfilemap,
    __in_ecount( crstfilemap ) JET_RSTMAP* rgstmap
)
;

JET_RSTINFO_W*
EsetestWidenJET_RSTINFO(
    __in PSTR   szFunction,
    __in const JET_RSTINFO*     prstInfo
)
;

JET_ERR
EsetestUnwidenJET_RSTINFO(
    __in PSTR   szFunction,
    __in JET_RSTINFO_W*     wrgstinfo,
    __inout JET_RSTINFO*    rgstinfo
)
;

JET_RSTINFO2_W*
EsetestWidenJET_RSTINFO2(
    __in PSTR   szFunction,
    __in const JET_RSTINFO2*    prstInfo
)
;

JET_ERR
EsetestUnwidenJET_RSTINFO2(
    __in PSTR   szFunction,
    __in JET_RSTINFO2_W*        wrgstinfo,
    __inout JET_RSTINFO2*       rgstinfo
)
;

JET_SETSYSPARAM_W*
EsetestWidenJET_SETSYSPARAM(
    __in PSTR   szFunction,
    __in_ecount( crstfilemap ) const JET_SETSYSPARAM*   rgstmap,
    __in const long         crstfilemap
)
;

JET_ERR
EsetestUnwidenJET_SETSYSPARAM(
    __in PSTR   szFunction,
    __in_ecount( crstfilemap ) JET_SETSYSPARAM_W*       wrgstmap,
    __in const long         crstfilemap,
    __inout_ecount( crstfilemap ) JET_SETSYSPARAM*      rgstmap
)
;


JET_TABLECREATE_W*
EsetestWidenJET_TABLECREATE(
    __in PSTR   szFunction,
    __in const JET_TABLECREATE* ptablecreate
)
;

JET_ERR
EsetestUnwidenJET_TABLECREATE(
    __in PSTR   szFunction,
    __in const JET_TABLECREATE_W*   wptablecreate,
    __inout JET_TABLECREATE* const  ptablecreate
)
;

JET_TABLECREATE2_W*
EsetestWidenJET_TABLECREATE2(
    __in PSTR   szFunction,
    __in const JET_TABLECREATE2*    ptablecreate2
)
;

JET_ERR
EsetestUnwidenJET_TABLECREATE2(
    __in PSTR   szFunction,
    __in const JET_TABLECREATE2_W*  wptablecreate,
    __inout JET_TABLECREATE2* const ptablecreate
)
;

JET_TABLECREATE3_W*
EsetestWidenJET_TABLECREATE3(
    __in PSTR   szFunction,
    __in const JET_TABLECREATE3*    ptablecreate3
)
;

JET_ERR
EsetestUnwidenJET_TABLECREATE3(
    __in PSTR   szFunction,
    __in const JET_TABLECREATE3_W*  wptablecreate,
    __inout JET_TABLECREATE3* const ptablecreate
)
;

JET_TABLECREATE5_W*
EsetestWidenJET_TABLECREATE5(
    __in PSTR   szFunction,
    __in const JET_TABLECREATE5*    ptablecreate5
)
;

JET_ERR
EsetestUnwidenJET_TABLECREATE5(
    __in PSTR   szFunction,
    __in const JET_TABLECREATE5_W*  wptablecreate,
    __inout JET_TABLECREATE5* const ptablecreate
)
;

JET_INDEXCREATE_W*
EsetestWidenJET_INDEXCREATE(
    __in PSTR   szFunction,
    __in_ecount( crstfilemap ) const JET_INDEXCREATE*   rgstmap,
    __in const long         crstfilemap
)
;

JET_ERR
EsetestUnwidenJET_INDEXCREATE(
    __in PSTR   szFunction,
    __in_ecount( crstfilemap ) JET_INDEXCREATE_W*       wrgindexcreate,
    __in const long         crstfilemap,
    __inout_ecount( crstfilemap ) JET_INDEXCREATE* const    rgindexcreate
)
;

JET_INDEXCREATE2_W*
EsetestWidenJET_INDEXCREATE2(
    __in PSTR   szFunction,
    __in_ecount( crstfilemap ) const JET_INDEXCREATE2*  rgstmap,
    __in const long         crstfilemap
)
;

JET_ERR
EsetestUnwidenJET_INDEXCREATE2(
    __in PSTR   szFunction,
    __in_ecount( crstfilemap ) JET_INDEXCREATE2_W*      wrgindexcreate2,
    __in const long         crstfilemap,
    __inout_ecount( crstfilemap ) JET_INDEXCREATE2* const   rgindexcreate2
)
;

JET_INDEXCREATE3_W*
EsetestWidenJET_INDEXCREATE3(
    __in PSTR   szFunction,
    __in_ecount( crstfilemap ) const JET_INDEXCREATE3*  rgstmap,
    __in const long         crstfilemap
)
;

JET_ERR
EsetestUnwidenJET_INDEXCREATE3(
    __in PSTR   szFunction,
    __in_ecount( crstfilemap ) JET_INDEXCREATE3_W*      wrgindexcreate3,
    __in const long         crstfilemap,
    __inout_ecount( crstfilemap ) JET_INDEXCREATE3* const   rgindexcreate3
)
;

JET_CONVERT_W*
EsetestWidenJET_CONVERT(
    __in PSTR   szFunction,
    __in const JET_CONVERT* pconvert
)
;

JET_ERR
EsetestUnwidenJET_CONVERT(
    __in PSTR   szFunction,
    __in JET_CONVERT_W*     wpconvert,
    __inout JET_CONVERT* const  pconvert
)
;

JET_CONDITIONALCOLUMN_W*
EsetestWidenJET_CONDITIONALCOLUMN(
    __in PSTR   szFunction,
    __in_ecount( ccolumns ) const JET_CONDITIONALCOLUMN* rgconditionalcolumn,
    __in const long     ccolumns    
)
;

JET_ERR
EsetestUnwidenJET_CONDITIONALCOLUMN(
    __in PSTR   szFunction,
    __in JET_CONDITIONALCOLUMN_W*   wrgconditionalcolumn,
    __in const long         ccolumns,
    __inout JET_CONDITIONALCOLUMN* const    rgconditionalcolumn
)
;

JET_COLUMNCREATE_W*
EsetestWidenJET_COLUMNCREATE(
    __in PSTR   szFunction,
    __in_ecount( ccolumns ) const JET_COLUMNCREATE* rgcolumncreate,
    __in const long     ccolumns    
)
;

JET_ERR
EsetestUnwidenJET_COLUMNCREATE(
    __in PSTR   szFunction,
    __in JET_COLUMNCREATE_W*        wrgcolumncreate,
    __in const long         ccolumns,
    __inout JET_COLUMNCREATE* const rgcolumncreate
)
;
JET_LOGINFO_W*
EsetestWidenJET_LOGINFO(
    __in PSTR   szFunction,
    __in const JET_LOGINFO*     pLogInfo
)
;
JET_ERR
EsetestUnwidenJET_LOGINFO(
    __in PSTR   szFunction,
    __in JET_LOGINFO_W* wpLogInfo,
    __inout JET_LOGINFO* const  pLogInfo
)
;

#endif


JET_ERR
EsetestAdaptJET_INDEXCREATE(
    __in PSTR   szFunction,
    __in_ecount_opt( cIndexCreate ) JET_INDEXCREATE*    rgindexcreateInUnknownFormat,
    unsigned long   cIndexCreate,
    __deref_out_ecount( cIndexCreate ) JET_INDEXCREATE**    prgindexcreate,
    __out bool*     pfAdapted
)
;

JET_ERR
EsetestUnadaptJET_INDEXCREATE(
    __in PSTR   szFunction,
    __inout_ecount( cIndexCreate ) JET_INDEXCREATE* rgindexcreateInUnknownFormat,
    unsigned long   cIndexCreate,
    __inout_ecount( cIndexCreate ) JET_INDEXCREATE**    prgindexcreate
)
;
JET_ERR
EsetestAdaptJET_TABLECREATE(
    __in PSTR   szFunction,
    __in JET_TABLECREATE*   ptablecreateInNewFormat,
    __deref_out JET_TABLECREATE**   pptablecreate,
    __out bool*     pfAdapted
)
;

JET_ERR
EsetestUnadaptJET_TABLECREATE(
    __in PSTR   szFunction,
    __inout JET_TABLECREATE*    ptablecreateInNewFormat,
    __inout JET_TABLECREATE**   pptablecreate
)
;

JET_ERR
EsetestAdaptJET_TABLECREATE2(
    __in PSTR   szFunction,
    __in JET_TABLECREATE2*  ptablecreateInNewFormat,
    __deref_out JET_TABLECREATE2**  pptablecreate,
    __out bool*     pfAdapted
)
;

JET_ERR
EsetestUnadaptJET_TABLECREATE2(
    __in PSTR   szFunction,
    __inout JET_TABLECREATE2*   ptablecreateInNewFormat,
    __inout JET_TABLECREATE2**  pptablecreate
)
;


JET_COLUMNDEF*
EsetestCompressJET_COLUMNDEF(
    __in const JET_COLUMNDEF*   pcolumndef,
    __out bool*                 pfAllocated
)
;

JET_COLUMNCREATE*
EsetestCompressJET_COLUMNCREATE(
    __in_ecount_opt(cColumns) const JET_COLUMNCREATE*   pcolumncreate,
    unsigned long                                       cColumns,
    __out bool*                                         pfAllocated
)
;

JET_SETCOLUMN*
EsetestCompressJET_SETCOLUMN(
    __in_ecount_opt(csetcolumn) const JET_SETCOLUMN*    psetcolumn,
    unsigned long                                       csetcolumn,
    __out bool*                                         pfAllocated
)
;


JET_ERR JET_API dummyColumnUserCallBack(
    JET_SESID       sesid,
    JET_DBID        dbid,
    JET_TABLEID     tableid,
    JET_CBTYP       cbtyp,
    void*           pvArg1,
    void*           pvArg2,
    void*           pvContext,
    ULONG_PTR       ulUnused )
;

