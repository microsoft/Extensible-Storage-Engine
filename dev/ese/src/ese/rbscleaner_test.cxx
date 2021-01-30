// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#ifndef ENABLE_JET_UNIT_TEST
#error This file should only be compiled with the unit tests!
#endif


#define MaxTestRunTimeInMSec    2000
#define SleepTillConditionSatisfied( condition, cmsecMinSleep, cmsecMaxSleep )      \
{                                                                                   \
    DWORD cmsecCurSleep = 0;                                                        \
    while ( true != (condition) && cmsecCurSleep < cmsecMaxSleep )                  \
    {                                                                               \
        UtilSleep( cmsecMinSleep );                                                 \
        cmsecCurSleep += cmsecMinSleep;                                             \
    }                                                                               \
    CHECK( (condition) );                                                           \
}

class RBSCleanerTestConfig : public IRBSCleanerConfig
{
public:
    RBSCleanerTestConfig() { SetDefaultTestState(); }
    virtual ~RBSCleanerTestConfig() {}
    
    INT CPassesMax() { return m_cPassesMax; }
    VOID SetCPassesMax( INT cPassesMax ) { m_cPassesMax = cPassesMax; }

    BOOL FEnableCleanup() { return m_fEnableCleanup; }
    VOID SetEnableCleanup( BOOL fEnableCleanup ) { m_fEnableCleanup = fEnableCleanup; }

    QWORD CbLowDiskSpaceThreshold() { return m_cbLowDiskSpaceThreshold; }
    VOID SetCbLowDiskSpaceThreshold( QWORD cbLowDiskSpaceThreshold ) { m_cbLowDiskSpaceThreshold = cbLowDiskSpaceThreshold; }

    QWORD CbMaxSpaceForRBSWhenLowDiskSpace() { return m_cbMaxSpaceForRBSWhenLowDiskSpace; }
    VOID SetCbMaxSpaceForRBSWhenLowDiskSpace( QWORD cbMaxSpaceForRBSWhenLowDiskSpace ) { m_cbMaxSpaceForRBSWhenLowDiskSpace = cbMaxSpaceForRBSWhenLowDiskSpace; }

    INT CSecRBSMaxTimeSpan() { return m_cSecRBSMaxTimeSpan; }
    VOID SetCSecRBSMaxTimeSpan( INT cSecRBSMaxTimeSpan ) { m_cSecRBSMaxTimeSpan = cSecRBSMaxTimeSpan; }

    INT CSecMinCleanupIntervalTime() { return m_cSecMinCleanupIntervalTime; }
    VOID SetCSecMinCleanupIntervalTime( INT cSecMinCleanupIntervalTime ) { m_cSecMinCleanupIntervalTime = cSecMinCleanupIntervalTime; }

    LONG LFirstValidRBSGen() { return m_lFirstValidRBSGen; }
    VOID LFirstValidRBSGen( LONG lFirstValidRBSGen ) { m_lFirstValidRBSGen = lFirstValidRBSGen; }

    VOID SetDefaultTestState()
    {
        m_cPassesMax = 10;
        m_fEnableCleanup = fTrue;
        m_cbLowDiskSpaceThreshold = 1073741824;
        m_cbMaxSpaceForRBSWhenLowDiskSpace = 1048576;
        m_cSecRBSMaxTimeSpan = 300;
        m_cSecMinCleanupIntervalTime = 1;
        m_lFirstValidRBSGen = 1;
    }

private:
    INT     m_cPassesMax;
    BOOL    m_fEnableCleanup;
    QWORD   m_cbLowDiskSpaceThreshold;
    QWORD   m_cbMaxSpaceForRBSWhenLowDiskSpace;
    INT     m_cSecRBSMaxTimeSpan;
    INT     m_cSecMinCleanupIntervalTime;
    LONG    m_lFirstValidRBSGen;
};

class RBSCleanerTestState : public IRBSCleanerState
{
private:
    __int64                         m_ftPrevPassCompletionTime;
    __int64                         m_ftPassStartTime;
    INT                             m_cPassesFinished;
public:
    RBSCleanerTestState();
    virtual ~RBSCleanerTestState() {}
   
    __int64 FtPrevPassCompletionTime() { return m_ftPrevPassCompletionTime; }
    VOID SetFtPrevPassCompletionTime( __int64 ftPrevPassCompletionTime) { m_ftPrevPassCompletionTime = ftPrevPassCompletionTime; }

