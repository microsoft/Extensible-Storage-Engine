// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#include <pshpack1.h>

VOID LGReportEvent( DWORD IDEvent, ERR err );

const WORD  diffInsert                  = 0;
const WORD  diffDelete                  = 1;
const WORD  diffReplaceSameLength       = 2;
const WORD  diffReplaceDifferentLength  = 3;

PERSISTED
class DIFFHDR
{
    public:

        enum { ibOffsetMask                 = 0x1FFF };
        enum { ibOffsetShf                  = 0      };
        enum { fReplaceWithSameLength       = 0x2000 };
        enum { fInsert                      = 0x4000 };
        enum { fUseTwoBytesForDataLength    = 0x8000 };

        UnalignedLittleEndian< WORD >   m_bitfield;

        DIFFHDR( void ) : m_bitfield( 0 ) { }

        USHORT Ib( void ) const                             { return USHORT( ( m_bitfield & ibOffsetMask ) >> ibOffsetShf ); }
        BOOL FReplaceWithSameLength( void ) const       { return m_bitfield & fReplaceWithSameLength; }
        BOOL FInsert( void ) const                              { return m_bitfield & fInsert; }
        BOOL FUseTwoBytesForDataLength( void ) const    { return m_bitfield & fUseTwoBytesForDataLength; }
};

PERSISTED
class DIFFHDR2
{
    public:
        enum { fReplaceWithSameLength       = 1 << 5 };
        enum { fInsert                      = 1 << 6 };
        enum { fUseTwoBytesForDataLength    = 1 << 7 };
        enum { shfFlag = 8 };

        UnalignedLittleEndian< WORD > m_ib;
        BYTE m_flags;

        DIFFHDR2( void ): m_ib( 0 ), m_flags( 0 ) { }
        DIFFHDR2( const DIFFHDR DH ) : m_ib( DH.Ib() ), m_flags( BYTE( DH.m_bitfield >> shfFlag ) )
    {
            Assert( DIFFHDR::fReplaceWithSameLength >> shfFlag == fReplaceWithSameLength );
            Assert( DIFFHDR::fInsert >> shfFlag == fInsert );
            Assert( DIFFHDR::fUseTwoBytesForDataLength >> shfFlag == fUseTwoBytesForDataLength );
    }

        USHORT Ib( void ) const                                 { return m_ib; }
        BOOL FReplaceWithSameLength( void ) const       { return m_flags & fReplaceWithSameLength; }
        BOOL FInsert( void ) const                          { return m_flags & fInsert; }
        BOOL FUseTwoBytesForDataLength( void ) const    { return m_flags & fUseTwoBytesForDataLength; }

        VOID SetIb( WORD ib )                           { m_ib = ib; }
        BOOL SetFReplaceWithSameLength( void )      { m_flags |= ( BYTE )fReplaceWithSameLength; }
        BOOL SetFInsert( void )                         { m_flags |= ( BYTE )fInsert; }
        BOOL SetFUseTwoBytesForDataLength( void )   { m_flags |= ( BYTE )fUseTwoBytesForDataLength; }
};

VOID LGDumpDiff( const LOG* const plog, const LR * const plr, CPRINTF * const pcprintf, const ULONG cbIndent );

VOID LGSetColumnDiffs(
    FUCB        *pfucb,
    const DATA& dataNew,
    const DATA& dataOld,
    BYTE        *pbDiff,
    BOOL        *pfOverflow,
    SIZE_T      *pcbDiff );
VOID LGSetLVDiffs(
    FUCB        *pfucb,
    const DATA& dataNew,
    const DATA& dataOld,
    BYTE        *pbDiff,
    BOOL        *pfOverflow,
    SIZE_T      *pcbDiff );
ERR ErrLGGetAfterImage(
    const IFMP   ifmp,
    BYTE        *pbDiff,
    const SIZE_T    cbDiff,
    BYTE        *pbOld,
    const SIZE_T    cbOld,
    const BOOL  fDiff2,
    BYTE        *pbNew,
    SIZE_T      *pcbNew );


typedef BYTE LRTYP;


const LRTYP lrtypNOP                        = 0;
const LRTYP lrtypInit                       = 1;
const LRTYP lrtypTerm                       = 2;
const LRTYP lrtypMS                         = 3;
const LRTYP lrtypEnd                        = 4;


const LRTYP lrtypBegin                      = 5;
const LRTYP lrtypCommit                     = 6;
const LRTYP lrtypRollback                   = 7;
const LRTYP lrtypBegin0                     = 8;
const LRTYP lrtypCommit0                    = 9;
const LRTYP lrtypRefresh                    = 10;
const LRTYP lrtypMacroBegin                 = 11;
const LRTYP lrtypMacroCommit                = 12;
const LRTYP lrtypMacroAbort                 = 13;

const LRTYP lrtypCreateDB                   = 14;
const LRTYP lrtypAttachDB                   = 15;
const LRTYP lrtypDetachDB                   = 16;

const LRTYP lrtypRecoveryUndo               = 17;
const LRTYP lrtypRecoveryQuit               = 18;

const LRTYP lrtypFullBackup                 = 19;
const LRTYP lrtypIncBackup                  = 20;

const LRTYP lrtypJetOp                      = 21;
const LRTYP lrtypTrace                      = 22;

const LRTYP lrtypShutDownMark               = 23;

const LRTYP lrtypCreateMultipleExtentFDP    = 24;
const LRTYP lrtypCreateSingleExtentFDP      = 25;
const LRTYP lrtypConvertFDP                 = 26;

const LRTYP lrtypSplit                      = 27;
const LRTYP lrtypMerge                      = 28;

const LRTYP lrtypInsert                     = 29;
const LRTYP lrtypFlagInsert                 = 30;
const LRTYP lrtypFlagInsertAndReplaceData   = 31;
const LRTYP lrtypFlagDelete                 = 32;
const LRTYP lrtypReplace                    = 33;
const LRTYP lrtypReplaceD                   = 34;
const LRTYP lrtypDelete                     = 35;

const LRTYP lrtypUndoInfo                   = 36;

const LRTYP lrtypDelta                      = 37;

const LRTYP lrtypSetExternalHeader          = 38;
const LRTYP lrtypUndo                       = 39;

const LRTYP lrtypSLVPageAppendOBSOLETE      = 40;
const LRTYP lrtypSLVSpaceOBSOLETE           = 41;

const LRTYP lrtypChecksum                   = 42;

const LRTYP lrtypSLVPageMoveOBSOLETE        = 43;


const LRTYP lrtypForceDetachDBOBSOLETE      = 44;
const LRTYP lrtypExtRestore                 = 45;

const LRTYP lrtypBackup                     = 46;

const LRTYP lrtypEmptyTree                  = 48;

const LRTYP lrtypInit2                      = 49;
const LRTYP lrtypTerm2                      = 50;
const LRTYP lrtypRecoveryUndo2              = 51;
const LRTYP lrtypRecoveryQuit2              = 52;

const LRTYP lrtypBeginDT                    = 53;
const LRTYP lrtypPrepCommit                 = 54;
const LRTYP lrtypPrepRollback               = 55;

const LRTYP lrtypForceWriteLog              = 57;

const LRTYP lrtypConvertFDP2                = 58;

const LRTYP lrtypExtRestore2                = 59;
const LRTYP lrtypBackup2                    = 60;

const LRTYP lrtypForceLogRollover           = 61;

const LRTYP lrtypSplit2                     = 62;
const LRTYP lrtypMerge2                     = 63;

const LRTYP lrtypScrub                      = 64;

const LRTYP lrtypPageMove                   = 65;

const LRTYP lrtypPagePatchRequest           = 66;

const LRTYP lrtypMacroInfo                  = 67;
const LRTYP lrtypExtendDB                   = 68;
const LRTYP lrtypCommitCtx                  = 69;
const LRTYP lrtypScanCheck                  = 70;
const LRTYP lrtypNOP2                       = 71;
const LRTYP lrtypReAttach                   = 72;

const LRTYP lrtypMacroInfo2                 = 73;
const LRTYP lrtypFreeFDP                    = 74;
const LRTYP lrtypIgnored9                   = 75;

const LRTYP lrtypFragmentBegin              = 76;
const LRTYP lrtypFragment                   = 77;

const LRTYP lrtypShrinkDB                   = 78;

const LRTYP lrtypCreateDBFinish             = 79;

const LRTYP lrtypRecoveryQuitKeepAttachments= 80;

const LRTYP lrtypTrimDB                     = 81;

const LRTYP lrtypIgnored10                  = 82;
const LRTYP lrtypIgnored11                  = 83;
const LRTYP lrtypIgnored12                  = 84;
const LRTYP lrtypIgnored13                  = 85;
const LRTYP lrtypIgnored14                  = 86;
const LRTYP lrtypIgnored15                  = 87;
const LRTYP lrtypIgnored16                  = 88;
const LRTYP lrtypIgnored17                  = 89;
const LRTYP lrtypIgnored18                  = 90;
const LRTYP lrtypIgnored19                  = 91;

const LRTYP lrtypSetDbVersion               = 92;
const LRTYP lrtypDelta64                    = 93;
const LRTYP lrtypShrinkDB2                  = 94;
const LRTYP lrtypNewPage                    = 95;
const LRTYP lrtypRootPageMove               = 96;
const LRTYP lrtypSignalAttachDb             = 97;
const LRTYP lrtypScanCheck2                 = 98;
const LRTYP lrtypShrinkDB3                  = 99;
const LRTYP lrtypExtentFreed                = 100;

const LRTYP lrtypMax                        = 101;

const LRTYP lrtypMaxMax                     = 128;
C_ASSERT( lrtypMax < lrtypMaxMax );




PERSISTED
class LR
    :   private CZeroInit
{
    private:
        LR();

    public:
        LR( const size_t cb ) : CZeroInit( cb ) {}
        
    public:
        UnalignedLittleEndian< LRTYP >  lrtyp;
};

PERSISTED
class LRSHUTDOWNMARK
    :   public LR
{
    public:
        LRSHUTDOWNMARK() : LR( sizeof( *this ) ) {}
};

PERSISTED
class LRNOP
    :   public LR
{
    public:
        LRNOP() : LR( sizeof( *this ) ) {}
};

PERSISTED
class LRPAGEPATCHREQUEST
    : public LR
{
public:
    LRPAGEPATCHREQUEST() :
        LR( sizeof( *this ) )
    {
        lrtyp = lrtypPagePatchRequest;
    }

    DBID Dbid() const { return m_dbid; }
    void SetDbid(const DBID dbid) { m_dbid = dbid; }

    PGNO Pgno() const { return m_pgno; }
    void SetPgno(const PGNO pgno) { m_pgno = pgno; }

    DBTIME Dbtime() const { return m_dbtime; }
    void SetDbtime(const DBTIME dbtime) { m_dbtime = dbtime; }

protected:
    UnalignedLittleEndian< DBID >       m_dbid;
    UnalignedLittleEndian< PGNO >       m_pgno;
    UnalignedLittleEndian< DBTIME >     m_dbtime;
    BYTE                                m_rgbReserved[32];
};

PERSISTED
class LRIGNORED
    : public LR
{
public:
    LRIGNORED(const LRTYP lrtyp) :
        LR( sizeof( *this ) )
    {
        this->lrtyp = lrtyp;
    }

    ULONG Cb() const { return m_cb; }
    void SetCb(const ULONG cb) { m_cb = cb; }

protected:
    UnalignedLittleEndian< ULONG >      m_cb;
};

const BYTE  fLRPageVersioned        = 0x01;
const BYTE  fLRPageDeleted          = 0x02;
const BYTE  fLRPageUnique           = 0x04;
const BYTE  fLRPageSpace            = 0x08;
const BYTE  fLRPageConcCI           = 0x10;
const BYTE  fLRPageSystemTask       = 0x20;

