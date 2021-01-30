// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#include "PageSizeClean.hxx"


THREADSTATSCOUNTERS g_tscountersDbTasks;

#ifdef PERFMON_SUPPORT

LONG LDBTASKPageReferencedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    g_tscountersDbTasks.cPageReferenced.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDBTASKPageReadCEFLPv( LONG iInstance, VOID * pvBuf )
{
    g_tscountersDbTasks.cPageRead.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDBTASKPagePrereadCEFLPv( LONG iInstance, VOID * pvBuf )
{
    g_tscountersDbTasks.cPagePreread.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDBTASKPageDirtiedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    g_tscountersDbTasks.cPageDirtied.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDBTASKPageRedirtiedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    g_tscountersDbTasks.cPageRedirtied.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDBTASKLogRecordCEFLPv( LONG iInstance, VOID * pvBuf )
{
    g_tscountersDbTasks.cLogRecord.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDBTASKLogBytesCEFLPv( LONG iInstance, VOID * pvBuf )
{
    g_tscountersDbTasks.cbLogRecord.PassTo( iInstance, pvBuf );
    return 0;
}

#endif


INT TASK::g_tasksDropped = 0;

TASK::TASK()
{
}

TASK::~TASK()
{
}

DWORD TASK::DispatchGP( void *pvThis )
{
    ERR             err;
    TASK * const    ptask   = (TASK *)pvThis;
    INST * const    pinst   = ptask->PInstance();
    PIB *           ppibT;

    UPDATETHREADSTATSCOUNTERS   updatetscounters( pinst, &g_tscountersDbTasks );

    Assert( pinstNil != pinst );

    err = pinst->ErrGetSystemPib( &ppibT );
    if ( err >= JET_errSuccess )
    {
        err = ptask->ErrExecute( ppibT );
        pinst->ReleaseSystemPib( ppibT );
    }
    else
    {
        ++g_tasksDropped;
        if( 0 == g_tasksDropped % 100 )
        {
        }
    }

    if ( err < JET_errSuccess )
    {
        ptask->HandleError( err );
    }

    delete ptask;
    return 0;
}

VOID TASK::Dispatch( PIB * const ppib, const ULONG_PTR ulThis )
{
    UPDATETHREADSTATSCOUNTERS   updatetscounters( PinstFromPpib( ppib ), &g_tscountersDbTasks );

    TASK * const    ptask   = (TASK *)ulThis;
    const ERR       err     = ptask->ErrExecute( ppib );

    if ( err < 0 )
    {
        ptask->HandleError( err );
    }

    delete ptask;
}



DBTASK::DBTASK( const IFMP ifmp ) :
    m_ifmp( ifmp )
{
    FMP * const pfmp = &g_rgfmp[m_ifmp];
    CallS( pfmp->RegisterTask() );
}

ERR DBTASK::ErrExecute( PIB * const ppib )
{
    FMP * const pfmp = &g_rgfmp[m_ifmp];
    if ( pfmp->FShrinkIsRunning() || pfmp->FLeakReclaimerIsRunning() )
    {
        return ErrERRCheck( JET_errTaskDropped );
    }

    return ErrExecuteDbTask( ppib );
}

INST *DBTASK::PInstance()
{
    return PinstFromIfmp( m_ifmp );
}

DBTASK::~DBTASK()
{
    FMP * const pfmp = &g_rgfmp[m_ifmp];
    CallS( pfmp->UnregisterTask() );
}


DBREGISTEROLD2TASK::DBREGISTEROLD2TASK(
    _In_ const IFMP ifmp,
    _In_z_ const char * const szTableName,
    _In_opt_z_ const char * const szIndexName,
    _In_ DEFRAGTYPE defragtype ) :
    DBTASK( ifmp ), m_defragtype( defragtype )
{
    OSStrCbCopyA( m_szTableName, sizeof(m_szTableName), szTableName );
    m_szIndexName[ 0 ] = '\0';

    if ( szIndexName != NULL )
    {
        OSStrCbCopyA( m_szIndexName, sizeof( m_szIndexName ), szIndexName );
    }
}

ERR DBREGISTEROLD2TASK::ErrExecuteDbTask( _In_opt_ PIB *const ppib )
{
    return ErrOLDRegisterObjectForOLD2( m_ifmp, m_szTableName, m_szIndexName, m_defragtype );
}

void DBREGISTEROLD2TASK::HandleError( const ERR err )
{
}


DBEXTENDTASK::DBEXTENDTASK( const IFMP ifmp ) :
    DBTASK( ifmp )
{
}

ERR DBEXTENDTASK::ErrExecuteDbTask( PIB * const ppib )
{
    ERR         err         = JET_errSuccess;
    FMP *       pfmp        = g_rgfmp + m_ifmp;

    pfmp->AcquireIOSizeChangeLatch();

    QWORD cbSize = 0;
    err = pfmp->Pfapi()->ErrSize( &cbSize, IFileAPI::filesizeLogical );

    if ( err >= JET_errSuccess )
    {
        const QWORD cbSizeTarget = pfmp->CbFsFileSizeAsyncTarget();
        Assert( cbSizeTarget >= cbSize );

        if ( cbSizeTarget > cbSize )
        {
            PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
            tcScope->iorReason.SetIorp( iorpDatabaseExtension );
            tcScope->SetDwEngineObjid( objidSystemRoot );

            err = pfmp->Pfapi()->ErrSetSize( *tcScope,
                                                cbSizeTarget,
                                                fTrue,
                                                QosSyncDefault( pfmp->Pinst() ) );
            Assert( err <= JET_errSuccess );

            if ( err < JET_errSuccess )
            {
                pfmp->SetFsFileSizeAsyncTarget( cbSize );
            }
        }
    }

    pfmp->ReleaseIOSizeChangeLatch();

    return err;
}

VOID DBEXTENDTASK::HandleError( const ERR err )
{
}


RECTASK::RECTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm ) :
    DBTASK( ifmp ),
    m_pgnoFDP( pgnoFDP ),
    m_pfcb( pfcb ),
    m_cbBookmarkKey( bm.key.Cb() ),
    m_cbBookmarkData( bm.data.Cb() )
{
    AssertRTL( !FFMPIsTempDB( ifmp ) );

    Assert( !bm.key.FNull() );
    Assert( m_cbBookmarkKey <= sizeof( m_rgbBookmarkKey ) );
    Assert( m_cbBookmarkData <= sizeof( m_rgbBookmarkData ) );
    bm.key.CopyIntoBuffer( m_rgbBookmarkKey, sizeof( m_rgbBookmarkKey ) );
    memcpy( m_rgbBookmarkData, bm.data.Pv(), min( sizeof( m_rgbBookmarkData ), m_cbBookmarkData ) );

    Assert( m_pgnoFDP == m_pfcb->PgnoFDP() );
    Assert( prceNil != m_pfcb->PrceOldest() || m_pfcb->WRefCount() > 0 );


    m_pfcb->RegisterTask();

}

RECTASK::~RECTASK()
{
    Assert( m_pfcb->PgnoFDP() == m_pgnoFDP );


    m_pfcb->UnregisterTask();
}

ERR RECTASK::ErrOpenCursor( PIB * const ppib, FUCB ** ppfucb )
{
    ERR     err;

    Assert( m_pfcb->FInitialized() );
    Assert( m_pfcb->CTasksActive() > 0 );
    Assert( m_pgnoFDP == m_pfcb->PgnoFDP() );

    Call( ErrDIROpen( ppib, m_pfcb, ppfucb ) );
    Assert( pfucbNil != *ppfucb );
    FUCBSetIndex( *ppfucb );

HandleError:
    return err;
}

ERR RECTASK::ErrOpenCursorAndGotoBookmark( PIB * const ppib, FUCB ** ppfucb )
{
    ERR         err;
    BOOKMARK    bookmark;

    CallR( ErrOpenCursor( ppib, ppfucb ) );
    GetBookmark( &bookmark );
    Call( ErrDIRGotoBookmark( *ppfucb, bookmark ) );

HandleError:
    if( err < 0 )
    {
        DIRClose( *ppfucb );
        *ppfucb = pfucbNil;
    }

    return err;
}

VOID RECTASK::GetBookmark( __out BOOKMARK * const pbookmark ) const
{
    pbookmark->key.Nullify();
    pbookmark->key.suffix.SetPv( const_cast<BYTE *>( m_rgbBookmarkKey ) );
    pbookmark->key.suffix.SetCb( m_cbBookmarkKey );
    pbookmark->data.SetPv( const_cast<BYTE *>( m_rgbBookmarkData ) );
    pbookmark->data.SetCb( m_cbBookmarkData );
}

bool RECTASK::FBookmarkIsLessThan( const RECTASK * ptask1, const RECTASK * ptask2 )
{
    BOOKMARK    bookmark1;
    BOOKMARK    bookmark2;

    ptask1->GetBookmark( &bookmark1 );
    ptask2->GetBookmark( &bookmark2 );

    return ( CmpBM( bookmark1, bookmark2 ) < 0 );
}


DELETERECTASK::DELETERECTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm ) :
    RECTASK( pgnoFDP, pfcb, ifmp, bm )
{
}

ERR DELETERECTASK::ErrExecuteDbTask( PIB * const ppib )
{
    ERR         err;
    FUCB *      pfucb       = pfucbNil;
    BOOKMARK    bookmark;

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortRecTask );

    Assert( 0 == ppib->Level() );
    CallR( ErrOpenCursor( ppib, &pfucb ) );
    Assert( pfucbNil != pfucb );

    GetBookmark( &bookmark );

    Call( ErrBTDelete( pfucb, bookmark ) );

HandleError:
    DIRClose( pfucb );

    return err;
}

