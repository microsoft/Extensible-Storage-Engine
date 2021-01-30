// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#include "PageSizeClean.hxx"



class CPRINTFBUFFER : public CPRINTF
{
public:
    CPRINTFBUFFER() { m_szBuffer[0] = 0; }
    virtual ~CPRINTFBUFFER() {}

    const char * SzBuffer() const { return m_szBuffer; }
    void __cdecl operator()( const _TCHAR* szFormat, ... )
    {
        va_list arg_ptr;
        va_start( arg_ptr, szFormat );
        const size_t cchBufferUsed = strlen( m_szBuffer );
        StringCbVPrintfA( m_szBuffer + cchBufferUsed, m_cchBuffer - cchBufferUsed, szFormat, arg_ptr );
        va_end( arg_ptr );
    }

private:
    static const size_t m_cchBuffer = 1024;
    char m_szBuffer[m_cchBuffer];
};


void UtilPrintColumnValue(
        const void * const pv,
        const size_t cb,
        const JET_COLTYP coltyp,
              CPRINTF * const pcprintf )
{
    if( NULL == pv || 0 == cb )
    {
        ( *pcprintf )("<null>" );
    }
    else
    {
        switch( coltyp )
        {
            case JET_coltypUnsignedByte:
                Assert( sizeof( BYTE ) == cb );
                ( *pcprintf )("0x%X", *( BYTE * )pv );
                break;
            case JET_coltypShort:
                Assert( sizeof( SHORT ) == cb );
                ( *pcprintf )("%d", *( SHORT * )pv );
                break;
            case JET_coltypUnsignedShort:
                Assert( sizeof( USHORT ) == cb );
                ( *pcprintf )("0x%X", *( USHORT * )pv );
                break;
            case JET_coltypLong:
                Assert( sizeof( LONG ) == cb );
                ( *pcprintf )("%d", *( LONG * )pv );
                break;
            case JET_coltypUnsignedLong:
                Assert( sizeof( ULONG ) == cb );
                ( *pcprintf )("0x%X", *( ULONG * )pv );
                break;
            case JET_coltypLongLong:
                Assert( sizeof( __int64 ) == cb );
                ( *pcprintf )("0x%I64X", *( __int64 * )pv );
                break;
            case JET_coltypUnsignedLongLong:
                Assert( sizeof( __int64 ) == cb );
                ( *pcprintf )("0x%I64X", *( __int64 * )pv );
                break;
            case JET_coltypLongText:
                ( *pcprintf )("%.*s", cb, ( char * )pv );
                break;
            default:
                for( size_t i = 0; i < cb; ++i )
                {
                    const BYTE * const pb = ( BYTE * )pv;
                    ( *pcprintf )("%2.2X", pb[i] );
                    if( i != ( cb-1 ) )
                    {
                        ( *pcprintf )(" " );
                    }
                }
                break;
        }
    }
}



namespace TableDataStoreFactory
{
    ERR ErrOpen_(
        INST * const pinst,
        const wchar_t * wszDatabase,
        const char * const szTable,
        const bool fCreateIfNotFound,
        IDataStore ** ppstore );
    
    ERR ErrCreateTable_(
        const JET_SESID sesid,
        const JET_DBID dbid,
        const char * const szTable );
    
    ERR ErrCreateRecord_( const JET_SESID sesid, const JET_TABLEID tableid );
}


class TableDataStore : public IDataStore
{
public:
    virtual ~TableDataStore();

    ERR ErrColumnExists(
        const char * const szColumn,
        __out bool * const pfExists,
        __out JET_COLTYP * const pcoltyp ) const;
    ERR ErrCreateColumn( const char * const szColumn, const JET_COLTYP coltyp );
    
    ERR ErrLoadDataFromColumn(
        const char * const szColumn,
        __out void * pv,
        __out size_t * const pcbActual,
        const size_t cbMax ) const;

    ERR ErrPrepareUpdate();
    ERR ErrStoreDataToColumn( const char * const szColumn, const void * const pv, const size_t cb );
    ERR ErrCancelUpdate();
    ERR ErrUpdate();

    ERR ErrDataStoreUnavailable() const;

private:
    friend ERR TableDataStoreFactory::ErrOpen_(
        INST * const pinst,
        const wchar_t * wszDatabase,
        const char * const szTable,
        const bool fCreateIfNotFound,
        IDataStore ** ppstore );
    TableDataStore( const JET_SESID sesid, const JET_TABLEID tableid );

private:
    ERR ErrGetColumnid_( const char * const szColumn, __out JET_COLUMNID * const pcolumnid ) const;
    
    ERR ErrSetColumn_( const JET_COLUMNID columnid, const void * const pv, const size_t cb );
    ERR ErrRetrieveColumn_(
        const JET_COLUMNID columnid,
        __out void * pv,
        __out size_t * const pcbActual,
        const size_t cbMax ) const;
    
private:
    const JET_SESID m_sesid;
    const JET_TABLEID m_tableid;

    bool m_fInTransaction;
    bool m_fInUpdate;

private:
    TableDataStore( const TableDataStore& );
    TableDataStore& operator=( const TableDataStore& );
};



TableDataStore::TableDataStore( const JET_SESID sesid, const JET_TABLEID tableid ) :
    m_sesid( sesid ),
    m_tableid( tableid ),
    m_fInTransaction( false ),
    m_fInUpdate( false )
{
}

TableDataStore::~TableDataStore()
{
    Assert( !m_fInTransaction || JET_errSuccess != ErrDataStoreUnavailable() );
    Assert( !m_fInUpdate || JET_errSuccess != ErrDataStoreUnavailable() );

    if( JET_errSuccess == ( ( PIB* )m_sesid )->ErrRollbackFailure() )
    {
        ( void )ErrIsamCloseTable( m_sesid, m_tableid );
        ( void )ErrIsamEndSession( m_sesid, NO_GRBIT );
    }
}

ERR TableDataStore::ErrDataStoreUnavailable() const
{
    Assert( JET_errSuccess == m_errDataStoreUnavailable || PinstFromPpib( ( PIB* )m_sesid )->FInstanceUnavailable() );
    return m_errDataStoreUnavailable;
}

