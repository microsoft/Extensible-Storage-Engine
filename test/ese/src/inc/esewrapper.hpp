#pragma once


//================================================
#include <windows.h>


//================================================
// STL headers won't compile with warning level 4
#pragma warning( push, 3 )
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#pragma warning( pop )

// Some other STL headers even won't compile with warning level 3
#pragma warning( push, 2 )
#include <sstream>
#pragma warning( pop )


//================================================
#include "ese_common.hxx"


//================================================
#if 0x6000 == eseVersion
// 0x6000 - ESE98 (Exchange 2000)
#define JET_API_PTR ULONG_PTR

#elif 0x5600 == eseVersion
// 0x5600 - ESENT98 (Whistler)

#else
#error  "ESEWRAPPER class library only support ESE version 0x5600 or 0x6000"
#endif


//================================================
#define OPFUN       ()
#define REF( x )        ( void )x

//================================================
enum {
    CP_NIL = 0,
    CP_ANSI = 1252,
    CP_UNICODE = 1200
};


//================================================
// Utility functions
//================================================
size_t strlen2( const char* szz );


//================================================
class EWInstance;
class EWSession;
class EWDatabase;
class EWTable;
class EWTempTable;
class EWColumn;
class EWColumnGroup;


//================================================
//typedef std::vector< EWSession >      VEC_EWSES;
//typedef std::vector< EWDatabase >     VEC_EWDB;
//typedef std::vector< EWTempTable >    VEC_EWTTAB;
//typedef std::vector< EWTable >            VEC_EWTAB;
typedef std::vector< EWColumn >     VEC_EWCOL;

//typedef std::map< std::string, EWColumn > MAP_STREWCOL;
//typedef std::pair< std::string, EWColumn >    PAIR_STREWCOL;

typedef void        ESENIL;
typedef bool        ESEBIT;

typedef UINT8   ESEUBYTE;
typedef INT16   ESESHORT;
typedef INT32   ESELONG;
typedef INT64   ESECURRENCY;

typedef float       ESESINGLE;
typedef double  ESEDOUBLE;

//typedef double        ESE_DATETIME;
//typedef std::string   ESE_BINARY
//typedef std::string   ESE_TEXT
//typedef std::string   ESE_LONGBINARY
//typedef std::string   ESE_LONGTEXT


//================================================
#if 0
class Data {
public:
    ~Data( void );
    Data( unsigned long cbCapacity = 0, void* pvData = NULL, unsigned long cbData = 0 );
    Data( std::string str );
    Data( const Data& data );
    Data& operator=( const Data& data );

    //================================
    unsigned long cap( void ) const { return m_cbCapacity; }
    void* pv( void ) const  { return m_pvData; }
    unsigned long cb( void ) const  { return m_cbData; }

    unsigned long setCb( unsigned long cb ) { return m_cbData = cb; }

    //================================
    operator std::string( void )    { return std::string( ( const char* )pv(), cb() ); }

protected:
    unsigned long   m_cbCapacity;
    void*   m_pvData;
    unsigned long   m_cbData;
};
#endif


//================================================
class EW {
public:
    static JET_ERR EnableMultiInstance(
        JET_SETSYSPARAM* pjSetSysP = NULL,
        unsigned long   cjSetSysP = 0,
        unsigned long* pcjSetSysP = NULL )
    {
        return JetEnableMultiInstance( pjSetSysP, cjSetSysP, pcjSetSysP );
    }

    static JET_ERR StopService( void )
    {
        return JetStopService();
    }
    static JET_ERR StopBackup( void )
    {
        return JetStopBackup();
    }

    static JET_ERR GetInstanceInfo(
        unsigned long* pcInstInfo,
        JET_INSTANCE_INFO** paInstInfo )
    {
        return JetGetInstanceInfo( pcInstInfo, paInstInfo );
    }

    static JET_ERR FreeBuffer( char* pbBuf )
    {
        return JetFreeBuffer( pbBuf );
    }

    static JET_ERR SetSystemParameter(
        unsigned long ulParamId,
        JET_API_PTR pvParam,
        const char* szParam )
    {
        JET_INSTANCE jInstId = JET_instanceNil;
        return JetSetSystemParameter( &jInstId, JET_sesidNil, ulParamId, pvParam, szParam );
    }
    static JET_ERR GetSystemParameter(
        unsigned long   ulParamId,
        JET_API_PTR* ppvParam,
        char * szParam,
        unsigned long   cbMax )
    {
        return JetGetSystemParameter( JET_instanceNil, JET_sesidNil, ulParamId, ppvParam, szParam, cbMax );
    }

    static JET_ERR GetDatabaseFileInfo(
        const char* szDb,
        void* pvResult,
        unsigned long   cbMax,
        unsigned long   ulInfoLevel )
    {
        return JetGetDatabaseFileInfo( szDb, pvResult, cbMax, ulInfoLevel );
    }

    static JET_ERR BeginExternalBackup(
        JET_GRBIT jGrBit = JET_bitNil )
    {
        return JetBeginExternalBackup( jGrBit );
    }
    static JET_ERR OpenFile(
        const char* szFileName,
        JET_HANDLE* phfFile,
        unsigned long* pulFileSizeLow,
        unsigned long* pulFileSizeHigh )
    {
        return JetOpenFile( szFileName, phfFile, pulFileSizeLow, pulFileSizeHigh);
    }
    static JET_ERR ReadFile(
        JET_HANDLE hfFile,
        void* pv,
        unsigned long cb,
        unsigned long* pcb )
    {
        return JetReadFile( hfFile, pv, cb, pcb);
    }
};