PERSISTED
class LRPAGE_
    :   public LR
{
    private:
        LRPAGE_();

    protected:
        LRPAGE_( const size_t cb ) : LR( cb ) { }

    public:
        UnalignedLittleEndian< PROCID >     le_procid;
        UnalignedLittleEndian< DBID >       dbid;
        UnalignedLittleEndian< PGNO >       le_pgno;

        UnalignedLittleEndian< DBTIME >     le_dbtime;
        UnalignedLittleEndian< DBTIME >     le_dbtimeBefore;

        UnalignedLittleEndian< TRX >        le_trxBegin0;
        UnalignedLittleEndian< LEVEL >      level;

    private:
        UnalignedLittleEndian< USHORT >     mle_iline;
        UnalignedLittleEndian< BYTE >       m_fLRPageFlags;

    public:
        INLINE USHORT ILine() const                         { return mle_iline; }
        INLINE VOID SetILine( const USHORT iline )          { mle_iline = iline; }

        INLINE BOOL FVersioned() const                      { return ( m_fLRPageFlags & fLRPageVersioned ); }
        INLINE VOID SetFVersioned()                         { m_fLRPageFlags = BYTE( m_fLRPageFlags | fLRPageVersioned ); }
        INLINE VOID ResetFVersioned()                       { m_fLRPageFlags = BYTE( m_fLRPageFlags & ~fLRPageVersioned ); }

        INLINE BOOL FDeleted() const                        { return ( m_fLRPageFlags & fLRPageDeleted ); }
        INLINE VOID SetFDeleted()                           { m_fLRPageFlags = BYTE( m_fLRPageFlags | fLRPageDeleted ); }

        INLINE BOOL FUnique() const                         { return ( m_fLRPageFlags & fLRPageUnique ); }
        INLINE VOID SetFUnique()                            { m_fLRPageFlags = BYTE( m_fLRPageFlags | fLRPageUnique ); }

        INLINE BOOL FSpace() const                          { return ( m_fLRPageFlags & fLRPageSpace ); }
        INLINE VOID SetFSpace()                             { m_fLRPageFlags = BYTE( m_fLRPageFlags | fLRPageSpace ); }

        INLINE BOOL FConcCI() const                         { return ( m_fLRPageFlags & fLRPageConcCI ); }
        INLINE VOID SetFConcCI()                            { m_fLRPageFlags = BYTE( m_fLRPageFlags | fLRPageConcCI ); }

        INLINE BOOL FSystemTask() const                     { return ( m_fLRPageFlags & fLRPageSystemTask ); }
        INLINE VOID SetFSystemTask()                        { m_fLRPageFlags = BYTE( m_fLRPageFlags | fLRPageSystemTask ); }
};

PERSISTED
class LRNODE_
    :   public LRPAGE_
{
    private:
        LRNODE_();

    protected:
        LRNODE_( const size_t cb ) : LRPAGE_( cb ) { }

    public:
        UnalignedLittleEndian< RCEID >      le_rceid;
        UnalignedLittleEndian< PGNO >       le_pgnoFDP;
        UnalignedLittleEndian< OBJID >      le_objidFDP;
};

PERSISTED
class LRSCRUB
    :   public LRNODE_
{
    public:
        LRSCRUB() : LRNODE_( sizeof( *this ) ) { }
    
    private:
        static const ULONG m_fLRScrubUnusedPage     = 0x1;
        
    public:
        INLINE USHORT CbData() const                        { return mle_cbData; }
        INLINE VOID SetCbData( const USHORT cbData )        { mle_cbData = cbData; }

        INLINE BYTE * PbData()                              { return m_rgbData; }
        INLINE const BYTE * PbData() const                  { return m_rgbData; }

        INLINE BOOL FUnusedPage() const                     { return ( mle_fLRScrubFlags & m_fLRScrubUnusedPage ); }
        INLINE VOID SetFUnusedPage()                        { mle_fLRScrubFlags |= m_fLRScrubUnusedPage; }

        INLINE INT CscrubOper() const                       { return mle_cscrubOper; }
        INLINE VOID SetCscrubOper( const INT cscrubOper )   { mle_cscrubOper = cscrubOper; }
        
        INLINE SCRUBOPER * RgbScrubOper()                   { return (SCRUBOPER *)m_rgbData; }
        INLINE const SCRUBOPER * RgbScrubOper() const       { return (SCRUBOPER *)m_rgbData; }
    
    private:
        UnalignedLittleEndian< ULONG >      mle_fLRScrubFlags;
        UnalignedLittleEndian< ULONG >      mle_cscrubOper;
        UnalignedLittleEndian< USHORT >     mle_cbData;
        BYTE                                m_rgbData[0];
};

PERSISTED
class LRSCANCHECK final
    : protected LRIGNORED
{
    public:
        LRSCANCHECK() : LRIGNORED( lrtypScanCheck )
        {
            
            SetCb( sizeof(LRSCANCHECK) - sizeof(LRIGNORED) );
        }

    private:
        UnalignedLittleEndian< DBID >       dbid;
        UnalignedLittleEndian< PGNO >       le_pgno;
        UnalignedLittleEndian< DBTIME >     le_dbtimePage;
        UnalignedLittleEndian< DBTIME >     le_dbtimeCurrent;
        UnalignedLittleEndian< DBTIME >     le_dbtimeBefore;
        UnalignedLittleEndian< DBTIME >     le_dbtimeAfter;
        UnalignedLittleEndian< USHORT >     le_uschksum;

    public:
        VOID InitLegacyScanCheck(
            const DBID dbidIn,
            const PGNO pgno,
            const DBTIME dbtimePage,
            const DBTIME dbtimeCurrent,
            const USHORT usCompLogicalPageChecksum )
        {
            dbid                = dbidIn;
            le_pgno             = pgno;
            le_dbtimeBefore     = dbtimePage;
            le_dbtimeAfter      = dbtimeCurrent;
            le_uschksum         = usCompLogicalPageChecksum;
        }

        DBID Dbid() const               { return dbid; }
        PGNO Pgno() const               { return le_pgno; }
        DBTIME DbtimeBefore() const     { return le_dbtimeBefore; }
        DBTIME DbtimeAfter() const      { return le_dbtimeAfter; }
        USHORT UsChecksum() const       { return le_uschksum; }
};

PERSISTED
class LRSCANCHECK2
    : public LR
{
    public:
        LRSCANCHECK2() :
            LR( sizeof( *this ) )
        {
            lrtyp = lrtypScanCheck2;
        }

    private:
        UnalignedLittleEndian< DBID >       le_dbid;
        UnalignedLittleEndian< PGNO >       le_pgno;
        UnalignedLittleEndian< BYTE >       le_bSource;
        UnalignedLittleEndian< DBTIME >     le_dbtimePage;
        UnalignedLittleEndian< DBTIME >     le_dbtimeCurrent;
        UnalignedLittleEndian< ULONG >      le_ulChksum;

    protected:
        VOID InitScanCheckFromLegacyScanCheck( const LRSCANCHECK* const plrscancheck )
        {
            InitScanCheck(
                plrscancheck->Dbid(),
                plrscancheck->Pgno(),
                scsDbScan,
                plrscancheck->DbtimeBefore(),
                plrscancheck->DbtimeAfter(),
                plrscancheck->UsChecksum() );
            lrtyp = lrtypScanCheck;
        }

    public:
        VOID InitScanCheck(
                const DBID dbid,
                const PGNO pgno,
                const BYTE bSource,
                const DBTIME dbtimePage,
                const DBTIME dbtimeCurrent,
                const ULONG ulCompLogicalPageChecksum )
        {
            static_assert( sizeof( ScanCheckSource ) == sizeof( le_bSource ), "ScanCheckSource's size must match le_bSource's." );
            le_dbid          = dbid;
            le_pgno          = pgno;
            le_bSource       = bSource;
            le_dbtimePage    = dbtimePage;
            le_dbtimeCurrent = dbtimeCurrent;
            le_ulChksum      = ulCompLogicalPageChecksum;
        }

        VOID InitScanCheck( const LR* const plr )
        {
            Assert( ( plr->lrtyp == lrtypScanCheck ) || ( plr->lrtyp == lrtypScanCheck2 ) );
            if ( plr->lrtyp == lrtypScanCheck )
            {
                InitScanCheckFromLegacyScanCheck( (LRSCANCHECK*)plr );
            }
            else
            {
                UtilMemCpy( this, plr, sizeof( *this ) );
            }
        }

        DBID Dbid() const                   { return le_dbid; }
        PGNO Pgno() const                   { return le_pgno; }
        BYTE BSource() const                { return le_bSource; }
        DBTIME DbtimePage() const           { return le_dbtimePage; }
        DBTIME DbtimeCurrent() const        { return le_dbtimeCurrent; }
        ULONG UlChecksum() const            { return le_ulChksum; }
};

PERSISTED
class LRINSERT
    :   public LRNODE_
{
    public:
        LRINSERT() : LRNODE_( sizeof( *this ) ) { }

    private:
        UnalignedLittleEndian< USHORT >     mle_cbSuffix;
        UnalignedLittleEndian< USHORT >     mle_cbPrefix;
        UnalignedLittleEndian< USHORT >     mle_cbData;

    public:
        CHAR                                szKey[0];

        INLINE USHORT CbSuffix() const                      { return mle_cbSuffix; }
        INLINE VOID SetCbSuffix( const USHORT cbSuffix )    { mle_cbSuffix = cbSuffix; }

        INLINE USHORT CbPrefix() const                      { return mle_cbPrefix; }
        INLINE VOID SetCbPrefix( const USHORT cbPrefix )    { mle_cbPrefix = cbPrefix; }

        INLINE USHORT CbData() const                        { return mle_cbData; }
        INLINE VOID SetCbData( const USHORT cbData )        { mle_cbData = cbData; }
};

PERSISTED
class LRFLAGINSERT
    :   public LRNODE_
{
    public:
        LRFLAGINSERT() : LRNODE_( sizeof( *this ) ) { }

    private:
        UnalignedLittleEndian< USHORT >     mle_cbKey;
        UnalignedLittleEndian< USHORT >     mle_cbData;

    public:
        BYTE                                rgbData[0];

    public:
        INLINE USHORT CbKey() const                         { return mle_cbKey; }
        INLINE VOID SetCbKey( const USHORT cbKey )          { mle_cbKey = cbKey; }

        INLINE USHORT CbData() const                        { return mle_cbData; }
        INLINE VOID SetCbData( const USHORT cbData )        { mle_cbData = cbData; }
};

PERSISTED
class LRFLAGINSERTANDREPLACEDATA
    :   public LRNODE_
{
    public:
        LRFLAGINSERTANDREPLACEDATA() : LRNODE_( sizeof( *this ) ) { }

    private:
        UnalignedLittleEndian< USHORT >     mle_cbKey;
        UnalignedLittleEndian< USHORT >     mle_cbData;

    public:
        UnalignedLittleEndian< RCEID >      le_rceidReplace;
        BYTE                                rgbData[0];

    public:
        INLINE USHORT CbKey() const                         { return mle_cbKey; }
        INLINE VOID SetCbKey( const USHORT cbKey )          { mle_cbKey = cbKey; }

        INLINE USHORT CbData() const                        { return mle_cbData; }
        INLINE VOID SetCbData( const USHORT cbData )        { mle_cbData = cbData; }
};

PERSISTED
class LRREPLACE
    :   public LRNODE_
{
    public:
        LRREPLACE() : LRNODE_( sizeof( *this ) ) { }

    private:
        enum { cbMask = 0x7fff };
        enum { fDiff2 = 0x8000 };

        UnalignedLittleEndian< USHORT >     mle_cb;
        UnalignedLittleEndian< USHORT >     mle_cbOldData;
        UnalignedLittleEndian< USHORT >     mle_cbNewData;

    public:
        CHAR                                szData[0];

    public:
        INLINE USHORT Cb( void ) const                      { return USHORT( mle_cb & cbMask ); }
        INLINE void SetCb( const USHORT cb )                { Assert( 0 == ( cb & ~cbMask ) ); mle_cb = USHORT( mle_cb & fDiff2 | cb ); }

        INLINE BOOL FDiff2( void ) const                        { return mle_cb & fDiff2; }
        INLINE void SetFDiff2( void )                       { mle_cb = USHORT( mle_cb | fDiff2 ); }
        INLINE void ResetFDiff2( void )                     { mle_cb = USHORT( mle_cb & ~fDiff2 ); }

        INLINE USHORT CbOldData() const                     { return mle_cbOldData; }
        INLINE VOID SetCbOldData( const USHORT cbOldData )  { mle_cbOldData = cbOldData; }

        INLINE USHORT CbNewData() const                     { return mle_cbNewData; }
        INLINE VOID SetCbNewData( const USHORT cbNewData )  { mle_cbNewData = cbNewData; }
};

template< typename TDelta >
struct LRDELTA_TRAITS
{
    static_assert( sizeof( TDelta ) == -1, "Specialize LRDELTA_TRAITS to map an lrtyp to a specific LRDELTA." );
    static const LRTYP lrtyp = lrtypNOP;
};

template<> struct LRDELTA_TRAITS< LONG >            { static const LRTYP lrtyp = lrtypDelta; };
template<> struct LRDELTA_TRAITS< LONGLONG >        { static const LRTYP lrtyp = lrtypDelta64; };

