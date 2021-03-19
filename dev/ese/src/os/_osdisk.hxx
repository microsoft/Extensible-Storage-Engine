// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef __OSDISK_HXX_INCLUDED
#define __OSDISK_HXX_INCLUDED

#include <winioctl.h>


////////////////////////////////////////
//  I/O request

class _OSFILE;
class COSDisk;
typedef _OSFILE* P_OSFILE;

struct IFILEIBOFFSET
{
    QWORD                   iFile;
    QWORD                   ibOffset;


    const static IFILEIBOFFSET   IfileiboffsetNotFound;

    //  Set of operators required by [Invasive]RedBlackTree
    //  
    bool operator==( const IFILEIBOFFSET& rhs ) const
    {
        return ( iFile == rhs.iFile && ibOffset == rhs.ibOffset );
    }

    bool operator<( const IFILEIBOFFSET& rhs ) const
    {
        return ( iFile != rhs.iFile ?
                iFile < rhs.iFile :
                ibOffset < rhs.ibOffset );
    }
    bool operator>( const IFILEIBOFFSET& rhs ) const
    {
        return ( iFile != rhs.iFile ?
                iFile > rhs.iFile :
                ibOffset > rhs.ibOffset );
    }
};


// Just a simple template to help in creating a class that can be memcopied
// Used with IOREQ which is almost memcopyable except specific members
// Usage: Derive from MemCopyable to ensure that memcpy is the first thing that runs on construction
// Then do any custom initialization as needed
// WARNING: Any members with custom constructors will overwrite the memcpy because they will run after the base class constructor
//          Handle them separately.

template< typename T >
class MemCopyable
{
    public:
        MemCopyable() {};
        MemCopyable( const T* pSrc )
        {
            memcpy( this, pSrc, sizeof( T ) );
        }
};

class IOREQ : MemCopyable<IOREQ>
{
    public:
        static SIZE_T OffsetOfAPIC()        { return OffsetOf( IOREQ, m_apic ); }

    private:
        // Since we have a critical section for the object, this operator is now illegal.
        IOREQ& operator=( const IOREQ& ioreq );
        
    public:
    
        IOREQ();
        IOREQ( const IOREQ* pioreqToCopy );

    public:
        enum IOREQTYPE
        {
            ioreqUnknown    = 0,

            ioreqInAvailPool,           //  in Avail pool
            ioreqCachedInTLS,           //  cached in a ptls
            ioreqInReservePool,         //  in the pool of reserved IO

            ioreqAllocFromAvail,        //  allocated from Avail pool (though not yet used)
            ioreqAllocFromEwreqLookaside,// in the lookaside slot for a deferred extending write

            ioreqInIOCombiningList,     //  in I/O combining link list

            ioreqEnqueuedInIoHeap,      //  in I/O heap
            ioreqEnqueuedInVipList,     //  in I/O VIP list
            ioreqEnqueuedInMetedQ,      //  in I/O lower priority / meted queue

            ioreqRemovedFromQueue,      //  removed from queue (heap || VIP) and I/O about to be issued

            ioreqIssuedSyncIO,          //  "sync" I/O issued (or about to be issued) from foreground thread
            ioreqIssuedAsyncIO,         //  async I/O issued (or about to be issued) from IO thread

            ioreqSetSize,               //  servicing a request to set file size
            ioreqExtendingWriteIssued,  //  extending write I/O issued (or about to be issued)

            ioreqCompleted,             //  OS completed the IOREQ, processing completion

            ioreqMax
        };

        enum IOMETHOD // iomethod
        {
            iomethodNone = 0,
            iomethodSync,
            iomethodSemiSync,
            iomethodAsync,
            iomethodScatterGather
        };

    public:

        //
        //  IOREQ Memeber Variables
        //

        //
        //      OS Operational Info
        //

        OVERLAPPED              ovlp;               //  must be first
            //  20 / 32 bytes

        //
        //      IOREQ Tracking Info
        //

        CPool< IOREQ, OffsetOfAPIC >::CInvasiveContext      m_apic;
            //  28 / 48
        IOREQ*                  pioreqIorunNext;            //  same I/O run (while in IO combining list or going throuh I/O path - enqueued, issued, etc)
        LONG                    ipioreqHeap;                //  only used if IOREQ is in I/O heap
            //  36 / 60
            
    private:
        //
        //      Primary IOREQ State Control
        //

        volatile DWORD          m_ciotime;                  //  "io time", tracks worst/best case "debugger-adjusted" IO latencies
            //  40 / 64

        CCriticalSection        m_crit;                     //  critical section ONLY for IO completion and hung IO processing (but NOT ciotime incrementing)
            //  48 / 72

        IOREQTYPE               m_ioreqtype:8;              //  "IO[REQ] state"

        IOMETHOD                m_iomethod:8;               //  type of OS IO method invoked

        DWORD                   m_reserved:16;
            //  52 / 76

    public:

        volatile DWORD          m_grbitHungActionsTaken;    //  hung IO actions taken
            //  56 / 80

    public:
        //  Individual State Accessor Functions, State Modification is below ...

        IOREQTYPE   Ioreqtype() const           { return m_ioreqtype; }
        IOMETHOD    Iomethod() const            { return m_iomethod; }
        DWORD       Ciotime() const             { return m_ciotime; }

#ifdef DEBUG
        BOOL        FNotOwner() const           { return m_crit.FNotOwner(); }
        BOOL        FOwner() const              { return m_crit.FOwner(); }
#endif  //  DEBUG

        //
        //      In Progress IO Info
        //

        HRT                     hrtIOStart;                 //  HRT start time of an IO
            //  64 / 88

        COSDisk *               m_posdCurrentIO;            //  to determine if active IOs are from more than one disk for Hung IO detection
            //  68 / 96

        //
        //      Various - 
        //          Basic IO info, IO coallescing, queue / heap reservation and quota, QOS info, 
        //          completion info, tracking data.
        //

        FLAG32                  fWrite:1;
        INT                     group:2;
        FLAG32                  fCoalesced:1;
        FLAG32                  fFromReservePool:1;         //  from the reserve avail pool
        FLAG32                  m_fFromTLSCache:1;          //  tracking this was gotten from TLS alloc

        FLAG32                  m_fHasHeapReservation:1;                //  can be sorted into the heap
        FLAG32                  m_fHasBackgroundReservation:1;          //  got into the background quota
        FLAG32                  m_fHasUrgentBackgroundReservation:1;    //  got into the urgent background quota

        FLAG32                  m_fCanCombineIO:1;          //  can be combined with TLS IO run / on front end