//================================================
class EWInstance {
//friend class EWSession;
public:
    ~EWInstance( void ) { }
    EWInstance( void )
    {
        nullify();
    }
    EWInstance(
        const EWInstance& rewInst )
    {
        nullify();
        *this = rewInst;
    }

    // TODO: obsolete!
    EWInstance(
        JET_GRBIT jGrBit,
        JET_RSTINFO* pjRSTInfo = NULL,
        const char* szInst = NULL,
        const char* szDisp = NULL
    );

    //EWInstance( JSystemParameterVector jSpv );

    EWInstance& operator=(
        const EWInstance& rewInst )
    {
        m_jErr = rewInst.jErr();
        m_jInstId = rewInst.jInstId();
        //m_vewSes = rewInst.vewSes();

        return *this;
    }

    //================================
    EWSession BeginSession(
        const char* szUser = NULL,
        const char* szPass = NULL
    );

    //================================
    JET_ERR jErr( void ) const { return m_jErr; }
    JET_INSTANCE jInstId( void ) const { return m_jInstId; }
    //VEC_EWSES vewSes( void ) const { return m_vewSes; }

    //================================
    bool operator==( const EWInstance& rewInst )
    {
        return jInstId() == rewInst.jInstId();
    }

    JET_ERR CreateInstance(
        const char* szInst = NULL,
        const char* szDisp = NULL,
        JET_GRBIT jGrBit = JET_bitNil )
    {
        return m_jErr = JetCreateInstance2( &m_jInstId, szInst, szDisp, jGrBit );
    }
    JET_ERR Init(
        JET_GRBIT jGrBit = JET_bitNil,
        JET_RSTMAP* pjRSTMap = NULL,
        long cRSTMap = 0 )
    {
        JET_RSTINFO rstinfo = { 0 };
        rstinfo.cbStruct = sizeof( rstinfo );
        rstinfo.rgrstmap = pjRSTMap;
        rstinfo.crstmap = cRSTMap;
        return m_jErr = JetInit3( &m_jInstId, &rstinfo, jGrBit );
    }
    JET_ERR Init3(
        JET_GRBIT jGrBit,
        JET_RSTINFO* pjRSTInfo
        )
    {
        return m_jErr = JetInit3( &m_jInstId, pjRSTInfo, jGrBit );
    }
    JET_ERR Term(
        JET_GRBIT jGrBit = JET_bitNil )
    {
        m_jErr = JetTerm2( jInstId(), jGrBit );
        if ( JET_errSuccess <= jErr() ) {
            m_jInstId = JET_instanceNil;
        }

        return jErr();
    }

    JET_ERR StopServiceInstance( void )
    {
        return m_jErr = JetStopServiceInstance( jInstId() );
    }
    JET_ERR StopBackupInstance( void )
    {
        return m_jErr = JetStopBackupInstance( jInstId() );
    }

    JET_ERR SetSystemParameter(
        unsigned long ulParamId,
        JET_API_PTR pvParam,
        const char* szParam )
    {
        return m_jErr = JetSetSystemParameter( &m_jInstId, JET_sesidNil, ulParamId, pvParam, szParam );
    }
    JET_ERR GetSystemParameter(
        unsigned long   ulParamId,
        JET_API_PTR* ppvParam,
        char * szParam,
        unsigned long   cbMax )
    {
        return m_jErr = JetGetSystemParameter( m_jInstId, JET_sesidNil, ulParamId, ppvParam, szParam, cbMax );
    }
    JET_ERR GetAttachInfoInstance(
        void* pv,
        unsigned long cbMax,
        unsigned long* pcbActual )
    {
#if ( JET_VERSION < 0x0600 )
        return m_jErr = JetGetAttachInfoInstance( jInstId(), pv, cbMax, pcbActual );
#else

        return m_jErr = JetGetAttachInfoInstance( jInstId(), ( char* )pv, cbMax, pcbActual );
#endif
    }

    JET_ERR BeginExternalBackupInstance(
        JET_GRBIT jGrBit = JET_bitNil )
    {
        return m_jErr = JetBeginExternalBackupInstance( jInstId(), jGrBit );
    }

    JET_ERR OpenFileInstance(
        const char* szFileName,
        JET_HANDLE* phfFile,
        unsigned long* pulFileSizeLow,
        unsigned long* pulFileSizeHigh )
    {
        return m_jErr = JetOpenFileInstance( jInstId(), szFileName, phfFile, pulFileSizeLow, pulFileSizeHigh);
    }

    JET_ERR ReadFileInstance(
        JET_HANDLE hfFile,
        void* pv,
        unsigned long cb,
        unsigned long* pcb )
    {
        return m_jErr = JetReadFileInstance( jInstId(), hfFile, pv, cb, pcb);
    }

protected:
    void nullify( void )
    {
        m_jErr = JET_errTestError;
        m_jInstId = JET_instanceNil;
    }

    //void Register( EWSession& rewSes );
    //void Unregister( EWSession& rewSes );

    JET_ERR m_jErr;
    JET_INSTANCE m_jInstId;
    //VEC_EWSES     m_vewSes;       // EWSessions we hold
};


//================================================
class EWSession {
friend class EWInstance;
public:
    ~EWSession( void ) { }
    EWSession( void )
    {
        nullify();
    }
    EWSession(
        const EWSession& rewSes )
    {
        nullify();
        *this = rewSes;
    }
    EWSession& operator=(
        const EWSession& rewSes )
    {
        m_jErr = rewSes.m_jErr;
        m_pewInst = rewSes.m_pewInst;
        m_jSesId = rewSes.m_jSesId;

        return *this;
    }

    //================================
    EWSession DupSession( void )
    {
        return EWSession( m_pewInst, jSesId() );
    }

