// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.




class CKVPStore : CZeroInit
{

private:

    IFMP                        m_ifmp;

    PERSISTED WCHAR             p_wszTableName[JET_cbNameMost];


#ifdef FUTURE_SUPPORT_MORE_VALUE_TYPES
    PERSISTED static const INT  p_dwMinorAddBlobValueType = 1;
    PERSISTED static const INT  p_dwUpdateAddQwordValueType = 1;
#endif

    PERSISTED static const INT  p_dwInternalMajorVersion        = 1;
#ifdef FUTURE_SUPPORT_MORE_VALUE_TYPES
    PERSISTED static const INT  p_dwInternalMinorVersion        = p_dwMinorAddBlobValueType;
    PERSISTED static const INT  p_dwInternalUpdateVersion       = p_dwUpdateAddQwordValueType;
#else
    PERSISTED static const INT  p_dwInternalMinorVersion        = 0;
    PERSISTED static const INT  p_dwInternalUpdateVersion       = 0;
#endif


    static const WCHAR      s_wchSecret = L'.';

    JET_COLUMNID            m_cidKey;

    JET_COLUMNID            m_cidType;

    PERSISTED typedef enum _ValueType
    {
        kvpvtInvalidType = 0,

        kvpvtSchemaValueType,

        kvpvtIntValueType,
#ifdef FUTURE_SUPPORT_MORE_VALUE_TYPES
        kvpvtQwordValueType,
        kvpvtBlobValueType,
#endif

        kvpvtMaxValueType,

    } KVPIValueType;

    static const INT        cbKVPIVariableDataSize = 0;
    struct KVPIDataColInfo
    {
        KVPIValueType       m_type;
        JET_COLUMNID        m_cid;
        INT                 m_cbDataCol;
    };
    struct KVPIDataColInfo  m_rgDataTypeInfo[kvpvtMaxValueType];



    
    ERR ErrKVPIInitIBootstrapCriticalCOLIDs();
    ERR ErrKVPIInitICreateTable( PIB * ppib, const IFMP ifmp, const CHAR * const szTableName, const DWORD dwMajorVersionExpected );
#ifdef FUTURE_SUPPORT_MORE_VALUE_TYPES
    ERR ErrKVPIInitIUpgradeIAddBlobValueType( const DWORD dwUpgradeVersion );
    ERR ErrKVPIInitIUpgradeIAddQwordValueType( const DWORD dwUpgradeVersion );
#endif
    ERR ErrKVPIInitIUpgradeTable();
    ERR ErrKVPIInitILoadValueTypeCOLIDs();



    CCriticalSection        m_critGlobalTableLock;
    PIB *                   m_ppibGlobalLock;
    FUCB *                  m_pfucbGlobalLock;

    ERR ErrKVPIOpenGlobalPibAndFucb();
    VOID KVPICloseGlobalPibAndFucb();

    typedef enum _TrxPosition   { eInvalidTrxPosition = 0,  eNewTrx, eHasTrx }          TrxPosition;

public:

    typedef enum _TrxUpdate     { eInvalidTrxUpdate = 0,    eReadOnly, eReadWrite }     TrxUpdate;

private:

    ERR ErrKVPIBeginTrx( const TrxUpdate eTrxUpd, TRXID trxid );
    ERR ErrKVPIPrepUpdate( const WCHAR * const wszKey, KVPIValueType eValueType );
    ERR ErrKVPIEndTrx( const ERR errRet );
    ERR ErrKVPIAcquireTrx( const TrxUpdate eTrxUpd, __inout PIB * ppib, TRXID trxid );
    ERR ErrKVPIReleaseTrx( const ERR errRet );


    ERR ErrKVPISetValue( const TrxPosition eTrxPos, const WCHAR * const wszKey, const KVPIValueType eValueType, _In_opt_bytecount_( cbValue ) const BYTE * const pbValue, const INT cbValue );
    ERR ErrKVPIGetValue( const TrxPosition eTrxPos, const WCHAR * const wszKey, const KVPIValueType eValueType, _Out_bytecap_( cbValue ) BYTE * const pbValue, const INT cbValue );