        static const INT        cRetriesMax = 0x7ffe;       //  leave extra 1 off (instead of 0x7fff), just because
        INT                     m_cRetries:15;              //  number of times OOM or other error has caused a retry (maxes out at 0x3ffe/cRetriesMax)
        INT                     m_reserved1:7;              //  unused

            //  72 / 100

        FullTraceContext        m_tc;
        OSFILEQOS               grbitQOS;

        //  96 / 128

        //
        //      Basic IO Info (offset, size, and actual data)
        //

        QWORD                   ibOffset;
        const BYTE *            pbData;
        DWORD                   cbData;
            //  112 / 148
            //  note: +4 byte void (64-bit)

        //
        //      Completion Info
        //

        P_OSFILE                p_osf;
        DWORD_PTR               dwCompletionKey;
        PFN                     pfnCompletion;
            //  124 / 176

        static SIZE_T OffsetOfHIIC()                        { return OffsetOf( IOREQ, m_hiic ); }
    private:
        CInvasiveList< IOREQ, OffsetOfHIIC >::CElement      m_hiic; // invasive context for hung IO

    public:
            //  136 / 192

        DWORD                   m_tidAlloc;         //  TID of the thread requesting this IO
        TICK                    m_tickAlloc;        //  TICK when the IOREQ was allocated (indicates time in IO queue)

            //  144 / 200

        DWORD_PTR               m_iocontext;        //  IO context for performance tracking via IFilePerfAPI

        //
        //      IO Qs.
        //

    public:
        static SIZE_T OffsetOfMetedQueueIC()        { return OffsetOf( IOREQ, m_irbticMetedQ ); }
    private:
        InvasiveRedBlackTree< IFILEIBOFFSET, IOREQ, OffsetOfMetedQueueIC >::InvasiveContext
                                m_irbticMetedQ;

    public:
        IOREQ*                  pioreqVipList;      //  next IO run in the VIP list

    public:

        //  Should we be filling out to 64 byte cache line size to prevent false sharing?

                //  192 / 264

        //  Debugging / Extra check information

#ifndef RTM
        FLAG32                  m_fDequeueCombineIO:1;      //  can be combined at dequeue / i.e. on back end

        FLAG32                  m_fSplitIO:1;               //  set if the IO was deemed to need to be split
        FLAG32                  m_fReMergeSplitIO:1;        //  set if the IO was split and then later put back together b/c of an IO error
        FLAG32                  m_fOutOfMemory:1;           //  set if the IO got an out of memory on issue

        INT                     m_cTooComplex:3;            //  count of the number of times we got invalid parameter and reduced IO functionality
        FLAG32                  m_reservedNRtm:25;          //  unused
#endif

        // If we ever need to save space, consider these opportunities:
        //
        //  - 2     There are 20 some-odd unused bits in there to boot.
        //  - 8     We could virtualize ibOffset to IbOffset() and then get the value
        //              from the OVERLAPPED structure which already has space for this
        //              info.  Unless it's used for some other purpose?
        //  - 8(64) Wasting a tiny bit on alignment in 64-bit
        //  - 2     OSFILEQOS might be compressible to a SHORT if the bits are reordered.
        //  - 4/8(64) m_hiic only needs a singly linked list, could move to that.
        //  - 4     May be able to combine m_ciotime with m_grbitHungActionsTaken
        //  - ?/?   At least a couple empty DWORDs between variables
        //  - 4/8   May be able to remove m_posdCurrentIO at some point.
        //  - 4     Move m_ioreqtype and m_iomethod back to the main bit-field.
        //
        // NOTE: And that's without going back to the scary unions of like m_apic
        // and ipioreqHeap.

    public:
    
        //
        //  Individual State Modification
        //
    
        VOID ValidateIOREQTypeTransition( const IOREQTYPE ioreqtypeNext );
    
        VOID SetIOREQType( const IOREQTYPE ioreqtypeNext, const IOMETHOD iomethod = iomethodNone );

        VOID BeginIO( const IOMETHOD iomethod, const HRT hrtIOStart, const BOOL fHead );
        typedef enum { CompletedIO /* successful or not */ = 1, ReEnqueueingIO = 2 } COMPLETION_STATUS;
        VOID CompleteIO( const COMPLETION_STATUS eCompletionStatus );

        VOID IncrementIOTime();
        

        //
        //  Accessor Functions
        //

        //  End offset / ib for this specific ioreq (not the whole pioreqIorunNext chain / run)
        //  Note this is 1 beyond the actual last ib in use, it is a "Max" type value.
        QWORD IbBlockMax() const
        {
            Assert( ibOffset < (QWORD)llMax ); // of course, unfortunately 0 is a valid offset, so can't check != 0
            Assert( cbData != 0 );
            return ibOffset + (QWORD)cbData;
        }
        

#ifndef RTM
        void AssertValid( void )  const;
#endif

        //  Debugger support

        friend VOID DumpOneIOREQ( const IOREQ * const pioreqDebuggee, const IOREQ * const pioreq );

        //  The Hung IO Patrol Dog functions must be friends.

        friend ERR ErrIOMgrPatrolDogICheckOutIOREQ( _In_ ULONG ichunk, _In_ ULONG iioreq, _In_ IOREQ * pioreq, void * pctx );
        friend DWORD IOMgrIOPatrolDogThread( DWORD_PTR dwContext );

    public:

        INLINE BOOL FInAvailPool() const                        { return ioreqInAvailPool == m_ioreqtype; }
        INLINE BOOL FCachedInTLS() const                        { return ioreqCachedInTLS == m_ioreqtype; }
        INLINE BOOL FInReservePool() const                      { return ioreqInReservePool == m_ioreqtype; }
        INLINE BOOL FAllocFromAvail() const                     { return ioreqAllocFromAvail == m_ioreqtype; }
        INLINE BOOL FAllocFromEwreqLookaside() const            { return ioreqAllocFromEwreqLookaside == m_ioreqtype; }
        INLINE BOOL FInIOCombiningList() const                  { return ioreqInIOCombiningList == m_ioreqtype; }
        INLINE BOOL FEnqueuedInHeap() const                     { return ioreqEnqueuedInIoHeap == m_ioreqtype; }
        INLINE BOOL FEnqueuedInVIPList() const                  { return ioreqEnqueuedInVipList == m_ioreqtype; }
        INLINE BOOL FEnqueuedInMetedQ() const                   { return ioreqEnqueuedInMetedQ == m_ioreqtype; }
        INLINE BOOL FRemovedFromQueue() const                   { return ioreqRemovedFromQueue == m_ioreqtype; }
        INLINE BOOL FIssuedSyncIO() const                       { return ioreqIssuedSyncIO == m_ioreqtype; }
        INLINE BOOL FIssuedAsyncIO() const                      { return ioreqIssuedAsyncIO == m_ioreqtype; }
        INLINE BOOL FSetSize() const                            { return ioreqSetSize == m_ioreqtype; }
        INLINE BOOL FExtendingWriteIssued() const               { return ioreqExtendingWriteIssued == m_ioreqtype; }
        INLINE BOOL FCompleted() const                          { return ioreqCompleted == m_ioreqtype; }