    EWDatabase CreateDatabase(
        const char* szDb,
        JET_GRBIT jGrBit = JET_bitNil,
        unsigned long cpgDbMax = 0
    );
    EWDatabase OpenDatabase(
        const char* szDb,
        const char* szConn = NULL,
        JET_GRBIT jGrBit = JET_bitNil
    );
    EWTempTable OpenTempTable(
        JET_COLUMNDEF* pjColDef,
        unsigned long   cCol,
        JET_GRBIT jGrBit = JET_bitNil,
        JET_UNICODEINDEX* pjUCIndex = NULL
    );
    EWTempTable OpenTempTable2(
        JET_COLUMNDEF* pjColDef,
        unsigned long   cCol,
        unsigned long   lcid,
        JET_GRBIT jGrBit = JET_bitNil
    );

    //================================
    EWInstance* pEWInstance( void ) const { return m_pewInst; }

    JET_ERR jErr( void ) const { return m_jErr; }
    JET_SESID jSesId( void ) const { return m_jSesId; }
    JET_INSTANCE jInstId( void ) const { return m_pewInst->jInstId(); }

    //================================
    bool operator==(
        const EWSession& rjSes )
    {
        return jSesId() == rjSes.jSesId()
            && jInstId() == rjSes.jInstId();
    }

    JET_ERR AttachDatabase(
        const char* szDb,
        JET_GRBIT jGrBit = JET_bitNil,
        unsigned long   cpgDbMax = 0 )
    {
        return m_jErr = JetAttachDatabase2( jSesId(), szDb, cpgDbMax, jGrBit );
    }
    //========================
    JET_ERR BeginTransaction( void )
    {
        return m_jErr = JetBeginTransaction( jSesId() );
    }
    //========================
    JET_ERR CommitTransaction(
        JET_GRBIT jGrBit = JET_bitNil )
    {
        return m_jErr = JetCommitTransaction( jSesId(), jGrBit );
    }
    //--------------------------------
    JET_ERR Defragment(
        const char* szDatabase,
        const char* szTable,
        unsigned long* pcPasses,
        unsigned long* pcSeconds,
        JET_CALLBACK pfnCallBack = NULL,
        void* pvContext = NULL,
        JET_GRBIT jGrBit = JET_bitDefragmentBatchStart )
    {
        return m_jErr = JetDefragment3( jSesId(), szDatabase, szTable,
                        pcPasses, pcSeconds, pfnCallBack, pvContext, jGrBit );
    }

    JET_ERR DetachDatabase(
        const char* szDb,
        JET_GRBIT jGrBit = JET_bitNil )
    {
        return m_jErr = JetDetachDatabase2( jSesId(), szDb, jGrBit );
    }

    JET_ERR EndSession(
        JET_GRBIT jGrBit = JET_bitNil )
    {
        m_jErr = JetEndSession( jSesId(), jGrBit );
        if ( JET_errSuccess <= jErr() ) {
            m_jSesId = JET_sesidNil;
        }

        return jErr();
    }
    JET_ERR Idle(
        JET_GRBIT jGrBit = JET_bitIdleFlushBuffers )
        {
            return m_jErr = JetIdle( jSesId(), jGrBit );
    }
    JET_ERR SetDatabaseSize(
        const char* szDb,
        unsigned long cpg,
        unsigned long* pcpgReal )
    {
        return m_jErr = JetSetDatabaseSize( jSesId(), szDb, cpg, pcpgReal );
    }

    JET_ERR UpgradeDatabase(
        const char* szDb,
        const char* szSLV,
        JET_GRBIT jGrBit = JET_bitNil )
    {
        return m_jErr = JetUpgradeDatabase( jSesId(), szDb, szSLV, jGrBit );
    }

    JET_ERR Rollback(
        JET_GRBIT jGrBit = JET_bitNil )
    {
        return m_jErr = JetRollback( jSesId(), jGrBit );
    }

    JET_ERR SetSessionContext(
        JET_API_PTR japCtx )
    {
        return m_jErr = JetSetSessionContext( jSesId(), japCtx );
    }
    JET_ERR ResetSessionContext( void )
    {
        return m_jErr = JetResetSessionContext( jSesId() );
    }

protected:
    // called by EWInstance::BeginSession
    EWSession(
        EWInstance* pjInst,
        const char* szUser = NULL,
        const char* szPass = NULL )
    {
        nullify();
        m_pewInst = pjInst;
        m_jErr = JetBeginSession( jInstId(), &m_jSesId, szUser, szPass );
    }

    // called by EWSession::DupSession
    EWSession(
        EWInstance* pjInst,
        JET_SESID jSesId )
    {
        nullify();
        m_pewInst = pjInst;
        m_jErr = JetDupSession( jSesId, &m_jSesId );
    }

    void nullify( void )
    {
        m_jErr = JET_errTestError;
        m_pewInst = NULL;
        m_jSesId = JET_sesidNil;
    }

    JET_ERR m_jErr;
    EWInstance* m_pewInst;      // Pointer to EWInstance to which we belong
    JET_SESID m_jSesId;
};


//================================================
class EWDatabase {
friend class EWSession;
public:
    ~EWDatabase( void ) { }
    EWDatabase( void )
    {
        nullify();
    }
    EWDatabase(
        const EWDatabase& rewDb )
    {
        nullify();
        *this = rewDb;
    }
    EWDatabase& operator=(
        const EWDatabase& rewDb )
    {
        m_jErr = rewDb.m_jErr;
        m_pewSes = rewDb.m_pewSes;
        m_jDbId = rewDb.m_jDbId;
        m_strDb = rewDb.m_strDb;

        return *this;
    }

