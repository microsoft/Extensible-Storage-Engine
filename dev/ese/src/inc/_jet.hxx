// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _JET_H
#define _JET_H

#define CATCH_EXCEPTIONS    // catch exceptions in JET and handle like asserts

#define CODECONST(type) type const

#define Unused( var ) ( var )

typedef ULONG OBJID;

    /* cbFilenameMost includes the trailing null terminator */

const INT   cbFilenameMost          = 260;  // Windows NT limit

const INT   cbUnpathedFilenameMax   = 12;   // max length of an 8.3-format filename, without a path

    /*** Global system initialization variables ***/

extern BOOL g_fRepair;      /* if this is for repair or not */


INLINE VOID ProbeClientBuffer( __out_bcount(cb) VOID * pv, const ULONG cb, const BOOL fNullOkay = fFalse )
{
#ifdef DEBUG
    if ( NULL != pv || !fNullOkay )
    {
        if ( cb > 0 )
        {
            volatile BYTE* volatile pb = (volatile BYTE*) pv;
            BYTE bStart = pb[ 0 ];
            BYTE bEnd = pb[ cb - 1 ];

            // This will crash if the buffer is invalid.
            pb[ 0 ] = 42;
            pb[ cb - 1 ] = 43;

            // Restore the previous values.
            pb[ 0 ] = bStart;
            pb[ cb - 1 ] = bEnd;
        }
    }
#endif
}

    /* util.c */

ERR ErrUTILICheckName(
    __out_bcount(JET_cbNameMost+1) PSTR const       szNewName,
    const CHAR * const  szName,
    const BOOL          fTruncate );
INLINE ERR ErrUTILCheckName(
    // UNDONE_BANAPI: shoudl be, but must remove callers of ErrUTILICheckName ...
    //__out_ecount(cbNameMost) PSTR const       szNewName,
    __out_ecount(JET_cbNameMost+1) PSTR const       szNewName,
    const CHAR * const  szName,
    const ULONG         cbNameMost,
    const BOOL          fTruncate = fFalse )
{
    //  ErrUTILICheckName() currently assumes a buffer of JET_cbNameMost+1
    //
    Assert( JET_cbNameMost+1 == cbNameMost );
    return ErrUTILICheckName( szNewName, szName, fTruncate );
}


ERR ErrUTILICheckPathName(
    __out_bcount(OSFSAPI_MAX_PATH * sizeof(WCHAR)) PWSTR const      wszNewName,
    PCWSTR const        wszName,
    const BOOL          fTruncate );
INLINE ERR ErrUTILCheckPathName(
    __in_bcount(cbNameMost) PWSTR const     wszNewName,
    const WCHAR * const wszName,
    const ULONG         cbNameMost,
    const BOOL          fTruncate = fFalse )
{
    //  ErrUTILICheckPathName() currently assumes a buffer of IFileSystemAPI::cchPathMax
    //
    Assert( IFileSystemAPI::cchPathMax * sizeof(WCHAR) == cbNameMost );
    return ErrUTILICheckPathName( wszNewName, wszName, fTruncate );
}


template< INT cchMax, OSSTR_CONVERSION osstrConversion = OSSTR_CONTEXT_DEPENDENT_CONVERSION >
class CAutoISZ
{
    public:
        CAutoISZ();
        ~CAutoISZ();

    public:
        ERR ErrAlloc( const size_t cb );

        ERR ErrGet( WCHAR * const sz, const size_t cbMax );
        ERR ErrSet( const WCHAR * sz );

        operator char*()        { return m_sz; }

        char * Pv()             { return m_sz; }
        size_t Cb() const       { return m_cb; }

    private:
        char *  m_sz;
        size_t  m_cb;
        char    m_szPreAlloc[ cchMax + 1 ];
};

template< INT cchMax, OSSTR_CONVERSION osstrConversion >
inline CAutoISZ< cchMax, osstrConversion >::
CAutoISZ()
{
    m_sz = NULL;
    m_cb = 0;
}

template< INT cchMax, OSSTR_CONVERSION osstrConversion >
inline CAutoISZ< cchMax, osstrConversion >::
~CAutoISZ()
{
    if ( m_sz != m_szPreAlloc )
    {
        delete[] m_sz;
    }
}

template< INT cchMax, OSSTR_CONVERSION osstrConversion >
inline ERR CAutoISZ< cchMax, osstrConversion >::
ErrSet( const WCHAR * sz )
{
    ERR         err         = JET_errSuccess;
    char *      szNew       = NULL;

    if ( m_sz != m_szPreAlloc )
    {
        delete[] m_sz;
    }
    m_sz = NULL;
    m_cb = 0;

    if (sz)
    {
        size_t      cch;

        err = ErrOSSTRUnicodeToAscii( sz, NULL, 0, &cch, OSSTR_NOT_LOSSY, osstrConversion );

        // we don't expect other errors but still be check
        Assert( JET_errBufferTooSmall == err );

        if ( JET_errBufferTooSmall == err )
        {
            err = JET_errSuccess;
        }
        Call( err );

        if ( cch < cchMax )
        {
            szNew = m_szPreAlloc;
        }
        else
        {
            Alloc( szNew = static_cast<char *>( new char[cch] ) );
        }

        Call( ErrOSSTRUnicodeToAscii( sz, szNew, cch, NULL, OSSTR_NOT_LOSSY, osstrConversion ) );

        m_cb = cch * sizeof( char );
        m_sz = szNew;
        szNew = NULL;
    }

HandleError:

    if ( szNew != m_szPreAlloc )
    {
        delete[] szNew;
    }
    return err;
}

template< INT cchMax, OSSTR_CONVERSION osstrConversion >
inline ERR CAutoISZ< cchMax, osstrConversion >::
ErrAlloc( const size_t cb )
{
    ERR             err         = JET_errSuccess;

    if ( m_sz != m_szPreAlloc )
    {
        delete[] m_sz;
    }
    m_sz = NULL;
    m_cb = 0;

    if ( cchMax > cb )
    {
        m_sz = m_szPreAlloc;
    }
    else
    {
        AllocR( m_sz = static_cast<char *>( new char[cb] ) );
    }

    memset( m_sz, 0, cb );
    m_cb = cb;

    return err;
}

template< INT cchMax, OSSTR_CONVERSION osstrConversion >
inline ERR CAutoISZ< cchMax, osstrConversion >::
ErrGet( WCHAR * const sz, const size_t cbMax )
{
    size_t          cwchActual  = 0;

    return ErrOSSTRAsciiToUnicode( m_sz, sz, cbMax / sizeof( WCHAR ), &cwchActual, osstrConversion );
}

// generic CAutoWSZ with no pre-allocated size
//
typedef CAutoISZ< 0 >   CAutoSZ;

// this should be JET_cbNameMost not 64
//
//typedef CAutoISZ< 64 >    CAutoSZDDL;

class CAutoSZDDL: public CAutoISZ< 64, OSSTR_FIXED_CONVERSION >
{

    public:
    
        // we need to overwrite to return a specific error
        // on invalid characters in the name
        //
        ERR ErrSet( const WCHAR * sz )
        {
            ERR err = ((CAutoISZ<64, OSSTR_FIXED_CONVERSION>*)this)->ErrSet( sz );
            if ( err == JET_errUnicodeTranslationFail )
            {
                err = ErrERRCheck( JET_errInvalidName );
            }
            return err;
        }
};

template< INT cchMax, OSSTR_CONVERSION osstrConversion = OSSTR_CONTEXT_DEPENDENT_CONVERSION >
class CAutoIWSZ
{
    public:
        CAutoIWSZ();
        ~CAutoIWSZ();

    public:
        ERR ErrAlloc( const size_t cb );

        ERR ErrGet( CHAR * const sz, const size_t cbMax );
        ERR ErrSet( const CHAR * sz );
        ERR ErrSet( const UnalignedLittleEndian< WCHAR > * sz );

        operator WCHAR*()       { return m_sz; }

        WCHAR * Pv()            { return m_sz; }
        size_t Cb() const       { return m_cb; }

    private:
        WCHAR * m_sz;
        size_t  m_cb;
        WCHAR   m_szPreAlloc[ cchMax + 1 ];
};

template< INT cchMax, OSSTR_CONVERSION osstrConversion >
inline CAutoIWSZ< cchMax, osstrConversion >::
CAutoIWSZ()
{
    m_sz = NULL;
    m_cb = 0;
}

template< INT cchMax, OSSTR_CONVERSION osstrConversion >
inline CAutoIWSZ< cchMax, osstrConversion >::
~CAutoIWSZ()
{
    if (m_sz != m_szPreAlloc )
    {
        delete[] m_sz;
    }
}

template< INT cchMax, OSSTR_CONVERSION osstrConversion >
inline ERR CAutoIWSZ< cchMax, osstrConversion >::
ErrSet( const CHAR * sz )
{
    ERR         err         = JET_errSuccess;
    WCHAR *     szNew       = NULL;

    if (m_sz != m_szPreAlloc )
    {
        delete[] m_sz;
    }
    m_sz = NULL;
    m_cb = 0;

    if (sz)
    {
        size_t      cch;

        // UNICODE_UNDONE_DEFERRED: shell we try to actually convert first
        // in the preallocated buffer? I think so ...
        // Do we get the actual needed count IF we do have a buffer
        // passed in which is not big enough?
        //
        err = ErrOSSTRAsciiToUnicode( sz, NULL, 0, &cch, osstrConversion );

        // we don't expect other errors but still be check
        Assert( JET_errBufferTooSmall == err );

        if ( JET_errBufferTooSmall == err )
        {
            err = JET_errSuccess;
        }
        Call( err );

        if ( cchMax > cch )
        {
            szNew = m_szPreAlloc;
        }
        else
        {
            Alloc( szNew = static_cast<WCHAR *>( new WCHAR[cch] ) );
        }

        Call( ErrOSSTRAsciiToUnicode( sz, szNew, cch, NULL, osstrConversion ) );

        m_cb = cch * sizeof( WCHAR );
        m_sz = szNew;
        szNew = NULL;
    }

HandleError:

    if (szNew != m_szPreAlloc )
    {
        delete[] szNew;
    }
    return err;
}

