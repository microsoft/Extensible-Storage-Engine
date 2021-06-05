// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

// We want to avoid debug crt headers to be included, so that we don't create dependencies on
// dbg crts.
#ifndef _AFX_NO_DEBUG_CRT
#define _AFX_NO_DEBUG_CRT
#include <afx.h>
#undef _AFX_NO_DEBUG_CRT
#else
#include <afx.h>
#endif

#include "ese_common.hxx"

// Disable Windows threadpool threads
#define _MT_NONTHREADPOOL_

/*******************************************************************
 Globals
*******************************************************************/
class JET_MT_SYSPARAM;
class JET_MT_THREAD;
class JET_MT_TABLE;
class JET_MT_DB;
class JET_MT_INSTANCE;
class MT_RUN;

class JMThread;
class JMRandReplace;

extern int gnDebugLevel;            // Set the level of debug output 0 (none) to 2 (most)
extern BOOL gfRegisterForBackup;    // Should we register a backup callback DLL
extern BOOL gfRegisterForSurrogateBackup;   // Should we register a surrogate backup 

/*******************************************************************
 Backup/Restore DLL function pointers
 *******************************************************************/
#if !defined (JET_noBackup)

// Defines from ESEBack2.H
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
#endif // MIDL_PASS

//Defines from MVCallB.H
#define BACKUP_ANNOTATION               L"MULTITEST"
#define SURROGATE_BACKUP_ANNOTATION         L"VSSACTIVE"
//#define BACKUP_ANNOTATION             L"0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"
#define BACKUP_ANNOTATION_DISPLAY   L"JET stress simulator"
#define CALLBACK_DLL                    L"BackrestCallback.dll"

#ifndef DATABASE_MOUNTED
#define DATABASE_MOUNTED                0x00000010
#endif
#ifndef DATABASE_UNMOUNTED
#define DATABASE_UNMOUNTED          0x00000000
#endif

// Exports from ESEBACK2.DLL
typedef long (CALLBACK* CallHrESEBackupRestoreRegister)(WCHAR*, unsigned long, WCHAR*, WCHAR*, GUID*);
typedef long (CALLBACK* CallHrESEBackupRestoreUnregister)(void);
typedef long (CALLBACK* CallHrESERecoverAfterRestore)(WCHAR*, WCHAR*, WCHAR*, WCHAR*);
typedef long (CALLBACK* CallHrESEBackupInstanceAbort)(JET_INSTANCE, unsigned long);

#endif // BACKUP

// Casting simplifier for ues with _beginthreadex
typedef unsigned ( __stdcall *PTHREAD_START )( void * );

// LPTHREAD defined diferently for non-thread pool implementation
#ifdef _MT_NONTHREADPOOL_
    #define LPTHREAD PTHREAD_START
#else
    #define LPTHREAD LPTHREAD_START_ROUTINE
#endif

//================================================================
// Utility functions
//================================================================
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

//================================================================
class JET_MT_SYSPARAM
{
public:
    static int              iDummyForSourceInsightParsing;  // SI 3.5 doesn't recognize the first member of a class.
    unsigned long   ulParamID;
    ULONG_PTR   plParam;
    char            szParam[ MAX_PATH + 1 ];

    JET_MT_SYSPARAM( unsigned long ulSetParam,
                                ULONG_PTR plLongParam,
                                const char* szStringParam );
    void Apply( JET_MT_INSTANCE* pjmOwner,
                JET_SESID jCurrentSession = NULL );
};

//================================================================
class JET_MT_THREAD
{
public:
    static int              iDummyForSourceInsightParsing;  // SI 3.5 doesn't recognize the first member of a class.
    JET_MT_DB*  pjmParentDatabase;      // A pointer to the databse the thread accesses
    long            lThreadNum;             // What number the thread is on a given DB

    LPTHREAD    lptExecutionFunction;   // What function the thread should execute
    HANDLE      hThread;                // A handle to the kernel thread object
    unsigned        uThreadID;              // The kernel ID of the thread

    CTime       tStart;                 // The time when the thread started executing
    CTime       tEnd;                   // The time when the thread finished executing
    long            lExecutionTime;             // The time the thread executed for (in sec)

    CTimeSpan   tTimeToExecute;         // How long the thread runs for
    CTime       tTimeToStop;            // the time when thread should stop
    long            lExecuteOperations;         // Number of operations to execute (if not time based)

    long            lBeginOffset;           // The offset into the database where operations begin
    long            lNumToCommit;           // Number of transactions per commit
    BOOL        bLazyCommit;            // Should the thread perform lazy commits?
    BOOL        bUpdateDBSize;          // Set lRecordsPerTable to the current DB size before running