    //================================
    EWTable CreateTable(
        const char* szTab,
        unsigned long   ulPages     = 0,
        unsigned long   ulDensity = 100
    );
    EWTable OpenTable(
        const char* szTab,
        JET_GRBIT jGrBit = JET_bitNil
    );

    EWTable CreateTableColumnIndex(
        JET_TABLECREATE* pjTabCreate
    );
    EWTable CreateTableColumnIndex2(
        JET_TABLECREATE2* pjTabCreate2
    );

    //================================
    EWSession* pEWSession( void ) const { return m_pewSes; }
    EWInstance* pEWInstance( void ) const { return pEWSession()->pEWInstance(); }

    JET_ERR jErr( void ) const { return m_jErr; }
    JET_DBID jDbId( void ) const { return m_jDbId; }
    JET_SESID jSesId( void ) const { return m_pewSes->jSesId(); }
    JET_INSTANCE jInstId( void ) const { return m_pewSes->jInstId(); }

    //================================
    bool operator==(
        const EWDatabase& rewDb )
    {
        return jDbId() == rewDb.jDbId()
            && jInstId() == rewDb.jInstId();
    }

    JET_ERR CloseDatabase(
        JET_GRBIT jGrBit = JET_bitNil )
    {
        m_jErr = JetCloseDatabase( jSesId(), jDbId(), jGrBit );
        if ( JET_errSuccess <= jErr() ) {
            m_jDbId = JET_dbidNil;
        }

        return jErr();
    }

    JET_ERR GrowDatabase(
        unsigned long cpg,
        unsigned long* pcpgReal )
    {
        return m_jErr = JetGrowDatabase( jSesId(), jDbId(), cpg, pcpgReal );
    }
    JET_ERR GetDatabaseInfo(
        void* pvResult,
        unsigned long   cbMax,
        unsigned long   ulInfoLevel )
    {
        return m_jErr = JetGetDatabaseInfo( jSesId(), jDbId(),
                        pvResult, cbMax, ulInfoLevel );
    }

    JET_ERR DeleteTable(
        const char* szTab )
    {
        return m_jErr = JetDeleteTable( jSesId(), jDbId(), szTab );
    }
    JET_ERR RenameTable(
        const char* szOld,
        const char* szNew )
    {
        return m_jErr = JetRenameTable( jSesId(), jDbId(), szOld, szNew );
    }

    JET_ERR SetColumnDefaultValue(
        const char* szTable,
        const char* szColumn,
        void* pvData,
        unsigned long   cbData,
        JET_GRBIT jGrBit = JET_bitNil )
    {
        return m_jErr = JetSetColumnDefaultValue( jSesId(), jDbId(),
                        szTable, szColumn, pvData, cbData, jGrBit );
    }
    JET_ERR SetColumnDefaultValue(
        const std::string& strTable,
        std::string strColumn,
        std::string strData,
        JET_GRBIT jGrBit = JET_bitNil )
    {
        return SetColumnDefaultValue( strTable.c_str(), strColumn.c_str(),
                ( void* )strData.c_str(), ( unsigned long )strData.size(), jGrBit );
    }

    JET_ERR Defragment(
        const char* szTable,
        unsigned long* pcPasses,
        unsigned long* pcSeconds,
        JET_GRBIT jGrBit = JET_bitDefragmentBatchStart,
        JET_CALLBACK pfnCallBack = NULL )
    {
        return m_jErr = JetDefragment2( jSesId(), jDbId(), szTable,
                        pcPasses, pcSeconds, pfnCallBack, jGrBit );
    }

protected:
    // called by EWSession::CreateDatabase/CreateDatabase2
    EWDatabase(
        EWSession* pjSes,
        const char* szDb,
        JET_GRBIT jGrBit,
        unsigned long cpgDbMax )
    {
        nullify();
        m_pewSes = pjSes;
        m_strDb = szDb;
        m_jErr = JetCreateDatabase2( jSesId(), m_strDb.c_str(),
                    cpgDbMax, &m_jDbId, jGrBit );
    }

    // called by EWSession::OpenDatabase
    EWDatabase(
        EWSession* pjSes,
        const char* szDb,
        const char* szConn,
        JET_GRBIT jGrBit )
    {
        nullify();
        m_pewSes = pjSes;
        m_strDb = szDb;
        m_jErr = JetOpenDatabase( jSesId(), m_strDb.c_str(), szConn, &m_jDbId, jGrBit );
    }

    void nullify( void )
    {
        m_jErr = JET_errTestError;
        m_pewSes = NULL;
        m_jDbId = JET_dbidNil;
    }

    JET_ERR m_jErr;
    EWSession* m_pewSes;            // Pointer to EWSession to which we belong
    JET_DBID m_jDbId;
    std::string m_strDb;
};


//================================================
class EWTable {
friend class EWDatabase;
public:
    ~EWTable( void ) { }
    EWTable( void )
    {
        nullify();
    }
    EWTable(
        const EWTable& rewTab )
    {
        nullify();
        *this = rewTab;
    }
    EWTable& operator=(
        const EWTable& rewTab )
    {
        m_jErr = rewTab.m_jErr;
        m_pewDb = rewTab.m_pewDb;
        m_jTabId = rewTab.m_jTabId;
        m_strTab = rewTab.m_strTab;

        return *this;
    }

    //================================
    EWColumn operator[](
        const char* szCol
    );
    EWColumnGroup operator OPFUN(
        const char* szzCols
    );

