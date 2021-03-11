// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#ifndef _AFX_NO_DEBUG_CRT
#define _AFX_NO_DEBUG_CRT
#include <afx.h>
#undef _AFX_NO_DEBUG_CRT
#else
#include <afx.h>
#endif

#include "ese_common.hxx"

#define _MT_NONTHREADPOOL_


class JET_MT_SYSPARAM;
class JET_MT_THREAD;
class JET_MT_TABLE;
class JET_MT_DB;
class JET_MT_INSTANCE;
class MT_RUN;

class JMThread;
class JMRandReplace;

extern int gnDebugLevel;
extern BOOL gfRegisterForBackup;
extern BOOL gfRegisterForSurrogateBackup;


#if !defined (JET_noBackup)

#ifndef ESE_REGISTER_BACKUP
#define ESE_REGISTER_BACKUP             0x00000001
#endif
#ifndef ESE_REGISTER_ONLINE_RESTORE
#define ESE_REGISTER_ONLINE_RESTORE     0x00000002
#endif
#ifndef ESE_REGISTER_SURROGATE_BACKUP
#define ESE_REGISTER_SURROGATE_BACKUP 0x00000020
#endif

#ifdef MIDL_PASS
#define RPC_STRING      [unique, string] WCHAR *
#else
#define RPC_STRING      WCHAR *
#endif

#define BACKUP_ANNOTATION               L"MULTITEST"
#define SURROGATE_BACKUP_ANNOTATION         L"VSSACTIVE"
#define BACKUP_ANNOTATION_DISPLAY   L"JET stress simulator"
#define CALLBACK_DLL                    L"BackrestCallback.dll"

#ifndef DATABASE_MOUNTED
#define DATABASE_MOUNTED                0x00000010
#endif
#ifndef DATABASE_UNMOUNTED
#define DATABASE_UNMOUNTED          0x00000000
#endif

typedef long (CALLBACK* CallHrESEBackupRestoreRegister)(WCHAR*, unsigned long, WCHAR*, WCHAR*, GUID*);
typedef long (CALLBACK* CallHrESEBackupRestoreUnregister)(void);
typedef long (CALLBACK* CallHrESERecoverAfterRestore)(WCHAR*, WCHAR*, WCHAR*, WCHAR*);
typedef long (CALLBACK* CallHrESEBackupInstanceAbort)(JET_INSTANCE, unsigned long);

#endif

typedef unsigned ( __stdcall *PTHREAD_START )( void * );

#ifdef _MT_NONTHREADPOOL_
    #define LPTHREAD PTHREAD_START
#else
    #define LPTHREAD LPTHREAD_START_ROUTINE
#endif

JET_ERR JTRandReplace(
    JET_SESID jSesId,
    JET_TABLEID jTabId,
    JET_SETCOLUMN* pJSetColumn,
    ULONG ulSetColumn,
    long lRecord );

JET_ERR JTPeriodicCommit(
    JET_SESID jSesId,
    BOOL fLazy,
    long lCount,
    long lPeriod );

class JET_MT_SYSPARAM
{
public:
    static int              iDummyForSourceInsightParsing;
    unsigned long   ulParamID;
    ULONG_PTR   plParam;
    char            szParam[ MAX_PATH + 1 ];

    JET_MT_SYSPARAM( unsigned long ulSetParam,
                                ULONG_PTR plLongParam,
                                const char* szStringParam );
    void Apply( JET_MT_INSTANCE* pjmOwner,
                JET_SESID jCurrentSession = NULL );
};

class JET_MT_THREAD
{
public:
    static int              iDummyForSourceInsightParsing;
    JET_MT_DB*  pjmParentDatabase;
    long            lThreadNum;

    LPTHREAD    lptExecutionFunction;
    HANDLE      hThread;
    unsigned        uThreadID;

    CTime       tStart;
    CTime       tEnd;
    long            lExecutionTime;

    CTimeSpan   tTimeToExecute;
    CTime       tTimeToStop;
    long            lExecuteOperations;

    long            lBeginOffset;
    long            lNumToCommit;
    BOOL        bLazyCommit;
    BOOL        bUpdateDBSize;

