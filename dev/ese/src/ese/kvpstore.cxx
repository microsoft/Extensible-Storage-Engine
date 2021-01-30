// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#define dbidMax     dbidMax_YOU_MIGHT_THINK_YOU_WANT_DBIDMAX_BUT_YOU_COULD_BE_WRONG




PERSISTED static const WCHAR *  p_wszSchemaInternalMajorVersion     = L".Schema\\Internal\\Major";
PERSISTED static const WCHAR *  p_wszSchemaInternalMinorVersion     = L".Schema\\Internal\\Minor";
PERSISTED static const WCHAR *  p_wszSchemaInternalUpdateVersion    = L".Schema\\Internal\\Update";
PERSISTED static const WCHAR *  p_wszSchemaExternalMajorVersion     = L".Schema\\External\\Major";
PERSISTED static const WCHAR *  p_wszSchemaExternalMinorVersion     = L".Schema\\External\\Minor";
PERSISTED static const WCHAR *  p_wszSchemaExternalUpdateVersion    = L".Schema\\External\\Update";



CKVPStore::CKVPStore( IFMP ifmp, const WCHAR * const wszTableName )
    : CZeroInit( sizeof( *this ) ),
    m_critGlobalTableLock( CLockBasicInfo( CSyncBasicInfo( szKVPStore ), rankKVPStore, 0 ) ),
    m_ifmp ( ifmp )
{
    OSStrCbCopyW( p_wszTableName, sizeof( p_wszTableName ), wszTableName );

    Assert( p_wszSchemaInternalMajorVersion[0] == s_wchSecret );

    Assert( 0 == kvpvtInvalidType );
    for( KVPIValueType iType = kvpvtInvalidType; iType < kvpvtMaxValueType; iType = KVPIValueType( iType + 1 ) )
    {
        m_rgDataTypeInfo[iType].m_type = iType;
        Assert( m_rgDataTypeInfo[iType].m_cid == 0 );
        Assert( m_rgDataTypeInfo[kvpvtSchemaValueType].m_cbDataCol == 0 );
    }

    Assert( m_cidKey == 0 );
    Assert( m_cidType == 0 );

    Assert( m_critGlobalTableLock.FNotOwner() );
}

CKVPStore::~CKVPStore( )
{
    AssertRTL( NULL == m_pfucbGlobalLock );
    AssertRTL( NULL == m_ppibGlobalLock );
}

ERR CKVPStore::ErrKVPIInitIBootstrapCriticalCOLIDs()
{
    ERR err = JET_errSuccess;

    Assert( m_critGlobalTableLock.FOwner() );

    JET_COLUMNDEF jcd;
    jcd.cbStruct = sizeof( jcd );

    Call( ErrIsamGetTableColumnInfo( (JET_SESID)m_ppibGlobalLock, (JET_VTID)m_pfucbGlobalLock, "Key", NULL, &jcd, sizeof( jcd ), JET_ColInfo, fFalse ) );
    Assert( m_cidKey == jcd.columnid || m_cidKey == 0  );
    m_cidKey = jcd.columnid;

    Call( ErrIsamGetTableColumnInfo( (JET_SESID)m_ppibGlobalLock, (JET_VTID)m_pfucbGlobalLock, "Type", NULL, &jcd, sizeof( jcd ), JET_ColInfo, fFalse ) );
    Assert( m_cidType == jcd.columnid || m_cidType == 0  );
    m_cidType = jcd.columnid;

    Call( ErrIsamGetTableColumnInfo( (JET_SESID)m_ppibGlobalLock, (JET_VTID)m_pfucbGlobalLock, "iValue", NULL, &jcd, sizeof( jcd ), JET_ColInfo, fFalse ) );
    Assert( m_rgDataTypeInfo[kvpvtSchemaValueType].m_cid == jcd.columnid || m_rgDataTypeInfo[kvpvtSchemaValueType].m_cid == 0  );
    m_rgDataTypeInfo[kvpvtSchemaValueType].m_cid = jcd.columnid;
    Assert( m_rgDataTypeInfo[kvpvtSchemaValueType].m_cbDataCol == cbKVPIVariableDataSize ||
            m_rgDataTypeInfo[kvpvtSchemaValueType].m_cbDataCol == 4 );

HandleError:

    return err;
}

ERR CKVPStore::ErrKVPIInitICreateTable( PIB * ppib, const IFMP ifmp, const CHAR * const szTableName, const DWORD dwMajorVersionExpected )
{
    ERR err = JET_errSuccess;

    Assert( !g_fRepair );
    Assert( ppibNil != ppib );
    Assert( ifmpNil != ifmp );
    Assert( NULL == m_pfucbGlobalLock );

    BOOL                fInTransaction  = fFalse;

    ULONG iValueDefault = 0;
    JET_COLUMNCREATE_A  rgcolumncreateKVPTable[] = {
        { sizeof(JET_COLUMNCREATE_A), "Key",        JET_coltypBinary,       0,  JET_bitColumnVariable,                          NULL,    0,         0,  0,        JET_errSuccess },
        { sizeof(JET_COLUMNCREATE_A), "Type",       JET_coltypUnsignedByte, 0,  JET_bitColumnFixed|JET_bitColumnNotNULL,        NULL,    0,         0,  0,        JET_errSuccess },
        { sizeof(JET_COLUMNCREATE_A), "iValue",     JET_coltypLong,         0,  JET_bitColumnFixed|JET_bitColumnEscrowUpdate,   &iValueDefault,  sizeof(iValueDefault),         0,  0,        JET_errSuccess },
    };

    static const CHAR szMSysKVPTableIndex[]     = "KeyPrimary";
    static const CHAR szMSysKVPTableIndexKey[]  = "+Key\0";

    JET_INDEXCREATE3_A  rgindexcreateKVPTable[] = {
    {
        sizeof( JET_INDEXCREATE3_A ),
        const_cast<char *>( szMSysKVPTableIndex ),
        const_cast<char *>( szMSysKVPTableIndexKey ),
        sizeof( szMSysKVPTableIndexKey ),
        JET_bitIndexPrimary|JET_bitIndexDisallowTruncation,
        92,
        0,
        0,
        NULL,
        0,
        JET_errSuccess,
        255,
        NULL
    },
};

    JET_TABLECREATE5_A  tablecreateKVPTable = {
        sizeof( JET_TABLECREATE5_A ),
        const_cast<char *>( szTableName ),
        NULL,
        1,
        92,
        rgcolumncreateKVPTable,
        _countof(rgcolumncreateKVPTable),
        rgindexcreateKVPTable,
        _countof(rgindexcreateKVPTable),
        NULL,
        JET_cbtypNull,
        JET_bitTableCreateSystemTable,
        NULL,
        NULL,
        0,
        0,
        JET_TABLEID( pfucbNil ),
        0,
};

    const JET_DBID  jdbid   = (JET_DBID)ifmp;

    Assert( !fInTransaction );
    Call( ErrDIRBeginTransaction( ppib, 53719, NO_GRBIT ) );
    fInTransaction = fTrue;

    Call( ErrFILECreateTable( ppib, jdbid, &tablecreateKVPTable ) );


    m_pfucbGlobalLock = (FUCB*)tablecreateKVPTable.tableid;


    Call( ErrKVPIInitIBootstrapCriticalCOLIDs() );


    Assert( m_cidKey == rgcolumncreateKVPTable[0].columnid );
    Assert( m_cidType == rgcolumncreateKVPTable[1].columnid );
    Assert( m_rgDataTypeInfo[kvpvtSchemaValueType].m_type == kvpvtSchemaValueType );
    Assert( m_rgDataTypeInfo[kvpvtSchemaValueType].m_cid = rgcolumncreateKVPTable[2].columnid );

    INT dwOldVersion = 0;
    Call( ErrKVPISetValue( eHasTrx, p_wszSchemaInternalMajorVersion, kvpvtSchemaValueType, (BYTE*)&p_dwInternalMajorVersion, sizeof( p_dwInternalMajorVersion ) ) );
    Call( ErrKVPISetValue( eHasTrx, p_wszSchemaInternalMinorVersion, kvpvtSchemaValueType, (BYTE*)&dwOldVersion, sizeof( dwOldVersion ) ) );
    Call( ErrKVPISetValue( eHasTrx, p_wszSchemaInternalUpdateVersion, kvpvtSchemaValueType, (BYTE*)&dwOldVersion, sizeof( dwOldVersion ) ) );

    INT dwInitialUserSubVersions = 0;
    Call( ErrKVPISetValue( eHasTrx, p_wszSchemaExternalMajorVersion, kvpvtSchemaValueType, (BYTE*)&dwMajorVersionExpected, sizeof( dwMajorVersionExpected ) ) );
    Call( ErrKVPISetValue( eHasTrx, p_wszSchemaExternalMinorVersion, kvpvtSchemaValueType, (BYTE*)&dwInitialUserSubVersions, sizeof( dwInitialUserSubVersions ) ) );
    Call( ErrKVPISetValue( eHasTrx, p_wszSchemaExternalUpdateVersion, kvpvtSchemaValueType, (BYTE*)&dwInitialUserSubVersions, sizeof( dwInitialUserSubVersions ) ) );

    m_pfucbGlobalLock = NULL;
    Call( ErrFILECloseTable( ppib, (FUCB *)tablecreateKVPTable.tableid ) );

    Assert( fInTransaction );
    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    fInTransaction = fFalse;

HandleError:

    if( fInTransaction )
    {
        const ERR errRollback = ErrDIRRollback( ppib );
        CallSx( errRollback, JET_errRollbackError );
        Assert( errRollback >= JET_errSuccess || PinstFromPpib( ppib )->FInstanceUnavailable( ) );
        m_pfucbGlobalLock = NULL;
        fInTransaction = fFalse;
    }

    Assert( NULL == m_pfucbGlobalLock );

    return err;
}

#ifdef FUTURE_SUPPORT_MORE_VALUE_TYPES

ERR CKVPStore::ErrKVPIInitIUpgradeIAddBlobValueType( const DWORD dwUpgradeVersion )
{
    ERR err = JET_errSuccess;
    BOOL fInTransaction = fFalse;
    JET_COLUMNDEF jcd = { 0 };

    Assert( dwUpgradeVersion == p_dwMinorAddBlobValueType );

    Assert( !fInTransaction );
    Call( ErrDIRBeginTransaction( m_ppibGlobalLock, 41431, NO_GRBIT ) );
    fInTransaction = fTrue;

    jcd.cbStruct = sizeof( jcd );
    jcd.coltyp = JET_coltypBinary;
    jcd.grbit = JET_bitColumnTagged;

    JET_COLUMNID jcid;

    Call( ErrIsamAddColumn( m_ppibGlobalLock, m_pfucbGlobalLock, "rgValue", &jcd, NULL, 0, &jcid ) );

    Call( ErrKVPISetValue( eHasTrx, p_wszSchemaInternalMinorVersion, kvpvtSchemaValueType, (BYTE*)&dwUpgradeVersion, sizeof( dwUpgradeVersion ) ) );

    Assert( fInTransaction );
    Call( ErrDIRCommitTransaction( m_ppibGlobalLock, NO_GRBIT ) );
    fInTransaction = fFalse;

    OSTrace( JET_tracetagUpgrade, OSFormat( "KVPStore( %ws ) upgraded minor version: %d -> %d\n", p_wszTableName, dwUpgradeVersion - 1, dwUpgradeVersion ) );

HandleError:

    if( fInTransaction )
    {
        const ERR errRollback = ErrDIRRollback( m_ppibGlobalLock );
        CallSx( errRollback, JET_errRollbackError );
        Assert( errRollback >= JET_errSuccess || PinstFromPpib( m_ppibGlobalLock )->FInstanceUnavailable( ) );
        fInTransaction = fFalse;
    }

    return err;
}


ERR CKVPStore::ErrKVPIInitIUpgradeIAddQwordValueType( const DWORD dwUpgradeVersion )
{
    ERR err = JET_errSuccess;
    BOOL fInTransaction = fFalse;
    JET_COLUMNDEF jcd = { 0 };

    Assert( dwUpgradeVersion == p_dwUpdateAddQwordValueType );

    Assert( !fInTransaction );
    Call( ErrDIRBeginTransaction( m_ppibGlobalLock, 57815, NO_GRBIT ) );
    fInTransaction = fTrue;


    jcd.cbStruct = sizeof( jcd );
    jcd.coltyp = JET_coltypCurrency;
    jcd.grbit = JET_bitColumnVariable;

    JET_COLUMNID jcid;

    Call( ErrIsamAddColumn( m_ppibGlobalLock, m_pfucbGlobalLock, "qwValue", &jcd, NULL, 0, &jcid ) );

    Call( ErrKVPISetValue( eHasTrx, p_wszSchemaInternalUpdateVersion, kvpvtSchemaValueType, (BYTE*)&dwUpgradeVersion, sizeof( dwUpgradeVersion ) ) );

    Assert( fInTransaction );
    Call( ErrDIRCommitTransaction( m_ppibGlobalLock, NO_GRBIT ) );
    fInTransaction = fFalse;

    OSTrace( JET_tracetagUpgrade, OSFormat( "KVPStore( %ws ) upgraded update version: %u -> %u\n", p_wszTableName, dwUpgradeVersion - 1, dwUpgradeVersion ) );

HandleError:

    if( fInTransaction )
    {
        const ERR errRollback = ErrDIRRollback( m_ppibGlobalLock );
        CallSx( errRollback, JET_errRollbackError );
        Assert( errRollback >= JET_errSuccess || PinstFromPpib( m_ppibGlobalLock )->FInstanceUnavailable( ) );
        fInTransaction = fFalse;
    }

    return err;
}

