// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

/***********************************************************

Introduction to the version store
=================================

The version store provides several important functions:
- Record locking
- Rollback management
- Visibility calculation
- Deferred cleanup (and runtime scrubbing)
- Deferred before image logging (occasionally - only under page flush pressure)

Record locking
--------------
When a record is modified a Revision Control Entry (RCE) is 
created in the version store. The RCE records the transaction 
id (TRX) and session that modified the record. If another 
session attempts to modify a record that has been modified by 
a session where:
    - the session has not committed
    - the session committed after the current session began
A write conflict error is generated.

Rollback Management
-------------------
Each PIB has a list of RCEs linked off its prceNewest memeber
via the m_prceNextOfSession and m_prcePrevOfSession members
of the RCE. When a session "rollsback" it uses the information
in each RCE to undo the operations performed by the transaction
from oldest to newest.

Deferred before image logging
-----------------------------
There are two types of information that need to be logged when
a modification is made:
    - redo: information needed to redo the operation if the page
            isn't flushed
    - undo: information needed to undo the operation if the page
            is flushed, but the transaction never committed
For replaces the undo information is the before-image of the 
record. For flag deletes the undo information is the bookmark 
of the record.

As an optimization the undo information is logged only if the
page containing the record needing the undo information is
flushed to disk before the transaction finishes. This is
accomplished by:
    - linking the RCE to the buffer containing the record
    - the buffer manager logs the before image if it flushes
      a page linked to a RCE
    - the link to the buffer is removed when the transaction is
      committed or rolledback

Visibility calculation
----------------------
ESE uses a database isolation level called "Snapshot Isolation".
This means that when a transaction begins, it sees the database
as it was when the transaction began. Updates to the records in
the database are immediate so when a session looks at a record
with the version bit set it consults the version store to determine
what the visibility of the record is. There are several things that
can happen:
    - the record is present, but was inserted by a conflicting
      transaction so the session cannot actually see the record
    - the record is flagged as deleted, but was deleted by a
      conflicting transaction so the session can actually see the
      record
    - the record was replaced by a conflicting transaction so
      the session sees the before-image of the replace (stored
      in the RCE)
    - the record was escrowed by a conflicting transaction so
      the escrow operations are undone

Deferred cleanup
----------------
It is not possible to immediately remove a record when it is deleted
or delete a LV when its refcount falls to 0. There are two reasons
for this:
    - rollback would have to re-insert records which would make rollback
      harder and create the forbidden scenario where rollback could
      fail
    - other sessions can still see the records (snapshot isolation)
The version store cleanup thread works through the RCEs from oldest to
newest. If the RCE is committed and the trxCommit is older than the
trxBegin of the oldest current transaction the RCE is cleanable. If the
RCE is a flagDelete or a delta operation on a LV, then version store cleanup
will physically remove the record or flag-delete the LV (a second pass
of version store cleanup will physically remove the LV). Escrow callbacks
are performed by version store cleanup.

As version store cleanup stops when it encounters an uncleanable RCE;
A long-running transaction will cause the version store to become very
large, even if the transaction is not modifying the database (the RCE
is still uncleanable because it committed after the oldest transaction
began).

Starting with Exchange 14 / Windows 8 the version store cleanup also
triggers (just via the various node operations now scrubbing the data)
runtime scrubbing of the data.  This means that when physically removing /
deleting the node we dirty the page AND push pattern fills bytes over the
node data.

*************************************************************

Structures in the version store
===============================

RCE - Revision Control Entry
---
A RCE describes one modification to the database. The two broad types of
RCE are:
    - DML: operations that affect records or the database directly,
           such as insert/replace/delete and space allocation. DML RCEs
           that modify a record need a bookmark, ifmp and pgnoFDP to identify
           a version. A single hash table is used to access DML RCEs that
           modify records. Typical hash overflow is supported
    - DDL: operations that affect the meta-data of the database, such as
           createIndex/deleteColumn etc.

BUCKET
------
A bucket it the basic unit of memory allocation for RCEs. RCEs are appended to
each other in a bucket, starting after the header. Buckets are linked together
from oldest to newest. Version store cleanup traverses the bucket list from oldest
to newest, cleaning RCEs in the bucket from oldest to newest.

VER
---
This is the main per instance structure, hanging off the INST as m_pver.  The VER 
structure contains all the variables for one instance of the version store.

VERREPLACE
VEREXT
VERDELTA
VERADDCOLUMN
------------
These are appended to an RCE. They contain information specific to a particular
operation type. For example VERREPLACE contains the before image of the record
being replaced.

VERPROXY
--------
This is used by recovery and concurrent create index to create an RCE "on behalf"
of a different session.

*************************************************************

Versioning a record modification
================================

1.  Create the version for the operation
    - the version is in the hash table, this locks the record
    - for an insert a preInsert operation is created
2.  Seek into the b-tree and perform the operation, latching the page
3a. If the operation succeeds, insert the RCE into the FCB and PIB lists.
    Attach the RCE to the buffer if deferred logging is needed
3b. If the operation fails, nullify the RCE (effectively deleting it)

*************************************************************

Incompleteness of verstore cleanup
==================================

The version store deferred cleanup (and now scrubbing) is incomplete in that it
misses a few cases, causing both a lack of runtime scrubbing (which is picked
up by Database Maintenance/DBM) and essentially leaking of DB space until either
OLD v1 or OLD v2 or DBM comes along and frees it.

1.  Crashes ... the version store doesn't re-load and retry / re-cleanup things
    it was working on when it crashed.
2.  Rollback ... the version store on "cleanup" of a rollback just leaves inserted
    nodes on the pages without attempting any cleanup (and thus scrubbing).  Deletes
    are obviously un-flag deleted as required.  Replaces are probably fine, as the
    verstore has to put back the old record, and the node operation probably performs
    the necessary scrubbing (if the node shrinks).
3.  Pitched deletes ... since deletes can cause I/O and hold up the version store
    cleanup, the decision was made to not hold up verstore clean over correctness
    here, and just toss out cleanup tasks without doing anything.
    - We don't necessarily have to suffer all the maladies of cleanup falling
      behind here ... we could for instance allow sparse cleanup (to cleanup
      before eviction from buffer cache) and allow the version store to spill
      to disk (to minimize memory presence) and just allow cleanup to eventually
      get to removing the data (perhaps in the middle of the night) ... akin to
      OLDv2 / B+ Tree defrag style.
4.  Table deletes (index drops) ... the version store does not dispatch a task to
    scrub all the pages that formerly belonged to the tree.
    - Even worse in the case of a crash during table delete ... the data is
      purely leaked in that neither OLD nor DBM will ever get the page space
      back.  It will get scrubbed however as dead table space by objid.  Offline 
      defrag will "free" it for re-use of course, as well if DB Shrink happens 
      to run across that space.

*************************************************************/

//  Forward delcaration

class INST;
class VER;
INLINE VER *PverFromIfmp( IFMP ifmp );

//  ****************************************************************
//  MACROS
//  ****************************************************************

///#define VERPERF
#ifdef VERPERF
//  VERHASH_PERF:  expensive checks on efficency of version store hashing
///#define VERHASH_PERF
#endif  //  VERPERF

//  ****************************************************************
//  DEBUG
//  ****************************************************************


#ifdef DEBUG
BOOL FIsRCECleanup();   //  is the current thread a RCE cleanup thread?
BOOL FInCritBucket( VER *pver );        //  are we in the bucket allocation critical section?
#endif  //  DEBUG


