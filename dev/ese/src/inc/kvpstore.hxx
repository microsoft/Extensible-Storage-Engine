// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  ================================================================
//  Key-Value Pair Store.
//  ================================================================

//  This module implements store that supports a loosely-schematized 
//  key and semi-strongly typed value pair that can handle 3 basic 
//  types: LONG, LONGLONG, and BYTE[].
//
//  Any key of 111 WCHARs (w/o NUL) or less is accepted, as long as 
//  it does not begin with a ".", which is reserved for internal 
//  KVPStore keys. The key is stored in Unicode, but no text 
//  normalization is performed... this is intended for code "paths", 
//  not user data.
//
//  It favors simplicity over concurrency, serializing all updates,
//  however has enough flexibility built in, to be able to extend
//  past this limitation if needed.
//
//  It favors simplicity over space usage, storing most likely ASCII
//  keys as BYTE[].
//
//  Has upgrade support facilities, in case the usage of the 
//  KVPStore needs to evolve.
//

class CKVPStore : CZeroInit
{

private:
    //  --------------------------------------------------------------------------
    //
    //      Basic Identity Information
    //

    //  DB and table name
    //
    IFMP                        m_ifmp;

    PERSISTED WCHAR             p_wszTableName[JET_cbNameMost];

    //  --------------------------------------------------------------------------
    //
    //      .Schema Versions
    //

    //   Versions:
    //   1.0.0 - Initial release
    //   1.0.1 - Added LongBinary value type.    JET_efvKVPStoreV2 transitory upgrade state.
    //   1.0.2 - Added LongLong value type.      JET_efvKVPStoreV2 end upgrade state.

    //  Updates to the internal schema of the KVPStore
    //
    PERSISTED static const ULONG  p_ulUpdateAddLongBinaryValueType = 1;
    PERSISTED static const ULONG  p_ulUpdateAddLongLongValueType = 2;

    // Minor version upgrades to the internal schema of the KVPStore
    // None yet.

    // No support for major version upgrades yet.

    //  Maximum supported internal upgrades and internal .schema versions
    //  The initial release was version 1.0.0.
    PERSISTED static const ULONG  p_ulInternalMajorVersion  = 1;
    PERSISTED static const ULONG  p_ulInternalMinorVersion  = 0;
    PERSISTED static const ULONG  p_ulInternalUpdateVersion = p_ulUpdateAddLongLongValueType;

    // Current supported internal upgrades and internal .schema versions
    ULONG  m_ulInternalMajorVersion;
    ULONG  m_ulInternalMinorVersion;
    ULONG  m_ulInternalUpdateVersion;

    //  --------------------------------------------------------------------------
    //
    //      Schema Data and Information
    //

    //  protected key begin
    //
    static const WCHAR      s_wchSecret = L'.';

    //  key column information
    //
    JET_COLUMNID            m_cidKey;

    //  type column information
    //
    JET_COLUMNID            m_cidType;

    //  data column information (indexed by type enum)
    //
    static const ULONG      cbKVPIVariableDataSize = 0;

    PERSISTED typedef enum _ValueType
    {
        kvpvtInvalidType = 0,       //

        kvpvtSchemaValueType,       //  internal schema value (LONG)

        kvpvtLongValueType,         //  LONG value

        kvpvtLongBinaryValueType,   //  Binary data (stored as LVs so > 255 bytes is OK).
        
        kvpvtLongLongValueType,     //  LONGLONG value

        kvpvtMaxValueType,          //  <sentinel>

    } KVPIValueType;

    struct KVPIDataColInfo
    {
        KVPIValueType       m_type;      // index variable...
        JET_COLUMNID        m_cid;       // Column ID of column in table.
        JET_COLTYP          m_coltyp;    // Column type to use when creating column.
        JET_GRBIT           m_grbit;     // Grbit to use when creating column.
        ULONG               m_cbDataCol; // Size of data storable in column.
        ULONG               m_ulMajor;   // What version of the table
        ULONG               m_ulMinor;   //    -    introduced this
        ULONG               m_ulUpdate;  //    -    schema element?
        PCSTR               m_szName;
    };
    struct KVPIDataColInfo  m_rgDataTypeInfo[kvpvtMaxValueType] =
    {
        {
            kvpvtInvalidType,
            JET_columnidNil,
            0,
            NO_GRBIT,
            cbKVPIVariableDataSize,
            1, 0, 0,
            "Invalid"
        },
        {
            // Note that this data type is stored in the same column as kvpvtLongValueType,
            // we know what we're reading from context.
            kvpvtSchemaValueType,
            JET_columnidNil,
            JET_coltypLong,
            ( JET_bitColumnFixed | JET_bitColumnEscrowUpdate ),
            sizeof( LONG ),
            1, 0, 0,
            "iValue"
        },
        {
            kvpvtLongValueType,
            JET_columnidNil,
            JET_coltypLong,
            ( JET_bitColumnFixed | JET_bitColumnEscrowUpdate ),
            sizeof( LONG ),
            1, 0, 0,
            "iValue"
        },
        {
            kvpvtLongBinaryValueType,
            JET_columnidNil,
            JET_coltypLongBinary,
            JET_bitColumnTagged,
            cbKVPIVariableDataSize,
            1, 0, p_ulUpdateAddLongBinaryValueType,
            "rgbValue"
        },
        {
            kvpvtLongLongValueType,
            JET_columnidNil,
            JET_coltypCurrency,
            JET_bitColumnVariable,
            sizeof( LONGLONG ),
            1, 0, p_ulUpdateAddLongLongValueType,
            "i64Value"
        }
    };

