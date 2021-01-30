// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#ifdef PERFMON_SUPPORT


PERFInstanceDelayedTotal<HRT>       cIOTotalDhrts[iotypeMax][iofileMax];
PERFInstanceDelayedTotal<QWORD>     cIOTotalBytes[iotypeMax][iofileMax];
PERFInstanceDelayedTotal<>          cIOTotal[iotypeMax][iofileMax];
PERFInstanceDelayedTotal<>          cIOInHeap[iotypeMax][iofileMax];
PERFInstanceDelayedTotal<>          cIOAsyncPending[iotypeMax][iofileMax];


PERFInstance<HRT>       cIOPerDBTotalDhrts[iotypeMax];
PERFInstance<QWORD>     cIOPerDBTotalBytes[iotypeMax];
PERFInstance<>          cIOPerDBTotal[iotypeMax];

PERFInstance<QWORD, fFalse>  cIOPerDBLatencyCount[iotypeMax][iocatMax];
PERFInstance<QWORD, fFalse>  cIOPerDBLatencyAve[iotypeMax][iocatMax];
PERFInstance<QWORD, fFalse>  cIOPerDBLatencyP50[iotypeMax][iocatMax];
PERFInstance<QWORD, fFalse>  cIOPerDBLatencyP90[iotypeMax][iocatMax];
PERFInstance<QWORD, fFalse>  cIOPerDBLatencyP99[iotypeMax][iocatMax];
PERFInstance<QWORD, fFalse>  cIOPerDBLatencyP100[iotypeMax][iocatMax];
PERFInstance<QWORD, fFalse>  cIODatabaseMetedQueueDepth[iofileMax];
PERFInstance<QWORD, fFalse>  cIODatabaseMetedOutstandingMax[iofileMax];
PERFInstance<QWORD, fFalse>  cIODatabaseAsyncReadPending[iofileMax];


C_ASSERT( _countof( cIOTotalBytes[0] ) == iofileMax );


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

#endif

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





ERR ErrIOInit()
{
    ERR err = JET_errSuccess;


    if ( g_iothreadinfotable.ErrInit( 5, 1 ) != CIOThreadInfoTable::ERR::errSuccess )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

HandleError:
    return err;
}

VOID IOTerm()
{

    g_iothreadinfotable.Term();
}

void FlightConcurrentMetedOps( INT cioOpsMax, INT cioLowThreshold, TICK dtickStarvation );

ERR ErrIOInit( INST *pinst )
{

    pinst->m_plog->SetCheckpointEnabled( fFalse );

    FlightConcurrentMetedOps( (ULONG)UlParam( pinst, JET_paramFlight_ConcurrentMetedOps ),
                                (ULONG)UlParam( pinst, JET_paramFlight_LowMetedOpsThreshold ),
                                (ULONG)UlParam( pinst, JET_paramFlight_MetedOpStarvedThreshold ) );

    return JET_errSuccess;
}

ERR ErrIOTermGetShutDownMark( INST * const pinst, LOG * const plog, LGPOS& lgposShutDownMarkRec )
{
    ERR err = JET_errSuccess;
    

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
    
    

    {
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

    }

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
    
    Assert( pfmp->Pfapi()->CioNonFlushed() == 0 || ( pfmp->Pfapi()->Fmf() & IFileAPI::fmfTemporary ) || FRFSFailureDetected( OSFileFlush ) );

    delete pfmp->Pfapi();
    pfmp->SetPfapi( NULL );
}

void IOResetFMPFields( FMP * const pfmp, const LOG * const plog )
{
    Assert( FMP::FWriterFMPPool() );

    DBResetFMP( pfmp, plog );

    pfmp->ResetFlags();


    if ( pfmp->WszDatabaseName() )
    {
        OSMemoryHeapFree( pfmp->WszDatabaseName() );
        pfmp->SetWszDatabaseName( NULL );
    }

    pfmp->Pinst()->m_mpdbidifmp[ pfmp->Dbid() ] = g_ifmpMax;
    pfmp->SetDbid( dbidMax );
    pfmp->SetPinst( NULL );
    pfmp->SetCPin( 0 );
}