template< INT cchMax, OSSTR_CONVERSION osstrConversion >
inline ERR CAutoIWSZ< cchMax, osstrConversion >::
ErrSet( const UnalignedLittleEndian< WCHAR > * sz )
{
    ERR         err         = JET_errSuccess;

    if (m_sz != m_szPreAlloc )
    {
        delete[] m_sz;
    }
    m_sz = NULL;
    m_cb = 0;

    if (sz)
    {
        const INT cch = LOSStrLengthUnalignedW( sz ) + 1;

        if ( cchMax > cch )
        {
            m_sz = m_szPreAlloc;
        }
        else
        {
            Alloc( m_sz = new WCHAR[cch] );
        }

        m_cb = cch * sizeof( WCHAR );
        memcpy( m_sz, sz, m_cb );
    }

HandleError:

    return err;
}

template< INT cchMax, OSSTR_CONVERSION osstrConversion >
inline ERR CAutoIWSZ< cchMax, osstrConversion >::
ErrAlloc( const size_t cb )
{
    ERR             err         = JET_errSuccess;
    const size_t    cch         = cb / sizeof( WCHAR );

    if (m_sz != m_szPreAlloc )
    {
        delete[] m_sz;
    }
    m_sz = NULL;
    m_cb = 0;

    if ( 0 !=  ( cb % sizeof( WCHAR ) ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( cchMax > cch )
    {
        m_sz = m_szPreAlloc;
    }
    else
    {
        AllocR( m_sz = static_cast<WCHAR *>( new WCHAR[cch] ) );
    }

    memset( m_sz, 0, cb );
    m_cb = cb;

    return err;
}

template< INT cchMax, OSSTR_CONVERSION osstrConversion >
inline ERR CAutoIWSZ< cchMax, osstrConversion >::
ErrGet( CHAR * const sz, const size_t cbMax )
{
    size_t              cchActual   = 0;

    return ErrOSSTRUnicodeToAscii( m_sz, sz, cbMax / sizeof( CHAR ), &cchActual, OSSTR_NOT_LOSSY, osstrConversion );
}


// generic CAutoWSZ with no pre-allocated size
//
typedef CAutoIWSZ< 0 >  CAutoWSZ;

// CAutoWSZ with no pre-allocated size for ASCII usage
//
typedef CAutoIWSZ< 0, OSSTR_FIXED_CONVERSION >  CAutoWSZFixedConversion;

// this should be JET_cbNameMost not 64 (or JET_cbNameMost / sizeof( WCHAR )
// but JET_cbNameMost depends on the JET_UNICODE definition ...
//
typedef CAutoIWSZ< 64, OSSTR_FIXED_CONVERSION > CAutoWSZDDL;

// not always best to use as it will put on the stack 180+ bytes
// but it should be enough for most users having path shorted then this
// For the others, it will go to the allocation
//
typedef CAutoIWSZ< 90 > CAutoWSZPATH;


#ifdef CATCH_EXCEPTIONS
#ifdef MINIMAL_FUNCTIONALITY

/       swapping to the ET* calls here.  Same for _etguidApiCallStop as well obvi.

#define JET_TRY_( api, func, fDisableLockCheck )                                    \
{                                                                                   \
    JET_ERR err;                                                                    \
    DWORD cDisableDeadlockCheck, cDisableOwnershipCheck, cLocks;                    \
    const DWORD ulTraceApiId = api;                                                 \
    OSEventTrace( _etguidApiCall_Start, 1, &ulTraceApiId );                         \
    CLockDeadlockDetectionInfo::GetApiEntryState(&cDisableDeadlockCheck, &cDisableOwnershipCheck, &cLocks);                     \
    Assert( FBFApiClean() );                                                        \
    TRY                                                                             \
    {                                                                               \
        err = (func);                                                               \
    }                                                                               \
    EXCEPT( GrbitParam( JET_paramExceptionAction ) != JET_ExceptionNone ? ExceptionFail( _T( "API" ) ) : efaContinueSearch )    \
    {                                                                               \
        AssertPREFIX( !"This code path should be impossible (the exception-handler should have terminated the process)." );     \
        err = ErrERRCheck( JET_errInternalError );                                  \
    }                                                                               \
    ENDEXCEPT                                                                       \
    AssertRTL( err > -65536 && err < 65536 );                                       \
    fDisableLockCheck ? CLockDeadlockDetectionInfo::DisableLockCheckOnApiExit() : 0;\
    CLockDeadlockDetectionInfo::AssertCleanApiExit(cDisableDeadlockCheck, cDisableOwnershipCheck, cLocks);                      \
    fDisableLockCheck ? CLockDeadlockDetectionInfo::EnableLockCheckOnApiExit() : 0; \
    Assert( FBFApiClean() );                                                        \
    OSEventTrace( _etguidApiCall_Stop, 2, &ulTraceApiId, &err );                    \
                                                                                    \
    return err;                                                                     \
}

#else  //  !MINIMAL_FUNCTIONALITY

#define JET_TRY_( api, func, fDisableLockCheck )                                    \
{                                                                                   \
    JET_ERR err;                                                                    \
    DWORD cDisableDeadlockCheck, cDisableOwnershipCheck, cLocks;                    \
    const DWORD ulTraceApiId = api;                                                 \
    OSEventTrace( _etguidApiCall_Start, 1, &ulTraceApiId );                         \
    CLockDeadlockDetectionInfo::GetApiEntryState(&cDisableDeadlockCheck, &cDisableOwnershipCheck, &cLocks);                     \
    TRY                                                                             \
    {                                                                               \
        err = (func);                                                               \
        OSTrace( JET_tracetagAPI, OSFormat( "End %s with error %d (0x%x)", _T( #func ), err, err ) );   \
    }                                                                               \
    EXCEPT( GrbitParam( JET_paramExceptionAction ) != JET_ExceptionNone ? ExceptionFail( _T( #func ) ) : efaContinueSearch )    \
    {                                                                               \
        AssertPREFIX( !"This code path should be impossible (the exception-handler should have terminated the process)." );     \
        err = ErrERRCheck( JET_errInternalError );                                  \
    }                                                                               \
    ENDEXCEPT                                                                       \
    AssertRTL( err > -65536 && err < 65536 );                                       \
    fDisableLockCheck ? CLockDeadlockDetectionInfo::DisableLockCheckOnApiExit() : 0;\
    CLockDeadlockDetectionInfo::AssertCleanApiExit(cDisableDeadlockCheck, cDisableOwnershipCheck, cLocks);                      \
    fDisableLockCheck ? CLockDeadlockDetectionInfo::EnableLockCheckOnApiExit() : 0; \
    Assert( FBFApiClean() );                                                        \
    Assert( !FOSRefTraceErrors() || Ptls()->fInCallback );                          \
    OSEventTrace( _etguidApiCall_Stop, 2, &ulTraceApiId, &err );                    \
                                                                                    \
    return err;                                                                     \
}

#endif  //  MINIMAL_FUNCTIONALITY
#else  //  !CATCH_EXCEPTIONS

#define JET_TRY_( api, func, fDisableLockCheck )                                    \
{                                                                                   \
    DWORD cDisableDeadlockCheck, cDisableOwnershipCheck, cLocks;                    \
    const DWORD ulTraceApiId = api;                                                 \
    OSEventTrace( _etguidApiCall_Start, 1, &ulTraceApiId );                         \
    CLockDeadlockDetectionInfo::GetApiEntryState(&cDisableDeadlockCheck, &cDisableOwnershipCheck, &cLocks);                     \
    const JET_ERR   err     = (func);                                               \
    OSTrace( JET_tracetagAPI, OSFormat( "End %s with error %d (0x%x)", _T( #func ), err, err ) );   \
    AssertRTL( err > -65536 && err < 65536 );                                       \
    fDisableLockCheck ? CLockDeadlockDetectionInfo::DisableLockCheckOnApiExit() : 0;\
    CLockDeadlockDetectionInfo::AssertCleanApiExit(cDisableDeadlockCheck, cDisableOwnershipCheck, cLocks);                      \
    fDisableLockCheck ? CLockDeadlockDetectionInfo::EnableLockCheckOnApiExit() : 0; \
    OSEventTrace( _etguidApiCall_Stop, 2, &ulTraceApiId, &err );                    \
    return err;                                                                     \
}

#endif  //  CATCH_EXCEPTIONS

#define JET_TRY( api, func )            JET_TRY_( api, func, fFalse )

INLINE LONG_PTR IbAlignForThisPlatform( const LONG_PTR ib )
{
    const LONG_PTR  lMask   = (LONG_PTR)( 0 - sizeof(LONG_PTR) );
    return ( ( ib + ( sizeof(LONG_PTR) - 1 ) ) & lMask );
}
INLINE VOID *PvAlignForThisPlatform( const VOID * const pv )
{
    Assert( NULL != pv );
    return reinterpret_cast<VOID *>( IbAlignForThisPlatform( reinterpret_cast<LONG_PTR>( pv ) ) );
}
INLINE BOOL FAlignedForThisPlatform( const VOID * const pv )
{
    Assert( NULL != pv );
    return ( !( reinterpret_cast<LONG_PTR>( pv ) & ( sizeof(LONG_PTR) - 1 ) ) );
}


#define opIdle                              1
#define opGetTableIndexInfo                 2
#define opGetIndexInfo                      3
#define opGetObjectInfo                     4
#define opGetTableInfo                      5
#define opCreateObject                      6
#define opDeleteObject                      7
#define opRenameObject                      8
#define opBeginTransaction                  9
#define opCommitTransaction                 10
#define opRollback                          11
#define opOpenTable                         12
#define opDupCursor                         13
#define opCloseTable                        14
#define opGetTableColumnInfo                15
#define opGetColumnInfo                     16
#define opRetrieveColumn                    17
#define opRetrieveColumns                   18
#define opSetColumn                         19
#define opSetColumns                        20
#define opPrepareUpdate                     21
#define opUpdate                            22
#define opDelete                            23
#define opGetCursorInfo                     24
#define opGetCurrentIndex                   25
#define opSetCurrentIndex                   26
#define opMove                              27
#define opMakeKey                           28
#define opSeek                              29
#define opGetBookmark                       30
#define opGotoBookmark                      31
#define opGetRecordPosition                 32
#define opGotoPosition                      33
#define opRetrieveKey                       34
#define opCreateDatabase                    35
#define opOpenDatabase                      36
#define opGetDatabaseInfo                   37
#define opCloseDatabase                     38
#define opCapability                        39
#define opCreateTable                       40
#define opRenameTable                       41
#define opDeleteTable                       42
#define opAddColumn                         43
#define opRenameColumn                      44
#define opDeleteColumn                      45
#define opCreateIndex                       46
#define opRenameIndex                       47
#define opDeleteIndex                       48
#define opComputeStats                      49
#define opAttachDatabase                    50
#define opDetachDatabase                    51
#define opOpenTempTable                     52
#define opSetIndexRange                     53
#define opIndexRecordCount                  54
#define opGetChecksum                       55
#define opGetObjidFromName                  56
#define opEscrowUpdate                      57
#define opGetLock                           58
#define opRetrieveTaggedColumnList          59
#define opCreateTableColumnIndex            60
#define opSetColumnDefaultValue             61
#define opPrepareToCommitTransaction        62
#define opSetTableSequential                63
#define opResetTableSequential              64
#define opRegisterCallback                  65
#define opUnregisterCallback                66
#define opSetLS                             67
#define opGetLS                             68
#define opGetVersion                        69
#define opBeginSession                      70
#define opDupSession                        71
#define opEndSession                        72
#define opBackupInstance                    73
#define opBeginExternalBackupInstance       74
#define opGetAttachInfoInstance             75
#define opOpenFileInstance                  76
#define opReadFileInstance                  77
#define opCloseFileInstance                 78
#define opGetLogInfoInstance                79
#define opGetTruncateLogInfoInstance        80
#define opTruncateLogInstance               81
#define opEndExternalBackupInstance         82
#define opSnapshotStart                     83
#define opSnapshotStop                      84
#define opResetCounter                      85
#define opGetCounter                        86
#define opCompact                           87
#define opConvertDDL                        88
#define opUpgradeDatabase                   89
#define opDefragment                        90
#define opSetDatabaseSize                   91
#define opGrowDatabase                      92
#define opSetSessionContext                 93
#define opResetSessionContext               94
#define opSetSystemParameter                95
#define opGetSystemParameter                96
#define opTerm                              97
#define opInit                              98
#define opIntersectIndexes                  99
#define opDBUtilities                       100
#define opGetResourceParam                  101
#define opSetResourceParam                  102
#define opEnumerateColumns                  103
#define opOSSnapPrepareInstance             104
#define opOSSnapTruncateLogInstance         105
#define opGetRecordSize                     106
#define opGetSessionInfo                    107
#define opGetInstanceMiscInfo               108
#define opGetDatabasePages                  109
#define opBeginSurrogateBackup              110
#define opEndSurrogateBackup                111
#define opRemoveLogfile                     112
#define opDatabaseScan                      113
#define opBeginDatabaseIncrementalReseed    114
#define opEndDatabaseIncrementalReseed      115
#define opPatchDatabasePages                116
#define opPrereadKeys                       117
#define opTestHook                          118
#define opConsumeLogData                    119
#define opOnlinePatchDatabasePage           120
#define opGetErrorInfo                      121
#define opResizeDatabase                    122
#define opSetSessionParameter               123
#define opGetSessionParameter               124
#define opPrereadTable                      125 // [Removed]
#define opCommitTransaction2                126
#define opPrereadIndexRanges                127
#define opSetCursorFilter                   128
#define opTracing                           129
#define opGetDatabaseFileInfo               130
#define opGetLogFileInfo                    131
#define opGetPageInfo                       132
#define opGetThreadStats                    133
#define opGetDatabaseSize                   134
#define opEnableMultiInstance               135
#define opCreateInstance                    136
#define opRestoreInstance                   137
#define opStopServiceInstance               138
#define opStopBackupInstance                139
#define opFreeBuffer                        140
#define opGetInstanceInfo                   141
#define opOSSnapshotPrepare                 142
#define opOSSnapshotFreeze                  143
#define opOSSnapshotThaw                    144
#define opOSSnapshotAbort                   145
#define opOSSnapshotTruncateLog             146
#define opOSSnapshotGetFreezeInfo           147
#define opOSSnapshotEnd                     148
#define opPrereadTables                     149
#define opPrereadIndexRange                 150
#define opCreateEncryptionKey               151
#define opSetTableInfo                      152
#define opRetrieveColumnByReference         153
#define opPrereadColumnsByReference         154
#define opStreamRecords                     155
#define opRetrieveColumnFromRecordStream    156
#define opFileSectionInstance               157
#define opRBSPrepareRevert                  158
#define opRBSExecuteRevert                  159
#define opRBSCancelRevert                   160
#define opMax                               161


/* Typedefs for dispatched functions that use tableid(pfucb). */

#define VTAPI

typedef ULONG_PTR JET_VTID;        /* Received from dispatcher */


typedef ERR VTAPI VTFNAddColumn(JET_SESID sesid, JET_VTID vtid,
    const char  *szColumn, const JET_COLUMNDEF  *pcolumndef,
    const void  *pvDefault, ULONG cbDefault,
    JET_COLUMNID  *pcolumnid);

typedef ERR VTAPI VTFNCloseTable(JET_SESID sesid, JET_VTID vtid);

typedef ERR VTAPI VTFNComputeStats(JET_SESID sesid, JET_VTID vtid);

typedef ERR VTAPI VTFNCreateIndex(JET_SESID sesid, JET_VTID vtid,
    JET_INDEXCREATE3_A * pindexcreate, ULONG cIndexCreate );

typedef ERR VTAPI VTFNDelete(JET_SESID sesid, JET_VTID vtid);

typedef ERR VTAPI VTFNDeleteColumn(
    JET_SESID       sesid,
    JET_VTID        vtid,
    const char      *szColumn,
    const JET_GRBIT grbit );

typedef ERR VTAPI VTFNDeleteIndex(JET_SESID sesid, JET_VTID vtid,
    const char  *szIndexName);

typedef ERR VTAPI VTFNDupCursor(JET_SESID sesid, JET_VTID vtid,
    JET_TABLEID  *ptableid, JET_GRBIT grbit);

typedef ERR VTAPI VTFNEscrowUpdate(JET_SESID sesid, JET_VTID vtid,
    JET_COLUMNID columnid, void  *pv, ULONG cbMax,
    void  *pvOld, ULONG cbOldMax, ULONG *pcbOldActual,
    JET_GRBIT grbit);

typedef ERR VTAPI VTFNGetBookmark(
    JET_SESID       sesid,
    JET_VTID        vtid,
    VOID * const    pvBookmark,
    const ULONG     cbMax,
    ULONG * const   pcbActual );

typedef ERR VTAPI VTFNGetIndexBookmark(
    JET_SESID       sesid,
    JET_VTID        vtid,
    VOID * const    pvSecondaryKey,
    const ULONG     cbSecondaryKeyMax,
    ULONG * const   pcbSecondaryKeyActual,
    VOID * const    pvPrimaryBookmark,
    const ULONG     cbPrimaryBookmarkMax,
    ULONG * const   pcbPrimaryBookmarkActual );

typedef ERR VTAPI VTFNGetChecksum(JET_SESID sesid, JET_VTID vtid,
    ULONG  *pChecksum);

typedef ERR VTAPI VTFNGetCurrentIndex(JET_SESID sesid, JET_VTID vtid,
    _Out_z_bytecap_(cchIndexName) PSTR szIndexName, ULONG cchIndexName);

typedef ERR VTAPI VTFNGetCursorInfo(JET_SESID sesid, JET_VTID vtid,
    void  *pvResult, ULONG cbMax, ULONG InfoLevel);

typedef ERR VTAPI VTFNGetRecordPosition(JET_SESID sesid, JET_VTID vtid,
    JET_RECPOS  *pkeypos, ULONG cbKeypos);

typedef ERR VTAPI VTFNGetTableColumnInfo(JET_SESID sesid, JET_VTID vtid,
    const char  *szColumnName, const JET_COLUMNID *pcolid, void  *pvResult,
    ULONG cbMax, ULONG InfoLevel, const BOOL fUnicodeNames );

typedef ERR VTAPI VTFNGetTableIndexInfo(JET_SESID sesid, JET_VTID vtid,
    const char  *szIndexName, void  *pvResult,
    ULONG cbMax, ULONG InfoLevel, const BOOL fUnicodeNames);

typedef ERR VTAPI VTFNGetTableInfo(JET_SESID sesid, JET_VTID vtid,
    _Out_bytecap_(cbMax) void  *pvResult, ULONG cbMax, ULONG InfoLevel);

typedef ERR VTAPI VTFNSetTableInfo(JET_SESID sesid, JET_VTID vtid,
    _In_reads_bytes_opt_(cbParam) const void  *pvParam, ULONG cbParam, ULONG InfoLevel);

typedef ERR VTAPI VTFNGotoBookmark(
    JET_SESID           sesid,
    JET_VTID            vtid,
    const VOID * const  pvBookmark,
    const ULONG         cbBookmark );

typedef ERR VTAPI VTFNGotoIndexBookmark(
    JET_SESID           sesid,
    JET_VTID            vtid,
    const VOID * const  pvSecondaryKey,
    const ULONG         cbSecondaryKey,
    const VOID * const  pvPrimaryBookmark,
    const ULONG         cbPrimaryBookmark,
    const JET_GRBIT     grbit );

typedef ERR VTAPI VTFNGotoPosition(JET_SESID sesid, JET_VTID vtid,
    JET_RECPOS *precpos);

typedef ERR VTAPI VTFNMakeKey(
    JET_SESID       sesid,
    JET_VTID        vtid,
    __in_bcount( cbData ) const VOID *      pvData,
    const ULONG     cbData,
    JET_GRBIT       grbit );

typedef ERR VTAPI VTFNMove(
    JET_SESID           sesid,
    JET_VTID            vtid,
    LONG                cRow,
    JET_GRBIT           grbit );

typedef ERR VTAPI VTFNSetCursorFilter(
    JET_SESID           sesid,
    JET_VTID            vtid,
    JET_INDEX_COLUMN *  rgFilters,
    DWORD               cFilters,
    JET_GRBIT           grbit );

typedef ERR VTAPI VTFNGetLock(JET_SESID sesid, JET_VTID vtid, JET_GRBIT grbit);

typedef ERR VTAPI VTFNPrepareUpdate(JET_SESID sesid, JET_VTID vtid,
    ULONG prep);

typedef ERR VTAPI VTFNRenameColumn(JET_SESID sesid, JET_VTID vtid,
    const char  *szColumn, const char  *szColumnNew, const JET_GRBIT grbit );

typedef ERR VTAPI VTFNRenameIndex(JET_SESID sesid, JET_VTID vtid,
    const char  *szIndex, const char  *szIndexNew);

typedef ERR VTAPI VTFNRenameReference(JET_SESID sesid, JET_VTID vtid,
    const char  *szReference, const char  *szReferenceNew);

typedef ERR VTAPI VTFNRetrieveColumn(
    _In_ JET_SESID                                                          sesid,
    _In_ JET_VTID                                                           vtid,
    _In_ JET_COLUMNID                                                       columnid,
    _Out_writes_bytes_to_opt_( cbData, min( cbData, *pcbActual ) ) VOID*    pvData,
    _In_ const ULONG                                                        cbData,
    _Out_opt_ ULONG *                                                       pcbActual,
    _In_ JET_GRBIT                                                          grbit,
    _Inout_opt_ JET_RETINFO *                                               pretinfo );


typedef ERR VTAPI VTFNRetrieveColumns(
    JET_SESID           sesid,
    JET_VTID            vtid,
    JET_RETRIEVECOLUMN* pretcols,
    ULONG               cretcols );

typedef ERR VTAPI VTFNEnumerateColumns(
    JET_SESID               sesid,
    JET_VTID                vtid,
    ULONG           cEnumColumnId,
    JET_ENUMCOLUMNID*       rgEnumColumnId,
    ULONG*          pcEnumColumn,
    JET_ENUMCOLUMN**        prgEnumColumn,
    JET_PFNREALLOC          pfnRealloc,
    void*                   pvReallocContext,
    ULONG           cbDataMost,
    JET_GRBIT               grbit );

typedef ERR VTAPI VTFNGetRecordSize(
    JET_SESID           vsesid,
    JET_VTID            vtid,
    JET_RECSIZE3 *      precsize,
    const JET_GRBIT     grbit );

typedef ERR VTAPI VTFNRetrieveKey(
    JET_SESID           sesid,
    JET_VTID            vtid,
    VOID*               pvKey,
    const ULONG         cbMax,
    ULONG*              pcbActual,
    JET_GRBIT           grbit );

typedef ERR VTAPI VTFNSeek(
    JET_SESID           sesid,
    JET_VTID            vtid,
    JET_GRBIT           grbit );

typedef ERR VTAPI VTFNPrereadKeys(
    const JET_SESID             sesid,
    const JET_VTID              vtid,
    const void * const * const  rgpvKeys,
    const ULONG * const rgcbKeys,
    const LONG                  ckeys,
    LONG * const                pckeysPreread,
    const JET_GRBIT             grbit );

typedef ERR VTAPI VTFNPrereadIndexRanges(
    const JET_SESID             sesid,
    const JET_VTID              vtid,
    const JET_INDEX_RANGE * const   rgIndexRanges,
    const ULONG         cIndexRanges,
    ULONG * const       pcRangesPreread,
    const JET_COLUMNID * const  rgcolumnidPreread,
    const ULONG         ccolumnidPreread,
    const JET_GRBIT             grbit );

typedef ERR VTAPI VTFNSetColumn(
    JET_SESID           sesid,
    JET_VTID            vtid,
    JET_COLUMNID        columnid,
    const VOID*         pvData,
    const ULONG         cbData,
    JET_GRBIT           grbit,
    JET_SETINFO*        psetinfo );

typedef ERR VTAPI VTFNSetColumns(JET_SESID sesid, JET_VTID vtid,
    JET_SETCOLUMN   *psetcols, ULONG csetcols );

typedef ERR VTAPI VTFNSetIndexRange(JET_SESID sesid, JET_VTID vtid,
    JET_GRBIT grbit);

typedef ERR VTAPI VTFNUpdate(
    JET_SESID sesid,
    JET_VTID vtid,
    _Out_writes_bytes_to_opt_(cbBookmark, *pcbActual) VOID *pvBookmark,
    ULONG cbBookmark,
    ULONG  *pcbActual,
    JET_GRBIT grbit);

typedef ERR VTAPI VTFNRegisterCallback( JET_SESID sesid, JET_VTID vtid, JET_CBTYP cbtyp,
    JET_CALLBACK pCallback, void * pvContext, JET_HANDLE *phCallbackId );

typedef ERR VTAPI VTFNUnregisterCallback( JET_SESID sesid, JET_VTID vtid, JET_CBTYP cbtyp,
    JET_HANDLE hCallbackId );

typedef ERR VTAPI VTFNSetLS( JET_SESID sesid, JET_VTID vtid, JET_LS ls, JET_GRBIT grbit );

typedef ERR VTAPI VTFNGetLS( JET_SESID sesid, JET_VTID vtid, JET_LS *pls, JET_GRBIT grbit );

typedef ERR VTAPI VTFNIndexRecordCount(
    JET_SESID sesid,
    JET_VTID vtid,
    ULONG64 *pcrec,
    ULONG64 crecMax );

typedef ERR VTAPI VTFNRetrieveTaggedColumnList(
    JET_SESID           vsesid,
    JET_VTID            vtid,
    ULONG               *pcentries,
    VOID                *pv,
    const ULONG         cbMax,
    const JET_COLUMNID  columnidStart,
    const JET_GRBIT     grbit );

typedef ERR VTAPI VTFNSetSequential(
    const JET_SESID     vsesid,
    const JET_VTID      vtid,
    const JET_GRBIT     grbit );

typedef ERR VTAPI VTFNResetSequential(
    const JET_SESID     vsesid,
    const JET_VTID      vtid,
    const JET_GRBIT     grbit );

typedef ERR VTAPI VTFNSetCurrentIndex(
    JET_SESID           sesid,
    JET_VTID            tableid,
    const CHAR          *szIndexName,
    const JET_INDEXID   *pindexid,
    const JET_GRBIT     grbit,
    const ULONG         itagSequence );

typedef ERR VTAPI VTFNPrereadIndexRange(
    _In_ const JET_SESID                    sesid,
    _In_ const JET_VTID                 vtid,
    _In_ const JET_INDEX_RANGE * const  pIndexRange,
    _In_ const ULONG                cPageCacheMin,
    _In_ const ULONG                cPageCacheMax,
    _In_ const JET_GRBIT                grbit,
    _Out_opt_ ULONG * const     pcPageCacheActual );

typedef ERR VTAPI VTFNRetrieveColumnByReference(
    _In_ const JET_SESID                                                        sesid,
    _In_ const JET_TABLEID                                                      tableid,
    _In_reads_bytes_( cbReference ) const void * const                          pvReference,
    _In_ const ULONG                                                    cbReference,
    _In_ const ULONG                                                    ibData,
    _Out_writes_bytes_to_opt_( cbData, min( cbData, *pcbActual ) ) void * const pvData,
    _In_ const ULONG                                                    cbData,
    _Out_opt_ ULONG * const                                             pcbActual,
    _In_ const JET_GRBIT                                                        grbit );

typedef ERR VTAPI VTFNPrereadColumnsByReference(
    _In_ const JET_SESID                                    sesid,
    _In_ const JET_TABLEID                                  tableid,
    _In_reads_( cReferences ) const void * const * const    rgpvReferences,
    _In_reads_( cReferences ) const ULONG * const   rgcbReferences,
    _In_ const ULONG                                cReferences,
    _In_ const ULONG                                cPageCacheMin,
    _In_ const ULONG                                cPageCacheMax,
    _Out_opt_ ULONG * const                         pcReferencesPreread,
    _In_ const JET_GRBIT                                    grbit );

typedef ERR VTAPI VTFNStreamRecords(
    _In_ JET_SESID                                                  sesid,
    _In_ JET_TABLEID                                                tableid,
    _In_ const ULONG                                        ccolumnid,
    _In_reads_opt_( ccolumnid ) const JET_COLUMNID * const          rgcolumnid,
    _Out_writes_bytes_to_opt_( cbData, *pcbActual ) void * const    pvData,
    _In_ const ULONG                                        cbData,
    _Out_opt_ ULONG * const                                 pcbActual,
    _In_ const JET_GRBIT                                            grbit );


    /* The following structure is that used to allow dispatching to */
    /* a VT provider.  Each VT provider must create an instance of */
    /* this structure and give the pointer to this instance when */
    /* allocating a table id. */

typedef struct VTDBGDEF
{
    USHORT          cbStruct;
    USHORT          filler;
    char                    szName[32];
    ULONG           dwRFS;
    ULONG           dwRFSMask[4];
} VTDBGDEF;

    /* Please add to the end of the table */

typedef struct tagVTFNDEF {
    USHORT                  cbStruct;
    USHORT                  filler;
    const VTDBGDEF                  *pvtdbgdef;
    VTFNAddColumn                   *pfnAddColumn;
    VTFNCloseTable                  *pfnCloseTable;
    VTFNComputeStats                *pfnComputeStats;
    VTFNCreateIndex                 *pfnCreateIndex;
    VTFNDelete                      *pfnDelete;
    VTFNDeleteColumn                *pfnDeleteColumn;
    VTFNDeleteIndex                 *pfnDeleteIndex;
    VTFNDupCursor                   *pfnDupCursor;
    VTFNEscrowUpdate                *pfnEscrowUpdate;
    VTFNGetBookmark                 *pfnGetBookmark;
    VTFNGetIndexBookmark            *pfnGetIndexBookmark;
    VTFNGetChecksum                 *pfnGetChecksum;
    VTFNGetCurrentIndex             *pfnGetCurrentIndex;
    VTFNGetCursorInfo               *pfnGetCursorInfo;
    VTFNGetRecordPosition           *pfnGetRecordPosition;
    VTFNGetTableColumnInfo          *pfnGetTableColumnInfo;
    VTFNGetTableIndexInfo           *pfnGetTableIndexInfo;
    VTFNGetTableInfo                *pfnGetTableInfo;
    VTFNSetTableInfo                *pfnSetTableInfo;
    VTFNGotoBookmark                *pfnGotoBookmark;
    VTFNGotoIndexBookmark           *pfnGotoIndexBookmark;
    VTFNGotoPosition                *pfnGotoPosition;
    VTFNMakeKey                     *pfnMakeKey;
    VTFNMove                        *pfnMove;
    VTFNSetCursorFilter             *pfnSetCursorFilter;
    VTFNPrepareUpdate               *pfnPrepareUpdate;
    VTFNRenameColumn                *pfnRenameColumn;
    VTFNRenameIndex                 *pfnRenameIndex;
    VTFNRetrieveColumn              *pfnRetrieveColumn;
    VTFNRetrieveColumns             *pfnRetrieveColumns;
    VTFNRetrieveKey                 *pfnRetrieveKey;
    VTFNSeek                        *pfnSeek;
    VTFNPrereadKeys                 *pfnPrereadKeys;
    VTFNPrereadIndexRanges          *pfnPrereadIndexRanges;
    VTFNSetCurrentIndex             *pfnSetCurrentIndex;
    VTFNSetColumn                   *pfnSetColumn;
    VTFNSetColumns                  *pfnSetColumns;
    VTFNSetIndexRange               *pfnSetIndexRange;
    VTFNUpdate                      *pfnUpdate;
    VTFNGetLock                     *pfnGetLock;
    VTFNRegisterCallback            *pfnRegisterCallback;
    VTFNUnregisterCallback          *pfnUnregisterCallback;
    VTFNSetLS                       *pfnSetLS;
    VTFNGetLS                       *pfnGetLS;
    VTFNIndexRecordCount            *pfnIndexRecordCount;
    VTFNRetrieveTaggedColumnList    *pfnRetrieveTaggedColumnList;
    VTFNSetSequential               *pfnSetSequential;
    VTFNResetSequential             *pfnResetSequential;
    VTFNEnumerateColumns            *pfnEnumerateColumns;
    VTFNGetRecordSize               *pfnGetRecordSize;
    VTFNPrereadIndexRange           *pfnPrereadIndexRange;
    VTFNRetrieveColumnByReference   *pfnRetrieveColumnByReference;
    VTFNPrereadColumnsByReference   *pfnPrereadColumnsByReference;
    VTFNStreamRecords               *pfnStreamRecords;
} VTFNDEF;


/* The following entry points are to be used by VT providers */
/* in their VTFNDEF structures for any function that is not */
/* provided.  This functions returns JET_errIllegalOperation */


extern VTFNAddColumn                    ErrIllegalAddColumn;
extern VTFNCloseTable                   ErrIllegalCloseTable;
extern VTFNComputeStats                 ErrIllegalComputeStats;
extern VTFNCreateIndex                  ErrIllegalCreateIndex;
extern VTFNDelete                       ErrIllegalDelete;
extern VTFNDeleteColumn                 ErrIllegalDeleteColumn;
extern VTFNDeleteIndex                  ErrIllegalDeleteIndex;
extern VTFNDupCursor                    ErrIllegalDupCursor;
extern VTFNEscrowUpdate                 ErrIllegalEscrowUpdate;
extern VTFNGetBookmark                  ErrIllegalGetBookmark;
extern VTFNGetIndexBookmark             ErrIllegalGetIndexBookmark;
extern VTFNGetChecksum                  ErrIllegalGetChecksum;
extern VTFNGetCurrentIndex              ErrIllegalGetCurrentIndex;
extern VTFNGetCursorInfo                ErrIllegalGetCursorInfo;
extern VTFNGetRecordPosition            ErrIllegalGetRecordPosition;
extern VTFNGetTableColumnInfo           ErrIllegalGetTableColumnInfo;
extern VTFNGetTableIndexInfo            ErrIllegalGetTableIndexInfo;
extern VTFNGetTableInfo                 ErrIllegalGetTableInfo;
extern VTFNSetTableInfo                 ErrIllegalSetTableInfo;
extern VTFNGotoBookmark                 ErrIllegalGotoBookmark;
extern VTFNGotoIndexBookmark            ErrIllegalGotoIndexBookmark;
extern VTFNGotoPosition                 ErrIllegalGotoPosition;
extern VTFNMakeKey                      ErrIllegalMakeKey;
extern VTFNMove                         ErrIllegalMove;
extern VTFNSetCursorFilter              ErrIllegalSetCursorFilter;
extern VTFNPrepareUpdate                ErrIllegalPrepareUpdate;
extern VTFNRenameColumn                 ErrIllegalRenameColumn;
extern VTFNRenameIndex                  ErrIllegalRenameIndex;
extern VTFNRetrieveColumn               ErrIllegalRetrieveColumn;
extern VTFNRetrieveColumns              ErrIllegalRetrieveColumns;
extern VTFNRetrieveKey                  ErrIllegalRetrieveKey;
extern VTFNSeek                         ErrIllegalSeek;
extern VTFNPrereadKeys                  ErrIllegalPrereadKeys;
extern VTFNPrereadIndexRanges           ErrIllegalPrereadIndexRanges;
extern VTFNSetCurrentIndex              ErrIllegalSetCurrentIndex;
extern VTFNSetColumn                    ErrIllegalSetColumn;
extern VTFNSetColumns                   ErrIllegalSetColumns;
extern VTFNSetIndexRange                ErrIllegalSetIndexRange;
extern VTFNUpdate                       ErrIllegalUpdate;
extern VTFNGetLock                      ErrIllegalGetLock;
extern VTFNRegisterCallback             ErrIllegalRegisterCallback;
extern VTFNUnregisterCallback           ErrIllegalUnregisterCallback;
extern VTFNSetLS                        ErrIllegalSetLS;
extern VTFNGetLS                        ErrIllegalGetLS;
extern VTFNIndexRecordCount             ErrIllegalIndexRecordCount;
extern VTFNRetrieveTaggedColumnList     ErrIllegalRetrieveTaggedColumnList;
extern VTFNSetSequential                ErrIllegalSetSequential;
extern VTFNResetSequential              ErrIllegalResetSequential;
extern VTFNEnumerateColumns             ErrIllegalEnumerateColumns;
extern VTFNGetRecordSize                ErrIllegalGetRecordSize;
extern VTFNPrereadIndexRange            ErrIllegalPrereadIndexRange;
extern VTFNRetrieveColumnByReference    ErrIllegalRetrieveColumnByReference;
extern VTFNPrereadColumnsByReference    ErrIllegalPrereadColumnsByReference;
extern VTFNStreamRecords                ErrIllegalStreamRecords;


/*  The following APIs are VT APIs are are dispatched using the TABLEID parameter
 *  and there is an entry in VTFNDEF.
 */

#ifdef DEBUG
BOOL FFUCBValidTableid( const JET_SESID sesid, const JET_TABLEID tableid ); // Exported from FUCB.CXX
#endif

INLINE VOID ValidateTableid( JET_SESID sesid, JET_TABLEID tableid )
{
    AssertSzRTL( 0 != tableid, "Invalid (NULL) Table ID parameter." );
    AssertSzRTL( JET_tableidNil != tableid, "Invalid (JET_tableidNil) Table ID parameter." );
    Assert( FFUCBValidTableid( sesid, tableid ) );
}

__forceinline ERR VTAPI ErrDispAddColumn(
    JET_SESID           sesid,
    JET_TABLEID         tableid,
    const char          *szColumn,
    const JET_COLUMNDEF *pcolumndef,
    const void          *pvDefault,
    ULONG       cbDefault,
    JET_COLUMNID        *pcolumnid )
{
    ProbeClientBuffer( pcolumnid, sizeof(JET_COLUMNID), fTrue );

    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnAddColumn( sesid, tableid, szColumn, pcolumndef, pvDefault, cbDefault, pcolumnid );

    return err;
}


__forceinline ERR VTAPI ErrDispCloseTable(
    JET_SESID       sesid,
    JET_TABLEID     tableid )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnCloseTable( sesid, tableid );

    return err;
}

__forceinline ERR VTAPI ErrDispComputeStats(
    JET_SESID       sesid,
    JET_TABLEID     tableid )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnComputeStats( sesid, tableid );

    return err;
}

__forceinline ERR VTAPI ErrDispCopyBookmarks(
    JET_SESID       sesid,
    JET_TABLEID     tableidSrc,
    JET_TABLEID     tableidDest,
    JET_COLUMNID    columnidDest,
    ULONG   crecMax )
{
    ValidateTableid( sesid, tableidSrc );
    ValidateTableid( sesid, tableidDest );

//  const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableidSrc );
//  const ERR       err         = pvtfndef->pfnCopyBookmarks( sesid, tableidSrc, tableidDest, columnidDest, crecMax );
    const ERR       err     = JET_errFeatureNotAvailable;

    return err;
}

__forceinline ERR VTAPI ErrDispCreateIndex(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_INDEXCREATE3_A  *pindexcreate,
    ULONG   cIndexCreate )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnCreateIndex( sesid, tableid, pindexcreate, cIndexCreate );

    return err;
}

__forceinline ERR VTAPI ErrDispDelete(
    JET_SESID       sesid,
    JET_TABLEID     tableid )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnDelete( sesid, tableid );

    return err;
}

__forceinline ERR VTAPI ErrDispDeleteColumn(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szColumn,
    const JET_GRBIT grbit )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnDeleteColumn( sesid, tableid, szColumn, grbit );

    return err;
}

__forceinline ERR VTAPI ErrDispDeleteIndex(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnDeleteIndex( sesid, tableid, szIndexName );

    return err;
}

__forceinline ERR VTAPI ErrDispDupCursor(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_TABLEID     *ptableid,
    JET_GRBIT       grbit )
{
    ProbeClientBuffer( ptableid, sizeof(JET_TABLEID), fFalse );

    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnDupCursor( sesid, tableid, ptableid, grbit );

    return err;
}

__forceinline ERR VTAPI ErrDispGetBookmark(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    VOID * const    pvBookmark,
    const ULONG     cbMax,
    ULONG * const   pcbActual )
{
    ProbeClientBuffer( pvBookmark, cbMax );
    ProbeClientBuffer( pcbActual, sizeof(ULONG), fTrue );

    ValidateTableid( sesid, tableid );

    const VTFNDEF * pvtfndef    = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnGetBookmark( sesid, tableid, pvBookmark, cbMax, pcbActual );

    return err;
}

__forceinline ERR VTAPI ErrDispGetIndexBookmark(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    VOID * const    pvSecondaryKey,
    const ULONG     cbSecondaryKeyMax,
    ULONG * const   pcbSecondaryKeyActual,
    VOID * const    pvPrimaryBookmark,
    const ULONG     cbPrimaryBookmarkMax,
    ULONG * const   pcbPrimaryBookmarkActual )
{
    ProbeClientBuffer( pvSecondaryKey, cbSecondaryKeyMax );
    ProbeClientBuffer( pcbSecondaryKeyActual, sizeof(ULONG), fTrue );
    ProbeClientBuffer( pvPrimaryBookmark, cbPrimaryBookmarkMax );
    ProbeClientBuffer( pcbPrimaryBookmarkActual, sizeof(ULONG), fTrue );

    ValidateTableid( sesid, tableid );

    const VTFNDEF * pvtfndef    = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnGetIndexBookmark(
                                                    sesid,
                                                    tableid,
                                                    pvSecondaryKey,
                                                    cbSecondaryKeyMax,
                                                    pcbSecondaryKeyActual,
                                                    pvPrimaryBookmark,
                                                    cbPrimaryBookmarkMax,
                                                    pcbPrimaryBookmarkActual );
    return err;
}

__forceinline ERR VTAPI ErrDispGetChecksum(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    ULONG   *pChecksum )
{
    ProbeClientBuffer( pChecksum, sizeof(ULONG) );

    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnGetChecksum( sesid, tableid, pChecksum );

    return err;
}

__forceinline ERR VTAPI ErrDispGetCurrentIndex(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    _Out_z_bytecap_(cchIndexName) PSTR szIndexName,
    ULONG   cchIndexName )
{
    ProbeClientBuffer( szIndexName, cchIndexName );

    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnGetCurrentIndex( sesid, tableid, szIndexName, cchIndexName );
    return err;
}

__forceinline ERR VTAPI ErrDispGetCursorInfo(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    void            *pvResult,
    ULONG   cbMax,
    ULONG   lInfoLevel )
{
    ProbeClientBuffer( pvResult, cbMax );

    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnGetCursorInfo( sesid, tableid, pvResult, cbMax, lInfoLevel );

    return err;
}

__forceinline ERR VTAPI ErrDispGetRecordPosition(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_RECPOS      *pkeypos,
    ULONG   cbKeypos )
{
    ProbeClientBuffer( pkeypos, cbKeypos );

    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnGetRecordPosition( sesid, tableid, pkeypos, cbKeypos );

    return err;
}

__forceinline ERR VTAPI ErrDispGetTableColumnInfo(
    JET_SESID           sesid,
    JET_TABLEID         tableid,
    const char          *szColumnName,
    const JET_COLUMNID  *pcolid,
    void                *pvResult,
    ULONG       cbMax,
    ULONG       lInfoLevel,
    const BOOL          fUnicodeNames )
{
    ProbeClientBuffer( pvResult, cbMax );

    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnGetTableColumnInfo( sesid, tableid, szColumnName, pcolid, pvResult, cbMax, lInfoLevel, fUnicodeNames );

    return err;
}

__forceinline ERR VTAPI ErrDispGetTableIndexInfo(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndexName,
    void            *pvResult,
    ULONG   cbMax,
    ULONG   lInfoLevel,
    const BOOL      fUnicodeNames )
{
    ProbeClientBuffer( pvResult, cbMax );

    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnGetTableIndexInfo( sesid, tableid, szIndexName, pvResult, cbMax, lInfoLevel, fUnicodeNames );

    return err;
}

__forceinline ERR VTAPI ErrDispGetTableInfo(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    void            *pvResult,
    ULONG   cbMax,
    ULONG   lInfoLevel )
{
    ProbeClientBuffer( pvResult, cbMax );

    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnGetTableInfo( sesid, tableid, pvResult, cbMax, lInfoLevel );

    return err;
}

__forceinline ERR VTAPI ErrDispSetTableInfo(
    JET_SESID sesid,
    JET_TABLEID tableid,
    _In_reads_bytes_opt_(cbParam) const void  *pvParam,
    ULONG cbParam,
    ULONG InfoLevel)
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnSetTableInfo( sesid, tableid, pvParam, cbParam, InfoLevel );

    return err;
}

__forceinline ERR VTAPI ErrDispGotoBookmark(
    JET_SESID           sesid,
    JET_TABLEID         tableid,
    const VOID * const  pvBookmark,
    const ULONG         cbBookmark )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF *     pvtfndef    = *( (VTFNDEF **)tableid );
    const ERR           err         = pvtfndef->pfnGotoBookmark( sesid, tableid, pvBookmark, cbBookmark );

    return err;
}

__forceinline ERR VTAPI ErrDispGotoIndexBookmark(
    JET_SESID           sesid,
    JET_TABLEID         tableid,
    const VOID * const  pvSecondaryKey,
    const ULONG         cbSecondaryKey,
    const VOID * const  pvPrimaryBookmark,
    const ULONG         cbPrimaryBookmark,
    const JET_GRBIT     grbit )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF *     pvtfndef    = *( (VTFNDEF **)tableid );
    const ERR           err         = pvtfndef->pfnGotoIndexBookmark(
                                                    sesid,
                                                    tableid,
                                                    pvSecondaryKey,
                                                    cbSecondaryKey,
                                                    pvPrimaryBookmark,
                                                    cbPrimaryBookmark,
                                                    grbit );
    return err;
}

__forceinline ERR VTAPI ErrDispGotoPosition(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_RECPOS      *precpos )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnGotoPosition( sesid, tableid, precpos );

    return err;
}

__forceinline ERR VTAPI ErrDispMakeKey(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const VOID*     pvData,
    const ULONG     cbData,
    JET_GRBIT       grbit )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF*  pvtfndef    = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnMakeKey( sesid, tableid, pvData, cbData, grbit );

    return err;
}

__forceinline ERR VTAPI ErrDispMove(
    JET_SESID           sesid,
    JET_TABLEID         tableid,
    LONG                cRow,
    JET_GRBIT           grbit )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF       *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR           err         = pvtfndef->pfnMove( sesid, tableid, cRow, grbit );

    return err;
}

