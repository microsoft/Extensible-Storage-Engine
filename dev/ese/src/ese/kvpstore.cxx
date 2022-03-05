// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

//  You might think you want dbidMax, but you could be wrong.  Probably you've confused JET_DBIDs with DBIDs, and you want g_ifmpMax.
#define dbidMax     dbidMax_YOU_MIGHT_THINK_YOU_WANT_DBIDMAX_BUT_YOU_COULD_BE_WRONG


//  ================================================================
//  .Schema Versions
//  ================================================================

//  keys for the schema versions

// Internal version is owned by the KVP class and describes the capabilities of the class.
PERSISTED static const WCHAR *  p_wszSchemaInternalMajorVersion     = L".Schema\\Internal\\Major";
PERSISTED static const WCHAR *  p_wszSchemaInternalMinorVersion     = L".Schema\\Internal\\Minor";
PERSISTED static const WCHAR *  p_wszSchemaInternalUpdateVersion    = L".Schema\\Internal\\Update";
// External version is owned by the caller and doesn't affect the actual KVP capabilities.
PERSISTED static const WCHAR *  p_wszSchemaExternalMajorVersion     = L".Schema\\External\\Major";
PERSISTED static const WCHAR *  p_wszSchemaExternalMinorVersion     = L".Schema\\External\\Minor";
PERSISTED static const WCHAR *  p_wszSchemaExternalUpdateVersion    = L".Schema\\External\\Update";

PERSISTED static const PCSTR p_szKeyColumn = "Key";
PERSISTED static const PCSTR p_szTypeColumn = "Type";

//  ================================================================
//  Basic Initialization / Term
//  ================================================================

CKVPStore::CKVPStore( IFMP ifmp, const WCHAR * const wszTableName )
    : CZeroInit( sizeof( *this ) ),
    m_critGlobalTableLock( CLockBasicInfo( CSyncBasicInfo( szKVPStore ), rankKVPStore, 0 ) ),
    m_ifmp ( ifmp )
{
    OSStrCbCopyW( p_wszTableName, sizeof( p_wszTableName ), wszTableName );

    //  the magic character that separates all internal keys from user keys ...
    //
    Assert( p_wszSchemaInternalMajorVersion[0] == s_wchSecret );

    Assert( 0 == kvpvtInvalidType );

    ULONG ulInternalMajorVersionLast = 0;
    ULONG ulInternalMinorVersionLast = 0;
    ULONG ulInternalUpdateVersionLast = 0;
    
    for( KVPIValueType iType = kvpvtInvalidType; iType < kvpvtMaxValueType; iType = KVPIValueType( iType + 1 ) )
    {
        Assert( m_rgDataTypeInfo[iType].m_type == iType );
        Assert( m_rgDataTypeInfo[iType].m_cid == JET_columnidNil );
        Assert( m_rgDataTypeInfo[iType].m_szName != NULL );

        // Types should be in version order and version components never decrease.
        //  The next non-major version after 1.0.2 could be either 1.0.3, 1.1.2, or 1.1.3,
        //  but not 1.1.0.
        //
        Assert( m_rgDataTypeInfo[iType].m_ulMajor >= ulInternalMajorVersionLast );
        Assert( m_rgDataTypeInfo[iType].m_ulMinor >= ulInternalMinorVersionLast );
        Assert( m_rgDataTypeInfo[iType].m_ulUpdate >= ulInternalUpdateVersionLast );
        ulInternalMajorVersionLast = m_rgDataTypeInfo[iType].m_ulMajor;
        ulInternalMinorVersionLast = m_rgDataTypeInfo[iType].m_ulMinor;
        ulInternalUpdateVersionLast = m_rgDataTypeInfo[iType].m_ulUpdate;
    }

    m_cidKey = JET_columnidNil;
    m_cidType = JET_columnidNil;

    Assert( m_critGlobalTableLock.FNotOwner() );
}

CKVPStore::~CKVPStore( )
{
    //  leaking?
    AssertRTL( NULL == m_pfucbGlobalLock );
    AssertRTL( NULL == m_ppibGlobalLock );
}

ERR CKVPStore::ErrKVPIInitIBootstrapCriticalCOLIDs()
{
    ERR err = JET_errSuccess;

    Assert( m_critGlobalTableLock.FOwner() );

    JET_COLUMNDEF jcd;
    jcd.cbStruct = sizeof( jcd );

    Call( ErrIsamGetTableColumnInfo(
              (JET_SESID)m_ppibGlobalLock,
              (JET_VTID)m_pfucbGlobalLock,
              p_szKeyColumn,
              NULL,
              &jcd,
              sizeof( jcd ),
              JET_ColInfo,
              fFalse ) );
    Assert( m_cidKey == JET_columnidNil ); /* was uninit */
    m_cidKey = jcd.columnid;
    
    Call( ErrIsamGetTableColumnInfo(
              (JET_SESID)m_ppibGlobalLock,
              (JET_VTID)m_pfucbGlobalLock,
              p_szTypeColumn,
              NULL,
              &jcd,
              sizeof( jcd ),
              JET_ColInfo,
              fFalse ) );
    Assert( m_cidType == JET_columnidNil ); /* was uninit */ 
    m_cidType = jcd.columnid;

    Call( ErrIsamGetTableColumnInfo(
              (JET_SESID)m_ppibGlobalLock,
              (JET_VTID)m_pfucbGlobalLock,
              m_rgDataTypeInfo[kvpvtSchemaValueType].m_szName,
              NULL,
              &jcd,
              sizeof( jcd ),
              JET_ColInfo,
              fFalse ) );
    Assert( m_rgDataTypeInfo[kvpvtSchemaValueType].m_cid == JET_columnidNil ); /* was uninit */
    Assert( m_rgDataTypeInfo[kvpvtSchemaValueType].m_cbDataCol == sizeof( LONG ) );
    m_rgDataTypeInfo[kvpvtSchemaValueType].m_cid = jcd.columnid;

HandleError:

    return err;
}