    __int64 FtPassStartTime() { return m_ftPassStartTime; }
    VOID SetPassStartTime( __int64 ftPassStartTime ) { m_ftPassStartTime = ftPassStartTime; }
    VOID SetPassStartTime() { m_ftPassStartTime = UtilGetCurrentFileTime(); }

    INT CPassesFinished() { return m_cPassesFinished; }
    VOID SetCPassesFinished( INT cPassesFinished ) { m_cPassesFinished = cPassesFinished; }

    VOID CompletedPass();
};

RBSCleanerTestState::RBSCleanerTestState() 
{
    m_ftPrevPassCompletionTime = 0;
    m_ftPassStartTime = 0;
    m_cPassesFinished = 0;
}

INLINE VOID RBSCleanerTestState::CompletedPass()
{
    m_ftPrevPassCompletionTime = UtilGetCurrentFileTime();
    m_cPassesFinished++;
}

class RBSCleanerTestIOOperator : public IRBSCleanerIOOperator
{
public:
    RBSCleanerTestIOOperator( );
    virtual ~RBSCleanerTestIOOperator() {}

    ERR ErrRBSDiskSpace( QWORD* pcbFreeForUser );
    ERR ErrGetDirSize( PCWSTR wszDirPath, _Out_ QWORD* pcbSize );
    ERR ErrRBSGetLowestAndHighestGen( LONG* plRBSGenMin, LONG* plRBSGenMax );
    ERR ErrRBSFilePathForGen( __out_bcount ( cbDirPath ) WCHAR* wszRBSDirPath, LONG cbDirPath, __out_bcount ( cbFilePath ) WCHAR* wszRBSFilePath, LONG cbFilePath, LONG lRBSGen );
    ERR ErrRBSFileHeader( PCWSTR wszRBSFilePath, _Out_ RBSFILEHDR* prbsfilehdr );
    ERR ErrRemoveFolder( PCWSTR wszDirPath, PCWSTR wszRBSRemoveReason);
    PCWSTR WSZRBSAbsRootDirPath() { return L""; }

    ERR m_errRBSDiskSpace;
    ERR m_errGetDirSize;
    ERR m_errRBSGetLowestAndHighestGen;
    ERR m_errRBSFilePathForGen;
    ERR m_errRBSFileHdr;
    ERR m_errRemoveFolder;

    INT m_cRemoveFolderCalls;
    INT m_lastRemovedRBSGen;

    INT m_cSecBetweenCreateTime;
    INT m_cSecLastCreateTimeFromCurrentTime;

    LONG m_lRBSGenMin;
    LONG m_lRBSGenMax;

    QWORD m_cbDirSize;
    QWORD m_cbDiskSize;

    LONG    m_lRBSGenWithInvalidPrevTime;
};

RBSCleanerTestIOOperator::RBSCleanerTestIOOperator()
{
    m_errRBSDiskSpace               = JET_errSuccess;
    m_errGetDirSize                 = JET_errSuccess;
    m_errRBSGetLowestAndHighestGen  = JET_errSuccess;
    m_errRBSFilePathForGen          = JET_errSuccess;
    m_errRBSFileHdr                 = JET_errSuccess;
    m_errRemoveFolder               = JET_errSuccess;

    m_cRemoveFolderCalls                = 0;
    m_lastRemovedRBSGen                 = 0;
    m_cSecBetweenCreateTime             = 300;
    m_cSecLastCreateTimeFromCurrentTime = 300;

    m_lRBSGenMin = 1;
    m_lRBSGenMax = 10;

    m_cbDirSize     = 104857;
    m_cbDiskSize = 2147483648;

    m_lRBSGenWithInvalidPrevTime = 1;
}

ERR RBSCleanerTestIOOperator::ErrRBSDiskSpace( QWORD* pcbFreeForUser )
{
    if ( m_errRBSDiskSpace == JET_errSuccess )
    {
        *pcbFreeForUser = m_cbDiskSize - ( m_cbDirSize * ( m_lRBSGenMax - m_lRBSGenMin + 1 ) );
    }

    return m_errRBSDiskSpace;
}

ERR RBSCleanerTestIOOperator::ErrGetDirSize( PCWSTR wszDirPath, _Out_ QWORD* pcbSize )
{
    if ( m_errGetDirSize == JET_errSuccess )
    {
        if ( wcscmp( wszDirPath, L"" ) == 0 )
        {
            *pcbSize = m_cbDirSize * ( m_lRBSGenMax - m_lRBSGenMin + 1 );
        }
        else
        {
            *pcbSize = m_cbDirSize;
        }
    }

    return m_errGetDirSize;
}

