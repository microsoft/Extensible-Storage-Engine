// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"


THREADSTATSCOUNTERS g_tscountersOLD;

#ifdef PERFMON_SUPPORT

PERFInstanceLiveTotal<> cOLDPagesFreed;
PERFInstanceLiveTotal<> cOLDPartialMerges;
PERFInstanceLiveTotal<> cOLDPagesMoved;

PERFInstanceLiveTotal<> cOLDTasksPosted;
PERFInstanceLiveTotal<> cOLDTasksCompleted;
PERFInstanceDelayedTotal<> cOLDTasksRunning;
PERFInstanceDelayedTotal<> cOLDTasksRegistered;
PERFInstanceDelayedTotal<> cOLDTasksPostponed;

LONG LOLDPageReadCEFLPv( LONG iInstance, VOID * pvBuf )
{
    g_tscountersOLD.cPageRead.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LOLDPagesFreedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cOLDPagesFreed.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LOLDPartialMergesCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cOLDPartialMerges.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LOLDPagesMovedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cOLDPagesMoved.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LOLDPagePrereadCEFLPv( LONG iInstance, VOID * pvBuf )
{
    g_tscountersOLD.cPagePreread.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LOLDPageDirtiedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    g_tscountersOLD.cPageDirtied.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LOLDLogBytesCEFLPv( LONG iInstance, VOID * pvBuf )
{
    g_tscountersOLD.cbLogRecord.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LOLDTasksPostedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cOLDTasksPosted.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LOLDTasksCompletedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cOLDTasksCompleted.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LOLDTasksRunningCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cOLDTasksRunning.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LOLDTasksRegisteredCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cOLDTasksRegistered.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LOLDTasksPostponedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cOLDTasksPostponed.PassTo( iInstance, pvBuf );
    return 0;
}

#endif

class RECCHECKOLD : public RECCHECK
{
    protected:
        RECCHECKOLD( const PGNO pgnoFDP, const IFMP ifmp, FUCB * pfucb, INST * const pinst );

    protected:
        const PGNO m_pgnoFDP;
        const IFMP m_ifmp;
        FUCB * const m_pfucb;
        INST * const m_pinst;

    private:
        RECCHECKOLD( const RECCHECKOLD& );
        RECCHECKOLD& operator=( const RECCHECKOLD& );
};


template< typename TDelta >
class RECCHECKFINALIZE : public RECCHECKOLD
{
    public:
        RECCHECKFINALIZE(   const FID fid,
                            const USHORT ibRecordOffset,
                            const PGNO pgnoFDP,
                            const IFMP ifmp,
                            FUCB * const pfucb,
                            INST * const pinst,
                            const BOOL fCallback,
                            const BOOL fDelete );
        ERR operator()( const KEYDATAFLAGS& kdf, const PGNO pgno );

    protected:
        const FID       m_fid;
        const USHORT    m_ibRecordOffset;
        const BYTE      m_fCallback;
        const BYTE      m_fDelete;

    private:
        RECCHECKFINALIZE( const RECCHECKFINALIZE& );
        RECCHECKFINALIZE& operator=( const RECCHECKFINALIZE& );
};


class RECCHECKDELETELV : public RECCHECKOLD
{
    public:
        RECCHECKDELETELV( const PGNO pgnoFDP, const IFMP ifmp, FUCB * const pfucb, INST * const pinst );
        ERR operator()( const KEYDATAFLAGS& kdf, const PGNO pgno );

    private:
        RECCHECKDELETELV( const RECCHECKDELETELV& );
        RECCHECKDELETELV& operator=( RECCHECKDELETELV& );
};

class OLD2_STATUS
{
    public:
        OLD2_STATUS( const OBJID objidTable, const OBJID objidFDP );
        ~OLD2_STATUS() {}

    public:
        static const CHAR * SzTableName();

        static ERR ErrOpenTable(
            __in PIB * const        ppib,
            const IFMP              ifmp,
            __out FUCB ** const     ppfucb );

        static ERR ErrOpenOrCreateTable(
            __in PIB * const        ppib,
            const IFMP              ifmp,
            __out FUCB ** const     ppfucb );

        static ERR ErrGetObjids(
            __in PIB * const        ppib,
            __in FUCB * const       pfucb,
            __out OBJID * const     pobjidTable,
            __out OBJID * const     pobjidFDP );

        static ERR ErrDumpTable(
            __in PIB * const        ppib,
            const IFMP              ifmp );
        
        static ERR ErrSave(
            __in PIB * const        ppib,
            __in FUCB * const       pfucbDefrag,
            __in const OLD2_STATUS& old2status );

        static ERR ErrInsertNewRecord(
            _In_ PIB * const        ppib,
            _In_ FUCB * const       pfucbDefragStatusState,
            _In_ const OLD2_STATUS& old2status );

        static ERR ErrLoad(
            __in PIB * const        ppib,
            __in FUCB * const       pfucbDefrag,
            __in OLD2_STATUS&       old2status );

        static ERR ErrDelete(
            __in PIB * const        ppib,
            __in FUCB * const       pfucbDefrag,
            __in const OLD2_STATUS& old2status );

    public:
        CPG CpgVisited() const      { return m_cpgVisited; }
        VOID IncrementCpgVisited()  { ++m_cpgVisited; }

        CPG CpgFreed() const        { return m_cpgFreed; }
        VOID IncrementCpgFreed()    { ++m_cpgFreed; }

        CPG CpgPartialMerges() const        { return m_cpgPartialMerges; }
        VOID IncrementCpgPartialMerges()    { ++m_cpgPartialMerges; }

        CPG CpgMoved() const        { return m_cpgMoved; }
        VOID IncrementCpgMoved()    { ++m_cpgMoved; }
        
        OBJID ObjidTable() const    { return m_objidTable; }
        OBJID ObjidFDP() const      { return m_objidFDP; }
        
        __int64 StartTime() const   { return m_startTime; }
        VOID SetStartTime( const __int64 time );

        BOOKMARK GetBookmark();
        VOID SetBookmark( const BOOKMARK& bm );

    private:
        const OBJID m_objidTable;
        const OBJID m_objidFDP;

        __int64     m_startTime;

        CPG         m_cpgVisited;
        CPG         m_cpgFreed;
        CPG         m_cpgPartialMerges;
        CPG         m_cpgMoved;
        
        size_t      m_cbBookmarkKey;
        size_t      m_cbBookmarkData;

        static const size_t m_cbBookmark = cbBookmarkAlloc;
        BYTE                m_rgbBookmark[m_cbBookmark];

    private:
        static CCriticalSection s_crit;
        
    private:
        static const CHAR           s_szOLD2[];
        
        static const FID s_fidOLD2ObjidTable;
        static const FID s_fidOLD2ObjidFDP;
        static const FID s_fidOLD2StartDateTime;
        static const FID s_fidOLD2PagesVisited;
        static const FID s_fidOLD2PagesFreed;
        static const FID s_fidOLD2PartialMerges;
        static const FID s_fidOLD2PageMoves;
        static const FID s_fidOLD2UpdateTime;

        static const FID s_fidOLD2BookmarkKey;
        static const FID s_fidOLD2BookmarkData;
        
    private:
        static ERR ErrDumpOneRecord_(
            __in PIB * const ppib,
            __in FUCB * const pfucb );
        
        static ERR ErrCreateTable_(
            __in PIB * const ppib,
            const IFMP ifmp );
        
        static ERR ErrSetLongLong_(
            __in PIB * const ppib,
            __in FUCB * const pfucb,
            const FID fid,
            const __int64 qwValue );
        static ERR ErrGetLongLong_(
            __in PIB * const ppib,
            __in FUCB * const pfucb,
            const FID fid,
            __out __int64 * const pqwValue );
        
        static ERR ErrSetLong_(
            __in PIB * const ppib,
            __in FUCB * const pfucb,
            const FID fid,
            const LONG lValue );
        static ERR ErrGetLong_(
            __in PIB * const ppib,
            __in FUCB * const pfucb,
            const FID fid,
            __out LONG * const plValue );
        
        static ERR ErrSeek_(
            _In_ PIB * const        ppib,
            _In_ FUCB * const       pfucbDefrag,
            _In_ const OLD2_STATUS& old2status );

        static ERR ErrCheckCurrency_(
            __in PIB * const        ppib,
            __in FUCB * const       pfucbDefrag,
            __in const OLD2_STATUS& old2status );

#ifndef RTM
    private:
        static VOID Test();
        static ERR ErrLoadAndCheck( __in PIB * const ppib, __in FUCB * const pfucb, const OLD2_STATUS& old2status );
        static ERR ErrTestTable( __in PIB * const ppib, __in FUCB * const pfucb );
#endif
};

CCriticalSection OLD2_STATUS::s_crit( CLockBasicInfo( CSyncBasicInfo( "OLD2_STATUS::s_crit" ), rankOLD, 0 ) );

class CTableDefragment
{
    public:
        CTableDefragment(
            _In_ const IFMP ifmp,
            _In_z_ const char * const szTable,
            _In_opt_z_ const char * const szIndex,
            _In_ DEFRAGTYPE defragtype );
        ~CTableDefragment();

    public:
        IFMP Ifmp() const { return m_ifmp; }
        _Ret_z_ const CHAR * SzTable() const { return m_szTable; }
        _Ret_z_ const CHAR * SzIndex() const { return m_szIndex; }
        DEFRAGTYPE DefragType() { return m_defragtype; }
        
    public:
        bool FNoMoreDefrag() const;
        
        ERR ErrTerm();
        ERR ErrDefragStep();

    private:
        bool FIsInit_() const;
        ERR ErrInit_();

        ERR ErrGetInitialBookmark_();
        ERR ErrAdjustBookmark_();

        bool FIsCompleted_() const;
        VOID SetCompleted_();

        bool FTableWasDeleted_() const;

        ERR ErrDefragStep_();
        bool FShouldUpdateStatus_() const;
        ERR ErrUpdateStatus_();
        ERR ErrPerformMerges_();
        
        ERR ErrPerformOneMerge_( PrereadInfo * const pPrereadInfo );

        ERR ErrSelectDefragRange_();
        void ClearDefragRange_();

        ERR ErrSetBTreeDefragCompleted_();
        VOID LogCompletionEvent_() const;
        
    private:
        const IFMP  m_ifmp;
        CHAR        m_szTable[JET_cbNameMost+1];
        CHAR        m_szIndex[JET_cbNameMost+1];
        PIB *       m_ppib;
    
        FUCB *      m_pfucbOwningTable;
        FUCB *      m_pfucbToDefrag;
        FUCB *      m_pfucbDefragStatus;

        RECCHECK *  m_preccheck;

        bool        m_fInit;
        bool        m_fCompleted;
        DEFRAGTYPE  m_defragtype;

        bool        m_fDefragRangeSelected;
        BOOKMARK    m_bmDefragRangeStart;
        BOOKMARK    m_bmDefragRangeEnd;

        CPG     m_cpgVisitedLastUpdate;
        VOID *  m_pvBookmarkBuf;

        OLD2_STATUS * m_pold2Status;


    private:
        static const INT m_cpgUpdateThreshold = 128;

        static const INT m_cpgToPreread = 8;

    private:
        CTableDefragment( const CTableDefragment& );
        CTableDefragment& operator= ( const CTableDefragment& );
};

class CDefragTask
{
    public:
        CDefragTask();
        ~CDefragTask();

        VOID SetFIssued() { m_fIssued = true; }
        VOID ResetFIssued() { m_fIssued = false; }
        VOID SetTickEnd( ULONG tickEnd );
        VOID SetPlCompleted( LONG * const plCompleted );
        VOID SetPtabledefragment( CTableDefragment * const ptabledefragment );

        CTableDefragment * Ptabledefragment() const { return m_ptabledefragment; }

        bool FInit() const { return ( NULL != m_ptabledefragment ); }
        bool FIssued() const { return m_fIssued; }
        bool FCompleted() const { return FInit() && m_ptabledefragment->FNoMoreDefrag(); }
        
    public:
        static DWORD DispatchTask( void *pvThis );

    private:
        DWORD Task_();
        
    private:
        bool m_fIssued;
        ULONG m_tickEnd;
        LONG * m_plCompleted;
        CTableDefragment * m_ptabledefragment;
};

class CDefragManager
{
    public:
        static CDefragManager& Instance();
        static const INT s_ctasksMax = 2;
        static const char * const szCriticalSectionName;
        
    public:
        ERR ErrInit();
        VOID Term();
        
        ERR ErrExplicitRegisterTableAndChildren(
            _In_ const IFMP ifmp,
            _In_z_ const CHAR * const szTable );

        ERR ErrRegisterOneTreeOnly(
            _In_ const IFMP ifmp,
            _In_z_ const CHAR * const szTable,
            _In_opt_z_ const CHAR * const szIndex,
            _In_ DEFRAGTYPE defragtype );

        VOID DeregisterInst( const INST * const pinst );

        VOID DeregisterIfmp( const IFMP ifmp );

        ERR ErrTryAddTaskAtFreeSlot(
            _In_ const IFMP ifmp,
            _In_z_ const CHAR * const szTable,
            _In_opt_z_ const CHAR * const szIndex,
            _In_ DEFRAGTYPE defragtype );

        bool FTableIsRegistered(
            _In_ const IFMP ifmp,
            _In_z_ const CHAR * const szTable,
            _In_opt_z_ const CHAR * const szIndex,
            _In_ DEFRAGTYPE defragtype ) const;

    private:
        VOID EnsureTimerScheduled_();
        
        static VOID DispatchOsTimerTask_( VOID* const pvGroupContext, VOID* pvRuntimeContext );
        
        BOOL FOsTimerTask_();

        ERR ErrAddTask_(
            _In_ const IFMP ifmp,
            _In_z_ const CHAR * const szTable,
            _In_opt_z_ const CHAR * const szIndex,
            _In_ DEFRAGTYPE defragtype,
            _In_ const INT itask );

        VOID RemoveTask_( const INT itask, const bool fWaitForTask );

        BOOL FIssueTasks_( const INT ctasksToIssue );

    private:
        static CDefragManager s_instance;

    private:
        CCriticalSection m_crit;

        POSTIMERTASK m_posttDispatchOsTimerTask;

        bool m_fTimerScheduled;
        
        ULONG m_cmsecPeriod;

        CDefragTask * m_rgtasks;

        INT m_itaskLastIssued;
        
        const INT m_ctasksIncrement;
        
        static const size_t m_clCompleted = 32;
        LONG m_rglCompleted[m_clCompleted];

        INT m_ilCompleted;

        INT m_ctasksIssued;
        INT m_ctasksToIssue;
        INT m_ctasksToIssueNext;
        
    private:
        CDefragManager();
        ~CDefragManager();
        
        CDefragManager( const CDefragManager& );
        CDefragManager& operator= ( const CDefragManager& );
};


LOCAL const CHAR        szOLDObsolete[]                 = "MSysDefrag1";
LOCAL const CHAR        szOLD[]                         = "MSysDefrag2";

LOCAL __int64 zero = 0;

LOCAL const FID         fidOLDObjidFDP                  = fidFixedLeast;
LOCAL const FID         fidOLDStatus                    = fidFixedLeast+1;
LOCAL const FID         fidOLDPassStartDateTime         = fidFixedLeast+2;
LOCAL const FID         fidOLDPassElapsedSeconds        = fidFixedLeast+3;
LOCAL const FID         fidOLDPassInvocations           = fidFixedLeast+4;
LOCAL const FID         fidOLDPassPagesVisited          = fidFixedLeast+5;
LOCAL const FID         fidOLDPassPagesFreed            = fidFixedLeast+6;
LOCAL const FID         fidOLDPassPartialMerges         = fidFixedLeast+7;
LOCAL const FID         fidOLDTotalPasses               = fidFixedLeast+8;
LOCAL const FID         fidOLDTotalElapsedSeconds       = fidFixedLeast+9;
LOCAL const FID         fidOLDTotalInvocations          = fidFixedLeast+10;
LOCAL const FID         fidOLDTotalDefragDays           = fidFixedLeast+11;
LOCAL const FID         fidOLDTotalPagesVisited         = fidFixedLeast+12;
LOCAL const FID         fidOLDTotalPagesFreed           = fidFixedLeast+13;
LOCAL const FID         fidOLDTotalPartialMerges        = fidFixedLeast+14;

LOCAL const FID         fidOLDCurrentKey                = fidTaggedLeast;

LOCAL const CPG         cpgOLDUpdateBookmarkThreshold   = 500;

BOOL FOLDSystemTable( const CHAR * const szTableName )
{
    return ( 0 == UtilCmpName( szTableName, szOLD )
            || 0 == UtilCmpName( szTableName, szOLDObsolete )
            || 0 == UtilCmpName( szTableName, OLD2_STATUS::SzTableName() ) );
}


LOCAL VOID OLDAssertLongLongColumn( const FID fid )
{
    Assert( fidOLDObjidFDP != fid );
    Assert( fidOLDStatus != fid );
    Assert( fidOLDCurrentKey != fid );
}

LOCAL ERR ErrOLDRetrieveLongLongColumn(
    __in PIB * const ppib,
    __in FUCB * const pfucb,
    const FID fid,
    __out __int64 * const pValue)
{
    OLDAssertLongLongColumn( fid );
    
    ERR err;

    *pValue = 0;

    ULONG cbActual;
    Call( ErrIsamRetrieveColumn(
        ppib,
        pfucb,
        fid,
        pValue,
        sizeof( __int64 ),
        &cbActual,
        NO_GRBIT,
        NULL ) );
    CallS( err );
    Assert( sizeof( __int64 ) == cbActual );
    
HandleError:
    return err;
}

LOCAL ERR ErrOLDSetLongLongColumn(
    __in PIB * const ppib,
    __in FUCB * const pfucb,
    const FID fid,
    __in __int64 value)
{
    OLDAssertLongLongColumn( fid );

    ERR err;
    DATA dataField;
    
    dataField.SetPv( &value );
    dataField.SetCb( sizeof(value) );
    Call( ErrRECSetColumn(
                pfucb,
                fid,
                1,
                &dataField ) );

HandleError:
    return err;
}

LOCAL ERR ErrOLDIncrementLongLongColumn(
    __in PIB * const ppib,
    __in FUCB * const pfucb,
    const FID fid,
    const __int64 delta)
{
    OLDAssertLongLongColumn( fid );

    ERR err;
    DATA dataField;
    __int64 value;

    Call( ErrOLDRetrieveLongLongColumn( ppib, pfucb, fid, &value ) );
    
    value += delta;
    
    dataField.SetPv( &value );
    dataField.SetCb( sizeof(value) );
    Call( ErrRECSetColumn(
                pfucb,
                fid,
                1,
                &dataField ) );

HandleError:
    return err;
}

const INT cchColumnNameOLDDump = 20;

ERR ErrOLDDumpLongColumn(
    __in PIB * const ppib,
    __in FUCB * const pfucb,
    const FID fid,
    const WCHAR * const szColumn)
{
    ERR err;

    ULONG cbActual;
    DWORD dw;
    
    Call( ErrIsamRetrieveColumn(
        ppib,
        pfucb,
        fid,
        &dw,
        sizeof( dw ),
        &cbActual,
        NO_GRBIT,
        NULL ) );
    wprintf( L"%*.*s: %d\n", cchColumnNameOLDDump, cchColumnNameOLDDump, szColumn, dw );

HandleError:
    return err;
}

ERR ErrOLDDumpLongLongColumn(
    __in PIB * const ppib,
    __in FUCB * const pfucb,
    const FID fid,
    const WCHAR * const szColumn)
{
    ERR err;
    __int64 qw = 0;
    
    Call( ErrOLDRetrieveLongLongColumn(
        ppib,
        pfucb,
        fid,
        &qw ) );
    wprintf( L"%*.*s: %I64d\n", cchColumnNameOLDDump, cchColumnNameOLDDump, szColumn, qw );

HandleError:
    return err;
}

ERR ErrOLDDumpFileTimeColumn(
    __in PIB * const ppib,
    __in FUCB * const pfucb,
    const FID fid,
    const WCHAR * const szColumn)
{
    ERR err;
    __int64 qw;

    WCHAR * szAlloc = NULL;
    size_t cchRequired;
    
    Call( ErrOLDRetrieveLongLongColumn(
        ppib,
        pfucb,
        fid,
        &qw ) );
    wprintf( L"%*.*s: %I64d", cchColumnNameOLDDump, cchColumnNameOLDDump, szColumn, qw );

    if( qw > 0 )
    {
        Call( ErrUtilFormatFileTimeAsDate(
            qw,
            0,
            0,
            &cchRequired ) );
        Alloc( szAlloc = new WCHAR[cchRequired] );
        Call( ErrUtilFormatFileTimeAsDate(
            qw,
            szAlloc,
            cchRequired,
            &cchRequired ) );
        wprintf( L"%s ", szAlloc );
        delete[] szAlloc;
        szAlloc = NULL;

        Call( ErrUtilFormatFileTimeAsTime(
            qw,
            0,
            0,
            &cchRequired ) );
        Alloc( szAlloc = new WCHAR[cchRequired] );
        Call( ErrUtilFormatFileTimeAsTime(
            qw,
            szAlloc,
            cchRequired,
            &cchRequired ) );
        wprintf( L"%s (0x%I64x)\n", szAlloc, qw );
        delete[] szAlloc;
        szAlloc = NULL;
    }
    else
    {
        wprintf( L"\n" );
    }

HandleError:
    delete [] szAlloc;
    return err;
}

ERR ErrOLDDumpMSysDefrag( __in PIB * const ppib, const IFMP ifmp )
{
    ERR err;
    FUCB * pfucbDefrag = pfucbNil;

    Call( OLD2_STATUS::ErrDumpTable( ppib, ifmp) );
    
    err = ErrFILEOpenTable( ppib, ifmp, &pfucbDefrag, szOLD );
    if ( JET_errObjectNotFound == err )
    {
        err = JET_errSuccess;
        goto HandleError;
    }
    Call( err );

    wprintf( L"******************************** MSysDefrag DUMP ***********************************\n" );

    
    err = ErrIsamMove( ppib, pfucbDefrag, JET_MoveFirst, NO_GRBIT );
    Assert( JET_errNoCurrentRecord != err );
    Assert( JET_errRecordNotFound != err );
    Call( err );

    ULONG cbActual;

    DWORD dw;
    Call( ErrIsamRetrieveColumn(
        ppib,
        pfucbDefrag,
        fidOLDObjidFDP,
        &dw,
        sizeof( dw ),
        &cbActual,
        NO_GRBIT,
        NULL ) );
    wprintf( L"%*.*s: %d\n", cchColumnNameOLDDump, cchColumnNameOLDDump, L"ObjidFDP", dw );

    WORD w;
    Call( ErrIsamRetrieveColumn(
        ppib,
        pfucbDefrag,
        fidOLDStatus,
        &w,
        sizeof( w ),
        &cbActual,
        NO_GRBIT,
        NULL ) );
    if( JET_wrnColumnNull == err )
    {
        wprintf( L"%*.*s: NULL\n", cchColumnNameOLDDump, cchColumnNameOLDDump, L"OLDStatus" );
    }
    else
    {
        const WCHAR * szDefragtype = NULL;
        switch( w )
        {
            case defragtypeNull:
                szDefragtype = L"null";
                break;
            case defragtypeTable:
                szDefragtype = L"table";
                break;
            case defragtypeLV:
                szDefragtype = L"LV";
                break;
            case defragtypeSpace:
                szDefragtype = L"space";
                break;
            default:
                szDefragtype = L"unknown";
                break;
        }
        wprintf( L"%*.*s: %d (%s)\n", cchColumnNameOLDDump, cchColumnNameOLDDump, L"OLDStatus", w, szDefragtype );
    }

    Call( ErrOLDDumpFileTimeColumn( ppib, pfucbDefrag, fidOLDPassStartDateTime, L"PassStartDateTime" ) );
    
    Call( ErrOLDDumpLongLongColumn( ppib, pfucbDefrag, fidOLDPassElapsedSeconds, L"PassElapsedSeconds" ) );
    Call( ErrOLDDumpLongLongColumn( ppib, pfucbDefrag, fidOLDPassInvocations, L"PassInvocations" ) );
    Call( ErrOLDDumpLongLongColumn( ppib, pfucbDefrag, fidOLDPassPagesVisited, L"PassPagesVisited" ) );
    Call( ErrOLDDumpLongLongColumn( ppib, pfucbDefrag, fidOLDPassPagesFreed, L"PassPagesFreed" ) );
    Call( ErrOLDDumpLongLongColumn( ppib, pfucbDefrag, fidOLDPassPartialMerges, L"PassPartialMerges" ) );
    Call( ErrOLDDumpLongLongColumn( ppib, pfucbDefrag, fidOLDTotalPasses, L"TotalPasses" ) );
    Call( ErrOLDDumpLongLongColumn( ppib, pfucbDefrag, fidOLDTotalElapsedSeconds, L"TotalElapsedSeconds" ) );
    Call( ErrOLDDumpLongLongColumn( ppib, pfucbDefrag, fidOLDTotalInvocations, L"TotalInvocations" ) );
    Call( ErrOLDDumpLongLongColumn( ppib, pfucbDefrag, fidOLDTotalDefragDays, L"TotalDefragDays" ) );
    Call( ErrOLDDumpLongLongColumn( ppib, pfucbDefrag, fidOLDTotalPagesVisited, L"TotalPagesVisited" ) );
    Call( ErrOLDDumpLongLongColumn( ppib, pfucbDefrag, fidOLDTotalPagesFreed, L"TotalPagesFreed" ) );
    Call( ErrOLDDumpLongLongColumn( ppib, pfucbDefrag, fidOLDTotalPartialMerges, L"TotalPartialMerges" ) );

    wprintf( L"\n" );
    
HandleError:
    if( pfucbNil != pfucbDefrag )
    {
        CallS( ErrFILECloseTable( ppib, pfucbDefrag ) );
    }
    return err;
}


INLINE VOID OLDDB_STATUS::Reset( INST * const pinst )
{
    Assert( pinst->m_critOLD.FOwner() );

    m_cTasksDispatched = 0;

#ifdef OLD_DEPENDENCY_CHAIN_HACK
    m_pgnoPrevPartialMerge = 0;
    m_cpgAdjacentPartialMerges = 0;
#endif

    Reset_();
}



RECCHECKOLD::RECCHECKOLD( const PGNO pgnoFDP, const IFMP ifmp, FUCB * const pfucb, INST * const pinst ) :
    m_pgnoFDP( pgnoFDP ),
    m_ifmp( ifmp ),
    m_pfucb( pfucb ),
    m_pinst( pinst )
{
    Assert( pgnoFDP == pfucb->u.pfcb->PgnoFDP() );
}



template< typename TDelta >
RECCHECKFINALIZE<TDelta>::RECCHECKFINALIZE(
    const FID fid,
    const USHORT ibRecordOffset,
    const PGNO pgnoFDP,
    const IFMP ifmp,
    FUCB * const pfucb,
    INST * const pinst,
    const BOOL fCallback,
    const BOOL fDelete ) :
    RECCHECKOLD( pgnoFDP, ifmp, pfucb, pinst ),
    m_fid( fid ),
    m_ibRecordOffset( ibRecordOffset ),
    m_fCallback( !!fCallback ),
    m_fDelete( !!fDelete )
{
    Assert( FFixedFid( m_fid ) );

    Assert( m_fCallback || m_fDelete );
}

template< typename TDelta >
ERR RECCHECKFINALIZE<TDelta>::operator()( const KEYDATAFLAGS& kdf, const PGNO pgno )
{
    const REC * prec = (REC *)kdf.data.Pv();
    if( m_fid > prec->FidFixedLastInRec() )
    {
        Assert( fFalse );
        return JET_errSuccess;
    }

    const UINT  ifid            = m_fid - fidFixedLeast;
    const BYTE  *prgbitNullity  = prec->PbFixedNullBitMap() + ifid/8;
    if ( FFixedNullBit( prgbitNullity, ifid ) )
    {
        return JET_errSuccess;
    }

    const ULONG ulColumn        = *(UnalignedLittleEndian< ULONG > *)((BYTE *)prec + m_ibRecordOffset );
    if ( 0 == ulColumn )
    {
        BOOKMARK bm;
        bm.key = kdf.key;
        bm.data.Nullify();

        if( !FVERActive( m_pfucb, bm ) )
        {
            FINALIZETASK<TDelta> * ptask = new FINALIZETASK<TDelta>(
                                            m_pgnoFDP,
                                            m_pfucb->u.pfcb,
                                            m_ifmp,
                                            bm,
                                            m_ibRecordOffset,
                                            m_fCallback,
                                            m_fDelete );
            if( NULL == ptask )
            {
                return ErrERRCheck( JET_errOutOfMemory );
            }

            OLDDB_STATUS * const poldstatDB = m_pinst->m_rgoldstatDB + g_rgfmp[m_ifmp].Dbid();
            poldstatDB->IncrementCTasksDispatched();

            m_pinst->m_pver->IncrementCAsyncCleanupDispatched();
            const ERR err = m_pinst->Taskmgr().ErrTMPost( TASK::DispatchGP, ptask );
            if( err < JET_errSuccess )
            {
                m_pinst->m_pver->IncrementCCleanupFailed();
                delete ptask;
                return err;
            }
        }
    }

    return JET_errSuccess;
}



RECCHECKDELETELV::RECCHECKDELETELV(
    const PGNO pgnoFDP,
    const IFMP ifmp,
    FUCB * const pfucb,
    INST * const pinst ) :
    RECCHECKOLD( pgnoFDP, ifmp, pfucb, pinst )
{
}

ERR RECCHECKDELETELV::operator()( const KEYDATAFLAGS& kdf, const PGNO pgno )
{
    if( FIsLVRootKey( kdf.key ) )
    {
        if( sizeof( LVROOT ) != kdf.data.Cb() && sizeof( LVROOT2 ) != kdf.data.Cb() )
        {
            LvId lid;
            LidFromKey( &lid, kdf.key );
            LVReportAndTrapCorruptedLV( m_pfucb, lid, L"3613b349-cb73-48dc-8911-189c0c1cb7b8" );
            return ErrERRCheck( JET_errLVCorrupted );
        }
        const LVROOT * const plvroot = (LVROOT *)kdf.data.Pv();
        if( 0 == plvroot->ulReference )
        {
            BOOKMARK bm;
            bm.key = kdf.key;
            bm.data.Nullify();

            Assert( 0 == bm.data.Cb() );

            if( !FVERActive( m_pfucb, bm ) )
            {
                DELETELVTASK * ptask = new DELETELVTASK( m_pgnoFDP, m_pfucb->u.pfcb, m_ifmp, bm );
                if( NULL == ptask )
                {
                    return ErrERRCheck( JET_errOutOfMemory );
                }

                OLDDB_STATUS * const poldstatDB = m_pinst->m_rgoldstatDB + g_rgfmp[m_ifmp].Dbid();
                poldstatDB->IncrementCTasksDispatched();

                m_pinst->m_pver->IncrementCAsyncCleanupDispatched();
                const ERR err = m_pinst->Taskmgr().ErrTMPost( TASK::DispatchGP, ptask );
                if( err < JET_errSuccess )
                {
                    m_pinst->m_pver->IncrementCCleanupFailed();
                    delete ptask;
                    return err;
                }
            }
        }
    }
    return JET_errSuccess;
}


ERR ErrOLD2ResumeOneTree(
    __in PIB * const ppib,
    const IFMP ifmp,
    const OBJID objidTable,
    const OBJID objidFDP,
    BOOL fDryRun )
{
    ERR err;

    CHAR szTableName[JET_cbNameMost+1];
    CHAR szIndexName[JET_cbNameMost+1];
    PGNO pgnoFDP;
    SYSOBJ treetype;
    DEFRAGTYPE defragtype = defragtypeNull;
    FUCB * pfucbCatalog = pfucbNil;
    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );

    Call( ErrCATISeekTableType( ppib, pfucbCatalog, objidTable, objidFDP, &treetype, szIndexName, JET_cbNameMost ) );
    if ( sysobjTable == treetype )
    {
        defragtype = defragtypeTable;
    }
    else if ( sysobjIndex == treetype )
    {
        defragtype = defragtypeIndex;
        Error( ErrERRCheck( JET_errDisabledFunctionality ) );
    }

    Call( ErrCATSeekTableByObjid(
        ppib,
        ifmp,
        objidTable,
        szTableName,
        sizeof(szTableName),
        &pgnoFDP ) );

    if ( !fDryRun )
    {
        Call( CDefragManager::Instance().ErrRegisterOneTreeOnly( ifmp, szTableName, szIndexName, defragtype) );
    }

HandleError:
    if ( pfucbNil != pfucbCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
    }
    return err;
}


ERR ErrOLD2Resume( __in PIB * const ppib, const IFMP ifmp )
{
    ERR err;
    FUCB * pfucb = pfucbNil;
    IFMP ifmpT = ifmpNil;
    bool fDatabaseOpen = false;

    if ( g_rgfmp[ ifmp ].FReadOnlyAttach() )
    {
        Error( ErrERRCheck( JET_errDatabaseFileReadOnly ) );
    }

    Call( ErrDBOpenDatabase( ppib, g_rgfmp[ifmp].WszDatabaseName(), &ifmpT, NO_GRBIT ) );
    Assert( ifmp == ifmpT );
    fDatabaseOpen = true;

    err = OLD2_STATUS::ErrOpenTable( ppib, ifmp, &pfucb );
    if( JET_errObjectNotFound == err )
    {
        err = JET_errSuccess;
        goto HandleError;
    }
    Call( err );

    err = ErrIsamMove( ppib, pfucb, JET_MoveFirst, NO_GRBIT );
    size_t cResumesAttempted = 0;

    while( JET_errSuccess == err )
    {
        OBJID objidTable;
        OBJID objidFDP;
        Call( OLD2_STATUS::ErrGetObjids( ppib, pfucb, &objidTable, &objidFDP ) );
        Assert( objidNil != objidTable );
        Assert( objidNil != objidFDP );

        err = ErrOLD2ResumeOneTree( ppib, ifmp, objidTable, objidFDP, fTrue );

        AssertSz( err != JET_errRecordNotFound, "JET_errRecordNotFound should be converted by the CAT functions." );
        if ( JET_errObjectNotFound == err || JET_errDisabledFunctionality == err )
        {
            Call( ErrIsamDelete( ppib, pfucb ) );
        }
        else
        {
            if ( cResumesAttempted <= CDefragManager::s_ctasksMax )
            {
                const ERR errNonfatal = ErrOLD2ResumeOneTree( ppib, ifmp, objidTable, objidFDP, fFalse );

                if ( errNonfatal >= JET_errSuccess )
                {
                    ++cResumesAttempted;
                }
            }
        }

        Call( err );
        err = ErrIsamMove( ppib, pfucb, JET_MoveNext, NO_GRBIT );
    }

    if( JET_errNoCurrentRecord == err )
    {
        err = JET_errSuccess;
    }
    Call( err );
    

HandleError:
    if( pfucbNil != pfucb )
    {
        CallS( ErrFILECloseTable( ppib, pfucb ) );
    }
    if( fDatabaseOpen )
    {
        CallS( ErrDBCloseDatabase( ppib, ifmpT, NO_GRBIT ) );
    }
    return err;
}


VOID OLD2TermFmp( const IFMP ifmp )
{
    CDefragManager::Instance().DeregisterIfmp( ifmp );
}

BOOL FOLDRunning( const IFMP ifmp )
{
    INST * const        pinst       = PinstFromIfmp( ifmp );
    const DBID          dbid        = g_rgfmp[ifmp].Dbid();

    pinst->m_critOLD.Enter();

    const OLDDB_STATUS * const  poldstatDB  = pinst->m_rgoldstatDB + dbid;

    const BOOL fRunning = poldstatDB->FRunning();

    pinst->m_critOLD.Leave();

    return fRunning;
}

LOCAL VOID OLDITermThread( OLDDB_STATUS * const poldstatDB, INST * const pinst )
{
    Assert( pinst->m_critOLD.FOwner() );
    
    if ( poldstatDB->FRunning()
        && !poldstatDB->FTermRequested() )
    {
        poldstatDB->SetFTermRequested();
        pinst->m_critOLD.Leave();

        poldstatDB->SetSignal();
        if ( poldstatDB->FRunning() )
        {
            poldstatDB->ThreadEnd();
#ifdef OLD_SCRUB_DB
            poldstatDB->ScrubThreadEnd();
#endif
        }

        pinst->m_critOLD.Enter();
        poldstatDB->Reset( pinst );
    }
}

VOID OLDTermFmp( const IFMP ifmp )
{
    INST * const        pinst       = PinstFromIfmp( ifmp );
    const DBID          dbid        = g_rgfmp[ifmp].Dbid();

    pinst->m_critOLD.Enter();

    OLDDB_STATUS * const    poldstatDB  = pinst->m_rgoldstatDB + dbid;
    g_rgfmp[ifmp].SetFDontStartOLD();
    OLDITermThread( poldstatDB, pinst );

    pinst->m_critOLD.Leave();
}


VOID OLDTermInst( INST *pinst )
{
    DBID    dbid;
    
    pinst->m_critOLD.Enter();

    FMP::EnterFMPPoolAsReader();

    for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        Assert( dbidTemp != dbid );

        OLDDB_STATUS * const    poldstatDB  = pinst->m_rgoldstatDB + dbid;

        IFMP ifmp = pinst->m_mpdbidifmp[ dbid ];

        if ( ifmp < g_ifmpMax )
        {
            g_rgfmp[ifmp].SetFDontStartOLD();
        }

        FMP::LeaveFMPPoolAsReader();
        OLDITermThread( poldstatDB, pinst );
        FMP::EnterFMPPoolAsReader();
    }

    FMP::LeaveFMPPoolAsReader();

    pinst->m_critOLD.Leave();
}