ERR CKVPStore::ErrKVPIInitICreateTable( PIB * ppib, const IFMP ifmp, const CHAR * const szTableName, const ULONG ulMajorVersionExpected )
{
    ERR err = JET_errSuccess;

    Assert( !g_fRepair );
    Assert( ppibNil != ppib );
    Assert( ifmpNil != ifmp );
    Assert( NULL == m_pfucbGlobalLock );

    BOOL                fInTransaction  = fFalse;

    //  This initial table contains all the most critical elements of the KVPStore:
    //      The key - which is the primary / clustered index.
    //      The type - as it must not be NULL.
    //      The int ValueType - as we may want to escrow update it later.
    //
    //  Upgrade paths add new columntypes
    ULONG iValueDefault = 0;
    JET_COLUMNCREATE_A  rgcolumncreateKVPTable[] = {
        {
            sizeof(JET_COLUMNCREATE_A),
            const_cast<PSTR>(p_szKeyColumn), // "Key"
            JET_coltypBinary,
            0,
            JET_bitColumnVariable,
            NULL,
            0,
            0,
            0,
            JET_errSuccess
        },
        {
            sizeof(JET_COLUMNCREATE_A),
            const_cast<PSTR>(p_szTypeColumn), // "Type" ,
            JET_coltypUnsignedByte,
            0,
            JET_bitColumnFixed|JET_bitColumnNotNULL,
            NULL,
            0,
            0,
            0,
            JET_errSuccess
        },
        {
            sizeof(JET_COLUMNCREATE_A),
            const_cast<PSTR>(m_rgDataTypeInfo[kvpvtSchemaValueType].m_szName), //  "iValue",
            m_rgDataTypeInfo[kvpvtSchemaValueType].m_coltyp,
            0,
            m_rgDataTypeInfo[kvpvtSchemaValueType].m_grbit,
            &iValueDefault,
            sizeof(iValueDefault),
            0,
            0,
            JET_errSuccess
        },
    };

    static const CHAR szMSysKVPTableIndex[]     = "KeyPrimary";
    static const CHAR szMSysKVPTableIndexKey[]  = "+Key\0";

    JET_INDEXCREATE3_A  rgindexcreateKVPTable[] = {
        {
            sizeof( JET_INDEXCREATE3_A ),                   // size of this structure
            const_cast<char *>( szMSysKVPTableIndex ),      // index name
            const_cast<char *>( szMSysKVPTableIndexKey ),   // index key
            sizeof( szMSysKVPTableIndexKey ),               // length of key
            JET_bitIndexPrimary|JET_bitIndexDisallowTruncation,         // index options
            92,                                             // index density
            0,                                              // lcid for the index
            0,                                              // maximum length of variable length columns in index key
            NULL,                                           // pointer to conditional column structure
            0,                                              // number of conditional columns
            JET_errSuccess,                                 // returned error code,
            255,                                            // maximum key size
            NULL                                            // space hints
        },
    };

    JET_TABLECREATE5_A  tablecreateKVPTable = {
        sizeof( JET_TABLECREATE5_A ),       // size of this structure
        const_cast<char *>( szTableName ),  // name of table
        NULL,                               // name of base table
        1,                                  // initial pages
        92,                                 // density
        rgcolumncreateKVPTable,             // columns to create
        _countof(rgcolumncreateKVPTable),   // number of columns to create
        rgindexcreateKVPTable,              // array of index creation info
        _countof(rgindexcreateKVPTable),    // number of indexes to create
        NULL,                               // callback to use for this table
        JET_cbtypNull,                      // when the callback should be called
        JET_bitTableCreateSystemTable,      // grbit
        NULL,                               // Sequential index space hints.
        NULL,                               // LV index space hints
        0,                                  // cbSeparateLV threshold
        0,                                  // cbLVChunkMax
        JET_TABLEID( pfucbNil ),            // returned tableid
        0,                                  // returned count of objects created
    };

    const JET_DBID  jdbid   = (JET_DBID)ifmp;

    Assert( !fInTransaction );
    Call( ErrDIRBeginTransaction( ppib, 53719, NO_GRBIT ) );
    fInTransaction = fTrue;

    Call( ErrFILECreateTable( ppib, jdbid, &tablecreateKVPTable ) );

    //  temporarily use the create FUCB

    m_pfucbGlobalLock = (FUCB*)tablecreateKVPTable.tableid;

    //  bootstrap the current key values

    Call( ErrKVPIInitIBootstrapCriticalCOLIDs() );

    //  validate bootstrapping proceeded strappingly

    Assert( m_cidKey == rgcolumncreateKVPTable[0].columnid );
    Assert( m_cidType == rgcolumncreateKVPTable[1].columnid );
    Assert( m_rgDataTypeInfo[kvpvtSchemaValueType].m_type == kvpvtSchemaValueType );
    Assert( m_rgDataTypeInfo[kvpvtSchemaValueType].m_cid = rgcolumncreateKVPTable[2].columnid );

    //  first time we've used this table, initialize the schema elements for Internal version numbers.
    //
    ULONG ulInitialVersion = 0;   // We start with the first-release values to handle efv upgrade restrictions.
    Call( ErrKVPISetValue( eHasTrx, p_wszSchemaInternalMajorVersion, kvpvtSchemaValueType, (BYTE*)&p_ulInternalMajorVersion, sizeof( p_ulInternalMajorVersion ) ) );
    Call( ErrKVPISetValue( eHasTrx, p_wszSchemaInternalMinorVersion, kvpvtSchemaValueType, (BYTE*)&ulInitialVersion, sizeof( ulInitialVersion ) ) );
    Call( ErrKVPISetValue( eHasTrx, p_wszSchemaInternalUpdateVersion, kvpvtSchemaValueType, (BYTE*)&ulInitialVersion, sizeof( ulInitialVersion ) ) );


    //  we set all the External version to X.0.0, where X is the user version requested.
    //
    ULONG ulInitialUserSubVersions = 0;
    Call( ErrKVPISetValue( eHasTrx, p_wszSchemaExternalMajorVersion, kvpvtSchemaValueType, (BYTE*)&ulMajorVersionExpected, sizeof( ulMajorVersionExpected ) ) );
    Call( ErrKVPISetValue( eHasTrx, p_wszSchemaExternalMinorVersion, kvpvtSchemaValueType, (BYTE*)&ulInitialUserSubVersions, sizeof( ulInitialUserSubVersions ) ) );
    Call( ErrKVPISetValue( eHasTrx, p_wszSchemaExternalUpdateVersion, kvpvtSchemaValueType, (BYTE*)&ulInitialUserSubVersions, sizeof( ulInitialUserSubVersions ) ) );

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

// Add a new type to the schema for the KVP table as an update.
ERR CKVPStore::ErrKVPIAddValueTypeToSchema( const KVPIValueType kvpvt )
{
    ERR err = JET_errSuccess;
    BOOL fInTransaction = fFalse;
    JET_COLUMNDEF jcd = { 0 };

    Assert( m_rgDataTypeInfo[kvpvt].m_ulUpdate <= p_ulInternalUpdateVersion );    // make sure client code is consistent with our upgrade version

    Assert( !fInTransaction );
    Call( ErrDIRBeginTransaction( m_ppibGlobalLock, 41431, NO_GRBIT ) );
    fInTransaction = fTrue;

    jcd.cbStruct = sizeof( jcd );
    //jcd.columnid =
    jcd.coltyp = m_rgDataTypeInfo[kvpvt].m_coltyp;
    //jcd.wCountry =
    //jcd.langid =
    //jcd.cp =
    //jcd.wCollate =        /* Must be 0 */
    //jcd.cbMax =
    jcd.grbit = m_rgDataTypeInfo[kvpvt].m_grbit;

    JET_COLUMNID jcid;

    Call( ErrIsamAddColumn(
              m_ppibGlobalLock,
              m_pfucbGlobalLock,
              m_rgDataTypeInfo[kvpvt].m_szName,
              &jcd,
              NULL,
              0,
              &jcid ) );

    Call( ErrKVPISetValue(
              eHasTrx,
              p_wszSchemaInternalUpdateVersion,
              kvpvtSchemaValueType,
              (BYTE*)&(m_rgDataTypeInfo[kvpvt].m_ulUpdate),
              sizeof( m_rgDataTypeInfo[kvpvt].m_ulUpdate ) ) );

    Assert( fInTransaction );
    Call( ErrDIRCommitTransaction( m_ppibGlobalLock, NO_GRBIT ) );
    fInTransaction = fFalse;

    OSTrace(
        JET_tracetagUpgrade,
        OSFormat(
            "KVPStore( %ws ) updated: %u -> %u\n",
            p_wszTableName,
            m_rgDataTypeInfo[kvpvt].m_ulUpdate - 1,
            m_rgDataTypeInfo[kvpvt].m_ulUpdate ) );

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

ERR CKVPStore::ErrKVPIInitIUpgradeTable( const IFMP ifmp, bool fReadOnly )
{
    ERR err = JET_errSuccess;

    Assert( m_critGlobalTableLock.FOwner() );

    ULONG ulKeyValueTableMajorVersion;
    ULONG ulKeyValueTableMinorVersion;
    ULONG ulKeyValueTableUpdateVersion;
    //  Technically not in a trx ... but has global lock ... this is OK, during init ... for reading only ...
    Call( ErrKVPIGetValue( eHasTrx, p_wszSchemaInternalMajorVersion, kvpvtSchemaValueType, (BYTE*)&ulKeyValueTableMajorVersion, sizeof( ulKeyValueTableMajorVersion ) ) );
    Call( ErrKVPIGetValue( eHasTrx, p_wszSchemaInternalMinorVersion, kvpvtSchemaValueType, (BYTE*)&ulKeyValueTableMinorVersion, sizeof( ulKeyValueTableMinorVersion ) ) );
    Call( ErrKVPIGetValue( eHasTrx, p_wszSchemaInternalUpdateVersion, kvpvtSchemaValueType, (BYTE*)&ulKeyValueTableUpdateVersion, sizeof( ulKeyValueTableUpdateVersion ) ) );

    BOOL fVersionMismatch = fFalse;

    // p_ulInternalMinorVersion, p_ulInternalMajorVersion, and p_ulInternalUpdateVersion are baked into the binary;
    // they represent the maximum the binary is capable of supporting.
    // However, the actual upgrade is limited by an efv value.  So, figure out how far we really need to upgrade.
    if ( (BOOL)UlConfigOverrideInjection( 43991, fFalse ) )
    {
        // A test scenario.  Hold the version to the initial release values.
        m_ulInternalMajorVersion = 1;
        m_ulInternalMinorVersion = 0;
        m_ulInternalUpdateVersion = 0;
    }
    else if ( fReadOnly )
    {
        // Since we're read only, our target is just the stored values. But we still come through
        // here to sanity check the stored values against the this.p_XXX values.
        m_ulInternalMajorVersion = ulKeyValueTableMajorVersion;
        m_ulInternalMinorVersion = ulKeyValueTableMinorVersion;
        m_ulInternalUpdateVersion = ulKeyValueTableUpdateVersion;
    }
    else if ( g_rgfmp[ifmp].FEfvSupported( JET_efvKVPStoreV2 ) )
    {
        m_ulInternalMajorVersion = 1;
        m_ulInternalMinorVersion = 0;
        m_ulInternalUpdateVersion = 2;
    }
    else
    {
        // These are the values from the initial release of the KVP Store.
        m_ulInternalMajorVersion = 1;
        m_ulInternalMinorVersion = 0;
        m_ulInternalUpdateVersion = 0;
    }    

    Assert( m_ulInternalMajorVersion <= p_ulInternalMajorVersion );
    Assert( m_ulInternalMinorVersion <= p_ulInternalMinorVersion );
    Assert( m_ulInternalUpdateVersion <= p_ulInternalUpdateVersion );
    
    if ( m_ulInternalMajorVersion != ulKeyValueTableMajorVersion ||
         m_ulInternalMinorVersion != ulKeyValueTableMinorVersion ||
         m_ulInternalUpdateVersion != ulKeyValueTableUpdateVersion )
    {
        OSTrace(
            JET_tracetagUpgrade,
            OSFormat(
                "KVPStore( %ws ) Internal Schema Version (initial): %u.%u.%u\n",
                p_wszTableName,
                ulKeyValueTableMajorVersion,
                ulKeyValueTableMinorVersion,
                ulKeyValueTableUpdateVersion ) );
        fVersionMismatch = fTrue;
    }

    //  Major Version
    //
    if ( m_ulInternalMajorVersion != ulKeyValueTableMajorVersion )
    {
        //  We are not forward or backward compatible in this regard, bail ...
        AssertSz( fFalse, "Huh, opening a mismatched major version?!?" );
        Error( ErrERRCheck( JET_errInvalidDatabaseVersion ) );
    }

    //  Minor Version
    //
    if ( m_ulInternalMinorVersion < ulKeyValueTableMinorVersion )
    {
        //  We are not forward compatible in this regard, bail ...
        AssertSz( fFalse, "Huh, opening a newer minor version?" );
        Error( ErrERRCheck( JET_errInvalidDatabaseVersion ) );
    }
    else if ( m_ulInternalMinorVersion > ulKeyValueTableMinorVersion )
    {
        //  Perform upgrade to latest minor version ...
        //
        for( ULONG ulMinorVersion = ulKeyValueTableMinorVersion + 1; ulMinorVersion <= m_ulInternalMinorVersion; ulMinorVersion++ )
        {
            switch( ulMinorVersion )
            {
                // We don't have any Minor Version upgrades at this point.
                
            case ULONG_MAX:
            default:
                AssertSz( fFalse, "Unexpected upgrade version ... was the upgrade table not updated with all intermediate versions" );
                break;
            }
            
            //  Everything should be complete, check our version is right ...
            
            Call( ErrKVPIGetValue(
                      eHasTrx,
                      p_wszSchemaInternalMinorVersion,
                      kvpvtSchemaValueType,
                      (BYTE*)&ulKeyValueTableMinorVersion,
                      sizeof( ulKeyValueTableMinorVersion ) ) );
            Assert( ulKeyValueTableMinorVersion == ulMinorVersion );
        }
        
        Call( ErrKVPIGetValue(
                  eHasTrx,
                  p_wszSchemaInternalMinorVersion,
                  kvpvtSchemaValueType,
                  (BYTE*)&ulKeyValueTableMinorVersion,
                  sizeof( ulKeyValueTableMinorVersion ) ) );
        Assert( ulKeyValueTableMinorVersion == m_ulInternalMinorVersion );
    }
        
    //  Update Version
    //
    if ( m_ulInternalUpdateVersion < ulKeyValueTableUpdateVersion )
    {
    }
    else if ( m_ulInternalUpdateVersion > ulKeyValueTableUpdateVersion )
    {
        //  Perform upgrade to latest update version ...
        //
        for( ULONG ulUpdateVersion = ulKeyValueTableUpdateVersion + 1; ulUpdateVersion <= m_ulInternalUpdateVersion; ulUpdateVersion++ )
        {
            switch( ulUpdateVersion )
            {
            case p_ulUpdateAddLongBinaryValueType:
                Call( ErrKVPIAddValueTypeToSchema( kvpvtLongBinaryValueType ) );
                break;
                
            case p_ulUpdateAddLongLongValueType:
                Call( ErrKVPIAddValueTypeToSchema( kvpvtLongLongValueType ) );
                break;
                
            default:
                AssertSz( fFalse, "Unexpected upgrade version ... was the upgrade table not updated with all intermediate versions" );
                break;
            }
            
            //  Everything should be complete, check our version is right ...
            
            Call( ErrKVPIGetValue(
                      eHasTrx,
                      p_wszSchemaInternalUpdateVersion,
                      kvpvtSchemaValueType,
                      (BYTE*)&ulKeyValueTableUpdateVersion,
                      sizeof( ulKeyValueTableUpdateVersion ) ) );
            Assert( ulKeyValueTableUpdateVersion == ulUpdateVersion );
        }
        
        Call( ErrKVPIGetValue(
                  eHasTrx,
                  p_wszSchemaInternalUpdateVersion,
                  kvpvtSchemaValueType,
                  (BYTE*)&ulKeyValueTableUpdateVersion,
                  sizeof( ulKeyValueTableUpdateVersion ) ) );
        Assert( ulKeyValueTableUpdateVersion == m_ulInternalUpdateVersion );
    }
    
    Call( ErrKVPIGetValue(
              eHasTrx,
              p_wszSchemaInternalMajorVersion,
              kvpvtSchemaValueType,
              (BYTE*)&ulKeyValueTableMajorVersion,
              sizeof( ulKeyValueTableMajorVersion ) ) );
    Call( ErrKVPIGetValue(
              eHasTrx,
              p_wszSchemaInternalMinorVersion,
              kvpvtSchemaValueType,
              (BYTE*)&ulKeyValueTableMinorVersion,
              sizeof( ulKeyValueTableMinorVersion ) ) );
    Call( ErrKVPIGetValue(
              eHasTrx,
              p_wszSchemaInternalUpdateVersion,
              kvpvtSchemaValueType,
              (BYTE*)&ulKeyValueTableUpdateVersion,
              sizeof( ulKeyValueTableUpdateVersion ) ) );

    if ( fVersionMismatch )
    {
        OSTrace(
            JET_tracetagUpgrade,
            OSFormat(
                "KVPStore( %ws ) Internal Schema Version (final): %u.%u.%u\n",
                p_wszTableName,
                ulKeyValueTableMajorVersion,
                ulKeyValueTableMinorVersion,
                ulKeyValueTableUpdateVersion ) );
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
    for( iValueType = kvpvtLongValueType; iValueType < kvpvtMaxValueType; iValueType = KVPIValueType( iValueType + 1 ) )
    {
        Assert( m_rgDataTypeInfo[iValueType].m_type == iValueType );

        if ( ( m_rgDataTypeInfo[iValueType].m_ulMajor > m_ulInternalMajorVersion ) ||
             ( m_rgDataTypeInfo[iValueType].m_ulMinor > m_ulInternalMinorVersion ) ||
             ( m_rgDataTypeInfo[iValueType].m_ulUpdate > m_ulInternalUpdateVersion )  )
        {
            // Although we're aware of this data type, we don't actually support it right now.
            // Don't need to try to read any more columnids, the columns are in version order.
            break;
        }

        //  Load the ColumnID out of the table, ignoring warnings.

        Call( ErrIsamGetTableColumnInfo(
                  (JET_SESID)m_ppibGlobalLock,
                  (JET_VTID)m_pfucbGlobalLock,
                  m_rgDataTypeInfo[iValueType].m_szName,
                  NULL,
                  &jcd,
                  sizeof( jcd ),
                  JET_ColInfo,
                  fFalse ) );

        m_rgDataTypeInfo[iValueType].m_cid = jcd.columnid;
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
        //  normal path ...
        return JET_errSuccess;
    }

    Assert( ppibNil == m_ppibGlobalLock );      //  double init?
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

    //  check if the table exists
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
    Assert( ( m_pfucbGlobalLock == NULL ) || ( m_ppibGlobalLock != NULL ) );        // inconsistent state.

    if ( NULL == m_ppibGlobalLock && NULL == m_pfucbGlobalLock )
    {
        //  yeah, already done ...
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

ERR CKVPStore::ErrKVPInitStore( PIB * const ppibProvided, const TrxUpdate eTrxUpd, const ULONG ulMajorVersionExpected )
{
    ERR err = JET_errSuccess;
    IFMP ifmp = g_ifmpMax;
    Assert( ifmp == g_ifmpMax );
    CAutoSZDDL  szTableName;
    bool fReadOnly = (eReadOnly == eTrxUpd);
    bool fFailedOpenDatabase = false;

    Assert( ppibNil == m_ppibGlobalLock );      //  double init?
    Assert( NULL == m_pfucbGlobalLock );

    //  lock the table for initialization

    m_critGlobalTableLock.Enter();

    CallS( szTableName.ErrSet( p_wszTableName ) );

    if ( ppibProvided )
    {
        //  we're probably during system init, so we MUST use the provided system PIB and DBID

        m_ppibGlobalLock = ppibProvided;

        ifmp = m_ifmp;  // assuming this IFMP is opened for this PIB ...
    }
    else
    {
        //  nothing given to us, so open a session to do the dirty work ...

        Call( ErrPIBBeginSession( PinstFromIfmp( m_ifmp ), &m_ppibGlobalLock, procidNil, fFalse ) );

        OPERATION_CONTEXT opContext = { OCUSER_KVPSTORE, 0, 0, 0, 0 };
        Call( m_ppibGlobalLock->ErrSetOperationContext( &opContext, sizeof( opContext ) ) );

        //  if ErrDBOpenDatabase() fails, the resulting ifmp is undefined and we don't need to call ErrDBCloseDatabase()

        fFailedOpenDatabase = true;
        Call( ErrDBOpenDatabase( m_ppibGlobalLock, g_rgfmp[m_ifmp].WszDatabaseName(), &ifmp, fReadOnly ? JET_bitDbReadOnly : NO_GRBIT ) );
        fFailedOpenDatabase = false;

        Assert( ifmp == m_ifmp );
        if ( JET_wrnFileOpenReadOnly == err )
        {
            fReadOnly = true;
        }
    }

    //  check if the table exists
    err = ErrFILEOpenTable( m_ppibGlobalLock, m_ifmp, &m_pfucbGlobalLock, (CHAR*)szTableName );

    if ( JET_errObjectNotFound == err && !fReadOnly )
    {
        //  if it does not exist, create the table

        Call( ErrKVPIInitICreateTable( m_ppibGlobalLock, m_ifmp, (CHAR*)szTableName, ulMajorVersionExpected ) );

        //  then re-open it.
        Call( ErrFILEOpenTable( m_ppibGlobalLock, m_ifmp, &m_pfucbGlobalLock, (CHAR*)szTableName ) );
    }
    else
    {
        Call( err );

        //  init the needed Column IDs from the table

        Call( ErrKVPIInitIBootstrapCriticalCOLIDs() );
    }
    Assert( m_cidKey != m_cidType /* tricky way checking one is not zero as they start zero */ );

    //  check the internal version of the KVP table for compatibility and upgrades ...
    Call( ErrKVPIInitIUpgradeTable( m_ifmp, fReadOnly ) );

    //  load the data-type columnids
    Call( ErrKVPIInitILoadValueTypeCOLIDs() );

    //  check the user major version of the table

    ULONG ulUserMajorVersion = 0;

    Call( ErrKVPIGetValue( eHasTrx, p_wszSchemaExternalMajorVersion, kvpvtSchemaValueType, (BYTE*)&ulUserMajorVersion, sizeof( ulUserMajorVersion ) ) );

    if ( ulUserMajorVersion != ulMajorVersionExpected )
    {
        Error( ErrERRCheck( JET_errInvalidDatabaseVersion ) );
    }

HandleError:

    //  Under failure, cleanup anything we allocated ...

    if ( m_pfucbGlobalLock )
    {
        CallS( ErrFILECloseTable( m_ppibGlobalLock, m_pfucbGlobalLock ) );
        m_pfucbGlobalLock = NULL;
    }

    //  if the caller provided a PIB and JET_DBID/IFMP, then we may not close them

    if ( ppibProvided )
    {
        ifmp = g_ifmpMax;             // not ours to deallocate ...
        Assert( ifmp == g_ifmpMax );
        m_ppibGlobalLock = NULL;    // not ours to deallocate ...
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
    //  Cleanup everything ...
    //
    m_critGlobalTableLock.Enter();

    KVPICloseGlobalPibAndFucb();

    Assert( NULL == m_ppibGlobalLock );
    Assert( NULL == m_pfucbGlobalLock );

    m_critGlobalTableLock.Leave();
}


//  ================================================================
//  Version Handling
//  ================================================================

ERR CKVPStore::ErrKVPGetUserVersion( _Out_ ULONG * pulMinorVersion, _Out_ ULONG * pulUpdateVersion )
{
    ERR err = JET_errSuccess;

    //  begin transaction

    CallR( ErrKVPIBeginTrx( eReadWrite, 40919 ) );

    //  fetch values

    Call( ErrKVPIGetValue( eHasTrx, p_wszSchemaExternalMinorVersion, kvpvtSchemaValueType, (BYTE*)pulMinorVersion, sizeof( *pulMinorVersion ) ) );
    Call( ErrKVPIGetValue( eHasTrx, p_wszSchemaExternalUpdateVersion, kvpvtSchemaValueType, (BYTE*)pulUpdateVersion, sizeof( *pulUpdateVersion ) ) );

HandleError:

    //  commit (or rollback) transaction

    err = ErrKVPIEndTrx( err );

    return err;
}

ERR CKVPStore::ErrKVPSetUserVersion( const ULONG ulMinorVersion, const ULONG ulUpdateVersion )
{
    ERR err = JET_errSuccess;

    //  begin transaction

    CallR( ErrKVPIBeginTrx( eReadWrite, 57303 ) );

    //  update values

    Call( ErrKVPISetValue( eHasTrx, p_wszSchemaExternalMinorVersion, kvpvtSchemaValueType, (BYTE*)&ulMinorVersion, sizeof( ulMinorVersion ) ) );
    Call( ErrKVPISetValue( eHasTrx, p_wszSchemaExternalUpdateVersion, kvpvtSchemaValueType, (BYTE*)&ulUpdateVersion, sizeof( ulUpdateVersion ) ) );

HandleError:

    //  commit (or rollback) transaction

    err = ErrKVPIEndTrx( err );

    return err;
}


//  ================================================================
//  Transaction Control
//  ================================================================

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

    //  process the commit (or rollback if there are issues)

    if( ( errRet < JET_errSuccess ) ||
        ( ( errC = ErrIsamCommitTransaction( (JET_SESID)m_ppibGlobalLock, JET_bitCommitLazyFlush ) ) < JET_errSuccess ) )
    {
        //  Problems ... whatever they were, we should rollback ...
        //
        errR = ErrIsamRollback( (JET_SESID)m_ppibGlobalLock, NO_GRBIT );
        Assert( errR >= JET_errSuccess || PinstFromPpib( m_ppibGlobalLock )->FInstanceUnavailable() );
    }

    //  cleanup resources that we cannot hold open

    KVPICloseGlobalPibAndFucb();

    //  leave the critical section

    Assert( NULL == m_ppibGlobalLock );
    Assert( NULL == m_pfucbGlobalLock );

    m_critGlobalTableLock.Leave();

    //  process the error

    if ( errRet < JET_errSuccess )
    {
        return errRet;  // most important error
    }
    if ( errC >= JET_errSuccess )
    {
        // we don't actually expect commit to warn.
        CallS( errC );

        // Commit succeeded. Original operation succeeded or warned.  Return
        // original operation, which is the same as commit OR has more info.
        return errRet;
    }
    if ( errR < JET_errSuccess )
    {
        Assert( errR != JET_errInternalError );
        return ErrERRCheck( JET_errRollbackError ); // nothing worked, communicate that
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

    Assert( m_ppibGlobalLock == ppibNil || m_ppibGlobalLock == (PIB*)JET_sesidNil );    // double init?, leaked close?
    Assert( NULL == m_pfucbGlobalLock );

    //  "acquire" a PIB / set the global transaction ...

    m_ppibGlobalLock = ppib;

    //  get handle to our KVP store table

    CallS( szTableName.ErrSet( p_wszTableName ) );
    Call( ErrFILEOpenTable( m_ppibGlobalLock, m_ifmp, &m_pfucbGlobalLock, (CHAR*)szTableName ) );

    //  begin a (possibly nested) transaction to perform the updates

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

    //  process the commit (or rollback if there are issues)

    if( ( errRet < JET_errSuccess ) ||
        ( ( errC = ErrIsamCommitTransaction( (JET_SESID)m_ppibGlobalLock, NO_GRBIT ) ) < JET_errSuccess ) )
    {
        //  Problems ... whatever they were, we should rollback ...
        //
        errR = ErrIsamRollback( (JET_SESID)m_ppibGlobalLock, NO_GRBIT );
        CallSx( errR, JET_errRollbackError );
        Assert( errR >= JET_errSuccess || PinstFromPpib( m_ppibGlobalLock )->FInstanceUnavailable() );
    }

    //  cleanup resources that we cannot hold open

    CallS( ErrFILECloseTable( m_ppibGlobalLock, m_pfucbGlobalLock ) );
    m_pfucbGlobalLock = NULL;

    //  "release" resources that don't belong to us.

    m_ppibGlobalLock = NULL;

    //  leave the critical section

    Assert( NULL == m_ppibGlobalLock );
    Assert( NULL == m_pfucbGlobalLock );

    m_critGlobalTableLock.Leave();

    //  process the error

    if ( errRet < JET_errSuccess )
    {
        return errRet;  // most important error
    }
    if ( errC >= JET_errSuccess )
    {
        // we don't actually expect commit to warn.
        CallS( errC );

        // Commit succeeded. Original operation succeeded or warned.  Return
        // original operation, which is the same as commit OR has more info.
        return errRet;
    }
    if ( errR < JET_errSuccess )
    {
        Assert( errR == JET_errRollbackError );
        return errR;    // nothing worked, communicate that
    }

    return errC;
}


//  ================================================================
//  Internal KVP/Entry Prepare for Insertion and Replace
//  ================================================================

ERR CKVPStore::ErrKVPIPrepUpdate(  const WCHAR * const wszKey, KVPIValueType kvpvt )
{
    ERR     err = JET_errSuccess;

    BOOL    fInUpdate = fFalse;

    Assert( kvpvt > kvpvtInvalidType );
    Assert( kvpvt < kvpvtMaxValueType );

    //  First find the entry we're interested in via key
    //

    // Note: We store this w/ the NUL ...
    const INT cbKey = CbKVPKeySize( wszKey );

    Call( ErrIsamMakeKey( m_ppibGlobalLock, m_pfucbGlobalLock, wszKey, cbKey, JET_bitNewKey ) );

    err = ErrIsamSeek( m_ppibGlobalLock, m_pfucbGlobalLock, JET_bitSeekEQ );

    //  we can handle only a missing record error ...
    if ( err != JET_errRecordNotFound )
    {
        Call( err );    // some kind of failure ... or success/warning drops through ...
    }

    //  Next prepare the replace or insert per presence of the entry
    //

    if ( err == JET_errRecordNotFound )
    {
        //  Doing an Insert ...
        //

        //  prepare the record for insert

        Call( ErrIsamPrepareUpdate( m_ppibGlobalLock, m_pfucbGlobalLock, JET_prepInsert ) );
        fInUpdate  = fTrue;

        //  setup the key for the insert

        Call( ErrIsamSetColumn( m_ppibGlobalLock, m_pfucbGlobalLock, m_cidKey, (void*)wszKey, cbKey, NO_GRBIT, NULL ) );

        //  setup the type

        Assert( kvpvt < kvpvtMaxValueType );
        C_ASSERT( kvpvtMaxValueType < 255 );
        BYTE eType = (BYTE)kvpvt;

        Call( ErrIsamSetColumn( m_ppibGlobalLock, m_pfucbGlobalLock, m_cidType, (void*)&eType, sizeof( eType ), NO_GRBIT, NULL ) );
    }
    else
    {
        //  Doing a Replace ...
        //

        //  prepare the record for replace

        Assert( err >= JET_errSuccess );
        Call( ErrIsamPrepareUpdate( m_ppibGlobalLock, m_pfucbGlobalLock, JET_prepReplace ) );
        fInUpdate  = fTrue;

        //  validate the type is what is expected

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

    Assert( err < JET_errSuccess ); // some kind of failure ...
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


//  ================================================================
//  Internal KVP/Entry Setting / Getting / Deleting
//  ================================================================

VOID CKVPStore::AssertKVPIValidUserKey( const KVPIValueType kvpvt, const WCHAR * const wszKey )
{
    Assert( wszKey );
    Assert( wszKey[0] != 0 );
    Assert( wszKey[0] != s_wchSecret || kvpvt == kvpvtSchemaValueType );    // users can't have keys beginning with L'.' / secret char
    Assert( NULL == wcschr( wszKey, L'*' ) );   // otherwise wildcard searches would be weird
}

ERR CKVPStore::ErrKVPIDeleteKey(
    const TrxPosition eTrxPos,
    const WCHAR * const wszKey)
{
    ERR     err = JET_errSuccess;
    BOOL    fOwnsTrx = fFalse;

    //
    //  Validate args
    //

    // We don't allow deleting a schema value type, which is indicated by the key beginning with
    // the secret character.
    if ( ( NULL == wszKey ) ||
         ( wszKey[0] == 0 ) ||
         ( wszKey[0] == s_wchSecret ) ||
         ( NULL != wcschr( wszKey, L'*' ) ) )   // Can't delete with a wildcard key
    {
        AssertSz( FNegTest( fInvalidUsage ), "Invalid deletion of KVPI key here.\n" );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    //  Begin a transaction if necessary ...
    //

    if ( eTrxPos == eNewTrx )
    {
        CallR( ErrKVPIBeginTrx( eReadWrite, 60086 ) );
        fOwnsTrx = fTrue;
    }
    else
    {
        Assert( eTrxPos == eHasTrx );
    }
    Assert( m_critGlobalTableLock.FOwner() );

    Assert( m_ppibGlobalLock != ppibNil && m_ppibGlobalLock != (PIB*)JET_sesidNil );
    Assert( m_pfucbGlobalLock != pfucbNil && m_pfucbGlobalLock != (FUCB*)JET_tableidNil );

    // Note: We store this w/ the NUL ...
    const INT cbKey = CbKVPKeySize( wszKey );

    Call( ErrIsamMakeKey( m_ppibGlobalLock, m_pfucbGlobalLock, wszKey, cbKey, JET_bitNewKey ) );

    err = ErrIsamSeek( m_ppibGlobalLock, m_pfucbGlobalLock, JET_bitSeekEQ );
    // Expect no warnings
    if ( err > JET_errSuccess )
    {
        CallS( err );
        err = JET_errInternalError;
    }

    switch ( err )
    {
    case JET_errSuccess:
        Call( ErrIsamDelete( m_ppibGlobalLock, m_pfucbGlobalLock ) );
        break;

    case JET_errRecordNotFound:
        // Record is already not here.  Just make that a warning.
        err = ErrERRCheck( wrnKVPEntryAlreadyNotPresent );
        break;

    default:
        Call( err );
    }
    
HandleError:

    if ( JET_errRecordNotFound == err )
    {
        AssertSz( fFalse, "We don't expect this error from any other API other than ErrIsamSeek()!" );
        err = ErrERRCheck( JET_errInternalError );
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

ERR CKVPStore::ErrKVPISetValue(
    const TrxPosition eTrxPos,
    const WCHAR * const wszKey,
    const KVPIValueType kvpvt,
    _In_opt_bytecount_( cbValue ) const BYTE * const pbValue,
    const ULONG cbValue )
{
    ERR     err = JET_errSuccess;

    BOOL    fOwnsTrx = fFalse;
    BOOL    fInUpdate = fFalse;

    //
    //  Validate args
    //

    AssertKVPIValidUserKey( kvpvt, wszKey );

    //  Validate this is a has a value type we know of ...
    //

    if ( kvpvt <= kvpvtInvalidType || kvpvt >= kvpvtMaxValueType )
    {
        AssertSz( fFalse, "Invalid KVPIValueType = %d here.\n", kvpvt );
        return ErrERRCheck( JET_errInvalidColumnType );
    }

    //  Validate the size matched with the value type caller provided ...
    //

    if ( m_rgDataTypeInfo[kvpvt].m_cbDataCol != cbValue &&
            m_rgDataTypeInfo[kvpvt].m_cbDataCol != cbKVPIVariableDataSize )
    {
        AssertSz( fFalse, "Invalid size of data element passed to CKVPStore = %d\n", cbValue );
        return ErrERRCheck( JET_errInvalidBufferSize );
    }

    //  Begin a transaction if necessary ...
    //

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

    //  This prepares an entry for either insertion or update, setting both they key and type
    //
    Call( ErrKVPIPrepUpdate( wszKey, kvpvt ) );
    fInUpdate = fTrue;
    
    //  This sets the value of the KVP.
    //
    
    Call( ErrIsamSetColumn( m_ppibGlobalLock, m_pfucbGlobalLock, m_rgDataTypeInfo[kvpvt].m_cid, pbValue, cbValue, NO_GRBIT, NULL ) );
    
    Call( ErrIsamUpdate( m_ppibGlobalLock, m_pfucbGlobalLock, NULL, 0, NULL, NO_GRBIT ) );
    fInUpdate = fFalse;
    
    CallS( err );

HandleError:

    Assert( err <= JET_errSuccess );    // some kind of failure ...
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

ERR CKVPStore::ErrKVPIGetValue(
    const TrxPosition eTrxPos,
    const WCHAR * const wszKey,
    const KVPIValueType kvpvt,
    _Out_bytecap_( cbValue ) BYTE * const pbValue,
    const ULONG cbValue,
    _Out_opt_ ULONG *pcbActual )
{
    ERR err = JET_errSuccess;

    BOOL    fOwnsTrx = fFalse;

    //  Validate args
    //

    AssertKVPIValidUserKey( kvpvt, wszKey );

    //  Validate this is a has a value type we know of ...

    if ( kvpvt <= kvpvtInvalidType || kvpvt >= kvpvtMaxValueType )
    {
        AssertSz( fFalse, "Invalid KVPIValueType = %d here.\n", kvpvt );
        return ErrERRCheck( JET_errInvalidColumnType );
    }

    //  Validate the size matched with the value type caller provided ...

    if ( m_rgDataTypeInfo[kvpvt].m_cbDataCol != cbValue &&
            m_rgDataTypeInfo[kvpvt].m_cbDataCol != cbKVPIVariableDataSize )
    {
        AssertSz( fFalse, "Invalid size of data element passed to CKVPStore = %d\n", cbValue );
        return ErrERRCheck( JET_errInvalidBufferSize );
    }

    //  This code path doesn't fail on a truncated key, so we have to emulate it to keep the
    //  same contract as ErrKVP*SetValue().

    const INT cbKey = CbKVPKeySize( wszKey );
    Assert( 0 == ( cbKey % 2 ) );
    if ( cbKey > cbKVPMaxKey )
    {
        //  key was longer than max allowed, fail ...
        return ErrERRCheck( JET_errKeyTruncated );
    }

    //  Do the "get" operation
    //

    if ( pcbActual )
    {
        *pcbActual = 0;
    }
    
    //  Begin a transaction if necessary ...

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

    //  Make the key and seek to the record

    Call( ErrIsamMakeKey( m_ppibGlobalLock, m_pfucbGlobalLock, wszKey, cbKey, JET_bitNewKey ) );

    Call( ErrIsamSeek( m_ppibGlobalLock, m_pfucbGlobalLock, JET_bitSeekEQ ) );

    //  validate the type is what is expected

    ULONG cbActual;
    BYTE bValueType = (BYTE)kvpvtInvalidType;
    Call( ErrIsamRetrieveColumn( m_ppibGlobalLock, m_pfucbGlobalLock, m_cidType, (void*)&bValueType, sizeof( bValueType ), &cbActual, NO_GRBIT, NULL ) );
    Assert( cbActual == 1 );
    if ( ((KVPIValueType)bValueType) != kvpvt )
    {
        AssertSz( FNegTest( fInvalidUsage ), "The value types did not match: %d != %d", ((KVPIValueType)bValueType), kvpvt );
        Call( ErrERRCheck( JET_errInvalidColumnType ) );
    }

    //  Retrieve the actual value

    Call( ErrIsamRetrieveColumn( m_ppibGlobalLock, m_pfucbGlobalLock, m_rgDataTypeInfo[kvpvt].m_cid, pbValue, cbValue, &cbActual, NO_GRBIT, NULL ) );
    
    if ( m_rgDataTypeInfo[kvpvt].m_cbDataCol != (INT)cbActual &&
            m_rgDataTypeInfo[kvpvt].m_cbDataCol != cbKVPIVariableDataSize )
    {
        AssertSz( fFalse, "Invalid size of data element in table = %d\n", cbActual );
        return ErrERRCheck( JET_errInvalidBufferSize );
    }

    if ( pcbActual )
    {
        *pcbActual = cbActual;
    }
    
HandleError:

    if ( fOwnsTrx )
    {
        (void)ErrKVPIEndTrx( err ); // doesn't matter, we already have the value
        fOwnsTrx = fFalse;
    }

    Assert( fOwnsTrx == fFalse );

    return err;
}


//  ================================================================
//  API KVP Setting / Getting / Deleting
//  ================================================================

INT CKVPStore::CbKVPKeySize( const WCHAR * const wszKey ) const
{
    // Note: We store the key with the NULL in this class.
    const INT cbKey = sizeof(WCHAR) * ( 1 + LOSStrLengthW( wszKey ) );
    Assert( 0 == ( cbKey % 2 ) );
    Assert( cbKey > 0 );
    Assert( cbKey <= cbKVPMaxKey || FNegTest( fInvalidUsage ) );
    return cbKey;
}

ERR CKVPStore::ErrKVPDeleteKey( const WCHAR * const wszKey )
{
    return ErrKVPIDeleteKey( eNewTrx, wszKey );
}

ERR CKVPStore::ErrKVPSetValue( const WCHAR * const wszKey, const INT iValue )
{
    return ErrKVPISetValue( eNewTrx, wszKey, kvpvtLongValueType, (BYTE*)&iValue, sizeof( iValue ) );
}

ERR CKVPStore::ErrKVPSetValue( const WCHAR * const wszKey, const INT64 i64Value )
{
    return ErrKVPISetValue( eNewTrx, wszKey, kvpvtLongLongValueType, (BYTE*)&i64Value, sizeof( i64Value ) );
}

ERR CKVPStore::ErrKVPSetValue( const WCHAR * const wszKey, const BYTE * const pbValue, const ULONG cbValue )
{
    return ErrKVPISetValue( eNewTrx, wszKey, kvpvtLongBinaryValueType, pbValue, cbValue );
}

ERR CKVPStore::ErrKVPAdjValue( __inout PIB * ppibProvided, const WCHAR * const wszKey, const INT iAdjustment )
{
    ERR err = JET_errSuccess;

    //  begin transaction

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

    //  fetch the original value

    err = ErrKVPIGetValue( eHasTrx, wszKey, kvpvtLongValueType, (BYTE*)&iValue, sizeof( iValue ) );

    //  presume 0 value if not exists yet

    if ( err == JET_errRecordNotFound )
    {
        iValue = 0;
        err = JET_errSuccess;
    }
    Call( err );

    //  apply the adjustment

    if ( ( ( iAdjustment > 0 ) && ( ( iValue + iAdjustment ) < iValue ) ) ||    // inc and overflow
         ( ( iAdjustment < 0 ) && ( ( iValue + iAdjustment ) > iValue ) ) ) // dec and overflow
    {
        Call( ErrERRCheck( JET_errOutOfAutoincrementValues ) );
    }

    iValue += iAdjustment;

    //  set the value

    Call( ErrKVPISetValue( eHasTrx, wszKey, kvpvtLongValueType, (BYTE*)&iValue, sizeof( iValue ) ) );

HandleError:

    //  commit (or rollback) transaction

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

ERR CKVPStore::ErrKVPGetValue( const WCHAR * const wszKey, _Out_ INT * piValue )
{
    return ErrKVPIGetValue( eNewTrx, wszKey, kvpvtLongValueType, (BYTE*)piValue, sizeof( *piValue ) );
}

ERR CKVPStore::ErrKVPGetValue( const WCHAR * const wszKey, _Out_ INT64 * pi64Value )
{
    return ErrKVPIGetValue( eNewTrx, wszKey, kvpvtLongLongValueType, (BYTE*)pi64Value, sizeof( *pi64Value ) );
}

// Retrieves variable size binary buffer from KVP store.
// Note:  If a value was set as INT or INT64, this routine will not retrieve it.  Keep the same typed routines for Set/Get.
ERR CKVPStore::ErrKVPGetValue( const WCHAR * const wszKey, _Out_ BYTE * const pbValue, const ULONG cbValueMax, _Out_opt_ ULONG *pcbValueActual )
{
    return ErrKVPIGetValue( eNewTrx, wszKey, kvpvtLongBinaryValueType, (BYTE*)pbValue, cbValueMax, pcbValueActual );
}

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

//  ================================================================
//  Cursor Enumeration Helper Routines
//  ================================================================

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

    Assert( pwszKey );  // but just in case ...
    if( NULL == pwszKey )
    {
        return fFalse;
    }

    if( FKVPSCursorIUserWildcard( m_wszWildKey ) )
    {
        //  User wild card...
        //
        return pwszKey[0] != CKVPStore::s_wchSecret;
    }
    else if( FKVPSCursorIInternalWildcard( m_wszWildKey ) )
    {
        //  Internal wild card...
        //
        return pwszKey[0] == CKVPStore::s_wchSecret;
    }
    else if( FKVPSCursorIAllWildcard( m_wszWildKey ) )
    {
        //  Everything wild card...
        //
        return fTrue;
    }
    else
    {
        //  General wild card ..
        //

        const WCHAR * pwszWildchar = wcschr( m_wszWildKey, L'*' );
        const ULONG cchWildkey = LOSStrLengthW( m_wszWildKey );

        if( pwszWildchar && // has wild card char and the end of string
            ( pwszWildchar == ( m_wszWildKey + cchWildkey - 1 ) ) )
        {
            return 0 == wcsncmp( m_wszWildKey, pwszKey, cchWildkey - 1 );
        }
        else
        {
            //  do exact match - kind of not point in this as they could've just
            //  called CKVPStore::ErrKVPGetValue()

            return 0 == LOSStrCompareW( m_wszWildKey, pwszKey );
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

//  ================================================================
//  Cursor Enumeration APIs
//  ================================================================

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
        //  move to the first entry in the table

        Call( ErrIsamMove( m_pkvps->m_ppibGlobalLock, m_pkvps->m_pfucbGlobalLock, JET_MoveFirst, NO_GRBIT ) );

        m_eState = kvpscEnumerating;

        //  check the very first entry for a match

        Call( ErrCKVPSCursorIEstablishCurrency() );

        if ( FKVPSCursorIMatches() )
        {
            //  Success!  Return the first result to the client

            return JET_errSuccess;
        }

        //  else fall through and try the next entry until we fine a match
    }

    Assert( m_eState == kvpscEnumerating );

    do
    {
        //  move to the next entry ...

        Call( ErrIsamMove( m_pkvps->m_ppibGlobalLock, m_pkvps->m_pfucbGlobalLock, 1, NO_GRBIT ) );

        //  and check for a match.

        Call( ErrCKVPSCursorIEstablishCurrency() );
    }
    while( !FKVPSCursorIMatches() );

    //  Success!  Return the result to the client

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

//  ================================================================
//  Cursor Retrieving APIs
//  ================================================================

const WCHAR * CKVPStore::CKVPSCursor::WszKVPSCursorCurrKey() const
{
    return m_wszCurrentKey;
}

//  not a fan that I had to duplicate this, consider another way of getting around
//  the locking problem, probably a 2nd session for the cursor object.
ERR CKVPStore::CKVPSCursor::ErrKVPSCursorGetValue( _Out_ INT * piValue )
{
    ERR err = JET_errSuccess;

    Assert( m_eState == kvpscEnumerating );

    CKVPStore::KVPIValueType kvpvt = CKVPStore::kvpvtInvalidType;

    CallR( ErrKVPSCursorIGetType( &kvpvt ) );

    if( kvpvt == CKVPStore::kvpvtSchemaValueType ||
        kvpvt == CKVPStore::kvpvtLongValueType )
    {
        return m_pkvps->ErrKVPIGetValue( eHasTrx, m_wszCurrentKey, kvpvt, (BYTE*)piValue, sizeof( *piValue ) );
    }

    //  else we don't know the type ...

    AssertSz( fFalse, "Unknown type %d\n", kvpvt );

    return ErrERRCheck( JET_errInvalidColumnType );
}

//  not a fan that I had to duplicate this, consider another way of getting around
//  the locking problem, probably a 2nd session for the cursor object.
ERR CKVPStore::CKVPSCursor::ErrKVPSCursorUpdValue( _In_ INT iValue )
{
    ERR err = JET_errSuccess;

    Assert( m_eState == kvpscEnumerating );

    CKVPStore::KVPIValueType kvpvt = CKVPStore::kvpvtInvalidType;

    CallR( ErrKVPSCursorIGetType( &kvpvt ) );

    if( kvpvt == CKVPStore::kvpvtSchemaValueType ||
        kvpvt == CKVPStore::kvpvtLongValueType )
    {
        return m_pkvps->ErrKVPISetValue( eHasTrx, m_wszCurrentKey, kvpvt, (BYTE*)&iValue, sizeof( iValue ) );
    }

    //  else we don't know the type ...

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

//  ================================================================
//  Unit testing Helper routines ...
//  ================================================================

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

//  ================================================================
//  Unit testing for CKVPStore stuff ...
//  ================================================================

#ifdef ENABLE_JET_UNIT_TEST

JETUNITTESTDB( KVPStore, TestCreate, dwOpenDatabase )
{
    const WCHAR * wszMSysTestTableName = L"MSysTESTING_KVPStoreTestCreate";
    //  Construct CKVPStore class, and initialize (should create table)

    CKVPStore kvpstore( IfmpTest(), wszMSysTestTableName );
    CHECKCALLS( kvpstore.ErrKVPInitStore( 1 ) );

    //  Validate we can open the table ...
    //

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

//  CHECK( 0 == FNegTestSet( fInvalidUsage ) );
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
        // if set succeeded, get should succeed ... otherwise probably a key size limitation
        // mis-match between set/get.
        const ERR errGet = kvpstore.ErrKVPGetValue( wszOneName, &iValue );
        CHECKCALLS( errGet );
        CHECK( iValue == ( cbKey * 3 ) );
    }

    CHECK( cbKey < ( sizeof( wszOneName ) - 10 ) );     // shouldn't have come near filing the full wszOneName array ...
    // largest allowed key is 224 bytes, cbKey should've incremented +2 past that ...
    CHECK( 226 == cbKey );
    CHECK( cbKey == ( CKVPStore::cbKVPMaxKey + 2 ) );   // otherwise we need to change ErrKVPGetValue() ...

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

JETUNITTESTDB( KVPStore, TestDeleteValue, dwOpenDatabase )
{
    CKVPStore kvpstore( IfmpTest(), L"MSysTESTING_KVPStoreTestDeleteValue" );

    const WCHAR * wszOneName = L"JustAKeyName";
    const WCHAR * wszAnotherName = L"JustAnotherKeyName";
    const WCHAR * wszALongName = L"ThisIsAVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryLongName";
    const WCHAR * wszASplatName = L"JustA*Name";
    
    CHECKCALLS( kvpstore.ErrKVPInitStore( 1 ) );

    INT iValue = 42;

    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 42 );

    CHECKCALLS( kvpstore.ErrKVPSetValue( wszOneName, 2 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 2 );

    // Delete the key that exists.
    CHECK( JET_errSuccess == kvpstore.ErrKVPDeleteKey( wszOneName ) );
    // Fail to delete the key that used to exist.
    CHECK( wrnKVPEntryAlreadyNotPresent == kvpstore.ErrKVPDeleteKey( wszOneName ) );
    // Fail to delete a key that never existed.
    CHECK( wrnKVPEntryAlreadyNotPresent == kvpstore.ErrKVPDeleteKey( wszAnotherName ) );
    // Fail to delete a key that never existed and is very long.
    FNegTestSet( fInvalidUsage );
    CHECK( wrnKVPEntryAlreadyNotPresent == kvpstore.ErrKVPDeleteKey( wszALongName ) );
    FNegTestUnset( fInvalidUsage ); 
    // Fail to delete a key that never existed that has a * in it.
    FNegTestSet( fInvalidUsage );
    CHECK( JET_errInvalidParameter == kvpstore.ErrKVPDeleteKey( wszASplatName ) );
    FNegTestUnset( fInvalidUsage ); 
    // Fail to delete a schema key.
    FNegTestSet( fInvalidUsage );
    CHECK( JET_errInvalidParameter == kvpstore.ErrKVPDeleteKey( p_wszSchemaInternalMajorVersion ) );
    FNegTestUnset( fInvalidUsage ); 

    kvpstore.KVPTermStore();
}


JETUNITTESTDB( KVPStore, TestSetNulValue, dwOpenDatabase )
{
    CKVPStore kvpstore( IfmpTest(), L"MSysTESTING_KVPStoreTestSetNulValue" );

    CHECKCALLS( kvpstore.ErrKVPInitStore( 1 ) );

    // INT iValue = 42;

    // does this work?  sort of, it asserts()
    //CHECKCALLS( kvpstore.ErrKVPSetValue( L"", 2 ) );
    //CHECKCALLS( kvpstore.ErrKVPGetValue( L"", &iValue ) );
    //CHECK( iValue == 2 );

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
    CHECK( iValue == 34 );  // original un-peturbed

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

    //  setup some basic data to start with

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
    CHECK( iValue == 34 );  // original un-peturbed

    //  negative adjustment

    CHECKCALLS( kvpstore.ErrKVPAdjValue( wszOneName, -1 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 33 );

    //  positive adjustment

    CHECKCALLS( kvpstore.ErrKVPAdjValue( wszOneName, 2 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 35 );

    //  adjusting non-existant value

    CHECKCALLS( kvpstore.ErrKVPAdjValue( L"Doesn'tExistYet", 1 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( L"Doesn'tExistYet" /* name out of date! :) */, &iValue ) );
    CHECK( iValue == 1 );

    //  negative adjusting non-existant value

    CHECKCALLS( kvpstore.ErrKVPAdjValue( L"AlsoNonExistantCurrently", 1 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( L"AlsoNonExistantCurrently" /* name out of date! :) */, &iValue ) );
    CHECK( iValue == 1 );

    //  adjusting other value

    CHECKCALLS( kvpstore.ErrKVPAdjValue( L"TotallyDifferentKeyName", 25 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( L"TotallyDifferentKeyName", &iValue ) );
    CHECK( iValue == 50 );

    //  setting still works

    CHECKCALLS( kvpstore.ErrKVPSetValue( wszOneName, 20 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 20 );

    //  adjust one more time

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

    //  setup some basic data to start with

    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 42 );

    CHECKCALLS( kvpstore.ErrKVPSetValue( wszOneName, 2 ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 2 );

    //  large positive "over"-adjustment

    CHECK( JET_errOutOfAutoincrementValues == kvpstore.ErrKVPAdjValue( wszOneName, ( 0x7FFFFFFF - 1 ) ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 2 );

    //  large positive "up-to"-adjustment

    CHECKCALLS( kvpstore.ErrKVPAdjValue( wszOneName, ( 0x7FFFFFFF - 2 ) ) );
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 0x7FFFFFFF );

    //  small positive "over"-adjustment

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

    //  Load it with some data and basic operations...
    //
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

    //  Term and Destroy the KVPStore ...
    //
    pkvps->KVPTermStore();
    delete pkvps;

    //  Now re-initialize a new KVPStore ...
    //
    pkvps = new CKVPStore( IfmpTest(), wszMSysTestTableName );
    CHECK( NULL != pkvps );

    CHECKCALLS( pkvps->ErrKVPInitStore( 1 ) );

    //  Should have old values, as table is opened, not re-created ...
    //
    CHECKCALLS( pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 34 );
    CHECKCALLS( pkvps->ErrKVPGetValue( L"TotallyDifferentKeyName", &iValue ) );
    CHECK( iValue == 25 );

    //  Now load it with some new operations ...
    //
    iValue = 13;
    CHECKCALLS( pkvps->ErrKVPSetValue( wszOneName, iValue ) );
    CHECKCALLS( pkvps->ErrKVPGetValue( L"TotallyDifferentKeyName", &iValue ) );
    CHECK( iValue == 25 );
    CHECKCALLS( pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 13 );

    //  Term and Destroy the KVPStore ... again.
    //
    pkvps->KVPTermStore();
    delete pkvps;
}

JETUNITTESTDB( KVPStore, TestSetLongLongValue, dwOpenDatabase )
{
    CKVPStore kvpstore( IfmpTest(), L"MSysTESTING_KVPStoreTestSetLongLongValue" );

    const WCHAR * wszOneName = L"JustAKeyName";
    const WCHAR * wszAnotherName = L"TotallyDifferentKeyName";
    
    CHECKCALLS( kvpstore.ErrKVPInitStore( 1 ) );

    const INT iValueDefault = 42;
    const INT64 i64ValueDefault = 0x100000001;
    const INT cbSomeData = 32;
    BYTE rgbSomeData[cbSomeData];
    for ( int i = 0; i < cbSomeData; i++ )
    {
        rgbSomeData[i] = (BYTE)TickOSTimeCurrent();
    }

    const INT64 i64Value1 = 2;
    const INT64 i64Value2 = 34;
    const INT64 i64ValueAnother = 25;

    INT iValueResults;
    INT64 i64ValueResults;
    BYTE rgbSomeResults[cbSomeData];
    ULONG cbDataRead;

    // Read various value types, none of which should be there.  On error,
    // the _Out variables should be untouched, except for cbDataRead, which
    // should be 0.
    iValueResults = iValueDefault;
    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( wszOneName, &iValueResults ) );
    CHECK( iValueResults == iValueDefault );
    
    i64ValueResults = i64ValueDefault;
    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( wszOneName, &i64ValueResults ) );
    CHECK( i64ValueResults == i64ValueDefault );

    cbDataRead = iValueDefault;
    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( wszOneName, rgbSomeResults, sizeof( rgbSomeResults ), &cbDataRead ) );
    CHECK( 0 == cbDataRead );

    // Set an INT64
    CHECKCALLS( kvpstore.ErrKVPSetValue( wszOneName, i64Value1 ) );

    // Try reading again, which should only succeed when reading an INT64
    iValueResults = iValueDefault;
    OnNonRTM( CHECK( 0 == FNegTestSet( fInvalidUsage ) ) );
    CHECK( JET_errInvalidColumnType == kvpstore.ErrKVPGetValue( wszOneName, &iValueResults ) );
    OnNonRTM( (void)FNegTestUnset( fInvalidUsage ) );
    CHECK( iValueResults == iValueDefault );

    i64ValueResults = i64ValueDefault;
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &i64ValueResults ) );
    CHECK( i64ValueResults == i64Value1 );

    cbDataRead = iValueDefault;
    OnNonRTM( CHECK( 0 == FNegTestSet( fInvalidUsage ) ) );
    CHECK( JET_errInvalidColumnType == kvpstore.ErrKVPGetValue( wszOneName, rgbSomeResults, sizeof( rgbSomeResults ), &cbDataRead ) );
    OnNonRTM( (void)FNegTestUnset( fInvalidUsage ) );
    CHECK( cbDataRead == 0 );

    // Verify that we can set another key-value pair without disturbing this one.
    CHECKCALLS( kvpstore.ErrKVPSetValue( wszAnotherName, i64ValueAnother ) );
    i64ValueResults = i64ValueDefault;
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &i64ValueResults ) );
    CHECK( i64ValueResults == i64Value1 );  // original un-peturbed
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszAnotherName, &i64ValueResults ) );
    CHECK( i64ValueResults == i64ValueAnother );
    
    // Change the value.
    CHECKCALLS( kvpstore.ErrKVPSetValue( wszOneName, i64Value2 ) );

    // Reread.
    iValueResults = iValueDefault;
    OnNonRTM( CHECK( 0 == FNegTestSet( fInvalidUsage ) ) );
    CHECK( JET_errInvalidColumnType == kvpstore.ErrKVPGetValue( wszOneName, &iValueResults ) );
    OnNonRTM( (void)FNegTestUnset( fInvalidUsage ) );
    CHECK( iValueResults == iValueDefault );
    
    i64ValueResults = i64ValueDefault;
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, &i64ValueResults ) );
    CHECK( i64ValueResults == i64Value2 );

    cbDataRead = iValueDefault;
    OnNonRTM( CHECK( 0 == FNegTestSet( fInvalidUsage ) ) );
    CHECK( JET_errInvalidColumnType == kvpstore.ErrKVPGetValue( wszOneName, rgbSomeResults, sizeof( rgbSomeResults ), &cbDataRead ) );
    OnNonRTM( (void)FNegTestUnset( fInvalidUsage ) );
    CHECK( cbDataRead == 0 );

    // Try to change INT64 to a LongBinary.
    OnNonRTM( CHECK( 0 == FNegTestSet( fInvalidUsage ) ) );
    CHECK( JET_errInvalidColumnType == kvpstore.ErrKVPSetValue( wszOneName, rgbSomeData, cbSomeData ) );
    OnNonRTM( (void)FNegTestUnset( fInvalidUsage ) );

    // Finally, delete the data.
    CHECKCALLS( kvpstore.ErrKVPDeleteKey( wszOneName ) );

    // And try to reread.
    iValueResults = iValueDefault;
    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( wszOneName, &iValueResults ) );
    CHECK( iValueResults == iValueDefault );
    
    i64ValueResults = i64ValueDefault;
    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( wszOneName, &i64ValueResults ) );
    CHECK( i64ValueResults == i64ValueDefault );

    cbDataRead = iValueDefault;
    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( wszOneName, rgbSomeResults, sizeof( rgbSomeResults ), &cbDataRead ) );
    CHECK( 0 == cbDataRead );
    
    kvpstore.KVPTermStore();
}

JETUNITTESTDB( KVPStore, TestSetLongBinary, dwOpenDatabase )
{

    CKVPStore kvpstore( IfmpTest(), L"MSysTESTING_KVPStoreTestSetLongBinaryValue" );

    const WCHAR * wszOneName = L"JustAKeyName";

    CHECKCALLS( kvpstore.ErrKVPInitStore( 1 ) );

    const INT iValueDefault = 42;
    const INT64 i64ValueDefault = 0x100000001;
    const INT cbSomeData = 2048;
    BYTE rgbSomeData[cbSomeData];
    for ( int i = 0; i < cbSomeData; i++ )
    {
        rgbSomeData[i] = (BYTE)TickOSTimeCurrent();
    }
    const INT cbALittleData = 32;

    Assert( cbALittleData < cbSomeData / 2 );  // goal is to test significant different data size.
    const INT iValue1 = 2;
    
    INT iValueResults;
    INT64 i64ValueResults;
    BYTE rgbSomeResults[cbSomeData];
    ULONG cbDataRead;
    
    // Read various value types, none of which should be there.  On error,
    // the _Out variables should be untouched, except for cbDataRead, which
    // should be 0.
    iValueResults = iValueDefault;
    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( wszOneName, &iValueResults ) );
    CHECK( iValueResults == iValueDefault );
    
    i64ValueResults = i64ValueDefault;
    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( wszOneName, &i64ValueResults ) );
    CHECK( i64ValueResults == i64ValueDefault );

    cbDataRead = iValueDefault;
    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( wszOneName, rgbSomeResults, sizeof( rgbSomeResults ), &cbDataRead ) );
    CHECK( 0 == cbDataRead );

    // Set some binary data.
    CHECKCALLS( kvpstore.ErrKVPSetValue( wszOneName, rgbSomeData, cbALittleData ) );
    
    // Try reading again, which should only succeed when reading as a binary data.
    iValueResults = iValueDefault;
    OnNonRTM( CHECK( 0 == FNegTestSet( fInvalidUsage ) ) );
    CHECK( JET_errInvalidColumnType == kvpstore.ErrKVPGetValue( wszOneName, &iValueResults ) );
    OnNonRTM( (void)FNegTestUnset( fInvalidUsage ) );
    CHECK( iValueResults == iValueDefault );
    
    i64ValueResults = i64ValueDefault;
    OnNonRTM( CHECK( 0 == FNegTestSet( fInvalidUsage ) ) );
    CHECK( JET_errInvalidColumnType == kvpstore.ErrKVPGetValue( wszOneName, &i64ValueResults ) );
    OnNonRTM( (void)FNegTestUnset( fInvalidUsage ) );
    CHECK( i64ValueResults == i64ValueDefault );

    cbDataRead = iValueDefault;
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, rgbSomeResults, sizeof( rgbSomeResults ), &cbDataRead ) );
    CHECK( cbDataRead == cbALittleData );
    CHECK( 0 == memcmp( rgbSomeData, rgbSomeResults, cbDataRead ) );

    // Change the binary data to  much bigger binary data.
    CHECKCALLS( kvpstore.ErrKVPSetValue( wszOneName, rgbSomeData, cbSomeData ) );

    // Reread.
    iValueResults = iValueDefault;
    OnNonRTM( CHECK( 0 == FNegTestSet( fInvalidUsage ) ) );
    CHECK( JET_errInvalidColumnType == kvpstore.ErrKVPGetValue( wszOneName, &iValueResults ) );
    OnNonRTM( (void)FNegTestUnset( fInvalidUsage ) );
    CHECK( iValueResults == iValueDefault );
    
    i64ValueResults = i64ValueDefault;
    OnNonRTM( CHECK( 0 == FNegTestSet( fInvalidUsage ) ) );
    CHECK( JET_errInvalidColumnType == kvpstore.ErrKVPGetValue( wszOneName, &i64ValueResults ) );
    OnNonRTM( (void)FNegTestUnset( fInvalidUsage ) );
    CHECK( i64ValueResults == i64ValueDefault );

    cbDataRead = iValueDefault;
    CHECKCALLS( kvpstore.ErrKVPGetValue( wszOneName, rgbSomeResults, sizeof( rgbSomeResults ), &cbDataRead ) );
    CHECK( cbDataRead == cbSomeData );
    CHECK( 0 == memcmp( rgbSomeData, rgbSomeResults, cbDataRead ) );

    // Try to read into a result buffer that's NULL
    CHECK( JET_wrnBufferTruncated == kvpstore.ErrKVPGetValue( wszOneName, NULL, 0, &cbDataRead ) );
    CHECK( cbDataRead == cbSomeData ); // How much data was really there, not how much we read.

    // Try to read into a result buffer that's too small.
    CHECK( JET_wrnBufferTruncated == kvpstore.ErrKVPGetValue( wszOneName, rgbSomeResults, cbALittleData, &cbDataRead ) );
    CHECK( cbDataRead == cbSomeData ); // How much data was really there, not how much we read.
    CHECK( 0 == memcmp( rgbSomeData, rgbSomeResults, cbALittleData ) );

    // Try to change binary data to an INT64.
    OnNonRTM( CHECK( 0 == FNegTestSet( fInvalidUsage ) ) );
    CHECK( JET_errInvalidColumnType == kvpstore.ErrKVPSetValue( wszOneName, iValue1 ) );
    OnNonRTM( (void)FNegTestUnset( fInvalidUsage ) );

    // Finally, delete the data.
    CHECKCALLS( kvpstore.ErrKVPDeleteKey( wszOneName ) );

    // And try to reread.
    iValueResults = iValueDefault;
    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( wszOneName, &iValueResults ) );
    CHECK( iValueResults == iValueDefault );
    
    i64ValueResults = i64ValueDefault;
    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( wszOneName, &i64ValueResults ) );
    CHECK( i64ValueResults == i64ValueDefault );

    cbDataRead = iValueDefault;
    CHECK( JET_errRecordNotFound == kvpstore.ErrKVPGetValue( wszOneName, rgbSomeResults, sizeof( rgbSomeResults ), &cbDataRead ) );
    CHECK( 0 == cbDataRead );

    kvpstore.KVPTermStore();
}