__forceinline ERR VTAPI ErrDispSetCursorFilter(
    JET_SESID           sesid,
    JET_TABLEID         tableid,
    JET_INDEX_COLUMN*   rgFilters,
    DWORD               cFilters,
    JET_GRBIT           grbit )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF       *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR           err         = pvtfndef->pfnSetCursorFilter( sesid, tableid, rgFilters, cFilters, grbit );

    return err;
}

__forceinline ERR VTAPI ErrDispGetLock(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_GRBIT       grbit )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnGetLock( sesid, tableid, grbit );

    return err;
}


__forceinline ERR VTAPI ErrDispPrepareUpdate(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    ULONG   prep )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnPrepareUpdate( sesid, tableid, prep );

    return err;
}

__forceinline ERR VTAPI ErrDispRenameColumn(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szColumn,
    const char      *szColumnNew,
    const JET_GRBIT grbit )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnRenameColumn( sesid, tableid, szColumn, szColumnNew, grbit );

    return err;
}

__forceinline ERR VTAPI ErrDispRenameIndex(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    const char      *szIndex,
    const char      *szIndexNew )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnRenameIndex( sesid, tableid, szIndex, szIndexNew );
    return err;
}

__forceinline ERR VTAPI ErrDispRetrieveColumn(
    _In_ JET_SESID                                                          sesid,
    _In_ JET_TABLEID                                                        tableid,
    _In_ JET_COLUMNID                                                       columnid,
    _Out_writes_bytes_to_opt_( cbData, min( cbData, *pcbActual ) ) VOID*    pvData,
    _In_ const ULONG                                                        cbData,
    _Out_opt_ ULONG*                                                        pcbActual,
    _In_ JET_GRBIT                                                          grbit,
    _Inout_opt_ JET_RETINFO*                                                pretinfo )
{
    ProbeClientBuffer( pvData, cbData, fTrue );
    ProbeClientBuffer( pcbActual, sizeof(ULONG), fTrue );

    ValidateTableid( sesid, tableid );

    ULONG           cbActualTmp = 0;
    const VTFNDEF*  pvtfndef    = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnRetrieveColumn( sesid, tableid, columnid, pvData, cbData, &cbActualTmp, grbit, pretinfo );

    // set the output value
    if ( pcbActual )
    {
        *pcbActual = cbActualTmp;
    }

    // Historical Note:

    // prior to E14SP1/Win8, we would (in debug) always smash the entire buffer to help validate that the buffer promised
    // was entirely there. Some clients using only retail builds have adapted to a programming model where they 
    // initialize the contents of pvData with a default value; and then they ignore the errors: JET_wrnColumnNull
    // and JET_errColumnNotFound. Now smash none of the buffer for those two special cases, all of the buffer for
    // other errors, and just the remainder of the buffer for positive cases.

    // In E15, we changed from FillClientBuffer (modifying the buffer in debug builds only) to
    // ProbeClientBuffer (touching the buffer and restoring the previous contents).

    
    return err;
}