ERR TableDataStore::ErrColumnExists(
    const char * const szColumn,
    __out bool * const pfExists,
    __out JET_COLTYP * const pcoltyp ) const
{
    ERR err;
    *pfExists = false;
    *pcoltyp = JET_coltypNil;
    
    JET_COLUMNID columnid;
    JET_COLUMNDEF columndef;
    err = ErrIsamGetTableColumnInfo(
        m_sesid,
        m_tableid,
        szColumn,
        &columnid,
        &columndef,
        sizeof( columndef ),
        JET_ColInfo,
        fFalse );
    
    if ( JET_errColumnNotFound == err )
    {
        Assert( false == *pfExists );
        Assert( JET_coltypNil == *pcoltyp );
        err = JET_errSuccess;
    }
    else if ( JET_errSuccess == err )
    {
        *pfExists = true;
        *pcoltyp = columndef.coltyp;
    }
    Call( err );

HandleError:
    return err;
}

ERR TableDataStore::ErrCreateColumn( const char * const szColumn, const JET_COLTYP coltyp )
{
    ERR err;
    bool fInTransaction = false;

    Call( ErrIsamBeginTransaction( m_sesid, 49381, NO_GRBIT ) );
    fInTransaction = true;
    
    bool fColumnExists;
    JET_COLTYP coltypColumn;
    Call( ErrColumnExists( szColumn, &fColumnExists, &coltypColumn ) );
    
    if( fColumnExists && coltyp == coltypColumn )
    {
    }
    else if( fColumnExists && coltyp != coltypColumn )
    {
        AssertSzRTL( fFalse, "Meta-data mismatch. Existing column is the wrong type" );
        Call( ErrERRCheck( JET_errInvalidColumnType ) );
    }
    else
    {
        Assert( !fColumnExists );
        JET_COLUMNDEF columndef = {0};
        columndef.cbStruct = sizeof( columndef );
        columndef.coltyp = coltyp;
        Call( ErrIsamAddColumn( m_sesid, m_tableid, szColumn, &columndef, NULL, 0, NULL ) );
    }

    Call( ErrIsamCommitTransaction( m_sesid, JET_bitCommitLazyFlush ) );
    fInTransaction = false;

    Call( ErrIsamMove( m_sesid, m_tableid, JET_MoveFirst, NO_GRBIT ) );

HandleError:
    if( fInTransaction )
    {
        Assert( err < JET_errSuccess );
        ( void )ErrIsamRollback( m_sesid, NO_GRBIT );
    }
    return err;
}

ERR TableDataStore::ErrLoadDataFromColumn(
    const char * const szColumn,
    __out void * pv,
    __out size_t * const pcbActual,
    const size_t cbMax ) const
{
    ERR err;
    JET_COLUMNID columnid;
    Call( ErrGetColumnid_( szColumn, &columnid ) );
    Call( ErrRetrieveColumn_( columnid, pv, pcbActual, cbMax ) );

HandleError:
    return err;
}

ERR TableDataStore::ErrPrepareUpdate()
{
    Assert( !m_fInTransaction );
    Assert( !m_fInUpdate );
    
    ERR err;
    Call( ErrIsamBeginTransaction( m_sesid, 48869, NO_GRBIT ) );
    m_fInTransaction = true;
    Call( ErrIsamPrepareUpdate( m_sesid, m_tableid, JET_prepReplace ) );
    m_fInUpdate = true;

    Assert( m_fInTransaction );
    Assert( m_fInUpdate );

    return JET_errSuccess;
    
HandleError:
    Assert( err < JET_errSuccess );
    ( void )ErrCancelUpdate();
    return err;
}

ERR TableDataStore::ErrStoreDataToColumn( const char * const szColumn, const void * const pv, const size_t cb )
{
    Assert( m_fInTransaction );
    Assert( m_fInUpdate );
    
    ERR err;
    JET_COLUMNID columnid;
    Call( ErrGetColumnid_( szColumn, &columnid ) );
    Call( ErrSetColumn_( columnid, pv, cb ) );

HandleError:
    return err;
}

ERR TableDataStore::ErrCancelUpdate()
{
    ERR err = JET_errSuccess;

    if( m_fInUpdate )
    {
        Call( ErrIsamPrepareUpdate( m_sesid, m_tableid, JET_prepCancel ) );
        m_fInUpdate = false;
    }
    
    if( m_fInTransaction )
    {
        Call( ErrIsamRollback( m_sesid, NO_GRBIT ) );
        m_fInTransaction = false;
    }

    Assert( !m_fInTransaction );
    Assert( !m_fInUpdate );

HandleError:
    if ( ( err < JET_errSuccess ) && ( JET_errSuccess != ( ( PIB* )m_sesid )->ErrRollbackFailure() ) )
    {
        Assert( !m_fInUpdate );
        m_errDataStoreUnavailable = err;
    }

    return err;
}

ERR TableDataStore::ErrUpdate()
{
    Assert( m_fInTransaction );
    Assert( m_fInUpdate );
    
    ERR err;
    Call( ErrIsamUpdate( m_sesid, m_tableid, NULL, 0, NULL, NO_GRBIT ) );
    m_fInUpdate = false;
    Call( ErrIsamCommitTransaction( m_sesid, JET_bitCommitLazyFlush ) );
    m_fInTransaction = false;

{
    BFLatch bfl = { 0 };
    PIBTraceContextScope tcScope = ( ( PIB* )m_sesid )->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( ( FUCB* )m_tableid );
    tcScope->SetDwEngineObjid( ObjidFDP( ( FUCB* )m_tableid ) );

    const FUCB * const pfucb = ( FUCB* )m_tableid;

    if( ErrBFReadLatchPage( &bfl, pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bflfNoWait, ((PIB*)m_sesid)->BfpriPriority( pfucb->ifmp ), *tcScope ) >= JET_errSuccess )
    {
        BFReadUnlatch( &bfl );
    }
}

    Assert( !m_fInTransaction );
    Assert( !m_fInUpdate );

HandleError:
    return err;
}

ERR TableDataStore::ErrGetColumnid_( const char * const szColumn, __out JET_COLUMNID * const pcolumnid ) const
{
    ERR err;

    
    *pcolumnid = JET_columnidNil;
    
    JET_COLUMNID columnidIgnored;
    JET_COLUMNDEF columndef = {0};
    Call( ErrIsamGetTableColumnInfo(
        m_sesid,
        m_tableid,
        szColumn,
        &columnidIgnored,
        &columndef,
        sizeof( columndef ),
        JET_ColInfo,
        fFalse ) );

    *pcolumnid = columndef.columnid;
    
HandleError:
    return err;
}

