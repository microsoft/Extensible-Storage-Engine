// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef _SCB_H
#error SCB.HXX already included
#endif
#define _SCB_H


//  tune these constants for optimal performance

//  maximum amount of fast memory (cache) to use for sorting
const LONG cbSortMemFast = ( 16 * ( 4088 + 1 ) );

//  maximum amount of normal memory to use for sorting
const LONG cbSortMemNorm = ( 1024 * 1024 );

//  maximum size for memory resident Temp Table
const LONG cbResidentTTMax = ( 64 * 1024 );

//  minimum count of sort pairs effectively sorted by Quicksort
//  NOTE:  must be greater than 2!
const LONG cspairQSortMin = 32;

//  maximum partition stack depth for Quicksort
const LONG cpartQSortMax = 16;

//  maximum count of runs to merge at once (fan-in)
const LONG crunFanInMax = 16;

//  I/O cluster size (in pages)
const LONG cpgClusterSize = 16;

//  Sort Record in normal sort memory
//
//
#include <pshpack1.h>

PERSISTED
struct SREC
{
    UnalignedLittleEndian< USHORT >     cbRec;          //  record size
    UnalignedLittleEndian< USHORT >     cbKey;          //  key size
    BYTE                                rgbKey[];       //  key
//  BYTE                                rgbData[];      //  data (just for illustration)
};

const INT cbSrecHeader = sizeof( USHORT ) + sizeof( USHORT );

#include <poppack.h>

//  Sort Page structure
//
//  This is a custom page layout for use in the temporary database by sorting
//  only.  Sufficient structure still remains so that other page reading code
//  can recognize that they do not know this format and can continue on their
//  merry way.

#include <pshpack1.h>

PERSISTED
struct SPAGE_FIX
{
    LittleEndian< ULONG >       ulChecksum;             //  page checksum ( UlUtilChecksum( prawpage, g_cbPage ) )
    LittleEndian< PGNO >        pgnoThisPage;           //  PGNO of this page (pgnoNull indicates an uninit page)
};

//  free data space per SPAGE
#define cbFreeSPAGE ( max( g_cbPage, g_cbPageMin ) - sizeof( SPAGE_FIX ) )

#include <poppack.h>

//  returns start of free data area in a sort page
INLINE BYTE *PbDataStartPspage( SPAGE_FIX *pspage )
{
    return (BYTE *)( pspage ) + sizeof( SPAGE_FIX );
}

//  returns end of free data area in a sort page + 1
INLINE BYTE *PbDataEndPspage( SPAGE_FIX *pspage )
{
    return (BYTE *)( pspage ) + g_cbPage;
}

//  maximum count of SPAGEs' data that can be stored in normal sort memory
#define cspageSortMax (cbSortMemNorm / cbFreeSPAGE)

//  amount of normal memory actually used for sorting
//  (designed to make original runs fill pages exactly)
#define cbSortMemNormUsed ( cspageSortMax * cbFreeSPAGE )


//  Sort Pair in fast sort memory
//
//  (key prefix, index) pairs are sorted so that most of the data that needs
//  to be examined during a sort will be loaded into cache memory, allowing
//  the sort to run very fast.  If two key prefixes are equal, we must go out
//  to slower memory to compare the remainder of the keys (if any) to determine
//  the proper sort order.  This makes it important for the prefixes to be as
//  discriminatory as possible for each record.
//
//  CONSIDER:  adding a flag to indicate that the entire key is present
//
//  Indexes are compressed pointers that describe the record's position in
//  the slow memory sort buffer.  Each record's position can only be known to
//  a granularity designated by the size of the normal sort memory.  For
//  example, if you specify 128KB of normal memory, the granularity is 2
//  because the index can only take on 65536 values:
//      ceil( ( 128 * 1024 ) / 65536 ) = 2.

//  size of key prefix (in bytes)
const INT cbKeyPrefix = 14;

#include <pshpack1.h>

//  NOTE:  sizeof(SPAIR) must be a power of 2 >= 8
struct SPAIR
{
    USHORT      irec;                   //  record index
    BYTE        rgbKey[cbKeyPrefix];    //  key prefix
};

#include <poppack.h>

//  addressing granularity of record indexes (fit indexes into USHORT)
//      (run disk usage is optimal for cbIndexGran == 1)
#define cbIndexGran (( cbSortMemNormUsed + 0xFFFFL ) / 0x10000L )

//  maximum index of records that can be stored in normal memory
#define irecSortMax ( cbSortMemNormUsed / cbIndexGran )