__forceinline ERR VTAPI ErrDispRetrieveKey(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    VOID*           pvKey,
    const ULONG     cbMax,
    ULONG*          pcbActual,
    JET_GRBIT       grbit )
{
    ProbeClientBuffer( pvKey, cbMax );
    ProbeClientBuffer( pcbActual, sizeof(ULONG), fTrue );

    ValidateTableid( sesid, tableid );

    const VTFNDEF*  pvtfndef    = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnRetrieveKey( sesid, tableid, pvKey, cbMax, pcbActual, grbit );

    Assert( err != JET_wrnColumnNull && err != JET_errColumnNotFound );
    
    return err;
}

__forceinline ERR VTAPI ErrDispSeek(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_GRBIT       grbit )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnSeek( sesid, tableid, grbit );

    return err;
}

__forceinline ERR VTAPI ErrDispPrereadKeys(
    __in const JET_SESID                                sesid,
    __in const JET_TABLEID                              tableid,
    __in_ecount(ckeys) const void * const * const       rgpvKeys,
    __in_ecount(ckeys) const ULONG * const      rgcbKeys,
    __in const LONG                                     ckeys,
    __out_opt LONG * const                              pckeysPreread,
    __in const JET_GRBIT                                grbit )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   * const pvtfndef    = *( (VTFNDEF **)tableid );
    const ERR       err                 = pvtfndef->pfnPrereadKeys( sesid, tableid, rgpvKeys, rgcbKeys, ckeys, pckeysPreread, grbit );
    
    return err;
}