VOID OLD2TermInst( INST *pinst )
{
    CDefragManager::Instance().DeregisterInst( pinst );
}


INLINE BOOL FOLDContinue( const IFMP ifmp )
{
    const INST * const          pinst       = PinstFromIfmp( ifmp );
    const DBID                  dbid        = g_rgfmp[ifmp].Dbid();
    const OLDDB_STATUS * const  poldstatDB  = pinst->m_rgoldstatDB + dbid;

    return ( !poldstatDB->FTermRequested()
        && !poldstatDB->FReachedMaxElapsedTime()
        && !pinst->m_fStopJetService );
}

INLINE BOOL FOLDContinueTree( const FUCB * pfucb )
{
    return ( !pfucb->u.pfcb->FDeletePending() && FOLDContinue( pfucb->ifmp ) );
}


LOCAL ERR ErrOLDStatusUpdate(
    __in PIB * const                    ppib,
    __in FUCB * const               pfucbDefrag,
    __in DEFRAG_STATUS&             defragstat )
{
    ERR                         err;
    const INST * const          pinst       = PinstFromPpib( ppib );
    const DBID                  dbid        = g_rgfmp[ pfucbDefrag->ifmp ].Dbid();
    const OLDDB_STATUS * const  poldstatDB  = pinst->m_rgoldstatDB + dbid;
    OBJID                       objid       = defragstat.ObjidCurrentTable();
    DEFRAGTYPE                  defragtype  = defragstat.DefragType();
    DATA                        dataT;

    const __int64               elapsedSeconds = UtilConvertFileTimeToSeconds( UtilGetCurrentFileTime() - defragstat.StartTime() );
    
    if ( poldstatDB->FAvailExtOnly() )
        return JET_errSuccess;

    Assert( !Pcsr( pfucbDefrag )->FLatched() );

    CallR( ErrDIRBeginTransaction( ppib, 61221, NO_GRBIT ) );

    Call( ErrIsamPrepareUpdate( ppib, pfucbDefrag, JET_prepReplace ) )

    dataT.SetPv( &objid );
    dataT.SetCb( sizeof(OBJID) );
    Call( ErrRECSetColumn(
                pfucbDefrag,
                fidOLDObjidFDP,
                1,
                &dataT ) );

    dataT.SetPv( &defragtype );
    dataT.SetCb( sizeof(DEFRAGTYPE) );
    Call( ErrRECSetColumn(
                pfucbDefrag,
                fidOLDStatus,
                1,
                defragstat.FTypeNull() ? NULL : &dataT ) );

    if ( defragstat.CbCurrentKey() > 0 )
    {
        dataT.SetPv( defragstat.RgbCurrentKey() );
        dataT.SetCb( defragstat.CbCurrentKey() );

        Assert( FTaggedFid( fidOLDCurrentKey ) );
        err = ErrRECSetLongField(
                    pfucbDefrag,
                    fidOLDCurrentKey,
                    1,
                    &dataT,
                    CompressFlags( compress7Bit | compressXpress ) );
        Assert( JET_errRecordTooBig != err );
        Call( err );
    }
    else
    {
        Call( ErrRECSetColumn(
                    pfucbDefrag,
                    fidOLDCurrentKey,
                    1,
                    NULL ) );
    }

    Call( ErrOLDIncrementLongLongColumn( ppib, pfucbDefrag, fidOLDPassElapsedSeconds, elapsedSeconds ) );
    Call( ErrOLDIncrementLongLongColumn( ppib, pfucbDefrag, fidOLDPassPagesVisited, defragstat.CpgVisited() ) );
    Call( ErrOLDIncrementLongLongColumn( ppib, pfucbDefrag, fidOLDPassPagesFreed, defragstat.CpgFreed() ) );
    Call( ErrOLDIncrementLongLongColumn( ppib, pfucbDefrag, fidOLDPassPartialMerges, defragstat.CpgPartialMerges() ) );
    Call( ErrOLDIncrementLongLongColumn( ppib, pfucbDefrag, fidOLDTotalElapsedSeconds, elapsedSeconds ) );
    Call( ErrOLDIncrementLongLongColumn( ppib, pfucbDefrag, fidOLDTotalPagesVisited, defragstat.CpgVisited() ) );
    Call( ErrOLDIncrementLongLongColumn( ppib, pfucbDefrag, fidOLDTotalPagesFreed, defragstat.CpgFreed() ) );
    Call( ErrOLDIncrementLongLongColumn( ppib, pfucbDefrag, fidOLDTotalPartialMerges, defragstat.CpgPartialMerges() ) );
    
    Call( ErrIsamUpdate( ppib, pfucbDefrag, NULL, 0, NULL, NO_GRBIT ) );
    Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );

    defragstat.ResetCpgVisited();
    defragstat.ResetCpgFreed();
    defragstat.ResetCpgPartialMerges();
    defragstat.SetStartTime( UtilGetCurrentFileTime() );

    return JET_errSuccess;

HandleError:
    CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    return err;
}

LOCAL ERR ErrOLDLogResumeEvent(
    __in PIB * const                ppib,
    __in FUCB * const               pfucbDefrag)
{
    ERR err;
    
    const INST * const          pinst       = PinstFromPpib( ppib );
    const IFMP                  ifmp        = pfucbDefrag->ifmp;

    const INT                   cchInt64Max = 22;
    
    const INT                   cszMax = 3;
    const WCHAR *               rgszT[cszMax];
    INT                         isz = 0;

    WCHAR * szPassStartDateTime = 0;
    size_t cchPassStartDateTime;

    CallR( ErrDIRBeginTransaction( ppib, 36645, JET_bitTransactionReadOnly ) );

    __int64 passStartDateTime;
    
    Call( ErrOLDRetrieveLongLongColumn( ppib, pfucbDefrag, fidOLDPassStartDateTime, &passStartDateTime ) );

    const __int64 passElapsedDays = UtilConvertFileTimeToDays( UtilGetCurrentFileTime() - passStartDateTime );
    WCHAR szPassElapsedDays[cchInt64Max];
    OSStrCbFormatW( szPassElapsedDays, cchInt64Max, L"%d", (INT)passElapsedDays );

    Call( ErrUtilFormatFileTimeAsDate(
        passStartDateTime,
        0,
        0,
        &cchPassStartDateTime) );
    Alloc( szPassStartDateTime = new WCHAR[cchPassStartDateTime] );
    Call( ErrUtilFormatFileTimeAsDate(
        passStartDateTime,
        szPassStartDateTime,
        cchPassStartDateTime,
        &cchPassStartDateTime) );

    rgszT[isz++] = g_rgfmp[ifmp].WszDatabaseName();
    rgszT[isz++] = szPassStartDateTime;
    rgszT[isz++] = szPassElapsedDays;
    
    Assert( isz <= cszMax );
    
    UtilReportEvent(
        eventInformation,
        ONLINE_DEFRAG_CATEGORY,
        OLD_RESUME_PASS_ID,
        isz,
        rgszT,
        0,
        NULL,
        pinst );

HandleError:
    CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    delete[] szPassStartDateTime;
    return err;
}

LOCAL ERR ErrOLDStatusResumePass(
    __in PIB * const                ppib,
    __in FUCB * const               pfucbDefrag,
    __in DEFRAG_STATUS&             defragstat )
{
    ERR                         err;

    const INST * const          pinst       = PinstFromPpib( ppib );
    const IFMP                  ifmp        = pfucbDefrag->ifmp;
    const DBID                  dbid        = g_rgfmp[ ifmp ].Dbid();
    const OLDDB_STATUS * const  poldstatDB  = pinst->m_rgoldstatDB + dbid;

    CHAR szTrace[64];
    
    Assert( !poldstatDB->FAvailExtOnly() );
    Assert( !Pcsr( pfucbDefrag )->FLatched() );

    CallR( ErrDIRBeginTransaction( ppib, 53029, NO_GRBIT ) );
    Call( ErrIsamPrepareUpdate( ppib, pfucbDefrag, JET_prepReplace ) );
    
    Call( ErrOLDIncrementLongLongColumn( ppib, pfucbDefrag, fidOLDPassInvocations, 1 ) );
    Call( ErrOLDIncrementLongLongColumn( ppib, pfucbDefrag, fidOLDTotalInvocations, 1 ) );
    
    Call( ErrIsamUpdate( ppib, pfucbDefrag, NULL, 0, NULL, NO_GRBIT ) );
    Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );

    Call( ErrOLDLogResumeEvent( ppib, pfucbDefrag ) );
    
    OSStrCbFormatA( szTrace, sizeof(szTrace), "OLD RESUME (ifmp %d)", (ULONG)ifmp );
    CallS( pinst->m_plog->ErrLGTrace( ppib, szTrace ) );

    defragstat.ResetCpgVisited();
    defragstat.ResetCpgFreed();
    defragstat.ResetCpgPartialMerges();
    defragstat.SetPassPartial();
    defragstat.SetStartTime( UtilGetCurrentFileTime() );

    return JET_errSuccess;

HandleError:
    CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    return err;
}

LOCAL ERR ErrOLDStatusNewPass(
    __in PIB * const                ppib,
    __in FUCB * const               pfucbDefrag,
    __in DEFRAG_STATUS&             defragstat )
{
    ERR                         err;

    const INST * const          pinst       = PinstFromPpib( ppib );
    const IFMP                  ifmp        = pfucbDefrag->ifmp;
    const DBID                  dbid        = g_rgfmp[ ifmp ].Dbid();
    const OLDDB_STATUS * const  poldstatDB  = pinst->m_rgoldstatDB + dbid;
    const WCHAR *               rgszT[1]    = { g_rgfmp[ifmp].WszDatabaseName() };

    CHAR szTrace[64];
    
    Assert( !poldstatDB->FAvailExtOnly() );
    Assert( !Pcsr( pfucbDefrag )->FLatched() );

    CallR( ErrDIRBeginTransaction( ppib, 46885, NO_GRBIT ) );
    
    Call( ErrIsamPrepareUpdate( ppib, pfucbDefrag, JET_prepReplace ) );

    Call( ErrOLDSetLongLongColumn( ppib, pfucbDefrag, fidOLDPassStartDateTime, UtilGetCurrentFileTime() ) );
    Call( ErrOLDSetLongLongColumn( ppib, pfucbDefrag, fidOLDPassElapsedSeconds, 0 ) );
    Call( ErrOLDSetLongLongColumn( ppib, pfucbDefrag, fidOLDPassInvocations, 1 ) );
    Call( ErrOLDSetLongLongColumn( ppib, pfucbDefrag, fidOLDPassPagesVisited, 0 ) );
    Call( ErrOLDSetLongLongColumn( ppib, pfucbDefrag, fidOLDPassPagesFreed, 0 ) );
    Call( ErrOLDSetLongLongColumn( ppib, pfucbDefrag, fidOLDPassPartialMerges, 0 ) );
    Call( ErrOLDIncrementLongLongColumn( ppib, pfucbDefrag, fidOLDTotalInvocations, 1 ) );
    
    Call( ErrIsamUpdate( ppib, pfucbDefrag, NULL, 0, NULL, NO_GRBIT ) );
    Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );

    UtilReportEvent(
        eventInformation,
        ONLINE_DEFRAG_CATEGORY,
        OLD_BEGIN_FULL_PASS_ID,
        1,
        rgszT,
        0,
        NULL,
        pinst );

    OSStrCbFormatA( szTrace, sizeof(szTrace), "OLD BEGIN (ifmp %d)", (ULONG)ifmp );
    CallS( pinst->m_plog->ErrLGTrace( ppib, szTrace ) );

    defragstat.ResetCpgVisited();
    defragstat.ResetCpgFreed();
    defragstat.ResetCpgPartialMerges();
    defragstat.SetPassFull();
    defragstat.SetStartTime( UtilGetCurrentFileTime() );

    return JET_errSuccess;