//  ================================================================
enum NS
//  ================================================================
//
//  returned by NsVERAccessNode
//
//-
{
    nsNone,                 //  should never be returned
    nsDatabase,             //  node is not versioned, must consult db for visibility
    nsVersionedUpdate,      //  node has non-visible versioned operation, but the node itself is visible
    nsVersionedInsert,      //  node has non-visible versioned insert, so node is not visible
    nsUncommittedVerInDB,   //  node has visible uncommitted version, must consult db for visibility
    nsCommittedVerInDB      //  node has visible committed version, must consult db for visibility
};


//  ================================================================
enum VS
//  ================================================================
//
//  returned by VsVERCheck
//
//-
{
    vsNone,     //  should never be returned
    vsCommitted,
    vsUncommittedByCaller,
    vsUncommittedByOther
};

//  This function checks if the bookmark is visible to the current transaction.
//
//  Incidentally one of the more performance sensitive functions of ESE.
//
VS  VsVERCheck( const FUCB * pfucb, CSR * pcsr, const BOOKMARK& bookmark );
INLINE VS VsVERCheck( const FUCB * pfucb, const BOOKMARK& bookmark )
{
    return VsVERCheck( pfucb, Pcsr( const_cast<FUCB *>( pfucb ) ), bookmark );
}

//  ================================================================
INLINE INT TrxCmp( TRX trx1, TRX trx2 )
//  ================================================================
//
//  TrxMax has to sort greater than anything else
//
//  Take 8 bits number as an example:
//      if both are positive, then return wac1.l - wac2.l
//          e.g. 0000000-0000001 (0) - (1) = -1
//      else if both are negative, then still return wac1.l - wac2.l
//          e.g. 1000000-1000001 (-128) - (-127) = -1
//      else one of them is negative, then the bigger value has the HighVal bit reset.
//          e.g. 01xxxxx -> 10xxxxx the latter will be bigger
//          e.g. 11xxxxx -> 00xxxxx the latter will be bigger
//
//
{
    LONG lTrx1 = (LONG)trx1;
    LONG lTrx2 = (LONG)trx2;

    if( trx1 == trx2 )
    {
        return 0;
    }
    else if( trxMax == trx2 )
    {
        return -1;
    }
    else if( trxMax == trx1 )
    {
        return 1;
    }
    else
    {
        return lTrx1 - lTrx2;
    }
}

//  ================================================================
INLINE INT RceidCmp( RCEID rceid1, RCEID rceid2 )
//  ================================================================
//
//  Wraparound-aware comparison for RCEIDs. rceidNull has to compare
//  as less than anything.
//
//-
{
    const LONG lRceid1 = (LONG)rceid1;
    const LONG lRceid2 = (LONG)rceid2;

    if( rceid1 == rceid2 )
    {
        return 0;
    }
    else if( rceidNull== rceid1 )
    {
        return -1;
    }
    else if( rceidNull == rceid2 )
    {
        return 1;
    }
    else
    {
        return lRceid1 - lRceid2;
    }
}

//  operation type
//  these describe all the different operations that RCEs can be created for

typedef UINT OPER;

const OPER operMaskNull                 = 0x1000;   //  to void an RCE
const OPER operMaskNullForMove          = 0x2000;   //  to void an RCE being moved (for debugging only)
const OPER operMaskDDL                  = 0x0100;

//  DML operations

//  insertion of a record
const OPER operInsert                   = 0x0001;
//  modification of a record. the before-image is stored in the RCE
const OPER operReplace                  = 0x0002;
//  setting the deleted flag on a record
const OPER operFlagDelete               = 0x0003;
//  escrow-updating a number (similar to atomic addition) this is used
//  by the escrow update code and to keep track of the refcount of long-values
const OPER operDelta                    = 0x0004;
//  read-lock a record. other read-locks by other transactions will succeed
const OPER operReadLock                 = 0x0005;
//  write-lock a record. used when a replace is prepared on a record
const OPER operWriteLock                = 0x0006;
//  created when a record is about to be replaced. this is used to check for
//  duplicates in the version store and to lock the record. at the time this
//  version is created we don't know if there is a duplicate (a record with the
//  same key) in the b-tree so it isn't clear if the insert will succeed.
//  after the insert succeeds a preInsert is turned into an insert
//  we use a pre-insert RCE as creating an insert RCE for a record that exists
//  in the b-tree would change its visibility
const OPER operPreInsert                = 0x0080;
//  used to version database space allocation
const OPER operAllocExt                 = 0x0007;
//  escrow-updating a 64-bit column (similar to atomic addition) this is used
//  by the escrow update code. Not used to refcount long-values.
const OPER operDelta64                  = 0x0009;

//  DDL operations
const OPER operCreateTable              = 0x0100;
const OPER operDeleteTable              = 0x0300;
const OPER operAddColumn                = 0x0700;
const OPER operDeleteColumn             = 0x0900;
const OPER operCreateLV                 = 0x0b00;
const OPER operCreateIndex              = 0x0d00;
const OPER operDeleteIndex              = 0x0f00;

const OPER operRegisterCallback         = 0x0010;
const OPER operUnregisterCallback       = 0x0020;


//  ================================================================
INLINE BOOL FOperNull( const OPER oper )
//  ================================================================
{
    return ( oper & operMaskNull );
}


//  ================================================================
INLINE BOOL FOperDDL( const OPER oper )
//  ================================================================
{
    return ( oper & operMaskDDL ) && !FOperNull( oper );
}


//  ================================================================
INLINE BOOL FOperInHashTable( const OPER oper )
//  ================================================================
{
    return  !FOperNull( oper )
            && !FOperDDL( oper )
            && operAllocExt != oper
            && operCreateLV != oper;
}


//  ================================================================
INLINE BOOL FOperReplace( const OPER oper )
//  ================================================================
{
    return ( operReplace == oper );
}


//  ================================================================
INLINE BOOL FOperConcurrent( const OPER oper )
//  ================================================================
//
//  Can this operation be done by more than one session?
//
//-
{
    return ( operDelta == oper
            || operDelta64 == oper
            || operWriteLock == oper
            || operPreInsert == oper
            || operReadLock == oper );
}


//  ================================================================
INLINE BOOL FOperAffectsSecondaryIndex( const OPER oper )
//  ================================================================
//
//  Possible primary index operations that would affect a secondary index.
//
//-
{
    return ( operInsert == oper || operFlagDelete == oper || operReplace == oper );
}


//  ================================================================
INLINE BOOL FUndoableLoggedOper( const OPER oper )
//  ================================================================
{
    return (    oper == operReplace
                || oper == operInsert
                || oper == operFlagDelete
                || oper == operDelta
                || oper == operDelta64 );
}


const UINT uiHashInvalid = UINT_MAX;