//  maximum count of SPAIRs' data that can be stored in fast sort memory
//  NOTE:  we are reserving one for temporary sort key storage (at cspairSortMax)
const INT cspairSortMax = cbSortMemFast / sizeof( SPAIR ) - 1;

//  amount of fast memory actually used for sorting (counting reserve SPAIR)
const INT cbSortMemFastUsed = ( cspairSortMax + 1 ) * sizeof( SPAIR );

//  count of "Sort Record indexes" required to store count bytes of data
//      (This is fast if numbers are chosen to make cbIndexGran a power of 2
//      (especially 1) due to compiler optimizations)
INLINE LONG CirecToStoreCb( LONG cb )
{
    return ( cb + cbIndexGran - 1 ) / cbIndexGran;
}


//  Unique run identifier (first page of run = run id)

typedef PGNO        RUN;

const RUN runNull = (RUN) pgnoNull;
const RUN crunAll = 0x7FFFFFFFL;


//  Run Information structure

struct RUNINFO
{
    RUN     run;            //  this run
    CPG     cpg;            //  count of pages in run
    QWORD   cbRun;          //  count of bytes of data in run
    ULONG   crecRun;        //  count of records in each run (NOTE: only used for stats/debugging, so no harm if it overflows)
    CPG     cpgUsed;        //  count of pages actually used
};


//  Run Link structure (used in RUNLIST)

struct RUNLINK
{
    RUNLINK             *prunlinkNext;  //  next run
    RUNINFO             runinfo;        //  runinfo for this run
};

RUNLINK * const prunlinkNil = 0;


//  RUNLINK allocation operators

INLINE RUNLINK * PrunlinkRUNLINKAlloc()  { return new RUNLINK; }

INLINE VOID RUNLINKReleasePrunlink( RUNLINK *& prunlink )
{
    delete prunlink;
#ifdef DEBUG
    prunlink = 0;
#endif
}

//  Run List structure

struct RUNLIST
{
    RUNLINK         *prunlinkHead;      //  head of runlist
    LONG            crun;               //  count of runs in list
};


//  Merge Tree Node
//
//  These nodes are used in the replacement-selection sort tree that merges
//  the incoming runs into one large run.  Due to the way the tree is set up,
//  each node acts as both an internal (loser) node and as an external (input)
//  node, with the exception of node 0, which keeps the last winner instead
//  of a loser.

struct MTNODE
{
    //  external node
    struct RCB      *prcb;      //  input run
    MTNODE  *pmtnodeExtUp;      //  pointer to father node

    //  internal node
    SREC            *psrec;             //  current record
    MTNODE          *pmtnodeSrc;        //  record's source node
    MTNODE          *pmtnodeIntUp;      //  pointer to father node
};

//  Special values for psrec for replacement-selection sort.  psrecNegInf is a
//  sentinel value less than any possible key and is used for merge tree
//  initialization.  psrecInf is a sentinel value greater than any possible key
//  and is used to indicate the end of the input stream.
SREC * const psrecNegInf    = ( SREC * ) -1;
SREC * const psrecInf       = ( SREC * ) 0;


//  Optimized Tree Merge Node
//
//  These nodes are used to build the merge plan for the depth first merge of
//  an optimized tree merge.  This tree is built so that we perform the merges
//  from the smaller side of the tree to the larger side of the tree, all in
//  the interest of increasing our cache locality during the merge process.

struct OTNODE
{
    RUNLIST     runlist;                    //  list of runs for this node
    OTNODE      *rgpotnode[crunFanInMax];   //  subtrees for this node
    OTNODE      *potnodeAllocNext;          //  next node (allocation)
    OTNODE      *potnodeLevelNext;          //  next node (level)
};

OTNODE * const potnodeNil = (OTNODE *) 0;

//  Special value for potnode for the optimized tree merge tree build routine.
//  potnodeLevel0 means that the current level is comprised of original runs,
//  not of other merge nodes.
OTNODE * const potnodeLevel0 = (OTNODE *) -1;


//  OTNODE allocation operators

INLINE OTNODE * PotnodeOTNODEAlloc() { return new OTNODE; }

INLINE VOID OTNODEReleasePotnode( OTNODE *& potnode )
{
    delete potnode;
#ifdef DEBUG                    /*  Debug check for illegal use of freed otnode  */
    potnode = 0;
#endif
}


//  Sort Control Block (SCB)