HandleError:
    CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    return err;
}

LOCAL ERR ErrOLDLogCompletionEvent(
    __in PIB * const                    ppib,
    __in FUCB * const               pfucbDefrag,
    const MessageId             messageId)
{
    ERR                         err;

    const INST * const          pinst       = PinstFromPpib( ppib );
    const IFMP                  ifmp        = pfucbDefrag->ifmp;

    const INT                   cchInt64Max = 22;

    const INT                   cszMax = 7;
    const WCHAR *               rgszT[cszMax];
    INT                         isz = 0;

    WCHAR * szPassStartDateTime = 0;
    size_t cchPassStartDateTime;

    CallR( ErrDIRBeginTransaction( ppib, 63269, JET_bitTransactionReadOnly ) );
    
    __int64 passStartDateTime;
    __int64 passPagesFreed;
    __int64 passElapsedSeconds;
    __int64 passInvocations;
    __int64 totalPasses;
    
    Call( ErrOLDRetrieveLongLongColumn( ppib, pfucbDefrag, fidOLDPassStartDateTime, &passStartDateTime ) );
    Call( ErrOLDRetrieveLongLongColumn( ppib, pfucbDefrag, fidOLDPassPagesFreed, &passPagesFreed ) );
    Call( ErrOLDRetrieveLongLongColumn( ppib, pfucbDefrag, fidOLDPassElapsedSeconds, &passElapsedSeconds ) );
    Call( ErrOLDRetrieveLongLongColumn( ppib, pfucbDefrag, fidOLDPassInvocations, &passInvocations ) );
    Call( ErrOLDRetrieveLongLongColumn( ppib, pfucbDefrag, fidOLDTotalPasses, &totalPasses ) );

    const __int64 passElapsedDays = UtilConvertFileTimeToDays( UtilGetCurrentFileTime() - passStartDateTime );
    WCHAR szPassElapsedDays[cchInt64Max];
    OSStrCbFormatW( szPassElapsedDays, cchInt64Max, L"%d", (INT)passElapsedDays );

    Call( ErrUtilFormatFileTimeAsDate(
        passStartDateTime,
        0,
        0,
        &cchPassStartDateTime) );
    Alloc( szPassStartDateTime = new WCHAR[cchPassStartDateTime] );
    Call( ErrUtilFormatFileTimeAsDate(
        passStartDateTime,
        szPassStartDateTime,
        cchPassStartDateTime,
        &cchPassStartDateTime) );

    WCHAR szPassPagesFreed[cchInt64Max];
    OSStrCbFormatW( szPassPagesFreed, cchInt64Max, L"%d", (INT)passPagesFreed );

    WCHAR szPassElapsedSeconds[cchInt64Max];
    OSStrCbFormatW( szPassElapsedSeconds, cchInt64Max, L"%d", (INT)passElapsedSeconds );

    WCHAR szPassInvocations[cchInt64Max];
    OSStrCbFormatW( szPassInvocations, cchInt64Max, L"%d", (INT)passInvocations );

    WCHAR szTotalPasses[cchInt64Max];
    OSStrCbFormatW( szTotalPasses, cchInt64Max, L"%d", (INT)totalPasses );

    rgszT[isz++] = g_rgfmp[ifmp].WszDatabaseName();
    rgszT[isz++] = szPassPagesFreed;
    rgszT[isz++] = szPassStartDateTime;
    rgszT[isz++] = szPassElapsedSeconds;
    rgszT[isz++] = szPassInvocations;
    rgszT[isz++] = szPassElapsedDays;
    rgszT[isz++] = szTotalPasses;
    
    Assert( isz <= cszMax );
    
    UtilReportEvent(
        eventInformation,
        ONLINE_DEFRAG_CATEGORY,
        messageId,
        isz,
        rgszT,
        0,
        NULL,
        pinst );

HandleError:
    CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    delete[] szPassStartDateTime;
    return err;
}

LOCAL ERR ErrOLDStatusCompletedPass(
    __in PIB * const                ppib,
    __in FUCB * const               pfucbDefrag,
    __in DEFRAG_STATUS&             defragstat )
{
    ERR                         err;

    const INST * const          pinst       = PinstFromPpib( ppib );
    const IFMP                  ifmp        = pfucbDefrag->ifmp;
    const DBID                  dbid        = g_rgfmp[ ifmp ].Dbid();
    const OLDDB_STATUS * const  poldstatDB  = pinst->m_rgoldstatDB + dbid;
    const MessageId             messageId   = defragstat.FPassFull() ? OLD_COMPLETE_FULL_PASS_ID : OLD_COMPLETE_RESUMED_PASS_ID;

    CHAR szTrace[64];
    
    Assert( !poldstatDB->FAvailExtOnly() );
    Assert( !Pcsr( pfucbDefrag )->FLatched() );

    defragstat.SetTypeNull();
    defragstat.SetObjidCurrentTable( objidFDPMSO );

    CallR( ErrDIRBeginTransaction( ppib, 38693, NO_GRBIT ) );
    Call( ErrOLDStatusUpdate( ppib, pfucbDefrag, defragstat ) );

    __int64 passStartDateTime;
    Call( ErrOLDRetrieveLongLongColumn( ppib, pfucbDefrag, fidOLDPassStartDateTime, &passStartDateTime ) );
    const __int64 passElapsedDays = UtilConvertFileTimeToDays( UtilGetCurrentFileTime() - passStartDateTime );
    
    Call( ErrIsamPrepareUpdate( ppib, pfucbDefrag, JET_prepReplace ) );
    Call( ErrOLDIncrementLongLongColumn( ppib, pfucbDefrag, fidOLDTotalPasses, 1 ) );
    Call( ErrOLDIncrementLongLongColumn( ppib, pfucbDefrag, fidOLDTotalDefragDays, passElapsedDays ) );
    Call( ErrIsamUpdate( ppib, pfucbDefrag, NULL, 0, NULL, NO_GRBIT ) );
    Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );

    Call( ErrOLDLogCompletionEvent( ppib, pfucbDefrag, messageId ) );

    OSStrCbFormatA( szTrace, sizeof(szTrace), "OLD COMPLETED PASS (ifmp %d)", (ULONG)ifmp );
    CallS( pinst->m_plog->ErrLGTrace( ppib, szTrace ) );
    
    return JET_errSuccess;

HandleError:
    CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    return err;
}

LOCAL VOID PauseOLDIfNeeded(
    PIB* ppib,
    FUCB* pfucb,
    RECCHECK* preccheck,
    const ULONG_PTR cTasksDispatched )
{
    INST* const pinst = PinstFromPpib( ppib );
    const IFMP ifmp = pfucb->ifmp;
    OLDDB_STATUS* poldstatDB = pinst->m_rgoldstatDB + g_rgfmp[ifmp].Dbid();

    if ( NULL != preccheck
        && poldstatDB->CTasksDispatched() != cTasksDispatched
        && pinst->Taskmgr().CPostedTasks() > UlParam( pinst, JET_paramVersionStoreTaskQueueMax )
        && FOLDContinueTree( pfucb ) )
    {
        poldstatDB->FWaitOnSignal( 1000 );
    }

    while ( pinst->m_pbackup->FBKBackupInProgress( BACKUP_CONTEXT::backupLocalOnly ) && FOLDContinueTree( pfucb ) )
    {
        poldstatDB->FWaitOnSignal( cmsecWaitForBackup );
    }
}

LOCAL ERR ErrOLDDefragOneTree(
    PIB             *ppib,
    FUCB            *pfucb,
    FUCB            *pfucbDefrag,
    DEFRAG_STATUS&  defragstat,
    const BOOL      fResumingTree,
    RECCHECK        *preccheck )
{
    ERR             err;
    INST *          const pinst             = PinstFromPpib( ppib );
    OLDDB_STATUS *  poldstatDB              = pinst->m_rgoldstatDB + g_rgfmp[pfucb->ifmp].Dbid();
    DIB             dib;
    BOOKMARK        bmStart;
    BOOKMARK        bmNext;
    MERGETYPE       mergetype;
    BYTE            *pbKeyBuf               = NULL;
    BOOL            fInTrx                  = fFalse;

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortOLD );

    Alloc( pbKeyBuf = (BYTE*)RESBOOKMARK.PvRESAlloc() );

    Assert( pfucbNil != pfucb );
    Assert( !FFMPIsTempDB( pfucb->ifmp ) );
    Assert( ( !fResumingTree && defragstat.FTypeSpace() )
        || defragstat.FTypeTable()
        || defragstat.FTypeIndex()
        || defragstat.FTypeLV() );

    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( !Pcsr( pfucbDefrag )->FLatched() );

    Assert( pfucb->u.pfcb->FSpaceInitialized() );
    Assert( pgnoNull != pfucb->u.pfcb->PgnoOE() );
    Assert( pgnoNull != pfucb->u.pfcb->PgnoAE() );

    bmStart.Nullify();
    bmStart.key.suffix.SetPv( defragstat.RgbCurrentKey() );
    bmNext.Nullify();
    bmNext.key.suffix.SetPv( pbKeyBuf );

    if ( fResumingTree && defragstat.CbCurrentKey() > 0 )
    {
        bmStart.key.suffix.SetCb( defragstat.CbCurrentKey() );
        dib.pos = posDown;
        dib.pbm = &bmStart;
    }
    else
    {
        dib.pos = posLast;
    }

    dib.dirflag = fDIRAllNode;
    err = ErrBTDown( pfucb, &dib, latchReadTouch );
    if ( err < 0 )
    {
        if ( JET_errRecordNotFound == err )
            err = JET_errSuccess;

        goto Cleanup;
    }

    Assert( Pcsr( pfucb )->FLatched() );


    if( !FFUCBUnique( pfucb ) )
    {
        Error( ErrERRCheck( JET_errFeatureNotAvailable ) );
    }

    NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bmStart );

    Assert( !bmStart.key.FNull() );

    Assert( bmNext.key.suffix.Pv() == pbKeyBuf );
    Assert( 0 == bmNext.key.prefix.Cb() );
    bmStart.key.CopyIntoBuffer( bmNext.key.suffix.Pv(), bmStart.key.Cb() );
    bmNext.key.suffix.SetCb( bmStart.key.Cb() );

    bmNext.data.SetPv( (BYTE *)bmNext.key.suffix.Pv() + bmNext.key.Cb() );
    bmStart.data.CopyInto( bmNext.data );

    bmStart.Nullify();
    bmStart.key.suffix.SetPv( defragstat.RgbCurrentKey() );

#ifdef OLD_DEPENDENCY_CHAIN_HACK
    poldstatDB->SetPgnoPrevPartialMerge( 0 );
    poldstatDB->ResetCpgAdjacentPartialMerges();
#endif

    while ( !bmNext.key.FNull()
        && FOLDContinueTree( pfucb ) )
    {
        UPDATETHREADSTATSCOUNTERS   updatetscounters( pinst, &g_tscountersOLD );

        BOOL                fPerformedRCEClean  = fFalse;
        const ULONG_PTR     cTasksDispatched    = ( NULL != preccheck ? poldstatDB->CTasksDispatched() : 0 );

        VOID *              pvSwap              = bmStart.key.suffix.Pv();
        bmStart.key.suffix.SetPv( bmNext.key.suffix.Pv() );
        bmStart.key.suffix.SetCb( bmNext.key.suffix.Cb() );
        bmStart.data.SetPv( bmNext.data.Pv() );
        bmStart.data.SetCb( bmNext.data.Cb() );

        bmNext.Nullify();
        bmNext.key.suffix.SetPv( pvSwap );

        Assert( bmStart.key.prefix.Cb() == 0 );
        Assert( bmNext.key.prefix.Cb() == 0 );

        Assert( bmStart.data.Cb() == 0 );
        Assert( bmNext.data.Cb() == 0 );

        BTUp( pfucb );

        Assert( !fInTrx );
        Call( ErrDIRBeginTransaction( ppib, 55077, NO_GRBIT ) );
        fInTrx = fTrue;

        forever
        {
            err = ErrBTIMultipageCleanup( pfucb, bmStart, &bmNext, preccheck, &mergetype, fTrue );
            BTUp( pfucb );

            if ( err < 0 )
            {
                if ( ( JET_errVersionStoreOutOfMemory == err || JET_errVersionStoreOutOfMemoryAndCleanupTimedOut == err )
                    && !fPerformedRCEClean )
                {
                    CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
                    fInTrx = fFalse;

                    UtilSleep( 1000 );

                    Call( ErrDIRBeginTransaction( ppib, 42789, NO_GRBIT ) );
                    fInTrx = fTrue;

                    fPerformedRCEClean = fTrue;
                }
                else if ( errBTMergeNotSynchronous != err )
                {
                    goto HandleError;
                }
            }
            else
            {
                break;
            }
        }
        
        if ( !defragstat.FTypeSpace() )
        {
            Assert( !FFUCBSpace( pfucb ) );
            if ( bmNext.key.suffix.Pv() == defragstat.RgbCurrentKey() )
            {
                defragstat.SetCbCurrentKey( bmNext.key.suffix.Cb() );

                Assert( bmStart.key.suffix.Pv() == pbKeyBuf );
            }
            else
            {
                Assert( bmStart.key.suffix.Pv() == defragstat.RgbCurrentKey() );
                Assert( (ULONG)bmStart.key.suffix.Cb() == defragstat.CbCurrentKey() );
                Assert( bmNext.key.suffix.Pv() == pbKeyBuf );
            }

            defragstat.IncrementCpgVisited();
            switch( mergetype )
            {
                case mergetypeNone:
                    break;
                case mergetypeEmptyPage:
                case mergetypeFullRight:
                case mergetypeEmptyTree:
                case mergetypeFullLeft:
                    defragstat.IncrementCpgFreed();
                    PERFOpt( cOLDPagesFreed.Inc( pinst ) );
                    break;
                case mergetypePartialRight:
                case mergetypePartialLeft:
                    defragstat.IncrementCpgPartialMerges();
                    PERFOpt( cOLDPartialMerges.Inc( pinst ) );
                    break;
                default:
                    AssertSz( fFalse, "Unexpected case" );
                    Call( ErrERRCheck( JET_errInternalError ) );
                    break;
            }
            
            if ( defragstat.CpgVisited() > cpgOLDUpdateBookmarkThreshold )
            {
                Assert( 0 == bmNext.data.Cb() );
                Assert( 0 == bmNext.key.prefix.Cb() );


                Assert( defragstat.CbCurrentKey() < cbLVIntrinsicMost );

                Assert( defragstat.FTypeTable() || defragstat.FTypeLV() );
                Call( ErrOLDStatusUpdate( ppib, pfucbDefrag, defragstat ) );
            }
        }
        else
        {
            Assert( FFUCBSpace( pfucb ) );
            if ( bmNext.key.suffix.Pv() == defragstat.RgbCurrentKey() )
            {
                Assert( bmStart.key.suffix.Pv() == pbKeyBuf );
            }
            else
            {
                Assert( bmStart.key.suffix.Pv() == defragstat.RgbCurrentKey() );
                Assert( bmNext.key.suffix.Pv() == pbKeyBuf );
            }
        }

        Assert( fInTrx );
        Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
        fInTrx = fFalse;

        PauseOLDIfNeeded( ppib, pfucb, preccheck, cTasksDispatched );
    }

    Assert( !fInTrx );

HandleError:
    BTUp( pfucb );

Cleanup:
    if ( fInTrx )
    {
        Assert( err < 0 );
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    RESBOOKMARK.Free( pbKeyBuf );

    return err;
}


LOCAL ERR ErrOLDDefragSpaceTree(
    PIB             * ppib,
    FUCB            * pfucb,
    FUCB            * pfucbDefrag,
    const BOOL      fDefragAvailExt )
{
    ERR             err;
    FUCB            * pfucbT    = pfucbNil;
    DEFRAG_STATUS   defragstat;

    Assert( !FFUCBSpace( pfucb ) );

    Assert( pfucb->u.pfcb->FSpaceInitialized() );
    Assert( pgnoNull != pfucb->u.pfcb->PgnoAE() );
    Assert( pgnoNull != pfucb->u.pfcb->PgnoOE() );

    if ( fDefragAvailExt )
    {
        CallR( ErrSPIOpenAvailExt( ppib, pfucb->u.pfcb, &pfucbT ) );
    }
    else
    {
        CallR( ErrSPIOpenOwnExt( ppib, pfucb->u.pfcb, &pfucbT ) );
    }
    Assert( FFUCBSpace( pfucbT ) );

    defragstat.SetTypeSpace();
    err = ErrOLDDefragOneTree(
                ppib,
                pfucbT,
                pfucbDefrag,
                defragstat,
                fFalse,
                NULL );

    Assert( pfucbNil != pfucbT );
    BTClose( pfucbT );

    return err;
}

LOCAL ERR ErrOLDCheckForFinalize(
    _In_ PIB * const        ppib,
    _In_ FUCB * const   pfucb,
    _In_ INST * const   pinst,
    _Outptr_ RECCHECK **        ppreccheck )
{
    ERR             err         = JET_errSuccess;

    FCB * const     pfcb        = pfucb->u.pfcb;
    Assert( NULL != pfcb );
    Assert( pfcb->FTypeTable() );
    Assert( NULL != pfcb->Ptdb() );
    pfcb->EnterDML();

    TDB * const     ptdb        = pfcb->Ptdb();

#ifdef DEBUG
    BOOL            fDoCheck    = fTrue;
#else
    BOOL            fDoCheck    = fFalse;
#endif

    if ( ptdb->FTableHasDeleteOnZeroColumn() )
    {
        fDoCheck = fTrue;
    }

    else if ( ptdb->FTableHasFinalizeColumn() )
    {
        for ( const CBDESC * pcbdesc = ptdb->Pcbdesc(); NULL != pcbdesc; pcbdesc = pcbdesc->pcbdescNext )
        {
            if ( pcbdesc->cbtyp & JET_cbtypFinalize )
            {
                fDoCheck = fTrue;
                break;
            }
        }
    }

    *ppreccheck = NULL;

    if ( fDoCheck )
    {
        const FID   fidLast = ptdb->FidFixedLast();
        for( FID fid = fidFixedLeast; fid <= fidLast; ++fid )
        {
            const BOOL          fTemplateColumn = ptdb->FFixedTemplateColumn( fid );
            const FIELD * const pfield          = ptdb->PfieldFixed( ColumnidOfFid( fid, fTemplateColumn ) );

            if ( ( FFIELDFinalize( pfield->ffield ) || FFIELDDeleteOnZero( pfield->ffield ) )
                && !FFIELDDeleted( pfield->ffield ) )
            {
                Assert( ptdb->FTableHasFinalizeColumn() || ptdb->FTableHasDeleteOnZeroColumn() );
                Assert( !FFIELDFinalize( pfield->ffield ) || ptdb->FTableHasFinalizeColumn() );
                Assert( !FFIELDDeleteOnZero( pfield->ffield ) || ptdb->FTableHasDeleteOnZeroColumn() );

                if ( pfield->cbMaxLen == 4 )
                {
                    *ppreccheck = new RECCHECKFINALIZE<LONG>(
                                    fid,
                                    pfield->ibRecordOffset,
                                    pfcb->PgnoFDP(),
                                    pfcb->Ifmp(),
                                    pfucb,
                                    pinst,
                                    FFIELDFinalize( pfield->ffield ),
                                    FFIELDDeleteOnZero( pfield->ffield ) );
                }
                else if ( pfield->cbMaxLen == 8 )
                {
                    *ppreccheck = new RECCHECKFINALIZE<LONGLONG>(
                                    fid,
                                    pfield->ibRecordOffset,
                                    pfcb->PgnoFDP(),
                                    pfcb->Ifmp(),
                                    pfucb,
                                    pinst,
                                    FFIELDFinalize( pfield->ffield ),
                                    FFIELDDeleteOnZero( pfield->ffield ) );
                }
                else
                {
                    Assert( fFalse );
                    Call( ErrERRCheck( JET_errInvalidOperation ) );
                }

                err = ( NULL == *ppreccheck ) ? ErrERRCheck( JET_errOutOfMemory ) : JET_errSuccess;
                break;
            }
        }
    }

HandleError:
    pfcb->LeaveDML();
    return err;
}

typedef struct
{
    PIB *  ppib;
    FUCB * pfucbOwningTableToDefrag;
    FUCB * pfucbDefragStatus;
    DEFRAG_STATUS * pDefragstat;
    const OLDDB_STATUS * poldstatDB;
    BOOL fResumingTree;
} OLDDefragIndexParam;

ERR ErrOLDDefragOneSecondaryIndex(
    _In_ const OLDDefragIndexParam * pparam,
    _In_z_ CHAR * szIndex )
{
    return ErrERRCheck( JET_errDisabledFunctionality );
}