//  ================================================================
class RCE
//  ================================================================
{
#ifdef DEBUGGER_EXTENSION
    friend VOID EDBGVerHashSum( INST * pinstDebuggee, BOOL fVerbose );
#endif

    public:
        RCE (
            FCB *       pfcb,
            FUCB *      pfucb,
            UPDATEID    updateid,
            TRX         trxBegin0,
            TRX *       ptrxCommit0,
            LEVEL       level,
            USHORT      cbBookmarkKey,
            USHORT      cbBookmarkData,
            USHORT      cbData,
            OPER        oper,
            UINT        uiHash,
            BOOL        fProxy,
            RCEID       rceid
            );

        VOID    AssertValid ()  const;

    public:
        BOOL    FOperNull           ()  const;
        BOOL    FOperDDL            ()  const;
        BOOL    FUndoableLoggedOper ()  const;
        BOOL    FOperInHashTable    ()  const;
        BOOL    FOperReplace        ()  const;
        BOOL    FOperConcurrent     ()  const;
        BOOL    FOperAffectsSecondaryIndex  ()  const;
        BOOL    FActiveNotByMe      ( const PIB * ppib, const TRX trxSession ) const;

        BOOL    FRolledBack         ()  const;
        BOOL    FRolledBackEDBG     ()  const;
        BOOL    FMoved              ()  const;
        BOOL    FProxy              ()  const;
        BOOL    FEmptyDiff          ()  const;

        const   BYTE    *PbData         ()  const;
        const   BYTE    *PbBookmark     ()  const;
                INT     CbBookmarkKey   ()  const;
                INT     CbBookmarkData  ()  const;
                INT     CbData          ()  const;
                INT     CbRce           ()  const;
                INT     CbRceEDBG       ()  const;
        VOID    GetBookmark ( BOOKMARK * pbookmark ) const;
        VOID    CopyBookmark( const BOOKMARK& bookmark );
        ERR     ErrGetTaskForDelete( VOID ** ppvtask ) const;

        RCE     *PrceHashOverflow   ()  const;
        RCE     *PrceHashOverflowEDBG() const;
        RCE     *PrceNextOfNode     ()  const;
        RCE     *PrcePrevOfNode     ()  const;
        RCE     *PrceNextOfSession  ()  const;
        RCE     *PrcePrevOfSession  ()  const;
        BOOL    FFutureVersionsOfNode   ()  const;
        RCE     *PrceNextOfFCB      ()  const;
        RCE     *PrcePrevOfFCB      ()  const;
        RCE     *PrceUndoInfoNext ()    const;
        RCE     *PrceUndoInfoPrev ()    const;

        PGNO    PgnoUndoInfo        ()  const;

        RCEID   Rceid               ()  const;
        TRX     TrxBegin0           ()  const;

        LEVEL   Level       ()  const;
        IFMP    Ifmp        ()  const;
        PGNO    PgnoFDP     ()  const;
        OBJID   ObjidFDP    ()  const;
        TRX     TrxCommitted()  const;
        TRX     TrxCommittedEDBG()  const;
        BOOL    FFullyCommitted() const;

        UINT                UiHash      ()  const;
        CReaderWriterLock&  RwlChain    ();

        FUCB    *Pfucb      ()  const;
        FCB     *Pfcb       ()  const;
        OPER    Oper        ()  const;

        UPDATEID    Updateid()  const;

#ifdef DEBUGGER_EXTENSION
        VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif  //  DEBUGGER_EXTENSION

    public:
        BYTE    *PbData             ();
        BYTE    *PbBookmark         ();

        RCE *&  PrceHashOverflow    ();

        VOID    ChangeOper          ( const OPER operNew );
        VOID    NullifyOper         ();
        ERR     ErrPrepareToDeallocate( TRX trxOldest );
        VOID    NullifyOperForMove  ();

        BOOL    FRCECorrectEDBG( IFMP ifmp, PGNO pgnoFDP, const BOOKMARK& bookmark );

        VOID    FlagRolledBack      ();
        VOID    FlagMoved           ();
        VOID    FlagEmptyDiff       ();

        VOID    SetLevel            ( LEVEL level );

        VOID    SetPrceHashOverflow ( RCE * prce );
        VOID    SetPrceNextOfNode   ( RCE * prce );
        VOID    SetPrcePrevOfNode   ( RCE * prce );
        VOID    SetPrcePrevOfSession( RCE * prce );
        VOID    SetPrceNextOfSession( RCE * prce );
        VOID    SetPrceNextOfFCB    ( RCE * prce );
        VOID    SetPrcePrevOfFCB    ( RCE * prce );

        VOID    SetTrxCommitted     ( const TRX trx );

        VOID    AddUndoInfo     ( PGNO pgno, RCE *prceNext, const BOOL fMove = fFalse );
        VOID    RemoveUndoInfo  ( const BOOL fMove = fFalse );

        VOID    SetCommittedByProxy( TRX trxCommit );

    private:
        //  a private destructor prevents the creation of stack-based RCE's. Always use placement new
                ~RCE        ();
                RCE         ( const RCE& );
        RCE&    operator=   ( const RCE& );

#ifdef DEBUG
    private:
        BOOL    FAssertRwlHash_() const;
        BOOL    FAssertRwlHashAsWriter_() const;
        BOOL    FAssertCritFCBRCEList_() const;
        BOOL    FAssertRwlPIB_() const;
        BOOL    FAssertReadable_() const;
#endif  //  DEBUG

    private:


        const TRX           m_trxBegin0;

        TRX                 m_trxCommittedInactive; //  time when this RCE was committed (valid only when RCE is inactive)
        volatile TRX *      m_ptrxCommitted;        //  pointer to trxCommitted (made VOLATILE to guarantee that convergence loop in TrxCommitted() will see sudden changes)

        const IFMP          m_ifmp;                 //  ifmp
        //  16 bytes

        const PGNO          m_pgnoFDP;              //  pgnoFDP

        union
        {
            ULONG           m_ulFlags;
            // OPTIMIZER RISK HERE!  Note that the m_level the way it's changed w/in
            // the RCE can oscillate to 0 temporarily ... so if someone is checking an
            // RCE concurrently they could read an inconsistent state.  Couldn't prove 
            // there was a read from an unlocked path that might happen concurrently 
            // with the SetLevel() call.
            // Here is the SetLevel() code inlined from in VERCommitTransaction():
            //  ese!VERCommitTransaction+0x2b [e:\src\e15\esedt2\sources\dev\ese\src\ese\ver.cxx @ 6727]:
            //   6727 00000000'543e112b 83621ce0        and     dword ptr [rdx+1Ch],0FFFFFFE0h  <-- sets RCE m_level to 0. Danger!
            //   6727 00000000'543e112f 418d40ff        lea     eax,[r8-1]
            //   6727 00000000'543e1133 83e01f          and     eax,1Fh
            //   6727 00000000'543e1136 09421c          or      dword ptr [rdx+1Ch],eax         <-- sets it back to m_level - 1.
            //   6727 00000000'543e1139 488b5248        mov     rdx,qword ptr [rdx+48h]
            //   6727 00000000'543e113d 4885d2          test    rdx,rdx
            struct
            {
                UINT        m_level:5;              //  current transaction level. can change
                UINT        m_fRolledBack:1;        //  first phase of rollback is done
                UINT        m_fMoved:1;             //  RCE has been moved in the buckets
                UINT        m_fProxy:1;             //  RCE was created by proxy
                UINT        m_fEmptyDiff:1;         //  RCE represents an empty diff
            };
        };

        USHORT              m_oper;                 //  operation that RCE represents
        const USHORT        m_cbBookmarkKey;        //  length of key portion of bookmark
        const USHORT        m_cbBookmarkData;       //  length of data portion of bookmark
        const USHORT        m_cbData;               //  length of data portion of node
        //  32 bytes

        const RCEID         m_rceid;
        const UPDATEID      m_updateid;             //  for cancelling an update

        FCB * const         m_pfcb;                 //  for clean up    (committed-only)
        FUCB * const        m_pfucb;                //  for undo        (uncommitted-only)
        //  48 bytes

        RCE *               m_prceNextOfSession;    //  next RCE of same session
        RCE *               m_prcePrevOfSession;    //  previous RCE of same session
        RCE *               m_prceNextOfFCB;        //  next RCE of same table
        RCE *               m_prcePrevOfFCB;        //  prev RCE of same table
        //  64 bytes

        RCE *               m_prceNextOfNode;       //  next RCE for same node
        RCE *               m_prcePrevOfNode;       //  previous RCE for same node

        RCE *               m_prceUndoInfoNext;     //  link list for deferred before image.
        RCE *               m_prceUndoInfoPrev;     //  if Prev is NULL, BF points to RCE
        //  80 bytes

        volatile PGNO       m_pgnoUndoInfo;         //  which page deferred before image is on.

        const UINT          m_uiHash;               //  cached hash value of RCE
        RCE *               m_prceHashOverflow;     //  hash overflow RCE chain
        //  92 bytes

        //  WARNING! WARNING!
        //  m_rgbData should be aligned on a pointer boundary (4-bytes for X86, 8-bytes
        //  for a 64-bit build) as we store pointers in m_rgbData (e.g. FCB pointers for
        //  operCreateIndex) make a pointer member the last member before m_rgbData to
        //  make sure the alignment is correct

        BYTE                m_rgbData[0];           //  this stores the BOOKMARK of the node and then possibly the data portion
};

