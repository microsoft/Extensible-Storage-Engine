// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#ifdef PERFMON_SUPPORT

//  Global and per-instance counters.

PERFInstanceDelayedTotal<HRT>       cIOTotalDhrts[iotypeMax][iofileMax];
PERFInstanceDelayedTotal<QWORD>     cIOTotalBytes[iotypeMax][iofileMax];
PERFInstanceDelayedTotal<>          cIOTotal[iotypeMax][iofileMax];
PERFInstanceDelayedTotal<>          cIOInHeap[iotypeMax][iofileMax];
PERFInstanceDelayedTotal<>          cIOAsyncPending[iotypeMax][iofileMax];

//  Per-DB counters.

PERFInstance<HRT>       cIOPerDBTotalDhrts[iotypeMax];
PERFInstance<QWORD>     cIOPerDBTotalBytes[iotypeMax];
PERFInstance<>          cIOPerDBTotal[iotypeMax];

//  These counters primarily use .Set() which has to set all iCpus values, so better to NOT be hashed by CPU.
PERFInstance<QWORD, fFalse>  cIOPerDBLatencyCount[iotypeMax][iocatMax];
PERFInstance<QWORD, fFalse>  cIOPerDBLatencyAve[iotypeMax][iocatMax];
PERFInstance<QWORD, fFalse>  cIOPerDBLatencyP50[iotypeMax][iocatMax];
PERFInstance<QWORD, fFalse>  cIOPerDBLatencyP90[iotypeMax][iocatMax];
PERFInstance<QWORD, fFalse>  cIOPerDBLatencyP99[iotypeMax][iocatMax];
PERFInstance<QWORD, fFalse>  cIOPerDBLatencyP100[iotypeMax][iocatMax];
PERFInstance<QWORD, fFalse>  cIODatabaseMetedQueueDepth[iofileMax];
PERFInstance<QWORD, fFalse>  cIODatabaseMetedOutstandingMax[iofileMax];
PERFInstance<QWORD, fFalse>  cIODatabaseAsyncReadPending[iofileMax];

//  protecting you from yourself

C_ASSERT( _countof( cIOTotalBytes[0] ) == iofileMax );

//  we are also building tools to depend upon these (so shifting them may cause data analysis tools to break)

C_ASSERT( iofileDbAttached == (IOFILE)1 );
C_ASSERT( iofileDbRecovery == (IOFILE)2 );
C_ASSERT( iofileLog == (IOFILE)3 );
C_ASSERT( iofileDbTotal == (IOFILE)4 );
C_ASSERT( iofileFlushMap == (IOFILE)5 );

C_ASSERT( JET_filetypeDatabase == iofileDbAttached );
C_ASSERT( JET_filetypeLog == iofileLog );


LONG LIODbReadIOTotalAttachedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotal[iotypeRead][iofileDbAttached].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbReadIOTotalRecoveryCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotal[iotypeRead][iofileDbRecovery].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbReadIOTotalTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotal[iotypeRead][iofileDbTotal].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOLogReadIOTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotal[iotypeRead][iofileLog].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOSnapshotReadIOTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotal[iotypeRead][iofileRBS].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOOtherReadIOTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotal[iotypeRead][iofileOther].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbReadIOTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBTotal[iotypeRead].PassTo( iInstance, pvBuf );
    return 0;
}

LONG LIODbReadIOTotalTicksAttachedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*)pvBuf = CmsecHRTFromDhrt( cIOTotalDhrts[iotypeRead][iofileDbAttached].Get( iInstance ) );
    }
    return 0;
}
LONG LIODbReadIOTotalTicksRecoveryCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*)pvBuf = CmsecHRTFromDhrt( cIOTotalDhrts[iotypeRead][iofileDbRecovery].Get( iInstance ) );
    }
    return 0;
}
LONG LIODbReadIOTotalTicksTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*)pvBuf = CmsecHRTFromDhrt( cIOTotalDhrts[iotypeRead][iofileDbTotal].Get( iInstance ) );
    }
    return 0;
}
LONG LIOLogReadIOTotalTicksCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*)pvBuf = CmsecHRTFromDhrt( cIOTotalDhrts[iotypeRead][iofileLog].Get( iInstance ) );
    }
    return 0;
}
LONG LIOSnapshotReadIOTotalTicksCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*)pvBuf = CmsecHRTFromDhrt( cIOTotalDhrts[iotypeRead][iofileRBS].Get( iInstance ) );
    }
    return 0;
}
LONG LIOOtherReadIOTotalTicksCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*)pvBuf = CmsecHRTFromDhrt( cIOTotalDhrts[iotypeRead][iofileOther].Get( iInstance ) );
    }
    return 0;
}
LONG LIODbReadIOTicksTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*)pvBuf = CmsecHRTFromDhrt( cIOPerDBTotalDhrts[iotypeRead].Get( iInstance ) );
    }
    return 0;
}

LONG LIODbReadIOTotalBytesAttachedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotalBytes[iotypeRead][iofileDbAttached].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbReadIOTotalBytesRecoveryCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotalBytes[iotypeRead][iofileDbRecovery].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbReadIOTotalBytesTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotalBytes[iotypeRead][iofileDbTotal].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOLogReadIOTotalBytesCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotalBytes[iotypeRead][iofileLog].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOSnapshotReadIOTotalBytesCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotalBytes[iotypeRead][iofileRBS].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOOtherReadIOTotalBytesCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotalBytes[iotypeRead][iofileOther].PassTo( iInstance, pvBuf );
    return 0;
}
// NOTE: For per DB the iInstance is not the pinst->m_iInstance, but based off the ifmp.
LONG LIODbReadIOBytesTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBTotalBytes[iotypeRead].PassTo( iInstance, pvBuf );
    return 0;
}

LONG LIODbReadTransactionalIoLatencyCountCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyCount[iotypeRead][iocatTransactional].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbReadTransactionalIoLatencyAveCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyAve[iotypeRead][iocatTransactional].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbReadTransactionalIoLatencyP50CEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyP50[iotypeRead][iocatTransactional].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbReadTransactionalIoLatencyP90CEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyP90[iotypeRead][iocatTransactional].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbReadTransactionalIoLatencyP99CEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyP99[iotypeRead][iocatTransactional].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbReadTransactionalIoLatencyP100CEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyP100[iotypeRead][iocatTransactional].PassTo( iInstance, pvBuf );
    return 0;
}

LONG LIODbReadMaintenanceIoLatencyCountCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyCount[iotypeRead][iocatMaintenance].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbReadMaintenanceIoLatencyAveCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyAve[iotypeRead][iocatMaintenance].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbReadMaintenanceIoLatencyP50CEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyP50[iotypeRead][iocatMaintenance].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbReadMaintenanceIoLatencyP90CEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyP90[iotypeRead][iocatMaintenance].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbReadMaintenanceIoLatencyP99CEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyP99[iotypeRead][iocatMaintenance].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbReadMaintenanceIoLatencyP100CEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyP100[iotypeRead][iocatMaintenance].PassTo( iInstance, pvBuf );
    return 0;
}

LONG LIODatabaseMetedQueueDepthCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIODatabaseMetedQueueDepth[iofileDbTotal].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODatabaseMetedOutstandingMaxCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIODatabaseMetedOutstandingMax[iofileDbTotal].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODatabaseAsyncReadPendingCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIODatabaseAsyncReadPending[iofileDbTotal].PassTo( iInstance, pvBuf );
    return 0;
}


LONG LIODbReadIOInHeapAttachedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOInHeap[iotypeRead][iofileDbAttached].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbReadIOInHeapRecoveryCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOInHeap[iotypeRead][iofileDbRecovery].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbReadIOInHeapTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOInHeap[iotypeRead][iofileDbTotal].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOLogReadIOInHeapCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOInHeap[iotypeRead][iofileLog].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOSnapshotReadIOInHeapCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOInHeap[iotypeRead][iofileRBS].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOOtherReadIOInHeapCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOInHeap[iotypeRead][iofileOther].PassTo( iInstance, pvBuf );
    return 0;
}

LONG LIODbReadIOAsyncPendingAttachedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOAsyncPending[iotypeRead][iofileDbAttached].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbReadIOAsyncPendingRecoveryCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOAsyncPending[iotypeRead][iofileDbRecovery].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbReadIOAsyncPendingTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOAsyncPending[iotypeRead][iofileDbTotal].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOLogReadIOAsyncPendingCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOAsyncPending[iotypeRead][iofileLog].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOSnapshotReadIOAsyncPendingCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOAsyncPending[iotypeRead][iofileRBS].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOOtherReadIOAsyncPendingCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOAsyncPending[iotypeRead][iofileOther].PassTo( iInstance, pvBuf );
    return 0;
}

LONG LIODbWriteIOTotalAttachedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotal[iotypeWrite][iofileDbAttached].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbWriteIOTotalRecoveryCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotal[iotypeWrite][iofileDbRecovery].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbWriteIOTotalTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotal[iotypeWrite][iofileDbTotal].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOFmWriteIOTotalTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotal[iotypeWrite][iofileFlushMap].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOLogWriteIOTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotal[iotypeWrite][iofileLog].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOSnapshotWriteIOTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotal[iotypeWrite][iofileRBS].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOOtherWriteIOTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotal[iotypeWrite][iofileOther].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbWriteIOTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBTotal[iotypeWrite].PassTo( iInstance, pvBuf );
    return 0;
}

LONG LIODbWriteIOTotalTicksAttachedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*)pvBuf = CmsecHRTFromDhrt( cIOTotalDhrts[iotypeWrite][iofileDbAttached].Get( iInstance ) );
    }
    return 0;
}
LONG LIODbWriteIOTotalTicksRecoveryCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*)pvBuf = CmsecHRTFromDhrt( cIOTotalDhrts[iotypeWrite][iofileDbRecovery].Get( iInstance ) );
    }
    return 0;
}
LONG LIODbWriteIOTotalTicksTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*)pvBuf = CmsecHRTFromDhrt( cIOTotalDhrts[iotypeWrite][iofileDbTotal].Get( iInstance ) );
    }
    return 0;
}
LONG LIOFmWriteIOTotalTicksTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*)pvBuf = CmsecHRTFromDhrt( cIOTotalDhrts[iotypeWrite][iofileFlushMap].Get( iInstance ) );
    }
    return 0;
}
LONG LIOLogWriteIOTotalTicksCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*)pvBuf = CmsecHRTFromDhrt( cIOTotalDhrts[iotypeWrite][iofileLog].Get( iInstance ) );
    }
    return 0;
}
LONG LIOSnapshotWriteIOTotalTicksCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*)pvBuf = CmsecHRTFromDhrt( cIOTotalDhrts[iotypeWrite][iofileRBS].Get( iInstance ) );
    }
    return 0;
}
LONG LIOOtherWriteIOTotalTicksCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*)pvBuf = CmsecHRTFromDhrt( cIOTotalDhrts[iotypeWrite][iofileOther].Get( iInstance ) );
    }
    return 0;
}
LONG LIODbWriteIOTicksTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*)pvBuf = CmsecHRTFromDhrt( cIOPerDBTotalDhrts[iotypeWrite].Get( iInstance ) );
    }
    return 0;
}

LONG LIODbWriteIOTotalBytesAttachedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotalBytes[iotypeWrite][iofileDbAttached].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbWriteIOTotalBytesRecoveryCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotalBytes[iotypeWrite][iofileDbRecovery].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbWriteIOTotalBytesTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotalBytes[iotypeWrite][iofileDbTotal].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOFmWriteIOTotalBytesTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotalBytes[iotypeWrite][iofileFlushMap].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOLogWriteIOTotalBytesCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotalBytes[iotypeWrite][iofileLog].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOSnapshotWriteIOTotalBytesCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotalBytes[iotypeWrite][iofileRBS].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOOtherWriteIOTotalBytesCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOTotalBytes[iotypeWrite][iofileOther].PassTo( iInstance, pvBuf );
    return 0;
}
// NOTE: For per DB the iInstance is not the pinst->m_iInstance, but the based off the ifmp.
LONG LIODbWriteIOBytesTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBTotalBytes[iotypeWrite].PassTo( iInstance, pvBuf );
    return 0;
}

LONG LIODbWriteTransactionalIoLatencyCountCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyCount[iotypeWrite][iocatTransactional].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbWriteTransactionalIoLatencyAveCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyAve[iotypeWrite][iocatTransactional].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbWriteTransactionalIoLatencyP50CEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyP50[iotypeWrite][iocatTransactional].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbWriteTransactionalIoLatencyP90CEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyP90[iotypeWrite][iocatTransactional].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbWriteTransactionalIoLatencyP99CEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyP99[iotypeWrite][iocatTransactional].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbWriteTransactionalIoLatencyP100CEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyP100[iotypeWrite][iocatTransactional].PassTo( iInstance, pvBuf );
    return 0;
}

LONG LIODbWriteMaintenanceIoLatencyCountCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyCount[iotypeWrite][iocatMaintenance].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbWriteMaintenanceIoLatencyAveCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyAve[iotypeWrite][iocatMaintenance].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbWriteMaintenanceIoLatencyP50CEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyP50[iotypeWrite][iocatMaintenance].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbWriteMaintenanceIoLatencyP90CEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyP90[iotypeWrite][iocatMaintenance].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbWriteMaintenanceIoLatencyP99CEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyP99[iotypeWrite][iocatMaintenance].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbWriteMaintenanceIoLatencyP100CEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOPerDBLatencyP100[iotypeWrite][iocatMaintenance].PassTo( iInstance, pvBuf );
    return 0;
}

LONG LIODbWriteIOInHeapAttachedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOInHeap[iotypeWrite][iofileDbAttached].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbWriteIOInHeapRecoveryCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOInHeap[iotypeWrite][iofileDbRecovery].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbWriteIOInHeapTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOInHeap[iotypeWrite][iofileDbTotal].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOLogWriteIOInHeapCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOInHeap[iotypeWrite][iofileLog].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOSnapshotWriteIOInHeapCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOInHeap[iotypeWrite][iofileRBS].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOOtherWriteIOInHeapCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOInHeap[iotypeWrite][iofileOther].PassTo( iInstance, pvBuf );
    return 0;
}

LONG LIODbWriteIOAsyncPendingAttachedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOAsyncPending[iotypeWrite][iofileDbAttached].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbWriteIOAsyncPendingRecoveryCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOAsyncPending[iotypeWrite][iofileDbRecovery].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIODbWriteIOAsyncPendingTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOAsyncPending[iotypeWrite][iofileDbTotal].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOLogWriteIOAsyncPendingCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOAsyncPending[iotypeWrite][iofileLog].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOSnapshotWriteIOAsyncPendingCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOAsyncPending[iotypeWrite][iofileRBS].PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOOtherWriteIOAsyncPendingCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cIOAsyncPending[iotypeWrite][iofileOther].PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<HRT>       cFFBTotalDhrts;
PERFInstanceDelayedTotal<>          cFFBTotal;
LONG LIOFFBTotalCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cFFBTotal.PassTo( iInstance, pvBuf );
    return 0;
}
LONG LIOFFBTotalTicksCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*)pvBuf = CmsecHRTFromDhrt( cFFBTotalDhrts.Get( iInstance ) );
    }
    return 0;
}

PERFInstanceDelayedTotal<> cIODatabaseFileExtensionStall;
LONG LIODatabaseFileExtensionStallCEFLPv( LONG iInstance, void* pvBuf )
{
    cIODatabaseFileExtensionStall.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<>          cBKReadPages;
LONG LBKReadPagesCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cBKReadPages.PassTo( iInstance, pvBuf );
    return 0;
}

#endif  //  PERFMON_SUPPORT

//================================================
VOID ConvertFileTimeToLogTime( __int64 ft, LOGTIME* plt )
{
    if ( 0 == ft )
    {
        memset( plt, 0, sizeof(LOGTIME) );
    }
    else
    {
        DATETIME dt;
        UtilConvertFileTimeToDateTime( ft, &dt );

        Assert( 1900 <= dt.year );
        plt->bYear = ( BYTE )( dt.year - 1900 );
        plt->bMonth = ( BYTE )dt.month;
        plt->bDay = ( BYTE )dt.day;
        plt->bHours = ( BYTE )dt.hour;
        plt->bMinutes = ( BYTE )dt.minute;
        plt->bSeconds = ( BYTE )dt.second;
        plt->SetMilliseconds( dt.millisecond );
        Assert( plt->Milliseconds() == dt.millisecond );
        plt->fTimeIsUTC = fTrue;
    }
}

bool FLogtimeIsNull( const LOGTIME * const plt )
{
    return (
        0 == plt->bYear
        && 0 == plt->bMonth
        && 0 == plt->bDay
        && 0 == plt->bHours
        && 0 == plt->bMinutes
        && 0 == plt->bSeconds
        && 0 == plt->Milliseconds() );
}

bool FLogtimeIsValid( const LOGTIME * const plt )
{
    return (
        ( plt->fTimeIsUTC || FLogtimeIsNull( plt ) ) &&
        ( 0 < plt->bMonth && plt->bMonth <= 12 ) &&
        ( 0 < plt->bDay && plt->bDay <= 31 ) &&
        ( 0 <= plt->bHours && plt->bHours < 24 ) &&
        ( 0 <= plt->bMinutes && plt->bMinutes < 60 ) &&
        ( 0 <= plt->bSeconds && plt->bSeconds < 60 ) &&
        plt->Milliseconds() < 1000 );
}

__int64 ConvertLogTimeToFileTime( const LOGTIME* plt )
{
    if ( FLogtimeIsNull( plt ) )
    {
        return 0;
    }
    else
    {
        Assert( FLogtimeIsValid( plt ) );

        DATETIME dt;
        dt.year = plt->bYear + 1900;
        dt.month = plt->bMonth;
        dt.day = plt->bDay;
        dt.hour = plt->bHours;
        dt.minute = plt->bMinutes;
        dt.second = plt->bSeconds;
        dt.millisecond = plt->Milliseconds();

        return UtilConvertDateTimeToFileTime( &dt );
    }
}

CIOThreadInfoTable  g_iothreadinfotable( rankIOThreadInfoTable );

/******************************************************************/
/*              IO                                                */
/******************************************************************/

ERR ErrIOInit()
{
    ERR err = JET_errSuccess;

    //  init the IO thread info table

    if ( g_iothreadinfotable.ErrInit( 5, 1 ) != CIOThreadInfoTable::ERR::errSuccess )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

HandleError:
    return err;
}

VOID IOTerm()
{
    //  shutdown the IO thread info table

    g_iothreadinfotable.Term();
}

void FlightConcurrentMetedOps( INT cioOpsMax, INT cioLowThreshold, TICK dtickStarvation );

ERR ErrIOInit( INST *pinst )
{
    //  Set up ifmp fmp maps

    pinst->m_plog->SetCheckpointEnabled( fFalse );

    //  Note: This is kind of wonky, we're setting a global as part of inst init.  BUT for a flighted stuff
    //  I'm not going to worry about this for now ... if we have to make it a real param, solve this.
    FlightConcurrentMetedOps( (ULONG)UlParam( pinst, JET_paramFlight_ConcurrentMetedOps ),
                                (ULONG)UlParam( pinst, JET_paramFlight_LowMetedOpsThreshold ),
                                (ULONG)UlParam( pinst, JET_paramFlight_MetedOpStarvedThreshold ) );

    return JET_errSuccess;
}

ERR ErrIOTermGetShutDownMark( INST * const pinst, LOG * const plog, LGPOS& lgposShutDownMarkRec )
{
    ERR err = JET_errSuccess;
    
    //  If we are doing recovering and
    //  if it is in redo mode or
    //  if it is in undo mode but last record seen is shutdown mark, then no
    //  need to log and set shutdown mark again. Note the latter case (undo mode)
    //  is to prevent to log two shutdown marks in a row after recovery and
    //  invalidate the previous one which the attached database have used as
    //  the consistent point.

    if ( plog->FRecovering()
        && ( plog->FRecoveringMode() == fRecoveringRedo
            || ( plog->FRecoveringMode() == fRecoveringUndo && plog->FLastLRIsShutdown() ) ) )
    {
        lgposShutDownMarkRec = plog->LgposShutDownMark();
    }
    else
    {
        err = ErrLGShutDownMark( pinst, &lgposShutDownMarkRec );
    }
    return err;
}

LOCAL ERR ErrIOTermUpdateAndWriteDatabaseHeader(
    IFileSystemAPI * const pfsapi,
    FMP * const pfmp,
    const INST * const pinst,
    const LGPOS& lgposShutDownMarkRec )
{
    ERR err = JET_errSuccess;
    const LOG * const plog = pinst->m_plog;
    
    /*  Update database header.
     */

    { // .ctor acquires PdbfilehdrReadWrite
    PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();

    pdbfilehdr->le_dbtimeDirtied = pfmp->DbtimeLast();
    Assert( pdbfilehdr->le_dbtimeDirtied != 0 );
    pdbfilehdr->le_objidLast = pfmp->ObjidLast();
    Assert( pdbfilehdr->le_objidLast != 0 );

    if ( JET_dbstateDirtyAndPatchedShutdown != pdbfilehdr->Dbstate() )
    {
        pdbfilehdr->SetDbstate( JET_dbstateCleanShutdown );
    }
    else
    {
        //  For the JET_dbstateDirtyAndPatchedShutdown case we must maintain the same dbstate 
        //  and do not update the le_lGenMaxRequired because we are not done getting to a clean
        //  or consistent state yet.
    }

    if ( plog->FRecovering() )
    {
        const BKINFO * const pbkInfoToCopy = &(pdbfilehdr->bkinfoFullCur);

        if ( pbkInfoToCopy->le_genLow != 0 )
        {
            Assert( pbkInfoToCopy->le_genHigh != 0 );
            if ( pinst->m_pbackup->FBKLogsTruncated() )
            {
                pdbfilehdr->bkinfoFullPrev = (*pbkInfoToCopy);
            }
            memset( &pdbfilehdr->bkinfoFullCur, 0, sizeof( BKINFO ) );
            memset( &pdbfilehdr->bkinfoSnapshotCur, 0, sizeof( BKINFO ) );
            memset( &pdbfilehdr->bkinfoIncPrev, 0, sizeof( BKINFO ) );
        }

        // delete the previous backup info on any hard recovery
        // this will prevent a incremental backup and log truncation problems
        // UNDONE: the above logic to copy bkinfoFullPrev is probably not needed
        // (we may consider this and delete it)

        if ( pfmp->FHardRecovery( pdbfilehdr.get() ) )
        {
            memset( &pdbfilehdr->bkinfoFullPrev, 0, sizeof( BKINFO ) );
            memset( &pdbfilehdr->bkinfoIncPrev, 0, sizeof( BKINFO ) );
            memset( &pdbfilehdr->bkinfoCopyPrev, 0, sizeof( BKINFO ) );
            memset( &pdbfilehdr->bkinfoDiffPrev, 0, sizeof( BKINFO ) );
            pdbfilehdr->bkinfoTypeFullPrev = DBFILEHDR::backupNormal;
            pdbfilehdr->bkinfoTypeIncPrev = DBFILEHDR::backupNormal;
        }
    }

    Assert( !pfmp->FLogOn() || !plog->FLogDisabled() );
    if ( pfmp->FLogOn() )
    {
        Assert( 0 != CmpLgpos( &lgposShutDownMarkRec, &lgposMin ) );

        Assert( FSIGSignSet( &pdbfilehdr->signLog ) );
        pdbfilehdr->le_lgposConsistent = lgposShutDownMarkRec;
        pdbfilehdr->le_lgposDetach = lgposShutDownMarkRec;
        if ( pfmp->ErrDBFormatFeatureEnabled( pdbfilehdr.get(), JET_efvLgposLastResize ) == JET_errSuccess )
        {
            pdbfilehdr->le_lgposLastResize = pdbfilehdr->le_lgposConsistent;
        }
    }

    LGIGetDateTime( &pdbfilehdr->logtimeConsistent );

    pdbfilehdr->logtimeDetach = pdbfilehdr->logtimeConsistent;

    Assert( pdbfilehdr->le_objidLast );

    } // .dtor releases PdbfilehdrReadWrite

    // Note: This may ONLY be safe because we are threaded for this guy at this point.
    Call( ErrUtilWriteAttachedDatabaseHeaders( pinst, pfsapi, pfmp->WszDatabaseName(), pfmp, pfmp->Pfapi() ) );

HandleError:

    if ( err < JET_errSuccess )
    {
        (void)ErrIOFlushDatabaseFileBuffers( pfmp->Ifmp(), iofrDefensiveCloseFlush );
    }
    else
    {
        err = ErrIOFlushDatabaseFileBuffers( pfmp->Ifmp(), iofrFlushIfmpContext );
    }

    return err;
}

void IOTermFreePfapi( FMP * const pfmp )
{
    Assert( NULL != pfmp->Pdbfilehdr() );
    
    ULONG rgul[4] = { (ULONG)( pfmp - g_rgfmp), 0x42, 0, 0 };
    *((DWORD_PTR*)&(rgul[2])) = (DWORD_PTR)pfmp->Pfapi();
    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlPfapiDelete|sysosrtlContextFmp, pfmp, rgul, sizeof(rgul) );
    
    //  delete of pfapi Assert()s same, but this here to explicitly communicate that ESE
    //  should have all IOs flushed out by this time.
    Assert( pfmp->Pfapi()->CioNonFlushed() == 0 || ( pfmp->Pfapi()->Fmf() & IFileAPI::fmfTemporary ) || FRFSFailureDetected( OSFileFlush ) );

    delete pfmp->Pfapi();
    pfmp->SetPfapi( NULL );
}

void IOResetFMPFields( FMP * const pfmp, const LOG * const plog )
{
    Assert( FMP::FWriterFMPPool() );

    DBResetFMP( pfmp, plog );

    pfmp->ResetFlags();

    //  free the fmp entry

    if ( pfmp->WszDatabaseName() )
    {
        OSMemoryHeapFree( pfmp->WszDatabaseName() );
        pfmp->SetWszDatabaseName( NULL );
    }

    pfmp->Pinst()->m_mpdbidifmp[ pfmp->Dbid() ] = g_ifmpMax;
    pfmp->SetDbid( dbidMax );
    pfmp->SetPinst( NULL );
    pfmp->SetCPin( 0 );     // User may term without close the db
}

ERR ErrIOTermFMP( FMP *pfmp, LGPOS lgposShutDownMarkRec, BOOL fNormal )
{
    ERR err = JET_errSuccess;
    IFMP ifmp = pfmp->Ifmp();
    INST *pinst = pfmp->Pinst();
    LOG *plog = pinst->m_plog;
    OnDebug( const BOOL fNormalOrig = fNormal; )
    BOOL fPartialCreateDb = fFalse;

    // If we saw CreateDB and not CreateDBFinish, we need to clean up
    // outstanding latch
    if ( pfmp->FCreatingDB() && pfmp->CrefWriteLatch() == 1 )
    {
        Assert( pinst->FRecovering() );
    {
        PdbfilehdrReadOnly pdbfilehdr = pfmp->Pdbfilehdr();
        Assert( pdbfilehdr == NULL || pdbfilehdr->Dbstate() == JET_dbstateJustCreated );
    }
        pfmp->ReleaseWriteLatch( pfmp->PpibWriteLatch() );
        fNormal = fFalse;
        fPartialCreateDb = fTrue;
    }

    pfmp->ResetWaypoint();

    pfmp->RwlDetaching().EnterAsWriter();
    pfmp->SetDetachingDB( );

    // Either this is a temp database, or we're in recovery or DBM should be turned
    // off for it. The goal here is to make sure DBM is not possible of running for
    // a user database that is not recovering by this point.  Same for follower mode.
    Assert( pfmp->FIsTempDB() || pinst->FRecovering() || ( pfmp->FDontStartDBM() && !pfmp->FDBMScanOn() ) );
    Assert( pfmp->PdbmFollower() == NULL );

    Assert( NULL == pfmp->PkvpsMSysLocales() ); // should be gone by now

    if ( fNormal && plog->FRecovering() )
    {
        Assert( !pfmp->FReadOnlyAttach() );
        if ( pfmp->Patchchk() )
        {
            Assert( !pfmp->FIsTempDB() );
            pfmp->Patchchk()->lgposConsistent = lgposShutDownMarkRec;
        }
    }

    /*  free file handle and pdbfilehdr
     */
    Assert( pfmp->Pfapi()
            || NULL == pfmp->Pdbfilehdr()
            || !fNormal );      //  on error, may have dbfilehdr and no file handle
    if ( pfmp->Pfapi() )
    {
        Assert( !!pfmp->FReadOnlyAttach() == !!( pfmp->Pfapi()->Fmf() & IFileAPI::fmfReadOnly ) );
        Assert( ( pfmp->PFlushMap() == NULL ) || !pfmp->FIsTempDB() ); // temp DB flushmaps should not exist.
        
        if ( pfmp->FAttached() && !pfmp->FReadOnlyAttach() && !pfmp->FIsTempDB() )
        {
            //  close flush map.
            if ( pfmp->PFlushMap() )
            {
                if ( fNormal )
                {
                    const ERR errT = pfmp->PFlushMap()->ErrCleanFlushMap();
                    if ( errT < 0 )
                    {
                        fNormal = fFalse;
                    }
                    if ( err >= 0 )
                    {
                        err = errT;
                    }
                }
                else
                {
                    pfmp->PFlushMap()->TermFlushMap();
                }
            }

            //  This is very likely an unnecessary flush technically as there is only a DB
            //  header update missing, and we already flushed at end of ErrBFFlush, so it 
            //  probably isn't important to bring the DB header update with this one update
            //  before actually marking it clean altogether.
            //  Ensures that we can AssertTrack / and eventually Enforce below that we have 
            //  zero NonFlushed IO before we write a clean DB header below.
            const ERR errFfb = ErrIOFlushDatabaseFileBuffers( ifmp, iofrFlushIfmpContext );
            if ( errFfb < 0 )
            {
                fNormal = fFalse;
            }
            if ( err >= 0 )
            {
                err = errFfb;
            }

            Assert( pfmp->Pfapi()->CioNonFlushed() == 0 || !fNormal );

            if ( fNormal )
            {
                //  Should be zero, or should've been failing with fNormal == fFalse at this point.
                AssertTrack( pfmp->Pfapi()->CioNonFlushed() == 0, "UnexpectedPendingFlushesTermFmpPreDbHdrWrite" );
                
                ERR errT = ErrFaultInjection( 55588 );
                if ( errT >= 0 )
                {
                    errT = ErrIOTermUpdateAndWriteDatabaseHeader( pinst->m_pfsapi, pfmp, pinst, lgposShutDownMarkRec );
                }
                if ( errT < 0 )
                {
                    fNormal = fFalse;
                }
                if ( err >= 0 )
                {
                    err = errT;
                }
            }

        }

        if ( ( pfmp->Pfapi()->CioNonFlushed() > 0 ) && fNormal && !( pfmp->Pfapi()->Fmf() & IFileAPI::fmfTemporary ) )
        {
            //  Should be zero by now.  No point in Enforce()ing though, we've already screwed up
            //  if this hits because we've marked it clean.
            AssertTrack( fFalse, "UnexpectedPendingFlushesTermFmpPostDbHdrWrite" );
        }

        if ( !fNormal )
        {
            pfmp->Pfapi()->SetNoFlushNeeded();
        }
        IOTermFreePfapi( pfmp );
    }

    if ( pfmp->PFlushMap() != NULL )
    {
        pfmp->PFlushMap()->TermFlushMap();
    }

    if ( fNormal && err >= JET_errSuccess )
    {
        // snapshot the header info, retain the cache for this DB, in addition
        // to its redo maps, if any,
        pfmp->SnapshotHeaderSignature();
    }

    // If we saw CreateDB and not CreateDBFinish, we need to clean up
    // semi-created db before transitioning to do-state
    if ( pfmp->FCreatingDB() )
    {
        Assert( pinst->FRecovering() );
    {
        PdbfilehdrReadOnly pdbfilehdr = pfmp->Pdbfilehdr();
        Assert( pdbfilehdr == NULL || pdbfilehdr->Dbstate() == JET_dbstateJustCreated );
    }
        (VOID)ErrIODeleteDatabase( pinst->m_pfsapi, ifmp );
        pfmp->m_isdlCreate.TermSequence();
    }

    if ( pfmp->Pdbfilehdr() )
    {
        pfmp->FreePdbfilehdr();
    }

    // memory leak fix: if the backup was stoped (JetTerm with grbit JET_bitTermStopBackup)
    // the backup header may be still allocated
    if ( pfmp->Ppatchhdr() )
    {
        OSMemoryPageFree( pfmp->Ppatchhdr() );
        pfmp->SetPpatchhdr( NULL );
    }

    if ( !pfmp->DataHeaderSignature().FNull() )
    {
        BFResetBFFMPContextAttached( ifmp );
        pfmp->RwlDetaching().LeaveAsWriter();
    }
    else
    {
        pfmp->RwlDetaching().LeaveAsWriter();
        BFPurge( ifmp );
    }

    //  reset fmp fields
    FMP::EnterFMPPoolAsWriter();
    pfmp->RwlDetaching().EnterAsWriter();

    IOResetFMPFields( pfmp, plog );

    pfmp->RwlDetaching().LeaveAsWriter();
    FMP::LeaveFMPPoolAsWriter();

    Assert( !fNormalOrig || fNormal || err < JET_errSuccess || fPartialCreateDb );

    return err;
}