    long            lNumSuccess;
    long            lNumFailure;
    BOOL        bERRORS;

    JET_MT_THREAD( JET_MT_DB* pjmOwner,
                            CTimeSpan tTime,
                            long lOperations,
                            LPTHREAD lptExecute,
                            long lCommit = 1,
                            long lBegin = 0,
                            BOOL bLazy = FALSE );
    JET_MT_THREAD( const JET_MT_THREAD& jmThread,
                            JET_MT_DB* pjmOwner );
    ~JET_MT_THREAD( void );

    void Run( void );
    void Complete( void );
    void init( void );
    BOOL stop( void );

    JET_MT_DB *pDatabase( void );
    JET_MT_TABLE *pTable( void );
    JET_MT_INSTANCE *pInstance( void );
    MT_RUN *pRun( void );
    long lThread( void );
    long lDatabase( void );
    long lInstance( void );

    DWORD RandReplace( void );
};

class JET_MT_TABLE
{
public:
    static int          iDummyForSourceInsightParsing;
    JET_TABLECREATE     jTable;
    JET_COLUMNCREATE*   jColumn;
    JET_INDEXCREATE*    jIndex;
    char                    szTableName[30];
    long                    lColCount;
    long                    lIndexCount;
    long                    lColOffset;
    long                    lIndexOffset;
    BOOL                bFinalized;
    BOOL                bSLVColumn;

    JET_MT_TABLE( const char                *szName,
                        JET_COLUMNCREATE    * jColList,
                        long                    lNumCol,
                        JET_INDEXCREATE     *jIndexList,
                        long                    lNumIndex );

    JET_MT_TABLE( const char                *szName,
                        long                    lNumCol,
                        long                    lNumIndex );

    ~JET_MT_TABLE( void );

    void AssignColumn( JET_COLUMNCREATE jNewCol );
    void AssignIndex( JET_INDEXCREATE jNewIndex );
    void Finalize( void );
    int AllocateColumnData( JET_SETCOLUMN **ppjsetcolumn );
    void FreeColumnDataP( JET_SETCOLUMN** ppJetSetColumn );
    void RandomFillColumnData( JET_SETCOLUMN    *pjsetcolumn,
                                    long                lRecord,
                                    long                lSeed );
};

class JET_MT_DB
{
public:
    static int              iDummyForSourceInsightParsing;
    JET_MT_INSTANCE     *pjmParentInstance;
    JET_MT_TABLE        *pjmTable;
    JET_MT_THREAD       **pjmThread;

    JET_DBID            jDatabaseID;
    BOOL                bAttached;
    char                    szDBName[ MAX_PATH ];
    char                    szSLVName[ MAX_PATH ];
    char                    szSLVRoot[ 32 ];
    long                    lNumThreads;
    long                    lThreadOffset;
    long                    lDatabaseNum;
    long                    lRecordsPerTable;
    long                    lDBCurrPos;
    long                    lTotalExecutes;

    JET_MT_DB( JET_MT_INSTANCE      *pjmOwner,
                        const char          *szName,
                                int                 nNum,
                                JET_MT_TABLE        *jmCurrTable,
                                long                    lNumRec,
                                BOOL                bAttach = FALSE );
    ~JET_MT_DB( void );

    void RegisterDBWithCallback( void );

    void AssignThread( CTimeSpan        tTime,
                            LPTHREAD    lptExecute,
                            long            lCommit = 1,
                            long            lBegin = 0 );
    void AssignThread( long         lOperations,
                            LPTHREAD    lptExecute,
                            long            lCommit = 1,
                            long            lBegin = 0 );
    void AssignThread( const JET_MT_THREAD &jmThread );

    void AssignThread( const JMThread& jmThread );

    void MoveDB( const char *szLocation );
    void MoveSLV( const char *szLocation );

    void AttachDatabase( void );
    void DetachDatabase( void );
    void GetDBSize( void );

    void Run( void );
    void Cleanup( void );
    void ResetThreads( void );
    long CountActiveThread( void );