RCE * const prceNil     = 0;
#ifdef DEBUG
#ifdef _WIN64
RCE * const prceInvalid = (RCE *)0xFEADFEADFEADFEAD;
#else
RCE * const prceInvalid = (RCE *)(DWORD_PTR)0xFEADFEAD;     //  HACK: cast to DWORD_PTR then to RCE* in order to permit compiling with /Wp64
#endif
#endif  //  DEBUG


INLINE RCEID Rceid( const RCE * prce )
{
    if ( prceNil == prce )
    {
        return rceidNull;
    }
    else
    {
        return prce->Rceid();
    }
}


//  ================================================================
INLINE BOOL RCE::FOperNull() const
//  ================================================================
{
    return ::FOperNull( m_oper );
}


//  ================================================================
INLINE TRX RCE::TrxCommitted() const
//  ================================================================
{
    Assert( FIsRCECleanup() || FAssertReadable_() );

//  There's an infinitessimally small window where we
//  could retrieve a trxCommit0 by taking a snapshot
//  of trxNewest, but before we can transfer the value
//  to the ppib of the committing session, someone else
//  begins a new transaction and then starts inspecting
//  RCE's of the commit session.  He will see these RCE's
//  as uncommitted until the trxCommit0 in the committing
//  session's ppib can be updated, at which point the
//  RCE's will suddenly look committed, resulting in
//  inconsistent results.
//
//  The way SOMEONE figured out we can prevent this is to
//  flag the trxCommit0 in the committing session's ppib
//  with a special sentinel value (trxPrecommit), then
//  anyone who tries to inspect an RCE and sees this value
//  must wait briefly until the the value is updated to
//  the true trxCommit0 value.
//

    TRX trx;

    while ( trxPrecommit == ( trx = *m_ptrxCommitted ) )
    {
//      To see this in action, enable the following AssertTrack
//      and run accept. It should get hit fairly quickly.
//      AssertTrack( fFalse, "TrxAboutToCommit" );
        UtilSleep( 1 );
    }

    return trx;
}

//  ================================================================
INLINE TRX RCE::TrxCommittedEDBG() const
//  ================================================================
{
    //  HACK: allows debugger extensions to extract trxCommitted
    //  without tripping over the assert above (don't care if
    //  RCE is in the middle of being committed, so just return
    //  m_trxCommittedInactive, which saves us from having to deal
    //  with deref'ing ptrxCommitted)
    //
    return m_trxCommittedInactive;
}

//  ================================================================
INLINE BOOL RCE::FFullyCommitted() const
//  ================================================================
{
    return ( trxMax != m_trxCommittedInactive );
}

//  ================================================================
INLINE INT RCE::CbRce () const
//  ================================================================
{
    Assert( FIsRCECleanup() || FInCritBucket( PverFromIfmp( m_ifmp ) ) || FAssertReadable_() );
    return sizeof(RCE) + m_cbData + m_cbBookmarkKey + m_cbBookmarkData;
}
//  ================================================================
INLINE INT RCE::CbRceEDBG () const
//  ================================================================
{
    //  HACK: allows debugger extensions to extract cbRCE
    //  without tripping over the assert above
    return sizeof(RCE) + m_cbData + m_cbBookmarkKey + m_cbBookmarkData;
}


//  ================================================================
INLINE INT RCE::CbData () const
//  ================================================================
{
    Assert( FInCritBucket( PverFromIfmp( m_ifmp ) ) || FAssertReadable_() );
    return m_cbData;
}


//  ================================================================
INLINE INT RCE::CbBookmarkKey() const
//  ================================================================
{
    Assert( FInCritBucket( PverFromIfmp( m_ifmp ) ) || FAssertReadable_() );
    return m_cbBookmarkKey;
}


//  ================================================================
INLINE INT RCE::CbBookmarkData() const
//  ================================================================
{
    Assert( FInCritBucket( PverFromIfmp( m_ifmp ) ) || FAssertReadable_() );
    return m_cbBookmarkData;
}


//  ================================================================
INLINE RCEID RCE::Rceid() const
//  ================================================================
{
    Assert( FAssertReadable_() );
    return m_rceid;
}


//  ================================================================
INLINE TRX RCE::TrxBegin0 () const
//  ================================================================
{
    Assert( FAssertReadable_() );
    return m_trxBegin0;
}


//  ================================================================
INLINE BOOL RCE::FOperDDL() const
//  ================================================================
{
    Assert( FAssertReadable_() );
    return ::FOperDDL( m_oper );
}


//  ================================================================
INLINE BOOL RCE::FOperInHashTable() const
//  ================================================================
{
    Assert( FAssertReadable_() );
    return ::FOperInHashTable( m_oper );
}


//  ================================================================
INLINE BOOL RCE::FUndoableLoggedOper() const
//  ================================================================
{
    Assert( FAssertReadable_() );
    return ::FUndoableLoggedOper( m_oper );
}


//  ================================================================
INLINE BOOL RCE::FOperReplace() const
//  ================================================================
{
    Assert( FAssertReadable_() );
    return ::FOperReplace( m_oper );
}


//  ================================================================
INLINE BOOL RCE::FOperConcurrent() const
//  ================================================================
{
    Assert( FAssertReadable_() );
    return ::FOperConcurrent( m_oper );
}


//  ================================================================
INLINE BOOL RCE::FOperAffectsSecondaryIndex() const
//  ================================================================
{
    Assert( FAssertReadable_() );
    return ::FOperAffectsSecondaryIndex( m_oper );
}

//  ================================================================
INLINE BYTE * RCE::PbData()
//  ================================================================
{
    //  m_rgbData contains pointers (e.g. a FCB*). To avoid alignment
    //  faults, the address of m_rgbData should be aligned on a pointer
    //  boundary
    Assert( FAssertReadable_() );
    AssertRTL( 0 == (LONG_PTR)(m_rgbData) % sizeof( LONG_PTR ) );
    return m_rgbData;
}