struct SCB
{
    SCB( const IFMP ifmp, const PGNO pgnoFDP )
        :   fcb( ifmp, pgnoFDP ),
            rgbRec( NULL ),
            rgspair( NULL )
    {
    }
    ~SCB()
    {
        OSMemoryPageFree( rgbRec );
        OSMemoryPageFree( rgspair );
    }

#pragma push_macro( "new" )
#undef new
private:
    void* operator new( size_t );           //  meaningless without INST*
    void* operator new[]( size_t );         //  meaningless without INST*
    void operator delete[]( void* );        //  not supported
public:
    void* operator new( size_t cbAlloc, INST* pinst )
    {
        return pinst->m_cresSCB.PvRESAlloc_( SzNewFile(), UlNewLine() );
    }
    void operator delete( void* pv )
    {
        if ( pv )
        {
            SCB* pscb = reinterpret_cast< SCB* >( pv );
            PinstFromIfmp( pscb->fcb.Ifmp() )->m_cresSCB.Free( pv );
        }
    }
#pragma pop_macro( "new" )


    FCB         fcb;                        //  FCB MUST BE FIRST FIELD IN STRUCTURE

    JET_GRBIT   grbit;                      //  sort grbit
    INT         fFlags;                     //  sort flags

    QWORD       cRecords;                   //  count of records in sort
//  16 bytes

    //  memory-resident sorting
    SPAIR       *rgspair;                   //  sort pair buffer
    LONG        ispairMac;                  //  next available sort pair

    BYTE        *rgbRec;                    //  record buffer
    LONG        irecMac;                    //  next available record index
//  32 bytes
    LONG        crecBuf;                    //  count of records in buffer
    LONG        cbData;                     //  total record data size (actual)

    //  disk-resident sorting
    LONG        crun;                       //  count of original runs generated
    RUNLIST     runlist;                    //  list of runs to be merged
//  52 bytes

    //  sort/merge run output
    PGNO        pgnoNext;                   //  next page in output run
    BFLatch     bflOut;                     //  current output buffer
    //  64 bytes
    BYTE        *pbOutMac;                  //  next available byte in page
    BYTE        *pbOutMax;                  //  end of available page

    //  merge (replacement-selection sort)
    LONG        crunMerge;                  //  count of runs being read/merged
    BYTE        rgbReserved1[4];            //  FREE SPACE
//  80 bytes
    MTNODE      rgmtnode[crunFanInMax];     //  merge tree
//  400 bytes

    //  merge duplicate removal
    BFLatch     bflLast;                    //  last used read ahead buffer
//  408 bytes
    VOID        *pvAssyLast;                //  last used assembly buffer
//  412 bytes

};

SCB * const pscbNil = 0;

//
//  Resource management
//

INLINE VOID SCBTerm( INST *pinst )
{
    //  term the SCB resource manager
    //
    pinst->m_cresSCB.Term();
    if ( pinst->FRecovering() )
    {
        CallS( ErrRESSetResourceParam( pinst, JET_residSCB, JET_resoperEnableMaxUse, fTrue ) );
    }
}

INLINE ERR ErrSCBInit( INST *pinst )
{
    ERR err = JET_errSuccess;

    if ( UlParam( pinst, JET_paramMaxTemporaryTables ) == 0 )
    {
        return JET_errSuccess;
    }

    //  init the SCB resource manager
    //
    //  NOTE:  if we are recovering, then disable quotas
    //
    if ( pinst->FRecovering() )
    {
        CallS( ErrRESSetResourceParam( pinst, JET_residSCB, JET_resoperEnableMaxUse, fFalse ) );
    }
    Call( pinst->m_cresSCB.ErrInit( JET_residSCB ) );

HandleError:
    if ( err < JET_errSuccess )
    {
        SCBTerm( pinst );
    }
    return err;
}

//  SCB fFlags

const INT fSCBInsert                    = 0x0001;
const INT fSCBRemoveDuplicateKey        = 0x0002;
const INT fSCBRemoveDuplicateKeyData    = 0x0004;

//  SCB fFlags operators

INLINE BOOL FSCBInsert                      ( const SCB * pscb )    { return pscb->fFlags & fSCBInsert; }
INLINE VOID SCBSetInsert                    ( SCB * pscb )          { pscb->fFlags |= fSCBInsert;  Assert( FSCBInsert( pscb ) ); }
INLINE VOID SCBResetInsert                  ( SCB * pscb )          { pscb->fFlags &= ~fSCBInsert; Assert( !FSCBInsert( pscb ) ); }

INLINE BOOL FSCBRemoveDuplicateKey          ( const SCB * pscb )    { return pscb->fFlags & fSCBRemoveDuplicateKey; }
INLINE VOID SCBSetRemoveDuplicateKey        ( SCB * pscb )          { pscb->fFlags |= fSCBRemoveDuplicateKey;  Assert( FSCBRemoveDuplicateKey( pscb ) ); }
INLINE VOID SCBResetRemoveDuplicateKey      ( SCB * pscb )          { pscb->fFlags &= ~fSCBRemoveDuplicateKey; Assert( !FSCBRemoveDuplicateKey( pscb ) ); }