    VOID AssertKVPIValidUserKey( const KVPIValueType eValueType, const WCHAR * const wszKey );

public:



    CKVPStore( IFMP ifmp, const WCHAR * const wszTableName );
    ERR ErrKVPInitStore( PIB * const ppibProvided, const TrxUpdate eTrxUpd, const DWORD dwMajorVersionExpected );
    ERR ErrKVPInitStore( const DWORD dwMajorVersionExpected )   { return ErrKVPInitStore( NULL, eReadWrite, dwMajorVersionExpected ); }
    VOID KVPTermStore();
    ~CKVPStore( );


    ERR ErrKVPGetUserVersion( __out DWORD * pdwMinorVersion, __out DWORD * pwUpdateVersion );
    ERR ErrKVPSetUserVersion( const DWORD dwMinorVersion, const DWORD dwUpdateVersion );


    static const INT cbKVPMaxKey = ( 111 + 1  ) * sizeof(WCHAR);
    INT CbKVPKeySize( const WCHAR * const wszKey ) const;


    ERR ErrKVPSetValue( const WCHAR * const wszKey, const INT iValue );
#ifdef FUTURE_SUPPORT_MORE_VALUE_TYPES
    ERR ErrKVPSetValue( const WCHAR * const wszKey, const INT64 i64Value );
    ERR ErrKVPSetValue( const WCHAR * const wszKey, const BYTE * const pbValue, const ULONG cbValue );
#endif
    ERR ErrKVPAdjValue( const WCHAR * const wszKey, const INT iAdjustment );
    ERR ErrKVPAdjValue( __inout PIB * ppibProvided, const WCHAR * const wszKey, const INT iAdjustment );

    
    ERR ErrKVPGetValue( const WCHAR * const wszKey, __out INT * piValue );
#ifdef FUTURE_SUPPORT_MORE_VALUE_TYPES
    ERR ErrKVPGetValue( const WCHAR * const wszKey, __out INT64 * pi64Value );
    ERR ErrKVPGetValue( const WCHAR * const wszKey, __out BYTE * const pbValue, const ULONG cbValue );
#endif


    VOID Dump( CPRINTF* pcprintf, const BOOL fInternalToo );


    class CKVPSCursor
    {

        friend VOID CKVPStore::Dump( CPRINTF* pcprintf, const BOOL fInternalToo );

    private:
        typedef enum _KVPSCursorIState {
                kvpscUninit = 0,
                kvpscAcquiredTrx,
                kvpscEnumerating,
        } KVPSCursorIState;

        CKVPStore *         m_pkvps;
        KVPSCursorIState    m_eState;
        WCHAR               m_wszWildKey[cbKVPMaxKey/sizeof(WCHAR)];
        WCHAR               m_wszCurrentKey[cbKVPMaxKey/sizeof(WCHAR)];

        BOOL FKVPSCursorIMatches() const;
        ERR ErrCKVPSCursorIEstablishCurrency();

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



        static WCHAR * WszUserWildcard()        { WCHAR * wsz = L"*";  Assert( FKVPSCursorIUserWildcard( wsz ) );     return wsz; }
        static WCHAR * WszInternalWildcard()    { WCHAR * wsz = L".*"; Assert( FKVPSCursorIInternalWildcard( wsz ) ); return wsz; }
        static WCHAR * WszAllWildcard()         { WCHAR * wsz = L"**"; Assert( FKVPSCursorIAllWildcard( wsz ) );      return wsz; }


        CKVPSCursor( CKVPStore * const pkvps, __in_z const WCHAR * const wszWildKey );
        ~CKVPSCursor();


        ERR ErrKVPSCursorReset();
        ERR ErrKVPSCursorLocateNextKVP();
        VOID KVPSCursorEnd();


        const WCHAR * WszKVPSCursorCurrKey() const;
        ERR ErrKVPSCursorGetValue( __out INT * piValue );
    protected:
        ERR ErrKVPSCursorIGetType( CKVPStore::KVPIValueType * const pkvpvt );

    public:


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


