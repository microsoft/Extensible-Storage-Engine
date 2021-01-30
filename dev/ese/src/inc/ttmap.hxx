// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

class TTARRAY
{
    public:

        class RUN
        {
            public:
                RUN()
                    :   pgno( pgnoNull ),
                        fWriteLatch( fFalse )
                {
                    bfl.pv          = NULL;
                    bfl.dwContext   = NULL;
                }
            
            public:
                PGNO    pgno;
                BFLatch bfl;
                BOOL    fWriteLatch;
        };
    
    public:
        TTARRAY( const ULONG culEntries, const ULONG ulDefault = 0 );
        ~TTARRAY();

        ERR ErrInit( INST * const pinst );

        VOID BeginRun( PIB* const ppib, RUN* const prun );
        VOID EndRun( PIB* const ppib, RUN* const prun );
        
        ERR ErrSetValue( PIB * const ppib, const ULONG ulEntry, const ULONG ulValue, RUN* const prun = NULL );
        ERR ErrGetValue( PIB * const ppib, const ULONG ulEntry, ULONG * const pulValue, RUN* const prun = NULL ) const;

        ULONG CEntries() const;
        
    private:
        TTARRAY( const TTARRAY& );
        TTARRAY& operator= ( const TTARRAY& );

        BOOL FPageInit( const PGNO pgno ) const;
        VOID SetPageInit( const PGNO pgno );

    private:

        ULONG           m_culEntries;
        ULONG           m_ulDefault;

        PIB*            m_ppib;
        FUCB*           m_pfucb;
        PGNO            m_pgnoFirst;
        ULONG*          m_rgbitInit;
        
        static DWORD    s_cTTArray;
};


class TTMAP
{
    public:
        TTMAP( PIB * const ppib );
        ~TTMAP();

        ERR ErrInit( INST * const );
        ERR ErrInsertKeyValue( const unsigned __int64 ui64Key, const ULONG ulValue )
        {
            return ErrInsertKeyValue_( ui64Key, ulValue );
        }
        ERR ErrSetValue( const unsigned __int64 ui64Key, const ULONG ulValue );
        ERR ErrGetValue( const unsigned __int64 ui64Key, ULONG * const pulValue ) const;
        ERR ErrIncrementValue( const unsigned __int64 ui64Key );
        ERR ErrDeleteKey( const unsigned __int64 ui64Key );

        ERR ErrMoveFirst();
        ERR ErrMoveNext();
        ERR ErrGetCurrentKeyValue( unsigned __int64 * const pui64Key, ULONG * const pulValue ) const;
        
        ERR ErrFEmpty( BOOL * const pfEmpty ) const;

    private:
        TTMAP( const TTMAP& );
        TTMAP& operator= ( const TTMAP& );

    private:
        ERR ErrInsertKeyValue_( const unsigned __int64 ui64Key, const ULONG ulValue );
        ERR ErrRetrieveValue_( const unsigned __int64 ui64Key, ULONG * const pulValue ) const;
        
    private:
        JET_TABLEID m_tableid;
        JET_SESID   m_sesid;

        JET_COLUMNID    m_columnidKey;
        JET_COLUMNID    m_columnidValue;
        
        LONG            m_crecords;
};