//  ================================================================
INLINE const BYTE * RCE::PbData() const
//  ================================================================
{
    Assert( FAssertReadable_() );
    AssertRTL( 0 == (LONG_PTR)(m_rgbData) % sizeof( LONG_PTR ) );
    return m_rgbData;
}


//  ================================================================
INLINE const BYTE * RCE::PbBookmark() const
//  ================================================================
{
    Assert( FAssertReadable_() );
    return m_rgbData + m_cbData;
}


//  ================================================================
INLINE VOID RCE::GetBookmark( BOOKMARK * pbookmark ) const
//  ================================================================
{
    Assert( FAssertReadable_() );
    pbookmark->key.prefix.Nullify();
    pbookmark->key.suffix.SetPv( const_cast<BYTE *>( PbBookmark() ) );
    pbookmark->key.suffix.SetCb( CbBookmarkKey() );
    pbookmark->data.SetPv( const_cast<BYTE *>( PbBookmark() ) + CbBookmarkKey() );
    pbookmark->data.SetCb( CbBookmarkData() );
    ASSERT_VALID( pbookmark );
}


//  ================================================================
INLINE VOID RCE::CopyBookmark( const BOOKMARK& bookmark )
//  ================================================================
{
    bookmark.key.CopyIntoBuffer( m_rgbData + m_cbData );
    UtilMemCpy( m_rgbData + m_cbData + bookmark.key.Cb(), bookmark.data.Pv(), bookmark.data.Cb() );
}


//  ================================================================
INLINE UINT RCE::UiHash() const
//  ================================================================
{
    Assert( FIsRCECleanup() || FAssertReadable_() );
    Assert( uiHashInvalid != m_uiHash );

    // Note: that this value is sometimes accessed directly to avoid those asserts.

    
    return m_uiHash;
}


//  ================================================================
INLINE IFMP RCE::Ifmp() const
//  ================================================================
{
    Assert( FIsRCECleanup() || FAssertReadable_() );
    return m_ifmp;
}


//  ================================================================
INLINE PGNO RCE::PgnoFDP() const
//  ================================================================
{
    Assert( FAssertReadable_() );
    return m_pgnoFDP;
}

//  ================================================================
INLINE OBJID RCE::ObjidFDP() const
//  ================================================================
{
    Assert( FAssertReadable_() );
    return Pfcb()->ObjidFDP();
}


//  ================================================================
INLINE FUCB *RCE::Pfucb() const
//  ================================================================
{
    Assert( FAssertReadable_() );
    Assert( !FFullyCommitted() );   // pfucb only valid for uncommitted RCEs
    return m_pfucb;
}


//  ================================================================
INLINE UPDATEID RCE::Updateid() const
//  ================================================================
{
    Assert( FAssertReadable_() );
    Assert( !FFullyCommitted() );   // pfucb only valid for uncommitted RCEs
    return m_updateid;
}


//  ================================================================
INLINE FCB *RCE::Pfcb() const
//  ================================================================
{
    Assert( FAssertReadable_() );
    return m_pfcb;
}


//  ================================================================
INLINE OPER RCE::Oper() const
//  ================================================================
{
    Assert( FIsRCECleanup() || FAssertReadable_() );
    return m_oper;
}


//  ================================================================
INLINE BOOL RCE::FActiveNotByMe( const PIB * ppib, const TRX trxSession ) const
//  ================================================================
{
    Assert( FAssertReadable_() );

    const TRX   trxCommitted    = TrxCommitted();

    //  normally, this session's Begin0 cannot be equal to anyone else's Commit0,
    //  but because we only log an approximate trxCommit0, during recovery, we
    //  may find that this session's Begin0 is equal to someone else's Commit0
    Assert( trxSession != trxCommitted || PinstFromPpib( ppib )->FRecovering() );
    const BOOL  fActiveNotByMe  = ( trxMax == trxCommitted && m_pfucb->ppib != ppib )
                                    || ( trxMax != trxCommitted && TrxCmp( trxCommitted, trxSession ) > 0 );
    return fActiveNotByMe;
}


//  ================================================================
INLINE RCE *RCE::PrceHashOverflow() const
//  ================================================================
{
    Assert( FAssertRwlHash_() );
    return m_prceHashOverflow;
}


//  ================================================================
INLINE RCE *RCE::PrceHashOverflowEDBG() const
//  ================================================================
{
    //  HACK: allows debugger extensions to extract prceHashOverflow
    //  without tripping over the assert above
    return m_prceHashOverflow;
}


//  ================================================================
INLINE RCE *RCE::PrceNextOfNode() const
//  ================================================================
{
    Assert( FAssertRwlHash_() );
    return m_prceNextOfNode;
}


//  ================================================================
INLINE RCE *RCE::PrcePrevOfNode() const
//  ================================================================
{
    Assert( FAssertRwlHash_() );
    return m_prcePrevOfNode;
}


//  ================================================================
INLINE BOOL RCE::FFutureVersionsOfNode() const
//  ================================================================
{
    return prceNil != m_prceNextOfNode;
}


//  ================================================================
INLINE LEVEL RCE::Level() const
//  ================================================================
{
    Assert( PinstFromIfmp( Ifmp() )->FRecovering() || !FFullyCommitted() );
    return (LEVEL)m_level;
}


//  ================================================================
INLINE RCE *RCE::PrceNextOfSession() const
//  ================================================================
{
    return m_prceNextOfSession;
}


//  ================================================================
INLINE RCE *RCE::PrcePrevOfSession() const
//  ================================================================
{
    return m_prcePrevOfSession;
}


//  ================================================================
INLINE BOOL RCE::FRolledBack() const
//  ================================================================
{
    Assert( FIsRCECleanup() || FAssertReadable_() );
    return m_fRolledBack;
}
//  ================================================================
INLINE BOOL RCE::FRolledBackEDBG() const
//  ================================================================
{
    //  HACK: allows debugger extensions to extract fRolledBack
    //  without tripping over the assert above
    return m_fRolledBack;
}


//  ================================================================
INLINE BOOL RCE::FMoved() const
//  ================================================================
{
/// Assert( FAssertReadable_() );
    return m_fMoved;
}


//  ================================================================
INLINE BOOL RCE::FProxy() const
//  ================================================================
{
/// Assert( FAssertReadable_() );
    return m_fProxy;
}


//  ================================================================
INLINE BOOL RCE::FEmptyDiff() const
//  ================================================================
{
/// Assert( FAssertReadable_() );
    return m_fEmptyDiff;
}


//  ================================================================
INLINE VOID RCE::FlagEmptyDiff()
//  ================================================================
{
    m_fEmptyDiff = fTrue;
}


//  ================================================================
INLINE RCE *RCE::PrceNextOfFCB() const
//  ================================================================
{
    Assert( PinstFromIfmp( Ifmp() )->FRecovering() || FAssertCritFCBRCEList_() );
    return m_prceNextOfFCB;
}


//  ================================================================
INLINE RCE *RCE::PrcePrevOfFCB() const
//  ================================================================
{
    Assert( PinstFromIfmp( Ifmp() )->FRecovering() || FAssertCritFCBRCEList_() );
    return m_prcePrevOfFCB;
}


//  ================================================================
INLINE VOID RCE::SetPrcePrevOfFCB( RCE * prce )
//  ================================================================
{
    Assert( PinstFromIfmp( Ifmp() )->FRecovering() || FAssertCritFCBRCEList_() );
    m_prcePrevOfFCB = prce;
}