ERR TableDataStore::ErrSetColumn_( const JET_COLUMNID columnid, const void * const pv, const size_t cb )
{
    ERR err;
    Call( ErrIsamSetColumn( m_sesid, m_tableid, columnid, pv, cb, NO_GRBIT, NULL ) );

HandleError:
    return err;
}

ERR TableDataStore::ErrRetrieveColumn_(
    const JET_COLUMNID columnid,
    __out void * pv,
    __out size_t * const pcbActual,
    const size_t cbMax ) const
{
    ERR err;

    const ULONG cbMaxT = ( ULONG )cbMax;
    ULONG cbActualT;
    
    Call( ErrIsamRetrieveColumn(
        m_sesid,
        m_tableid,
        columnid,
        pv,
        cbMaxT,
        &cbActualT,
        NO_GRBIT,
        NULL ) );

HandleError:
    *pcbActual = cbActualT;
    return err;
}



ERR TableDataStoreFactory::ErrOpenOrCreate(
    INST * const pinst,
    const wchar_t * wszDatabase,
    const char * const szTable,
    IDataStore ** ppstore )
{
    return ErrOpen_( pinst, wszDatabase, szTable, true, ppstore );
}

ERR TableDataStoreFactory::ErrOpenExisting(
    INST * const pinst,
    const wchar_t * wszDatabase,
    const char * const szTable,
    IDataStore ** ppstore )
{
    return ErrOpen_( pinst, wszDatabase, szTable, false, ppstore );
}

ERR TableDataStoreFactory::ErrOpen_(
    INST * const pinst,
    const wchar_t * wszDatabase,
    const char * const szTable,
    const bool fCreateIfNotFound,
    IDataStore ** ppstore )
{
    ERR err;
    JET_OPERATIONCONTEXT operationContext = { OCUSER_DBSCAN, 0, 0, 0 };

    *ppstore = NULL;
    
    const JET_INSTANCE inst = ( JET_INSTANCE )pinst;
    JET_SESID sesid         = JET_sesidNil;
    JET_TABLEID tableid     = JET_tableidNil;
    JET_DBID dbid           = JET_dbidNil;

    Call( ErrIsamBeginSession( inst, &sesid ) );

    Call( ErrIsamSetSessionParameter( sesid, JET_sesparamOperationContext, &operationContext, sizeof( operationContext ) ) );

    Call( ErrIsamOpenDatabase( sesid, wszDatabase, NULL, &dbid, NO_GRBIT ) );

    err = ErrIsamOpenTable( sesid, dbid, &tableid, szTable, NO_GRBIT );
    if( JET_errObjectNotFound == err && fCreateIfNotFound )
    {
        Call( ErrCreateTable_( sesid, dbid, szTable ) );
        Call( ErrIsamOpenTable( sesid, dbid, &tableid, szTable, NO_GRBIT ) );
    }
    Call( err );

    Call( ErrIsamMove( sesid, tableid, JET_MoveFirst, NO_GRBIT ) );

    Alloc( *ppstore = new TableDataStore( sesid, tableid ) );
    sesid = JET_sesidNil;
    tableid = JET_tableidNil;

HandleError:
    if( JET_tableidNil != tableid )
    {
        ( void )ErrIsamCloseTable( sesid, tableid );
    }
    if( JET_sesidNil != sesid )
    {
        ( void )ErrIsamEndSession( sesid, NO_GRBIT );
    }
    return err;
}

ERR TableDataStoreFactory::ErrCreateTable_(
        const JET_SESID sesid,
        const JET_DBID dbid,
        const char * const szTable )
{
    ERR err;
    JET_TABLECREATE5_A tablecreate = {0};
    bool fInTransaction = false;
    
    Call( ErrIsamBeginTransaction( sesid, 65253, NO_GRBIT ) );
    fInTransaction = true;
    
    tablecreate.cbStruct = sizeof( tablecreate );
    tablecreate.szTableName = const_cast<char *>( szTable );
    tablecreate.grbit = JET_bitTableCreateSystemTable;

    
    PIB * const ppib = ( PIB * )sesid;
    const IFMP ifmp = ( IFMP )dbid;
    CallR( ErrFILECreateTable( ppib, ifmp, &tablecreate ) );
    FUCB * const pfucb = ( FUCB * )( tablecreate.tableid );
    pfucb->pvtfndef = &vtfndefIsam;
    
    Call( ErrCreateRecord_( sesid, tablecreate.tableid ) );
    Call( ErrIsamCloseTable( sesid, tablecreate.tableid ) );

    Call( ErrIsamCommitTransaction( sesid, JET_bitCommitLazyFlush ) );
    fInTransaction = false;

HandleError:
    if( fInTransaction )
    {
        Assert( err < JET_errSuccess );
        ( void )ErrIsamRollback( sesid, NO_GRBIT );
    }
    return err;
}

ERR TableDataStoreFactory::ErrCreateRecord_( const JET_SESID sesid, const JET_TABLEID tableid )
{
    ERR err;

    bool fInTransaction = false;
    bool fInUpdate = false;
    
    Call( ErrIsamBeginTransaction( sesid, 40677, NO_GRBIT ) );
    fInTransaction = true;
    Call( ErrIsamPrepareUpdate( sesid, tableid, JET_prepInsert ) );
    fInUpdate = true;
    Call( ErrIsamUpdate( sesid, tableid, NULL, 0, NULL, NO_GRBIT ) );
    fInUpdate = false;
    Call( ErrIsamCommitTransaction( sesid, JET_bitCommitLazyFlush ) );
    fInTransaction = false;
    
HandleError:
    if( fInUpdate )
    {
        Assert( err < JET_errSuccess );
        ( void )ErrIsamPrepareUpdate( sesid, tableid, JET_prepCancel );
    }
    if( fInTransaction )
    {
        Assert( err < JET_errSuccess );
        ( void )ErrIsamRollback( sesid, NO_GRBIT );
    }
    
    return err;
}



void DataBinding::Print( CPRINTF * const pcprintf ) const
{
    ( *pcprintf )("%s: ", SzColumn() );
    const void * pv;
    size_t cb;
    GetPvCb( &pv, &cb );
    UtilPrintColumnValue( pv, cb, Coltyp(), pcprintf );
}



DataBindings::DataBindings() : m_cbindings( 0 )
{
}

