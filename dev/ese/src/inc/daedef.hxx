// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef DAEDEF_INCLUDED
#error DAEDEF.HXX already included
#endif

#define DAEDEF_INCLUDED

#include "collection.hxx"


#ifdef DATABASE_FORMAT_CHANGE
#endif


#define CHECK_UNIQUE_KEY_ON_NONUNIQUE_INDEX

#define PARALLEL_BATCH_INDEX_BUILD
#define DONT_LOG_BATCH_INDEX_BUILD

#ifndef ESENT
#define XPRESS9_COMPRESSION
#ifdef _AMD64_
#define XPRESS10_COMPRESSION
#endif
#endif


#ifdef DEBUG
#endif

#define VTAPI

#define PERSISTED


#ifdef _WIN64
#define FMTSZ3264 L"I64"
#else
#define FMTSZ3264 L"l"
#endif

class PIB;
struct FUCB;
class FCB;
struct SCB;
class TDB;
class IDB;
struct DIB;

class RCE;

class VER;
class LOG;
class ILogStream;
class CShadowLogStream;

class OLDDB_STATUS;
class DATABASESCANNER;
class CIrsOpContext;

class CRevertSnapshot;
class RBSCleaner;
class CRBSRevertContext;

typedef unsigned char SCRUBOPER;

typedef LONG    UPDATEID;
const UPDATEID  updateidIncrement   = 2;
const UPDATEID  updateidMin         = 1;
const UPDATEID  updateidNil         = 0;

typedef USHORT          PROCID;
typedef INT             DIRFLAG;
enum DIRLOCK { readLock, writeLock };

#define pNil            0
#define pbNil           ((BYTE *)0)
#define pinstNil        ((INST *)0)
#define plineNil        ((LINE *)0)
#define pkeyNil         ((KEY *)0)
#define ppibNil         ((PIB *)0)
#define pwaitNil        ((WAIT *)0)
#define pssibNil        ((SSIB *)0)
#define pfucbNil        ((FUCB *)0)
#define pcsrNil         ((CSR *)0)
#define pfcbNil         ((FCB *)0)
#define ptdbNil         ((TDB *)0)
#define pfieldNil       ((FIELD *)0)
#define pidbNil         ((IDB *)0)
#define procidNil       ((PROCID) 0xffff)
#define prceheadNil     ((RCEHEAD *)0)
#define precNil         ((REC *)0)
#define pfmpNil         ((FMP *)0)

typedef ULONG           PGNO;
const PGNO pgnoNull     = 0;
const PGNO pgnoMax      = (PGNO)-1;

const PGNO              pgnoScanFirst           = 1;
PERSISTED const PGNO    pgnoScanLastSentinel    = (PGNO)-2;

typedef LONG            CPG;

typedef LONG FMPGNO;
typedef FMPGNO CFMPG;

typedef BYTE            LEVEL;
const LEVEL levelNil    = LEVEL( 0xff );

typedef BYTE            _DBID;
typedef _DBID           DBID;
typedef WORD            _FID;

enum FIDTYP { fidtypUnknown = 0, fidtypFixed, fidtypVar, fidtypTagged };
enum FIDLIMIT { fidlimMin = 0, fidlimMax = (0xFFFE), fidlimNone = (0xFFFF) };

#define cbitFixed           32
#define cbitVariable        32
#define cbitFixedVariable   (cbitFixed + cbitVariable)
#define cbitTagged          192
const ULONG cbRgbitIndex    = 32;

class FID
{
private:
    _FID m_fidVal;

    
public:
    static const _FID _fidTaggedNoneFullVal = JET_ccolMost + 1;
    static const _FID _fidTaggedMaxVal      = JET_ccolMost;
    static const _FID _fidTaggedMinVal      = 256;
    static const _FID _fidTaggedNoneVal     = _fidTaggedMinVal - 1;

    static const _FID _fidVarNoneFullVal    = _fidTaggedMinVal;
    static const _FID _fidVarMaxVal         = _fidTaggedMinVal - 1;
    static const _FID _fidVarMinVal         = 128;
    static const _FID _fidVarNoneVal        = _fidVarMinVal - 1;

    static const _FID _fidFixedNoneFullVal  = _fidVarMinVal;
    static const _FID _fidFixedMaxVal       = _fidVarMinVal - 1;
    static const _FID _fidFixedMinVal       = 1;
    static const _FID _fidFixedNoneVal      = _fidFixedMinVal - 1;
    
public:
    FID( const _FID wFidVal = 0 )
    {
        m_fidVal = wFidVal;
        Assert( FValid() );
    }

    explicit FID( const FIDTYP fidtypType, const WORD wIndex )
    {
        switch ( fidtypType )
        {
        case fidtypFixed:
            switch ( wIndex )
            {
            case fidlimMax:
                m_fidVal = _fidFixedMaxVal;
                break;

            case fidlimNone:
                m_fidVal = _fidFixedNoneVal;
                break;

            default:
                m_fidVal = _fidFixedMinVal + wIndex;
                Assert( FFixed() );
                break;
            }
            break;
            
        case fidtypVar:
            switch ( wIndex )
            {
            case fidlimMax:
                m_fidVal = _fidVarMaxVal;
                break;

            case fidlimNone:
                m_fidVal = _fidVarNoneVal;
                break;

            default:
                m_fidVal = _fidVarMinVal + wIndex;
                Assert( FVar() );
                break;
            }
            break;
            
        case fidtypTagged:
            switch ( wIndex )
            {
            case fidlimMax:
                m_fidVal = _fidTaggedMaxVal;
                break;

            case fidlimNone:
                m_fidVal = _fidTaggedNoneVal;
                break;

            default:
                m_fidVal = _fidTaggedMinVal + wIndex;
                Assert( FTagged() );
            }
            break;

#ifdef DEBUG
        case fidtypUnknown:
            m_fidVal = wIndex;
            break;
#endif
            
        default:
            AssertSz( fFalse, "Unknown fid type." );
            break;
        }
    }
            
    INLINE operator _FID() const
    {
        return m_fidVal;
    }

    INLINE FIDTYP Fidtyp() const
    {
        if ( FFixed() )
        {
            return fidtypFixed;
        }

        if ( FVar() )
        {
            return fidtypVar;
        }

        if ( FTagged() )
        {
            return fidtypTagged;
        }

        AssertSz( fFalse, "Unknown fid type." );
        return fidtypUnknown;
    }
    
    INLINE BOOL FValid( ) const
    {
        if ( ( _fidFixedNoneVal <= m_fidVal ) && ( m_fidVal <= _fidFixedNoneFullVal ) )
        {
            return fTrue;
        }
        
        if ( ( _fidVarNoneVal <= m_fidVal ) && ( m_fidVal <= _fidVarNoneFullVal ) )
        {
            return fTrue;
        }
        
        if ( ( _fidTaggedNoneVal <= m_fidVal ) && ( m_fidVal <= _fidTaggedNoneFullVal ) )
        {
            return fTrue;
        }
        
        return fFalse;
    }
    
    INLINE BOOL FFixed( ) const
    {
        Assert( FValid() );
        return ( ( _fidFixedMinVal <= m_fidVal ) && ( m_fidVal <= _fidFixedMaxVal ) );
    }

    INLINE BOOL FFixedNone( ) const
    {
        Assert( FValid() );
        return ( m_fidVal == _fidFixedNoneVal );
    }

    INLINE BOOL FFixedMax( ) const
    {
        Assert( FValid() );
        return ( m_fidVal == _fidFixedMaxVal );
    }
            
    INLINE BOOL FFixedNoneFull( ) const
    {
        Assert( FValid() );
        return ( m_fidVal == _fidFixedNoneFullVal );
    }
    
    INLINE BOOL FVar( ) const
    {
        Assert( FValid() );
        return ( m_fidVal ^ _fidVarMinVal ) < _fidVarMinVal;
    }

    INLINE BOOL FVarNone( ) const
    {
        Assert( FValid() );
        return ( m_fidVal == _fidVarNoneVal );
    }
    
    INLINE BOOL FVarMax( ) const
    {
        Assert( FValid() );
        return ( m_fidVal == _fidVarMaxVal );
    }
            
    INLINE BOOL FVarNoneFull( ) const
    {
        Assert( FValid() );
        return ( m_fidVal == _fidVarNoneFullVal );
    }
    
    INLINE BOOL FTagged( ) const
    {
        Assert( FValid() );
        return ( ( _fidTaggedMinVal <= m_fidVal ) && ( m_fidVal <= _fidTaggedMaxVal ) );
    }

    INLINE BOOL FTaggedNone( ) const
    {
        Assert( FValid() );
        return ( m_fidVal == _fidTaggedNoneVal );
    }

    INLINE BOOL FTaggedMax( ) const
    {
        Assert( FValid() );
        return ( m_fidVal == _fidTaggedMaxVal );
    }
            
    INLINE BOOL FTaggedNoneFull( ) const
    {
        Assert( FValid() );
        return ( m_fidVal == _fidTaggedNoneFullVal );
    }
    
    INLINE INT Index( FIDTYP fidtypType ) const
    {
        switch ( fidtypType )
        {
        case fidtypFixed:
            Assert( FFixed() );
            return ( m_fidVal - _fidFixedMinVal );

        case fidtypVar:
            Assert( FVar() );
            return ( m_fidVal - _fidVarMinVal );

        case fidtypTagged:
            Assert( FTagged() );
            return ( m_fidVal - _fidTaggedMinVal );

        default:
            AssertSz( fFalse, "Unknown fid type." );
            return -1;
        }
    }

    INLINE INT Index( const FID& fidBase ) const
    {
#ifdef DEBUG
        switch ( fidBase.m_fidVal )
        {
        case _fidTaggedNoneVal:
            Assert( FTaggedNone() || FTagged() );
            break;

        case _fidVarNoneVal:
            Assert( FVarNone() || FVar() );
            break;

        case _fidFixedNoneVal:
            Assert( FFixedNone() || FFixed() );
            break;

        default:
            Assert( Fidtyp() == fidBase.Fidtyp() );
            break;
        }
#endif
        Assert( m_fidVal >= fidBase.m_fidVal );
        return ( m_fidVal - fidBase.m_fidVal );
    }
    
};
static_assert( sizeof(_FID) == sizeof(FID), "FID is just a wrapper around a _FID to monitor its use.");

class FID_ITERATOR
{
private:
    FID m_fidFirst;
    FID m_fidLast;
    FID m_fidCurrent;
    BOOL m_fEnd;
    
public:
    FID_ITERATOR( const FID fidFirst, const FID fidLast )
    {
        Expected( ( fidFirst <= fidLast ) || ( fidFirst.FTaggedNoneFull() ? fidLast.FTaggedMax() : fTrue ) );
        Expected( ( fidFirst <= fidLast ) || ( fidFirst.FVarNoneFull() ? fidLast.FVarMax() : fTrue ) );
        Expected( ( fidFirst <= fidLast ) || ( fidFirst.FFixedNoneFull() ? fidLast.FFixedMax() : fTrue ) );
        Expected( ( fidFirst > fidLast )  || ( fidFirst.Fidtyp() == fidLast.Fidtyp() ) );
        
        m_fidFirst = fidFirst;
        m_fidLast = fidLast;
        m_fidCurrent = fidFirst;
        if ( m_fidFirst > m_fidLast )
        {
            m_fEnd = fTrue;
        }
        else
        {
            m_fEnd = fFalse;
        }
    };

    INLINE operator const FID*() const
    {
        if ( m_fEnd )
        {
            return NULL;
        }
        
        return &m_fidCurrent;
    }

    INLINE BOOL FEnd() const
    {
        return m_fEnd;
    }

    INLINE VOID Advance( const WORD wDistance )
    {
        if ( m_fEnd )
        {
            Assert( m_fidCurrent == max( m_fidFirst, m_fidLast ) );
            return;
        }

        Assert( m_fidLast >= m_fidFirst);
        
        if ( ( m_fidCurrent + wDistance) > m_fidLast )
        {
            m_fidCurrent = m_fidLast;
            m_fEnd = fTrue;
            return;
        }

        m_fidCurrent += wDistance;
    }
    
    INLINE FID_ITERATOR& operator++()
    {
        Advance( 1 );
        return *this;
    }
    
    INLINE FID_ITERATOR operator++(int)
    {
        FID_ITERATOR itfidTemp = *this;
        Advance( 1 );
        return itfidTemp;
    }

};

const _FID fidFixedLeast     = FID::_fidFixedMinVal;
const _FID fidVarLeast       = FID::_fidVarMinVal;
const _FID fidTaggedLeast    = FID::_fidTaggedMinVal;
const _FID fidFixedMost      = FID::_fidFixedMaxVal;
const _FID fidVarMost        = FID::_fidVarMaxVal;
const _FID fidTaggedMost     = FID::_fidTaggedMaxVal;

const _FID fidMin            = 1;
const _FID fidMax            = fidTaggedMost;

INLINE BOOL FFixedFid( const FID fid )
{
    return fid.FFixed();
}

INLINE BOOL FVarFid( const FID fid )
{
    return fid.FVar();
}

INLINE BOOL FTaggedFid( const FID fid )
{
    return fid.FTagged();
}

INLINE INT IbFromFid ( FID fid )
{
    INT ib;
    if ( FFixedFid( fid ) )
    {
        ib = ((fid - fidFixedLeast) % cbitFixed) / 8;
    }
    else if ( FVarFid( fid ) )
    {
        ib = (((fid - fidVarLeast) % cbitVariable) + cbitFixed) / 8;
    }
    else
    {
        Assert( FTaggedFid( fid ) );
        ib = (((fid - fidTaggedLeast) % cbitTagged) + cbitFixedVariable) / 8;
    }
    Assert( ib >= 0 && ib < 32 );
    return ib;
}

INLINE INT IbitFromFid ( FID fid )
{
    INT ibit;
    if ( FFixedFid( fid ) )
    {
        ibit =  1 << ((fid - fidFixedLeast) % 8 );
    }
    else if ( FVarFid( fid ) )
    {
        ibit =  1 << ((fid - fidVarLeast) % 8);
    }
    else
    {
        Assert( FTaggedFid( fid ) );
        ibit =  1 << ((fid - fidTaggedLeast) % 8);
    }
    return ibit;
}

struct TCIB
{
    FID fidFixedLast;
    FID fidVarLast;
    FID fidTaggedLast;
};

const BYTE  fIDXSEGTemplateColumn       = 0x80;
const BYTE  fIDXSEGDescending           = 0x40;
const BYTE  fIDXSEGMustBeNull           = 0x20;

typedef JET_COLUMNID    COLUMNID;

const COLUMNID      fCOLUMNIDTemplate   = 0x80000000;

INLINE BOOL FCOLUMNIDValid( const COLUMNID columnid )
{
    if ( ( columnid & ~fCOLUMNIDTemplate ) & 0xFFFF0000 )
        return fFalse;

    if ( (_FID)columnid < fidMin || (_FID)columnid > fidMax )
        return fFalse;

    return fTrue;
}

INLINE BOOL FCOLUMNIDTemplateColumn( const COLUMNID columnid )
{
    Assert( FCOLUMNIDValid( columnid ) );
    return !!( columnid & fCOLUMNIDTemplate );
}
INLINE VOID COLUMNIDSetFTemplateColumn( COLUMNID& columnid )
{
    Assert( FCOLUMNIDValid( columnid ) );
    columnid |= fCOLUMNIDTemplate;
}
INLINE VOID COLUMNIDResetFTemplateColumn( COLUMNID& columnid )
{
    Assert( FCOLUMNIDValid( columnid ) );
    columnid &= ~fCOLUMNIDTemplate;
}

INLINE BOOL FCOLUMNIDFixed( const COLUMNID columnid )
{
    Assert( FCOLUMNIDValid( columnid ) );
    return FFixedFid( (_FID)columnid );
}

INLINE BOOL FCOLUMNIDVar( const COLUMNID columnid )
{
    Assert( FCOLUMNIDValid( columnid ) );
    return FVarFid( (_FID)columnid );
}

INLINE BOOL FCOLUMNIDTagged( const COLUMNID columnid )
{
    Assert( FCOLUMNIDValid( columnid ) );
    return FTaggedFid( (_FID)columnid );
}

INLINE COLUMNID ColumnidOfFid( const FID fid, const BOOL fTemplateColumn )
{
    COLUMNID    columnid    = fid;

    if ( fTemplateColumn )
        COLUMNIDSetFTemplateColumn( columnid );

    return columnid;
}

INLINE FID FidOfColumnid( const COLUMNID columnid )
{
    return (_FID)columnid;
}


typedef SHORT IDXSEG_OLD;

class IDXSEG;

PERSISTED
class LE_IDXSEG
{
    public:
        UnalignedLittleEndian< BYTE >   bFlags;
        UnalignedLittleEndian< BYTE >   bReserved;
        UnalignedLittleEndian< FID >    fid;

        VOID operator=( const IDXSEG &idxseg );
};

class IDXSEG
{
    public:
        INLINE VOID operator=( const LE_IDXSEG &le_idxseg )
        {
            m_bFlags    = le_idxseg.bFlags;
            bReserved   = 0;
            m_fid       = le_idxseg.fid;
        }

    private:
        BYTE        m_bFlags;
        BYTE        bReserved;
        FID         m_fid;

    public:
        INLINE FID Fid() const                          { return m_fid; }
        INLINE VOID SetFid( const FID fid )             { m_fid = fid; }

        INLINE BYTE FFlags() const                      { return m_bFlags; }
        INLINE VOID ResetFlags()                        { m_bFlags = 0; bReserved = 0; }

        INLINE BOOL FTemplateColumn() const             { return ( m_bFlags & fIDXSEGTemplateColumn ); }
        INLINE VOID SetFTemplateColumn()                { m_bFlags |= fIDXSEGTemplateColumn; }
        INLINE VOID ResetFTemplateColumn()              { m_bFlags &= ~fIDXSEGTemplateColumn; }

        INLINE BOOL FDescending() const                 { return ( m_bFlags & fIDXSEGDescending ); }
        INLINE VOID SetFDescending()                    { m_bFlags |= fIDXSEGDescending; }
        INLINE VOID ResetFDescending()                  { m_bFlags &= ~fIDXSEGDescending; }

        INLINE BOOL FMustBeNull() const                 { return ( m_bFlags & fIDXSEGMustBeNull ); }
        INLINE VOID SetFMustBeNull()                    { m_bFlags |= fIDXSEGMustBeNull; }
        INLINE VOID ResetFMustBeNull()                  { m_bFlags &= ~fIDXSEGMustBeNull; }

        INLINE COLUMNID Columnid() const
        {
            return ColumnidOfFid( Fid(), FTemplateColumn() );
        }
        INLINE VOID SetColumnid( const COLUMNID columnid )
        {
            if ( FCOLUMNIDTemplateColumn( columnid ) )
                SetFTemplateColumn();

            SetFid( FidOfColumnid( columnid ) );
        }

        INLINE BOOL FIsEqual( const COLUMNID columnid ) const
        {
            return ( Fid() == FidOfColumnid( columnid )
                && ( ( FTemplateColumn() && FCOLUMNIDTemplateColumn( columnid ) )
                    || ( !FTemplateColumn() && !FCOLUMNIDTemplateColumn( columnid ) ) ) );
        }
};


INLINE VOID LE_IDXSEG::operator=( const IDXSEG &idxseg )
{
    bFlags      = idxseg.FFlags();
    bReserved   = 0;
    fid         = idxseg.Fid();
}

typedef UINT                IFMP;

DBID DbidFromIfmp( const IFMP ifmp );

typedef JET_LS              LSTORE;

enum ENTERCRIT { enterCriticalSection, dontEnterCriticalSection };


class LGPOS;

PERSISTED
class LE_LGPOS
{
    public:
        UnalignedLittleEndian<USHORT>   le_ib;
        UnalignedLittleEndian<USHORT>   le_isec;
        UnalignedLittleEndian<LONG>     le_lGeneration;

    public:
        operator LGPOS() const;
};

class LGPOS
{
    public:
        union
        {
            struct
            {
                USHORT  ib;
                USHORT  isec;
                LONG    lGeneration;
            };
            QWORD qw;
        };

    public:
        operator LE_LGPOS() const;