PERSISTED
template< typename TDelta >
class _LRDELTA
    :   public LRNODE_
{
    public:
        _LRDELTA() : LRNODE_( sizeof( *this ) ) { }

    private:
        UnalignedLittleEndian< USHORT >     mle_cbBookmarkKey;
        UnalignedLittleEndian< USHORT >     mle_cbBookmarkData;
        UnalignedLittleEndian< TDelta >     mle_delta;
        UnalignedLittleEndian< USHORT >     mle_cbOffset;

    public:
        typedef LRDELTA_TRAITS< TDelta >    TRAITS;
        BYTE                                rgbData[0];

    public:
        INLINE USHORT CbBookmarkKey() const                 { return mle_cbBookmarkKey; }
        INLINE VOID SetCbBookmarkKey( const USHORT cb )     { mle_cbBookmarkKey = cb; }

        INLINE USHORT CbBookmarkData() const                { return mle_cbBookmarkData; }
        INLINE VOID SetCbBookmarkData( const USHORT cb )    { mle_cbBookmarkData = cb; }

        INLINE TDelta Delta() const                     { return mle_delta; }
        INLINE VOID SetDelta( const TDelta delta )      { mle_delta = delta; }

        INLINE USHORT CbOffset() const                      { return mle_cbOffset; }
        INLINE VOID SetCbOffset( const USHORT cbOffset )    { mle_cbOffset = cbOffset; }
};

typedef _LRDELTA< LONG >            LRDELTA32;
typedef _LRDELTA< LONGLONG >        LRDELTA64;

static_assert( 54 == sizeof( LRDELTA32 ), "LRDELTA32 isn't of the expected size." );
static_assert( 58 == sizeof( LRDELTA64 ), "LRDELTA64 isn't of the expected size." );

PERSISTED
class LRFLAGDELETE
    :   public LRNODE_
{
    public:
        LRFLAGDELETE() : LRNODE_( sizeof( *this ) ) { }
};

PERSISTED
class LRDELETE
    :   public LRNODE_
{
    public:
        LRDELETE() : LRNODE_( sizeof( *this ) ) { }
};

PERSISTED
class LRUNDO
    :   public LRNODE_
{
    public:
        LRUNDO() : LRNODE_( sizeof( *this ) ) { }

    private:
        UnalignedLittleEndian< USHORT >     mle_cbBookmarkKey;
        UnalignedLittleEndian< USHORT >     mle_cbBookmarkData;

    public:
        UnalignedLittleEndian< USHORT >     le_oper;
        UnalignedLittleEndian< LEVEL >      level;
        BYTE                                rgbBookmark[0];

    public:
        INLINE USHORT CbBookmarkKey() const                 { return mle_cbBookmarkKey; }
        INLINE VOID SetCbBookmarkKey( const USHORT cb )     { mle_cbBookmarkKey = cb; }

        INLINE USHORT CbBookmarkData() const                { return mle_cbBookmarkData; }
        INLINE VOID SetCbBookmarkData( const USHORT cb )    { mle_cbBookmarkData = cb; }
};

PERSISTED
class LRUNDOINFO
    :   public LRNODE_
{
    public:
        LRUNDOINFO() : LRNODE_( sizeof( *this ) ) { }

    private:
        UnalignedLittleEndian< USHORT >     mle_cbBookmarkKey;
        UnalignedLittleEndian< USHORT >     mle_cbBookmarkData;

    public:
        UnalignedLittleEndian< USHORT >     le_oper;
        UnalignedLittleEndian< SHORT >      le_cbMaxSize;
        UnalignedLittleEndian< SHORT >      le_cbDelta;
        UnalignedLittleEndian< USHORT >     le_cbData;
        BYTE                                rgbData[0];

    public:
        INLINE USHORT CbBookmarkKey() const                 { return mle_cbBookmarkKey; }
        INLINE VOID SetCbBookmarkKey( const USHORT cb )     { mle_cbBookmarkKey = cb; }

        INLINE USHORT CbBookmarkData() const                { return mle_cbBookmarkData; }
        INLINE VOID SetCbBookmarkData( const USHORT cb )    { mle_cbBookmarkData = cb; }
};

PERSISTED
class LRSETEXTERNALHEADER
    :   public LRNODE_
{
    public:
        LRSETEXTERNALHEADER() : LRNODE_( sizeof( *this ) ) { }

    private:
        UnalignedLittleEndian< USHORT >     mle_cbData;

    public:
        BYTE                                rgbData[0];

    public:
        INLINE USHORT CbData() const                        { return mle_cbData; }
        INLINE VOID SetCbData( const USHORT cbData )        { mle_cbData = cbData; }
};

PERSISTED
class LRSPLIT_
    :   public LRPAGE_
{
    private:
        LRSPLIT_();

    protected:
        LRSPLIT_( const size_t cb ) : LRPAGE_( cb ) { }

    public:
        UnalignedLittleEndian< PGNO >       le_pgnoNew;
        UnalignedLittleEndian< PGNO >       le_pgnoRight;
        UnalignedLittleEndian< DBTIME >     le_dbtimeRightBefore;
        UnalignedLittleEndian< PGNO >       le_pgnoParent;
        UnalignedLittleEndian< DBTIME >     le_dbtimeParentBefore;
        UnalignedLittleEndian< PGNO >       le_pgnoFDP;
        UnalignedLittleEndian< OBJID >      le_objidFDP;

        UnalignedLittleEndian< BYTE >       splittype;
        UnalignedLittleEndian< BYTE >       splitoper;
        UnalignedLittleEndian< USHORT >     le_ilineOper;
        UnalignedLittleEndian< USHORT >     le_ilineSplit;
        UnalignedLittleEndian< USHORT >     le_clines;

        UnalignedLittleEndian< ULONG >      le_fNewPageFlags;
        UnalignedLittleEndian< ULONG >      le_fSplitPageFlags;
        UnalignedLittleEndian< SHORT >      le_cbUncFreeSrc;
        UnalignedLittleEndian< SHORT >      le_cbUncFreeDest;

        UnalignedLittleEndian< LONG >       le_ilinePrefixSplit;
        UnalignedLittleEndian< LONG >       le_ilinePrefixNew;

        UnalignedLittleEndian< USHORT >     le_cbPrefixSplitOld;
        UnalignedLittleEndian< USHORT >     le_cbPrefixSplitNew;

        UnalignedLittleEndian< USHORT >     le_cbKeyParent;
};

PERSISTED
class LRSPLITOLD
    :   public LRSPLIT_
{
    public:
        LRSPLITOLD() : LRSPLIT_( sizeof( *this ) ) { }

    public:
        BYTE                        rgb[0];
};

PERSISTED const ULONG fPreimageDehydrated = 0x1;
PERSISTED const ULONG fPreimageXpress = 0x2;

PERSISTED
class LRSPLITNEW
    :   public LRSPLIT_
{
    public:
        LRSPLITNEW() : LRSPLIT_( sizeof( *this ) ) { }

    private:
        UnalignedLittleEndian< ULONG >      le_flags;
        UnalignedLittleEndian< ULONG >      le_cbPageBeforeImage;

    public:
        BYTE                        rgb[0];

        INLINE ULONG CbPageBeforeImage() const                  { return le_cbPageBeforeImage; }
        INLINE VOID SetCbPageBeforeImage( const ULONG cbData )  { le_cbPageBeforeImage = cbData; }

        VOID SetPreimageDehydrated() { le_flags = le_flags | fPreimageDehydrated; }
        BOOL FPreimageDehydrated() const { return !!( le_flags & fPreimageDehydrated ); }

        VOID SetPreimageXpress() { le_flags = le_flags | fPreimageXpress; }
        BOOL FPreimageXpress() const { return !!( le_flags & fPreimageXpress ); }
};

const BYTE  fLRMergeKeyChange   = 0x01;
const BYTE  fLRMergeDeleteNode  = 0x02;
const BYTE  fLRMergeEmptyPage   = 0x04;

PERSISTED
class LRMERGE_
    :   public LRPAGE_
{
    private:
        LRMERGE_();

    protected:
        LRMERGE_( const size_t cb ) : LRPAGE_( cb ) { }

    public:
        UnalignedLittleEndian< PGNO >       le_pgnoRight;
        UnalignedLittleEndian< DBTIME >     le_dbtimeRightBefore;
        UnalignedLittleEndian< PGNO >       le_pgnoLeft;
        UnalignedLittleEndian< DBTIME >     le_dbtimeLeftBefore;
        UnalignedLittleEndian< PGNO >       le_pgnoParent;
        UnalignedLittleEndian< DBTIME >     le_dbtimeParentBefore;
        UnalignedLittleEndian< PGNO >       le_pgnoFDP;
        UnalignedLittleEndian< OBJID >      le_objidFDP;

        UnalignedLittleEndian< USHORT >     le_cbSizeTotal;
        UnalignedLittleEndian< USHORT >     le_cbSizeMaxTotal;
        UnalignedLittleEndian< USHORT >     le_cbUncFreeDest;

    private:
        UnalignedLittleEndian< USHORT >     mle_ilineMerge;
        UnalignedLittleEndian< BYTE >       m_fLRMergeFlags;

    public:
        UnalignedLittleEndian< BYTE >       mergetype;
        
        UnalignedLittleEndian< USHORT >     le_cbKeyParentSep;

    public:
        INLINE USHORT ILineMerge() const                    { return mle_ilineMerge; }
        INLINE VOID SetILineMerge( const USHORT ilineMerge ){ mle_ilineMerge = ilineMerge; }

        INLINE BOOL FKeyChange() const                      { return ( m_fLRMergeFlags & fLRMergeKeyChange ); }
        INLINE VOID SetFKeyChange()                         { m_fLRMergeFlags = BYTE( m_fLRMergeFlags | fLRMergeKeyChange ); }

        INLINE BOOL FDeleteNode() const                     { return ( m_fLRMergeFlags & fLRMergeDeleteNode ); }
        INLINE VOID SetFDeleteNode()                        { m_fLRMergeFlags = BYTE( m_fLRMergeFlags | fLRMergeDeleteNode ); }

        INLINE BOOL FEmptyPage() const                      { return ( m_fLRMergeFlags & fLRMergeEmptyPage ); }
        INLINE VOID SetFEmptyPage()                         { m_fLRMergeFlags = BYTE( m_fLRMergeFlags | fLRMergeEmptyPage ); }
};

PERSISTED
class LRMERGEOLD
    :   public LRMERGE_
{
    public:
        LRMERGEOLD() : LRMERGE_( sizeof( *this ) ) { }

    public:
        BYTE                                rgb[0];
};

PERSISTED
class LRMERGENEW
    :   public LRMERGE_
{
    public:
        LRMERGENEW() : LRMERGE_( sizeof( *this ) ) { }

    private:
        UnalignedLittleEndian< ULONG >      le_flags;
        UnalignedLittleEndian< ULONG >      le_cbPageBeforeImage;

    public:
        BYTE                                rgb[0];

        INLINE ULONG CbPageBeforeImage() const                  { return le_cbPageBeforeImage; }
        INLINE VOID SetCbPageBeforeImage( const ULONG cbData )  { le_cbPageBeforeImage = cbData; }

        VOID SetPreimageDehydrated() { le_flags = le_flags | fPreimageDehydrated; }
        BOOL FPreimageDehydrated() const { return !!( le_flags & fPreimageDehydrated ); }

        VOID SetPreimageXpress() { le_flags = le_flags | fPreimageXpress; }
        BOOL FPreimageXpress() const { return !!( le_flags & fPreimageXpress ); }
};


PERSISTED
class LREMPTYTREE
    :   public LRNODE_
{
    public:
        LREMPTYTREE() : LRNODE_( sizeof( *this ) ) { }

    private:
        UnalignedLittleEndian< USHORT >     le_cbEmptyPageList;

    public:
        BYTE                                rgb[0];

    public:
        INLINE USHORT CbEmptyPageList() const               { return le_cbEmptyPageList; }
        INLINE VOID SetCbEmptyPageList( const USHORT cb )   { le_cbEmptyPageList = cb; }
};

PERSISTED
class LRNEWPAGE
    :   public LR
{
    public:

        LRNEWPAGE() :
            LR( sizeof( *this ) ),
            le_procid( procidNil ),
            le_dbid( dbidMin ),
            le_pgno( pgnoNull ),
            le_objid( objidNil ),
            le_dbtimeAfter( dbtimeInvalid )
        {
            lrtyp = lrtypNewPage;
        }

    public:

        PROCID Procid() const       { return le_procid; }
        DBID Dbid() const           { return le_dbid; }
        PGNO Pgno() const           { return le_pgno; }
        OBJID Objid() const         { return le_objid; }
        DBTIME DbtimeAfter() const  { return le_dbtimeAfter; }

    public:

        VOID SetProcid( const PROCID procid )           { le_procid = procid; }
        VOID SetDbid( const DBID dbid )                 { le_dbid = dbid; }
        VOID SetPgno( const PGNO pgno )                 { le_pgno = pgno; }
        VOID SetObjid( const OBJID objid )              { le_objid = objid; }
        VOID SetDbtimeAfter( const DBTIME dbtimeAfter ) { le_dbtimeAfter = dbtimeAfter; }

    private:

        UnalignedLittleEndian< PROCID > le_procid;
        UnalignedLittleEndian< DBID >   le_dbid;
        UnalignedLittleEndian< PGNO >   le_pgno;
        UnalignedLittleEndian< OBJID >  le_objid;
        UnalignedLittleEndian< DBTIME > le_dbtimeAfter;
};