DataBindings::~DataBindings()
{
}

void DataBindings::AddBinding( DataBinding * const pbinding )
{
    AssertRTLPREFIX( m_cbindings < m_cbindingsMax );
    m_rgpbindings[m_cbindings++] = pbinding;
    AssertRTLPREFIX( m_cbindings < m_cbindingsMax );
}

DataBindings::iterator DataBindings::begin() const
{
    return m_rgpbindings;
}

DataBindings::iterator DataBindings::end() const
{
    return m_rgpbindings+m_cbindings;
}



DataSerializer::DataSerializer( const DataBindings& bindings ) :
    m_bindings( bindings )
{
}

DataSerializer::~DataSerializer()
{
}

void DataSerializer::SetBindingsToDefault()
{
    for_each( m_bindings.begin(), m_bindings.end(), mem_fun( &DataBinding::SetToDefault ) );
}

ERR DataSerializer::ErrSaveBindings( IDataStore * const pstore )
{
    ERR err = JET_errSuccess;
    bool fInUpdate = false;

    Call( pstore->ErrDataStoreUnavailable() );

    Call( ErrCreateAllColumns_( pstore ) );
    Call( pstore->ErrPrepareUpdate() );
    fInUpdate = true;
    for( DataBindings::iterator i = m_bindings.begin();
        i != m_bindings.end();
        ++i )
    {
        const void * pv;
        size_t cb;
        ( *i )->GetPvCb( &pv, &cb );
        Call( pstore->ErrStoreDataToColumn( ( *i )->SzColumn(), pv, cb ) );
    }
    Call( pstore->ErrUpdate() );
    fInUpdate = false;
    
HandleError:
    if( fInUpdate )
    {
        ( void )pstore->ErrCancelUpdate();
    }
    return err;
}

void DataSerializer::Print( CPRINTF * const pcprintf )
{
    for( DataBindings::iterator i = m_bindings.begin();
        i != m_bindings.end();
        ++i )
    {
        ( *i )->Print( pcprintf );
        ( *pcprintf )("\n" );
    }
}

ERR DataSerializer::ErrLoadBindings( const IDataStore * const pstore )
{
    ERR err = JET_errSuccess;

    BYTE * pb = NULL;
    size_t cbMax = 0;
    size_t cbActual;

    Call( pstore->ErrDataStoreUnavailable() );

    cbMax = 64;
    Alloc( pb = new BYTE[cbMax] );
    
    for( DataBindings::iterator i = m_bindings.begin();
        i != m_bindings.end();
        ++i )
    {
        bool fExists;
        JET_COLTYP coltyp;
        Call( pstore->ErrColumnExists( ( *i )->SzColumn(), &fExists, &coltyp ) );
        if( !fExists )
        {
            ( *i )->SetToDefault();
        }
        else if( coltyp != ( *i )->Coltyp() )
        {
            AssertSzRTL( fFalse, "meta-data mistmatch" );
            Call( ErrERRCheck( JET_errInvalidColumnType ) );
        }
        else
        {
            Call( pstore->ErrLoadDataFromColumn( ( *i )->SzColumn(), pb, &cbActual, cbMax ) );
            if( JET_wrnColumnNull == err )
            {
                ( *i )->SetToDefault();
                err = JET_errSuccess;
            }
            else
            {
                if( cbActual > cbMax )
                {
                    delete [] pb;
                    Alloc( pb = new BYTE[cbActual] );
                    cbMax = cbActual;
                    Call( pstore->ErrLoadDataFromColumn( ( *i )->SzColumn(), pb, &cbActual, cbMax ) );
                }
                Call( ( *i )->ErrSetFromPvCb( pb, cbActual ) );
            }
        }
    }
    
HandleError:
    delete [] pb;
    return err;
}

ERR DataSerializer::ErrCreateAllColumns_( IDataStore * const pstore )
{
    ERR err = JET_errSuccess;
    for( DataBindings::iterator i = m_bindings.begin();
        i != m_bindings.end();
        ++i )
    {
        bool fExists;
        JET_COLTYP coltyp;
        Call( pstore->ErrColumnExists( ( *i )->SzColumn(), &fExists, &coltyp ) );
        if( !fExists )
        {
            Call( pstore->ErrCreateColumn( ( *i )->SzColumn(), ( *i )->Coltyp() ) );
        }
        else if( ( *i )->Coltyp() != coltyp )
        {
            AssertSzRTL( fFalse, "meta-data mistmatch" );
            Call( ErrERRCheck( JET_errInvalidColumnType ) );
        }
    }

HandleError:
    return err;
}



MemoryDataStore::MemoryDataStore() :
    IDataStore(),
    m_ccolumns( 0 ),
    m_fInUpdate( false )
{
}

MemoryDataStore::~MemoryDataStore()
{
}
    
ERR MemoryDataStore::ErrColumnExists(
    const char * const szColumn,
    __out bool * const pfExists,
    __out JET_COLTYP * const pcoltyp ) const
{
    const INT i = IColumn( szColumn );
    if( -1 == i )
    {
        *pfExists = false;
        *pcoltyp = JET_coltypNil;
    }
    else
    {
        *pfExists = true;
        *pcoltyp = m_rgcoltyps[i];
    }
    return JET_errSuccess;
}

ERR MemoryDataStore::ErrCreateColumn( const char * const szColumn, const JET_COLTYP coltyp )
{
    AssertPREFIX( m_ccolumns < m_ccolumnsMax );
    m_rgszColumns[m_ccolumns] = szColumn;
    m_rgcoltyps[m_ccolumns] = coltyp;
    m_ccolumns++;
    AssertPREFIX( m_ccolumns < m_ccolumnsMax );
    return JET_errSuccess;
}

ERR MemoryDataStore::ErrLoadDataFromColumn(
    const char * const szColumn,
    __out void * pv,
    __out size_t * const pcbActual,
    const size_t cbMax ) const
{
    const INT i = IColumn( szColumn );
    if( -1 != i && NULL != m_rgpbData[i].get() )
    {
        *pcbActual = m_rgcbData[i];
        const size_t cbToCopy = min( cbMax, *pcbActual );
        if( pv )
        {
            memcpy( pv, m_rgpbData[i].get(), cbToCopy );
        }
        return JET_errSuccess;
    }
    else
    {
        *pcbActual = 0;
        return ErrERRCheck( JET_wrnColumnNull );
    }
}