VOID DELETERECTASK::HandleError( const ERR err )
{
    Assert( err < JET_errSuccess );

    PinstFromIfmp( m_ifmp )->m_pver->IncrementCCleanupFailed();
}



template< typename TDelta >
FINALIZETASK<TDelta>::FINALIZETASK(
    const PGNO      pgnoFDP,
    FCB * const     pfcb,
    const IFMP      ifmp,
    const BOOKMARK& bm,
    const USHORT    ibRecordOffset,
    const BOOL      fCallback,
    const BOOL      fDelete ) :
    RECTASK( pgnoFDP, pfcb, ifmp, bm ),
    m_ibRecordOffset( ibRecordOffset ),
    m_fCallback( !!fCallback ),
    m_fDelete( !!fDelete )
{
    Assert( m_fCallback ^ m_fDelete );
    static BOOL s_fLimitFinalizeTaskNyi = fFalse;

    if ( m_fCallback && !FNegTest( fInvalidAPIUsage ) && !s_fLimitFinalizeTaskNyi )
    {
        AssertTrack( fFalse, "NyiFinalizeBehaviorInFinalizeTask" );
        s_fLimitFinalizeTaskNyi = fTrue;
    }
}


template< typename TDelta >
ERR FINALIZETASK<TDelta>::ErrExecuteDbTask( PIB * const ppib )
{
    ERR     err;
    FUCB *  pfucb       = pfucbNil;

    Assert( 0 == ppib->Level() );
    CallR( ErrOpenCursorAndGotoBookmark( ppib, &pfucb ) );
    Assert( pfucbNil != pfucb );

    Assert( m_fCallback ^ m_fDelete );
    Assert( m_fDelete || FNegTest( fInvalidAPIUsage ) );

    BOOL    fInTrx      = fFalse;

    Call( ErrDIRBeginTransaction( ppib, 36133, NO_GRBIT ) );
    fInTrx = fTrue;

    Call( ErrDIRGet( pfucb ) );

    TDelta  tColumnValue;
    tColumnValue = *( (UnalignedLittleEndian< TDelta > *)( (BYTE *)pfucb->kdfCurr.data.Pv() + m_ibRecordOffset ) );

    const BOOL fDeletable = 0 == tColumnValue
                    && !FVERActive( pfucb, pfucb->bmCurr )
                    && !pfucb->u.pfcb->FDeletePending();

    CallS( ErrDIRRelease( pfucb ) );

    if ( fDeletable )
    {
        Call( ErrIsamDelete( ppib, pfucb ) );
    }

    Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
    Assert( 0 == ppib->Level() );
    fInTrx = fFalse;

HandleError:
    if ( fInTrx )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    Assert( pfucbNil != pfucb );
    DIRClose( pfucb );
    return err;
}

