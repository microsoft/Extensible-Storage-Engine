// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma warning(push)
#pragma warning(disable: 4481)

//  ================================================================
class  TASK
//  ================================================================
//
//  These will be deleted after dequeueing. Use "new" to allocate
//
//-
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

//  ================================================================
class DBTASK : public TASK
//  ================================================================
//
//  Tasks that are issued on a specific database.
//
//-
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

//  ================================================================
class DBREGISTEROLD2TASK : public DBTASK
//  ================================================================
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

private:    // not implemented
    DBREGISTEROLD2TASK(const DBREGISTEROLD2TASK&);
    DBREGISTEROLD2TASK& operator=(const DBREGISTEROLD2TASK&);
};


//  ================================================================
class DBEXTENDTASK : public DBTASK
//  ================================================================
//
//  This task is used to extend the database asynchronously.
//
//-
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


//  ================================================================
class RECTASK : public DBTASK
//  ================================================================
//
//  Used for all TASKS that operate on a record
//  This copies the bookmark passed to it and can open a FUCB on the
//  record
//
//-
{
    public:
        virtual ERR ErrExecuteDbTask( PIB * const ppib ) = 0;

    protected:
        RECTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm );

    public:
        virtual ~RECTASK();

        PGNO PgnoFDP() const { return m_pgnoFDP; }
        IFMP Ifmp() const { return m_ifmp; }
        VOID GetBookmark( _Out_ BOOKMARK * const pbookmark ) const;

        // used when the BATCHRECTASK sorts RECTASKs by bookmark
        static bool FBookmarkIsLessThan( const RECTASK * ptask1, const RECTASK * ptask2 );

    protected:

        ERR ErrOpenCursor( PIB * const ppib, FUCB ** ppfucb );
        ERR ErrOpenCursorAndGotoBookmark( PIB * const ppib, FUCB ** ppfucb );

    protected:
        const PGNO      m_pgnoFDP;              //  UNDONE: pgnoFDP probably not need anymore since FCB
        FCB             * const m_pfcb;         //  is now passed in, but keep it for debugging for now
        const ULONG     m_cbBookmarkKey;
        const ULONG     m_cbBookmarkData;

        BYTE m_rgbBookmarkKey[cbKeyAlloc];
        BYTE m_rgbBookmarkData[cbKeyAlloc];

    private:
        RECTASK( const RECTASK& );
        RECTASK& operator=( const RECTASK& );
};


//  ================================================================
class DELETERECTASK : public RECTASK
//  ================================================================
//
//  Perform a finalize callback on the given record
//
//-
{
    public:
        DELETERECTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm );

        ERR ErrExecuteDbTask( PIB * const ppib );
        VOID HandleError( const ERR err );

    private:
        DELETERECTASK( const DELETERECTASK& );
        DELETERECTASK& operator=( const DELETERECTASK& );
};


//  ================================================================
template< typename TDelta >
class FINALIZETASK : public RECTASK
//  ================================================================
//
//  Perform a finalize callback on the given record
//
//-
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
        const BYTE      m_fCallback;        //  should a callback be issued?
        const BYTE      m_fDelete;          //  should the record be deleted?
};


//  ================================================================
class DELETELVTASK : public RECTASK
//  ================================================================
//
//  Flag-deletes a dereferenced LV. Pass the root of the LV to delete
//
//-
{
    public:
        DELETELVTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm );

        ERR ErrExecuteDbTask( PIB * const ppib );
        VOID HandleError( const ERR err );

        //  maximum pages of LV data that we will delete in one transaction
        //
        //  note that the typical fanout of a parent-of-leaf page of an LV tree
        //  with really large long-values should be about 275 using 4k pages and
        //  about 581 using 8k pages (assuming my math is correct), so given this,
        //  I don't know whether the magic number I pulled out of thin air for
        //  this threshold is appropriate or not (assuming the parent-of-leaf
        //  fanout is even really relevant with respect to this threshold)
        //
        enum { g_cpgLVDataToDeletePerTrx = 256 };

    private:
        DELETELVTASK( const DELETELVTASK& );
        DELETELVTASK& operator=( const DELETELVTASK& );

        ERR ErrExecute_SynchronousCleanup( PIB * const ppib );
        ERR ErrExecute_LegacyVersioned( PIB * const ppib );
};