        //      Categories of State Accessor Functions...
        //

        //  These are useful for declaring generic conditions in debugging / diagnostic code, but 
        //  rather not depend upon these potentially fragile relationships for actual retail code.
#ifndef RTM
        BOOL FInAvailState() const                          { return FInAvailPool() || FCachedInTLS() || FInReservePool(); }
        BOOL FInPendingState() const                        { return FAllocFromAvail() || FAllocFromEwreqLookaside() || FInIOCombiningList(); }
#endif
        BOOL FInEnqueuedState() const                       { return FEnqueuedInHeap() || FEnqueuedInVIPList() || FEnqueuedInMetedQ(); }
        // helpers for FInIssuedState()
        INLINE BOOL FVirtuallyIssuedState() const           { return FRemovedFromQueue() /* <- this one is "sort of issued" / in process of being issued */; }
        INLINE BOOL FOtherIssuedState() const               { return FSetSize() || FExtendingWriteIssued(); }
        INLINE BOOL FOSIssuedState() const                  { return FIssuedSyncIO() || FIssuedAsyncIO(); }

        BOOL FInIssuedState() const                         { return FVirtuallyIssuedState() ||     // <- this one is "sort of issued" / in process of being issued
                                                                                FOSIssuedState() ||
                                                                                FOtherIssuedState(); }
#ifndef RTM
        //  
        INLINE BOOL FEnqueueable() const                    { return m_fHasHeapReservation || grbitQOS & qosIOPoolReserved; }
#endif
        INLINE BOOL FUseMetedQ() const                      { return !!( grbitQOS & qosIODispatchBackground ) || !!( grbitQOS & qosIODispatchWriteMeted ); }

};

inline INT CmpIOREQOffset( const IOREQ * const pioreq1, const IOREQ * const pioreq2 )
{
    if ( pioreq1->ibOffset < pioreq2->ibOffset )
    {
        return -1;
    }
    else if ( pioreq1->ibOffset > pioreq2->ibOffset )
    {
        return 1;
    }
    Assert( pioreq1->ibOffset == pioreq2->ibOffset );
    return 0;
}



////////////////////////////////////////
//  I/O request pool chunk support

typedef struct _IOREQCHUNK
{
    DWORD                   cioreq;
    DWORD                   cioreqMac;
    struct _IOREQCHUNK *    pioreqchunkNext;

    BYTE                    rgbReserved[32-sizeof(DWORD)-sizeof(DWORD)-sizeof(struct _IOREQCHUNK *)];

    IOREQ                   rgioreq[];
} IOREQCHUNK;


////////////////////////////////////////
//  I/O servicer (i.e. a Disk)

class COSDisk : public CZeroInit
{

    //  Glue: Global List of OSDisks
    //
    // NOTE: The m_ile must be first or we don't think our list is empty.
    //  Is global ctor not getting called?
    public:
        static SIZE_T OffsetOfILE() { return OffsetOf( COSDisk, m_ile ); }
    private:
        CInvasiveList< COSDisk, OffsetOfILE >::CElement m_ile;

    //
    //  OSDisk Lifecycle Functionality
    //

    public:
        COSDisk();
        ERR ErrInitDisk( __in_z const WCHAR * const wszDiskPathId, _In_ const DWORD dwDiskNumber );
        ~COSDisk();

        enum OSDiskState {
            eOSDiskInitCtor = 1,
            eOSDiskConnected
    };

        void AddRef();
        static void Release( COSDisk * posd );
        ULONG CRef() const;
        BOOL FIsDisk( __in_z const WCHAR * const wszTargetDisk ) const;

        void TraceStationId( const TraceStationIdentificationReason tsidr );

        void Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;

    private:
        OSDiskState     m_eState;

        //  Number of referrers to this, i.e. files open against the disk and/or IO thread.
        volatile ULONG  m_cref;

        DWORD           m_dwDiskNumber;

        COSEventTraceIdCheck m_traceidcheckDisk;

        //  The disk path ID. See comments in _osfs.hxx for ErrDiskId() on what to expect for this
        //  path.
        WCHAR           m_wszDiskPathId[IFileSystemAPI::cchPathMax];

        //  S.M.A.R.T. and other OS identify information
        //
        #define         szEseNoLoadSmartData        "EseNoLo-S%d-E%d"
        #define         cchMaxEseNoLoadSmartData    ( 10 + 2 /* for 1st %d */ + 12 + 1 + 2 /* safety */ )
        enum { eSmartLoadDiskOpenFailed = 1, eSmartLoadDiskOpenRoFailed = 2, eSmartLoadDevIoCtrlGetVerFailed = 3, eSmartLoadSmartVersionUnexpected = 4,
                eSmartLoadSmartRevisionUnexpected = 5, eSmartLoadSmartCmdCapabilityNotSet = 6, eSmartLoadDevIoCtrlRcvDriveDataFailed = 7 };
        void SetSmartEseNoLoadFailed( _In_ const ULONG iStep, _In_ const DWORD error, __out_bcount_z(cbIdentifier) CHAR * szIdentifier, _In_ const ULONG cbIdentifier );