template< typename TDelta >
VOID FINALIZETASK<TDelta>::HandleError( const ERR err )
{
    Assert( err < JET_errSuccess );

    AssertTrack( !g_rgfmp[m_ifmp].FShrinkIsRunning(), "FinalizeTaskDroppedOnShrink" );
    AssertTrack( !g_rgfmp[m_ifmp].FLeakReclaimerIsRunning(), "FinalizeTaskDroppedOnLeakReclaim" );

    PinstFromIfmp( m_ifmp )->m_pver->IncrementCCleanupFailed();
}

template class FINALIZETASK<LONG>;
template class FINALIZETASK<LONGLONG>;



DELETELVTASK::DELETELVTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm ) :
    RECTASK( pgnoFDP, pfcb, ifmp, bm )
{
}

ERR DELETELVTASK::ErrExecuteDbTask( PIB * const ppib )
{
    ERR err = JET_errSuccess;

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortRecTask );

    if ( UlParam( PinstFromPpib( ppib ), JET_paramFlight_SynchronousLVCleanup) &&
         g_rgfmp[m_ifmp].ErrDBFormatFeatureEnabled( JET_efvSynchronousLVCleanup ) >= JET_errSuccess )
    {
        Call( ErrExecute_SynchronousCleanup( ppib ) );
    }
    else
    {
        Call( ErrExecute_LegacyVersioned(ppib ) );
    }

HandleError:
    return err;
}