/*  go through FMP closing files.
/**/
ERR ErrIOTerm( INST *pinst, BOOL fNormal )
{
    ERR         err = JET_errSuccess;
    LGPOS       lgposShutDownMarkRec = lgposMin;
    LOG         * const plog = pinst->m_plog;
    OnDebug( const BOOL fNormalOrig = fNormal; )

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDatapoint|sysosrtlContextInst, pinst, (PVOID)&(pinst->m_iInstance), sizeof(pinst->m_iInstance) );

    //  Reset global variables.
    //
    Assert( pinst->m_ppibGlobal == ppibNil );

    /*  update checkpoint before fmp is cleaned if m_plog->m_fFMPLoaded is true.
     */
    err = plog->ErrLGUpdateCheckpointFile( fTrue );

    //  note: FIOCheckUserDbNonFlushedIos( pinst, 0 ) not true, occasionally 2 IOs outstanding due to DB header updates from 
    //  async log roll update of gen required.
    Assert( !fNormal || FIOCheckUserDbNonFlushedIos( pinst, cioAllowLogRollHeaderUpdates ) );

    //  There should be no attaching/detaching/creating going on
    Assert( err != JET_errDatabaseSharingViolation );

    if ( err < 0 && plog->FRecovering() )
    {
        //  disable log writing but clean fmps
        plog->SetNoMoreLogWrite( err );
    }

    /*  No more checkpoint update from now on. Now I can safely clean up the
     *  g_rgfmp.
     */
    plog->SetCheckpointEnabled( fFalse );

    /*  Set proper shut down mark.
     */
    if ( fNormal && !plog->FLogDisabled() )
    {
        const ERR errT = ErrIOTermGetShutDownMark( pinst, plog, lgposShutDownMarkRec );
        if ( errT < 0 )
        {
            fNormal = fFalse;
        }
        if ( err >= 0 )
        {
            err = errT;
        }
    }

    if ( pinst->m_isdlTerm.FActiveSequence() )
    {
        pinst->m_isdlTerm.FixedData().sTermData.lgposTerm = lgposShutDownMarkRec;
    }
    else
    {
        Expected( pinst->m_isdlInit.FActiveSequence() );
    }

    for ( DBID dbid = dbidMin; dbid < dbidMax; dbid++ )
    {
        //  maintain the attach checker.
        const IFMP ifmp = pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;
        
        if ( FFMPIsTempDB( ifmp ) ) 
        {
            // We're closing the temp DB, so reset the flag saying we opened it.
            pinst->m_fTempDBCreated = false;
        }
        
        FMP * const pfmp = &g_rgfmp[ ifmp ];

        const ERR errT = ErrIOTermFMP( pfmp, lgposShutDownMarkRec, fNormal );
        if ( errT < 0 )
        {
            fNormal = fFalse;
        }
        if ( err >= 0 )
        {
            err = errT;
        }
    }

    Assert( !fNormalOrig || fNormal || err < JET_errSuccess );

    return err;
}


LOCAL ERR ErrIODispatchAsyncExtension( const IFMP ifmp )
{
    ERR                     err;
    INST *                  pinst           = PinstFromIfmp( ifmp );
    DBEXTENDTASK * const    ptaskDbExtend   = new DBEXTENDTASK( ifmp );

    if( NULL == ptaskDbExtend )
    {
        //  release semaphore and err out
        //
        err = ErrERRCheck( JET_errOutOfMemory );
    }

    else if ( pinst->m_pver->m_fSyncronousTasks || g_rgfmp[ifmp].FDetachingDB() )
    {
        //  no longer dispatching tasks
        //
        delete ptaskDbExtend;
        err = JET_errSuccess;
    }

    else
    {
        err = pinst->Taskmgr().ErrTMPost( TASK::DispatchGP, ptaskDbExtend );
        if( err < JET_errSuccess )
        {
            //  the task was not enqueued successfully.
            //
            delete ptaskDbExtend;
        }
    }

    return err;
}

ERR ErrIOResizeUpdateDbHdrCount( const IFMP ifmp, const BOOL fExtend )
{
    ERR err     = JET_errSuccess;
    FMP *pfmp   = &g_rgfmp[ifmp];

    Assert( pfmp->Pdbfilehdr() );

    if ( NULL != pfmp->Pdbfilehdr() )   // for insurance
    {
        if ( fExtend )
        {
            PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();
            pdbfilehdr->le_ulExtendCount++;
            LGIGetDateTime( &pdbfilehdr->logtimeLastExtend );
        }
        else
        {
            PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();
            pdbfilehdr->le_ulShrinkCount++;
            LGIGetDateTime( &pdbfilehdr->logtimeLastShrink );
        }

        Call( ErrUtilWriteAttachedDatabaseHeaders( PinstFromIfmp( ifmp ), PinstFromIfmp( ifmp )->m_pfsapi, pfmp->WszDatabaseName(), pfmp, pfmp->Pfapi() ) );
    }

HandleError:
    return err;
}

ERR ErrIOResizeUpdateDbHdrLgposLast( const IFMP ifmp, const LGPOS& lgposLastResize )
{
    ERR err     = JET_errSuccess;
    FMP *pfmp   = &g_rgfmp[ifmp];

    Assert( pfmp->Pdbfilehdr() );
    Assert( CmpLgpos( lgposLastResize, lgposMin ) != 0 );
    Assert( CmpLgpos( lgposLastResize, lgposMax ) != 0 );

    if ( NULL != pfmp->Pdbfilehdr() )   // for insurance
    {
        {
        PdbfilehdrReadOnly pdbfilehdr = pfmp->Pdbfilehdr();
        Assert( CmpLgpos( pdbfilehdr->le_lgposAttach, lgposMin ) != 0 );
        Assert( CmpLgpos( pdbfilehdr->le_lgposLastResize, lgposMin ) == 0 ||
                CmpLgpos( pdbfilehdr->le_lgposLastResize, pdbfilehdr->le_lgposAttach ) >= 0 );
        }

        const INT icmpLgposLastVsCurrent = CmpLgpos( lgposLastResize, pfmp->Pdbfilehdr()->le_lgposLastResize );
        if ( icmpLgposLastVsCurrent > 0 )
        {
            pfmp->PdbfilehdrUpdateable()->le_lgposLastResize = lgposLastResize;
            Call( ErrUtilWriteAttachedDatabaseHeaders( PinstFromIfmp( ifmp ), PinstFromIfmp( ifmp )->m_pfsapi, pfmp->WszDatabaseName(), pfmp, pfmp->Pfapi() ) );

            // this is for insurance only because this comes after a database resize and the resize
            // itself will do an FFB.
            Call( ErrIOFlushDatabaseFileBuffers( ifmp, iofrDbHdrUpdLgposLastResize ) );
        }
        else if ( icmpLgposLastVsCurrent < 0 )
        {
            Assert( !pfmp->FShrinkDatabaseEofOnAttach() &&
                    ( PinstFromIfmp( ifmp )->m_plog->FRecoveringMode() == fRecoveringRedo ) );

        }
    }

HandleError:
    return err;
}

ERR ErrIONewSize(
    const IFMP ifmp,
    const TraceContext& tc,
    const CPG cpgNewSize,
    const CPG cpgAsyncExtension,
    const JET_GRBIT grbit )
{
    ERR         err             = JET_errSuccess;
    FMP *       pfmp            = g_rgfmp + ifmp;
    INST *      pinst           = PinstFromIfmp( ifmp );
    TraceContextScope tcScope;
    tcScope->SetDwEngineObjid( objidSystemRoot );
    if ( tcScope->nParentObjectClass == pocNone )
    {
        tcScope->nParentObjectClass = tceNone;
    }

    Assert( cpgAsyncExtension >= 0 );
    Assert( ( grbit == JET_bitNil ) || ( grbit == JET_bitResizeDatabaseOnlyGrow ) || ( grbit == JET_bitResizeDatabaseOnlyShrink ) );
    const BOOL fGrowOnly = ( grbit == JET_bitResizeDatabaseOnlyGrow );
    const BOOL fShrinkOnly = ( grbit == JET_bitResizeDatabaseOnlyShrink );
    Assert( !fShrinkOnly || ( cpgAsyncExtension == 0 ) );

    const TICK tickStartWait = TickOSTimeCurrent();
    const QWORD cbNewSize = CbFileSizeOfCpg( cpgNewSize );
    QWORD cbFsFileSizeAsyncTargetNew = cbNewSize + QWORD( cpgAsyncExtension ) * g_cbPage;

    OSTraceFMP(
        ifmp,
        JET_tracetagSpaceManagement,
        OSFormat(
            "Request to change the file size=['%ws':0x%x] to %I64u bytes (and an additional %I64u bytes asynchronously) started.",
            pfmp->WszDatabaseName(),
            ifmp,
            cbNewSize,
            pfmp->CbOfCpg( cpgAsyncExtension ) ) );
    
    pfmp->AcquireIOSizeChangeLatch();
    QWORD cbFsFileSize = 0;
    err = pfmp->Pfapi()->ErrSize( &cbFsFileSize, IFileAPI::filesizeLogical );
    if ( err >= JET_errSuccess )
    {
        if ( cbNewSize < cbFsFileSize ) // shrinking the database
        {
            // Only really truncate the file if the caller allows it.
            if ( !fGrowOnly )
            {
                const QWORD cbTruncationChunk = pfmp->CbOfCpg( CpgBFGetOptimalLockPageRangeSizeForExternalZeroing( ifmp ) );
                QWORD cbNewSizePartial = cbFsFileSize;
                while ( cbNewSizePartial > cbNewSize )
                {
                    Expected( ( cbNewSizePartial % g_cbPage ) == 0 );
                    cbNewSizePartial = rounddn( cbNewSizePartial, g_cbPage );  // just in case
                    cbNewSizePartial -= min( cbTruncationChunk, cbNewSizePartial );
                    cbNewSizePartial = max( cbNewSizePartial, cbNewSize );

                    Expected( cpgAsyncExtension == 0 );
                    cbFsFileSizeAsyncTargetNew = cbNewSizePartial; // just in case.

                    DWORD_PTR dwRangeLockContext = NULL;

                    //  synchronously shrink db
                    //
                    tcScope->iorReason.SetIorp( iorpDatabaseShrink );

                    if ( !pfmp->FIsTempDB() )
                    {
                        const PGNO pgnoFirst = PgnoOfOffset( cbNewSizePartial );
                        AssertTrack( pgnoFirst > 0, "IONewSizeTooSmall" );
                        Expected( pgnoFirst > 16 );

                        if ( pgnoFirst > 0 )
                        {
                            const CPG cpgToPurge = PgnoOfOffset( cbFsFileSize - 1 ) - pgnoFirst + 1;
                            Assert( cpgToPurge > 0 );
                            err = ErrBFLockPageRangeForExternalZeroing( ifmp, pgnoFirst, cpgToPurge, fFalse, *tcScope, &dwRangeLockContext );
                            if ( err >= JET_errSuccess )
                            {
                                BFPurgeLockedPageRangeForExternalZeroing( dwRangeLockContext, *tcScope );
                            }
                        }
                    }  // if ( !pfmp->FIsTempDB() )

                    if ( err >= JET_errSuccess )
                    {
                        // Even with shrink disabled, logical file-size can go down (because it starts out too high at beginning of recovery).
                        err = pfmp->Pfapi()->ErrSetSize( *tcScope, cbNewSizePartial, fFalse, QosSyncDefault( pinst ) );
                        Expected( err <= JET_errSuccess ); // warnings not expected

                        Assert( !BoolParam( JET_paramEnableViewCache ) || ( err >= JET_errSuccess ) );

                        //  Consolidate physical size target if we succeeded.
                        if ( err >= JET_errSuccess )
                        {
                            cbFsFileSize = cbNewSizePartial;
                        }
                    }  // if ( err >= JET_errSuccess )
                    BFUnlockPageRangeForExternalZeroing( dwRangeLockContext, *tcScope );
                }  // while ( cbNewSizePartial > cbNewSize )
                Assert( cbNewSizePartial == cbNewSize );

                const TICK dtickWait = TickOSTimeCurrent() - tickStartWait;
                OSTraceFMP(
                    ifmp,
                    JET_tracetagSpaceManagement,
                    OSFormat(
                        "Request to shrink the file size=['%ws':0x%x] completed in %u msec with error %d (0x%x).",
                        pfmp->WszDatabaseName(),
                        ifmp,
                        dtickWait,
                        err,
                        err ) );
            }
        }
        else if ( cbNewSize > cbFsFileSize ) // extending the database
        {
            if ( !fShrinkOnly )
            {
                if ( !pfmp->FIsTempDB() )
                {
                    err = pfmp->PFlushMap()->ErrSetFlushMapCapacity( cpgNewSize );
                }

                if ( err >= JET_errSuccess )
                {
                    //  must synchronously extend db
                    //
                    PERFOpt( cIODatabaseFileExtensionStall.Inc( pinst ) );

                    tcScope->iorReason.SetIorp( iorpDatabaseExtension );
                    tcScope->SetDwEngineObjid( objidSystemRoot );

                    err = pfmp->Pfapi()->ErrSetSize( *tcScope, cbNewSize, fTrue, QosSyncDefault( pinst ) );
                    Expected( err <= JET_errSuccess ); // warnings not expected
                    if ( err >= JET_errSuccess )
                    {
                        cbFsFileSize = cbNewSize;
                    }
                }

                const TICK dtickWait = TickOSTimeCurrent() - tickStartWait;

                OSTraceFMP(
                    ifmp,
                    JET_tracetagSpaceManagement,
                    OSFormat(
                        "Request to extend the file size=['%ws':0x%x] completed in %u msec with error %d (0x%x).",
                        pfmp->WszDatabaseName(),
                        ifmp,
                        dtickWait,
                        err,
                        err ) );
            }
        }

        // Kick-off async extension if applicable
        if ( err >= JET_errSuccess )
        {
            const QWORD cbFsFileSizeAsyncTargetOld = pfmp->CbFsFileSizeAsyncTarget();
            pfmp->SetFsFileSizeAsyncTarget( max( cbFsFileSizeAsyncTargetNew, cbFsFileSize ) );

            if ( !fShrinkOnly &&
                    ( cbFsFileSizeAsyncTargetNew > cbFsFileSize ) &&
                    ( cbFsFileSizeAsyncTargetNew > cbFsFileSizeAsyncTargetOld ) )
            {
                if ( ErrIODispatchAsyncExtension( ifmp ) < JET_errSuccess )
                {
                    pfmp->SetFsFileSizeAsyncTarget( cbFsFileSize );
                }
            }
        }
        else
        {
            pfmp->SetFsFileSizeAsyncTarget( cbFsFileSize );
        }
    }

    pfmp->ReleaseIOSizeChangeLatch();

    return err;
}

ERR ErrIOIGetJsaPathFromDbPath(
    WCHAR* const wszJsaPath,
    const FMP* const pfmp,
    const DATETIME& dt,
    const PGNO pgnoStart,
    const PGNO pgnoEnd )
{
    ERR err = JET_errSuccess;
    IFileSystemAPI* pfsapi = pfmp->Pinst()->m_pfsapi;
    WCHAR wszJsaFileName[ IFileSystemAPI::cchPathMax ] = { L"\0" };
    WCHAR wszDbFolder[ IFileSystemAPI::cchPathMax ] = { L"\0" };
    WCHAR wszDbFileName[ IFileSystemAPI::cchPathMax ] = { L"\0" };
    WCHAR wszDbExtension[ IFileSystemAPI::cchPathMax ] = { L"\0" };
    DWORD cchDbPath = 0;
    DWORD cchDbExtension = 0;
    DWORD cchJsaExtension = 0;

    Call( pfsapi->ErrPathParse( pfmp->WszDatabaseName(), wszDbFolder, wszDbFileName, wszDbExtension ) );

    // ErrPathBuild throws a CRT exception if we pass in invalid parameters. Avoid calling that function
    // if we know we won't have enough capacity to hold the JSA path.
    cchDbPath = (DWORD)wcslen( pfmp->WszDatabaseName() );
    cchDbExtension = (DWORD)wcslen( wszDbExtension );
    cchJsaExtension = (DWORD)wcslen( wszShrinkArchiveExt );
    if ( ( cchDbPath - cchDbExtension + cchDbExtension  // folder, name and extension
        + 3         // dashes
        + 14        // timestamp (8 date + 6 time)
        + ( 2 * 8 ) // 2 pgnos in hex format
        + 1         // terminator
        ) > IFileSystemAPI::cchPathMax )
    {
        Error( ErrERRCheck( JET_errInvalidPath ) );
    }
    else
    {
        // Format: <DbName>-YYYYMMDDHHmmss-<PgnoStart>-<PgnoEnd>.jsa
        OSStrCbFormatW(
            wszJsaFileName,
            IFileSystemAPI::cchPathMax * sizeof( WCHAR ),
            L"%ws-%04u%02u%02u%02u%02u%02u-%08x-%08x",
            wszDbFileName,                  // DB name
            dt.year, dt.month, dt.day,      // date
            dt.hour, dt.minute, dt.second,  // time
            pgnoStart, pgnoEnd );           // pgnos
        Call( pfsapi->ErrPathBuild( wszDbFolder, wszJsaFileName, wszShrinkArchiveExt, wszJsaPath ) );
    }

HandleError:
    return err;
}

ERR ErrIOIGetJsaWildcardPathFromDbPath( WCHAR* const wszJsaWildcardPath, const FMP* const pfmp )
{
    ERR err = JET_errSuccess;
    IFileSystemAPI* pfsapi = pfmp->Pinst()->m_pfsapi;
    WCHAR wszJsaWildcardFileName[ IFileSystemAPI::cchPathMax ] = { L"\0" };
    WCHAR wszDbFolder[ IFileSystemAPI::cchPathMax ] = { L"\0" };
    WCHAR wszDbFileName[ IFileSystemAPI::cchPathMax ] = { L"\0" };
    WCHAR wszDbExtension[ IFileSystemAPI::cchPathMax ] = { L"\0" };
    DWORD cchDbPath = 0;
    DWORD cchDbExtension = 0;
    DWORD cchJsaExtension = 0;

    Call( pfsapi->ErrPathParse( pfmp->WszDatabaseName(), wszDbFolder, wszDbFileName, wszDbExtension ) );

    // ErrPathBuild throws a CRT exception if we pass in invalid parameters. Avoid calling that function
    // if we know we won't have enough capacity to hold the JSA path.
    cchDbPath = (DWORD)wcslen( pfmp->WszDatabaseName() );
    cchDbExtension = (DWORD)wcslen( wszDbExtension );
    cchJsaExtension = (DWORD)wcslen( wszShrinkArchiveExt );
    if ( ( cchDbPath - cchDbExtension + cchDbExtension  // folder, name and extension
        + 1         // dash
        + 1         // asterisk
        + 1         // terminator
        ) > IFileSystemAPI::cchPathMax )
    {
        Error( ErrERRCheck( JET_errInvalidPath ) );
    }
    else
    {
        // Format: <DbName>-*.jsa
        OSStrCbFormatW(
            wszJsaWildcardFileName,
            IFileSystemAPI::cchPathMax * sizeof( WCHAR ),
            L"%ws-*",
            wszDbFileName );
        Call( pfsapi->ErrPathBuild( wszDbFolder, wszJsaWildcardFileName, wszShrinkArchiveExt, wszJsaWildcardPath ) );
    }

HandleError:
    return err;
}

// This files will save off cthe data that is being truncated in separate files. The files will have the following
// naming convention: <DbName>-YYYYMMDDHHmmss-<PgnoStart>-<PgnoEnd>.jsa (jsa for JET Shrink Archive)
ERR ErrIOArchiveShrunkPages(
    const IFMP ifmp,
    const TraceContext& tc,
    const PGNO pgnoFirst,
    const CPG cpgArchive )
{
    ERR err = JET_errSuccess;
    IFileAPI* pfapi = NULL;

    Assert( pgnoFirst != pgnoNull );
    Assert( cpgArchive >= 0 );
    Expected( cpgArchive > 0 );

    const ULONG cbIoSize = UlFunctionalMin( (ULONG)UlParam( JET_paramMaxCoalesceReadSize ), (ULONG)UlParam( JET_paramMaxCoalesceWriteSize ) );
    const CPG cpgIoSize = LFunctionalMax( g_rgfmp[ ifmp ].CpgOfCb( cbIoSize ), 1 );
    BYTE* const pv = (BYTE*)PvOSMemoryPageAlloc( (DWORD)g_rgfmp[ ifmp ].CbOfCpg( cpgIoSize ), NULL );
    Alloc( pv );

    FMP* const pfmp = g_rgfmp + ifmp;
    INST* const pinst = pfmp->Pinst();
    DATETIME dt;
    UtilGetCurrentDateTime( &dt );

    // Save and shrink chunk by chunk.
    const CPG cpgNewSize = (CPG)( pgnoFirst - 1 );
    CPG cpgCurrentSize = cpgNewSize + cpgArchive;
    const CPG cpgChunkSize = (CPG)UlParam( pinst, JET_paramDbExtensionSize );
    while ( cpgCurrentSize > cpgNewSize )
    {
        const PGNO pgnoChunkEnd = cpgCurrentSize;
        cpgCurrentSize = LFunctionalMax( cpgCurrentSize - cpgChunkSize, cpgNewSize );
        const PGNO pgnoChunkStart = cpgCurrentSize + 1;
        Assert( pgnoChunkStart <= pgnoChunkEnd );

        // File name.
        WCHAR wszJsaFilePath[ IFileSystemAPI::cchPathMax ] = { L'\0' };
        Call( ErrIOIGetJsaPathFromDbPath( wszJsaFilePath, pfmp, dt, pgnoChunkStart, pgnoChunkEnd ) );

        // Create destination file.
        Call( CIOFilePerf::ErrFileCreate(
                    pinst->m_pfsapi,
                    pinst,
                    wszJsaFilePath,
                    IFileAPI::fmfStorageWriteBack | IFileAPI::fmfOverwriteExisting,
                    iofileOther,
                    ifmp,
                    &pfapi ) );

        // Read/write following I/O size constraints.
        PGNO pgnoIoStart = pgnoChunkStart;
        while ( pgnoIoStart <= pgnoChunkEnd )
        {
            const PGNO pgnoIoEnd = UlFunctionalMin( pgnoIoStart + cpgIoSize - 1, pgnoChunkEnd );

            // Read from source.
            const CPG cpgToRead = pgnoIoEnd - pgnoIoStart + 1;
            CPG cpgRead = 0;
            Call( pfmp->ErrDBReadPages( pgnoIoStart, pv, cpgToRead, &cpgRead, tc, fFalse ) );
            Assert( cpgRead == cpgToRead );
            Assert( cpgRead > 0 );

            // Write to destination.
            Call( pfapi->ErrIOWrite( tc,
                                    pfmp->CbOfCpg( pgnoIoStart - pgnoChunkStart ),
                                    (DWORD)pfmp->CbOfCpg( cpgRead ),
                                    pv,
                                    QosSyncDefault( pinst ) ) );

            pgnoIoStart = pgnoIoEnd + 1;
        }

        // Flush buffers and close destination file.
        Call( ErrUtilFlushFileBuffers( pfapi, iofrUtility ) );
        delete pfapi;
        pfapi = NULL;
    }

HandleError:
    OSMemoryPageFree( pv );

    if ( pfapi != NULL )
    {
        pfapi->SetNoFlushNeeded();
        delete pfapi;
        pfapi = NULL;
    }

    return err;
}

// Deletes all previously saved shrink archive files.
ERR ErrIODeleteShrinkArchiveFiles( const IFMP ifmp )
{
    ERR err = JET_errSuccess;
    FMP* const pfmp = g_rgfmp + ifmp;
    INST* const pinst = pfmp->Pinst();
    IFileSystemAPI* const pfsapi = pinst->m_pfsapi;
    IFileFindAPI* pffapi = NULL;
    WCHAR wszJsaWildcardPath[ IFileSystemAPI::cchPathMax ] = { L'\0' };

    Call( ErrIOIGetJsaWildcardPathFromDbPath( wszJsaWildcardPath, pfmp ) );

    // Iterate over all files.
    Call( pfsapi->ErrFileFind( wszJsaWildcardPath, &pffapi ) );
    while ( ( err = pffapi->ErrNext() ) == JET_errSuccess )
    {
        WCHAR wszJsaPath[ IFileSystemAPI::cchPathMax ];
        Call( pffapi->ErrPath( wszJsaPath ) );
        Call( pfsapi->ErrFileDelete( wszJsaPath ) );
    }

    err = ( err == JET_errFileNotFound ) ? JET_errSuccess : err;
    Call( err );

HandleError:
    delete pffapi;
    return err;
}

// Returns the rounded up (and down) values for the specified range.
// e.g. with page size of 4k, passing in page=18;cpg=3, then the output will
// be all zeroes.
ERR ErrIOTrimNormalizeOffsetsAndPgnos(
    _In_ const IFMP ifmp,
    _In_ const PGNO pgnoStartZeroes,
    _In_ const CPG cpgZeroLength,
    _Out_ QWORD* pibStartZeroes,
    _Out_ QWORD* pcbZeroLength,
    _Out_ PGNO* ppgnoStartZeroes,
    _Out_ CPG* pcpgZeroLength )
{
    ERR         err             = JET_errSuccess;
    FMP *       pfmp            = g_rgfmp + ifmp;

    Assert( cpgZeroLength > 0);

    *pibStartZeroes = 0;
    *pcbZeroLength = 0;

    const QWORD ibStartZeroesRaw = OffsetOfPgno( pgnoStartZeroes );
    const QWORD ibEndZeroesRaw = OffsetOfPgno( pgnoStartZeroes + cpgZeroLength );

    // NTFS only deals with sparse-ness at a 64k granularity.
    const QWORD ibStartZeroesAligned = roundup( ibStartZeroesRaw, cbSparseFileGranularity );
    const QWORD ibEndZeroesAligned = rounddn( ibEndZeroesRaw, cbSparseFileGranularity );
    const QWORD cbZeroesAligned = ibEndZeroesAligned - ibStartZeroesAligned;

    *pibStartZeroes = ibStartZeroesAligned;
    *ppgnoStartZeroes = PgnoOfOffset( ibStartZeroesAligned );
    *pcbZeroLength = cbZeroesAligned;
    *pcpgZeroLength = g_rgfmp[ ifmp ].CpgOfCb( cbZeroesAligned );

    Assert( ibEndZeroesAligned + cbSparseFileGranularity >= ibStartZeroesRaw );

    // If the page is in the middle of a 64k block, then we can't actually free it back to the OS.
    if ( ibEndZeroesAligned <= ibStartZeroesAligned )
    {
        OSTraceFMP(
            ifmp,
            JET_tracetagSpaceManagement,
            OSFormat(
                "Request to trim the file size=['%ws':0x%x] was less than the permissible granularity (%#x). Requested: (start = %I64u b (%I64u kb, pgno %u) bytes; end offset = %I64u (%I64u kb, cpg=%d)). Aligned: (start=%I64u, end=%I64u)",
                pfmp->WszDatabaseName(),
                ifmp,
                cbSparseFileGranularity,
                ibStartZeroesRaw, ibStartZeroesRaw / 1024,
                pgnoStartZeroes,
                ibEndZeroesRaw, ibEndZeroesRaw / 1024,
                cpgZeroLength,
                ibStartZeroesAligned,
                ibEndZeroesAligned ) );

        *pibStartZeroes = 0;
        *ppgnoStartZeroes = 0;
        *pcbZeroLength = 0;
        *pcpgZeroLength = 0;
        err = JET_errSuccess;
        goto HandleError;
    }

    Assert( ibStartZeroesAligned < ibEndZeroesAligned );
    Assert( *pcbZeroLength > 0 );
    Assert( *pcpgZeroLength > 0 );

HandleError:
    AssertSz( ( *pibStartZeroes > 0 ) == ( *pcbZeroLength > 0 ), "Either both of ( *pcbZeroLength, *pibStartZeroes ) should be zero, or both non-zero." );

    return err;
}


ERR ErrIOTrim(
    _In_ const IFMP ifmp,
    _In_ const QWORD ibStartZeroesAligned,
    _In_ const QWORD cbZeroLength )
{
    ERR         err                 = JET_errSuccess;
    FMP *       pfmp                = g_rgfmp + ifmp;
    DWORD_PTR   dwRangeLockContext  = NULL;
    TraceContextScope tcScope( iorpDatabaseTrim );
    if ( tcScope->nParentObjectClass == pocNone )
    {
        tcScope->nParentObjectClass = tceNone;
    }

    Assert( cbZeroLength > 0);
    Assert( 0 == ibStartZeroesAligned % cbSparseFileGranularity );
    Assert( 0 == cbZeroLength % cbSparseFileGranularity );

    const TICK  tickStartWait   = TickOSTimeCurrent();

    if ( cbZeroLength > 0 )
    {
        if ( pfmp->FTrimSupported() )
        {
            const PGNO pgnoFirst = PgnoOfOffset( ibStartZeroesAligned );
            const CPG cpg = g_rgfmp[ ifmp ].CpgOfCb( cbZeroLength );
            
            OSTraceFMP(
               ifmp,
               JET_tracetagSpaceManagement,
               OSFormat(
                  "Request to trim the file size=['%ws':0x%x] by setting %I64u (%I64u kb, cpg %d) to zero, starting at offset %I64u (%I64u kb, pgno %u) bytes.",
                  pfmp->WszDatabaseName(),
                  ifmp,
                  cbZeroLength, cbZeroLength / 1024,
                  cpg,
                  ibStartZeroesAligned, ibStartZeroesAligned / 1024,
                  pgnoFirst ) );

            const CPG cpgSparseFileGranularity = pfmp->CpgOfCb( cbSparseFileGranularity );
            CPG cpgTrimOptimal = CpgBFGetOptimalLockPageRangeSizeForExternalZeroing( ifmp );
            cpgTrimOptimal = roundup( cpgTrimOptimal, cpgSparseFileGranularity );
            PGNO pgnoTrimRangeThis = pgnoFirst;
            const PGNO pgnoTrimRangeLast = pgnoFirst + cpg - 1;
            while ( pgnoTrimRangeThis <= pgnoTrimRangeLast )
            {
                const CPG cpgTrimRangeThis = LFunctionalMin( cpgTrimOptimal, (CPG)( pgnoTrimRangeLast - pgnoTrimRangeThis + 1 ) );
                Assert( cpgTrimRangeThis >= cpgSparseFileGranularity );
                Assert( ( cpgTrimRangeThis % cpgSparseFileGranularity ) == 0 );

                // Lock, purge and trim. It's OK if we purge and fail the trim because we're already discarding the pages anyways,
                // so it means they are useless and this point (i.e., emptied out of data and any useful state).
                dwRangeLockContext = NULL;
                Call( ErrBFLockPageRangeForExternalZeroing( ifmp, pgnoTrimRangeThis, cpgTrimRangeThis, fTrue, *tcScope, &dwRangeLockContext ) );
                BFPurgeLockedPageRangeForExternalZeroing( dwRangeLockContext, *tcScope );
                Call( pfmp->Pfapi()->ErrIOTrim( *tcScope, OffsetOfPgno( pgnoTrimRangeThis ), pfmp->CbOfCpg( cpgTrimRangeThis ) ) );

                BFUnlockPageRangeForExternalZeroing( dwRangeLockContext, *tcScope );
                dwRangeLockContext = NULL;

                pgnoTrimRangeThis += cpgTrimRangeThis;
            }

            const TICK dtickWait = TickOSTimeCurrent() - tickStartWait;

            OSTraceFMP(
               ifmp,
               JET_tracetagSpaceManagement,
               OSFormat(
                  "Request to trim the file size=['%ws':0x%x] completed in %u msec with error %d (0x%x).",
                  pfmp->WszDatabaseName(),
                  ifmp,
                  dtickWait,
                  err,
                  err ) );

            // Ideally we'd assert that the operation did something, but we may be trimming an
            // already-trimmed area, which means the file size won't actually be any different.
        }
        else
        {
            Error( ErrERRCheck( JET_errFeatureNotAvailable ) );
        }
    }
    else
    {
        err = ErrERRCheck( errCodeInconsistency );
    }

HandleError:
    BFUnlockPageRangeForExternalZeroing( dwRangeLockContext, *tcScope );
    return err;
}

/*
 *  triggers update notification of the checkpoint settings for each attached DB
 */