__forceinline ERR VTAPI ErrDispPrereadIndexRanges(
    __in const JET_SESID                                sesid,
    __in const JET_TABLEID                              tableid,
    __in_ecount(cIndexRanges) const JET_INDEX_RANGE * const rgIndexRanges,
    __in const ULONG                            cIndexRanges,
    __out_opt ULONG * const                     pcRangesPreread,
    __in_ecount(ccolumnidPreread) const JET_COLUMNID * const rgcolumnidPreread,
    __in const ULONG                            ccolumnidPreread,
    __in const JET_GRBIT                                grbit )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   * const pvtfndef    = *( (VTFNDEF **)tableid );
    const ERR       err                 = pvtfndef->pfnPrereadIndexRanges( sesid, tableid, rgIndexRanges, cIndexRanges, pcRangesPreread, rgcolumnidPreread, ccolumnidPreread, grbit );
    
    return err;
}

__forceinline ERR VTAPI ErrDispSetCurrentIndex(
    JET_SESID           sesid,
    JET_VTID            tableid,
    const CHAR          *szIndexName,
    const JET_INDEXID   *pindexid,
    const JET_GRBIT     grbit,
    const ULONG         itagSequence )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF       *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR           err         = pvtfndef->pfnSetCurrentIndex(
                                                    sesid,
                                                    tableid,
                                                    szIndexName,
                                                    pindexid,
                                                    grbit,
                                                    itagSequence );

    return err;
}