#endif

INLINE BOOL FSkipUpgrade()
{
    return (BOOL)UlConfigOverrideInjection( 43991, fFalse );
}

ERR CKVPStore::ErrKVPIInitIUpgradeTable()
{
    ERR err = JET_errSuccess;

    Assert( m_critGlobalTableLock.FOwner() );

    INT dwKeyValueTableMajorVersion;
    INT dwKeyValueTableMinorVersion;
    INT dwKeyValueTableUpdateVersion;
    Call( ErrKVPIGetValue( eHasTrx, p_wszSchemaInternalMajorVersion, kvpvtSchemaValueType, (BYTE*)&dwKeyValueTableMajorVersion, sizeof( dwKeyValueTableMajorVersion ) ) );
    Call( ErrKVPIGetValue( eHasTrx, p_wszSchemaInternalMinorVersion, kvpvtSchemaValueType, (BYTE*)&dwKeyValueTableMinorVersion, sizeof( dwKeyValueTableMinorVersion ) ) );
    Call( ErrKVPIGetValue( eHasTrx, p_wszSchemaInternalUpdateVersion, kvpvtSchemaValueType, (BYTE*)&dwKeyValueTableUpdateVersion, sizeof( dwKeyValueTableUpdateVersion ) ) );

    BOOL fVersionMismatch = fFalse;
    if ( p_dwInternalMajorVersion != dwKeyValueTableMajorVersion ||
            p_dwInternalMinorVersion != dwKeyValueTableMinorVersion ||
            p_dwInternalUpdateVersion != dwKeyValueTableUpdateVersion )
    {
        OSTrace( JET_tracetagUpgrade, OSFormat( "KVPStore( %ws ) Internal Schema Version (initial): %u.%u.%u\n", p_wszTableName, dwKeyValueTableMajorVersion, dwKeyValueTableMinorVersion, dwKeyValueTableUpdateVersion ) );
        fVersionMismatch = fTrue;
    }


    if ( p_dwInternalMajorVersion != dwKeyValueTableMajorVersion )
    {
        AssertSz( fFalse, "Huh, opening a mismatched major version?!?" );
        Error( ErrERRCheck( JET_errInvalidDatabaseVersion ) );
    }


    if ( p_dwInternalMinorVersion < dwKeyValueTableMinorVersion )
    {
        AssertSz( fFalse, "Huh, opening a newer minor version?" );
        Error( ErrERRCheck( JET_errInvalidDatabaseVersion ) );
    }

    if ( p_dwInternalMinorVersion > dwKeyValueTableMinorVersion )
    {
        for( INT dwUpgradeVersion = dwKeyValueTableMinorVersion + 1; dwUpgradeVersion <= p_dwInternalMinorVersion; dwUpgradeVersion++ )
        {
            if ( FSkipUpgrade() )
            {
                break;
            }

            switch( dwUpgradeVersion )
            {
#ifdef FUTURE_SUPPORT_MORE_VALUE_TYPES
                case p_dwMinorAddBlobValueType:
                {
                    Call( ErrKVPIInitIUpgradeIAddBlobValueType( p_dwMinorAddBlobValueType ) );
                }
                    break;
#endif

                case INT_MAX:
                default:
                    AssertSz( fFalse, "Unexpected upgrade version ... was the upgrade table not updated with all intermediate versions" );
                    break;
            }


            Call( ErrKVPIGetValue( eHasTrx, p_wszSchemaInternalMinorVersion, kvpvtSchemaValueType, (BYTE*)&dwKeyValueTableMinorVersion, sizeof( dwKeyValueTableMinorVersion ) ) );
            Assert( dwKeyValueTableMinorVersion == dwUpgradeVersion );
        }

        Call( ErrKVPIGetValue( eHasTrx, p_wszSchemaInternalMinorVersion, kvpvtSchemaValueType, (BYTE*)&dwKeyValueTableMinorVersion, sizeof( dwKeyValueTableMinorVersion ) ) );
        Assert( dwKeyValueTableMinorVersion == p_dwInternalMinorVersion || FSkipUpgrade() );

    }


    if ( p_dwInternalUpdateVersion != dwKeyValueTableUpdateVersion )
    {
        OSTrace( JET_tracetagUpgrade, OSFormat( "KVPStore( %ws ) had differnet update versions, OnDisk = %d, Code = %d\n", p_wszTableName, dwKeyValueTableUpdateVersion, p_dwInternalUpdateVersion ) );
    }
    if ( p_dwInternalUpdateVersion > dwKeyValueTableUpdateVersion )
    {
        for( INT dwUpgradeVersion = dwKeyValueTableUpdateVersion + 1; dwUpgradeVersion <= p_dwInternalUpdateVersion; dwUpgradeVersion++ )
        {
            if ( FSkipUpgrade() )
            {
                break;
            }

            switch( dwUpgradeVersion )
            {
#ifdef FUTURE_SUPPORT_MORE_VALUE_TYPES
                case p_dwUpdateAddQwordValueType:
                {
                    Call( ErrKVPIInitIUpgradeIAddQwordValueType( p_dwUpdateAddQwordValueType ) );
                }
                    break;
#endif

                case INT_MAX:
                default:
                    AssertSz( fFalse, "Unexpected upgrade version ... was the upgrade table not updated with all intermediate versions" );
                    break;
            }


            Call( ErrKVPIGetValue( eHasTrx, p_wszSchemaInternalUpdateVersion, kvpvtSchemaValueType, (BYTE*)&dwKeyValueTableUpdateVersion, sizeof( dwKeyValueTableUpdateVersion ) ) );
            Assert( dwKeyValueTableUpdateVersion == dwUpgradeVersion );
        }

        Call( ErrKVPIGetValue( eHasTrx, p_wszSchemaInternalUpdateVersion, kvpvtSchemaValueType, (BYTE*)&dwKeyValueTableUpdateVersion, sizeof( dwKeyValueTableUpdateVersion ) ) );
        Assert( dwKeyValueTableUpdateVersion == p_dwInternalMinorVersion || FSkipUpgrade() );
    }

    Call( ErrKVPIGetValue( eHasTrx, p_wszSchemaInternalMajorVersion, kvpvtSchemaValueType, (BYTE*)&dwKeyValueTableMajorVersion, sizeof( dwKeyValueTableMajorVersion ) ) );
    Call( ErrKVPIGetValue( eHasTrx, p_wszSchemaInternalMinorVersion, kvpvtSchemaValueType, (BYTE*)&dwKeyValueTableMinorVersion, sizeof( dwKeyValueTableMinorVersion ) ) );
    Call( ErrKVPIGetValue( eHasTrx, p_wszSchemaInternalUpdateVersion, kvpvtSchemaValueType, (BYTE*)&dwKeyValueTableUpdateVersion, sizeof( dwKeyValueTableUpdateVersion ) ) );

    if ( fVersionMismatch )
    {
        OSTrace( JET_tracetagUpgrade, OSFormat( "KVPStore( %ws ) Internal Schema Version (final): %u.%u.%u\n", p_wszTableName, dwKeyValueTableMajorVersion, dwKeyValueTableMinorVersion, dwKeyValueTableUpdateVersion ) );
    }

HandleError:

    return err;
}

ERR CKVPStore::ErrKVPIInitILoadValueTypeCOLIDs()
{
    ERR err = JET_errSuccess;

    JET_COLUMNDEF jcd;
    jcd.cbStruct = sizeof( jcd );

    Assert( m_ppibGlobalLock->Level() == 0 );

    KVPIValueType iValueType;
    Assert( 1 == kvpvtSchemaValueType );
    for( iValueType = kvpvtSchemaValueType; iValueType < kvpvtMaxValueType; iValueType = KVPIValueType( iValueType + 1 ) )
    {
        Assert( m_rgDataTypeInfo[iValueType].m_type == iValueType );

        CHAR * pszValueDataColumn = NULL;
        INT cbColumnSize = cbKVPIVariableDataSize;

        switch ( iValueType )
        {
            case kvpvtSchemaValueType:
            case kvpvtIntValueType:
                pszValueDataColumn = "iValue";
                cbColumnSize = 4;
                break;
#ifdef FUTURE_SUPPORT_MORE_VALUE_TYPES
            case kvpvtQwordValueType:
                pszValueDataColumn = "qwValue";
                cbColumnSize = 8;
                break;
            case kvpvtBlobValueType:
                pszValueDataColumn = "rgValue";
                cbColumnSize = cbKVPIVariableDataSize;
                break;
#endif
            default:
                AssertSz( fFalse, "Unknown KVPIValueType = %d!\n", iValueType );
        }


        const ERR errT = ErrIsamGetTableColumnInfo( (JET_SESID)m_ppibGlobalLock, (JET_VTID)m_pfucbGlobalLock, pszValueDataColumn, NULL, &jcd, sizeof( jcd ), JET_ColInfo, fFalse );
        if ( errT < JET_errSuccess && FSkipUpgrade() )
        {
            continue;
        }
        Call( errT );


        m_rgDataTypeInfo[iValueType].m_cid = jcd.columnid;
        m_rgDataTypeInfo[iValueType].m_cbDataCol = cbColumnSize;
    }

HandleError:

    return err;
}

ERR CKVPStore::ErrKVPIOpenGlobalPibAndFucb()
{
    ERR err = JET_errSuccess;
    CAutoSZDDL  szTableName;

    Assert( m_critGlobalTableLock.FOwner() );

    if ( m_ppibGlobalLock && m_pfucbGlobalLock )
    {
        return JET_errSuccess;
    }

    Assert( ppibNil == m_ppibGlobalLock );
    Assert( NULL == m_pfucbGlobalLock );

    IFMP ifmp = g_ifmpMax;
    BOOL fDatabaseOpened = fFalse;
    OPERATION_CONTEXT opContext = { OCUSER_KVPSTORE, 0, 0, 0, 0 };

    Call( ErrPIBBeginSession( PinstFromIfmp( m_ifmp ), &m_ppibGlobalLock, procidNil, fFalse ) );
    Call( m_ppibGlobalLock->ErrSetOperationContext( &opContext, sizeof( opContext ) ) );

    Call( ErrDBOpenDatabase( m_ppibGlobalLock, g_rgfmp[m_ifmp].WszDatabaseName(), &ifmp, NO_GRBIT ) );
    Assert( ifmp == m_ifmp );

    fDatabaseOpened = fTrue;

    CallS( szTableName.ErrSet( p_wszTableName ) );

    Call( ErrFILEOpenTable( m_ppibGlobalLock, m_ifmp, &m_pfucbGlobalLock, (CHAR*)szTableName ) );

HandleError:

    if ( err < JET_errSuccess )
    {
        if ( fDatabaseOpened )
        {
            ErrDBCloseDatabase( m_ppibGlobalLock, ifmp, NO_GRBIT );
        }
        if ( m_ppibGlobalLock )
        {
            PIBEndSession( m_ppibGlobalLock );
            m_ppibGlobalLock = NULL;
        }
        Assert( NULL == m_pfucbGlobalLock );
    }

    return err;
}

VOID CKVPStore::KVPICloseGlobalPibAndFucb()
{
    Assert( m_critGlobalTableLock.FOwner() );
    Assert( ( m_pfucbGlobalLock == NULL ) || ( m_ppibGlobalLock != NULL ) );

    if ( NULL == m_ppibGlobalLock && NULL == m_pfucbGlobalLock )
    {
        return;
    }

    if ( m_ppibGlobalLock != NULL )
    {
        if ( m_pfucbGlobalLock != NULL )
        {
            CallS( ErrFILECloseTable( m_ppibGlobalLock, m_pfucbGlobalLock ) );
            m_pfucbGlobalLock = NULL;
        }

        CallS( ErrDBCloseDatabase( m_ppibGlobalLock, m_ifmp, NO_GRBIT ) );

        PIBEndSession( m_ppibGlobalLock );
        m_ppibGlobalLock = NULL;
    }
}