ERR ErrIOUpdateCheckpoints( INST * pinst )
{
    ERR err = JET_errSuccess;
    
    FMP::EnterFMPPoolAsReader();
    
    FMP *   pfmpCurr = NULL;
    
    if ( pinst && ( pinst->m_fJetInitialized || ( pinst->m_plog && pinst->m_plog->FRecovering() ) ) )
    {
        for ( DBID dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
        {
            const IFMP ifmp = pinst->m_mpdbidifmp[ dbidT ];
            if ( ifmp >= g_ifmpMax )
            {
                continue;
            }
        
            if ( g_rgfmp[ifmp].FReadOnlyAttach() )
            {
                continue;
            }
    
            pfmpCurr = &g_rgfmp[ifmp];
            pfmpCurr->RwlDetaching().EnterAsReader();
    
            if ( pfmpCurr->FLogOn() && pfmpCurr->FAttached() && pfmpCurr->Pdbfilehdr() != NULL )
            {
                const ERR errT = ErrBFConsumeSettings( bfcsCheckpoint, ifmp );
                if ( err >= JET_errSuccess )
                {
                    //  We track the first error, and continue to try to induce all the other
                    //  checkpoints to trigger.
                    err = errT;
                }
            }
    
            pfmpCurr->RwlDetaching().LeaveAsReader();
            pfmpCurr = NULL;
        }
    }
    FMP::LeaveFMPPoolAsReader();

    return err;
}

/*
 *  opens database file, returns JET_errSuccess if file is already open
 */
ERR ErrIOOpenDatabase(
    _In_ IFileSystemAPI *const pfsapi,
    _In_ IFMP ifmp,
    _In_ PCWSTR wszDatabaseName,
    _In_ const IOFILE iofile,
    _In_ const BOOL fSparseEnabledFile )
{
    FMP::AssertVALIDIFMP( ifmp );

    if ( FIODatabaseOpen( ifmp ) )
    {
        return JET_errSuccess;
    }

    ERR         err;
    IFileAPI*   pfapi;
    FMP*        pfmp        = &g_rgfmp[ ifmp ];
    BOOL        fReadOnly   = pfmp->FReadOnlyAttach();

    Assert( !!fReadOnly == !!( pfmp->FmfDbDefault() & IFileAPI::fmfReadOnly ) );    //  make sure the FMP's FileModeFlags are consistent
    Assert( !!( !fReadOnly && BoolParam( JET_paramEnableFileCache ) && !pfmp->FLogOn() ) == !!( pfmp->FmfDbDefault() & IFileAPI::fmfLossyWriteBack ) );

    //  Reset per DB / FMP IO stats

    IOResetFmpIoLatencyStats( pfmp->Ifmp() );


    if ( pfmp->FIsTempDB() )
    {
        Assert( !pfmp->FReadOnlyAttach() );
        Assert( !pfmp->FLogOn() );
#ifndef FORCE_STORAGE_WRITEBACK
        Expected( !( pfmp->FmfDbDefault() & IFileAPI::fmfStorageWriteBack ) );
#endif
        CallR( CIOFilePerf::ErrFileCreate(
                                pfsapi,
                                pfmp->Pinst(),
                                wszDatabaseName,
                                pfmp->FmfDbDefault() | IFileAPI::fmfOverwriteExisting | IFileAPI::fmfTemporary,
                                iofile,
                                ifmp,
                                &pfapi ) );
        pfmp->SetPfapi( pfapi );

        pfmp->SetOwnedFileSize( 0 );
        pfmp->AcquireIOSizeChangeLatch();
        pfmp->SetFsFileSizeAsyncTarget( 0 );
        pfmp->ReleaseIOSizeChangeLatch();
    }
    else
    {
        CPG cpg = 0;

        CallR( CIOFilePerf::ErrFileOpen(
                                pfsapi,
                                pfmp->Pinst(),
                                wszDatabaseName,
                                pfmp->FmfDbDefault(),
                                iofile,
                                ifmp,
                                &pfapi ) );
        pfmp->SetPfapi( pfapi );

        //  It may seem odd to flush file buffers, but if ESE was previously opened with
        //  write caching on, and then crashed in the middle, we can then read DB pages
        //  and thing higher page versions / DBTIMEs are actually persisted to disk when
        //  in fact they are only in the FS or Disk caches.  So by FFBs now, we ensure
        //  that anything we may read is persisted to disk already and therefore usable
        //  for updating / calculating the checkpoint or DB min required, etc.
        Call( ErrUtilFlushFileBuffers( pfapi, iofrOpeningFileFlush ) ); // essentially ErrIOFlushDatabaseFileBuffers before Pfapi() set.

        //  just opened the file, so the file size must be correctly buffered
        //
        QWORD cbSize;
        Call( pfapi->ErrSize( &cbSize, IFileAPI::filesizeLogical ) );
        cpg = (CPG)roundupdiv( cbSize, g_cbPage );
        cpg -= cpgDBReserved;

        //  adjust flush map capacity
        //
        Call( pfmp->PFlushMap()->ErrSetFlushMapCapacity( LFunctionalMax( cpg, 1 ) ) );

        //  set database size in FMP -- the non-true/non-FS value should NOT include the reserved pages
        //
        pfmp->AcquireIOSizeChangeLatch();
        pfmp->SetFsFileSizeAsyncTarget( cbSize );
        pfmp->ReleaseIOSizeChangeLatch();

        // We're setting the logical file size here with its physical size, which is innacurate, but we
        // need to initialize it with a reasonable value, otherwise, we'll hit asserts trying to latch
        // pages to get the logical size itself. This is expected to be fixed up soon during DB create
        // or attach.
        Assert( pfmp->FAttachingDB() || pfmp->FCreatingDB() || pfmp->Pinst()->m_plog->FRecovering() );
        pfmp->SetOwnedFileSize( cbSize );

        if ( !fReadOnly
             && fSparseEnabledFile
             && ( UlParam( pfmp->Pinst(), JET_paramWaypointLatency ) == 0 ) )
        {
            err = pfapi->ErrSetSparseness();

            OSTraceFMP(
               ifmp,
               JET_tracetagSpaceManagement,
               OSFormat(
                  "Request to set the file=['%ws':0x%x]'s sparse-ness attribute returned %d.",
                  pfmp->WszDatabaseName(),
                  ifmp,
                  err ) );

            if ( JET_errUnloadableOSFunctionality == err )
            {
                // Probably running on FAT. It's safe to ignore.
                err = JET_errSuccess;
            }
            else
            {
                Call( err );

                // Bit flags require grabbing a lock first.
                FMP::EnterFMPPoolAsWriter();
                pfmp->SetTrimSupported();
                FMP::LeaveFMPPoolAsWriter();
            }
        }
        else
        {
            OSTraceFMP(
               ifmp,
               JET_tracetagSpaceManagement,
               OSFormat(
                  "Not setting the file=['%ws':0x%x]'s sparse-ness attribute: Either it's readonly, JET_paramEnableShrinkDatabase is false, or LLR is on..",
                  pfmp->WszDatabaseName(),
                  ifmp ) );
        }
    }

    ULONG rgul[4] = { (ULONG)ifmp, 0x42, 0, 0 };
    *((DWORD_PTR*)&(rgul[2])) = (DWORD_PTR)pfapi;
    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlSetPfapi|sysosrtlContextFmp, pfmp, rgul, sizeof(rgul) );

    Assert( err >= JET_errSuccess );
    Assert( pfmp->Pfapi() != NULL );

    return err;

HandleError:
    Assert( err < JET_errSuccess );
    pfmp->SetPfapi( NULL );
    delete pfapi;

    return err;
}


VOID IOCloseDatabase( IFMP ifmp )
{
    FMP::AssertVALIDIFMP( ifmp );
    FMP *pfmp = &g_rgfmp[ ifmp ];

//  Assert( PinstFromIfmp( ifmp )->m_plog->FRecovering() || FDBIDWriteLatch(ifmp) == fTrue );
    Assert( pfmp->Pfapi() );

    //  flush the file before it is closed if this is a RW attach and the file
    //  wasn't opened with write back caching (which already flushes)

    if (    !pfmp->FReadOnlyAttach() &&
            !pfmp->FCreatingDB() &&
            !( !pfmp->FReadOnlyAttach() && ( pfmp->Pfapi()->Fmf() & IFileAPI::fmfCached ) && !pfmp->FLogOn() ) )
    {
        //  Don't care about error I guess
        (void)ErrIOFlushDatabaseFileBuffers( ifmp, iofrWriteBackFlushIfmpContext );
    }

    //  delete of pfapi Assert()s same, but this here to explicitly communicate that ESE
    //  should have all IOs flushed out by this time.
    Assert( pfmp->Pfapi()->CioNonFlushed() == 0 || FRFSFailureDetected( OSFileFlush ) );

    delete pfmp->Pfapi();
    pfmp->SetPfapi( NULL );

    IOResetFmpIoLatencyStats( pfmp->Ifmp() );
}


ERR ErrIODeleteDatabase( IFileSystemAPI *const pfsapi, IFMP ifmp )
{
    ERR err = JET_errSuccess;

    FMP::AssertVALIDIFMP( ifmp );
//  Assert( FDBIDWriteLatch(ifmp) == fTrue );

    const FMP* const pfmp = &g_rgfmp[ifmp];

    if ( !pfmp->FIsTempDB() )
    {
        WCHAR wszFmFilePath[IFileSystemAPI::cchPathMax] = { L'\0' };
        if ( CFlushMap::ErrGetFmPathFromDbPath( wszFmFilePath, pfmp->WszDatabaseName() ) >= JET_errSuccess )
        {
            (VOID)pfsapi->ErrFileDelete( wszFmFilePath );
        }
    }

    CallR( pfsapi->ErrFileDelete( pfmp->WszDatabaseName() ) );

    return JET_errSuccess;
}

/*
the function return an inside allocated array of structures.
It will be one structure for each runnign instance.
For each isstance it will exist information about instance name and attached databases

We compute in advence the needed memory for all the returned data: array, databases and names
so that everythink is alocated in one chunk and can be freed with JetFreeBuffer()

Note:   if there is a snapshot session specified - in which case this is
        an internal call - the function returns ONLY the instances (and dbs)
        which are part of the snapshot session.
*/

extern ULONG    g_cpinstInit;

ERR ISAMAPI ErrIsamGetInstanceInfo(
    ULONG *                 pcInstanceInfo,
    JET_INSTANCE_INFO_W **          paInstanceInfo,
    const CESESnapshotSession *     pSnapshotSession )
{
    Assert( pcInstanceInfo && paInstanceInfo);

    if ( NULL == pcInstanceInfo || NULL == paInstanceInfo )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    // protected by g_critInst
    CHAR*       pMemoryBuffer       = NULL;
    CHAR*       pCurrentPosArrays   = NULL;
    WCHAR*      pCurrentPosNames    = NULL;
    SIZE_T      cbCurrentPosNames   = 0;
    SIZE_T      cbMemoryBuffer      = 0;

    JET_ERR     err                 = JET_errSuccess;

    ULONG       cInstances          = 0;
    ULONG       cDatabasesTotal     = 0;
    SIZE_T      cbNamesSize         = 0;

    ULONG       ipinst              = 0;
    IFMP        ifmp                = g_ifmpMax;

    // validation/instrumentation that nothing changes while we are processing here
    C_ASSERT( dbidMax < CHAR_MAX );
#ifdef DEBUG
    INT         cmpinstcdb          = cMaxInstances;    //  makes next alloc 10k now
    unsigned char * mpinstcdb       = NULL;
    mpinstcdb = (unsigned char *)PvOSMemoryHeapAlloc( cmpinstcdb );
    //  bad style to change code flow on debug, so we're going to just insulate ourselves ...
    if ( mpinstcdb )
    {
        memset( mpinstcdb, 0, cmpinstcdb );
    }
#endif

    // for the snapshot case, we only need to worry about the FreezeInstance and
    // it is guaranteed not to vanish during this call
    
    if ( !pSnapshotSession )
    {
        // taking this prevents against instances coming and going
        INST::EnterCritInst();
    }

    // taking this protects against FMPs coming and going; and it
    // protects against FMPs changing relevant state (FInUse, FLogOn, FAttached)
    FMP::EnterFMPPoolAsWriter();

    if ( 0 == g_cpinstInit )
    {
        *pcInstanceInfo = 0;
        *paInstanceInfo = NULL;
        goto HandleError;
    }

    // we count the number of instances, of databases
    // and of characters to be used by all names
    for ( ipinst = 0; ipinst < g_cpinstMax; ipinst++ )
    {
        INST *  pinst = g_rgpinst[ ipinst ];
        if ( pinstNil == pinst )
            continue;

        // check if we are looking for a snapshot session only list
        // and if so, check if the instance is part of that snapshot
        //
        if ( pSnapshotSession && !pSnapshotSession->FFreezeInstance( pinst ) )
            continue;

        if ( NULL != pinst->m_wszInstanceName )
        {
            cbNamesSize += sizeof( WCHAR ) * ( LOSStrLengthW( pinst->m_wszInstanceName ) + 1 );
        }
        cInstances++;
    }
    Assert( cInstances == g_cpinstInit || pSnapshotSession );

    for ( ifmp = FMP::IfmpMinInUse(); ifmp <= FMP::IfmpMacInUse(); ifmp++ )
    {
        FMP *   pfmp = &g_rgfmp[ ifmp ];
        if ( !pfmp->FInUse() )
            continue;

        if ( !pfmp->FLogOn() || !pfmp->FAttached() || pfmp->FAttachingDB() )
            continue;

        if ( pSnapshotSession && !pSnapshotSession->FFreezeInstance( PinstFromIfmp(ifmp) ) )
            continue;

        Assert( FMP::FAllocatedFmp( ifmp ) );
        Assert( !pfmp->FSkippedAttach() );
        Assert( !pfmp->FDeferredAttach() );

        cbNamesSize += sizeof(WCHAR) * ( LOSStrLengthW(pfmp->WszDatabaseName()) + 1 );
        cDatabasesTotal++;

        const INST * pinst = PinstFromIfmp( ifmp );
        
        AssertPREFIX( pinst->m_iInstance >= 0 );
        AssertPREFIX( pinst->m_iInstance < cMaxInstances );

        OnDebug( if ( mpinstcdb ) mpinstcdb[pinst->m_iInstance] += 1 );
        Assert( mpinstcdb == NULL || mpinstcdb[pinst->m_iInstance] <= dbidMax );
    }

    // we allocate memory for the result in one chunck
    cbMemoryBuffer = 0;
    // memory for the array of structures
    cbMemoryBuffer += cInstances * sizeof(JET_INSTANCE_INFO_W);
    // memory for pointers to database names (file name, display name, slv file name)
    cbMemoryBuffer += 3 * cDatabasesTotal * sizeof(WCHAR *);
    // memory for all names (database names and instance names)
    cbMemoryBuffer += cbNamesSize;

    if ( 0 == cbMemoryBuffer )
    {
        Assert( *pcInstanceInfo == 0 );
        Assert( *paInstanceInfo == NULL );
        Assert( JET_errSuccess == err );
        goto HandleError;
    }

    // we have at least one instance to return
    //
    Assert( 0 != cInstances );

    // and we have something to allocate
    //
    Assert( cbMemoryBuffer );
    
    Alloc( pMemoryBuffer = (CHAR *)PvOSMemoryHeapAlloc( cbMemoryBuffer ) );

    memset( pMemoryBuffer, '\0', cbMemoryBuffer );

    *pcInstanceInfo = cInstances;
    *paInstanceInfo = (JET_INSTANCE_INFO_W *)pMemoryBuffer;

    pCurrentPosArrays = pMemoryBuffer + ( cInstances * sizeof(JET_INSTANCE_INFO_W) );
    Assert( pMemoryBuffer + cbMemoryBuffer >= pCurrentPosArrays );

    pCurrentPosNames = (WCHAR*)(pCurrentPosArrays + ( 3 * cDatabasesTotal * sizeof(WCHAR *) ));
    cbCurrentPosNames = cbMemoryBuffer - ( ((CHAR*)pCurrentPosNames) - pMemoryBuffer );

    ULONG cInstancesExpected = cInstances;

    for ( ipinst = 0, cInstances = 0; ipinst < g_cpinstMax; ipinst++ )
    {
        INST *              pinst               = g_rgpinst[ ipinst ];
        JET_INSTANCE_INFO_W *pInstInfo          = &(*paInstanceInfo)[cInstances];
        ULONG               cDatabasesCurrInst;
        DBID                dbid;

        if ( pinstNil == pinst )
            continue;

        if ( pSnapshotSession && !pSnapshotSession->FFreezeInstance( pinst ) )
            continue;

        cInstances++;

        //  because we own CritInst, no instances can come or go
        Assert( cInstances <= cInstancesExpected );

        //  capture the instance name
        pInstInfo->hInstanceId = (JET_INSTANCE) pinst;
        Assert( NULL == pInstInfo->szInstanceName );
        if ( NULL != pinst->m_wszInstanceName )
        {
            OSStrCbCopyW( pCurrentPosNames, cbCurrentPosNames, pinst->m_wszInstanceName );
            pInstInfo->szInstanceName = pCurrentPosNames;
            cbCurrentPosNames -= sizeof(WCHAR) * ( LOSStrLengthW( pCurrentPosNames ) + 1 );
            pCurrentPosNames += LOSStrLengthW( pCurrentPosNames ) + 1;
            Assert( pMemoryBuffer + cbMemoryBuffer >= (CHAR*)pCurrentPosNames );
        }

        //  iterate over databases
        cDatabasesCurrInst = 0;
        for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
        {
            if ( pinst->m_mpdbidifmp[ dbid ] >= g_ifmpMax )
                continue;

            const FMP * const   pfmp    = &g_rgfmp[pinst->m_mpdbidifmp[ dbid ] ];
            Assert( pfmp );
            Assert( pfmp->FInUse() );
            Assert( 0 < LOSStrLengthW( pfmp->WszDatabaseName() ) );

            if ( !pfmp->FLogOn() || !pfmp->FAttached() || pfmp->FAttachingDB() )
                continue;

            Assert( !pfmp->FSkippedAttach() );
            Assert( !pfmp->FDeferredAttach() );

            cDatabasesCurrInst++;
        }

        Assert( mpinstcdb == NULL || mpinstcdb[ pinst->m_iInstance ] == cDatabasesCurrInst );
        pInstInfo->cDatabases = cDatabasesCurrInst;

        Assert( NULL == pInstInfo->szDatabaseFileName);
        Assert( NULL == pInstInfo->szDatabaseDisplayName);
        Assert( NULL == pInstInfo->szDatabaseSLVFileName_Obsolete);

        if ( 0 != pInstInfo->cDatabases )
        {
            pInstInfo->szDatabaseFileName = (WCHAR **)pCurrentPosArrays;
            pCurrentPosArrays += pInstInfo->cDatabases * sizeof(WCHAR *);
            Assert( pMemoryBuffer + cbMemoryBuffer >= pCurrentPosArrays );

            pInstInfo->szDatabaseDisplayName = (WCHAR **)pCurrentPosArrays;
            pCurrentPosArrays += pInstInfo->cDatabases * sizeof(WCHAR *);
            Assert( pMemoryBuffer + cbMemoryBuffer >= pCurrentPosArrays );

            pInstInfo->szDatabaseSLVFileName_Obsolete = NULL;
            pCurrentPosArrays += pInstInfo->cDatabases * sizeof(WCHAR *);
            Assert( pMemoryBuffer + cbMemoryBuffer >= pCurrentPosArrays );
        }

        cDatabasesCurrInst = 0;
        for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
        {
            if ( pinst->m_mpdbidifmp[ dbid ] >= g_ifmpMax )
                continue;

            const FMP * const   pfmp    = &g_rgfmp[pinst->m_mpdbidifmp[ dbid ] ];
            Assert( pfmp );
            Assert( pfmp->FInUse() );
            Assert( 0 < LOSStrLengthW ( pfmp->WszDatabaseName() ) );

            if ( !pfmp->FLogOn() || !pfmp->FAttached() || pfmp->FAttachingDB() )
                continue;

            Assert( !pfmp->FSkippedAttach() );
            Assert( !pfmp->FDeferredAttach() );

            // because we own rwlFMPPool, the number of databases should not change
            Assert( mpinstcdb == NULL || mpinstcdb[ pinst->m_iInstance ] > cDatabasesCurrInst );

            Assert( NULL == pInstInfo->szDatabaseFileName[cDatabasesCurrInst] );
            OSStrCbCopyW( pCurrentPosNames, cbCurrentPosNames, pfmp->WszDatabaseName() );
            pInstInfo->szDatabaseFileName[cDatabasesCurrInst] = pCurrentPosNames;
            cbCurrentPosNames -= sizeof(WCHAR) * ( LOSStrLengthW( pCurrentPosNames ) + 1 );
            pCurrentPosNames += LOSStrLengthW( pCurrentPosNames ) + 1;
            Assert( pMemoryBuffer + cbMemoryBuffer >= (CHAR*)pCurrentPosNames );

            //  currently unused
            pInstInfo->szDatabaseDisplayName[cDatabasesCurrInst] = NULL;

            cDatabasesCurrInst++;
        }
        Assert( pInstInfo->cDatabases == cDatabasesCurrInst );

    }

    Assert( cInstances == *pcInstanceInfo );
    Assert( pMemoryBuffer
                + ( cInstances * sizeof(JET_INSTANCE_INFO_W) )
                + ( 3 * cDatabasesTotal * sizeof(WCHAR *) )
            == pCurrentPosArrays );
    Assert( pMemoryBuffer + cbMemoryBuffer == (CHAR*)pCurrentPosNames );

HandleError:

    FMP::LeaveFMPPoolAsWriter();
    if ( !pSnapshotSession )
    {
        INST::LeaveCritInst();
    }

    if ( err < JET_errSuccess )
    {
        *pcInstanceInfo = 0;
        *paInstanceInfo = NULL;

        if ( NULL != pMemoryBuffer )
        {
            OSMemoryHeapFree( pMemoryBuffer );
        }
    }

#ifdef DEBUG
    if ( NULL != mpinstcdb )
    {
        OSMemoryHeapFree( mpinstcdb );
    }
#endif

    return err;
}

//  ================================================================
void INST::WaitForDBAttachDetach( )
//  ================================================================
{
    BOOL fDetachAttach = fTrue;

    // we check this only during backup
    Assert ( m_pbackup->FBKBackupInProgress() );

    while ( fDetachAttach )
    {
        fDetachAttach = fFalse;
        for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
        {
            IFMP ifmp = m_mpdbidifmp[ dbid ];
            if ( ifmp >= g_ifmpMax )
                continue;

            if ( ( fDetachAttach = g_rgfmp[ifmp].CrefWriteLatch() != 0 ) != fFalse )
                break;
        }

        if ( fDetachAttach )
        {
            UtilSleep( cmsecWaitGeneric );
        }
    }
}

ULONG INST::CAttachedUserDBs() const
{
    ULONG cUserDBs = 0;
    for( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        if (m_mpdbidifmp[dbid] < g_ifmpMax)
        {
            cUserDBs++;
        }
    }
    return cUserDBs;
}

//  ================================================================
ERR INST::ErrReserveIOREQ()
//  ================================================================
{
    const ERR err = ErrOSDiskIOREQReserve();

    if ( err >= JET_errSuccess )
    {
        const LONG cIOReserved = AtomicIncrement( (LONG*)&m_cIOReserved );
        Assert( cIOReserved > 0 );

        //  prevents resource leakage
        Assert( cIOReserved <= (LONG)UlParam( JET_paramOutstandingIOMax ) );
    }

    return err;
}

//  ================================================================
VOID INST::UnreserveIOREQ()
//  ================================================================
{
    OSDiskIOREQUnreserve();
    const LONG cIOReserved = AtomicDecrement( (LONG*)&m_cIOReserved );
    Assert( cIOReserved >= 0 );
}

CCriticalSection CESESnapshotSession::g_critOSSnapshot( CLockBasicInfo( CSyncBasicInfo( "OSSnapshot" ), rankOSSnapshot, 0 ) );

JET_OSSNAPID        CESESnapshotSession::g_idSnapshot   = 0;
DWORD               CESESnapshotSession::g_cListEntries = 0;
const INST * const  CESESnapshotSession::g_pinstInvalid = (INST *)(~(DWORD_PTR(0x0)));


// UNDONE : comment
//
CESESnapshotSession::CESESnapshotSessionList g_ilSnapshotSessionEntry;

ERR CESESnapshotSession::ErrInit()
{

    Assert( g_critOSSnapshot.FNotOwner() );

    Assert( g_ilSnapshotSessionEntry.FEmpty() );
    Assert( CESESnapshotSession::g_cListEntries == 0 );

    return JET_errSuccess;
}

ERR ErrSNAPInit()
{
    return CESESnapshotSession::ErrInit();
}

void CESESnapshotSession::Term()
{

    Assert( g_critOSSnapshot.FNotOwner() );

    // Even if not 100% needed, it would be very good
    // proactive to have all the snapshot session closed
    // by the time the last Jet instance is closed.
    //
    AssertRTL( g_ilSnapshotSessionEntry.FEmpty() );
}

void SNAPTerm()
{
    CESESnapshotSession::Term();
}


ERR CESESnapshotSession::ErrAllocSession( CESESnapshotSession ** ppSession )
{
    ERR                     err     = JET_errSuccess;
    CESESnapshotSession *   pNew    = NULL;

    Assert( ppSession );
    Assert( CESESnapshotSession::g_critOSSnapshot.FOwner() );

    // g_idSnapshot protected by crit section above
    //
    Alloc( pNew = new CESESnapshotSession( ++g_idSnapshot ) );
    Assert( pNew );

    g_ilSnapshotSessionEntry.InsertAsNextMost( pNew );
    CESESnapshotSession::g_cListEntries++;

    *ppSession = pNew;

    // because there are no major downsides of leaking snapshot sessions
    // other then the memory used, we want to just have a warning
    // (if we leak an instance as part of a session is bad but it will 
    // be seen anyway)
    // Note: the check will leave a margin of 2 instances just because
    // testing can attempt to start more instances for boundary testing
    // and this would fire.
    // 
    // Note: INST::EnterCritInst() is in theory needed for g_cpinstInit
    // but it should be ak to get a snapshot here.
    //
    AssertSzRTL( CESESnapshotSession::g_cListEntries <= g_cpinstInit + 2, "Potential leak of snapshot sessions!" );
    
    Assert( err == JET_errSuccess );
    
HandleError:
    return err;
}

ERR CESESnapshotSession::ErrGetSessionByID( JET_OSSNAPID snapId, CESESnapshotSession ** ppSession )
{
    ERR                     err         = JET_errSuccess;
    CESESnapshotSession *   pCurrent    = NULL;

    Assert( ppSession );
    Assert( CESESnapshotSession::g_critOSSnapshot.FOwner() );

    pCurrent = g_ilSnapshotSessionEntry.PrevMost();
    while ( pCurrent )
    {
        if ( pCurrent->GetId() == snapId )
        {
            break;
        }
        pCurrent = g_ilSnapshotSessionEntry.Next( pCurrent );
    }

    if ( !pCurrent )
    {
        Error( ErrERRCheck( JET_errOSSnapshotInvalidSnapId ) );
    }
    
    *ppSession = pCurrent;

    Assert( err == JET_errSuccess );

HandleError:
    return err;
}

ERR CESESnapshotSession::RemoveSession( JET_OSSNAPID snapId )
{
    CESESnapshotSession *   pSession    = NULL;
    ERR                     err         = JET_errSuccess;

    Assert( CESESnapshotSession::g_critOSSnapshot.FOwner() );
    
    CallR( CESESnapshotSession::ErrGetSessionByID( snapId, &pSession ) );
    Assert( pSession );
    
    RemoveSession( pSession );

    return err;
}

void CESESnapshotSession::RemoveSession( CESESnapshotSession * pSession )
{
    Assert( CESESnapshotSession::g_critOSSnapshot.FOwner() );
    Assert( CESESnapshotSession::FMember( pSession ) );

    g_ilSnapshotSessionEntry.Remove( pSession );
    delete pSession;
    CESESnapshotSession::g_cListEntries--;
}

BOOL CESESnapshotSession::FMember( CESESnapshotSession * pSession )
{
    return g_ilSnapshotSessionEntry.FMember( pSession );
}

JET_OSSNAPID CESESnapshotSession::GetId()
{
    return m_idSnapshot;
}

void CESESnapshotSession::SetFreezeInstances()
{
    INST::EnterCritInst();
    for ( size_t ipinst = 0; ipinst < g_cpinstMax; ipinst++ )
    {
        if ( pinstNil != g_rgpinst[ ipinst ] &&
            ( NULL == g_rgpinst[ ipinst ]->m_pOSSnapshotSession ) &&
            m_fFreezeAllInstances &&
            g_rgpinst[ ipinst ]->m_fJetInitialized )
        {
            g_rgpinst[ ipinst ]->m_pOSSnapshotSession = this;

            //  ignoring error, as this is logged
            //  purely to facilitiate debugging
            //
            (void)ErrLGBackupBegin(
                        g_rgpinst[ ipinst ]->m_plog,
                        DBFILEHDR::backupOSSnapshot,
                        this->IsIncrementalSnapshot(),
                        NULL );
        }
    }
    INST::LeaveCritInst();
}

void CESESnapshotSession::SetInitialFreeze( const BOOL fFreezeAll )
{
    m_fFreezeAllInstances = fFreezeAll;
    SetFreezeInstances();
}


ERR CESESnapshotSession::ErrAddInstanceToFreeze( const INT ipinst )
{
    Assert( (size_t)ipinst < g_cpinstMax );
    Assert( pinstNil != g_rgpinst[ ipinst ] );
    Assert( g_rgpinst[ ipinst ]->m_fJetInitialized );

    if ( m_fFreezeAllInstances )
    {
        m_fFreezeAllInstances = fFalse;
        SetFreezeInstances();
    }

    INST::EnterCritInst();
    Assert( NULL == g_rgpinst[ ipinst ]->m_pOSSnapshotSession );
    g_rgpinst[ ipinst ]->m_pOSSnapshotSession = this;
    INST::LeaveCritInst();

    if ( !m_fFreezeAllInstances )
    {
        //  ignoring error, as this is logged
        //  purely to facilitiate debugging
        //
        (void)ErrLGBackupBegin(
                    g_rgpinst[ ipinst ]->m_plog,
                    DBFILEHDR::backupOSSnapshot,
                    this->IsIncrementalSnapshot(),
                    NULL );
    }

    return JET_errSuccess;
}

INST * CESESnapshotSession::GetFirstInstance()
{
    m_ipinstCurrent = 0;
    return GetNextNotNullInstance();
}

INST * CESESnapshotSession::GetNextInstance()
{
    m_ipinstCurrent++;
    return GetNextNotNullInstance();
}

INST * CESESnapshotSession::GetNextNotNullInstance()
{
    Assert ( INST::FOwnerCritInst() );

    while ( (ULONG)m_ipinstCurrent < g_cpinstMax )
    {
        if ( g_rgpinst[ m_ipinstCurrent ] && ( this == g_rgpinst[ m_ipinstCurrent ]->m_pOSSnapshotSession ) )
        {
            return g_rgpinst[ m_ipinstCurrent ];
        }
        m_ipinstCurrent++;
    }
    return NULL;
}

BOOL CESESnapshotSession::FFreezeInstance( const INST * pinst ) const
{
    Assert( pinst );
    return ( pinst->m_pOSSnapshotSession == this );
}

ERR CESESnapshotSession::ErrFreezeInstance()
{
    const INST *    pinstLGFlush            = g_pinstInvalid;
    const INST *    pinstCheckpoint         = g_pinstInvalid;
    const INST *    pinstAPI                = g_pinstInvalid;
    ERR     err                     = JET_errSuccess;

    INST::EnterCritInst();

    // all set, now stop log flushing
    // then stop checkpoint (including db headers update)
    for ( pinstLGFlush = GetFirstInstance(); NULL != pinstLGFlush; pinstLGFlush = GetNextInstance() )
    {
        Call( pinstLGFlush->m_pbackup->ErrBKOSSnapshotStopLogging( IsIncrementalSnapshot() ) );
    }
    Assert( NULL == pinstLGFlush );

    for ( pinstCheckpoint = GetFirstInstance(); NULL != pinstCheckpoint; pinstCheckpoint = GetNextInstance() )
    {
        // UNDONE: consider not entering m_critCheckpoint and
        // keeping it during the snapshot (this will prevent
        // db header and checkpoint update) but instead
        // enter() set m_fLogDisable leave()
        // and reset the same way on Thaw
        pinstCheckpoint->m_plog->LockCheckpoint();
        pinstCheckpoint->m_pbackup->BKInitForSnapshot(
                IsIncrementalSnapshot(),
                pinstCheckpoint->m_plog->LgposGetCheckpoint().le_lGeneration );
    }
    Assert( NULL == pinstCheckpoint );

    Assert ( JET_errSuccess == err );

    // we used to hold INST::LeaveCritInst(); during the 
    // freeze but now with parallel snapshots that would
    // have serialized the freeze period. We are switching 
    // to the per instance m_critBackupInProgress which
    // has also the advantage of allowing cleaner interaction
    // with a potential JetTerm with "stop backup"
    //
    for ( pinstAPI = GetFirstInstance(); NULL != pinstAPI; pinstAPI = GetNextInstance() )
    {
        pinstAPI->m_pbackup->BKLockBackup();
    }
    Assert( NULL == pinstAPI );

HandleError:

    INST::LeaveCritInst();

    if ( JET_errSuccess > err )
    {
        ThawInstance( pinstAPI, pinstCheckpoint, pinstLGFlush );
    }

    return err;
}