LOCAL ERR ErrOLDIExplicitDefragOneTable(
    PIB *                       ppib,
    FUCB *                      pfucbCatalog,
    FUCB *                      pfucbDefrag,
    DEFRAG_STATUS&              defragstat )
{
    ERR                         err;
    const IFMP                  ifmp            = pfucbCatalog->ifmp;
    const OLDDB_STATUS * const  poldstatDB      = PinstFromIfmp( ifmp )->m_rgoldstatDB + g_rgfmp[ifmp].Dbid();
    FUCB *                      pfucb           = pfucbNil;
    FUCB *                      pfucbLV         = pfucbNil;
    OBJID                       objidTable;
    DATA                        dataField;
    CHAR                        szTable[JET_cbNameMost+1];
    BOOL                        fLatchedCatalog = fFalse;

    Assert( !Pcsr( pfucbDefrag )->FLatched() );

    Assert( !Pcsr( pfucbCatalog )->FLatched() );
    err = ErrDIRGet( pfucbCatalog );
    if ( err < 0 )
    {
        if ( JET_errRecordDeleted == err )
        {
            err = JET_errSuccess;
        }
        goto HandleError;
    }
    fLatchedCatalog = fTrue;

    Assert( FFixedFid( fidMSO_Type ) );
    CallS( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Type,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    Assert( dataField.Cb() == sizeof(SYSOBJ) );

    if ( sysobjTable != *( (UnalignedLittleEndian< SYSOBJ > *)dataField.Pv() ) )
    {
        err = JET_errSuccess;
        goto HandleError;
    }

    Assert( FFixedFid( fidMSO_ObjidTable ) );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_ObjidTable,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(OBJID) );
    UtilMemCpy( &objidTable, dataField.Pv(), sizeof(OBJID) );

    Assert( objidTable >= objidFDPMSO );
    if ( defragstat.ObjidCurrentTable() != objidTable )
    {
        defragstat.SetObjidCurrentTable( objidTable );

        defragstat.SetTypeNull();
    }

    Assert( FVarFid( fidMSO_Name ) );
    Call( ErrRECIRetrieveVarColumn(
                pfcbNil,
                pfucbCatalog->u.pfcb->Ptdb(),
                fidMSO_Name,
                pfucbCatalog->kdfCurr.data,
                &dataField ) );
    CallS( err );

    const INT cbDataField = dataField.Cb();

    if ( cbDataField < 0 || cbDataField > JET_cbNameMost )
    {
        AssertSz( fFalse, "Invalid fidMSO_Name column in catalog." );
        Error( ErrERRCheck( JET_errCatalogCorrupted ) );
    }
    Assert( sizeof( szTable ) > cbDataField );
    UtilMemCpy( szTable, dataField.Pv(), cbDataField );
    szTable[cbDataField] = '\0';

    Assert( fLatchedCatalog );
    CallS( ErrDIRRelease( pfucbCatalog ) );
    fLatchedCatalog = fFalse;

    err = ErrFILEOpenTable( ppib, ifmp, &pfucb, szTable, JET_bitTableTryPurgeOnClose );
    if ( err < 0 )
    {
        Assert( pfucbNil == pfucb );
        if ( JET_errTableLocked == err || JET_errObjectNotFound == err )
            err = JET_errSuccess;
        goto HandleError;
    }

    Assert( pfucbNil != pfucb );

    if( pfucb->u.pfcb->FUseOLD2() )
    {
        goto HandleError;
    }
    
    if ( !pfucb->u.pfcb->FSpaceInitialized() )
    {
        Call( ErrSPDeferredInitFCB( pfucb ) );
    }

#ifdef DEBUG
    Assert( pfucb->u.pfcb->FSpaceInitialized() );
    if ( pgnoNull == pfucb->u.pfcb->PgnoOE() )
    {
        Assert( pgnoNull == pfucb->u.pfcb->PgnoAE() );
    }
    else
    {
        Assert( pgnoNull != pfucb->u.pfcb->PgnoAE() );
        Assert( pfucb->u.pfcb->PgnoAE() == pfucb->u.pfcb->PgnoOE()+1 );
    }
#endif

    const BOOL fDefragMainTrees = !!UlConfigOverrideInjection( 45068, fTrue );
    OSTrace( JET_tracetagOLDRegistration, OSFormat( __FUNCTION__ ": UlConfigOverrideInjection says to %s main trees",
                                        fDefragMainTrees ? "DEFRAG" : "NOT defrag" ) );

    const BOOL fTableIsSmall = pgnoNull == pfucb->u.pfcb->PgnoAE();
    if ( fTableIsSmall )
    {
        OSTrace( JET_tracetagOLDRegistration, OSFormat( __FUNCTION__ ": Skipping table '%s', because it is so small it doesn't even have a space tree!",
                                      szTable ) );
    }

    if ( FOLDContinueTree( pfucb )
        && !fTableIsSmall
         && fDefragMainTrees )
    {
        const BOOL  fResumingTree   = !defragstat.FTypeNull();

        if ( !poldstatDB->FAvailExtOnly() )
        {
            if ( defragstat.FTypeNull() || defragstat.FTypeTable() )
            {
                RECCHECK * preccheck = NULL;
                Call( ErrOLDCheckForFinalize( ppib, pfucb, PinstFromPpib( ppib ), &preccheck ) );

                defragstat.SetTypeTable();
                err = ErrOLDDefragOneTree(
                            ppib,
                            pfucb,
                            pfucbDefrag,
                            defragstat,
                            fResumingTree,
                            preccheck );

                if( NULL != preccheck )
                {
                    delete preccheck;
                    preccheck = NULL;
                }

                if ( err >= 0 && FOLDContinueTree( pfucb ) )
                {
                    defragstat.SetTypeLV();
                    defragstat.SetCbCurrentKey( 0 );
                }
                else
                {
                    goto HandleError;
                }
            }
            else
            {
                Assert( defragstat.FTypeLV() );
                Assert( fResumingTree );
            }

            Call( ErrOLDDefragSpaceTree(
                        ppib,
                        pfucb,
                        pfucbDefrag,
                        fFalse ) );
            if ( !FOLDContinueTree( pfucb ) )
            {
                goto HandleError;
            }
        }

        Call( ErrOLDDefragSpaceTree(
                    ppib,
                    pfucb,
                    pfucbDefrag,
                    fTrue ) );
        if ( !FOLDContinueTree( pfucb ) )
        {
            goto HandleError;
        }

        if ( defragstat.FTypeLV() || poldstatDB->FAvailExtOnly() )
        {
            Call( ErrFILEOpenLVRoot( pfucb, &pfucbLV, fFalse ) );
            if ( wrnLVNoLongValues == err )
            {
                Assert( pfucbNil == pfucbLV );
            }
            else
            {
                CallS( err );
                Assert( pfucbNil != pfucbLV );
                Assert( pfucbLV->u.pfcb->FSpaceInitialized() );

                if ( pgnoNull == pfucbLV->u.pfcb->PgnoAE() )
                {
                    Assert( pgnoNull == pfucbLV->u.pfcb->PgnoOE() );
                    goto HandleError;
                }
                else
                {
                    Assert( pgnoNull != pfucb->u.pfcb->PgnoOE() );
                    Assert( pfucbLV->u.pfcb->PgnoAE() == pfucbLV->u.pfcb->PgnoOE()+1 );
                }

                if ( FOLDContinueTree( pfucb )
                    && !poldstatDB->FAvailExtOnly() )
                {
                    RECCHECKDELETELV reccheck(
                        pfucbLV->u.pfcb->PgnoFDP(),
                        pfucbLV->ifmp,
                        pfucbLV,
                        PinstFromPfucb( pfucb ) );

                    Call( ErrOLDDefragOneTree(
                                ppib,
                                pfucbLV,
                                pfucbDefrag,
                                defragstat,
                                fResumingTree,
                                &reccheck ) );

                    if ( !FOLDContinueTree( pfucb ) )
                    {
                        goto HandleError;
                    }

                    Call( ErrOLDDefragSpaceTree(
                                ppib,
                                pfucbLV,
                                pfucbDefrag,
                                fFalse ) );

                    if ( !FOLDContinueTree( pfucb ) )
                    {
                        goto HandleError;
                    }
                }

                Call( ErrOLDDefragSpaceTree(
                            ppib,
                            pfucbLV,
                            pfucbDefrag,
                            fTrue ) );
            }
        }

        if ( !FOLDContinueTree( pfucb ) )
        {
            goto HandleError;
        }

        defragstat.SetTypeIndex();
        defragstat.SetCbCurrentKey( 0 );
    }

HandleError:
    if ( fLatchedCatalog )
    {
        CallS( ErrDIRRelease( pfucbCatalog ) );
    }
    if ( pfucbNil != pfucbLV )
    {
        DIRClose( pfucbLV );
    }

    if ( pfucbNil != pfucb )
    {
        CallS( ErrFILECloseTable( ppib, pfucb ) );
    }

    return err;
}

LOCAL ERR ErrOLDIExplicitDefragTables(
    PIB *                   ppib,
    FUCB *                  pfucbDefrag,
    DEFRAG_STATUS&          defragstat )
{
    ERR                     err;
    INST * const            pinst           = PinstFromPpib( ppib );
    const IFMP              ifmp            = pfucbDefrag->ifmp;
    OLDDB_STATUS * const    poldstatDB      = pinst->m_rgoldstatDB + g_rgfmp[ifmp].Dbid();
    FUCB *                  pfucbCatalog    = pfucbNil;
    OBJID                   objidNext       = defragstat.ObjidCurrentTable();
    ULONG_PTR               csecStartPass;
    CHAR                    szTrace[64];

    Assert( !FFMPIsTempDB( ifmp ) );
    Assert( 0 == poldstatDB->CPasses() );
    Assert( 0 == defragstat.CpgVisited() );
    Assert( 0 == defragstat.CpgFreed() );
    Assert( 0 == defragstat.CpgPartialMerges() );
    Assert( defragstat.FPassNull() );

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    if ( objidNext > objidFDPMSO )
    {
        Call( ErrOLDStatusResumePass( ppib, pfucbDefrag, defragstat ) );
    }
    else
    {
        Call( ErrOLDStatusNewPass( ppib, pfucbDefrag, defragstat ) );
    }

    csecStartPass = UlUtilGetSeconds();

    while ( FOLDContinue( ifmp ) )
    {
        Call( ErrIsamMakeKey(
                    ppib,
                    pfucbCatalog,
                    (BYTE *)&objidNext,
                    sizeof(objidNext),
                    JET_bitNewKey ) );
        err = ErrIsamSeek( ppib, pfucbCatalog, JET_bitSeekGE );
        if ( JET_errRecordNotFound == err )
        {
            err = JET_errSuccess;

            Call( ErrOLDStatusCompletedPass( ppib, pfucbDefrag, defragstat ) );
            
            if ( defragstat.FPassFull() )
            {
                poldstatDB->IncCPasses();
                if ( poldstatDB->FReachedMaxPasses() )
                {
                    defragstat.SetPassCompleted();
                    break;
                }
            }
            
            ULONG   cmsecWait   =   cmsecAsyncBackgroundCleanup;
            if ( poldstatDB->FInfinitePasses() )
            {
                const ULONG_PTR     csecCurrentPass = UlUtilGetSeconds() - csecStartPass;
                if ( csecCurrentPass < csecOLDMinimumPass )
                {
                    cmsecWait = (ULONG)max( ( csecOLDMinimumPass - csecCurrentPass ) * 1000, cmsecAsyncBackgroundCleanup );
                }
            }

            poldstatDB->FWaitOnSignal( cmsecWait );

            if ( FOLDContinue( ifmp ) )
            {
                Call( ErrOLDStatusNewPass( ppib, pfucbDefrag, defragstat ) );
                Assert( defragstat.FPassFull() );
                csecStartPass = UlUtilGetSeconds();
            }
        }
        else
        {
            Call( err );
            Assert( JET_wrnSeekNotEqual == err );


            Call( ErrOLDIExplicitDefragOneTable(
                        ppib,
                        pfucbCatalog,
                        pfucbDefrag,
                        defragstat ) );

            if ( !FOLDContinue( ifmp ) )
                break;

            defragstat.SetTypeNull();
            defragstat.SetObjidNextTable();
        }

        objidNext = defragstat.ObjidCurrentTable();
    }

    if ( !defragstat.FPassCompleted() )
    {
        const WCHAR * rgszT[1] = { g_rgfmp[ifmp].WszDatabaseName() };
        UtilReportEvent(
            eventInformation,
            ONLINE_DEFRAG_CATEGORY,
            OLD_INTERRUPTED_PASS_ID,
            1,
            rgszT,
            0,
            NULL,
            pinst );
    }

HandleError:

    OSStrCbFormatA( szTrace, sizeof(szTrace), "OLD END (ifmp %d, err %d)", (ULONG)ifmp, err );
    CallS( pinst->m_plog->ErrLGTrace( ppib, szTrace ) );

    Assert( pfucbNil != pfucbCatalog );
    CallS( ErrCATClose( ppib, pfucbCatalog ) );

    return err;
}

#ifndef RTM

LOCAL ERR ErrOLDInternalTest(
    PIB * const                 ppib,
    FUCB * const                pfucbDefrag)
{
    ERR                         err;
    __int64                     value;
    
    Assert( !Pcsr( pfucbDefrag )->FLatched() );

    CallR( ErrDIRBeginTransaction( ppib, 59173, NO_GRBIT ) );

    
    err = ErrIsamMove( ppib, pfucbDefrag, JET_MoveFirst, NO_GRBIT );
    Assert( JET_errNoCurrentRecord != err );
    Assert( JET_errRecordNotFound != err );
    Call( err );
    
    Call( ErrOLDRetrieveLongLongColumn( ppib, pfucbDefrag, fidOLDPassInvocations, &value ) );
    AssertRTL( 0 == value );

    Call( ErrIsamPrepareUpdate( ppib, pfucbDefrag, JET_prepReplace ) );
    Call( ErrOLDSetLongLongColumn( ppib, pfucbDefrag, fidOLDPassInvocations, 7 ) );
    Call( ErrIsamUpdate( ppib, pfucbDefrag, NULL, 0, NULL, NO_GRBIT ) );
    
    Call( ErrOLDRetrieveLongLongColumn( ppib, pfucbDefrag, fidOLDPassInvocations, &value ) );
    AssertRTL( 7 == value );

    Call( ErrIsamPrepareUpdate( ppib, pfucbDefrag, JET_prepReplace ) );
    Call( ErrOLDIncrementLongLongColumn( ppib, pfucbDefrag, fidOLDPassInvocations, 2 ) );
    Call( ErrIsamUpdate( ppib, pfucbDefrag, NULL, 0, NULL, NO_GRBIT ) );
    
    Call( ErrOLDRetrieveLongLongColumn( ppib, pfucbDefrag, fidOLDPassInvocations, &value ) );
    AssertRTL( 9 == value );

    Call( ErrDIRRollback( ppib ) );
    
HandleError:
    return err;
}

#endif

LOCAL ERR ErrOLDCreate( PIB *ppib, const IFMP ifmp )
{
    ERR             err;
    FUCB            *pfucb;
    DATA            dataField;
    OBJID           objidFDP    = objidFDPMSO;

    JET_COLUMNCREATE_A  rgjccOLD[]                      =
    {
        sizeof(JET_COLUMNCREATE_A), "ObjidFDP",             JET_coltypLong,         0,  JET_bitColumnFixed | JET_bitColumnNotNULL,  NULL,   0,  0,  0,  0,
        sizeof(JET_COLUMNCREATE_A), "Status",               JET_coltypShort,        0,  JET_bitColumnFixed, NULL,   0,  0,  0,  0,
        sizeof(JET_COLUMNCREATE_A), "PassStartDateTime",    JET_coltypLongLong,     0,  JET_bitColumnFixed, &zero,  sizeof(zero),   0,  0,  0,
        sizeof(JET_COLUMNCREATE_A), "PassElapsedSeconds",   JET_coltypLongLong,     0,  JET_bitColumnFixed, &zero,  sizeof(zero),   0,  0,  0,
        sizeof(JET_COLUMNCREATE_A), "PassInvocations",      JET_coltypLongLong,     0,  JET_bitColumnFixed, &zero,  sizeof(zero),   0,  0,  0,
        sizeof(JET_COLUMNCREATE_A), "PassPagesVisited",     JET_coltypLongLong,     0,  JET_bitColumnFixed, &zero,  sizeof(zero),   0,  0,  0,
        sizeof(JET_COLUMNCREATE_A), "PassPagesFreed",       JET_coltypLongLong,     0,  JET_bitColumnFixed, &zero,  sizeof(zero),   0,  0,  0,
        sizeof(JET_COLUMNCREATE_A), "PassPartialMerges",    JET_coltypLongLong,     0,  JET_bitColumnFixed, &zero,  sizeof(zero),   0,  0,  0,
        sizeof(JET_COLUMNCREATE_A), "TotalPasses",          JET_coltypLongLong,     0,  JET_bitColumnFixed, &zero,  sizeof(zero),   0,  0,  0,
        sizeof(JET_COLUMNCREATE_A), "TotalElapsedSeconds",  JET_coltypLongLong,     0,  JET_bitColumnFixed, &zero,  sizeof(zero),   0,  0,  0,
        sizeof(JET_COLUMNCREATE_A), "TotalInvocations",     JET_coltypLongLong,     0,  JET_bitColumnFixed, &zero,  sizeof(zero),   0,  0,  0,
        sizeof(JET_COLUMNCREATE_A), "TotalDefragDays",      JET_coltypLongLong,     0,  JET_bitColumnFixed, &zero,  sizeof(zero),   0,  0,  0,
        sizeof(JET_COLUMNCREATE_A), "TotalPagesVisited",    JET_coltypLongLong,     0,  JET_bitColumnFixed, &zero,  sizeof(zero),   0,  0,  0,
        sizeof(JET_COLUMNCREATE_A), "TotalPagesFreed",      JET_coltypLongLong,     0,  JET_bitColumnFixed, &zero,  sizeof(zero),   0,  0,  0,
        sizeof(JET_COLUMNCREATE_A), "TotalPartialMerges",   JET_coltypLongLong,     0,  JET_bitColumnFixed, &zero,  sizeof(zero),   0,  0,  0,

        sizeof(JET_COLUMNCREATE_A), "CurrentKey",           JET_coltypLongBinary,   0,  JET_bitColumnTagged,NULL,   0,  0,  0,  0,
    };
    const ULONG     ccolOLD                         = _countof( rgjccOLD );

    JET_TABLECREATE5_A  jtcOLD      =
    {
        sizeof(JET_TABLECREATE5_A),
        (CHAR *)szOLD,
        NULL,
        0,
        100,
        rgjccOLD,
        ccolOLD,
        NULL,
        0,
        NULL,
        0,
        JET_bitTableCreateFixedDDL|JET_bitTableCreateSystemTable,
        NULL,
        NULL,
        0,
        0,
        JET_TABLEID( pfucbNil ),
        0
    };

    CallR( ErrDIRBeginTransaction( ppib, 34597, NO_GRBIT ) );

    err = ErrFILEDeleteTable( ppib, ifmp, szOLDObsolete );
    if( JET_errObjectNotFound == err )
    {
        err = JET_errSuccess;
    }
    Call( err );
    
    Call( ErrFILECreateTable( ppib, ifmp, &jtcOLD ) );
    pfucb = (FUCB *)jtcOLD.tableid;

    Call( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepInsert ) )
    dataField.SetPv( &objidFDP );
    dataField.SetCb( sizeof(OBJID) );
    Call( ErrRECSetColumn(
                pfucb,
                fidOLDObjidFDP,
                1,
                &dataField ) );
    Call( ErrIsamUpdate( ppib, pfucb, NULL, 0, NULL, NO_GRBIT ) );

#ifndef RTM
    Call( ErrOLDInternalTest( ppib, pfucb ) );
#endif

    Call( ErrFILECloseTable( ppib, pfucb ) );

    Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );

    return JET_errSuccess;

HandleError:
    CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );

    return err;
}


DWORD OLDDefragDb( DWORD_PTR dw )
{
    ERR                     err;
    IFMP                    ifmp            = (IFMP)dw;
    INST * const            pinst           = PinstFromIfmp( ifmp );
    OLDDB_STATUS * const    poldstatDB      = pinst->m_rgoldstatDB + g_rgfmp[ifmp].Dbid();
    PIB *                   ppib            = ppibNil;
    FUCB *                  pfucb           = pfucbNil;
    BOOL                    fOpenedDb       = fFalse;
    DEFRAG_STATUS           defragstat;
    DATA                    dataField;

    Assert( 0 == poldstatDB->CPasses() );

    if ( !FOLDContinue( ifmp ) )
    {
        return 0;
    }

    CallR( ErrPIBBeginSession( pinst, &ppib, procidNil, fFalse ) );
    Assert( ppibNil != ppib );

    ppib->SetFUserSession();
    ppib->SetFSessionOLD();
    ppib->grbitCommitDefault = JET_bitCommitLazyFlush;

    Call( ErrDBOpenDatabaseByIfmp( ppib, ifmp ) );
    fOpenedDb = fTrue;

    err = ErrFILEOpenTable( ppib, ifmp, &pfucb, szOLD, JET_bitTableTryPurgeOnClose );
    if ( err < 0 )
    {
        if ( JET_errObjectNotFound != err )
            goto HandleError;

        Call( ErrOLDCreate( ppib, ifmp ) );
        Call( ErrFILEOpenTable( ppib, ifmp, &pfucb, szOLD, JET_bitTableTryPurgeOnClose ) );
    }

    Assert( pfucbNil != pfucb );


    err = ErrIsamMove( ppib, pfucb, JET_MoveFirst, NO_GRBIT );
    Assert( JET_errNoCurrentRecord != err );
    Assert( JET_errRecordNotFound != err );
    Call( err );

    Assert( !Pcsr( pfucb )->FLatched() );
    Call( ErrDIRGet( pfucb ) );

    Assert( FFixedFid( fidOLDObjidFDP ) );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucb->u.pfcb->Ptdb(),
                fidOLDObjidFDP,
                pfucb->kdfCurr.data,
                &dataField ) );
    CallS( err );
    Assert( dataField.Cb() == sizeof(OBJID) );
    defragstat.SetObjidCurrentTable( *( (UnalignedLittleEndian< OBJID > *)dataField.Pv() ) );

    Assert( FFixedFid( fidOLDStatus ) );
    Call( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfucb->u.pfcb->Ptdb(),
                fidOLDStatus,
                pfucb->kdfCurr.data,
                &dataField ) );
    if ( JET_errSuccess == err )
    {
        Assert( dataField.Cb() == sizeof(DEFRAGTYPE) );
        defragstat.SetType( *( (UnalignedLittleEndian< DEFRAGTYPE > *)dataField.Pv() ) );
        Assert( defragstat.FTypeTable() || defragstat.FTypeLV() || defragstat.FTypeIndex() );

        Assert( FTaggedFid( fidOLDCurrentKey ) );
        Call( ErrRECIRetrieveTaggedColumn(
                    pfucb->u.pfcb,
                    ColumnidOfFid( fidOLDCurrentKey, fFalse ),
                    1,
                    pfucb->kdfCurr.data,
                    &dataField ) );

        const INT cbDataField = dataField.Cb();
        
        AssertRTL( cbDataField <= cbKeyMostMost );
        if ( cbDataField > cbKeyMostMost )
        {
            err = ErrERRCheck( JET_wrnBufferTruncated );
        }

        Assert( wrnRECUserDefinedDefault != err );
        Assert( wrnRECSeparatedLV != err );

        Assert( wrnRECLongField != err );
        if ( JET_errSuccess == err || wrnRECIntrinsicLV == err )
        {
            memcpy( defragstat.RgbCurrentKey(), dataField.Pv(), cbDataField );
            defragstat.SetCbCurrentKey( cbDataField );
        }
        else if ( wrnRECCompressed == err )
        {
            INT cbActual;
            CallS( ErrPKDecompressData(
                    dataField,
                    pfucb,
                    defragstat.RgbCurrentKey(),
                    cbKeyAlloc,
                    &cbActual ) );
            defragstat.SetCbCurrentKey( cbActual );
        }
        else
        {
            Assert( JET_wrnColumnNull == err );
            Assert( 0 == cbDataField );
            defragstat.SetCbCurrentKey( 0 );
        }
    }
    else
    {
        Assert( JET_wrnColumnNull == err );
        defragstat.SetTypeNull();
    }

    if ( poldstatDB->FAvailExtOnly() )
    {
        defragstat.SetTypeNull();
        defragstat.SetObjidCurrentTable( objidFDPMSO );
    }

    CallS( ErrDIRRelease( pfucb ) );

    Call( ErrOLDIExplicitDefragTables( ppib, pfucb, defragstat ) );

    Call( ErrOLDStatusUpdate( ppib, pfucb, defragstat ) );

    if( NULL != poldstatDB->Callback() )
    {
        Assert( ppibNil != ppib );
        (VOID)( poldstatDB->Callback() )(
                    reinterpret_cast<JET_SESID>( ppib ),
                    static_cast<JET_DBID>( ifmp ),
                    JET_tableidNil,
                    JET_cbtypOnlineDefragCompleted,
                    NULL,
                    NULL,
                    NULL,
                    0 );
    }

HandleError:
    Assert( ppibNil != ppib );

    if ( err < 0 )
    {
        WCHAR       szErr[16];
        const WCHAR *rgszT[2];

        OSStrCbFormatW( szErr, sizeof(szErr), L"%d", err );

        rgszT[0] = g_rgfmp[ifmp].WszDatabaseName();
        rgszT[1] = szErr;

        UtilReportEvent(
            eventWarning,
            ONLINE_DEFRAG_CATEGORY,
            OLD_ERROR_ID,
            2,
            rgszT,
            0,
            NULL,
            pinst );
    }

    if ( pfucbNil != pfucb )
    {
        CallS( ErrFILECloseTable( ppib, pfucb ) );
    }
    if ( fOpenedDb )
    {
        CallS( ErrDBCloseAllDBs( ppib ) );
    }

    PIBEndSession( ppib );

    pinst->m_critOLD.Enter();
    if ( !poldstatDB->FTermRequested() )
    {

        poldstatDB->ThreadEnd();
#ifdef OLD_SCRUB_DB
        poldstatDB->ScrubThreadEnd();
#endif
        poldstatDB->Reset( pinst );
    }
    pinst->m_critOLD.Leave();

    return 0;
}


#ifdef OLD_SCRUB_DB