ERR CKVPStore::ErrKVPInitStore( PIB * const ppibProvided, const TrxUpdate eTrxUpd, const DWORD dwMajorVersionExpected )
{
    ERR err = JET_errSuccess;
    IFMP ifmp = g_ifmpMax;
    Assert( ifmp == g_ifmpMax );
    CAutoSZDDL  szTableName;
    bool fReadOnly = (eReadOnly == eTrxUpd);
    bool fFailedOpenDatabase = false;

    Assert( ppibNil == m_ppibGlobalLock );
    Assert( NULL == m_pfucbGlobalLock );


    m_critGlobalTableLock.Enter();

    CallS( szTableName.ErrSet( p_wszTableName ) );

    if ( ppibProvided )
    {

        m_ppibGlobalLock = ppibProvided;

        ifmp = m_ifmp;
    }
    else
    {

        Call( ErrPIBBeginSession( PinstFromIfmp( m_ifmp ), &m_ppibGlobalLock, procidNil, fFalse ) );

        OPERATION_CONTEXT opContext = { OCUSER_KVPSTORE, 0, 0, 0, 0 };
        Call( m_ppibGlobalLock->ErrSetOperationContext( &opContext, sizeof( opContext ) ) );


        fFailedOpenDatabase = true;
        Call( ErrDBOpenDatabase( m_ppibGlobalLock, g_rgfmp[m_ifmp].WszDatabaseName(), &ifmp, fReadOnly ? JET_bitDbReadOnly : NO_GRBIT ) );
        fFailedOpenDatabase = false;

        Assert( ifmp == m_ifmp );
        if ( JET_wrnFileOpenReadOnly == err )
        {
            fReadOnly = true;
        }
    }

    err = ErrFILEOpenTable( m_ppibGlobalLock, m_ifmp, &m_pfucbGlobalLock, (CHAR*)szTableName );

    if ( JET_errObjectNotFound == err && !fReadOnly )
    {

        Call( ErrKVPIInitICreateTable( m_ppibGlobalLock, m_ifmp, (CHAR*)szTableName, dwMajorVersionExpected ) );

        Call( ErrFILEOpenTable( m_ppibGlobalLock, m_ifmp, &m_pfucbGlobalLock, (CHAR*)szTableName ) );
    }
    else
    {
        Call( err );


        Call( ErrKVPIInitIBootstrapCriticalCOLIDs() );
    }
    Assert( m_cidKey != m_cidType  );


    Call( ErrKVPIInitIUpgradeTable() );


    Call( ErrKVPIInitILoadValueTypeCOLIDs() );


    DWORD dwUserMajorVersion = 0;

    Call( ErrKVPIGetValue( eHasTrx, p_wszSchemaExternalMajorVersion, kvpvtSchemaValueType, (BYTE*)&dwUserMajorVersion, sizeof( dwUserMajorVersion ) ) );

    if ( dwUserMajorVersion != dwMajorVersionExpected )
    {
        Error( ErrERRCheck( JET_errInvalidDatabaseVersion ) );
    }

HandleError:


    if ( m_pfucbGlobalLock )
    {
        CallS( ErrFILECloseTable( m_ppibGlobalLock, m_pfucbGlobalLock ) );
        m_pfucbGlobalLock = NULL;
    }


    if ( ppibProvided )
    {
        ifmp = g_ifmpMax;
        Assert( ifmp == g_ifmpMax );
        m_ppibGlobalLock = NULL;
    }

    if ( ifmp != g_ifmpMax && !fFailedOpenDatabase )
    {
        CallS( ErrDBCloseDatabase( m_ppibGlobalLock, m_ifmp, NO_GRBIT ) );
    }

    if ( m_ppibGlobalLock )
    {
        PIBEndSession( m_ppibGlobalLock );
        m_ppibGlobalLock = NULL;
    }

    Assert( NULL == m_ppibGlobalLock );
    Assert( NULL == m_pfucbGlobalLock );

    m_critGlobalTableLock.Leave();

    return err;
}

VOID CKVPStore::KVPTermStore()
{
    m_critGlobalTableLock.Enter();

    KVPICloseGlobalPibAndFucb();

    Assert( NULL == m_ppibGlobalLock );
    Assert( NULL == m_pfucbGlobalLock );

    m_critGlobalTableLock.Leave();
}



ERR CKVPStore::ErrKVPGetUserVersion( __out DWORD * pdwMinorVersion, __out DWORD * pwUpdateVersion )
{
    ERR err = JET_errSuccess;


    CallR( ErrKVPIBeginTrx( eReadWrite, 40919 ) );


    Call( ErrKVPIGetValue( eHasTrx, p_wszSchemaExternalMinorVersion, kvpvtSchemaValueType, (BYTE*)pdwMinorVersion, sizeof( *pdwMinorVersion ) ) );
    Call( ErrKVPIGetValue( eHasTrx, p_wszSchemaExternalUpdateVersion, kvpvtSchemaValueType, (BYTE*)pwUpdateVersion, sizeof( *pwUpdateVersion ) ) );

HandleError:


    err = ErrKVPIEndTrx( err );

    return err;
}

ERR CKVPStore::ErrKVPSetUserVersion( const DWORD dwMinorVersion, const DWORD dwUpdateVersion )
{
    ERR err = JET_errSuccess;


    CallR( ErrKVPIBeginTrx( eReadWrite, 57303 ) );


    Call( ErrKVPISetValue( eHasTrx, p_wszSchemaExternalMinorVersion, kvpvtSchemaValueType, (BYTE*)&dwMinorVersion, sizeof( dwMinorVersion ) ) );
    Call( ErrKVPISetValue( eHasTrx, p_wszSchemaExternalUpdateVersion, kvpvtSchemaValueType, (BYTE*)&dwUpdateVersion, sizeof( dwUpdateVersion ) ) );

HandleError:


    err = ErrKVPIEndTrx( err );

    return err;
}



ERR CKVPStore::ErrKVPIBeginTrx( const TrxUpdate eTrxUpd, TRXID trxid )
{
    ERR err = JET_errSuccess;

    Assert( m_critGlobalTableLock.FNotOwner() );

    Assert( eTrxUpd == eReadOnly || eTrxUpd == eReadWrite );

    m_critGlobalTableLock.Enter();

    Assert( NULL == m_ppibGlobalLock );

    Call( ErrKVPIOpenGlobalPibAndFucb() );

    Assert( m_ppibGlobalLock != ppibNil && m_ppibGlobalLock != (PIB*)JET_sesidNil );
    Assert( m_pfucbGlobalLock != pfucbNil && m_pfucbGlobalLock != (FUCB*)JET_tableidNil );


    Call( ErrIsamBeginTransaction( (JET_SESID)m_ppibGlobalLock, trxid, ( eTrxUpd == eReadOnly ) ? JET_bitTransactionReadOnly : NO_GRBIT ) );

HandleError:

    if ( err < JET_errSuccess )
    {
        Assert( NULL == m_ppibGlobalLock );
        Assert( NULL == m_pfucbGlobalLock );

        m_critGlobalTableLock.Leave();
    }

    Assert( ( m_critGlobalTableLock.FOwner() && err >= JET_errSuccess ) ||
            ( m_critGlobalTableLock.FNotOwner() && err < JET_errSuccess ) );

    return err;
}

ERR CKVPStore::ErrKVPIEndTrx( const ERR errRet )
{
    Assert( m_critGlobalTableLock.FOwner() );
    Assert( m_ppibGlobalLock != ppibNil && m_ppibGlobalLock != (PIB*)JET_sesidNil );
    Assert( m_pfucbGlobalLock != pfucbNil && m_pfucbGlobalLock != (FUCB*)JET_tableidNil );

    ERR errC = JET_errInternalError;
    ERR errR = JET_errInternalError;


    if( ( errRet < JET_errSuccess ) ||
        ( ( errC = ErrIsamCommitTransaction( (JET_SESID)m_ppibGlobalLock, JET_bitCommitLazyFlush ) ) < JET_errSuccess ) )
    {
        errR = ErrIsamRollback( (JET_SESID)m_ppibGlobalLock, NO_GRBIT );
        Assert( errR >= JET_errSuccess || PinstFromPpib( m_ppibGlobalLock )->FInstanceUnavailable() );
    }


    KVPICloseGlobalPibAndFucb();


    Assert( NULL == m_ppibGlobalLock );
    Assert( NULL == m_pfucbGlobalLock );

    m_critGlobalTableLock.Leave();


    if ( errRet < JET_errSuccess )
    {
        return errRet;
    }
    if ( errC >= JET_errSuccess )
    {
        return errC;
    }
    if ( errR < JET_errSuccess )
    {
        Assert( errR != JET_errInternalError );
        return ErrERRCheck( JET_errRollbackError );
    }

    return errC;
}


ERR CKVPStore::ErrKVPIAcquireTrx( const TrxUpdate eTrxUpd, __inout PIB * ppib, TRXID trxid )
{
    ERR     err = JET_errSuccess;
    CAutoSZDDL  szTableName;

    Assert( m_critGlobalTableLock.FNotOwner() );

    Assert( eTrxUpd == eReadOnly || eTrxUpd == eReadWrite );

    Assert( ppib );
    Assert( g_rgfmp[ m_ifmp ].FExclusiveBySession( ppib ) );
    Assert( !g_rgfmp[ m_ifmp ].FReadOnlyAttach() || eTrxUpd == eReadOnly );

    m_critGlobalTableLock.Enter();

    Assert( m_ppibGlobalLock == ppibNil || m_ppibGlobalLock == (PIB*)JET_sesidNil );
    Assert( NULL == m_pfucbGlobalLock );


    m_ppibGlobalLock = ppib;


    CallS( szTableName.ErrSet( p_wszTableName ) );
    Call( ErrFILEOpenTable( m_ppibGlobalLock, m_ifmp, &m_pfucbGlobalLock, (CHAR*)szTableName ) );


    Assert( m_ppibGlobalLock->Level() == 0 || !m_ppibGlobalLock->FReadOnlyTrx() || eTrxUpd == eReadOnly );

    Call( ErrIsamBeginTransaction( (JET_SESID)m_ppibGlobalLock, trxid, ( eTrxUpd == eReadOnly ) ? JET_bitTransactionReadOnly : NO_GRBIT ) );

HandleError:

    if ( err < JET_errSuccess )
    {
        if ( m_pfucbGlobalLock )
        {
            CallS( ErrFILECloseTable( m_ppibGlobalLock, m_pfucbGlobalLock ) );
            m_pfucbGlobalLock = NULL;
        }

        m_ppibGlobalLock = NULL;

        Assert( NULL == m_ppibGlobalLock );
        Assert( NULL == m_pfucbGlobalLock );

        m_critGlobalTableLock.Leave();
    }

    Assert( ( m_critGlobalTableLock.FOwner() && err >= JET_errSuccess ) ||
            ( m_critGlobalTableLock.FNotOwner() && err < JET_errSuccess ) );

    return err;
}


ERR CKVPStore::ErrKVPIReleaseTrx( const ERR errRet )
{
    Assert( m_critGlobalTableLock.FOwner() );
    Assert( m_ppibGlobalLock != ppibNil && m_ppibGlobalLock != (PIB*)JET_sesidNil );
    Assert( m_pfucbGlobalLock != pfucbNil && m_pfucbGlobalLock != (FUCB*)JET_tableidNil );

    ERR errC = JET_errInternalError;
    ERR errR = JET_errInternalError;


    if( ( errRet < JET_errSuccess ) ||
        ( ( errC = ErrIsamCommitTransaction( (JET_SESID)m_ppibGlobalLock, NO_GRBIT ) ) < JET_errSuccess ) )
    {
        errR = ErrIsamRollback( (JET_SESID)m_ppibGlobalLock, NO_GRBIT );
        CallSx( errR, JET_errRollbackError );
        Assert( errR >= JET_errSuccess || PinstFromPpib( m_ppibGlobalLock )->FInstanceUnavailable() );
    }


    CallS( ErrFILECloseTable( m_ppibGlobalLock, m_pfucbGlobalLock ) );
    m_pfucbGlobalLock = NULL;


    m_ppibGlobalLock = NULL;


    Assert( NULL == m_ppibGlobalLock );
    Assert( NULL == m_pfucbGlobalLock );

    m_critGlobalTableLock.Leave();


    if ( errRet < JET_errSuccess )
    {
        return errRet;
    }
    if ( errC >= JET_errSuccess )
    {
        return errC;
    }
    if ( errR < JET_errSuccess )
    {
        Assert( errR == JET_errRollbackError );
        return errR;
    }

    return errC;
}