ERR RBSCleanerTestIOOperator::ErrRBSGetLowestAndHighestGen( LONG* plRBSGenMin, LONG* plRBSGenMax )
{
    if ( m_errRBSGetLowestAndHighestGen == JET_errSuccess )
    {
        *plRBSGenMin = m_lRBSGenMin;
        *plRBSGenMax = m_lRBSGenMax;
    }

    return m_errRBSGetLowestAndHighestGen;
}

ERR RBSCleanerTestIOOperator::ErrRBSFilePathForGen( __out_bcount ( cbDirPath ) WCHAR* wszRBSDirPath, LONG cbDirPath, __out_bcount ( cbFilePath ) WCHAR* wszRBSFilePath, LONG cbFilePath, LONG lRBSGen )
{
    ERR err = JET_errSuccess;
    if ( m_errRBSFilePathForGen == JET_errSuccess )
    {
        WCHAR wszRBSGen[ cbOSFSAPI_MAX_PATHW ];
        swprintf_s( wszRBSGen, ARRAYSIZE(wszRBSGen), L"%d", lRBSGen);

        Call( ErrOSStrCbCopyW( wszRBSDirPath, cbDirPath, wszRBSGen ) );
        Call( ErrOSStrCbCopyW( wszRBSFilePath, cbDirPath, wszRBSGen ) );
    }

    return m_errRBSFilePathForGen;

HandleError:
    return err;
}

ERR RBSCleanerTestIOOperator::ErrRBSFileHeader( PCWSTR wszRBSFilePath, _Out_ RBSFILEHDR* prbsfilehdr )
{
    if ( m_errRBSFileHdr == JET_errSuccess )
    {
        Assert( prbsfilehdr );
        INT     lRBSGen         = wcstol( wszRBSFilePath, NULL, 10 );
        __int64 ftCurrent       = UtilGetCurrentFileTime();
        __int64 cSecBehind      = m_cSecLastCreateTimeFromCurrentTime + ( ( m_lRBSGenMax - lRBSGen ) * m_cSecBetweenCreateTime );
        __int64 cSecBehindPrev  = m_cSecLastCreateTimeFromCurrentTime + ( ( m_lRBSGenMax - lRBSGen + 1) * m_cSecBetweenCreateTime );

        if ( lRBSGen == m_lRBSGenWithInvalidPrevTime )
        {
            prbsfilehdr->rbsfilehdr.tmPrevGen = { 0 };
        }
        else
        {
            ConvertFileTimeToLogTime( ftCurrent - ( cSecBehindPrev * 10000000 ), &prbsfilehdr->rbsfilehdr.tmPrevGen );
        }

        ConvertFileTimeToLogTime( ftCurrent - ( cSecBehind * 10000000 ), &prbsfilehdr->rbsfilehdr.tmCreate );
    }

    return m_errRBSFileHdr;
}

ERR RBSCleanerTestIOOperator::ErrRemoveFolder( PCWSTR wszDirPath, PCWSTR wszRBSRemoveReason)
{
    if ( m_errRemoveFolder == JET_errSuccess )
    {
        INT     lRBSGen     = wcstol( wszDirPath, NULL, 10 );

        Assert( lRBSGen == m_lRBSGenMin );
        m_lRBSGenMin++;

        m_cRemoveFolderCalls++;
        m_lastRemovedRBSGen = lRBSGen;
    }

    return m_errRemoveFolder;
}

JETUNITTEST( RBSCleaner, RBSCleanupDisabled )
{
    unique_ptr<RBSCleanerTestConfig> pconfig( new RBSCleanerTestConfig() );
    pconfig->SetEnableCleanup( fFalse );
    pconfig->SetCSecMinCleanupIntervalTime( 0 );
    
    RBSCleanerTestState* pstate             = new RBSCleanerTestState();
    RBSCleanerTestIOOperator* piooperator   = new RBSCleanerTestIOOperator();

    INST* pinst;
    CHECKCALLS( JetCreateInstance2W( (JET_INSTANCE*) &pinst, NULL, NULL, JET_bitNil ) );
    unique_ptr<RBSCleaner> prbscleaner( new RBSCleaner( pinst, piooperator, pstate, pconfig.release() ) );
    CHECK( JET_errSuccess == prbscleaner->ErrStartCleaner() );

    UtilSleep( 10 );

    CHECK( pstate->FtPassStartTime() == 0 );
    CHECK( pstate->CPassesFinished() == 0 );
    CHECK( prbscleaner->FIsCleanerRunning() == fFalse );

    CHECKCALLS( JetTerm2( (JET_INSTANCE) pinst, JET_bitTermAbrupt ) );
}