//  ================================================================
INLINE VOID RCE::SetPrceNextOfFCB( RCE * prce )
//  ================================================================
{
    Assert( PinstFromIfmp( Ifmp() )->FRecovering() || FAssertCritFCBRCEList_() );
    m_prceNextOfFCB = prce;
}


//  ================================================================
INLINE RCE * RCE::PrceUndoInfoNext() const
//  ================================================================
{
    return m_prceUndoInfoNext;
}

//  ================================================================
INLINE RCE * RCE::PrceUndoInfoPrev() const
//  ================================================================
{
    return m_prceUndoInfoPrev;
}

//  ================================================================
INLINE PGNO RCE::PgnoUndoInfo() const
//  ================================================================
{
    return m_pgnoUndoInfo;
}


//  ================================================================
INLINE VOID RCE::AddUndoInfo( PGNO pgno, RCE *prceNext, const BOOL fMove )
//  ================================================================
{
    //  inserts in undo-info list before prceNext
    //  which is first in list
    //
    Assert( prceInvalid == m_prceUndoInfoNext );
    Assert( prceInvalid == m_prceUndoInfoPrev );
    Assert( prceNil == prceNext ||
            prceNil == prceNext->PrceUndoInfoPrev() );
    Assert( !fMove && pgnoNull == m_pgnoUndoInfo ||
            fMove && pgnoNull != m_pgnoUndoInfo );

    m_prceUndoInfoNext = prceNext;
    m_prceUndoInfoPrev = prceNil;

    if ( prceNext != prceNil )
    {
        Assert( prceNil == prceNext->m_prceUndoInfoPrev );
        prceNext->m_prceUndoInfoPrev = this;
    }

    m_pgnoUndoInfo = pgno;
}


//  ================================================================
INLINE VOID RCE::RemoveUndoInfo( const BOOL fMove )
//  ================================================================
{
    Assert( pgnoNull != m_pgnoUndoInfo );

    if ( prceNil != m_prceUndoInfoPrev )
    {
        Assert( this == m_prceUndoInfoPrev->m_prceUndoInfoNext );
        m_prceUndoInfoPrev->m_prceUndoInfoNext = m_prceUndoInfoNext;
    }

    if ( prceNil != m_prceUndoInfoNext )
    {
        Assert( this == m_prceUndoInfoNext->m_prceUndoInfoPrev );
        m_prceUndoInfoNext->m_prceUndoInfoPrev = m_prceUndoInfoPrev;
    }

#ifdef DEBUG
    m_prceUndoInfoPrev = prceInvalid;
    m_prceUndoInfoNext = prceInvalid;
#endif

    if ( !fMove )
    {
        m_pgnoUndoInfo = pgnoNull;
    }
}


INLINE VOID RCE::SetCommittedByProxy( TRX trxCommitted )
{
    Assert( TrxCommitted() == trxMax );

    // RCE is committed, so don't insert into any session list.
    m_prceNextOfSession = prceNil;
    m_prcePrevOfSession = prceNil;

    m_level = 0;

    Assert( m_ptrxCommitted == &m_trxCommittedInactive );
    m_trxCommittedInactive = trxCommitted;
}


//  ================================================================
INLINE VOID RCE::ChangeOper( const OPER operNew )
//  ================================================================
//
//-
{
    Assert( operPreInsert == m_oper );
    Assert( USHORT( operNew ) == operNew );
    m_oper = USHORT( operNew );
}


//  ================================================================
struct VERREPLACE
//  ================================================================
{
    SHORT   cbMaxSize;
    SHORT   cbDelta;
    RCEID   rceidBeginReplace;  //  the RCEID when we did the PrepareUpdate
    BYTE    rgbBeforeImage[0];
};

const INT cbReplaceRCEOverhead = sizeof( VERREPLACE );


//  ================================================================
struct VEREXT
//  ================================================================
//
//  free extent parameter block
//
//-
{
    PGNO    pgnoFDP;
    PGNO    pgnoChildFDP;
    PGNO    pgnoFirst;
    CPG     cpgSize;
};


//  ================================================================
template< typename TDelta > struct _VERDELTA;
//  ================================================================
//
//  LV delta and delta64 parameter block
//
//-

template< typename TDelta >
struct VERDELTA_TRAITS
{
    static_assert( sizeof( TDelta ) == -1, "Specialize VERDELTA_TRAITS to map a VERDELTA to a specific verstore oper constant." );  // sizeof( TDelta ) == -1 forces the static_assert evaluation to happen at template instantiation
    static const OPER oper = 0;
};

template<> struct VERDELTA_TRAITS< LONG >           { static const OPER oper = operDelta; };
template<> struct VERDELTA_TRAITS< LONGLONG >       { static const OPER oper = operDelta64; };

template< typename TDelta >
struct _VERDELTA
{
    TDelta          tDelta;
    USHORT          cbOffset;
    USHORT          fDeferredDelete:1;
    USHORT          fCallbackOnZero:1;
    USHORT          fDeleteOnZero:1;

    typedef VERDELTA_TRAITS< TDelta > TRAITS;
};

typedef _VERDELTA< LONG >       VERDELTA32;
typedef _VERDELTA< LONGLONG >   VERDELTA64;

//  ================================================================
struct VERADDCOLUMN
//  ================================================================
//
//  Add column parameter block
//
//-
{
    JET_COLUMNID    columnid;
    BYTE            *pbOldDefaultRec;
};


//  ================================================================
struct VERCALLBACK
//  ================================================================
//
//  callback parameter block
//
//-
{
    JET_CALLBACK    pcallback;
    JET_CBTYP       cbtyp;
    VOID            *pvContext;
    CBDESC          *pcbdesc;
};


enum PROXYTYPE
{
    proxyRedo,
    proxyCreateIndex
};
//  ================================================================
struct VERPROXY
//  ================================================================
{
    union
    {
        RCEID   rceid;              //  for proxyRedo
        RCE     *prcePrimary;       //  for proxyCreateIndex
    };

    PROXYTYPE   proxy:8;
    ULONG       level:8;            //  for proxyRedo
    ULONG       cbitReserved:16;
};

//  ================================================================
struct VERDELETETABLEDATA
//  ================================================================
{
    PGNO pgnoFDP;
    PGNO pgnoFDPLV;
    BOOL fRevertableTableDelete;
};

//  ****************************************************************
//  FUNCTION/GLOBAL DECLARATIONS
//  ****************************************************************


extern volatile LONG g_crefVERCreateIndexLock;


//  Transactions
//  ================================================================
INLINE VOID VERBeginTransaction( PIB *ppib, const TRXID trxid )
//  ================================================================
//
//  Increment the session transaction level.
//
//-
{
    ASSERT_VALID( ppib );

    //  increment session transaction level.
    ppib->IncrementLevel( trxid );
    Assert( ppib->Level() < levelMax );

#ifdef NEVER
    // 2.24.99:  we don't need this, as a defer-begin transaction will get this information
    LOG *plog = PinstFromPpib( ppib )->m_plog;
    if ( 1 == ppib->Level() && !( plog->FLogDisabled() || plog->FRecovering() ) && !( ppib->fReadOnly ) )
    {
        plog->GetLgposOfPbEntryWithLock( &ppib->lgposStart );
    }
#endif
}

VOID    VERCommitTransaction    ( PIB * const ppib );
ERR     ErrVERRollback          ( PIB *ppib );
ERR     ErrVERRollback          ( PIB *ppib, UPDATEID updateid );   //  for rolling back the operations from one update