ERR CKVPStore::ErrKVPIPrepUpdate(  const WCHAR * const wszKey, KVPIValueType kvpvt )
{
    ERR     err = JET_errSuccess;

    BOOL    fInUpdate = fFalse;

    Assert( kvpvt > kvpvtInvalidType );
    Assert( kvpvt < kvpvtMaxValueType );


    const INT cbKey = CbKVPKeySize( wszKey );

    Call( ErrIsamMakeKey( m_ppibGlobalLock, m_pfucbGlobalLock, wszKey, cbKey, JET_bitNewKey ) );

    err = ErrIsamSeek( m_ppibGlobalLock, m_pfucbGlobalLock, JET_bitSeekEQ );

    if ( err != JET_errRecordNotFound )
    {
        Call( err );
    }


    if ( err == JET_errRecordNotFound )
    {


        Call( ErrIsamPrepareUpdate( m_ppibGlobalLock, m_pfucbGlobalLock, JET_prepInsert ) );
        fInUpdate  = fTrue;


        Call( ErrIsamSetColumn( m_ppibGlobalLock, m_pfucbGlobalLock, m_cidKey, (void*)wszKey, cbKey, NO_GRBIT, NULL ) );


        Assert( kvpvt < kvpvtMaxValueType );
        C_ASSERT( kvpvtMaxValueType < 255 );
        BYTE eType = (BYTE)kvpvt;

        Call( ErrIsamSetColumn( m_ppibGlobalLock, m_pfucbGlobalLock, m_cidType, (void*)&eType, sizeof( eType ), NO_GRBIT, NULL ) );
    }
    else
    {


        Assert( err >= JET_errSuccess );
        Call( ErrIsamPrepareUpdate( m_ppibGlobalLock, m_pfucbGlobalLock, JET_prepReplace ) );
        fInUpdate  = fTrue;


        ULONG cbActual;
        BYTE bValueType = (BYTE)kvpvtInvalidType;
        Call( ErrIsamRetrieveColumn( m_ppibGlobalLock, m_pfucbGlobalLock, m_cidType, (void*)&bValueType, sizeof( bValueType ), &cbActual, NO_GRBIT, NULL ) );
        Assert( cbActual == 1 );
        if ( ((KVPIValueType)bValueType) != kvpvt )
        {
            AssertSz( FNegTest( fInvalidUsage ), "The value types did not match: %d != %d", ((KVPIValueType)bValueType), kvpvt );
            Error( ErrERRCheck( JET_errInvalidColumnType ) );
        }
    }

    CallS( err );
    Assert( fInUpdate );
    return err;

HandleError:

    Assert( err < JET_errSuccess );
    if ( JET_errRecordNotFound == err )
    {
        AssertSz( fFalse, "We don't expect this error from any other API other than ErrIsamSeek()!" );
        err = ErrERRCheck( JET_errInternalError );
    }

    if ( fInUpdate )
    {
        CallS( ErrIsamPrepareUpdate( m_ppibGlobalLock, m_pfucbGlobalLock, JET_prepCancel ) );
    }

    return err;
}



VOID CKVPStore::AssertKVPIValidUserKey( const KVPIValueType kvpvt, const WCHAR * const wszKey )
{
    Assert( wszKey );
    Assert( wszKey[0] != 0 );
    Assert( wszKey[0] != s_wchSecret || kvpvt == kvpvtSchemaValueType );
    Assert( NULL == wcschr( wszKey, L'*' ) );
}

ERR CKVPStore::ErrKVPISetValue( const TrxPosition eTrxPos, const WCHAR * const wszKey, const KVPIValueType kvpvt, _In_opt_bytecount_( cbValue ) const BYTE * const pbValue, const INT cbValue )
{
    ERR     err = JET_errSuccess;

    BOOL    fOwnsTrx = fFalse;
    BOOL    fInUpdate = fFalse;


    AssertKVPIValidUserKey( kvpvt, wszKey );


    if ( kvpvt <= kvpvtInvalidType ||
            kvpvt >= kvpvtMaxValueType )
    {
        AssertSz( fFalse, "Invalid KVPIValueType = %d here.\n", kvpvt );
        return ErrERRCheck( JET_errInvalidColumnType );
    }


    if ( m_rgDataTypeInfo[kvpvt].m_cbDataCol != cbValue &&
            m_rgDataTypeInfo[kvpvt].m_cbDataCol != cbKVPIVariableDataSize )
    {
        AssertSz( fFalse, "Invalid size of data element passed to CKVPStore = %d\n", cbValue );
        return ErrERRCheck( JET_errInvalidBufferSize );
    }


    if ( eTrxPos == eNewTrx )
    {
        CallR( ErrKVPIBeginTrx( eReadWrite, 33239 ) );
        fOwnsTrx = fTrue;
    }
    else
    {
        Assert( eTrxPos == eHasTrx );
    }
    Assert( m_critGlobalTableLock.FOwner() );

    Assert( m_ppibGlobalLock != ppibNil && m_ppibGlobalLock != (PIB*)JET_sesidNil );
    Assert( m_pfucbGlobalLock != pfucbNil && m_pfucbGlobalLock != (FUCB*)JET_tableidNil );

    Call( ErrKVPIPrepUpdate( wszKey, kvpvt ) );
    fInUpdate = fTrue;


    Call( ErrIsamSetColumn( m_ppibGlobalLock, m_pfucbGlobalLock, m_rgDataTypeInfo[kvpvt].m_cid, pbValue, cbValue, NO_GRBIT, NULL ) );

    Call( ErrIsamUpdate( m_ppibGlobalLock, m_pfucbGlobalLock, NULL, 0, NULL, NO_GRBIT ) );
    fInUpdate = fFalse;

    CallS( err );

HandleError:

    Assert( err <= JET_errSuccess );
    if ( JET_errRecordNotFound == err )
    {
        AssertSz( fFalse, "We don't expect this error from any other API other than ErrIsamSeek()!" );
        err = ErrERRCheck( JET_errInternalError );
    }

    if ( fInUpdate )
    {

        CallS( ErrIsamPrepareUpdate( m_ppibGlobalLock, m_pfucbGlobalLock, JET_prepCancel ) );
    }

    if ( fOwnsTrx )
    {
        err = ErrKVPIEndTrx( err );
        fOwnsTrx = fFalse;
    }

    Assert( m_critGlobalTableLock.FNotOwner() || !fOwnsTrx );

    Assert( fOwnsTrx == fFalse );

    return err;
}

ERR CKVPStore::ErrKVPIGetValue( const TrxPosition eTrxPos, const WCHAR * const wszKey, const KVPIValueType kvpvt, _Out_bytecap_( cbValue ) BYTE * const pbValue, const INT cbValue )
{
    ERR err = JET_errSuccess;

    BOOL    fOwnsTrx = fFalse;


    AssertKVPIValidUserKey( kvpvt, wszKey );


    if ( kvpvt <= kvpvtInvalidType ||
            kvpvt >= kvpvtMaxValueType )
    {
        AssertSz( fFalse, "Invalid KVPIValueType = %d here.\n", kvpvt );
        return ErrERRCheck( JET_errInvalidColumnType );
    }


    if ( m_rgDataTypeInfo[kvpvt].m_cbDataCol != cbValue &&
            m_rgDataTypeInfo[kvpvt].m_cbDataCol != cbKVPIVariableDataSize )
    {
        AssertSz( fFalse, "Invalid size of data element passed to CKVPStore = %d\n", cbValue );
        return ErrERRCheck( JET_errInvalidBufferSize );
    }


    const INT cbKey = CbKVPKeySize( wszKey );
    Assert( 0 == ( cbKey % 2 ) );
    if ( cbKey > cbKVPMaxKey )
    {
        return ErrERRCheck( JET_errKeyTruncated );
    }



    if ( eTrxPos == eNewTrx )
    {
        CallR( ErrKVPIBeginTrx( eReadOnly, 49623 ) );
        fOwnsTrx = fTrue;
    }
    else
    {
        Assert( eTrxPos == eHasTrx );
    }
    Assert( m_critGlobalTableLock.FOwner() );

    Assert( m_ppibGlobalLock != ppibNil && m_ppibGlobalLock != (PIB*)JET_sesidNil );
    Assert( m_pfucbGlobalLock != pfucbNil && m_pfucbGlobalLock != (FUCB*)JET_tableidNil );


    Call( ErrIsamMakeKey( m_ppibGlobalLock, m_pfucbGlobalLock, wszKey, cbKey, JET_bitNewKey ) );

    Call( ErrIsamSeek( m_ppibGlobalLock, m_pfucbGlobalLock, JET_bitSeekEQ ) );


    ULONG cbActual;
    BYTE bValueType = (BYTE)kvpvtInvalidType;
    Call( ErrIsamRetrieveColumn( m_ppibGlobalLock, m_pfucbGlobalLock, m_cidType, (void*)&bValueType, sizeof( bValueType ), &cbActual, NO_GRBIT, NULL ) );
    Assert( cbActual == 1 );
    if ( ((KVPIValueType)bValueType) != kvpvt )
    {
        AssertSz( FNegTest( fInvalidUsage ), "The value types did not match: %d != %d", ((KVPIValueType)bValueType), kvpvt );
        Call( ErrERRCheck( JET_errInvalidColumnType ) );
    }


    Call( ErrIsamRetrieveColumn( m_ppibGlobalLock, m_pfucbGlobalLock, m_rgDataTypeInfo[kvpvt].m_cid, pbValue, cbValue, &cbActual, NO_GRBIT, NULL ) );

    if ( m_rgDataTypeInfo[kvpvt].m_cbDataCol != (INT)cbActual &&
            m_rgDataTypeInfo[kvpvt].m_cbDataCol != cbKVPIVariableDataSize )
    {
        AssertSz( fFalse, "Invalid size of data element in table = %d\n", cbActual );
        return ErrERRCheck( JET_errInvalidBufferSize );
    }

HandleError:

    if ( fOwnsTrx )
    {
        (void)ErrKVPIEndTrx( err );
        fOwnsTrx = fFalse;
    }

    Assert( fOwnsTrx == fFalse );

    return err;
}



INT CKVPStore::CbKVPKeySize( const WCHAR * const wszKey ) const
{
    const INT cbKey = sizeof(WCHAR) * ( 1 + LOSStrLengthW( wszKey ) );
    Assert( 0 == ( cbKey % 2 ) );
    Assert( cbKey > 0 );
    Assert( cbKey <= cbKVPMaxKey || FNegTest( fInvalidUsage ) );
    return cbKey;
}

ERR CKVPStore::ErrKVPSetValue( const WCHAR * const wszKey, const INT iValue )
{
    return ErrKVPISetValue( eNewTrx, wszKey, kvpvtIntValueType, (BYTE*)&iValue, sizeof( iValue ) );
}

#ifdef FUTURE_SUPPORT_MORE_VALUE_TYPES
ERR CKVPStore::ErrKVPSetValue( const WCHAR * const wszKey, const INT64 i64Value )
{
    return ErrKVPISetValue( eNewTrx, wszKey, kvpvtQwordValueType, (BYTE*)&i64Value, sizeof( i64Value ) );
}

ERR CKVPStore::ErrKVPSetValue( const WCHAR * const wszKey, const BYTE * const pbValue, const ULONG cbValue )
{
    return ErrKVPISetValue( eNewTrx, wszKey, kvpvtBlobValueType, pbValue, cbValue );
}
#endif

ERR CKVPStore::ErrKVPAdjValue( __inout PIB * ppibProvided, const WCHAR * const wszKey, const INT iAdjustment )
{
    ERR err = JET_errSuccess;


    if ( ppibProvided != ppibNil )
    {
        Assert( g_rgfmp[ m_ifmp ].FExclusiveBySession( ppibProvided ) );
        CallR( ErrKVPIAcquireTrx( eReadWrite, ppibProvided, 38519 ) );
    }
    else
    {
        CallR( ErrKVPIBeginTrx( eReadWrite, 49111 ) );
    }

    INT iValue = 0;


    err = ErrKVPIGetValue( eHasTrx, wszKey, kvpvtIntValueType, (BYTE*)&iValue, sizeof( iValue ) );


    if ( err == JET_errRecordNotFound )
    {
        iValue = 0;
        err = JET_errSuccess;
    }
    Call( err );


    if ( ( ( iAdjustment > 0 ) && ( ( iValue + iAdjustment ) < iValue ) ) ||
         ( ( iAdjustment < 0 ) && ( ( iValue + iAdjustment ) > iValue ) ) )
    {
        Call( ErrERRCheck( JET_errOutOfAutoincrementValues ) );
    }

    iValue += iAdjustment;


    Call( ErrKVPISetValue( eHasTrx, wszKey, kvpvtIntValueType, (BYTE*)&iValue, sizeof( iValue ) ) );

HandleError:


    if ( ppibProvided != ppibNil )
    {
        err = ErrKVPIReleaseTrx( err );
    }
    else
    {
        err = ErrKVPIEndTrx( err );
    }

    return err;
}