        INLINE void SetByIbOffset( const QWORD ibOffset, const ULONG cSec, const ULONG cbSec )
        {
            Assert( cSec != 0 && cbSec != 0 );
            ib          = USHORT( ( ibOffset % ( cSec * cbSec ) ) % cbSec );
            isec        = USHORT( ( ibOffset % ( cSec * cbSec ) ) / cbSec );
            lGeneration = LONG( ibOffset / ( cSec * cbSec ) );
        }

        INLINE QWORD IbOffset( const ULONG cSec, const ULONG cbSec ) const
        {
            Assert( cSec != 0 && cbSec != 0 );
            return  QWORD( ib ) +
                    QWORD( isec ) * cbSec +
                    QWORD( lGeneration ) * cSec * cbSec;
        }

        INLINE LGPOS LgposAtomicRead() const
        {
            LGPOS lgpos;
            lgpos.qw = (QWORD)AtomicRead( (__int64*)&qw );
            return lgpos;
        }

        INLINE LGPOS LgposAtomicExchange( const LGPOS& lgpos )
        {
            LGPOS lgposOld;
            lgposOld.qw = (QWORD)AtomicExchange( (__int64*)&qw, (__int64)lgpos.qw );
            return lgposOld;
        }

        INLINE LGPOS LgposAtomicExchangeMax( const LGPOS& lgpos )
        {
            static_assert( fHostIsLittleEndian, "Please, implement big-endian LgposAtomicExchangeMax." );

            LGPOS lgposOld;
            lgposOld.qw = (QWORD)AtomicExchangeMax( (__int64*)&qw, (__int64)lgpos.qw );
            return lgposOld;
        }
};

INLINE LE_LGPOS::operator LGPOS() const
{
    LGPOS lgposT;

    lgposT.ib           = le_ib;
    lgposT.isec         = le_isec;
    lgposT.lGeneration  = le_lGeneration;

    return lgposT;
}

INLINE LGPOS::operator LE_LGPOS() const
{
    LE_LGPOS le_lgposT;

    le_lgposT.le_ib             = ib;
    le_lgposT.le_isec           = isec;
    le_lgposT.le_lGeneration    = lGeneration;

    return le_lgposT;
}

extern const LGPOS lgposMax;
extern const LGPOS lgposMin;

class RBS_POS;

PERSISTED class LE_RBSPOS
{
public:
    UnalignedLittleEndian<LONG> le_iSegment;
    UnalignedLittleEndian<LONG> le_lGeneration;

public:
    operator RBS_POS() const;
};

class RBS_POS
{
public:
    union
    {
        struct
        {
            LONG    iSegment;
            LONG    lGeneration;
        };
        QWORD qw;
    };

    operator LE_RBSPOS() const;
};

INLINE LE_RBSPOS::operator RBS_POS() const
{
    RBS_POS rbsposT;

    rbsposT.iSegment    = le_iSegment;
    rbsposT.lGeneration = le_lGeneration;

    return rbsposT;
}

INLINE RBS_POS::operator LE_RBSPOS() const
{
    LE_RBSPOS le_rbsposT;

    le_rbsposT.le_iSegment       = iSegment;
    le_rbsposT.le_lGeneration    = lGeneration;

    return le_rbsposT;
}

extern const RBS_POS rbsposMin;

enum RECOVERING_MODE { fRecoveringNone,
                       fRecoveringRedo,
                       fRecoveringUndo,
                       fRestorePatch,
                       fRestoreRedo };

#define wszDefaultTempDbFileName        L"tmp"
#define wszDefaultTempDbExt         L".edb"

extern BOOL g_fEseutil;

extern DWORD g_dwGlobalMajorVersion;
extern DWORD g_dwGlobalMinorVersion;
extern DWORD g_dwGlobalBuildNumber;
extern LONG g_lGlobalSPNumber;

extern BOOL g_fPeriodicTrimEnabled;


const LONG  g_cbPageDefault = 1024 * 4;
const LONG  g_cbPageMin     = 1024 * 4;
const LONG  g_cbPageMax     = 1024 * 32;
const LONG  cbPageOld       = 4096;





PERSISTED const CHAR chRECFixedColAndKeyFill                = ' ';
PERSISTED const WCHAR wchRECFixedColAndKeyFill              = L' ';
PERSISTED const CHAR chRECFill                              = '*';

PERSISTED const CHAR chPAGEReplaceFill                      = 'R';
PERSISTED const CHAR chPAGEDeleteFill                       = 'D';
PERSISTED const CHAR chPAGEReorgContiguousFreeFill          = 'O';
PERSISTED const CHAR chPAGEReHydrateFill                    = 'H';

PERSISTED const CHAR chSCRUBLegacyDeletedDataFill           = 'd';
PERSISTED const CHAR chSCRUBLegacyLVChunkFill               = 'l';
PERSISTED const CHAR chSCRUBLegacyUsedPageFreeFill          = 'z';
PERSISTED const CHAR chSCRUBLegacyUnusedPageFill            = 'u';

PERSISTED const CHAR chSCRUBDBMaintDeletedDataFill          = 'D';
PERSISTED const CHAR chSCRUBDBMaintLVChunkFill              = 'L';
PERSISTED const CHAR chSCRUBDBMaintUsedPageFreeFill         = 'Z';
PERSISTED const CHAR chSCRUBDBMaintUnusedPageFill           = 'U';
PERSISTED const CHAR chSCRUBDBMaintEmptyPageLastNodeFill    = 0x00;



const BYTE bCRESAllocFill                                   = (BYTE)0xaa;
const BYTE bCRESFreeFill                                    = (BYTE)0xee;

const LONG_PTR  lBitsAllFlipped                             = ~( (LONG_PTR)0 );


const CHAR chPAGETestPageFill                               = 'T';
const CHAR chPAGETestUsedPageFreeFill                       = 'X';
const CHAR chRECCompressTestFill                            = 'C';
const CHAR chDATASERIALIZETestFill                          = 'X';


const INT NO_GRBIT          = 0;
const ULONG NO_ITAG         = 0;

typedef __int64         _DBTIME;
#define DBTIME          _DBTIME
const DBTIME dbtimeNil = 0xffffffffffffffff;
const DBTIME dbtimeInvalid = 0xfffffffffffffffe;
const DBTIME dbtimeShrunk = 0xfffffffffffffffd;
const DBTIME dbtimeRevert = 0xfffffffffffffffc;
const DBTIME dbtimeUndone = dbtimeNil;
const DBTIME dbtimeStart = 0x40;

typedef ULONG       TRX;
const TRX trxMin    = 0;
const TRX trxMax    = 0xffffffff;

const TRX trxPrecommit  = trxMax - 0x10;

#define BoolSetFlag( flags, flag )      ( flag == ( flags & flag ) )


enum PageNodeBoundsChecking
{
    pgnbcChecked    = 1,
    pgnbcNoChecks   = 2,
};


typedef UINT FLAGS;


class DATA
{
    public:
        VOID    *Pv         ()                          const;
        INT     Cb          ()                          const;
        INT     FNull       ()                          const;
        VOID    CopyInto    ( DATA& dataDest )          const;

        VOID    SetPv       ( VOID * pv );
        VOID    SetCb       ( const SIZE_T cb );
        VOID    DeltaPv     ( INT_PTR i );
        VOID    DeltaCb     ( INT i );
        VOID    Nullify     ();

#ifdef DEBUG
    public:
                DATA        ();
                ~DATA       ();
        VOID    Invalidate  ();

        VOID    AssertValid ( BOOL fAllowNullPv = fFalse ) const;
#endif

        BOOL    operator==  ( const DATA& )             const;

    private:
        VOID    *m_pv;
        ULONG   m_cb;
};


INLINE VOID * DATA::Pv() const
{
    return m_pv;
}

INLINE INT DATA::Cb() const
{
    Assert( m_cb <= JET_cbLVColumnMost || FNegTest( fCorruptingPageLogically ) );
    return m_cb;
}

INLINE VOID DATA::SetPv( VOID * pvNew )
{
    m_pv = pvNew;
}

INLINE VOID DATA::SetCb( const SIZE_T cb )
{
    Assert( cb <= JET_cbLVColumnMost || FNegTest( fCorruptingPageLogically ) );
    m_cb = (ULONG)cb;
}

INLINE VOID DATA::DeltaPv( INT_PTR i )
{
    m_pv = (BYTE *)m_pv + i;
}

INLINE VOID DATA::DeltaCb( INT i )
{
    OnDebug( const ULONG cb = m_cb );
    m_cb += i;
    Assert( ( ( i >= 0 ) && ( m_cb >= cb ) ) ||
            ( ( i < 0 ) && ( m_cb < cb ) ) );
}


INLINE INT DATA::FNull() const
{
    BOOL fNull = ( 0 == m_cb );
    return fNull;
}


INLINE VOID DATA::CopyInto( DATA& dataDest ) const
{
    dataDest.SetCb( m_cb );
    UtilMemCpy( dataDest.Pv(), m_pv, m_cb );
}


INLINE VOID DATA::Nullify()
{
    m_pv = 0;
    m_cb = 0;
}


#ifdef DEBUG


INLINE DATA::DATA() :
    m_pv( (VOID *)lBitsAllFlipped ),
    m_cb( INT_MAX )
{
}


INLINE DATA::~DATA()
{
    Invalidate();
}


INLINE VOID DATA::Invalidate()
{
    m_pv = (VOID *)lBitsAllFlipped;
    m_cb = INT_MAX;
}


INLINE VOID DATA::AssertValid( BOOL fAllowNullPv ) const
{
    Assert( (VOID *)lBitsAllFlipped != m_pv );
    Assert( INT_MAX != m_cb );
    Assert( 0 == m_cb || NULL != m_pv || fAllowNullPv );
}


#endif

INLINE BOOL DATA::operator==( const DATA& data ) const
{
    const BOOL  fEqual = ( m_pv == data.Pv() && m_cb == (ULONG)data.Cb() );
    return fEqual;
}

#define cbKeyMostMost               JET_cbKeyMost32KBytePage
#define cbLimitKeyMostMost          ( cbKeyMostMost + 1 )

#define cbKeyAlloc                  cbLimitKeyMostMost

#define cbBookmarkMost              ( 2 * cbKeyMostMost )
#define cbBookmarkAlloc             cbBookmarkMost



class KEY
{
    public:
        DATA    prefix;
        DATA    suffix;

    public:
        INT     Cb              ()                  const;
        BOOL    FNull           ()                  const;
        VOID    CopyIntoBuffer  ( _Out_writes_(cbMax) VOID * pvDest, INT cbMax = INT_MAX )  const;

        VOID    Advance     ( INT cb );
        VOID    Nullify     ();

#ifdef DEBUG
    public:
        VOID    Invalidate  ();

        VOID    AssertValid () const;
#endif

        BOOL    operator==  ( const KEY& )          const;

    public:
        static USHORT   CbLimitKeyMost( const USHORT usT );
};


INLINE INT KEY::Cb() const
{
    INT cb = prefix.Cb() + suffix.Cb();
    return cb;
}


INLINE INT KEY::FNull() const
{
    BOOL fNull = prefix.FNull() && suffix.FNull();
    return fNull;
}


INLINE VOID KEY::CopyIntoBuffer(
    _Out_writes_(cbMax) VOID * pvDest,
    INT cbMax ) const
{
    Assert( NULL != pvDest || 0 == cbMax );

    Assert( prefix.Cb() >= 0 );
    Assert( suffix.Cb() >= 0 );
    INT cbPrefix    = max( 0, prefix.Cb() );
    INT cbSuffix    = max( 0, suffix.Cb() );

    UtilMemCpy( pvDest, prefix.Pv(), min( cbPrefix, cbMax ) );

    INT cbCopy = max ( 0, min( cbSuffix, cbMax - cbPrefix ) );
    UtilMemCpy( reinterpret_cast<BYTE *>( pvDest )+cbPrefix, suffix.Pv(), cbCopy );
}


INLINE VOID KEY::Advance( INT cb )
{
    __assume_bound( cb );
    Assert( cb >= 0 );
    Assert( cb <= Cb() );
    if ( cb < 0 )
    {
    }
    else if ( cb < prefix.Cb() )
    {
        prefix.SetPv( (BYTE *)prefix.Pv() + cb );
        prefix.SetCb( prefix.Cb() - cb );
    }
    else
    {
        INT cbPrefix = prefix.Cb();
        prefix.Nullify();
        suffix.SetPv( (BYTE *)suffix.Pv() + ( cb - cbPrefix ) );
        suffix.SetCb( suffix.Cb() - ( cb - cbPrefix ) );
    }
}


INLINE VOID KEY::Nullify()
{
    prefix.Nullify();
    suffix.Nullify();
}


INLINE USHORT KEY::CbLimitKeyMost( const USHORT usT )
{
    return USHORT( usT + 1 );
}


#ifdef DEBUG

INLINE VOID KEY::Invalidate()
{
    prefix.Invalidate();
    suffix.Invalidate();
}


INLINE VOID KEY::AssertValid() const
{
    Assert( prefix.Cb() == 0 || prefix.Cb() > sizeof( USHORT ) || FNegTest( fCorruptingPageLogically ) );
    ASSERT_VALID( &prefix );
    ASSERT_VALID( &suffix );
}


#endif


INLINE BOOL KEY::operator==( const KEY& key ) const
{
    BOOL fEqual = prefix == key.prefix
            && suffix == key.suffix
            ;
    return fEqual;
}


struct LE_KEYLEN
{
    LittleEndian<USHORT>    le_cbPrefix;
    LittleEndian<USHORT>    le_cbSuffix;
};


class LINE
{
    public:
        VOID    *pv;
        ULONG   cb;
        FLAGS   fFlags;
};


class BOOKMARK
{
    public:
        KEY     key;
        DATA    data;

        VOID    Nullify     ();
#ifdef DEBUG
    public:
        VOID    Invalidate  ();
        VOID    AssertValid () const;
#endif
};


INLINE VOID BOOKMARK::Nullify()
{
    key.Nullify();
    data.Nullify();
}

#ifdef DEBUG

INLINE VOID BOOKMARK::Invalidate()
{
    key.Invalidate();
    data.Invalidate();
}


INLINE VOID BOOKMARK::AssertValid() const
{
    ASSERT_VALID( &key );
    ASSERT_VALID( &data );
}


#endif


class BOOKMARK_COPY : public BOOKMARK
{
    public:
        BOOKMARK_COPY();
        ~BOOKMARK_COPY();
        ERR ErrCopyKey( const KEY& keySrc );
        ERR ErrCopyKeyData( const KEY& keySrc, const DATA& dataSrc );
        VOID FreeCopy();

    private:
        BYTE* m_pb;
};

INLINE BOOKMARK_COPY::BOOKMARK_COPY()
{
    m_pb = NULL;
    OnDebug( Invalidate() );
}

INLINE BOOKMARK_COPY::~BOOKMARK_COPY()
{
    FreeCopy();
}

INLINE ERR BOOKMARK_COPY::ErrCopyKey( const KEY& keySrc )
{
    DATA dataSrc;
    dataSrc.Nullify();
    return ErrCopyKeyData( keySrc, dataSrc );
}