    JET_MT_TABLE *pTable( void );
    JET_MT_INSTANCE *pInstance( void );
    MT_RUN *pRun( void );
    long lDatabase( void );
    long lInstance( void );
};

class JET_MT_INSTANCE
{
public:
    static int              iDummyForSourceInsightParsing;
    MT_RUN              *pjmParentRun;
    JET_MT_DB           **pjmDatabase;
    JET_MT_SYSPARAM **pjmSysParam;

    JET_INSTANCE        jJETInstance;
    JET_SESID           jSessionID;
    char                    szInstanceName[ MAX_PATH ];
    char                    szSystemPath[ MAX_PATH ];
    char                    szLogPath[ MAX_PATH ];
    long                    lNumDatabases;
    long                    lDBOffset;
    long                    lInstanceNum;

    JET_MT_INSTANCE( MT_RUN             *pjmOwner,
                            const char          *szName,
                            const char          *szSystem,
                            const char          *szLog,
                            long                    lNumDB,
                            long                    lSysParamCount = 0,
                            JET_MT_SYSPARAM *pjmSysParam[] = NULL );
    ~JET_MT_INSTANCE( void );

    void RegisterInstWithCallback( void );

    void AssignDatabase( const char             *szName,
                            JET_MT_TABLE        *jmCurrTable,
                            int                 nNumThreads,
                            long                    lNumRec,
                            BOOL                bAttach = FALSE );

    void Run( void );
    void Cleanup( void );
    void ResetThreads( void );
    long CountActiveThread( void );

    long HrBackupAbort( void );

    MT_RUN *pRun( void );
    long lInstance( void );
};

class MT_RUN {
public:
    static int              iDummyForSourceInsightParsing;
    JET_MT_INSTANCE**   pjmInstance;
    long                    lNumInstances;
    long                    lInstanceOffset;
    
    long                    lThreadCounter;
    HANDLE              hThreadsComplete;
    HANDLE              hProcessKeeping;
    HANDLE              hACKSemaphore;
    unsigned long       fEseBackSvcFlags;

    MT_RUN( long            lInstanceCount,
                const char* szCommand,              
                HANDLE      hRecoverySignal = NULL,
                HANDLE      hACK = NULL,
                unsigned long fInitialEseBackSvcFlags = 0 );

    ~MT_RUN( void );

    void AssignInstance(    const char*         szName,
                            const char*         szSystem,
                            const char*         szLog,
                            long                    lNumDB,
                            long                    lNumSysParam = 0,
                            JET_MT_SYSPARAM**   pjmSysParam = NULL );
    void AssignDatabase(    int             nInstance,
                            const char*     szName,
                            JET_MT_TABLE*   jmCurrTable,
                            int             nNumThreads,
                            long                lNumRec,
                            BOOL            bAttach = FALSE );
    void AssignThread(  int         nInstance,
                            int         nDatabase,
                            CTimeSpan   tTime,
                            LPTHREAD    lptExecute,
                            long            lCommit = 1,
                            long            lBegin = 0 );
    void AssignThread(  int         nInstance,
                            int         nDatabase,
                            long            lOperations,
                            LPTHREAD    lptExecute,
                            long            lCommit = 1,
                            long            lBegin = 0 );
    void AssignThread(  int                 nInstance,
                            int                 nDatabase,
                            const JET_MT_THREAD&    jmThread );

    void Run( void );
    void WaitForFinish( void );
    void RunAndWaitForFinish( void );
    void Cleanup( int nReason );
    void ResetThreads( void );
    long CountActiveThread( void );
    long ChangeEseBackSvcs( unsigned long fNewServiceFlags );
    MT_RUN *pRun( void );

protected:
    void CheckInstance( int nInstance );
    void CheckInstanceAndDatabase(  int nInstance,
                                            int nDatabase );
};

inline MT_RUN *MT_RUN::pRun( void ) { return this; }

inline MT_RUN *JET_MT_INSTANCE::pRun( void ) { return pjmParentRun; }
inline long JET_MT_INSTANCE::lInstance( void ) { return lInstanceNum; }