ERR CKVPStore::ErrKVPAdjValue( const WCHAR * const wszKey, const INT iAdjustment )
{
    return ErrKVPAdjValue( ppibNil, wszKey, iAdjustment );
}

ERR CKVPStore::ErrKVPGetValue( const WCHAR * const wszKey, __out INT * piValue )
{
    return ErrKVPIGetValue( eNewTrx, wszKey, kvpvtIntValueType, (BYTE*)piValue, sizeof( *piValue ) );
}

#ifdef FUTURE_SUPPORT_MORE_VALUE_TYPES
ERR CKVPStore::ErrKVPGetValue( const WCHAR * const wszKey, __out INT64 * pi64Value )
{
    return ErrKVPIGetValue( eNewTrx, wszKey, kvpvtQwordValueType, (BYTE*)pi64Value, sizeof( *pi64Value ) );
}

ERR CKVPStore::ErrKVPGetValue( const WCHAR * const wszKey, __out BYTE * const pbValue, const ULONG cbValue )
{
    return ErrKVPIGetValue( eNewTrx, wszKey, kvpvtBlobValueType, (BYTE*)pbValue, cbValue );
}
#endif

VOID CKVPStore::Dump( CPRINTF* pcprintf, const BOOL fInternalToo )
{
    CKVPSCursor kvpscursor( this, fInternalToo ?  L"**" : L"*" );
    ERR err = JET_errSuccess;

    ULONG c = 0;
    while( ( err = kvpscursor.ErrKVPSCursorLocateNextKVP() ) >= JET_errSuccess )
    {
        KVPIValueType kvpvt = kvpvtInvalidType;

        (void)kvpscursor.ErrKVPSCursorIGetType( &kvpvt );

        INT iValue;
        if ( ( err = kvpscursor.ErrKVPSCursorGetValue( &iValue ) ) >= JET_errSuccess )
        {
            (*pcprintf)( "KVP Elem[%d] key = %ws, type = %d, value = %d\n", c, kvpscursor.WszKVPSCursorCurrKey(), kvpvt, iValue );
        }
        else
        {
            (*pcprintf)( "KVP Elem[%d] key = %ws, type = %d, value = Couldn't retrieve, error %d\n", c, kvpscursor.WszKVPSCursorCurrKey(), kvpvt, err );
        }
        c++;
    }
    if ( err != JET_errNoCurrentRecord )
    {
        (*pcprintf)( "Enumerating KVPStore elements failed %d\n", err );
    }
}


CKVPStore::CKVPSCursor::CKVPSCursor( CKVPStore * const pkvps, __in_z const WCHAR * const wszWildKey )
{
    m_pkvps = pkvps;
    m_eState = kvpscUninit;
    OSStrCbCopyW( m_wszWildKey, sizeof( m_wszWildKey ), wszWildKey );
    m_wszCurrentKey[0] = L'\0';
}

CKVPStore::CKVPSCursor::~CKVPSCursor()
{
    if ( m_eState >= kvpscAcquiredTrx )
    {
        (void)m_pkvps->ErrKVPIEndTrx( JET_errSuccess );
    }
    m_eState = kvpscUninit;
    m_pkvps = NULL;
    m_wszWildKey[0] = L'\0';
    m_wszCurrentKey[0] = L'\0';
}

BOOL CKVPStore::CKVPSCursor::FKVPSCursorIMatches() const
{
    const WCHAR * pwszKey = WszKVPSCursorCurrKey();

    Assert( pwszKey );
    if( NULL == pwszKey )
    {
        return fFalse;
    }

    if( FKVPSCursorIUserWildcard( m_wszWildKey ) )
    {
        return pwszKey[0] != CKVPStore::s_wchSecret;
    }
    else if( FKVPSCursorIInternalWildcard( m_wszWildKey ) )
    {
        return pwszKey[0] == CKVPStore::s_wchSecret;
    }
    else if( FKVPSCursorIAllWildcard( m_wszWildKey ) )
    {
        return fTrue;
    }
    else
    {

        const WCHAR * pwszWildchar = wcschr( m_wszWildKey, L'*' );
        const ULONG cchWildkey = LOSStrLengthW( m_wszWildKey );

        if( pwszWildchar &&
            ( pwszWildchar == ( m_wszWildKey + cchWildkey - 1 ) ) )
        {
            return 0 == wcsncmp( m_wszWildKey, pwszKey, cchWildkey - 1 );
        }
        else
        {

            return 0 == wcscmp( m_wszWildKey, pwszKey );
        }
    }
}

ERR CKVPStore::CKVPSCursor::ErrCKVPSCursorIEstablishCurrency()
{
    Assert( m_eState == kvpscEnumerating );

    ULONG cbActual;
    const ERR err = ErrIsamRetrieveColumn( m_pkvps->m_ppibGlobalLock, m_pkvps->m_pfucbGlobalLock, m_pkvps->m_cidKey, (void*)m_wszCurrentKey, sizeof( m_wszCurrentKey ), &cbActual, NO_GRBIT, NULL );
    Assert( cbActual > 2 );
    Assert( cbActual < sizeof( m_wszCurrentKey ) );
    Assert( m_wszCurrentKey[cbActual/sizeof(WCHAR)-1] == L'\0' );
    Assert( m_wszCurrentKey[cbActual/sizeof(WCHAR)-2] != L'\0' );

    return err;
}


ERR CKVPStore::CKVPSCursor::ErrKVPSCursorReset()
{
    if ( m_eState == kvpscAcquiredTrx ||
            m_eState == kvpscEnumerating )
    {
        m_eState = kvpscAcquiredTrx;

        return JET_errSuccess;
    }

    return ErrERRCheck( JET_errInvalidParameter );
}

ERR CKVPStore::CKVPSCursor::ErrKVPSCursorLocateNextKVP()
{
    ERR err = JET_errSuccess;

    if ( m_eState == kvpscUninit )
    {
        Call( m_pkvps->ErrKVPIBeginTrx( CKVPStore::eReadWrite, 65495 ) );

        m_eState = kvpscAcquiredTrx;
    }

    if ( m_eState == kvpscAcquiredTrx )
    {

        Call( ErrIsamMove( m_pkvps->m_ppibGlobalLock, m_pkvps->m_pfucbGlobalLock, JET_MoveFirst, NO_GRBIT ) );

        m_eState = kvpscEnumerating;


        Call( ErrCKVPSCursorIEstablishCurrency() );

        if ( FKVPSCursorIMatches() )
        {

            return JET_errSuccess;
        }

    }

    Assert( m_eState == kvpscEnumerating );

    do
    {

        Call( ErrIsamMove( m_pkvps->m_ppibGlobalLock, m_pkvps->m_pfucbGlobalLock, 1, NO_GRBIT ) );


        Call( ErrCKVPSCursorIEstablishCurrency() );
    }
    while( !FKVPSCursorIMatches() );


    Assert( FKVPSCursorIMatches() );

HandleError:

    Assert( FKVPSCursorIMatches() || err < JET_errSuccess );

    return err;
}

VOID CKVPStore::CKVPSCursor::KVPSCursorEnd()
{
    if ( m_eState >= kvpscAcquiredTrx )
    {
        (void)m_pkvps->ErrKVPIEndTrx( JET_errSuccess );
    }
    m_eState = kvpscUninit;
}


const WCHAR * CKVPStore::CKVPSCursor::WszKVPSCursorCurrKey() const
{
    return m_wszCurrentKey;
}

ERR CKVPStore::CKVPSCursor::ErrKVPSCursorGetValue( __out INT * piValue )
{
    ERR err = JET_errSuccess;

    Assert( m_eState == kvpscEnumerating );

    CKVPStore::KVPIValueType kvpvt = CKVPStore::kvpvtInvalidType;

    CallR( ErrKVPSCursorIGetType( &kvpvt ) );

    if( kvpvt == CKVPStore::kvpvtSchemaValueType ||
        kvpvt == CKVPStore::kvpvtIntValueType )
    {
        return m_pkvps->ErrKVPIGetValue( eHasTrx, m_wszCurrentKey, kvpvt, (BYTE*)piValue, sizeof( *piValue ) );
    }


    AssertSz( fFalse, "Unknown type %d\n", kvpvt );

    return ErrERRCheck( JET_errInvalidColumnType );
}

ERR CKVPStore::CKVPSCursor::ErrKVPSCursorUpdValue( __in INT iValue )
{
    ERR err = JET_errSuccess;

    Assert( m_eState == kvpscEnumerating );

    CKVPStore::KVPIValueType kvpvt = CKVPStore::kvpvtInvalidType;

    CallR( ErrKVPSCursorIGetType( &kvpvt ) );

    if( kvpvt == CKVPStore::kvpvtSchemaValueType ||
        kvpvt == CKVPStore::kvpvtIntValueType )
    {
        return m_pkvps->ErrKVPISetValue( eHasTrx, m_wszCurrentKey, kvpvt, (BYTE*)&iValue, sizeof( iValue ) );
    }


    AssertSz( fFalse, "Unknown type %d\n", kvpvt );

    return ErrERRCheck( JET_errInvalidColumnType );
}

ERR CKVPStore::CKVPSCursor::ErrKVPSCursorIGetType( CKVPStore::KVPIValueType * const pkvpvt )
{
    ULONG cbActual;
    *pkvpvt = CKVPStore::kvpvtInvalidType;
    const ERR err = ErrIsamRetrieveColumn( m_pkvps->m_ppibGlobalLock, m_pkvps->m_pfucbGlobalLock, m_pkvps->m_cidType, (void*)pkvpvt, 1, &cbActual, NO_GRBIT, NULL );
    Assert( cbActual == 1 );
    return err;
}


ERR ErrKVPStoreTestGetTable( const IFMP ifmpTest, const WCHAR * const wszTable, JET_SESID * const psesid, JET_DBID * const pdbid, JET_TABLEID * pcursor )
{
    JET_ERR err = JET_errSuccess;

    *psesid = JET_sesidNil;
    *pdbid = JET_dbidNil;
    *pcursor = JET_tableidNil;

    Call( JetBeginSessionW( (JET_INSTANCE)PinstFromIfmp( ifmpTest ), psesid, NULL, NULL ) );
    Call( JetOpenDatabaseW( *psesid, g_rgfmp[ifmpTest].WszDatabaseName(), NULL, pdbid, NO_GRBIT ) );
    Assert( ifmpTest == *pdbid );
    Call( JetOpenTableW( *psesid, *pdbid, wszTable, NULL, 0, NO_GRBIT, pcursor ) );

HandleError:

    return err;
}

ERR ErrKVPStoreTestGetRidOfTable( JET_SESID sesid, JET_DBID dbid, JET_TABLEID cursor )
{
    JET_ERR err = JET_errSuccess;

    if ( cursor != NULL && cursor != JET_tableidNil )
    {
        Call( JetCloseTable( sesid, cursor ) );
    }
    if ( dbid != NULL && dbid != JET_dbidNil )
    {
        Call( JetCloseDatabase( sesid, dbid, NO_GRBIT ) );
    }
    if ( sesid != NULL && sesid != JET_sesidNil )
    {
        Call( JetEndSession( sesid, NO_GRBIT ) );
    }

HandleError:

    return err;
}

ULONG CTestMatchingKeys( CKVPStore::CKVPSCursor * const pkvpscursor )
{
    ULONG c = 0;
    ERR err = 0;
    while( ( err = pkvpscursor->ErrKVPSCursorLocateNextKVP() ) >= JET_errSuccess )
    {
        c++;
    }

    return c;
}

ULONG CTestAccumulatedValues( CKVPStore::CKVPSCursor * const pkvpscursor )
{
    ULONG cAccum = 0;
    while( pkvpscursor->ErrKVPSCursorLocateNextKVP() >= JET_errSuccess )
    {
        INT cValue = 0;
        if ( pkvpscursor->ErrKVPSCursorGetValue( &cValue ) >= JET_errSuccess )
        {
            cAccum += cValue;
        }
    }
    return cAccum;
}


#ifdef ENABLE_JET_UNIT_TEST

JETUNITTESTDB( KVPStore, TestCreate, dwOpenDatabase )
{
    const WCHAR * wszMSysTestTableName = L"MSysTESTING_KVPStoreTestCreate";

    CKVPStore kvpstore( IfmpTest(), wszMSysTestTableName );
    CHECKCALLS( kvpstore.ErrKVPInitStore( 1 ) );


    JET_SESID sesid;
    JET_DBID dbid;
    JET_TABLEID cursor;
    CHECKCALLS( ErrKVPStoreTestGetTable( IfmpTest(), wszMSysTestTableName, &sesid, &dbid, &cursor ) );
    CHECKCALLS( ErrKVPStoreTestGetRidOfTable( sesid, dbid, cursor ) );

    kvpstore.KVPTermStore();
}