        typedef struct OSDiskInfo_
        {
            //  S.M.A.R.T. Capable
            BYTE            m_bDiskSmartVersion;
            BYTE            m_bDiskSmartRevision;
            BYTE            m_bDiskSmartIdeDevMap;
            DWORD           m_grbitDiskSmartCapabilities;

            //  Disk Product/Serial Identity Info
            CHAR            m_szDiskModel[ max( 50 + 1, cchMaxEseNoLoadSmartData ) ];        // note: SMART says only needs 39 chars for this field
            CHAR            m_szDiskModelSmart[ max( 39 + 1, cchMaxEseNoLoadSmartData ) ];   // note: the model from S.M.A.R.T. APIs is longer / more qualified.
            CHAR            m_szDiskSerialNumber[ max( 50 + 1, cchMaxEseNoLoadSmartData ) ]; // note: SMART says only needs 20 chars for this field
            CHAR            m_szDiskFirmwareRev[ 20 + 1 ]; // note: SMART says only needs 8 chars for this field

            //  Disk Hardware & Cache Info
            //      (for each structure, only valid if preceeding matching m_errorVar is 0)
            DWORD                           m_errorOsdci;
            _Field_size_( sizeof(DISK_CACHE_INFORMATION) )
            DISK_CACHE_INFORMATION          m_osdci;

            DWORD                           m_errorOsswcp;
            _Field_size_( sizeof(STORAGE_WRITE_CACHE_PROPERTY) )
            STORAGE_WRITE_CACHE_PROPERTY    m_osswcp;

            DWORD                           m_errorOssad;
            _Field_size_( sizeof(STORAGE_ADAPTER_DESCRIPTOR) )
            STORAGE_ADAPTER_DESCRIPTOR      m_ossad;

            DWORD                           m_errorOsdspd;
            _Field_size_( sizeof(DEVICE_SEEK_PENALTY_DESCRIPTOR) )
            DEVICE_SEEK_PENALTY_DESCRIPTOR  m_osdspd;

            DWORD                           m_errorOsdtd;
            _Field_size_( sizeof(DEVICE_TRIM_DESCRIPTOR) )
            DEVICE_TRIM_DESCRIPTOR          m_osdtd;

            DWORD                           m_errorOsdcod;
            _Field_size_( sizeof(DEVICE_COPY_OFFLOAD_DESCRIPTOR) )
            DEVICE_COPY_OFFLOAD_DESCRIPTOR  m_osdcod;
        }
        OSDiskInfo;

        OSDiskInfo      m_osdi;

        void LoadDiskInfo_( __in_z PCWSTR wszDiskPath, _In_ const DWORD dwDiskNumber );
        void LoadCachePerf_( HANDLE hDisk );

    //
    //  Accessors for simple info
    //
    public:

        const WCHAR * WszDiskPathId() const;

        //  get disk ID for this disk
        
        ERR ErrDiskId( ULONG_PTR* const pulDiskId ) const;

        DWORD DwDiskNumber() const { return m_dwDiskNumber; }
        
        BOOL FSeekPenalty() const;

    //
    //  Debug Routines
    //

    public:

#ifdef DEBUG
        void AssertValid() const;
#endif


    //
    //  I/O Run and I/O Run Pool support (helper classes)
    //

    public:

        class IORun
        {
            private:
                CSimpleQueue<IOREQ>     m_storage;
#ifdef DEBUG
                ULONG                   m_cbRun;
#endif

                void InitRun_( IOREQ * pioreqRun );

            public:

#ifdef DEBUG
                void AssertValid( ) const;
#endif

                IORun() : m_storage() { OnDebug( m_cbRun = 0 ); }
                IORun( IOREQ * pioreqHead ) : m_storage()
                {
                    OnDebug( m_cbRun = 0 );
                    FAddToRun( pioreqHead );
                }

                bool FEmpty( ) const                { ASSERT_VALID( this ); return 0 == m_storage.CElements(); }
                ULONG Cioreq( ) const               { ASSERT_VALID( this ); return m_storage.CElements(); }
#ifdef DEBUG
                BOOL FRunMember( const IOREQ * pioreqCheck ) const;
#endif
                ULONG CbRun( ) const;

                //  Note this is 1 beyond the actual last ib in use, it is a "Max" type value.
                QWORD IbRunMax() const              { ASSERT_VALID( this ); Assert( !FEmpty() ); return FEmpty() ? 0 : ( m_storage.Head()->ibOffset + (QWORD)CbRun() ); }

                QWORD IbOffset( ) const             { ASSERT_VALID( this ); Assert( !FEmpty() ); return FEmpty() ? 0 : m_storage.Head()->ibOffset; }
                BOOL FWrite( ) const                { ASSERT_VALID( this ); Assert( !FEmpty() ); return FEmpty() ? 2 : !!m_storage.Head()->fWrite; }
                _OSFILE * P_OSF( ) const            { ASSERT_VALID( this ); Assert( !FEmpty() ); return FEmpty() ? NULL : m_storage.Head()->p_osf; }
                BOOL FHasHeapReservation( ) const   { ASSERT_VALID( this ); Assert( !FEmpty() ); return FEmpty() ? fFalse : m_storage.Head()->m_fHasHeapReservation; }
                BOOL FUseMetedQ( ) const            { ASSERT_VALID( this ); Assert( !FEmpty() ); return FEmpty() ? 0 : m_storage.Head()->FUseMetedQ(); }
                BOOL FIssuedFromQueue( ) const      { ASSERT_VALID( this ); Assert( !FEmpty() ); return FEmpty() ? fFalse : m_storage.Head()->FIssuedAsyncIO(); }
                
                BOOL FMultiBlock( ) const           { ASSERT_VALID( this ); Expected( !FEmpty() ); return FEmpty() ?  fFalse : ( m_storage.Head() != m_storage.Tail() ); }
                IOREQ::IOMETHOD IomethodGet( ) const;

#ifndef RTM
                const IOREQ * PioreqHead( ) const   { ASSERT_VALID( this ); return m_storage.Head(); }
#endif

                bool FAddToRun( IOREQ * pioreq );
                void SetIOREQType( const IOREQ::IOREQTYPE ioreqtypeNext );

                void PrepareForIssue(
                            _In_ const IOREQ::IOMETHOD              iomethod,
                            _Out_ DWORD * const                     pcbRun,
                            _Out_ BOOL * const                      pfIOOSLowPriority,
                            __inout PFILE_SEGMENT_ELEMENT const     rgfse,
                            _In_ const DWORD                        cfse );
                IOREQ * PioreqGetRun();             // destructive version of PioreqHead()
                
        };  // end of IORun

        static DWORD CbIOLength( const IOREQ * const    pioreqHead )
        {
            QWORD           ibOffsetMin = _I64_MAX;
            QWORD           ibOffsetMax = 0;
            DWORD           cbOffsetMaxData = 0;

            //  Due to read gapping we can't just add up the cbData's

            for ( const IOREQ * pioreqT = pioreqHead; pioreqT; pioreqT = pioreqT->pioreqIorunNext )
            {
                QWORD   ibOffset    = pioreqT->ibOffset;
                if ( ibOffsetMin > ibOffset )
                {
                    ibOffsetMin = ibOffset;
                }
                if ( ibOffsetMax <= ibOffset )
                {
                    ibOffsetMax = ibOffset;
                    cbOffsetMaxData = pioreqT->cbData;
                }
            }

            Assert( ibOffsetMin <= ibOffsetMax );
            Assert( cbOffsetMaxData );
            return (DWORD)( ibOffsetMax - ibOffsetMin ) + cbOffsetMaxData;
        }