JETUNITTEST( RBSCleaner, RBSDisabledWithNoExistingGen )
{
    __int64 ftStartTime = UtilGetCurrentFileTime();
    unique_ptr<RBSCleanerTestConfig> pconfig( new RBSCleanerTestConfig() );
    pconfig->SetCSecMinCleanupIntervalTime( 0 );
    
    RBSCleanerTestState* pstate             = new RBSCleanerTestState();
    RBSCleanerTestIOOperator* piooperator   = new RBSCleanerTestIOOperator();
    piooperator->m_lRBSGenMin = 0;
    piooperator->m_lRBSGenMax = 0;

    INST* pinst;
    CHECKCALLS( JetCreateInstance2W( (JET_INSTANCE*) &pinst, NULL, NULL, JET_bitNil ) );
    CHECKCALLS( JetSetSystemParameterW( (JET_INSTANCE*) &pinst, JET_sesidNil, JET_paramEnableRBS, 0, NULL ) );

    unique_ptr<RBSCleaner> prbscleaner( new RBSCleaner( pinst, piooperator, pstate, pconfig.release() ) );
    CHECK( JET_errSuccess == prbscleaner->ErrStartCleaner() );

    SleepTillConditionSatisfied( pstate->CPassesFinished() == 1, 2, MaxTestRunTimeInMSec );

    CHECK( pstate->FtPassStartTime() >= ftStartTime );
    CHECK( pstate->FtPrevPassCompletionTime() >= pstate->FtPassStartTime() );
    CHECK( piooperator->m_cRemoveFolderCalls == 0 );

    CHECKCALLS( JetTerm2( (JET_INSTANCE) pinst, JET_bitTermAbrupt ) );
}

JETUNITTEST( RBSCleaner, MaxPassesNotCrossed )
{
    __int64 ftStartTime = UtilGetCurrentFileTime();
    unique_ptr<RBSCleanerTestConfig> pconfig( new RBSCleanerTestConfig() );
    pconfig->SetCSecMinCleanupIntervalTime( 0 );
    pconfig->SetCPassesMax( 5 );
    
    RBSCleanerTestState* pstate             = new RBSCleanerTestState();
    RBSCleanerTestIOOperator* piooperator   = new RBSCleanerTestIOOperator();
    piooperator->m_lRBSGenMin = 1;
    piooperator->m_lRBSGenMax = 1;

    INST* pinst;
    CHECKCALLS( JetCreateInstance2W( (JET_INSTANCE*) &pinst, NULL, NULL, JET_bitNil ) );
    CHECKCALLS( JetSetSystemParameterW( (JET_INSTANCE*) &pinst, JET_sesidNil, JET_paramEnableRBS, 1, NULL ) );

    unique_ptr<RBSCleaner> prbscleaner( new RBSCleaner( pinst, piooperator, pstate, pconfig.release() ) );
    CHECK( JET_errSuccess == prbscleaner->ErrStartCleaner() );

    SleepTillConditionSatisfied( pstate->CPassesFinished() == 5, 2, MaxTestRunTimeInMSec );

    CHECK( pstate->FtPassStartTime() >= ftStartTime );
    CHECK( pstate->FtPrevPassCompletionTime() >= pstate->FtPassStartTime() );
    CHECK( piooperator->m_cRemoveFolderCalls == 0 );

    CHECKCALLS( JetTerm2( (JET_INSTANCE) pinst, JET_bitTermAbrupt ) );
}