void CESESnapshotSession::ThawInstance( const INST * pinstLastAPI, const INST * pinstLastCheckpoint, const INST * pinstLastLGFlush )
{
    INST *  pinst           = NULL;

    //  UNDONE:  this next enter is in fact out of ranking order.  We currently
    //  own LOG::m_critBackupInProgress, LOG::m_critCheckpoint and LOG::m_critLGFlush,
    //  which are both lower than g_critInst. We are lucky that this doesn't deadlock!
    //
    CLockDeadlockDetectionInfo::NextOwnershipIsNotADeadlock();
    INST::EnterCritInst();

    if ( pinstLastAPI != g_pinstInvalid )
    {
        for ( pinst = GetFirstInstance(); NULL != pinst && pinstLastAPI != pinst; pinst = GetNextInstance() )
        {
            Assert ( pinst->m_plog );
            pinst->m_pbackup->BKUnlockBackup();
        }
    }

    if ( pinstLastCheckpoint != g_pinstInvalid )
    {
        for ( pinst = GetFirstInstance(); NULL != pinst && pinstLastCheckpoint != pinst; pinst = GetNextInstance() )
        {
            Assert ( pinst->m_plog );
            pinst->m_plog->UnlockCheckpoint();
        }
    }

    if ( pinstLastLGFlush != g_pinstInvalid )
    {
        for ( pinst = GetFirstInstance(); NULL != pinst && pinstLastLGFlush != pinst; pinst = GetNextInstance() )
        {
            Assert ( pinst->m_plog );
            pinst->m_pbackup->BKOSSnapshotResumeLogging();
        }
    }

    INST::LeaveCritInst();

}

extern BOOL g_fSystemInit;

ERR CESESnapshotSession::SetBackupInProgress()
{
    ERR     err     = JET_errSuccess;
    INST *  pinst   = NULL;


    const INT cbFillString = 20;
    WCHAR szSnap[cbFillString + 1];
    const WCHAR* rgszT[1] = { szSnap };

    OSStrCbFormatW( szSnap, sizeof( szSnap ), L"%lu", (ULONG)m_idSnapshot );

    INST::EnterCritInst();

    if ( !g_fSystemInit )
    {
        INST::LeaveCritInst();
        CallR( ErrERRCheck( JET_errNotInitialized ) );
    }

    for ( pinst = GetFirstInstance(); pinst; pinst = GetNextInstance() )
    {
        LOG * pLog = pinst->m_plog;
        BACKUP_CONTEXT * pbackup = pinst->m_pbackup;

        UtilReportEvent( eventInformation, OS_SNAPSHOT_BACKUP, OS_SNAPSHOT_FREEZE_START_ID, 1, rgszT, 0, NULL, pinst );

        pLog->LGLockWrite(); // protect the recovery flag
        pbackup->BKLockBackup();

        if ( pinst->m_pbackup->FBKBackupInProgress() || !pinst->m_fBackupAllowed )
        {
            pbackup->BKUnlockBackup();
            pLog->LGUnlockWrite();

            //  the call to ResetBackupInProgress() in the error-handler
            //  below is going to exclude this pinst from cleanup,
            //  so we must do it here
            //
            Assert( FFreezeInstance( pinst ) );
            pinst->m_pOSSnapshotSession = NULL;

            Error( ErrERRCheck( JET_errOSSnapshotNotAllowed ) );
        }

        pbackup->BKSetFlags( IsFullSnapshot() );

        // marked as during backup: will prevent attach/detach
        pbackup->BKUnlockBackup();

        // leave LGFlush to allow attach/detach in progress to complete
        pLog->LGUnlockWrite();

        pinst->WaitForDBAttachDetach();
    }
    Assert( NULL == pinst );

HandleError:

    INST::LeaveCritInst();

    if (JET_errSuccess > err)
    {
        Assert( pinst );
        ResetBackupInProgress( pinst );
    }

    return err;
}

void CESESnapshotSession::ResetBackupInProgress(const INST * pinstLastBackupInProgress )
{
    INST *  pinst   = NULL;

    INST::EnterCritInst();

    Assert( g_pinstInvalid != pinstLastBackupInProgress );

    for ( pinst = GetFirstInstance(); pinst && pinstLastBackupInProgress != pinst; pinst = GetNextInstance() )
    {
        LOG * pLog = pinst->m_plog;
        BACKUP_CONTEXT *pbackup = pinst->m_pbackup;

        pbackup->BKLockBackup();

        //  if we only got as far as SnapshotPrepare,
        //  the instance will not have been placed
        //  in the backup-in-progress state yet
        //  (at least, not by this session -- I
        //  believe it's possible for it to get
        //  placed in the backup-in-progress state
        //  by streaming backup)
        //
        if ( statePrepare != State() )
        {
            Assert( pinst->m_pbackup->FBKBackupInProgress( BACKUP_CONTEXT::backupLocalOnly ) );
        }

        //  It is possible for a freeze to fail before
        //  the session is marked as frozen. In that case
        //  we need to make sure all the flags are reset.
        pbackup->BKResetFlags();
        
        pbackup->BKUnlockBackup();

        //  this forces a new gen, to facilitate
        //  detecting a successful backup as
        //  soon as possible in surrogate backup
        //  scenarios
        //
        (void)ErrLGBackupStop(
                    pLog,
                    DBFILEHDR::backupOSSnapshot,
                    this->IsIncrementalSnapshot(),
                    BoolParam( pinst, JET_paramAggressiveLogRollover ) ? fLGCreateNewGen : 0,
                    NULL );

        Assert( FFreezeInstance( pinst ) );
        pinst->m_pOSSnapshotSession = NULL;
    }

    INST::LeaveCritInst();
}

ERR CESESnapshotSession::ErrFreezeDatabase()
{
    ERR     err         = JET_errSuccess;
    IFMP    ifmp        = 0;

    // no need to call FMP::EnterFMPPoolAsReader()
    // as the already have all instances set "during backup"
    // which will prevent new attach/detach and we have waited for
    // all attache/detach in progress

    for ( ifmp = FMP::IfmpMinInUse(); ifmp <= FMP::IfmpMacInUse(); ifmp++ )
    {
        FMP * pfmp = &g_rgfmp[ ifmp ];

        if ( !pfmp->FInUse() )
        {
            continue;
        }

        if ( pfmp->FIsTempDB() )
        {
            continue;
        }

        if ( !FFreezeInstance( PinstFromIfmp(ifmp) ) )
        {
            continue;
        }

        Assert ( pfmp->Dbid() >= dbidUserLeast );
        Assert ( pfmp->Dbid() < dbidMax );

        Assert ( PinstFromIfmp(ifmp)->m_pbackup->FBKBackupInProgress() );
        Assert ( pfmp->FAttached() );

        // prevent database size changes
        pfmp->AcquireIOSizeChangeLatch();
    }

    for ( ifmp = FMP::IfmpMinInUse(); ifmp <= FMP::IfmpMacInUse(); ifmp++ )
    {
        FMP * pfmp = &g_rgfmp[ ifmp ];

        if ( !pfmp->FInUse() )
        {
            continue;
        }

        if ( pfmp->FIsTempDB() )
        {
            continue;
        }

        if ( !FFreezeInstance( PinstFromIfmp(ifmp) ) )
        {
            continue;
        }

        Assert ( pfmp->Dbid() >= dbidUserLeast );
        Assert ( pfmp->Dbid() < dbidMax );

        Assert( pfmp->PgnoSnapBackupMost() == pgnoNull );
        PGNO pgnoLastFileSystem = pgnoNull;
        Call( pfmp->ErrPgnoLastFileSystem( &pgnoLastFileSystem ) );
        Assert( pgnoLastFileSystem != pgnoNull );
        pfmp->SetPgnoSnapBackupMost( pgnoLastFileSystem );

        err = pfmp->ErrRangeLock( 0, pfmp->PgnoSnapBackupMost() );
        if ( JET_errSuccess > err )
        {
            Call ( err );
            Assert ( fFalse );
        }
    }

    CallS( err );

HandleError:

    if ( JET_errSuccess > err )
    {
        ThawDatabase( ifmp );
    }

    return err;
}

void CESESnapshotSession::ThawDatabase( const IFMP ifmpLast )
{
    // no need to call FMP::EnterFMPPoolAsReader()
    // as the already have all instances set "during backup"
    // which will prevent new attach/detach and we have waited for
    // all attache/detach in progress

    const IFMP  ifmpRealLast    = ( ifmpLast < g_ifmpMax ? ifmpLast : FMP::IfmpMacInUse() + 1 );

    IFMP ifmp;
    for ( ifmp = FMP::IfmpMinInUse(); ifmp < ifmpRealLast; ifmp++ )
    {
        FMP * pfmp = &g_rgfmp[ ifmp ];

        if ( !pfmp->FInUse() )
        {
            continue;
        }

        if ( pfmp->FIsTempDB() )
        {
            continue;
        }

        if ( !FFreezeInstance( PinstFromIfmp(ifmp) ) )
        {
            continue;
        }

        Assert ( pfmp->Dbid() >= dbidUserLeast );
        Assert ( pfmp->Dbid() < dbidMax );

        Assert ( PinstFromIfmp(ifmp)->m_pbackup->FBKBackupInProgress() );
        Assert ( pfmp->FAttached() );

#ifdef DEBUG
        if ( pfmp->PgnoSnapBackupMost() != pgnoNull )
        {
            PGNO pgnoLastFileSystem = pgnoNull;
            if ( pfmp->ErrPgnoLastFileSystem( &pgnoLastFileSystem ) >= JET_errSuccess )
            {
                Assert( pgnoLastFileSystem != pgnoNull );
                Assert( pgnoLastFileSystem == pfmp->PgnoSnapBackupMost() );
            }
        }
#endif

        pfmp->RangeUnlock( 0, pfmp->PgnoSnapBackupMost() );
        pfmp->SetPgnoSnapBackupMost( pgnoNull );
    }

    for ( ifmp = FMP::IfmpMinInUse(); ifmp <= FMP::IfmpMacInUse(); ifmp++ )
    {
        FMP * pfmp = &g_rgfmp[ ifmp ];

        if ( !pfmp->FInUse() )
        {
            continue;
        }

        if ( pfmp->FIsTempDB() )
        {
            continue;
        }

        if ( !FFreezeInstance( PinstFromIfmp(ifmp) ) )
        {
            continue;
        }

        Assert ( pfmp->Dbid() >= dbidUserLeast );
        Assert ( pfmp->Dbid() < dbidMax );

        pfmp->ReleaseIOSizeChangeLatch();
    }

}

ERR CESESnapshotSession::ErrFreeze()
{
    ERR err = JET_errSuccess;

    CallR( CESESnapshotSession::SetBackupInProgress() );

    CallJ ( ErrFreezeInstance(), HandleFreezeError );

    Call ( ErrFreezeDatabase() );

    Assert ( JET_errSuccess == err );


HandleError:

    if ( JET_errSuccess > err )
    {
        ThawInstance();
    }

HandleFreezeError:

    if ( JET_errSuccess > err )
    {
        ResetBackupInProgress( NULL );
    }

    return err;
}

void CESESnapshotSession::Thaw( const BOOL fTimeOut )
{
    ThawDatabase();

    ThawInstance();

    // On successful Thaw, we leave "BackupInProgress" set
    // This will prevent attach/detach database which in turn
    // will allow database stamping with the new backup time
    if ( fTimeOut )
    {
        ResetBackupInProgress( NULL );
    }
}

void CESESnapshotSession::SnapshotThreadProc( const ULONG ulTimeOut )
{

    const INT cbFillString = 20;
    WCHAR szSnap[cbFillString + 1];
    WCHAR szError[12];
    const WCHAR* rgszT[2] = { szSnap, szError };

    OSStrCbFormatW( szSnap, sizeof( szSnap ), L"%lu", (ULONG)m_idSnapshot );

    UtilReportEvent( eventInformation, OS_SNAPSHOT_BACKUP, OS_SNAPSHOT_FREEZE_START_ID, 1, rgszT );

    m_errFreeze = ErrFreeze();

    if ( JET_errSuccess > m_errFreeze )
    {
        UtilThreadEnd( m_thread );
        m_thread = 0;

        // signal the starting thread that snapshot couldn't start
        m_asigSnapshotStarted.Set();

        OSStrCbFormatW( szError, sizeof(szError), L"%d", m_errFreeze );
        UtilReportEvent( eventError, OS_SNAPSHOT_BACKUP, OS_SNAPSHOT_FREEZE_START_ERROR_ID, 2, rgszT );
        return;
    }

    // signal the starting thread that snapshot did started
    // so that the Freeze API returns
    m_asigSnapshotStarted.Set();


    const BOOL fTimeOut = !m_asigSnapshotThread.FWait( ulTimeOut );
    SNAPSHOT_STATE newState = fTimeOut?stateTimeOut:stateThaw;

    Thaw( fTimeOut );

    SnapshotCritEnter();
    Assert ( stateFreeze == State() );
    Assert ( FCanSwitchTo( newState ) );
    SwitchTo ( newState );

    // on time-out, we need to close the thread handle
    if ( fTimeOut )
    {
#ifdef OS_SNAPSHOT_TRACE
    {
        const WCHAR* rgszT[2] = { L"thread SnapshotThreadProc", szError };
        OSStrCbFormatW( szError, sizeof( szError ), L"%d", JET_errOSSnapshotTimeOut );

        UtilReportEvent( eventError, OS_SNAPSHOT_BACKUP, OS_SNAPSHOT_TRACE_ID, 2, rgszT );
    }
#endif // OS_SNAPSHOT_TRACE
    }
    SnapshotCritLeave();

    if ( fTimeOut )
    {
        OSStrCbFormatW( szError, sizeof(szError), L"%lu", ulTimeOut );
        UtilReportEvent( eventError, OS_SNAPSHOT_BACKUP, OS_SNAPSHOT_TIME_OUT_ID, 2, rgszT );
    }
    else
    {
        UtilReportEvent( eventInformation, OS_SNAPSHOT_BACKUP, OS_SNAPSHOT_FREEZE_STOP_ID, 1, rgszT );
    }
}

DWORD DwSnapshotThreadProc( DWORD_PTR lpParameter )
{
    CESESnapshotSession * pSession = (CESESnapshotSession *)lpParameter;

    Assert( CESESnapshotSession::FMember( pSession ) );

    pSession->SnapshotThreadProc( (ULONG)UlParam( JET_paramOSSnapshotTimeout ) );

    return 0;
}

ERR CESESnapshotSession::ErrStartSnapshotThreadProc()
{
    ERR err = JET_errSuccess;

    Assert ( CESESnapshotSession::g_critOSSnapshot.FOwner() );
    Assert ( statePrepare == State() );

    m_asigSnapshotStarted.Reset();
    Call ( ErrUtilThreadCreate( DwSnapshotThreadProc,
                                OSMemoryPageReserveGranularity(),
                                priorityNormal,
                                &m_thread,
                                DWORD_PTR( this ) ) );

    // wait until snapshot thread is starting
    m_asigSnapshotStarted.Wait();

    // freeze start failed
    if ( JET_errSuccess > m_errFreeze )
    {
        // thread is gone
        Assert ( 0 == m_thread);
        Call ( m_errFreeze );
        Assert ( fFalse );
    }

    SwitchTo( CESESnapshotSession::stateFreeze );

    Assert ( JET_errSuccess == err );

HandleError:
    return err;
}


void CESESnapshotSession::StopSnapshotThreadProc( const BOOL fWait )
{
    m_asigSnapshotThread.Set();

    if ( fWait )
    {
        (void) UtilThreadEnd( m_thread );
        m_thread = 0;
    }

    return;
}


BOOL CESESnapshotSession::FCanSwitchTo( const SNAPSHOT_STATE stateNew ) const
{
    Assert ( CESESnapshotSession::g_critOSSnapshot.FOwner() );

    switch ( stateNew )
    {
        case stateStart:
            if ( m_state != stateTimeOut )  // time-out end
            {
                return fFalse;
            }
            break;

        case stateAllowLogTruncate:
            if ( m_state != stateThaw )    // normal Thaw
            {
                return fFalse;
            }
            break;

        case statePrepare:
            // We used to allow a new prepare from (almost?) any state.
            // With multiple Parallel VSS sessions, Prepare will create 
            // a new session so the only state left here is Start 
            //
            if ( m_state != stateStart )    // normal start
            {
                return fFalse;
            }
            break;

        case stateFreeze:
            if ( m_state != statePrepare )  // just after prepare
            {
                return fFalse;
            }
            break;

        case stateThaw:
            if ( m_state != stateFreeze )       // normal end
            {
                return fFalse;
            }
            break;

        case stateTimeOut:
            if ( m_state != stateFreeze )   // timeout only from the freeze state
            {
                return fFalse;
            }
            break;

        case stateLogTruncated:
            if ( m_state != stateAllowLogTruncate &&    // normal Thaw
                m_state != stateLogTruncated)          // allow multiple truncates (for each instance)
            {
                return fFalse;
            }
            break;

#ifdef OS_SNAPSHOT_ALLOW_TRUNCATE_ONLY
        case stateLogTruncatedNoFreeze:
            if ( m_state != statePrepare &&             // truncate only (w/o database copy)
               m_state != stateLogTruncatedNoFreeze)    // allow multiple truncates (for each instance)
            {
                return fFalse;
            }

            break;
#endif

        case stateAbort:
            // we can abort from any state other the Abort and End
            if ( stateAbort == m_state ||
                stateEnd == m_state)
            {
                    return fFalse;
            }
            break;

        case stateEnd:
            // we can normal end in the below cases,
            // otherwise they have to abort
            if ( stateAllowLogTruncate != m_state &&
                stateLogTruncated != m_state &&
                statePrepare != m_state
#ifdef OS_SNAPSHOT_ALLOW_TRUNCATE_ONLY
                && stateLogTruncatedNoFreeze != m_state
#endif
                )
            {
                    return fFalse;
            }
            break;

        default:
            Assert ( fFalse );
            return fFalse;
    }

    return fTrue;
}

void CESESnapshotSession::SwitchTo( const SNAPSHOT_STATE stateNew )
{
    Assert ( CESESnapshotSession::g_critOSSnapshot.FOwner() );
    m_state = stateNew;

    const INT cbFillString = 20;
    WCHAR szSnap[cbFillString + 1];
    const WCHAR* rgszT[1] = { szSnap };

    OSStrCbFormatW( szSnap, sizeof( szSnap ), L"%lu", (ULONG)m_idSnapshot );


    // do special action depending on the new state
    switch ( m_state )
    {
        case statePrepare:
            UtilReportEvent( eventInformation, OS_SNAPSHOT_BACKUP,
                IsFullSnapshot() ?
                    ( IsCopySnapshot() ? OS_SNAPSHOT_PREPARE_COPY_ID : OS_SNAPSHOT_PREPARE_FULL_ID ) :
                    ( IsCopySnapshot() ? OS_SNAPSHOT_PREPARE_DIFFERENTIAL_ID : OS_SNAPSHOT_PREPARE_INCREMENTAL_ID ),
                1, rgszT );

            // continue ...
        case stateStart:
        case stateAllowLogTruncate:
            m_asigSnapshotThread.Reset();
            break;

        case stateEnd:
            UtilReportEvent( eventInformation, OS_SNAPSHOT_BACKUP, OS_SNAPSHOT_END_ID, 1, rgszT );
            if ( m_thread )
            {
                UtilThreadEnd( m_thread );
                m_thread = 0;
            }
            break;

        case stateAbort:
            UtilReportEvent( eventError, OS_SNAPSHOT_BACKUP, OS_SNAPSHOT_ABORT_ID, 1, rgszT );
            if ( m_thread )
            {
                UtilThreadEnd( m_thread );
                m_thread = 0;
            }
            break;
        default:
            break;
    }

}

ERR ISAMAPI ErrIsamOSSnapshotPrepare( JET_OSSNAPID * psnapId, const JET_GRBIT grbit )
{
    ERR                     err             = JET_errSuccess;
    CESESnapshotSession *   pSession        = NULL;
    BOOL                    fCritSection    = fFalse;

    // If you might want to truncate logs and stamp the database headers with the backup 
    // info immediately after Thaw, then you should pass JET_bitContinueAfterThaw (such 
    // as Exchange Store does - at least currently).

    if( grbit & ~( JET_bitIncrementalSnapshot | JET_bitCopySnapshot | JET_bitContinueAfterThaw | JET_bitExplicitPrepare ))
    {
        return ErrERRCheck( JET_errInvalidGrbit );
    }

    if ( !psnapId )
    {
        CallR ( ErrERRCheck( JET_errInvalidParameter ) );
    }


    // We can do all the stuff in this function w/o jet to be initialized
    // but it will fail later (in Freeze when we set "backup in progress")
    // as it is nothing to snap anyway. We will just error out sooner.
    if ( !g_fSystemInit )
    {
        CallR ( ErrERRCheck( JET_errNotInitialized ) );
    }

    CESESnapshotSession::SnapshotCritEnter();
    fCritSection = fTrue;
    
    Call( CESESnapshotSession::ErrAllocSession( &pSession ) );
    
    Assert( pSession );

    if ( !pSession->FCanSwitchTo( CESESnapshotSession::statePrepare ) )
    {
        CESESnapshotSession::SnapshotCritLeave();
        Error( ErrERRCheck( JET_errOSSnapshotInvalidSequence ) );
    }


    // get the ID before switching the state in order to have it
    // when reporting the EventLog
    //
    *psnapId = pSession->GetId();

    if ( JET_bitIncrementalSnapshot & grbit )
    {
        pSession->SetIncrementalSnapshot ();
    }
    else
    {
        pSession->SetFullSnapshot ();
    }

    if ( JET_bitCopySnapshot & grbit )
    {
        pSession->SetCopySnapshot ();
    }
    else
    {
        pSession->ResetCopySnapshot ();
    }

    if ( JET_bitContinueAfterThaw & grbit )
    {
        pSession->SetContinueAfterThaw ();
    }
    else
    {
        pSession->ResetContinueAfterThaw ();
    }

    pSession->SwitchTo( CESESnapshotSession::statePrepare );

    CESESnapshotSession::SnapshotCritLeave();
    fCritSection = fFalse;

    pSession->SetInitialFreeze( !(BOOL)( JET_bitExplicitPrepare & grbit ) );

    Assert ( JET_errSuccess == err );

HandleError:
#ifdef OS_SNAPSHOT_TRACE
{
    WCHAR szError[12];
    const WCHAR* rgszT[2] = { L"JetOSSnapshotPrepare", szError };
    OSStrCbFormatW( szError, sizeof( szError ), L"%d", err );

    UtilReportEvent( JET_errSuccess > err?eventError:eventInformation, OS_SNAPSHOT_BACKUP, OS_SNAPSHOT_TRACE_ID, 2, rgszT );
}
#endif // OS_SNAPSHOT_TRACE

    if ( err < JET_errSuccess && pSession != NULL )
    {
        Assert( fCritSection );
        CESESnapshotSession::RemoveSession( pSession );
    }

    if ( fCritSection )
    {
        CESESnapshotSession::SnapshotCritLeave();
    }
    
    return err;
}


ERR ISAMAPI ErrIsamOSSnapshotPrepareInstance( JET_OSSNAPID snapId, INST * pinst, const JET_GRBIT    grbit )
{
    CESESnapshotSession *   pSession    = NULL;
    ERR                     err         = JET_errSuccess;
    
    if ( NO_GRBIT != grbit )
    {
        return ErrERRCheck( JET_errInvalidGrbit );
    }

    CESESnapshotSession::SnapshotCritEnter();

    Call( CESESnapshotSession::ErrGetSessionByID( snapId, &pSession ) );
    Assert( pSession );

    // we can add instance only just before freezing. Check this!
    if ( !pSession->FCanSwitchTo( CESESnapshotSession::stateFreeze ) )
    {
        Error( ErrERRCheck( JET_errOSSnapshotInvalidSequence ) );
    }

    Assert( pinstNil != pinst );
    const ULONG             ipinst      = IpinstFromPinst( pinst );

    //  verify instance is not already participating in a backup
    //
    Assert( (size_t)ipinst < g_cpinstMax );
    Assert( pinstNil != g_rgpinst[ ipinst ] );
    Assert( g_rgpinst[ ipinst ]->m_fJetInitialized );
    if ( NULL != g_rgpinst[ ipinst ]->m_pOSSnapshotSession
        || g_rgpinst[ ipinst ]->m_pbackup->FBKBackupInProgress()
        || !g_rgpinst[ ipinst ]->m_fBackupAllowed )
    {
        Error( ErrERRCheck( JET_errOSSnapshotNotAllowed ) );
    }

    // add instance to the list
    pSession->ErrAddInstanceToFreeze( ipinst );

    CESESnapshotSession::SnapshotCritLeave();

    Assert ( JET_errSuccess == err );
    return err;

HandleError:
    CESESnapshotSession::SnapshotCritLeave();
    return err;
}


ERR ISAMAPI ErrIsamOSSnapshotFreeze( const JET_OSSNAPID             snapId,
                                            ULONG *         pcInstanceInfo,
                                            JET_INSTANCE_INFO_W **  paInstanceInfo,
                                            const JET_GRBIT         grbit )
{
    CESESnapshotSession *   pSession                = NULL;
    ERR                     err                     = JET_errSuccess;

    if ( NO_GRBIT != grbit )
    {
        return ErrERRCheck( JET_errInvalidGrbit );
    }

    if ( NULL == pcInstanceInfo || NULL == paInstanceInfo )
    {
        CallR ( ErrERRCheck(JET_errInvalidParameter ) );
    }

    *pcInstanceInfo = 0;
    *paInstanceInfo = NULL;

    CESESnapshotSession::SnapshotCritEnter();

    Call( CESESnapshotSession::ErrGetSessionByID( snapId, &pSession ) );
    Assert( pSession );

    if ( !pSession->FCanSwitchTo( CESESnapshotSession::stateFreeze ) )
    {
        Error( ErrERRCheck( JET_errOSSnapshotInvalidSequence ) );
    }

    Call ( pSession->ErrStartSnapshotThreadProc() );

    Assert ( CESESnapshotSession::stateFreeze == pSession->State() );
    Assert ( JET_errSuccess == err );

    err = ErrIsamGetInstanceInfo( pcInstanceInfo, paInstanceInfo, pSession );
    if ( JET_errSuccess > err )
    {
        // need to thaw as we got an error at this point
        ERR errT;

        CESESnapshotSession::SnapshotCritLeave();
        errT = ErrIsamOSSnapshotThaw( snapId, grbit );
        Assert( JET_errSuccess == errT );
        CESESnapshotSession::SnapshotCritEnter();
    }
    Call ( err );

    Assert ( JET_errSuccess == err );

HandleError:
    CESESnapshotSession::SnapshotCritLeave();

#ifdef OS_SNAPSHOT_TRACE
{
    WCHAR szError[12];
    const WCHAR* rgszT[2] = { L"JetOSSnapshotFreeze", szError };
    OSStrCbFormatW( szError, sizeof( szError ), L"%d", err );

    UtilReportEvent( JET_errSuccess > err?eventError:eventInformation, OS_SNAPSHOT_BACKUP, OS_SNAPSHOT_TRACE_ID, 2, rgszT );
}
#endif // OS_SNAPSHOT_TRACE

    return err;
}

ERR ISAMAPI ErrIsamOSSnapshotThaw(  const JET_OSSNAPID snapId, const    JET_GRBIT grbit )
{
    CESESnapshotSession *   pSession    = NULL;
    ERR                     err         = JET_errSuccess;
    BOOL                    fInSnapCrit  = fFalse;

    if ( NO_GRBIT != grbit )
    {
        return ErrERRCheck( JET_errInvalidGrbit );
    }

    CESESnapshotSession::SnapshotCritEnter();
    fInSnapCrit = fTrue;

    Call( CESESnapshotSession::ErrGetSessionByID( snapId, &pSession ) );
    Assert( pSession );

    // we can end it if it is freezed or time-out already
    if ( CESESnapshotSession::stateFreeze != pSession->State() && // normal end from freeze
        CESESnapshotSession::stateTimeOut != pSession->State() ) // we arealdy got the time-out
    {
        Error( ErrERRCheck( JET_errOSSnapshotInvalidSequence ) );
    }


    // the state can't be End as long as we haven't signaled the thread
    Assert ( CESESnapshotSession::stateFreeze == pSession->State() || CESESnapshotSession::stateTimeOut == pSession->State() );

    CESESnapshotSession::SnapshotCritLeave();
    fInSnapCrit = fFalse;

RetryStop:

    CESESnapshotSession::SnapshotCritEnter();
    fInSnapCrit = fTrue;

    Assert ( CESESnapshotSession::stateFreeze == pSession->State() ||
            CESESnapshotSession::stateThaw == pSession->State() ||
            CESESnapshotSession::stateTimeOut == pSession->State() );

    if ( CESESnapshotSession::stateThaw == pSession->State() )
    {
        Assert ( pSession->FCanSwitchTo( CESESnapshotSession::stateAllowLogTruncate ) );
        pSession->SwitchTo( CESESnapshotSession::stateAllowLogTruncate );
        err = JET_errSuccess;
    }
    else if ( CESESnapshotSession::stateTimeOut == pSession->State() )
    {
        err = ErrERRCheck( JET_errOSSnapshotTimeOut );
    }
    else
    {
        // thread is signaled but not ended yet ...
        Assert ( CESESnapshotSession::stateFreeze == pSession->State() );

        // let the thread change the status and wait for it's complition
        CESESnapshotSession::SnapshotCritLeave();
        fInSnapCrit = fFalse;

        // signal thread to stop and wait thread complition
        pSession->StopSnapshotThreadProc( fTrue /* wait thread complition*/ );

        goto RetryStop;
    }

    if ( !pSession->FContinueAfterThaw() )
    {
        CESESnapshotSession::SnapshotCritLeave();
        fInSnapCrit = fFalse;

        CallS( ErrIsamOSSnapshotEnd( snapId, ( err >= JET_errSuccess )  ? 0 : JET_bitAbortSnapshot ) );
    }

HandleError:

    if ( fInSnapCrit )
    {
        CESESnapshotSession::SnapshotCritLeave();
        fInSnapCrit = fFalse;
    }

#ifdef OS_SNAPSHOT_TRACE
{
    WCHAR szError[12];
    const WCHAR* rgszT[2] = { L"JetOSSnapshotThaw", szError };
    OSStrCbFormatW( szError, sizeof( szError ), L"%d", err );

    UtilReportEvent( JET_errSuccess > err?eventError:eventInformation, OS_SNAPSHOT_BACKUP, OS_SNAPSHOT_TRACE_ID, 2, rgszT );
}
#endif // OS_SNAPSHOT_TRACE

    return err;
}

ERR ISAMAPI ErrIsamOSSnapshotAbort( const JET_OSSNAPID snapId, const JET_GRBIT grbit )
{
    if ( grbit != 0 )
    {
        return ErrERRCheck( JET_errInvalidGrbit );
    }
    return ErrIsamOSSnapshotEnd( snapId, JET_bitAbortSnapshot );
}

ERR CESESnapshotSession::ErrTruncateLogs(INST * pinstTruncate, const JET_GRBIT grbit)
{
    ERR         err         = JET_errSuccess;
    INST *      pinst;
    BOOL        fOneInstance = fFalse;

    Assert ( 0 == grbit || JET_bitAllDatabasesSnapshot == grbit );

    INST::EnterCritInst();

    for ( pinst = GetFirstInstance(); pinst && !fOneInstance; pinst = GetNextInstance() )
    {
        ERR errT;

        if (pinstTruncate && pinstTruncate != pinst)
        {
            continue;
        }

        if (pinstTruncate)
        {
            fOneInstance = fTrue;
        }

        errT = pinst->m_pbackup->ErrBKOSSnapshotTruncateLog( grbit );

        if ( err >= JET_errSuccess && errT < JET_errSuccess )
        {
            err = errT;
        }
    }

    INST::LeaveCritInst();

    if (pinstTruncate && !fOneInstance)
{
        err = ErrERRCheck( -1 );

}
    return err;
}

void CESESnapshotSession::SaveSnapshotInfo( const JET_GRBIT grbit )
{
    Assert ( CESESnapshotSession::stateAllowLogTruncate == State() ||
            CESESnapshotSession::stateLogTruncated == State());

    Assert ( 0 == grbit || JET_bitAbortSnapshot == grbit );

    // we can mark the database headers based on
    // the result of the snapshot (ok or timeout)

    if ( JET_bitAbortSnapshot != grbit )
    {
        INST::EnterCritInst();

        for ( INST* pinst = GetFirstInstance(); pinst; pinst = GetNextInstance() )
        {
            pinst->m_pbackup->BKOSSnapshotSaveInfo( IsIncrementalSnapshot(), IsCopySnapshot() );
        }

        INST::LeaveCritInst();
    }

    ResetBackupInProgress( NULL );
}