ERR MemoryDataStore::ErrPrepareUpdate()
{
    Assert( !m_fInUpdate );
    m_fInUpdate = true;
    return JET_errSuccess;
}

ERR MemoryDataStore::ErrStoreDataToColumn( const char * const szColumn, const void * const pv, const size_t cb )
{
    Assert( m_fInUpdate );
    const INT i = IColumn( szColumn );
    if( -1 == i )
    {
        AssertSzRTL( fFalse, "Column doesn't exist" );
        return ErrERRCheck( JET_errColumnNotFound );
    }
    else
    {
        BYTE * pb = new BYTE[cb];
        memcpy( pb, pv, cb );
        m_rgpbData[i] = unique_ptr<BYTE>( pb );
        m_rgcbData[i] = cb;
        return JET_errSuccess;
    }
}

ERR MemoryDataStore::ErrCancelUpdate()
{
    Assert( m_fInUpdate );
    m_fInUpdate = false;
    return JET_errSuccess;
}

ERR MemoryDataStore::ErrUpdate()
{
    Assert( m_fInUpdate );
    m_fInUpdate = false;
    return JET_errSuccess;
}

INT MemoryDataStore::IColumn( const char * const szColumn ) const
{
    for( INT i = 0; i < m_ccolumns; ++i )
    {
        if( 0 == LOSStrCompareA( szColumn, m_rgszColumns[i] ) )
        {
            return i;
        }
    }
    return -1;
}


#ifdef ENABLE_JET_UNIT_TEST


JETUNITTEST( DataBindingOf, ConstructorSetSzColumn )
{
    const char * const szColumn = "columnname";
    double d;
    
    DataBindingOf<double> binding( &d, szColumn );
    CHECK( 0 == strcmp( szColumn, binding.SzColumn() ) );
}

JETUNITTEST( DataBindingOf, ColtypOfByteIsUnsignedByte )
{
    BYTE b;
    DataBindingOf<BYTE> binding( &b, "any" );
    CHECK( JET_coltypUnsignedByte == binding.Coltyp() );
}

JETUNITTEST( DataBindingOf, ColtypOfShortIsShort )
{
    SHORT s;
    DataBindingOf<SHORT> binding( &s, "any" );
    CHECK( JET_coltypShort == binding.Coltyp() );
}

JETUNITTEST( DataBindingOf, ColtypOfWordIsUnsignedShort )
{
    WORD w;
    DataBindingOf<WORD> binding( &w, "any" );
    CHECK( JET_coltypUnsignedShort == binding.Coltyp() );
}

JETUNITTEST( DataBindingOf, ColtypOfIntIsLong )
{
    INT i;
    DataBindingOf<INT> binding( &i, "any" );
    CHECK( JET_coltypLong == binding.Coltyp() );
}

JETUNITTEST( DataBindingOf, ColtypOfUnsignedIntIsUnsignedLong )
{
    UINT ui;
    DataBindingOf<UINT> binding( &ui, "any" );
    CHECK( JET_coltypUnsignedLong == binding.Coltyp() );
}

JETUNITTEST( DataBindingOf, ColtypOfLongIsLong )
{
    LONG l;
    DataBindingOf<LONG> binding( &l, "any" );
    CHECK( JET_coltypLong == binding.Coltyp() );
}

JETUNITTEST( DataBindingOf, ColtypOfDwordIsUnsignedLong )
{
    DWORD dw;
    DataBindingOf<DWORD> binding( &dw, "any" );
    CHECK( JET_coltypUnsignedLong == binding.Coltyp() );
}

JETUNITTEST( DataBindingOf, ColtypOfInt64IsLongLong )
{
    __int64 x;
    DataBindingOf<__int64> binding( &x, "any" );
    CHECK( JET_coltypLongLong == binding.Coltyp() );
}

JETUNITTEST( DataBindingOf, ColtypOfStructIsLongBinary )
{
    LOGTIME logtime;
    DataBindingOf<LOGTIME> binding( &logtime, "any" );
    CHECK( JET_coltypLongBinary == binding.Coltyp() );
}

JETUNITTEST( DataBindingOf, GetPvCb )
{
    INT x;
    DataBindingOf<INT> binding( &x, "any" );

    const void * pv = NULL;
    size_t cb = 0;
    binding.GetPvCb( &pv, &cb );

    CHECK( pv == &x );
    CHECK( cb == sizeof( x ) );
}

JETUNITTEST( DataBindingOf, ErrSetFromPvCb )
{
    const __int64 value = 15;

    __int64 x = 0;
    __int64 y = 0;
    
    DataBindingOf<__int64> binding( &x, "any" );
    y = value;
    CHECK( JET_errSuccess == binding.ErrSetFromPvCb( &y, sizeof( y ) ) );
    CHECK( value == x );
}

JETUNITTEST( DataBindingOf, ErrSetFromPvCbFailsWithInvalidBuffer )
{
    char c;
    DataBindingOf<char> binding( &c, "any" );
    CHECK( JET_errInvalidBufferSize == binding.ErrSetFromPvCb( &c, 7 ) );
}

JETUNITTEST( DataBindingOf, SetToDefault )
{
    LONG l = 19;
    DataBindingOf<LONG> binding( &l, "any" );
    binding.SetToDefault();
    CHECK( 0 == l );
}

JETUNITTEST( DataBindingOf, Print )
{
    LONG l = 19;
    DataBindingOf<LONG> binding( &l, "column" );
    
    CPRINTFBUFFER cprintf;
    binding.Print( &cprintf );
    CHECK( 0 == strcmp("column: 19", cprintf.SzBuffer() ) );
}



JETUNITTEST( DataBindingOfChar, ConstructorSetSzColumn )
{
    const char * const szColumn = "columnname";
    char sz[64];
    
    DataBindingOf<char*> binding( sz, _countof( sz ), szColumn );
    CHECK( 0 == strcmp( szColumn, binding.SzColumn() ) );
}

JETUNITTEST( DataBindingOfChar, ColtypIsLongText )
{
    char sz[64];
    DataBindingOf<char*> binding( sz, _countof( sz ), "any" );
    CHECK( JET_coltypLongText == binding.Coltyp() );
}

JETUNITTEST( DataBindingOfChar, GetPvCb )
{
    char sz[] = "A test string";
    DataBindingOf<char*> binding( sz, _countof( sz ), "any" );

    const void * pv = NULL;
    size_t cb = 0;
    binding.GetPvCb( &pv, &cb );

    CHECK( pv == sz );
    CHECK( cb == strlen( sz ) );
}