__forceinline ERR VTAPI ErrDispSetColumn(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_COLUMNID    columnid,
    const VOID*     pvData,
    const ULONG     cbData,
    JET_GRBIT       grbit,
    JET_SETINFO*    psetinfo )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF*  pvtfndef    = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnSetColumn( sesid, tableid, columnid, pvData, cbData, grbit, psetinfo );

    return err;
}

__forceinline ERR VTAPI ErrDispSetColumns(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_SETCOLUMN   *psetcols,
    ULONG   csetcols )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnSetColumns( sesid, tableid, psetcols, csetcols );

    return err;
}

__forceinline ERR VTAPI ErrDispRetrieveColumns(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_RETRIEVECOLUMN  *pretcols,
    ULONG   cretcols )
{
    ValidateTableid( sesid, tableid );

    // In the error case, not all err/cbActual fields are initialized by pfnRetrieveColumns. 
    // So we pre-initialize all the errors to failures, and then only use cbData and pvData in the
    // error case.
    for ( ULONG iretcol = 0; iretcol < cretcols; iretcol++ )
    {
#ifdef DEBUG
        pretcols[ iretcol ].err = JET_errInternalError;
#endif // DEBUG
        ProbeClientBuffer( pretcols[ iretcol ].pvData, pretcols[ iretcol ].cbData, fTrue );
    }

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnRetrieveColumns( sesid, tableid, pretcols, cretcols );

    // Historical note:
    // AD was broken by FillClientBuffer() since they were retrieving non-null terminated string from the database in a null buffer

    return err;
}