        //  This class manages a pool of (foreground) collaescing ioruns, maintaining a single 
        //  iorun per file.

        class CIoRunPool    // iorunpool
        {
        private:
            static const INT        s_cConcurrentFileIoRuns = 2;
            static const INT        irunNil = -1;

            HRT                     m_rghrtIoRunStarted[ s_cConcurrentFileIoRuns ];     //  associated file IO runs with the previous handles
            IORun                   m_rgiorunIoRuns[ s_cConcurrentFileIoRuns ];         //  associated file IO runs with the previous handles

            INT IrunIoRunPoolIFindFileRun_( _In_ const _OSFILE * const p_osf ) const;
            IOREQ * PioreqIoRunPoolIRemoveRunSlot_( _In_ const INT irun );
            void IoRunPoolIAddNewRunSlot_( _In_ const INT irun, _Inout_ IOREQ * pioreqToAdd );

        public:

#ifdef ENABLE_JET_UNIT_TEST
            //      Init/Term
            CIoRunPool()
            {
                //  DO NOT ADD CODE HERE (it is only run in embedded unit test).
                //      The only reason this code is here, is the OSTLS gives us a zero init'd
                //      chunk of memory, but the embedded unit test just declares this on the
                //      stack - so we need to zero it out.
                memset( m_rgiorunIoRuns, 0, sizeof(m_rgiorunIoRuns) );
                ASSERT_VALID( &( m_rgiorunIoRuns[0] ) );
                Expected( m_rgiorunIoRuns[0].PioreqHead() == NULL );
                //  DO NOT ADD CODE HERE (it is only run in embedded unit test).
            }

            ~CIoRunPool()
            {
                //  DO NOT ADD CODE HERE (it is only run in embedded unit test).
            }
#endif

            void Dump() const;

            BOOL FCanCombineWithExistingRun(
                _In_ const _OSFILE *            p_osf,
                _In_ const BOOL                 fWrite,
                _In_ const QWORD                ibOffsetCombine,
                _In_ const DWORD                cbDataCombine,
                _In_ const BOOL                 fOverrideIoMax ) const;
#ifdef DEBUG
            //  Note: This function doesn't honor IO maxes for writes so we'll leave it debug only (for Asserts)
            BOOL FCanCombineWithExistingRun( _In_ const IOREQ * const pioreq ) const;
            //  Only needed for testing ...
            INT Cioruns() const;
#endif


            BOOL FContainsFileIoRun( _In_ const _OSFILE * const p_osf ) const;

            //  Tests to see if the iorunpool is either completely empty, or has at least
            //  one empty slot.

            enum IoRunPoolEmptyCheck { fCheckAllEmpty = 1, fCheckOneEmpty = 2 };

            BOOL FEmpty( _In_ const IoRunPoolEmptyCheck fCheck ) const;

            //  The contract of this function is a little weird, but the simplest way to
            //  accomplish a limited size pool ... it has two jobs:
            //   1) It will consume and add the provied pioreqToAdd to the most appropriate
            //      already existing iorun, OR create a new empty iorun.
            //   2) It will return any iorun that was forcibly evicted / removed from the
            //      pool as a side effect of performing job (1).  There are two reasons
            //      we might see such eviction:
            //       A) Is is not matching any building files (for any of the ioruns) and 
            //          there are no empty slots/runs, so much scavenge an existing iorun.
            //       B) There is a matching file but the offsets couldn't be combined (too
            //          far apart most likely).
            
            IOREQ * PioreqSwapAddIoreq( _Inout_ IOREQ * const pioreqToAdd );

            //  This function returns (and removes from the iorunpool) all the ioruns for 
            //  the specified file, or for all ioruns if NULL is passed in.

            IOREQ * PioreqEvictFileRuns( _In_ const _OSFILE * const p_osf );

        };  // end of CIoRunPool
    

    //
    //  Queue Operation
    //

    public:

        class QueueOp
        {
            public:

                enum EQueueOp { eUnknown = 0, eSpecialOperation, eIOOperation };

            private:
                EQueueOp            m_eQueueOp;
                IOREQ *             m_pioreq;
                COSDisk::IORun      m_iorun;

            public:

#ifdef DEBUG
                bool FValid( ) const;
#endif

                QueueOp() : m_eQueueOp( eUnknown ), m_pioreq( NULL ), m_iorun() { }
                QueueOp( IOREQ * pioreqHead ) :
                    m_eQueueOp( eUnknown ), m_pioreq( NULL ), m_iorun()
                {
                    bool fAccepted = FAddToRun( pioreqHead );
                    Assert( fAccepted );
                }

                bool FEmpty() const;
                BOOL FIssuedFromQueue() const;
                BOOL FHasHeapReservation() const;
                BOOL FUseMetedQ() const { return m_pioreq ? m_pioreq->FUseMetedQ() : m_iorun.FUseMetedQ(); }
                BOOL FWrite() const { Assert( FValid() ); Assert( !FEmpty() ); return m_pioreq ? m_pioreq->fWrite : m_iorun.FWrite(); }
                P_OSFILE P_osfPerfctrUpdates() const;
                DWORD CbRun() const;
                ULONG Cioreq() const;
#ifdef DEBUG
                BOOL FRunMember( const IOREQ * pioreqCheck ) const;
#endif

                bool FAddToRun( IOREQ * pioreq );
                void SetIOREQType( const IOREQ::IOREQTYPE ioreqtypeNext );

                IOREQ * PioreqOp() const;
                COSDisk::IORun * PiorunIO();
                IOREQ * PioreqGetRun();

        };


    //
    //  I/O Routines
    //

    public:

        __int64 IpassBeginDispatchPass()        { return m_pIOQueue->IpassBeginDispatchPass(); }
        __int64 IpassContinuingDispatchPass()   { return m_pIOQueue->IpassContinuingDispatchPass(); }