//  Getting information from the version store
BOOL    FVERActive          ( const IFMP ifmp, const PGNO pgnoFDP, const BOOKMARK& bm, const TRX trxSession );

BOOL    FVERActive          ( const FUCB * pfucb, const BOOKMARK& bm, const TRX trxSession );
INLINE BOOL FVERActive( const FUCB * pfucb, const BOOKMARK& bm )
{
    return FVERActive( pfucb, bm, TrxOldest( PinstFromPfucb( pfucb ) ) );
}
ERR     ErrVERAccessNode    ( FUCB * pfucb, const BOOKMARK& bookmark, NS * pns );
BOOL    FVERDeltaActiveNotByMe( const FUCB * pfucb, const BOOKMARK& bookmark, INT cbOffset );
BOOL    FVERWriteConflict   ( FUCB * pfucb, const BOOKMARK& bookmark, const OPER oper );

template< typename TDelta>
TDelta DeltaVERGetDelta ( const FUCB * pfucb, const BOOKMARK& bookmark, INT cbOffset );

// Explicitly instantiatiate the only allowed legal instances of this template
template LONG DeltaVERGetDelta<LONG>( const FUCB * pfucb, const BOOKMARK& bookmark, INT cbOffset );
template LONGLONG DeltaVERGetDelta<LONGLONG>( const FUCB * pfucb, const BOOKMARK& bookmark, INT cbOffset );

BOOL    FVERCheckUncommittedFreedSpace(
    const FUCB      * pfucb,
    CSR             * const pcsr,
    const INT       cbReq,
    const BOOL      fPermitUpdateUncFree );
INT     CbVERGetNodeMax     ( const FUCB * pfucb, const BOOKMARK& bookmark );
BOOL FVERGetReplaceImage(
    const PIB       *ppib,
    const IFMP      ifmp,
    const PGNO      pgnoLVFDP,
    const BOOKMARK& bookmark,
    const RCEID     rceidFirst,
    const RCEID     rceidLast,
    const TRX       trxBegin0,
    const TRX       trxCommitted,
    const BOOL      fAfterImage,
    const BYTE      **ppb,
    ULONG           * const pcbActual
    );
INT     CbVERGetNodeReserve(
    const PIB * ppib,
    const FUCB * pfucb,
    const BOOKMARK& bookmark,
    INT cbCurrentData
    );
enum UPDATEPAGE { fDoUpdatePage, fDoNotUpdatePage };
VOID    VERSetCbAdjust(
    CSR         *pcsr,
    const RCE   *prce,
    INT         cbDataNew,
    INT         cbData,
    UPDATEPAGE  updatepage
    );

//  Destroying RCEs
VOID VERNullifyFailedDMLRCE( RCE * prce );
VOID VERNullifyInactiveVersionsOnBM( const FUCB * pfucb, const BOOKMARK& bm );
VOID VERNullifyAllVersionsOnFCB( FCB * const pfcb );

VOID VERInsertRCEIntoLists( FUCB *pfucbNode, CSR *pcsr, RCE *prce, const VERPROXY *pverproxy = NULL );

//  ================================================================
INLINE TRX TrxVERISession ( const FUCB * const pfucb )
//  ================================================================
//
//  get trx for session.
//  UNDONE: CIM support for updates is currently broken -- set trxSession
//  to trxMax if session has committed or dirty cursor isolation model.
//
//-
{
    const TRX trxSession = pfucb->ppib->trxBegin0;
    return trxSession;
}



//  ================================================================
INLINE BOOL FVERPotThere( const VS vs, const BOOL fDelete )
//  ================================================================
{
    return ( !fDelete || vsUncommittedByOther == vs );
}

#ifdef DEBUGGER_EXTENSION

UINT UiVERHash( IFMP ifmp, PGNO pgnoFDP, const BOOKMARK& bookmark, const UINT crcehead );

#endif  //  DEBUGGER_EXTENSION


struct BUCKET;
BUCKET * const pbucketNil = 0;