INLINE ERR BOOKMARK_COPY::ErrCopyKeyData( const KEY& keySrc, const DATA& dataSrc )
{
    ERR err = JET_errSuccess;
    ASSERT_VALID( &keySrc );
    ASSERT_VALID( &dataSrc );
    Assert( m_pb == NULL );

    if ( keySrc.FNull() && dataSrc.FNull() )
    {
        Nullify();
        goto HandleError;
    }

    const DWORD_PTR cb = keySrc.Cb() + dataSrc.Cb();
    DWORD_PTR cbMax = 0;
    CallS( RESBOOKMARK.ErrGetParam( JET_resoperSize, &cbMax ) );
    if ( cb > cbMax )
    {
        Assert( fFalse );
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    Alloc( m_pb = (BYTE *)RESBOOKMARK.PvRESAlloc() );
    Nullify();
    BYTE* pb = m_pb;

    if ( !keySrc.prefix.FNull() )
    {
        key.prefix.SetPv( pb );
        key.prefix.SetCb( keySrc.prefix.Cb() );
        UtilMemCpy( pb, keySrc.prefix.Pv(), key.prefix.Cb() );
        pb += key.prefix.Cb();
    }

    if ( !keySrc.suffix.FNull() )
    {
        key.suffix.SetPv( pb );
        key.suffix.SetCb( keySrc.suffix.Cb() );
        UtilMemCpy( pb, keySrc.suffix.Pv(), key.suffix.Cb() );
        pb += key.suffix.Cb();
    }

    if ( !dataSrc.FNull() )
    {
        data.SetPv( pb );
        data.SetCb( dataSrc.Cb() );
        UtilMemCpy( pb, dataSrc.Pv(), data.Cb() );
        pb += data.Cb();
    }

    Assert( (DWORD_PTR)( pb - m_pb ) == cb );
    ASSERT_VALID( this );

HandleError:
    return err;
}

INLINE VOID BOOKMARK_COPY::FreeCopy()
{
    OnDebug( Invalidate() );
    if ( m_pb == NULL )
    {
        return;
    }

    RESBOOKMARK.Free( m_pb );
    m_pb = NULL;
}


class KEYDATAFLAGS
{
    public:
        KEY     key;
        DATA    data;
        FLAGS   fFlags;

        VOID    Nullify     ();

#ifdef DEBUG
    public:
        VOID    Invalidate  ();

        VOID    AssertValid () const;

        BOOL    operator == ( const KEYDATAFLAGS& ) const;
#endif
};


INLINE VOID KEYDATAFLAGS::Nullify()
{
    key.Nullify();
    data.Nullify();
    fFlags = 0;
}


#ifdef DEBUG

INLINE VOID KEYDATAFLAGS::Invalidate()
{
    key.Invalidate();
    data.Invalidate();
    fFlags = 0;
}


INLINE BOOL KEYDATAFLAGS::operator==( const KEYDATAFLAGS& kdf ) const
{
    BOOL fEqual = fFlags == kdf.fFlags
            && key == kdf.key
            && data == kdf.data
            ;
    return fEqual;
}


INLINE VOID KEYDATAFLAGS::AssertValid() const
{
    ASSERT_VALID( &key );
    ASSERT_VALID( &data );
}


#endif


INLINE INT CmpData( const DATA& data1, const DATA& data2 )
{
    const INT   db  = data1.Cb() - data2.Cb();
    const INT   cmp = memcmp( data1.Pv(), data2.Pv(), db < 0 ? data1.Cb() : data2.Cb() );

    return ( 0 == cmp ? db : cmp );
}

INLINE INT CmpData( const VOID * const pv1, const INT cb1, const VOID * const pv2, const INT cb2 )
{
    const INT   db  = cb1 - cb2;
    const INT   cmp = memcmp( pv1, pv2, db < 0 ? cb1 : cb2 );

    return ( 0 == cmp ? db : cmp );
}


INLINE BOOL FDataEqual( const DATA& data1, const DATA& data2 )
{
    if( data1.Cb() == data2.Cb() )
    {
        return( 0 == memcmp( data1.Pv(), data2.Pv(), data1.Cb() ) );
    }
    return fFalse;
}

INLINE BOOL FDataEqual( const VOID * const pv1, const INT cb1, const VOID * const pv2, const INT cb2 )
{
    if ( cb1 == cb2 )
    {
        return ( 0 == memcmp( pv1, pv2, cb1 ) );
    }
    return fFalse;
}


INLINE INT CmpKeyShortest( const KEY& key1, const KEY& key2 )
{
    const KEY * pkeySmallestPrefix;
    const KEY * pkeyLargestPrefix;
    BOOL        fReversed;
    if ( key1.prefix.Cb() < key2.prefix.Cb() )
    {
        fReversed           = fFalse;
        pkeySmallestPrefix  = &key1;
        pkeyLargestPrefix   = &key2;
    }
    else
    {
        fReversed           = fTrue;
        pkeySmallestPrefix  = &key2;
        pkeyLargestPrefix   = &key1;
    }
    Assert( pkeySmallestPrefix && pkeyLargestPrefix );

    const BYTE  *pb1            = (BYTE *)pkeySmallestPrefix->prefix.Pv();
    const BYTE  *pb2            = (BYTE *)pkeyLargestPrefix->prefix.Pv();
    INT         cbCompare       = pkeySmallestPrefix->prefix.Cb();
    INT         cmp             = 0;

    if ( pb1 == pb2 || ( cmp = memcmp( pb1, pb2, cbCompare)) == 0 )
    {
        pb1             = (BYTE *)pkeySmallestPrefix->suffix.Pv();
        pb2             += cbCompare;
        cbCompare       = min(  pkeyLargestPrefix->prefix.Cb() - cbCompare,
                                pkeySmallestPrefix->suffix.Cb() );
        cmp             = memcmp( pb1, pb2, cbCompare );

        if ( 0 == cmp )
        {
            pb1             += cbCompare;
            pb2             = (BYTE *)pkeyLargestPrefix->suffix.Pv();
            cbCompare       = min(  pkeySmallestPrefix->suffix.Cb() - cbCompare,
                                    pkeyLargestPrefix->suffix.Cb() );
            cmp             = memcmp( pb1, pb2, cbCompare );
        }
    }

    cmp = fReversed ? -cmp : cmp;
    return cmp;
}


INLINE INT CmpKey( const KEY& key1, const KEY& key2 )
{
    INT cmp = CmpKeyShortest( key1, key2 );
    if ( 0 == cmp )
    {
        cmp = key1.Cb() - key2.Cb();
    }
    return cmp;
}


INLINE INT CmpKey( const void * pvkey1, ULONG cbkey1, const void * pvkey2, ULONG cbkey2 )
{
    KEY key1;
    KEY key2;

    key1.prefix.Nullify();
    key1.suffix.SetPv( const_cast<void *>( pvkey1 ) );
    key1.suffix.SetCb( cbkey1 );

    key2.prefix.Nullify();
    key2.suffix.SetPv( const_cast<void *>( pvkey2 ) );
    key2.suffix.SetCb( cbkey2 );

    return CmpKey( key1, key2 );
}


INLINE BOOL FKeysEqual( const KEY& key1, const KEY& key2 )
{
    if( key1.Cb() == key2.Cb() )
    {
        return ( 0 == CmpKeyShortest( key1, key2 ) );
    }
    return fFalse;
}


template < class KEYDATA1, class KEYDATA2 >
INLINE INT CmpKeyData( const KEYDATA1& kd1, const KEYDATA2& kd2, BOOL * pfKeyEqual = 0 )
{
    INT cmp = CmpKey( kd1.key, kd2.key );
    if ( pfKeyEqual )
    {
        *pfKeyEqual = cmp;
    }
    if ( 0 == cmp )
    {
        cmp = CmpData( kd1.data, kd2.data );
    }
    return cmp;
}


template < class KEYDATA1 >
INLINE INT CmpKeyWithKeyData( const KEY& key, const KEYDATA1& keydata )
{
    KEY keySegment1 = keydata.key;
    KEY keySegment2;

    keySegment2.prefix.Nullify();
    keySegment2.suffix = keydata.data;

    INT cmp = CmpKeyShortest( key, keySegment1 );

    if ( 0 == cmp && key.Cb() > keySegment1.Cb() )
    {
        KEY keyT = key;
        keyT.Advance( keySegment1.Cb() );
        cmp = CmpKeyShortest( keyT, keySegment2 );
    }

    if ( 0 == cmp )
    {
        cmp = ( key.Cb() - ( keydata.key.Cb() + keydata.data.Cb() ) );
    }

    return cmp;
}


INLINE INT  CmpBM( const BOOKMARK& bm1, const BOOKMARK& bm2 )
{
    return CmpKeyData( bm1, bm2 );
}



INLINE ULONG CbCommonData( const DATA& data1, const DATA& data2 )
{
    ULONG   ib;
    ULONG   cbMax = min( data1.Cb(), data2.Cb() );

    for ( ib = 0;
          ib < cbMax &&
              ((BYTE *) data1.Pv() )[ib] == ((BYTE *) data2.Pv() )[ib];
          ib++ )
    {
    }

    return ib;
}

INLINE  ULONG CbCommonKey( const KEY& key1, const KEY& key2 )
{
    INT     ib = 0;
    INT     cbMax = min( key1.Cb(), key2.Cb() );

    const BYTE * pb1 = (BYTE *)key1.prefix.Pv();
    const BYTE * pb2 = (BYTE *)key2.prefix.Pv();

    if ( pb1 == pb2 )
    {
        ib = min( key1.prefix.Cb(), key2.prefix.Cb() );

        pb1 += ib;
        pb2 += ib;
    }

    for ( ; ib < cbMax; ib++ )
    {
        if ( key1.prefix.Cb() == ib )
        {
            pb1 = (BYTE *)key1.suffix.Pv();
        }
        if ( key2.prefix.Cb() == ib )
        {
            pb2 = (BYTE *)key2.suffix.Pv();
        }
        if (*pb1 != *pb2 )
        {
            break;
        }
        ++pb1;
        ++pb2;
    }
    return ib;
}

#define absdiff( x, y ) ( (x) > (y)  ? (x)-(y) : (y)-(x) )

INLINE VOID KeyFromLong( BYTE *rgbKey, ULONG const ul )
{
    *((ULONG*)rgbKey) = ReverseBytesOnLE( ul );
}

INLINE VOID LongFromKey( ULONG *pul, VOID const *rgbKey )
{
    *pul = ReverseBytesOnLE( *(ULONG*)rgbKey );
}

INLINE VOID LongFromKey( ULONG *pul, const KEY& key )
{
    BYTE rgbT[4];
    key.CopyIntoBuffer( rgbT, sizeof( rgbT ) );
    LongFromKey( pul, rgbT );
}

INLINE VOID UnalignedKeyFromLong( __out_bcount(sizeof(ULONG)) BYTE *rgbKey, ULONG const ul )
{
    *(UnalignedBigEndian< ULONG > *)rgbKey = ul;
}

INLINE VOID LongFromUnalignedKey( ULONG *pul, __in_bcount(sizeof(ULONG)) VOID const *rgbKey )
{
    *pul = *(UnalignedBigEndian< ULONG > *)rgbKey;
}



#define forever                 for(;;)



#define VOID            void
#define VDBAPI

enum EseSystemLevel
{
    eDll = 1,
    eOsal,
    eSys,
    eInst,
};

INLINE BOOL FUp( EseSystemLevel esl )
{
    switch ( esl )
    {
        case eDll:
            return FOSDllUp();
        case eOsal:
            return FOSLayerUp();
    }
    AssertSz( fFalse, "Unknown ESE system level" );
    return fFalse;
}


enum sysosrtl
{

    sysosrtlContextInst     = 0x10000000,
    sysosrtlContextFmp      = 0x20000000,

    sysosrtlOsuInitDone     = 101,
    sysosrtlOsuTermStart    = 102,

    sysosrtlDatapoint       = 105,

    sysosrtlInitBegin       = 110,
    sysosrtlInitSucceed     = 111,
    sysosrtlInitFail        = 112,

    sysosrtlSoftStartBegin  = 113,
    sysosrtlSoftStartFail   = 114,

    sysosrtlRedoBegin       = 115,
    sysosrtlUndoBegin       = 116,
    sysosrtlUndoSucceed     = 117,
    sysosrtlRedoUndoFail    = 118,

    sysosrtlTermBegin       = 120,
    sysosrtlTermSucceed     = 121,
    sysosrtlTermFail        = 122,

    sysosrtlInstDelete      = 123,

    sysosrtlSetPfapi        = 125,
    sysosrtlPfapiDelete     = 126,
    
    sysosrtlCreateBegin     = 130,
    sysosrtlCreateSucceed   = 131,
    sysosrtlCreateFail      = 132,

    sysosrtlAttachBegin     = 133,
    sysosrtlAttachSucceed   = 134,
    sysosrtlAttachFail      = 135,

    sysosrtlBfCreateContext = 136,

    sysosrtlDetachBegin     = 140,
    sysosrtlDetachSucceed   = 141,
    sysosrtlDetachFail      = 142,

    sysosrtlBfFlushBegin    = 143,
    sysosrtlBfFlushSucceed  = 144,
    sysosrtlBfFlushFail     = 145,

    sysosrtlBfPurge         = 146,
    sysosrtlBfTerm          = 147,
};

extern CODECONST(VTFNDEF) vtfndefInvalidTableid;
extern CODECONST(VTFNDEF) vtfndefIsam;
extern CODECONST(VTFNDEF) vtfndefIsamMustRollback;
extern CODECONST(VTFNDEF) vtfndefIsamCallback;
extern CODECONST(VTFNDEF) vtfndefTTSortIns;
extern CODECONST(VTFNDEF) vtfndefTTSortRet;
extern CODECONST(VTFNDEF) vtfndefTTBase;
extern CODECONST(VTFNDEF) vtfndefTTBaseMustRollback;

#include    "daeconst.hxx"

typedef enum
{
    fSTInitNotDone      = 0,
    fSTInitInProgress   = 1,
    fSTInitDone             = 2,
    fSTInitFailed       = 3,
    fSTInitMax          = 4
} INST_STINIT;

#include <pshpack1.h>
#define MAX_COMPUTERNAME_LENGTH 15


struct INDEXID
{
    ULONG           cbStruct;
    OBJID           objidFDP;
    FCB             *pfcbIndex;
    PGNO            pgnoFDP;
};


PERSISTED
struct LOGTIME
{
    BYTE    bSeconds;
    BYTE    bMinutes;
    BYTE    bHours;
    BYTE    bDay;
    BYTE    bMonth;
    BYTE    bYear;

    BYTE    fTimeIsUTC:1;
    BYTE    bMillisecondsLow:7;

    BYTE    fReserved:1;
    BYTE    bMillisecondsHigh:3;
    BYTE    fUnused:4;

    USHORT Milliseconds() const
    {
        return ( bMillisecondsLow | ( bMillisecondsHigh << 7 ) );
    }

    VOID SetMilliseconds( INT millisecond )
    {
        bMillisecondsLow = millisecond & 0x7f;
        bMillisecondsHigh = millisecond >> 7;
    }


    BOOL FIsSet() const
    {
        if ( fTimeIsUTC )
        {
            return fTrue;
        }

        const LOGTIME logtimeUnset = { 0 };
        return ( CmpLogTime( *this, logtimeUnset ) != 0 );
    }

    static INT CmpLogTime( const LOGTIME& logtime1, const LOGTIME& logtime2 )
    {
        Assert( !logtime1.fTimeIsUTC == !logtime2.fTimeIsUTC );
        if ( logtime1.bYear != logtime2.bYear )
        {
            return ( (INT)logtime1.bYear - (INT)logtime2.bYear );
        }

        if ( logtime1.bMonth != logtime2.bMonth )
        {
            return ( (INT)logtime1.bMonth - (INT)logtime2.bMonth );
        }

        if ( logtime1.bDay != logtime2.bDay )
        {
            return ( (INT)logtime1.bDay - (INT)logtime2.bDay );
        }

        if ( logtime1.bHours != logtime2.bHours )
        {
            return ( (INT)logtime1.bHours - (INT)logtime2.bHours );
        }

        if ( logtime1.bMinutes != logtime2.bMinutes )
        {
            return ( (INT)logtime1.bMinutes - (INT)logtime2.bMinutes );
        }

        if ( logtime1.bSeconds != logtime2.bSeconds )
        {
            return ( (INT)logtime1.bSeconds - (INT)logtime2.bSeconds );
        }

        if ( logtime1.bMillisecondsHigh != logtime2.bMillisecondsHigh )
        {
            return ( (INT)logtime1.bMillisecondsHigh - (INT)logtime2.bMillisecondsHigh );
        }

        if ( logtime1.bMillisecondsLow != logtime2.bMillisecondsLow )
        {
            return ( (INT)logtime1.bMillisecondsLow - (INT)logtime2.bMillisecondsLow );
        }

        return 0;
    }
};

PERSISTED
struct SIGNATURE
{
    UnalignedLittleEndian< ULONG >  le_ulRandom;
    LOGTIME                         logtimeCreate;
    CHAR                            szComputerName[ MAX_COMPUTERNAME_LENGTH + 1 ];
};

PERSISTED
struct BKINFO
{
    LE_LGPOS                        le_lgposMark;
    LOGTIME                         logtimeMark;
    UnalignedLittleEndian< ULONG >  le_genLow;
    UnalignedLittleEndian< ULONG >  le_genHigh;
};

const ULONG ulDAEMagic              = 0x89abcdef;


const ULONG ulShadowSectorChecksum = 0x5AD05EC7;
const ULONG32 ulLRCKChecksumSeed = 0xFEEDBEAF;
const ULONG32 ulLRCKShortChecksumSeed = 0xDEADC0DE;


struct GenVersion
{
    ULONG   ulVersionMajor;
    ULONG   ulVersionUpdateMajor;
    ULONG   ulVersionUpdateMinor;
};

struct DbVersion
{
    ULONG   ulDbMajorVersion;
    ULONG   ulDbUpdateMajor;
    ULONG   ulDbUpdateMinor;
};

C_ASSERT( OffsetOf( DbVersion, ulDbMajorVersion ) == OffsetOf( GenVersion, ulVersionMajor ) );
C_ASSERT( OffsetOf( DbVersion, ulDbUpdateMajor ) == OffsetOf( GenVersion, ulVersionUpdateMajor ) );
C_ASSERT( OffsetOf( DbVersion, ulDbUpdateMinor ) == OffsetOf( GenVersion, ulVersionUpdateMinor ) );

struct LogVersion
{
    ULONG   ulLGVersionMajor;
    ULONG   ulLGVersionUpdateMajor;
    ULONG   ulLGVersionUpdateMinor;
};

C_ASSERT( OffsetOf( LogVersion, ulLGVersionMajor ) == OffsetOf( GenVersion, ulVersionMajor ) );
C_ASSERT( OffsetOf( LogVersion, ulLGVersionUpdateMajor ) == OffsetOf( GenVersion, ulVersionUpdateMajor ) );
C_ASSERT( OffsetOf( LogVersion, ulLGVersionUpdateMinor ) == OffsetOf( GenVersion, ulVersionUpdateMinor ) );

struct FormatVersions
{
    ULONG       efv;
    DbVersion   dbv;
    LogVersion  lgv;
    GenVersion  fmv;
};


#if defined(DEBUG) || defined(ENABLE_JET_UNIT_TEST)
inline BOOL FMajorVersionMismatch( const INT icmpFromCmpFormatVer )
{
    return 4 == abs( icmpFromCmpFormatVer );
}

inline BOOL FUpdateMajorMismatch( const INT icmpFromCmpFormatVer )
{
    return 2 == abs( icmpFromCmpFormatVer );
}

#endif

inline BOOL FUpdateMinorMismatch( const INT icmpFromCmpFormatVer )
{
    return 1 == abs( icmpFromCmpFormatVer );
}

inline INT CmpFormatVer( const GenVersion * const pver1, const GenVersion * const pver2 )
{
    if ( pver1->ulVersionMajor == pver2->ulVersionMajor )
    {
        if ( pver1->ulVersionUpdateMajor == pver2->ulVersionUpdateMajor )
        {
            if ( pver1->ulVersionUpdateMinor == pver2->ulVersionUpdateMinor )
            {
                return 0;
            }
            else
            {
                return pver1->ulVersionUpdateMinor < pver2->ulVersionUpdateMinor ? -1 : 1;
            }
        }
        else
        {
            return pver1->ulVersionUpdateMajor < pver2->ulVersionUpdateMajor ? -2 : 2;
        }

    }
    else
    {
        return pver1->ulVersionMajor < pver2->ulVersionMajor ? -4 : 4;
    }
}

const JET_ENGINEFORMATVERSION efvExtHdrRootFieldAutoInc = JET_efvExtHdrRootFieldAutoIncStorageReleased;

inline INT CmpLgVer( const LogVersion& ver1, const LogVersion& ver2 )
{
    GenVersion * pver1 = (GenVersion*)&ver1;
    GenVersion * pver2 = (GenVersion*)&ver2;
    C_ASSERT( sizeof(GenVersion) == sizeof(LogVersion) );
    return CmpFormatVer( pver1, pver2 );
}

inline INT CmpDbVer( const DbVersion& ver1, const DbVersion& ver2 )
{
    GenVersion * pver1 = (GenVersion*)&ver1;
    GenVersion * pver2 = (GenVersion*)&ver2;
    C_ASSERT( sizeof(GenVersion) == sizeof(DbVersion) );
    return CmpFormatVer( pver1, pver2 );
}


extern const FormatVersions     g_rgfmtversEngine[];

const FormatVersions * PfmtversEngineMin();
const FormatVersions * PfmtversEngineMax();
const FormatVersions * PfmtversEngineDefault();
const FormatVersions * PfmtversEngineSafetyVersion();

inline JET_ENGINEFORMATVERSION EfvMinSupported() { return PfmtversEngineMin()->efv; }
inline JET_ENGINEFORMATVERSION EfvMaxSupported() { return PfmtversEngineMax()->efv; }
inline JET_ENGINEFORMATVERSION EfvDefault()      { return EfvMaxSupported(); }


ERR ErrGetDesiredVersion( _In_ const INST * const pinstStaging, _In_ const JET_ENGINEFORMATVERSION efv, _Out_ const FormatVersions ** const ppfmtversDesired, const BOOL fTestStagingOnly = fFalse );
ERR ErrDBFindHighestMatchingDbMajors( _In_ const DbVersion& dbvFromFileHeader, _Out_ const FormatVersions ** const ppfmtversMatching, _In_ const INST * const pinstStaging, const BOOL fDbMayBeTooHigh = fFalse );
ERR ErrLGFindHighestMatchingLogMajors( _In_ const LogVersion& lgv, _Out_ const FormatVersions ** const ppfmtversMatching );
INLINE ERR ErrDBFindHighestMatchingDbMajors( _In_ const DbVersion& dbvFromFileHeader, _Out_ const FormatVersions ** const ppfmtversMatching, const BOOL fDbMayBeTooHigh = fFalse )
{
    return ErrDBFindHighestMatchingDbMajors( dbvFromFileHeader, ppfmtversMatching, pinstNil, fDbMayBeTooHigh );
}

const ULONG cchFormatEfvSetting = 120;
void FormatEfvSetting( const JET_ENGINEFORMATVERSION efvFullParam, _Out_writes_bytes_(cbEfvSetting) WCHAR * const wszEfvSetting, _In_range_( 240, 240  ) const ULONG cbEfvSetting );
inline WCHAR * WszFormatEfvSetting( const JET_ENGINEFORMATVERSION efvFullParam, _Out_writes_bytes_(cbEfvSetting) WCHAR * const wszEfvSetting, _In_range_( 240, 240  ) const ULONG cbEfvSetting )
{
    FormatEfvSetting( efvFullParam, wszEfvSetting, cbEfvSetting );
    return wszEfvSetting;
}
void FormatDbvEfvMapping( const IFMP ifmp, _Out_writes_bytes_( cbSetting ) WCHAR * wszSetting, _In_ const ULONG cbSetting );


#define ulDAEVersionMax             ( PfmtversEngineMax()->dbv.ulDbMajorVersion )
#define ulDAEUpdateMajorMax         ( PfmtversEngineMax()->dbv.ulDbUpdateMajor )
#define ulDAEUpdateMinorMax         ( PfmtversEngineMax()->dbv.ulDbUpdateMinor )



const ULONG ulDAEVersion200         = 0x00000001;
const ULONG ulDAEVersion400         = 0x00000400;
const ULONG ulDAEVersion500         = 0x00000500;
const ULONG ulDAEVersion550         = 0x00000550;

const ULONG ulDAEVersionESE97       = 0x00000620;
const ULONG ulDAEUpdateMajorESE97   = 0x00000002;

const ULONG ulDAEVersionNewLVChunk      = 0x00000620;
const ULONG ulDAEUpdateMajorNewLVChunk  = 0x00000011;

const USHORT usDAECreateDbVersion       = 0x0002;
const USHORT usDAECreateDbUpdateMajor   = 0x0000;

const USHORT usDAECreateDbVersion_Implicit  = 0x0001;
const USHORT usDAECreateDbUpdate_Implicit   = 0x0000;



#define ulLGVersionMajorMax                         ( PfmtversEngineMax()->lgv.ulLGVersionMajor )
const ULONG ulLGVersionMinorFinalDeprecatedValue    = 4000;
#define ulLGVersionUpdateMajorMax                   ( PfmtversEngineMax()->lgv.ulLGVersionUpdateMajor )
#define ulLGVersionUpdateMinorMax                   ( PfmtversEngineMax()->lgv.ulLGVersionUpdateMinor )



const ULONG ulLGVersionMajorNewLVChunk  = 7;
const ULONG ulLGVersionMinorNewLVChunk  = 3704;
const ULONG ulLGVersionUpdateNewLVChunk = 14;

const ULONG ulLGVersionMajor_Win7       = 7;
const ULONG ulLGVersionMinor_Win7       = 3704;
const ULONG ulLGVersionUpdateMajor_Win7 = 15;

const ULONG ulLGVersionMajor_OldLrckFormat          = 7;
const ULONG ulLGVersionMinor_OldLrckFormat          = 3704;
const ULONG ulLGVersionUpdateMajor_OldLrckFormat    = 17;

const ULONG ulLGVersionMajor_ZeroFilled         = 8;
const ULONG ulLGVersionMinor_ZeroFilled         = 4000;
const ULONG ulLGVersionUpdateMajor_ZeroFilled   = 2;

const ULONG ulLGVersionMajor_AttachChkpt        = 8;
const ULONG ulLGVersionMinor_AttachChkpt        = 4000;
const ULONG ulLGVersionUpdateMajor_AttachChkpt  = 4;
const ULONG ulLGVersionUpdateMinor_AttachChkpt  = 3;

#define fmtlgverBeforePreImageCompression           8,4000,4,13

const ULONG ulLGVersionUpdateMajor_NewCommitCtx1    = 5;
const ULONG ulLGVersionUpdateMinor_NewCommitCtx1    = 2;
const ULONG ulLGVersionUpdateMajor_NewCommitCtx2    = 4;
const ULONG ulLGVersionUpdateMinor_NewCommitCtx2    = 8;


#define fmtlgverFormatV8                            8,4000,0,0
#define fmtlgverInitialDbSizeLoggedMain             8,4000,5,4

const ULONG ulLGVersionUpdateMinorPrevGenSegmentChecksum    = 12;
const ULONG ulLGVersionUpdateMinorPrevGenAllSegmentChecksum = 13;

#define fmtlgverPrevGenSegmentChecksum              8,4000,5,12

#define fmtlgverPrevGenAllSegmentChecksum           8,4000,5,13

#ifdef LOG_FORMAT_CHANGE
UNDONE REMINDER: on next log format change:
    - remove lrtypInit2 (and Term2, RcvUndo2, RcvQuit2)
    - remove bitfield in LRBEGIN
    - merge BEGIN0 and BEGIND
    - patching no longer supported (ie. cannot restore backups in previous format)
    - get rid of filetype hack and make sure checkpoint and log headers have common
      first 16 bytes as dbheader
    - merge together lrtypMacroCommit and lrtypMacroInfo and lrtypMacroInfo2
    - make lrtypExtendDB required (instead of a hint)
#endif




const ULONG ulFMVersionMajorMax         = 3;
const ULONG ulFMVersionUpdateMajorMax   = 0;
const ULONG ulFMVersionUpdateMinorMax   = 0;


inline INT CmpLogFormatVersion( const ULONG ulMajorV1, const ULONG ulMinorV1, const ULONG ulUpdateMajorV1, const ULONG ulUpdateMinorV1,
                                const ULONG ulMajorV2, const ULONG ulMinorV2, const ULONG ulUpdateMajorV2, const ULONG ulUpdateMinorV2 )
{
    if( ulMajorV1 < ulMajorV2 )
    {
        return -4;
    }
    else if ( ulMajorV1 > ulMajorV2 )
    {
        return 4;
    }

    if( ulMinorV1 < ulMinorV2 )
    {
        return -3;
    }
    else if ( ulMinorV1 > ulMinorV2 )
    {
        return 3;
    }

    if( ulUpdateMajorV1 < ulUpdateMajorV2 )
    {
        return -2;
    }
    else if ( ulUpdateMajorV1 > ulUpdateMajorV2 )
    {
        return 2;
    }

    if( ulUpdateMinorV1 < ulUpdateMinorV2 )
    {
        return -1;
    }
    else if ( ulUpdateMinorV1 > ulUpdateMinorV2 )
    {
        return 1;
    }

    Assert( ulMajorV1 == ulMajorV2 && ulMinorV1 == ulMinorV2 && ulUpdateMajorV1 == ulUpdateMajorV2 && ulUpdateMinorV1 == ulUpdateMinorV2 );

    return 0;
}

inline BOOL FLogFormatUnifiedMinorUpdateVersion( const ULONG ulMajor, const ULONG ulMinor, const ULONG ulUpdateMajor, const ULONG ulUpdateMinor )
{
    if ( 0 <= CmpLogFormatVersion( ulMajor, ulMinor, ulUpdateMajor, ulUpdateMinor, fmtlgverFormatV8 ) &&
        11 <= ulUpdateMinor )
    {
        return fTrue;
    }
    else
    {
        return fFalse;
    }
}

#define attribDb    0

const BYTE  fDbShadowingDisabled    = 0x01;
const BYTE  fDbUpgradeDb            = 0x02;

PERSISTED
struct DBFILEHDR : public CZeroInit
{
    LittleEndian<ULONG>     le_ulChecksum;
    LittleEndian<ULONG>     le_ulMagic;
    LittleEndian<ULONG>     le_ulVersion;
    LittleEndian<LONG>      le_attrib;

    LittleEndian<DBTIME>    le_dbtimeDirtied;

    SIGNATURE               signDb;

    LittleEndian<ULONG>     le_dbstate;

    LE_LGPOS                le_lgposConsistent;
    LOGTIME                 logtimeConsistent;

    LOGTIME                 logtimeAttach;
    LE_LGPOS                le_lgposAttach;

    LOGTIME                 logtimeDetach;
    LE_LGPOS                le_lgposDetach;

    LittleEndian<ULONG>     le_dbid;

    SIGNATURE               signLog;

    BKINFO                  bkinfoFullPrev;

    BKINFO                  bkinfoIncPrev;

    BKINFO                  bkinfoFullCur;

    union
    {
        ULONG               m_ulDbFlags;
        BYTE                m_rgbDbFlags[4];
    };

    LittleEndian<OBJID>     le_objidLast;


    LittleEndian<DWORD>     le_dwMajorVersion;
    LittleEndian<DWORD>     le_dwMinorVersion;

    LittleEndian<DWORD>     le_dwBuildNumber;
    LittleEndian<LONG>      le_lSPNumber;

    LittleEndian<ULONG>     le_ulDaeUpdateMajor;

    LittleEndian<ULONG>     le_cbPageSize;

    LittleEndian<ULONG>     le_ulRepairCount;
    LOGTIME                 logtimeRepair;

    BYTE                    rgbReservedSignSLV[ sizeof( SIGNATURE ) ];

    LittleEndian<DBTIME>    le_dbtimeLastScrub;

    LOGTIME                 logtimeScrub;

    LittleEndian<LONG>      le_lGenMinRequired;

    LittleEndian<LONG>      le_lGenMaxRequired;

    LittleEndian<LONG>      le_cpgUpgrade55Format;
    LittleEndian<LONG>      le_cpgUpgradeFreePages;
    LittleEndian<LONG>      le_cpgUpgradeSpaceMapPages;

    BKINFO                  bkinfoSnapshotCur;

    LittleEndian<ULONG>     le_ulCreateVersion;
    LittleEndian<ULONG>     le_ulCreateUpdate;

    LOGTIME                 logtimeGenMaxCreate;

    typedef enum
    {
        backupNormal = 0x0,
        backupOSSnapshot,
        backupSnapshot,
        backupSurrogate,
    } BKINFOTYPE;

    LittleEndian<BKINFOTYPE>    bkinfoTypeFullPrev;
    LittleEndian<BKINFOTYPE>    bkinfoTypeIncPrev;

    LittleEndian<ULONG>     le_ulRepairCountOld;

    LittleEndian<ULONG>     le_ulECCFixSuccess;
    LOGTIME                 logtimeECCFixSuccess;
    LittleEndian<ULONG>     le_ulECCFixSuccessOld;

    LittleEndian<ULONG>     le_ulECCFixFail;
    LOGTIME                 logtimeECCFixFail;
    LittleEndian<ULONG>     le_ulECCFixFailOld;

    LittleEndian<ULONG>     le_ulBadChecksum;
    LOGTIME                 logtimeBadChecksum;
    LittleEndian<ULONG>     le_ulBadChecksumOld;


    LittleEndian<LONG>      le_lGenMaxCommitted;
    BKINFO                  bkinfoCopyPrev;
    BKINFO                  bkinfoDiffPrev;

    LittleEndian<BKINFOTYPE>    bkinfoTypeCopyPrev;
    LittleEndian<BKINFOTYPE>    bkinfoTypeDiffPrev;

    LittleEndian<ULONG>     le_ulIncrementalReseedCount;
    LOGTIME                 logtimeIncrementalReseed;
    LittleEndian<ULONG>     le_ulIncrementalReseedCountOld;

    LittleEndian<ULONG>     le_ulPagePatchCount;
    LOGTIME                 logtimePagePatch;
    LittleEndian<ULONG>     le_ulPagePatchCountOld;

    UnalignedLittleEndian<QWORD>    le_qwSortVersion;

    LOGTIME                 logtimeDbscanPrev;
    LOGTIME                 logtimeDbscanStart;
    LittleEndian<PGNO>      le_pgnoDbscanHighestContinuous;

    LittleEndian<LONG>      le_lGenRecovering;


    LittleEndian<ULONG>     le_ulExtendCount;
    LOGTIME                 logtimeLastExtend;
    LittleEndian<ULONG>     le_ulShrinkCount;
    LOGTIME                 logtimeLastShrink;


    LOGTIME                 logtimeLastReAttach;
    LE_LGPOS                le_lgposLastReAttach;

    LittleEndian<ULONG>     le_ulTrimCount;

    SIGNATURE               signDbHdrFlush;
    SIGNATURE               signFlushMapHdrFlush;

    LittleEndian<LONG>      le_lGenMinConsistent;

    LittleEndian<ULONG>     le_ulDaeUpdateMinor;
    LittleEndian<JET_ENGINEFORMATVERSION>   le_efvMaxBinAttachDiagnostic;

    LittleEndian<PGNO>      le_pgnoDbscanHighest;

    LE_LGPOS                le_lgposLastResize;

    BYTE                    rgbReserved[3];

    UnalignedLittleEndian<ULONG>    le_filetype;

    BYTE                    rgbReserved2[1];

    LOGTIME                 logtimeGenMaxRequired;

    LittleEndian<LONG>      le_lGenPreRedoMinConsistent;
    LittleEndian<LONG>      le_lGenPreRedoMinRequired;

    SIGNATURE               signRBSHdrFlush;

    LittleEndian<ULONG>     le_ulRevertCount;
    LOGTIME                 logtimeRevertFrom;
    LOGTIME                 logtimeRevertTo;
    LittleEndian<ULONG>     le_ulRevertPageCount;
    LE_LGPOS                le_lgposCommitBeforeRevert;
                                                           



    DBFILEHDR() :
        CZeroInit( sizeof( DBFILEHDR ) )
    {
    }



    ULONG   Dbstate( ) const                { return le_dbstate; }
    VOID    SetDbstate( const ULONG dbstate, const LONG lGenMin, const LONG lGenMax, const LOGTIME * plogtimeCurrent = NULL, const bool fLogged = fFalse );
    VOID    SetDbstate( const ULONG dbstate, const LONG lGenCurrent = 0, const LOGTIME * plogtimeCurrent = NULL, const bool fLogged = fFalse );

    BOOL    FShadowingDisabled( ) const     { return m_rgbDbFlags[0] & fDbShadowingDisabled; }
    VOID    SetShadowingDisabled()          { m_rgbDbFlags[0] |= fDbShadowingDisabled; }

    VOID    ResetUpgradeDb()                { m_rgbDbFlags[0] &= ~fDbUpgradeDb; }

    DbVersion Dbv() const;

#ifdef DEBUGGER_EXTENSION
    ERR     Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif
    ERR     DumpLite( CPRINTF* pcprintf, const char * const szNewLine, DWORD_PTR dwOffset = 0 ) const;

};

C_ASSERT( sizeof( DBFILEHDR ) == 748 );
C_ASSERT( sizeof(DBFILEHDR::BKINFOTYPE) == sizeof(DWORD) );
C_ASSERT( 667 == offsetof( DBFILEHDR, le_filetype ) );
C_ASSERT( sizeof( DBFILEHDR ) <= g_cbPageMin );

INLINE BOOL FNullLogTime( const LOGTIME * const plogtime )
{
    BYTE * pb = (BYTE*)plogtime;
    for (INT i = 0; i < sizeof( LOGTIME ); ++i )
    {
        if ( pb[i] != 0 )
        {
            return fFalse;
        }
    }
    return fTrue;
}

INT CmpLgpos( const LGPOS& lgpos1, const LGPOS& lgpos2 );

INLINE VOID DBFILEHDR::SetDbstate( const ULONG dbstate, const LONG lGenMin, const LONG lGenMax, const LOGTIME * const plogtimeCurrent, const bool fLogged )
{
#ifndef RTM

    switch ( le_dbstate )
    {
        case 0:
            AssertRTL( JET_dbstateJustCreated == dbstate );
            break;
        case JET_dbstateJustCreated:
            AssertRTL( JET_dbstateDirtyShutdown == dbstate );
            break;
        case JET_dbstateDirtyShutdown:
            AssertRTL( JET_dbstateCleanShutdown == dbstate ||
                        JET_dbstateIncrementalReseedInProgress == dbstate ||
                        JET_dbstateRevertInProgress == dbstate );
            break;
        case JET_dbstateCleanShutdown:
            AssertRTL( JET_dbstateDirtyShutdown == dbstate ||
                        JET_dbstateRevertInProgress == dbstate );
            break;        
        case JET_dbstateBeingConverted:
            AssertRTL( JET_dbstateCleanShutdown == dbstate );
            break;
        case JET_dbstateIncrementalReseedInProgress:
            AssertRTL( JET_dbstateDirtyAndPatchedShutdown == dbstate ||
                        ( g_fRepair && JET_dbstateDirtyShutdown == dbstate ) );
            break;
        case JET_dbstateDirtyAndPatchedShutdown:
            AssertRTL( JET_dbstateDirtyShutdown == dbstate ||
                        JET_dbstateIncrementalReseedInProgress == dbstate || 
                        JET_dbstateRevertInProgress == dbstate );
            AssertRTL( 0 != lGenMin );
            AssertRTL( 0 != lGenMax );
            break;
        case JET_dbstateRevertInProgress:
            AssertRTL( JET_dbstateDirtyAndPatchedShutdown == dbstate || 
                        JET_dbstateCleanShutdown == dbstate || 
                        JET_dbstateDirtyShutdown == dbstate ||
                        JET_dbstateJustCreated == dbstate );
            break;      
        default:
            AssertSzRTL( fFalse, "Unknown pre-existing state in DB header!!" );
    }

    switch ( dbstate )
    {
        case JET_dbstateJustCreated:
            AssertRTL( 0 == lGenMin );
            AssertRTL( 0 == lGenMax );
            AssertRTL( NULL == plogtimeCurrent );
            break;
        case JET_dbstateDirtyShutdown:
            if ( le_dbstate == JET_dbstateDirtyAndPatchedShutdown )
            {
                AssertRTL( 0 != lGenMin );
                AssertRTL( 0 != lGenMax );
            }
            else if ( fLogged && 0 != lGenMin )
            {
                AssertRTL( 0 != lGenMin && lGenerationInvalid != lGenMin );
                AssertRTL( 0 != lGenMax && lGenerationInvalid != lGenMax );
                AssertRTL( NULL != plogtimeCurrent  );
            }
            else
            {
                AssertRTL( 0 == lGenMin );
                AssertRTL( 0 == lGenMax );
                AssertRTL( NULL == plogtimeCurrent || FNullLogTime( plogtimeCurrent ) );
            }
            break;
        case JET_dbstateCleanShutdown:
            AssertRTL( 0 == lGenMin );
            AssertRTL( 0 == lGenMax );
            AssertRTL( NULL == plogtimeCurrent );
            break;
        case JET_dbstateBeingConverted:
            AssertRTL( 0 == lGenMin );
            AssertRTL( 0 == lGenMax );
            AssertRTL( NULL == plogtimeCurrent );
            break;
        case JET_dbstateIncrementalReseedInProgress:
            if ( fLogged )
            {
                AssertRTL( lGenerationInvalid == lGenMin );
                AssertRTL( lGenerationInvalid == lGenMax );
                AssertRTL( NULL == plogtimeCurrent );
            }
            else
            {
                AssertSzRTL( fFalse, "Incremental reseed is only valid on logged databases." );
            }
            break;
        case JET_dbstateDirtyAndPatchedShutdown:
            AssertRTL( fLogged );
            AssertRTL( 0 != lGenMin && lGenerationInvalid != lGenMin );
            AssertRTL( 0 != lGenMax && lGenerationInvalid != lGenMax );

            AssertRTL( NULL != plogtimeCurrent  );
            break;
        case JET_dbstateRevertInProgress:
            AssertRTL( fLogged );
            AssertRTL( NULL == plogtimeCurrent );
            AssertRTL( lGenerationInvalid == lGenMin );
            AssertRTL( lGenerationInvalid == lGenMax );
            break;
        default:
            AssertSzRTL( fFalse, "Trying to switch to an unknown state!!!" );
    }
#endif

    if ( dbstate != JET_dbstateDirtyShutdown )
    {
        le_lGenPreRedoMinRequired = 0;
        le_lGenPreRedoMinConsistent = 0;
    }


    le_dbstate = dbstate;
    if ( lGenerationInvalid != lGenMin )
    {
        le_lGenMinRequired = lGenMin;
        le_lGenMinConsistent = lGenMin;
    }
    if ( lGenerationInvalid != lGenMax )
    {
        le_lGenMaxRequired = lGenMax;
        le_lGenMaxCommitted = lGenMax;
    }

    if ( lGenerationInvalid == lGenMin ||
            lGenerationInvalid == lGenMax )
    {
        Expected( NULL == plogtimeCurrent );
    }
    else if ( 0 != lGenMax && NULL != plogtimeCurrent )
    {
        memcpy( &logtimeGenMaxCreate, plogtimeCurrent, sizeof( LOGTIME ) );

        const FormatVersions * pfmpver = NULL;
        CallS( ErrGetDesiredVersion( NULL, JET_efvLogtimeGenMaxRequired, &pfmpver ) );
        if ( pfmpver != NULL && CmpDbVer( Dbv(), pfmpver->dbv ) >= 0 )
        {
            memcpy( &logtimeGenMaxRequired, plogtimeCurrent, sizeof( LOGTIME ) );
        }
        else
        {
            memset( &logtimeGenMaxRequired, '\0', sizeof( LOGTIME ) );
        }
    }
    else
    {
        memset( &logtimeGenMaxCreate, '\0', sizeof( LOGTIME ) );
        memset( &logtimeGenMaxRequired, '\0', sizeof( LOGTIME ) );
    }
}

INLINE VOID DBFILEHDR::SetDbstate( const ULONG dbstate, const LONG lGenCurrent, const LOGTIME * const plogtimeCurrent, const bool fLogged )
{
    SetDbstate( dbstate, lGenCurrent, lGenCurrent, plogtimeCurrent, fLogged );
}

inline DbVersion DBFILEHDR::Dbv() const
{
    DbVersion dbvHdr = { le_ulVersion, le_ulDaeUpdateMajor, le_ulDaeUpdateMinor };
    return dbvHdr;
}

INLINE ERR SigToSz( const SIGNATURE * const psig, __out_bcount(cbSigBuffer) PSTR szSigBuffer, ULONG cbSigBuffer )
{
    LOGTIME tm = psig->logtimeCreate;
    const char * szSigFormat = "Create time:%02d/%02d/%04d %02d:%02d:%02d.%3.3d Rand:%lu Computer:%s";

    ErrOSStrCbFormatA( szSigBuffer, cbSigBuffer, szSigFormat,
                        (SHORT) tm.bMonth, (SHORT) tm.bDay, (SHORT) tm.bYear + 1900,
                        (SHORT) tm.bHours, (SHORT) tm.bMinutes, (SHORT) tm.bSeconds,
                        (SHORT) tm.Milliseconds(),
                        ULONG(psig->le_ulRandom),
                        psig->szComputerName );
    return(JET_errSuccess);
}

INLINE VOID DUMPPrintSig( CPRINTF* pcprintf, const SIGNATURE * const psig )
{
    CHAR rgSig[128];
    SigToSz( psig, rgSig, sizeof(rgSig) );
    (*pcprintf)( "%s", rgSig );
}

INLINE  VOID DUMPPrintLogTime( CPRINTF* pcprintf, const LOGTIME * const plt )
{
    (*pcprintf)( "%02d/%02d/%04d %02d:%02d:%02d.%3.3d %s",
        ( SHORT )plt->bMonth, ( SHORT )plt->bDay, ( SHORT )plt->bYear + 1900,
        ( SHORT )plt->bHours, ( SHORT )plt->bMinutes, ( SHORT )plt->bSeconds,
        ( SHORT )plt->Milliseconds(), plt->fTimeIsUTC ? "UTC" : "LOC" );
}

extern const QWORD g_qwUpgradedLocalesTable;

const CHAR * SzFromState( ULONG dbstate );

INLINE ERR DBFILEHDR::DumpLite( CPRINTF* pcprintf, const char * const szNewLine, DWORD_PTR dwOffset ) const
{
    LGPOS   lgpos;
    LOGTIME tm;

    (*pcprintf)( "        File Type: %s%s", attribDb == le_attrib ? "Database" : "UNKNOWN", szNewLine );
    (*pcprintf)( "         Checksum: 0x%x%s", (ULONG) le_ulChecksum, szNewLine );
    (*pcprintf)( "   Format ulMagic: 0x%x%s", (ULONG) le_ulMagic, szNewLine );
    (*pcprintf)( "   Engine ulMagic: 0x%x%s", ulDAEMagic, szNewLine );
    (*pcprintf)( " Format ulVersion: %d.%d.%d  (attached by %d)%s", (ULONG) le_ulVersion, (ULONG) le_ulDaeUpdateMajor, (ULONG) le_ulDaeUpdateMinor, (ULONG) le_efvMaxBinAttachDiagnostic, szNewLine );
    (*pcprintf)( " Engine ulVersion: %d.%d.%d%s", ulDAEVersionMax, ulDAEUpdateMajorMax, ulDAEUpdateMinorMax, szNewLine );
    (*pcprintf)( "Created ulVersion: %d.%d%s", (ULONG) le_ulCreateVersion, (ULONG) le_ulCreateUpdate, szNewLine );
    (*pcprintf)( "     DB Signature: " );
    DUMPPrintSig( pcprintf, &signDb );
    (*pcprintf)( "%s", szNewLine );
    (*pcprintf)( "         cbDbPage: %d%s", (ULONG) le_cbPageSize, szNewLine );
    const DBTIME dbtimeDirtied = le_dbtimeDirtied;
    (*pcprintf)( "           dbtime: %I64u (0x%I64x)%s", (DBTIME) dbtimeDirtied, (DBTIME) dbtimeDirtied, szNewLine );
    (*pcprintf)( "            State: %hs%s", SzFromState( Dbstate() ), szNewLine );
    (*pcprintf)( "     Log Required: %u-%u (0x%x-0x%x) [Min. Req. Pre-Redo: %u (0x%x)]%s",
                    (ULONG) le_lGenMinRequired,
                    (ULONG) le_lGenMaxRequired,
                    (ULONG) le_lGenMinRequired,
                    (ULONG) le_lGenMaxRequired,
                    (ULONG) le_lGenPreRedoMinRequired,
                    (ULONG) le_lGenPreRedoMinRequired,
                    szNewLine );
    (*pcprintf)( "    Log Committed: %u-%u (0x%x-0x%x)%s",
                    (ULONG) 0,
                    (ULONG) le_lGenMaxCommitted,
                    (ULONG) 0,
                    (ULONG) le_lGenMaxCommitted,
                    szNewLine );
    (*pcprintf)( "   Log Recovering: %u (0x%x)%s",
                    (ULONG) le_lGenRecovering,
                    (ULONG) le_lGenRecovering,
                    szNewLine );
    (*pcprintf)( "   Log Consistent: %u (0x%x)  [Pre-Redo: %u (0x%x)]%s",
                    (ULONG) le_lGenMinConsistent,
                    (ULONG) le_lGenMinConsistent,
                    (ULONG) le_lGenPreRedoMinConsistent,
                    (ULONG) le_lGenPreRedoMinConsistent,
                    szNewLine );
    tm = logtimeGenMaxCreate;
    (*pcprintf)( "  GenMax Creation: " );
    DUMPPrintLogTime( pcprintf, &tm );
    (*pcprintf)( "%s", szNewLine );
    (*pcprintf)( "         Shadowed: %s%s", FShadowingDisabled( ) ? "No" : "Yes", szNewLine );
    (*pcprintf)( "       Last Objid: %u%s", (ULONG) le_objidLast, szNewLine );
    const DBTIME dbtimeLastScrub = le_dbtimeLastScrub;
    (*pcprintf)( "     Scrub Dbtime: %I64u (0x%I64x)%s", dbtimeLastScrub, dbtimeLastScrub, szNewLine );
    tm = logtimeScrub;
    (*pcprintf)( "       Scrub Date: " );
    DUMPPrintLogTime( pcprintf, &tm );
    (*pcprintf)( "%s", szNewLine );
    (*pcprintf)( "     Repair Count: %u%s", (ULONG) le_ulRepairCount, szNewLine );
    (*pcprintf)( "      Repair Date: " );
    tm = logtimeRepair;
    DUMPPrintLogTime( pcprintf, &tm );
    (*pcprintf)( "%s", szNewLine );
    (*pcprintf)( " Old Repair Count: %u%s", (ULONG) le_ulRepairCountOld, szNewLine );
    lgpos = le_lgposConsistent;
    (*pcprintf)( "  Last Consistent: (0x%X,%X,%X)  ", lgpos.lGeneration, lgpos.isec, lgpos.ib );
    tm = logtimeConsistent;
    DUMPPrintLogTime( pcprintf, &tm );
    (*pcprintf)( "%s", szNewLine );
    lgpos = le_lgposAttach;
    (*pcprintf)( "      Last Attach: (0x%X,%X,%X)  ", lgpos.lGeneration, lgpos.isec, lgpos.ib );
    tm = logtimeAttach;
    DUMPPrintLogTime( pcprintf, &tm );
    (*pcprintf)( "%s", szNewLine );

    lgpos = le_lgposDetach;
    (*pcprintf)( "      Last Detach: (0x%X,%X,%X)  ", lgpos.lGeneration, lgpos.isec, lgpos.ib );

    tm = logtimeDetach;
    DUMPPrintLogTime( pcprintf, &tm );
    (*pcprintf)( "%s", szNewLine );

    lgpos = le_lgposLastReAttach;
    (*pcprintf)( "    Last ReAttach: (0x%X,%X,%X)  ", lgpos.lGeneration, lgpos.isec, lgpos.ib );
    tm = logtimeLastReAttach;
    DUMPPrintLogTime( pcprintf, &tm );
    (*pcprintf)( "%s", szNewLine );

    (*pcprintf)( "             Dbid: %d%s", (SHORT) le_dbid, szNewLine );

    (*pcprintf)( "    Log Signature: " );
    DUMPPrintSig( pcprintf, &signLog );
    (*pcprintf)( "%s", szNewLine );

    (*pcprintf)( "       OS Version: (%u.%u.%u SP %u ",
                (ULONG) le_dwMajorVersion,
                (ULONG) le_dwMinorVersion,
                (ULONG) le_dwBuildNumber,
                (ULONG) le_lSPNumber );
    if ( g_qwUpgradedLocalesTable == le_qwSortVersion )
    {
        (*pcprintf)( "MSysLocales)%s", szNewLine );
    }
    else
    {
        (*pcprintf)( "%x.%x)%s",
                    (DWORD)( ( le_qwSortVersion >> 32 ) & 0xFFFFFFFF ),
                    (DWORD)( le_qwSortVersion & 0xFFFFFFFF ),
                    szNewLine );
    }

    (*pcprintf)( "            Flags: 0x%x%s", m_ulDbFlags, szNewLine );

    (*pcprintf)( "     Reseed Count: %u%s", (ULONG) le_ulIncrementalReseedCount, szNewLine );
    (*pcprintf)( "      Reseed Date: " );
    tm = logtimeIncrementalReseed;
    DUMPPrintLogTime( pcprintf, &tm );
    (*pcprintf)( "%s", szNewLine );
    (*pcprintf)( " Old Reseed Count: %u%s", (ULONG) le_ulIncrementalReseedCountOld, szNewLine );
    (*pcprintf)( "      Patch Count: %u%s", (ULONG) le_ulPagePatchCount, szNewLine );
    (*pcprintf)( "       Patch Date: " );
    tm = logtimePagePatch;
    DUMPPrintLogTime( pcprintf, &tm );
    (*pcprintf)( "%s", szNewLine );
    (*pcprintf)( "  Old Patch Count: %u%s", (ULONG) le_ulPagePatchCountOld, szNewLine );

    lgpos = le_lgposLastResize;
    (*pcprintf)( "%s", szNewLine );
    (*pcprintf)( "      Last Resize: (0x%X,%X,%X)  ", lgpos.lGeneration, lgpos.isec, lgpos.ib );
    if ( CmpLgpos( lgpos, le_lgposAttach ) == 0 )
    {
        (*pcprintf)( "(Matches \"Last Attach\", no subsequent resize)" );
    }
    else if ( CmpLgpos( lgpos, le_lgposConsistent ) == 0 )
    {
        (*pcprintf)( "(Matches \"Last Consistent\", no subsequent resize)" );
    }
    (*pcprintf)( "%s", szNewLine );

    (*pcprintf)( "     Extend Count: %u%s", (ULONG) le_ulExtendCount, szNewLine );
    (*pcprintf)( "      Extend Date: " );
    tm = logtimeLastExtend;
    DUMPPrintLogTime( pcprintf, &tm );
    (*pcprintf)( "%s", szNewLine );

    (*pcprintf)( "     Shrink Count: %u%s", (ULONG) le_ulShrinkCount, szNewLine );
    (*pcprintf)( "      Shrink Date: " );
    tm = logtimeLastShrink;
    DUMPPrintLogTime( pcprintf, &tm );
    (*pcprintf)( "%s", szNewLine );

    (*pcprintf)( "       Trim Count: %u%s", (ULONG) le_ulTrimCount, szNewLine );

    (*pcprintf)( " DB Hdr Flush Sig: " );
    DUMPPrintSig( pcprintf, &signDbHdrFlush );
    (*pcprintf)( "%s", szNewLine );

    (*pcprintf)( " FM Hdr Flush Sig: " );
    DUMPPrintSig( pcprintf, &signFlushMapHdrFlush);
    (*pcprintf)( "%s", szNewLine );

    (*pcprintf)( "RBS Hdr Flush Sig: " );
    DUMPPrintSig( pcprintf, &signRBSHdrFlush);
    (*pcprintf)( "%s", szNewLine );

    (*pcprintf)( "     Revert Count: %u%s", (ULONG) le_ulRevertCount, szNewLine );
    (*pcprintf)( " Revert From Date: " );
    tm = logtimeRevertFrom;
    DUMPPrintLogTime( pcprintf, &tm );
    (*pcprintf)( "%s", szNewLine );
    (*pcprintf)( "   Revert To Date: " );
    tm = logtimeRevertTo;
    DUMPPrintLogTime( pcprintf, &tm );
    (*pcprintf)( "%s", szNewLine );
    (*pcprintf)( "Revert Page Count: %u%s", (ULONG) le_ulRevertPageCount, szNewLine );

    lgpos = le_lgposCommitBeforeRevert;
    (*pcprintf)( "Last Commit Before Revert: (0x%X,%X,%X)  ", lgpos.lGeneration, lgpos.isec, lgpos.ib );

    return JET_errSuccess;
}

#ifdef DEBUGGER_EXTENSION

extern const CHAR * const mpdbstatesz[ JET_dbstateDirtyAndPatchedShutdown + 1 ];

INLINE ERR DBFILEHDR::Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset ) const
{
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulChecksum, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulMagic, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulVersion, dwOffset ) );
    (*pcprintf)( FORMAT_INT( DBFILEHDR, this, le_attrib, dwOffset ) );
    (*pcprintf)( FORMAT_INT( DBFILEHDR, this, le_dbtimeDirtied, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( DBFILEHDR, this, signDb, dwOffset ) );
    (*pcprintf)( FORMAT_ENUM( DBFILEHDR, this, le_dbstate, dwOffset, mpdbstatesz, 0, JET_dbstateDirtyAndPatchedShutdown + 1 ) );
    (*pcprintf)( FORMAT_LE_LGPOS( DBFILEHDR, this, le_lgposConsistent, dwOffset ) );
    (*pcprintf)( FORMAT_LOGTIME( DBFILEHDR, this, logtimeConsistent, dwOffset ) );
    (*pcprintf)( FORMAT_LOGTIME( DBFILEHDR, this, logtimeAttach, dwOffset ) );
    (*pcprintf)( FORMAT_LE_LGPOS( DBFILEHDR, this, le_lgposAttach, dwOffset ) );
    (*pcprintf)( FORMAT_LOGTIME( DBFILEHDR, this, logtimeDetach, dwOffset ) );
    (*pcprintf)( FORMAT_LE_LGPOS( DBFILEHDR, this, le_lgposDetach, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_dbid, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( DBFILEHDR, this, signLog, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( DBFILEHDR, this, bkinfoFullPrev, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( DBFILEHDR, this, bkinfoIncPrev, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( DBFILEHDR, this, bkinfoFullCur, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, m_ulDbFlags, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_objidLast, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_dwMajorVersion, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_dwMinorVersion, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_dwBuildNumber, dwOffset ) );
    (*pcprintf)( FORMAT_INT( DBFILEHDR, this, le_lSPNumber, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulDaeUpdateMajor, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_cbPageSize, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulRepairCount, dwOffset ) );
    (*pcprintf)( FORMAT_LOGTIME( DBFILEHDR, this, logtimeRepair, dwOffset ) );
    (*pcprintf)( FORMAT_INT( DBFILEHDR, this, le_dbtimeLastScrub, dwOffset ) );
    (*pcprintf)( FORMAT_LOGTIME( DBFILEHDR, this, logtimeScrub, dwOffset ) );
    (*pcprintf)( FORMAT_INT( DBFILEHDR, this, le_lGenMinRequired, dwOffset ) );
    (*pcprintf)( FORMAT_INT( DBFILEHDR, this, le_lGenMaxRequired, dwOffset ) );
    (*pcprintf)( FORMAT_INT( DBFILEHDR, this, le_lGenMinConsistent, dwOffset ) );
    (*pcprintf)( FORMAT_INT( DBFILEHDR, this, le_lGenPreRedoMinRequired, dwOffset ) );
    (*pcprintf)( FORMAT_INT( DBFILEHDR, this, le_lGenPreRedoMinConsistent, dwOffset ) );
    (*pcprintf)( FORMAT_INT( DBFILEHDR, this, le_cpgUpgrade55Format, dwOffset ) );
    (*pcprintf)( FORMAT_INT( DBFILEHDR, this, le_cpgUpgradeFreePages, dwOffset ) );
    (*pcprintf)( FORMAT_INT( DBFILEHDR, this, le_cpgUpgradeSpaceMapPages, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( DBFILEHDR, this, bkinfoSnapshotCur, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulCreateVersion, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulCreateUpdate, dwOffset ) );
    (*pcprintf)( FORMAT_LOGTIME( DBFILEHDR, this, logtimeGenMaxCreate, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, bkinfoTypeFullPrev, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, bkinfoTypeIncPrev, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulRepairCountOld, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulECCFixSuccess, dwOffset ) );
    (*pcprintf)( FORMAT_LOGTIME( DBFILEHDR, this, logtimeECCFixSuccess, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulECCFixSuccessOld, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulECCFixFail, dwOffset ) );
    (*pcprintf)( FORMAT_LOGTIME( DBFILEHDR, this, logtimeECCFixFail, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulECCFixFailOld, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulBadChecksum, dwOffset ) );
    (*pcprintf)( FORMAT_LOGTIME( DBFILEHDR, this, logtimeBadChecksum, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulBadChecksumOld, dwOffset ) );
    (*pcprintf)( FORMAT_INT( DBFILEHDR, this, le_lGenMaxCommitted, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( DBFILEHDR, this, bkinfoCopyPrev, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( DBFILEHDR, this, bkinfoDiffPrev, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, bkinfoTypeCopyPrev, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, bkinfoTypeDiffPrev, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulIncrementalReseedCount, dwOffset ) );
    (*pcprintf)( FORMAT_LOGTIME( DBFILEHDR, this, logtimeIncrementalReseed, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulIncrementalReseedCountOld, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulPagePatchCount, dwOffset ) );
    (*pcprintf)( FORMAT_LOGTIME( DBFILEHDR, this, logtimePagePatch, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulPagePatchCountOld, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_qwSortVersion, dwOffset ) );
    (*pcprintf)( FORMAT_LOGTIME( DBFILEHDR, this, logtimeDbscanPrev, dwOffset ) );
    (*pcprintf)( FORMAT_LOGTIME( DBFILEHDR, this, logtimeDbscanStart, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_pgnoDbscanHighestContinuous, dwOffset ) );
    (*pcprintf)( FORMAT_INT( DBFILEHDR, this, le_lGenRecovering, dwOffset ) );
    (*pcprintf)( FORMAT_LE_LGPOS( DBFILEHDR, this, le_lgposLastResize, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulExtendCount, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( DBFILEHDR, this, logtimeLastExtend, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulShrinkCount, dwOffset ) );
    (*pcprintf)( FORMAT_LOGTIME( DBFILEHDR, this, logtimeLastShrink, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulTrimCount, dwOffset ) );
    (*pcprintf)( FORMAT_LE_LGPOS( DBFILEHDR, this, le_lgposLastReAttach, dwOffset ) );
    (*pcprintf)( FORMAT_LOGTIME( DBFILEHDR, this, logtimeLastReAttach, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( DBFILEHDR, this, signDbHdrFlush, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( DBFILEHDR, this, signFlushMapHdrFlush, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulDaeUpdateMinor, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_efvMaxBinAttachDiagnostic, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_pgnoDbscanHighest, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_filetype, dwOffset ) );

    (*pcprintf)( FORMAT_VOID( DBFILEHDR, this, signRBSHdrFlush, dwOffset ) );

    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulRevertCount, dwOffset ) );
    (*pcprintf)( FORMAT_LOGTIME( DBFILEHDR, this, logtimeRevertFrom, dwOffset ) );
    (*pcprintf)( FORMAT_LOGTIME( DBFILEHDR, this, logtimeRevertTo, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( DBFILEHDR, this, le_ulRevertPageCount, dwOffset ) );
    (*pcprintf)( FORMAT_LE_LGPOS( DBFILEHDR, this, le_lgposCommitBeforeRevert, dwOffset ) );

    return JET_errSuccess;
}

#endif

typedef DBFILEHDR   DBFILEHDR_FIX;


VOID UtilLoadDbinfomiscFromPdbfilehdr( JET_DBINFOMISC7* pdbinfomisc, const ULONG cbdbinfomisc, const DBFILEHDR_FIX *pdbfilehdr );

typedef ULONG RCEID;
const RCEID rceidNull   = 0;


class CJetParam;

struct JetParam
{
    ~JetParam();

    enum Type
    {
        typeNull,
        typeBoolean,
        typeGrbit,
        typeInteger,
        typeString,
        typePointer,
        typeFolder,
        typePath,
        typeBlockSize,
        typeUserDefined,

        typeMax,
    };

    typedef INT         Config;
    #define configMaxLegacy (2)

    typedef
    ERR
    PfnGet( const CJetParam* const  pjetparam,
            INST* const             pinst,
            PIB* const              ppib,
            ULONG_PTR* const        pulParam,
            WCHAR* const            wszParam,
            const size_t            cbParamMax );
    typedef
    ERR
    PfnSet( CJetParam* const    pjetparam,
            INST* const         pinst,
            PIB* const          ppib,
            const ULONG_PTR     ulParam,
            __in_opt PCWSTR     wszParam );
    typedef
    ERR
    PfnClone(   CJetParam* const    pjetparamSrc,
                INST* const         pinstSrc,
                PIB* const          ppibSrc,
                CJetParam* const    pjetparamDst,
                INST* const         pinstDst,
                PIB* const          ppibDst );

    const CHAR* m_szParamName;
    ULONG_PTR   m_paramid:8;

    ULONG_PTR   m_type:4;
    ULONG_PTR   m_fCustomGetSet:1;
    ULONG_PTR   m_fCustomStorage:1;

    ULONG_PTR   m_fAdvanced:1;
    ULONG_PTR   m_fGlobal:1;
    ULONG_PTR   m_fMayNotWriteAfterGlobalInit:1;
    ULONG_PTR   m_fMayNotWriteAfterInstanceInit:1;

    ULONG_PTR   m_fWritten:1;
    ULONG_PTR   m_fRegDefault:1;
    ULONG_PTR   m_fRegOverride:1;
    ULONG_PTR   m_fFreeValue:1;

    ULONG_PTR   m_rangeLow;
    ULONG_PTR   m_rangeHigh;

    PfnGet*     m_pfnGet;
    PfnSet*     m_pfnSet;
    PfnClone*   m_pfnClone;

    ULONG_PTR   m_valueDefault[ configMaxLegacy ];

    ULONG_PTR   m_valueCurrent;
};

class CJetParam
    :   public JetParam
{
    public:

        CJetParam();

        ERR Get(    INST* const         pinst,
                    PIB* const          ppib,
                    ULONG_PTR* const    pulParam,
                    __out_bcount(cbParamMax) WCHAR* const   wszParam,
                    const size_t        cbParamMax ) const;
        ERR Set(    INST* const         pinst,
                    PIB* const          ppib,
                    const ULONG_PTR     ulParam,
                    __in_opt PCWSTR     wszParam );
        ERR Reset(  INST* const         pinst,
                    const ULONG_PTR     ulpValue );
        ERR Clone(  INST* const         pinstSrc,
                    PIB* const          ppibSrc,
                    CJetParam* const    pjetparamDst,
                    INST* const         pinstDst,
                    PIB* const          ppibDst );

        Type Type_() const          { return Type( m_type ); }
        BOOL FCustomGetSet() const  { return m_fCustomGetSet; }
        BOOL FCustomStorage() const { return m_fCustomStorage; }
        BOOL FAdvanced() const      { return m_fAdvanced; }
        BOOL FGlobal() const        { return m_fGlobal; }
        BOOL FWritten() const       { return m_fWritten; }
        ULONG_PTR Value() const     { return m_valueCurrent; }

    public:

        static ERR IgnoreGet(   const CJetParam* const  pjetparam,
                                INST* const             pinst,
                                PIB* const              ppib,
                                ULONG_PTR* const        pulParam,
                                __out_bcount(cbParamMax) WCHAR* const   wszParam,
                                const size_t            cbParamMax );
        static ERR IllegalGet(  const CJetParam* const  pjetparam,
                                INST* const             pinst,
                                PIB* const              ppib,
                                ULONG_PTR* const        pulParam,
                                __out_bcount(cbParamMax) WCHAR* const           wszParam,
                                const size_t            cbParamMax );
        static ERR GetInteger(  const CJetParam* const  pjetparam,
                                INST* const             pinst,
                                PIB* const              ppib,
                                ULONG_PTR* const        pulParam,
                                __out_bcount(cbParamMax) WCHAR* const   wszParam,
                                const size_t            cbParamMax );
        static ERR GetString(   const CJetParam* const  pjetparam,
                                INST* const             pinst,
                                PIB* const              ppib,
                                ULONG_PTR* const        pulParam,
                                __out_bcount(cbParamMax) WCHAR* const   wszParam,
                                const size_t            cbParamMax );

        static ERR IgnoreSet(   CJetParam* const    pjetparam,
                                INST* const         pinst,
                                PIB* const          ppib,
                                const ULONG_PTR     ulParam,
                                PCWSTR          wszParam );
        static ERR IllegalSet(  CJetParam* const    pjetparam,
                                INST* const         pinst,
                                PIB* const          ppib,
                                const ULONG_PTR     ulParam,
                                PCWSTR          wszParam );
        static ERR ValidateSet( CJetParam* const    pjetparam,
                                INST* const         pinst,
                                PIB* const          ppib,
                                const ULONG_PTR     ulParam,
                                PCWSTR          wszParam,
                                const BOOL          fString     = fFalse );
        static ERR SetBoolean(  CJetParam* const    pjetparam,
                                INST* const         pinst,
                                PIB* const          ppib,
                                const ULONG_PTR     ulParam,
                                PCWSTR          wszParam );
        static ERR SetGrbit(    CJetParam* const    pjetparam,
                                INST* const         pinst,
                                PIB* const          ppib,
                                const ULONG_PTR     ulParam,
                                PCWSTR          wszParam );
        static ERR SetInteger(  CJetParam* const    pjetparam,
                                INST* const         pinst,
                                PIB* const          ppib,
                                const ULONG_PTR     ulParam,
                                PCWSTR          wszParam );
        static ERR SetString(   CJetParam* const    pjetparam,
                                INST* const         pinst,
                                PIB* const          ppib,
                                const ULONG_PTR     ulParam,
                                PCWSTR          wszParam );
        static ERR SetPointer(  CJetParam* const    pjetparam,
                                INST* const         pinst,
                                PIB* const          ppib,
                                const ULONG_PTR     ulParam,
                                PCWSTR          wszParam );
        static ERR SetFolder(   CJetParam* const    pjetparam,
                                INST* const         pinst,
                                PIB* const          ppib,
                                const ULONG_PTR     ulParam,
                                PCWSTR          wszParam );
        static ERR SetPath( CJetParam* const    pjetparam,
                            INST* const         pinst,
                            PIB* const          ppib,
                            const ULONG_PTR     ulParam,
                            PCWSTR          wszParam );
        static ERR SetBlockSize(    CJetParam* const    pjetparam,
                                    INST* const         pinst,
                                    PIB* const          ppib,
                                    const ULONG_PTR     ulParam,
                                    PCWSTR          wszParam );
        static ERR CloneDefault(    CJetParam* const    pjetparamSrc,
                                    INST* const         pinstSrc,
                                    PIB* const          ppibSrc,
                                    CJetParam* const    pjetparamDst,
                                    INST* const         pinstDst,
                                    PIB* const          ppibDst );
        static ERR CloneString(     CJetParam* const    pjetparamSrc,
                                    INST* const         pinstSrc,
                                    PIB* const          ppibSrc,
                                    CJetParam* const    pjetparamDst,
                                    INST* const         pinstDst,
                                    PIB* const          ppibDst );
        static ERR IllegalClone(    CJetParam* const    pjetparamSrc,
                                    INST* const         pinstSrc,
                                    PIB* const          ppibSrc,
                                    CJetParam* const    pjetparamDst,
                                    INST* const         pinstDst,
                                    PIB* const          ppibDst );
};


VOID FixDefaultSystemParameters();




const BYTE  fDBMSPARAMCircularLogging   = 0x1;

PERSISTED
struct DBMS_PARAM
{
    CHAR                                szSystemPathDebugOnly[261];
    CHAR                                szLogFilePathDebugOnly[261];
    BYTE                                fDBMSPARAMFlags;

    UnalignedLittleEndian< ULONG >      le_lSessionsMax;
    UnalignedLittleEndian< ULONG >      le_lOpenTablesMax;
    UnalignedLittleEndian< ULONG >      le_lVerPagesMax;
    UnalignedLittleEndian< ULONG >      le_lCursorsMax;
    UnalignedLittleEndian< ULONG >      le_lLogBuffers;
    UnalignedLittleEndian< ULONG >      le_lcsecLGFile;
    UnalignedLittleEndian< ULONG >      le_ulCacheSizeMax;

    BYTE                                rgbReserved[16];
};

#include <poppack.h>


const INT       cFCBBuckets = 257;
const ULONG     cFCBPurgeAboveThresholdInterval     = 500;

enum TERMTYPE {
        termtypeCleanUp,
        termtypeNoCleanUp,
        termtypeError,
        termtypeRecoveryQuitKeepAttachments
    };



INLINE UINT UiHashIfmpPgnoFDP( IFMP ifmp, PGNO pgnoFDP )
{


    return UINT( pgnoFDP + ( ifmp << 13 ) + ( pgnoFDP >> 17 ) );
}

class FCBHashKey
{
    public:

        FCBHashKey()
        {
        }

        FCBHashKey( IFMP ifmpIn, PGNO pgnoFDPIn )
            :   m_ifmp( ifmpIn ),
                m_pgnoFDP( pgnoFDPIn )
        {
        }

        FCBHashKey( const FCBHashKey &src )
        {
            m_ifmp = src.m_ifmp;
            m_pgnoFDP = src.m_pgnoFDP;
        }

        const FCBHashKey &operator =( const FCBHashKey &src )
        {
            m_ifmp = src.m_ifmp;
            m_pgnoFDP = src.m_pgnoFDP;

            return *this;
        }

    public:

        IFMP    m_ifmp;
        PGNO    m_pgnoFDP;
};

class FCBHashEntry
{
    public:

        FCBHashEntry()
        {
        }

        FCBHashEntry( PGNO pgnoFDP, FCB *pFCBIn )
            :   m_pgnoFDP( pgnoFDP ),
                m_pfcb( pFCBIn )
        {
            Assert( pfcbNil != pFCBIn );
        }

        FCBHashEntry( const FCBHashEntry &src )
        {
            m_pgnoFDP = src.m_pgnoFDP;
            m_pfcb = src.m_pfcb;
            Assert( pfcbNil != src.m_pfcb );
        }

        const FCBHashEntry &operator =( const FCBHashEntry &src )
        {
            m_pgnoFDP = src.m_pgnoFDP;
            m_pfcb = src.m_pfcb;
            Assert( pfcbNil != src.m_pfcb );

            return *this;
        }

    public:

        PGNO    m_pgnoFDP;
        FCB     *m_pfcb;
};

typedef CDynamicHashTable< FCBHashKey, FCBHashEntry > FCBHash;

extern SIZE_T OffsetOfTrxOldestILE();

extern IFMP g_ifmpMax;



class CESESnapshotSession
{
public:

    CESESnapshotSession( const JET_OSSNAPID snapId ):
        m_idSnapshot( snapId ),
        m_state( stateStart ),
        m_thread( 0 ),
        m_errFreeze ( JET_errSuccess ),
        m_asigSnapshotThread( CSyncBasicInfo( _T( "asigSnapshotThread" ) ) ),
        m_asigSnapshotStarted( CSyncBasicInfo( _T( "asigSnapshotStarted" ) ) ),
        m_fFreezeAllInstances ( fFalse ),
        m_ipinstCurrent ( 0 ),
        m_fFlags ( 0 )
        { }

    typedef enum {
        stateStart,
        statePrepare,
        stateFreeze,
        stateThaw,
        stateTimeOut,
        stateAllowLogTruncate,
        stateLogTruncated,
#ifdef OS_SNAPSHOT_ALLOW_TRUNCATE_ONLY
        stateLogTruncatedNoFreeze,
#endif
        stateEnd,
        stateAbort } SNAPSHOT_STATE;

private:
    CESESnapshotSession():
        m_state( stateStart ),
        m_thread( 0 ),
        m_errFreeze ( JET_errSuccess ),
        m_asigSnapshotThread( CSyncBasicInfo( _T( "asigSnapshotThread" ) ) ),
        m_asigSnapshotStarted( CSyncBasicInfo( _T( "asigSnapshotStarted" ) ) ),
        m_fFreezeAllInstances ( fFalse ),
        m_ipinstCurrent ( 0 ),
        m_fFlags ( 0 )
        { }

public:
    BOOL    FCanSwitchTo( const SNAPSHOT_STATE stateNew ) const ;
    SNAPSHOT_STATE  State( ) const { return m_state; }
    BOOL    FFreeze( ) const { return stateFreeze == m_state; }

    JET_OSSNAPID    GetId( );
    void    SwitchTo( const SNAPSHOT_STATE stateNew );

    void    SetInitialFreeze( const BOOL fFreezeAll );
    ERR     ErrAddInstanceToFreeze( const INT ipinst );

    ERR     ErrStartSnapshotThreadProc( );

    void    StopSnapshotThreadProc( const BOOL fWait = fFalse );

    void    SnapshotThreadProc( const ULONG ulTimeOut );

    ERR     ErrTruncateLogs( INST * pinstTruncate, const JET_GRBIT grbit);

    void    SaveSnapshotInfo(const JET_GRBIT grbit);

    void    ResetBackupInProgress( const INST * pinstLastBackupInProgress );

private:
    ERR     ErrFreeze();
    void    Thaw( const BOOL fTimeOut );

    ERR     ErrFreezeInstance();
    void    ThawInstance( const INST * pinstLastAPI = NULL, const INST * pinstLastCheckpoint = NULL, const INST * pinstLastLGFlush = NULL );


    ERR     ErrFreezeDatabase();
    void    ThawDatabase( const IFMP ifmpLast = g_ifmpMax );

    ERR     SetBackupInProgress();


private:
    
    JET_OSSNAPID        m_idSnapshot;
    
    SNAPSHOT_STATE      m_state;
    THREAD              m_thread;
    ERR                 m_errFreeze;

    CAutoResetSignal    m_asigSnapshotThread;
    CAutoResetSignal    m_asigSnapshotStarted;


private:
    union
    {
        ULONG           m_fFlags;
        struct
        {
            FLAG32      m_fFullSnapshot:1;
            FLAG32      m_fCopySnapshot:1;
            FLAG32      m_fContinueAfterThaw:1;
        };
    };

public:
    void                SetFullSnapshot()               { m_fFullSnapshot = fTrue; }
    void                SetIncrementalSnapshot()        { m_fFullSnapshot = fFalse; }
    BOOL                IsFullSnapshot() const          { return m_fFullSnapshot; }
    BOOL                IsIncrementalSnapshot()  const  { return !m_fFullSnapshot; }
    void                SetCopySnapshot()               { m_fCopySnapshot = fTrue; }
    void                ResetCopySnapshot()             { m_fCopySnapshot = fFalse; }
    BOOL                IsCopySnapshot () const         { return m_fCopySnapshot; }
    void                SetContinueAfterThaw()          { m_fContinueAfterThaw= fTrue; }
    void                ResetContinueAfterThaw()        { m_fContinueAfterThaw = fFalse; }
    BOOL                FContinueAfterThaw() const      { return m_fContinueAfterThaw; }

    BOOL                FFreezeInstance( const INST * pinst ) const;

private:
    BOOL                m_fFreezeAllInstances;
    INT                 m_ipinstCurrent;
    INST *              GetFirstInstance();
    INST *              GetNextInstance();
    INST *              GetNextNotNullInstance();

    void                SetFreezeInstances();



public:

    static SIZE_T OffsetOfILE() { return offsetof( CESESnapshotSession, m_ile ); }

    static ERR          ErrAllocSession( CESESnapshotSession ** ppSession );
    static ERR          ErrGetSessionByID( JET_OSSNAPID snapId, CESESnapshotSession ** ppSession );
    static ERR          RemoveSession( JET_OSSNAPID snapId );
    static void         RemoveSession( CESESnapshotSession * pSession );
    static BOOL         FMember( CESESnapshotSession * pSession );

public:
    typedef CInvasiveList< CESESnapshotSession, CESESnapshotSession::OffsetOfILE > CESESnapshotSessionList;

private:
    CESESnapshotSessionList::CElement               m_ile;
    static DWORD                                    g_cListEntries;

public:
    static void     SnapshotCritEnter() { CESESnapshotSession::g_critOSSnapshot.Enter(); }
    static void     SnapshotCritLeave() { CESESnapshotSession::g_critOSSnapshot.Leave(); }

    static ERR      ErrInit();
    static void     Term();
    
private:
    static CCriticalSection g_critOSSnapshot;

    static JET_OSSNAPID g_idSnapshot;


    static const INST * const g_pinstInvalid;

};

ERR ErrSNAPInit();
void SNAPTerm();

class SCRUBDB;
typedef struct tagLGSTATUSINFO LGSTATUSINFO;
PERSISTED struct CHECKPOINT_FIXED;
struct LOG_VERIFY_STATE;

typedef struct
{
    BOOL            fInUse;
    BOOL            fDatabase;
    BOOL            fIsLog;
    LOG_VERIFY_STATE *pLogVerifyState;
    IFileAPI        *pfapi;
    IFMP            ifmp;
    QWORD           ib;
    QWORD           cb;
    WCHAR *         wszFileName;
    LONG            lGeneration;
    DBFILEHDR *     pdbfilehdr;
} RHF;
#define crhfMax 1

class BACKUP_CONTEXT : public CZeroInit
{
public:
    BACKUP_CONTEXT( INST * pinst );

    ERR ErrBKBackup(
        const WCHAR *wszBackup,
        JET_GRBIT grbit,
        JET_PFNINITCALLBACK pfnStatus,
        void * pfnStatusContext );

    ERR ErrBKBeginExternalBackup( JET_GRBIT grbit, ULONG lgenFirst = 0, ULONG lgenLast = 0 );
    ERR ErrBKGetAttachInfo(
        _Out_opt_z_bytecap_post_bytecount_(cbMax, *pcbActual) PWSTR szDatabases,
        ULONG cbMax,
        ULONG *pcbActual );
    ERR ErrBKOpenFile(
            const WCHAR                 *wszFileName,
            JET_HANDLE                  *phfFile,
            QWORD                       ibRead,
            ULONG                       *pulFileSizeLow,
            ULONG                       *pulFileSizeHigh,
            const BOOL                  fIncludePatch );
    ERR ErrBKReadFile(
            JET_HANDLE                  hfFile,
            VOID                        *pv,
            ULONG                       cbMax,
            ULONG                       *pcbActual );
    ERR ErrBKCloseFile( JET_HANDLE hfFile );

    ERR ErrBKGetLogInfo(
            __out_bcount_part_opt(cbMax, *pcbActual) PWSTR wszzLogs,
            __in ULONG                  cbMax,
            __out ULONG                 *pcbActual,
            JET_LOGINFO_W               *pLogInfo,
            const BOOL                  fIncludePatch );

    ERR ErrBKGetTruncateLogInfo(
            __out_bcount_part_opt(cbMax, *pcbActual) PWSTR wszzLogs,
            __in ULONG                  cbMax,
            __out ULONG                 *pcbActual );

    ERR ErrBKTruncateLog();

    ERR ErrBKUpdateHdrBackupInfo(
            const IFMP              ifmp,
            DBFILEHDR::BKINFOTYPE   bkinfoType,
            const BOOL              fLogOnly,
            const BOOL              fLogTruncated,
            const INT               lgenLow,
            const INT               lgenHigh,
            const LGPOS *           plgposMark,
            const LOGTIME *         plogtimeMark,
            BOOL *                  pfLogged );
    ERR ErrBKExternalBackupCleanUp( ERR error, const JET_GRBIT grbit = 0 );

    ERR ErrBKOSSnapshotTruncateLog( const JET_GRBIT grbit );
    void BKOSSnapshotSaveInfo( const BOOL fIncremental, const BOOL fCopy = fFalse );

    ERR ErrBKOSSnapshotStopLogging( const BOOL fIncremental );
    VOID BKOSSnapshotResumeLogging();

    typedef enum
    {
        backupStateNotStarted,
        backupStateDatabases,
        backupStateLogsAndPatchs,
        backupStateDone,
    } BACKUP_STATUS;

     VOID BKSetBackupStatus( BACKUP_STATUS status )
    {
        m_fBackupStatus = status;
    }

    typedef enum
    {
        backupLocalOnly,
        backupAny
    } BACKUP_PROGRESS;

    BOOL FBKBackupInProgress( BACKUP_PROGRESS fLocalOnly = backupAny ) const
    {
        Assert( !m_fBackupInProgressLocal || m_fBackupInProgressAny );

        if ( fLocalOnly == backupLocalOnly )
        {
            return m_fBackupInProgressLocal;
        }
        return m_fBackupInProgressAny;
    }

    VOID BKLockBackup()
    {
        m_critBackupInProgress.Enter();
    }

    VOID BKUnlockBackup()
    {
        m_critBackupInProgress.Leave();
    }

    VOID BKInitForSnapshot( BOOL fIncrementalSnapshot, LONG lgenCheckpoint );

    VOID BKSetFlags( BOOL fFullSnapshot )
    {
        m_fStopBackup = fFalse;
        m_fBackupFull = fFullSnapshot;
        m_fBackupInProgressAny = fTrue;
        m_fBackupInProgressLocal = fTrue;
    }

    VOID BKResetFlags()
    {
        m_fBackupInProgressAny = fFalse;
        m_fBackupInProgressLocal = fFalse;
        m_fBackupFull = fFalse;
        m_fStopBackup = fFalse;
    }

    VOID BKAssertNoBackupInProgress()
    {
        Assert( m_fBackupInProgressAny == fFalse );
        Assert( m_fBackupInProgressLocal == fFalse );
        Assert( m_fStopBackup == fFalse );
        Assert( m_fBackupStatus == backupStateNotStarted );
    }

    ERR ErrBKBackupPibInit();

    VOID BKBackupPibTerm();

    VOID BKSetStopBackup( BOOL fValue )
    {
        m_fStopBackup = fValue;
    }

    VOID BKCopyLastBackupStateFromCheckpoint( const CHECKPOINT_FIXED * pcheckpoint );
    VOID BKCopyLastBackupStateToCheckpoint( CHECKPOINT_FIXED * pcheckpoint ) const;

    VOID BKSetLgposFullBackup( const LGPOS lgpos )
    {
        m_lgposFullBackup = lgpos;
    }

    VOID BKSetLgposIncBackup( const LGPOS lgpos )
    {
        m_lgposIncBackup = lgpos;
    }

    LONG BKLgenCopyMic() const
    {
        return m_lgenCopyMic;
    }

    BOOL FBKLogsTruncated() const
    {
        return m_fLogsTruncated;
    }
    VOID BKSetLogsTruncated( BOOL fTruncated )
    {
        m_fLogsTruncated = fTruncated;
    }

    VOID BKSetIsSuspended( BOOL fSuspended )
    {
        Assert( m_critBackupInProgress.FOwner() );
        m_fCtxIsSuspended = fSuspended;
    }

    BOOL FBKBackupIsInternal() const
    {
        return m_fBackupIsInternal;
    }

#ifdef DEBUGGER_EXTENSION
    VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset ) const;
#endif

private:

    INST            *m_pinst;

    BACKUP_STATUS m_fBackupStatus;

    LONG            m_lgenCopyMic;
    LONG            m_lgenCopyMac;

    LGPOS           m_lgposFullBackup;
    LOGTIME         m_logtimeFullBackup;

    LGPOS           m_lgposIncBackup;
    LOGTIME         m_logtimeIncBackup;

    BOOL            m_fStopBackup;
    BOOL            m_fBackupFull;

    PIB             *m_ppibBackup;

    CCriticalSection    m_critBackupInProgress;
    BOOL            m_fBackupInProgressAny;
    BOOL            m_fBackupInProgressLocal;

    LONG            m_lgenDeleteMic;
    LONG            m_lgenDeleteMac;

    BOOL            m_fLogsTruncated;

    BOOL            m_fBackupIsInternal;
    LGPOS           m_lgposFullBackupMark;
    LOGTIME         m_logtimeFullBackupMark;
    SIZE_T          m_cGenCopyDone;
    BOOL            m_fBackupBeginNewLogFile;

    BOOL            m_fCtxIsSuspended;


    SCRUBDB         *m_pscrubdb;
    ULONG_PTR       m_ulSecsStartScrub;
    DBTIME          m_dbtimeLastScrubNew;

    RHF             m_rgrhf[crhfMax];
    INT             m_crhfMac;

#ifdef DEBUG
    BOOL            m_fDBGTraceBR;
    LONG            m_cbDBGCopied;
#endif

    VOID BKIOSSnapshotGetFreezeLogRec( LGPOS* const plgposFreezeLogRec );

    ERR ErrBKIPrepareLogFiles(
            JET_GRBIT                   grbit,
            __in PCWSTR                  wszLogFilePath,
            __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) PWSTR   wszPathJetChkLog,
            __in PCWSTR                  wszBackupPath );

    ERR ErrBKIReadPages(
                RHF *prhf,
                VOID *pvPageMin,
                INT cpage,
                INT *pcbActual
#ifdef DEBUG
                , __out_bcount(cbLGDBGPageList) PSTR szLGDBGPageList,
                ULONG cbLGDBGPageList
#endif
                );

    ERR ErrBKICopyFile(
        const WCHAR *wszFileName,
        const WCHAR *wszBackup,
        JET_PFNINITCALLBACK pfnStatus,
        void * pvStatusContext,
        const BOOL   fOverwriteExisting = fFalse );
    ERR ErrBKIPrepareDirectory( __in PCWSTR wszBackup, __out_bcount(cbBackupPath) PWSTR wszBackupPath, const size_t cbBackupPath, JET_GRBIT grbit );
    ERR ErrBKIPromoteDirectory( __in PCWSTR wszBackup, __out_bcount(OSFSAPI_MAX_PATH * sizeof(WCHAR)) PWSTR wszBackupPath, JET_GRBIT grbit );
    ERR ErrBKICleanupDirectory( __in PCWSTR wszBackup, __out_bcount(cbBackupPath) PWSTR wszBackupPath, size_t cbBackupPath );

    ERR ErrBKIPrepareLogInfo();

    ERR ErrBKIGetLogInfo(
            const ULONG                 ulGenLow,
            const ULONG                 ulGenHigh,
            const BOOL                  fIncludePatch,
            __out_bcount_part_opt(cbMax, *pcbActual) PWSTR wszzLogs,
            __in ULONG                  cbMax,
            __out_opt ULONG             *pcbActual,
            JET_LOGINFO_W               *pLogInfo );

    VOID BKIMakeDbTrailer(const IFMP ifmp,  BYTE *pvPage);
    ERR ErrBKICheckLogsForIncrementalBackup( LONG lGenMinExisting );

    VOID BKIGetPatchName( __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) PWSTR wszPatch, PCWSTR wszDatabaseName, __in PCWSTR wszDirectory = NULL );

};


enum : BYTE
{
    eSequenceStart = 0
};

enum : BYTE
{
    eInitOslayerDone = 1,
    eInitIsamSystemDone,
    eInitSelectAllocInstDone,
    eInitVariableInitDone,
    eInitConfigInitDone,
    eInitBaseLogInitDone,
    eInitLogReadAndPatchFirstLog,
    eInitLogRecoverySilentRedoDone,
    eInitLogRecoveryVerboseRedoDone,
    eInitLogRecoveryReopenDone,
    eInitLogRecoveryUndoDone,
    eInitLogRecoveryBfFlushDone,
    eInitLogInitDone,
    eInitSubManagersDone,
    eInitBigComponentsDone,

    eInitDone,

    eInitSeqMax
};

enum : BYTE
{
    eTermWaitedForUserWorkerThreads = 1,
    eTermWaitedForSystemWorkerThreads,
    eTermVersionStoreDone,
    eTermRestNoReturning,
    eTermBufferManagerFlushDone,
    eTermBufferManagerPurgeDone,
    eTermVersionStoreDoneAgain,
    eTermCleanupAndResetting,
    eTermLogFlushed,
    eTermLogFsFlushed,
    eTermCheckpointFlushed,
    eTermLogTasksTermed,
    eTermLogFilesClosed,
    eTermLogTermDone,

    eTermDone,

    eTermSeqMax
};

enum : BYTE
{
    eCreateInitVariableDone = 1,
    eCreateLogged,
    eCreateHeaderInitialized,
    eCreateDatabaseZeroed,
    eCreateDatabaseSpaceInitialized,
    eCreateDatabaseCatalogInitialized,
    eCreateDatabaseInitialized,
    eCreateFinishLogged,
    eCreateDatabaseDirty,
    eCreateLatentUpgradesDone,

    eCreateDone,

    eCreateSeqMax
};

enum : BYTE
{
    eAttachInitVariableDone = 1,
    eAttachReadDBHeader,
    eAttachToLogStream,
    eAttachIOOpenDatabase,
    eAttachFastReAttachAcquireFmp,
    eAttachLeakReclaimerDone,
    eAttachShrinkDone,
    eAttachFastBaseReAttachDone,
    eAttachSlowBaseAttachDone,
    eAttachCreateMSysObjids,
    eAttachCreateMSysLocales,
    eAttachDeleteMSysUnicodeFixup,
    eAttachCheckForOutOfDateLocales,
    eAttachDeleteUnicodeIndexes,
    eAttachUpgradeUnicodeIndexes,

    eAttachDone,

    eAttachSeqMax
};

enum : BYTE
{
    eDetachQuiesceSystemTasks = 1,
    eDetachSetDetaching,
    eDetachVersionStoreDone,
    eDetachStopSystemTasks,
    eDetachVersionStoreReallyDone,
    eDetachBufferManagerFlushDone,
    eDetachBufferManagerPurgeDone,
    eDetachLogged,
    eDetachUpdateHeader,
    eDetachIOClose,

    eDetachDone,

    eDetachSeqMax
};

const BYTE eForwardLogBaselineStep = eInitBaseLogInitDone;


enum IsamSequenceDiagLogType : BYTE
{
    isdltypeNone = 0,
    isdltypeTest = 1,
    isdltypeOsinit = 2,
    isdltypeInit = 3,
    isdltypeTerm = 4,
    isdltypeCreate = 5,
    isdltypeAttach = 6,
    isdltypeDetach = 7,
    isdltypeLogFile = 8,
};


class CIsamSequenceDiagLog : CZeroInit
{

    

public:


    struct FixedInitData
    {
        LGPOS           lgposRecoveryStartMin;
        LGPOS           lgposRecoverySilentRedoEnd;
        LGPOS           lgposRecoveryVerboseRedoEnd;
        LGPOS           lgposRecoveryUndoEnd;
        HRT             hrtRecoveryForwardLogs;
        LGPOS           lgposRecoveryForwardLogs;
        INT             cReInits;

    };

    struct FixedTermData
    {
        LGPOS           lgposTerm;
    };

    struct FixedCreateData
    {
        LGPOS           lgposCreate;
    };

    struct FixedAttachData
    {
        LGPOS           lgposAttach;
    };

    struct FixedDetachData
    {
        LGPOS           lgposDetach;
    };

    typedef union _SEQFIXEDDATA
    {
        FixedInitData       sInitData;
        FixedTermData       sTermData;
        FixedCreateData     sCreateData;
        FixedAttachData     sAttachData;
        FixedDetachData     sDetachData;
    }
    SEQFIXEDDATA;

private:

    static INT                  s_iDumpVersion;

    static SEQFIXEDDATA         s_uDummyFixedData;

    struct SEQDIAGINFO
    {
        INT                 seq;
        HRT                 hrtEnd;
        __int64             cCallbacks;
        double              secInCallback;
        __int64             cThrottled;
        double              secThrottled;
        DATETIME            dtm;
        JET_THREADSTATS4    thstat;
        __int64             cbCacheMem;
        MEMSTAT             memstat;
    };

private:

    IsamSequenceDiagLogType             m_isdltype;

    BYTE                                m_fPreAllocatedSeqDiagInfo:1;
    BYTE                                m_fAllocFailure:1;
    BYTE                                m_fInitialized:1;
    BYTE                                m_grbitReserved:6;
    BYTE                                m_cseqMax;
    BYTE                                m_cseqMac;
    BYTE                                rgbReserved[2];

    SEQFIXEDDATA *                      m_pFixedData;
    SEQDIAGINFO *                       m_rgDiagInfo;



#ifdef DEBUG
    DWORD                               m_tidOwner;
#endif



    inline bool FValidSequence_( _In_ const INT seq ) const
    {
        Assert( m_cseqMax >= 2 );
        return seq >= 0 && seq < (INT)m_cseqMax;
    }

    inline bool FTriggeredSequence_( _In_ const INT seq ) const
    {
        Expected( FActiveSequence() );
        Assert( m_rgDiagInfo );
        return FValidSequence_( seq ) && seq < m_cseqMac && m_rgDiagInfo[seq].dtm.year != 0;
    }

    BOOL FAllocatedFixedData_() const
    {
        return m_pFixedData != &s_uDummyFixedData;
    }

public:

    CIsamSequenceDiagLog( const IsamSequenceDiagLogType isdltype ) :
        CZeroInit( sizeof( CIsamSequenceDiagLog ) )
    {
        Assert( m_isdltype == isdltypeNone );
        Assert( m_fAllocFailure == fFalse );
        Assert( m_grbitReserved == 0 );
        Assert( m_cseqMax == 0 );
        Assert( m_cseqMac == 0 );
        Assert( m_rgDiagInfo == NULL );
        Assert( m_tidOwner == 0x0 );
        Expected( m_pFixedData == NULL );
        m_pFixedData = &s_uDummyFixedData;
        m_isdltype = isdltype;
        Assert( !m_fInitialized );
    }

    ~CIsamSequenceDiagLog()
    {
        Assert( m_rgDiagInfo == NULL );
        Assert( !FAllocatedFixedData_() );
        Assert( m_pFixedData == &s_uDummyFixedData );
    }


    void InitSequence( _In_ const IsamSequenceDiagLogType isdltype, _In_range_( 2, 128 ) const BYTE cseqMax );

    void InitConsumeSequence( _In_ const IsamSequenceDiagLogType isdltype, _In_ CIsamSequenceDiagLog * const ptmseqSeed, _In_range_( 3, 128 ) const BYTE cseqMax );

    static const ULONG cbSingleStepPreAlloc = sizeof(SEQDIAGINFO) * 2;
    void InitSingleStep( _In_ const IsamSequenceDiagLogType isdltype, void * const pvPreAllocSequence, const ULONG cbPreAllocSequence )
    {
        Assert( cbPreAllocSequence >= cbSingleStepPreAlloc );
        m_rgDiagInfo = (SEQDIAGINFO*) pvPreAllocSequence;
        m_fPreAllocatedSeqDiagInfo = fTrue;
        InitSequence( isdltype, 2 );
    }

    void ReThreadSequence()
    {
        Assert( FActiveSequence() );
        OnDebug( m_tidOwner = DwUtilThreadId() );
    }

    void TermSequence();

public:


    BOOL FActiveSequence() const
    {
        return m_fInitialized;
    }

    BOOL FTriggeredStep( _In_ const INT seq ) const
    {
        if ( m_fAllocFailure && m_rgDiagInfo == NULL )
        {
            return fFalse;
        }
        return FTriggeredSequence_( seq );
    }

    BYTE EFinalStep() const
    {
        return m_cseqMax - 1;
    }

#ifdef DEBUG
    BOOL FFailedAllocate() const
    {
        return m_fAllocFailure;
    }
#endif


    SEQFIXEDDATA& FixedData() const
    {
        Expected( FActiveSequence() );
        return *m_pFixedData;
    }

    void Trigger( _In_ const BYTE seqTrigger );
    void AddCallbackTime( const double secsCallback, const __int64 cCallbacks = 1 );
    void AddThrottleTime( const double secsThrottle, const __int64 cThrottled = 1 );
    double GetCallbackTime( __int64 *pcCallbacks );
    double GetThrottleTime( __int64 *pcThrottled );


    __int64 UsecTimer( _In_ INT seqBegin, _In_ const INT seqEnd ) const;

    ULONG CbSprintFixedData() const
    {
        Assert( m_tidOwner == DwUtilThreadId() );
        const ULONG cb = 600;
        return cb;
    }
    void SprintFixedData( _Out_writes_bytes_(cbFixedData) WCHAR * wszFixedData, _In_range_( 4, 4000 ) ULONG cbFixedData) const;

    ULONG CbSprintTimings() const
    {
        Assert( m_tidOwner == DwUtilThreadId() );
        const ULONG cb = ( (ULONG)m_cseqMax * 213  + 10  ) * sizeof(WCHAR);
        return cb;
    }

    void SprintTimings( _Out_writes_bytes_(cbTimeSeq) WCHAR * wszTimeSeq, _In_range_( 4, 4000 ) ULONG cbTimeSeq ) const;
    
};


static const INT perfStatusRecoveryRedo     = 1;
static const INT perfStatusRecoveryUndo     = 2;
static const INT perfStatusRuntime          = 3;
static const INT perfStatusTerm             = 4;
static const INT perfStatusError            = 5;


class INST
    :   public CZeroInit
{
    friend class APICALL_INST;
public:

    INST( INT iInstance );
    ~INST();

#pragma push_macro( "new" )
#undef new
    private:
        void* operator new[]( size_t );
        void operator delete[]( void* );
    public:
        void* operator new( size_t cbAlloc )
        {
            return RESINST.PvRESAlloc_( SzNewFile(), UlNewLine() );
        }
        void operator delete( void* pv )
        {
            RESINST.Free( pv );
        }
#pragma pop_macro( "new" )

    WCHAR *             m_wszInstanceName;
    WCHAR *             m_wszDisplayName;
    const INT           m_iInstance;

    LONG                m_cSessionInJetAPI;
    BOOL                m_fJetInitialized;
    BOOL                m_fTermInProgress;
    BOOL                m_fTermAbruptly;
    INST_STINIT         m_fSTInit;
    INT                 m_perfstatusEvent;
    
    BOOL                m_fBackupAllowed;
    volatile BOOL       m_fStopJetService;
    ERR                 m_errInstanceUnavailable;
    BOOL                m_fNoWaypointLatency;
    CESESnapshotSession *       m_pOSSnapshotSession;
    __int64             m_ftInit;


    CJetParam*          m_rgparam;

    JET_GRBIT           m_grbitCommitDefault;
    CJetParam::Config   m_config;


    CCriticalSection    m_critLV;
    PIB *               m_ppibLV;


    VER                 *m_pver;
    LOG                 *m_plog;
    BACKUP_CONTEXT      *m_pbackup;


    IFMP                m_mpdbidifmp[ dbidMax ];


    UPDATEID            m_updateid;


    volatile TRX        m_trxNewest;
    CCriticalSection    m_critPIB;
    RWLPOOL< PIB >      m_rwlpoolPIBTrx;
    CResource           m_cresPIB;
    PIB* volatile       m_ppibGlobal;
    CAutoResetSignal    m_sigTrxOldestDuringRecovery;
    BOOL                m_fTrxNewestSetByRecovery;

    CRevertSnapshot     *m_prbs;
    RBSCleaner          *m_prbscleaner;

private:
    volatile TRX        m_trxOldestCached;
public:
    

    CResource           m_cresFCB;
    CResource           m_cresTDB;
    CResource           m_cresIDB;


    FCBHash             *m_pfcbhash;


    CCriticalSection    m_critFCBList;
    FCB                 *m_pfcbList;
    FCB                 *m_pfcbAvailMRU;
    FCB                 *m_pfcbAvailLRU;
    ULONG               m_cFCBAvail;
    volatile ULONG      m_cFCBAlloc;


    CCriticalSection    m_critFCBCreate;


    POSTRACEREFLOG      m_pFCBRefTrace;


    CResource           m_cresFUCB;


    CResource           m_cresSCB;


    CGPTaskManager      m_taskmgr;
    volatile LONG       m_cOpenedSystemPibs;
    volatile LONG       m_cUsedSystemPibs;

    CCriticalSection    m_critOLD;
    OLDDB_STATUS        *m_rgoldstatDB;


    IFileSystemConfiguration*   m_pfsconfig;
    IFileSystemAPI*     m_pfsapi;


    CIrsOpContext **    m_rgpirs;
    CRBSRevertContext*             m_prbsrc;

    JET_PFNINITCALLBACK m_pfnInitCallback;
    void *              m_pvInitCallbackContext;
    BOOL                m_fAllowAPICallDuringRecovery;


    BOOL                m_fFlushLogForCleanThread;

    volatile BOOL       m_fTempDBCreated;

private:
    volatile LONG       m_cIOReserved;
    LONG                m_cNonLoggedIndexCreators;


    const VOID * const  m_rgEDBGGlobals;
    const VOID * const  m_rgfmp;
    INST ** const       m_rgpinst;

    struct ListNodePPIB
    {
        ListNodePPIB *  pNext;
        PIB *           ppib;
    };
    ListNodePPIB *      m_plnppibBegin;
    ListNodePPIB *      m_plnppibEnd;
    CCriticalSection    m_critLNPPIB;

    CCriticalSection    m_critTempDbCreate;

public:
    static LONG         iInstanceActiveMin;
    static LONG         iInstanceActiveMac;

    static LONG         cInstancesCounter;


public:
    class PLS
    {
        public:

            PLS() :
                m_rwlPIBTrxOldest( CLockBasicInfo( CSyncBasicInfo( szTrxOldest ), rankTrxOldest, CLockDeadlockDetectionInfo::subrankNoDeadlock ) )
            {
            }
            ~PLS()
            {
            }

        public:

            CReaderWriterLock   m_rwlPIBTrxOldest;
            CInvasiveList< PIB, OffsetOfTrxOldestILE >
                                m_ilTrxOldest;
            BYTE                m_rgbPad[ 256 - sizeof( CCriticalSection ) - sizeof( CInvasiveList< PIB, OffsetOfTrxOldestILE > ) ];
    };

    size_t              m_cpls;
    PLS*                m_rgpls;

    PLS* Ppls();
    PLS* Ppls( const size_t iProc );


    TICK                        m_tickStopped;
    TICK                        m_tickStopCanceled;
    JET_GRBIT                   m_grbitStopped;
    BOOL                        m_fCheckpointQuiesce;

    COSEventTraceIdCheck        m_traceidcheckInst;


    BOOL                        m_rgfStaticBetaFeatures[EseFeatureMax];

    ULONG                       m_dwLCMapFlagsDefault;
    WCHAR                       m_wszLocaleNameDefault[NORM_LOCALE_NAME_MAX_LENGTH];

    CIsamSequenceDiagLog        m_isdlInit;
    CIsamSequenceDiagLog        m_isdlTerm;

private:
    ERR ErrAPIAbandonEnter_( const LONG lOld );

    static ERR ErrINSTCreateTempDatabase_( void* const pvInst );

public:

    friend
    ERR ErrNewInst(
        INST **         ppinst,
        const WCHAR *   wszInstanceName,
        const WCHAR *   wszDisplayName,
        INT *           pipinst );

    ERR ErrINSTInit();

    ERR ErrINSTTerm( TERMTYPE termtype );

    ERR ErrINSTCreateTempDatabase();

    VOID SaveDBMSParams( DBMS_PARAM *pdbms_param );
    VOID RestoreDBMSParams( DBMS_PARAM *pdbms_param );
    ERR ErrAPIEnter( const BOOL fTerminating );
private:
    ERR ErrAPIEnterFromTerm();
    ERR ErrAPIEnterForInit();
    ERR ErrAPIEnterWithoutInit( const BOOL fAllowInitInProgress );
public:
    VOID APILeave();

    BOOL    FInstanceUnavailable() const;
    ERR     ErrInstanceUnavailableErrorCode() const;
    VOID    SetInstanceUnavailable( const ERR err );

    ERR     ErrCheckForTermination() const;

    VOID SetInstanceStatusReporting( const INT iInstance, const INT perfStatus );
    VOID SetStatusRedo();
    VOID SetStatusUndo();
    VOID SetStatusRuntime();
    VOID SetStatusTerm();
    VOID SetStatusError();

    VOID TraceStationId( const TraceStationIdentificationReason tsidr );

    const enum {
            maskAPILocked               = 0xFF000000,
            maskAPISessionCount         = 0x00FFFFFF,
            maskAPIReserved             = 0x80000000,
            fAPIInitializing            = 0x10000000,
            fAPITerminating             = 0x20000000,
            fAPIRestoring               = 0x40000000,
            fAPICheckpointing           = 0x01000000,
            fAPIDeleting                = 0x02000000,
        };

    BOOL APILock( const LONG fAPIAction );
    VOID APIUnlock( const LONG fAPIAction );

    VOID MakeUnavailable();

    BOOL FRecovering() const;
    BOOL FComputeLogDisabled();

    FCB **PpfcbAvailMRU()                       { return &m_pfcbAvailMRU; }
    FCB **PpfcbAvailLRU()                       { return &m_pfcbAvailLRU; }

    ERR ErrGetSystemPib( PIB **pppib );
    VOID ReleaseSystemPib( PIB *ppib );
    CReaderWriterLock& RwlTrx( PIB* const ppib )    { return m_rwlpoolPIBTrx.Rwl( ppib ); }
    CGPTaskManager& Taskmgr() { return m_taskmgr; }
    BOOL FSetInstanceName( PCWSTR wszInstanceName );
    BOOL FSetDisplayName( PCWSTR wszDisplayName );

    VOID WaitForDBAttachDetach( );
    ULONG CAttachedUserDBs( ) const;

    LONG CNonLoggedIndexCreators() const        { return m_cNonLoggedIndexCreators; }
    VOID IncrementCNonLoggedIndexCreators()     { AtomicIncrement( &m_cNonLoggedIndexCreators ); }
    VOID DecrementCNonLoggedIndexCreators()     { AtomicDecrement( &m_cNonLoggedIndexCreators ); }

    TRX TrxOldestCached() const { return m_trxOldestCached; }
    void SetTrxOldestCached(const TRX trx) { m_trxOldestCached = trx; }
    
    ERR ErrReserveIOREQ();
    VOID UnreserveIOREQ();

#ifdef DEBUGGER_EXTENSION
    VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif

    static VOID EnterCritInst();
    static VOID LeaveCritInst();

#ifdef DEBUG
    static BOOL FOwnerCritInst();
#endif

    static INST * GetInstanceByName( PCWSTR wszInstanceName );
    static INST * GetInstanceByFullLogPath( PCWSTR wszLogPath );

    static LONG AllocatedInstances() { return cInstancesCounter; }
    static LONG IncAllocInstances() { return AtomicIncrement( &cInstancesCounter ); }
    static LONG DecAllocInstances() { return AtomicDecrement( &cInstancesCounter ); }
    static ERR  ErrINSTSystemInit();
    static VOID INSTSystemTerm();
};

VOID FreePinst( INST *pinst );

extern ULONG    g_cpinstMax;
extern INST **  g_rgpinst;

extern IFileSystemConfiguration* const g_pfsconfigGlobal;

#include "os\perfmon.hxx"
#include "perfctrs.hxx"


INLINE ULONG IpinstFromPinst( INST *pinst )
{
    ULONG   ipinst;

    for ( ipinst = 0; ipinst < g_cpinstMax && pinst != g_rgpinst[ipinst]; ipinst++ )
    {
        NULL;
    }

    EnforceSz( ipinst < g_cpinstMax, "CorruptedInstArrayInIpinstFromPinst" );

    Assert( ipinst + 1 == (ULONG)pinst->m_iInstance );

    return ipinst;
}

PIB * const     ppibSurrogate   = (PIB *)( lBitsAllFlipped << 1 );
const PROCID    procidSurrogate = (PROCID)0xFFFE;
const PROCID    procidReadDuringRecovery = (PROCID)0xFFFD;
const PROCID    procidRCEClean = (PROCID)0xFFFC;
const PROCID    procidRCECleanCallback = (PROCID)0xFFFB;


INLINE void UtilAssertNotInAnyCriticalSection()
{
#ifdef SYNC_DEADLOCK_DETECTION
    AssertSz( !Pcls()->pownerLockHead || Ptls()->fInCallback, "Still in a critical section" );
#endif
}

INLINE void UtilAssertCriticalSectionCanDoIO()
{
#ifdef SYNC_DEADLOCK_DETECTION
    AssertSz(   !Pcls()->pownerLockHead || Pcls()->pownerLockHead->m_plddiOwned->Info().Rank() > rankIO,
                "In a critical section we cannot hold over I/O" );
#endif
}


INLINE BOOL INST::FInstanceUnavailable() const
{
    return ( JET_errSuccess != m_errInstanceUnavailable );
}

INLINE ERR INST::ErrInstanceUnavailableErrorCode() const
{
    Assert( FInstanceUnavailable() );

    return ErrERRCheck(
            ( JET_errLogDiskFull == m_errInstanceUnavailable ) ?
                                JET_errInstanceUnavailableDueToFatalLogDiskFull :
                                JET_errInstanceUnavailable );
}


INLINE ERR INST::ErrCheckForTermination() const
{
    if( m_fStopJetService )
    {
        return ErrERRCheck( JET_errClientRequestToStopJetService );
    }
    return JET_errSuccess;
}

template< class CEntry >
class CSimpleHashTable
{
    public:
        CSimpleHashTable( const ULONG centries, const ULONG ulOffsetOverflowNext ) :
            m_rgpentries( NULL ),
            m_centries( centries ),
            m_ulOffsetOverflowNext( ulOffsetOverflowNext )  {}
        ~CSimpleHashTable()
        {
            if ( NULL != m_rgpentries )
            {
                OSMemoryHeapFree( m_rgpentries );
            }
        }

    private:
        CEntry **       m_rgpentries;
        const ULONG     m_centries;
        const ULONG     m_ulOffsetOverflowNext;

    public:
        ULONG CEntries() const          { Assert( m_centries > 0 ); return m_centries; }

        ERR ErrInit()
        {
            ERR             err;
            const ULONG     cbAlloc = sizeof(CEntry *) * m_centries;

            if ( 0 == m_centries )
            {
                return ErrERRCheck( JET_errInternalError );
            }

            Assert( NULL == m_rgpentries );
            AllocR( m_rgpentries = (CEntry **)PvOSMemoryHeapAlloc( cbAlloc ) );
            memset( m_rgpentries, 0, cbAlloc );

            return JET_errSuccess;
        }

    private:
        CEntry ** PpentryOverflowNext( CEntry * const pentry )
        {
            return (CEntry **)( (BYTE *)pentry + m_ulOffsetOverflowNext );
        }

        CEntry * PentryOverflowNext( CEntry * const pentry )
        {
            return *PpentryOverflowNext( pentry );
        }

    protected:
        VOID InsertEntry( CEntry * pentry, const ULONG ulHash )
        {
            CEntry **   ppentryNext     = PpentryOverflowNext( pentry );

            Assert( NULL == *ppentryNext );
            *ppentryNext = m_rgpentries[ulHash];
            m_rgpentries[ulHash] = pentry;
        }

        VOID RemoveEntry( CEntry * pentry, const ULONG ulHash )
        {
            for ( CEntry ** ppentry = m_rgpentries + ulHash;
                NULL != *ppentry;
                ppentry = PpentryOverflowNext( *ppentry ) )
            {
                if ( *ppentry == pentry )
                {
                    *ppentry = PentryOverflowNext( pentry );
                    *PpentryOverflowNext( pentry ) = NULL;
                    return;
                }
            }

            Assert( fFalse );
        }

        CEntry * PentryOfHash( const ULONG ulHash ) const
        {
            return m_rgpentries[ulHash];
        }
};



extern CJetParam::Config g_config;

inline BOOL FJetConfigLowMemory()
{
    return g_config & JET_configLowMemory;
}

inline BOOL FJetConfigMedMemory()
{
    return g_config & JET_configDynamicMediumMemory;
}

inline BOOL FJetConfigLowPower()
{
    return g_config & JET_configLowPower;
}

inline BOOL FJetConfigSSDProfileIO()
{
    return g_config & JET_configSSDProfileIO;
}

inline BOOL FJetConfigRunSilent()
{
    return g_config & JET_configRunSilent;
}


inline CJetParam* const Param_( const INST* const pinst, const ULONG paramid )
{
    extern CJetParam* const g_rgparam;
    CJetParam*              pjetparam   = NULL;

    Assert( pinst != pinstNil ||
            ( 0 == INST::AllocatedInstances() ) ||
            g_rgparam[ paramid ].FGlobal() );

    if ( pinst == pinstNil || g_rgparam[ paramid ].FGlobal() )
    {
        pjetparam = g_rgparam + paramid;
    }
    else
    {
        pjetparam = pinst->m_rgparam + paramid;
    }

    return pjetparam;
}

inline CJetParam* const Param( const INST* const pinst, const ULONG paramid )
{
    extern CJetParam* const g_rgparam;

    Assert( g_rgparam != NULL );

    Assert( !g_rgparam[ paramid ].FCustomStorage() || paramid == JET_paramConfigStoreSpec );

    Assert( FUp( eDll ) );

    return Param_( pinst, paramid );
}

inline ERR SetParam(
            INST* const         pinst,
            PIB* const          ppib,
            const ULONG paramid,
            const ULONG_PTR     ulParam,
            __in PCWSTR         wszParam )
{
    return Param_( pinst, paramid )->Set( pinst, ppib, ulParam, wszParam );
}

inline BOOL BoolParam( const ULONG paramid )
{
    return (BOOL)Param( pinstNil, paramid )->Value();
}
inline BOOL BoolParam( const INST* const pinst, const ULONG paramid )
{
    return (BOOL)Param( pinst, paramid )->Value();
}
inline JET_GRBIT GrbitParam( const ULONG paramid )
{
    return (JET_GRBIT)Param( pinstNil, paramid )->Value();
}
inline JET_GRBIT GrbitParam( const INST* const pinst, const ULONG paramid )
{
    return (JET_GRBIT)Param( pinst, paramid )->Value();
}
inline ULONG_PTR UlParam( const ULONG paramid )
{
    return (ULONG_PTR)Param( pinstNil, paramid )->Value();
}
inline ULONG_PTR UlParam( const INST* const pinst, const ULONG paramid )
{
    return (ULONG_PTR)Param( pinst, paramid )->Value();
}
inline PCWSTR SzParam( const ULONG paramid )
{
    return (WCHAR*)Param( pinstNil, paramid )->Value();
}
inline PCWSTR SzParam( const INST* const pinst, const ULONG paramid )
{
    return (WCHAR*)Param( pinst, paramid )->Value();
}
inline const void* PvParam( const ULONG paramid )
{
    return (void*)Param( pinstNil, paramid )->Value();
}
inline const void* PvParam( const INST* const pinst, const ULONG paramid )
{
    return (void*)Param( pinst, paramid )->Value();
}

inline BOOL FDefaultParam( const ULONG paramid )
{
    return !Param( pinstNil, paramid )->FWritten();
}
inline BOOL FDefaultParam( const INST* const pinst, const ULONG paramid )
{
    return !Param( pinst, paramid )->FWritten();
}

inline ULONG_PTR UlParamDefault( const ULONG paramid )
{
    return (ULONG_PTR)Param( pinstNil, paramid )->m_valueDefault[ FJetConfigLowMemory() ?  0 : 1 ];
}

extern volatile BOOL    g_fDllUp;

INT CPERFESEInstanceObjects( const INT cUsedInstances );

VOID PERFSetDatabaseNames( IFileSystemAPI* const pfsapi );

LONG CPERFTCEObjects();

ULONG CPERFDatabaseObjects( const ULONG cIfmpsInUse );

ULONG CPERFObjectsMinReq();

class TableClassNamesLifetimeManager
{
    public:
        TableClassNamesLifetimeManager() :
            m_wszTableClassNames( nullptr ),
            m_cchTableClassNames( 0 )
        {
        }

        ~TableClassNamesLifetimeManager()
        {
            AssertSz( !g_fDllUp || !m_wszTableClassNames, "This destructor is intended to be called only when the DLL is being unloaded." );

            delete[] m_wszTableClassNames;
        }

        WCHAR* WszTableClassNames() const
        {
            return m_wszTableClassNames;
        }

        size_t CchTableClassNames() const
        {
            return m_cchTableClassNames;
        }

        JET_ERR ErrAllocateIfNeeded(
            _In_ const size_t cchTableClassNames )
        {
            JET_ERR err = JET_errSuccess;

            if ( !m_wszTableClassNames )
            {
                Assert( m_cchTableClassNames == 0 );
                Alloc( m_wszTableClassNames = new WCHAR[cchTableClassNames] );
                memset( m_wszTableClassNames, 0, cchTableClassNames * sizeof( m_wszTableClassNames[0] ) );
            }

            Assert( m_cchTableClassNames == 0 || m_cchTableClassNames == cchTableClassNames );

            m_cchTableClassNames = cchTableClassNames;
        HandleError:
            return err;
        }

    private:
        WCHAR* m_wszTableClassNames;
        size_t m_cchTableClassNames;
};





LONG CbSYSMaxPageSize();


LONG CbINSTMaxPageSize( const INST * const pinst );

#define g_cbPage    ((LONG)UlParam( JET_paramDatabasePageSize ))

inline LONG CbAssertGlobalPageSizeMatchRTL_( _In_ const LONG cbGlobalPageReplacementSource, _In_ const CHAR * szFile, _In_ const LONG lLine )
{
    if ( g_cbPage != cbGlobalPageReplacementSource )
    {
        CHAR szT[ 140 ];
        OSStrCbFormatA( szT, sizeof(szT), "PageSizeReplaceMismatch-%hs:%d", SzSourceFileName( szFile ), lLine );
        FireWall( szT );
    }
    return g_cbPage;
}
#define CbAssertGlobalPageSizeMatchRTL( cbGlobalPageReplacementSource )    CbAssertGlobalPageSizeMatchRTL_( cbGlobalPageReplacementSource, __FILE__, __LINE__ )

inline bool FIsValidPageSize( ULONG cbPage )
{
    switch ( cbPage )
    {
        case 2 * 1024:
        case 4 * 1024:
        case 8 * 1024:
        case 16 * 1024:
        case 32 * 1024:
            return true;

        default:
            return false;
    }
}

static bool FIsSmallPage( ULONG cbPage )    { return cbPage <= 1024 * 8; }
static bool FIsSmallPage( void )            { return FIsSmallPage( g_cbPage ); }

inline INT CbKeyMostForPage( const LONG cbPage )
{
    switch ( cbPage )
    {
        case 2048:
            return JET_cbKeyMost2KBytePage;
        case 4096:
            return JET_cbKeyMost4KBytePage;
        case 8192:
            return JET_cbKeyMost8KBytePage;
        case 1024 * 16:
            return JET_cbKeyMost16KBytePage;
        case 1024 * 32:
            return JET_cbKeyMost32KBytePage;
        default:
            AssertSz( fFalse, "Unexpected page size" );
            return JET_cbKeyMost_OLD;
    }
}
inline INT CbKeyMostForPage()
{
    return CbKeyMostForPage( g_cbPage );
}

INLINE CPG CpgEnclosingCb( ULONG cbPage, QWORD cb )   { return (CPG) ( ( cb / cbPage ) + ( ( cb % cbPage ) ? 1 : 0 ) ); }


#define case_AllDatabaseStorageCorruptionErrs   \
    case JET_errReadVerifyFailure:              \
    case JET_errDiskReadVerificationFailure:    \
    case JET_errReadLostFlushVerifyFailure:     \
    case JET_errFileSystemCorruption:           \
    case JET_errReadPgnoVerifyFailure

inline bool FErrIsDbCorruption( __in const ERR err )
{
    switch( err )
    {
        case_AllDatabaseStorageCorruptionErrs:
            return true;
        default:
            break;
    }
    return false;
}

inline bool FErrIsDbHeaderCorruption( __in const ERR err )
{
    return ( JET_errReadVerifyFailure == err ) || ( JET_errDiskReadVerificationFailure == err );
}

INLINE ULONG_PTR PctINSTCachePriority( const INST* const pinst )
{
    Assert( pinst != pinstNil );
    const ULONG_PTR pctCachePriority = UlParam( pinst, JET_paramCachePriority );
    Assert( FIsCachePriorityValid( pctCachePriority ) );
    return pctCachePriority;
}


INLINE ERR ErrFromCStatsErr( const CStats::ERR err )
{
    switch ( err )
    {
    case CStats::ERR::errSuccess:
    case CStats::ERR::wrnOutOfSamples:
        return JET_errSuccess;

    case CStats::ERR::errOutOfMemory:
        return ErrERRCheck( JET_errOutOfMemory );

    default:
        Assert( fFalse );
        return ErrERRCheck( JET_errInternalError );
    }
}
