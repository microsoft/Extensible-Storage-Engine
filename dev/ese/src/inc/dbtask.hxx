// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma warning(push)
#pragma warning(disable: 4481)

class  TASK
{
    public:
        virtual ERR ErrExecute( PIB * const ppib ) = 0;
        virtual VOID HandleError( const ERR err ) = 0;
        virtual INST *PInstance() = 0;

        static DWORD DispatchGP( void *pvThis );

        static VOID Dispatch( PIB * const ppib, const ULONG_PTR ulThis );

    protected:
        TASK();

    public:
        virtual ~TASK();

    private:
        static INT g_tasksDropped;

    private:
        TASK( const TASK& );
        TASK& operator=( const TASK& );
};

class DBTASK : public TASK
{
    public:
#pragma warning( disable : 4481 )
        ERR ErrExecute( PIB * const ppib ) sealed;
#pragma warning( default : 4481 )
        virtual ERR ErrExecuteDbTask( PIB * const ppib ) = 0;
        virtual VOID HandleError( const ERR err ) = 0;
#pragma warning( disable : 4481 )
        INST *PInstance() sealed;
#pragma warning( default : 4481 )

    protected:
        DBTASK( const IFMP ifmp );

    public:
        virtual ~DBTASK();

    protected:
        const IFMP  m_ifmp;

    private:
        DBTASK( const DBTASK& );
        DBTASK& operator=( const DBTASK& );
};

class DBREGISTEROLD2TASK : public DBTASK
{
    typedef USHORT      DEFRAGTYPE;

public:
    DBREGISTEROLD2TASK(
        _In_ const IFMP ifmp,
        _In_z_ const char * const szTableName,
        _In_opt_z_ const char * const szIndexName,
        _In_ DEFRAGTYPE defragtype );

    ERR ErrExecuteDbTask( _In_opt_ PIB *const ppib );
    void HandleError(const ERR err);

private:
    char m_szTableName[JET_cbNameMost+1];
    char m_szIndexName[JET_cbNameMost+1];
    DEFRAGTYPE m_defragtype;

private:
    DBREGISTEROLD2TASK(const DBREGISTEROLD2TASK&);
    DBREGISTEROLD2TASK& operator=(const DBREGISTEROLD2TASK&);
};


class DBEXTENDTASK : public DBTASK
{
    public:
        ERR ErrExecuteDbTask( PIB * const ppib );
        VOID HandleError( const ERR err );

    public:
        DBEXTENDTASK( const IFMP ifmp );

    private:
        DBEXTENDTASK( const DBEXTENDTASK& );
        DBEXTENDTASK& operator=( const DBEXTENDTASK& );
};


class RECTASK : public DBTASK
{
    public:
        virtual ERR ErrExecuteDbTask( PIB * const ppib ) = 0;

    protected:
        RECTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm );

    public:
        virtual ~RECTASK();

        PGNO PgnoFDP() const { return m_pgnoFDP; }
        IFMP Ifmp() const { return m_ifmp; }
        VOID GetBookmark( __out BOOKMARK * const pbookmark ) const;

        static bool FBookmarkIsLessThan( const RECTASK * ptask1, const RECTASK * ptask2 );

    protected:

        ERR ErrOpenCursor( PIB * const ppib, FUCB ** ppfucb );
        ERR ErrOpenCursorAndGotoBookmark( PIB * const ppib, FUCB ** ppfucb );

    protected:
        const PGNO      m_pgnoFDP;
        FCB             * const m_pfcb;
        const ULONG     m_cbBookmarkKey;
        const ULONG     m_cbBookmarkData;

        BYTE m_rgbBookmarkKey[cbKeyAlloc];
        BYTE m_rgbBookmarkData[cbKeyAlloc];

    private:
        RECTASK( const RECTASK& );
        RECTASK& operator=( const RECTASK& );
};