PERSISTED
class LRPAGEMOVE
    :   public LRPAGE_
{
    public:
        LRPAGEMOVE();
        VOID AssertValid() const;

    public:
        static const LRPAGEMOVE * PlrpagemoveFromLr( const LR * const plr );
        static BOOL FUnitTest();

    private:
        LRPAGEMOVE( const LRPAGEMOVE& );
        LRPAGEMOVE& operator= ( const LRPAGEMOVE& );

    public:

        enum { fRootNone         = 0x00000000 };
        enum { fRootFDP          = 0x00000001 };
        enum { fRootOE           = 0x00000002 };
        enum { fRootAE           = 0x00000004 };

        
        PGNO PgnoSource() const     { return le_pgno; }
        PGNO PgnoDest() const       { return m_pgnoDest; }
        PGNO PgnoLeft() const       { return m_pgnoLeft; }
        PGNO PgnoRight() const      { return m_pgnoRight; }
        PGNO PgnoParent() const     { return m_pgnoParent; }

        
        DBTIME DbtimeSourceBefore() const   { return le_dbtimeBefore; }
        DBTIME DbtimeLeftBefore() const     { return m_dbtimeLeftBefore; }
        DBTIME DbtimeRightBefore() const    { return m_dbtimeRightBefore; }
        DBTIME DbtimeParentBefore() const   { return m_dbtimeParentBefore; }


        DBTIME DbtimeAfter() const { return le_dbtime; }


        DBID Dbid() const       { return dbid; }
        PGNO PgnoFDP() const    { return m_pgnoFDP; }
        OBJID ObjidFDP() const  { return m_objidFDP; }


        INT IlineParent() const { return m_ilineParent; }
        

        ULONG CbPageHeader() const  { return m_cbPageHeader; }
        ULONG CbPageTrailer() const { return m_cbPageTrailer; }
        ULONG CbTotal() const       { return m_cbPageHeader + m_cbPageTrailer; }

        const VOID * PvPageHeader() const   { return m_rgbData; }
        const VOID * PvPageTrailer() const  { return m_rgbData + m_cbPageHeader; }


        ULONG FMoveFlags() const       { return m_fMoveFlags; }
        BOOL FPageMoveRootFDP() const  { return !!( m_fMoveFlags & fRootFDP ); }
        BOOL FPageMoveRootOE() const   { return !!( m_fMoveFlags & fRootOE ); }
        BOOL FPageMoveRootAE() const   { return !!( m_fMoveFlags & fRootAE ); }
        BOOL FPageMoveRoot() const     { return ( FPageMoveRootFDP() || FPageMoveRootOE() || FPageMoveRootAE() ); }

    public:
    
        VOID SetPgnoSource( const PGNO pgno )   { le_pgno = pgno; }
        VOID SetPgnoDest( const PGNO pgno )     { m_pgnoDest = pgno; }
        VOID SetPgnoLeft( const PGNO pgno )     { m_pgnoLeft = pgno; }
        VOID SetPgnoRight( const PGNO pgno )    { m_pgnoRight = pgno; }
        VOID SetPgnoParent( const PGNO pgno )   { m_pgnoParent = pgno; }

        VOID SetDbtimeSourceBefore( const DBTIME dbtime )   { le_dbtimeBefore = dbtime; }
        VOID SetDbtimeLeftBefore( const DBTIME dbtime )     { m_dbtimeLeftBefore = dbtime; }
        VOID SetDbtimeRightBefore( const DBTIME dbtime )    { m_dbtimeRightBefore = dbtime; }
        VOID SetDbtimeParentBefore( const DBTIME dbtime )   { m_dbtimeParentBefore = dbtime; }

        VOID SetDbtimeAfter( const DBTIME dbtime )  { le_dbtime = dbtime; }

        VOID SetPgnoFDP( const PGNO pgno )      { m_pgnoFDP = pgno; }
        VOID SetObjidFDP( const OBJID objid )   { m_objidFDP = objid; }

        VOID SetIlineParent( const INT iline )  { m_ilineParent = iline; }
        VOID SetCbPageHeader( const ULONG cb )  { m_cbPageHeader = cb; }
        VOID SetCbPageTrailer( const ULONG cb ) { m_cbPageTrailer = cb; }

        VOID SetPageMoveRootFDP()    { m_fMoveFlags = m_fMoveFlags | fRootFDP; }
        VOID SetPageMoveRootOE()     { m_fMoveFlags = m_fMoveFlags | fRootOE; }
        VOID SetPageMoveRootAE()     { m_fMoveFlags = m_fMoveFlags | fRootAE; }

    private:

        UnalignedLittleEndian< PGNO >       m_pgnoDest;
        UnalignedLittleEndian< PGNO >       m_pgnoLeft;
        UnalignedLittleEndian< PGNO >       m_pgnoRight;
        UnalignedLittleEndian< PGNO >       m_pgnoParent;

        UnalignedLittleEndian< DBTIME >     m_dbtimeLeftBefore;
        UnalignedLittleEndian< DBTIME >     m_dbtimeRightBefore;
        UnalignedLittleEndian< DBTIME >     m_dbtimeParentBefore;
        
        UnalignedLittleEndian< PGNO >       m_pgnoFDP;
        UnalignedLittleEndian< OBJID >      m_objidFDP;

        UnalignedLittleEndian< LONG >       m_ilineParent;
        
        UnalignedLittleEndian< ULONG >      m_fMoveFlags;
        UnalignedLittleEndian< ULONG >      m_cbPageHeader;
        UnalignedLittleEndian< ULONG >      m_cbPageTrailer;

        BYTE                                m_rgbData[0];
};

PERSISTED
class LRROOTPAGEMOVE
    :   public LR
{
    public:

        LRROOTPAGEMOVE() :
            LR( sizeof( *this ) ),
            le_dbtimeAfter( dbtimeInvalid ),
            le_dbid( dbidMin ),
            le_objid( objidNil ),
            le_procid( procidNil )
        {
            lrtyp = lrtypRootPageMove;
        }

        VOID AssertValid() const
        {
            Assert( lrtyp == lrtypRootPageMove );
            Assert( ( le_dbtimeAfter != dbtimeNil ) &&
                    ( le_dbtimeAfter != dbtimeInvalid ) &&
                    ( le_dbtimeAfter >= dbtimeStart ) );
            Assert( le_dbid != dbidTemp );
            Assert( le_objid != objidNil );
            Assert( le_procid != procidNil );
        }

    public:

        DBTIME DbtimeAfter() const  { return le_dbtimeAfter; }
        DBID Dbid() const           { return le_dbid; }
        OBJID Objid() const         { return le_objid; }
        PROCID Procid() const       { return le_procid; }

    public:

        VOID SetDbtimeAfter( const DBTIME dbtimeAfter ) { le_dbtimeAfter = dbtimeAfter; }
        VOID SetDbid( const DBID dbid )                 { le_dbid = dbid; }
        VOID SetObjid( const OBJID objid )              { le_objid = objid; }
        VOID SetProcid( const PROCID procid )           { le_procid = procid; }

    private:

        UnalignedLittleEndian< DBTIME > le_dbtimeAfter;
        UnalignedLittleEndian< DBID >   le_dbid;
        UnalignedLittleEndian< OBJID >  le_objid;
        UnalignedLittleEndian< PROCID > le_procid;
};


PERSISTED
class LRCREATEMEFDP
    :   public LRPAGE_
{
    public:
        LRCREATEMEFDP() : LRPAGE_( sizeof( *this ) ) { }

    public:
        UnalignedLittleEndian< OBJID >      le_objidFDP;
        UnalignedLittleEndian< PGNO >       le_pgnoOE;
        UnalignedLittleEndian< PGNO >       le_pgnoAE;
        UnalignedLittleEndian< PGNO >       le_pgnoFDPParent;
        UnalignedLittleEndian< CPG >        le_cpgPrimary;
        UnalignedLittleEndian< ULONG >      le_fPageFlags;
};

PERSISTED
class LRCREATESEFDP
    :   public LRPAGE_
{
    public:
        LRCREATESEFDP() : LRPAGE_( sizeof( *this ) ) { }

    public:
        UnalignedLittleEndian< OBJID >      le_objidFDP;
        UnalignedLittleEndian< PGNO >       le_pgnoFDPParent;
        UnalignedLittleEndian< CPG >        le_cpgPrimary;
        UnalignedLittleEndian< ULONG >      le_fPageFlags;
};

PERSISTED
class LRCONVERTFDP_OBSOLETE
    :   public LRPAGE_
{
    private:
        LRCONVERTFDP_OBSOLETE();

    public:
        LRCONVERTFDP_OBSOLETE( const size_t cb ) : LRPAGE_( cb ) { }

    public:
        UnalignedLittleEndian< OBJID >      le_objidFDP;
        UnalignedLittleEndian< PGNO >       le_pgnoOE;
        UnalignedLittleEndian< PGNO >       le_pgnoAE;
        UnalignedLittleEndian< PGNO >       le_pgnoFDPParent;
        UnalignedLittleEndian< PGNO >       le_pgnoSecondaryFirst;
        UnalignedLittleEndian< CPG >        le_cpgPrimary;
        UnalignedLittleEndian< CPG >        le_cpgSecondary;
};

PERSISTED
class LRCONVERTFDP
    :   public LRCONVERTFDP_OBSOLETE
{
    public:
        LRCONVERTFDP() : LRCONVERTFDP_OBSOLETE( sizeof( *this ) ) { }
        
    public:
        UnalignedLittleEndian< UINT >       le_rgbitAvail;
        UnalignedLittleEndian< ULONG >      le_fCpageFlags;
};

PERSISTED
typedef enum {
    fFDPTypeUnknown = 0,
    fFDPTypeTable = 1,
    fFDPTypeIndex = 2
} FDPTYPE;

PERSISTED
class LRFREEFDP
    : protected LRIGNORED
{
    public:
        LRFREEFDP() : LRIGNORED( lrtypFreeFDP )
        {

            SetCb( sizeof(LRFREEFDP) - sizeof(LRIGNORED) );
        }

        FDPTYPE FFDPType() const { return (FDPTYPE)bFDPType; }

        UnalignedLittleEndian< DBID >       le_dbid;
        UnalignedLittleEndian< PGNO >       le_pgnoFDP;
        UnalignedLittleEndian< TRX >        le_trxCommitted;
        BYTE                                bFDPType;
};

PERSISTED
class LRTRX_
    :   public LR
{
    private:
        LRTRX_();

    protected:
        LRTRX_( const size_t cb ) : LR( cb ) { }

    public:
        UnalignedLittleEndian< USHORT >     le_procid;
};

PERSISTED
class LRBEGIN
    :   public LRTRX_
{
    public:
        LRBEGIN() : LRTRX_( sizeof( *this ) ) { }
        
    public:
        BYTE                                levelBeginFrom:4;       
        BYTE                                clevelsToBegin:4;       
};

PERSISTED
class LRBEGIN0
    :   public LRBEGIN
{
    public:
        LRBEGIN0() : le_trxBegin0( 0 ) {}
        
    public:
        UnalignedLittleEndian< TRX >        le_trxBegin0;
};

PERSISTED
class LRBEGINDT
    :   public LRBEGIN0
{
};

PERSISTED
class LRREFRESH
    :   public LRTRX_
{
    public:
        LRREFRESH() : LRTRX_( sizeof( *this ) ) { }
        
    public:
        UnalignedLittleEndian< TRX >        le_trxBegin0;
};


PERSISTED
class LRPREPCOMMIT
    :   public LRTRX_
{
    public:
        LRPREPCOMMIT() : LRTRX_( sizeof( *this ) ) { }
        
    public:
        UnalignedLittleEndian<ULONG>        le_cbData;
        BYTE                                rgbData[0];
};

PERSISTED
class LRCOMMIT
    :   public LRTRX_
{
    public:
        LRCOMMIT() : LRTRX_( sizeof( *this ) ) { }
        
    public:
        UnalignedLittleEndian< LEVEL >      levelCommitTo;
};

PERSISTED
class LRCOMMIT0
    :   public LRCOMMIT
{
    public:
        LRCOMMIT0() : le_trxCommit0( 0 ) {}
        
    public:
        UnalignedLittleEndian< TRX >        le_trxCommit0;
};

PERSISTED
class LRROLLBACK
    :   public LRTRX_
{
    public:
        LRROLLBACK() : LRTRX_( sizeof( *this ) ) { }
        
    public:
        UnalignedLittleEndian< LEVEL >      levelRollback;
};

PERSISTED
class LRPREPROLLBACK
    :   public LRTRX_
{
    public:
        LRPREPROLLBACK() : LRTRX_( sizeof( *this ) ) { }
};

PERSISTED
class LRMACROBEGIN
    :   public LRTRX_
{
    public:
        LRMACROBEGIN() : LRTRX_( sizeof( *this ) ) { }
        
    public:
        UnalignedLittleEndian< DBTIME >     le_dbtime;
};