    long            lNumSuccess;            // Number of successful executions of the thread
    long            lNumFailure;            // Number of failed executions of the thread
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

//================================================================
class JET_MT_TABLE
{
public:
    static int          iDummyForSourceInsightParsing;  // SI 3.5 doesn't recognize the first member of a class.
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

//================================================================
class JET_MT_DB
{
public:
    static int              iDummyForSourceInsightParsing;  // SI 3.5 doesn't recognize the first member of a class.
    JET_MT_INSTANCE     *pjmParentInstance;     // A pointer to the parent instance of the DB
    JET_MT_TABLE        *pjmTable;              // The table definition for the DB
    JET_MT_THREAD       **pjmThread;            // Pointer to a list of threads accessing the DB

    JET_DBID            jDatabaseID;            // The JET ID of the database
    BOOL                bAttached;              // Is this DB attached?
    char                    szDBName[ MAX_PATH ];   // The location and filename of the DB
    char                    szSLVName[ MAX_PATH ];  // The location and filename of the STM file
    char                    szSLVRoot[ 32 ];            // The location and filename of the STM file
    long                    lNumThreads;            // How many threads access the DB
    long                    lThreadOffset;          // How many threads do we have initialised
    long                    lDatabaseNum;           // Offset of current DB in global DB array
    long                    lRecordsPerTable;       // Max records in the table
    long                    lDBCurrPos;             // DB 'cursor' - used to all sequential access to the
    long                    lTotalExecutes;         // The number of executions made by ALL threads
                                                //  DB by numerous threads

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

//================================================================
class JET_MT_INSTANCE
{
public:
    static int              iDummyForSourceInsightParsing;  // SI 3.5 doesn't recognize the first member of a class.
    MT_RUN              *pjmParentRun;              // Pointer to the parent Run of the instance
    JET_MT_DB           **pjmDatabase;              // Pointer to a list of databases controlled
    JET_MT_SYSPARAM **pjmSysParam;              // Pointer to the system parameter list

    JET_INSTANCE        jJETInstance;               // The JET_INSTANCE handle
    JET_SESID           jSessionID;                 // The JET session used to create databases,
    char                    szInstanceName[ MAX_PATH ];     // The string name of the instance
    char                    szSystemPath[ MAX_PATH ];   // Path where checkpoint file, etc. resides
    char                    szLogPath[ MAX_PATH ];      // Path where the log files reside etc. under the instance
    long                    lNumDatabases;              // How many databases the Instance has
    long                    lDBOffset;                  // Offset into the instances DB array by the instance
    long                    lInstanceNum;               // Offset of current Instance in global array

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

//================================================================
class MT_RUN {
public:
    static int              iDummyForSourceInsightParsing;  // SI 3.5 doesn't recognize the first member of a class.
    JET_MT_INSTANCE**   pjmInstance;        // Global array of all the JET instances the run creates
    long                    lNumInstances;      // TOTAL number of instances
    long                    lInstanceOffset;        // Current offset into gjmInstance array
    /*******************************************************************
     Globals used to record how many threads are running and signal
     an event when they all complete.
      - glThreadCounter is incremented by JET_MT_THREAD::Run()
      - worker functions must call JET_MT_THREAD::Complete() when
        finished executing to decrement glThreadCounter
      - Main function should wait on ghThreadsComplete which is signalled
        when JET_MT_THREAD::Complete() decrements glThreadCounter to 0
     *******************************************************************/
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

//================================================================
// inline getters
//================================================================
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

//================================================================
// new style JT_MT_THREAD
//================================================================
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

    // getters
    JET_MT_DB *pDatabase( void );
    JET_MT_TABLE *pTable( void );
    JET_MT_INSTANCE *pInstance( void );
    MT_RUN *pRun( void );
    long lThread( void );
    long lDatabase( void );
    long lInstance( void );

protected:
    // object control
    JET_MT_DB*  m_pjMtDb;               // A pointer to the databse the thread accesses
    long            m_lOrdinal;                 // What number the thread is on a given DB

    // Win32 thread control
    HANDLE      m_hThread;              // A handle to the kernel thread object
    unsigned        m_uTID;                 // The kernel ID of the thread

    // stop control
    CTimeSpan   m_tsExecutionExp;       // How long the thread runs for
    CTime       m_tStopExp;             // the time when thread should stop
    long            m_lOperationsExp;       // Number of operations to execute (if not time based)

    // DB access control
    long            m_lTransactionSize;     // Number of transactions per commit
    long            m_lDbOffset;            // The offset into the database where operations begin
    BOOL        m_fLazyCommit;          // Should the thread perform lazy commits?
    BOOL        m_fUpdateDBSize;        // Set lRecordsPerTable to the current DB size before running

    // statistics
    CTime       m_tStart;               // The time when the thread started executing
    CTime       m_tEnd;                 // The time when the thread finished executing
    CTimeSpan   m_tsExecution;          // The time the thread executed for (in sec)
    long            m_lSuccess;             // Number of successful executions of the thread
    long            m_lFailure;                 // Number of failed executions of the thread

    // ESE objects
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

//================================================================
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
    long m_lThreadOffset;   // record offset for this thread
    long m_lThreadLength;   // record share for this thread
};