    //  --------------------------------------------------------------------------
    //
    //      Internal Initialization
    //

    //  create table if not existing, and initialize 
    //
    
    ERR ErrKVPIInitIBootstrapCriticalCOLIDs();
    ERR ErrKVPIInitICreateTable( PIB * ppib, const IFMP ifmp, const CHAR * const szTableName, const ULONG ulMajorVersionExpected );
    ERR ErrKVPIAddValueTypeToSchema( const KVPIValueType kvpvt );
    ERR ErrKVPIInitIUpgradeTable( const IFMP ifmp, bool fReadOnly );
    ERR ErrKVPIInitILoadValueTypeCOLIDs();


    //  --------------------------------------------------------------------------
    //
    //      Active Transaction Management
    //

    //  session, table cursor, and status information
    //
    CCriticalSection        m_critGlobalTableLock;
    PIB *                   m_ppibGlobalLock;
    FUCB *                  m_pfucbGlobalLock;

    ERR ErrKVPIOpenGlobalPibAndFucb();
    VOID KVPICloseGlobalPibAndFucb();

    typedef enum _TrxPosition   { eInvalidTrxPosition = 0,  eNewTrx, eHasTrx }          TrxPosition;

public:
    //  constants for API
    //

    typedef enum _TrxUpdate     { eInvalidTrxUpdate = 0,    eReadOnly, eReadWrite }     TrxUpdate;

private:

    ERR ErrKVPIBeginTrx( const TrxUpdate eTrxUpd, TRXID trxid );
    ERR ErrKVPIPrepUpdate( const WCHAR * const wszKey, KVPIValueType eValueType );
    ERR ErrKVPIEndTrx( const ERR errRet );
    ERR ErrKVPIAcquireTrx( const TrxUpdate eTrxUpd, __inout PIB * ppib, TRXID trxid );
    ERR ErrKVPIReleaseTrx( const ERR errRet );

    //  Internal DML operations on the KVP Table
    //

    ERR ErrKVPIDeleteKey(
        const TrxPosition eTrxPos,
        const WCHAR * const wszKey );

    ERR ErrKVPISetValue(
        const TrxPosition eTrxPos,
        const WCHAR * const wszKey,
        const KVPIValueType eValueType,
        _In_opt_bytecount_( cbValue ) const BYTE * const pbValue,
        const ULONG cbValue );

    ERR ErrKVPIGetValue(
        const TrxPosition eTrxPos,
        const WCHAR * const wszKey,
        const KVPIValueType kvpvt,
        _Out_writes_bytes_to_( cbValue, min( cbValue, *pcbActual ) ) BYTE * const pbValue,
        const ULONG cbValue,
        _Out_opt_ ULONG *pcbActual = NULL );

    //  Internal Validation
    //

    VOID AssertKVPIValidUserKey( const KVPIValueType eValueType, const WCHAR * const wszKey );

public:

    //  --------------------------------------------------------------------------
    //
    //      API
    //

    //  .ctor and init / term
    //

    CKVPStore( IFMP ifmp, const WCHAR * const wszTableName );
    ERR ErrKVPInitStore( PIB * const ppibProvided, const TrxUpdate eTrxUpd, const ULONG ulMajorVersionExpected );
    ERR ErrKVPInitStore( const ULONG ulMajorVersionExpected )   { return ErrKVPInitStore( NULL, eReadWrite, ulMajorVersionExpected ); }
    VOID KVPTermStore();
    ~CKVPStore( );

    //  user version control
    //

    ERR ErrKVPGetUserVersion( _Out_ ULONG * pulMinorVersion, _Out_ ULONG * pwUpdateVersion );
    ERR ErrKVPSetUserVersion( const ULONG ulMinorVersion, const ULONG ulUpdateVersion );

    //  key limitations
    //

    static const INT cbKVPMaxKey = ( 111 + 1 /* NUL */ ) * sizeof(WCHAR);   // 111 WCHARs + a NUL character ...
    INT CbKVPKeySize( const WCHAR * const wszKey ) const;