PERSISTED
class LRMACROEND
    :   public LRTRX_
{
    public:
        LRMACROEND() : LRTRX_( sizeof( *this ) ) { }
        
    public:
        UnalignedLittleEndian< DBTIME >     le_dbtime;
};


PERSISTED
class LRMACROINFO
    : protected LRIGNORED
{
    public:
        LRMACROINFO() : LRIGNORED( lrtypMacroInfo )
        {
            
            C_ASSERT( sizeof( LRIGNORED ) == sizeof( *this ) );
        }
    
    public:
        void SetCountOfPgno( const CPG cpgno )
        {
            Assert( cpgno > 0 );
            SetCb( cpgno * sizeof( PGNO ) );
            Assert( CountOfPgno() == cpgno );
        }
        CPG CountOfPgno() const
        {
            Assert( Cb() % sizeof ( PGNO ) == 0 );
            return ( Cb() / sizeof( PGNO ) );
        }
        PGNO GetPgno( const CPG ipgno ) const
        {
            Assert( ipgno < CountOfPgno() );
            Assert( m_rgpgno[ ipgno ] != pgnoNull );
            return m_rgpgno[ ipgno ];
        }

    private:
        UnalignedLittleEndian< PGNO >           m_rgpgno[0];
};


PERSISTED
class LRMACROINFO2
    : protected LRIGNORED
{
    public:
        LRMACROINFO2() : LRIGNORED( lrtypMacroInfo2 )
        {

            SetCb( sizeof( LRMACROINFO2 ) - sizeof( LRIGNORED ) );
        }

    public:
        void SetDbid( const DBID dbid )
        {
            le_dbid = dbid;
        }
        DBID Dbid() const
        {
            return le_dbid;
        }
        void SetCountOfPgno( const CPG cpgno )
        {
            le_cpgno = cpgno;
            SetCb( sizeof( LRMACROINFO2 ) - sizeof( LRIGNORED ) + cpgno * sizeof( PGNO ) );
        }
        CPG CountOfPgno() const
        {
            return le_cpgno;
        }
        PGNO GetPgno( const CPG ipgno ) const
        {
            Assert( ipgno < CountOfPgno() );
            Assert( m_rgpgno[ipgno] != pgnoNull );
            return m_rgpgno[ipgno];
        }

    private:
        UnalignedLittleEndian< DBID >           le_dbid;
        UnalignedLittleEndian< ULONG >          le_cpgno;
        UnalignedLittleEndian< PGNO >           m_rgpgno[0];
};

PERSISTED
class LRFORCEWRITELOG
    :   public LRTRX_
{
    public:
        LRFORCEWRITELOG() : LRTRX_( sizeof( *this ) ) { }
};

const BYTE  fLRCreateDbVersionInfo              = 0x04;
const BYTE  fLRCreateDbUnicodeNames             = 0x08;
const BYTE  fLRCreateDbSparseEnabledFile        = 0x10;

PERSISTED
class LRCREATEDB
    :   public LR
{
    public:
        LRCREATEDB() : LR( sizeof( *this ) ) { }

    public:
        UnalignedLittleEndian< USHORT >         le_procid;
        UnalignedLittleEndian< DBID >           dbid;

        UnalignedLittleEndian< JET_GRBIT >      le_grbit;
        UnalignedLittleEndian< ULONG >          le_cpgDatabaseSizeMax;
        SIGNATURE                               signDb;

    private:

        UnalignedLittleEndian< USHORT >         mle_cbPath;
        UnalignedLittleEndian< BYTE >           m_fLRCreateDbFlags;
        UnalignedLittleEndian< WCHAR >          m_rgb[0];

#ifdef LOG_FORMAT_CHANGE
#error "Haven't gotten a fun log format change yet."
        UnalignedLittleEndian< USHORT >         mle_usVersion;
        UnalignedLittleEndian< USHORT >         mle_usUpdateMajor;
#else
    public:
        struct VersionInfo
        {
            UnalignedLittleEndian< USHORT >     mle_usVersion;
            UnalignedLittleEndian< USHORT >     mle_usUpdateMajor;
        };
#endif

    public:
        INLINE __out_range(0, cbAttach-sizeof(ATTACHINFO)) USHORT CbPath() const
        {
            Assert(mle_cbPath <= (cbAttach-sizeof(ATTACHINFO)));
            return mle_cbPath;
        }
        INLINE VOID SetCbPath( __in_range(0, cbAttach-sizeof(ATTACHINFO)) const USHORT cbPath )
        {
            Assert(cbPath <= (cbAttach-sizeof(ATTACHINFO)));
            mle_cbPath = cbPath;
        }

        INLINE BOOL FVersionInfo() const                    { return !!( m_fLRCreateDbFlags & fLRCreateDbVersionInfo ); }
        INLINE VOID SetFVersionInfo()                       { m_fLRCreateDbFlags = BYTE( m_fLRCreateDbFlags | fLRCreateDbVersionInfo ); }

        INLINE BOOL FUnicodeNames() const                   { return !!( m_fLRCreateDbFlags & fLRCreateDbUnicodeNames ); }
        INLINE VOID SetFUnicodeNames()                      { m_fLRCreateDbFlags = BYTE( m_fLRCreateDbFlags | fLRCreateDbUnicodeNames ); }

        INLINE BOOL FSparseEnabledFile() const                  { return !!( m_fLRCreateDbFlags & fLRCreateDbSparseEnabledFile ); }
        INLINE VOID SetFSparseEnabledFile()                     { m_fLRCreateDbFlags = BYTE( m_fLRCreateDbFlags | fLRCreateDbSparseEnabledFile ); }

        INLINE USHORT UsVersion() const                     { return ( FVersionInfo() ? ( (VersionInfo *)m_rgb )->mle_usVersion : 0 ); }
        INLINE USHORT UsUpdateMajor() const                     { return ( FVersionInfo() ? ( (VersionInfo *)m_rgb )->mle_usUpdateMajor : 0 ); }

        INLINE VOID GetNames( __out_bcount(cbAttach-sizeof(ATTACHINFO)) CHAR * const szNames )
                                                            { memcpy( szNames, (CHAR *)( (CHAR *)m_rgb + ( FVersionInfo() ? sizeof(VersionInfo) : 0 ) ), min( CbPath(), cbAttach-sizeof( ATTACHINFO ) ) ); }
        INLINE CHAR * SzNames( )                        { Enforce( !FUnicodeNames() ); return ( (CHAR *)m_rgb + ( FVersionInfo() ? sizeof(VersionInfo) : 0 ) ); }
        INLINE UnalignedLittleEndian< WCHAR > * WszUnalignedNames( )        { Enforce( FUnicodeNames() );  return (UnalignedLittleEndian< WCHAR > *)( (CHAR *)m_rgb + ( FVersionInfo() ? sizeof(VersionInfo) : 0 ) ); }
};

PERSISTED
class LRCREATEDBFINISH
    :   public LR
{
    public:
        LRCREATEDBFINISH() : LR( sizeof( *this ) ) { }

    public:
        UnalignedLittleEndian< USHORT >         le_procid;
        UnalignedLittleEndian< DBID >           dbid;
};

const BYTE  fLRAttachDbUnicodeNames             = 0x08;
const BYTE  fLRAttachDbSparseEnabledFile        = 0x10;

PERSISTED
class LRATTACHDB
    :   public LR
{
    public:
        LRATTACHDB() : LR( sizeof( *this ) ) { }

    public:
        UnalignedLittleEndian< USHORT >     le_procid;
        UnalignedLittleEndian< DBID >       dbid;

        UnalignedLittleEndian< ULONG >      le_cpgDatabaseSizeMax;
        LE_LGPOS                            lgposConsistent;
        SIGNATURE                           signDb;
        SIGNATURE                           signLog;

    private:
        UnalignedLittleEndian< USHORT >     mle_cbPath;
        UnalignedLittleEndian< BYTE >       m_fLRAttachDbFlags;

    public:
        UnalignedLittleEndian< WCHAR >      rgb[0];

    public:

        BOOL FUnicodeNames() const                  { return !!( m_fLRAttachDbFlags & fLRAttachDbUnicodeNames ); }
        VOID SetFUnicodeNames()                     { m_fLRAttachDbFlags = BYTE( m_fLRAttachDbFlags | fLRAttachDbUnicodeNames ); }

        BOOL FSparseEnabledFile() const             { return !!( m_fLRAttachDbFlags & fLRAttachDbSparseEnabledFile ); }
        VOID SetFSparseEnabledFile()                { m_fLRAttachDbFlags = BYTE( m_fLRAttachDbFlags | fLRAttachDbSparseEnabledFile ); }

        USHORT CbPath() const                       { return mle_cbPath; }
        VOID SetCbPath( const USHORT cbPath )       { mle_cbPath = cbPath; }

        UnalignedLittleEndian< WCHAR > * WszUnalignedNames()        { Enforce( FUnicodeNames() ); return rgb; }
};


const BYTE  fLRDetachDBUnicodeNames     = 0x10;

PERSISTED
class LRDETACHCOMMON
    :   public LR
{
    public:
        LRDETACHCOMMON() : LR( sizeof( *this ) ) { }

    public:
        UnalignedLittleEndian< USHORT >     le_procid;
        UnalignedLittleEndian< DBID >       dbid;

    private:
        UnalignedLittleEndian< USHORT >     mle_cbPath;
        UnalignedLittleEndian< BYTE >       m_fLRDetachDbFlags;

    public:
        ULONG CbPath() const                    { return mle_cbPath; }
        VOID SetCbPath( const ULONG cbPath )    { Assert( USHORT( cbPath ) == cbPath ); mle_cbPath = USHORT( cbPath ); }
    public:
        BOOL FUnicodeNames() const              { return ( m_fLRDetachDbFlags & fLRDetachDBUnicodeNames ); }
        INT Flags() const                       { return m_fLRDetachDbFlags; }
        
        VOID SetFUnicodeNames()                 { SetFlags( fLRDetachDBUnicodeNames ); }

    protected:
        VOID SetFlags( BYTE flags )     { m_fLRDetachDbFlags = BYTE( m_fLRDetachDbFlags | flags ); }
};

PERSISTED
class LRDETACHDB
    :   public LRDETACHCOMMON
{
    public:
        UnalignedLittleEndian< WCHAR >      rgb[0];

        UnalignedLittleEndian< WCHAR > * WszUnalignedNames()        { Enforce( FUnicodeNames() ); return rgb; }
};

PERSISTED
class LRSIGNALATTACHDB
    :   public LR
{
    public:

        LRSIGNALATTACHDB() :
            LR( sizeof( *this ) ),
            le_dbid( dbidMin )
        {
            lrtyp = lrtypSignalAttachDb;
        }

    public:

        DBID Dbid() const           { return le_dbid; }

    public:

        VOID SetDbid( const DBID dbid )                 { le_dbid = dbid; }

    private:

        UnalignedLittleEndian< DBID >   le_dbid;
};

PERSISTED
class LRMS
    :   public LR
{
    public:
        LRMS() : LR( sizeof( *this ) ) { }

    public:
        UnalignedLittleEndian< USHORT >     le_ibForwardLink;
        UnalignedLittleEndian< ULONG >      le_ulCheckSum;
        UnalignedLittleEndian< USHORT >     le_isecForwardLink;
};

PERSISTED
class LREXTRESTORE
    :   public LR
{
    public:
        LREXTRESTORE() : LR( sizeof( *this ) ) { }

    private:
        UnalignedLittleEndian<USHORT>       mle_cbInfo;

    public:
        BYTE                                rgb[0];

    public:
    INLINE USHORT CbInfo() const                        { return mle_cbInfo; }
    INLINE VOID SetCbInfo( const USHORT cbInfo )        { mle_cbInfo = cbInfo; }
};

PERSISTED
class LREXTRESTORE2
    :   public LR
{
    public:
        LREXTRESTORE2() : LR( sizeof( *this ) ) { }

    private:
        UnalignedLittleEndian<USHORT>       mle_cbInfo;

    public:
        UnalignedLittleEndian< WCHAR >      rgb[0];

    public:
    INLINE USHORT CbInfo() const                        { return mle_cbInfo; }
    INLINE VOID SetCbInfo( const USHORT cbInfo )        { mle_cbInfo = cbInfo; }
};

#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT

#define bShortChecksumOn    0x9A
#define bShortChecksumOff   0xD1

PERSISTED
class LRCHECKSUM
    :   public LR
{
    public:
        LRCHECKSUM() : LR( sizeof( *this ) ) { }

    public:
        UnalignedLittleEndian< ULONG32 >    le_cbBackwards;
        UnalignedLittleEndian< ULONG32 >    le_cbForwards;
        UnalignedLittleEndian< ULONG32 >    le_cbNext;
        UnalignedLittleEndian< ULONG32 >    le_ulChecksum;
        UnalignedLittleEndian< ULONG32 >    le_ulShortChecksum;
        UnalignedLittleEndian< BYTE >       bUseShortChecksum;
};