ERR ISAMAPI ErrIsamOSSnapshotTruncateLog(   const JET_OSSNAPID snapId, INST * pinst, const  JET_GRBIT grbit )
{
    CESESnapshotSession *   pSession    = NULL;
    ERR                     err         = JET_errSuccess;

    if( grbit & ~JET_bitAllDatabasesSnapshot )
    {
        return ErrERRCheck( JET_errInvalidGrbit );
    }

    Assert ( 0 == grbit || JET_bitAllDatabasesSnapshot == grbit );

    CESESnapshotSession::SnapshotCritEnter();

    Call( CESESnapshotSession::ErrGetSessionByID( snapId, &pSession ) );
    Assert( pSession );

    // we don't allow truncate in states other than after a normal freeze/thaw
    // which is setting the state to stateStartAllowTruncate
    if ( !pSession->FCanSwitchTo( CESESnapshotSession::stateLogTruncated )
#ifdef OS_SNAPSHOT_ALLOW_TRUNCATE_ONLY
        && !pSession->FCanSwitchTo( CESESnapshotSession::stateLogTruncatedNoFreeze )
#endif
        )
    {
        Error( ErrERRCheck( JET_errOSSnapshotInvalidSequence ) );
    }

    if (pSession->IsCopySnapshot ())
    {
        Error( ErrERRCheck( JET_errOSSnapshotInvalidSequence ) );
    }

    Call ( pSession->ErrTruncateLogs( pinst, grbit ) );

#ifdef OS_SNAPSHOT_ALLOW_TRUNCATE_ONLY
    if (pSession->FCanSwitchTo( CESESnapshotSession::stateLogTruncated ))
    {
        pSession->SwitchTo( CESESnapshotSession::stateLogTruncated );
    }
    else
    {
        Assert ( pSession->FCanSwitchTo( CESESnapshotSession::stateLogTruncatedNoFreeze ) );
        pSession->SwitchTo( CESESnapshotSession::stateLogTruncatedNoFreeze );
    }
#else
        pSession->SwitchTo( CESESnapshotSession::stateLogTruncated );
#endif

    Assert ( JET_errSuccess == err );

HandleError:
    CESESnapshotSession::SnapshotCritLeave();
    return err;
}


ERR ISAMAPI ErrIsamOSSnapshotGetFreezeInfo( const JET_OSSNAPID      snapId,
                                            ULONG *         pcInstanceInfo,
                                            JET_INSTANCE_INFO_W **  paInstanceInfo,
                                            const JET_GRBIT         grbit )
{
    CESESnapshotSession *   pSession                = NULL;
    ERR                     err                     = JET_errSuccess;
    BOOL                    fCritInst               = fFalse;

    if ( NO_GRBIT != grbit )
    {
        return ErrERRCheck( JET_errInvalidGrbit );
    }
    else if ( NULL == pcInstanceInfo || NULL == paInstanceInfo )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    *pcInstanceInfo = 0;
    *paInstanceInfo = NULL;

    CESESnapshotSession::SnapshotCritEnter();

    Call( CESESnapshotSession::ErrGetSessionByID( snapId, &pSession ) );
    Assert( pSession );

    switch ( pSession->State() )
    {
        case CESESnapshotSession::stateStart:
        case CESESnapshotSession::stateEnd:
        case CESESnapshotSession::stateAbort:
            Error( ErrERRCheck( JET_errOSSnapshotInvalidSequence ) );
            break;

        case CESESnapshotSession::statePrepare:
        case CESESnapshotSession::stateThaw:
        case CESESnapshotSession::stateTimeOut:
        case CESESnapshotSession::stateAllowLogTruncate:
        case CESESnapshotSession::stateLogTruncated:
#ifdef OS_SNAPSHOT_ALLOW_TRUNCATE_ONLY
        case CESESnapshotSession::stateLogTruncatedNoFreeze:
#endif
            INST::EnterCritInst();
            fCritInst = fTrue;

            // not get the instance + databases information

        case CESESnapshotSession::stateFreeze:
            err = ErrIsamGetInstanceInfo( pcInstanceInfo, paInstanceInfo, pSession );

            if (fCritInst)
            {
                INST::LeaveCritInst();
                fCritInst = fFalse;
            }
            break;

        default:
            Assert( fFalse );
            Error( ErrERRCheck( JET_errOSSnapshotInvalidSequence ) );
    }

    Call( err );
    CallS( err );

HandleError:
    CESESnapshotSession::SnapshotCritLeave();

#ifdef OS_SNAPSHOT_TRACE
{
    WCHAR szError[12];
    const WCHAR* rgszT[2] = { L"JetOSSnapshotGetFreezeInfo", szError };
    OSStrCbFormatW( szError, sizeof( szError ), L"%d", err );

    UtilReportEvent( JET_errSuccess > err?eventError:eventInformation, OS_SNAPSHOT_BACKUP, OS_SNAPSHOT_TRACE_ID, 2, rgszT );
}
#endif // OS_SNAPSHOT_TRACE

    return err;
}


ERR ISAMAPI ErrIsamOSSnapshotEnd(   const JET_OSSNAPID snapId, const    JET_GRBIT grbit )
{
    CESESnapshotSession *   pSession    = NULL;
    ERR                     err         = JET_errSuccess;

    if( grbit & ~JET_bitAbortSnapshot )
    {
        return ErrERRCheck( JET_errInvalidGrbit );
    }

    Assert ( 0 == grbit || JET_bitAbortSnapshot == grbit );

    CESESnapshotSession::SnapshotCritEnter();

    Call( CESESnapshotSession::ErrGetSessionByID( snapId, &pSession ) );
    Assert( pSession );

    CESESnapshotSession::SNAPSHOT_STATE newState;

    if (JET_bitAbortSnapshot == grbit)
    {
        newState = CESESnapshotSession::stateAbort;
    }
    else
    {
        newState = CESESnapshotSession::stateEnd;
    }

    if ( !pSession->FCanSwitchTo( newState ) )
    {
        Error( ErrERRCheck( JET_errOSSnapshotInvalidSequence ) );
    }

    const BOOL fContinueAfterThaw = pSession->FContinueAfterThaw();

    // we can mark the database headers based on
    // the result of the snapshot (ok or timeout)
    switch ( pSession->State() )
    {
        case CESESnapshotSession::statePrepare:
            // we just prepared so we can get out w/o problems
            pSession->ResetBackupInProgress( NULL );
            break;
            
        case CESESnapshotSession::stateEnd:
        case CESESnapshotSession::stateAbort:
            break;

        case CESESnapshotSession::stateFreeze:
            Assert( JET_bitAbortSnapshot == grbit );
            // try to thaw, then retry this function again
            CESESnapshotSession::SnapshotCritLeave();

            err = ErrIsamOSSnapshotThaw(snapId , NO_GRBIT );
            if ( fContinueAfterThaw )
            {
                return ErrIsamOSSnapshotEnd(snapId , grbit );
            }
            else
            {
#ifdef DEBUG
                //  verify that this session was released in ErrIsamOSSnapshotThaw
                
                CESESnapshotSession::SnapshotCritEnter();
                CESESnapshotSession* pSessionT;
                Assert( CESESnapshotSession::ErrGetSessionByID( snapId, &pSessionT ) == JET_errOSSnapshotInvalidSnapId );
                CESESnapshotSession::SnapshotCritLeave();
#endif //  DEBUG
                return err;
            }

        case CESESnapshotSession::stateAllowLogTruncate:
        case CESESnapshotSession::stateLogTruncated:
            pSession->SaveSnapshotInfo( grbit );
            break;

#ifdef OS_SNAPSHOT_ALLOW_TRUNCATE_ONLY
        case CESESnapshotSession::stateLogTruncatedNoFreeze:
            // they truncated w/o doing a freeze

            //  Umm, don't we need to reset backup-in-progress??
            //  I'm forcing a compilation failure to ensure that
            //  this issue is considered if this code path ever
            //  gets activated
            //
            #error "need to evaluate before enabling this code"
            break;
#endif
        case CESESnapshotSession::stateTimeOut:
            Assert( JET_bitAbortSnapshot == grbit );
            // snapshot failed due to time-out, we won't stamp the headers and
            // the "BackupInProgress" should not be set anyway, since it should
            // have gotten reset by Thaw()
            break;

        case CESESnapshotSession::stateThaw:
            // this is a temporary state, we should not see it here
        case CESESnapshotSession::stateStart:
            // we dealt with this before the switch
        default:
            Assert ( 0 );
    }

    pSession->SwitchTo( newState );

    Assert ( JET_errSuccess == err );

    CESESnapshotSession::RemoveSession( pSession );
    pSession = NULL;

HandleError:
    CESESnapshotSession::SnapshotCritLeave();
    return err;
}

#define ENABLE_LOST_FLUSH_INSTRUMENTATION

ERR FMP::ErrDBReadPages(
    const PGNO pgnoFirst,
    VOID* const pvPageFirst,
    const CPG cpg,
    CPG* const pcpgActual,
    const TraceContext& tc,
    const BOOL fExtensiveChecks )
{
    ERR err = JET_errSuccess;
    PGNO pgnoLastFileSystem = pgnoNull;

    //  caller should have verified that buffer is not empty
    //
    Assert( cpg > 0 );
    Assert( pgnoFirst >= 1 );
    const PGNO pgnoLast = pgnoFirst + cpg - 1;

    Assert( pcpgActual != NULL );
    *pcpgActual = 0;

    // Prevent database from physically growing.
    AcquireIOSizeChangeLatch();

    Call( ErrPgnoLastFileSystem( &pgnoLastFileSystem ) );

    // only actually read pages from the database if the start
    // of the requested range is before the last page in the
    // database.
    if ( pgnoFirst <= pgnoLastFileSystem )
    {
        const PGNO pgnoStart = pgnoFirst;
        const PGNO pgnoEnd = UlFunctionalMin( pgnoLast, pgnoLastFileSystem );

        Assert( pgnoStart <= pgnoEnd );

        //  engage range lock for the region to copy
        Call( ErrRangeLock( pgnoStart, pgnoEnd ) );

#ifdef ENABLE_LOST_FLUSH_INSTRUMENTATION
#ifdef DEBUG
        CPAGE::PageFlushType rgpgft[ ( 1 * 1024 * 1024 ) / g_cbPageMin ];
        if ( m_pflushmap != NULL )
        {
            for ( PGNO pgno = pgnoStart; pgno <= pgnoEnd; pgno++ )
            {
                const size_t ipgno = pgno - pgnoStart;
                if ( ipgno >= _countof( rgpgft ) )
                {
                    break;
                }
                rgpgft[ ipgno ] = m_pflushmap->PgftGetPgnoFlushType( pgno );
            }
        }
#endif // DEBUG
#endif // ENABLE_LOST_FLUSH_INSTRUMENTATION

        err = ErrIOReadDbPages( Ifmp(), Pfapi(), (BYTE *)pvPageFirst, pgnoStart, pgnoEnd, fTrue, pgnoLastFileSystem, tc, fExtensiveChecks );

#ifdef ENABLE_LOST_FLUSH_INSTRUMENTATION
        BYTE* pvPageFirstCopy = NULL;

        // If we got an unexpected lost flush error, loop for a while to see if the page fixes itself.
        if ( ( err == JET_errReadLostFlushVerifyFailure ) && !FNegTest( fCorruptingWithLostFlush ) )
        {
            DWORD cRetriesMax = 0;
            
#ifdef DEBUG
            const CPG cpgActual = pgnoEnd - pgnoStart + 1;
            pvPageFirstCopy = new BYTE[ cpgActual * g_cbPage ];
            if ( pvPageFirstCopy != NULL )
            {
                UtilMemCpy( pvPageFirstCopy, pvPageFirst, cpgActual * g_cbPage );
            }

            cRetriesMax = 100;
#else // !DEBUG
            // FNegTest() always returns fFalse in RETAIL, so do not run the retry loop when running tests, otherwise
            // they will get confused with multiple events and take too long with 1 sec per lost flush detected.
            cRetriesMax = ( _wcsicmp( WszUtilProcessName(), L"Microsoft.Exchange.Store.Worker" ) == 0 ) ? 10 : 0;
#endif // DEBUG

            if ( cRetriesMax > 0 )
            {
                DWORD cRetries = 0;
                ERR errRetry = JET_errSuccess;
                while ( cRetries < cRetriesMax )
                {
                    cRetries++;
                    errRetry = ErrIOReadDbPages( Ifmp(), Pfapi(), (BYTE *)pvPageFirst, pgnoStart, pgnoEnd, fTrue, pgnoLastFileSystem, tc, fExtensiveChecks );
                    if ( errRetry != JET_errReadLostFlushVerifyFailure )
                    {
                        break;
                    }

                    UtilSleep( 100 );
                }

                AssertTrack( fFalse, OSFormat( "UnexpectedLostFlush:%I32u:%d:%d", cRetries, err, errRetry ) );
            }
        }

#ifdef DEBUG
        if ( m_pflushmap != NULL )
        {
            for ( PGNO pgno = pgnoStart; pgno <= pgnoEnd; pgno++ )
            {
                const size_t ipgno = pgno - pgnoStart;
                if ( ipgno >= _countof( rgpgft ) )
                {
                    break;
                }

                // The flush state must not have changed while we are reading the page
                // and holding the range lock, except for when the page was previously
                // uncached and caching it triggered initializing the page's flush state.
                Assert( ( rgpgft[ ipgno ] == m_pflushmap->PgftGetPgnoFlushType( pgno ) ) ||
                        ( rgpgft[ ipgno ] == CPAGE::pgftUnknown ) );
            }
        }
#endif // DEBUG

        delete[] pvPageFirstCopy;

#endif // ENABLE_LOST_FLUSH_INSTRUMENTATION

        /*  disengage range lock for the region copied
        /**/
        RangeUnlock( pgnoStart, pgnoEnd );

        Call( err );
    }

    if ( pgnoLast > pgnoLastFileSystem )
    {
        // for all the pages beyond the database size range,
        // we will fill them in with shrunk pages.
        for ( PGNO pgnoShrunk = UlFunctionalMax( pgnoFirst, pgnoLastFileSystem + 1 ); pgnoShrunk <= pgnoLast; pgnoShrunk++ )
        {
            CPAGE cpage;
            cpage.GetShrunkPage( Ifmp(), pgnoShrunk, (BYTE*)pvPageFirst + ( g_cbPage * ( pgnoShrunk - pgnoFirst ) ), CbPage() );
        }
    }

    *pcpgActual = cpg;

HandleError:

    ReleaseIOSizeChangeLatch();

    return err;
}