ERR DELETELVTASK::ErrExecute_LegacyVersioned( PIB * const ppib )
{
    ERR             err                     = JET_errSuccess;
    FUCB *          pfucb                   = pfucbNil;
    BOOL            fInTrx                  = fFalse;
    ULONG           ulLVDeleteOffset        = 0;
    CPG             cpgLVToDelete           = 0;
    ULONG           citer                   = 0;
    BOOKMARK        bookmark;
    LVROOT2         lvroot = { 0 };
    DATA            data;
    const LONG      cbLVChunkMost = m_pfcb->PfcbTable()->Ptdb()->CbLVChunkMost();
    const ULONG     cbLVDataToDeletePerTrx  = ULONG( cbLVChunkMost * g_cpgLVDataToDeletePerTrx );


    bookmark.key.Nullify();
    bookmark.key.suffix.SetPv( m_rgbBookmarkKey );
    bookmark.key.suffix.SetCb( m_cbBookmarkKey );
    bookmark.data.SetPv( m_rgbBookmarkData );
    bookmark.data.SetCb( m_cbBookmarkData );

    Assert( FIsLVRootKey( bookmark.key ) );
    Assert( 0 == m_cbBookmarkData );

    Assert( m_pfcb->FTypeLV() );
    CallR( ErrOpenCursor( ppib, &pfucb ) );

    do
    {

        Call( PinstFromIfmp( pfucb->ifmp )->m_plog->ErrLGCheckState() );

        Call( ErrDIRGotoBookmark( pfucb, bookmark ) );

        Call( ErrDIRBeginTransaction( ppib, 46373, NO_GRBIT ) );
        fInTrx = fTrue;

        Call( ErrDIRGet( pfucb ) );

        Assert( sizeof(LVROOT) == pfucb->kdfCurr.data.Cb() || sizeof(LVROOT2) == pfucb->kdfCurr.data.Cb() );
        Assert( FIsLVRootKey( pfucb->kdfCurr.key ) );

#pragma warning(suppress: 26015)
        UtilMemCpy( &lvroot, pfucb->kdfCurr.data.Pv(), min( pfucb->kdfCurr.data.Cb(), sizeof(lvroot) ) );
        data.SetPv( &lvroot );
        data.SetCb( min( pfucb->kdfCurr.data.Cb(), sizeof(lvroot) ) );


        if ( 0 != lvroot.ulReference
            || ( 0 != citer && lvroot.ulSize != ulLVDeleteOffset )
            || FDIRActiveVersion( pfucb, ( 0 != citer ? ppib->trxBegin0 : TrxOldest( PinstFromPpib( ppib ) ) ) ) )
        {
            Assert( 0 == citer );
            Error( ErrERRCheck( JET_errRecordNotDeleted ) );
        }

        Assert( 0 == citer
            || 0 == lvroot.ulSize % cbLVDataToDeletePerTrx );

        if ( lvroot.ulSize > cbLVDataToDeletePerTrx )
        {
            ulLVDeleteOffset = ( ( lvroot.ulSize - 1 ) / cbLVDataToDeletePerTrx ) * cbLVDataToDeletePerTrx;

            Assert( 0 == ulLVDeleteOffset % cbLVDataToDeletePerTrx );
            Assert( 0 == cbLVDataToDeletePerTrx % cbLVChunkMost );
            Assert( ulLVDeleteOffset < lvroot.ulSize );

            cpgLVToDelete = ( lvroot.ulSize - ulLVDeleteOffset + cbLVChunkMost - 1 ) / cbLVChunkMost;

            lvroot.ulSize = ulLVDeleteOffset;
            Assert( data.Pv() == &lvroot );
            Assert( data.Cb() == sizeof(LVROOT) || data.Cb() == sizeof(LVROOT2) );
            Call( ErrDIRReplace( pfucb, data, fDIRNull ) );
            Call( ErrDIRGet( pfucb ) );
        }
        else
        {
            ulLVDeleteOffset = 0;
            cpgLVToDelete = ( lvroot.ulSize + cbLVChunkMost - 1 ) / cbLVChunkMost;
        }
        
        FUCBSetPrereadForward( pfucb, cpgLVToDelete );

        Call( ErrRECDeleteLV_LegacyVersioned( pfucb, ulLVDeleteOffset, fDIRNull ) );

        if ( Pcsr( pfucb )->FLatched() )
        {
            CallS( ErrDIRRelease( pfucb ) );
        }
    
        Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
        fInTrx = fFalse;

        citer++;
    }
    while ( 0 != ulLVDeleteOffset );

HandleError:
    if ( fInTrx )
    {
        if ( Pcsr( pfucb )->FLatched() )
        {
            CallS( ErrDIRRelease( pfucb ) );
        }
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    if ( pfucbNil != pfucb )
    {
        DIRClose( pfucb );
    }
    return err;
}

ERR DELETELVTASK::ErrExecute_SynchronousCleanup( PIB * const ppib )
{
    ERR             err                     = JET_errSuccess;
    FUCB *          pfucb                   = pfucbNil;
    BOOL            fInTrx                  = fFalse;
    ULONG           cChunksToDelete         = 0;
    ULONG           cRemainChunksToDelete   = 0;
    CPG             cpgLVToDelete           = 0;
    ULONG           citer                   = 0;
    BOOKMARK        bookmark;
    LVROOT2         lvroot                  = { 0 };
    DATA            data;
    const LONG      cbLVChunkMost           = m_pfcb->PfcbTable()->Ptdb()->CbLVChunkMost();
    const ULONG     cChunksPerPage          = ULONG( g_rgfmp[ m_pfcb->Ifmp() ].CbPage() / cbLVChunkMost );
    const ULONG     cChunksToDeletePerTrx   = ULONG( g_cpgLVDataToDeletePerTrx * cChunksPerPage );
    ULONG           ulLVSize                = 0;

    bookmark.key.Nullify();
    bookmark.key.suffix.SetPv( m_rgbBookmarkKey );
    bookmark.key.suffix.SetCb( m_cbBookmarkKey );
    bookmark.data.SetPv( m_rgbBookmarkData );
    bookmark.data.SetCb( m_cbBookmarkData );

    Assert( FIsLVRootKey( bookmark.key ) );
    Assert( 0 == m_cbBookmarkData );

    Assert( m_pfcb->FTypeLV() );
    CallR( ErrOpenCursor( ppib, &pfucb ) );

    do
    {

        Call( PinstFromIfmp( pfucb->ifmp )->m_plog->ErrLGCheckState() );

        err = ErrDIRGotoBookmark( pfucb, bookmark );
        if ( JET_errNoCurrentRecord == err || JET_errWriteConflict == err )
        {
            err = JET_errSuccess;
            break;
        }
        Call( err );

        Assert( 0 == ppib->Level() );
        Call( ErrDIRBeginTransaction( ppib, 38778, NO_GRBIT ) );
        fInTrx = fTrue;

        err = ErrDIRGetLock( pfucb, writeLock );
        if( JET_errWriteConflict == err )
        {
            err = JET_errSuccess;
            break;
        }
        Call( err );

        Call( ErrDIRGet( pfucb ) );

        Assert( sizeof(LVROOT) == pfucb->kdfCurr.data.Cb() || sizeof(LVROOT2) == pfucb->kdfCurr.data.Cb() );
        Assert( FIsLVRootKey( pfucb->kdfCurr.key ) );

#pragma warning(suppress: 26015)
        UtilMemCpy( &lvroot, pfucb->kdfCurr.data.Pv(), min( pfucb->kdfCurr.data.Cb(), sizeof(lvroot) ) );
        data.SetPv( &lvroot );
        data.SetCb( min( pfucb->kdfCurr.data.Cb(), sizeof(lvroot) ) );

        if ( ( 0 != lvroot.ulReference && !FPartiallyDeletedLV( lvroot.ulReference ) )
            || ( 0 != citer && lvroot.ulSize != ulLVSize ) )
        {
            Assert( 0 == citer );
            Error( ErrERRCheck( JET_errRecordNotDeleted ) );
        }

        if ( citer == 0 )
        {
            Assert( lvroot.ulReference == 0 || lvroot.ulReference == ulMax );
            Assert( data.Pv() == &lvroot );
            Assert( data.Cb() == sizeof(LVROOT) || data.Cb() == sizeof(LVROOT2) );
            
            lvroot.ulReference = ulMax;
            Call( ErrDIRReplace( pfucb, data, fDIRNoVersion ) );
            Call( ErrDIRGet( pfucb ) );

            cRemainChunksToDelete = ( lvroot.ulSize + cbLVChunkMost - 1 ) / cbLVChunkMost;
            cRemainChunksToDelete++;
            
            ulLVSize = lvroot.ulSize;
        }

        cChunksToDelete = min( cChunksToDeletePerTrx, cRemainChunksToDelete );
        cpgLVToDelete = cChunksToDelete / cChunksPerPage;
        FUCBSetPrereadForward( pfucb, cpgLVToDelete );

        Call( ErrRECDeleteLV_SynchronousCleanup( pfucb, cChunksToDelete ) );

        if ( Pcsr( pfucb )->FLatched() )
        {
            CallS( ErrDIRRelease( pfucb ) );
        }
    
        Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
        fInTrx = fFalse;

        citer++;
        cRemainChunksToDelete -= cChunksToDelete;
    }
    while ( cRemainChunksToDelete > 0 );

HandleError:
    if ( fInTrx )
    {
        if ( Pcsr( pfucb )->FLatched() )
        {
            DIRReleaseLatch( pfucb );
        }
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    if ( pfucbNil != pfucb )
    {
        DIRClose( pfucb );
    }

    return err;
}

VOID DELETELVTASK::HandleError( const ERR err )
{
    Assert( err < JET_errSuccess );

    PinstFromIfmp( m_ifmp )->m_pver->IncrementCCleanupFailed();
}



MERGEAVAILEXTTASK::MERGEAVAILEXTTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm ) :
    RECTASK( pgnoFDP, pfcb, ifmp, bm )
{
}

ERR MERGEAVAILEXTTASK::ErrExecuteDbTask( PIB * const ppib )
{
    ERR         err         = JET_errSuccess;
    FUCB        * pfucbAE   = pfucbNil;
    BOOL        fInTrx      = fFalse;
    BOOKMARK    bookmark;

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortRecTask );

    GetBookmark( &bookmark );

    Assert( m_pgnoFDP == m_pfcb->PgnoFDP() );
    Assert( pgnoNull != m_pfcb->PgnoAE() );
    CallR( ErrSPIOpenAvailExt( ppib, m_pfcb, &pfucbAE ) );

    Call( ErrDIRBeginTransaction( ppib, 62757, NO_GRBIT ) );
    fInTrx = fTrue;

    Call( ErrBTIMultipageCleanup( pfucbAE, bookmark, NULL, NULL, NULL, fTrue ) );

    Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
    fInTrx = fFalse;

HandleError:
    Assert( pfucbNil != pfucbAE );
    Assert( !Pcsr( pfucbAE )->FLatched() );

    if ( fInTrx )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    Assert( pfucbNil != pfucbAE );
    BTClose( pfucbAE );

    return err;
}