class DELETERECTASK : public RECTASK
{
    public:
        DELETERECTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm );

        ERR ErrExecuteDbTask( PIB * const ppib );
        VOID HandleError( const ERR err );

    private:
        DELETERECTASK( const DELETERECTASK& );
        DELETERECTASK& operator=( const DELETERECTASK& );
};


template< typename TDelta >
class FINALIZETASK : public RECTASK
{
    public:
        FINALIZETASK(
            const PGNO      pgnoFDP,
            FCB * const     pfcb,
            const IFMP      ifmp,
            const BOOKMARK& bm,
            const USHORT    ibRecordOffset,
            const BOOL      fCallback,
            const BOOL      fDelete );

        ERR ErrExecuteDbTask( PIB * const ppib );
        VOID HandleError( const ERR err );

    private:
        FINALIZETASK( const FINALIZETASK& );
        FINALIZETASK& operator=( const FINALIZETASK& );

        const USHORT    m_ibRecordOffset;
        const BYTE      m_fCallback;
        const BYTE      m_fDelete;
};


class DELETELVTASK : public RECTASK
{
    public:
        DELETELVTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm );

        ERR ErrExecuteDbTask( PIB * const ppib );
        VOID HandleError( const ERR err );

        enum { g_cpgLVDataToDeletePerTrx = 256 };

    private:
        DELETELVTASK( const DELETELVTASK& );
        DELETELVTASK& operator=( const DELETELVTASK& );

        ERR ErrExecute_SynchronousCleanup( PIB * const ppib );
        ERR ErrExecute_LegacyVersioned( PIB * const ppib );
};


class MERGEAVAILEXTTASK : public RECTASK
{
    public:
        MERGEAVAILEXTTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm );

        ERR ErrExecuteDbTask( PIB * const ppib );
        VOID HandleError( const ERR err );

    private:
        MERGEAVAILEXTTASK( const MERGEAVAILEXTTASK& );
        MERGEAVAILEXTTASK& operator=( const MERGEAVAILEXTTASK& );
};

class BATCHRECTASK : public DBTASK
{
    public:
        virtual ERR ErrExecuteDbTask( PIB * const ppib );
        virtual void HandleError( const ERR err );

    public:
        BATCHRECTASK( const PGNO pgnoFDP, const IFMP ifmp );
        virtual ~BATCHRECTASK();

    public:
        PGNO PgnoFDP() const    { return m_pgnoFDP; }
        IFMP Ifmp() const       { return m_ifmp; }

        INT CTasks() const      { return m_iptaskCurr; }

        ERR ErrAddTask( RECTASK * const prectask );

    protected:
        VOID SortTasksByBookmark();

        VOID PrereadTaskBookmarks( PIB * const ppib, const INT itaskStart, __out LONG * pctasksPreread );

    protected:
        const PGNO m_pgnoFDP;

        INT m_cptaskMax;
        INT m_iptaskCurr;
        RECTASK ** m_rgprectask;

    private:
        BATCHRECTASK( const BATCHRECTASK& );
        BATCHRECTASK& operator=( const BATCHRECTASK& );
};

class RECTASKBATCHER
{
    public:
        RECTASKBATCHER(
            INST * const pinst,
            const INT cbatchesMax,
            const INT ctasksPerBatchMax,
            const INT ctasksBatchedMax
            );
        ~RECTASKBATCHER();

        ERR ErrPost( RECTASK * prectask );

        ERR ErrPostAllPending();
        BOOL FTasksPending() { return ( m_rgpbatchtask != NULL && m_ctasksBatched > 0 ); }

    private:
        ERR ErrPostOneBatch( const INT ipbatchtask );

    private:
        INST * const m_pinst;

        const INT m_cbatchesMax;

        const INT m_ctasksPerBatchMax;

        const INT m_ctasksBatchedMax;

        BATCHRECTASK ** m_rgpbatchtask;

        INT m_ctasksBatched;

    private:
        RECTASKBATCHER( const RECTASKBATCHER& );
        RECTASKBATCHER& operator=( const RECTASKBATCHER& );
};

#pragma warning(pop)