ULONG OLDScrubDb( DWORD_PTR dw )
{
    const CPG cpgPreread = 256;

    ERR             err;
    IFMP            ifmp            = (IFMP)dw;
    PIB             *ppib           = ppibNil;
    BOOL            fOpenedDb       = fFalse;
    SCRUBDB         * pscrubdb      = NULL;

    const ULONG_PTR ulSecStart = UlUtilGetSeconds();

    CallR( ErrPIBBeginSession( PinstFromIfmp( ifmp ), &ppib, procidNil, fFalse ) );
    Assert( ppibNil != ppib );

    ppib->SetFUserSession();
    ppib->grbitCommitDefault = JET_bitCommitLazyFlush;

    Call( ErrDBOpenDatabaseByIfmp( ppib, ifmp ) );
    fOpenedDb = fTrue;

    pscrubdb = new SCRUBDB( ifmp );
    if( NULL == pscrubdb )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    while( FOLDContinue( ifmp ) )
    {
        const WCHAR * rgszT[1];
        INT isz = 0;

        rgszT[isz++] = g_rgfmp[ifmp].WszDatabaseName();
        Assert( isz <= _countof( rgszT ) );

        UtilReportEvent(
                eventInformation,
                DATABASE_ZEROING_CATEGORY,
                DATABASE_ZEROING_STARTED_ID,
                isz,
                rgszT,
                0,
                NULL,
                PinstFromIfmp( ifmp ) );

        PGNO pgnoLast;
        pgnoLast = g_rgfmp[ifmp].PgnoLast();

        DBTIME dbtimeLastScrubNew;
        dbtimeLastScrubNew = g_rgfmp[ifmp].Dbtime();

        Call( pscrubdb->ErrInit( ppib, CUtilProcessProcessor() ) );

        PGNO pgno;
        pgno = 1;

        while( pgnoLast >= pgno && FOLDContinue( ifmp ) )
        {
            ULONG_PTR cpgCache;
            CallS( ErrBFGetCacheSize( &cpgCache ) );

            const CPG cpgChunk      = 256;
            const CPG cpgPreread    = min( cpgChunk, pgnoLast - pgno + 1 );
            BFPrereadPageRange( ifmp, pgno, bfprfDefault, BfpriBackgroundRead( m_ifmp, ppib ), cpgPreread );

            Call( pscrubdb->ErrScrubPages( pgno, cpgPreread ) );
            pgno += cpgPreread;

            while( ( pscrubdb->Scrubstats().cpgSeen + ( cpgCache / 4 ) ) < pgno
                   && FOLDContinue( ifmp ) )
            {
                UtilSleep( cmsecWaitGeneric );
            }
        }

        Call( pscrubdb->ErrTerm() );

        if( pgno > pgnoLast )
        {
            g_rgfmp[ifmp].Pdbfilehdr()->dbtimeLastScrub = dbtimeLastScrubNew;
            LGIGetDateTime( &g_rgfmp[ifmp].Pdbfilehdr()->logtimeScrub );
        }

        {
            const ULONG_PTR ulSecFinished   = UlUtilGetSeconds();
            const ULONG_PTR ulSecs          = ulSecFinished - ulSecStart;

            const WCHAR * rgszT[16];
            INT isz = 0;

            WCHAR   szSeconds[16];
            WCHAR   szErr[16];
            WCHAR   szPages[16];
            WCHAR   szBlankPages[16];
            WCHAR   szUnchangedPages[16];
            WCHAR   szUnusedPages[16];
            WCHAR   szUsedPages[16];
            WCHAR   szDeletedRecordsZeroed[16];
            CHAR    szOrphanedLV[16];

            OSStrCbFormatW( szSeconds, sizeof(szSeconds), L"%d", ulSecs );
            OSStrCbFormatW( szErr, sizeof(szErr), L"%d", err );
            OSStrCbFormatW( szPages, sizeof(szPages), L"%d", pscrubdb->Scrubstats().cpgSeen );
            OSStrCbFormatW( szBlankPages, sizeof(szBlankPages), L"%d", pscrubdb->Scrubstats().cpgUnused );
            OSStrCbFormatW( szUnchangedPages, sizeof(szUnchangedPages), L"%d", pscrubdb->Scrubstats().cpgUnchanged );
            OSStrCbFormatW( szUnusedPages, sizeof(szUnusedPages), L"%d", pscrubdb->Scrubstats().cpgZeroed );
            OSStrCbFormatW( szUsedPages, sizeof(szUsedPages), L"%d", pscrubdb->Scrubstats().cpgUsed );
            OSStrCbFormatW( szDeletedRecordsZeroed, sizeof(szDeletedRecordsZeroed), L"%d", pscrubdb->Scrubstats().cFlagDeletedNodesZeroed );
            OSStrCbFormatW( szOrphanedLV, sizeof(szOrphanedLV), L"%d", pscrubdb->Scrubstats().cOrphanedLV );

            rgszT[isz++] = g_rgfmp[ifmp].WszDatabaseName();
            rgszT[isz++] = szSeconds;
            rgszT[isz++] = szErr;
            rgszT[isz++] = szPages;
            rgszT[isz++] = szBlankPages;
            rgszT[isz++] = szUnchangedPages;
            rgszT[isz++] = szUnusedPages;
            rgszT[isz++] = szUsedPages;
            rgszT[isz++] = szDeletedRecordsZeroed;
            rgszT[isz++] = szOrphanedLV;

            Assert( isz <= _countof( rgszT ) );
            UtilReportEvent(
                    eventInformation,
                    DATABASE_ZEROING_CATEGORY,
                    DATABASE_ZEROING_STOPPED_ID,
                    isz,
                    rgszT,
                    0,
                    NULL,
                    PinstFromIfmp( ifmp ) );
        }

        pinst->m_rgoldstatDB[ g_rgfmp[ifmp].Dbid() ].FWaitOnSignal( 60 * 1000 );
    }

HandleError:
    if ( fOpenedDb )
    {
        CallS( ErrDBCloseAllDBs( ppib ) );
    }

    if( NULL != pscrubdb )
    {
        (VOID)pscrubdb->ErrTerm();
        delete pscrubdb;
    }

    Assert( ppibNil != ppib );
    PIBEndSession( ppib );

    return 0;
}

#endif


ERR ErrOLDDefragment(
    const IFMP      ifmp,
    const CHAR *    szTableName,
    ULONG *         pcPasses,
    ULONG *         pcsec,
    JET_CALLBACK    callback,
    JET_GRBIT       grbit )
{
    ERR             err                 = JET_errSuccess;

    const BOOL      fReturnPassCount    = ( NULL != pcPasses && ( grbit & JET_bitDefragmentBatchStop ) );
    const BOOL      fReturnElapsedTime  = ( NULL != pcsec && ( grbit & JET_bitDefragmentBatchStop ) );

    if ( fReturnPassCount )
        *pcPasses = 0;

    if ( fReturnElapsedTime )
        *pcsec = 0;

#ifdef MINIMAL_FUNCTIONALITY
    if (callback != nullptr)
    {
        err = ErrERRCheck( JET_errDisabledFunctionality );
    }

    return err;
#else

    INST * const    pinst               = PinstFromIfmp( ifmp );
    const LONG      fOLDLevel           = ( GrbitParam( pinst, JET_paramEnableOnlineDefrag ) & JET_OnlineDefragAllOBSOLETE ?
                                                JET_OnlineDefragAll :
                                                GrbitParam( pinst, JET_paramEnableOnlineDefrag ) );
    const BOOL      fAvailExtOnly       = ( ( grbit & JET_bitDefragmentAvailSpaceTreesOnly ) ?
                                                ( fOLDLevel & (JET_OnlineDefragDatabases|JET_OnlineDefragSpaceTrees) ) :
                                                ( ( fOLDLevel & JET_OnlineDefragSpaceTrees )
                                                    && !( fOLDLevel & JET_OnlineDefragDatabases ) ) );
    const BOOL      fNoPartialMerges    = !!( grbit & JET_bitDefragmentNoPartialMerges );

    grbit &= ~ ( JET_bitDefragmentAvailSpaceTreesOnly | JET_bitDefragmentNoPartialMerges );

    Assert( !pinst->FRecovering() );
    Assert( !g_fRepair );
    
    if ( 0 == fOLDLevel )
        return JET_errSuccess;

    if ( FFMPIsTempDB( ifmp ) )
    {
        err = ErrERRCheck( JET_errInvalidDatabaseId );
        return err;
    }
    Assert( g_rgfmp[ ifmp ].Dbid() > 0 );

    if ( g_rgfmp[ ifmp ].FReadOnlyAttach() )
    {
        err = ErrERRCheck( JET_errDatabaseFileReadOnly );
        return err;
    }

    pinst->m_critOLD.Enter();

    OLDDB_STATUS * const        poldstatDB  = pinst->m_rgoldstatDB + g_rgfmp[ifmp].Dbid();

    switch ( grbit )
    {
        case JET_bitDefragmentBatchStart:
            if ( poldstatDB->FRunning() )
            {
                err = ErrERRCheck( JET_wrnDefragAlreadyRunning );
            }
            else if ( !( fOLDLevel & JET_OnlineDefragDatabases )
                    && !fAvailExtOnly )
            {
                err = JET_errSuccess;
            }
            else if ( g_rgfmp[ ifmp ].FDontStartOLD() )
            {
                err = JET_errSuccess;
            }
            else
            {
                poldstatDB->Reset( pinst );

                if ( fAvailExtOnly )
                    poldstatDB->SetFAvailExtOnly();

                if ( fNoPartialMerges )
                    poldstatDB->SetFNoPartialMerges();

                if ( NULL != pcPasses )
                    poldstatDB->SetCPassesMax( *pcPasses );

                poldstatDB->SetCsecStart( UlUtilGetSeconds() );
                if ( NULL != pcsec && *pcsec > 0 )
                    poldstatDB->SetCsecMax( poldstatDB->CsecStart() + *pcsec );

                if ( NULL != callback )
                    poldstatDB->SetCallback( callback );

                Assert( !g_rgfmp[ifmp].FDontStartOLD() );

                err = poldstatDB->ErrThreadCreate( ifmp, OLDDefragDb );

#ifdef OLD_SCRUB_DB
                if( err >= 0 )
                {
                    EnforceSz( fFalse, "ScrubWithOldNotSupported" );

                    err = poldstatDB->ErrScrubThreadCreate( ifmp );
                }
#endif
            }
            break;
        case JET_bitDefragmentBatchStop:
            if ( !poldstatDB->FRunning()
                || poldstatDB->FTermRequested() )
            {
                if ( fOLDLevel & (JET_OnlineDefragDatabases|JET_OnlineDefragSpaceTrees) )
                {
                    err = ErrERRCheck( JET_wrnDefragNotRunning );
                }
                else
                {
                    err = JET_errSuccess;
                }
            }
            else
            {
                poldstatDB->SetFTermRequested();
                pinst->m_critOLD.Leave();

                poldstatDB->SetSignal();
                if ( poldstatDB->FRunning() )
                {
                    poldstatDB->ThreadEnd();
#ifdef OLD_SCRUB_DB
                    poldstatDB->ScrubThreadEnd();
#endif
                }

                pinst->m_critOLD.Enter();

                if ( fReturnPassCount )
                    *pcPasses = poldstatDB->CPasses();
                if ( fReturnElapsedTime )
                    *pcsec = (ULONG)( UlUtilGetSeconds() - poldstatDB->CsecStart() );

                poldstatDB->Reset( pinst );

                err = JET_errSuccess;
            }
            break;

        default:
            err = ErrERRCheck( JET_errInvalidGrbit );
    }

    pinst->m_critOLD.Leave();

    return err;

#endif
}

LOCAL ERR ErrOLDIEnsureMsysDefragTableCreated(
    _In_ const IFMP ifmp
    )
{
    ERR err = JET_errSuccess;
    INST * const pinst = PinstFromIfmp( ifmp );
    PIB* ppib = ppibNil;
    FUCB* pfucbDefragStatus = pfucbNil;

    Call( ErrPIBBeginSession( pinst, &ppib, procidNil, fFalse ) );
    ppib->SetFUserSession();
    ppib->SetFSessionOLD2();

    Call( ErrDBOpenDatabaseByIfmp( ppib, ifmp ) );
    Call( OLD2_STATUS::ErrOpenOrCreateTable( ppib, ifmp, &pfucbDefragStatus ) );
    CallS( ErrFILECloseTable( ppib, pfucbDefragStatus ) );

HandleError:
    if ( ppibNil != ppib )
    {
        CallS( ErrDBCloseAllDBs( ppib ) );
        PIBEndSession( ppib );
    }

    return err;
}

ERR ErrOLDRegisterObjectForOLD2(
    _In_ const IFMP ifmp,
    _In_z_ const CHAR * const szTable,
    _In_z_ const CHAR * const szIndex,
    _In_ DEFRAGTYPE defragtype )
{
    ERR err;

    if ( g_rgfmp[ ifmp ].FReadOnlyAttach() )
    {
        Error( ErrERRCheck( JET_errDatabaseFileReadOnly ) )
    }

    Call( ErrOLDIEnsureMsysDefragTableCreated( ifmp ) );

    if ( defragtypeAll == defragtype )
    {
        Call( CDefragManager::Instance().ErrExplicitRegisterTableAndChildren( ifmp, szTable ) );
    }
    else
    {
        Call( CDefragManager::Instance().ErrRegisterOneTreeOnly( ifmp, szTable, szIndex, defragtype ) );
    }

HandleError:

    return err;
}

ERR ErrOLDInit()
{
    ERR err;

    Call( CDefragManager::Instance().ErrInit() );
    
HandleError:
    return err;
}

VOID OLDTerm()
{
    CDefragManager::Instance().Term();
}