        //  We must have the alloc take the QOS grbit, because this also reserves the right
        //  to enqueue or issue an IO at that QOS ... then EnqueueIORun() is guaranteed to
        //  succeed.
        ERR ErrReserveQueueSpace( _In_ OSFILEQOS grbitQOS, __inout IOREQ * pioreq );
        ERR ErrAllocIOREQ( _In_ OSFILEQOS grbitQOS, _In_ const _OSFILE * p_osf, _In_ const BOOL fWrite, _In_ QWORD ibOffsetCombine, _In_ DWORD cbDataCombine, _Out_ IOREQ ** ppioreq );
        VOID FreeIOREQ( _In_ IOREQ * pioreq );
        void EnqueueIORun( _In_ IOREQ * pioreqHead );
        static void EnqueueDeferredIORun( _In_ const _OSFILE * const p_osf );
        void EnqueueIOREQ( _In_ IOREQ * pioreq );
        ERR ErrDequeueIORun( __inout COSDisk::QueueOp * pqop );
        void IncCioDispatching( void );
        void DecCioDispatching( void );
        void IncCioAsyncDispatching( _In_ const BOOL fWrite );
        void QueueCompleteIORun( _In_ IOREQ * pioreqHead );
        BOOL FQueueCompleteIOREQ( _In_ IOREQ * const pioreq );
        ERR ErrRemoveDeferredIoOp( _In_ TraceContext& tc, _In_ const QWORD ibOffsetMatch, _In_ const DWORD cbDataMatch, _In_ const IOREQ * const pioreqMatch, _Out_ IOREQ ** ppioreqHead );

#ifdef DEBUG
        LONG CioOutstanding() const;
#endif
        LONG CioDispatched() const;
        LONG CioUrgentEnqueued() const;
        LONG CioAllowedMetedOps( _In_ const LONG cioWaitingQ ) const;
        LONG CioReadyMetedEnqueued() const;
        LONG CioReadyWriteEnqueued() const         { return m_pIOQueue->CioWriteQueue(); }
        LONG CioAllEnqueued() const;
        LONG CioAsyncReadDispatching() const       { return m_cioAsyncReadDispatching; }
        LONG CioMetedReadQueue() const             { return m_pIOQueue->CioMetedReadQueue(); }

        void IncCioForeground( void )
        {
            AtomicIncrement( (LONG*)&m_cioForeground );
            Assert( m_cioForeground > 0 );
        }
        void DecCioForeground( void )
        {
            AtomicDecrement( (LONG*)&m_cioForeground );
            Assert( m_cioForeground >= 0 );
        }
        ULONG CioForeground() const
        {
            return m_cioForeground;
        }

        ERR ErrBeginConcurrentIo( const BOOL fForegroundSyncIO );
        void EndConcurrentIo( const BOOL fForegroundSyncIO, const BOOL fWrite );
        void BeginExclusiveIo();
        void EndExclusiveIo();

        void SuspendDiskIo();
        void ResumeDiskIo();

        void TrackOsFfbBegin( const IOFLUSHREASON iofr, const QWORD hFile );
        void TrackOsFfbComplete( const IOFLUSHREASON iofr, const DWORD error, const HRT hrtStart, const QWORD usFfb, const LONG64 cioFlushing, const WCHAR * const wszFileName );

    private:
        //  This is the IO that the IO thread is dispatching or more likely is 
        //  dispatched.  I called it Dispatch_ing_ just to underscore there is
        //  a slim timing window where it might not be out to the OS yet. I call
        //  the accessor CioDispatched() though.
        __int64         m_cioDequeued;
        volatile ULONG  m_cioForeground;
        volatile ULONG  m_cioDispatching;
        volatile LONG   m_cioAsyncReadDispatching;
        volatile LONG   m_cioAsyncWriteDispatching;
        volatile BOOL   m_fExclusiveIo;
        volatile ULONG  m_cFfbOutstanding;
        HRT             m_hrtLastFfb;
        HRT             m_hrtLastMetedDispatch;


    //
    //  IO Queue / Heap
    //

    public:
        class IOQueueToo : CZeroInit
        {
            //  Constants
            //
            public:
                enum QueueInitType
                {
                    qitRead = 0x1,
                    qitWrite = 0x2,
                };

                enum QueueFirstIoFlags // qfif
                {
                    qfifDraining = 0x1,
                    qfifBuilding = 0x2,
                    qfifEither   = ( qfifDraining | qfifBuilding ),
                };

            //  Initialization / dtor
            //
            public:
                IOQueueToo( CCriticalSection * pcritController, const QueueInitType qitInit );
                ~IOQueueToo();

            //  The basic enqueue /  dequeue / and interrogation of queue interface
            //
            public:
                BOOL FInsertIo( _In_ IOREQ * pioreqHead, _In_ const LONG cioreqRun, _Out_ IOREQ ** ppioreqHeadAccepted );
                void ExtractIo( _Inout_ COSDisk::QueueOp * const pqop, _Out_ IOREQ ** ppioreqTraceHead, _Inout_ BOOL * pfCycles );
                void Cycle();

                IFILEIBOFFSET IfileiboffsetFirstIo( _In_ const QueueFirstIoFlags qfif );

                BOOL FStarvedMetedOp() const;

                LONG CioEnqueued() const
                {
                    return m_cioEnqueued;
                }
                
            
            //  Member variables
            //
            private:
                OnDebug( QueueInitType          m_qit );
                OnDebug( CCriticalSection *     m_pcritIoQueue );
                ULONG                           m_cioEnqueued;
                ULONG                           m_cioreqEnqueued;   // not really needed

                HRT                             m_hrtBuildingStart;
                typedef InvasiveRedBlackTree< IFILEIBOFFSET, IOREQ, IOREQ::OffsetOfMetedQueueIC > IRBTQ;
                IRBTQ                           m_irbtBuilding;
                IRBTQ                           m_irbtDraining;