JETUNITTEST( RBSCleaner, InvalidSnapshotsDeleted )
{
    __int64 ftStartTime = UtilGetCurrentFileTime();
    unique_ptr<RBSCleanerTestConfig> pconfig( new RBSCleanerTestConfig() );
    pconfig->SetCSecRBSMaxTimeSpan( 3600 );
    
    RBSCleanerTestState* pstate             = new RBSCleanerTestState();
    RBSCleanerTestIOOperator* piooperator   = new RBSCleanerTestIOOperator();
    piooperator->m_lRBSGenMin = 1;
    piooperator->m_lRBSGenMax = 10;

    INST* pinst;
    CHECKCALLS( JetCreateInstance2W( (JET_INSTANCE*) &pinst, NULL, NULL, JET_bitNil ) );
    CHECKCALLS( JetSetSystemParameterW( (JET_INSTANCE*) &pinst, JET_sesidNil, JET_paramEnableRBS, 1, NULL ) );

    unique_ptr<RBSCleaner> prbscleaner( new RBSCleaner( pinst, piooperator, pstate, pconfig.release() ) );
    prbscleaner->SetFirstValidGen( 3 );
    CHECK( JET_errSuccess == prbscleaner->ErrStartCleaner() );

    SleepTillConditionSatisfied( pstate->CPassesFinished() == 1, 2, MaxTestRunTimeInMSec );

    CHECK( pstate->FtPassStartTime() >= ftStartTime );
    CHECK( pstate->FtPrevPassCompletionTime() >= pstate->FtPassStartTime() );
    CHECK( piooperator->m_cRemoveFolderCalls == 2 );
    CHECK( piooperator->m_lRBSGenMin == 3 );

    CHECKCALLS( JetTerm2( (JET_INSTANCE) pinst, JET_bitTermAbrupt ) );
}

JETUNITTEST( RBSCleaner, ExpiredSnapshotRemoved )
{
    __int64 ftStartTime = UtilGetCurrentFileTime();
    
    RBSCleanerTestState* pstate             = new RBSCleanerTestState();
    RBSCleanerTestIOOperator* piooperator   = new RBSCleanerTestIOOperator();
    piooperator->m_lRBSGenMin = 1;
    piooperator->m_lRBSGenMax = 10;

    unique_ptr<RBSCleanerTestConfig> pconfig( new RBSCleanerTestConfig() );

    pconfig->SetCSecRBSMaxTimeSpan(  piooperator->m_cSecLastCreateTimeFromCurrentTime + ( ( piooperator->m_lRBSGenMax - 5 ) * piooperator->m_cSecBetweenCreateTime ) + 1);

    INST* pinst;
    CHECKCALLS( JetCreateInstance2W( (JET_INSTANCE*) &pinst, NULL, NULL, JET_bitNil ) );
    CHECKCALLS( JetSetSystemParameterW( (JET_INSTANCE*) &pinst, JET_sesidNil, JET_paramEnableRBS, 1, NULL ) );

    unique_ptr<RBSCleaner> prbscleaner( new RBSCleaner( pinst, piooperator, pstate, pconfig.release() ) );
    CHECK( JET_errSuccess == prbscleaner->ErrStartCleaner() );

    SleepTillConditionSatisfied( pstate->CPassesFinished() == 1, 2, MaxTestRunTimeInMSec );

    CHECK( pstate->FtPassStartTime() >= ftStartTime );
    CHECK( pstate->FtPrevPassCompletionTime() >= pstate->FtPassStartTime() );
    CHECK( piooperator->m_cRemoveFolderCalls == 4 );
    CHECK( piooperator->m_lRBSGenMin == 5 );

    CHECKCALLS( JetTerm2( (JET_INSTANCE) pinst, JET_bitTermAbrupt ) );
}

JETUNITTEST( RBSCleaner, LowDiskSpaceButRBSWithinLimits )
{
    __int64 ftStartTime = UtilGetCurrentFileTime();
    unique_ptr<RBSCleanerTestConfig> pconfig( new RBSCleanerTestConfig() );
    pconfig->SetCSecRBSMaxTimeSpan( 3600 );
    
    RBSCleanerTestState* pstate             = new RBSCleanerTestState();
    RBSCleanerTestIOOperator* piooperator   = new RBSCleanerTestIOOperator();
    piooperator->m_lRBSGenMin = 1;
    piooperator->m_lRBSGenMax = 10;

    QWORD cbRBSTotalSize = ( piooperator->m_lRBSGenMax - piooperator->m_lRBSGenMin + 1 ) * piooperator->m_cbDirSize;

    pconfig->SetCbLowDiskSpaceThreshold( piooperator->m_cbDiskSize - cbRBSTotalSize + 1 );

    pconfig->SetCbMaxSpaceForRBSWhenLowDiskSpace( cbRBSTotalSize + 1 );

    INST* pinst;
    CHECKCALLS( JetCreateInstance2W( (JET_INSTANCE*) &pinst, NULL, NULL, JET_bitNil ) );
    CHECKCALLS( JetSetSystemParameterW( (JET_INSTANCE*) &pinst, JET_sesidNil, JET_paramEnableRBS, 1, NULL ) );

    unique_ptr<RBSCleaner> prbscleaner( new RBSCleaner( pinst, piooperator, pstate, pconfig.release() ) );
    CHECK( JET_errSuccess == prbscleaner->ErrStartCleaner() );

    SleepTillConditionSatisfied( pstate->CPassesFinished() == 1, 2, MaxTestRunTimeInMSec );

    CHECK( pstate->FtPassStartTime() >= ftStartTime );
    CHECK( pstate->FtPrevPassCompletionTime() >= pstate->FtPassStartTime() );
    CHECK( piooperator->m_cRemoveFolderCalls == 0 );
    CHECK( piooperator->m_lRBSGenMin == 1 );

    CHECKCALLS( JetTerm2( (JET_INSTANCE) pinst, JET_bitTermAbrupt ) );
}