class VER
    :   public CZeroInit
{
#ifdef DEBUGGER_EXTENSION
    friend VOID EDBGVerHashSum( INST * pinstDebuggee, BOOL fVerbose );
#endif

private:
    struct RCEHEAD
    {
        RCEHEAD() :
        rwl( CLockBasicInfo( CSyncBasicInfo( szRCEChain ), rankRCEChain, 0 ) ),
        prceChain( prceNil )
        {}

        CReaderWriterLock   rwl;            //  reader writer lock protecting this hash chain
        RCE*                prceChain;      //  head of RCE hash chain
    };

    struct RCEHEADLEGACY  //  used for size computations only
    {
        RCEHEADLEGACY() :
            crit( CLockBasicInfo( CSyncBasicInfo( szRCEChain ), rankRCEChain, 0 ) ),
            prceChain( prceNil )
        {
        }

        CCriticalSection    crit;           //  critical section protecting this hash chain
        RCE*                prceChain;      //  head of RCE hash chain
    };

public:
    VER( INST *pinst );
    ~VER();

private:
#pragma push_macro( "new" )
#undef new
    void* operator new[]( size_t );         //  not supported
    void operator delete[]( void* );        //  not supported
#pragma pop_macro( "new" )
public:

    enum { dtickRCECleanPeriod = 1000000 };     //  a little over 15 minutes

    INST                *m_pinst;

    //  RCE Clean up control
    volatile LONG       m_fVERCleanUpWait;
    TICK                m_tickLastRCEClean;
    CManualResetSignal  m_msigRCECleanPerformedRecently;
    CAutoResetSignal    m_asigRCECleanDone;
    CNestableCriticalSection    m_critRCEClean;

    //  global bucket chain
    CCriticalSection    m_critBucketGlobal;
    BUCKET              *m_pbucketGlobalHead;
    BUCKET              *m_pbucketGlobalTail;

    CResource           m_cresBucket;
    DWORD_PTR           m_cbBucket;

    //  session opened by VERClean
    PIB                 *m_ppibRCEClean;
    PIB                 *m_ppibRCECleanCallback;

    //  unique id for each RCE
    RCEID               m_rceidLast;

    //  for debugging JET_errVersionStoreOutOfMemory
    PIB *               m_ppibTrxOldestLastLongRunningTransaction;
    DWORD_PTR           m_dwTrxContextLastLongRunningTransaction;
    TRX                 m_trxBegin0LastLongRunningTransaction;

    BOOL                m_fSyncronousTasks;
    RECTASKBATCHER      m_rectaskbatcher;

    BOOL                m_fAboveMaxTransactionSize;
public:

#ifdef VERPERF
    QWORD               qwStartTicks;

    INT                 m_cbucketCleaned;
    INT                 m_cbucketSeen;
    INT                 m_crceSeen;
    INT                 m_crceCleaned;
    INT                 m_crceFlagDelete;
    INT                 m_crceDeleteLV;

    CCriticalSection    m_critVERPerf;
#endif  //  VERPERF

public:
    size_t              m_crceheadHashTable;
    enum { cbrcehead = sizeof( VER::RCEHEAD ) };
    enum { cbrceheadLegacy = sizeof( VER::RCEHEADLEGACY ) };

private:
    BOOL                m_frceHashTableInited;
    // this should be the last member in the structure
    RCEHEAD             m_rgrceheadHashTable[ 0 ];  //  ( size of allocation - sizeof( VER ) ) / sizeof( VER::RCEHEAD )

private:

    VOID VERIReportDiscardedDeletes( const RCE * const prce );
    VOID VERIReportVersionStoreOOM( PIB * ppib, BOOL fMaxTrxSize, const BOOL fCleanupBlocked );

    //  BUCKET LAYER


    INLINE size_t CbBUFree( const BUCKET * pbucket );
    INLINE BOOL FVERICleanWithoutIO();
    INLINE BOOL FVERICleanDiscardDeletes();
    INLINE ERR ErrVERIBUAllocBucket( const INT cbRCE, const UINT uiHash );
    INLINE BUCKET *PbucketVERIGetOldest( );
    BUCKET *PbucketVERIFreeAndGetNextOldestBucket( BUCKET * pbucket );

    ERR ErrVERIAllocateRCE( INT cbRCE, RCE ** pprce, const UINT uiHash );
    ERR ErrVERIMoveRCE( RCE * prce );
    ERR ErrVERICreateRCE(
            INT         cbNewRCE,
            FCB         *pfcb,
            FUCB        *pfucb,
            UPDATEID    updateid,
            const TRX   trxBegin0,
            TRX *       ptrxCommit0,
            const LEVEL level,
            INT         cbBookmarkKey,
            INT         cbBookmarkData,
            OPER        oper,
            UINT        uiHash,
            RCE         **pprce,
            const BOOL  fProxy = fFalse,
            RCEID       rceid = rceidNull
            );
    ERR ErrVERICreateDMLRCE(
            FUCB            * pfucb,
            UPDATEID        updateid,
            const BOOKMARK& bookmark,
            const UINT      uiHash,
            const OPER      oper,
            const LEVEL     level,
            const BOOL      fProxy,
            RCE             **pprce,
            RCEID           rceid
            );

    template< typename TDelta >
    ERR ErrVERICleanDeltaRCE( const RCE * const prce );

    ERR ErrVERIPossiblyDeleteLV( const RCE * const prce, ENTERCRITICALSECTION *pcritRCEChain );
    ERR ErrVERIPossiblyFinalize( const RCE * const prce, ENTERCRITICALSECTION *pcritRCEChain );

    static DWORD VERIRCECleanProc( VOID *pvThis );

public:
    static VER * VERAlloc( INST* pinst );
    static VOID VERFree( VER * pVer );
    ERR ErrVERInit( INST* pinst );
    VOID VERTerm( BOOL fNormal );
    INLINE ERR ErrVERModifyCommitted(
            FCB             *pfcb,
            const BOOKMARK& bookmark,
            const OPER      oper,
            const TRX       trxBegin0,
            const TRX       trxCommitted,
            RCE             **pprce
            );

    ERR ErrVERCheckTransactionSize( PIB * const ppib );
    ERR ErrVERModify(
            FUCB            * pfucb,
            const BOOKMARK& bookmark,
            const OPER      oper,
            RCE             **pprce,
            const VERPROXY  * const pverproxy
            );
    ERR ErrVERFlag( FUCB * pfucb, OPER oper, const VOID * pv, INT cb );
    ERR ErrVERStatus( );
    ERR ErrVERICleanOneRCE( RCE * const prce );
    ERR ErrVERRCEClean( const IFMP ifmp = g_ifmpMax );
    ERR ErrVERIRCEClean( const IFMP ifmp = g_ifmpMax );
    ERR ErrVERIDelete( PIB * ppib, const RCE * const prce );
    VOID VERSignalCleanup();

    RCEID RceidLast();
    RCEID RceidLastIncrement();

    VOID IncrementCAsyncCleanupDispatched();
    VOID IncrementCSyncCleanupDispatched();
    VOID IncrementCCleanupFailed();
    VOID IncrementCCleanupDiscarded( const RCE * const prce );

#ifdef DEBUGGER_EXTENSION
    VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif  //  DEBUGGER_EXTENSION

    // RCEHEAD functions
    //
    INLINE CReaderWriterLock& RwlRCEChain( UINT ui );
    INLINE RCE *GetChain( UINT ui ) const;
    INLINE RCE **PGetChain( UINT ui );
    INLINE VOID SetChain( UINT ui, RCE * );

#ifdef RTM
#else
public:
    ERR ErrInternalCheck();

protected:
    ERR ErrCheckRCEHashList( const RCE * const prce, const UINT uiHash ) const;
    ERR ErrCheckRCEChain( const RCE * const prce, const UINT uiHash ) const;
#endif  //  !RTM
};

INLINE VER *PverFromIfmp( const IFMP ifmp ) { return g_rgfmp[ ifmp ].Pinst()->m_pver; }
INLINE VER *PverFromPpib( const PIB * const ppib ) { return ppib->m_pinst->m_pver; }
INLINE VOID VERSignalCleanup( const PIB * const ppib ) { PverFromPpib( ppib )->VERSignalCleanup(); }


//  ================================================================
INLINE RCEID VER::RceidLast()
//  ================================================================
{
    // No need to obtain critical section.  If we're a little bit
    // off, it doesn't matter - we just need a general starting
    // point from which to start scanning for before-images.
    const RCEID rceid = m_rceidLast;
    Assert( rceid != rceidNull );
    return rceid;
}

//  increments rceidLast and returns new value
//
//
//  ================================================================
INLINE RCEID VER::RceidLastIncrement( )
    //  ================================================================
{
    const RCEID rceid = AtomicExchangeAdd( (LONG *)&m_rceidLast, 2 ) + 2;
    Assert(rceidNull != rceid);
    return rceid;
}


//  ****************************************************************
//  BUCKETS
//  ****************************************************************


//  ================================================================
struct BUCKETHDR
//  ================================================================
{
    BUCKETHDR( VER* const pverIn, BYTE* const rgb )
        :   pver( pverIn ),
            pbucketPrev( NULL ),
            pbucketNext( NULL ),
            prceNextNew( (RCE *)rgb ),
            prceOldest( (RCE*)rgb ),
            pbLastDelete( rgb )
    {
    }

    VER         *pver;                          // parent version store
    BUCKET      *pbucketPrev;                   // prev bucket
    BUCKET      *pbucketNext;                   // next bucket
    RCE         *prceNextNew;                   // pointer to beginning of free space in bucket
    RCE         *prceOldest;                    // pointer to beginning of first non-NULL RCE in bucket
    BYTE        *pbLastDelete;                  // pointer to end of last moved flagDelete RCE in bucket
};


//  ================================================================
struct BUCKET
//  ================================================================
{
    BUCKET( VER* const pver )
        :   hdr( pver, rgb )
    {
    }

#pragma push_macro( "new" )
#undef new
    private:
        void* operator new( size_t );           //  meaningless without VER*
        void* operator new[]( size_t );         //  meaningless without VER*
        void operator delete[]( void* );        //  not supported
    public:
        void* operator new( size_t cbAlloc, VER* pver )
        {
            return pver->m_cresBucket.PvRESAlloc_( SzNewFile(), UlNewLine() );
        }
        void operator delete( void* pv )
        {
            if ( pv )
            {
                BUCKET* pbucket = reinterpret_cast< BUCKET* >( pv );
                pbucket->hdr.pver->m_cresBucket.Free( pv );
            }
        }
#pragma pop_macro( "new" )

    BUCKETHDR       hdr;
    BYTE            rgb[ 0 ];   // space for storing RCEs (will be VER::m_cbBucket - sizeof( BUCKETHDR )
};