#endif

PERSISTED
class LRINIT
    :   public LR
{
    public:
        LRINIT() : LR( sizeof( *this ) ) { }

    public:
        DBMS_PARAM                  dbms_param;
};

PERSISTED
class LRINIT2
    :   public LRINIT
{
    public:
        LRINIT2() { memset( &logtime, 0, sizeof( logtime ) ); }

    public:
        LOGTIME                     logtime;
};

PERSISTED
class LRTERMREC
    :   public LR
{
    public:
        LRTERMREC() : LR( sizeof( *this ) ) { }

    public:
        LE_LGPOS                            lgpos;
        LE_LGPOS                            lgposRedoFrom;
        UnalignedLittleEndian< BYTE >       fHard;
};

PERSISTED
class LRTERMREC2
    :   public LRTERMREC
{
    public:
        LRTERMREC2() { memset( &logtime, 0, sizeof( logtime ) ); }

    public:
        LOGTIME                             logtime;
};

PERSISTED
class LRRECOVERYUNDO
    :   public LR
{
    public:
        LRRECOVERYUNDO() : LR( sizeof( *this ) ) { }

    public:
};

PERSISTED
class LRRECOVERYUNDO2
    :   public LRRECOVERYUNDO
{
    public:
        LRRECOVERYUNDO2() { memset( &logtime, 0, sizeof( logtime ) ); }
        
    public:
        LOGTIME                             logtime;
};


PERSISTED
class LRLOGRESTORE
    :   public LR
{
    public:
        LRLOGRESTORE() : LR( sizeof( *this ) ) { }

    public:
        UnalignedLittleEndian< USHORT >     le_cbPath;
        CHAR                                szData[0];
};

PERSISTED
class LRLOGBACKUP_OBSOLETE
    :   public LR
{
    public:
        LRLOGBACKUP_OBSOLETE() : LR( sizeof( *this ) ) { }

    public:

typedef enum {
    fLGBackupFull,
    fLGBackupIncremental,
    fLGBackupSnapshotStart,
    fLGBackupSnapshotStop,
    fLGBackupTruncateLogs,
    fLGBackupOSSnapshot,
    fLGBackupOSSnapshotIncremental,
} LRLOGBACKUP_TYPE;

    public:
        UnalignedLittleEndian< BYTE >       m_fBackupType;
        UnalignedLittleEndian< BYTE >       m_Reserved;
        UnalignedLittleEndian< USHORT >     le_cbPath;
        CHAR                                szData[0];

    public:
        INLINE BOOL FIncremental() const                    { return ( m_fBackupType == fLGBackupIncremental ); }
        INLINE VOID SetFIncremental()                       { m_fBackupType = fLGBackupIncremental; }
        INLINE BOOL FFull() const                           { return ( m_fBackupType == fLGBackupFull ); }
        INLINE VOID SetFFull()                              { m_fBackupType = fLGBackupFull; }
        INLINE BOOL FSnapshotStart() const                  { return ( m_fBackupType == fLGBackupSnapshotStart ); }
        INLINE VOID SetFSnapshotStart()                     { m_fBackupType = fLGBackupSnapshotStart; }
        INLINE BOOL FSnapshotStop() const                   { return ( m_fBackupType == fLGBackupSnapshotStop ); }
        INLINE VOID SetFSnapshotStop()                      { m_fBackupType = fLGBackupSnapshotStop; }
        INLINE BOOL FTruncateLogs() const                   { return ( m_fBackupType == fLGBackupTruncateLogs ); }
        INLINE VOID SetFTruncateLogs()                      { m_fBackupType = fLGBackupTruncateLogs; }
        INLINE BOOL FOSSnapshot() const                     { return ( m_fBackupType == fLGBackupOSSnapshot ); }
        INLINE VOID SetFOSSnapshot()                        { m_fBackupType = fLGBackupOSSnapshot; }
        INLINE BOOL FOSSnapshotIncremental() const          { return ( m_fBackupType == fLGBackupOSSnapshotIncremental ); }
        INLINE VOID SetFOSSnapshotIncremental()             { m_fBackupType = fLGBackupOSSnapshotIncremental; }
};

PERSISTED
class LRLOGBACKUP
    :   public LR
{
    public:
        LRLOGBACKUP() : LR( sizeof( *this ) ) { }

    public:

typedef enum {
    fLGBackupScopeFull          = 0x01,
    fLGBackupScopeIncremental   = 0x02,
    fLGBackupScopeCopy      = 0x05,
    fLGBackupScopeDifferential  = 0x06,
} LRLOGBACKUP_SCOPE;
typedef enum {
    fLGBackupPhaseBegin         = 0x1,
    fLGBackupPhasePrepareLogs,
    fLGBackupPhaseTruncate,
    fLGBackupPhaseUpdate,
    fLGBackupPhaseAbort,
    fLGBackupPhaseStop,
} LRLOGBACKUP_PHASE;

    public:
        UnalignedLittleEndian< BYTE >       eBackupPhase;
        UnalignedLittleEndian< DBFILEHDR::BKINFOTYPE >  eBackupType;
        UnalignedLittleEndian< BYTE >       eBackupScope;
        UnalignedLittleEndian< DBID >       dbid;
        BKINFO                              phaseDetails;
        UnalignedLittleEndian< BYTE >       m_Reserved;
        UnalignedLittleEndian< USHORT >     le_cbPath;
        CHAR                                szData[0];

};

PERSISTED
class LRJETOP
    :   public LR
{
    public:
        LRJETOP() : LR( sizeof( *this ) ) { }

    public:
        UnalignedLittleEndian< USHORT >     le_procid;
        UnalignedLittleEndian< BYTE >       op;
};

PERSISTED
class LRTRACE
    :   public LR
{
    public:
        LRTRACE() : LR( sizeof( *this ) ) { }

    public:
        UnalignedLittleEndian< USHORT >     le_procid;
        UnalignedLittleEndian< USHORT >     le_cb;

        CHAR    sz[0];
};

#define LRFORCELOGROLLOVER  LRTRACE


PERSISTED
class LREXTENDDB
    : protected LRIGNORED
{
    public:
        LREXTENDDB() : LRIGNORED( lrtypExtendDB )
        {
            
            SetCb( sizeof(LREXTENDDB) - sizeof(LRIGNORED) );
        }

    public:
        void SetDbid( const DBID dbid )
        {
            le_dbid = dbid;
        }
        DBID Dbid() const
        {
            return le_dbid;
        }
        void SetPgnoLast( const PGNO pgnoLast)
        {
            Assert( pgnoLast != pgnoNull );
            le_pgnoLast = pgnoLast;
        }
        PGNO PgnoLast() const
        {
            Assert( le_pgnoLast != pgnoNull );
            return le_pgnoLast;
        }

    protected:
        UnalignedLittleEndian< DBID >       le_dbid;
        UnalignedLittleEndian< PGNO >       le_pgnoLast;
};


const INT cbCommitCtxExpected = 100;

PERSISTED
class LRCOMMITCTXOLD
    : protected LRIGNORED
{
    public:
        LRCOMMITCTXOLD( const INT cbClientCommitContext ) : LRIGNORED( lrtypCommitCtx )
        {

            Expected( cbClientCommitContext < 80 );

            SetCb( sizeof(LRCOMMITCTXOLD) - sizeof(LRIGNORED) + cbClientCommitContext );
        }

    public:
        void SetProcID( const PROCID procid )
        {
            le_procid = procid;
        }
        PROCID ProcID() const
        {
            return le_procid;
        }

        INT CbCommitCtx() const
        {
            return Cb() - ( sizeof(LRCOMMITCTXOLD) - sizeof(LRIGNORED) );
        }
        const BYTE * PbCommitCtx() const
        {
            return rgbData;
        }

    protected:
        UnalignedLittleEndian< USHORT >     le_procid;
        BYTE                                rgbData[0];
};

PERSISTED
#define fCommitCtxCallbackNeeded        0x1
#define fCommitCtxContainsCustomerData  0x2

PERSISTED
class LRCOMMITCTX
    : protected LRIGNORED
{
    public:
        LRCOMMITCTX( const INT cbClientCommitContext ) : LRIGNORED( lrtypCommitCtx )
        {

            Expected( cbClientCommitContext < 80 );

            SetCb( sizeof(LRCOMMITCTX) - sizeof(LRIGNORED) + cbClientCommitContext );
            m_fCtxFlags = 0;
        }

    public:
        void SetProcID( const PROCID procid )
        {
            le_procid = procid;
        }
        PROCID ProcID() const
        {
            return le_procid;
        }

        BOOL FCallbackNeeded() const
        {
            return !!( m_fCtxFlags & fCommitCtxCallbackNeeded );
        }
        VOID SetFCallbackNeeded()
        {
            m_fCtxFlags = ( m_fCtxFlags | fCommitCtxCallbackNeeded );
        }
        VOID ResetFCallbackNeeded()
        {
            m_fCtxFlags = ( m_fCtxFlags & ~fCommitCtxCallbackNeeded );
        }

        BOOL FContainsCustomerData() const
        {
            return !!( m_fCtxFlags & fCommitCtxContainsCustomerData );
        }
        VOID SetContainsCustomerData( BOOL fContainsCustomerData )
        {
            if ( fContainsCustomerData )
            {
                m_fCtxFlags = ( m_fCtxFlags | fCommitCtxContainsCustomerData );
            }
            else
            {
                m_fCtxFlags = ( m_fCtxFlags & ~fCommitCtxContainsCustomerData );
            }
        }

        INT CbCommitCtx() const
        {
            return Cb() - ( sizeof(LRCOMMITCTX) - sizeof(LRIGNORED) );
        }
        const BYTE * PbCommitCtx() const
        {
            return rgbData;
        }

    protected:
        UnalignedLittleEndian< USHORT >     le_procid;
        UnalignedLittleEndian< BYTE >       m_fCtxFlags;
        BYTE                                rgbData[0];
};


PERSISTED
class LRSHRINKDB : public LR
{
    public:
        LRSHRINKDB( const LRTYP lrtypShrink )
            : LR( sizeof( *this ) )
        {
            Assert( ( lrtypShrink == lrtypShrinkDB ) || ( lrtypShrink == lrtypShrinkDB2 ) );
            lrtyp = lrtypShrink;
        }

    protected:
        LRSHRINKDB( const LRTYP lrtypShrink, const size_t cblrshrinkdb )
            : LR( cblrshrinkdb )
        {
            lrtyp = lrtypShrink;
            Assert( cblrshrinkdb >= CbLGSizeOfRec( this ) );
            Assert( cblrshrinkdb >= sizeof( LRSHRINKDB ) );
            Assert( ( lrtypShrink == lrtypShrinkDB ) || ( lrtypShrink == lrtypShrinkDB2 ) || ( lrtypShrink == lrtypShrinkDB3 ) );
        }

    public:
        void SetDbid( const DBID dbid )
        {
            le_dbid = dbid;
        }
        DBID Dbid() const
        {
            return le_dbid;
        }
        void SetPgnoLast( const PGNO pgnoLast)
        {
            Assert( pgnoLast != pgnoNull );
            le_pgnoLast = pgnoLast;
        }
        PGNO PgnoLast() const
        {
            Assert( le_pgnoLast != pgnoNull );
            return le_pgnoLast;
        }
        void SetCpgShrunk( const CPG cpgShrunk )
        {
            Assert( cpgShrunk != 0 );
            le_cpgShrunk = cpgShrunk;
        }
        CPG CpgShrunk() const
        {
            Assert( le_cpgShrunk != 0 );
            return le_cpgShrunk;
        }
    protected:
        UnalignedLittleEndian< DBID >       le_dbid;
        UnalignedLittleEndian< PGNO >       le_pgnoLast;
        UnalignedLittleEndian< ULONG >      le_cpgShrunk;
};


PERSISTED
class LRSHRINKDB3 : public LRSHRINKDB
{
    public:
        LRSHRINKDB3() :
            LRSHRINKDB( lrtypShrinkDB3, sizeof( *this ) )
        {
            le_dbtime = dbtimeNil;
        }

        LRSHRINKDB3( const LRSHRINKDB* const plrshrinkdb ) :
            LRSHRINKDB( plrshrinkdb->lrtyp, sizeof( *this ) )
        {
            le_dbtime = dbtimeNil;
            UtilMemCpy( this, plrshrinkdb, CbLGSizeOfRec( plrshrinkdb ) );
        }

    public:
        void SetDbtime( const DBTIME dbtime )
        {
            Assert( dbtime > 0 );
            le_dbtime = dbtime;
        }
        DBTIME Dbtime() const
        {
            Assert( ( le_dbtime > 0 ) || ( le_dbtime == dbtimeNil ) );
            return le_dbtime;
        }
    protected:
        UnalignedLittleEndian< DBTIME > le_dbtime;
};