JETUNITTESTDB( KVPStore, TestGetNonExistantValue, dwOpenDatabase )
{
    CKVPStore kvpstore( IfmpTest(), L"MSysTESTING_KVPStoreTestGetNonExistantValue" );

    const WCHAR * wszOneName = L"JustAKeyName";

    CHECKCALLS( kvpstore.ErrKVPInitStore( 1 ) );

    INT iValue = 42;
    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 42 );

    kvpstore.KVPTermStore();
}

JETUNITTESTDB( KVPStore, AssertMaxKeySupported, dwOpenDatabase )
{
    CKVPStore kvpstore( IfmpTest(), L"MSysTESTING_KVPStoreTestMaxKeySupported" );

    CHECKCALLS( kvpstore.ErrKVPInitStore( 1 ) );

    kvpstore.CbKVPKeySize( L"12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890" );
    OnNonRTM( (void)FNegTestUnset( fInvalidUsage ) );

    OnNonRTM( CHECK( 0 == FNegTestSet( fInvalidUsage ) ) );
    kvpstore.CbKVPKeySize( L"AAA12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890" );
    OnNonRTM( (void)FNegTestUnset( fInvalidUsage ) );

    kvpstore.KVPTermStore();
}


JETUNITTESTDB( KVPStore, TestMaxKeySupported, dwOpenDatabase )
{
    CKVPStore kvpstore( IfmpTest(), L"MSysTESTING_KVPStoreTestMaxKeySupported" );

    WCHAR wszOneName[400];

    CHECKCALLS( kvpstore.ErrKVPInitStore( 1 ) );

    OSStrCbCopyW( wszOneName, sizeof( wszOneName ), L"This is at least a 60-byte string, so lets at least start here:" );

    INT cbKey = sizeof(WCHAR) * ( LOSStrLengthW( wszOneName ) + 1 );
    CHECK( cbKey == kvpstore.CbKVPKeySize( wszOneName ) );

    INT iValue;

    CHECKCALLS( kvpstore.ErrKVPSetValue( wszOneName, cbKey * 3 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == ( cbKey * 3 ) );

    ULONG cEntries = 1;

    while ( cbKey < sizeof( wszOneName )-2 )
    {
        OSStrCbAppendW( wszOneName, sizeof( wszOneName ), L"A" );
        cbKey += 2;
        Assert( (ULONG)cbKey == sizeof(WCHAR) * ( LOSStrLengthW( wszOneName ) + 1 ) );

        OnNonRTM( CHECK( 0 == FNegTestSet( fInvalidUsage ) ) );
        CHECK( cbKey == kvpstore.CbKVPKeySize( wszOneName ) );

        const ERR errSet = kvpstore.ErrKVPSetValue( wszOneName, ( cbKey * 3 ) );
        if ( errSet )
        {
            CHECK( JET_errKeyTruncated == errSet );
            CHECK( JET_errKeyTruncated == kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
            OnNonRTM( (void)FNegTestUnset( fInvalidUsage ) );
            break;
        }
        OnNonRTM( (void)FNegTestUnset( fInvalidUsage ) );
        cEntries++;
        const ERR errGet = kvpstore.ErrKVPGetValue( wszOneName, &iValue );
        CHECKCALLS( errGet );
        CHECK( iValue == ( cbKey * 3 ) );
    }

    CHECK( cbKey < ( sizeof( wszOneName ) - 10 ) );
    CHECK( 226 == cbKey );
    CHECK( cbKey == ( CKVPStore::cbKVPMaxKey + 2 ) );

    kvpstore.KVPTermStore();
}

JETUNITTESTDB( KVPStore, TestSetValue, dwOpenDatabase )
{
    CKVPStore kvpstore( IfmpTest(), L"MSysTESTING_KVPStoreTestSetValue" );

    const WCHAR * wszOneName = L"JustAKeyName";

    CHECKCALLS( kvpstore.ErrKVPInitStore( 1 ) );

    INT iValue = 42;

    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 42 );

    CHECKCALLS( kvpstore.ErrKVPSetValue( wszOneName, 2 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 2 );

    kvpstore.KVPTermStore();
}


JETUNITTESTDB( KVPStore, TestSetNulValue, dwOpenDatabase )
{
    CKVPStore kvpstore( IfmpTest(), L"MSysTESTING_KVPStoreTestSetNulValue" );

    CHECKCALLS( kvpstore.ErrKVPInitStore( 1 ) );



    kvpstore.KVPTermStore();
}


JETUNITTESTDB( KVPStore, TestModifyValue, dwOpenDatabase )
{
    CKVPStore kvpstore( IfmpTest(), L"MSysTESTING_KVPStoreTestModifyValue" );

    const WCHAR * wszOneName = L"JustAKeyName";

    CHECKCALLS( kvpstore.ErrKVPInitStore( 1 ) );

    INT iValue = 42;

    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 42 );

    CHECKCALLS( kvpstore.ErrKVPSetValue( wszOneName, 2 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 2 );

    CHECKCALLS( kvpstore.ErrKVPSetValue( wszOneName, 34 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 34 );

    kvpstore.KVPTermStore();
}

JETUNITTESTDB( KVPStore, TestSet2Value, dwOpenDatabase )
{
    CKVPStore kvpstore( IfmpTest(), L"MSysTESTING_KVPStoreTestSet2Value" );

    const WCHAR * wszOneName = L"JustAKeyName";

    CHECKCALLS( kvpstore.ErrKVPInitStore( 1 ) );

    INT iValue = 42;

    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 42 );

    CHECKCALLS( kvpstore.ErrKVPSetValue( wszOneName, 2 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 2 );

    CHECKCALLS( kvpstore.ErrKVPSetValue( wszOneName, 34 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 34 );

    CHECKCALLS( kvpstore.ErrKVPSetValue( L"TotallyDifferentKeyName", 25 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 34 );

    CHECKCALLS( kvpstore.ErrKVPGetValue( L"TotallyDifferentKeyName", &iValue ) );
    CHECK( iValue == 25 );

    kvpstore.KVPTermStore();
}


JETUNITTESTDB( KVPStore, TestSetAdjValue, dwOpenDatabase )
{
    CKVPStore kvpstore( IfmpTest(), L"MSysTESTING_KVPStoreTestSetAdjValue" );

    const WCHAR * wszOneName = L"JustAKeyName";

    CHECKCALLS( kvpstore.ErrKVPInitStore( 1 ) );

    INT iValue = 42;


    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 42 );

    CHECKCALLS( kvpstore.ErrKVPSetValue( wszOneName, 2 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 2 );

    CHECKCALLS( kvpstore.ErrKVPSetValue( wszOneName, 34 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 34 );

    CHECKCALLS( kvpstore.ErrKVPSetValue( L"TotallyDifferentKeyName", 25 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 34 );


    CHECKCALLS( kvpstore.ErrKVPAdjValue( wszOneName, -1 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 33 );


    CHECKCALLS( kvpstore.ErrKVPAdjValue( wszOneName, 2 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 35 );


    CHECKCALLS( kvpstore.ErrKVPAdjValue( L"Doesn'tExistYet", 1 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( L"Doesn'tExistYet" , &iValue ) );
    CHECK( iValue == 1 );


    CHECKCALLS( kvpstore.ErrKVPAdjValue( L"AlsoNonExistantCurrently", 1 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( L"AlsoNonExistantCurrently" , &iValue ) );
    CHECK( iValue == 1 );


    CHECKCALLS( kvpstore.ErrKVPAdjValue( L"TotallyDifferentKeyName", 25 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( L"TotallyDifferentKeyName", &iValue ) );
    CHECK( iValue == 50 );


    CHECKCALLS( kvpstore.ErrKVPSetValue( wszOneName, 20 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 20 );


    CHECKCALLS( kvpstore.ErrKVPAdjValue( wszOneName, 1 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 21 );

    kvpstore.KVPTermStore();
}

JETUNITTESTDB( KVPStore, TestSetAdjValueLimits, dwOpenDatabase )
{
    CKVPStore kvpstore( IfmpTest(), L"MSysTESTING_KVPStoreTestSetAdjValueLimits" );

    const WCHAR * wszOneName = L"JustAKeyName";

    CHECKCALLS( kvpstore.ErrKVPInitStore( 1 ) );

    INT iValue = 42;


    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 42 );

    CHECKCALLS( kvpstore.ErrKVPSetValue( wszOneName, 2 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 2 );


    CHECK( JET_errOutOfAutoincrementValues == kvpstore.ErrKVPAdjValue( wszOneName, ( 0x7FFFFFFF - 1 ) ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 2 );


    CHECKCALLS( kvpstore.ErrKVPAdjValue( wszOneName, ( 0x7FFFFFFF - 2 ) ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 0x7FFFFFFF );


    CHECK( JET_errOutOfAutoincrementValues == kvpstore.ErrKVPAdjValue( wszOneName, 1 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 0x7FFFFFFF );

    kvpstore.KVPTermStore();
}


JETUNITTESTDB( KVPStore, ReOpenTable, dwOpenDatabase )
{
    const WCHAR * wszMSysTestTableName = L"MSysTESTING_KVPStoreReOpenTable";
    const WCHAR * wszOneName = L"JustAKeyName";

    CKVPStore * pkvps = new CKVPStore( IfmpTest(), wszMSysTestTableName );
    CHECK( NULL != pkvps );

    CHECKCALLS( pkvps->ErrKVPInitStore( 1 ) );

    INT iValue = 42;

    CHECK( JET_errRecordNotFound == pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 42 );
    CHECKCALLS( pkvps->ErrKVPSetValue( wszOneName, 2 ) );
    CHECKCALLS( pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 2 );
    CHECKCALLS( pkvps->ErrKVPSetValue( wszOneName, 34 ) );
    CHECKCALLS( pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 34 );
    CHECKCALLS( pkvps->ErrKVPSetValue( L"TotallyDifferentKeyName", 25 ) );
    CHECKCALLS( pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 34 );
    CHECKCALLS( pkvps->ErrKVPGetValue( L"TotallyDifferentKeyName", &iValue ) );
    CHECK( iValue == 25 );

    pkvps->KVPTermStore();
    delete pkvps;

    pkvps = new CKVPStore( IfmpTest(), wszMSysTestTableName );
    CHECK( NULL != pkvps );

    CHECKCALLS( pkvps->ErrKVPInitStore( 1 ) );

    CHECKCALLS( pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 34 );
    CHECKCALLS( pkvps->ErrKVPGetValue( L"TotallyDifferentKeyName", &iValue ) );
    CHECK( iValue == 25 );

    iValue = 13;
    CHECKCALLS( pkvps->ErrKVPSetValue( wszOneName, iValue ) );
    CHECKCALLS( pkvps->ErrKVPGetValue( L"TotallyDifferentKeyName", &iValue ) );
    CHECK( iValue == 25 );
    CHECKCALLS( pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 13 );

    pkvps->KVPTermStore();
    delete pkvps;
}

#ifdef FUTURE_SUPPORT_MORE_VALUE_TYPES

JETUNITTESTDB( KVPStore, TestSetQwordValue, dwOpenDatabase )
{
    CKVPStore kvpstore( IfmpTest(), L"MSysTESTING_KVPStoreTestSetQwordValue" );

    const WCHAR * wszOneName = L"JustAKeyName";

    CHECKCALLS( kvpstore.ErrKVPInitStore( 1 ) );

    INT64 iValue = 42;

    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 42 );

    CHECKCALLS( kvpstore.ErrKVPSetValue( wszOneName, (INT64)2 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 2 );

    CHECKCALLS( kvpstore.ErrKVPSetValue( wszOneName, (INT64)34 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 34 );

    CHECKCALLS( kvpstore.ErrKVPSetValue( L"TotallyDifferentKeyName", (INT64)25 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 34 );

    CHECKCALLS( kvpstore.ErrKVPGetValue( L"TotallyDifferentKeyName", &iValue ) );
    CHECK( iValue == 25 );

    kvpstore.KVPTermStore();
}


JETUNITTESTDB( KVPStore, TestSetBlobValue, dwOpenDatabase )
{
    CKVPStore kvpstore( IfmpTest(), L"MSysTESTING_KVPStoreTestSetBlobValue" );

    const WCHAR * wszOneName = L"JustAKeyName";

    CHECKCALLS( kvpstore.ErrKVPInitStore( 1 ) );

    INT iValue = 42;

    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 42 );

    CHAR szSomeData[] = "MyMyThisIsLessThan30";
    CHAR szSomeResults[30];
    ULONG cbData = strlen( szSomeData ) + 1;

    CHECKCALLS( kvpstore.ErrKVPSetValue( wszOneName, (BYTE*)szSomeData, cbData ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, (BYTE*)szSomeResults, sizeof( szSomeResults ) ) );

    CHECK( 0 == memcmp( szSomeData, szSomeResults, cbData ) );

    kvpstore.KVPTermStore();
}


JETUNITTESTDB( KVPStore, UpgradeKVPInternalSchema, dwOpenDatabase )
{
    const WCHAR * wszMSysTestTableName = L"MSysTESTING_UpgradeKVPInternalSchema";
    const WCHAR * wszOneName = L"JustAKeyName";

    ErrEnableTestInjection( 43991, fTrue, JET_TestInjectConfigOverride, 100, JET_bitInjectionProbabilityPct );

    CKVPStore * pkvps = new CKVPStore( IfmpTest(), wszMSysTestTableName );
    CHECK( NULL != pkvps );

    CHECKCALLS( pkvps->ErrKVPInitStore( 1 ) );

    INT iValue = 42;

    CHECK( JET_errRecordNotFound == pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 42 );
    CHECKCALLS( pkvps->ErrKVPSetValue( wszOneName, 2 ) );
    CHECKCALLS( pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 2 );
    CHECKCALLS( pkvps->ErrKVPSetValue( wszOneName, 34 ) );
    CHECKCALLS( pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 34 );
    CHECKCALLS( pkvps->ErrKVPSetValue( L"TotallyDifferentKeyName", 25 ) );
    CHECKCALLS( pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 34 );
    CHECKCALLS( pkvps->ErrKVPGetValue( L"TotallyDifferentKeyName", &iValue ) );
    CHECK( iValue == 25 );

    pkvps->KVPTermStore();
    delete pkvps;

    ErrEnableTestInjection( 43991, fTrue, JET_TestInjectConfigOverride, 0, JET_bitInjectionProbabilityPct );

    pkvps = new CKVPStore( IfmpTest(), wszMSysTestTableName );
    CHECK( NULL != pkvps );

    CHECKCALLS( pkvps->ErrKVPInitStore( 1 ) );

    CHECKCALLS( pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 34 );
    CHECKCALLS( pkvps->ErrKVPGetValue( L"TotallyDifferentKeyName", &iValue ) );
    CHECK( iValue == 25 );

    CHAR szSomeData[] = "MyMyThisIsLessThan30";
    CHAR szSomeResults[30];
    ULONG cbData = strlen( szSomeData ) + 1;

    CHECKCALLS( pkvps->ErrKVPSetValue( L"A new name", (BYTE*)szSomeData, cbData ) );
    CHECKCALLS( pkvps->ErrKVPGetValue( L"A new name", (BYTE*)szSomeResults, sizeof( szSomeResults ) ) );

    CHECK( 0 == memcmp( szSomeData, szSomeResults, cbData ) );

    OnNonRTM( CHECK( 0 == FNegTestSet( fInvalidUsage ) ) );
    CHECK( JET_errInvalidColumnType == pkvps->ErrKVPSetValue( wszOneName, (BYTE*)szSomeData, cbData ) );
    OnNonRTM( (void)FNegTestUnset( fInvalidUsage ) );

    pkvps->KVPTermStore();
    delete pkvps;
}

#endif



JETUNITTESTDB( KVPStoreCursor, TestEnumAllUserValues, dwOpenDatabase )
{
    CKVPStore kvpstore( IfmpTest(), L"MSysTESTING_KVPStoreTestEnumAllUserValues" );

    CHECKCALLS( kvpstore.ErrKVPInitStore( 1 ) );

    INT iValue = 42;

    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( L"Element1", &iValue ) );
    CHECK( iValue == 42 );

    CHECKCALLS( kvpstore.ErrKVPSetValue( L"Element1", 2 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( L"Element1", &iValue ) );
    CHECK( iValue == 2 );

    CHECKCALLS( kvpstore.ErrKVPSetValue( L"Element2", 4 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( L"Element2", &iValue ) );
    CHECK( iValue == 4 );

    CHECKCALLS( kvpstore.ErrKVPSetValue( L"Element3", 8 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( L"Element3", &iValue ) );
    CHECK( iValue == 8 );

    CKVPStore::CKVPSCursor kvpscur( &kvpstore, CKVPStore::CKVPSCursor::WszUserWildcard() );

    ULONG c = 0;
    ERR err = 0;
    while( ( err = kvpscur.ErrKVPSCursorLocateNextKVP() ) >= JET_errSuccess )
    {
        c++;
        CHECKCALLS( kvpscur.ErrKVPSCursorGetValue( &iValue ) );
        CHECK( ( 1 << c ) == iValue );
    }

    CHECK( JET_errNoCurrentRecord == err );
    CHECK( c == 3 );

    kvpscur.KVPSCursorEnd();

    kvpstore.KVPTermStore();
}

JETUNITTESTDB( KVPStoreCursor, TestEnumEmptySetFails, dwOpenDatabase )
{
    CKVPStore kvpstore( IfmpTest(), L"MSysTESTING_KVPStoreTestEnumEmptySetFails" );

    CHECKCALLS( kvpstore.ErrKVPInitStore( 1 ) );

    INT iValue = 42;

    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( L"Element1", &iValue ) );
    CHECK( iValue == 42 );

    CKVPStore::CKVPSCursor kvpscur( &kvpstore, CKVPStore::CKVPSCursor::WszUserWildcard() );

    ULONG c = 0;
    ERR err = 0;
    while( ( err = kvpscur.ErrKVPSCursorLocateNextKVP() ) >= JET_errSuccess )
    {
        c++;
    }

    CHECK( JET_errNoCurrentRecord == err );
    CHECK( c == 0 );

    kvpscur.KVPSCursorEnd();


    CKVPStore::CKVPSCursor * pkvpscurAll = new CKVPStore::CKVPSCursor( &kvpstore, CKVPStore::CKVPSCursor::WszAllWildcard() );
    CHECK( NULL != pkvpscurAll );
    CHECK( 6 == CTestMatchingKeys( pkvpscurAll ) );
    delete pkvpscurAll;

    kvpstore.KVPTermStore();
}

JETUNITTESTDB( KVPStoreCursor, TestMatchingWildCards, dwOpenDatabase )
{
    CKVPStore kvpstore( IfmpTest(), L"MSysTESTING_KVPStoreTestMatchingWildCards" );

    CHECKCALLS( kvpstore.ErrKVPInitStore( 1 ) );

    INT iValue = 42;

    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( L"Ale Dal Monte Salary", &iValue ) );
    CHECK( iValue == 42 );

    CHECKCALLS( kvpstore.ErrKVPSetValue( L"Ale Dal Monte Salary", 112242 ) );
    CHECKCALLS( kvpstore.ErrKVPSetValue( L"Mati Kask Salary", 163021 ) );
    CHECKCALLS( kvpstore.ErrKVPSetValue( L"Uma Thurman Salary", 1144203 ) );
    CHECKCALLS( kvpstore.ErrKVPSetValue( L"Jens Mander Salary", 128525 ) );
    CHECKCALLS( kvpstore.ErrKVPSetValue( L"Marc Majerus Salary", 148024 ) );
    CHECKCALLS( kvpstore.ErrKVPSetValue( L"Marie Dubois Salary", 128624 ) );


    CKVPStore::CKVPSCursor * pkvpscurAllSalaries = new CKVPStore::CKVPSCursor( &kvpstore, L"*" );
    CHECK( NULL != pkvpscurAllSalaries );
    CHECK( 6 == CTestMatchingKeys( pkvpscurAllSalaries ) );
    delete pkvpscurAllSalaries;

    CKVPStore::CKVPSCursor * pkvpscurMaSalaries = new CKVPStore::CKVPSCursor( &kvpstore, L"Ma*" );
    CHECK( NULL != pkvpscurMaSalaries );
    CHECK( 3 == CTestMatchingKeys( pkvpscurMaSalaries ) );
    delete pkvpscurMaSalaries;

    (*(CPRINTFSTDOUT::PcprintfInstance()))("\nDumping table:\n");
    kvpstore.Dump( CPRINTFSTDOUT::PcprintfInstance(), fTrue );

    kvpstore.KVPTermStore();
}

JETUNITTESTDB( KVPStoreCursor, TestCountAllValues, dwOpenDatabase )
{
    CKVPStore kvpstore( IfmpTest(), L"MSysTESTING_KVPStoreTestCountAllValues" );

    CHECKCALLS( kvpstore.ErrKVPInitStore( 1 ) );

    INT iValue = 42;

    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( L"Element1", &iValue ) );
    CHECK( iValue == 42 );

    CHECKCALLS( kvpstore.ErrKVPSetValue( L"Element1", 2 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( L"Element1", &iValue ) );
    CHECK( iValue == 2 );
    CHECKCALLS( kvpstore.ErrKVPSetValue( L"Element2", 4 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( L"Element2", &iValue ) );
    CHECK( iValue == 4 );
    CHECKCALLS( kvpstore.ErrKVPSetValue( L"Element3", 8 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( L"Element3", &iValue ) );
    CHECK( iValue == 8 );
    CHECKCALLS( kvpstore.ErrKVPSetValue( L"Element4", 16 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( L"Element4", &iValue ) );
    CHECK( iValue == 16 );


    CKVPStore::CKVPSCursor * pkvpscurUser = new CKVPStore::CKVPSCursor( &kvpstore, CKVPStore::CKVPSCursor::WszUserWildcard() );
    CHECK( NULL != pkvpscurUser );
    CHECK( 4 == CTestMatchingKeys( pkvpscurUser ) );
    delete pkvpscurUser;


    CKVPStore::CKVPSCursor * pkvpscurInternal = new CKVPStore::CKVPSCursor( &kvpstore, CKVPStore::CKVPSCursor::WszInternalWildcard() );
    CHECK( NULL != pkvpscurInternal );
    CHECK( 6 == CTestMatchingKeys( pkvpscurInternal ) );
    delete pkvpscurInternal;


    CKVPStore::CKVPSCursor * pkvpscurAll = new CKVPStore::CKVPSCursor( &kvpstore, CKVPStore::CKVPSCursor::WszAllWildcard() );
    CHECK( NULL != pkvpscurAll );
    CHECK( 10 == CTestMatchingKeys( pkvpscurAll ) );
    delete pkvpscurAll;

    kvpstore.KVPTermStore();
}

JETUNITTESTDB( KVPStoreCursor, TestEnumAndUpdValues, dwOpenDatabase )
{
    CKVPStore kvpstore( IfmpTest(), L"MSysTESTING_KVPStoreTestEnumAndUpdValues" );

    CHECKCALLS( kvpstore.ErrKVPInitStore( 1 ) );

    INT iValue = 42;

    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( L"Element1", &iValue ) );
    CHECK( iValue == 42 );

    CHECKCALLS( kvpstore.ErrKVPSetValue( L"Element1", 2 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( L"Element1", &iValue ) );
    CHECK( iValue == 2 );

    CHECKCALLS( kvpstore.ErrKVPSetValue( L"Element2", 4 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( L"Element2", &iValue ) );
    CHECK( iValue == 4 );

    CHECKCALLS( kvpstore.ErrKVPSetValue( L"Element3", 8 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( L"Element3", &iValue ) );
    CHECK( iValue == 8 );

    CKVPStore::CKVPSCursor kvpscur( &kvpstore, CKVPStore::CKVPSCursor::WszUserWildcard() );

    ULONG c = 0;
    ERR err = 0;
    while( ( err = kvpscur.ErrKVPSCursorLocateNextKVP() ) >= JET_errSuccess )
    {
        c++;
        CHECKCALLS( kvpscur.ErrKVPSCursorGetValue( &iValue ) );
        CHECK( ( 1 << c ) == iValue );
        if ( c == 2 )
        {
            CHECKCALLS( kvpscur.ErrKVPSCursorUpdValue( iValue + 1 ) );
            CHECKCALLS( kvpscur.ErrKVPSCursorGetValue( &iValue ) );
            CHECK( ( 1 << c ) + 1 == iValue );
        }
    }

    CHECK( JET_errNoCurrentRecord == err );
    CHECK( c == 3 );

    kvpscur.KVPSCursorEnd();

    CHECKCALLS( kvpstore.ErrKVPGetValue( L"Element2", &iValue ) );
    CHECK( iValue == 5 );

    kvpstore.KVPTermStore();
}


JETUNITTESTDB( KVPStore, TestUserMajorVersionIncompat, dwOpenDatabase )
{
    const WCHAR * wszMSysTestTableName = L"MSysTESTING_KVPStoreTestUserMajorVersionIncompat";
    const WCHAR * wszOneName = L"JustAKeyName";

    CKVPStore * pkvps = new CKVPStore( IfmpTest(), wszMSysTestTableName );
    CHECK( NULL != pkvps );

    CHECKCALLS( pkvps->ErrKVPInitStore( 1 ) );

    DWORD dwMinor = 42;
    DWORD dwUpdate = 42;

    CHECKCALLS( pkvps->ErrKVPGetUserVersion( &dwMinor, &dwUpdate ) );
    CHECK( 0 == dwMinor );
    CHECK( 0 == dwUpdate );

    INT iValue = 42;

    CHECK( JET_errRecordNotFound == pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 42 );
    CHECKCALLS( pkvps->ErrKVPSetValue( wszOneName, 2 ) );
    CHECKCALLS( pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 2 );

    pkvps->KVPTermStore();
    delete pkvps;

    pkvps = new CKVPStore( IfmpTest(), wszMSysTestTableName );
    CHECK( NULL != pkvps );


    CHECK( JET_errInvalidDatabaseVersion == pkvps->ErrKVPInitStore( 2 ) );

    delete pkvps;
}


JETUNITTESTDB( KVPStore, TestUserVersionUpgrade, dwOpenDatabase )
{
    const WCHAR * wszMSysTestTableName = L"MSysTESTING_KVPStoreTestUserVersionUpgrade";
    const WCHAR * wszOneName = L"JustAKeyName";

    CKVPStore * pkvps = new CKVPStore( IfmpTest(), wszMSysTestTableName );
    CHECK( NULL != pkvps );

    CHECKCALLS( pkvps->ErrKVPInitStore( 1 ) );

    DWORD dwMinor = 42;
    DWORD dwUpdate = 42;

    CHECKCALLS( pkvps->ErrKVPGetUserVersion( &dwMinor, &dwUpdate ) );
    CHECK( 0 == dwMinor );
    CHECK( 0 == dwUpdate );

    INT iValue = 42;

    CHECK( JET_errRecordNotFound == pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 42 );
    CHECKCALLS( pkvps->ErrKVPSetValue( wszOneName, 2 ) );
    CHECKCALLS( pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 2 );

    pkvps->KVPTermStore();
    delete pkvps;

    pkvps = new CKVPStore( IfmpTest(), wszMSysTestTableName );
    CHECK( NULL != pkvps );

    CHECKCALLS( pkvps->ErrKVPInitStore( 1 ) );

    CHECKCALLS( pkvps->ErrKVPGetUserVersion( &dwMinor, &dwUpdate ) );
    CHECK( 0 == dwMinor );
    CHECK( 0 == dwUpdate );

    iValue = 42;
    CHECKCALLS( pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 2 );

    CHECKCALLS( pkvps->ErrKVPSetUserVersion( 2, 3 ) );

    CHECKCALLS( pkvps->ErrKVPGetUserVersion( &dwMinor, &dwUpdate ) );
    CHECK( 2 == dwMinor );
    CHECK( 3 == dwUpdate );

    pkvps->KVPTermStore();
    delete pkvps;

    pkvps = new CKVPStore( IfmpTest(), wszMSysTestTableName );
    CHECK( NULL != pkvps );

    CHECKCALLS( pkvps->ErrKVPInitStore( 1 ) );

    CHECKCALLS( pkvps->ErrKVPGetUserVersion( &dwMinor, &dwUpdate ) );
    CHECK( 2 == dwMinor );
    CHECK( 3 == dwUpdate );

    pkvps->KVPTermStore();
    delete pkvps;
}



JETUNITTESTDB( KVPStoreForMSysLocales, ExpectedLCIDTableUsage, dwOpenDatabase )
{
    CKVPStore * pkvps = new CKVPStore( IfmpTest(), L"MSysTESTING_MSysLocales" );

    const DWORD dwMSysLocalesMajorVersion = 1;

    CHECKCALLS( pkvps->ErrKVPInitStore( dwMSysLocalesMajorVersion ) );

    WCHAR wszLcidVerKey[60];


    QWORD qwEnglishSortVersion;
    QWORD qwGermanSortVersion;
    QWORD qwItalianSortVersion;
    WCHAR wszLocaleName[NORM_LOCALE_NAME_MAX_LENGTH];

    CHECKCALLS( ErrNORMLcidToLocale( 1033, wszLocaleName, _countof( wszLocaleName ) ) );
    CHECKCALLS( ErrNORMGetSortVersion( wszLocaleName, &qwEnglishSortVersion, NULL ) );
    CHECKCALLS( ErrNORMLcidToLocale( 1031, wszLocaleName, _countof( wszLocaleName ) ) );
    CHECKCALLS( ErrNORMGetSortVersion( wszLocaleName, &qwGermanSortVersion, NULL ) );
    CHECKCALLS( ErrNORMLcidToLocale( 1040, wszLocaleName, _countof( wszLocaleName ) ) );
    CHECKCALLS( ErrNORMGetSortVersion( wszLocaleName, &qwItalianSortVersion, NULL ) );

    OSStrCbFormatW( wszLcidVerKey, sizeof( wszLcidVerKey ), L"LCID=%d,Ver=%I64x", 1031, qwGermanSortVersion );
    CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, +1 ) );

    OSStrCbFormatW( wszLcidVerKey, sizeof( wszLcidVerKey ), L"LCID=%d,Ver=%I64x", 1033, qwEnglishSortVersion );
    CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, +1 ) );
    CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, +1 ) );
    CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, -1 ) );
    CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, +1 ) );
    CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, +1 ) );
    CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, +1 ) );

    OSStrCbFormatW( wszLcidVerKey, sizeof( wszLcidVerKey ), L"LCID=%d,Ver=%I64x", 1040, qwItalianSortVersion );
    CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, +1 ) );
    CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, +1 ) );

    OSStrCbFormatW( wszLcidVerKey, sizeof( wszLcidVerKey ), L"LCID=%d,Ver=%I64x", 1033, qwEnglishSortVersion );
    CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, +1 ) );


    OSStrCbFormatW( wszLcidVerKey, sizeof( wszLcidVerKey ), L"LCID=%d,Ver=%I64x", 1033, qwEnglishSortVersion + 4 );
    CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, +1 ) );


    OSStrCbFormatW( wszLcidVerKey, sizeof( wszLcidVerKey ), L"LCID=%d,*", 1033 );
    CKVPStore::CKVPSCursor * pkvpscurAllEnglishIndices = new CKVPStore::CKVPSCursor( pkvps, wszLcidVerKey );
    CHECK( NULL != pkvpscurAllEnglishIndices );

    CHECK( 2 == CTestMatchingKeys( pkvpscurAllEnglishIndices ) );
    CHECKCALLS( pkvpscurAllEnglishIndices->ErrKVPSCursorReset() );
    ULONG cEnglishIndices = 0;

    while( pkvpscurAllEnglishIndices->ErrKVPSCursorLocateNextKVP() >= JET_errSuccess )
    {
        INT cEnglishIndicesThisVer = 0;
        if ( pkvpscurAllEnglishIndices->ErrKVPSCursorGetValue( &cEnglishIndicesThisVer ) >= JET_errSuccess )
        {
            cEnglishIndices += cEnglishIndicesThisVer;
        }
    }
    CHECK( 6 == cEnglishIndices );

    CHECKCALLS( pkvpscurAllEnglishIndices->ErrKVPSCursorReset() );
    CHECK( 6 == CTestAccumulatedValues( pkvpscurAllEnglishIndices ) );

    delete pkvpscurAllEnglishIndices;


    CKVPStore::CKVPSCursor * pkvpscurAllIndices = new CKVPStore::CKVPSCursor( pkvps, L"LCID=*" );
    CHECK( NULL != pkvpscurAllIndices );

    CHECK( 4 == CTestMatchingKeys( pkvpscurAllIndices ) );
    CHECKCALLS( pkvpscurAllIndices->ErrKVPSCursorReset() );
    CHECK( 9 == CTestAccumulatedValues( pkvpscurAllIndices ) );

    wprintf( L"Processing LCIDs Tracked:\n" );
    CHECKCALLS( pkvpscurAllIndices->ErrKVPSCursorReset() );
    while( pkvpscurAllIndices->ErrKVPSCursorLocateNextKVP() >= JET_errSuccess )
    {
        const WCHAR * pwszLCID = wcschr( pkvpscurAllIndices->WszKVPSCursorCurrKey(), L'=' );
        if ( pwszLCID == NULL ||
             *pwszLCID == L'\0' ||
             *(pwszLCID+1) == L'\0' )
        {
            CHECK( !"This data store is corrupted couldn't find the LCID= delimiter." );
            break;
        }

        pwszLCID++;
        WCHAR * pwszVer = NULL;
        DWORD lcid = wcstol( pwszLCID, &pwszVer, 10 );
        if ( lcid == 0 )
        {
            CHECK( !"This data store is corrupted, the LCID = 0!" );
            break;
        }

        pwszVer = wcschr( pwszVer, L'=' );
        if ( pwszVer == NULL ||
             *pwszVer == L'\0' ||
             *(pwszVer+1) == L'\0' )
        {
            CHECK( !"This data store is corrupted couldn't find the Ver= delimiter." );
            break;
        }

        pwszVer++;
        WCHAR * pwszNull = NULL;
        QWORD qwSortedVersion = _wcstoui64( pwszVer, &pwszNull, 16 );
        Expected( *pwszNull == L'\0' );

        INT cCountIndices = 0;
        if ( pkvpscurAllIndices->ErrKVPSCursorGetValue( &cCountIndices ) < JET_errSuccess )
        {
            CHECK( !"This data store is corrupted." );
            break;
        }

        CHECKCALLS( ErrNORMLcidToLocale( lcid, wszLocaleName, _countof( wszLocaleName ) ) );

        wprintf( L"   Produced LCID = %d (0x%x), LocaleName= %s, qwSortVersion = 0x%I64x, Count = %d", lcid, lcid, wszLocaleName, qwSortedVersion, cCountIndices );

        QWORD qwSortedVersionOS;
        CHECKCALLS( ErrNORMGetSortVersion( wszLocaleName, &qwSortedVersionOS, NULL ) );

        if ( qwSortedVersion == qwSortedVersionOS )
        {
            wprintf( L" (Index Current)\n" );
        }
        else
        {
            wprintf( L" (Index Out Of Date: OS Sort Version = 0x%I64x)\n", qwSortedVersionOS );
        }
    }

    CHECKCALLS( pkvpscurAllIndices->ErrKVPSCursorReset() );
    delete pkvpscurAllIndices;

    pkvps->Dump( CPRINTFSTDOUT::PcprintfInstance(), fTrue );

    pkvps->KVPTermStore();
    delete pkvps;
}

JETUNITTESTDB( KVPStoreForMSysLocales, LCIDRecordsPerPage, dwOpenDatabase )
{
    WCHAR * wszMSysTestTableName = L"MSysTESTING_LCIDRecordsPerPage";
    CKVPStore * pkvps = new CKVPStore( IfmpTest(), wszMSysTestTableName );

    CHECKCALLS( pkvps->ErrKVPInitStore( 1 ) );

    WCHAR wszLcidVerKey[60];


    JET_SESID sesid;
    JET_DBID dbid;
    JET_TABLEID cursor;
    CHECKCALLS( ErrKVPStoreTestGetTable( IfmpTest(), wszMSysTestTableName, &sesid, &dbid, &cursor ) );

    CPG cpgOwned;
    CPG cpgAvail;

    CHECKCALLS( JetGetTableInfoW( sesid, cursor, &cpgOwned, sizeof( cpgOwned ), JET_TblInfoSpaceOwned ) );
    CHECKCALLS( JetGetTableInfoW( sesid, cursor, &cpgAvail, sizeof( cpgAvail ), JET_TblInfoSpaceAvailable ) );

    CHECK( cpgOwned == 1 );
    CHECK( cpgAvail == 0 );


    ULONG i;
    for ( i = 0; i < INT_MAX; i++ )
    {
        LCID lcid = ( ( rand() % 256 + 1 ) << 24 ) + rand();
        OSStrCbFormatW( wszLcidVerKey, sizeof( wszLcidVerKey ), L"LCID=%d,Ver=%I64x", lcid, 0xFE24012C01AFEFEF);
        CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, +1 ) );

        CPG cpgOwnedNew = 0;
        CHECKCALLS( JetGetTableInfoW( sesid, cursor, &cpgOwnedNew, sizeof( cpgOwnedNew ), JET_TblInfoSpaceOwned ) );
        if ( cpgOwnedNew != cpgOwned )
        {
            wprintf(L"Hit after : %d th add ... cpgOwnedNew = %d\n", i, cpgOwnedNew );
            break;
        }
    }
    CHECK( i > 10 );

    pkvps->Dump( CPRINTFSTDOUT::PcprintfInstance(), fTrue );

    CHECKCALLS( ErrKVPStoreTestGetRidOfTable( sesid, dbid, cursor ) );

    pkvps->KVPTermStore();
    delete pkvps;
}

#endif