JETUNITTEST( DataBindingOfChar, ErrSetFromPvCb )
{
    const char szValue[] = "Another test string";

    char sz[64];
    DataBindingOf<char*> binding( sz, _countof( sz ), "any" );
    CHECK( JET_errSuccess == binding.ErrSetFromPvCb( szValue, strlen( szValue ) ) );
    CHECK( 0 == strcmp( szValue, sz ) );
}

JETUNITTEST( DataBindingOfChar, ErrSetFromPvCbFailsWithInvalidBuffer )
{
    const char szValue[] = "This string is far too long";

    char sz[4];
    DataBindingOf<char*> binding( sz, _countof( sz ), "any" );
    CHECK( JET_errInvalidBufferSize == binding.ErrSetFromPvCb( szValue, strlen( szValue ) ) );
}

JETUNITTEST( DataBindingOfChar, SetToDefault )
{
    char sz[64] = "hello";
    DataBindingOf<char*> binding( sz, _countof( sz ), "any" );
    binding.SetToDefault();
    CHECK( 0 == strcmp("", sz ) );
}



JETUNITTEST( DataBindings, NewObjectHasNoBindings )
{
    DataBindings bindings;
    CHECK( 0 == distance( bindings.begin(), bindings.end() ) );
}

JETUNITTEST( DataBindings, AddOneBinding )
{
    INT x;
    
    DataBindings bindings;
    DataBindingOf<INT> binding( &x, "any" );
    bindings.AddBinding( &binding );

    CHECK( 1 == distance( bindings.begin(), bindings.end() ) );
    CHECK( &binding == *bindings.begin() );
}

JETUNITTEST( DataBindings, AddTwoBindings )
{
    INT x;
    INT y;
    
    DataBindings bindings;
    DataBindingOf<INT> binding1( &x, "any" );
    DataBindingOf<INT> binding2( &y, "anyother" );
    bindings.AddBinding( &binding1 );
    bindings.AddBinding( &binding2 );

    CHECK( 2 == distance( bindings.begin(), bindings.end() ) );

    DataBindings::iterator i = bindings.begin();
    CHECK( &binding1 == *i );
    i++;
    CHECK( &binding2 == *i );
}


JETUNITTEST( MemoryDataStore, NoColumnInNewObject )
{
    bool fExists;
    JET_COLTYP coltyp;
    MemoryDataStore store;
    CHECK( JET_errSuccess == store.ErrColumnExists("any", &fExists, &coltyp ) );
    CHECK( false == fExists );
    CHECK( JET_coltypNil == coltyp );
}

JETUNITTEST( MemoryDataStore, CreateOneColumn )
{
    const char * const szColumn = "anycolumn";
    const JET_COLTYP coltyp = JET_coltypCurrency;
    
    MemoryDataStore store;
    CHECK( JET_errSuccess == store.ErrCreateColumn( szColumn, coltyp ) );
    
    bool fExists;
    JET_COLTYP coltypT;
    CHECK( JET_errSuccess == store.ErrColumnExists( szColumn, &fExists, &coltypT ) );
    CHECK( true == fExists );
    CHECK( coltyp == coltypT );
}

JETUNITTEST( MemoryDataStore, CreateTwoColumns )
{
    const char * const szColumn1 = "firstcolumn";
    const JET_COLTYP coltyp1 = JET_coltypLongBinary;

    const char * const szColumn2 = "secondcolumn";
    const JET_COLTYP coltyp2 = JET_coltypText;
    
    MemoryDataStore store;
    CHECK( JET_errSuccess == store.ErrCreateColumn( szColumn1, coltyp1 ) );
    CHECK( JET_errSuccess == store.ErrCreateColumn( szColumn2, coltyp2 ) );
    
    bool fExists;
    JET_COLTYP coltypT;
    
    CHECK( JET_errSuccess == store.ErrColumnExists( szColumn1, &fExists, &coltypT ) );
    CHECK( true == fExists );
    CHECK( coltyp1 == coltypT );

    CHECK( JET_errSuccess == store.ErrColumnExists( szColumn2, &fExists, &coltypT ) );
    CHECK( true == fExists );
    CHECK( coltyp2 == coltypT );
}

JETUNITTEST( MemoryDataStore, CreateColumnCheckColumnDoesNotExist )
{
    const char * const szColumn = "anycolumn";
    const JET_COLTYP coltyp = JET_coltypCurrency;
    
    MemoryDataStore store;
    CHECK( JET_errSuccess == store.ErrCreateColumn( szColumn, coltyp ) );
    
    bool fExists;
    JET_COLTYP coltypT;
    CHECK( JET_errSuccess == store.ErrColumnExists("nocolumn", &fExists, &coltypT ) );
    CHECK( false == fExists );
}

JETUNITTEST( MemoryDataStore, LoadDataFromNonexistentColumnGivesNull )
{
    MemoryDataStore store;
    const void * pv;
    size_t cb;
    CHECK( JET_wrnColumnNull == store.ErrLoadDataFromColumn("nocolumn", &pv, &cb, 0 ) );
}

JETUNITTEST( MemoryDataStore, LoadDataFromUnsetColumnGivesNull )
{
    const char * const szColumn = "anycolumn";
    
    MemoryDataStore store;
    CHECK( JET_errSuccess == store.ErrCreateColumn( szColumn, JET_coltypBinary ) );
    const void * pv;
    size_t cb;
    CHECK( JET_wrnColumnNull == store.ErrLoadDataFromColumn( szColumn, &pv, &cb, 0 ) );
}

JETUNITTEST( MemoryDataStore, SaveAndLoadColumn )
{
    const char * const szColumn = "anycolumn";
    
    MemoryDataStore store;
    CHECK( JET_errSuccess == store.ErrCreateColumn( szColumn, JET_coltypBinary ) );
    BYTE rgb[] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8 };
    CHECK( JET_errSuccess == store.ErrPrepareUpdate() );
    CHECK( JET_errSuccess == store.ErrStoreDataToColumn( szColumn, rgb, _countof( rgb ) ) );
    CHECK( JET_errSuccess == store.ErrUpdate() );
    
    BYTE rgbRetrieved[sizeof( rgb )];
    size_t cbActual;
    CHECK( JET_errSuccess == store.ErrLoadDataFromColumn( szColumn, rgbRetrieved, &cbActual, sizeof( rgbRetrieved ) ) );
    CHECK( _countof( rgb ) == cbActual );
    CHECK( 0 == memcmp( rgb, rgbRetrieved, cbActual ) );
}