    //================================
    EWDatabase* pEWDatabase( void ) const { return m_pewDb; }
    virtual EWSession* pEWSession( void ) const { return pEWDatabase()->pEWSession(); }
    virtual EWInstance* pEWInstance( void ) const { return pEWDatabase()->pEWInstance(); }

    JET_ERR jErr( void ) const { return m_jErr; }
    JET_TABLEID jTabId( void ) const { return m_jTabId; }
    JET_DBID jDbId( void ) const { return m_pewDb->jDbId(); }
    virtual JET_SESID jSesId( void ) const { return m_pewDb->jSesId(); }
    virtual JET_INSTANCE jInstId( void ) const { return m_pewDb->jInstId(); }

    //================================
    bool operator==(
        const EWTable& rewTab )
    {
        return jTabId() == rewTab.jTabId()
            && jInstId() == rewTab.jInstId();
    }

    //================
    JET_ERR AddColumn(
        const char* szCol,
        JET_COLTYP jColTyp,
        JET_GRBIT jGrBit = JET_bitNil,
        unsigned long cbMax = 0,
        unsigned short usCP = CP_NIL
    );
    //================
    JET_ERR AddColumn(
        const char* szCol,
        const JET_COLUMNDEF* pjColDef,
        const void* pvDefault = NULL,
        unsigned long   cbDefault = 0,
        JET_COLUMNID* pjColId = NULL )
    {
        JET_COLUMNID jColIdDefault = JET_colidNil;

        JET_COLUMNID& jColId = pjColId ? *pjColId : jColIdDefault;
        return m_jErr = JetAddColumn( jSesId(), jTabId(), szCol, pjColDef, pvDefault, cbDefault, &jColId );
    }
    //================
    JET_ERR DeleteColumn(
        const char* szCol,
        JET_GRBIT jGrBit = JET_bitNil )
    {
        return m_jErr = JetDeleteColumn2( jSesId(), jTabId(), szCol, jGrBit );
    }
    //================
    JET_ERR CloseTable( void )
    {
        m_jErr = JetCloseTable( jSesId(), jTabId() );
        if ( JET_errSuccess <= jErr() ) {
            m_jTabId = JET_tableidNil;
        }

        return jErr();
    }
    //================
    JET_ERR CreateIndex(
        const char* szIndex,
        const char* szKey,
        JET_GRBIT jGrBit = JET_bitNil,
        unsigned long lDensity = 100,
        unsigned long cbKey = 0 )
    {
        if ( 0 == cbKey ) {
            cbKey = ( unsigned long )( strlen2( szKey ) + 1 );
        }
        return m_jErr = JetCreateIndex( jSesId(), jTabId(),
                        szIndex, jGrBit, szKey, cbKey, lDensity );
    }
    //================
    JET_ERR CreateIndex(
        std::string strIndex,
        std::string strKey,
        JET_GRBIT jGrBit = JET_bitNil,
        unsigned long   lDensity = 100 )
    {
        return CreateIndex( strIndex.c_str(), strKey.c_str(),
                jGrBit, lDensity, ( unsigned long )strKey.size() );
    }
    //================
    JET_ERR CreateIndex2(
        JET_INDEXCREATE* pjIdxCreate,
        unsigned long   cjIdxCreate )
    {
        return m_jErr = JetCreateIndex2( jSesId(), jTabId(), pjIdxCreate, cjIdxCreate );
    }
    //================
    JET_ERR Delete( void )
    {
        return m_jErr = JetDelete( jSesId(), jTabId() );
    }
    //================
    JET_ERR DeleteIndex(
        const char* szIndex )
    {
        return m_jErr = JetDeleteIndex( jSesId(), jTabId(), szIndex );
    }
    //================
    JET_ERR GetLock(
        JET_GRBIT jGrBit )
    {
        return m_jErr = JetGetLock( jSesId(), jTabId(), jGrBit );
    }
    //================
    JET_ERR GetTableInfo(
        void* pvResult,
        unsigned long cbMax,
        unsigned long ulInfoLevel )
    {
        return m_jErr = JetGetTableInfo( jSesId(), jTabId(),
                        pvResult, cbMax, ulInfoLevel );
    }
    //================
    JET_ERR GetTableColumnInfo(
        const char* szCol,
        void* pvResult,
        unsigned long cbMax,
        unsigned long ulInfoLevel )
    {
        return m_jErr = JetGetTableColumnInfo( jSesId(), jTabId(),
                        szCol, pvResult, cbMax, ulInfoLevel );
    }
    //================
    JET_ERR GetTableIndexInfo(
        const char* szIdx,
        void* pvResult,
        unsigned long cbMax,
        unsigned long ulInfoLevel )
    {
        return m_jErr = JetGetTableIndexInfo( jSesId(), jTabId(),
                        szIdx, pvResult, cbMax, ulInfoLevel );
    }
    //================
    JET_ERR IndexRecordCount(
        unsigned long* pcRec,
        unsigned long cRecMax = UINT_MAX )
    {
        return m_jErr = JetIndexRecordCount( jSesId(), jTabId(), pcRec, cRecMax );
    }
    //================
    JET_ERR MakeKey(
        void* pvData,
        unsigned long cbData,
        JET_GRBIT jGrBit = JET_bitNewKey )
    {
        return m_jErr = JetMakeKey( jSesId(), jTabId(), pvData, cbData, jGrBit );
    }
    //================
    JET_ERR MakeKey(
        const std::string& strKey,
        JET_GRBIT jGrBit = JET_bitNewKey )
    {
        return MakeKey( ( void* )strKey.c_str(), ( unsigned long )strKey.size(), jGrBit );
    }
    //================
    JET_ERR Move(
        long cRow = JET_MoveNext,
        JET_GRBIT jGrBit = JET_bitNil )
    {
        return m_jErr = JetMove( jSesId(), jTabId(), cRow, jGrBit );
    }
    //================
    JET_ERR PrepareUpdate(
        unsigned long ulPrep = JET_prepInsert )
    {
        return m_jErr = JetPrepareUpdate( jSesId(), jTabId(), ulPrep );
    }
    //================
    JET_ERR RenameColumn(
        const char* szOld,
        const char* szNew,
        JET_GRBIT jGrBit = JET_bitNil )
    {
        return m_jErr = JetRenameColumn( jSesId(), jTabId(), szOld, szNew, jGrBit );
    }
    //================
    JET_ERR ResetTableSequential(
        JET_GRBIT jGrBit = JET_bitNil )
    {
        return m_jErr = JetResetTableSequential( jSesId(), jTabId(), jGrBit );
    }
    //================
    JET_ERR RetrieveColumns(
        JET_RETRIEVECOLUMN* pjRetCol,
        unsigned long   cjRetCol )
    {
        return m_jErr = JetRetrieveColumns( jSesId(), jTabId(), pjRetCol, cjRetCol );
    }
    //================
    JET_ERR Seek(
        JET_GRBIT jGrBit = JET_bitSeekEQ )
    {
        return m_jErr = JetSeek( jSesId(), jTabId(), jGrBit );
    }
    //================
    JET_ERR SetColumns(
        JET_SETCOLUMN* pjSetCol,
        unsigned long   cjSetCol )
    {
        return m_jErr = JetSetColumns( jSesId(), jTabId(), pjSetCol, cjSetCol );
    }
    //================
    JET_ERR SetCurrentIndex(
        const char* szIndex,
        JET_GRBIT jGrBit = JET_bitMoveFirst,
        unsigned long ulTagSeq = 1,
        JET_INDEXID* pjIndId = NULL )
    {
        return m_jErr = JetSetCurrentIndex4( jSesId(), jTabId(),
                        szIndex, pjIndId, jGrBit, ulTagSeq );
    }
    //================
    JET_ERR SetTableSequential(
        JET_GRBIT jGrBit = JET_bitNil )
    {
        return m_jErr = JetSetTableSequential( jSesId(), jTabId(), jGrBit );
    }
    //================
    JET_ERR Update(
        void* pvBookmark = NULL,
        unsigned long cbBookmark = 0,
        unsigned long* pcbActual = NULL )
    {
        return m_jErr = JetUpdate( jSesId(), jTabId(),
                        pvBookmark, cbBookmark, pcbActual );
    }

protected:
    // called by EWDatabase::CreateTable
    EWTable(
        EWDatabase* pjDb,
        const char* szTab,
        unsigned long ulPages,
        unsigned long ulDensity )
    {
        nullify();
        m_pewDb = pjDb;
        m_strTab = szTab;
        m_jErr = JetCreateTable( jSesId(), jDbId(), m_strTab.c_str(), ulPages, ulDensity, &m_jTabId );
    }