ERR ErrIsamDefragment(
    JET_SESID   vsesid,
    JET_DBID    vdbid,
    const CHAR  *szTableName,
    ULONG       *pcPasses,
    ULONG       *pcsec,
    JET_CALLBACK callback,
    JET_GRBIT   grbit )
{
    ERR         err;
    PIB         *ppib       = (PIB *)vsesid;
    const IFMP  ifmp        = IFMP( vdbid );

    Call( ErrPIBCheck( ppib ) );
    Call( ErrPIBCheckIfmp( ppib, ifmp ) );
    Call( ErrPIBCheckUpdatable( ppib ) );

    if ( grbit & JET_bitDefragmentBTreeBatch )
    {

        const JET_GRBIT grbitSubOptions = grbit & ~JET_bitDefragmentBTreeBatch;

        if ( pcPasses != NULL )
        {
            AssertSz( FNegTest( fInvalidAPIUsage ), "The JET_bitDefragmentBTreeBatch option(s) do not honor, nor can set the pcPasses parameter, so no point in provided (must be NULL). Reserved for future." );
            Call( ErrERRCheck( JET_errInvalidParameter ) );
        }
        if ( pcsec != NULL )
        {
            AssertSz( FNegTest( fInvalidAPIUsage ), "The JET_bitDefragmentBTreeBatch option(s) do not honor, nor can set the pcsec parameter, so no point in provided (must be NULL). Reserved for future." );
            Call( ErrERRCheck( JET_errInvalidParameter ) );
        }
        if ( grbitSubOptions != JET_bitDefragmentBatchStart )
        {
            AssertSz( FNegTest( fInvalidAPIUsage ), "Unknown additional grbit (%#x) provided with JET_bitDefragmentBTreeBatch.", grbitSubOptions );
            Error( ErrERRCheck( JET_errInvalidGrbit ) );
        }
        if ( callback != NULL )
        {
            AssertSz( FNegTest( fInvalidAPIUsage ), "The JET_bitDefragmentBTreeBatch option(s) do not honor the callback parameter, so no point in providing (must be NULL). Reserved for future." );
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
        if ( szTableName != NULL )
        {
            AssertSz( FNegTest( fInvalidAPIUsage ), "Used the JET_bitDefragmentBTreeBatch in conjuction with a table name, these arguments can not currently be used together (must be NULL). Reserved for future." );
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }

        Call( ErrOLD2Resume( (PIB*)vsesid, (IFMP)vdbid ) );
    }
    else if ( grbit & JET_bitDefragmentBTree )
    {

        if ( pcPasses )
        {
            *pcPasses = 0;
        }
        if ( pcsec )
        {
            *pcsec = 0;
        }

        if ( grbit & ~JET_bitDefragmentBTree )
        {
            AssertSz( FNegTest( fInvalidAPIUsage ), "Unknown additional grbit (%#x) provided with JET_bitDefragmentBTree.", grbit & ~JET_bitDefragmentBTree );
            Error( ErrERRCheck( JET_errInvalidGrbit ) );
        }
        if ( callback != NULL )
        {
            AssertSz( FNegTest( fInvalidAPIUsage ), "The JET_bitDefragmentBTree option(s) do not honor the callback parameter, so no point in providing (must be NULL). Reserved for future." );
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }


        if ( szTableName == NULL )
        {
            AssertSz( FNegTest( fInvalidAPIUsage ), "Specified null table name to defragment with the JET_bitDefragmentBTree bit. By default should pass a table name or other options (e.g. JET_bitDefragmentBatchStart)." );
            Error( ErrERRCheck( JET_errInvalidName ) );
        }

        Call( ErrOLDRegisterObjectForOLD2( ifmp, szTableName, NULL, defragtypeAll ) );
    }
    else
    {
        Call( ErrOLDDefragment( ifmp, szTableName, pcPasses, pcsec, callback, grbit ) );
    }

HandleError:

    return err;
}

OLD2_STATUS::OLD2_STATUS( const OBJID objidTable, const OBJID objidFDP ) :
    m_cpgVisited ( 0 ),
    m_cpgFreed( 0 ),
    m_cpgPartialMerges( 0 ),
    m_cpgMoved( 0 ),
    m_objidTable( objidTable ),
    m_objidFDP( objidFDP ),
    m_cbBookmarkKey( 0 ),
    m_cbBookmarkData( 0 ),
    m_startTime( 0 )
{
    Assert( objidNil != objidTable );
    Assert( objidNil != objidFDP );
    Assert( objidFDP >= objidTable );

#ifndef RTM
    Test();
#endif
}

VOID OLD2_STATUS::SetStartTime( const __int64 time )
{
    Assert( 0 == m_startTime );
    m_startTime = time;
    Assert( 0 != m_startTime );
}

BOOKMARK OLD2_STATUS::GetBookmark()
{
    BOOKMARK bm;
    bm.key.prefix.Nullify();
    bm.key.suffix.SetPv( m_rgbBookmark );
    bm.key.suffix.SetCb( m_cbBookmarkKey );
    bm.data.SetPv( m_rgbBookmark + m_cbBookmarkKey );
    bm.data.SetCb( m_cbBookmarkData );
    return bm;
}

VOID OLD2_STATUS::SetBookmark( const BOOKMARK& bm )
{
    BYTE * pbBuffer = m_rgbBookmark;
    size_t cbBufferRemaining = m_cbBookmark;

    bm.key.CopyIntoBuffer( pbBuffer, cbBufferRemaining );

    m_cbBookmarkKey = bm.key.Cb();
    pbBuffer += m_cbBookmarkKey;
    cbBufferRemaining -= m_cbBookmarkKey;

    Assert( cbBufferRemaining >= (size_t)bm.data.Cb() );
    m_cbBookmarkData = min( (size_t)bm.data.Cb(), cbBufferRemaining );
    memcpy( pbBuffer, bm.data.Pv(), m_cbBookmarkData );
}

ERR OLD2_STATUS::ErrSetLongLong_(
    __in PIB * const ppib,
    __in FUCB * const pfucb,
    const FID fid,
    const __int64 qwValue )
{
    ERR err;
    DATA dataField;
    
    dataField.SetPv( const_cast<__int64 *>( &qwValue ) );
    dataField.SetCb( sizeof(qwValue) );
    Call( ErrRECSetColumn(
                pfucb,
                fid,
                1,
                &dataField ) );

HandleError:
    return err;
}

ERR OLD2_STATUS::ErrGetLongLong_(
    __in PIB * const ppib,
    __in FUCB * const pfucb,
    const FID fid,
    __out __int64 * const pqwValue )
{
    ERR err;

    *pqwValue = 0;

    ULONG cbActual;
    Call( ErrIsamRetrieveColumn(
        ppib,
        pfucb,
        fid,
        pqwValue,
        sizeof( __int64 ),
        &cbActual,
        NO_GRBIT,
        NULL ) );
    CallS( err );
    Assert( sizeof( __int64 ) == cbActual );

HandleError:
    return err;
}

ERR OLD2_STATUS::ErrSetLong_(
    __in PIB * const ppib,
    __in FUCB * const pfucb,
    const FID fid,
    const LONG lValue )
{
    ERR err;
    DATA dataField;
    
    dataField.SetPv( const_cast<LONG *>( &lValue ) );
    dataField.SetCb( sizeof(lValue) );
    Call( ErrRECSetColumn(
                pfucb,
                fid,
                1,
                &dataField ) );

HandleError:
    return err;
}

ERR OLD2_STATUS::ErrGetLong_(
    __in PIB * const ppib,
    __in FUCB * const pfucb,
    const FID fid,
    __out LONG * const plValue )
{
    ERR err;

    *plValue = 0;

    ULONG cbActual;
    Call( ErrIsamRetrieveColumn(
        ppib,
        pfucb,
        fid,
        plValue,
        sizeof( LONG ),
        &cbActual,
        NO_GRBIT,
        NULL ) );
    CallS( err );
    Assert( sizeof( LONG ) == cbActual );

HandleError:
    return err;
}

ERR OLD2_STATUS::ErrSeek_(
    __in PIB * const            ppib,
    __in FUCB * const           pfucbDefrag,
    __in const OLD2_STATUS&     old2status )
{
    ERR err;

    const OBJID objidTable  = old2status.ObjidTable();
    const OBJID objidFDP    = old2status.ObjidFDP();
    
    Call( ErrIsamMakeKey(
                ppib,
                pfucbDefrag,
                &objidTable,
                sizeof(objidTable),
                JET_bitNewKey ) );
    Call( ErrIsamMakeKey(
                ppib,
                pfucbDefrag,
                &objidFDP,
                sizeof(objidFDP),
                NO_GRBIT ) );
    Call( ErrIsamSeek( ppib, pfucbDefrag, JET_bitSeekEQ ) );

    CallS( ErrCheckCurrency_( ppib, pfucbDefrag, old2status ) );
    
HandleError:
    return err;
}

ERR OLD2_STATUS::ErrCheckCurrency_(
    __in PIB * const        ppib,
    __in FUCB * const       pfucbDefrag,
    __in const OLD2_STATUS& old2status )
{
    ERR err;

    const OBJID objidTableExpected  = old2status.ObjidTable();
    const OBJID objidFDPExpected    = old2status.ObjidFDP();

    OBJID objidTableActual;
    OBJID objidFDPActual;

    Call( ErrGetLong_( ppib, pfucbDefrag, s_fidOLD2ObjidTable, (LONG *)&objidTableActual ) );
    Call( ErrGetLong_( ppib, pfucbDefrag, s_fidOLD2ObjidFDP, (LONG *)&objidFDPActual ) );
    
    AssertRTL( objidTableActual == objidTableExpected );
    AssertRTL( objidFDPActual == objidFDPExpected );

HandleError:
    return err;
}

ERR OLD2_STATUS::ErrCreateTable_(
    __in PIB * const ppib,
    const IFMP ifmp )
{
    Assert( s_crit.FOwner() );

    JET_COLUMNCREATE_A rgjccOLD2[] =
    {
            { sizeof(JET_COLUMNCREATE_A),   "ObjidTable",       JET_coltypLong,     0,  JET_bitColumnFixed | JET_bitColumnNotNULL,  NULL,   0,  0,  0,  0 },
            { sizeof(JET_COLUMNCREATE_A),   "ObjidFDP",         JET_coltypLong,     0,  JET_bitColumnFixed | JET_bitColumnNotNULL,  NULL,   0,  0,  0,  0 },
            { sizeof(JET_COLUMNCREATE_A),   "StartDateTime",    JET_coltypLongLong, 0,  JET_bitColumnFixed | JET_bitColumnNotNULL,  NULL,   0,  0,  0,  0 },
            { sizeof(JET_COLUMNCREATE_A),   "PagesVisited",     JET_coltypLong,     0,  JET_bitColumnFixed | JET_bitColumnNotNULL,  NULL,   0,  0,  0,  0 },
            { sizeof(JET_COLUMNCREATE_A),   "PagesFreed",       JET_coltypLong,     0,  JET_bitColumnFixed | JET_bitColumnNotNULL,  NULL,   0,  0,  0,  0 },
            { sizeof(JET_COLUMNCREATE_A),   "PartialMerges",    JET_coltypLong,     0,  JET_bitColumnFixed | JET_bitColumnNotNULL,  NULL,   0,  0,  0,  0 },
            { sizeof(JET_COLUMNCREATE_A),   "PageMoves",        JET_coltypLong,     0,  JET_bitColumnFixed | JET_bitColumnNotNULL,  NULL,   0,  0,  0,  0 },
            { sizeof(JET_COLUMNCREATE_A),   "UpdateTime",       JET_coltypLongLong, 0,  JET_bitColumnFixed | JET_bitColumnNotNULL,  NULL,   0,  0,  0,  0 },

            { sizeof(JET_COLUMNCREATE_A),   "BookmarkKey",      JET_coltypLongBinary,   0,  JET_bitColumnTagged,NULL,   0,  0,  0,  0 },
            { sizeof(JET_COLUMNCREATE_A),   "BookmarkData",     JET_coltypLongBinary,   0,  JET_bitColumnTagged,NULL,   0,  0,  0,  0 },
    };

    const CHAR szMSysOLD2IndexName[] = "ObjidIndex";
    const CHAR szMSysOLD2IndexKey[] = "+ObjidTable\0+ObjidFDP\0";

    JET_INDEXCREATE3_A rgjidxcOLD2[] =
    {
        {
            sizeof(JET_INDEXCREATE3_A),
            const_cast<char *>( szMSysOLD2IndexName ),
            const_cast<char *>( szMSysOLD2IndexKey ),
            _countof(szMSysOLD2IndexKey) * sizeof(szMSysOLD2IndexKey[0]),
            JET_bitIndexPrimary,
            100,
            NULL,
            NULL,
            NULL,
            0,
            JET_errSuccess,
            0,
            NULL
        },
    };

    JET_TABLECREATE5_A jtcOLD2 =
    {
        sizeof(JET_TABLECREATE5_A),
        const_cast<CHAR *>( s_szOLD2 ),
        NULL,
        0,
        100,
        rgjccOLD2,
        _countof( rgjccOLD2 ),
        rgjidxcOLD2,
        _countof( rgjidxcOLD2 ),
        NULL,
        0,
        JET_bitTableCreateFixedDDL|JET_bitTableCreateSystemTable,
        NULL,
        NULL,
        0,
        0,
        JET_tableidNil,
        0
    };

    ERR err;

    bool fInTrx = false;
    Call( ErrDIRBeginTransaction( ppib, 50981, NO_GRBIT ) );
    fInTrx = true;
    
    Call( ErrFILECreateTable( ppib, ifmp, &jtcOLD2 ) );
#ifndef RTM
    Call( ErrTestTable( ppib, (FUCB *)jtcOLD2.tableid ) );
#endif
    Call( ErrFILECloseTable( ppib, (FUCB *)jtcOLD2.tableid  ) );

    Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
    fInTrx = false;

HandleError:
    if( fInTrx )
    {
        Assert( err < JET_errSuccess );
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }
    return err;
}

ERR OLD2_STATUS::ErrDumpOneRecord_( __in PIB * const ppib, __in FUCB * const pfucb )
{
    ERR err;

    Call( ErrOLDDumpLongColumn( ppib, pfucb, s_fidOLD2ObjidTable, L"ObjidTable" ) );
    Call( ErrOLDDumpLongColumn( ppib, pfucb, s_fidOLD2ObjidFDP, L"ObjidFDP" ) );

    Call( ErrOLDDumpFileTimeColumn( ppib, pfucb, s_fidOLD2StartDateTime, L"StartDateTime" ) );
    
    Call( ErrOLDDumpLongColumn( ppib, pfucb, s_fidOLD2PagesVisited, L"PagesVisited" ) );
    Call( ErrOLDDumpLongColumn( ppib, pfucb, s_fidOLD2PagesFreed, L"PagesFreed" ) );
    Call( ErrOLDDumpLongColumn( ppib, pfucb, s_fidOLD2PartialMerges, L"PartialMerges" ) );
    Call( ErrOLDDumpLongColumn( ppib, pfucb, s_fidOLD2PageMoves, L"PageMoves" ) );

    Call( ErrOLDDumpFileTimeColumn( ppib, pfucb, s_fidOLD2UpdateTime, L"UpdateTime" ) );

    wprintf( L"\n" );
    
HandleError:
    return err;
}

ERR OLD2_STATUS::ErrLoad(
    __in PIB * const    ppib,
    __in FUCB * const   pfucbDefrag,
    __in OLD2_STATUS&   old2status )
{
    Assert( objidNil != old2status.ObjidTable() );
    Assert( objidNil != old2status.ObjidFDP() );
    
    ERR err;

    bool fInTrx = false;
    Call( ErrDIRBeginTransaction( ppib, 47909, JET_bitTransactionReadOnly ) );
    fInTrx = true;

    Call( ErrSeek_( ppib, pfucbDefrag, old2status ) );
    
    Call( ErrGetLongLong_( ppib, pfucbDefrag, s_fidOLD2StartDateTime, &(old2status.m_startTime ) ) );
    Call( ErrGetLong_( ppib, pfucbDefrag, s_fidOLD2PagesVisited, &(old2status.m_cpgVisited ) ) );
    Call( ErrGetLong_( ppib, pfucbDefrag, s_fidOLD2PagesFreed, &(old2status.m_cpgFreed) ) );
    Call( ErrGetLong_( ppib, pfucbDefrag, s_fidOLD2PartialMerges, &(old2status.m_cpgPartialMerges) ) );
    Call( ErrGetLong_( ppib, pfucbDefrag, s_fidOLD2PageMoves, &(old2status.m_cpgMoved) ) );

    BYTE * pbBuffer = old2status.m_rgbBookmark;
    size_t cbBufferRemaining = old2status.m_cbBookmark;
    ULONG cbActual;
    
    Call( ErrIsamRetrieveColumn(
        ppib,
        pfucbDefrag,
        s_fidOLD2BookmarkKey,
        pbBuffer,
        cbBufferRemaining,
        &cbActual,
        NO_GRBIT,
        NULL ) );
    Assert( cbActual <= cbBufferRemaining );
    Assert( JET_wrnBufferTruncated != err );

    if ( cbActual == 0 )
    {
        err = ErrERRCheck( JET_errRecordNotFound );
        Call( err );
    }

    pbBuffer += cbActual;
    cbBufferRemaining -= cbActual;
    old2status.m_cbBookmarkKey = cbActual;

    Call( ErrIsamRetrieveColumn(
        ppib,
        pfucbDefrag,
        s_fidOLD2BookmarkData,
        pbBuffer,
        cbBufferRemaining,
        &cbActual,
        NO_GRBIT,
        NULL ) );
    Assert( cbActual <= cbBufferRemaining );
    Assert( JET_wrnBufferTruncated != err );

    old2status.m_cbBookmarkData = cbActual;
    
HandleError:
    if( fInTrx )
    {
        CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    }
    return err;
}

const CHAR * OLD2_STATUS::SzTableName()
{
    return s_szOLD2;
}

ERR OLD2_STATUS::ErrOpenTable(
    __in PIB * const        ppib,
    const IFMP              ifmp,
    __out FUCB ** const     ppfucb )
{
    ERR err;

    Call( ErrFILEOpenTable( ppib, ifmp, ppfucb, s_szOLD2 ) );

HandleError:
    return err;
}

ERR OLD2_STATUS::ErrOpenOrCreateTable(
    __in PIB * const        ppib,
    const IFMP              ifmp,
    __out FUCB ** const     ppfucb )
{
    ERR err;

    err = ErrFILEOpenTable( ppib, ifmp, ppfucb, s_szOLD2 );
    if( JET_errObjectNotFound == err )
    {
        ENTERCRITICALSECTION enterCrit( &s_crit );

        err = ErrFILEOpenTable( ppib, ifmp, ppfucb, s_szOLD2 );
        if( JET_errObjectNotFound == err )
        {
            Call( ErrCreateTable_( ppib, ifmp ) );
            Call( ErrFILEOpenTable( ppib, ifmp, ppfucb, s_szOLD2 ) );
        }
    }
    Call( err );

HandleError:
    return err;
}

ERR OLD2_STATUS::ErrGetObjids(
    __in PIB * const        ppib,
    __in FUCB * const       pfucb,
    __out OBJID * const     pobjidTable,
    __out OBJID * const     pobjidFDP )
{
    ERR err;

    Call( ErrGetLong_( ppib, pfucb, s_fidOLD2ObjidTable, (LONG *)pobjidTable ) );
    Call( ErrGetLong_( ppib, pfucb, s_fidOLD2ObjidFDP, (LONG *)pobjidFDP ) );

HandleError:
    return err;
}
    
ERR OLD2_STATUS::ErrSave(
    __in PIB * const    ppib,
    __in FUCB * const   pfucbDefrag,
    __in const OLD2_STATUS& old2status )
{
    Assert( objidNil != old2status.ObjidTable() );
    Assert( objidNil != old2status.ObjidFDP() );

    if ( 0 == old2status.StartTime() )
    {
        Assert( 0 == old2status.CpgVisited() );
        Assert( 0 == old2status.CpgFreed() );
        Assert( 0 == old2status.CpgPartialMerges() );
        Assert( 0 == old2status.CpgMoved() );
    }
    
    ERR err;

    DATA dataT;

    const BOOKMARK bm = const_cast<OLD2_STATUS *>(&old2status)->GetBookmark();

    bool fInTrx     = false;
    bool fInUpdate  = false;

    Call( ErrDIRBeginTransaction( ppib, 64293, NO_GRBIT ) );
    fInTrx = true;

    JET_GRBIT grbitUpdate = JET_prepReplace;
    err = ErrSeek_( ppib, pfucbDefrag, old2status );
    if( JET_errRecordNotFound == err )
    {
        grbitUpdate = JET_prepInsert;
        err = JET_errSuccess;
    }
    Call( err);

    Assert( !fInUpdate );
    Call( ErrIsamPrepareUpdate( ppib, pfucbDefrag, grbitUpdate ) );
    fInUpdate = fTrue;

    if( JET_prepInsert == grbitUpdate )
    {
        Call( ErrSetLong_( ppib, pfucbDefrag, s_fidOLD2ObjidTable, old2status.ObjidTable() ) );
        Call( ErrSetLong_( ppib, pfucbDefrag, s_fidOLD2ObjidFDP, old2status.ObjidFDP() ) );
    }
    
    Call( ErrSetLongLong_( ppib, pfucbDefrag, s_fidOLD2StartDateTime, old2status.StartTime() ) );
    
    Call( ErrSetLong_( ppib, pfucbDefrag, s_fidOLD2PagesVisited, old2status.CpgVisited() ) );
    Call( ErrSetLong_( ppib, pfucbDefrag, s_fidOLD2PagesFreed, old2status.CpgFreed() ) );
    Call( ErrSetLong_( ppib, pfucbDefrag, s_fidOLD2PartialMerges, old2status.CpgPartialMerges() ) );
    Call( ErrSetLong_( ppib, pfucbDefrag, s_fidOLD2PageMoves, old2status.CpgMoved() ) );

    Call( ErrSetLongLong_( ppib, pfucbDefrag, s_fidOLD2UpdateTime, UtilGetCurrentFileTime() ) );
    
    Assert( bm.key.prefix.FNull() );
    dataT.SetPv( bm.key.suffix.Pv() );
    dataT.SetCb( bm.key.suffix.Cb() );

    Assert( FTaggedFid( s_fidOLD2BookmarkKey ) );
    Call( ErrRECSetLongField(
                pfucbDefrag,
                s_fidOLD2BookmarkKey,
                1,
                &dataT,
                CompressFlags( compress7Bit | compressXpress ) ) );

    dataT.SetPv( bm.data.Pv() );
    dataT.SetCb( bm.data.Cb() );

    Assert( FTaggedFid( s_fidOLD2BookmarkData ) );
    Call( ErrRECSetLongField(
                pfucbDefrag,
                s_fidOLD2BookmarkData,
                1,
                &dataT,
                CompressFlags( compress7Bit | compressXpress ) ) );
    
    Assert( fInUpdate );
    Call( ErrIsamUpdate( ppib, pfucbDefrag, NULL, 0, NULL, NO_GRBIT ) );
    fInUpdate = false;
    Assert( fInTrx );
    Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
    fInTrx = false;
    
HandleError:
    if( fInUpdate )
    {
        Assert( err < JET_errSuccess );
        CallS( ErrIsamPrepareUpdate( ppib, pfucbDefrag, JET_prepCancel ) );
    }
    if( fInTrx )
    {
        Assert( err < JET_errSuccess );
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }
    return err;
}

ERR OLD2_STATUS::ErrInsertNewRecord(
    _In_ PIB * const        ppib,
    _In_ FUCB * const       pfucbDefragStatusState,
    _In_ const OLD2_STATUS& old2status )
{
    bool fInTrx     = false;

    ERR err;
    Call( ErrDIRBeginTransaction( ppib, 33644, NO_GRBIT ) );
    fInTrx = true;

    err = OLD2_STATUS::ErrSeek_( ppib, pfucbDefragStatusState, old2status );

    if( JET_errRecordNotFound == err )
    {
        err = OLD2_STATUS::ErrSave( ppib, pfucbDefragStatusState, old2status );
    }

    AssertSz( err != JET_errWriteConflict, "Write Conflict not expected. This should be debugged." );

    Call( err );

    Assert( fInTrx );
    Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
    fInTrx = false;

HandleError:
    if( fInTrx )
    {
        Assert( err < JET_errSuccess );
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    return err;
}


ERR OLD2_STATUS::ErrDelete(
    __in PIB * const        ppib,
    __in FUCB * const       pfucbDefrag,
    __in const OLD2_STATUS& old2status )
{
    ERR err;

    bool fInTrx = false;
    Call( ErrDIRBeginTransaction( ppib, 39717, NO_GRBIT ) );
    fInTrx = true;
    
    Call( ErrSeek_( ppib, pfucbDefrag, old2status ) );
    Call( ErrIsamDelete( ppib, pfucbDefrag ) );

    Assert( fInTrx );
    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    fInTrx = false;

HandleError:
    if( fInTrx )
    {
        Assert( err < JET_errSuccess );
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }
    return err;
}

ERR OLD2_STATUS::ErrDumpTable( __in PIB * const ppib, const IFMP ifmp )
{
    ERR err;
    FUCB * pfucb = pfucbNil;
    
    err = ErrFILEOpenTable( ppib, ifmp, &pfucb, s_szOLD2 );
    if ( JET_errObjectNotFound == err )
    {
        err = JET_errSuccess;
        goto HandleError;
    }
    Call( err );

    wprintf( L"******************************** MSysOLD2 DUMP ***********************************\n" );
    
    
    err = ErrIsamMove( ppib, pfucb, JET_MoveFirst, NO_GRBIT );
    while( JET_errSuccess == err )
    {
        Call( ErrDumpOneRecord_( ppib, pfucb ) );
        err = ErrIsamMove( ppib, pfucb, JET_MoveNext, NO_GRBIT );
    }
    if( JET_errNoCurrentRecord == err )
    {
        err = JET_errSuccess;
    }
    Call( err );
    
HandleError:
    if( pfucbNil != pfucb )
    {
        CallS( ErrFILECloseTable( ppib, pfucb ) );
    }
    return err;

}

const CHAR OLD2_STATUS::s_szOLD2[] = "MSysOLD2";

const FID   OLD2_STATUS::s_fidOLD2ObjidTable    = fidFixedLeast;
const FID   OLD2_STATUS::s_fidOLD2ObjidFDP      = fidFixedLeast+1;
const FID   OLD2_STATUS::s_fidOLD2StartDateTime = fidFixedLeast+2;
const FID   OLD2_STATUS::s_fidOLD2PagesVisited  = fidFixedLeast+3;
const FID   OLD2_STATUS::s_fidOLD2PagesFreed    = fidFixedLeast+4;
const FID   OLD2_STATUS::s_fidOLD2PartialMerges = fidFixedLeast+5;
const FID   OLD2_STATUS::s_fidOLD2PageMoves     = fidFixedLeast+6;
const FID   OLD2_STATUS::s_fidOLD2UpdateTime    = fidFixedLeast+7;

const FID   OLD2_STATUS::s_fidOLD2BookmarkKey   = fidTaggedLeast;
const FID   OLD2_STATUS::s_fidOLD2BookmarkData  = fidTaggedLeast+1;

#ifndef RTM
VOID OLD2_STATUS::Test()
{
    static bool fTested = false;

    if(!fTested)
    {
        fTested = true;
        
        BYTE rgbKeyPrefix[] = { 'A', 'B', 'C' };
        BYTE rgbKeySuffix[] = { '1', '2', '3', '4' };
        BYTE rgbData[]      = { '*', '%', '&' };

        BOOKMARK bm;
        bm.key.prefix.SetPv( rgbKeyPrefix );
        bm.key.prefix.SetCb( sizeof( rgbKeyPrefix ) );
        bm.key.suffix.SetPv( rgbKeySuffix );
        bm.key.suffix.SetCb( sizeof( rgbKeySuffix ) );
        bm.data.SetPv( rgbData );
        bm.data.SetCb( sizeof( rgbData ) );
        
        OLD2_STATUS old2statusT( 10, 11 );
        AssertRTL( 10 == old2statusT.ObjidTable() );
        AssertRTL( 11 == old2statusT.ObjidFDP() );
        
        AssertRTL( 0 == old2statusT.CpgVisited() );
        AssertRTL( 0 == old2statusT.CpgFreed() );
        AssertRTL( 0 == old2statusT.CpgPartialMerges() );
        AssertRTL( 0 == old2statusT.CpgMoved() );
        AssertRTL( 0 == old2statusT.StartTime() );

        BOOKMARK bmT;
        bmT = old2statusT.GetBookmark();
        AssertRTL( bmT.key.FNull() );
        AssertRTL( bmT.data.FNull() );
        
        old2statusT.SetStartTime( 0x1234 );
        AssertRTL( 0x1234 == old2statusT.StartTime() );
        
        old2statusT.IncrementCpgVisited();
        old2statusT.IncrementCpgVisited();
        old2statusT.IncrementCpgPartialMerges();
        old2statusT.IncrementCpgVisited();
        old2statusT.IncrementCpgPartialMerges();
        old2statusT.IncrementCpgFreed();
        old2statusT.IncrementCpgVisited();
        old2statusT.IncrementCpgPartialMerges();
        old2statusT.IncrementCpgFreed();
        old2statusT.IncrementCpgMoved();
        AssertRTL( 4 == old2statusT.CpgVisited() );
        AssertRTL( 3 == old2statusT.CpgPartialMerges() );
        AssertRTL( 2 == old2statusT.CpgFreed() );
        AssertRTL( 1 == old2statusT.CpgMoved() );

        old2statusT.SetBookmark( bm );
        bmT = old2statusT.GetBookmark();

        AssertRTL( 0 == CmpBM( bm, bmT ) );
    }
}

ERR OLD2_STATUS::ErrLoadAndCheck( __in PIB * const ppib, __in FUCB * const pfucb, const OLD2_STATUS& old2status )
{
    ERR err;

    OLD2_STATUS old2statusT( old2status.ObjidTable(), old2status.ObjidFDP() );
    Call( ErrLoad( ppib, pfucb, old2statusT ) );
    AssertRTL( old2statusT.StartTime() == old2status.StartTime() );
    AssertRTL( old2statusT.CpgVisited() == old2status.CpgVisited() );
    AssertRTL( old2statusT.CpgFreed() == old2status.CpgFreed() );
    AssertRTL( old2statusT.CpgMoved() == old2status.CpgMoved() );
    AssertRTL( old2statusT.CpgPartialMerges() == old2status.CpgPartialMerges() );

{
    const BOOKMARK bm = const_cast<OLD2_STATUS *>(&old2status)->GetBookmark();
    const BOOKMARK bmT = old2statusT.GetBookmark();
    AssertRTL( 0 == CmpBM( bm, bmT ) );
}

HandleError:
    return err;
}

ERR OLD2_STATUS::ErrTestTable( __in PIB * const ppib, __in FUCB * const pfucb )
{
    static bool fTested = false;

    if(fTested)
    {
        return JET_errSuccess;
    }
    fTested = true;

    OLD2_STATUS old2status1( 10, 16 );
    OLD2_STATUS old2status2( 145, 152 );
    OLD2_STATUS old2status3( 207, 207 );
    BOOKMARK bm1;
    BOOKMARK bm2;
    BOOKMARK bm3;

    ERR err;

    bool fInTrx = false;
    Call( ErrDIRBeginTransaction( ppib, 56101, NO_GRBIT ) );
    fInTrx = true;

    err = ErrSeek_( ppib, pfucb, old2status1 );
    AssertRTL( JET_errRecordNotFound == err );

    old2status1.SetStartTime( 6578 );
    old2status2.SetStartTime( 6579 );
    old2status3.SetStartTime( 6580 );

    BYTE rgbKeySuffix1[] = { 0x02, 0x04, 0x01, 0x03, 0x05 };
    BYTE rgbData1[]     = { 0x00, 0xFF };

    BYTE rgbKeySuffix2[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
    BYTE rgbData2[]     = { 0xFF, 0x00 };

    BYTE rgbKeySuffix3[] = { 0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 };
    BYTE rgbData3[]     = { 0xFF, 0x00 };

    bm1.key.prefix.Nullify();
    bm1.key.suffix.SetPv( rgbKeySuffix1 );
    bm1.key.suffix.SetCb( sizeof( rgbKeySuffix1 ) );
    bm1.data.SetPv( rgbData1 );
    bm1.data.SetCb( sizeof( rgbData1 ) );

    bm2.key.prefix.Nullify();
    bm2.key.suffix.SetPv( rgbKeySuffix2 );
    bm2.key.suffix.SetCb( sizeof( rgbKeySuffix2 ) );
    bm2.data.SetPv( rgbData2 );
    bm2.data.SetCb( sizeof( rgbData2 ) );

    bm3.key.prefix.Nullify();
    bm3.key.suffix.SetPv( rgbKeySuffix3 );
    bm3.key.suffix.SetCb( sizeof( rgbKeySuffix3 ) );
    bm3.data.SetPv( rgbData3 );
    bm3.data.SetCb( sizeof( rgbData3 ) );

    old2status1.SetBookmark( bm1 );

    old2status2.IncrementCpgVisited();
    old2status2.IncrementCpgVisited();
    old2status2.IncrementCpgVisited();
    old2status2.IncrementCpgVisited();
    old2status2.IncrementCpgFreed();
    old2status2.IncrementCpgFreed();
    old2status2.IncrementCpgFreed();
    old2status2.IncrementCpgMoved();
    old2status2.IncrementCpgMoved();
    old2status2.IncrementCpgPartialMerges();
    old2status2.SetBookmark( bm2 );
    
    old2status3.SetBookmark( bm3 );

    AssertRTL( 6579 == old2status2.StartTime() );
    AssertRTL( 4 == old2status2.CpgVisited() );
    AssertRTL( 3 == old2status2.CpgFreed() );
    AssertRTL( 2 == old2status2.CpgMoved() );
    AssertRTL( 1 == old2status2.CpgPartialMerges() );

    Call( ErrSave( ppib, pfucb, old2status1 ) );
    Call( ErrSave( ppib, pfucb, old2status2 ) );
    Call( ErrSave( ppib, pfucb, old2status3 ) );

    old2status1.IncrementCpgVisited();
    Call( ErrSave( ppib, pfucb, old2status1 ) );
    
    Call( ErrLoadAndCheck( ppib, pfucb, old2status1 ) );
    Call( ErrLoadAndCheck( ppib, pfucb, old2status2 ) );
    Call( ErrLoadAndCheck( ppib, pfucb, old2status3 ) );
    
    Call( ErrDelete( ppib, pfucb, old2status2 ) );

    Call( ErrLoadAndCheck( ppib, pfucb, old2status1 ) );
    err = ErrLoadAndCheck( ppib, pfucb, old2status2 );
    AssertRTL( JET_errRecordNotFound == err );
    Call( ErrLoadAndCheck( ppib, pfucb, old2status3 ) );

    Call( ErrDelete( ppib, pfucb, old2status1 ) );
    Call( ErrDelete( ppib, pfucb, old2status3 ) );
    
HandleError:
    if( fInTrx )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }
    return err;
}
#endif

void FreeBookmarkMemory(BOOKMARK& bm)
{
    delete [] bm.key.prefix.Pv();
    delete [] bm.key.suffix.Pv();
    delete [] bm.data.Pv();
    bm.Nullify();
}

CTableDefragment::CTableDefragment(
    _In_ const IFMP ifmp,
    _In_z_ const char * const szTable,
    _In_opt_z_ const char * const szIndex,
    _In_ DEFRAGTYPE defragtype ) :
    m_fInit( false ),
    m_ifmp( ifmp ),
    m_ppib( ppibNil),
    m_pfucbOwningTable( pfucbNil ),
    m_pfucbToDefrag( pfucbNil ),
    m_pfucbDefragStatus( pfucbNil ),
    m_preccheck( NULL ),
    m_fCompleted( false ),
    m_cpgVisitedLastUpdate( 0 ),
    m_pvBookmarkBuf( NULL ),
    m_pold2Status( NULL ),
    m_fDefragRangeSelected( false ),
    m_defragtype( defragtype )
{
    OSStrCbCopyA( m_szTable, sizeof( m_szTable ), szTable );
    if ( defragtype == defragtypeIndex )
    {
        AssertSz( fFalse, "Defrag of secondary index is no longer supported" );
        Assert( NULL != szIndex );
        OSStrCbCopyA( m_szIndex, sizeof( m_szIndex ), szIndex );
    }
    else
    {
        Expected( defragtype == defragtypeTable );
        m_szIndex[ 0 ] = '\0';
    }

    m_bmDefragRangeStart.Nullify();
    m_bmDefragRangeEnd.Nullify();
}

CTableDefragment::~CTableDefragment()
{
    ClearDefragRange_();
    Assert( !FIsInit_() );
    Assert( ppibNil == m_ppib );
    Assert( pfucbNil == m_pfucbToDefrag );
    Assert( pfucbNil == m_pfucbOwningTable );
    Assert( pfucbNil == m_pfucbDefragStatus );
    Assert( NULL == m_preccheck );
    Assert( NULL == m_pvBookmarkBuf );
    Assert( NULL == m_pold2Status );
}

bool CTableDefragment::FNoMoreDefrag() const
{
    return FIsCompleted_() || ( FIsInit_() && FTableWasDeleted_() );
}

ERR CTableDefragment::ErrTerm()
{
    ERR err = JET_errSuccess;
    JET_GRBIT action = JET_bitOld2Suspend;
    
    if( FIsInit_() && !FNoMoreDefrag() )
    {
        err = OLD2_STATUS::ErrSave( m_ppib, m_pfucbDefragStatus, *m_pold2Status );
    }
    if ( FNoMoreDefrag() )
    {
        action = JET_bitOld2End;
    }

    if( m_pvBookmarkBuf )
    {
        RESBOOKMARK.Free( m_pvBookmarkBuf );
        m_pvBookmarkBuf = NULL;
    }

    if( m_pfucbToDefrag )
    {
        if ( m_defragtype == defragtypeTable )
        {
            Assert( m_pfucbToDefrag->u.pfcb->FOLD2Running() || !FIsInit_() );
            m_pfucbToDefrag->u.pfcb->Lock();
            m_pfucbToDefrag->u.pfcb->ResetOLD2Running();
            m_pfucbToDefrag->u.pfcb->Unlock();

            (void)ErrRECCallback( m_ppib, m_pfucbToDefrag, JET_cbtypOld2Action, 0, m_szTable, (void*)&action, 0 );

            Assert( m_pfucbOwningTable == m_pfucbToDefrag );
            m_pfucbToDefrag = pfucbNil;
        }
        else if ( m_defragtype == defragtypeIndex )
        {
            AssertSz( fFalse, "Defrag of secondary index is no longer supported" );
            Assert( m_pfucbToDefrag->u.pfcb->FOLD2Running() || !FIsInit_() );
            m_pfucbToDefrag->u.pfcb->Lock();
            m_pfucbToDefrag->u.pfcb->ResetOLD2Running();
            m_pfucbToDefrag->u.pfcb->Unlock();


        }
        else
        {
            AssertSz( fFalse, "m_defragtype (%d) is not recognized!", m_defragtype );
        }
    }

    if ( m_pfucbOwningTable )
    {
        CallS( ErrFILECloseTable( m_ppib, m_pfucbOwningTable ) );
        m_pfucbOwningTable = pfucbNil;
    }

    if( m_pfucbDefragStatus )
    {
        CallS( ErrFILECloseTable( m_ppib, m_pfucbDefragStatus ) );
    }

    if( m_ppib )
    {
        CallS( ErrDBCloseAllDBs( m_ppib ) );
        PIBEndSession( m_ppib );
    }

    delete m_preccheck;
    delete m_pold2Status;

    m_ppib          = ppibNil;
    m_pfucbToDefrag = pfucbNil;
    m_pfucbOwningTable  = pfucbNil;
    m_pfucbDefragStatus = pfucbNil;
    m_preccheck     = NULL;
    m_pold2Status   = NULL;

    m_fInit = false;
    
    Assert( !FIsInit_() );
    return err;
}

ERR CTableDefragment::ErrDefragStep()
{
    ERR err;
    AssertSz( !g_fRepair, "OLD should never be run during Repair." );
    AssertSz( !g_rgfmp[ m_ifmp ].FReadOnlyAttach(), "OLD should not be run on a read-only database." );

    OSTrace( JET_tracetagOLDRegistration, OSFormat( __FUNCTION__ ": Entering for table %s:%s...", m_szTable, m_szIndex ) );

    if ( m_defragtype == defragtypeTable )
    {
        const BOOL fDefragMainTrees = !!UlConfigOverrideInjection( 45068, fTrue );
        if ( !fDefragMainTrees )
        {
            OSTrace( JET_tracetagOLDRegistration, OSFormat( __FUNCTION__ ": Test Injection says that we should skip defragging %s", m_szTable ) );
            SetCompleted_();
            CallS( ErrTerm() );
            return JET_errSuccess;
        }
    }
    else if ( m_defragtype == defragtypeIndex )
    {
        OSTrace( JET_tracetagOLDRegistration, OSFormat( __FUNCTION__ ": We should skip defragging the indices of %s as it is not supported", m_szTable ) );
        SetCompleted_();
        CallS( ErrTerm() );
        return JET_errSuccess;
    }

    if( !FIsInit_() )
    {
        err = ErrInit_();
        Assert( FIsInit_() || err < JET_errSuccess );
        OSTrace( JET_tracetagOLDWork, OSFormat( __FUNCTION__ ": ErrInit( %s:%s ) returned %d.", m_szTable, m_szIndex, err ) );
    }
    else
    {
        err = ErrDefragStep_();
        OSTrace( JET_tracetagOLDWork, OSFormat( __FUNCTION__ ": ErrDefragStep_( %s:%s ). Returned %d.", m_szTable, m_szIndex, err ) );
    }

    if( err < JET_errSuccess )
    {
        SetCompleted_();
        CallS( ErrTerm() );
    }
    return err;
}

bool CTableDefragment::FIsInit_() const
{
    return m_fInit;
}

ERR CTableDefragment::ErrInit_()
{
    Assert( !FIsInit_() );
    Assert( ifmpNil != m_ifmp );
    Assert( ppibNil == m_ppib );
    Assert( pfucbNil == m_pfucbToDefrag );
    Assert( pfucbNil == m_pfucbOwningTable );
    Assert( pfucbNil == m_pfucbDefragStatus );
    Assert( NULL == m_pold2Status );
    
    ERR err = JET_errSuccess;
    JET_GRBIT action = JET_bitOld2Resume;

    OSTrace( JET_tracetagOLDRegistration, OSFormat( __FUNCTION__ ": Entering for table %s:%s...", m_szTable, m_szIndex ) );

    INST * const pinst = PinstFromIfmp( m_ifmp );
    
    Call( ErrPIBBeginSession( pinst, &m_ppib, procidNil, fFalse ) );
    m_ppib->SetFUserSession();
    m_ppib->SetFSessionOLD2();

    Call( ErrDBOpenDatabaseByIfmp( m_ppib, m_ifmp ) );

    Call( OLD2_STATUS::ErrOpenOrCreateTable( m_ppib, m_ifmp, &m_pfucbDefragStatus ) );
    
    if ( m_defragtype == defragtypeTable )
    {
        Call( ErrFILEOpenTable( m_ppib, m_ifmp, &m_pfucbOwningTable, m_szTable ) );
        Call( ErrOLDCheckForFinalize( m_ppib, m_pfucbOwningTable, pinst, &m_preccheck ) );
        Alloc( m_pold2Status = new OLD2_STATUS( ObjidFDP( m_pfucbOwningTable), ObjidFDP( m_pfucbOwningTable ) ) );

        m_pfucbToDefrag = m_pfucbOwningTable;
    }
    else if ( m_defragtype == defragtypeIndex )
    {
        AssertSz( fFalse, "Defrag of secondary index is no longer supported" );

        Call( ErrFILEOpenTable( m_ppib, m_ifmp, &m_pfucbOwningTable, m_szTable ) );
        Call( ErrOLDCheckForFinalize( m_ppib, m_pfucbOwningTable, pinst, &m_preccheck ) );
        m_pfucbOwningTable->pfucbCurIndex = pfucbNil;
        err = ErrIsamSetCurrentIndex( (JET_SESID) m_ppib, (JET_TABLEID) m_pfucbOwningTable, m_szIndex );

        if ( ( pfucbNil != m_pfucbOwningTable->pfucbCurIndex ) && !FFUCBUnique( m_pfucbOwningTable->pfucbCurIndex ) )
        {
            Assert( pfucbNil == m_pfucbToDefrag );
            Error( ErrERRCheck( JET_errFeatureNotAvailable ) );
        }

        if ( pfucbNil == m_pfucbOwningTable->pfucbCurIndex || FFUCBPrimary( m_pfucbOwningTable->pfucbCurIndex ) )
        {
            Assert( pfucbNil == m_pfucbToDefrag );
            Error( ErrERRCheck( JET_errFeatureNotAvailable ) );
        }

        if ( err < 0 || pfucbNil == m_pfucbOwningTable->pfucbCurIndex )
        {
            AssertSz( fFalse, "I thought that Primary Indices would be Not Found?" );
            Error(  ErrERRCheck( JET_errObjectNotFound ) );
        }

        Call( err );

        m_pfucbToDefrag = m_pfucbOwningTable->pfucbCurIndex;

        Alloc( m_pold2Status = new OLD2_STATUS( ObjidFDP( m_pfucbOwningTable ), ObjidFDP( m_pfucbToDefrag) ) );
    }
    else
    {
        AssertSz( fFalse, "m_defragtype (%d) is not recognized!", m_defragtype );
    }

    Assert( pfucbNil != m_pfucbToDefrag );

    Assert( !( m_pfucbToDefrag->u.pfcb->FOLD2Running() ) );
    m_pfucbToDefrag->u.pfcb->Lock();
    m_pfucbToDefrag->u.pfcb->SetOLD2Running();
    m_pfucbToDefrag->u.pfcb->Unlock();

    err = OLD2_STATUS::ErrLoad( m_ppib, m_pfucbDefragStatus, *m_pold2Status );
    if( JET_errRecordNotFound == err )
    {
        Call( ErrGetInitialBookmark_() );
        action = JET_bitOld2Start;
    }
    else
    {
        Assert( JET_bitOld2Resume == action );
        Call( err );
        Call( ErrAdjustBookmark_() );
    }

    Alloc( m_pvBookmarkBuf = RESBOOKMARK.PvRESAlloc() );

    Assert( !FIsInit_() );
    m_fInit = true;

    if ( m_defragtype == defragtypeTable )
    {
        (void)ErrRECCallback( m_ppib, m_pfucbToDefrag, JET_cbtypOld2Action, 0, m_szTable, (void*)&action, 0 );
    }
    Assert( ifmpNil != m_ifmp );
    Assert( ppibNil != m_ppib );
    Assert( pfucbNil != m_pfucbOwningTable );
    Assert( pfucbNil != m_pfucbToDefrag );

    Assert( m_pfucbOwningTable == m_pfucbToDefrag );
    Assert( pfucbNil != m_pfucbDefragStatus );
    Assert( NULL != m_pold2Status );
    Assert( FIsInit_() );

HandleError:
    OSTrace( JET_tracetagOLDWork, OSFormat( __FUNCTION__ ": Leaving table %s:%s. Returning %d.", m_szTable, m_szIndex, err ) );

    if ( ( m_defragtype == defragtypeTable ) && m_fInit )
    {
        Assert( err >= JET_errSuccess );
        Assert( m_pfucbToDefrag->u.pfcb->FOLD2Running() );
    }

    if( err < JET_errSuccess )
    {
        CallS( ErrTerm() );
    }
    return err;
}

ERR CTableDefragment::ErrGetInitialBookmark_()
{
    ERR err;
    
    DIB dib;
    dib.pos     = posFirst;
    dib.dirflag = fDIRAllNode;
    
    PIBTraceContextScope tcScope = m_ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortOLD2 );

    err = ErrBTDown( m_pfucbToDefrag, &dib, latchReadTouch );
    if ( JET_errRecordNotFound == err )
    {
        Call( ErrSetBTreeDefragCompleted_() );
    }
    else
    {
        BOOKMARK bmStart;
        
        Call( err );
        Assert( Pcsr( m_pfucbToDefrag )->FLatched() );
        
        NDGetBookmarkFromKDF( m_pfucbToDefrag, m_pfucbToDefrag->kdfCurr, &bmStart );
        m_pold2Status->SetBookmark(bmStart);
        m_pold2Status->SetStartTime( UtilGetCurrentFileTime() );

        BTUp( m_pfucbToDefrag );
        Call( OLD2_STATUS::ErrSave( m_ppib, m_pfucbDefragStatus, *m_pold2Status ) );
    }

HandleError:
    return err;
}

ERR CTableDefragment::ErrAdjustBookmark_()
{
    ERR err;

    BOOKMARK bmStartNew;
    const BOOKMARK bmStart = m_pold2Status->GetBookmark();
    Assert( !bmStart.key.FNull() );
    
    DIB dib;
    dib.pos = posDown;
    dib.pbm = const_cast<BOOKMARK *>( &bmStart );
    dib.dirflag = fDIRAllNode;
    
    PIBTraceContextScope tcScope = m_ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortOLD2 );

    err = ErrBTDown( m_pfucbToDefrag, &dib, latchReadTouch );
    if ( JET_errRecordNotFound == err )
    {
        Call( ErrSetBTreeDefragCompleted_() );
        goto HandleError;
    }
    Call( err );
    
    Assert( Pcsr( m_pfucbToDefrag )->FLatched() );
    while( Pcsr( m_pfucbToDefrag )->Pgno() == Pcsr( m_pfucbToDefrag )->Cpage().PgnoPrev() + 1 )
    {
        err = ErrBTNext( m_pfucbToDefrag, fDIRAllNode );
        if ( err < JET_errSuccess )
        {
            BTUp( m_pfucbToDefrag );
            if ( JET_errNoCurrentRecord == err )
            {
                err = ErrSetBTreeDefragCompleted_();
            }
            goto HandleError;
        }
    }
    
    NDGetBookmarkFromKDF( m_pfucbToDefrag, m_pfucbToDefrag->kdfCurr, &bmStartNew );
    m_pold2Status->SetBookmark(bmStartNew);

    BTUp( m_pfucbToDefrag );
    Call( OLD2_STATUS::ErrSave( m_ppib, m_pfucbDefragStatus, *m_pold2Status ) );

HandleError:
    Assert( !Pcsr( m_pfucbToDefrag )->FLatched() );
    return err;
}