#ifdef DEBUG
// This test uses TestInjection, which only works in Debug builds.
JETUNITTESTDB( KVPStore, UpgradeKVPInternalSchema, dwOpenDatabase )
{
    const WCHAR * wszMSysTestTableName = L"MSysTESTING_UpgradeKVPInternalSchema";
    const WCHAR * wszOneName = L"JustAKeyName";
    const WCHAR * wszAnotherName = L"TotallyDifferentKeyName";
    const WCHAR * wszANewName = L"A new name";

    //  tell CKVPStore to skip normal upgrade and create an "old" / original KVPStore table / schema ...
    //
    ErrEnableTestInjection( 43991, fTrue, JET_TestInjectConfigOverride, 100, JET_bitInjectionProbabilityPct );

    CKVPStore * pkvps = new CKVPStore( IfmpTest(), wszMSysTestTableName );
    CHECK( NULL != pkvps );

    CHECKCALLS( pkvps->ErrKVPInitStore( 1 ) );

    INT iValue = 42;

    //  Data for a column that is not present in a non-upgraded table.
    CHAR szSomeData[] = "MyMyThisIsLessThan30";
    CHAR szSomeResults[30];
    ULONG cbData = strlen( szSomeData ) + 1;
    

    //  Load it with some data and basic operations...
    //
    CHECK( JET_errRecordNotFound == pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 42 );
    CHECKCALLS( pkvps->ErrKVPSetValue( wszOneName, 2 ) );
    CHECKCALLS( pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 2 );
    CHECKCALLS( pkvps->ErrKVPSetValue( wszOneName, 34 ) );
    CHECKCALLS( pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 34 );
    CHECKCALLS( pkvps->ErrKVPSetValue( wszAnotherName, 25 ) );
    CHECKCALLS( pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 34 );
    CHECKCALLS( pkvps->ErrKVPGetValue( wszAnotherName, &iValue ) );
    CHECK( iValue == 25 );

    // Try to write a data type that isn't present in a non-upgraded schema.
    CHECK( JET_errBadColumnId == pkvps->ErrKVPSetValue( wszANewName, (BYTE*)szSomeData, cbData ) );
    
    //  Term and Destroy the KVPStore ...
    //
    pkvps->KVPTermStore();
    delete pkvps;

    //  Tell KVPStore to upgrade to current table / schema ...
    //
    ErrEnableTestInjection( 43991, fTrue, JET_TestInjectConfigOverride, 0, JET_bitInjectionProbabilityPct );

    //  Now re-initialize and UPGRADE a new KVPStore ...
    //
    pkvps = new CKVPStore( IfmpTest(), wszMSysTestTableName );
    CHECK( NULL != pkvps );

    CHECKCALLS( pkvps->ErrKVPInitStore( 1 ) );

    //  Should have old values, as table is opened, not re-created ...
    //
    CHECKCALLS( pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 34 );
    CHECKCALLS( pkvps->ErrKVPGetValue( wszAnotherName, &iValue ) );
    CHECK( iValue == 25 );

    //  Now load it with some new binary data operations ...
    //
    CHECKCALLS( pkvps->ErrKVPSetValue( wszANewName, (BYTE*)szSomeData, cbData ) );
    CHECKCALLS( pkvps->ErrKVPGetValue( wszANewName, (BYTE*)szSomeResults, sizeof( szSomeResults ) ) );

    CHECK( 0 == memcmp( szSomeData, szSomeResults, cbData ) );

    //  if we try to implicitly change types of the original name, it should balk ...
    //
    OnNonRTM( CHECK( 0 == FNegTestSet( fInvalidUsage ) ) );
    CHECK( JET_errInvalidColumnType == pkvps->ErrKVPSetValue( wszOneName, (BYTE*)szSomeData, cbData ) );
    OnNonRTM( (void)FNegTestUnset( fInvalidUsage ) );

    //  Term and Destroy the KVPStore ... again.
    //
    pkvps->KVPTermStore();
    delete pkvps;
}
#endif