    // called by EWDatabase::OpenTable
    EWTable(
        EWDatabase* pjDb,
        const char* szTab,
        JET_GRBIT jGrBit )
    {
        nullify();
        m_pewDb = pjDb;
        m_strTab = szTab;
        m_jErr = JetOpenTable( jSesId(), jDbId(), m_strTab.c_str(), NULL, 0, jGrBit, &m_jTabId );
    }

    // called by EWDatabase::CreateTableColumnIndex
    EWTable(
        EWDatabase* pjDb,
        JET_TABLECREATE2* pjTabCreate2 )
    {
        nullify();
        m_pewDb = pjDb;
        m_strTab    = pjTabCreate2->szTableName;
        m_jErr = JetCreateTableColumnIndex2( jSesId(), jDbId(), pjTabCreate2 );
        m_jTabId    = pjTabCreate2->tableid;
    }

    // called by EWDatabase::CreateTableColumnIndex2
    EWTable(
        EWDatabase* pjDb,
        JET_TABLECREATE* pjTabCreate )
    {
        nullify();
        m_pewDb = pjDb;
        m_strTab    = pjTabCreate->szTableName;
        m_jErr = JetCreateTableColumnIndex( jSesId(), jDbId(), pjTabCreate );
        m_jTabId    = pjTabCreate->tableid;
    }

    void nullify( void )
    {
        m_jErr = JET_errTestError;
        m_pewDb = NULL;
        m_jTabId = JET_tableidNil;
    }

    JET_ERR m_jErr;
    EWDatabase* m_pewDb;        // parent EWDatabase
    JET_TABLEID m_jTabId;
    std::string m_strTab;
};

//================================================
class EWTempTable : public EWTable {
friend class EWSession;
public:
    ~EWTempTable( void )
    {
        pdelete_v( m_pjColDef );
    }
    EWTempTable( void )
    {
        nullify();
    }
    EWTempTable(
        const EWTempTable& rewTTab )
    {
        nullify();
        *this = rewTTab;
    }
    EWTempTable& operator=(
        const EWTempTable& rewTTab )
    {
        m_jErr = rewTTab.m_jErr;
        m_pewDb = NULL;
        m_jTabId = rewTTab.m_jTabId;
        m_strTab = rewTTab.m_strTab;

        m_pewSes = rewTTab.m_pewSes;
        assign1( rewTTab.m_pjColDef, rewTTab.m_cCol );

        return *this;
    }

    //================================
    EWColumn operator[]( size_t cIndex );
    //EWColumnGroup operator OPFUN( const char* szzCols );

    //================================
    virtual EWSession* pEWSession( void ) const { return m_pewSes; }
    virtual EWInstance* pEWInstance( void ) const { return pEWSession()->pEWInstance(); }