JETUNITTEST( RBSCleaner, LowDiskSpaceRequiringOneRBSRemoval )
{
    __int64 ftStartTime = UtilGetCurrentFileTime();
    unique_ptr<RBSCleanerTestConfig> pconfig( new RBSCleanerTestConfig() );
    pconfig->SetCSecRBSMaxTimeSpan( 3600 );
    
    RBSCleanerTestState* pstate             = new RBSCleanerTestState();
    RBSCleanerTestIOOperator* piooperator   = new RBSCleanerTestIOOperator();
    piooperator->m_lRBSGenMin = 1;
    piooperator->m_lRBSGenMax = 10;

    QWORD cbRBSTotalSize = ( piooperator->m_lRBSGenMax - piooperator->m_lRBSGenMin + 1 ) * piooperator->m_cbDirSize;

    pconfig->SetCbLowDiskSpaceThreshold( piooperator->m_cbDiskSize - cbRBSTotalSize + 1 );

    pconfig->SetCbMaxSpaceForRBSWhenLowDiskSpace( cbRBSTotalSize - ( cbRBSTotalSize/2 ) );

    INST* pinst;
    CHECKCALLS( JetCreateInstance2W( (JET_INSTANCE*) &pinst, NULL, NULL, JET_bitNil ) );
    CHECKCALLS( JetSetSystemParameterW( (JET_INSTANCE*) &pinst, JET_sesidNil, JET_paramEnableRBS, 1, NULL ) );

    unique_ptr<RBSCleaner> prbscleaner( new RBSCleaner( pinst, piooperator, pstate, pconfig.release() ) );
    CHECK( JET_errSuccess == prbscleaner->ErrStartCleaner() );

    SleepTillConditionSatisfied( pstate->CPassesFinished() == 1, 2, MaxTestRunTimeInMSec );

    CHECK( pstate->FtPassStartTime() >= ftStartTime );
    CHECK( pstate->FtPrevPassCompletionTime() >= pstate->FtPassStartTime() );
    CHECK( piooperator->m_cRemoveFolderCalls == 1 );
    CHECK( piooperator->m_lRBSGenMin == 2 );

    CHECKCALLS( JetTerm2( (JET_INSTANCE) pinst, JET_bitTermAbrupt ) );
}

JETUNITTEST( RBSCleaner, LowDiskSpaceRequiringMultipleRBSRemoval )
{
    __int64 ftStartTime = UtilGetCurrentFileTime();
    unique_ptr<RBSCleanerTestConfig> pconfig( new RBSCleanerTestConfig() );
    pconfig->SetCSecRBSMaxTimeSpan( 3600 );
    
    RBSCleanerTestState* pstate             = new RBSCleanerTestState();
    RBSCleanerTestIOOperator* piooperator   = new RBSCleanerTestIOOperator();
    piooperator->m_lRBSGenMin = 1;
    piooperator->m_lRBSGenMax = 10;

    QWORD cbRBSTotalSize = ( piooperator->m_lRBSGenMax - piooperator->m_lRBSGenMin + 1 ) * piooperator->m_cbDirSize;

    pconfig->SetCbLowDiskSpaceThreshold( piooperator->m_cbDiskSize - cbRBSTotalSize + ( cbRBSTotalSize/2 ) );

    pconfig->SetCbMaxSpaceForRBSWhenLowDiskSpace( cbRBSTotalSize - ( cbRBSTotalSize/2 ) );

    INST* pinst;
    CHECKCALLS( JetCreateInstance2W( (JET_INSTANCE*) &pinst, NULL, NULL, JET_bitNil ) );
    CHECKCALLS( JetSetSystemParameterW( (JET_INSTANCE*) &pinst, JET_sesidNil, JET_paramEnableRBS, 1, NULL ) );

    unique_ptr<RBSCleaner> prbscleaner( new RBSCleaner( pinst, piooperator, pstate, pconfig.release() ) );
    CHECK( JET_errSuccess == prbscleaner->ErrStartCleaner() );

    SleepTillConditionSatisfied( pstate->CPassesFinished() == 1, 2, MaxTestRunTimeInMSec );

    CHECK( pstate->FtPassStartTime() >= ftStartTime );
    CHECK( pstate->FtPrevPassCompletionTime() >= pstate->FtPassStartTime() );
    CHECK( piooperator->m_cRemoveFolderCalls == 5 );
    CHECK( piooperator->m_lRBSGenMin == 6 );

    CHECKCALLS( JetTerm2( (JET_INSTANCE) pinst, JET_bitTermAbrupt ) );
}