//  ================================================================
//  Unit testing for CKVPStore::CKVPSCursor stuff ...
//  ================================================================

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

    //  due to not being able to control order of .dtors, must explicitly call KVPSCursorEnd().
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

    //  due to not being able to control order of .dtors, must explicitly call KVPSCursorEnd().
    kvpscur.KVPSCursorEnd();

    //  but not really empty, has 6 internal KVPs ...

    CKVPStore::CKVPSCursor * pkvpscurAll = new CKVPStore::CKVPSCursor( &kvpstore, CKVPStore::CKVPSCursor::WszAllWildcard() );
    CHECK( NULL != pkvpscurAll );
    // 6 b/c of the Major, Minor, and Update versions for the internal schema and the user schema.
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

    //  count all and then a subset of KVPs

    CKVPStore::CKVPSCursor * pkvpscurAllSalaries = new CKVPStore::CKVPSCursor( &kvpstore, L"*" );
    CHECK( NULL != pkvpscurAllSalaries );
    CHECK( 6 == CTestMatchingKeys( pkvpscurAllSalaries ) );
    //  Note: since deleting it, we don't have to call KVPSCursorEnd().
    delete pkvpscurAllSalaries;

    CKVPStore::CKVPSCursor * pkvpscurMaSalaries = new CKVPStore::CKVPSCursor( &kvpstore, L"Ma*" );
    CHECK( NULL != pkvpscurMaSalaries );
    CHECK( 3 == CTestMatchingKeys( pkvpscurMaSalaries ) );
    //  Note: since deleting it, we don't have to call KVPSCursorEnd().
    delete pkvpscurMaSalaries;

    //  Testing dumping ...
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

    //  count all user KVPs

    CKVPStore::CKVPSCursor * pkvpscurUser = new CKVPStore::CKVPSCursor( &kvpstore, CKVPStore::CKVPSCursor::WszUserWildcard() );
    CHECK( NULL != pkvpscurUser );
    CHECK( 4 == CTestMatchingKeys( pkvpscurUser ) );
    //  Note: since deleting it, we don't have to call KVPSCursorEnd().
    delete pkvpscurUser;

    //  count all internal KVPs

    CKVPStore::CKVPSCursor * pkvpscurInternal = new CKVPStore::CKVPSCursor( &kvpstore, CKVPStore::CKVPSCursor::WszInternalWildcard() );
    CHECK( NULL != pkvpscurInternal );
    // 6 b/c of the Major, Minor, and Update versions for the internal schema and the user schema.
    CHECK( 6 == CTestMatchingKeys( pkvpscurInternal ) );
    delete pkvpscurInternal;

    //  count all KVPs together

    CKVPStore::CKVPSCursor * pkvpscurAll = new CKVPStore::CKVPSCursor( &kvpstore, CKVPStore::CKVPSCursor::WszAllWildcard() );
    CHECK( NULL != pkvpscurAll );
    // combo of the above
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
            //  so middle Element2 should now be 5
            CHECKCALLS( kvpscur.ErrKVPSCursorUpdValue( iValue + 1 ) );
            CHECKCALLS( kvpscur.ErrKVPSCursorGetValue( &iValue ) );
            CHECK( ( 1 << c ) + 1 == iValue );
        }
    }

    CHECK( JET_errNoCurrentRecord == err );
    CHECK( c == 3 );

    //  due to not being able to control order of .dtors, must explicitly call KVPSCursorEnd().
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

    ULONG ulMinor = 42;
    ULONG ulUpdate = 42;

    CHECKCALLS( pkvps->ErrKVPGetUserVersion( &ulMinor, &ulUpdate ) );
    CHECK( 0 == ulMinor );
    CHECK( 0 == ulUpdate );

    INT iValue = 42;

    //  Load it with some basic data operations...
    //
    CHECK( JET_errRecordNotFound == pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 42 );
    CHECKCALLS( pkvps->ErrKVPSetValue( wszOneName, 2 ) );
    CHECKCALLS( pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 2 );

    //  Term and Destroy the KVPStore ...
    //
    pkvps->KVPTermStore();
    delete pkvps;

    //  Now re-initialize a new KVPStore ...
    //
    pkvps = new CKVPStore( IfmpTest(), wszMSysTestTableName );
    CHECK( NULL != pkvps );

    //  Try to re-init the table with a newer major version!

    CHECK( JET_errInvalidDatabaseVersion == pkvps->ErrKVPInitStore( 2 ) );

    // Shouldn't need this:
    //pkvps->KVPTermStore();
    delete pkvps;
}