JETUNITTEST( MemoryDataStore, DefaultErrDataStoreFailure )
{
    MemoryDataStore store;
    CHECK( JET_errSuccess == store.ErrDataStoreUnavailable() );
}

class TestMemoryDataStore : public MemoryDataStore
{
public:
    void SetDataStoreUnavailable( ERR err ) { m_errDataStoreUnavailable = err; }
};

JETUNITTEST( DataSerializer, LoadWithoutSet )
{
    INT x = -1;
    DataBindingOf<INT> binding( &x, "no column" );
    DataBindings bindings;
    bindings.AddBinding( &binding );
    
    MemoryDataStore store;
    DataSerializer serializer( bindings );
    CHECK( JET_errSuccess == serializer.ErrLoadBindings( &store ) );
    CHECK( 0 == x );
}

JETUNITTEST( DataSerializer, SetBindingsToDefault )
{
    INT x = -1;
    char szData[64] = "xyzzy";
    
    DataBindingOf<INT> binding1( &x, "no column" );
    DataBindingOf<char*> binding2( szData, _countof( szData ), "anycolumn" );
    
    DataBindings bindings;
    bindings.AddBinding( &binding1 );
    bindings.AddBinding( &binding2 );
    
    DataSerializer serializer( bindings );
    serializer.SetBindingsToDefault();
    CHECK( 0 == x );
    CHECK( 0 == strcmp("", szData ) );
}

JETUNITTEST( DataSerializer, LoadFromNullColumn )
{
    const char * const szColumn = "col";
    INT x = -1;
    DataBindingOf<INT> binding( &x, szColumn );
    DataBindings bindings;
    bindings.AddBinding( &binding );
    
    MemoryDataStore store;
    CHECK( JET_errSuccess == store.ErrCreateColumn( szColumn, JET_coltypLong ) );

    DataSerializer serializer( bindings );
    CHECK( JET_errSuccess == serializer.ErrLoadBindings( &store ) );
    CHECK( 0 == x );
}

JETUNITTEST( DataSerializer, LoadFromData )
{
    const char * const szColumn = "col1";
    const char * const sz = "hello world";
    char szData[64];
    
    DataBindingOf<char*> binding( szData, _countof( szData ), szColumn );
    DataBindings bindings;
    bindings.AddBinding( &binding );
    
    MemoryDataStore store;
    CHECK( JET_errSuccess == store.ErrCreateColumn( szColumn, JET_coltypLongText ) );
    CHECK( JET_errSuccess == store.ErrPrepareUpdate() );
    CHECK( JET_errSuccess == store.ErrStoreDataToColumn( szColumn, sz, strlen( sz ) ) );
    CHECK( JET_errSuccess == store.ErrUpdate() );

    DataSerializer serializer( bindings );
    CHECK( JET_errSuccess == serializer.ErrLoadBindings( &store ) );
    CHECK( 0 == strcmp( szData, sz ) );
}

JETUNITTEST( DataSerializer, LoadFromLargeData )
{
    const char * const szColumn = "col1";

    const size_t cch = 2048;
    char sz[cch];
    char szData[cch];
    memset( sz, chDATASERIALIZETestFill, cch );
    sz[cch-1] = 0;
    
    DataBindingOf<char*> binding( szData, _countof( szData ), szColumn );
    DataBindings bindings;
    bindings.AddBinding( &binding );
    
    MemoryDataStore store;
    CHECK( JET_errSuccess == store.ErrCreateColumn( szColumn, JET_coltypLongText ) );
    CHECK( JET_errSuccess == store.ErrPrepareUpdate() );
    CHECK( JET_errSuccess == store.ErrStoreDataToColumn( szColumn, sz, strlen( sz ) ) );
    CHECK( JET_errSuccess == store.ErrUpdate() );

    DataSerializer serializer( bindings );
    CHECK( JET_errSuccess == serializer.ErrLoadBindings( &store ) );
    CHECK( 0 == strcmp( szData, sz ) );
}

JETUNITTEST( DataSerializer, SaveBinding )
{
    const char * const szColumn = "column1";
    DWORD dw = 0xCCDDEEFF;
    
    DataBindingOf<DWORD> binding( &dw, szColumn );
    DataBindings bindings;
    bindings.AddBinding( &binding );
    
    MemoryDataStore store;
    DataSerializer serializer( bindings );
    CHECK( JET_errSuccess == serializer.ErrSaveBindings( &store ) );

    bool fExists;
    JET_COLTYP coltyp;
    CHECK( JET_errSuccess == store.ErrColumnExists( szColumn, &fExists, &coltyp ) );

    BYTE rgbRetrieved[4];
    size_t cbActual;
    CHECK( JET_errSuccess == store.ErrLoadDataFromColumn( szColumn, rgbRetrieved, &cbActual, sizeof( rgbRetrieved ) ) );
    CHECK( sizeof( dw ) == cbActual );
    CHECK( 0 == memcmp( &dw, rgbRetrieved, cbActual ) );
}

JETUNITTEST( DataSerializer, SaveBindingToExistingColumn )
{
    const char * const szColumn = "column1";
    DWORD dw = 0x12DD34FF;
    
    DataBindingOf<DWORD> binding( &dw, szColumn );
    DataBindings bindings;
    bindings.AddBinding( &binding );
    
    MemoryDataStore store;
    CHECK( JET_errSuccess == store.ErrCreateColumn( szColumn, JET_coltypUnsignedLong ) );
    
    DataSerializer serializer( bindings );
    CHECK( JET_errSuccess == serializer.ErrSaveBindings( &store ) );

    bool fExists;
    JET_COLTYP coltyp;
    CHECK( JET_errSuccess == store.ErrColumnExists( szColumn, &fExists, &coltyp ) );

    BYTE rgbRetrieved[4];
    size_t cbActual;
    CHECK( JET_errSuccess == store.ErrLoadDataFromColumn( szColumn, rgbRetrieved, &cbActual, sizeof( rgbRetrieved ) ) );
    CHECK( sizeof( dw ) == cbActual );
    CHECK( 0 == memcmp( &dw, rgbRetrieved, cbActual ) );
}