    virtual JET_SESID jSesId( void ) const { return m_pewSes->jSesId(); }
    virtual JET_INSTANCE jInstId( void ) const { return m_pewSes->jInstId(); }

    //================================

protected:
    //called by EWSEssion::OpenTempTable
    EWTempTable(
        EWSession* pewSes,
        JET_COLUMNDEF* pjColDef,
        unsigned long cCol,
        JET_GRBIT jGrBit,
        JET_UNICODEINDEX* pjUCIndex
    );

    // called by EWSession::OpenTempTable2
    EWTempTable(
        EWSession* pewSes,
        JET_COLUMNDEF* pjColDef,
        unsigned long cCol,
        unsigned long lcid,
        JET_GRBIT jGrBit
    );

    void nullify( void )
    {
        m_strTab = "TempTable";
        m_pewSes = NULL;
        m_pjColDef = NULL;
        m_cCol = 0;
    }

    // init m_pjColDef with another JET_COLUMNDEF array
    void assign1(
        JET_COLUMNDEF* pjColDef,
        unsigned long cCol
    );
    // init m_pjColDef->columnid with another JET_COLUMNID array
    void assign2(
        JET_COLUMNID* pjColId,
        unsigned long cCol
    );

    EWSession* m_pewSes;            // Pointer to EWSession to which we belong
    JET_COLUMNDEF* m_pjColDef;
    unsigned long m_cCol;
};

//================================================
class EWColumn {
friend class EWTable;
friend class EWTempTable;

public:
    ~EWColumn( void )
    {
        pdelete_v( m_pbData );
    }
    EWColumn( void )
    {
        nullify();
    }
    EWColumn(
        const EWColumn& rewCol )
    {
        nullify();
        *this = rewCol;
    }
    EWColumn& operator=(
        const EWColumn& rewCol )
    {
        m_jErr = rewCol.m_jErr;
        m_pewTab = rewCol.m_pewTab;
        m_jColDef = rewCol.m_jColDef;
        m_strCol = rewCol.m_strCol;

        m_jSetGrBit = rewCol.m_jSetGrBit;
        m_jSetInfo = rewCol.m_jSetInfo;

        m_cbData = rewCol.m_cbData;
        m_cbActual = rewCol.m_cbActual;

        pdelete_v( m_pbData );
        if ( 0 < m_cbData ) {
            m_pbData = new char[ m_cbData ];
            memcpy( m_pbData, rewCol.m_pbData, m_cbData );
        }
        else {
            m_pbData = NULL;
        }

        return *this;
    }

    //================================
    EWTable* pEWTable( void ) const { return m_pewTab; }
    EWDatabase* pEWDatabase( void ) const { return pEWTable()->pEWDatabase(); }
    EWSession* pEWSession( void ) const { return pEWTable()->pEWSession(); }
    EWInstance* pEWInstance( void ) const { return pEWTable()->pEWInstance(); }

    JET_ERR jErr( void ) const { return m_jErr; }
    JET_TABLEID jTabId( void ) const { return m_pewTab->jTabId(); }
    JET_DBID jDbId( void ) const { return m_pewTab->jDbId(); }
    JET_SESID jSesId( void ) const { return m_pewTab->jSesId(); }
    JET_INSTANCE jInstId( void ) const { return m_pewTab->jInstId(); }

    JET_COLUMNDEF jColDef( void ) const { return m_jColDef; }
    JET_COLTYP jColTyp( void ) const { return m_jColDef.coltyp; }
    JET_GRBIT jGrBit( void ) const { return m_jColDef.grbit; }
    JET_COLUMNID jColId( void ) const { return m_jColDef.columnid; }

    //================================
    bool operator==(
        const EWColumn& rjCol )
    {
        return jColId() == rjCol.jColId()
            && jTabId() == rjCol.jTabId()
            && jInstId() == rjCol.jInstId();
    }
    //operator std::string( JET_GRBIT jGrBit );
    //operator Data( void );

    //JET_ERR operator=( const Data& data );
    //JET_ERR operator=( ESECURRENCY eseCurrency );
    JET_ERR operator=(
        ESEUBYTE eseUByte )
    {
        return SetColumn( &eseUByte, sizeof( eseUByte ), m_jSetGrBit, &m_jSetInfo );
    }
    JET_ERR operator=(
        ESESHORT eseShort )
    {
        return SetColumn( &eseShort, sizeof( eseShort ), m_jSetGrBit, &m_jSetInfo );
    }
    JET_ERR operator=(
        ESELONG eseLong )
    {
        return SetColumn( &eseLong, sizeof( eseLong ), m_jSetGrBit, &m_jSetInfo );
    }
    JET_ERR operator=(
        ESESINGLE eseSingle )
    {
        return SetColumn( &eseSingle, sizeof( eseSingle ), m_jSetGrBit, &m_jSetInfo );
    }
    JET_ERR operator=(
        ESEDOUBLE eseDouble )
    {
        return SetColumn( &eseDouble, sizeof( eseDouble ), m_jSetGrBit, &m_jSetInfo );
    }
    JET_ERR operator=(
        const std::string& str )
    {
        return SetColumn( ( void* )str.c_str(), ( unsigned long )str.size(), m_jSetGrBit, &m_jSetInfo );
    }
    JET_ERR operator+=(
        ESELONG eseLong )
    {
        return EscrowUpdate( &eseLong, sizeof( eseLong ) );
    }
    JET_ERR operator-=(
        ESELONG eseLong )
    {
        ESELONG eseLongTemp = -eseLong;
        return EscrowUpdate( &eseLongTemp, sizeof( eseLongTemp ) );
    }