                friend void COSDisk::Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset ) const;
        };

    //  Private implementation of our IO queue.
    private:

        class IOQueue : CZeroInit
        {
            //  Initialization / dtor
            //
            public:
                IOQueue( CCriticalSection * pcritController );
                ERR ErrIOQueueInit( _In_ LONG cIOEnqueuedMax, _In_ LONG cIOBackgroundMax, _In_ LONG cIOUrgentBackgroundMax );
                ~IOQueue();
            private:
                void IOQueueTerm();
                ERR _ErrIOHeapInit( _In_ LONG cIOEnqueuedMax );
                void _IOHeapTerm();

                void TrackIorunEnqueue( _In_ const IOREQ * const pioreqHead, _In_ const DWORD cbRun, _In_ HRT hrtEnqueueBegin, _In_ OSDiskIoQueueManagement dioqm ) const;
                void TrackIorunDequeue( _In_ const IOREQ * const pioreqHead, _In_ const DWORD cbRun, _In_ HRT hrtDequeueBegin, _In_ OSDiskIoQueueManagement dioqm, _In_ const USHORT cIorunCombined, _In_ const IOREQ * const pioreqAdded ) const;

            //
            //  The basic enqueue /  dequeue / and count of queue interface
            //
            public:

                __int64 IpassBeginDispatchPass();
                __int64 IpassContinuingDispatchPass();
                ERR ErrReserveQueueSpace( _In_ OSFILEQOS grbitQOS, __inout IOREQ * pioreq );
                BOOL FReleaseQueueSpace( __inout IOREQ * pioreq );
                void InsertOp( __inout COSDisk::QueueOp * pqop );
                void ExtractOp( _In_ const COSDisk * const posd, _Inout_ COSDisk::QueueOp * pqop );
                ERR ErrRemoveDeferredIoOp( _In_ const IOREQ * const pioreqFind, _Out_ IOREQ ** ppioreqHead );

                INLINE LONG CioreqIOHeap() const;
                INLINE LONG CioVIPList() const;
                INLINE LONG CioMetedReadQueue() const;
                INLINE LONG CioWriteQueue() const          { return m_qWriteIo.CioEnqueued(); }

            private:
                __int64                 m_cDispatchPass;
                __int64                 m_cDispatchContinues;


            //  private implementation of an oscillating A and B heaps
            //
            private:

                ////////////////
                //  I/O Heap A

                class IOHeapA
                {
                    friend ERR COSDisk::IOQueue::_ErrIOHeapInit( _In_ LONG cIOEnqueuedMax );

                    private:
                        IOREQ* volatile *           rgpioreqIOAHeap;
                        LONG                        ipioreqIOAHeapMax;
                        volatile LONG               ipioreqIOAHeapMac;
                        CCriticalSection * const    m_pcrit;            // used only for validation of locking purposes

                    //  I/O Heap A Functions


                    private:
                        void _HeapATerm();
                    public:
                        IOHeapA( CCriticalSection * pcritController ) :
                            m_pcrit( pcritController ),
                            rgpioreqIOAHeap( NULL ),
                            ipioreqIOAHeapMax( 0 ),
                            ipioreqIOAHeapMac( 0 )
                        {
                        }
                        ERR ErrHeapAInit( _In_ LONG cIOEnqueuedMax );
                        ~IOHeapA()              { _HeapATerm(); }

                    public:
                        BOOL FHeapAEmpty() const;
                        LONG CioreqHeapA() const;
                        IOREQ* PioreqHeapATop();
                        void HeapAAdd( IOREQ* pioreq );
                        void HeapARemove( IOREQ* pioreq );
                        BOOL FHeapAFrom( const IOREQ * pioreq ) const;
                    private:
                        BOOL FHeapAISmaller( IOREQ* pioreq1, IOREQ* pioreq2 ) const;
                        LONG IpioreqHeapAIParent( LONG ipioreq ) const;
                        LONG IpioreqHeapAILeftChild( LONG ipioreq ) const;
                        LONG IpioreqHeapAIRightChild( LONG ipioreq ) const;
                    private:
                        void HeapAIUpdate( IOREQ* pioreq );

                };  // end of IOHeapA

                ////////////////
                //  I/O Heap B
                class IOHeapB
                {
                    private:

                        IOREQ* volatile *           rgpioreqIOBHeap;
                        LONG                        ipioreqIOBHeapMax;
                        volatile LONG               ipioreqIOBHeapMic;
                        CCriticalSection * const    m_pcrit;

                        //  I/O Heap B Functions

                    private:
                        void _HeapBTerm();
                    public:
                        IOHeapB( CCriticalSection * pcritController ) :
                            m_pcrit( pcritController ),
                            rgpioreqIOBHeap( NULL ),
                            ipioreqIOBHeapMax( 0 ),
                            ipioreqIOBHeapMic( 0 )
                        {
                        }
                        ERR ErrHeapBInit(
                                IOREQ* volatile *   rgpioreqIOAHeap,
                                LONG                ipioreqIOAHeapMax );
                        ~IOHeapB()          { _HeapBTerm(); }

                    public:
                        BOOL FHeapBEmpty() const;
                        LONG CioreqHeapB() const;
                        IOREQ* PioreqHeapBTop();
                        void HeapBAdd( IOREQ* pioreq );
                        void HeapBRemove( IOREQ* pioreq );
                    private:
                        BOOL FHeapBISmaller( IOREQ* pioreq1, IOREQ* pioreq2 ) const;
                        LONG IpioreqHeapBIParent( LONG ipioreq ) const;
                        LONG IpioreqHeapBILeftChild( LONG ipioreq ) const;
                        LONG IpioreqHeapBIRightChild( LONG ipioreq ) const;
                        void HeapBIUpdate( IOREQ* pioreq );

                };  // end of IOHeapB

            //////////////
            //  I/O Heap Data

            private:

                //  critical section protecting the I/O Heap

                CCriticalSection * m_pcritIoQueue;

                //  I/O heap size and protection

                LONG                m_cioreqMax;
                CSemaphore          m_semIOQueue;

                //  actual I/O heaps

#ifdef DEBUG
                // .dtor is a friend to for debug code to avoid AV in fault injection 17360 scenario 
                friend COSDisk::~COSDisk();
#endif
                IOHeapA * m_pIOHeapA;
                IOHeapB * m_pIOHeapB;

                //  I/O Heap usage / cursors

                BOOL    fUseHeapA;

                QWORD   iFileCurrent;
                QWORD   ibOffsetCurrent;

            //////////////
            //  I/O Heap Functions

            private:

                INLINE void IOHeapAdd( IOREQ* pioreq, _Out_ OSDiskIoQueueManagement * const pdioqmTypeTracking );
                INLINE void IOHeapRemove( IOREQ* pioreq, _Out_ OSDiskIoQueueManagement * const pdioqmTypeTracking );
                INLINE BOOL FIOHeapEmpty();
                INLINE IOREQ* PioreqIOHeapTop();


            //////////////
            //  Overflow / VIP IO operations list

            private:

                LONG                            m_cioVIPList;
                CLocklessLinkedList<IOREQ>      m_VIPListHead;
                

                BOOL    FValidVIPList() const
                {
                    Assert( m_pcritIoQueue->FOwner() ); // only valid if we have IO heap crit

                    Assert( 0 == m_cioVIPList || !m_VIPListHead.FEmpty() );
                    Assert( 0 != m_cioVIPList || m_VIPListHead.FEmpty() );
                    #ifdef DEBUG
                    const IOREQ * pioreqT = m_VIPListHead.Head();
                    LONG cioreqT = 0;
                    while ( pioreqT )
                    {
                        cioreqT++;
                        Assert( pioreqT->FEnqueuedInVIPList() );
                        // walk all entries, make sure matches count ...
                        pioreqT = pioreqT->pioreqVipList;
                    }
                    Assert( cioreqT == m_cioVIPList );
                    //  Lets assert that this slim overflow list does not get used very often.
                    Assert( cioreqT < 100 ||
                            // cough, we're just going to use this as knowing the client is going
                            // to potentially overloaded us ... like SOMEONE's 100 inst, 6 DBs test
                            // to reproduce the exhausted thread pool bug.
                            ( FNegTestSet( fDisableTimeoutDeadlockDetection ) && cioreqT < 800 ) );
                    return fTrue;
                    #endif
                }


            //////////////
            //  Meted Queue (the new one)

            private:
                IOQueueToo              m_qMetedAsyncReadIo;
                IOQueueToo              m_qWriteIo;

            //////////////
            //  QOS Quota data

            private:

                INT                 m_cioQosBackgroundMax;
                INT                 m_cioreqQOSBackgroundLow;
                volatile INT            m_cioQosBackgroundCurrent;

                INT                 m_cioQosUrgentBackgroundMax;
                // Note:            m_cioreqQOSUrgentBackgroundLow is variable, so no variable controls it
                volatile INT            m_cioQosUrgentBackgroundCurrent;

                //////////////
                //  Debug Helper
                
                friend void COSDisk::Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset ) const;

        };  // end of IOQueue

    private:

        //  finally the actual IO queue based on the Heap A/B implementation above.

        IOQueue *           m_pIOQueue;

        //  critical section protecting the I/O Heap

        CCriticalSection    m_critIOQueue;

        //////////////
        //  Performance

    public:

        // Returns the actual physical disk queue depth

        INLINE DWORD CioOsQueueDepth();

    private:

        //  physical disk handle

        HANDLE              m_hDisk;

        //  physical disk performance (updated periodically)

        static const TICK   s_dtickPerformancePeriod        = 100;
        TICK                m_tickPerformanceLastMeasured;
        DWORD               m_cioOsQueueDepth;

        // Refreshes the physical disk performance information if necessary

        INLINE VOID RefreshDiskPerformance();

        // Queries the performance of the physical disk

        VOID QueryDiskPerformance();

};  // end of COSDisk