JETUNITTEST( DataSerializer, SaveAndLoadBindings )
{
    const INT i             = 0x1;
    const __int64 i64       = 0x10;
    const char * const sz   = "test string";
    
    const char * const szColumn1 = "x";
    const char * const szColumn2 = "y";
    const char * const szColumn3 = "z";
    
    INT     value1      = i;
    __int64 value2      = i64;
    char    value3[64];
    OSStrCbCopyA( value3, _countof( value3 ), sz );
    
    DataBindingOf<INT> binding1( &value1, szColumn1 );
    DataBindingOf<__int64> binding2( &value2, szColumn2 );
    DataBindingOf<char*> binding3( value3, _countof( value3 ), szColumn3 );

    DataBindings bindings;
    bindings.AddBinding( &binding1 );
    bindings.AddBinding( &binding2 );
    bindings.AddBinding( &binding3 );

    MemoryDataStore store;
    
    DataSerializer serializer( bindings );
    CHECK( JET_errSuccess == serializer.ErrSaveBindings( &store ) );
    value1 = -1;
    value2 = -1;
    value3[0] = 0;
    CHECK( JET_errSuccess == serializer.ErrLoadBindings( &store ) );
    CHECK( i == value1 );
    CHECK( i64 == value2 );
    CHECK( 0 == strcmp( value3, sz ) );
}

JETUNITTEST( DataSerializer, Print )
{
    USHORT us = 0x1;
    LONG l = 1;
    
    DataBindingOf<USHORT> binding1( &us, "a" );
    DataBindingOf<LONG> binding2( &l, "b" );
    DataBindings bindings;
    bindings.AddBinding( &binding1 );
    bindings.AddBinding( &binding2 );
    DataSerializer serializer( bindings );

    CPRINTFBUFFER cprintf;
    serializer.Print( &cprintf );
    CHECK( 0 == strcmp("a: 0x1\nb: 1\n", cprintf.SzBuffer() ) );
}

JETUNITTEST( DataSerializer, FailuresInDataStore )
{
    INT x = -1;
    DataBindingOf<INT> binding( &x, "no column" );
    DataBindings bindings;
    bindings.AddBinding( &binding );

    TestMemoryDataStore store;
    DataSerializer serializer( bindings );
    store.SetDataStoreUnavailable( JET_errAccessDenied );
    CHECK( JET_errAccessDenied == serializer.ErrLoadBindings( &store ) );
    CHECK( JET_errAccessDenied == serializer.ErrSaveBindings( &store ) );
    CHECK( -1 == x );
}


JETUNITTEST( UtilPrintColumnValue, PrintNullPvColumn )
{
    MemoryDataStore store;
    CPRINTFBUFFER cprintf;
    UtilPrintColumnValue( NULL, 3, JET_coltypBinary, &cprintf );
    CHECK( 0 == strcmp("<null>", cprintf.SzBuffer() ) );
}

JETUNITTEST( UtilPrintColumnValue, PrintZeroCbColumn )
{
    MemoryDataStore store;
    CPRINTFBUFFER cprintf;
    UtilPrintColumnValue( &store, 0, JET_coltypBinary, &cprintf );
    CHECK( 0 == strcmp("<null>", cprintf.SzBuffer() ) );
}

JETUNITTEST( UtilPrintColumnValue, PrintByteColumn )
{
    const BYTE rgb[] = { 0x4f };
    CPRINTFBUFFER cprintf;
    UtilPrintColumnValue( rgb, sizeof( rgb ), JET_coltypUnsignedByte, &cprintf );
    CHECK( 0 == strcmp("0x4F", cprintf.SzBuffer() ) );
}

JETUNITTEST( UtilPrintColumnValue, PrintShortColumn )
{
    const SHORT s = 456;
    CPRINTFBUFFER cprintf;
    UtilPrintColumnValue( &s, sizeof( s ), JET_coltypShort, &cprintf );
    CHECK( 0 == strcmp("456", cprintf.SzBuffer() ) );
}

JETUNITTEST( UtilPrintColumnValue, PrintUnsignedShortColumn )
{
    const USHORT us = 0x4567;
    CPRINTFBUFFER cprintf;
    UtilPrintColumnValue( &us, sizeof( us ), JET_coltypUnsignedShort, &cprintf );
    CHECK( 0 == strcmp("0x4567", cprintf.SzBuffer() ) );
}

JETUNITTEST( UtilPrintColumnValue, PrintLongColumn )
{
    const LONG l = 987654321;
    CPRINTFBUFFER cprintf;
    UtilPrintColumnValue( &l, sizeof( l ), JET_coltypLong, &cprintf );
    CHECK( 0 == strcmp("987654321", cprintf.SzBuffer() ) );
}

JETUNITTEST( UtilPrintColumnValue, PrintUnsignedLongColumn )
{
    LONG ul = 0x123BAD;
    CPRINTFBUFFER cprintf;
    UtilPrintColumnValue( &ul, sizeof( ul ), JET_coltypUnsignedLong, &cprintf );
    CHECK( 0 == strcmp("0x123BAD", cprintf.SzBuffer() ) );
}

JETUNITTEST( UtilPrintColumnValue, PrintLongLongColumn )
{
    __int64 i64 = 0xBBB123FFFF;
    CPRINTFBUFFER cprintf;
    UtilPrintColumnValue( &i64, sizeof( i64 ), JET_coltypLongLong, &cprintf );
    CHECK( 0 == strcmp("0xBBB123FFFF", cprintf.SzBuffer() ) );
}

JETUNITTEST( UtilPrintColumnValue, PrintTextColumn )
{
    char sz[] = "thestring";
    CPRINTFBUFFER cprintf;
    UtilPrintColumnValue( sz, strlen( sz ), JET_coltypLongText, &cprintf );
    CHECK( 0 == strcmp( sz, cprintf.SzBuffer() ) );
}

JETUNITTEST( UtilPrintColumnValue, PrintBinaryColumn )
{
    BYTE rgb[] = { 0x4, 0x1, 0xFF, 0xE1, 0x00, 0x2 };
    CPRINTFBUFFER cprintf;
    UtilPrintColumnValue( rgb, sizeof( rgb ), JET_coltypLongBinary, &cprintf );
    CHECK( 0 == strcmp("04 01 FF E1 00 02", cprintf.SzBuffer() ) );
}

#endif