    //================
    EWColumn& operator[](
        unsigned long ulTagSeq
    );
    //================
    // prepare EWColumn object for JetSetColumn
    EWColumn& operator OPFUN(
        JET_GRBIT jGrBit,
        unsigned long ulTagSeq,
        unsigned long ulOffset
    );
    // prepare EWColumn object for JetRetrieveColumn
    //EWColumn& operator OPFUN(
    //  JET_GRBIT jGrBit,
    //  unsigned long cbData,
    //  unsigned long ulTagSeq,
    //  unsigned long ulOffset
    //);
    //================
    JET_ERR SetColumn(
        void* pvData,
        unsigned long cbData,
        JET_GRBIT jGrBit = JET_bitNil,
        JET_SETINFO* pjSetInfo = NULL)
    {
        return m_jErr = JetSetColumn( jSesId(), jTabId(), jColId(),
                        pvData, cbData, jGrBit, pjSetInfo );
    }
    //================
    JET_ERR RetrieveColumn(
        void* pvData,
        unsigned long cbData,
        unsigned long* pcbDataActual,
        JET_GRBIT jGrBit = JET_bitNil,
        JET_RETINFO* pjRetInfo = NULL )
    {
        return m_jErr = JetRetrieveColumn( jSesId(), jTabId(), jColId(),
                        pvData, cbData, pcbDataActual, jGrBit, pjRetInfo );
    }
    //================
    JET_ERR EscrowUpdate(
        void* pvData,
        unsigned long cbData,
        void* pvOld = NULL,
        unsigned long cbOld = 0,
        unsigned long* pcbOldActual = NULL,
        JET_GRBIT jGrBit = JET_bitNil )
    {
        return m_jErr = JetEscrowUpdate( jSesId(), jTabId(), jColId(),
                        pvData, cbData, pvOld, cbOld, pcbOldActual, jGrBit );
    }
    //================
    void AllocateBuffer(
        unsigned long cbData = 0
    );
    //================
    void FillRandom(
        long lSeed,
        bool fSave = true
    );

protected:
    //called by EWTable::operator[], EWTempTable::operator[]
    EWColumn(
        EWTable* pewTab,
        const char* szCol,
        JET_COLUMNDEF jColDef
    );

    void nullify( void )
    {
        m_jErr = JET_errTestError;
        m_pewTab = NULL;

        m_jColDef.cbStruct = sizeof( m_jColDef );
        m_jColDef.columnid = JET_colidNil;

        m_pbData = NULL;
        m_cbData = 0;
        m_cbActual = 0;
    }

    JET_ERR m_jErr;             // jet error of last operation
    EWTable* m_pewTab;          // Pointer to EWTable to which we belong
    JET_COLUMNDEF m_jColDef;    // our column definition
    std::string m_strCol;           // column name

    JET_GRBIT m_jSetGrBit;      // temp var to hold JET_GRBIT used by JetSetColumn()
    JET_SETINFO m_jSetInfo;     // temp var to hold JET_SETINFO used by JetSetColumn()

    char* m_pbData;             // pointer to internal data buffer
    unsigned long   m_cbData;       // size of internal data buffer
    unsigned long   m_cbActual;     // actuall data size

    // TODO:
    //JET_GRBIT m_jRetGrBit;
    //unsigned long m_cbRetData;
    //JET_RETINFO m_jRetInfo;
};


//================================================
class EWColumnGroup {
public:
    ~EWColumnGroup( void )
    {
        m_vewCol.clear();
    }
    EWColumnGroup( void )
    {
        nullify();
    }
    EWColumnGroup(
        const EWColumnGroup& rewCG )
    {
        *this = rewCG;
    }
    EWColumnGroup& operator=(
        const EWColumnGroup& rewCG )
    {
        m_jErr = rewCG.m_jErr;
        m_vewCol = rewCG.m_vewCol;

        return *this;
    }

    //================================
    JET_ERR jErr( void ) const { return m_jErr; }

    //================================
    void AddEWColumn(
        const EWColumn& ewCol )
    {
        m_vewCol.push_back( ewCol );
    }

    void AllocateBuffers(
        unsigned long cbData = 0
    );
    void FillRandom(
        long lSeed,
        bool fSave = true
    );

    JET_ERR SetColumns( void );

protected:
    void nullify( void )
    {
        m_jErr = JET_errTestError;
    }

    JET_ERR m_jErr;
    VEC_EWCOL m_vewCol;
};


//================================================
// class in order to facilitate transaction handling
//================================================
class EWTransaction {
friend class EWSession;
public:
    ~EWTransaction( void );

    EWTransaction( void )
    {
        nullify();
    }
    EWTransaction(
        const EWTransaction& rewTrx )
    {
        nullify();
        *this = rewTrx;
    }
    EWTransaction& operator=(
        const EWTransaction& rewTrx )
    {
        m_jErr = rewTrx.m_jErr;
        m_pewSes = rewTrx.m_pewSes;

        return *this;
    }

    //================================
    // TODO: should use EWSession to manufacture EWTransaction?
    //EWTransaction BeginTransaction(
    //  JET_GRBIT jGrBit
    //);

    JET_ERR CommitTransaction(
        JET_GRBIT jGrBit = JET_bitNil
    );
    JET_ERR JetRollback(
        JET_GRBIT jGrBit = JET_bitNil
    );

protected:
    void nullify( void )
    {
        m_jErr = JET_errTestError;
        m_pewSes = NULL;
    }

    EWTransaction( EWSession* pewSes )
    {
        REF( pewSes );
    }

    JET_ERR m_jErr;
    EWSession* m_pewSes;
};