VOID MERGEAVAILEXTTASK::HandleError( const ERR err )
{
}


BATCHRECTASK::BATCHRECTASK( const PGNO pgnoFDP, const IFMP ifmp ) :
    DBTASK( ifmp ),
    m_rgprectask( NULL ),
    m_cptaskMax( 0 ),
    m_iptaskCurr( 0 ),
    m_pgnoFDP( pgnoFDP )
{
    Assert( pgnoNull != m_pgnoFDP );
    Assert( ifmpNil != m_ifmp );
}

BATCHRECTASK::~BATCHRECTASK()
{
    if( m_rgprectask )
    {
        for( INT iptask = 0; iptask < m_iptaskCurr; ++iptask )
        {
            RECTASK * const prectaskT = m_rgprectask[iptask];
            m_rgprectask[iptask] = NULL;
            delete prectaskT;
        }
    }
    delete [] m_rgprectask;
    m_rgprectask = NULL;
}

ERR BATCHRECTASK::ErrAddTask( RECTASK * const prectask )
{
    Assert( prectask );
    Assert( prectask->PgnoFDP() == m_pgnoFDP );
    Assert( prectask->Ifmp() == m_ifmp );

    
    Assert( PinstFromIfmp( prectask->Ifmp() )->m_pver->m_critRCEClean.FOwner() );
    
    ERR err = JET_errSuccess;

    if( m_iptaskCurr >= m_cptaskMax )
    {
        const INT cptaskMaxT = m_cptaskMax + 16;
        RECTASK ** rgprectaskT;
        
        Alloc( rgprectaskT = new RECTASK*[cptaskMaxT] );
        memset( rgprectaskT, 0, cptaskMaxT * sizeof(TASK*) );
        
        UtilMemCpy( rgprectaskT, m_rgprectask, m_cptaskMax * sizeof(TASK*) );
        delete [] m_rgprectask;

        m_rgprectask = rgprectaskT;
        m_cptaskMax = cptaskMaxT;
    }

    AssertPREFIX( m_iptaskCurr < m_cptaskMax );
    m_rgprectask[m_iptaskCurr++] = prectask;
    AssertPREFIX( m_iptaskCurr <= m_cptaskMax );

HandleError:
    return err;
}