//      Returns an initialized / ref-counted interface to the OS Disk System

ERR ErrOSDiskConnect( __in_z const WCHAR * const wszDiskPathId, _In_ const DWORD dwDiskNumber, _Out_ IDiskAPI ** ppdiskapi );

//      Disconnects the IO context from the OS Disk System, and deinitializes if necessary.

void OSDiskDisconnect( __inout IDiskAPI * pdiskapi, _In_ const _OSFILE * p_osf );


////////////////////////////////////////
//  I/O "WatchDog" / PatrolDog Functionality

class PatrolDogSynchronizer
{
    private:
        enum PatrolDogState
        {
            pdsInactive,
            pdsActivating,
            pdsActive,
            pdsDeactivating
        };
        
        CAutoResetSignal    m_asigNudgePatrolDog;       //  Signals the PatrolDog thread to wake up
                                                        //  and check for wait and exit conditions.

        BOOL volatile       m_fPutDownPatrolDog;        //  Flag to indicate that the patrol dog thread
                                                        //  must return.

        DWORD volatile      m_cLoiters;                 //  Number of I/Os currently in-flight.

        THREAD              m_threadPatrolDog;          //  Thread handle for the PatrolDog thread.

        LONG volatile       m_patrolDogState;           //  Current state the object is in.

        void*               m_pvContext;                //  User context.

        void InitPatrolDogState_();

        void AssertPatrolDogInitialState_( const PatrolDogState pdsExpected = pdsInactive ) const;

    public:

        //  Ctor.
        
        PatrolDogSynchronizer();

        //  Dtor.

        ~PatrolDogSynchronizer();

        //  Spawns the pfnPatrolDog thread. The contract is that this thread
        //  will exit if conditions such that make FCheckForLoiter() return
        //  fFalse are met. In other words, the thread must bail out once
        //  TermPatrolDog() is called. The thread receives the pointer to
        //  the PatrolDogSynchronizer object as its context and can retrieve
        //  the user context being passed in by calling PvContext().

        ERR ErrInitPatrolDog( const PUTIL_THREAD_PROC pfnPatrolDog,
                                const EThreadPriority priority,
                                void* const pvContext );

        //  Sets the state such that FCheckForLoiter() will return fFalse
        //  if called and waits for the PatrolDog thread to exit. It is valid
        //  to call this function even if the PatrolDog thread is not active.

        void TermPatrolDog();

        //  Signals that a work unit needs tracking by the PatrolDog thread.
        //  This function should only be called between a successful return
        //  from ErrInitPatrolDog() and a call to TermPatrolDog().

        void EnterPerimeter();

        //  Signals that a work unit does not need tracking anymore.
        //  This function should only be called between a successful return
        //  from ErrInitPatrolDog() and a call to TermPatrolDog().

        void LeavePerimeter();

        //  Waits for a work unit that needs tracking. Returns fFalse
        //  if the thread should quit. fTrue means that the timeout has expired
        //  and there are still work units pending.

        BOOL FCheckForLoiter( const TICK dtickTimeout );

        //  Returns the user context passed into ErrInitPatrolDog().

        void* PvContext() const;

        //  Returns the size of this object. Necessary for preventing the workaround
        //  used to run unit tests for this class from breaking.        
        
        SIZE_T CbSizeOf() const;
};

extern PatrolDogSynchronizer g_patrolDogSync;


//      Internal glue
//

QWORD CmsecLatencyOfOSOperation( const IOREQ* const pioreq, const HRT dhrtIOElapsed );
QWORD CmsecLatencyOfOSOperation( const IOREQ* const pioreq );
void OSDiskIOThreadStartIssue( const P_OSFILE p_osf );
void OSDiskIIOThreadCompleteWithErr( DWORD error, DWORD cbTransfer, IOREQ* pioreqHead );

void OSDiskIIOThreadIComplete(  const DWORD     dwError,
                                const DWORD_PTR dwThreadContext,
                                DWORD           cbTransfer,
                                IOREQ           *pioreqHead );
VOID OSDiskIIOThreadIRetryIssue();


//      Miscellaneous
//

BOOL GetOverlappedResult_(  HANDLE          hFile,
                            LPOVERLAPPED    lpOverlapped,
                            LPDWORD         lpNumberOfBytesTransferred,
                            BOOL            bWait );

//  used to avoid signaling the completion port on SemiSync IOs ...
#define hNoCPEvent  ((DWORD_PTR)0x1)

#endif  //  __OSDISK_HXX_INCLUDED