INLINE BOOL FSCBRemoveDuplicateKeyData      ( const SCB * pscb )    { return pscb->fFlags & fSCBRemoveDuplicateKeyData; }
INLINE VOID SCBSetRemoveDuplicateKeyData    ( SCB * pscb )          { pscb->fFlags |= fSCBRemoveDuplicateKeyData;  Assert( FSCBRemoveDuplicateKeyData( pscb ) ); }
INLINE VOID SCBResetRemoveDuplicateKeyData  ( SCB * pscb )          { pscb->fFlags &= ~fSCBRemoveDuplicateKeyData; Assert( !FSCBRemoveDuplicateKeyData( pscb ) ); }

//  minimum amount of record that must be read in order to retrieve its size
const INT cbSRECReadMin = OffsetOf( SREC, cbKey );


INLINE VOID SCBInsertHashTable( SCB *pscb )
{
    pscb->fcb.InsertHashTable();
}
INLINE VOID SCBDeleteHashTable( SCB *pscb )
{
    pscb->fcb.DeleteHashTable();
}


//  the following functions abstract different operations on a sort record pointer
//  to perform the appropriate operations, depending on the flags set in the SCB

//  returns size of an existing sort record
INLINE LONG CbSRECSizePsrec( const SREC * psrec )
{
    return ( (SREC * ) psrec )->cbRec;
}

//  calculates size of a potential sort record
INLINE LONG CbSRECSizeCbCb( LONG cbKey, LONG cbData )
{
    return sizeof( SREC ) + cbKey + cbData;
}

//  sets size of sort record
INLINE VOID SRECSizePsrecCb( SREC * psrec, LONG cb )
{
    ( (SREC * ) psrec )->cbRec = (USHORT) cb;
}

//  returns size of sort record key
INLINE LONG CbSRECKeyPsrec( const SREC * psrec )
{
    return ( (SREC * ) psrec )->cbKey;
}

//  sets size of sort record key
INLINE VOID SRECKeySizePsrecCb( SREC * psrec, LONG cb )
{
    Assert( cb > 0 );
    Assert( cb <= cbKeyMostMost );
    psrec->cbKey = (SHORT)cb;   // Dangerous cast here -- dependent on cbKeyMostMost
}

//  returns sort record key buffer pointer
INLINE BYTE * PbSRECKeyPsrec( const SREC * psrec )
{
    return ( BYTE *) psrec->rgbKey;
}

//  returns sort record key as a Pascal string
INLINE BYTE * StSRECKeyPsrec( const SREC * psrec )
{
    return ( BYTE * ) &psrec->cbKey;
}

//  returns size of sort record data
INLINE LONG CbSRECDataPsrec( const SREC * psrec )
{
    return psrec->cbRec - psrec->cbKey - sizeof( SREC );
}

//  returns sort record data buffer pointer
INLINE BYTE * PbSRECDataPsrec( const SREC * psrec )
{
    return (BYTE *)(psrec->rgbKey + psrec->cbKey);
}

//  returns pointer to a sort record given a base address and a Sort Record Index
INLINE SREC * PsrecFromPbIrec( const BYTE * pb, LONG irec )
{
    return (SREC *) ( pb + irec * cbIndexGran );
}

//  returns the size of the key and the data in a record
INLINE LONG CbSRECKeyDataPsrec( const SREC * psrec )
{
    return psrec->cbRec - sizeof ( SREC );
}

//  Run Control Block
//
//  This control block is used for multiple instance use of the run input
//  functions ErrSORTIRunOpen, ErrSORTIRunNext, and ErrSORTIRunClose.

struct RCB
{
    SCB             *pscb;                  //  associated SCB
    RUNINFO         runinfo;                //  run information
    BFLatch         rgbfl[cpgClusterSize];  //  pinned read ahead buffers
    LONG            ipbf;                   //  current buffer
    BYTE            *pbInMac;               //  next byte in page data
    BYTE            *pbInMax;               //  end of page data
    VOID            *pvAssy;                //  record assembly buffer
    QWORD           cbRemaining;            //  remaining bytes of data in run
};

RCB * const prcbNil = 0;

//  RCB allocation operators

INLINE RCB * PrcbRCBAlloc() { return new RCB; }

INLINE VOID RCBReleasePrcb( RCB *& prcb )
{
    delete prcb;
#ifdef DEBUG
    prcb = 0;
#endif
}