inline JET_MT_TABLE *JET_MT_DB::pTable( void ) { return pjmTable; }
inline JET_MT_INSTANCE *JET_MT_DB::pInstance( void ) { return pjmParentInstance; }
inline MT_RUN *JET_MT_DB::pRun( void ) { return pInstance( )->pjmParentRun; }
inline long JET_MT_DB::lDatabase( void ) { return lDatabaseNum; }
inline long JET_MT_DB::lInstance( void ) { return pInstance( )->lInstanceNum; }

inline JET_MT_DB *JET_MT_THREAD::pDatabase( void ) { return pjmParentDatabase; }
inline JET_MT_TABLE *JET_MT_THREAD::pTable( void ) { return pDatabase( )->pTable( ); }
inline JET_MT_INSTANCE *JET_MT_THREAD::pInstance( void ) { return pDatabase( )->pjmParentInstance; }
inline MT_RUN *JET_MT_THREAD::pRun( void ) { return pInstance( )->pjmParentRun; }
inline long JET_MT_THREAD::lThread( void ) { return lThreadNum; }
inline long JET_MT_THREAD::lDatabase( void ) { return pDatabase( )->lDatabaseNum; }
inline long JET_MT_THREAD::lInstance( void ) { return pInstance( )->lInstanceNum; }

class JMThread {
public:
    ~JMThread( void );
    JMThread( JET_MT_DB* pjMtDb );

    void Start( void );
    static unsigned __stdcall ThreadProc( void* pv );
    virtual unsigned Run( void );

    virtual JET_ERR Init( void );
    virtual BOOL Condition( void );
    virtual JET_ERR Step( void );
    virtual JET_ERR Body( void );
    virtual JET_ERR Uninit( void );
    virtual unsigned Complete( void );

    JET_MT_DB *pDatabase( void );
    JET_MT_TABLE *pTable( void );
    JET_MT_INSTANCE *pInstance( void );
    MT_RUN *pRun( void );
    long lThread( void );
    long lDatabase( void );
    long lInstance( void );

protected:
    JET_MT_DB*  m_pjMtDb;
    long            m_lOrdinal;

    HANDLE      m_hThread;
    unsigned        m_uTID;

    CTimeSpan   m_tsExecutionExp;
    CTime       m_tStopExp;
    long            m_lOperationsExp;

    long            m_lTransactionSize;
    long            m_lDbOffset;
    BOOL        m_fLazyCommit;
    BOOL        m_fUpdateDBSize;

    CTime       m_tStart;
    CTime       m_tEnd;
    CTimeSpan   m_tsExecution;
    long            m_lSuccess;
    long            m_lFailure;

    JET_ERR             m_jErr;
    JET_SESID       m_jSesId;
    JET_DBID        m_jDbId;
    JET_TABLEID         m_jTabId;
    JET_SETCOLUMN*  m_pjSetColumn;
};

inline JET_MT_DB *JMThread::pDatabase( void ) { return m_pjMtDb; }
inline JET_MT_TABLE *JMThread::pTable( void ) { return pDatabase( )->pTable( ); }
inline JET_MT_INSTANCE* JMThread::pInstance( void ) { return pDatabase( )->pjmParentInstance; }
inline MT_RUN* JMThread::pRun( void ) { return pInstance( )->pjmParentRun; }
inline long JMThread::lThread( void ) { return m_lOrdinal; }
inline long JMThread::lDatabase( void ) { return pDatabase( )->lDatabaseNum; }
inline long JMThread::lInstance( void ) { return pInstance( )->lInstanceNum; }

class JMRandReplace : public JMThread {
public:
    ~JMRandReplace( void );
    JMRandReplace(
            JET_MT_DB* pMtDb,
            CTimeSpan tsExecutionExp,
            long lTransactionSize = 1,
            long lDbOffset = 0,
            BOOL fLazyCommit = FALSE );

    virtual JET_ERR Init( void );
    virtual JET_ERR Body( void );
    virtual JET_ERR Uninit( void );

protected:
    long m_lThreadOffset;
    long m_lThreadLength;
};