ERR
ErrIsamGetDatabasePages(
    __in JET_SESID                              vsesid,
    __in JET_DBID                               vdbid,
    __in ULONG                          pgnoStart,
    __in ULONG                          cpg,
    __out_bcount_part( cb, *pcbActual ) void *  pv,
    __in ULONG                          cb,
    __out ULONG *                       pcbActual,
    __in JET_GRBIT                              grbit )
{
    ERR                 err         = JET_errSuccess;
    PIB *               ppib        = (PIB *)vsesid;
    IFMP                ifmp        = g_ifmpMax;
    CPG                 cpgActual   = 0;

    //  check parameters
    //
    CallR( ErrPIBCheck( ppib ) );
    CallR( ErrPIBCheckIfmp( ppib, (IFMP)vdbid ) );

    ifmp = (IFMP) vdbid;
    if ( ifmp >= g_ifmpMax || !g_rgfmp[ ifmp ].FInUse()  )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    FMP::AssertVALIDIFMP( ifmp );

    if ( pgnoStart < 1 )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( CPG( cpg ) < 0 )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( pgnoStart + cpg > pgnoSysMax )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( UINT_PTR(0) != ( UINT_PTR(pv) & UINT_PTR( OSMemoryPageCommitGranularity( ) - 1 ) ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    
    if ( cb != 0 && pv == NULL )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( cb < cpg * g_cbPage )
    {
        Error( ErrERRCheck( JET_errBufferTooSmall ) );
    }

    if ( pcbActual == NULL )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( grbit != 0 )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    //  read the requested pages from disk
    //
    if ( 0 != cpg )
    {
        PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
        tcScope->iorReason.SetIort( iortPagePatching );
        tcScope->iorReason.SetIorp( iorpPatchFix );
        Call( g_rgfmp[ ifmp ].ErrDBReadPages( pgnoStart, pv, cpg, &cpgActual, *tcScope ) );
        Assert( (ULONG)cpgActual <= cpg );
    }

    //  return the number of pages actually read
    //
    *pcbActual = cpgActual * g_cbPage;

HandleError:
    return err;
}

// Patch a page in an online database
ERR ErrIsamOnlinePatchDatabasePage(
    __in JET_SESID                              vsesid,
    __in JET_DBID                               vdbid,
    __in ULONG                          pgno,
    __in_bcount(cbData) const void *            pvToken,
    __in ULONG                          cbToken,
    __in_bcount(cbData) const void *            pvData,
    __in ULONG                          cbData,
    __in JET_GRBIT                              grbit )
{
    ERR                 err         = JET_errSuccess;
    PIB *               ppib        = (PIB *)vsesid;
    IFMP                ifmp        = g_ifmpMax;

    //  check parameters
    //
    CallR( ErrPIBCheck( ppib ) );
    CallR( ErrPIBCheckIfmp( ppib, (IFMP)vdbid ) );

    PIBTraceContextScope tcRef = ppib->InitTraceContextScope();
    tcRef->iorReason.SetIort( iortPagePatching );

    ifmp = (IFMP) vdbid;
    if ( ifmp >= g_ifmpMax || !g_rgfmp[ ifmp ].FInUse()  )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    FMP::AssertVALIDIFMP( ifmp );

    if ( grbit & 0x2 /* = legacy JET_bitPatchAllowCorruption */ )
    {
        AssertSz( fFalse, "We have deprecated allowing online corruption of pages" );
        return ErrERRCheck( JET_errDisabledFunctionality );
    }

    //  we should only be called if JET_paramEnableExternalAutoHealing is set.
    //
    if ( !BoolParam( PinstFromIfmp( ifmp ), JET_paramEnableExternalAutoHealing ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    Call( ErrBFPatchPage( ifmp, pgno, pvToken, cbToken, pvData, cbData ) );

HandleError:
    return err;
}

//  ================================================================
LOCAL ERR ErrValidateHeaderForIncrementalReseed( const DBFILEHDR * const pdbfilehdr )
//  ================================================================
//
//  validate the header for a database that is being used by
//  incremental reseed
//
//-
{
    ERR err = JET_errSuccess;
    if ( pdbfilehdr->le_filetype != JET_filetypeDatabase )
    {
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }
    
    if ( ulDAEMagic != pdbfilehdr->le_ulMagic )
    {
        Error( ErrERRCheck( JET_errInvalidDatabaseVersion ) );
    }
    else if ( pdbfilehdr->le_ulVersion >= ulDAEVersionESE97 )
    {
        err = JET_errSuccess;
    }
    else
    {
        Assert( pdbfilehdr->le_ulVersion < ulDAEVersionMax );
        Error( ErrERRCheck( JET_errInvalidDatabaseVersion ) );
    }

    if( !FDBIsLVChunkSizeCompatible( pdbfilehdr->le_cbPageSize, pdbfilehdr->le_ulVersion, pdbfilehdr->le_ulDaeUpdateMajor ) )
    {
        Error( ErrERRCheck( JET_errInvalidDatabaseVersion ) );
    }

HandleError:
    return err;
}

//  ================================================================
JETUNITTEST( IO, ErrValidateHeaderForIncrementalReseed )
//  ================================================================
{
    DBFILEHDR dbfilehdr;
    dbfilehdr.le_filetype           = JET_filetypeDatabase;
    dbfilehdr.le_ulMagic            = ulDAEMagic;
    dbfilehdr.le_ulVersion          = ulDAEVersionMax;
    dbfilehdr.le_ulDaeUpdateMajor   = ulDAEUpdateMajorMax;
    dbfilehdr.le_cbPageSize         = 4*1024;

    CHECK( JET_errSuccess == ErrValidateHeaderForIncrementalReseed( &dbfilehdr ) );

    dbfilehdr.le_filetype = JET_filetypeLog;
    CHECK( JET_errDatabaseCorrupted == ErrValidateHeaderForIncrementalReseed( &dbfilehdr ) );
    dbfilehdr.le_filetype = JET_filetypeDatabase;

    dbfilehdr.le_ulMagic = ~ulDAEMagic;
    CHECK( JET_errInvalidDatabaseVersion == ErrValidateHeaderForIncrementalReseed( &dbfilehdr ) );
    dbfilehdr.le_ulMagic = ulDAEMagic;
 
    // if the update is 0 then the subtraction below will underflow
    Assert( ulDAEVersionMax > 0 );
    dbfilehdr.le_ulVersion = ulDAEVersionMax - 1;
    CHECK( JET_errInvalidDatabaseVersion == ErrValidateHeaderForIncrementalReseed( &dbfilehdr ) );
    dbfilehdr.le_ulVersion = ulDAEVersionMax;

    // incompatible LV chunk sizes
    dbfilehdr.le_ulDaeUpdateMajor = 0x10;
    dbfilehdr.le_cbPageSize     = 32*1024;
    CHECK( JET_errInvalidDatabaseVersion == ErrValidateHeaderForIncrementalReseed( &dbfilehdr ) );
    dbfilehdr.le_ulDaeUpdateMajor = ulDAEUpdateMajorMax;
    dbfilehdr.le_cbPageSize     = 4*1024;
}

#define ENABLE_INC_RESEED_TRACING 1

ERR ErrMakeIncReseedTracingNames(
    __in IFileSystemAPI* pfsapi,
    __in JET_PCWSTR szDatabase,
    __in_range(cbOSFSAPI_MAX_PATHW, cbOSFSAPI_MAX_PATHW) ULONG cbIrsRawPath,
    __out_bcount_z(cbIrsRawPath) WCHAR * wszIrsRawPath,
    __in_range(cbOSFSAPI_MAX_PATHW, cbOSFSAPI_MAX_PATHW) ULONG cbIrsRawBackupPath,
    __out_bcount_z(cbIrsRawBackupPath) WCHAR * wszIrsRawBackupPath )
{
    ERR err = JET_errSuccess;

    WCHAR * szIncReseedRawExt = L".IRS.RAW";
    WCHAR * szIncReseedRawBackupExt = L".IRS.RAW.Prev";

    // init out params

    wszIrsRawPath[0] = L'\0';
    wszIrsRawBackupPath[0] = L'\0';

    Call( ErrOSStrCbCopyW( wszIrsRawPath, cbIrsRawPath, szDatabase ) );
    Call( ErrOSStrCbAppendW( wszIrsRawPath, cbIrsRawPath, szIncReseedRawExt ) );

    Call( ErrOSStrCbCopyW( wszIrsRawBackupPath, cbIrsRawBackupPath, szDatabase ) );
    Call( ErrOSStrCbAppendW( wszIrsRawBackupPath, cbIrsRawBackupPath, szIncReseedRawBackupExt ) );

HandleError:

    return err;
}

ERR ErrBeginDatabaseIncReseedTracing_( _In_ IFileSystemAPI * pfsapi, _In_ JET_PCWSTR szDatabase, _Out_ CPRINTF ** ppcprintf )
{
    ERR err = JET_errSuccess;

    WCHAR wszIrsRawFile[ IFileSystemAPI::cchPathMax ]   = { 0 };
    WCHAR wszIrsRawBackupFile[ IFileSystemAPI::cchPathMax ] = { 0 };

    IFileAPI * pfapiSizeCheck = NULL;

    //  initialize to NULL tracer, in case we fail ...

    *ppcprintf = CPRINTFNULL::PcprintfInstance();

#ifndef ENABLE_INC_RESEED_TRACING
    return JET_errSuccess;
#endif

    //  validate args

    if ( !szDatabase )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( !szDatabase[ 0 ] )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  create the tracing path names

    Call( ErrMakeIncReseedTracingNames( pfsapi, szDatabase, sizeof(wszIrsRawFile), wszIrsRawFile, sizeof(wszIrsRawBackupFile), wszIrsRawBackupFile ) );

    //  next we rotate the laundry if necessary ...

    err = pfsapi->ErrFileOpen( wszIrsRawFile, IFileAPI::fmfNone, &pfapiSizeCheck );
    if ( JET_errSuccess == err )
    {
        QWORD cbSize;
        Call( pfapiSizeCheck->ErrSize( &cbSize, IFileAPI::filesizeLogical ) );
        delete pfapiSizeCheck;  // delete here b/c we may be about rename it
        pfapiSizeCheck = NULL;

        if ( cbSize > ( 50 * 1024 * 1024 )  )
        {
            //  clear the dryer out

            Call( pfsapi->ErrFileDelete( wszIrsRawBackupFile ) );

            //  put the wash in the dryer

            Call( pfsapi->ErrFileMove( wszIrsRawFile, wszIrsRawBackupFile ) );
        }
    }

    //  create the tracing file

    CPRINTF * const pcprintfAlloc = new CPRINTFFILE( wszIrsRawFile );
    Alloc( pcprintfAlloc ); // avoid clobbering the default / NULL tracer

    //  set tracing to goto the tracing file

    *ppcprintf = pcprintfAlloc;

    //  start putting stuff in the washer

    (**ppcprintf)( "Please ignore this file.  This is a tracing file for incremental reseed information.  You can delete this file if it bothers you.\r\n" );

HandleError:

    if ( pfapiSizeCheck )
    {
        delete pfapiSizeCheck;
        pfapiSizeCheck = NULL;
    }

    return err;
}

ERR ErrBeginDatabaseIncReseedTracing( _In_ IFileSystemAPI * pfsapi, _In_ JET_PCWSTR wszDatabase, _Out_ CPRINTF ** ppcprintf )
{
    Assert( ppcprintf );
    if ( *ppcprintf && *ppcprintf != CPRINTFNULL::PcprintfInstance() )
    {
        AssertSz( fFalse, "Think this should no longer happen since we managed an IRS context through whole IRS process." );
        return JET_errSuccess;
    }

    const ERR err = ErrBeginDatabaseIncReseedTracing_( pfsapi, wszDatabase, ppcprintf );

    //  Should equal a new valid CPRINTFFILE or the global static CPRINTFNULL object.
    Assert( *ppcprintf != NULL );

    return err;
}

VOID TraceFuncBegun( CPRINTF * const pcprintf, const CHAR * const szFunction )
{
    const __int64               fileTime = UtilGetCurrentFileTime();
    WCHAR                       wszDate[32];
    WCHAR                       wszTime[32];
    size_t                      cchRequired;
    ErrUtilFormatFileTimeAsTimeWithSeconds( fileTime, wszTime, _countof(wszTime), &cchRequired );
    ErrUtilFormatFileTimeAsDate( fileTime, wszDate, _countof(wszDate), &cchRequired );
    (*pcprintf)( "Begin %hs().	Time %ws %ws\r\n", szFunction, wszTime, wszDate );
}

VOID TraceFuncComplete( CPRINTF * const pcprintf, const CHAR * const szFunction, const ERR err )
{
    __int64                 fileTime;
    WCHAR                   szDate[32];
    WCHAR                   szTime[32];
    size_t                  cchRequired;
    fileTime = UtilGetCurrentFileTime();
    ErrUtilFormatFileTimeAsTimeWithSeconds(
            fileTime,
            szTime,
            _countof(szTime),
            &cchRequired);
    ErrUtilFormatFileTimeAsDate(
            fileTime,
            szDate,
            _countof(szDate),
            &cchRequired);

    if ( err == JET_errSuccess )
    {
        (*pcprintf)( "End %s() returns, JET_errSuccess.  Time %ws %ws\r\n", szFunction, szTime, szDate );
    }
    else
    {
        //  print with last throw info if available
        (*pcprintf)( "End %s() returns, %d (from %s:%d).  Time %ws %ws\r\n", szFunction, err,
                    PefLastThrow() ? PefLastThrow()->SzFile() : "",
                    PefLastThrow() ? PefLastThrow()->UlLine() : 0,
                    szTime, szDate );
    }
}

VOID EndDatabaseIncReseedTracing( _Out_ CPRINTF ** ppcprintf )
{
    if ( *ppcprintf == NULL ||
            CPRINTFNULL::PcprintfInstance() == *ppcprintf )
    {
        *ppcprintf = NULL;
        return;
    }

    (**ppcprintf)( "Closing incremental reseed tracing file.\r\n" );

    delete *ppcprintf;
    *ppcprintf = NULL;
}

BOOL FIRSFindIrsContext( _In_ const INST * const pinst, const CIrsOpContext * const pirs )
{
    Assert( pirs );
    if ( pirs == NULL ) // just in case
    {
        return fFalse;
    }

    for( ULONG ipirs = 0; ipirs < dbidMax; ipirs++ )
    {
        if ( pinst->m_rgpirs[ipirs] == pirs )
            return fTrue;
    }
    return fFalse;
}

class CIrsOpContext : public CZeroInit
{
private:
    CPRINTF *                       m_pcprintfIncReSeedTrace;
    IFileAPI *                      m_pfapiDb;
    CFlushMapForUnattachedDb *      m_pfm;
    DBFILEHDR *                     m_pdbfilehdr;

    //  IRS Pgno Diagnostics (updated for proper / non-corruption IRS patching only).
    //      Note: Importantly we log IRS done event based upon m_cpgPatched != 0.
    CPG                             m_cpgPatched;
    PGNO                            m_pgnoMin;
    PGNO                            m_pgnoMax;
    TICK                            m_tickStart;
    TICK                            m_tickFirstPage;

    //  Note: This path is not necessarily a full path, and just used as the "key" to identify 
    //  the IRS context is matching the one the client originally begun.  Since the three IRS
    //  functions are generally used in one context, they will all pass the same path, and this
    //  non-absolute path is not a big deal.  Also if we ever want to support concurrent IRS
    //  operations, then we can use this to find the correct IRS context.
    WCHAR                           m_wszOriginalDatabasePath[IFileSystemAPI::cchPathMax];

private:
    CIrsOpContext() : CZeroInit( sizeof( CIrsOpContext ) ) {}   // NO one should use this one!
public:

    CIrsOpContext( _In_ CPRINTF * pcprintf, _In_ PCWSTR szOriginalDatabasePath, _In_ IFileAPI * const pfapiDb, CFlushMapForUnattachedDb * const pfm, DBFILEHDR * pdbfilehdr ) :
        CZeroInit( sizeof( CIrsOpContext ) )
    {
        //  Note don't need to check return, because ErrCheckContext() does it for us, right after .ctor.
        OSStrCbCopyW( m_wszOriginalDatabasePath, sizeof( m_wszOriginalDatabasePath ), szOriginalDatabasePath );
        m_pcprintfIncReSeedTrace = pcprintf;
        m_pfapiDb = pfapiDb;
        m_pfm = pfm;
        m_pdbfilehdr = pdbfilehdr;
        m_tickStart = TickOSTimeCurrent();
        m_pgnoMin = pgnoMax;

        Assert( PcprintfTrace() == pcprintf );
        Assert( PfapiDb() == pfapiDb );
        Assert( Pfm() == pfm );
        Assert( Pdbfilehdr() == pdbfilehdr );
    }

    ERR ErrCheckAttachedIrsContext( const INST * const pinst, PCWSTR wszOriginalDatabasePath )
    {
        ERR err = JET_errSuccess;

        //  Test that valid / current IRS operational context

        if ( !FIRSFindIrsContext( pinst, this ) )
        {
            AssertSz( fFalse, "Impossible the outer ErrIRSGetAttachedIrsContext below should've caught this case, is code calling CIrsOpContext::ErrCheck... direclty?  Or have we called an IRS on a 2nd / different instance?" );
            Error( ErrERRCheck( JET_errDatabaseInvalidIncrementalReseed ) );
        }

        //  Check DB path as "key" for operation.

        if ( !wszOriginalDatabasePath )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }

        if ( 0 != wcscmp( m_wszOriginalDatabasePath, wszOriginalDatabasePath ) )
        {
            //  We do not log this, because we could be searching through all pinst->m_rgpirs[]s for the
            //  context matching the specified DB path.
            Error( ErrERRCheck( JET_errDatabaseNotFound ) );
        }

        //  
        //  Double Check DBHFILEHDR matches (not strictly necessary)
        //
        DBFILEHDR * pdbfilehdrCheck = NULL;
        Alloc( pdbfilehdrCheck = (DBFILEHDR*)PvOSMemoryPageAlloc( g_cbPage, NULL ) );
        err = ErrUtilReadShadowedHeader(    pinst,
                                            pinst->m_pfsapi,
                                            m_pfapiDb,
                                            (BYTE*)pdbfilehdrCheck,
                                            g_cbPage,
                                            OffsetOf( DBFILEHDR, le_cbPageSize ) );
        if ( FErrIsDbHeaderCorruption( err ) || JET_errFileIOBeyondEOF == err )
        {
            (*PcprintfTrace())( "Translate Error %d\r\n", err );
            err = ErrERRCheck( JET_errDatabaseCorrupted );
        }
        if ( err >= JET_errSuccess &&
                //  Serious problem if the DBFILEHDR doesn't match the one we have 'attached' / cached for the IRS operation!
                0 != memcmp( pdbfilehdrCheck, m_pdbfilehdr, g_cbPage ) )
        {
            (*PcprintfTrace())( "DBFILEHDR was updated while we had DB opened exclusively for IRSv2!\r\n" );
            AssertSz( fFalse, "Huh!?  Even though DB should have been open and attached the whole time, the current 'fake IRS attached' in memory header should definitely match the on on disk!  Something is horribly wrong." );
            err = ErrERRCheck( JET_errDatabaseInvalidIncrementalReseed );
        }
        OSMemoryPageFree( pdbfilehdrCheck );
        Call( err ); // for ErrUtilReadShadowedHeader + header validate / checksum.
        
        //
        //  Check all expected handles are open
        //
        // Note: You can't check Pfm(), because you may have loaded a DB without a .jfm file.
        if ( PcprintfTrace() == NULL ||
                PfapiDb() == NULL ||
                Pdbfilehdr() == NULL )
        {
            (*PcprintfTrace())( "Missing IRS required handle: %p %p %p %p\r\n",
                PcprintfTrace(), PfapiDb(), Pfm(), Pdbfilehdr() );
            AssertSz( fFalse, "Missing IRS required handle.  Begin shouldn't have let this happen." );
            Error( ErrERRCheck( JET_errDatabaseInvalidIncrementalReseed ) );
        }

        //  NOTE: Specifically we don't trace anything to m_pcprintfIncReSeedTrace here because this is also
        //  used for ErrIsamPatchDatabasePages() where the specific trace is designed to be super small, and
        //  a single line per page.
        
    HandleError:
        return err;
    }

    ERR ErrFlushAttachedFiles( _In_ INST * pinst, _In_ const IOFLUSHREASON iofr )
    {
        ERR err = JET_errSuccess;

        (*PcprintfTrace())( "Flushing DB / JFM files.\r\n" );

        // This will be NULL if there is no .jfm file.
        if ( m_pfm )
        {
            Call( m_pfm->ErrFlushMapFlushFileBuffers( iofr ) );
        }

        Call( ErrUtilFlushFileBuffers( m_pfapiDb, iofr ) );

    HandleError:
        return err;
    }

    void UpdatePagePatchedStats( const PGNO pgnoStart, const PGNO pgnoEnd )
    {
        m_pgnoMin = min( m_pgnoMin, pgnoStart );
        m_pgnoMax = max( m_pgnoMax, pgnoEnd );
        m_cpgPatched += ( pgnoEnd - pgnoStart + 1 );
        if ( m_tickFirstPage == 0 )
        {
            m_tickFirstPage = TickOSTimeCurrent();
        }
    }

    typedef enum { eIrsDetachClean, eIrsDetachError } IRS_DETACH_STATE;
    void CloseIrsContext( _In_ const INST * const pinst, _In_ PCWSTR wszOriginalDatabasePath, _In_ const IRS_DETACH_STATE eStopState )
    {
        Assert( 0 == wcscmp( m_wszOriginalDatabasePath, wszOriginalDatabasePath ) );

        Assert( m_pfapiDb->CioNonFlushed() == 0 );  // client should have flushed.

        if ( eStopState == eIrsDetachClean )
        {
            if ( m_cpgPatched != 0 )
            {
                //  We have patched a page (not as a corruption) so log event that IRS is done.

                WCHAR wszCpgPatched[20], wszPgnoRange[100], wszTiming[80];
                WCHAR wszDatabasePath[OSFSAPI_MAX_PATH] = L"";

                const TICK dtickDurationTotal = DtickDelta( m_tickStart, TickOSTimeCurrent() );

                const BOOL fCompletePath = ( m_pfapiDb->ErrPath( wszDatabasePath ) >= JET_errSuccess );
                OSStrCbFormatW( wszCpgPatched, sizeof(wszCpgPatched), L"%u", m_cpgPatched);
                OSStrCbFormatW( wszPgnoRange, sizeof(wszPgnoRange), L"%u - %u", m_pgnoMin, m_pgnoMax );
                OSStrCbFormatW( wszTiming, sizeof(wszTiming), L"%0.03f", (double)dtickDurationTotal / 1000.0 );

                const WCHAR * rgsz[] = { fCompletePath ? wszDatabasePath : m_wszOriginalDatabasePath, wszCpgPatched, wszPgnoRange, wszTiming };
                
                UtilReportEvent(
                        eventInformation,
                        DATABASE_CORRUPTION_CATEGORY,
                        DATABASE_INCREMENTAL_RESEED_PATCH_COMPLETE_ID,
                        _countof(rgsz),
                        rgsz,
                        0,
                        NULL,
                        pinst,
                        JET_EventLoggingLevelLow );
            }
        }

        delete m_pfm;
        m_pfm = NULL;
        OSMemoryPageFree( m_pdbfilehdr );
        m_pdbfilehdr = NULL;
        delete m_pfapiDb;
        m_pfapiDb = NULL;
        EndDatabaseIncReseedTracing( &m_pcprintfIncReSeedTrace );
        Assert( m_pcprintfIncReSeedTrace == NULL );
    }


    CPRINTF * PcprintfTrace() const             { return m_pcprintfIncReSeedTrace; }
    IFileAPI * PfapiDb() const                  { return m_pfapiDb; }
    CFlushMapForUnattachedDb *  Pfm() const     { return m_pfm; }
    DBFILEHDR * Pdbfilehdr() const              { return m_pdbfilehdr; }

    PGNO PgnoMax() const                        { return m_pgnoMax; }
};

ERR ErrIRSGetAttachedIrsContext( _In_ const INST * const pinst, _In_ PCWSTR wszOriginalDatabasePath, _Out_ CIrsOpContext ** ppirs )
{
    CIrsOpContext * pirsFound = NULL;

    Assert( ppirs );
    
    if ( pinst->m_rgpirs == NULL )
    {
        AssertSz( FNegTest( fInvalidAPIUsage ), "The client has called one of the later IRS funcs (Patch or End) with a pinst that has not had Begin called yet.  This is invalid, IRS context is not setup." );
        return ErrERRCheck( JET_errDatabaseInvalidIncrementalReseed );
    }

    for( ULONG ipirs = 0; ipirs < dbidMax; ipirs++ )
    {
        if ( pinst->m_rgpirs[ipirs] )
        {
            const ERR errCheck = pinst->m_rgpirs[ipirs]->ErrCheckAttachedIrsContext( pinst, wszOriginalDatabasePath );
            if ( errCheck == JET_errSuccess )
            {
                pirsFound = pinst->m_rgpirs[ipirs];
                break;
            }
            if ( errCheck == JET_errDatabaseNotFound )
            {
                //  Not the right DB
                continue;
            }
            ERR err;
            CallR( errCheck );
            AssertSz( fFalse, "Should be no warnings that fall through to ehre" );
        }
    }

    if ( pirsFound == NULL )
    {
        //  No IRS tracing context to complain on either! :)
        AssertSz( FNegTest( fInvalidAPIUsage ), "The client has called one of the later IRS funcs (Patch or End) with a pinst that has not had Begin called yet (or they provided a different DB name).  This is invalid, IRS context is not setup." );
        return ErrERRCheck( JET_errDatabaseInvalidIncrementalReseed );
    }

    Assert( FIRSFindIrsContext( pinst, pirsFound ) );

    *ppirs = pirsFound;

    return JET_errSuccess;
}


//  Note this doesn't perform a real attach, too much non-IRS relevant checks in there, but performs
//  an "attach for IRS purposes" let's say, setting up the INST's CIrsOpContext / m_rgpirs[x] with
//  all the DB, flushmap, tracing handles open, and a few common resources like DB header.
ERR ErrIRSAttachDatabaseForIrsV2( _Inout_ INST * const pinst, _In_ PCWSTR wszDatabase )
{
    ERR                         err = JET_errSuccess;
    CPRINTF *                   pcprintfIncReSeedTrace = NULL;
    IFileAPI*                   pfapiDb         = NULL;
    DBFILEHDR*                  pdbfilehdr      = NULL;
    CFlushMapForUnattachedDb*   pfm             = NULL;
    ULONG                       ipirsAvailable  = ulMax;
    CIrsOpContext *             pirsCheck       = NULL;

    //  start tracing (before anything else)
    //
    Call( ErrBeginDatabaseIncReseedTracing( pinst->m_pfsapi, wszDatabase, &pcprintfIncReSeedTrace ) );

    TraceFuncBegun( pcprintfIncReSeedTrace, __FUNCTION__ );

    //  validate our parameters
    //
    if ( !wszDatabase )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( !wszDatabase[ 0 ] )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  open the requested database
    //
    Call( pinst->m_pfsapi->ErrFileOpen( wszDatabase, IFileAPI::fmfNone, &pfapiDb ) );

    //  get some other resources together
    Alloc( pdbfilehdr = (DBFILEHDR*)PvOSMemoryPageAlloc( g_cbPage, NULL ) );

    WCHAR wszDatabasePath[OSFSAPI_MAX_PATH] = L"";
    if ( pfapiDb->ErrPath( wszDatabasePath ) >= JET_errSuccess )
    {
        (*pcprintfIncReSeedTrace)( "Database File: %ws\r\n", wszDatabasePath );
    }
    else
    {
        (*pcprintfIncReSeedTrace)( "Database File: %ws (note: Couldn't retrieve full path!)\r\n", wszDatabase );
    }

    //  read in the header of this database
    //
    err = ErrUtilReadShadowedHeader(    pinst,
                                        pinst->m_pfsapi,
                                        pfapiDb,
                                        (BYTE*)pdbfilehdr,
                                        g_cbPage,
                                        OffsetOf( DBFILEHDR, le_cbPageSize ) );

    if ( FErrIsDbHeaderCorruption( err ) || JET_errFileIOBeyondEOF == err )
    {
        (*pcprintfIncReSeedTrace)( "Translate Error %d\r\n", err );
        err = ErrERRCheck( JET_errDatabaseCorrupted );
    }
    Call( err );

    //  initialize persisted flush map
    //
    Call( CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDatabase, pdbfilehdr, pinst, &pfm ) );

    if ( pinst->m_rgpirs == NULL )
    {
        const size_t cbRgpirs = sizeof(CIrsOpContext*) * dbidMax;
        Expected( cbRgpirs == 28 || cbRgpirs == 56 );
        Alloc( pinst->m_rgpirs = (CIrsOpContext**)PvOSMemoryHeapAlloc( cbRgpirs ) );
        memset( pinst->m_rgpirs, 0, cbRgpirs );
    }

    for( ULONG ipirs = 0; ipirs < dbidMax; ipirs++ )
    {
        if ( pinst->m_rgpirs[ipirs] != NULL )
        {
            const ERR errCheck = pinst->m_rgpirs[ipirs]->ErrCheckAttachedIrsContext( pinst, wszDatabase );
            if ( errCheck == JET_errSuccess )
            {
                AssertSz( fFalse, "Should be impossible, as we opened the DB above, so with competing calls one will win the file lock." );
                Error( ErrERRCheck( JET_errDatabaseDuplicate ) );
            }
        }
        else if ( ipirsAvailable == ulMax )
        {
            ipirsAvailable = ipirs;
        }
    }
    if ( ipirsAvailable == ulMax )
    {
        Error( ErrERRCheck( JET_errTooManyAttachedDatabases ) );
    }

    //  now initialize the IRS operation context (and check it) ...
    //
    Alloc( pinst->m_rgpirs[ipirsAvailable] = new CIrsOpContext( pcprintfIncReSeedTrace, wszDatabase, pfapiDb, pfm, pdbfilehdr ) );

    CallS( ErrIRSGetAttachedIrsContext( pinst, wszDatabase, &pirsCheck ) );
    Assert( pirsCheck );
    Assert( pirsCheck == pinst->m_rgpirs[ipirsAvailable] );

    //  the IRS Context owns ALL these now ...
    pcprintfIncReSeedTrace = NULL;
    pfapiDb = NULL;
    pfm = NULL;
    pdbfilehdr = NULL;

    Assert( err >= JET_errSuccess );

HandleError:

    if ( err < JET_errSuccess )
    {
        //  if we managed to at least getting tracing up, trace the issue ... 
        if ( pcprintfIncReSeedTrace )
        {
            TraceFuncComplete( pcprintfIncReSeedTrace , __FUNCTION__, err );
        }

        //  failed ... cleanup all handles.
        delete pfm;
        OSMemoryPageFree( pdbfilehdr );
        delete pfapiDb;
        EndDatabaseIncReseedTracing( &pcprintfIncReSeedTrace );

        Assert( ipirsAvailable == ulMax ||
                pinst->m_rgpirs == NULL ||
                pinst->m_rgpirs[ipirsAvailable] == NULL );  // created as last thing, so if we failed should be NULL
    }
    else
    {
        //  success!
        Assert( pirsCheck );
        TraceFuncComplete( pirsCheck->PcprintfTrace(), __FUNCTION__, err );
    }
    return err;
}

ERR ErrIRSDetachDatabaseIrsHandles( _In_ INST * const pinst, _In_ CIrsOpContext * pirs, _In_ PCWSTR wszOriginalDatabasePath, _In_ const CIrsOpContext::IRS_DETACH_STATE eStopState )
{
    Assert( pirs );
    Assert( FIRSFindIrsContext( pinst, pirs ) );
    Assert( pirs->ErrCheckAttachedIrsContext( pinst, wszOriginalDatabasePath ) >= JET_errSuccess );

    if ( eStopState == CIrsOpContext::eIrsDetachClean )
    {
        ERR err = JET_errSuccess;
        CallR( pirs->ErrFlushAttachedFiles( pinst, iofrIncReseedUtil ) );
        (*pirs->PcprintfTrace())( "Completed IRSv2 Operations / and Flush!\r\n" );
    }
    else
    {
        Assert( eStopState == CIrsOpContext::eIrsDetachError );
        (void)pirs->ErrFlushAttachedFiles( pinst, iofrDefensiveCloseFlush );
        (*pirs->PcprintfTrace())( "Closing / cancelling IRSv2 operations!\r\n" );
    }

    for( ULONG ipirs = 0; ipirs < dbidMax; ipirs++ )
    {
        if ( pinst->m_rgpirs[ipirs] == pirs )
        {
            pinst->m_rgpirs[ipirs] = NULL;
        }
    }

    pirs->CloseIrsContext( pinst, wszOriginalDatabasePath, eStopState );
    delete pirs;

    Assert( !FIRSFindIrsContext( pinst, pirs ) );

    return JET_errSuccess;
}

void IRSCleanUpAllIrsResources( _In_ INST * const pinst )
{
    if ( pinst->m_rgpirs )
    {
        //  Someone HAS been playing around with IRS contexts! :)  Make sure they're all
        //  closed then remove the dbid to IRS context array / map.
        for( ULONG ipirs = 0; ipirs < dbidMax; ipirs++ )
        {
            if ( pinst->m_rgpirs[ipirs] )
            {
                AssertTrack( fFalse, "IrsContextNotCleanedUp" );
                CIrsOpContext * const pirs = pinst->m_rgpirs[ipirs];
                CallS( ErrIRSDetachDatabaseIrsHandles( pinst, pirs, NULL, CIrsOpContext::eIrsDetachError ) );
                Assert( FIRSFindIrsContext( pinst, pirs ) );
            }
        }

        OSMemoryHeapFree( pinst->m_rgpirs );
        pinst->m_rgpirs = NULL;
    }
}

#define IRSEnforce( pirs, check )   \
    if ( !( check ) )               \
    {                               \
        (*pirs->PcprintfTrace())( "Enforce (bad state during IRS): %hs (@ %hs : %d)\r\n", #check, __FILE__, __LINE__ ); \
        Enforce( check );           \
    }

//  If we ever need extra performance out of IncReSeed (or passive page patching), here is a list of low
//  hanging notable inefficiencies / improvements:
//   1. At HA level, they are passing 1 page at a time to JetPatchDatabasePages(), and so we do one patch
//      IO at a time.  Fixing this to pass a full extent of contiguous pages at a time would reduce IO by 
//      approximate the average run length of contiguous pages.
//   2. For each page we patch, we immediately patch and write out the corresponding flushmap(FM) page,
//      but since MANY regular DB pages map to 1 FM page, we could either just leave page dirty in the FM
//      and then write all FM pages in EndDatabaseIncrementalReseed (most efficient), or since the pages 
//      actually come in ascending order we could track the last updated FM page, and write the FM page, 
//      once a new page patch "switches to updating a new FM page" (nearly as efficient).
//   3. We could move to async IO for the page patch operation, and just allow ourselves to have one to
//      five or so concurrently outstanding.  Then make sure EndDatabaseIncrementalReseed waits for all
//      concurrent IOs to complete before flushing.  And of course have to balance IO load with other DBs 
//      needs on same disk.
//  I suspect these three improvements would make the IRS process lightning fast compared to before, but
//  today it doesn't seem to be a long pole for anybody.

ERR ErrIsamBeginDatabaseIncrementalReseed(
    __in JET_INSTANCE   jinst,
    __in JET_PCWSTR     szDatabase,
    __in JET_GRBIT      grbit )
{
    ERR                     err             = JET_errSuccess;
    INST* const             pinst           = (INST *)jinst;
    CIrsOpContext *         pirs            = NULL;
    CPRINTF *               pcprintfIncReSeedTrace = NULL;

    //  Establish connected DB, Flushmap, etc form inst + database name
    //

    Call( ErrIRSAttachDatabaseForIrsV2( pinst, szDatabase ) );

    CallS( ErrIRSGetAttachedIrsContext( pinst, szDatabase, &pirs ) );

    TraceFuncBegun( pirs->PcprintfTrace(), __FUNCTION__ );  // must be after ErrIRSAttachDatabaseForIrsV2() - so trace file initialized.

    DBFILEHDR * pdbfilehdr = pirs->Pdbfilehdr();
    pcprintfIncReSeedTrace = pirs->PcprintfTrace();
    Assert( pdbfilehdr && pcprintfIncReSeedTrace );

    //  Validate remaining args
    //

    if ( grbit )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    (*pcprintfIncReSeedTrace)( "The database header at beginning of BeginIncReseed:\r\n" );
    pdbfilehdr->DumpLite( pcprintfIncReSeedTrace, "\r\n" );

    //  validate the header
    //
    Call( ErrValidateHeaderForIncrementalReseed( pdbfilehdr ) );

    //  check the database state
    //
    if ( pdbfilehdr->Dbstate() == JET_dbstateDirtyShutdown ||
            pdbfilehdr->Dbstate() == JET_dbstateDirtyAndPatchedShutdown )
    {
        //  action must be taken
    }
    else if ( pdbfilehdr->Dbstate() == JET_dbstateIncrementalReseedInProgress )
    {
        //  no action will be taken
        Error( JET_errSuccess );
    }
    else
    {
        (*pcprintfIncReSeedTrace)( "Error: Bad dbstate, pdbfilehdr->Dbstate() = %d\r\n", (ULONG)(pdbfilehdr->Dbstate()) );
        Error( ErrERRCheck( JET_errDatabaseInvalidIncrementalReseed ) );
    }

    //  update the cached header to reflect that we are now in the incremental reseed in progress state
    //
    pdbfilehdr->SetDbstate( JET_dbstateIncrementalReseedInProgress, lGenerationInvalid, lGenerationInvalid, NULL, fTrue );
    pdbfilehdr->le_ulIncrementalReseedCount++;
    LGIGetDateTime( &pdbfilehdr->logtimeIncrementalReseed );

    //  write the header back to the database
    //
    Call( ErrUtilWriteUnattachedDatabaseHeaders( pinst, pinst->m_pfsapi, szDatabase, pdbfilehdr, pirs->PfapiDb(), pirs->Pfm() ) );
    Assert( pdbfilehdr == pirs->Pdbfilehdr() );

    Call( pirs->ErrFlushAttachedFiles( pinst, iofrIncReseedUtil ) );

HandleError:

    if ( pcprintfIncReSeedTrace )
    {
        TraceFuncComplete( pcprintfIncReSeedTrace, __FUNCTION__, err );
    }

    if ( err >= JET_errSuccess )
    {
        //  we will probably continue to patch database pages, so print out the headers ...
        Assert( pcprintfIncReSeedTrace );
        (*pcprintfIncReSeedTrace)( "Page Patch Records (all numbers in hex): P,p=<Pgno>,i[nitialPageImage]:<objid>,<dbtime>,a[fterPatchedImage]:<objid>,<dbtime>,<errOfPatchOp>\r\n", err );
    }
    else
    {
        if ( pirs )
        {
            //  Can't do anything about the failure at this point.
            CallS( ErrIRSDetachDatabaseIrsHandles( pinst, pirs, szDatabase, CIrsOpContext::eIrsDetachError ) );
            Assert( !FIRSFindIrsContext( pinst, pirs ) );
        }
    }

    return err;
}

LOCAL ERR ErrReadLogFileHeader(
    __in INST*              pinst,
    __in const TraceContext&    tc,
    __in ULONG      gen,
    __out IFileAPI** const  ppfapi,
    __out LGFILEHDR** const pplgfilehdr )
{
    ERR                     err                                     = JET_errSuccess;
    IFileSystemAPI* const   pfsapi                                  = pinst->m_pfsapi;
    WCHAR                   szLogFile[ IFileSystemAPI::cchPathMax ] = { 0 };
    IFileAPI*               pfapiT                                  = NULL;
    IFileAPI*&              pfapi                                   = ppfapi ? *ppfapi : pfapiT;
                            pfapi                                   = NULL;
    LGFILEHDR*              plgfilehdrT                             = NULL;
    LGFILEHDR*&             plgfilehdr                              = pplgfilehdr ? *pplgfilehdr : plgfilehdrT;
                            plgfilehdr                              = NULL;
    
    Call( LGFileHelper::ErrLGMakeLogNameBaselessEx( szLogFile,
                                            sizeof( szLogFile ),
                                            pfsapi,
                                            SzParam( pinst, JET_paramLogFilePath ),
                                            SzParam( pinst, JET_paramBaseName ),
                                            eArchiveLog,
                                            gen,
                                            ( UlParam( pinst, JET_paramLegacyFileNames ) & JET_bitESE98FileNames ) ? wszOldLogExt : wszNewLogExt,
                                            ( UlParam( pinst, JET_paramLegacyFileNames ) & JET_bitEightDotThreeSoftCompat ) ? 0 : 8 ) );
    err = pfsapi->ErrFileOpen(  szLogFile,
                                (   IFileAPI::fmfReadOnly |
                                    ( BoolParam( JET_paramEnableFileCache ) ?
                                        IFileAPI::fmfCached :
                                        IFileAPI::fmfNone ) ),
                                &pfapi );
    if ( JET_errFileNotFound == err )
    {
        Call( LGFileHelper::ErrLGMakeLogNameBaselessEx( szLogFile,
                                                sizeof( szLogFile ),
                                                pfsapi,
                                                SzParam( pinst, JET_paramLogFilePath ),
                                                SzParam( pinst, JET_paramBaseName ),
                                                eCurrentLog,
                                                0,
                                                ( UlParam( pinst, JET_paramLegacyFileNames ) & JET_bitESE98FileNames ) ? wszOldLogExt : wszNewLogExt,
                                                ( UlParam( pinst, JET_paramLegacyFileNames ) & JET_bitEightDotThreeSoftCompat ) ? 0 : 8 ) );
        Call( pfsapi->ErrFileOpen(  szLogFile,
                                    (   IFileAPI::fmfReadOnly |
                                        ( BoolParam( JET_paramEnableFileCache ) ?
                                            IFileAPI::fmfCached :
                                            IFileAPI::fmfNone ) ),
                                    &pfapi ) );
    }
    Call( err );
    Alloc( plgfilehdr = (LGFILEHDR*)PvOSMemoryPageAlloc( sizeof( LGFILEHDR ), NULL ) );
    Call( ErrLGIReadFileHeader( pfapi, tc, QosSyncDefault( pinst ), plgfilehdr ) );

HandleError:
    if ( err < JET_errSuccess )
    {
        delete pfapi;
        pfapi = NULL;
        OSMemoryPageFree( plgfilehdr );
        plgfilehdr = NULL;
    }
    delete pfapiT;
    OSMemoryPageFree( plgfilehdrT );
    return err;
}

ERR ErrIncrementalReseedFindValidAttachInfo(
    CIrsOpContext * const       pirs,
    const LGFILEHDR * const     plgfilehdr,
    const DBFILEHDR * const     pdbfilehdr,
    __in ULONG          genFirstDivergedLog,
    ATTACHINFO * const          pattachinfoOut
    )
{
    ERR                     err                 = JET_errSuccess;
    const ATTACHINFO *      pattachinfoT        = NULL;
    const ATTACHINFO *      pattachinfo         = NULL;

    //  find the attach info for this database in the min required log header.  if it doesn't exist or
    //  it is for a version of this database in the future then we must fail the incremental reseed
    //
    for ( const BYTE * pbT = plgfilehdr->rgbAttach; 0 != *pbT; pbT += sizeof( ATTACHINFO ) + pattachinfoT->CbNames() )
    {
        AssertPREFIX( pbT - plgfilehdr->rgbAttach < cbAttach );
        pattachinfoT = (ATTACHINFO*)pbT;
        
        if ( !memcmp( &pattachinfoT->signDb, &pdbfilehdr->signDb, sizeof( pattachinfoT->signDb ) ) &&
             CmpLgpos( &pattachinfoT->le_lgposAttach, &pdbfilehdr->le_lgposAttach ) <= 0 &&
             ( genFirstDivergedLog == 0 ||
               pattachinfoT->le_lgposAttach.le_lGeneration < (LONG)genFirstDivergedLog ) )
        {
            pattachinfo = pattachinfoT;
            break;
        }
    }

    if ( !pattachinfo )
    {
        if ( !pattachinfoT )
        {
            (*pirs->PcprintfTrace())( "Error: No attachment info(lgen=%d): %I64x, %I64x\r\n",
                        (ULONG)plgfilehdr->lgfilehdr.le_lGeneration,
                        (__int64)pattachinfoT, (__int64)pattachinfo );
            //  we call out this sub case to make it easier to spot this expected case
            Error( ErrERRCheck( JET_errNoAttachmentsFailedIncrementalReseed ) );
        }

        (*pirs->PcprintfTrace())( "Error: No attachment info(lgen=%d): %I64x\r\n",
                        (ULONG)plgfilehdr->lgfilehdr.le_lGeneration, (__int64)pattachinfo );
        Error( ErrERRCheck( JET_errDatabaseFailedIncrementalReseed ) );
    }

    // Note this doesn't get the szNames ...
    memcpy( (void*)pattachinfoOut, (void*)pattachinfo, sizeof(ATTACHINFO) );
    pattachinfoOut->SetCbNames( 0 );

HandleError:

    return err;
}

ERR ErrIsamEndDatabaseIncrementalReseed(
    __in JET_INSTANCE   jinst,
    __in JET_PCWSTR     szDatabase,
    __in ULONG  genMinRequired,
    __in ULONG  genFirstDivergedLog,
    __in ULONG  genMaxRequired,
    __in JET_GRBIT      grbit )
{
    ERR                     err                                             = JET_errSuccess;
    INST* const             pinst                                           = (INST *)jinst;
    IFileSystemAPI* const   pfsapi                                          = pinst->m_pfsapi;
    CIrsOpContext *         pirs                                            = NULL;
    LGFILEHDR*              plgfilehdrMin                                   = NULL;
    LONG                    genLast                                         = 0;
    SIGNATURE               signLogLast                                     = { 0 };
    LOGTIME                 tmCreateLast                                    = { 0 };
    IFileAPI*               pfapiLogFileMax                                 = NULL;
    LGFILEHDR*              plgfilehdrMax                                   = NULL;
    LGFILEHDR*              plgfilehdrCheck                                 = NULL;
    WCHAR                   szLogFileCheck[ IFileSystemAPI::cchPathMax ]    = { 0 };
    IFileAPI*               pfapiLogFileCheck                               = NULL;
    ATTACHINFO              attachinfo;
    WCHAR                   szCheckpointFile[ IFileSystemAPI::cchPathMax ]  = { 0 };
    IFileAPI*               pfapiCheckpoint                                 = NULL;
    CHECKPOINT*             pcheckpoint                                     = NULL;
    ULONG                   lGen;

    memset( &attachinfo, 0, sizeof(attachinfo) );

    //  validate generic parameters
    //
    Call( ErrIRSGetAttachedIrsContext( pinst, szDatabase, &pirs ) );
    Assert( pirs );

    DBFILEHDR *             pdbfilehdr                                      = pirs->Pdbfilehdr();

    //  start tracing (before anything else)
    //
    TraceFuncBegun( pirs->PcprintfTrace(), __FUNCTION__ );

    // split off cancelling the IRS operation
    if ( grbit == JET_bitEndDatabaseIncrementalReseedCancel )
    {
        if ( genMinRequired != 0 ||
                genFirstDivergedLog != 0 ||
                genMaxRequired != 0 )
        {
            //  For cancel, none of these args make sense yet.
            AssertSz( FNegTest( fInvalidAPIUsage ), "Client is testing our patience.  Get it right." );
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }

        //  For now, cancelling the IRS operation simply involves closing the context with all the
        //  open handles.

        if ( pirs )
        {
            CallS( ErrIRSDetachDatabaseIrsHandles( pinst, pirs, szDatabase, CIrsOpContext::eIrsDetachError ) );
            Assert( !FIRSFindIrsContext( pinst, pirs ) );
        }
        return JET_errSuccess;
    }

    //  validate End operation specific parameters
    //

    if ( genMinRequired < 1 )
    {
        AssertSz( FNegTest( fInvalidAPIUsage ), "This shouldn't happen, the client has done something really wrong to pass in a genMinRequired(%d) less than 1", genMinRequired );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( genMaxRequired >= (ULONG)lgposMax.lGeneration )
    {
        AssertSz( FNegTest( fInvalidAPIUsage ), "This shouldn't happen, the client has done something really wrong to pass in a genMaxRequired(%d) greater than lgposMax", genMaxRequired );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( genMinRequired > genMaxRequired )
    {
        AssertSz( FNegTest( fInvalidAPIUsage ), "This shouldn't happen, the client has done something really wrong to pass in a genMinRequired(%d) greater than the genMaxRequired(%d)", genMinRequired, genMaxRequired );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( genMinRequired > genFirstDivergedLog &&
        // passive page patching doesn't have a divergent log, so we must handle zero
        genFirstDivergedLog != 0 )
    {
        AssertSz( FNegTest( fInvalidAPIUsage ), "This shouldn't happen, the client has done something really wrong to pass in a genMinRequired(%d) greater than the genFirstDivergedLog(%d)", genMinRequired, genFirstDivergedLog );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( genFirstDivergedLog > genMaxRequired )
    {
        AssertSz( FNegTest( fInvalidAPIUsage ), "This shouldn't happen, the client has done something really wrong to pass in a genMaxRequired(%d) less than the genFirstDivergedLog(%d)", genMaxRequired, genFirstDivergedLog );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( grbit )
    {
        //  JET_bitEndDatabaseIncrementalReseedCancel is handled above, anything else unexpected
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    // If this is just a passive page patch (not a real inc-reseed), do not allow required range to go backwards.
    if ( genFirstDivergedLog == 0 )
    {
        genMaxRequired = max( genMaxRequired, (ULONG)pdbfilehdr->le_lGenMaxRequired );
    }

    //  allocate some memory up front
    //
    Alloc( plgfilehdrCheck = (LGFILEHDR*)PvOSMemoryPageAlloc( sizeof( LGFILEHDR ), NULL ) );

    //  flush any deferred writes from the patching of the actual pages (and flush map)
    //

    Call( pirs->ErrFlushAttachedFiles( pinst, iofrIncReseedUtil ) );

    //  dump the header for the beginning of end inc reseed
    //

    (*pirs->PcprintfTrace())( "The database header at beginning of EndIncReseed:\r\n" );
    pdbfilehdr->DumpLite( pirs->PcprintfTrace(), "\r\n" );

    //  revalidate the header (just for insurance, should be impossible to get here with unvalidated
    //  header because we have to get here through Jet/ErrIsamBeginDatabaseIncrementalReseed()).
    //
    IRSEnforce( pirs, ErrValidateHeaderForIncrementalReseed( pdbfilehdr ) >= JET_errSuccess );
    IRSEnforce( pirs, pdbfilehdr->Dbstate() == JET_dbstateIncrementalReseedInProgress );
    
    //  analyze log gens
    //
    (*pirs->PcprintfTrace())( "Log Generations [min(%d Passed, %d DB) - Div %d - Max %d\r\n",
            genMinRequired, (LONG)(pdbfilehdr->le_lGenMinRequired), genFirstDivergedLog, genMaxRequired );

    // The new genMinRequired should be a min of the current genMin and the one passed by the user (moving it up will break recovery).
    // The genMaxRequired can be truncated in case of a lossy failover and the subsequent reseed.
    // The reseed must guarantee that any log recs in the truncated range that are committed to the database are undone.
    // Otherwise, recovery will leave the database in a corrupted state.
    genMinRequired = min( (LONG) genMinRequired, pdbfilehdr->le_lGenMinRequired );

    // if we have the DB attach higher than the divergence ... this means that
    // we will want to find the highest log with valid/acceptable attach info,
    // just in case, no log past the lgenMinRequired has acceptable attach info.
    BOOL fFindLowerAttachInfo = ( genFirstDivergedLog == 0 || pdbfilehdr->le_lgposAttach.le_lGeneration >= (LONG)genFirstDivergedLog );

    // Are we forcing looking through lower gen logs?
    BOOL fForceLowerAttachInfo = (BOOL)UlConfigOverrideInjection( 38361, fFalse );

    ULONG genMinAttachInfo = 0; // will become the genMinRequired in a pinch ...
    ERR errFindAttach = errNotFound;
    
RestartFromLowerLogGeneration:

    Assert( pdbfilehdr->le_lgposAttach.le_lGeneration < (LONG)genFirstDivergedLog ||    // we don't need to find lower attach info
            fFindLowerAttachInfo ||     // we're on first pass, so haven't updated genMinRequired
            //  Or finally ... we should have a genMinRequired equivalent to genMinAttachInfo.
            genMinRequired == genMinAttachInfo );
                                                    //
    //  verify that the proposed required log sequence is intact
    //
    for ( lGen = genMinRequired; ; lGen++ )
    {
        Call( LGFileHelper::ErrLGMakeLogNameBaselessEx(
                    szLogFileCheck,
                    sizeof( szLogFileCheck ),
                    pfsapi,
                    SzParam( pinst, JET_paramLogFilePath ),
                    SzParam( pinst, JET_paramBaseName ),
                    eArchiveLog,
                    lGen,
                    ( UlParam( pinst, JET_paramLegacyFileNames ) & JET_bitESE98FileNames ) ? wszOldLogExt : wszNewLogExt,
                    ( UlParam( pinst, JET_paramLegacyFileNames ) & JET_bitEightDotThreeSoftCompat ) ? 0 : 8 ) );
        err = pfsapi->ErrFileOpen(  szLogFileCheck,
                                    (   IFileAPI::fmfReadOnly |
                                        ( BoolParam( JET_paramEnableFileCache ) ?
                                            IFileAPI::fmfCached :
                                            IFileAPI::fmfNone ) ),
                                    &pfapiLogFileCheck );
        if ( JET_errFileNotFound == err )
        {
            break;
        }
        Call( err );
        Call( ErrLGIReadFileHeader( pfapiLogFileCheck, *TraceContextScope( iorpPatchFix ), QosSyncDefault( pinst ), plgfilehdrCheck ) );

        if ( plgfilehdrCheck->lgfilehdr.le_lGeneration != (LONG)lGen )
        {
            (*pirs->PcprintfTrace())( "Error: plgfilehdrCheck->lgfilehdr.le_lGeneration = %d != lGen = %d\r\n",
                    (ULONG)plgfilehdrCheck->lgfilehdr.le_lGeneration, lGen );
            Error( ErrERRCheck( JET_errLogFileCorrupt ) );
        }

        if ( lGen == genMinRequired )
        {
            Alloc( plgfilehdrMin = (LGFILEHDR*)PvOSMemoryPageAlloc( sizeof( LGFILEHDR ), NULL ) );
            UtilMemCpy( plgfilehdrMin, plgfilehdrCheck, sizeof( LGFILEHDR ) );
        }
        else
        {
            if ( memcmp( &plgfilehdrCheck->lgfilehdr.signLog, &signLogLast, sizeof( plgfilehdrCheck->lgfilehdr.signLog ) ) )
            {
                (*pirs->PcprintfTrace())( "Error: plgfilehdrCheck(lgen=%d)->lgfilehdr.signLog = ",
                        (ULONG)plgfilehdrCheck->lgfilehdr.le_lGeneration );
                DUMPPrintSig( pirs->PcprintfTrace(), &plgfilehdrCheck->lgfilehdr.signLog );
                (*pirs->PcprintfTrace())( " != signLogLast = " );
                DUMPPrintSig( pirs->PcprintfTrace(), &signLogLast );
                (*pirs->PcprintfTrace())( "\r\n" );
                Error( ErrERRCheck( JET_errBadLogSignature ) );
            }
            if ( memcmp( &plgfilehdrCheck->lgfilehdr.tmPrevGen, &tmCreateLast, sizeof( plgfilehdrCheck->lgfilehdr.tmPrevGen ) ) )
            {
                (*pirs->PcprintfTrace())( "Error: plgfilehdrCheck(lgen=%d)->lgfilehdr.tmPrevGen = ",
                        (ULONG)plgfilehdrCheck->lgfilehdr.le_lGeneration );
                DUMPPrintLogTime( pirs->PcprintfTrace(), &plgfilehdrCheck->lgfilehdr.tmPrevGen );
                (*pirs->PcprintfTrace())( " != tmCreateLast = " );
                DUMPPrintLogTime( pirs->PcprintfTrace(), &tmCreateLast );
                (*pirs->PcprintfTrace())( "\r\n" );
                Error( ErrERRCheck( JET_errInvalidLogSequence ) );
            }
        }
        if ( lGen == genMaxRequired )
        {
            Alloc( plgfilehdrMax = (LGFILEHDR*)PvOSMemoryPageAlloc( sizeof( LGFILEHDR ), NULL ) );
            UtilMemCpy( plgfilehdrMax, plgfilehdrCheck, sizeof( LGFILEHDR ) );
        }

        genLast = plgfilehdrCheck->lgfilehdr.le_lGeneration;
        UtilMemCpy( &signLogLast, &plgfilehdrCheck->lgfilehdr.signLog, sizeof( signLogLast ) );
        UtilMemCpy( &tmCreateLast, &plgfilehdrCheck->lgfilehdr.tmCreate, sizeof( tmCreateLast ) );

        if ( 0 == attachinfo.le_lgposAttach.le_lGeneration )
        {
            errFindAttach = ErrIncrementalReseedFindValidAttachInfo( pirs, plgfilehdrCheck, pdbfilehdr, genFirstDivergedLog, &attachinfo );
        }

        delete pfapiLogFileCheck;
        pfapiLogFileCheck = NULL;

    }

    if ( ( 0 == attachinfo.le_lgposAttach.le_lGeneration &&
           fFindLowerAttachInfo ) ||
         fForceLowerAttachInfo )
    {
        for ( lGen = genMinRequired - 1; lGen > 0; lGen-- )
        {
            //  so we have the DB attach higher than the divergence ... this means that we will want
            //  find the highest log with valid / acceptable attach info, just in case, no log past
            //  the lgenMinRequired has acceptable attach info.

            Call( LGFileHelper::ErrLGMakeLogNameBaselessEx(
                    szLogFileCheck,
                    sizeof( szLogFileCheck ),
                    pfsapi,
                    SzParam( pinst, JET_paramLogFilePath ),
                    SzParam( pinst, JET_paramBaseName ),
                    eArchiveLog,
                    lGen,
                    ( UlParam( pinst, JET_paramLegacyFileNames ) & JET_bitESE98FileNames ) ? wszOldLogExt : wszNewLogExt,
                    ( UlParam( pinst, JET_paramLegacyFileNames ) & JET_bitEightDotThreeSoftCompat ) ? 0 : 8 ) );
            err = pfsapi->ErrFileOpen(  szLogFileCheck,
                                        (   IFileAPI::fmfReadOnly |
                                            ( BoolParam( JET_paramEnableFileCache ) ?
                                                IFileAPI::fmfCached :
                                                IFileAPI::fmfNone ) ),
                                        &pfapiLogFileCheck );
            if ( err == JET_errFileNotFound )
            {
                break;
            }
            Call( err );
            Call( ErrLGIReadFileHeader( pfapiLogFileCheck, *TraceContextScope( iorpPatchFix ), QosSyncDefault( pinst ), plgfilehdrCheck ) );

            if ( plgfilehdrCheck->lgfilehdr.le_lGeneration != (LONG)lGen )
            {
                (*pirs->PcprintfTrace())( "Error: plgfilehdrCheck->lgfilehdr.le_lGeneration = %d != lGen = %d\r\n",
                        (ULONG)plgfilehdrCheck->lgfilehdr.le_lGeneration, lGen );
                Error( ErrERRCheck( JET_errLogFileCorrupt ) );
            }

            errFindAttach = ErrIncrementalReseedFindValidAttachInfo( pirs, plgfilehdrCheck, pdbfilehdr, genFirstDivergedLog, &attachinfo );
            if ( errFindAttach >= JET_errSuccess &&
                 attachinfo.le_lgposAttach.le_lGeneration != 0 )
            {
                // this is the new best lgenMin in case all the logs past
                // the passed in value | DB header min have no attach info.
                genMinAttachInfo = max( genMinAttachInfo, (ULONG)plgfilehdrCheck->lgfilehdr.le_lGeneration );
            }

            delete pfapiLogFileCheck;
            pfapiLogFileCheck = NULL;

            if ( genMinAttachInfo )
            {
                break;
            }
        }

        if ( genMinAttachInfo )
        {
            //  We couldn't find acceptable attachinfo above the existing genMinRequired, but we
            //  have an even older log file that does have acceptable attachinfo.

            Assert( genMinAttachInfo < genMinRequired );

            //  report we are switching / lowering our gen min required to find attach info
            //

            (*pirs->PcprintfTrace())( "Moving genMinRequired (%d --> %d) BACK to find valid attachinfo!\r\n",
                    genMinRequired, genMinAttachInfo );
            (*pirs->PcprintfTrace())( "UPDATED: Log Generations [Min - Div - Max]: %d - %d - %d\r\n",
                    genMinRequired, genFirstDivergedLog, genMaxRequired );

            //  cleanup everything to reset ourselves to start from a lower generation
            //

            OSMemoryPageFree( plgfilehdrMin );
            plgfilehdrMin = NULL;
            OSMemoryPageFree( plgfilehdrMax );
            plgfilehdrMax = NULL;

            //  reset ourselves to the lower genMinRequired with the attach info, and retry ...
            //

            genMinRequired = genMinAttachInfo;
            fFindLowerAttachInfo = fFalse;
            fForceLowerAttachInfo = fFalse;

            goto RestartFromLowerLogGeneration;
        }
    }

    if ( 0 == attachinfo.le_lgposAttach.le_lGeneration )
    {
        if ( errNotFound == errFindAttach )
        {
            errFindAttach = ErrERRCheck( JET_errInvalidParameter );
        }
        Call( errFindAttach );
    }
    

    //  handle the case where the max required log is the current log
    //
    if ( (ULONG)genLast == genMaxRequired - 1 )
    {
        Call( ErrReadLogFileHeader( pinst, *TraceContextScope( iorpPatchFix ), genMaxRequired, &pfapiLogFileMax, &plgfilehdrMax ) );

        if ( (ULONG)plgfilehdrMax->lgfilehdr.le_lGeneration != genMaxRequired )
        {
            (*pirs->PcprintfTrace())( "Error: plgfilehdrMax->lgfilehdr.le_lGeneration = %d != genMaxRequired = %d\r\n",
                        (ULONG)plgfilehdrMax->lgfilehdr.le_lGeneration, genMaxRequired );
            Error( ErrERRCheck( JET_errLogFileCorrupt ) );
        }

        if ( plgfilehdrMax->lgfilehdr.le_lGeneration != genLast + 1 )
        {
            (*pirs->PcprintfTrace())( "Error: plgfilehdrMax->lgfilehdr.le_lGeneration = %d != genLast + 1 = %d\r\n",
                            (ULONG)plgfilehdrMax->lgfilehdr.le_lGeneration, genLast + 1 );
            Error( ErrERRCheck( JET_errLogFileCorrupt ) );
        }
        if ( memcmp( &plgfilehdrMax->lgfilehdr.signLog, &signLogLast, sizeof( plgfilehdrMax->lgfilehdr.signLog ) ) )
        {
            (*pirs->PcprintfTrace())( "Error: plgfilehdrMax(lgen=%d)->lgfilehdr.signLog = ",
                            (ULONG)plgfilehdrMax->lgfilehdr.le_lGeneration );
            DUMPPrintSig( pirs->PcprintfTrace(), &plgfilehdrMax->lgfilehdr.signLog );
            (*pirs->PcprintfTrace())( " != signLogLast = " );
            DUMPPrintSig( pirs->PcprintfTrace(), &signLogLast );
            (*pirs->PcprintfTrace())( "\r\n" );
            Error( ErrERRCheck( JET_errBadLogSignature ) );
        }
        if ( memcmp( &plgfilehdrMax->lgfilehdr.tmPrevGen, &tmCreateLast, sizeof( plgfilehdrMax->lgfilehdr.tmPrevGen ) ) )
        {
            (*pirs->PcprintfTrace())( "Error: plgfilehdrMax(lgen=%d)->lgfilehdr.tmPrevGen = ",
                            (ULONG)plgfilehdrMax->lgfilehdr.le_lGeneration );
            DUMPPrintLogTime( pirs->PcprintfTrace(), &plgfilehdrMax->lgfilehdr.tmPrevGen );
            (*pirs->PcprintfTrace())( " != tmCreateLast = " );
            DUMPPrintLogTime( pirs->PcprintfTrace(), &tmCreateLast );
            (*pirs->PcprintfTrace())( "\r\n" );
            Error( ErrERRCheck( JET_errInvalidLogSequence ) );
        }
        
        genLast = plgfilehdrMax->lgfilehdr.le_lGeneration;
        UtilMemCpy( &signLogLast, &plgfilehdrMax->lgfilehdr.signLog, sizeof( signLogLast ) );
        UtilMemCpy( &tmCreateLast, &plgfilehdrMax->lgfilehdr.tmCreate, sizeof( tmCreateLast ) );
    }

    if ( (ULONG)genLast < genMaxRequired )
    {
        (*pirs->PcprintfTrace())( "Error: genLast = %d < genMaxRequired = %d\r\n",
                        (ULONG)genLast, genMaxRequired );
        Error( ErrERRCheck( JET_errLogFileCorrupt ) );
    }

    //  verify that the log signature in the header matches the signature of the min required log.
    //  if it doesn't then we must fail the incremental reseed
    //
    if ( memcmp( &plgfilehdrMin->lgfilehdr.signLog, &pdbfilehdr->signLog, sizeof( plgfilehdrMin->lgfilehdr.signLog ) ) )
    {
        (*pirs->PcprintfTrace())( "Error: plgfilehdrMin(lgen=%d)->lgfilehdr.signLog = ",
                        (ULONG)plgfilehdrMin->lgfilehdr.le_lGeneration );
        DUMPPrintSig( pirs->PcprintfTrace(), &plgfilehdrMin->lgfilehdr.signLog );
        (*pirs->PcprintfTrace())( " != pdbfilehdr->signLog = " );
        DUMPPrintSig( pirs->PcprintfTrace(), &pdbfilehdr->signLog );
        (*pirs->PcprintfTrace())( "\r\n" );
        Error( ErrERRCheck( JET_errDatabaseFailedIncrementalReseed ) )
    }

    //  open the checkpoint file
    //
    CallS( pfsapi->ErrPathBuild(    SzParam( pinst, JET_paramSystemPath ),
                                    SzParam( pinst, JET_paramBaseName ),
                                    ( UlParam( pinst, JET_paramLegacyFileNames ) & JET_bitESE98FileNames ) ? wszOldChkExt : wszNewChkExt,
                                    szCheckpointFile ) )
    err = pfsapi->ErrFileOpen( szCheckpointFile, IFileAPI::fmfNone, &pfapiCheckpoint );
    if ( err < JET_errSuccess )
    {
        if ( JET_errFileNotFound == err )
        {
            //  if we couldn't find the checkpoint file then create a new one
            //
            Call( pfsapi->ErrFileCreate( szCheckpointFile, IFileAPI::fmfOverwriteExisting, &pfapiCheckpoint ) );
        }
    }
    Call( err );


    //  read in the checkpoint
    //
    Alloc( pcheckpoint = (CHECKPOINT*)PvOSMemoryPageAlloc( sizeof( CHECKPOINT ), NULL ) );
    err = ErrUtilReadShadowedHeader(    pinst,
                                        pfsapi,
                                        pfapiCheckpoint,
                                        (BYTE*)pcheckpoint,
                                        sizeof( CHECKPOINT ),
                                        -1,
                                        urhfNoAutoDetectPageSize );

    if ( err < JET_errSuccess )
    {
        if ( FErrIsDbHeaderCorruption( err ) || JET_errFileIOBeyondEOF == err )
        {
            //  if the checkpoint is corrupt then we will simply overwrite it with a new one
            //
            memset( pcheckpoint, 0, sizeof( CHECKPOINT ) );
            pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration   = pinst->m_plog->LgenInitial();
            pcheckpoint->checkpoint.le_lgposCheckpoint.le_isec          = (USHORT)pinst->m_plog->CSecLGHeader();
            pcheckpoint->checkpoint.le_lgposCheckpoint.le_ib            = 0;
            pcheckpoint->checkpoint.le_lgposDbConsistency               = pcheckpoint->checkpoint.le_lgposCheckpoint;
            pcheckpoint->checkpoint.signLog                             = plgfilehdrMin->lgfilehdr.signLog;
            pcheckpoint->checkpoint.le_filetype                         = JET_filetypeCheckpoint;
            err = JET_errSuccess;
        }
    }
    Call( err );

    //  validate the checkpoint
    //

    //   I would assert this is more likely a only partially filled checkpoint bug, than a 10+ year old checkpoint format - will be rejected
    //
    if ( pcheckpoint->checkpoint.le_filetype == JET_filetypeUnknown )
    {
        FireWall( "InvalidChkptFileTypeEndIrs" );
    }

    if ( pcheckpoint->checkpoint.le_filetype != JET_filetypeCheckpoint )
    {
        Error( ErrERRCheck( JET_errFileInvalidType ) )
    }
    if ( memcmp( &plgfilehdrMin->lgfilehdr.signLog, &pcheckpoint->checkpoint.signLog, sizeof( plgfilehdrMin->lgfilehdr.signLog ) ) )
    {
        Error( ErrERRCheck( JET_errBadCheckpointSignature ) )
    }

    //  update the cached header to be in the dirty shutdown state with
    //  the new min and max required logs set, and log time.
    //
    pdbfilehdr->SetDbstate( JET_dbstateDirtyAndPatchedShutdown, genMinRequired, genMaxRequired, &plgfilehdrMax->lgfilehdr.tmCreate, fTrue );

    Assert( CmpLgpos( pdbfilehdr->le_lgposAttach, lgposMin ) != 0 );
    Assert( ( CmpLgpos( pdbfilehdr->le_lgposLastResize, lgposMin ) == 0 ) ||
            ( CmpLgpos( pdbfilehdr->le_lgposLastResize, pdbfilehdr->le_lgposAttach ) >= 0 ) );

    //  update the last resize lgpos because the incremental reseed may have effectively undone some of of file resizes
    //
    if ( ( genFirstDivergedLog != 0 ) &&
         ( CmpLgpos( pdbfilehdr->le_lgposLastResize, lgposMin ) != 0 ) &&
         ( genFirstDivergedLog <= (ULONG)pdbfilehdr->le_lgposLastResize.le_lGeneration ) )
    {
        LGPOS lgposLastResize = lgposMin;
        lgposLastResize.lGeneration = (LONG)genFirstDivergedLog;
        pdbfilehdr->le_lgposLastResize = lgposLastResize;
    }

    //  update the gen recovering to be zero (because it can drift backwards during
    //  recovery otherwise) this will be reset as soon as we begin recovery ...
    //
    pdbfilehdr->le_lGenRecovering = 0;

    //  update the cached header to have the same lgposAttach as the new min required log
    //
    if ( CmpLgpos( &attachinfo.le_lgposAttach, &pdbfilehdr->le_lgposAttach ) <= 0 )
    {
        LGIGetDateTime( &pdbfilehdr->logtimeAttach );       //  make it clear we updated the attach info
        UtilMemCpy( &pdbfilehdr->le_lgposAttach, &attachinfo.le_lgposAttach, sizeof( pdbfilehdr->le_lgposAttach ) );
        LGIGetDateTime( &pdbfilehdr->logtimeConsistent );   //  make it clear we updated the attach info
        UtilMemCpy( &pdbfilehdr->le_lgposConsistent, &attachinfo.le_lgposConsistent, sizeof( pdbfilehdr->le_lgposConsistent ) );
    }

    Assert( ( CmpLgpos( pdbfilehdr->le_lgposLastResize, lgposMin ) == 0 ) ||
            ( CmpLgpos( pdbfilehdr->le_lgposLastResize, pdbfilehdr->le_lgposAttach ) >= 0 ) );

    //  update the cached header to remove information referring to invalidated backups
    //
    //  NOTE:  we must clear the entry if the high log copied into the backup is >= the new min
    //  required log because the new min required log could be different than that log we copied
    //  into the backup.  if that is the case then that backup contains part of the diverged log stream
    //  and thus must be invalidated
    //
    if ( pdbfilehdr->bkinfoFullPrev.le_genHigh >= genMinRequired )
    {
        memset( &pdbfilehdr->bkinfoFullPrev, 0, sizeof( pdbfilehdr->bkinfoFullPrev ) );
        pdbfilehdr->bkinfoTypeFullPrev = DBFILEHDR::backupNormal;
    }
    if ( pdbfilehdr->bkinfoIncPrev.le_genHigh >= genMinRequired )
    {
        memset( &pdbfilehdr->bkinfoIncPrev, 0, sizeof( pdbfilehdr->bkinfoIncPrev ) );
        pdbfilehdr->bkinfoTypeIncPrev = DBFILEHDR::backupNormal;
    }
    if ( pdbfilehdr->bkinfoFullCur.le_genHigh >= genMinRequired )
    {
        memset( &pdbfilehdr->bkinfoFullCur, 0, sizeof( pdbfilehdr->bkinfoFullCur ) );
    }
    if ( pdbfilehdr->bkinfoSnapshotCur.le_genHigh >= genMinRequired )
    {
        memset( &pdbfilehdr->bkinfoSnapshotCur, 0, sizeof( pdbfilehdr->bkinfoSnapshotCur ) );
    }
    if ( pdbfilehdr->bkinfoCopyPrev.le_genHigh >= genMinRequired )
    {
        memset( &pdbfilehdr->bkinfoCopyPrev, 0, sizeof( pdbfilehdr->bkinfoCopyPrev ) );
        pdbfilehdr->bkinfoTypeCopyPrev = DBFILEHDR::backupNormal;
    }
    if ( pdbfilehdr->bkinfoDiffPrev.le_genHigh >= genMinRequired )
    {
        memset( &pdbfilehdr->bkinfoDiffPrev, 0, sizeof( pdbfilehdr->bkinfoDiffPrev ) );
        pdbfilehdr->bkinfoTypeDiffPrev = DBFILEHDR::backupNormal;
    }

    if ( pirs->Pfm() != NULL )
    {
        //  fixup DB header state
        //
        pirs->Pfm()->SetDbGenMinRequired( pdbfilehdr->le_lGenMinRequired );
        pirs->Pfm()->SetDbGenMinConsistent( pdbfilehdr->le_lGenMinConsistent );

        //  flush the flush map
        //
        Call( pirs->Pfm()->ErrFlushAllSections( OnDebug( fTrue ) ) );
    }

    //  update the cached checkpoint to point to the new min required log
    //
    pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration   = genMinRequired;
    pcheckpoint->checkpoint.le_lgposCheckpoint.le_isec          = (USHORT)pinst->m_plog->CSecLGHeader();
    pcheckpoint->checkpoint.le_lgposCheckpoint.le_ib            = 0;
    pcheckpoint->checkpoint.le_lgposDbConsistency               = pcheckpoint->checkpoint.le_lgposCheckpoint;

    //  update the cached checkpoint to remove information referring to invalidated backups
    //
    if ( (ULONG)pcheckpoint->checkpoint.le_lgposLastFullBackupCheckpoint.le_lGeneration >= genMinRequired )
    {
        memset( &pcheckpoint->checkpoint.le_lgposLastFullBackupCheckpoint, 0, sizeof( pcheckpoint->checkpoint.le_lgposLastFullBackupCheckpoint ) );
    }
    if ( (ULONG)pcheckpoint->checkpoint.le_lgposFullBackup.le_lGeneration >= genMinRequired )
    {
        memset( &pcheckpoint->checkpoint.le_lgposFullBackup, 0, sizeof( pcheckpoint->checkpoint.le_lgposFullBackup ) );
        memset( &pcheckpoint->checkpoint.logtimeFullBackup, 0, sizeof( pcheckpoint->checkpoint.logtimeFullBackup ) );
    }
    if ( (ULONG)pcheckpoint->checkpoint.le_lgposIncBackup.le_lGeneration >= genMinRequired )
    {
        memset( &pcheckpoint->checkpoint.le_lgposIncBackup, 0, sizeof( pcheckpoint->checkpoint.le_lgposIncBackup ) );
        memset( &pcheckpoint->checkpoint.logtimeIncBackup, 0, sizeof( pcheckpoint->checkpoint.logtimeIncBackup ) );
    }

    //  update the cached checkpoint to contain the same attachments and params as the new min required log
    //
    UtilMemCpy( &pcheckpoint->checkpoint.dbms_param, &plgfilehdrMin->lgfilehdr.dbms_param, sizeof( pcheckpoint->checkpoint.dbms_param ) );
    UtilMemCpy( pcheckpoint->rgbAttach, plgfilehdrMin->rgbAttach, cbAttach );

    //  write the checkpoint back to the checkpoint file
    //
    //  NOTE:  we write this FIRST because if the checkpoint update succeeds but the header update
    //  fails then recovery will fail with JET_errDatabaseInvalidIncrementalReseed rather than failing
    //  in some obscure manner because the checkpoint was too far advanced.  on a retry, we will then
    //  rewrite all the important information in the checkpoint anyway
    //

    Call( ErrUtilWriteCheckpointHeaders( pinst, pfsapi, szCheckpointFile, iofrIncReseedUtil, pcheckpoint, pfapiCheckpoint ) );

    //  write the header back to the database (and flush everything)
    //

    (*pirs->PcprintfTrace())( "The database header at end of EndIncReseed:\r\n" );
    pdbfilehdr->DumpLite( pirs->PcprintfTrace(), "\r\n" );  // trace first

    Call( ErrUtilWriteUnattachedDatabaseHeaders( pinst, pfsapi, szDatabase, pdbfilehdr, pirs->PfapiDb(), pirs->Pfm() ) );

    (*pirs->PcprintfTrace())( "Completed IRSv2 Header Update!\r\n" );
    Call( ErrIRSDetachDatabaseIrsHandles( pinst, pirs, szDatabase, CIrsOpContext::eIrsDetachClean ) );
    Assert( !FIRSFindIrsContext( pinst, pirs ) );

HandleError:

    if ( err < JET_errSuccess && pirs )
    {
        TraceFuncComplete( pirs->PcprintfTrace(), __FUNCTION__, err );
    }
    // else Note: in success case of End, no IRS context to trace from as we're all done! :)

    OSMemoryPageFree( pcheckpoint );
    delete pfapiCheckpoint;
    OSMemoryPageFree( plgfilehdrCheck );
    delete pfapiLogFileCheck;
    OSMemoryPageFree( plgfilehdrMax );
    delete pfapiLogFileMax;
    OSMemoryPageFree( plgfilehdrMin );

    return err;
}

ERR
ErrIsamPatchDatabasePages(
    __in JET_INSTANCE               jinst,
    __in JET_PCWSTR                 szDatabase,
    __in ULONG              pgnoStart,
    __in ULONG              cpg,
    __in_bcount( cb ) const void *  pv,
    __in ULONG              cb,
    __in JET_GRBIT                  grbit )
{
    ERR                     err             = JET_errSuccess;
    INST * const            pinst           = (INST *)jinst;
    CIrsOpContext *         pirs            = NULL;
    BYTE *                  rgb             = NULL;
#ifdef ENABLE_INC_RESEED_TRACING
    BYTE *                  pbPrePages      = NULL;
#endif
    QWORD                   cbSize          = 0;

    TraceContextScope tcPatchPage( iorpPatchFix, iorsNone, iortPagePatching );

    //  mark path as not appropriate for FlushFileBuffer operations (want to roll all FFB calls
    //  into ErrrIsamPatchDatabase() to maintain good perf during O(n) part of IRS v2.
    //
    OnDebug( Ptls()->fFfbFreePath = fTrue );

    //  validate generic parameters
    //

    Call( ErrIRSGetAttachedIrsContext( pinst, szDatabase, &pirs ) );
    Assert( pirs );

    //  this format is declared / printed at end of ErrIsamBeginDatabaseIncrementalReseed.
    //TraceFuncBegun( pirs->PcprintfTrace(), __FUNCTION__ );
    (*pirs->PcprintfTrace())( "P," );

    //  validate Patch operation specific parameters
    //

    if ( pgnoStart < 1 )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( CPG( cpg ) < 0 )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( pgnoStart + cpg > pgnoSysMax )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    
    if ( cb != 0 && pv == NULL )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( !( grbit & JET_bitTestUninitShrunkPageImage ) && cb < cpg * ( max( min( g_cbPage, g_cbPageMax ), g_cbPageMin ) ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( ( grbit & ~( JET_bitTestUninitShrunkPageImage | JET_bitPatchingCorruptPage ) ) != 0 )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    //  revalidate the header (just for insurance, should be impossible to get here with unvalidated
    //  header because we have to get here through Jet/ErrIsamBeginDatabaseIncrementalReseed()).
    //
    IRSEnforce( pirs, ErrValidateHeaderForIncrementalReseed( pirs->Pdbfilehdr() ) >= JET_errSuccess );
    IRSEnforce( pirs, pirs->Pdbfilehdr()->Dbstate() == JET_dbstateIncrementalReseedInProgress );

    if ( 0 != cpg )
    {
#ifdef ENABLE_INC_RESEED_TRACING
        //  allocate space for the pre-images of the pages ...
        //
        Alloc( pbPrePages = (BYTE*)PvOSMemoryPageAlloc( cpg * g_cbPage, NULL ) );

        //  read the pre-images of the pages off disk for tracking purposes ...
        //
        const ERR errPreImageRead = pirs->PfapiDb()->ErrIORead( *tcPatchPage, OffsetOfPgno( pgnoStart ), cpg * g_cbPage, pbPrePages, QosSyncDefault( pinst ) );
#endif

        //  transfer the given pages to an aligned buffer to facilitate writing them to the database
        //
        Alloc( rgb = (BYTE*)PvOSMemoryPageAlloc( cpg * g_cbPage, NULL ) );
        if ( grbit & JET_bitTestUninitShrunkPageImage )
        {
            // Fake shrunk pages, just like reading pages from an online database returns.
            for ( PGNO pgnoT = pgnoStart; pgnoT < pgnoStart + cpg; pgnoT++ )
            {
                CPAGE cpage;
                const bool fPreviouslySet = FNegTestSet( fInvalidUsage );  // avoid Assert( ifmp != ifmpNil )
                cpage.GetShrunkPage( ifmpNil, pgnoT, rgb + g_cbPage * ( pgnoT - pgnoStart ), g_cbPage );
                FNegTestSet( fPreviouslySet );
            }
        }
        else
        {
            // Copy user-passed pages.
            UtilMemCpy( rgb, pv, cpg * g_cbPage );
        }

        //  checksum the pages in the buffer to ensure that they are good before writing them
        //
        //  NOTE:  we do not fix ECC errors here because they should be fixed when read from the
        //  source database.  any ECC errors we find happened in transit
        //
        for ( PGNO pgnoT = pgnoStart; pgnoT < pgnoStart + cpg; pgnoT++ )
        {
            CPageValidationNullAction nullaction;
            
            AssertTrack( pgnoT != 0, "IllegalAttemptPatchingDbShadowHdr" );

            CPAGE cpageT;
            cpageT.LoadPage( ifmpNil, pgnoT, rgb + ( pgnoT - pgnoStart ) * g_cbPage, g_cbPage );
            //  we don't have the flush type info from the source DB, so we can't validate the
            //  flush type.
            err = cpageT.ErrValidatePage( pgvfDoNotCheckForLostFlush, &nullaction );
            if ( JET_errPageNotInitialized == err )
            {
                err = JET_errSuccess;
            }
#ifdef ENABLE_INC_RESEED_TRACING
            if ( err < JET_errSuccess )
            {
                (*pirs->PcprintfTrace())( "p=%x,source page provided failed validation = %d", pgnoT, err );
            }
#endif
            Call( err );

            AssertTrack( pgnoT != 0, "IllegalPatchingDbShadowHdrChecksummed" );

#ifdef ENABLE_INC_RESEED_TRACING
            // should rename this to something more specific like "cpagePreImage;"
            CPAGE cpagePreT;
            cpagePreT.LoadPage( ifmpNil, pgnoT, pbPrePages + ( pgnoT - pgnoStart ) * g_cbPage, g_cbPage );

            const ERR errPreImageVerify = cpagePreT.ErrValidatePage( pgvfDoNotCheckForLostFlush, &nullaction );

            // Should we enforce or fail out?  NO!  We might be replacing a bad page ...
            //Assert( cpageT.PgnoThis() == cpagePreT.PgnoThis() );

            if ( errPreImageRead < JET_errSuccess )
            {
                (*pirs->PcprintfTrace())( "p=%x,i(FailedRead=%d):0,0,a:%x,%I64x", pgnoT, errPreImageRead,
                                                cpageT.ObjidFDP(), cpageT.Dbtime() );
            }
            else if ( errPreImageVerify < JET_errSuccess )
            {
                (*pirs->PcprintfTrace())( "p=%x,i(VerifyFailure=%d):%x,%I64x,a:%x,%I64x", pgnoT, errPreImageVerify,
                                                cpagePreT.ObjidFDP(), cpagePreT.Dbtime(),
                                                cpageT.ObjidFDP(), cpageT.Dbtime() );
            }
            else
            {
                (*pirs->PcprintfTrace())( "p=%x,i:%x,%I64x,a:%x,%I64x", pgnoT,
                                                cpagePreT.ObjidFDP(), cpagePreT.Dbtime(),
                                                cpageT.ObjidFDP(), cpageT.Dbtime() );
            }

            cpagePreT.UnloadPage();
#endif
            
            cpageT.UnloadPage();
        }

        //  if any part of the page range is beyond EOF, extend the database to accommodate the page
        Call( pirs->PfapiDb()->ErrSize( &cbSize, IFileAPI::filesizeLogical ) );
        const QWORD cbNewSize = OffsetOfPgno( pgnoStart ) + (QWORD)cpg * g_cbPage;
        if ( cbNewSize > cbSize )
        {
            //  extend the database, zero-filled
            const QWORD cbNewSizeEffective = roundup( cbNewSize, (QWORD)UlParam( pinst, JET_paramDbExtensionSize ) * g_cbPage );
            Call( pirs->PfapiDb()->ErrSetSize( *tcPatchPage, cbNewSizeEffective, fTrue, QosSyncDefault( pinst ) ) );
        }

        //  update the cached header to reflect that we are patching some pages
        //
        pirs->Pdbfilehdr()->le_ulPagePatchCount += cpg;
        LGIGetDateTime( &pirs->Pdbfilehdr()->logtimePagePatch );

        //  write the header back to the database
        //
        //  NOTE:  we write the header first so that we don't get into a state where we patched some
        //  pages but the header doesn't reflect that
        //
        Call( ErrUtilWriteUnattachedDatabaseHeaders( pinst, pinst->m_pfsapi, szDatabase, pirs->Pdbfilehdr(), pirs->PfapiDb(), pirs->Pfm() ) );

        if ( pirs->Pfm() != NULL )
        {
            //  set all flush types to pgftUnknown before patching the actual pages
            //
            Call( pirs->Pfm()->ErrSyncRangeInvalidateFlushType( pgnoStart, cpg ) );
        }

        //  write the pages to the database
        //
        Call( pirs->PfapiDb()->ErrIOWrite(
            *tcPatchPage,
            OffsetOfPgno( pgnoStart ),
            cpg * g_cbPage,
            rgb,
            QosSyncDefault( pinst ) ) );

        AssertTrack( pgnoStart != 0, "IllegalPatchingDbShadowHdrWritten" );

        if ( pirs->Pfm() != NULL )
        {
            //  set proper flush states and dbtimes once we've successfully patched the pages
            //
            for ( PGNO pgnoT = pgnoStart; pgnoT < pgnoStart + cpg; pgnoT++ )
            {
                CPAGE cpageT;
                cpageT.LoadPage( ifmpNil, pgnoT, rgb + ( pgnoT - pgnoStart ) * g_cbPage, g_cbPage );
                pirs->Pfm()->SetPgnoFlushType( pgnoT, cpageT.Pgft(), cpageT.Dbtime() );
                cpageT.UnloadPage();
            }
        }

        if ( grbit & JET_bitPatchingCorruptPage )
        {
            WCHAR szPgnoStart[64], szPgnoEnd[64];
            OSStrCbFormatW( szPgnoStart, sizeof(szPgnoStart), L"%u (0x%08x)", pgnoStart, pgnoStart );
            OSStrCbFormatW( szPgnoEnd, sizeof(szPgnoEnd), L"%u (0x%08x)", pgnoStart + cpg - 1, pgnoStart + cpg - 1 );

            const WCHAR *rgsz[3];
            rgsz[0] = szPgnoStart;
            rgsz[1] = szPgnoEnd;
            rgsz[2] = szDatabase;

            UtilReportEvent(
                    eventWarning,
                    DATABASE_CORRUPTION_CATEGORY,
                    PAGE_PATCHED_PASSIVE_ID,
                    _countof(rgsz),
                    rgsz,
                    0,
                    NULL,
                    pinst,
                    JET_EventLoggingLevelLow );
        }
        else
        {
            Assert( ( ( pgnoStart + cpg - 1 ) - pgnoStart + 1 ) == cpg ); // just checking my math in UpdatePagePatchedStats is right.
            pirs->UpdatePagePatchedStats( pgnoStart, pgnoStart + cpg - 1 );
        }
    }

HandleError:

#ifdef ENABLE_INC_RESEED_TRACING
    OSMemoryPageFree( pbPrePages );
#endif

    if ( err >= JET_errSuccess )
    {
        (*pirs->PcprintfTrace())( ",%d\r\n", err );
    }
    else // err < JET_errSuccess
    {
        if ( pirs )
        {
            (*pirs->PcprintfTrace())( ",%d(%s:%d)\r\n", err,
                            PefLastThrow() ? PefLastThrow()->SzFile() : "",
                            PefLastThrow() ? PefLastThrow()->Err() : 0 );
        }
    }

    OSMemoryPageFree( rgb );

    OnDebug( Ptls()->fFfbFreePath = fFalse );

    return err;
}

//  Zero out a BKINFO if its mark is in the specified generation
LOCAL VOID PossiblyZeroBkinfo(
    __inout BKINFO * const pbkinfo,
    __out LittleEndian<DBFILEHDR::BKINFOTYPE> * const pbkinfotype,
    const LONG lGeneration )
{
    Assert( lGeneration >= pbkinfo->le_lgposMark.le_lGeneration );
    if( lGeneration == pbkinfo->le_lgposMark.le_lGeneration )
    {
        memset( pbkinfo, 0, sizeof( BKINFO ) );
        *pbkinfotype = DBFILEHDR::backupNormal;
    }
}


ERR ErrIsamRemoveLogfile(
    _In_ IFileSystemAPI * const     pfsapi,
    _In_z_ const WCHAR * const      wszDatabase,
    _In_z_ const WCHAR * const      wszLogfile,
    _In_ const JET_GRBIT            grbit )
{
    JET_ERR err = JET_errSuccess;

    CPRINTF *     pcprintfTraceFile = NULL;

    IFileAPI *       pfapi  = NULL;
    DBFILEHDR * pdbfilehdr  = NULL;
    LGFILEHDR * plgfilehdr  = NULL;

    CFlushMapForUnattachedDb* pfm = NULL;

    //  start tracing (before anything else)
    //
    Call( ErrBeginDatabaseIncReseedTracing( pfsapi, wszDatabase, &pcprintfTraceFile ) );
    __int64                 fileTime;
    WCHAR                   szDate[32];
    WCHAR                   szTime[32];
    size_t                  cchRequired;
    fileTime = UtilGetCurrentFileTime();
    (void)ErrUtilFormatFileTimeAsTimeWithSeconds( fileTime, szTime, _countof(szTime), &cchRequired );
    (void)ErrUtilFormatFileTimeAsDate( fileTime, szDate, _countof(szDate), &cchRequired );
    (*pcprintfTraceFile)( "Begin " __FUNCTION__ "().  Time %ws %ws\r\n", szTime, szDate );

    // Validate arguments
    
    if( 0 != grbit )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if( 0 == wszDatabase || 0 == wszDatabase[0] )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if( 0 == wszLogfile || 0 == wszLogfile[0] )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    (*pcprintfTraceFile)( "Removing log file: %ws from DB: %ws\r\n", wszLogfile, wszDatabase );

    // Allocate memory

    Alloc( pdbfilehdr = static_cast<DBFILEHDR * >( PvOSMemoryPageAlloc( g_cbPage, NULL ) ) );
    Alloc( plgfilehdr = static_cast<LGFILEHDR * >( PvOSMemoryPageAlloc( sizeof(LGFILEHDR), NULL ) ) );

    // Read the headers

    Call( ErrUtilReadShadowedHeader(
            pinstNil,
            pfsapi,
            wszDatabase,
            reinterpret_cast<BYTE *>( pdbfilehdr ),
            g_cbPage,
            OffsetOf( DBFILEHDR_FIX, le_cbPageSize ),
            urhfReadOnly ) );

    (*pcprintfTraceFile)( "Header before removing the log file:\r\n" );
    pdbfilehdr->DumpLite( pcprintfTraceFile, "\r\n" );  // trace first

    Call( pfsapi->ErrFileOpen(  wszLogfile,
                                (   IFileAPI::fmfReadOnly |
                                    ( BoolParam( JET_paramEnableFileCache ) ?
                                        IFileAPI::fmfCached :
                                        IFileAPI::fmfNone ) ),
                                &pfapi ) );
    Call( ErrLGIReadFileHeader( pfapi, *TraceContextScope( iorpDirectAccessUtil ), qosIONormal, plgfilehdr ) );


    (*pcprintfTraceFile)( "Log file data at that path:\r\n" );
    (*pcprintfTraceFile)( "\tlgen = %d\r\n", (LONG)plgfilehdr->lgfilehdr.le_lGeneration );
    (*pcprintfTraceFile)( "\ttmCreate = " );
    DUMPPrintLogTime( pcprintfTraceFile, &plgfilehdr->lgfilehdr.tmCreate );
    (*pcprintfTraceFile)( "\r\n" );
    (*pcprintfTraceFile)( "\ttmPrevGen = " );
    DUMPPrintLogTime( pcprintfTraceFile, &plgfilehdr->lgfilehdr.tmPrevGen );
    (*pcprintfTraceFile)( "\r\n" );
    (*pcprintfTraceFile)( "\tsignLog = { " );
    DUMPPrintSig( pcprintfTraceFile, &plgfilehdr->lgfilehdr.signLog );
    (*pcprintfTraceFile)( " }\r\n" );
    (*pcprintfTraceFile)( "\tVersion = %d.%d.%d.%d\r\n",
                                (ULONG)plgfilehdr->lgfilehdr.le_ulMajor,
                                (ULONG)plgfilehdr->lgfilehdr.le_ulMinor,
                                (ULONG)plgfilehdr->lgfilehdr.le_ulUpdateMajor,
                                (ULONG)plgfilehdr->lgfilehdr.le_ulUpdateMinor );
    (*pcprintfTraceFile)( "\tfLGFlags = 0x%x\r\n", plgfilehdr->lgfilehdr.fLGFlags );
    (*pcprintfTraceFile)( "\tlgposCheckpoint = %08x:%04x:%04x\r\n",
                                (LONG)plgfilehdr->lgfilehdr.le_lgposCheckpoint.le_lGeneration,
                                (USHORT)plgfilehdr->lgfilehdr.le_lgposCheckpoint.le_isec,
                                (USHORT)plgfilehdr->lgfilehdr.le_lgposCheckpoint.le_ib );

    // Validate the database and logfile

    Assert( pdbfilehdr->le_lGenMaxCommitted >= pdbfilehdr->le_lGenMaxRequired );
    if( JET_dbstateDirtyShutdown != pdbfilehdr->Dbstate() )
    {
        // it doesn't make sense to remove logfiles for a consistent database
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if( 0 != memcmp( &pdbfilehdr->signLog, &plgfilehdr->lgfilehdr.signLog, sizeof(SIGNATURE) ) )
    {
        // this logfile isn't associated with this database
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    const LONG lGeneration = plgfilehdr->lgfilehdr.le_lGeneration;

    if( lGeneration > pdbfilehdr->le_lGenMaxCommitted )
    {
        // this logfile is neither required nor committed, nothing needs to be done
        (*pcprintfTraceFile)( "This logfile is neither required nor committed, nothing needs to be done ... bailing out with success.\r\n" );
        Assert( JET_errSuccess == err );
        goto HandleError;
    }
    
    if( lGeneration < pdbfilehdr->le_lGenMaxCommitted )
    {
        // this API can only be used to remove the last committed log
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if( lGeneration <= pdbfilehdr->le_lGenMaxRequired )
    {
        // the logfile we are trying to remove is required for recovery
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    // Update the header (and keep flushmap up to date w/ header)

    // If the logfile being removed is a backup mark then that member has
    // to be cleared
    
    PossiblyZeroBkinfo( &(pdbfilehdr->bkinfoFullPrev), &(pdbfilehdr->bkinfoTypeFullPrev), lGeneration );
    PossiblyZeroBkinfo( &(pdbfilehdr->bkinfoIncPrev), &(pdbfilehdr->bkinfoTypeIncPrev), lGeneration );
    PossiblyZeroBkinfo( &(pdbfilehdr->bkinfoCopyPrev), &(pdbfilehdr->bkinfoTypeCopyPrev), lGeneration );
    PossiblyZeroBkinfo( &(pdbfilehdr->bkinfoDiffPrev), &(pdbfilehdr->bkinfoTypeDiffPrev), lGeneration );
    
    pdbfilehdr->le_lGenMaxCommitted--;
    pdbfilehdr->le_lGenRecovering = 0;
    Assert( lGeneration == pdbfilehdr->le_lGenMaxCommitted+1 );
    Assert( pdbfilehdr->le_lGenMaxCommitted >= pdbfilehdr->le_lGenMaxRequired );

    memset( &(pdbfilehdr->logtimeGenMaxCreate), '\0', sizeof( LOGTIME ) );

    (*pcprintfTraceFile)( "Header after removing the log file:\r\n" );
    pdbfilehdr->DumpLite( pcprintfTraceFile, "\r\n" );  // trace first

    Call( CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDatabase, pdbfilehdr, pinstNil, &pfm ) );

    Call( ErrUtilWriteUnattachedDatabaseHeaders( pinstNil, pfsapi, wszDatabase, pdbfilehdr, NULL, pfm ) );

    if ( pfm != NULL )
    {
        pfm->TermFlushMap();
    }

    (*pcprintfTraceFile)( "Update/write header successful.\r\n" );


    Assert( JET_errSuccess == err );
    
HandleError:

    TraceFuncComplete( pcprintfTraceFile, __FUNCTION__, err );

    delete pfm;
    delete pfapi;
    OSMemoryPageFree( pdbfilehdr );
    OSMemoryPageFree( plgfilehdr );

    EndDatabaseIncReseedTracing( &pcprintfTraceFile );

    return err;
}

ERR ErrIsamRBSPrepareRevert(
    __in    JET_INSTANCE    jinst,
    __in    JET_LOGTIME     jltRevertExpected,
    __in    CPG             cpgCache,
    __in    JET_GRBIT       grbit,
    _Out_   JET_LOGTIME*    pjltRevertActual )
{
    ERR             err = JET_errSuccess;
    INST* const     pinst = (INST*) jinst;
    CRBSRevertContext* prbsrc = NULL;

    Call( CRBSRevertContext::ErrRBSRevertContextInit( pinst, *(LOGTIME*) &jltRevertExpected, cpgCache, grbit, (LOGTIME*) pjltRevertActual, &prbsrc ) );

    // In case prepare is called multiple times on same instance.
    if ( pinst->m_prbsrc )
    {
        delete pinst->m_prbsrc;
        pinst->m_prbsrc = NULL;
    }

    pinst->m_prbsrc = prbsrc;
 
HandleError:

    if ( err < JET_errSuccess )
    {
        delete pinst->m_prbsrc;
        pinst->m_prbsrc = NULL;
    }

    return err;
}

ERR ErrIsamRBSExecuteRevert(
    __in    JET_INSTANCE            jinst,
    __in    JET_GRBIT               grbit,
    _Out_   JET_RBSREVERTINFOMISC*  prbsrevertinfo)
{
    ERR             err     = JET_errSuccess;
    INST* const     pinst   = (INST*) jinst;

    // Should be initialized as part of ErrIsamRBSPrepareRevert
    Assert( pinst->m_prbsrc );

    Call( pinst->m_prbsrc->ErrExecuteRevert( grbit, prbsrevertinfo ) );

HandleError:
    delete pinst->m_prbsrc;
    pinst->m_prbsrc = NULL;

    return err;
}

ERR ErrIsamRBSCancelRevert(
    __in    JET_INSTANCE   jinst )
{
    INST* const     pinst   = (INST*) jinst;

    if ( pinst->m_prbsrc )
    {
        pinst->m_prbsrc->CancelRevert();

        // If execute revert has not been started yet, delete the revert context.
        if ( !pinst->m_prbsrc->FExecuteRevertStarted() )
        {
            delete pinst->m_prbsrc;
            pinst->m_prbsrc = NULL;
        }
    }

    return JET_errSuccess;
}

VOID FMP::SnapshotHeaderSignature()
{
    OnDebug( m_dataHdrSig.AssertValid() );
    Assert( m_dataHdrSig.FNull() );
    C_ASSERT( sizeof( DBFILEHDR ) == sizeof( DBFILEHDR_FIX ) );
    C_ASSERT( offsetof( DBFILEHDR, rgbReserved ) == offsetof( DBFILEHDR_FIX, rgbReserved ) );

    PdbfilehdrReadOnly pdbfilehdr = Pdbfilehdr();

    if ( !g_fRepair && !FIsTempDB() &&
         FAttached() &&
         ( !UlParam( JET_paramEnableViewCache ) || !FRedoMapsEmpty() ) &&
         pdbfilehdr && pdbfilehdr->le_filetype == JET_filetypeDatabase )
    {
        //  should have zeros, if we have stack or heap trash, the signature may not match.
        Assert( ( FUtilZeroed( ((const BYTE*)pdbfilehdr.get()) + offsetof( DBFILEHDR, rgbReserved ), sizeof( pdbfilehdr->rgbReserved ) ) &&
                  FUtilZeroed( ((const BYTE*)pdbfilehdr.get()) + offsetof( DBFILEHDR, rgbReserved2 ), sizeof( pdbfilehdr->rgbReserved2 ) ) ) ||
                    // one wonders if we should be taking the slightly risky approach of preserving caches of DBs at a update minor version.
                    CmpDbVer( pdbfilehdr->Dbv(), PfmtversEngineDefault()->dbv ) > 0 );

        //  We will keep as much of the header as necessary such that it includes all non-zeroed bytes.
        //  This is such that we are guaranteed to snapshot a header that checksums correctly, even in the
        //  case where there was a minor DB version upgrade that appended new fileds to the header.
        const size_t ibLastNonZeroedByte = IbUtilLastNonZeroed( (BYTE*)pdbfilehdr.get(), g_cbPageMin );
        Assert( ibLastNonZeroedByte >= offsetof( DBFILEHDR, le_lGenMaxCommitted ) );
        Assert( ibLastNonZeroedByte < (size_t)g_cbPageMin );

        size_t cbHeader = ibLastNonZeroedByte + 1;
        cbHeader = max( cbHeader, sizeof( DBFILEHDR ) );

        BYTE* const pbHeader = new BYTE[ cbHeader ];

        if ( pbHeader != NULL )
        {
            memcpy( pbHeader, pdbfilehdr, cbHeader );
            AssertDatabaseHeaderConsistent( (DBFILEHDR*)pbHeader, cbHeader, g_cbPage );
            C_ASSERT( offsetof( DBFILEHDR, le_ulMagic ) == sizeof( pdbfilehdr->le_ulChecksum ) );
            //  Checking not all zeros _except_ the checksum.
            Assert( !FUtilZeroed( pbHeader + offsetof( DBFILEHDR, le_ulMagic ), offsetof( DBFILEHDR, le_filetype ) - sizeof( pdbfilehdr->le_ulChecksum ) ) );
            SetDataHeaderSignature( (DBFILEHDR*)pbHeader, cbHeader );
        }
    }
}

VOID FMP::FreeHeaderSignature()
{
    OnDebug( m_dataHdrSig.AssertValid() );
    delete[] (BYTE*)m_dataHdrSig.Pv();
    m_dataHdrSig.Nullify();
}

/*  backup read completion function
/**/
struct READPAGE_DATA
{
    volatile ERR            err;
    volatile LONG*          pacRead;
    CAutoResetSignal*       pasigDone;
    BOOL                    fCheckPagesOffset;
    BOOL                    fExtensiveChecks;
    IFMP                    ifmp;
    LONG                    pgnoMost;
};

void IOReadDbPagesCompleted(
    const ERR err,
    IFileAPI *const pfapi,
    const FullTraceContext& tc,
    const OSFILEQOS grbitQOS,
    const QWORD ibOffset,
    const DWORD cbData,
    const BYTE* const pbData,
    READPAGE_DATA* const preaddata )
{
    Assert( !( grbitQOS & qosIOCompleteWriteGameOn ) ); // since QOS is immediate, this should be impossible

    /*  verify the chunk's pages
    /**/
    for ( DWORD ib = 0; ib < cbData; ib += g_cbPage )
    {
        ERR         errVerify   = JET_errSuccess;
        const PGNO pgnoExpected = preaddata->fCheckPagesOffset ? PgnoOfOffset( ibOffset + ib ) : pgnoNull;

        /*  there was an error on this read
        /**/
        if ( err < 0 )
        {
            errVerify = err;
        }
        /*  there was no error
        /**/
        else
        {
            //  Note that if we are replaying the required range, we'll completely drop lost flush validation
            //  and, furthermore, we won't fix up the flush map. This is because updating flush state and runtime
            //  state in the flush map for a given page is not thread-safe and thus the buffer manager is the
            //  component with the authority to do so. Because of this, we are basically losing the ability to
            //  detect lost flushes during replay from backup and from legacy DBM during recovery. Not a big
            //  deal in neither case.
            
            const LOG * const plog = PinstFromIfmp( preaddata->ifmp )->m_plog;
            const BOOL fInRecoveryRedo = ( !plog->FLogDisabled() && ( fRecoveringRedo == plog->FRecoveringMode() ) );
            const BOOL fReplayingRequiredRange = fInRecoveryRedo && g_rgfmp[ preaddata->ifmp ].FContainsDataFromFutureLogs();
            CPAGE cpage;
            INT logflags = CPageValidationLogEvent::LOG_ALL & ~CPageValidationLogEvent::LOG_UNINIT_PAGE;
            const BOOL fFailOnRuntimeLostFlushOnly = ( ( UlParam( PinstFromIfmp( preaddata->ifmp ), JET_paramPersistedLostFlushDetection ) & JET_bitPersistedLostFlushDetectionFailOnRuntimeOnly ) != 0 );

            const PAGEValidationFlags pgvf =
                pgvfFixErrors |
                ( preaddata->fExtensiveChecks ? pgvfExtensiveChecks : pgvfDefault ) |
                ( fFailOnRuntimeLostFlushOnly ? pgvfFailOnRuntimeLostFlushOnly : pgvfDefault ) |
                ( fReplayingRequiredRange ? pgvfDoNotCheckForLostFlush : pgvfDefault );
            CPageValidationLogEvent validationaction(
                preaddata->ifmp,
                logflags,
                LOGGING_RECOVERY_CATEGORY );

            cpage.LoadPage( preaddata->ifmp, pgnoExpected, (void*)( pbData + ib ), g_cbPage );
            errVerify = cpage.ErrValidatePage( pgvf, &validationaction );
            cpage.UnloadPage();
        }

        //  reading an uninit page during backup is not an error

        if ( JET_errPageNotInitialized == errVerify )
        {
            errVerify = JET_errSuccess;
        }

        if ( errVerify >= JET_errSuccess )
        {
            errVerify = ErrFaultInjection(50191);
        }

        //  there was a verification error

        if ( errVerify < JET_errSuccess )
        {
            //  fail the backup of this file with the error

            preaddata->err = errVerify;

            if ( BoolParam( PinstFromIfmp( preaddata->ifmp ), JET_paramEnableExternalAutoHealing ) &&
                PagePatching::FIsPatchableError( errVerify ) )
            {
                //
                // If validation fails then issue a patch request. It is safe to
                // issue multiple requests for the same page so we can call this
                // every time validation fails.  Note that issuing a patch
                // request will involve taking a BF lock, which can involve I/O
                // which cannot be done on a I/O thread (may cause deadlock)
                //
                PagePatching::RequestPagePatchOnNewThread( preaddata->ifmp, pgnoExpected );
            }
        }
    }

    /*  we are the last outstanding read
    /**/
    if ( !AtomicDecrement( (LONG*)preaddata->pacRead ) )
    {
        /*  signal the issuer thread that all reads are done
        /**/
        preaddata->pasigDone->Set();
    }
}

/*  read cpage into buffer ppageMin for backup.
 */

ERR ErrIOReadDbPages(
    IFMP ifmp,
    IFileAPI *pfapi,
    BYTE *pbData,
    LONG pgnoStart,
    LONG pgnoEnd,
    BOOL fCheckPagesOffset,
    LONG pgnoMost,
    const TraceContext& tc,
    BOOL fExtensiveChecks )
{
    ERR     err     = JET_errSuccess;
    /*  copy pages in aligned chunks of pages using groups of async reads
    /**/

    PGNO pgnoMaxDb = pgnoEnd + 1;

    volatile LONG acRead = 0;
    CAutoResetSignal asigDone( CSyncBasicInfo( _T( "ErrIOReadDbPages::asigDone" ) ) );

    READPAGE_DATA readdata;
    readdata.err = JET_errSuccess;
    readdata.pacRead = &acRead;
    readdata.pasigDone = &asigDone;
    readdata.fCheckPagesOffset = fCheckPagesOffset;
    readdata.fExtensiveChecks = fExtensiveChecks;
    readdata.ifmp = ifmp;
    readdata.pgnoMost = pgnoMost;

    DWORD cReadIssue;
    cReadIssue = 0;
    PGNO pgno1, pgno2;

    ULONG cpgBackupChunkSize = max( 1, (ULONG)UlParam( JET_paramMaxCoalesceReadSize ) / g_cbPage );
    
    for ( pgno1 = pgnoStart,
          pgno2 = min( PGNO( ( ( pgnoStart + cpgDBReserved - 1 ) / cpgBackupChunkSize + 1 ) * cpgBackupChunkSize - cpgDBReserved + 1 ), pgnoMaxDb );
          pgno1 < pgnoMaxDb && ( readdata.err >= 0 );
          pgno1 = pgno2,
          pgno2 = (PGNO)min( pgno1 + cpgBackupChunkSize, pgnoMaxDb ) )
    {
        /*  issue a read for the current aligned chunk of pages
        /**/
        QWORD ibOffset;
        ibOffset = OffsetOfPgno( pgno1 );

        DWORD cbData;
        cbData = ( pgno2 - pgno1 ) * g_cbPage;
        Expected( tc.iorReason.Iorp() != iorpNone );

        err = pfapi->ErrIORead( tc,
                                ibOffset,
                                cbData,
                                pbData,
                                // Consider even softer option ...
                                QosAsyncReadDefault( PinstFromIfmp( ifmp ) ),
                                IFileAPI::PfnIOComplete( IOReadDbPagesCompleted ),
                                DWORD_PTR( &readdata ) );
        if ( err < 0 && readdata.err >= 0 )
        {
            readdata.err = err;
        }
        pbData += cbData;

        cReadIssue++;
    }

    /*  wait for all issued reads to complete
    /**/
    if ( AtomicExchangeAdd( (LONG*)&acRead, cReadIssue ) + cReadIssue != 0 )
    {
        CallS( pfapi->ErrIOIssue() );
        asigDone.Wait();
    }

    if ( tc.iorReason.Iorp() == iorpBackup )
    {
        PERFOpt( cBKReadPages.Add( PinstFromIfmp( ifmp ), cReadIssue ) );
    }

    /*  get the error code from the reads
    /**/
    err = ( err < 0 ? err : readdata.err );
    return err;
}

BOOL FDBTestCheckShrunkPages( BYTE* pv, const CPG cpg )
{
    for ( INT i = 0; i < cpg; i++ )
    {
        CPAGE cpage;
        cpage.LoadPage( (BYTE *)pv + i * g_cbPage, g_cbPage );

        if ( !cpage.FShrunkPage() )
        {
            return fFalse;
        }
    }

    return fTrue;
}

ERR ErrIOFlushDatabaseFileBuffers( IFMP ifmp, const IOFLUSHREASON iofr )
{
    Assert( g_rgfmp[ifmp].Pfapi() );


    return ErrUtilFlushFileBuffers( g_rgfmp[ifmp].Pfapi(), iofr );
}

#ifdef DEBUG
BOOL FIOCheckUserDbNonFlushedIos( const INST * const pinst, const __int64 cioPerDbOutstandingLimit /* default 0 - meaning none */, IFMP ifmpTargetedDB )
{
    for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        const IFMP ifmp = pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;
        if ( ifmpTargetedDB != ifmpNil && ifmp != ifmpTargetedDB )
            continue;
        if ( g_rgfmp[ifmp].Pfapi() != NULL &&
                g_rgfmp[ifmp].Pfapi()->CioNonFlushed() > cioPerDbOutstandingLimit )
        {
            return fFalse;
        }
    }
    return fTrue;
}
#endif // DEBUG

#ifdef ENABLE_JET_UNIT_TEST

#pragma warning( push )

// C4307 hits for these statements.
// FMP * pfmp = g_rgfmp + IfmpTest();
#pragma warning( disable: 4307 ) // Integral constant overflow.

//  ================================================================
JETUNITTESTDB( IO, TestValidDBReadPage, dwOpenDatabase )
//  ================================================================
{
    BYTE * pv;
    FMP * pfmp = g_rgfmp + IfmpTest();
    CPG cpgActual = 0;
    
    pv = (BYTE*)PvOSMemoryPageAlloc( g_cbPage, NULL );
    CHECK( NULL != pv );

    CHECK( JET_errSuccess == pfmp->ErrDBReadPages(
        1,
        pv,
        1,
        &cpgActual,
        *TraceContextScope( iorpBackup ) ) );

    CHECK( 1 == cpgActual );
    CHECK( !FDBTestCheckShrunkPages( pv, 1 ) );

    OSMemoryPageFree( pv );
}

//  ================================================================
JETUNITTESTDB( IO, TestValidDBReadPages, dwOpenDatabase )
//  ================================================================
{
    BYTE * pv;
    FMP * pfmp = g_rgfmp + IfmpTest();
    CPG cpgActual = 0;
    const CPG cpg = 3;

    pv = (BYTE*)PvOSMemoryPageAlloc( cpg * g_cbPage, NULL );
    CHECK( NULL != pv );

    CHECK( JET_errSuccess == pfmp->ErrDBReadPages(
        1,
        pv,
        cpg,
        &cpgActual,
        *TraceContextScope( iorpBackup ) ) );

    CHECK( cpg == cpgActual );
    CHECK( !FDBTestCheckShrunkPages( pv, cpg ) );

    OSMemoryPageFree( pv );
}

//  ================================================================
JETUNITTESTDB( IO, TestOutOfRangeDBReadPage, dwOpenDatabase )
//  ================================================================
{
    BYTE * pv;
    FMP * pfmp = g_rgfmp + IfmpTest();
    CPG cpgActual = 0;
    
    pv = (BYTE*)PvOSMemoryPageAlloc( g_cbPage, NULL );
    CHECK( NULL != pv );

    CHECK( JET_errSuccess == pfmp->ErrDBReadPages(
        10000,
        pv,
        1,
        &cpgActual,
        *TraceContextScope( iorpBackup ) ) );

    CHECK( 1 == cpgActual );
    CHECK( FDBTestCheckShrunkPages( pv, 1 ) );

    OSMemoryPageFree( pv );
}

//  ================================================================
JETUNITTESTDB( IO, TestOutOfRangeDBReadPages, dwOpenDatabase )
//  ================================================================
{
    BYTE * pv;
    FMP * pfmp = g_rgfmp + IfmpTest();
    CPG cpgActual = 0;
    const CPG cpg = 3;

    pv = (BYTE*)PvOSMemoryPageAlloc( cpg * g_cbPage, NULL );
    CHECK( NULL != pv );

    CHECK( JET_errSuccess == pfmp->ErrDBReadPages(
        10000,
        pv,
        cpg,
        &cpgActual,
        *TraceContextScope( iorpBackup ) ) );

    CHECK( cpg == cpgActual );
    CHECK( FDBTestCheckShrunkPages( pv, cpg ) );

    OSMemoryPageFree( pv );
}

//  ================================================================
JETUNITTESTDB( IO, TestPartlyOutOfRangeDBReadPages, dwOpenDatabase )
//  ================================================================
{
    BYTE * pv;
    FMP * pfmp = g_rgfmp + IfmpTest();
    CPG cpgActual = 0;
    const CPG cpg = 4;

    pv = (BYTE*)PvOSMemoryPageAlloc( cpg * g_cbPage, NULL );
    CHECK( NULL != pv );

    CHECK( JET_errSuccess == pfmp->ErrDBReadPages(
        pfmp->PgnoLast() - 1,
        pv,
        cpg,
        &cpgActual,
        *TraceContextScope( iorpBackup ) ) );

    CHECK( cpg == cpgActual );
    CHECK( !FDBTestCheckShrunkPages( pv, 2 ) );
    CHECK( FDBTestCheckShrunkPages( (BYTE*)pv + 2*g_cbPage, cpg - 2) );

    OSMemoryPageFree( pv );
}

//  ================================================================
JETUNITTESTDB( IO, TestPartlyOutOfRangeDBReadManyPages, dwOpenDatabase )
//  ================================================================
{
    BYTE * pv;
    FMP * pfmp = g_rgfmp + IfmpTest();
    CPG cpgActual = 0;
    const CPG cpg = 30;

    pv = (BYTE*)PvOSMemoryPageAlloc( cpg * g_cbPage, NULL );
    CHECK( NULL != pv );

    CHECK( JET_errSuccess == pfmp->ErrDBReadPages(
        pfmp->PgnoLast() - 12,
        pv,
        cpg,
        &cpgActual,
        *TraceContextScope( iorpBackup ) ) );

    CHECK( cpg == cpgActual );
    CHECK( !FDBTestCheckShrunkPages( pv, 13 ) );
    CHECK( FDBTestCheckShrunkPages( (BYTE*)pv + 13*g_cbPage, cpg - 13) );

    OSMemoryPageFree( pv );
}
#pragma warning( pop ) // Unreferenced variables in Unit Tests.

#endif // ENABLE_JET_UNIT_TEST