JETUNITTESTDB( KVPStore, TestUserVersionUpgrade, dwOpenDatabase )
{
    const WCHAR * wszMSysTestTableName = L"MSysTESTING_KVPStoreTestUserVersionUpgrade";
    const WCHAR * wszOneName = L"JustAKeyName";

    CKVPStore * pkvps = new CKVPStore( IfmpTest(), wszMSysTestTableName );
    CHECK( NULL != pkvps );

    CHECKCALLS( pkvps->ErrKVPInitStore( 1 ) );

    ULONG ulMinor = 42;
    ULONG ulUpdate = 42;

    CHECKCALLS( pkvps->ErrKVPGetUserVersion( &ulMinor, &ulUpdate ) );
    CHECK( 0 == ulMinor );
    CHECK( 0 == ulUpdate );

    INT iValue = 42;

    //  Load it with some basic data operations...
    //
    CHECK( JET_errRecordNotFound == pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 42 );
    CHECKCALLS( pkvps->ErrKVPSetValue( wszOneName, 2 ) );
    CHECKCALLS( pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 2 );

    //  Term and Destroy the KVPStore ...
    //
    pkvps->KVPTermStore();
    delete pkvps;

    //  Now re-initialize a new KVPStore ...
    //
    pkvps = new CKVPStore( IfmpTest(), wszMSysTestTableName );
    CHECK( NULL != pkvps );

    CHECKCALLS( pkvps->ErrKVPInitStore( 1 ) );

    //  Check the initial user minor/update versions are zero / default.
    //
    CHECKCALLS( pkvps->ErrKVPGetUserVersion( &ulMinor, &ulUpdate ) );
    CHECK( 0 == ulMinor );
    CHECK( 0 == ulUpdate );

    //  Should have old values, as table is opened, not re-created ...
    //
    iValue = 42;
    CHECKCALLS( pkvps->ErrKVPGetValue( wszOneName, &iValue ) );
    CHECK( iValue == 2 );

    //  Try to upgrade the minor/update versions.
    //
    CHECKCALLS( pkvps->ErrKVPSetUserVersion( 2, 3 ) );

    //  Check the upgrade of minor/update versions worked.
    //
    CHECKCALLS( pkvps->ErrKVPGetUserVersion( &ulMinor, &ulUpdate ) );
    CHECK( 2 == ulMinor );
    CHECK( 3 == ulUpdate );

    //  Term and Destroy the KVPStore ... again.
    //
    pkvps->KVPTermStore();
    delete pkvps;

    //  Now re-initialize a new KVPStore ...
    //
    pkvps = new CKVPStore( IfmpTest(), wszMSysTestTableName );
    CHECK( NULL != pkvps );

    CHECKCALLS( pkvps->ErrKVPInitStore( 1 ) );

    //  Check the upgrade of minor/update versions stuck (through uninit/init)
    //
    CHECKCALLS( pkvps->ErrKVPGetUserVersion( &ulMinor, &ulUpdate ) );
    CHECK( 2 == ulMinor );
    CHECK( 3 == ulUpdate );

    //  Term and Destroy the KVPStore ... finally.
    //
    pkvps->KVPTermStore();
    delete pkvps;
}