PERSISTED
class LRTRIMDB : public LR
{
    public:
        LRTRIMDB()
            : LR( sizeof( *this ) )
        {
            lrtyp = lrtypTrimDB;
        }

    public:
        void SetDbid( const DBID dbid )
        {
            le_dbid = dbid;
        }
        DBID Dbid() const
        {
            return le_dbid;
        }
        void SetPgnoStartZeroes( const PGNO pgnoStartZeroes)
        {
            Assert( pgnoStartZeroes != pgnoNull );
            le_pgnoStartZeroes = pgnoStartZeroes;
        }
        PGNO PgnoStartZeroes() const
        {
            Assert( le_pgnoStartZeroes != pgnoNull );
            return le_pgnoStartZeroes;
        }
        void SetCpgZeroLength( const CPG cpgZeroLength )
        {
            Assert( cpgZeroLength != 0 );
            le_cpgZeroLength = cpgZeroLength;
        }
        CPG CpgZeroLength() const
        {
            Assert( le_cpgZeroLength != 0 );
            return le_cpgZeroLength;
        }
    protected:
        UnalignedLittleEndian< DBID >       le_dbid;
        UnalignedLittleEndian< PGNO >       le_pgnoStartZeroes;
        UnalignedLittleEndian< ULONG >      le_cpgZeroLength;
};

PERSISTED
class LRREATTACHDB
    : protected LRIGNORED
{
    public:
        LRREATTACHDB() : LRIGNORED( lrtypReAttach )
        {

            SetCb( sizeof(LRREATTACHDB) - sizeof(LRIGNORED) );
        }

        UnalignedLittleEndian< DBID >       le_dbid;

        LE_LGPOS                            lgposAttach;
        LE_LGPOS                            lgposConsistent;
        SIGNATURE                           signDb;
};


PERSISTED
class LRFRAGMENTBEGIN : public LR
{
public:
    LRFRAGMENTBEGIN()
      : LR( sizeof( *this ) )
    {
        lrtyp = lrtypFragmentBegin;
    }

    UnalignedLittleEndian<ULONG>    le_cbTotalLRSize;
    UnalignedLittleEndian<USHORT>   le_cbFragmentSize;
    BYTE                            le_rgbData[0];
};

PERSISTED
class LRFRAGMENT : public LR
{
public:
    LRFRAGMENT()
      : LR( sizeof( *this ) )
    {
        lrtyp = lrtypFragment;
    }

    UnalignedLittleEndian<USHORT>   le_cbFragmentSize;
    BYTE                            le_rgbData[0];
};

PERSISTED
class LRSETDBVERSION : public LR
{
public:
    LRSETDBVERSION()
      : LR( sizeof( *this ) )
    {
        lrtyp = lrtypSetDbVersion;
    }

    void SetDbid( const DBID dbid )
    {
        le_dbid = dbid;
    }
    DBID Dbid() const
    {
        return le_dbid;
    }

    void SetDbVersion( const DbVersion& dbv )
    {
        le_ulDbMajorVersion = dbv.ulDbMajorVersion;
        le_ulDbUpdateMajor = dbv.ulDbUpdateMajor;
        le_ulDbUpdateMinor = dbv.ulDbUpdateMinor;
    }

    DbVersion Dbv() const
    {
        DbVersion dbvRet = { le_ulDbMajorVersion, le_ulDbUpdateMajor, le_ulDbUpdateMinor };
        return dbvRet;
    }

protected:
    UnalignedLittleEndian<DBID>     le_dbid;

    UnalignedLittleEndian<ULONG>    le_ulDbMajorVersion;
    UnalignedLittleEndian<ULONG>    le_ulDbUpdateMajor;
    UnalignedLittleEndian<ULONG>    le_ulDbUpdateMinor;
};

PERSISTED
class LREXTENTFREED
    : public LR
{
    public:
        LREXTENTFREED() :
            LR( sizeof( *this ) )
        {
            lrtyp = lrtypExtentFreed;
            m_fFlags = 0;
        }

    private:
        UnalignedLittleEndian< DBID >       le_dbid;
        UnalignedLittleEndian< PGNO >       le_pgnoFirst;
        UnalignedLittleEndian< CPG >        le_cpgExtent;
        BYTE                                m_fFlags;

    public:
        VOID InitExtentFreed(
                const DBID  dbid,
                const PGNO  pgno,
                const CPG   cpgExtent )
        {
            le_dbid         = dbid;
            le_pgnoFirst    = pgno;
            le_cpgExtent    = cpgExtent;
        }

        VOID InitExtentFreed( const LR* const plr )
        {
            Assert( ( plr->lrtyp == lrtypExtentFreed ) );
            UtilMemCpy( this, plr, sizeof( *this ) );
        }

        DBID Dbid( ) const                          { return le_dbid; }
        void SetDbid(const DBID dbid)               { le_dbid = dbid; }

        PGNO PgnoFirst( ) const                     { return le_pgnoFirst; }
        void SetPgnoFirst(const PGNO pgnoFirst)     { le_pgnoFirst = pgnoFirst; }

        CPG  CpgExtent( ) const                     { return le_cpgExtent; }
        void SetCpgExtent( const CPG cpgExtent)     { le_cpgExtent = cpgExtent; }
};

#include <poppack.h>

INLINE const BYTE * PbData( const LRSPLIT_ * const plrsplit )
{
    switch( plrsplit->lrtyp )
    {
        case lrtypSplit:
            return( ((LRSPLITOLD *)plrsplit)->rgb);
            break;
        case lrtypSplit2:
            return( ((LRSPLITNEW *)plrsplit)->rgb);
            break;
        default:
            Assert( fFalse );
            return 0;
    }
}

INLINE LONG CbPageBeforeImage( const LRSPLIT_ * const plrsplit )
{
    switch( plrsplit->lrtyp )
    {
        case lrtypSplit:
            return 0;
            break;
        case lrtypSplit2:
            return ((LRSPLITNEW *)plrsplit)->CbPageBeforeImage();
            break;
        default:
            Assert( fFalse );
            return 0;
    }
}

INLINE const BYTE * PbData( const LRMERGE_ * const plrmerge )
{
    switch( plrmerge->lrtyp )
    {
        case lrtypMerge:
            return( ((LRMERGEOLD *)plrmerge)->rgb);
            break;
        case lrtypMerge2:
            return( ((LRMERGENEW *)plrmerge)->rgb);
            break;
        default:
            Assert( fFalse );
            return 0;
    }
}

INLINE LONG CbPageBeforeImage( const LRMERGE_ * const plrmerge )
{
    switch( plrmerge->lrtyp )
    {
        case lrtypMerge:
            return 0;
            break;
        case lrtypMerge2:
            return ((LRMERGENEW *)plrmerge)->CbPageBeforeImage();
            break;
        default:
            Assert( fFalse );
            return 0;
    }
}

#define fMustDirtyCSR   fTrue
#define fDontDirtyCSR   fFalse

VOID SIGGetSignature( SIGNATURE *psign );
BOOL FSIGSignSet( const SIGNATURE *psign );
VOID SIGResetSignature( SIGNATURE *psign );
ERR ErrLGInsert( const FUCB             * const pfucb,
                 CSR                    * const pcsr,
                 const KEYDATAFLAGS&    kdf,
                 const RCEID            rceid,
                 const DIRFLAG          dirflag,
                 LGPOS                  * const plgpos,
                 const VERPROXY         * const pverproxy,
                 const BOOL             fDirtyCSR );

ERR ErrLGFlagInsert( const FUCB             * const pfucb,
                     CSR                    * const pcsr,
                     const KEYDATAFLAGS&    kdf,
                     const RCEID            rceid,
                     const DIRFLAG          dirflag,
                     LGPOS                  * const plgpos,
                     const VERPROXY         * const pverproxy,
                    const BOOL              fDirtyCSR );


ERR  ErrLGFlagInsertAndReplaceData( const FUCB          * const pfucb,
                                    CSR         * const pcsr,
                                    const KEYDATAFLAGS& kdf,
                                    const RCEID         rceidInsert,
                                    const RCEID         rceidReplace,
                                    const DIRFLAG       dirflag,
                                    LGPOS               * const plgpos,
                                    const VERPROXY      * const pverproxy,
                                    const BOOL          fDirtyCSR );


ERR ErrLGFlagDelete( const FUCB         * const pfucb,
                     CSR            * const pcsr,
                     const RCEID        rceid,
                     const DIRFLAG      dirflag,
                     LGPOS              * const plgpos,
                     const VERPROXY     * const pverproxy,
                     const BOOL             fDirtyCSR );
ERR ErrLGReplace(
    const FUCB* const   pfucb,
    CSR* const          pcsr,
    const DATA&         dataOld,
    const DATA&         dataNew,
    const DATA* const   pdataDiff,
    const RCEID         rceid,
    const DIRFLAG       dirflag,
    LGPOS* const        plgpos,
    const BOOL          fDirtyCSR,
    const DBTIME dbtimeBefore = dbtimeNil );

template< typename TDelta >
ERR ErrLGDelta( const FUCB      *pfucb,
                CSR     *pcsr,
                const BOOKMARK& bm,
                INT             cbOffset,
                TDelta          tDelta,
                RCEID           rceid,
                DIRFLAG         dirflag,
                LGPOS           *plgpos,
                const BOOL      fDirtyCSR );

template ERR ErrLGDelta<LONG>( const FUCB *pfucb, CSR *pcsr, const BOOKMARK& bm, INT cbOffset, LONG delta, RCEID rceid, DIRFLAG dirflag, LGPOS *plgpos, const BOOL fDirtyCSR );
template ERR ErrLGDelta<LONGLONG>( const FUCB *pfucb, CSR *pcsr, const BOOKMARK& bm, INT cbOffset, LONGLONG delta, RCEID rceid, DIRFLAG dirflag, LGPOS *plgpos, const BOOL fDirtyCSR );

ERR ErrLGSetExternalHeader(
    const FUCB* pfucb,
    CSR*        pcsr,
    const DATA& data,
    LGPOS*      plgpos,
    const BOOL  fDirtyCSR,
    const DBTIME dbtimeBefore = dbtimeNil );

ERR ErrLGScrub(
    __in                        PIB * const             ppib,
    __in                        const IFMP              ifmp,
    __in                        CSR * const             pcsr,
    __in                        const BOOL              fUnusedPage,
    __in_ecount_opt(cscrubOper) const SCRUBOPER * const rgscrubOper,
    __in                        const INT               cscrubOper,
    __out                       LGPOS * const           plgpos );

ERR ErrLGNewPage(
    __in    PIB * const     ppib,
    __in    const IFMP      ifmp,
    __in    const PGNO      pgno,
    __in    const OBJID     objid,
    __in    const DBTIME    dbtime,
    __out   LGPOS * const   plgpos );

ERR ErrLGScanCheck(
    __in    const IFMP      ifmp,
    __in    const PGNO      pgno,
    __in    const BYTE      bSource,
    __in    const DBTIME    dbtimePage,
    __in    const DBTIME    dbtimeCurrent,
    __in    const ULONG     ulChecksum,
    __in    const BOOL      fScanCheck2Supported,
    __in    LGPOS* const    plgposLogRec = NULL );

ERR ErrLGPageMove(
    __in    const FUCB * const  pfucb,
    __in    MERGEPATH * const   pmergePath,
    __out   LGPOS * const       plgpos );

ERR ErrLGRootPageMove(
    __in    PIB * const         ppib,
    __in    const IFMP          ifmp,
    __in    ROOTMOVE * const    prm,
    __out   LGPOS * const       plgpos );

ERR ErrLGDelete(    const FUCB      *pfucb,
                    CSR             *pcsr,
                    LGPOS           *plgpos,
                    const BOOL      fDirtyCSR );


ERR ErrLGUndoInfo( const RCE *prce, LGPOS *plgpos );
ERR ErrLGUndoInfoWithoutRetry( const RCE *prce, LGPOS *plgpos );


ERR ErrLGBeginTransaction( PIB *ppib );
ERR ErrLGCommitTransaction( PIB *ppib, const LEVEL levelCommitTo, BOOL fFireRedoCallback, TRX *ptrxCommit0, LGPOS *plgposRec );
ERR ErrLGRollback( PIB *ppib, LEVEL levelsRollback, BOOL fRollbackToLevel0, BOOL fFireRedoCallback );
ERR ErrLGUndo(
    RCE                 *prce,
    CSR                 *pcsr,
    const BOOL          fDirtyCSR );
ERR ErrLGMacroAbort(
    PIB                 *ppib,
    DBTIME               dbtime,
    LGPOS               *plgpos);

ERR ErrLGCreateDB(
    _In_ PIB *                  ppib,
    _In_ const IFMP             ifmp,
    _In_ const JET_GRBIT        grbit,
    _In_ BOOL                   fSparseFile,
    _Out_ LGPOS * const         plgposRec );