ERR CTableDefragment::ErrSelectDefragRange_()
{
    Assert(!m_fDefragRangeSelected);
    ERR err = JET_errSuccess;

    OSTrace( JET_tracetagOLDRegistration, OSFormat( __FUNCTION__ ": %s:%s selecting defrag range.", m_szTable, m_szIndex ) );



    PIBTraceContextScope tcScope = m_ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortOLD2 );

    err = ErrBTFindFragmentedRange(
            m_pfucbToDefrag,
            m_pold2Status->GetBookmark(),
            &m_bmDefragRangeStart,
            &m_bmDefragRangeEnd );
    OSTrace( JET_tracetagOLDRegistration, OSFormat( __FUNCTION__ ": ErrBTFindFragmentedRange( %s:%s ) returned %d.",
                                        m_szTable, m_szIndex, err ) );
    if ( JET_errRecordNotFound == err )
    {
        Call( ErrSetBTreeDefragCompleted_() );
        goto HandleError;
    }
    Call( err );

    m_pold2Status->SetBookmark(m_bmDefragRangeStart);
    
    m_fDefragRangeSelected = true;
    Assert(m_fDefragRangeSelected);

HandleError:
    Assert( !Pcsr( m_pfucbToDefrag )->FLatched() );
    return err;
}


bool CTableDefragment::FTableWasDeleted_() const
{
    return ( m_pfucbToDefrag && m_pfucbToDefrag->u.pfcb->FDeletePending() );
}

ERR CTableDefragment::ErrDefragStep_()
{
    Assert( FIsInit_() );
    
    INST * const pinst = PinstFromPpib( m_ppib );
    UPDATETHREADSTATSCOUNTERS updatetscounters( pinst, &g_tscountersOLD );
    
    ERR err;

    bool fInTrx = false;
    Call( ErrDIRBeginTransaction( m_ppib, 43813, NO_GRBIT ) );
    fInTrx = true;

    Assert( m_cpgVisitedLastUpdate <= m_pold2Status->CpgVisited() );
    if ( FShouldUpdateStatus_() )
    {
        OSTrace( JET_tracetagOLDWork, OSFormat( __FUNCTION__ ": Calling ErrUpdateStatus_( %s:%s ).", m_szTable, m_szIndex ) );
        Call( ErrUpdateStatus_() );
    }
    else if ( !m_fDefragRangeSelected )
    {
        OSTrace( JET_tracetagOLDWork, OSFormat( __FUNCTION__ ": Calling ErrSelectDefragRange_( %s:%s ).", m_szTable, m_szIndex ) );
        Call( ErrSelectDefragRange_() );
    }
    else
    {
        OSTrace( JET_tracetagOLDWork, OSFormat( __FUNCTION__ ": Calling ErrPerformMerges_( %s:%s ).", m_szTable, m_szIndex ) );
        Call( ErrPerformMerges_() );
    }
    
    Assert( fInTrx );
    Call( ErrDIRCommitTransaction( m_ppib, JET_bitCommitLazyFlush ) );
    fInTrx = false;

HandleError:
    if ( fInTrx )
    {
        Assert( err < 0 );
        CallSx( ErrDIRRollback( m_ppib ), JET_errRollbackError );
    }
    OSTrace( JET_tracetagOLDWork, OSFormat( __FUNCTION__ "( %s:%s ): Returning %d.", m_szTable, m_szIndex, err ) );
    return err;
}

bool CTableDefragment::FShouldUpdateStatus_() const
{
    Assert( FIsInit_() );
    return (m_pold2Status->CpgVisited() > m_cpgVisitedLastUpdate + m_cpgUpdateThreshold);
}

ERR CTableDefragment::ErrUpdateStatus_()
{
    Assert( FIsInit_() );
    ERR err;
    OSTrace( JET_tracetagOLDWork, OSFormat( __FUNCTION__ ": Updating %s:%s.", m_szTable, m_szIndex ) );
    CallR( OLD2_STATUS::ErrSave( m_ppib, m_pfucbDefragStatus, *m_pold2Status ) );
    m_cpgVisitedLastUpdate = m_pold2Status->CpgVisited();
    return err;
}

ERR CTableDefragment::ErrPerformMerges_()
{
    ERR err;

    OSTrace( JET_tracetagOLDWork, OSFormat( __FUNCTION__ ": %s:%s", m_szTable, m_szIndex ) );
    
    PrereadInfo info( m_cpgToPreread );
    Call( ErrPerformOneMerge_( &info ) );

    
    for ( PGNO pgno = info.pgnoPrereadStart; pgno < info.pgnoPrereadStart + info.cpgActuallyPreread; pgno++ )
    {
        if ( FNoMoreDefrag() )
        {
            break;
        }
        err = ErrPerformOneMerge_( NULL );
        if ( err < 0 )
        {
            break;
        }
    }

    Assert( info.cpgToPreread >= info.cpgActuallyPreread );
    
    for ( PGNO pgno = info.pgnoPrereadStart; pgno < info.pgnoPrereadStart + info.cpgActuallyPreread; pgno++ )
    {
        if ( info.rgfPageWasAlreadyCached[ pgno - info.pgnoPrereadStart ] == fFalse )
        {
            BFMarkAsSuperCold( m_ifmp, pgno );
        }
    }

    Call( err );
    
HandleError:
    return err;
}
    
ERR CTableDefragment::ErrPerformOneMerge_( PrereadInfo * const pPrereadInfo )
{
    if ( pPrereadInfo )
    {
        pPrereadInfo->cpgActuallyPreread = 0;
    }


    PERFOptDeclare( INST * const pinst = PinstFromPpib( m_ppib ) );

    ERR err;

    BOOKMARK bmCurr = m_pold2Status->GetBookmark();
    BOOKMARK bmNext;
    bmNext.Nullify();
    bmNext.key.suffix.SetPv( m_pvBookmarkBuf );

    PIBTraceContextScope tcScope = m_ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortOLD2 );

    MERGETYPE mergetype;
    Call( ErrBTIMultipageCleanup( m_pfucbToDefrag, bmCurr, &bmNext, m_preccheck, &mergetype, fFalse, pPrereadInfo ) );
    BTUp( m_pfucbToDefrag );

    m_pold2Status->IncrementCpgVisited();
    switch ( mergetype )
    {
        case mergetypeNone:
            break;
        case mergetypeEmptyPage:
        case mergetypeEmptyTree:
        case mergetypeFullLeft:
            m_pold2Status->IncrementCpgFreed();
            PERFOpt( cOLDPagesFreed.Inc( pinst ) );
            break;
        case mergetypePartialLeft:
            m_pold2Status->IncrementCpgPartialMerges();
            PERFOpt( cOLDPartialMerges.Inc( pinst ) );
            break;
        default:
            AssertSz( fFalse, "Unexpected case" );
            Call( ErrERRCheck( JET_errInternalError ) );
            break;
    }

    if ( mergetypeNone == mergetype || mergetypePartialLeft == mergetype )
    {
        err = ErrBTPageMove( m_pfucbToDefrag, bmCurr, pgnoNull, fTrue, fSPContinuous, &bmNext );
        Call( err );

        if ( err != wrnBTShallowTree )
        {
            m_pold2Status->IncrementCpgMoved();
            PERFOpt( cOLDPagesMoved.Inc( pinst ) );
        }
    }

    m_pold2Status->SetBookmark( bmNext );
    if ( bmNext.key.FNull() )
    {
        Call( ErrSetBTreeDefragCompleted_() );
    }
    else if ( CmpBM( bmNext, m_bmDefragRangeEnd ) >= 0 )
    {
        ClearDefragRange_();
    }
    
HandleError:
    BTUp( m_pfucbToDefrag );
    return err;
}

void CTableDefragment::ClearDefragRange_()
{
    FreeBookmarkMemory( m_bmDefragRangeStart );
    FreeBookmarkMemory( m_bmDefragRangeEnd );
    m_fDefragRangeSelected = false;
}