//  ================================================================
//  Emulation of MSysLocales / LCID+Version Table Requirements
//  ================================================================

JETUNITTESTDB( KVPStoreForMSysLocales, ExpectedLCIDTableUsage, dwOpenDatabase )
{
    CKVPStore * pkvps = new CKVPStore( IfmpTest(), L"MSysTESTING_MSysLocales" );

    const ULONG ulMSysLocalesMajorVersion = 1;

    CHECKCALLS( pkvps->ErrKVPInitStore( ulMSysLocalesMajorVersion ) );

    WCHAR wszLcidVerKey[60];

    //  Start emulating LCID table activity ...
    //

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

    OSStrCbFormatW( wszLcidVerKey, sizeof( wszLcidVerKey ), L"LCID=%d,Ver=%I64x", 1031, qwGermanSortVersion );  //  german
    CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, +1 ) );

    OSStrCbFormatW( wszLcidVerKey, sizeof( wszLcidVerKey ), L"LCID=%d,Ver=%I64x", 1033, qwEnglishSortVersion ); //  english
    CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, +1 ) );
    CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, +1 ) );
    CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, -1 ) );
    CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, +1 ) );
    CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, +1 ) );
    CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, +1 ) );

    OSStrCbFormatW( wszLcidVerKey, sizeof( wszLcidVerKey ), L"LCID=%d,Ver=%I64x", 1040, qwItalianSortVersion ); //  italian
    CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, +1 ) );
    CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, +1 ) );

    OSStrCbFormatW( wszLcidVerKey, sizeof( wszLcidVerKey ), L"LCID=%d,Ver=%I64x", 1033, qwEnglishSortVersion ); //  english (again)
    CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, +1 ) );

    //  emulating an out of date (in the _future_ index)

    OSStrCbFormatW( wszLcidVerKey, sizeof( wszLcidVerKey ), L"LCID=%d,Ver=%I64x", 1033, qwEnglishSortVersion + 4 ); //  english (again), new version
    CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, +1 ) );

    //  Enumerate all English LCID Version indices
    //

    OSStrCbFormatW( wszLcidVerKey, sizeof( wszLcidVerKey ), L"LCID=%d,*", 1033 );   //  search for english all versions
    CKVPStore::CKVPSCursor * pkvpscurAllEnglishIndices = new CKVPStore::CKVPSCursor( pkvps, wszLcidVerKey );
    CHECK( NULL != pkvpscurAllEnglishIndices );

    CHECK( 2 == CTestMatchingKeys( pkvpscurAllEnglishIndices ) );       //  2 versions of english indices
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
    CHECK( 6 == cEnglishIndices );  // added 5, subtracted 1, added another, then added a final one for a different version

    CHECKCALLS( pkvpscurAllEnglishIndices->ErrKVPSCursorReset() );
    CHECK( 6 == CTestAccumulatedValues( pkvpscurAllEnglishIndices ) );  // should yield the same

    delete pkvpscurAllEnglishIndices;

    //  Enumerate all Locales and versions we track, noting out of dateness ...
    //

    CKVPStore::CKVPSCursor * pkvpscurAllIndices = new CKVPStore::CKVPSCursor( pkvps, L"LCID=*" );
    CHECK( NULL != pkvpscurAllIndices );

    CHECK( 4 == CTestMatchingKeys( pkvpscurAllIndices ) );      //  2 versions of english, 1 german, and 1 italian
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
        Expected( *pwszNull == L'\0' ); // expect that's the end of the key.

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

    //  Validate we can open the table ...
    //

    JET_SESID sesid;
    JET_DBID dbid;
    JET_TABLEID cursor;
    CHECKCALLS( ErrKVPStoreTestGetTable( IfmpTest(), wszMSysTestTableName, &sesid, &dbid, &cursor ) );

    CPG cpgOwned;
    CPG cpgAvail;

    CHECKCALLS( JetGetTableInfoW( sesid, cursor, &cpgOwned, sizeof( cpgOwned ), JET_TblInfoSpaceOwned ) );
    CHECKCALLS( JetGetTableInfoW( sesid, cursor, &cpgAvail, sizeof( cpgAvail ), JET_TblInfoSpaceAvailable ) );

    // Test space usage ...
    CHECK( cpgOwned == 1 );
    CHECK( cpgAvail == 0 );

    //  Start filling the page with random LCIDs
    //

    ULONG i;
    for ( i = 0; i < INT_MAX; i++ )
    {
        LCID lcid = ( ( rand() % 256 + 1 ) << 24 ) + rand();
        OSStrCbFormatW( wszLcidVerKey, sizeof( wszLcidVerKey ), L"LCID=%d,Ver=%I64x", lcid, 0xFE24012C01AFEFEF);    //  german
        CHECKCALLS( pkvps->ErrKVPAdjValue( wszLcidVerKey, +1 ) );

        CPG cpgOwnedNew = 0;
        CHECKCALLS( JetGetTableInfoW( sesid, cursor, &cpgOwnedNew, sizeof( cpgOwnedNew ), JET_TblInfoSpaceOwned ) );
        if ( cpgOwnedNew != cpgOwned )
        {
            wprintf(L"Hit after : %d th add ... cpgOwnedNew = %d\n", i, cpgOwnedNew );
            break;
        }
    }
    CHECK( i > 10 );    // should be able to fit at least 10 ...
    //  Estimated the average record size to be between 130 and 170 bytes ... 18 fit on a 4 KB page with the
    //  above badly-formatted (too big... LCIDs are normally like 1033, 1040, etc) LCIDs.  32 KB pages should
    //  be able to fit over 100 LCID records.

    pkvps->Dump( CPRINTFSTDOUT::PcprintfInstance(), fTrue );

    CHECKCALLS( ErrKVPStoreTestGetRidOfTable( sesid, dbid, cursor ) );

    pkvps->KVPTermStore();
    delete pkvps;
}

#endif // ENABLE_JET_UNIT_TEST