ERR BATCHRECTASK::ErrExecuteDbTask( PIB * const ppib )
{
    if( !m_rgprectask )
    {
        return JET_errSuccess;
    }
    
    SortTasksByBookmark();

    LONG ctasksPreread = 0;
    PrereadTaskBookmarks( ppib, 0, &ctasksPreread );
    
    for( INT iptask = 0; iptask < m_iptaskCurr; ++iptask )
    {
        if ( PInstance()->FInstanceUnavailable() )
        {
            return PInstance()->m_errInstanceUnavailable;
        }

        if ( PInstance()->m_plog->FNoMoreLogWrite() )
        {
            return ErrERRCheck( JET_errLogWriteFail );
        }

        RECTASK * const prectaskT = m_rgprectask[iptask];
        m_rgprectask[iptask] = NULL;
        TASK::Dispatch( ppib, (ULONG_PTR)prectaskT );

        if( ctasksPreread > 0 )
        {
            --ctasksPreread;
            if( 0 == ctasksPreread )
            {
                PrereadTaskBookmarks( ppib, iptask+1, &ctasksPreread );
            }
        }
    }
    return JET_errSuccess;
}


VOID BATCHRECTASK::HandleError( const ERR err )
{
    Assert( err < JET_errSuccess );

    for( INT iptask = 0; iptask < m_iptaskCurr; ++iptask )
    {
        if( m_rgprectask[iptask] )
        {
            PinstFromIfmp( m_ifmp )->m_pver->IncrementCCleanupFailed();
        }
    }
}