    //  setting values
    //

    ERR ErrKVPSetValue( const WCHAR * const wszKey, const INT iValue );
    ERR ErrKVPSetValue( const WCHAR * const wszKey, const INT64 i64Value );
    ERR ErrKVPSetValue( const WCHAR * const wszKey, const BYTE * const pbValue, const ULONG cbValue );
    ERR ErrKVPAdjValue( const WCHAR * const wszKey, const INT iAdjustment );
    ERR ErrKVPAdjValue( __inout PIB * ppibProvided, const WCHAR * const wszKey, const INT iAdjustment );

    //  getting values
    //
    
    ERR ErrKVPGetValue( const WCHAR * const wszKey, _Out_ INT * piValue );
    ERR ErrKVPGetValue( const WCHAR * const wszKey, _Out_ INT64 * pi64Value );
    ERR ErrKVPGetValue( const WCHAR * const wszKey, _Out_writes_bytes_to_( cbValueMax, min( cbValueMax, *pcbValueActual ) ) BYTE * const pbValue, const ULONG cbValueMax, _Out_opt_ ULONG *pcbValueActual = NULL );

    //  deleting keys
    //

    ERR ErrKVPDeleteKey( const WCHAR * const wszKey );

    //  debugging
    //

    VOID Dump( CPRINTF* pcprintf, const BOOL fInternalToo );

    //  enumerating values
    //

    class CKVPSCursor
    {

        friend VOID CKVPStore::Dump( CPRINTF* pcprintf, const BOOL fInternalToo );

    private:
        //  States for cursor
        //
        typedef enum _KVPSCursorIState {
                kvpscUninit = 0,
                kvpscAcquiredTrx,
                kvpscEnumerating,
        } KVPSCursorIState;

        //  Internal management state
        //
        CKVPStore *         m_pkvps;
        KVPSCursorIState    m_eState;
        WCHAR               m_wszWildKey[cbKVPMaxKey/sizeof(WCHAR)];
        WCHAR               m_wszCurrentKey[cbKVPMaxKey/sizeof(WCHAR)]; // ~200 bytes, pretty stiff

        //  Internal management routines
        //
        BOOL FKVPSCursorIMatches() const;
        ERR ErrCKVPSCursorIEstablishCurrency();

        //  Internal wildcard check routines
        //
        static BOOL FKVPSCursorIUserWildcard( const WCHAR * const wszKey )
        {
            return wszKey[0] == L'*' && wszKey[1] == L'\0';
        }
        static BOOL FKVPSCursorIInternalWildcard( const WCHAR * const wszKey )
        {
            Assert( L'.' == CKVPStore::s_wchSecret );
            return wszKey[0] == L'.' && wszKey[1] == L'*' && wszKey[2] == L'\0';
        }
        static BOOL FKVPSCursorIAllWildcard( const WCHAR * const wszKey )
        {
            return wszKey[0] == L'*' && wszKey[1] == L'*' && wszKey[2] == L'\0';
        }

    public:

        //  --------------------------------------------------------------------------
        //
        //      API
        //

        //  Wildcard Keys
        //

        static WCHAR * WszUserWildcard()        { WCHAR * wsz = L"*";  Assert( FKVPSCursorIUserWildcard( wsz ) );     return wsz; }
        static WCHAR * WszInternalWildcard()    { WCHAR * wsz = L".*"; Assert( FKVPSCursorIInternalWildcard( wsz ) ); return wsz; }
        static WCHAR * WszAllWildcard()         { WCHAR * wsz = L"**"; Assert( FKVPSCursorIAllWildcard( wsz ) );      return wsz; }
        //  OR: your own key with an optiona * at the end for prefix-matching.

        //  .ctor and initialization    
        //

        CKVPSCursor( CKVPStore * const pkvps, __in_z const WCHAR * const wszWildKey );
        ~CKVPSCursor();

        //  enumerating entries
        //

        ERR ErrKVPSCursorReset();
        ERR ErrKVPSCursorLocateNextKVP();
        VOID KVPSCursorEnd();

        //  retrieving properties
        //

        const WCHAR * WszKVPSCursorCurrKey() const;
        ERR ErrKVPSCursorGetValue( _Out_ INT * piValue );
    protected:
        // something to get the type for Dump().
        ERR ErrKVPSCursorIGetType( CKVPStore::KVPIValueType * const pkvpvt );

    public:

        //  updating properties
        //

        ERR ErrKVPSCursorUpdValue( _In_ INT iValue );

    };

    friend class CKVPSCursor;

};


INLINE BOOL FKVPTestTable( __in_z CHAR * szTable )
{
#ifdef DEBUG
    return 0 == _strnicmp( szTable, "MSysTESTING_", sizeof("MSysTESTING_")-1 );
#else
    return fFalse;
#endif
}