__forceinline ERR VTAPI ErrDispEnumerateColumns(
    JET_SESID               sesid,
    JET_TABLEID             tableid,
    ULONG           cEnumColumnId,
    JET_ENUMCOLUMNID*       rgEnumColumnId,
    ULONG*          pcEnumColumn,
    JET_ENUMCOLUMN**        prgEnumColumn,
    JET_PFNREALLOC          pfnRealloc,
    void*                   pvReallocContext,
    ULONG           cbDataMost,
    JET_GRBIT               grbit )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnEnumerateColumns(    sesid,
                                                                    tableid,
                                                                    cEnumColumnId,
                                                                    rgEnumColumnId,
                                                                    pcEnumColumn,
                                                                    prgEnumColumn,
                                                                    pfnRealloc,
                                                                    pvReallocContext,
                                                                    cbDataMost,
                                                                    grbit );

    return err;
}

__forceinline ERR VTAPI ErrDispGetRecordSize(
    JET_SESID           sesid,
    JET_TABLEID         tableid,
    JET_RECSIZE3 *      precsize,
    const JET_GRBIT     grbit )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF *     pvtfndef    = *( (VTFNDEF **)tableid );
    const ERR           err         = pvtfndef->pfnGetRecordSize( sesid, tableid, precsize, grbit );

    return err;
}

__forceinline ERR VTAPI ErrDispSetIndexRange(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_GRBIT       grbit )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnSetIndexRange( sesid, tableid, grbit );

    return err;
}