VOID BATCHRECTASK::SortTasksByBookmark()
{
    Assert( m_rgprectask );
    sort( m_rgprectask, m_rgprectask + m_iptaskCurr, RECTASK::FBookmarkIsLessThan );
}

VOID BATCHRECTASK::PrereadTaskBookmarks( PIB * const ppib, const INT itaskStart, __out LONG * pctasksPreread )
{
    *pctasksPreread = 0;

    const INT cbookmarksPreread = min( m_iptaskCurr - itaskStart, 1024 );
    if( 0 == cbookmarksPreread )
    {
        Assert( 0 == *pctasksPreread );
        return;
    }


    BOOKMARK * rgbm;
    if( NULL != ( rgbm = new BOOKMARK[cbookmarksPreread] ) )
    {
        FUCB * pfucb = pfucbNil;
        if( JET_errSuccess == ErrDIROpen( ppib, m_pgnoFDP, m_ifmp, &pfucb ) )
        {
            PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
            tcScope->iorReason.SetIort( iortRecTask );

            for( INT ibm = 0; ibm < cbookmarksPreread; ++ibm )
            {
                m_rgprectask[ibm+itaskStart]->GetBookmark( rgbm+ibm );
            }

            const ERR errT = ErrBTPrereadBookmarks(
                ppib,
                pfucb,
                rgbm,
                cbookmarksPreread,
                pctasksPreread,
                JET_bitPrereadForward | bitPrereadDoNotDoOLD2 );
            Assert( JET_errInvalidGrbit != errT );
            DIRClose(pfucb);
        }
        delete[] rgbm;
    }
}

    

RECTASKBATCHER::RECTASKBATCHER(
    INST * const pinst,
    const INT cbatchesMax,
    const INT ctasksPerBatchMax,
    const INT ctasksBatchedMax ) :
        m_pinst( pinst ),
        m_cbatchesMax( cbatchesMax ),
        m_ctasksPerBatchMax( ctasksPerBatchMax ),
        m_ctasksBatchedMax( ctasksBatchedMax ),
        m_rgpbatchtask( NULL ),
        m_ctasksBatched( 0 )
{
    Assert( cbatchesMax > 0 );
    Assert( ctasksPerBatchMax >= 2 );
    Assert( ctasksBatchedMax >= cbatchesMax * 2 );
}
    
RECTASKBATCHER::~RECTASKBATCHER()
{
    if( m_rgpbatchtask )
    {
        for( INT ipbatchtask = 0; ipbatchtask < m_cbatchesMax; ++ipbatchtask )
        {
            if( m_rgpbatchtask[ipbatchtask] )
            {
                BATCHRECTASK * const pbatchrectaskT = m_rgpbatchtask[ipbatchtask];
                m_ctasksBatched -= pbatchrectaskT->CTasks();
                m_rgpbatchtask[ipbatchtask] = NULL;
                
                delete pbatchrectaskT;
            }
        }
        Assert( 0 == m_ctasksBatched );
        delete[] m_rgpbatchtask;
        m_rgpbatchtask = NULL;
    }
}