VOID CTableDefragment::LogCompletionEvent_() const
{
    ERR                         err;

    const INST * const          pinst       = PinstFromPpib( m_ppib );
    const IFMP                  ifmp        = m_pfucbDefragStatus->ifmp;
    const INT                   cchInt64Max = 22;
    
    const INT                   cszMax = 8;
    const WCHAR *               rgszT[cszMax];
    INT                         isz = 0;

    CAutoWSZDDL     szBtreeName;
    CAutoWSZDDL     szTableName;

    CallS( szBtreeName.ErrSet( "<null>" ) );
    CallS( szTableName.ErrSet( "<null>" ) );

    FCB * const pfcb = m_pfucbToDefrag->u.pfcb;
    if ( pfcb->FTypeTable() )
    {
        pfcb->EnterDML();
        CallS( szBtreeName.ErrSet( pfcb->Ptdb()->SzTableName() ) );
        CallS( szTableName.ErrSet( pfcb->Ptdb()->SzTableName() ) );
        pfcb->LeaveDML();
    }
    else if ( pfcb->FTypeSecondaryIndex() )
    {
        if ( pfcbNil != pfcb->PfcbTable() && pidbNil != pfcb->Pidb() )
        {
            pfcb->PfcbTable()->EnterDML();
            CallS( szBtreeName.ErrSet( pfcb->PfcbTable()->Ptdb()->SzIndexName(
                                        pfcb->Pidb()->ItagIndexName(),
                                        pfcb->FDerivedIndex() ) ) );
            CallS( szTableName.ErrSet( pfcb->PfcbTable()->Ptdb()->SzTableName() ) );
            pfcb->PfcbTable()->LeaveDML();
        }
    }
    else if ( pfcb->FTypeLV() )
    {
        CallS( szBtreeName.ErrSet( "<LV>" ) );
        if ( pfcbNil != pfcb->PfcbTable() )
        {
            pfcb->PfcbTable()->EnterDML();
            CallS( szTableName.ErrSet( pfcb->PfcbTable()->Ptdb()->SzTableName() ) );
            pfcb->PfcbTable()->LeaveDML();
        }
    }

    WCHAR * szPassStartDateTime = 0;
    size_t cchPassStartDateTime;
    
    Call( ErrUtilFormatFileTimeAsDate(
        m_pold2Status->StartTime(),
        0,
        0,
        &cchPassStartDateTime) );
    Alloc( szPassStartDateTime = new WCHAR[cchPassStartDateTime] );
    Call( ErrUtilFormatFileTimeAsDate(
        m_pold2Status->StartTime(),
        szPassStartDateTime,
        cchPassStartDateTime,
        &cchPassStartDateTime) );

    WCHAR szPagesVisited[cchInt64Max];
    OSStrCbFormatW( szPagesVisited, sizeof(szPagesVisited), L"%d", m_pold2Status->CpgVisited() );

    WCHAR szPagesFreed[cchInt64Max];
    OSStrCbFormatW( szPagesFreed, sizeof(szPagesFreed), L"%d", m_pold2Status->CpgFreed() );

    WCHAR szPagesPartialMerges[cchInt64Max];
    OSStrCbFormatW( szPagesPartialMerges, sizeof(szPagesPartialMerges), L"%d", m_pold2Status->CpgPartialMerges() );

    WCHAR szPagesMoved[cchInt64Max];
    OSStrCbFormatW( szPagesMoved, sizeof(szPagesMoved), L"%d", m_pold2Status->CpgMoved() );

    rgszT[isz++] = g_rgfmp[ifmp].WszDatabaseName();
    rgszT[isz++] = (WCHAR *)szTableName;
    rgszT[isz++] = (WCHAR *)szBtreeName;
    rgszT[isz++] = szPassStartDateTime;
    rgszT[isz++] = szPagesVisited;
    rgszT[isz++] = szPagesFreed;
    rgszT[isz++] = szPagesPartialMerges;
    rgszT[isz++] = szPagesMoved;
    Assert( isz == cszMax );
    
    UtilReportEvent(
        eventInformation,
        ONLINE_DEFRAG_CATEGORY,
        OLD2_COMPLETE_PASS_ID,
        isz,
        rgszT,
        0,
        NULL,
        pinst,
        JET_EventLoggingLevelHigh );

HandleError:
    delete[] szPassStartDateTime;
}

ERR CTableDefragment::ErrSetBTreeDefragCompleted_()
{
    Assert( !FIsCompleted_() );
    
    ERR err;

    LogCompletionEvent_();
    
    Call( OLD2_STATUS::ErrDelete( m_ppib, m_pfucbDefragStatus, *m_pold2Status ) );

    SetCompleted_();
    
    Assert( FIsCompleted_() );
    
HandleError:
    return err;
}

bool CTableDefragment::FIsCompleted_() const
{
    return m_fCompleted;
}

VOID CTableDefragment::SetCompleted_()
{
    Assert( !FIsCompleted_() );
    m_fCompleted = true;
    Assert( FIsCompleted_() );
}

CDefragTask::CDefragTask() :
    m_tickEnd( 0 ),
    m_plCompleted( 0 ),
    m_ptabledefragment( NULL ),
    m_fIssued( false )
{
}

CDefragTask::~CDefragTask()
{
}

VOID CDefragTask::SetTickEnd( ULONG tickEnd )
{
    m_tickEnd = tickEnd;
}

VOID CDefragTask::SetPlCompleted( LONG * const plCompleted )
{
    m_plCompleted = plCompleted;
}

DWORD CDefragTask::DispatchTask( void *pvThis )
{
    CDefragTask * const pdefragtask = (CDefragTask *)pvThis;
    return pdefragtask->Task_();
}

DWORD CDefragTask::Task_()
{
    (VOID)m_ptabledefragment->ErrDefragStep();
    const ULONG tickEnded = TickOSTimeCurrent();
    if( tickEnded <= m_tickEnd )
    {
        AtomicIncrement( m_plCompleted );
    }

    PERFOptDeclare( INST * const pinst = PinstFromIfmp( m_ptabledefragment->Ifmp() ) );
    PERFOpt( cOLDTasksRunning.Dec( pinst ) );
    PERFOpt( cOLDTasksCompleted.Inc( pinst ) );

    m_fIssued = false;
    return 0;
}

VOID CDefragTask::SetPtabledefragment( CTableDefragment * const ptabledefragment )
{
    m_ptabledefragment = ptabledefragment;
}

CDefragManager CDefragManager::s_instance;
const char * const CDefragManager::szCriticalSectionName = "DefragManager";

CDefragManager& CDefragManager::Instance()
{
    return s_instance;
}

CDefragManager::CDefragManager() :
    m_crit( CLockBasicInfo( CSyncBasicInfo( szCriticalSectionName ), rankDefragManager, 0 ) ),
    m_posttDispatchOsTimerTask( NULL ),
    m_fTimerScheduled( false ),
    m_cmsecPeriod( 0  ),
    m_ctasksIncrement( 1 ),
    m_ilCompleted( 0 ),
    m_ctasksIssued( 1 ),
    m_ctasksToIssueNext( 1 ),
    m_rgtasks( NULL ),
    m_itaskLastIssued( 0 )
{
    for( INT il = 0; il < m_clCompleted; ++il )
    {
        m_rglCompleted[il] = 0;
    }
    
    m_rglCompleted[m_ilCompleted] = m_ctasksIssued;
}

CDefragManager::~CDefragManager()
{
}

ERR CDefragManager::ErrInit()
{
    ERR err = JET_errSuccess;

    m_cmsecPeriod = (ULONG)UlParam( JET_paramIOThrottlingTimeQuanta );

    Call( ErrOSTimerTaskCreate( DispatchOsTimerTask_, (void*)&ErrOLDInit, &m_posttDispatchOsTimerTask ) );

HandleError:
    return err;
}

VOID CDefragManager::Term()
{
    ENTERCRITICALSECTION enterCrit( &m_crit );

    OSTrace( JET_tracetagOLDWork, OSFormat( __FUNCTION__ ": Calling OSTimerTaskCancelTask." ) );
    if ( m_posttDispatchOsTimerTask )
    {
        OSTimerTaskCancelTask( m_posttDispatchOsTimerTask );
        OSTimerTaskDelete( m_posttDispatchOsTimerTask );
        m_posttDispatchOsTimerTask = NULL;
    }

    m_fTimerScheduled = false;

    if( m_rgtasks )
    {
        for( INT itask = 0; itask < s_ctasksMax; ++itask )
        {
            if( NULL != m_rgtasks[itask].Ptabledefragment() )
            {
                RemoveTask_( itask, false );
            }
        }
    }

    delete [] m_rgtasks;
    m_rgtasks = NULL;
}

typedef struct
{
    CDefragManager *pDefragManager;
    IFMP ifmp;
    _Field_z_ const CHAR * szTable;
} OLDRegisterIndexParam;

ERR ErrOLDRegisterIndexTableCallback(
    _In_ VOID * const pvparam,
    _In_z_ char * szIndex )
{
    const OLDRegisterIndexParam * const pparam = (OLDRegisterIndexParam *)pvparam;

    if ( !pparam->pDefragManager->FTableIsRegistered( pparam->ifmp, pparam->szTable, szIndex, defragtypeIndex ) )
    {
        OSTrace( JET_tracetagOLDRegistration, OSFormat( __FUNCTION__ ": Registering %s:%s for defragtypeIndex", pparam->szTable, szIndex ) );
        const ERR err = pparam->pDefragManager->ErrTryAddTaskAtFreeSlot( pparam->ifmp, pparam->szTable, szIndex, defragtypeIndex );

        return ( JET_errFeatureNotAvailable == err ) ? JET_errSuccess : err;
    }
    else
    {
        return JET_errSuccess;
    }
}

ERR CDefragManager::ErrRegisterOneTreeOnly(
    _In_ const IFMP ifmp,
    _In_z_ const CHAR * const szTable,
    _In_opt_z_ const CHAR * const szIndex,
    _In_ DEFRAGTYPE defragtype )
{
    ERR err = JET_errSuccess;
    PIB * ppib = ppibNil;

    Assert( defragtype != defragtypeNull );
    Assert( defragtype != defragtypeLV );

    if( !g_rgfmp[ifmp].FDontRegisterOLD2Tasks() )
    {
        ENTERCRITICALSECTION enterCrit( &m_crit );

        if( !g_rgfmp[ifmp].FDontRegisterOLD2Tasks() )
        {
            if( NULL == m_rgtasks )
            {
                Alloc( m_rgtasks = new CDefragTask[s_ctasksMax] );
            }
            
            if( !FTableIsRegistered( ifmp, szTable, szIndex, defragtype ) )
            {
                Call( ErrTryAddTaskAtFreeSlot( ifmp, szTable, szIndex, defragtype ) );
                OSTrace( JET_tracetagOLDRegistration, OSFormat( __FUNCTION__ ": Registered %s:%s for defragtype=%d", szTable, szIndex ? szIndex : "<NULL>", defragtype ) );
            }
        }
    }

HandleError:
    if( ppibNil != ppib )
    {
        CallS( ErrDBCloseAllDBs( ppib ) );
        PIBEndSession( ppib );
    }
    return err;
}

ERR CDefragManager::ErrExplicitRegisterTableAndChildren(
    _In_ const IFMP ifmp,
    _In_z_ const CHAR * const szTable )
{
    ERR err = JET_errSuccess;
    PIB * ppib = ppibNil;
    FUCB * pfucbCatalog = pfucbNil;
    const FMP * pfmp = &g_rgfmp[ifmp];

    if( !pfmp->FDontRegisterOLD2Tasks() )
    {
        ENTERCRITICALSECTION enterCrit( &m_crit );

        if( !pfmp->FDontRegisterOLD2Tasks() )
        {
            if( NULL == m_rgtasks )
            {
                Alloc( m_rgtasks = new CDefragTask[ s_ctasksMax ] );
            }

            if( !FTableIsRegistered( ifmp, szTable, NULL, defragtypeTable ) )
            {
                OSTrace( JET_tracetagOLDRegistration, OSFormat( __FUNCTION__ ": Registering %s:%s for defragtypeTable.", szTable, "<primary>" ) );


                Call( ErrTryAddTaskAtFreeSlot( ifmp, szTable, NULL, defragtypeTable ) );
            }
        }
    }
    
HandleError:
    if ( pfucbNil != pfucbCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
    }
    if( ppibNil != ppib )
    {
        CallS( ErrDBCloseAllDBs( ppib ) );
        PIBEndSession( ppib );
    }
    return err;
}

VOID CDefragManager::DeregisterInst( const INST * const pinst )
{
    ENTERCRITICALSECTION enterCrit( &m_crit );

    if( m_rgtasks )
    {
        for( INT itask = 0; itask < s_ctasksMax; ++itask )
        {
            if( NULL != m_rgtasks[itask].Ptabledefragment()
                && pinst == PinstFromIfmp( m_rgtasks[itask].Ptabledefragment()->Ifmp() ) )
            {
                RemoveTask_( itask, true );
            }
        }
    }
}

VOID CDefragManager::DeregisterIfmp( const IFMP ifmp )
{
    ENTERCRITICALSECTION enterCrit( &m_crit );
    Assert( g_rgfmp[ifmp].FDontRegisterOLD2Tasks() );

    if( m_rgtasks )
    {
        for( INT itask = 0; itask < s_ctasksMax; ++itask )
        {
            if( NULL != m_rgtasks[itask].Ptabledefragment()
                && ifmp == m_rgtasks[itask].Ptabledefragment()->Ifmp() )
            {
                RemoveTask_( itask, true );
            }
        }
    }
}

VOID CDefragManager::EnsureTimerScheduled_()
{
    Assert( m_crit.FOwner() );
    Assert( m_cmsecPeriod );

    if( !m_fTimerScheduled )
    {
        m_fTimerScheduled = true;
        OSTimerTaskScheduleTask(
            m_posttDispatchOsTimerTask,
            (void*)this,
            m_cmsecPeriod,
            0 );
    }
}

ERR CDefragManager::ErrTryAddTaskAtFreeSlot(
    _In_ const IFMP ifmp,
    _In_z_ const CHAR * const szTable,
    _In_opt_z_ const CHAR * const szIndex,
    _In_ DEFRAGTYPE defragtype )
{
    PIB * ppib = ppibNil;
    FUCB * pfucbDefragStatusState = pfucbNil;
    FUCB * pfucbTable = pfucbNil;
    FUCB * pfucbToDefrag = pfucbNil;
    ERR err = JET_errSuccess;
    BOOL fAdded = fFalse;

    Assert( defragtype != defragtypeLV );

    for( INT itask = 0; itask < s_ctasksMax; ++itask )
    {
        if( NULL == m_rgtasks[itask].Ptabledefragment() )
        {
            Assert( !fAdded );
            Call( ErrAddTask_( ifmp, szTable, szIndex, defragtype, itask ) );
            EnsureTimerScheduled_();
            fAdded = fTrue;
            break;
        }
    }

    if( !fAdded )
    {
        INST * const pinst = PinstFromIfmp( ifmp );
        Call( ErrPIBBeginSession( pinst, &ppib, procidNil, fFalse ) );
        Call( ErrDBOpenDatabaseByIfmp( ppib, ifmp ) );
        Call( OLD2_STATUS::ErrOpenOrCreateTable( ppib, ifmp, &pfucbDefragStatusState ) );
        Call( ErrFILEOpenTable( ppib, ifmp, &pfucbTable, szTable ) );

        if ( szIndex == NULL )
        {
            pfucbToDefrag = pfucbTable;
        }
        else
        {
            err = ErrIsamSetCurrentIndex( (JET_SESID) ppib, (JET_TABLEID) pfucbTable, szIndex );
            pfucbToDefrag = pfucbTable->pfucbCurIndex;

            if ( pfucbToDefrag != pfucbNil && !FFUCBUnique( pfucbToDefrag ) )
            {
                Error( ErrERRCheck ( JET_errFeatureNotAvailable ) );
            }

            if ( JET_errSuccess == err && pfucbToDefrag == pfucbNil )
            {
                pfucbToDefrag = pfucbTable;
            }

            Call( err );
        }

        Assert( pfucbToDefrag != pfucbNil );

        const OBJID objidTable = ObjidFDP( pfucbTable );
        const OBJID objidToDefrag = ObjidFDP( pfucbToDefrag );

        OLD2_STATUS old2status( objidTable, objidToDefrag );
        Call( OLD2_STATUS::ErrInsertNewRecord( ppib, pfucbDefragStatusState, old2status ) );

        PERFOpt( cOLDTasksPostponed.Inc( PinstFromIfmp( ifmp ) ) );
        err = ErrERRCheck( wrnOLD2TaskSlotFull );
    }
HandleError:
    if ( pfucbNil != pfucbDefragStatusState )
    {
        CallS( ErrFILECloseTable( ppib, pfucbDefragStatusState ) );
    }
    if ( pfucbNil != pfucbTable )
    {
        CallS( ErrFILECloseTable( ppib, pfucbTable ) );
    }
    if ( ppibNil != ppib )
    {
        CallS( ErrDBCloseAllDBs( ppib ) );
        PIBEndSession( ppib );
    }
    return err;
}

ERR CDefragManager::ErrAddTask_(
    _In_ const IFMP ifmp,
    _In_z_ const CHAR * const szTable,
    _In_opt_z_ const CHAR * const szIndex,
    _In_ DEFRAGTYPE defragtype,
    _In_ const INT itask )
{
    Assert( m_crit.FOwner() );
    Assert( NULL == m_rgtasks[itask].Ptabledefragment() );
    Assert( !m_rgtasks[itask].FInit() );
    
    ERR err;
    
    CTableDefragment * ptabledefragment;
    
    Alloc( ptabledefragment = new CTableDefragment( ifmp, szTable, szIndex, defragtype ) );
    m_rgtasks[itask].SetPtabledefragment( ptabledefragment );
    Assert( NULL != m_rgtasks[itask].Ptabledefragment() );
    Assert( m_rgtasks[itask].FInit() );
    PERFOpt( cOLDTasksRegistered.Inc( PinstFromIfmp( ifmp ) ) );

    return err;

HandleError:
    Assert( err < JET_errSuccess );
    delete ptabledefragment;
    return err;
}

VOID CDefragManager::RemoveTask_( const INT itask, const bool fWaitForTask )
{
    Assert( m_crit.FOwner() );
    Assert( NULL != m_rgtasks[itask].Ptabledefragment() );
    Assert( m_rgtasks[itask].FInit() );

    while( fWaitForTask && m_rgtasks[itask].FIssued() )
    {
        UtilSleep( cmsecWaitIOComplete );
    }

    
    CTableDefragment * const ptabledefragment = m_rgtasks[itask].Ptabledefragment();

    m_rgtasks[itask].SetTickEnd( 0 );
    m_rgtasks[itask].SetPlCompleted( NULL );
    m_rgtasks[itask].SetPtabledefragment( NULL );

    PERFOpt( cOLDTasksRegistered.Dec( PinstFromIfmp( ptabledefragment->Ifmp() ) ) );
    
    CallS( ptabledefragment->ErrTerm() );
    delete ptabledefragment;

    Assert( NULL == m_rgtasks[itask].Ptabledefragment() );
    Assert( !m_rgtasks[itask].FInit() );
}

bool CDefragManager::FTableIsRegistered(
    _In_ const IFMP ifmp,
    _In_z_ const CHAR * const szTable,
    _In_opt_z_ const CHAR * const szIndex,
    _In_ DEFRAGTYPE defragtype ) const
{
    for( INT itask = 0; itask < s_ctasksMax; ++itask )
    {
        if( NULL != m_rgtasks[itask].Ptabledefragment() )
        {
            if( ifmp == m_rgtasks[itask].Ptabledefragment()->Ifmp()
                && 0 == UtilCmpName( szTable, m_rgtasks[itask].Ptabledefragment()->SzTable() ) )
            {
                switch ( defragtype )
                {
                    case defragtypeTable:
                        if( defragtypeTable == m_rgtasks[itask].Ptabledefragment()->DefragType() )
                        {
                            return true;
                        }
                        break;

                    case defragtypeIndex:
                        AssertSz( fFalse, "Defrag of secondary index is no longer supported" );
                        Assert( NULL != szIndex );
                        if ( defragtypeIndex == m_rgtasks[itask].Ptabledefragment()->DefragType() &&
                            0 == UtilCmpName( szIndex, m_rgtasks[itask].Ptabledefragment()->SzIndex() ) )
                        {
                            return true;
                        }
                        break;

                    default:
                        AssertSz( fFalse, "Unexpected defragtype: (%d).", defragtype );
                        return false;
                }
            }
        }
    }
    return false;
}

VOID CDefragManager::DispatchOsTimerTask_( VOID* const pvGroupContext, VOID* pvRuntimeContext )
{
    CDefragManager * const pdefragmanager = (CDefragManager *)pvRuntimeContext;
    Assert( pdefragmanager->m_fTimerScheduled );
    if ( pdefragmanager->FOsTimerTask_() || !pdefragmanager->m_crit.FTryEnter() )
    {
        OSTimerTaskScheduleTask(
            pdefragmanager->m_posttDispatchOsTimerTask,
            (void*)pdefragmanager,
            pdefragmanager->m_cmsecPeriod,
            0 );
    }
    else
    {
        OSTrace( JET_tracetagOLDWork, OSFormat( __FUNCTION__ ": Quiescing DispatchOsTimerTask_." ) );
        pdefragmanager->m_fTimerScheduled = fFalse;
        pdefragmanager->m_crit.Leave();
    }
}


BOOL CDefragManager::FIssueTasks_( const INT ctasksToIssue )
{
    Assert( m_crit.FOwner() );
    Assert( m_cmsecPeriod );
    Assert( m_fTimerScheduled );
    
    m_ilCompleted = ( m_ilCompleted + 1 ) % m_clCompleted;
    m_rglCompleted[m_ilCompleted] = 0;
    m_ctasksIssued = 0;
    
    const DWORD tickStart   = TickOSTimeCurrent();
    const DWORD tickEnd     = tickStart + m_cmsecPeriod;

    bool fInitTaskFound = false;

    Assert( s_ctasksMax > 0 );
    const INT itaskStart = m_itaskLastIssued;
    INT itask = itaskStart;
    do
    {
        if( m_rgtasks[itask].FIssued() )
        {
            fInitTaskFound = true;
        }
        else if ( m_rgtasks[itask].FCompleted() )
        {
            RemoveTask_( itask, true );
        }
        else if ( m_rgtasks[itask].FInit() )
        {
            fInitTaskFound = true;
            if( m_ctasksIssued < ctasksToIssue )
            {
                INST * const pinst = PinstFromIfmp( m_rgtasks[itask].Ptabledefragment()->Ifmp() );

                m_rgtasks[itask].SetTickEnd( tickEnd );
                m_rgtasks[itask].SetPlCompleted( m_rglCompleted + m_ilCompleted );
                m_rgtasks[itask].SetFIssued();

                OSTrace( JET_tracetagOLDWork, OSFormat( __FUNCTION__ ": Issuing task at slot #%d.", itask ) );

                const ERR err = pinst->Taskmgr().ErrTMPost( CDefragTask::DispatchTask, &(m_rgtasks[itask]) );
                if( err >= JET_errSuccess )
                {
                    PERFOpt( cOLDTasksRunning.Inc( pinst ) );
                    PERFOpt( cOLDTasksPosted.Inc( pinst ) );
                    ++m_ctasksIssued;
                    m_itaskLastIssued = itask;
                }
                else
                {
                    m_rgtasks[itask].ResetFIssued();
                }
            }
        }
        itask = ( itask + 1 ) % s_ctasksMax;
    } while( itask != itaskStart );

    if ( !fInitTaskFound )
    {
        OSTrace( JET_tracetagOLDWork, OSFormat( __FUNCTION__ ": We didn't find any initialized tasks at all. Stop running this periodic task." ) );
        Assert( 0 == m_ctasksIssued );

        return fFalse;
    }

    return fTrue;
}

BOOL CDefragManager::FOsTimerTask_()
{
    if ( !m_crit.FTryEnter() )
    {
        return fTrue;
    }

    Expected( m_rgtasks );
    if( !m_rgtasks )
    {
        m_crit.Leave();
        return fFalse;
    }

    const INT ctasksCompleted = m_rglCompleted[m_ilCompleted];
    
    INT ctasksToIssue;
    if ( ctasksCompleted >= m_ctasksIssued )
    {
        ctasksToIssue = max( m_ctasksIssued + m_ctasksIncrement, m_ctasksToIssueNext );
        m_ctasksToIssueNext = ctasksToIssue;
    }
    else
    {
        const INT ctasksNotCompleted = m_ctasksIssued - ctasksCompleted;
        ctasksToIssue = ctasksCompleted - ctasksNotCompleted;
        m_ctasksToIssueNext = ctasksCompleted;
    }

    ctasksToIssue = min( ctasksToIssue, s_ctasksMax );
    ctasksToIssue = max( ctasksToIssue, 0 );
    m_ctasksToIssueNext = min( m_ctasksToIssueNext, s_ctasksMax );
    m_ctasksToIssueNext = max( m_ctasksToIssueNext, 0 );

    const BOOL fRescheduleTask = FIssueTasks_( ctasksToIssue );
    
    m_crit.Leave();

    return fRescheduleTask;
}

