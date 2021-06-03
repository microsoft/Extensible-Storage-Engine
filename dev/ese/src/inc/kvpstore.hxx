// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  ================================================================
//  Key-Value Pair Store.
//  ================================================================

//  This module implements store that supports a loosely-schematized 
//  key and semi-strongly typed value pair that can handle 3 basic 
//  types: int, qword, and blob.
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
//  keys as Unicode.
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

    //  upgrades to the internal schema of the KVPStore
    //
#ifdef FUTURE_SUPPORT_MORE_VALUE_TYPES
    PERSISTED static const INT  p_dwMinorAddBlobValueType = 1;
    PERSISTED static const INT  p_dwUpdateAddQwordValueType = 1;
#endif

    //  internal upgrades and internal .schema versions
    //
    PERSISTED static const INT  p_dwInternalMajorVersion        = 1;                            //  initial value 1
#ifdef FUTURE_SUPPORT_MORE_VALUE_TYPES
    PERSISTED static const INT  p_dwInternalMinorVersion        = p_dwMinorAddBlobValueType;    //  initial value 0
    PERSISTED static const INT  p_dwInternalUpdateVersion       = p_dwUpdateAddQwordValueType;  //  initial value 0
#else
    PERSISTED static const INT  p_dwInternalMinorVersion        = 0;                            //  initial value 0
    PERSISTED static const INT  p_dwInternalUpdateVersion       = 0;                            //  initial value 0
#endif

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

    PERSISTED typedef enum _ValueType
    {
        kvpvtInvalidType = 0,       //

        kvpvtSchemaValueType,       //  internal schema value

        kvpvtIntValueType,          //  standard integer value
#ifdef FUTURE_SUPPORT_MORE_VALUE_TYPES
        kvpvtQwordValueType,        //  large integer value
        kvpvtBlobValueType,         //  data blobs
#endif

        kvpvtMaxValueType,          //  <sentinel>

    } KVPIValueType;

    //  data column information (indexed by type enum)
    //
    static const INT        cbKVPIVariableDataSize = 0;
    struct KVPIDataColInfo
    {
        KVPIValueType       m_type; // index variable...
        JET_COLUMNID        m_cid;
        INT                 m_cbDataCol;
    };
    struct KVPIDataColInfo  m_rgDataTypeInfo[kvpvtMaxValueType];


    //  --------------------------------------------------------------------------
    //
    //      Internal Initialization
    //

    //  create table if not existing, and initialize 
    //
    
    ERR ErrKVPIInitIBootstrapCriticalCOLIDs();
    ERR ErrKVPIInitICreateTable( PIB * ppib, const IFMP ifmp, const CHAR * const szTableName, const DWORD dwMajorVersionExpected );
#ifdef FUTURE_SUPPORT_MORE_VALUE_TYPES
    ERR ErrKVPIInitIUpgradeIAddBlobValueType( const DWORD dwUpgradeVersion );
    ERR ErrKVPIInitIUpgradeIAddQwordValueType( const DWORD dwUpgradeVersion );
#endif
    ERR ErrKVPIInitIUpgradeTable();
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

    ERR ErrKVPISetValue( const TrxPosition eTrxPos, const WCHAR * const wszKey, const KVPIValueType eValueType, _In_opt_bytecount_( cbValue ) const BYTE * const pbValue, const INT cbValue );
    ERR ErrKVPIGetValue( const TrxPosition eTrxPos, const WCHAR * const wszKey, const KVPIValueType eValueType, _Out_bytecap_( cbValue ) BYTE * const pbValue, const INT cbValue );

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
    ERR ErrKVPInitStore( PIB * const ppibProvided, const TrxUpdate eTrxUpd, const DWORD dwMajorVersionExpected );
    ERR ErrKVPInitStore( const DWORD dwMajorVersionExpected )   { return ErrKVPInitStore( NULL, eReadWrite, dwMajorVersionExpected ); }
    VOID KVPTermStore();
    ~CKVPStore( );

    //  user version control
    //

    ERR ErrKVPGetUserVersion( __out DWORD * pdwMinorVersion, __out DWORD * pwUpdateVersion );
    ERR ErrKVPSetUserVersion( const DWORD dwMinorVersion, const DWORD dwUpdateVersion );

    //  key limitations
    //

    static const INT cbKVPMaxKey = ( 111 + 1 /* NUL */ ) * sizeof(WCHAR);   // 111 WCHARs + a NUL character ...
    INT CbKVPKeySize( const WCHAR * const wszKey ) const;

    //  setting values
    //

    ERR ErrKVPSetValue( const WCHAR * const wszKey, const INT iValue );
#ifdef FUTURE_SUPPORT_MORE_VALUE_TYPES
    ERR ErrKVPSetValue( const WCHAR * const wszKey, const INT64 i64Value );
    ERR ErrKVPSetValue( const WCHAR * const wszKey, const BYTE * const pbValue, const ULONG cbValue );
#endif
    ERR ErrKVPAdjValue( const WCHAR * const wszKey, const INT iAdjustment );
    ERR ErrKVPAdjValue( __inout PIB * ppibProvided, const WCHAR * const wszKey, const INT iAdjustment );

    //  getting values
    //
    
    ERR ErrKVPGetValue( const WCHAR * const wszKey, __out INT * piValue );
#ifdef FUTURE_SUPPORT_MORE_VALUE_TYPES
    ERR ErrKVPGetValue( const WCHAR * const wszKey, __out INT64 * pi64Value );
    ERR ErrKVPGetValue( const WCHAR * const wszKey, __out BYTE * const pbValue, const ULONG cbValue );
#endif

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
        ERR ErrKVPSCursorGetValue( __out INT * piValue );
    protected:
        // something to get the type for Dump().
        ERR ErrKVPSCursorIGetType( CKVPStore::KVPIValueType * const pkvpvt );

    public:

        //  updating properties
        //

        ERR ErrKVPSCursorUpdValue( __in INT iValue );

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