ERR RECTASKBATCHER::ErrPost( RECTASK * prectask )
{
    Assert( prectask );
    Assert( m_ctasksBatched >= 0 );

    
    Assert( PinstFromIfmp( prectask->Ifmp() )->m_pver->m_critRCEClean.FOwner() );

    ERR err = JET_errSuccess;
    bool fTaskAdded = false;
    if( !m_rgpbatchtask )
    {
        Alloc( m_rgpbatchtask = new BATCHRECTASK*[m_cbatchesMax] );
        memset( m_rgpbatchtask, 0, m_cbatchesMax*sizeof(BATCHRECTASK*) );
    }


    bool fFoundBatchTask = false;
    
    INT ipbatchtaskFree = m_cbatchesMax;
    for( INT ipbatchtask = 0; ipbatchtask < m_cbatchesMax; ++ipbatchtask )
    {
        if( m_rgpbatchtask[ipbatchtask] )
        {
            BATCHRECTASK * const pbatchrectask = m_rgpbatchtask[ipbatchtask];
            Assert( pbatchrectask->CTasks() < m_ctasksPerBatchMax );
            if( pbatchrectask->PgnoFDP() == prectask->PgnoFDP()
                && pbatchrectask->Ifmp() == prectask->Ifmp() )
            {
                Assert( !fFoundBatchTask );
                Call( pbatchrectask->ErrAddTask( prectask ) );
                fTaskAdded = true;
                fFoundBatchTask = true;
                m_ctasksBatched++;
                Assert( m_ctasksBatched > 0 );
                if( pbatchrectask->CTasks() >= m_ctasksPerBatchMax )
                {
                    Assert( pbatchrectask->CTasks() == m_ctasksPerBatchMax );
                    Assert( pbatchrectask == m_rgpbatchtask[ipbatchtask] );
                    Call( ErrPostOneBatch( ipbatchtask ) );
                }
                break;
            }
        }
        else
        {
            ipbatchtaskFree = min( ipbatchtaskFree, ipbatchtask );
        }
    }

    
    if( !fFoundBatchTask )
    {
        Assert( !fTaskAdded );
        if( ipbatchtaskFree < m_cbatchesMax )
        {
            Alloc( m_rgpbatchtask[ipbatchtaskFree] = new BATCHRECTASK( prectask->PgnoFDP(), prectask->Ifmp() ) );
            Call( m_rgpbatchtask[ipbatchtaskFree]->ErrAddTask( prectask ) );
            fTaskAdded = true;
            Assert( 1 == m_rgpbatchtask[ipbatchtaskFree]->CTasks() );
            m_ctasksBatched++;
            Assert( m_ctasksBatched > 0 );
        }
        else
        {
            Call( ErrPostAllPending() );
            Call( m_pinst->Taskmgr().ErrTMPost( TASK::DispatchGP, prectask ) );
            fTaskAdded = true;
            Assert( 0 == m_ctasksBatched );
        }
    }

    
    if( m_ctasksBatched >= m_ctasksBatchedMax )
    {
        Assert( m_ctasksBatched == m_ctasksBatchedMax );
        Call( ErrPostAllPending() );
        Assert( 0 == m_ctasksBatched );
    }

HandleError:
    
    if( ! fTaskAdded )
    {
        Assert( err < JET_errSuccess );
        PinstFromIfmp( prectask->Ifmp() )->m_pver->IncrementCCleanupFailed();
        delete prectask;
        prectask = NULL;
    }
    return err;
}

ERR RECTASKBATCHER::ErrPostAllPending()
{
    
    Assert( m_pinst->m_pver->m_critRCEClean.FOwner() );
    
    ERR err = JET_errSuccess;
    
    if( m_rgpbatchtask )
    {
        for( INT ipbatchtask = 0; ipbatchtask < m_cbatchesMax; ++ipbatchtask )
        {
            if( m_rgpbatchtask[ipbatchtask] )
            {
                const ERR errT = ErrPostOneBatch( ipbatchtask );
                if( errT < JET_errSuccess )
                {
                    err = errT;
                }
            }
        }
    }

    Assert( 0 == m_ctasksBatched );
    return err;
}

ERR RECTASKBATCHER::ErrPostOneBatch( const INT ipbatchtask )
{
    Assert( ipbatchtask >= 0 );
    Assert( ipbatchtask < m_cbatchesMax );
    Assert( m_rgpbatchtask );
    Assert( NULL != m_rgpbatchtask[ipbatchtask] );

    
    Assert( PinstFromIfmp( m_rgpbatchtask[ipbatchtask]->Ifmp() )->m_pver->m_critRCEClean.FOwner() );
    
    BATCHRECTASK * const pbatchrectaskT = m_rgpbatchtask[ipbatchtask];
    Assert( pbatchrectaskT );
    
    m_ctasksBatched -= pbatchrectaskT->CTasks();
    m_rgpbatchtask[ipbatchtask] = NULL;

    const ERR err = m_pinst->Taskmgr().ErrTMPost( TASK::DispatchGP, pbatchrectaskT );
    if( err < JET_errSuccess )
    {
        for( INT i = 0; i < pbatchrectaskT->CTasks(); ++i )
        {
            PinstFromIfmp( pbatchrectaskT->Ifmp() )->m_pver->IncrementCCleanupFailed();
        }
        delete pbatchrectaskT;
    }

    Assert( NULL == m_rgpbatchtask[ipbatchtask] );
    return err;
}