__forceinline ERR VTAPI ErrDispUpdate(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    void            *pvBookmark,
    ULONG   cbBookmark,
    ULONG   *pcbActual,
    const JET_GRBIT grbit )
{
    ProbeClientBuffer( pvBookmark, cbBookmark, fTrue );
    ProbeClientBuffer( pcbActual, sizeof(ULONG), fTrue );

    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnUpdate( sesid, tableid, pvBookmark, cbBookmark, pcbActual, grbit );

    return err;
}

__forceinline ERR VTAPI ErrDispEscrowUpdate(
    JET_SESID       sesid,
    JET_TABLEID     tableid,
    JET_COLUMNID    columnid,
    void            *pv,
    ULONG   cbMax,
    void            *pvOld,
    ULONG   cbOldMax,
    ULONG   *pcbOldActual,
    JET_GRBIT       grbit )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnEscrowUpdate( sesid, tableid, columnid, pv, cbMax, pvOld, cbOldMax, pcbOldActual, grbit );

    return err;
}


__forceinline ERR VTAPI ErrDispRegisterCallback(
    JET_SESID           sesid,
    JET_TABLEID         tableid,
    JET_CBTYP           cbtyp,
    JET_CALLBACK            pCallback,
    VOID *                  pvContext,
    JET_HANDLE          *phCallbackId )
{
    ValidateTableid( sesid, tableid );
    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnRegisterCallback(
                                    sesid,
                                    tableid,
                                    cbtyp,
                                    pCallback,
                                    pvContext,
                                    phCallbackId );
    return err;
}

__forceinline ERR VTAPI ErrDispUnregisterCallback(
    JET_SESID               sesid,
    JET_TABLEID             tableid,
    JET_CBTYP               cbtyp,
    JET_HANDLE              hCallbackId )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnUnregisterCallback( sesid, tableid, cbtyp, hCallbackId );

    return err;
}

__forceinline ERR VTAPI ErrDispSetLS(
    JET_SESID               sesid,
    JET_TABLEID             tableid,
    JET_LS                  ls,
    JET_GRBIT               grbit )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnSetLS( sesid, tableid, ls, grbit );

    return err;
}

__forceinline ERR VTAPI ErrDispGetLS(
    JET_SESID               sesid,
    JET_TABLEID             tableid,
    JET_LS                  *pls,
    JET_GRBIT               grbit )
{
    ProbeClientBuffer( pls, sizeof(JET_LS) );

    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnGetLS( sesid, tableid, pls, grbit );

    return err;
}

__forceinline ERR VTAPI ErrDispIndexRecordCount(
    JET_SESID sesid, JET_VTID tableid, ULONG64 *pcrec,
    ULONG64 crecMax )
{
    ProbeClientBuffer( pcrec, sizeof(ULONG64) );

    ValidateTableid( sesid, tableid );

    const VTFNDEF   *pvtfndef   = *( (VTFNDEF **)tableid );
    const ERR       err         = pvtfndef->pfnIndexRecordCount( sesid, tableid, pcrec, crecMax );

    return err;
}

__forceinline ERR VTAPI ErrDispRetrieveTaggedColumnList(
    JET_SESID           vsesid,
    JET_VTID            vtid,
    ULONG               *pcentries,
    VOID                *pv,
    const ULONG         cbMax,
    const JET_COLUMNID  columnidStart,
    const JET_GRBIT     grbit )
{
    ProbeClientBuffer( pv, cbMax );

    ValidateTableid( vsesid, vtid );

    const VTFNDEF       *pvtfndef   = *( (VTFNDEF **)vtid );
    const ERR           err         = pvtfndef->pfnRetrieveTaggedColumnList(
                                                    vsesid,
                                                    vtid,
                                                    pcentries,
                                                    pv,
                                                    cbMax,
                                                    columnidStart,
                                                    grbit );

    return err;
}


//  ================================================================
__forceinline ERR VTAPI ErrDispSetTableSequential(
    const JET_SESID     sesid,
    const JET_TABLEID   tableid,
    const JET_GRBIT     grbit )
//  ================================================================
{
    Assert( FFUCBValidTableid( sesid, tableid ) );

    const VTFNDEF   * const pvtfndef    = *( (VTFNDEF **)tableid );
    const ERR       err                 = pvtfndef->pfnSetSequential( sesid, tableid, grbit );

    return err;
}


//  ================================================================
__forceinline ERR VTAPI ErrDispResetTableSequential(
    const JET_SESID     sesid,
    const JET_TABLEID   tableid,
    const JET_GRBIT     grbit )
//  ================================================================
{
    Assert( FFUCBValidTableid( sesid, tableid ) );

    const VTFNDEF   * const pvtfndef    = *( (VTFNDEF **)tableid );
    const ERR       err                 = pvtfndef->pfnResetSequential( sesid, tableid, grbit );

    return err;
}

__forceinline ERR VTAPI ErrDispPrereadIndexRange(
    __in const JET_SESID                sesid,
    __in const JET_TABLEID              tableid,
    __in const JET_INDEX_RANGE * const  pIndexRange,
    __in const ULONG            cPageCacheMin,
    __in const ULONG            cPageCacheMax,
    __in const JET_GRBIT                grbit,
    __out_opt ULONG * const     pcPageCacheActual )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   * const pvtfndef = *( (VTFNDEF **)tableid );
    const ERR       err = pvtfndef->pfnPrereadIndexRange( sesid, tableid, pIndexRange, cPageCacheMin, cPageCacheMax, grbit, pcPageCacheActual );

    return err;
}

__forceinline ERR VTAPI ErrDispRetrieveColumnByReference(
    _In_ const JET_SESID                                                        sesid,
    _In_ const JET_TABLEID                                                      tableid,
    _In_reads_bytes_( cbReference ) const void * const                          pvReference,
    _In_ const ULONG                                                    cbReference,
    _In_ const ULONG                                                    ibData,
    _Out_writes_bytes_to_opt_( cbData, min( cbData, *pcbActual ) ) void * const pvData,
    _In_ const ULONG                                                    cbData,
    _Out_opt_ ULONG * const                                             pcbActual,
    _In_ const JET_GRBIT                                                        grbit )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   * const pvtfndef = *( (VTFNDEF **)tableid );
    const ERR       err = pvtfndef->pfnRetrieveColumnByReference( sesid, tableid, pvReference, cbReference, ibData, pvData, cbData, pcbActual, grbit );

    return err;
}

__forceinline ERR VTAPI ErrDispPrereadColumnsByReference(
    _In_ const JET_SESID                                    sesid,
    _In_ const JET_TABLEID                                  tableid,
    _In_reads_( cReferences ) const void * const * const    rgpvReferences,
    _In_reads_( cReferences ) const ULONG * const   rgcbReferences,
    _In_ const ULONG                                cReferences,
    _In_ const ULONG                                cPageCacheMin,
    _In_ const ULONG                                cPageCacheMax,
    _Out_opt_ ULONG * const                         pcReferencesPreread,
    _In_ const JET_GRBIT                                    grbit )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   * const pvtfndef = *( (VTFNDEF **)tableid );
    const ERR       err = pvtfndef->pfnPrereadColumnsByReference( sesid, tableid, rgpvReferences, rgcbReferences, cReferences, cPageCacheMin, cPageCacheMax, pcReferencesPreread, grbit );

    return err;
}

__forceinline ERR VTAPI ErrDispStreamRecords(
    _In_ JET_SESID                                                  sesid,
    _In_ JET_TABLEID                                                tableid,
    _In_ const ULONG                                        ccolumnid,
    _In_reads_opt_( ccolumnid ) const JET_COLUMNID * const          rgcolumnid,
    _Out_writes_bytes_to_opt_( cbData, *pcbActual ) void * const    pvData,
    _In_ const ULONG                                        cbData,
    _Out_opt_ ULONG * const                                 pcbActual,
    _In_ const JET_GRBIT                                            grbit )
{
    ValidateTableid( sesid, tableid );

    const VTFNDEF   * const pvtfndef = *( (VTFNDEF **)tableid );
    const ERR       err = pvtfndef->pfnStreamRecords( sesid, tableid, ccolumnid, rgcolumnid, pvData, cbData, pcbActual, grbit );

    return err;
}

/*

System can be in one of the 3 states:

1. multi-instance enabled
    Use:  JetEnableMultiInstance to set global-default parameters
    Use:  JetCreateInstance to allocate an instance
    Use:  JetSetSystemParameter with not null instance to set param per instance
    Use:  JetInit with not null pinstance to initialize it (or allocate and inititalize if *pinstance == 0)
    Error:  JetSetSystemParameter with null instance (try to set global param
    Error:  JetInit with null pinstance (default instance can be find only in one-instance mode)

2. one-instance enabled
    Use:  JetSetSystemParameter with null instance to set param per instance
    Use:  JetSetSystemParameter with not null instance to set param per instance
    Use:  JetInit with to initialize the instance (the second call before JetTerm will fail)
    Error:  JetEnableMultiInstance
    Error:  JetCreateInstance

3. mode not set
    no instance is started (initial state and state after last running instance calls JetTerm (g_cpinstInit == 0)

If the mode is not set, the first function call specific for one of the modes will set it
*/
typedef enum { runInstModeNoSet, runInstModeOneInst, runInstModeMultiInst} RUNINSTMODE;
extern RUNINSTMODE g_runInstMode;


// ------------------------------------------------------------------------------------------------
//
//  err.lib functions that ESE needs
//

JET_ERRCAT ErrcatERRLeastSpecific( const JET_ERR err );

#endif /* !_JET_H */