JETUNITTEST( RBSCleaner, LowDiskSpaceAllRBSRemovedExceptLatest )
{
    __int64 ftStartTime = UtilGetCurrentFileTime();
    unique_ptr<RBSCleanerTestConfig> pconfig( new RBSCleanerTestConfig() );
    pconfig->SetCSecRBSMaxTimeSpan( 3600 );
    
    RBSCleanerTestState* pstate             = new RBSCleanerTestState();
    RBSCleanerTestIOOperator* piooperator   = new RBSCleanerTestIOOperator();
    piooperator->m_lRBSGenMin = 1;
    piooperator->m_lRBSGenMax = 10;

    pconfig->SetCbLowDiskSpaceThreshold( piooperator->m_cbDiskSize );

    pconfig->SetCbMaxSpaceForRBSWhenLowDiskSpace( 0 );

    INST* pinst;
    CHECKCALLS( JetCreateInstance2W( (JET_INSTANCE*) &pinst, NULL, NULL, JET_bitNil ) );
    CHECKCALLS( JetSetSystemParameterW( (JET_INSTANCE*) &pinst, JET_sesidNil, JET_paramEnableRBS, 1, NULL ) );

    unique_ptr<RBSCleaner> prbscleaner( new RBSCleaner( pinst, piooperator, pstate, pconfig.release() ) );
    CHECK( JET_errSuccess == prbscleaner->ErrStartCleaner() );

    SleepTillConditionSatisfied( pstate->CPassesFinished() == 1, 2, MaxTestRunTimeInMSec );

    CHECK( pstate->FtPassStartTime() >= ftStartTime );
    CHECK( pstate->FtPrevPassCompletionTime() >= pstate->FtPassStartTime() );
    CHECK( piooperator->m_cRemoveFolderCalls == 9 );
    CHECK( piooperator->m_lRBSGenMin == 10 );

    CHECKCALLS( JetTerm2( (JET_INSTANCE) pinst, JET_bitTermAbrupt ) );
}

JETUNITTEST( RBSCleaner, CorruptSnapshotsDeleted )
{
    __int64 ftStartTime = UtilGetCurrentFileTime();
    unique_ptr<RBSCleanerTestConfig> pconfig( new RBSCleanerTestConfig() );
    pconfig->SetCSecRBSMaxTimeSpan( 3600 );
    
    RBSCleanerTestState* pstate             = new RBSCleanerTestState();
    RBSCleanerTestIOOperator* piooperator   = new RBSCleanerTestIOOperator();
    piooperator->m_lRBSGenMin = 1;
    piooperator->m_lRBSGenMax = 2;
    piooperator->m_errRBSFileHdr = JET_errReadVerifyFailure;

    INST* pinst;
    CHECKCALLS( JetCreateInstance2W( (JET_INSTANCE*) &pinst, NULL, NULL, JET_bitNil ) );
    CHECKCALLS( JetSetSystemParameterW( (JET_INSTANCE*) &pinst, JET_sesidNil, JET_paramEnableRBS, 1, NULL ) );

    unique_ptr<RBSCleaner> prbscleaner( new RBSCleaner( pinst, piooperator, pstate, pconfig.release() ) );
    CHECK( JET_errSuccess == prbscleaner->ErrStartCleaner() );

    SleepTillConditionSatisfied( pstate->CPassesFinished() == 1, 2, MaxTestRunTimeInMSec );

    CHECK( pstate->FtPassStartTime() >= ftStartTime );
    CHECK( pstate->FtPrevPassCompletionTime() >= pstate->FtPassStartTime() );
    CHECK( piooperator->m_cRemoveFolderCalls == 1 );
    CHECK( piooperator->m_lRBSGenMin == 2 );

    CHECKCALLS( JetTerm2( (JET_INSTANCE) pinst, JET_bitTermAbrupt ) );
}