ERR ErrIOTermFMP( FMP *pfmp, LGPOS lgposShutDownMarkRec, BOOL fNormal )
{
    ERR err = JET_errSuccess;
    IFMP ifmp = pfmp->Ifmp();
    INST *pinst = pfmp->Pinst();
    LOG *plog = pinst->m_plog;
    OnDebug( const BOOL fNormalOrig = fNormal; )
    BOOL fPartialCreateDb = fFalse;

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

    Assert( pfmp->FIsTempDB() || pinst->FRecovering() || ( pfmp->FDontStartDBM() && !pfmp->FDBMScanOn() ) );
    Assert( pfmp->PdbmFollower() == NULL );

    Assert( NULL == pfmp->PkvpsMSysLocales() );

    if ( fNormal && plog->FRecovering() )
    {
        Assert( !pfmp->FReadOnlyAttach() );
        if ( pfmp->Patchchk() )
        {
            Assert( !pfmp->FIsTempDB() );
            pfmp->Patchchk()->lgposConsistent = lgposShutDownMarkRec;
        }
    }

    
    Assert( pfmp->Pfapi()
            || NULL == pfmp->Pdbfilehdr()
            || !fNormal );
    if ( pfmp->Pfapi() )
    {
        Assert( !!pfmp->FReadOnlyAttach() == !!( pfmp->Pfapi()->Fmf() & IFileAPI::fmfReadOnly ) );
        Assert( ( pfmp->PFlushMap() == NULL ) || !pfmp->FIsTempDB() );
        
        if ( pfmp->FAttached() && !pfmp->FReadOnlyAttach() && !pfmp->FIsTempDB() )
        {
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
        pfmp->SnapshotHeaderSignature();
    }

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

    FMP::EnterFMPPoolAsWriter();
    pfmp->RwlDetaching().EnterAsWriter();

    IOResetFMPFields( pfmp, plog );

    pfmp->RwlDetaching().LeaveAsWriter();
    FMP::LeaveFMPPoolAsWriter();

    Assert( !fNormalOrig || fNormal || err < JET_errSuccess || fPartialCreateDb );

    return err;
}


ERR ErrIOTerm( INST *pinst, BOOL fNormal )
{
    ERR         err = JET_errSuccess;
    LGPOS       lgposShutDownMarkRec = lgposMin;
    LOG         * const plog = pinst->m_plog;
    OnDebug( const BOOL fNormalOrig = fNormal; )

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDatapoint|sysosrtlContextInst, pinst, (PVOID)&(pinst->m_iInstance), sizeof(pinst->m_iInstance) );

    Assert( pinst->m_ppibGlobal == ppibNil );

    
    err = plog->ErrLGUpdateCheckpointFile( fTrue );

    Assert( !fNormal || FIOCheckUserDbNonFlushedIos( pinst, cioAllowLogRollHeaderUpdates ) );

    Assert( err != JET_errDatabaseSharingViolation );

    if ( err < 0 && plog->FRecovering() )
    {
        plog->SetNoMoreLogWrite( err );
    }

    
    plog->SetCheckpointEnabled( fFalse );

    
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
        const IFMP ifmp = pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;
        
        if ( FFMPIsTempDB( ifmp ) ) 
        {
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
        err = ErrERRCheck( JET_errOutOfMemory );
    }

    else if ( pinst->m_pver->m_fSyncronousTasks || g_rgfmp[ifmp].FDetachingDB() )
    {
        delete ptaskDbExtend;
        err = JET_errSuccess;
    }

    else
    {
        err = pinst->Taskmgr().ErrTMPost( TASK::DispatchGP, ptaskDbExtend );
        if( err < JET_errSuccess )
        {
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

    if ( NULL != pfmp->Pdbfilehdr() )
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

    if ( NULL != pfmp->Pdbfilehdr() )
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
        if ( cbNewSize < cbFsFileSize )
        {
            if ( !fGrowOnly )
            {
                const QWORD cbTruncationChunk = pfmp->CbOfCpg( CpgBFGetOptimalLockPageRangeSizeForExternalZeroing( ifmp ) );
                QWORD cbNewSizePartial = cbFsFileSize;
                while ( cbNewSizePartial > cbNewSize )
                {
                    Expected( ( cbNewSizePartial % g_cbPage ) == 0 );
                    cbNewSizePartial = rounddn( cbNewSizePartial, g_cbPage );
                    cbNewSizePartial -= min( cbTruncationChunk, cbNewSizePartial );
                    cbNewSizePartial = max( cbNewSizePartial, cbNewSize );

                    Expected( cpgAsyncExtension == 0 );
                    cbFsFileSizeAsyncTargetNew = cbNewSizePartial;

                    DWORD_PTR dwRangeLockContext = NULL;

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
                    }

                    if ( err >= JET_errSuccess )
                    {
                        err = pfmp->Pfapi()->ErrSetSize( *tcScope, cbNewSizePartial, fFalse, QosSyncDefault( pinst ) );
                        Expected( err <= JET_errSuccess );

                        Assert( !BoolParam( JET_paramEnableViewCache ) || ( err >= JET_errSuccess ) );

                        if ( err >= JET_errSuccess )
                        {
                            cbFsFileSize = cbNewSizePartial;
                        }
                    }
                    BFUnlockPageRangeForExternalZeroing( dwRangeLockContext, *tcScope );
                }
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
        else if ( cbNewSize > cbFsFileSize )
        {
            if ( !fShrinkOnly )
            {
                if ( !pfmp->FIsTempDB() )
                {
                    err = pfmp->PFlushMap()->ErrSetFlushMapCapacity( cpgNewSize );
                }

                if ( err >= JET_errSuccess )
                {
                    PERFOpt( cIODatabaseFileExtensionStall.Inc( pinst ) );

                    tcScope->iorReason.SetIorp( iorpDatabaseExtension );
                    tcScope->SetDwEngineObjid( objidSystemRoot );

                    err = pfmp->Pfapi()->ErrSetSize( *tcScope, cbNewSize, fTrue, QosSyncDefault( pinst ) );
                    Expected( err <= JET_errSuccess );
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

    cchDbPath = (DWORD)wcslen( pfmp->WszDatabaseName() );
    cchDbExtension = (DWORD)wcslen( wszDbExtension );
    cchJsaExtension = (DWORD)wcslen( wszShrinkArchiveExt );
    if ( ( cchDbPath - cchDbExtension + cchDbExtension
        + 3
        + 14
        + ( 2 * 8 )
        + 1
        ) > IFileSystemAPI::cchPathMax )
    {
        Error( ErrERRCheck( JET_errInvalidPath ) );
    }
    else
    {
        OSStrCbFormatW(
            wszJsaFileName,
            IFileSystemAPI::cchPathMax * sizeof( WCHAR ),
            L"%ws-%04u%02u%02u%02u%02u%02u-%08x-%08x",
            wszDbFileName,
            dt.year, dt.month, dt.day,
            dt.hour, dt.minute, dt.second,
            pgnoStart, pgnoEnd );
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

    cchDbPath = (DWORD)wcslen( pfmp->WszDatabaseName() );
    cchDbExtension = (DWORD)wcslen( wszDbExtension );
    cchJsaExtension = (DWORD)wcslen( wszShrinkArchiveExt );
    if ( ( cchDbPath - cchDbExtension + cchDbExtension
        + 1
        + 1
        + 1
        ) > IFileSystemAPI::cchPathMax )
    {
        Error( ErrERRCheck( JET_errInvalidPath ) );
    }
    else
    {
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

    const CPG cpgNewSize = (CPG)( pgnoFirst - 1 );
    CPG cpgCurrentSize = cpgNewSize + cpgArchive;
    const CPG cpgChunkSize = (CPG)UlParam( pinst, JET_paramDbExtensionSize );
    while ( cpgCurrentSize > cpgNewSize )
    {
        const PGNO pgnoChunkEnd = cpgCurrentSize;
        cpgCurrentSize = LFunctionalMax( cpgCurrentSize - cpgChunkSize, cpgNewSize );
        const PGNO pgnoChunkStart = cpgCurrentSize + 1;
        Assert( pgnoChunkStart <= pgnoChunkEnd );

        WCHAR wszJsaFilePath[ IFileSystemAPI::cchPathMax ] = { L'\0' };
        Call( ErrIOIGetJsaPathFromDbPath( wszJsaFilePath, pfmp, dt, pgnoChunkStart, pgnoChunkEnd ) );

        Call( CIOFilePerf::ErrFileCreate(
                    pinst->m_pfsapi,
                    pinst,
                    wszJsaFilePath,
                    IFileAPI::fmfStorageWriteBack | IFileAPI::fmfOverwriteExisting,
                    iofileOther,
                    ifmp,
                    &pfapi ) );

        PGNO pgnoIoStart = pgnoChunkStart;
        while ( pgnoIoStart <= pgnoChunkEnd )
        {
            const PGNO pgnoIoEnd = UlFunctionalMin( pgnoIoStart + cpgIoSize - 1, pgnoChunkEnd );

            const CPG cpgToRead = pgnoIoEnd - pgnoIoStart + 1;
            CPG cpgRead = 0;
            Call( pfmp->ErrDBReadPages( pgnoIoStart, pv, cpgToRead, &cpgRead, tc, fFalse ) );
            Assert( cpgRead == cpgToRead );
            Assert( cpgRead > 0 );

            Call( pfapi->ErrIOWrite( tc,
                                    pfmp->CbOfCpg( pgnoIoStart - pgnoChunkStart ),
                                    (DWORD)pfmp->CbOfCpg( cpgRead ),
                                    pv,
                                    QosSyncDefault( pinst ) ) );

            pgnoIoStart = pgnoIoEnd + 1;
        }

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

ERR ErrIODeleteShrinkArchiveFiles( const IFMP ifmp )
{
    ERR err = JET_errSuccess;
    FMP* const pfmp = g_rgfmp + ifmp;
    INST* const pinst = pfmp->Pinst();
    IFileSystemAPI* const pfsapi = pinst->m_pfsapi;
    IFileFindAPI* pffapi = NULL;
    WCHAR wszJsaWildcardPath[ IFileSystemAPI::cchPathMax ] = { L'\0' };

    Call( ErrIOIGetJsaWildcardPathFromDbPath( wszJsaWildcardPath, pfmp ) );

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

    const QWORD ibStartZeroesAligned = roundup( ibStartZeroesRaw, cbSparseFileGranularity );
    const QWORD ibEndZeroesAligned = rounddn( ibEndZeroesRaw, cbSparseFileGranularity );
    const QWORD cbZeroesAligned = ibEndZeroesAligned - ibStartZeroesAligned;

    *pibStartZeroes = ibStartZeroesAligned;
    *ppgnoStartZeroes = PgnoOfOffset( ibStartZeroesAligned );
    *pcbZeroLength = cbZeroesAligned;
    *pcpgZeroLength = g_rgfmp[ ifmp ].CpgOfCb( cbZeroesAligned );

    Assert( ibEndZeroesAligned + cbSparseFileGranularity >= ibStartZeroesRaw );

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

    Assert( !!fReadOnly == !!( pfmp->FmfDbDefault() & IFileAPI::fmfReadOnly ) );
    Assert( !!( !fReadOnly && BoolParam( JET_paramEnableFileCache ) && !pfmp->FLogOn() ) == !!( pfmp->FmfDbDefault() & IFileAPI::fmfLossyWriteBack ) );


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

        Call( ErrUtilFlushFileBuffers( pfapi, iofrOpeningFileFlush ) );

        QWORD cbSize;
        Call( pfapi->ErrSize( &cbSize, IFileAPI::filesizeLogical ) );
        cpg = (CPG)roundupdiv( cbSize, g_cbPage );
        cpg -= cpgDBReserved;

        Call( pfmp->PFlushMap()->ErrSetFlushMapCapacity( LFunctionalMax( cpg, 1 ) ) );

        pfmp->AcquireIOSizeChangeLatch();
        pfmp->SetFsFileSizeAsyncTarget( cbSize );
        pfmp->ReleaseIOSizeChangeLatch();

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
                err = JET_errSuccess;
            }
            else
            {
                Call( err );

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

    Assert( pfmp->Pfapi() );


    if (    !pfmp->FReadOnlyAttach() &&
            !pfmp->FCreatingDB() &&
            !( !pfmp->FReadOnlyAttach() && ( pfmp->Pfapi()->Fmf() & IFileAPI::fmfCached ) && !pfmp->FLogOn() ) )
    {
        (void)ErrIOFlushDatabaseFileBuffers( ifmp, iofrWriteBackFlushIfmpContext );
    }

    Assert( pfmp->Pfapi()->CioNonFlushed() == 0 || FRFSFailureDetected( OSFileFlush ) );

    delete pfmp->Pfapi();
    pfmp->SetPfapi( NULL );

    IOResetFmpIoLatencyStats( pfmp->Ifmp() );
}


ERR ErrIODeleteDatabase( IFileSystemAPI *const pfsapi, IFMP ifmp )
{
    ERR err = JET_errSuccess;

    FMP::AssertVALIDIFMP( ifmp );

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

    C_ASSERT( dbidMax < CHAR_MAX );
#ifdef DEBUG
    INT         cmpinstcdb          = cMaxInstances;
    unsigned char * mpinstcdb       = NULL;
    mpinstcdb = (unsigned char *)PvOSMemoryHeapAlloc( cmpinstcdb );
    if ( mpinstcdb )
    {
        memset( mpinstcdb, 0, cmpinstcdb );
    }
#endif

    
    if ( !pSnapshotSession )
    {
        INST::EnterCritInst();
    }

    FMP::EnterFMPPoolAsWriter();

    if ( 0 == g_cpinstInit )
    {
        *pcInstanceInfo = 0;
        *paInstanceInfo = NULL;
        goto HandleError;
    }

    for ( ipinst = 0; ipinst < g_cpinstMax; ipinst++ )
    {
        INST *  pinst = g_rgpinst[ ipinst ];
        if ( pinstNil == pinst )
            continue;

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

    cbMemoryBuffer = 0;
    cbMemoryBuffer += cInstances * sizeof(JET_INSTANCE_INFO_W);
    cbMemoryBuffer += 3 * cDatabasesTotal * sizeof(WCHAR *);
    cbMemoryBuffer += cbNamesSize;

    if ( 0 == cbMemoryBuffer )
    {
        Assert( *pcInstanceInfo == 0 );
        Assert( *paInstanceInfo == NULL );
        Assert( JET_errSuccess == err );
        goto HandleError;
    }

    Assert( 0 != cInstances );

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

        Assert( cInstances <= cInstancesExpected );

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

            Assert( mpinstcdb == NULL || mpinstcdb[ pinst->m_iInstance ] > cDatabasesCurrInst );

            Assert( NULL == pInstInfo->szDatabaseFileName[cDatabasesCurrInst] );
            OSStrCbCopyW( pCurrentPosNames, cbCurrentPosNames, pfmp->WszDatabaseName() );
            pInstInfo->szDatabaseFileName[cDatabasesCurrInst] = pCurrentPosNames;
            cbCurrentPosNames -= sizeof(WCHAR) * ( LOSStrLengthW( pCurrentPosNames ) + 1 );
            pCurrentPosNames += LOSStrLengthW( pCurrentPosNames ) + 1;
            Assert( pMemoryBuffer + cbMemoryBuffer >= (CHAR*)pCurrentPosNames );

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

void INST::WaitForDBAttachDetach( )
{
    BOOL fDetachAttach = fTrue;

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

ERR INST::ErrReserveIOREQ()
{
    const ERR err = ErrOSDiskIOREQReserve();

    if ( err >= JET_errSuccess )
    {
        const LONG cIOReserved = AtomicIncrement( (LONG*)&m_cIOReserved );
        Assert( cIOReserved > 0 );

        Assert( cIOReserved <= (LONG)UlParam( JET_paramOutstandingIOMax ) );
    }

    return err;
}

VOID INST::UnreserveIOREQ()
{
    OSDiskIOREQUnreserve();
    const LONG cIOReserved = AtomicDecrement( (LONG*)&m_cIOReserved );
    Assert( cIOReserved >= 0 );
}

CCriticalSection CESESnapshotSession::g_critOSSnapshot( CLockBasicInfo( CSyncBasicInfo( "OSSnapshot" ), rankOSSnapshot, 0 ) );

JET_OSSNAPID        CESESnapshotSession::g_idSnapshot   = 0;
DWORD               CESESnapshotSession::g_cListEntries = 0;
const INST * const  CESESnapshotSession::g_pinstInvalid = (INST *)(~(DWORD_PTR(0x0)));


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

    Alloc( pNew = new CESESnapshotSession( ++g_idSnapshot ) );
    Assert( pNew );

    g_ilSnapshotSessionEntry.InsertAsNextMost( pNew );
    CESESnapshotSession::g_cListEntries++;

    *ppSession = pNew;

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

    for ( pinstLGFlush = GetFirstInstance(); NULL != pinstLGFlush; pinstLGFlush = GetNextInstance() )
    {
        Call( pinstLGFlush->m_pbackup->ErrBKOSSnapshotStopLogging( IsIncrementalSnapshot() ) );
    }
    Assert( NULL == pinstLGFlush );

    for ( pinstCheckpoint = GetFirstInstance(); NULL != pinstCheckpoint; pinstCheckpoint = GetNextInstance() )
    {
        pinstCheckpoint->m_plog->LockCheckpoint();
        pinstCheckpoint->m_pbackup->BKInitForSnapshot(
                IsIncrementalSnapshot(),
                pinstCheckpoint->m_plog->LgposGetCheckpoint().le_lGeneration );
    }
    Assert( NULL == pinstCheckpoint );

    Assert ( JET_errSuccess == err );

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

        pLog->LGLockWrite();
        pbackup->BKLockBackup();

        if ( pinst->m_pbackup->FBKBackupInProgress() || !pinst->m_fBackupAllowed )
        {
            pbackup->BKUnlockBackup();
            pLog->LGUnlockWrite();

            Assert( FFreezeInstance( pinst ) );
            pinst->m_pOSSnapshotSession = NULL;

            Error( ErrERRCheck( JET_errOSSnapshotNotAllowed ) );
        }

        pbackup->BKSetFlags( IsFullSnapshot() );

        pbackup->BKUnlockBackup();

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

        if ( statePrepare != State() )
        {
            Assert( pinst->m_pbackup->FBKBackupInProgress( BACKUP_CONTEXT::backupLocalOnly ) );
        }

        pbackup->BKResetFlags();
        
        pbackup->BKUnlockBackup();

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

        m_asigSnapshotStarted.Set();

        OSStrCbFormatW( szError, sizeof(szError), L"%d", m_errFreeze );
        UtilReportEvent( eventError, OS_SNAPSHOT_BACKUP, OS_SNAPSHOT_FREEZE_START_ERROR_ID, 2, rgszT );
        return;
    }

    m_asigSnapshotStarted.Set();


    const BOOL fTimeOut = !m_asigSnapshotThread.FWait( ulTimeOut );
    SNAPSHOT_STATE newState = fTimeOut?stateTimeOut:stateThaw;

    Thaw( fTimeOut );

    SnapshotCritEnter();
    Assert ( stateFreeze == State() );
    Assert ( FCanSwitchTo( newState ) );
    SwitchTo ( newState );

    if ( fTimeOut )
    {
#ifdef OS_SNAPSHOT_TRACE
    {
        const WCHAR* rgszT[2] = { L"thread SnapshotThreadProc", szError };
        OSStrCbFormatW( szError, sizeof( szError ), L"%d", JET_errOSSnapshotTimeOut );

        UtilReportEvent( eventError, OS_SNAPSHOT_BACKUP, OS_SNAPSHOT_TRACE_ID, 2, rgszT );
    }
#endif
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

    m_asigSnapshotStarted.Wait();

    if ( JET_errSuccess > m_errFreeze )
    {
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
            if ( m_state != stateTimeOut )
            {
                return fFalse;
            }
            break;

        case stateAllowLogTruncate:
            if ( m_state != stateThaw )
            {
                return fFalse;
            }
            break;

        case statePrepare:
            if ( m_state != stateStart )
            {
                return fFalse;
            }
            break;

        case stateFreeze:
            if ( m_state != statePrepare )
            {
                return fFalse;
            }
            break;

        case stateThaw:
            if ( m_state != stateFreeze )
            {
                return fFalse;
            }
            break;

        case stateTimeOut:
            if ( m_state != stateFreeze )
            {
                return fFalse;
            }
            break;

        case stateLogTruncated:
            if ( m_state != stateAllowLogTruncate &&
                m_state != stateLogTruncated)
            {
                return fFalse;
            }
            break;

#ifdef OS_SNAPSHOT_ALLOW_TRUNCATE_ONLY
        case stateLogTruncatedNoFreeze:
            if ( m_state != statePrepare &&
               m_state != stateLogTruncatedNoFreeze)
            {
                return fFalse;
            }

            break;
#endif

        case stateAbort:
            if ( stateAbort == m_state ||
                stateEnd == m_state)
            {
                    return fFalse;
            }
            break;

        case stateEnd:
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


    switch ( m_state )
    {
        case statePrepare:
            UtilReportEvent( eventInformation, OS_SNAPSHOT_BACKUP,
                IsFullSnapshot() ?
                    ( IsCopySnapshot() ? OS_SNAPSHOT_PREPARE_COPY_ID : OS_SNAPSHOT_PREPARE_FULL_ID ) :
                    ( IsCopySnapshot() ? OS_SNAPSHOT_PREPARE_DIFFERENTIAL_ID : OS_SNAPSHOT_PREPARE_INCREMENTAL_ID ),
                1, rgszT );

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


    if( grbit & ~( JET_bitIncrementalSnapshot | JET_bitCopySnapshot | JET_bitContinueAfterThaw | JET_bitExplicitPrepare ))
    {
        return ErrERRCheck( JET_errInvalidGrbit );
    }

    if ( !psnapId )
    {
        CallR ( ErrERRCheck( JET_errInvalidParameter ) );
    }


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
#endif

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

    if ( !pSession->FCanSwitchTo( CESESnapshotSession::stateFreeze ) )
    {
        Error( ErrERRCheck( JET_errOSSnapshotInvalidSequence ) );
    }

    Assert( pinstNil != pinst );
    const ULONG             ipinst      = IpinstFromPinst( pinst );

    Assert( (size_t)ipinst < g_cpinstMax );
    Assert( pinstNil != g_rgpinst[ ipinst ] );
    Assert( g_rgpinst[ ipinst ]->m_fJetInitialized );
    if ( NULL != g_rgpinst[ ipinst ]->m_pOSSnapshotSession
        || g_rgpinst[ ipinst ]->m_pbackup->FBKBackupInProgress()
        || !g_rgpinst[ ipinst ]->m_fBackupAllowed )
    {
        Error( ErrERRCheck( JET_errOSSnapshotNotAllowed ) );
    }

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
#endif

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

    if ( CESESnapshotSession::stateFreeze != pSession->State() &&
        CESESnapshotSession::stateTimeOut != pSession->State() )
    {
        Error( ErrERRCheck( JET_errOSSnapshotInvalidSequence ) );
    }


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
        Assert ( CESESnapshotSession::stateFreeze == pSession->State() );

        CESESnapshotSession::SnapshotCritLeave();
        fInSnapCrit = fFalse;

        pSession->StopSnapshotThreadProc( fTrue  );

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
#endif

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
#endif

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

    switch ( pSession->State() )
    {
        case CESESnapshotSession::statePrepare:
            pSession->ResetBackupInProgress( NULL );
            break;
            
        case CESESnapshotSession::stateEnd:
        case CESESnapshotSession::stateAbort:
            break;

        case CESESnapshotSession::stateFreeze:
            Assert( JET_bitAbortSnapshot == grbit );
            CESESnapshotSession::SnapshotCritLeave();

            err = ErrIsamOSSnapshotThaw(snapId , NO_GRBIT );
            if ( fContinueAfterThaw )
            {
                return ErrIsamOSSnapshotEnd(snapId , grbit );
            }
            else
            {
#ifdef DEBUG
                
                CESESnapshotSession::SnapshotCritEnter();
                CESESnapshotSession* pSessionT;
                Assert( CESESnapshotSession::ErrGetSessionByID( snapId, &pSessionT ) == JET_errOSSnapshotInvalidSnapId );
                CESESnapshotSession::SnapshotCritLeave();
#endif
                return err;
            }

        case CESESnapshotSession::stateAllowLogTruncate:
        case CESESnapshotSession::stateLogTruncated:
            pSession->SaveSnapshotInfo( grbit );
            break;

#ifdef OS_SNAPSHOT_ALLOW_TRUNCATE_ONLY
        case CESESnapshotSession::stateLogTruncatedNoFreeze:

            #error "need to evaluate before enabling this code"
            break;
#endif
        case CESESnapshotSession::stateTimeOut:
            Assert( JET_bitAbortSnapshot == grbit );
            break;

        case CESESnapshotSession::stateThaw:
        case CESESnapshotSession::stateStart:
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

    Assert( cpg > 0 );
    Assert( pgnoFirst >= 1 );
    const PGNO pgnoLast = pgnoFirst + cpg - 1;

    Assert( pcpgActual != NULL );
    *pcpgActual = 0;

    AcquireIOSizeChangeLatch();

    Call( ErrPgnoLastFileSystem( &pgnoLastFileSystem ) );

    if ( pgnoFirst <= pgnoLastFileSystem )
    {
        const PGNO pgnoStart = pgnoFirst;
        const PGNO pgnoEnd = UlFunctionalMin( pgnoLast, pgnoLastFileSystem );

        Assert( pgnoStart <= pgnoEnd );

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
#endif
#endif

        err = ErrIOReadDbPages( Ifmp(), Pfapi(), (BYTE *)pvPageFirst, pgnoStart, pgnoEnd, fTrue, pgnoLastFileSystem, tc, fExtensiveChecks );

#ifdef ENABLE_LOST_FLUSH_INSTRUMENTATION
        BYTE* pvPageFirstCopy = NULL;

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
#else
            cRetriesMax = ( _wcsicmp( WszUtilProcessName(), L"Microsoft.Exchange.Store.Worker" ) == 0 ) ? 10 : 0;
#endif

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

                Assert( ( rgpgft[ ipgno ] == m_pflushmap->PgftGetPgnoFlushType( pgno ) ) ||
                        ( rgpgft[ ipgno ] == CPAGE::pgftUnknown ) );
            }
        }
#endif

        delete[] pvPageFirstCopy;

#endif

        
        RangeUnlock( pgnoStart, pgnoEnd );

        Call( err );
    }

    if ( pgnoLast > pgnoLastFileSystem )
    {
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

    if ( 0 != cpg )
    {
        PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
        tcScope->iorReason.SetIort( iortPagePatching );
        tcScope->iorReason.SetIorp( iorpPatchFix );
        Call( g_rgfmp[ ifmp ].ErrDBReadPages( pgnoStart, pv, cpg, &cpgActual, *tcScope ) );
        Assert( (ULONG)cpgActual <= cpg );
    }

    *pcbActual = cpgActual * g_cbPage;

HandleError:
    return err;
}

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

    if ( grbit & 0x2  )
    {
        AssertSz( fFalse, "We have deprecated allowing online corruption of pages" );
        return ErrERRCheck( JET_errDisabledFunctionality );
    }

    if ( !BoolParam( PinstFromIfmp( ifmp ), JET_paramEnableExternalAutoHealing ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    Call( ErrBFPatchPage( ifmp, pgno, pvToken, cbToken, pvData, cbData ) );

HandleError:
    return err;
}

LOCAL ERR ErrValidateHeaderForIncrementalReseed( const DBFILEHDR * const pdbfilehdr )
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

JETUNITTEST( IO, ErrValidateHeaderForIncrementalReseed )
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
 
    Assert( ulDAEVersionMax > 0 );
    dbfilehdr.le_ulVersion = ulDAEVersionMax - 1;
    CHECK( JET_errInvalidDatabaseVersion == ErrValidateHeaderForIncrementalReseed( &dbfilehdr ) );
    dbfilehdr.le_ulVersion = ulDAEVersionMax;

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


    *ppcprintf = CPRINTFNULL::PcprintfInstance();

#ifndef ENABLE_INC_RESEED_TRACING
    return JET_errSuccess;
#endif


    if ( !szDatabase )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( !szDatabase[ 0 ] )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }


    Call( ErrMakeIncReseedTracingNames( pfsapi, szDatabase, sizeof(wszIrsRawFile), wszIrsRawFile, sizeof(wszIrsRawBackupFile), wszIrsRawBackupFile ) );


    err = pfsapi->ErrFileOpen( wszIrsRawFile, IFileAPI::fmfNone, &pfapiSizeCheck );
    if ( JET_errSuccess == err )
    {
        QWORD cbSize;
        Call( pfapiSizeCheck->ErrSize( &cbSize, IFileAPI::filesizeLogical ) );
        delete pfapiSizeCheck;
        pfapiSizeCheck = NULL;

        if ( cbSize > ( 50 * 1024 * 1024 )  )
        {

            Call( pfsapi->ErrFileDelete( wszIrsRawBackupFile ) );


            Call( pfsapi->ErrFileMove( wszIrsRawFile, wszIrsRawBackupFile ) );
        }
    }


    CPRINTF * const pcprintfAlloc = new CPRINTFFILE( wszIrsRawFile );
    Alloc( pcprintfAlloc );


    *ppcprintf = pcprintfAlloc;


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
    if ( pirs == NULL )
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

    CPG                             m_cpgPatched;
    PGNO                            m_pgnoMin;
    PGNO                            m_pgnoMax;
    TICK                            m_tickStart;
    TICK                            m_tickFirstPage;

    WCHAR                           m_wszOriginalDatabasePath[IFileSystemAPI::cchPathMax];

private:
    CIrsOpContext() : CZeroInit( sizeof( CIrsOpContext ) ) {}
public:

    CIrsOpContext( _In_ CPRINTF * pcprintf, _In_ PCWSTR szOriginalDatabasePath, _In_ IFileAPI * const pfapiDb, CFlushMapForUnattachedDb * const pfm, DBFILEHDR * pdbfilehdr ) :
        CZeroInit( sizeof( CIrsOpContext ) )
    {
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


        if ( !FIRSFindIrsContext( pinst, this ) )
        {
            AssertSz( fFalse, "Impossible the outer ErrIRSGetAttachedIrsContext below should've caught this case, is code calling CIrsOpContext::ErrCheck... direclty?  Or have we called an IRS on a 2nd / different instance?" );
            Error( ErrERRCheck( JET_errDatabaseInvalidIncrementalReseed ) );
        }


        if ( !wszOriginalDatabasePath )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }

        if ( 0 != wcscmp( m_wszOriginalDatabasePath, wszOriginalDatabasePath ) )
        {
            Error( ErrERRCheck( JET_errDatabaseNotFound ) );
        }

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
                0 != memcmp( pdbfilehdrCheck, m_pdbfilehdr, g_cbPage ) )
        {
            (*PcprintfTrace())( "DBFILEHDR was updated while we had DB opened exclusively for IRSv2!\r\n" );
            AssertSz( fFalse, "Huh!?  Even though DB should have been open and attached the whole time, the current 'fake IRS attached' in memory header should definitely match the on on disk!  Something is horribly wrong." );
            err = ErrERRCheck( JET_errDatabaseInvalidIncrementalReseed );
        }
        OSMemoryPageFree( pdbfilehdrCheck );
        Call( err );
        
        if ( PcprintfTrace() == NULL ||
                PfapiDb() == NULL ||
                Pdbfilehdr() == NULL )
        {
            (*PcprintfTrace())( "Missing IRS required handle: %p %p %p %p\r\n",
                PcprintfTrace(), PfapiDb(), Pfm(), Pdbfilehdr() );
            AssertSz( fFalse, "Missing IRS required handle.  Begin shouldn't have let this happen." );
            Error( ErrERRCheck( JET_errDatabaseInvalidIncrementalReseed ) );
        }

        
    HandleError:
        return err;
    }

    ERR ErrFlushAttachedFiles( _In_ INST * pinst, _In_ const IOFLUSHREASON iofr )
    {
        ERR err = JET_errSuccess;

        (*PcprintfTrace())( "Flushing DB / JFM files.\r\n" );

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

        Assert( m_pfapiDb->CioNonFlushed() == 0 );

        if ( eStopState == eIrsDetachClean )
        {
            if ( m_cpgPatched != 0 )
            {

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
                continue;
            }
            ERR err;
            CallR( errCheck );
            AssertSz( fFalse, "Should be no warnings that fall through to ehre" );
        }
    }

    if ( pirsFound == NULL )
    {
        AssertSz( FNegTest( fInvalidAPIUsage ), "The client has called one of the later IRS funcs (Patch or End) with a pinst that has not had Begin called yet (or they provided a different DB name).  This is invalid, IRS context is not setup." );
        return ErrERRCheck( JET_errDatabaseInvalidIncrementalReseed );
    }

    Assert( FIRSFindIrsContext( pinst, pirsFound ) );

    *ppirs = pirsFound;

    return JET_errSuccess;
}


ERR ErrIRSAttachDatabaseForIrsV2( _Inout_ INST * const pinst, _In_ PCWSTR wszDatabase )
{
    ERR                         err = JET_errSuccess;
    CPRINTF *                   pcprintfIncReSeedTrace = NULL;
    IFileAPI*                   pfapiDb         = NULL;
    DBFILEHDR*                  pdbfilehdr      = NULL;
    CFlushMapForUnattachedDb*   pfm             = NULL;
    ULONG                       ipirsAvailable  = ulMax;
    CIrsOpContext *             pirsCheck       = NULL;

    Call( ErrBeginDatabaseIncReseedTracing( pinst->m_pfsapi, wszDatabase, &pcprintfIncReSeedTrace ) );

    TraceFuncBegun( pcprintfIncReSeedTrace, __FUNCTION__ );

    if ( !wszDatabase )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( !wszDatabase[ 0 ] )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    Call( pinst->m_pfsapi->ErrFileOpen( wszDatabase, IFileAPI::fmfNone, &pfapiDb ) );

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

    Alloc( pinst->m_rgpirs[ipirsAvailable] = new CIrsOpContext( pcprintfIncReSeedTrace, wszDatabase, pfapiDb, pfm, pdbfilehdr ) );

    CallS( ErrIRSGetAttachedIrsContext( pinst, wszDatabase, &pirsCheck ) );
    Assert( pirsCheck );
    Assert( pirsCheck == pinst->m_rgpirs[ipirsAvailable] );

    pcprintfIncReSeedTrace = NULL;
    pfapiDb = NULL;
    pfm = NULL;
    pdbfilehdr = NULL;

    Assert( err >= JET_errSuccess );

HandleError:

    if ( err < JET_errSuccess )
    {
        if ( pcprintfIncReSeedTrace )
        {
            TraceFuncComplete( pcprintfIncReSeedTrace , __FUNCTION__, err );
        }

        delete pfm;
        OSMemoryPageFree( pdbfilehdr );
        delete pfapiDb;
        EndDatabaseIncReseedTracing( &pcprintfIncReSeedTrace );

        Assert( ipirsAvailable == ulMax ||
                pinst->m_rgpirs == NULL ||
                pinst->m_rgpirs[ipirsAvailable] == NULL );
    }
    else
    {
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


ERR ErrIsamBeginDatabaseIncrementalReseed(
    __in JET_INSTANCE   jinst,
    __in JET_PCWSTR     szDatabase,
    __in JET_GRBIT      grbit )
{
    ERR                     err             = JET_errSuccess;
    INST* const             pinst           = (INST *)jinst;
    CIrsOpContext *         pirs            = NULL;
    CPRINTF *               pcprintfIncReSeedTrace = NULL;


    Call( ErrIRSAttachDatabaseForIrsV2( pinst, szDatabase ) );

    CallS( ErrIRSGetAttachedIrsContext( pinst, szDatabase, &pirs ) );

    TraceFuncBegun( pirs->PcprintfTrace(), __FUNCTION__ );

    DBFILEHDR * pdbfilehdr = pirs->Pdbfilehdr();
    pcprintfIncReSeedTrace = pirs->PcprintfTrace();
    Assert( pdbfilehdr && pcprintfIncReSeedTrace );


    if ( grbit )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    (*pcprintfIncReSeedTrace)( "The database header at beginning of BeginIncReseed:\r\n" );
    pdbfilehdr->DumpLite( pcprintfIncReSeedTrace, "\r\n" );

    Call( ErrValidateHeaderForIncrementalReseed( pdbfilehdr ) );

    if ( pdbfilehdr->Dbstate() == JET_dbstateDirtyShutdown ||
            pdbfilehdr->Dbstate() == JET_dbstateDirtyAndPatchedShutdown )
    {
    }
    else if ( pdbfilehdr->Dbstate() == JET_dbstateIncrementalReseedInProgress )
    {
        Error( JET_errSuccess );
    }
    else
    {
        (*pcprintfIncReSeedTrace)( "Error: Bad dbstate, pdbfilehdr->Dbstate() = %d\r\n", (ULONG)(pdbfilehdr->Dbstate()) );
        Error( ErrERRCheck( JET_errDatabaseInvalidIncrementalReseed ) );
    }

    pdbfilehdr->SetDbstate( JET_dbstateIncrementalReseedInProgress, lGenerationInvalid, lGenerationInvalid, NULL, fTrue );
    pdbfilehdr->le_ulIncrementalReseedCount++;
    LGIGetDateTime( &pdbfilehdr->logtimeIncrementalReseed );

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
        Assert( pcprintfIncReSeedTrace );
        (*pcprintfIncReSeedTrace)( "Page Patch Records (all numbers in hex): P,p=<Pgno>,i[nitialPageImage]:<objid>,<dbtime>,a[fterPatchedImage]:<objid>,<dbtime>,<errOfPatchOp>\r\n", err );
    }
    else
    {
        if ( pirs )
        {
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
            Error( ErrERRCheck( JET_errNoAttachmentsFailedIncrementalReseed ) );
        }

        (*pirs->PcprintfTrace())( "Error: No attachment info(lgen=%d): %I64x\r\n",
                        (ULONG)plgfilehdr->lgfilehdr.le_lGeneration, (__int64)pattachinfo );
        Error( ErrERRCheck( JET_errDatabaseFailedIncrementalReseed ) );
    }

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

    Call( ErrIRSGetAttachedIrsContext( pinst, szDatabase, &pirs ) );
    Assert( pirs );

    DBFILEHDR *             pdbfilehdr                                      = pirs->Pdbfilehdr();

    TraceFuncBegun( pirs->PcprintfTrace(), __FUNCTION__ );

    if ( grbit == JET_bitEndDatabaseIncrementalReseedCancel )
    {
        if ( genMinRequired != 0 ||
                genFirstDivergedLog != 0 ||
                genMaxRequired != 0 )
        {
            AssertSz( FNegTest( fInvalidAPIUsage ), "Client is testing our patience.  Get it right." );
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }


        if ( pirs )
        {
            CallS( ErrIRSDetachDatabaseIrsHandles( pinst, pirs, szDatabase, CIrsOpContext::eIrsDetachError ) );
            Assert( !FIRSFindIrsContext( pinst, pirs ) );
        }
        return JET_errSuccess;
    }


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
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    if ( genFirstDivergedLog == 0 )
    {
        genMaxRequired = max( genMaxRequired, (ULONG)pdbfilehdr->le_lGenMaxRequired );
    }

    Alloc( plgfilehdrCheck = (LGFILEHDR*)PvOSMemoryPageAlloc( sizeof( LGFILEHDR ), NULL ) );


    Call( pirs->ErrFlushAttachedFiles( pinst, iofrIncReseedUtil ) );


    (*pirs->PcprintfTrace())( "The database header at beginning of EndIncReseed:\r\n" );
    pdbfilehdr->DumpLite( pirs->PcprintfTrace(), "\r\n" );

    IRSEnforce( pirs, ErrValidateHeaderForIncrementalReseed( pdbfilehdr ) >= JET_errSuccess );
    IRSEnforce( pirs, pdbfilehdr->Dbstate() == JET_dbstateIncrementalReseedInProgress );
    
    (*pirs->PcprintfTrace())( "Log Generations [min(%d Passed, %d DB) - Div %d - Max %d\r\n",
            genMinRequired, (LONG)(pdbfilehdr->le_lGenMinRequired), genFirstDivergedLog, genMaxRequired );

    genMinRequired = min( (LONG) genMinRequired, pdbfilehdr->le_lGenMinRequired );

    BOOL fFindLowerAttachInfo = ( genFirstDivergedLog == 0 || pdbfilehdr->le_lgposAttach.le_lGeneration >= (LONG)genFirstDivergedLog );

    BOOL fForceLowerAttachInfo = (BOOL)UlConfigOverrideInjection( 38361, fFalse );

    ULONG genMinAttachInfo = 0;
    ERR errFindAttach = errNotFound;
    
RestartFromLowerLogGeneration:

    Assert( pdbfilehdr->le_lgposAttach.le_lGeneration < (LONG)genFirstDivergedLog ||
            fFindLowerAttachInfo ||
            genMinRequired == genMinAttachInfo );
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

            Assert( genMinAttachInfo < genMinRequired );


            (*pirs->PcprintfTrace())( "Moving genMinRequired (%d --> %d) BACK to find valid attachinfo!\r\n",
                    genMinRequired, genMinAttachInfo );
            (*pirs->PcprintfTrace())( "UPDATED: Log Generations [Min - Div - Max]: %d - %d - %d\r\n",
                    genMinRequired, genFirstDivergedLog, genMaxRequired );


            OSMemoryPageFree( plgfilehdrMin );
            plgfilehdrMin = NULL;
            OSMemoryPageFree( plgfilehdrMax );
            plgfilehdrMax = NULL;


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

    CallS( pfsapi->ErrPathBuild(    SzParam( pinst, JET_paramSystemPath ),
                                    SzParam( pinst, JET_paramBaseName ),
                                    ( UlParam( pinst, JET_paramLegacyFileNames ) & JET_bitESE98FileNames ) ? wszOldChkExt : wszNewChkExt,
                                    szCheckpointFile ) )
    err = pfsapi->ErrFileOpen( szCheckpointFile, IFileAPI::fmfNone, &pfapiCheckpoint );
    if ( err < JET_errSuccess )
    {
        if ( JET_errFileNotFound == err )
        {
            Call( pfsapi->ErrFileCreate( szCheckpointFile, IFileAPI::fmfOverwriteExisting, &pfapiCheckpoint ) );
        }
    }
    Call( err );


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

    pdbfilehdr->SetDbstate( JET_dbstateDirtyAndPatchedShutdown, genMinRequired, genMaxRequired, &plgfilehdrMax->lgfilehdr.tmCreate, fTrue );

    Assert( CmpLgpos( pdbfilehdr->le_lgposAttach, lgposMin ) != 0 );
    Assert( ( CmpLgpos( pdbfilehdr->le_lgposLastResize, lgposMin ) == 0 ) ||
            ( CmpLgpos( pdbfilehdr->le_lgposLastResize, pdbfilehdr->le_lgposAttach ) >= 0 ) );

    if ( ( genFirstDivergedLog != 0 ) &&
         ( CmpLgpos( pdbfilehdr->le_lgposLastResize, lgposMin ) != 0 ) &&
         ( genFirstDivergedLog <= (ULONG)pdbfilehdr->le_lgposLastResize.le_lGeneration ) )
    {
        LGPOS lgposLastResize = lgposMin;
        lgposLastResize.lGeneration = (LONG)genFirstDivergedLog;
        pdbfilehdr->le_lgposLastResize = lgposLastResize;
    }

    pdbfilehdr->le_lGenRecovering = 0;

    if ( CmpLgpos( &attachinfo.le_lgposAttach, &pdbfilehdr->le_lgposAttach ) <= 0 )
    {
        LGIGetDateTime( &pdbfilehdr->logtimeAttach );
        UtilMemCpy( &pdbfilehdr->le_lgposAttach, &attachinfo.le_lgposAttach, sizeof( pdbfilehdr->le_lgposAttach ) );
        LGIGetDateTime( &pdbfilehdr->logtimeConsistent );
        UtilMemCpy( &pdbfilehdr->le_lgposConsistent, &attachinfo.le_lgposConsistent, sizeof( pdbfilehdr->le_lgposConsistent ) );
    }

    Assert( ( CmpLgpos( pdbfilehdr->le_lgposLastResize, lgposMin ) == 0 ) ||
            ( CmpLgpos( pdbfilehdr->le_lgposLastResize, pdbfilehdr->le_lgposAttach ) >= 0 ) );

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
        pirs->Pfm()->SetDbGenMinRequired( pdbfilehdr->le_lGenMinRequired );
        pirs->Pfm()->SetDbGenMinConsistent( pdbfilehdr->le_lGenMinConsistent );

        Call( pirs->Pfm()->ErrFlushAllSections( OnDebug( fTrue ) ) );
    }

    pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration   = genMinRequired;
    pcheckpoint->checkpoint.le_lgposCheckpoint.le_isec          = (USHORT)pinst->m_plog->CSecLGHeader();
    pcheckpoint->checkpoint.le_lgposCheckpoint.le_ib            = 0;
    pcheckpoint->checkpoint.le_lgposDbConsistency               = pcheckpoint->checkpoint.le_lgposCheckpoint;

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

    UtilMemCpy( &pcheckpoint->checkpoint.dbms_param, &plgfilehdrMin->lgfilehdr.dbms_param, sizeof( pcheckpoint->checkpoint.dbms_param ) );
    UtilMemCpy( pcheckpoint->rgbAttach, plgfilehdrMin->rgbAttach, cbAttach );


    Call( ErrUtilWriteCheckpointHeaders( pinst, pfsapi, szCheckpointFile, iofrIncReseedUtil, pcheckpoint, pfapiCheckpoint ) );


    (*pirs->PcprintfTrace())( "The database header at end of EndIncReseed:\r\n" );
    pdbfilehdr->DumpLite( pirs->PcprintfTrace(), "\r\n" );

    Call( ErrUtilWriteUnattachedDatabaseHeaders( pinst, pfsapi, szDatabase, pdbfilehdr, pirs->PfapiDb(), pirs->Pfm() ) );

    (*pirs->PcprintfTrace())( "Completed IRSv2 Header Update!\r\n" );
    Call( ErrIRSDetachDatabaseIrsHandles( pinst, pirs, szDatabase, CIrsOpContext::eIrsDetachClean ) );
    Assert( !FIRSFindIrsContext( pinst, pirs ) );

HandleError:

    if ( err < JET_errSuccess && pirs )
    {
        TraceFuncComplete( pirs->PcprintfTrace(), __FUNCTION__, err );
    }

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

    OnDebug( Ptls()->fFfbFreePath = fTrue );


    Call( ErrIRSGetAttachedIrsContext( pinst, szDatabase, &pirs ) );
    Assert( pirs );

    (*pirs->PcprintfTrace())( "P," );


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

    IRSEnforce( pirs, ErrValidateHeaderForIncrementalReseed( pirs->Pdbfilehdr() ) >= JET_errSuccess );
    IRSEnforce( pirs, pirs->Pdbfilehdr()->Dbstate() == JET_dbstateIncrementalReseedInProgress );

    if ( 0 != cpg )
    {
#ifdef ENABLE_INC_RESEED_TRACING
        Alloc( pbPrePages = (BYTE*)PvOSMemoryPageAlloc( cpg * g_cbPage, NULL ) );

        const ERR errPreImageRead = pirs->PfapiDb()->ErrIORead( *tcPatchPage, OffsetOfPgno( pgnoStart ), cpg * g_cbPage, pbPrePages, QosSyncDefault( pinst ) );
#endif

        Alloc( rgb = (BYTE*)PvOSMemoryPageAlloc( cpg * g_cbPage, NULL ) );
        if ( grbit & JET_bitTestUninitShrunkPageImage )
        {
            for ( PGNO pgnoT = pgnoStart; pgnoT < pgnoStart + cpg; pgnoT++ )
            {
                CPAGE cpage;
                const bool fPreviouslySet = FNegTestSet( fInvalidUsage );
                cpage.GetShrunkPage( ifmpNil, pgnoT, rgb + g_cbPage * ( pgnoT - pgnoStart ), g_cbPage );
                FNegTestSet( fPreviouslySet );
            }
        }
        else
        {
            UtilMemCpy( rgb, pv, cpg * g_cbPage );
        }

        for ( PGNO pgnoT = pgnoStart; pgnoT < pgnoStart + cpg; pgnoT++ )
        {
            CPageValidationNullAction nullaction;
            
            AssertTrack( pgnoT != 0, "IllegalAttemptPatchingDbShadowHdr" );

            CPAGE cpageT;
            cpageT.LoadPage( ifmpNil, pgnoT, rgb + ( pgnoT - pgnoStart ) * g_cbPage, g_cbPage );
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
            CPAGE cpagePreT;
            cpagePreT.LoadPage( ifmpNil, pgnoT, pbPrePages + ( pgnoT - pgnoStart ) * g_cbPage, g_cbPage );

            const ERR errPreImageVerify = cpagePreT.ErrValidatePage( pgvfDoNotCheckForLostFlush, &nullaction );


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

        Call( pirs->PfapiDb()->ErrSize( &cbSize, IFileAPI::filesizeLogical ) );
        const QWORD cbNewSize = OffsetOfPgno( pgnoStart ) + (QWORD)cpg * g_cbPage;
        if ( cbNewSize > cbSize )
        {
            const QWORD cbNewSizeEffective = roundup( cbNewSize, (QWORD)UlParam( pinst, JET_paramDbExtensionSize ) * g_cbPage );
            Call( pirs->PfapiDb()->ErrSetSize( *tcPatchPage, cbNewSizeEffective, fTrue, QosSyncDefault( pinst ) ) );
        }

        pirs->Pdbfilehdr()->le_ulPagePatchCount += cpg;
        LGIGetDateTime( &pirs->Pdbfilehdr()->logtimePagePatch );

        Call( ErrUtilWriteUnattachedDatabaseHeaders( pinst, pinst->m_pfsapi, szDatabase, pirs->Pdbfilehdr(), pirs->PfapiDb(), pirs->Pfm() ) );

        if ( pirs->Pfm() != NULL )
        {
            Call( pirs->Pfm()->ErrSyncRangeInvalidateFlushType( pgnoStart, cpg ) );
        }

        Call( pirs->PfapiDb()->ErrIOWrite(
            *tcPatchPage,
            OffsetOfPgno( pgnoStart ),
            cpg * g_cbPage,
            rgb,
            QosSyncDefault( pinst ) ) );

        AssertTrack( pgnoStart != 0, "IllegalPatchingDbShadowHdrWritten" );

        if ( pirs->Pfm() != NULL )
        {
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
            Assert( ( ( pgnoStart + cpg - 1 ) - pgnoStart + 1 ) == cpg );
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
    else
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

    Call( ErrBeginDatabaseIncReseedTracing( pfsapi, wszDatabase, &pcprintfTraceFile ) );
    __int64                 fileTime;
    WCHAR                   szDate[32];
    WCHAR                   szTime[32];
    size_t                  cchRequired;
    fileTime = UtilGetCurrentFileTime();
    (void)ErrUtilFormatFileTimeAsTimeWithSeconds( fileTime, szTime, _countof(szTime), &cchRequired );
    (void)ErrUtilFormatFileTimeAsDate( fileTime, szDate, _countof(szDate), &cchRequired );
    (*pcprintfTraceFile)( "Begin " __FUNCTION__ "().  Time %ws %ws\r\n", szTime, szDate );

    
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


    Alloc( pdbfilehdr = static_cast<DBFILEHDR * >( PvOSMemoryPageAlloc( g_cbPage, NULL ) ) );
    Alloc( plgfilehdr = static_cast<LGFILEHDR * >( PvOSMemoryPageAlloc( sizeof(LGFILEHDR), NULL ) ) );


    Call( ErrUtilReadShadowedHeader(
            pinstNil,
            pfsapi,
            wszDatabase,
            reinterpret_cast<BYTE *>( pdbfilehdr ),
            g_cbPage,
            OffsetOf( DBFILEHDR_FIX, le_cbPageSize ),
            urhfReadOnly ) );

    (*pcprintfTraceFile)( "Header before removing the log file:\r\n" );
    pdbfilehdr->DumpLite( pcprintfTraceFile, "\r\n" );

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


    Assert( pdbfilehdr->le_lGenMaxCommitted >= pdbfilehdr->le_lGenMaxRequired );
    if( JET_dbstateDirtyShutdown != pdbfilehdr->Dbstate() )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if( 0 != memcmp( &pdbfilehdr->signLog, &plgfilehdr->lgfilehdr.signLog, sizeof(SIGNATURE) ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    const LONG lGeneration = plgfilehdr->lgfilehdr.le_lGeneration;

    if( lGeneration > pdbfilehdr->le_lGenMaxCommitted )
    {
        (*pcprintfTraceFile)( "This logfile is neither required nor committed, nothing needs to be done ... bailing out with success.\r\n" );
        Assert( JET_errSuccess == err );
        goto HandleError;
    }
    
    if( lGeneration < pdbfilehdr->le_lGenMaxCommitted )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if( lGeneration <= pdbfilehdr->le_lGenMaxRequired )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }


    
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
    pdbfilehdr->DumpLite( pcprintfTraceFile, "\r\n" );

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
        Assert( ( FUtilZeroed( ((const BYTE*)pdbfilehdr.get()) + offsetof( DBFILEHDR, rgbReserved ), sizeof( pdbfilehdr->rgbReserved ) ) &&
                  FUtilZeroed( ((const BYTE*)pdbfilehdr.get()) + offsetof( DBFILEHDR, rgbReserved2 ), sizeof( pdbfilehdr->rgbReserved2 ) ) ) ||
                    CmpDbVer( pdbfilehdr->Dbv(), PfmtversEngineDefault()->dbv ) > 0 );

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
    Assert( !( grbitQOS & qosIOCompleteWriteGameOn ) );

    
    for ( DWORD ib = 0; ib < cbData; ib += g_cbPage )
    {
        ERR         errVerify   = JET_errSuccess;
        const PGNO pgnoExpected = preaddata->fCheckPagesOffset ? PgnoOfOffset( ibOffset + ib ) : pgnoNull;

        
        if ( err < 0 )
        {
            errVerify = err;
        }
        
        else
        {
            
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


        if ( JET_errPageNotInitialized == errVerify )
        {
            errVerify = JET_errSuccess;
        }

        if ( errVerify >= JET_errSuccess )
        {
            errVerify = ErrFaultInjection(50191);
        }


        if ( errVerify < JET_errSuccess )
        {

            preaddata->err = errVerify;

            if ( BoolParam( PinstFromIfmp( preaddata->ifmp ), JET_paramEnableExternalAutoHealing ) &&
                PagePatching::FIsPatchableError( errVerify ) )
            {
                PagePatching::RequestPagePatchOnNewThread( preaddata->ifmp, pgnoExpected );
            }
        }
    }

    
    if ( !AtomicDecrement( (LONG*)preaddata->pacRead ) )
    {
        
        preaddata->pasigDone->Set();
    }
}



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
        
        QWORD ibOffset;
        ibOffset = OffsetOfPgno( pgno1 );

        DWORD cbData;
        cbData = ( pgno2 - pgno1 ) * g_cbPage;
        Expected( tc.iorReason.Iorp() != iorpNone );

        err = pfapi->ErrIORead( tc,
                                ibOffset,
                                cbData,
                                pbData,
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

    
    if ( AtomicExchangeAdd( (LONG*)&acRead, cReadIssue ) + cReadIssue != 0 )
    {
        CallS( pfapi->ErrIOIssue() );
        asigDone.Wait();
    }

    if ( tc.iorReason.Iorp() == iorpBackup )
    {
        PERFOpt( cBKReadPages.Add( PinstFromIfmp( ifmp ), cReadIssue ) );
    }

    
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
BOOL FIOCheckUserDbNonFlushedIos( const INST * const pinst, const __int64 cioPerDbOutstandingLimit , IFMP ifmpTargetedDB )
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
#endif

#ifdef ENABLE_JET_UNIT_TEST

#pragma warning( push )

#pragma warning( disable: 4307 )

JETUNITTESTDB( IO, TestValidDBReadPage, dwOpenDatabase )
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

JETUNITTESTDB( IO, TestValidDBReadPages, dwOpenDatabase )
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

JETUNITTESTDB( IO, TestOutOfRangeDBReadPage, dwOpenDatabase )
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

JETUNITTESTDB( IO, TestOutOfRangeDBReadPages, dwOpenDatabase )
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

JETUNITTESTDB( IO, TestPartlyOutOfRangeDBReadPages, dwOpenDatabase )
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

JETUNITTESTDB( IO, TestPartlyOutOfRangeDBReadManyPages, dwOpenDatabase )
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
#pragma warning( pop )

#endif