//  ================================================================
class MERGEAVAILEXTTASK : public RECTASK
//  ================================================================
//
//  This attempts to merge a portion of the B-Tree.
//
//  Today this is only performed on the space trees, as all other trees will have
//  merge triggered as a part of version store cleanup.  The space trees need this
//  as they are updated unversioned.
//
//-
{
    public:
        MERGEAVAILEXTTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm );

        ERR ErrExecuteDbTask( PIB * const ppib );
        VOID HandleError( const ERR err );

    private:
        MERGEAVAILEXTTASK( const MERGEAVAILEXTTASK& );
        MERGEAVAILEXTTASK& operator=( const MERGEAVAILEXTTASK& );
};

//  ================================================================
class BATCHRECTASK : public DBTASK
//  ================================================================
//
//  This is used to accumulate a set of RECTASKS that operate on the
//  same b-tree. The tasks are then processed in bookmarks order.
//
//  This is a DBTASK but is not a RECTASK in itself because we don't
//  need to pin the FCB. The RECTASKS contained in each object do that.
//
//-
{
    public:
        virtual ERR ErrExecuteDbTask( PIB * const ppib );
        virtual void HandleError( const ERR err );

    public:
        BATCHRECTASK( const PGNO pgnoFDP, const IFMP ifmp );
        virtual ~BATCHRECTASK();

    public:
        // the pgnoFDP/ifmp that this BATCHRECTASK is batching operations for
        PGNO PgnoFDP() const    { return m_pgnoFDP; }
        IFMP Ifmp() const       { return m_ifmp; }

        // the number of tasks currently batched in this object
        INT CTasks() const      { return m_iptaskCurr; }

        // if ErrAddTask succeeds then the task is owned by the BATCHRECTASK
        ERR ErrAddTask( RECTASK * const prectask );

    protected:
        // reorder the m_rgprectask by bookmark, smallest to largest
        VOID SortTasksByBookmark();

        // preread the bookmarks in the tasks. returns the number of
        // tasks that had their bookmarks preread
        VOID PrereadTaskBookmarks( PIB * const ppib, const INT itaskStart, _Out_ LONG * pctasksPreread );

    protected:
        // the pgnoFDP of the b-tree we are batching operations for
        const PGNO m_pgnoFDP;

        // storing the batched RECTASKs
        INT m_cptaskMax;
        INT m_iptaskCurr;
        RECTASK ** m_rgprectask;

    private:
        BATCHRECTASK( const BATCHRECTASK& );
        BATCHRECTASK& operator=( const BATCHRECTASK& );
};

//  ================================================================
class RECTASKBATCHER
//  ================================================================
//
//  This class is used to group together RECTASKs into BATCHRECTASKs
//  before dispatching them to the task manager.
//
//  This class is _not_ multi-threaded safe. Concurrency control is
//  provided by the version store cleanup thread (critRCEClean).
//
//-
{
    public:
        RECTASKBATCHER(
            INST * const pinst,
            const INT cbatchesMax,
            const INT ctasksPerBatchMax,
            const INT ctasksBatchedMax
            );
        ~RECTASKBATCHER();

        // add a RECTASK to the batch
        ERR ErrPost( RECTASK * prectask );

        // dispatch all batched tasks
        ERR ErrPostAllPending();
        BOOL FTasksPending() { return ( m_rgpbatchtask != NULL && m_ctasksBatched > 0 ); }

    private:
        ERR ErrPostOneBatch( const INT ipbatchtask );

    private:
        // the instance (used to post the tasks)
        INST * const m_pinst;

        // maximum number of different b-trees that tasks will be
        // batched for. when this limit is reached all BATCHRECTASKs
        // are dispatched
        const INT m_cbatchesMax;

        // maximum number of batched tasks for one b-tree. when this
        // limit is reached the BATCHRECTASK for the b-tree is dispatched
        const INT m_ctasksPerBatchMax;

        // maximum number of all batched tasks. when this limit is reached
        // all BATCHRECTASKs are dispatched
        const INT m_ctasksBatchedMax;

        // array of pointers to the BATCHRECTASK objects
        BATCHRECTASK ** m_rgpbatchtask;

        // current number of tasks stored in all the BATCHRECTASKs
        INT m_ctasksBatched;

    private:
        RECTASKBATCHER( const RECTASKBATCHER& );
        RECTASKBATCHER& operator=( const RECTASKBATCHER& );
};

#pragma warning(pop)