JETUNITTEST( RBSCleaner, GetDirSizeError )
{
    __int64 ftStartTime = UtilGetCurrentFileTime();
    unique_ptr<RBSCleanerTestConfig> pconfig( new RBSCleanerTestConfig() );
    pconfig->SetCSecRBSMaxTimeSpan( 3600 );
    
    RBSCleanerTestState* pstate             = new RBSCleanerTestState();
    RBSCleanerTestIOOperator* piooperator   = new RBSCleanerTestIOOperator();
    piooperator->m_lRBSGenMin = 1;
    piooperator->m_lRBSGenMax = 10;
    piooperator->m_errGetDirSize = JET_errFileAccessDenied;

    pconfig->SetCbLowDiskSpaceThreshold( piooperator->m_cbDiskSize );

    pconfig->SetCbMaxSpaceForRBSWhenLowDiskSpace( 0 );

    INST* pinst;
    CHECKCALLS( JetCreateInstance2W( (JET_INSTANCE*) &pinst, NULL, NULL, JET_bitNil ) );
    CHECKCALLS( JetSetSystemParameterW( (JET_INSTANCE*) &pinst, JET_sesidNil, JET_paramEnableRBS, 1, NULL ) );

    unique_ptr<RBSCleaner> prbscleaner( new RBSCleaner( pinst, piooperator, pstate, pconfig.release() ) );
    CHECK( JET_errSuccess == prbscleaner->ErrStartCleaner() );

    SleepTillConditionSatisfied( pstate->CPassesFinished() == 1, 2, MaxTestRunTimeInMSec );

    CHECK( pstate->FtPassStartTime() >= ftStartTime );
    CHECK( pstate->FtPrevPassCompletionTime() >= pstate->FtPassStartTime() );
    CHECK( piooperator->m_cRemoveFolderCalls == 0 );
    CHECK( piooperator->m_lRBSGenMin == 1 );

    CHECKCALLS( JetTerm2( (JET_INSTANCE) pinst, JET_bitTermAbrupt ) );
}

JETUNITTEST( RBSCleaner, ComputeFirstValidRBSGenSetsInvalidSnapshot )
{
    __int64 ftStartTime = UtilGetCurrentFileTime();
    unique_ptr<RBSCleanerTestConfig> pconfig( new RBSCleanerTestConfig() );
    pconfig->SetCSecRBSMaxTimeSpan( 3600 );

    RBSCleanerTestState* pstate             = new RBSCleanerTestState();
    RBSCleanerTestIOOperator* piooperator   = new RBSCleanerTestIOOperator();
    piooperator->m_lRBSGenMin = 1;
    piooperator->m_lRBSGenMax = 10;

    piooperator->m_lRBSGenWithInvalidPrevTime = 7;

    INST* pinst;
    CHECKCALLS( JetCreateInstance2W( (JET_INSTANCE*) &pinst, NULL, NULL, JET_bitNil ) );
    CHECKCALLS( JetSetSystemParameterW( (JET_INSTANCE*) &pinst, JET_sesidNil, JET_paramEnableRBS, 1, NULL ) );

    unique_ptr<RBSCleaner> prbscleaner( new RBSCleaner( pinst, piooperator, pstate, pconfig.release() ) );
    CHECK( JET_errSuccess == prbscleaner->ErrStartCleaner() );

    SleepTillConditionSatisfied( pstate->CPassesFinished() == 1, 2, MaxTestRunTimeInMSec );

    CHECK( pstate->FtPassStartTime() >= ftStartTime );
    CHECK( pstate->FtPrevPassCompletionTime() >= pstate->FtPassStartTime() );
    CHECK( piooperator->m_cRemoveFolderCalls == 6 );
    CHECK( piooperator->m_lRBSGenMin == 7 );

    CHECKCALLS( JetTerm2( (JET_INSTANCE) pinst, JET_bitTermAbrupt ) );
}