ERR ErrLGCreateDBFinish(
    PIB                 *ppib,
    const IFMP          ifmp,
    LGPOS               *plgposRec );

ERR ErrLGAttachDB(
    _In_ PIB                *ppib,
    _In_ const IFMP         ifmp,
    _In_ const BOOL         fSparseFile,
    _Out_ LGPOS *const      plgposRec );

ERR ErrLGReAttachDB(
    _In_ const IFMP         ifmp,
    _Out_ LGPOS *const      plgposRec );

ERR ErrLGSignalAttachDB( _In_ const IFMP ifmp );

ERR ErrLGSetDbVersion(
    _In_ PIB                *ppib,
    _In_ const IFMP         ifmp,
    _In_ const DbVersion&   dbvUpdate,
    _Out_ LGPOS             *plgposRec );

ERR ErrLGForceWriteLog(
    PIB * ppib );

ERR ErrLGForceLogRollover(
    PIB * const         ppib,
    __in PSTR           szTrace,
    LGPOS* const        plgposLogRec    = NULL );

inline ERR ErrLGForceLogRollover(
    INST * const        pinst,
    __in PSTR           szTrace,
    LGPOS* const        plgposLogRec    = NULL )
{
    PIB pibFake;
    pibFake.m_pinst = pinst;
    pibFake.procid = 0;
    pibFake.lgposStart.lGeneration = lgposMin.lGeneration;
    pibFake.lgposStart.isec = lgposMin.isec;
    pibFake.lgposStart.ib = lgposMin.ib;
    return ErrLGForceLogRollover( &pibFake, szTrace, plgposLogRec );
}

ERR ErrLGDetachDB(
    PIB                 *ppib,
    const IFMP          ifmp,
    BYTE                flags,
    LGPOS               *plgposRec );

ERR ErrLGEmptyTree(
    FUCB            * const pfucb,
    CSR             * const pcsrRoot,
    EMPTYPAGE       * const rgemptypage,
    const CPG       cpgToFree,
    LGPOS           * const plgpos );
ERR ErrLGMerge(
    const FUCB          *pfucb,
    MERGEPATH           *pmergePath,
    LGPOS               *plgpos );
ERR ErrLGSplit(
    const FUCB          * const pfucb,
    _In_ SPLITPATH      * const psplitPath,
    const KEYDATAFLAGS& kdfOper,
    const RCEID         rceid1,
    const RCEID         rceid2,
    const DIRFLAG       dirflag,
    const DIRFLAG       dirflagPrevAssertOnly,
    LGPOS               * const plgpos,
    const VERPROXY      * const pverproxy );

ERR ErrLGCreateMultipleExtentFDP(
    const FUCB          *pfucb,
    const CSR           *pcsr,
    const SPACE_HEADER  *psph,
    const ULONG         fPageFlags,
    LGPOS               *plgpos );
ERR ErrLGCreateSingleExtentFDP(
    const FUCB          *pfucb,
    const CSR           *pcsr,
    const SPACE_HEADER  *psph,
    const ULONG         fPageFlags,
    LGPOS               *plgpos );
ERR ErrLGConvertFDP(
    const FUCB          *pfucb,
    const CSR           *pcsr,
    const SPACE_HEADER  *psph,
    const PGNO          pgnoSecondaryFirst,
    const CPG           cpgSecondary,
    const DBTIME        dbtimeBefore,
    const UINT          rgbitAvail,
    const ULONG         fCpageFlags,
    LGPOS               *plgpos );
ERR ErrLGFreeFDP(
    const FCB           *pfcb,
    const TRX           trxCommitted );

ERR ErrLGStart( INST *pinst );
ERR ErrLGShutDownMark( INST *pinst, LGPOS *plgposShutDownMark );

ERR ErrLGRecoveryUndo( LOG *plog, BOOL fAggressiveRollover );

#define ErrLGRecoveryQuit( plog, ple_lgposRecoveryUndo, ple_lgposRedoFrom, fKeepDbAttached, fHard, fAggressiveRollover, plgposRecoveryQuit) \
    ErrLGQuitRec( plog, fKeepDbAttached ? lrtypRecoveryQuitKeepAttachments : lrtypRecoveryQuit2, ple_lgposRecoveryUndo, ple_lgposRedoFrom, fHard, fAggressiveRollover, plgposRecoveryQuit )
#define ErrLGQuit( plog, ple_lgposStart, fAggressiveRollover, plgposQuit ) \
    ErrLGQuitRec( plog, lrtypTerm2, ple_lgposStart, pNil, 0, fAggressiveRollover, plgposQuit)
ERR ErrLGQuitRec( LOG *plog, LRTYP lrtyp, const LE_LGPOS *ple_lgposQuit, const LE_LGPOS *ple_lgposRedoFrom, BOOL fHard, BOOL fAggressiveRollover, LGPOS * plgposQuit);

#define ErrLGBackupBegin( plog, bkinfotype, fInc, plgposLogRec )                    \
    ErrLGLogBackup( plog, LRLOGBACKUP::fLGBackupPhaseBegin, bkinfotype, \
            (fInc) ? LRLOGBACKUP::fLGBackupScopeIncremental : LRLOGBACKUP::fLGBackupScopeFull,  \
        0, 0, NULL, NULL, 0, \
        0, \
        plgposLogRec )
#define ErrLGBackupPrepLogs( plog, bkinfotype, fInc, plogtime, fLGFlags, plgposLogRec )     \
    ErrLGLogBackup( plog, LRLOGBACKUP::fLGBackupPhasePrepareLogs,   bkinfotype,                     \
            (fInc) ? LRLOGBACKUP::fLGBackupScopeIncremental : LRLOGBACKUP::fLGBackupScopeFull,  \
        0, 0, NULL, plogtime, 0, \
        fLGFlags, \
        plgposLogRec )
#define ErrLGBackupTruncate( plog, bkinfotype, fInc, lgenDeleteMic, lgenDeleteMac, plgposLogRec )   \
    ErrLGLogBackup( plog, LRLOGBACKUP::fLGBackupPhaseTruncate, bkinfotype, \
            (fInc) ? LRLOGBACKUP::fLGBackupScopeIncremental : LRLOGBACKUP::fLGBackupScopeFull,  \
        lgenDeleteMic, lgenDeleteMac, NULL, NULL, 0, \
        0, \
        plgposLogRec )
#define ErrLGBackupUpdate( plog, bkinfotype, fLogOnly, fLogTrunc, lgenLow, lgenHigh, plgpos, plogtime, dbid, plgposLogRec ) \
    ErrLGLogBackup( plog, LRLOGBACKUP::fLGBackupPhaseUpdate, bkinfotype, \
        ( fLogTrunc ) ? ( fLogOnly ? LRLOGBACKUP::fLGBackupScopeIncremental : LRLOGBACKUP::fLGBackupScopeFull ) : ( fLogOnly ? LRLOGBACKUP::fLGBackupScopeDifferential : LRLOGBACKUP::fLGBackupScopeCopy ), \
        lgenLow, lgenHigh, plgpos, plogtime, dbid, \
        0, \
        plgposLogRec )
#define ErrLGBackupAbort( plog, bkinfotype, fInc, plgposLogRec )                    \
    ErrLGLogBackup( plog, LRLOGBACKUP::fLGBackupPhaseAbort, bkinfotype, \
            (fInc) ? LRLOGBACKUP::fLGBackupScopeIncremental : LRLOGBACKUP::fLGBackupScopeFull,  \
        0, 0, NULL, NULL, 0, \
        0, \
        plgposLogRec )
#define ErrLGBackupStop( plog, bkinfotype, fInc, fLGFlags, plgposLogRec )                       \
    ErrLGLogBackup( plog, LRLOGBACKUP::fLGBackupPhaseStop, bkinfotype, \
            (fInc) ? LRLOGBACKUP::fLGBackupScopeIncremental : LRLOGBACKUP::fLGBackupScopeFull,  \
        0, 0, NULL, NULL, 0, \
        fLGFlags, \
        plgposLogRec )

ERR ErrLGLogBackup(
    LOG *                               plog,
    LRLOGBACKUP::LRLOGBACKUP_PHASE      ePhase,
    DBFILEHDR::BKINFOTYPE               eType,
    LRLOGBACKUP::LRLOGBACKUP_SCOPE      eScope,
    const INT                           lgenLow,
    const INT                           lgenHigh,
    const LGPOS *                       plgpos,
    const LOGTIME *                     plogtime,
    const DBID                          dbid,
    const BOOL                          fLGFlags,
    LGPOS*                              plgposLogRec );

ERR ErrLGPagePatchRequest( LOG * const plog, const IFMP ifmp, const PGNO pgno, const DBTIME dbtime );
ERR ErrLGExtendDatabase( LOG * const plog, const IFMP ifmp, PGNO pgnoLast, LGPOS* const plgposExtend );
ERR ErrLGShrinkDatabase( LOG * const plog, const IFMP ifmp, const PGNO pgnoLast, const CPG cpgShrunk, LGPOS* const plgposShrink );
ERR ErrLGTrimDatabase( _In_ LOG * const plog, _In_ const IFMP ifmp, _In_ PIB* const ppib, _In_ const PGNO pgnoStartZeroes, _In_ const CPG cpgZeroLength );
ERR ErrLGIgnoredRecord( LOG * const plog, const IFMP ifmp, const INT cb );
ERR ErrLGExtentFreed( LOG * const plog, const IFMP ifmp, const PGNO pgnoFirst, const CPG cpgExtent );

ERR ErrLGWaitForWrite( PIB* const ppib, const LGPOS* const plgposLogRec );
ERR ErrLGWrite( PIB* const ppib );
ERR ErrLGScheduleWrite( PIB* const ppib, DWORD cmsecDurableCommit, LGPOS lgposCommit );
ERR ErrLGWaitForWrite( PIB* const ppib, const LGPOS* const plgposLogRec );
ERR ErrLGFlush( LOG* const plog, const IOFLUSHREASON iofr, const BOOL fDisableDeadlockDetection = fFalse );

INLINE INT CmpLgpos( const LGPOS& lgpos1, const LGPOS& lgpos2 )
{
    BYTE    *rgb1   = (BYTE *) &lgpos1;
    BYTE    *rgb2   = (BYTE *) &lgpos2;

#ifdef _X86_
    if ( *( (DWORD*) ( rgb1 + 4 ) ) < *( (DWORD*) ( rgb2 + 4 ) ) )
        return -1;
    if ( *( (DWORD*) ( rgb1 + 4 ) ) > *( (DWORD*) ( rgb2 + 4 ) ) )
        return 1;

    if ( *( (DWORD*) ( rgb1 + 0 ) ) < *( (DWORD*) ( rgb2 + 0 ) ) )
        return -1;
    if ( *( (DWORD*) ( rgb1 + 0 ) ) > *( (DWORD*) ( rgb2 + 0 ) ) )
        return 1;
#else
    if ( FHostIsLittleEndian() )
    {
        if ( *( (QWORD*) ( rgb1 + 0 ) ) < *( (QWORD*) ( rgb2 + 0 ) ) )
            return -1;
        if ( *( (QWORD*) ( rgb1 + 0 ) ) > *( (QWORD*) ( rgb2 + 0 ) ) )
            return 1;
    }
    else
    {
        INT t;
        if ( 0 == (t = lgpos1.lGeneration - lgpos2.lGeneration) )
        {
            if ( 0 == ( t = lgpos1.isec - lgpos2.isec ) )
            {
                return lgpos1.ib - lgpos2.ib;
            }
        }
        return t;
    }
#endif

    return 0;
}

INLINE INT CmpLgpos( const LGPOS* plgpos1, const LGPOS* plgpos2 )
{
    return CmpLgpos( *plgpos1, *plgpos2 );
}

INLINE INT CmpLgpos( const LE_LGPOS* plgpos1, const LGPOS* plgpos2 )
{
    return CmpLgpos( *plgpos1, *plgpos2 );
}

INLINE INT CmpLgpos( const LGPOS* plgpos1, const LE_LGPOS* plgpos2 )
{
    return CmpLgpos( *plgpos1, *plgpos2 );
}

INLINE INT CmpLgpos( const LE_LGPOS* plgpos1, const LE_LGPOS* plgpos2 )
{
    return CmpLgpos( *plgpos1, *plgpos2 );
}

#define MaxLgpos( lgpos1, lgpos2 )          \
    ( CmpLgpos( (lgpos1), (lgpos2) ) < 0 ) ? (lgpos2) : (lgpos1)
    
#define MinLgpos( lgpos1, lgpos2 )          \
    ( CmpLgpos( (lgpos1), (lgpos2) ) < 0 ) ? (lgpos1) : (lgpos2)

const char *SzLrtyp( LRTYP lrtyp );

inline const char* OSFormatLgpos( const LGPOS& lgpos )
{
    return OSFormat( "%06X,%04X,%04X", lgpos.lGeneration, lgpos.isec, lgpos.ib );
}


