// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef DAEDEF_INCLUDED
#error DAEDEF.HXX already included
#endif

#define DAEDEF_INCLUDED

#include "collection.hxx"

//  ****************************************************
//  global configuration macros
//

#ifdef DATABASE_FORMAT_CHANGE
//#define PAGE_FORMAT_CHANGE
//#define DBHDR_FORMAT_CHANGE
#endif

// if we define which, we need to make sure the format does support
// Unicode path names (which it wasn't changed for)
//
///#define LOG_FORMAT_CHANGE

//  support for JET_bitCheckUniqueness on JetSeek()
#define CHECK_UNIQUE_KEY_ON_NONUNIQUE_INDEX

//  index rebuild perf improvements
#define PARALLEL_BATCH_INDEX_BUILD
#define DONT_LOG_BATCH_INDEX_BUILD

//  xpress9/xpress10 compression
#ifdef USE_XPRESS_9_10_COMPRESSION
#define XPRESS9_COMPRESSION
#ifdef _AMD64_
#define XPRESS10_COMPRESSION
#endif
#endif

//  ****************************************************
//  global test hook / diagnostics macros
//

#ifdef DEBUG
//  Profiling JET API calls
//#define PROFILE_JET_API
#endif

//  *****************************************************
//  declaration macros
//
#define VTAPI

//  declares that a given class may be stored on disk at some point during its lifetime
#define PERSISTED

//  *****************************************************
//  global types and associated macros
//

#ifdef _WIN64
#define FMTSZ3264 L"I64"
#else
#define FMTSZ3264 L"l"
#endif

//  UNDONE: remove after typdefe-ing these at definition
//
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
const UPDATEID  updateidNil         = 0;    //  we start at 1 and increment by 2 so we will never reach 1

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

typedef ULONG           PGNO;   // database page number
const PGNO pgnoNull     = 0;
const PGNO pgnoMax      = (PGNO)-1;

const PGNO              pgnoScanFirst           = 1;
PERSISTED const PGNO    pgnoScanLastSentinel    = (PGNO)-2;
//  Note: The pgnoScanLastSentinel value is carefully chosen to not cause errant 
//  failures or events on the pre-existing nodes which already know about non-sentinel 
//  lrtypScanCheck.

typedef LONG            CPG;                // count of pages

typedef LONG FMPGNO;    // flush map page number. fmpgno 0 is the first data page, i.e., the header is fmpgno -1.
typedef FMPGNO CFMPG;   // flush map page count.

typedef BYTE            LEVEL;              // transaction levels
const LEVEL levelNil    = LEVEL( 0xff );

// Don't confuse JET_DBIDs with DBIDs ...
//
// JET_DBID = This is the API external handle to an attached database.  It is the exact same 
//  value as the ifmp / IFMP internally (and is cast on entrance/exit).
// IFMP = Index into g_rgfmp[], and our basic handle for a database from a _global_ perspective.
// DBID = Now this is where it gets subtle ... 
//  Unfortunately it has nothing to do with a JET_DBID, and is a different number space 
//  than the IFMP one.
//  DBIDs are a value from 0 - 6 (dbidMax == 7), and represent which attachment to the 
//  trx LOG they are.
typedef BYTE            _DBID;
typedef _DBID           DBID;
typedef WORD            _FID;

enum FIDTYP { fidtypUnknown = 0, fidtypFixed, fidtypVar, fidtypTagged };
// Special case values used for FID constructors.  Values other than those explicitly defined
// here are used as offsets.  See the constructor for their use.
enum FIDLIMIT { fidlimMin = 0, fidlimMax = (0xFFFE), fidlimNone = (0xFFFF) };

//  these are needed for setting columns and tracking indexes
//
#define cbitFixed           32
#define cbitVariable        32
#define cbitFixedVariable   (cbitFixed + cbitVariable)
#define cbitTagged          192
const ULONG cbRgbitIndex    = 32;   // size of index-tracking bit-array

class FID
{
private:
    _FID m_fidVal;

    // NOTE: The "None" val is one less than the "Min" val and is often used as a
    // NULL marker.  For example,
    //    fidHighestRequestedTagged = FID(FixedNoneVal)
    // might be used to indicate that no Tagged values were requested.
    //
    // More rarely, the "NoneFull" value is one more than the "Max" val and is used
    // as a marker that a table inherits from a Template table that has used up all the
    // FIDs of a given type, and the derived table has no explicit FIDs of that type.
    //
    // Note the unfortunate duplication.
    // 256 == VarNoneFullVal    & TaggedMinVal
    // 255 == TaggedNoneVal     & VarMaxVal
    // 128 == FixedNoneFullVal  & VarMinVal
    // 127 == VarNoneVal        & FixedMaxVal
    // 0   == FixedNoneVal      & fid.FNull()
    // This means that fid.Fidtyp() is unreliable for those cases.
    
public:
    // Tagged range from 256 to
    static const _FID _fidTaggedNoneFullVal = JET_ccolMost + 1;
    static const _FID _fidTaggedMaxVal      = JET_ccolMost;
    static const _FID _fidTaggedMinVal      = 256;
    static const _FID _fidTaggedNoneVal     = _fidTaggedMinVal - 1;

    // Var range from 128 to 255
    static const _FID _fidVarNoneFullVal    = _fidTaggedMinVal;
    static const _FID _fidVarMaxVal         = _fidTaggedMinVal - 1;
    static const _FID _fidVarMinVal         = 128;
    static const _FID _fidVarNoneVal        = _fidVarMinVal - 1;

    // Fixed range from 1 to 127
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
            // For a specific test, we need to create something that looks like a FID,
            // but contains an invalid value that won't pass FValid().
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
        //
        // Don't call any other FID methods while checking validity to avoid
        // accidentally introducing infinite recursion during validity checking.
        // This considers None values to be valid. Sometimes they are, sometimes they
        // aren't, depending on the context of the calling code.
        //
        // It's unfortunate, but this also considers "NoneFull" values to be valid.
        // Sometimes they are (but usually they aren't).  Still, sometimes they are.
        //
        if ( ( _fidFixedNoneVal <= m_fidVal ) && ( m_fidVal <= _fidFixedNoneFullVal ) )
        {
            // ( FFixedNone() || FFixed() || FFixedNoneFull() ) == fTrue;
            return fTrue;
        }
        
        if ( ( _fidVarNoneVal <= m_fidVal ) && ( m_fidVal <= _fidVarNoneFullVal ) )
        {
            // ( FVarNone() || FVar() || FVarNoneFull() ) == fTrue;
            return fTrue;
        }
        
        if ( ( _fidTaggedNoneVal <= m_fidVal ) && ( m_fidVal <= _fidTaggedNoneFullVal ) )
        {
            // ( FTaggedNone() || FTagged() || FTaggedNoneFull() ) == fTrue;
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
        //  return ( ( _fidVarLeast <= m_fidVal ) && ( m_fidVal <= _fidVarMaxVal ) )
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
        // Return the index of a fid value into the specified type.
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
        // Return the index of a fid value from the specified base FID.
#ifdef DEBUG
        // We occasionally index against a NoneVal. Fidtyp() doesn't work well on those.
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
    BOOL m_fEnd;         // fTrue if the iterator has advanced beyond m_fidLast.
    
public:
    FID_ITERATOR( const FID fidFirst, const FID fidLast )
    {
        // Note that it is expected (but rare) to get fidFirst >= fidLast.
        // An example is iterating all the indigenous fixed FIDs in a derived
        // table where the template table has already used up the full FID space.
        // In this case, fidFirst == fidFixedNoneFull (128) and fidLast == fidFixedMax (127).
        // If fidFirst < fidLast, then fidFirst and fidLast should be the same type.
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
        // Note that we only support going forward, not backward.
        if ( m_fEnd )
        {
            // Already at end.  Can't increment more.
            Assert( m_fidCurrent == max( m_fidFirst, m_fidLast ) );
            return;
        }

        Assert( m_fidLast >= m_fidFirst);
        
        if ( ( m_fidCurrent + wDistance) > m_fidLast )
        {
            // Increment passed the end.
            m_fidCurrent = m_fidLast;
            m_fEnd = fTrue;
            return;
        }

        // Normal increment.
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

const BYTE  fIDXSEGTemplateColumn       = 0x80;     //  column exists in the template table
const BYTE  fIDXSEGDescending           = 0x40;
const BYTE  fIDXSEGMustBeNull           = 0x20;

typedef JET_COLUMNID    COLUMNID;

const COLUMNID      fCOLUMNIDTemplate   = 0x80000000;

INLINE BOOL FCOLUMNIDValid( const COLUMNID columnid )
{
    //  only the Template bit of the two high bytes should be used
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

//  persisted version of IDXSEG
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

//  position within current series
//  note order of field is of the essence as log position used by
//  storage as timestamp, must in ib, isec, lGen order so that we can
//  use little endian integer comparisons.
//

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
                USHORT  ib;             // must be the last so that lgpos can
                USHORT  isec;           // index of disksec starting logsec
                LONG    lGeneration;    // generation of logsec
                                        // be casted to TIME.
            };
            QWORD qw;                   //  force QWORD alignment
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
            LONG    iSegment;       // segment number in the snapshot
            LONG    lGeneration;    // generation of snapshot
        };
        QWORD qw;                   // force QWORD alignment
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
                       fRestorePatch,   //  these are for hard-restore only
                       fRestoreRedo };

#define wszDefaultTempDbFileName        L"tmp"
#define wszDefaultTempDbExt         L".edb"

extern BOOL g_fEseutil;

extern DWORD g_dwGlobalMajorVersion;
extern DWORD g_dwGlobalMinorVersion;
extern DWORD g_dwGlobalBuildNumber;
extern LONG g_lGlobalSPNumber;

// Controls whether periodic database trimming is enabled (currently only available to be enabled via test hook).
extern BOOL g_fPeriodicTrimEnabled;

//  Page size related definition

const LONG  g_cbPageDefault = 1024 * 4;
const LONG  g_cbPageMin     = 1024 * 4;
const LONG  g_cbPageMax     = 1024 * 32;
const LONG  cbPageOld       = 4096;


//  ****************************************************
//  pattern fills
//

//  Pattern / Scrubbing fills (PERSISTED)
//

// PERSISTED const CHAR chLOGFill                           = (CHAR)0x7f;   // no longer used.

PERSISTED const CHAR chRECFixedColAndKeyFill                = ' ';  //  Fills the tail of fixed text columns and when making sort keys.
PERSISTED const WCHAR wchRECFixedColAndKeyFill              = L' ';
PERSISTED const CHAR chRECFill                              = '*';  //  Fills new space in internal record re-organizations (such as those necessary for inserting/expanding a fixed or variable column)

PERSISTED const CHAR chPAGEReplaceFill                      = 'R';  //  Runtime scrubbing for a replace that shrinks the record or requires moving the record to other free page space.
PERSISTED const CHAR chPAGEDeleteFill                       = 'D';  //  Runtime scrubbing of deleted data.
PERSISTED const CHAR chPAGEReorgContiguousFreeFill          = 'O';  //  Runtime scrubbing of any unused page space after the last node/data.
PERSISTED const CHAR chPAGEReHydrateFill                    = 'H';  //  Runtime scrubbing of page space formerly empty due to de-hydration.

PERSISTED const CHAR chSCRUBLegacyDeletedDataFill           = 'd';  //  Legacy scrubbing (during OLDv1 | Backup | eseutil /z) of data deleted, but missed (due to crash or version store bloat) by runtime scrubbing.
PERSISTED const CHAR chSCRUBLegacyLVChunkFill               = 'l';  //  Legacy scrubbing of Long Value data chunks for orphaned LVs, missed (due to crash or version store bloat) by runtime scrubbing.
PERSISTED const CHAR chSCRUBLegacyUsedPageFreeFill          = 'z';  //  Legacy scrubbing all unused page space (i.e. not header, and any existing non-deleted nodes/data) of a regular page (i.e. with useful data).
PERSISTED const CHAR chSCRUBLegacyUnusedPageFill            = 'u';  //  Legacy scrubbing all unused page space (i.e. not header) of an unused page.

PERSISTED const CHAR chSCRUBDBMaintDeletedDataFill          = 'D';  //  DB Maintenance scrubbing of data deleted, but missed (due to crash or version store bloat) by runtime scrubbing.
PERSISTED const CHAR chSCRUBDBMaintLVChunkFill              = 'L';  //  DB Maintenance scrubbing of Long Value data chunks for orphaned LVs, missed (due to crash or version store bloat) by runtime scrubbing.
PERSISTED const CHAR chSCRUBDBMaintUsedPageFreeFill         = 'Z';  //  DB Maintenance scrubbing all unused page space (i.e. not header, and any existing non-deleted nodes/data) of a regular page (i.e. with useful data).
PERSISTED const CHAR chSCRUBDBMaintUnusedPageFill           = 'U';  //  DB Maintenance scrubbing all unused page space (i.e. not header) of an unused page.
PERSISTED const CHAR chSCRUBDBMaintEmptyPageLastNodeFill    = 0x00; //  DB Maintenance scrubbing/replacing data of the only/last node of an empty page (used as a placeholder node on pages as we can't remove the last node from a tree yet).


//  Memory buffer pattern fills
//

// const CHAR chCATMSUSeekPrimaryKeyPreFill                 = 'w';  // No longer used.
// const CHAR chCATMSUSeekSecondaryKeyPreFill               = 'm';  // No longer used.
// const CHAR chCATMSUInsertPrimaryKeyPreFill               = 'u';  // No longer used.
// const CHAR chCATMSUInsertSecondaryKeyPreFill             = 'n';  // No longer used.
const BYTE bCRESAllocFill                                   = (BYTE)0xaa;
const BYTE bCRESFreeFill                                    = (BYTE)0xee;
//const CHAR chParamFill                                    = (CHAR)0xdd;   // No longer used.

const LONG_PTR  lBitsAllFlipped                             = ~( (LONG_PTR)0 );     //  generally used for filling a dead pointer

//  Test pattern fills
//

const CHAR chPAGETestPageFill                               = 'T';
const CHAR chPAGETestUsedPageFreeFill                       = 'X';
const CHAR chRECCompressTestFill                            = 'C';
const CHAR chDATASERIALIZETestFill                          = 'X';

//  ***********************************************
//  macros
//

const INT NO_GRBIT          = 0;
const ULONG NO_ITAG         = 0;

//  per database operation counter,
//  dbtime is logged and is used to compare
//  with dbtime of a page to decide if a redo of the logged operation
//  is necessary.
//
typedef __int64         _DBTIME;
#define DBTIME          _DBTIME
const DBTIME dbtimeNil = 0xffffffffffffffff;
const DBTIME dbtimeInvalid = 0xfffffffffffffffe;
const DBTIME dbtimeShrunk = 0xfffffffffffffffd;
const DBTIME dbtimeRevert = 0xfffffffffffffffc; // Used to track reverted new pages which get zeroed out as part of the revert and need to be ignored in dbscan check.
const DBTIME dbtimeUndone = dbtimeNil;          //  used to track the unused DBTIMEs in log records
const DBTIME dbtimeStart = 0x40;

//  transaction counter, used to keep track of the oldest transaction.
//
typedef ULONG       TRX;
const TRX trxMin    = 0;
const TRX trxMax    = 0xffffffff;

//  sentinel value to indicate that a session and its RCE's
//  are about to be committed
//
const TRX trxPrecommit  = trxMax - 0x10;

#define BoolSetFlag( flags, flag )      ( flag == ( flags & flag ) )


//  ================================================================
enum PageNodeBoundsChecking // pgnbc (incidentally NBC = Nuclear, Biological, Containment as well, c.f. "NBC suit")
//  ================================================================
{
    pgnbcChecked    = 1,  //   Checks various bounds, most significantly:
                          //      - that the tag in the page is within the tag array
                          //      - that a line / node is between the page header and the beginning of the tag array
                          //      - that the keys are bounded by the line / node.
    pgnbcNoChecks   = 2,  //   Skips bounds checks.  FASTER FASTER FASTER!  WHEEE!
};


//  ================================================================
typedef UINT FLAGS;
//  ================================================================


//  ================================================================
class DATA
//  ================================================================
//
//  DATA represents a chunk of raw memory -- a pointer
//  to the memory and the size of the memory
//
//-
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


//  ================================================================
INLINE VOID * DATA::Pv() const
//  ================================================================
{
    return m_pv;
}

//  ================================================================
INLINE INT DATA::Cb() const
//  ================================================================
{
    Assert( m_cb <= JET_cbLVColumnMost || FNegTest( fCorruptingPageLogically ) );
    return m_cb;
}

//  ================================================================
INLINE VOID DATA::SetPv( VOID * pvNew )
//  ================================================================
{
    m_pv = pvNew;
}

//  ================================================================
INLINE VOID DATA::SetCb( const SIZE_T cb )
//  ================================================================
{
    Assert( cb <= JET_cbLVColumnMost || FNegTest( fCorruptingPageLogically ) );
    m_cb = (ULONG)cb;
}

//  ================================================================
INLINE VOID DATA::DeltaPv( INT_PTR i )
//  ================================================================
{
    m_pv = (BYTE *)m_pv + i;
}

//  ================================================================
INLINE VOID DATA::DeltaCb( INT i )
//  ================================================================
{
    OnDebug( const ULONG cb = m_cb );
    m_cb += i;
    Assert( ( ( i >= 0 ) && ( m_cb >= cb ) ) ||
            ( ( i < 0 ) && ( m_cb < cb ) ) );
}


//  ================================================================
INLINE INT DATA::FNull() const
//  ================================================================
{
    BOOL fNull = ( 0 == m_cb );
    return fNull;
}


//  ================================================================
INLINE VOID DATA::CopyInto( DATA& dataDest ) const
//  ================================================================
{
    dataDest.SetCb( m_cb );
    UtilMemCpy( dataDest.Pv(), m_pv, m_cb );
}


//  ================================================================
INLINE VOID DATA::Nullify()
//  ================================================================
{
    m_pv = 0;
    m_cb = 0;
}


#ifdef DEBUG


//  ================================================================
INLINE DATA::DATA() :
//  ================================================================
    m_pv( (VOID *)lBitsAllFlipped ),
    m_cb( INT_MAX )
{
}


//  ================================================================
INLINE DATA::~DATA()
//  ================================================================
{
    Invalidate();
}


//  ================================================================
INLINE VOID DATA::Invalidate()
//  ================================================================
{
    m_pv = (VOID *)lBitsAllFlipped;
    m_cb = INT_MAX;
}


//  ================================================================
INLINE VOID DATA::AssertValid( BOOL fAllowNullPv ) const
//  ================================================================
{
    Assert( (VOID *)lBitsAllFlipped != m_pv );
    Assert( INT_MAX != m_cb );
    Assert( 0 == m_cb || NULL != m_pv || fAllowNullPv );
}


#endif  //  DEBUG

//  ================================================================
INLINE BOOL DATA::operator==( const DATA& data ) const
//  ================================================================
{
    const BOOL  fEqual = ( m_pv == data.Pv() && m_cb == (ULONG)data.Cb() );
    return fEqual;
}

#define cbKeyMostMost               JET_cbKeyMost32KBytePage
#define cbLimitKeyMostMost          ( cbKeyMostMost + 1 )

//  cbKeyAlloc must be as larg as cbLimitKeyMostMost and is a convenience for memory allocation
//
#define cbKeyAlloc                  cbLimitKeyMostMost

//  bookmark here is an allocation used for a B+tree internal key which may be at most
//  one secondary key and one primary key, or two times cbKeyMostMost
//
#define cbBookmarkMost              ( 2 * cbKeyMostMost )
#define cbBookmarkAlloc             cbBookmarkMost



//  ================================================================
class KEY
//  ================================================================
//
//  KEY represents a possibly compressed key
//
//-
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


//  ================================================================
INLINE INT KEY::Cb() const
//  ================================================================
{
    INT cb = prefix.Cb() + suffix.Cb();
    return cb;
}


//  ================================================================
INLINE INT KEY::FNull() const
//  ================================================================
{
    BOOL fNull = prefix.FNull() && suffix.FNull();
    return fNull;
}


//  ================================================================
INLINE VOID KEY::CopyIntoBuffer(
    _Out_writes_(cbMax) VOID * pvDest,
    INT cbMax ) const
//  ================================================================
{
    Assert( NULL != pvDest || 0 == cbMax );

    Assert( prefix.Cb() >= 0 ); // assert and defend ...
    Assert( suffix.Cb() >= 0 );
    INT cbPrefix    = max( 0, prefix.Cb() );
    INT cbSuffix    = max( 0, suffix.Cb() );

    UtilMemCpy( pvDest, prefix.Pv(), min( cbPrefix, cbMax ) );

    INT cbCopy = max ( 0, min( cbSuffix, cbMax - cbPrefix ) );
    UtilMemCpy( reinterpret_cast<BYTE *>( pvDest )+cbPrefix, suffix.Pv(), cbCopy );
}


//  ================================================================
INLINE VOID KEY::Advance( INT cb )
//  ================================================================
//
//  Advances the key pointer by the specified (non-negative) amoun
//
//-
{
    __assume_bound( cb );
    Assert( cb >= 0 );
    Assert( cb <= Cb() );
    if ( cb < 0 )
    {
        //  ignore requests to advance by a negative byte count
    }
    else if ( cb < prefix.Cb() )
    {
        //  advance the prefix
        prefix.SetPv( (BYTE *)prefix.Pv() + cb );
        prefix.SetCb( prefix.Cb() - cb );
    }
    else
    {
        //  move to the suffix
        INT cbPrefix = prefix.Cb();
        prefix.Nullify();
        suffix.SetPv( (BYTE *)suffix.Pv() + ( cb - cbPrefix ) );
        suffix.SetCb( suffix.Cb() - ( cb - cbPrefix ) );
    }
}


//  ================================================================
INLINE VOID KEY::Nullify()
//  ================================================================
{
    prefix.Nullify();
    suffix.Nullify();
}


//  ================================================================
INLINE USHORT KEY::CbLimitKeyMost( const USHORT usT )
//  ================================================================
{
    return USHORT( usT + 1 );
}


#ifdef DEBUG

//  ================================================================
INLINE VOID KEY::Invalidate()
//  ================================================================
{
    prefix.Invalidate();
    suffix.Invalidate();
}


//  ================================================================
INLINE VOID KEY::AssertValid() const
//  ================================================================
{
    Assert( prefix.Cb() == 0 || prefix.Cb() > sizeof( USHORT ) || FNegTest( fCorruptingPageLogically ) );
    ASSERT_VALID( &prefix );
    ASSERT_VALID( &suffix );
}


#endif


//  ================================================================
INLINE BOOL KEY::operator==( const KEY& key ) const
//  ================================================================
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


//  ================================================================
class LINE
//  ================================================================
//
//  LINE is a blob of data on a page, with its associated flags
//
//-
{
    public:
        VOID    *pv;
        ULONG   cb;
        FLAGS   fFlags;
};


//  ================================================================
class BOOKMARK
//  ================================================================
//
//  describes row-id as seen by DIR and BT
//  is unique within a directory
//
//-
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


//  ================================================================
INLINE VOID BOOKMARK::Nullify()
//  ================================================================
{
    key.Nullify();
    data.Nullify();
}

#ifdef DEBUG

//  ================================================================
INLINE VOID BOOKMARK::Invalidate()
//  ================================================================
{
    key.Invalidate();
    data.Invalidate();
}


//  ================================================================
INLINE VOID BOOKMARK::AssertValid() const
//  ================================================================
{
    ASSERT_VALID( &key );
    ASSERT_VALID( &data );
}


#endif


//  ================================================================
class BOOKMARK_COPY : public BOOKMARK
//  ================================================================
//
//  copy of a bookmark's content to the heap.
//
//-
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

//  ================================================================
INLINE BOOKMARK_COPY::BOOKMARK_COPY()
//  ================================================================
{
    m_pb = NULL;
    OnDebug( Invalidate() );
}

//  ================================================================
INLINE BOOKMARK_COPY::~BOOKMARK_COPY()
//  ================================================================
{
    FreeCopy();
}

//  ================================================================
INLINE ERR BOOKMARK_COPY::ErrCopyKey( const KEY& keySrc )
//  ================================================================
{
    DATA dataSrc;
    dataSrc.Nullify();
    return ErrCopyKeyData( keySrc, dataSrc );
}

//  ================================================================
INLINE ERR BOOKMARK_COPY::ErrCopyKeyData( const KEY& keySrc, const DATA& dataSrc )
//  ================================================================
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

    // Make sure we are able to hold our key in RESBOOKMARK (never expected to fail).
    const DWORD_PTR cb = keySrc.Cb() + dataSrc.Cb();
    DWORD_PTR cbMax = 0;
    CallS( RESBOOKMARK.ErrGetParam( JET_resoperSize, &cbMax ) );
    if ( cb > cbMax )
    {
        Assert( fFalse );
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    // Allocate memory.
    Alloc( m_pb = (BYTE *)RESBOOKMARK.PvRESAlloc() );
    Nullify();
    BYTE* pb = m_pb;

    // Copy key prefix.
    if ( !keySrc.prefix.FNull() )
    {
        key.prefix.SetPv( pb );
        key.prefix.SetCb( keySrc.prefix.Cb() );
        UtilMemCpy( pb, keySrc.prefix.Pv(), key.prefix.Cb() );
        pb += key.prefix.Cb();
    }

    // Copy key suffix.
    if ( !keySrc.suffix.FNull() )
    {
        key.suffix.SetPv( pb );
        key.suffix.SetCb( keySrc.suffix.Cb() );
        UtilMemCpy( pb, keySrc.suffix.Pv(), key.suffix.Cb() );
        pb += key.suffix.Cb();
    }

    // Copy data.
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

//  ================================================================
INLINE VOID BOOKMARK_COPY::FreeCopy()
//  ================================================================
{
    OnDebug( Invalidate() );
    if ( m_pb == NULL )
    {
        return;
    }

    RESBOOKMARK.Free( m_pb );
    m_pb = NULL;
}


//  ================================================================
class KEYDATAFLAGS
//  ================================================================
//
// a record definition, KEY/DATA and FLAGS associated with it
//
//-
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


//  ================================================================
INLINE VOID KEYDATAFLAGS::Nullify()
//  ================================================================
{
    key.Nullify();
    data.Nullify();
    fFlags = 0;
}


#ifdef DEBUG

//  ================================================================
INLINE VOID KEYDATAFLAGS::Invalidate()
//  ================================================================
{
    key.Invalidate();
    data.Invalidate();
    fFlags = 0;
}


//  ================================================================
INLINE BOOL KEYDATAFLAGS::operator==( const KEYDATAFLAGS& kdf ) const
//  ================================================================
{
    BOOL fEqual = fFlags == kdf.fFlags
            && key == kdf.key
            && data == kdf.data
            ;
    return fEqual;
}


//  ================================================================
INLINE VOID KEYDATAFLAGS::AssertValid() const
//  ================================================================
{
    ASSERT_VALID( &key );
    ASSERT_VALID( &data );
}


#endif


//  ================================================================
INLINE INT CmpData( const DATA& data1, const DATA& data2 )
//  ================================================================
//
//  compare two data structures. NULL (zero-length) is greater than
//  anything and ties are broken on db
//
//  memcmp returns 0 if it is passed a zero length
//
//-
{
    const INT   db  = data1.Cb() - data2.Cb();
    const INT   cmp = memcmp( data1.Pv(), data2.Pv(), db < 0 ? data1.Cb() : data2.Cb() );

    return ( 0 == cmp ? db : cmp );
}

//  ================================================================
INLINE INT CmpData( const VOID * const pv1, const INT cb1, const VOID * const pv2, const INT cb2 )
//  ================================================================
{
    const INT   db  = cb1 - cb2;
    const INT   cmp = memcmp( pv1, pv2, db < 0 ? cb1 : cb2 );

    return ( 0 == cmp ? db : cmp );
}


//  ================================================================
INLINE BOOL FDataEqual( const DATA& data1, const DATA& data2 )
//  ================================================================
//
//  compare two data structures. NULL (zero-length) is greater than
//  anything and ties are broken on db
//
//  memcmp returns 0 if it is passed a zero length
//
//-
{
    if( data1.Cb() == data2.Cb() )
    {
        return( 0 == memcmp( data1.Pv(), data2.Pv(), data1.Cb() ) );
    }
    return fFalse;
}

//  ================================================================
INLINE BOOL FDataEqual( const VOID * const pv1, const INT cb1, const VOID * const pv2, const INT cb2 )
//  ================================================================
{
    if ( cb1 == cb2 )
    {
        return ( 0 == memcmp( pv1, pv2, cb1 ) );
    }
    return fFalse;
}


//  ================================================================
INLINE INT CmpKeyShortest( const KEY& key1, const KEY& key2 )
//  ================================================================
//
// Compare two keys for the length of the shortest key
//
//-
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


//  ================================================================
INLINE INT CmpKey( const KEY& key1, const KEY& key2 )
//  ================================================================
//
//  Compares two keys, using length as a tie-breaker
//
//-
{
    INT cmp = CmpKeyShortest( key1, key2 );
    if ( 0 == cmp )
    {
        cmp = key1.Cb() - key2.Cb();
    }
    return cmp;
}


//  ================================================================
INLINE INT CmpKey( const void * pvkey1, ULONG cbkey1, const void * pvkey2, ULONG cbkey2 )
//  ================================================================
//
//  Compares two keys, using length as a tie-breaker
//
//-
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


//  ================================================================
INLINE BOOL FKeysEqual( const KEY& key1, const KEY& key2 )
//  ================================================================
//
//  Compares two keys, using length as a tie-breaker
//
//-
{
    if( key1.Cb() == key2.Cb() )
    {
        return ( 0 == CmpKeyShortest( key1, key2 ) );
    }
    return fFalse;
}


//  ================================================================
template < class KEYDATA1, class KEYDATA2 >
INLINE INT CmpKeyData( const KEYDATA1& kd1, const KEYDATA2& kd2, BOOL * pfKeyEqual = 0 )
//  ================================================================
//
// compare the KEY and DATA of two elements. we use a function template so
// that this will work for KEYDATAFLAGS and BOOKMARKS (and any combination)
// the function operates on two (or one) classes. Each class must have a member
// called key and a member called data
//
//-
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


//  ================================================================
template < class KEYDATA1 >
INLINE INT CmpKeyWithKeyData( const KEY& key, const KEYDATA1& keydata )
//  ================================================================
//
//  compare key with key-data
//
//-
{
    KEY keySegment1 = keydata.key;
    KEY keySegment2;

    keySegment2.prefix.Nullify();
    keySegment2.suffix = keydata.data;

    INT cmp = CmpKeyShortest( key, keySegment1 );

    if ( 0 == cmp && key.Cb() > keySegment1.Cb() )
    {
        //  the first segment was equal and we have more key to compare.
        //  keySegment2 is the data
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


//  ================================================================
INLINE INT  CmpBM( const BOOKMARK& bm1, const BOOKMARK& bm2 )
//  ================================================================
{
    return CmpKeyData( bm1, bm2 );
}



//  ================================================================
INLINE ULONG CbCommonData( const DATA& data1, const DATA& data2 )
//  ================================================================
//
//  finds number of common bytes between two data
//
//-
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

//  ================================================================
INLINE  ULONG CbCommonKey( const KEY& key1, const KEY& key2 )
//  ================================================================
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

//  END MACHINE DEPENDANT


//  ****************************************************
//  general C macros
//
#define forever                 for(;;)


//  *******************************************************
//  include Jet Project prototypes and constants
//

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
        // NYI - eSys / ErrIsamSystemInit() and eInst / at least one instance is up
    }
    AssertSz( fFalse, "Unknown ESE system level" );
    return fFalse;
}

//  System level RefTraceEntries

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

//  include other global DAE headers
//
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


//  WARNING: Must be packed to ensure size is the same as JET_INDEXID
struct INDEXID
{
    ULONG           cbStruct;
    OBJID           objidFDP;
//  WARNING: The following pointer must be 8-byte aligned for 64-bit NT
    FCB             *pfcbIndex;
    PGNO            pgnoFDP;
};


PERSISTED
struct LOGTIME
{
    BYTE    bSeconds;               //  0 - 60
    BYTE    bMinutes;               //  0 - 60
    BYTE    bHours;                 //  0 - 24
    BYTE    bDay;                   //  1 - 31
    BYTE    bMonth;                 //  0 - 11
    BYTE    bYear;                  //  current year - 1900

    BYTE    fTimeIsUTC:1;
    BYTE    bMillisecondsLow:7;

    BYTE    fReserved:1; // fOSSnapshot
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
        // Short circuit in case the fTimeIsUTC flag is set, as CmpLogTime()
        // below does not like mismatching flags.
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
    UnalignedLittleEndian< ULONG >  le_ulRandom;                                    //  a random number
    LOGTIME                         logtimeCreate;                                  //  time db created, in logtime format
    CHAR                            szComputerName[ MAX_COMPUTERNAME_LENGTH + 1 ];  // where db is created
};

PERSISTED
struct BKINFO
{
    LE_LGPOS                        le_lgposMark;   //  id for this backup
    LOGTIME                         logtimeMark;    //  timestamp of when le_lgposMark was logged
    UnalignedLittleEndian< ULONG >  le_genLow;      //  backup set's min lgen
    UnalignedLittleEndian< ULONG >  le_genHigh;     //  backup set's max lgen
};

//  Magic number used in database header for integrity checking
//
const ULONG ulDAEMagic              = 0x89abcdef;

//  Log LRCHECKSUM related checksum seeds

//  Special constant value that is checksummed with shadow sector data
//  to differentiate a regular partially full log sector from
//  the shadow sector.
const ULONG ulShadowSectorChecksum = 0x5AD05EC7;
const ULONG32 ulLRCKChecksumSeed = 0xFEEDBEAF;
const ULONG32 ulLRCKShortChecksumSeed = 0xDEADC0DE;


//      Format Version structures
//
struct GenVersion
{
    ULONG   ulVersionMajor;
    ULONG   ulVersionUpdateMajor;
    ULONG   ulVersionUpdateMinor;
};

struct DbVersion    // dbv
{
    ULONG   ulDbMajorVersion;
    ULONG   ulDbUpdateMajor;
    ULONG   ulDbUpdateMinor;
};

//  required for the unified version checking routines to cast and work effectively.
C_ASSERT( OffsetOf( DbVersion, ulDbMajorVersion ) == OffsetOf( GenVersion, ulVersionMajor ) );
C_ASSERT( OffsetOf( DbVersion, ulDbUpdateMajor ) == OffsetOf( GenVersion, ulVersionUpdateMajor ) );
C_ASSERT( OffsetOf( DbVersion, ulDbUpdateMinor ) == OffsetOf( GenVersion, ulVersionUpdateMinor ) );

struct LogVersion   // lgv
{
    //  Note: This only has the current version fields, if you have to compare legacy
    //  logs use CmpLgfilehdrVer() or CmpLogFormatVersion().
    ULONG   ulLGVersionMajor;
    ULONG   ulLGVersionUpdateMajor;
    ULONG   ulLGVersionUpdateMinor;
};

//  required for the unified version checking routines to cast and work effectively.
C_ASSERT( OffsetOf( LogVersion, ulLGVersionMajor ) == OffsetOf( GenVersion, ulVersionMajor ) );
C_ASSERT( OffsetOf( LogVersion, ulLGVersionUpdateMajor ) == OffsetOf( GenVersion, ulVersionUpdateMajor ) );
C_ASSERT( OffsetOf( LogVersion, ulLGVersionUpdateMinor ) == OffsetOf( GenVersion, ulVersionUpdateMinor ) );

struct FormatVersions   // fmtvers
{
    ULONG       efv;    //  "API" / JET_efv* / JET_paramEngineFormatVersion based values
    DbVersion   dbv;    //  Database version
    LogVersion  lgv;    //  Log files versions
    GenVersion  fmv;    //  Flushmap versions
};


#if defined(DEBUG) || defined(ENABLE_JET_UNIT_TEST)
//  Note this returns true if there is a full major version mismatch, irrelevant of the
//  mismatch state of update-major/minor as these are less severe.
inline BOOL FMajorVersionMismatch( const INT icmpFromCmpFormatVer )
{
    return 4 == abs( icmpFromCmpFormatVer );
}

//  Note this returns true if there is a update-major mismatch, but false if there is 
//  anything more severe (i.e. full major-version mismatch).  If there is a update-major
//  mismatch, AND an update-mismatch mismatch, this returns true as major-mismatch is more
//  severe than update-minor mismatch.
inline BOOL FUpdateMajorMismatch( const INT icmpFromCmpFormatVer )
{
    return 2 == abs( icmpFromCmpFormatVer );
}

#endif // DEBUG || ENABLE_JET_UNIT_TEST

//  Note this returns true if there is ONLY a update-minor mismatch, but false if there is 
//  anything more severe, such as a update-major or a full major-version mismatch.
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
                //  update minor mismatch
                return pver1->ulVersionUpdateMinor < pver2->ulVersionUpdateMinor ? -1 : 1;
            }
        }
        else
        {
            //  update major mismatch
            return pver1->ulVersionUpdateMajor < pver2->ulVersionUpdateMajor ? -2 : 2;
        }

        //  -3 vs. 3 may seem to be missing, but deprecated for historical the LOG's legacy minor version.
    }
    else
    {
        //  major ver mismatch
        return pver1->ulVersionMajor < pver2->ulVersionMajor ? -4 : 4;
    }
}

const JET_ENGINEFORMATVERSION efvExtHdrRootFieldAutoInc = JET_efvExtHdrRootFieldAutoIncStorageReleased;
//const JET_ENGINEFORMATVERSION efvExtHdrRootFieldAutoInc = OnDebugOrRetail( JET_efvExtHdrRootFieldAutoIncStorageStagedToDebug, JET_efvExtHdrRootFieldAutoIncStorageReleased );

inline INT CmpLgVer( const LogVersion& ver1, const LogVersion& ver2 )
{
    //  Thought about templating, but by doing this hack, the names of the variables
    //  for the types of version structures don't have to match.
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

// Define all formats in jethdr.w in order they are checked into our main development branch ... 
//   - Also add all engine release EFVs (such as JET_efvExchange2013Rtm, JET_efvWindows10Rtm, etc) after
//     the branch ships, duplicating the value of the last format feature that was shipped there.
//   - Also for any engine releases that should be publically addressible, surround them in () and ensure
//     you update the ManagedEsent layer.
//   - And finally add an entry to end of g_rgfmtversEngine[] table.

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
void FormatEfvSetting( const JET_ENGINEFORMATVERSION efvFullParam, _Out_writes_bytes_(cbEfvSetting) WCHAR * const wszEfvSetting, _In_range_( 240, 240 /* cchFormatEfvSetting * 2 */ ) const ULONG cbEfvSetting );
inline WCHAR * WszFormatEfvSetting( const JET_ENGINEFORMATVERSION efvFullParam, _Out_writes_bytes_(cbEfvSetting) WCHAR * const wszEfvSetting, _In_range_( 240, 240 /* cchFormatEfvSetting * 2 */ ) const ULONG cbEfvSetting )
{
    FormatEfvSetting( efvFullParam, wszEfvSetting, cbEfvSetting );
    return wszEfvSetting;
}
void FormatDbvEfvMapping( const IFMP ifmp, _Out_writes_bytes_( cbSetting ) WCHAR * wszSetting, _In_ const ULONG cbSetting );


//
//          Database / DB Versions
//
#define ulDAEVersionMax             ( PfmtversEngineMax()->dbv.ulDbMajorVersion )
#define ulDAEUpdateMajorMax         ( PfmtversEngineMax()->dbv.ulDbUpdateMajor )
#define ulDAEUpdateMinorMax         ( PfmtversEngineMax()->dbv.ulDbUpdateMinor )

//  higher update version #'s should always recognize db formats with a lower update version #
//  HISTORY:
//      0x620,0     - original OS Beta format (4/22/97)
//      0x620,1     - add columns in catalog for conditional indexing and OLD (5/29/97)
//      0x620,2     - add fLocalizedText flag in IDB (6/5/97)
//      0x620,3     - add SPLIT_BUFFER to space tree root pages (10/30/97)
//      0x620,2     - revert revision in order for ESE97 to remain forward-compatible (1/28/98)
//      0x620,3     - add new tagged columns to catalog ("CallbackData" and "CallbackDependencies")
//      0x620,4     - SLV support: signSLV, fSLVExists in db header (5/5/98)
//      0x620,5     - new SLV space tree (5/29/98)
//      0x620,6     - SLV space map [10/12/98]
//      0x620,7     - 4-byte IDXSEG [12/10/98]
//      0x620,8     - new template column format [1/25/99]
//      0x620,9     - sorted template columns [6/24/99]
//      0x620,A     - merged code base [3/26/2003]
//      0x620,B     - new checksum format [1/08/2004]
//      0x620,C     - increased max key length to 1000/2000 bytes for 4/8kb pages [1/15/2004]
//      0x620,D     - catalog space hints, space_header.v2 [7/15/2007]
//      0x620,E     - add new node/extent format to space manager, use it for reserved pools of space [8/9/2007]
//      0x620,F     - compression for intrinsic long-values [10/30/2007]
//      0x620,10        - compression for separated long-values [12/05/2007]
//      0x620,11        - new LV chunk size for large pages [12/29/2007]
//      0x620,12        - add LocaleName to the catalog [12/06/2011]
//      0x620,13        - <skipped by SOMEONE because he felt like it or is superstitious [2/1/2012]>
//      0x620,14        - add DotNetGuid sort order [2/1/2012]
//      1568,20 == 0x620,14
//          <NOTE: All future versions are listed in jethdr.w and sysver.cxx>


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

// implicit CreateDB used by E14/Win7 and earlier (with details of CreateDB
// not logged, but implicitly created / space,cat,etc filled  by engine 
// knowledge).
const USHORT usDAECreateDbVersion_Implicit  = 0x0001;
const USHORT usDAECreateDbUpdate_Implicit   = 0x0000;


//
//          Log / LG Versions
//

#define ulLGVersionMajorMax                         ( PfmtversEngineMax()->lgv.ulLGVersionMajor )
const ULONG ulLGVersionMinorFinalDeprecatedValue    = 4000;     // deprecated version field, final value ever used.
#define ulLGVersionUpdateMajorMax                   ( PfmtversEngineMax()->lgv.ulLGVersionUpdateMajor )
#define ulLGVersionUpdateMinorMax                   ( PfmtversEngineMax()->lgv.ulLGVersionUpdateMinor ) //  should NOT reset this on Major or UpdateMajor changes. Always goes forwards.


// You also need to update the HISTORY table in the comment below!

// snapshot of the versions when the new LV chunk size
// was introduced
//
const ULONG ulLGVersionMajorNewLVChunk  = 7;
const ULONG ulLGVersionMinorNewLVChunk  = 3704;
const ULONG ulLGVersionUpdateNewLVChunk = 14;

// version used by win7 ESENT
const ULONG ulLGVersionMajor_Win7       = 7;
const ULONG ulLGVersionMinor_Win7       = 3704;
const ULONG ulLGVersionUpdateMajor_Win7 = 15;

// last version used by old LRCHECKSUM log format
const ULONG ulLGVersionMajor_OldLrckFormat          = 7;
const ULONG ulLGVersionMinor_OldLrckFormat          = 3704;
const ULONG ulLGVersionUpdateMajor_OldLrckFormat    = 17;

// version when logs were zero-filled (rather than pattern filled)
const ULONG ulLGVersionMajor_ZeroFilled         = 8;
const ULONG ulLGVersionMinor_ZeroFilled         = 4000;
const ULONG ulLGVersionUpdateMajor_ZeroFilled   = 2;

// version when attach info started being written to checkpoint
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
//#define fmtlgverInitialDbSizeLoggedBackport       8,4000,4,10 - not needed, unless we backport this fix

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


//  higher major update version #'s should always recognize log formats with a lower major update version #
//  update versions greater than "7,3704,16,1" must always recognize (or skip) all LRTYPIGNORED records
//
//  HISTORY:
//      6.1503.0     - original format [1997/04/22]
//      6.1503.1     - SLV support: new lrtypes, add SLV name and flags in LRCREATE/ATTACHDB [1998/05/05]
//      6.1503.2     - SLV support: SLV space operations [1998/05/21]
//      6,1503.3     - new SLV space tree, log SLV root [1998/05/29]
//      6,1503.4     - fix format for SLV Roots (CreateDB/AttachDB) [1998/06/29]
//      6.1504.0     - FASTFLUSH single I/O log flushing with LRCHECKSUM instead of LRMS [1998/07/15]
//                     FASTFLUSH cannot read old log file format.
//      7,3700.0     - FASTFLUSH enabled, remove bitfields, dbtimeBefore (zeroed out) [1998/10/07]
//      7,3701.0     - reorg header, reorg ATTACHINFO, dbtimeBefore enabled, reorg lrtyps [1998/10/09]
//      7,3702.0     - change semantics of shutdown (forces db detach); add Flags to LRDETACHDB [1998/10/23]
//      7.3703.0     - log-extend pattern, new short-checksum semantics, corruption/torn-write detection
//                     and repair [1998/10/28]
//      7.3703.1     - lrtypSLVPageMove is added.
//      7.3704.0     - sorted tagged columns [1999/06/24] (accidentally changed the log-extend pattern)
//      7,3704,1     - lrtypUpgradeDb and lrtypEmptyTree [1999/11/02]
//      7,3704,2     - add date/time stamp to Init/Term/Rcv log records [1999/12/13]
//      7,3704,3     - add lrtypBeginDT, lrtypPrepCommit, and lrtypPrepRollback [2000/03/20]
//      7,3704,4     - add fLGCircularLogging to DBMS_PARAM [2000/04/24]
//      7,3704,5     - eliminate patching, add le_filetype to LGFILEHDR_FIXED [2001/05/01]
//      7,3704,7     - log enough with ConvertFDP to avoid a dependency [2003/02/20]
//      7,3704,8     - add version info for CreateDb [2003/06/25]
//      7,3704,9     - support Unicode names [2005/08/01]
//      7,3704,10    - Backup from Replica Support - lrtypBackup2 [2006/03/24]
//      7,3704,11    - Remove page dependencies - lrtypSplit2, lrtypMerge2 [2007/02/08]
//      7,3704,12    - lrtypScrub [2007/03/27] 
//      7,3704,13    - lrtypPageMove [2007/06/23]
//      7,3704,14    - new LV chunk size [2008/01/01]
//      7,3704,15    - removed force-detach [2008/01/17]
//      7,3704,16    - added LRPAGEPATCHREQUEST [2008/09/02]
//      7,3704,16    - Added Reserved LRs / lrtypIgnored[1-9] / LRTYPIGNORED.  Warning: No VERSION change! [2009/05/29]
//      7,3704,16,1  - Converted lrtypIgnored1 to lrtypMacroInfo for IRS [2009/09/01]
//      7,3704,16,2  - Converted lrtypIgnored2 to lrtypExtendDB to reduce fragmention in replay [2009/10/28]
//      7,3704,17,0  - E15 Start - Last version with LRCHECKSUM based format [2010/08/06]
//      8,4000,0,0   - First version with LGSEGHDR based format and ECC [2010/12/21]
//      8,4000,1,0   - lrtypShrinkDB [2011/03/21]
//      8,4000,2,0   - Do not re-pattern fill re-used log files.  No LRTYP added. [2011/04/11]
//      8,4000,3,0   - Fully Logged CreateDB!!! - added lrtypCreateDBFinish to facilitate this [2011/04/17]
//      8,4000,4,0   - Fast failover with dirty cache - added lrtypRecoveryQuitKeepAttachments [2011/06/11]
//      8,4000,4,1   - Added lrtypCommitC: user contexts for after Commit0 LRs [2011/10/06]
//      8,4000,4,2   - Added lrtypScanCheck: uses DB scan + LR to check for lost flushes / DB divergence [2013/01/24]
//      8,4000,4,3   - Added lrtypNOP2 - because 1 type of NOP is just not enough! [2013/02/19]
//      8,4000,4,4   - Added lrtypTrimDB to support sparse file trimming support [2013/03/31]
//      8,4000,4,5   - Added lrtypScanCheck(with pgnoSentinel value) to indicate a ScanCheck pass has finished. [2013/06/09]
//      8,4000,4,6   - Added code to allow replay of version 8,4000,5,0 logs [2014/04/29]
//      8,4000,5,0   - Can log and understand compressed BeforeImage in split/merge records. [2013/11/21]
//      8,4000,4,7 & 8,4000,5,1  - Log re-attach record and replay on passive. [2014/08/28]
//      8,4000,4,8 & 8,4000,5,2  - Breaking change to lrtypCommitCtx log record. [2014/09/10]
//      8,4000,4,9 & 8,4000,5,3  - Log the lrtypExtendDB record right after lrtypCreateDB (to replay the user specified create DB size during redo). [2014/10/02]
//      8,4000,4,10 & 8,4000,5,4 - (Special Ex15 Fork Change) This is really 8.4000.4.9/5.3 (ExtendDB after CreateDB), but without 8.4000,4.8/5.2 (new CommitCtx behavior). [2014/12/05]
//                  Note: This is because we needed to back port the ExtendDB solution back to before the CommitCtx change, putting our
//                  versioning system out of sorts :).  This update should only show up from the Ex15 Ent fork of the product, ironically
//                  though leaving datacenter at .9 (seemingly, but not actually) behind the forked on-premises product at .10.  This
//                  status will be short lived, with the upcoming forward-compat / forest-mode feature work.
//      x.xxxx.xx.11 - Note: Starting with this change, UpdateMinor versions are unified and always increasing across all other log version changes. Use FLogFormatUnifiedMinorUpdateVersion() to check.
//      8,4000,*,12  - Add previous log's last segment's checksum to the log header to catch lost flush/inc-reseed bugs [2014/12/09]
//      8,4000,*,13  - Change the log header to store previous log's accumulated segment checksum instead of only storing the last segment's checksum,
//                  to provide better resilience against lost flush/inc-reseed, or granular log shipping bugs. [2015/03/15]
//      8,4000,5,14  - Added lrtypMacroInfo2 (was lrtypIgnored7) for IRS of multiple databases and added lrtypIgnored10 - 19 [2015/06/02]
//      8,4000,5,15  - Implemented encryption of tagged columns (really a database version update, but ...) [2015/09/16]
//      8,4000,5,16  - Implemented lrtypFreeFDP to allow recovery to purge FCBs before space is freed. [2015/10/08]
//          <NOTE: All future versions are listed in jethdr.w and sysver.cxx>


const ULONG ulFMVersionMajorMax         = 3;
const ULONG ulFMVersionUpdateMajorMax   = 0;
const ULONG ulFMVersionUpdateMinorMax   = 0;

//  FM major version changes are neither backward nor forward compatible. Minor version changes are backward compatible only.
//  Update version changes are used to indicate a change in the persisted format, but are not expected to affect backward or
//  forward compatibility.
//
//  HISTORY:
//    1.0.0 - [2015/01/14]: original format.
//    2.0.0 - [2015/06/03]: technically, the file format did not change, but because this implementation of lost flush detection is able to recover
//                          from crashes (which requires the checkpoint to be held back a bit further), we are going to flip the major version to force
//                          1.0.0 maps to be always thrown away, since those are not guaranteed to be crash consistent.
//    3.0.0 - [2015/07/06]: technically, the file format did not change, but because version 2.0.0 had a bug in which we could have a dirty flush map
//                          associated with a clean database, there was a small window in which we could crash and end up with a map that did not
//                          contain all the state associated with the clean databse persisted. Bumping up the version forces such old maps to get thrown
//                          away.
//          <NOTE: All future versions are listed in jethdr.w and sysver.cxx>

inline INT CmpLogFormatVersion( const ULONG ulMajorV1, const ULONG ulMinorV1, const ULONG ulUpdateMajorV1, const ULONG ulUpdateMinorV1,
                                const ULONG ulMajorV2, const ULONG ulMinorV2, const ULONG ulUpdateMajorV2, const ULONG ulUpdateMinorV2 )
{
    //  most importatly is the major version different ...
    if( ulMajorV1 < ulMajorV2 )
    {
        return -4;
    }
    else if ( ulMajorV1 > ulMajorV2 )
    {
        return 4;
    }

    //  next check minor version ...
    if( ulMinorV1 < ulMinorV2 )
    {
        return -3;
    }
    else if ( ulMinorV1 > ulMinorV2 )
    {
        return 3;
    }

    //  then major update ...
    if( ulUpdateMajorV1 < ulUpdateMajorV2 )
    {
        return -2;
    }
    else if ( ulUpdateMajorV1 > ulUpdateMajorV2 )
    {
        return 2;
    }

    //  then minor update ...
    if( ulUpdateMinorV1 < ulUpdateMinorV2 )
    {
        return -1;
    }
    else if ( ulUpdateMinorV1 > ulUpdateMinorV2 )
    {
        return 1;
    }

    //  All inequalities should've been handled
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
// #define  attribSLV   1 // Obsolete

const BYTE  fDbShadowingDisabled    = 0x01;
const BYTE  fDbUpgradeDb            = 0x02;
// const BYTE   fDbSLVExists        = 0x04; // Obsolete

//  typedef struct _dbfilehdr_fixed
//  IMPORTANT: This CZeroInit() is NOT a replacement for the actual memset( pdbfilehdr, 0, g_cbPage ) in db.cxx, because 
//  we must set the remainder of the header ( g_cbPage - sizeof(DBFILEHDR_FIX) ) to zero as well to ensure easy upgrade
//  paths.
//
PERSISTED
struct DBFILEHDR : public CZeroInit
{
    LittleEndian<ULONG>     le_ulChecksum;          //  checksum of the 4k page
    LittleEndian<ULONG>     le_ulMagic;             //  Magic number
    LittleEndian<ULONG>     le_ulVersion;           //  version of DAE the db created (see ulDAEVersion)
    LittleEndian<LONG>      le_attrib;              //  attributes of the db
//  16 bytes

    LittleEndian<DBTIME>    le_dbtimeDirtied;       //  DBTime of this database
//  24 bytes

    SIGNATURE               signDb;                 //  (28 bytes) signature of the db (incl. creation time)
//  52 bytes

    LittleEndian<ULONG>     le_dbstate;             //  consistent/inconsistent state
//  56 bytes

    LE_LGPOS                le_lgposConsistent;     //  null if in inconsistent state
    LOGTIME                 logtimeConsistent;      //  null if in inconsistent state
//  72 bytes

    LOGTIME                 logtimeAttach;          //  Last attach time
    LE_LGPOS                le_lgposAttach;
//  88 bytes

    LOGTIME                 logtimeDetach;          //  Last detach time
    LE_LGPOS                le_lgposDetach;
//  104 bytes

    LittleEndian<ULONG>     le_dbid;                //  current db attachment
//  108 bytes

    SIGNATURE               signLog;                //  log signature
//  136 bytes

    BKINFO                  bkinfoFullPrev;         //  Last successful full backup
//  160 bytes

    BKINFO                  bkinfoIncPrev;          //  Last successful Incremental backup
//  184 bytes                                       //  Reset when bkinfoFullPrev is set

    BKINFO                  bkinfoFullCur;          //  current backup. Succeed if a
//  208 bytes                                       //  corresponding pat file generated

    union
    {
        ULONG               m_ulDbFlags;
        BYTE                m_rgbDbFlags[4];
    };

    LittleEndian<OBJID>     le_objidLast;           //  Object id used so far.

    //  NT version information. This is needed to decide if an index need
    //  be recreated due to sort table changes.

    LittleEndian<DWORD>     le_dwMajorVersion;      //  OS version info                             */
    LittleEndian<DWORD>     le_dwMinorVersion;
//  224 bytes

    LittleEndian<DWORD>     le_dwBuildNumber;
    LittleEndian<LONG>      le_lSPNumber;           //  use 31 bit only

    LittleEndian<ULONG>     le_ulDaeUpdateMajor;    //  used to track incremental database format updates that
                                                    //  are backward-compatible (see ulDAEUpdateMajorMax)

    LittleEndian<ULONG>     le_cbPageSize;          //  database page size (0 = 4k pages)
//  240 bytes

    LittleEndian<ULONG>     le_ulRepairCount;       //  number of times ErrREPAIRAttachForRepair has been called on this database
    LOGTIME                 logtimeRepair;          //  the date of the last time that repair was run
//  252 bytes

    BYTE                    rgbReservedSignSLV[ sizeof( SIGNATURE ) ];              //  signSLV signature of associated SLV file (obsolete)
//  280 bytes

    LittleEndian<DBTIME>    le_dbtimeLastScrub;     //  last dbtime the database was zeroed out
//  288 bytes

    LOGTIME                 logtimeScrub;           //  the date of the last time that the database was zeroed out
//  296 bytes

    LittleEndian<LONG>      le_lGenMinRequired;     //  the minimum log generation required for replaying the logs. Typically the checkpoint generation
//  300 bytes

    LittleEndian<LONG>      le_lGenMaxRequired;     //  the maximum log generation required for replaying the logs. This is known as the waypoint in BF.
//  304 bytes

    LittleEndian<LONG>      le_cpgUpgrade55Format;      //
    LittleEndian<LONG>      le_cpgUpgradeFreePages;     //
    LittleEndian<LONG>      le_cpgUpgradeSpaceMapPages; //
//  316 bytes

    BKINFO                  bkinfoSnapshotCur;          //  Current snapshot.
//  340 bytes

    LittleEndian<ULONG>     le_ulCreateVersion;     //  version of DAE that created db (debugging only)
    LittleEndian<ULONG>     le_ulCreateUpdate;
//  348 bytes

    LOGTIME                 logtimeGenMaxCreate;    //  creation time of the genMax log file
//  356 bytes

    typedef enum
    {
        backupNormal = 0x0,  // normal should be 0 for backward compatibility
        backupOSSnapshot,
        backupSnapshot,
        backupSurrogate,        // should not be persisted in header, but persisted and used by log.
    } BKINFOTYPE;

    LittleEndian<BKINFOTYPE>    bkinfoTypeFullPrev;         //  Type of Last successful full backup
    LittleEndian<BKINFOTYPE>    bkinfoTypeIncPrev;          //  Type of Last successful Incremental backup

//  364 bytes
    LittleEndian<ULONG>     le_ulRepairCountOld;        //  number of times ErrREPAIRAttachForRepair has been called on this database before the last defrag

    LittleEndian<ULONG>     le_ulECCFixSuccess;     //  number of times a one bit error was fixed and resulted in a good page
    LOGTIME                 logtimeECCFixSuccess;   //  the date of the last time that a one bit error was fixed and resulted in a good page
    LittleEndian<ULONG>     le_ulECCFixSuccessOld;  //  number of times a one bit error was fixed and resulted in a good page before last repair

    LittleEndian<ULONG>     le_ulECCFixFail;        //  number of times a one bit error was fixed and resulted in a bad page
    LOGTIME                 logtimeECCFixFail;      //  the date of the last time that a one bit error was fixed and resulted in a bad page
    LittleEndian<ULONG>     le_ulECCFixFailOld;     //  number of times a one bit error was fixed and resulted in a bad page before last repair

    LittleEndian<ULONG>     le_ulBadChecksum;       //  number of times a non-correctable ECC/checksum error was found
    LOGTIME                 logtimeBadChecksum;     //  the date of the last time that a non-correctable ECC/checksum error was found
    LittleEndian<ULONG>     le_ulBadChecksumOld;        //  number of times a non-correctable ECC/checksum error was found before last repair

//  416 bytes

    LittleEndian<LONG>      le_lGenMaxCommitted;    //  the last log generation to take active log records for this database.  Not
                                                    //  ensuring replay through this log generation will lose the D in ACID.
//  420 bytes
    BKINFO                  bkinfoCopyPrev;         //  Last successful Copy backup
    BKINFO                  bkinfoDiffPrev;         //  Last successful Differential backup, reset when bkinfoFullPrev is set

//  468 bytes
    LittleEndian<BKINFOTYPE>    bkinfoTypeCopyPrev; //  Type of Last successful Incremental backup
    LittleEndian<BKINFOTYPE>    bkinfoTypeDiffPrev;     //  Type of Last successful Differential backup

// 476 bytes
    LittleEndian<ULONG>     le_ulIncrementalReseedCount;    //  number of times incremental reseed has been initiated on this database
    LOGTIME                 logtimeIncrementalReseed;       //  the date of the last time that incremental reseed was initiated on this database
    LittleEndian<ULONG>     le_ulIncrementalReseedCountOld; //  number of times incremental reseed was initiated on this database before the last defrag

    LittleEndian<ULONG>     le_ulPagePatchCount;            //  number of pages patched in the database as a part of incremental reseed
    LOGTIME                 logtimePagePatch;               //  the date of the last time that a page was patched as a part of incremental reseed
    LittleEndian<ULONG>     le_ulPagePatchCountOld;         //  number of pages patched in the database as a part of incremental reseed before the last defrag

// 508 bytes
    UnalignedLittleEndian<QWORD>    le_qwSortVersion;       // DEPRECATED: In old versions had "default" (?English?) LCID version, in new versions has 0xFFFFFFFFFFFF.

//  516 bytes                                               // checksum during recovery state
    LOGTIME                 logtimeDbscanPrev;          // last checksum finish time (UTC - 1900y)
    LOGTIME                 logtimeDbscanStart;         // start time (UTC - 1900y)
    LittleEndian<PGNO>      le_pgnoDbscanHighestContinuous; // current pgno

//  536 bytes
    LittleEndian<LONG>      le_lGenRecovering;      //  the current log generation that we are doing recovery::redo for.

//  540 bytes

    LittleEndian<ULONG>     le_ulExtendCount;
    LOGTIME                 logtimeLastExtend;
    LittleEndian<ULONG>     le_ulShrinkCount;
    LOGTIME                 logtimeLastShrink;

//  564 bytes

    LOGTIME                 logtimeLastReAttach;
    LE_LGPOS                le_lgposLastReAttach;
//  580 bytes

    LittleEndian<ULONG>     le_ulTrimCount;
//  584 bytes

    SIGNATURE               signDbHdrFlush;                 //  random signature generated at the time of the last DB header flush
    SIGNATURE               signFlushMapHdrFlush;           //  random signature generated at the time of the last FM header flush
//  640 bytes

    LittleEndian<LONG>      le_lGenMinConsistent;           //  the minimum log generation required to bring the database to a clean state
                                                            //  it might be different from lGenMinRequired, which also encompasses flush map consistency
//  644 bytes

    LittleEndian<ULONG>     le_ulDaeUpdateMinor;            //  used to track incremental database format updates that
                                                            //  are forwards-compatible (see ulDAEUpdateMinorMax)
    LittleEndian<JET_ENGINEFORMATVERSION>   le_efvMaxBinAttachDiagnostic;   //  Max version of engine binary that has attached this database.
                                                            //      NOTE: This is NOT necessarily the format the DB creates/should maintain if JET_paramEnableFormatVersion is set.
                                                            //      NOTE: ALSO this is NOT propagated via redo, intentionally to avoid dependency on this.  Use le_ulDaeUpdateMinor instead.
//  652 bytes

    LittleEndian<PGNO>      le_pgnoDbscanHighest;       // highest pgno - used to avoid re-scanning pages
//  656 bytes

    LE_LGPOS                le_lgposLastResize;         // lgpos of the last database resize committed to the database.
                                                        // This is unset on older builds and it requires JET_efvLgposLastResize to start being populated.
                                                        // A database attachment sets this to le_lgposAttach, even though there isn't a real resize, so it can be
                                                        // be viewed as "resize checkpoint".
                                                        // A database extension or shrinkage updates this to the LGPOS of the operation.
                                                        // A clean databse detachment sets this to le_lgposConsistent.
                                                        // Incremental reseed may set this back if the first divergent log is lower than the current le_lgposLastResize.
//  664 bytes

    BYTE                    rgbReserved[3];     // keeping the le_filetype in the same
                                                // place as log file header and checkpoint
                                                // file header
                                                // Remember: when updating logfile, checkpoint or
                                                // flush map file headers, please check here
//  667 bytes

    //  WARNING: MUST be placed at this offset for
    //  uniformity with db/log headers
    UnalignedLittleEndian<ULONG>    le_filetype;    //  JET_filetypeDatabase or JET_filetypeStreamingFile
//  671 bytes

    BYTE                    rgbReserved2[1];    // For alignment
//  672 bytes

    LOGTIME                 logtimeGenMaxRequired;
//  680 bytes

    LittleEndian<LONG>      le_lGenPreRedoMinConsistent;   //  the minimum log generation required to bring the database to a clean state (value overriden during the start of redo, 0 if not overriden).
    LittleEndian<LONG>      le_lGenPreRedoMinRequired;     //  the minimum log generation required for replaying the logs (value overriden during the start of redo, 0 if not overriden).
//  688 bytes

    SIGNATURE               signRBSHdrFlush;               //  random signature generated at the time of the last RBS header flush
//  716 bytes

    LittleEndian<ULONG>     le_ulRevertCount;              //  number of times revert has been initiated on this database using the revert snapshots.
    LOGTIME                 logtimeRevertFrom;             //  the date of the last time that revert was initiated on this database using the revert snapshots.
    LOGTIME                 logtimeRevertTo;               //  the date of the last time we were reverting this database to.
    LittleEndian<ULONG>     le_ulRevertPageCount;          //  number of pages reverted on the database by the recent revert operation.
    LE_LGPOS                le_lgposCommitBeforeRevert;    //  lgpos of the last commit on the database before a revert was done and requires JET_efvApplyRevertSnapshot to start being populated.
                                                           //  This will be set only after a revert is completed and will be used to ignore JET_errDbTimeTooOld on passive copies.
                                                           //  This is because as part of revert any new page reverts will cause the page to be zeroed out and thereby might be behind the active's dbtime for the page.
                                                           
//  748 bytes

//  -----------------------------------------------------------------------

    //  Ctor.

    DBFILEHDR() :
        CZeroInit( sizeof( DBFILEHDR ) )
    {
    }


    //  Methods

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

};  //  end of struct DBFILEHDR

C_ASSERT( sizeof( DBFILEHDR ) == 748 ); // Make sure we are counting the bytes correctly above. Change this when expanding the struct.
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
    //  Databasae header state machine ... given a current state, what are the possibly 
    //  allowed states we might move to?
    //

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

    //  Ok, we've decided it is ok to switch from the current state to the new
    //  state, now ensure the additional state args are well validated ...
    switch ( dbstate )
    {
        case JET_dbstateJustCreated:
            AssertRTL( 0 == lGenMin );
            AssertRTL( 0 == lGenMax );
            AssertRTL( NULL == plogtimeCurrent );
            break;
        case JET_dbstateDirtyShutdown:
            // I don't like this 0 != lGenMin clause, but couldn't figure out how
            // to work out getting past this for JET_dbstateJustCreated != le_dbstate 
            // during initial DB create.
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
                // sometimes people pass in just an empty log time instead.  Silly.
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

    //  Finally, update the dbstate ...

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
        //  avoid touching logtimeGenMaxCreate/logtimeGenMaxRequired
        Expected( NULL == plogtimeCurrent );
    }
    else if ( 0 != lGenMax && NULL != plogtimeCurrent )
    {
        memcpy( &logtimeGenMaxCreate, plogtimeCurrent, sizeof( LOGTIME ) );

        // Only set logtimeGenMaxRequired if the database is on high enough version
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
    CHAR rgSig[128]; // enough space.
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
    (*pcprintf)( "         Shadowed: %s%s", FShadowingDisabled( ) ? "No" : "Yes", szNewLine );  // we just assume so
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

    //  start line ...
    (*pcprintf)( "       OS Version: (%u.%u.%u SP %u ",
                (ULONG) le_dwMajorVersion,
                (ULONG) le_dwMinorVersion,
                (ULONG) le_dwBuildNumber,
                (ULONG) le_lSPNumber );
    //  finish line
    if ( g_qwUpgradedLocalesTable == le_qwSortVersion )
    {
        (*pcprintf)( "MSysLocales)%s", szNewLine );
    }
    else
    {
        //  legacy / non-MSysLocales
        (*pcprintf)( "%x.%x)%s",
                    (DWORD)( ( le_qwSortVersion >> 32 ) & 0xFFFFFFFF ), // NLS Version.
                    (DWORD)( le_qwSortVersion & 0xFFFFFFFF ), // Defined Version.
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
    //  SOMEONE note these should've been in the structure in THIS order, like the _rest of 
    //  the fricken structure_ as the LOGTIME is unimportant and for debugging, and only 
    //  LGPOSs matter, ... just TRY to be a little more conscientious when dealing with
    //  PERSISTED structures.
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

#endif  //  DEBUGGER_EXTENSION

typedef DBFILEHDR   DBFILEHDR_FIX;

//  Util.cxx function

VOID UtilLoadDbinfomiscFromPdbfilehdr( JET_DBINFOMISC7* pdbinfomisc, const ULONG cbdbinfomisc, const DBFILEHDR_FIX *pdbfilehdr );

//  ================================================================
typedef ULONG RCEID;
//  ================================================================
const RCEID rceidNull   = 0;

//  ================================================================
//      JET PARAMETERS
//  ================================================================

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
    #define configMaxLegacy (2)     // for the legacy model, we only had 2 config defaults / settings

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
    //  22 bits used

    //  For string values, WCHAR counts (wcslen()) not including terminating NUL, for the minimum 
    //  and maxium string length.
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


//  ================================================================
//      JET INSTANCE
//  ================================================================


const BYTE  fDBMSPARAMCircularLogging   = 0x1;

// There is no easy way to change the structue to store Unicode
// especialy because it is used in the middle of the Log header
// More important, the information for log and system path are information only
// so we should be able to store any Unicode in Ascii with data lost w/o 
// any real issues.
// CAUTION: we are using WideCharToMultiByte using WC_DEFAULTCHAR which is with 
// information lost (using ? for non-ascii chars).
// As a result, the szLogFilePathDebugOnly and szSystemPathDebugOnly MUST be used
// for display information / debug only.
//  
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

    BYTE                                rgbReserved[16];    //  reserved:  initialized to zero
};

#include <poppack.h>

//  Instance variables for JetInit and JetTerm.

const INT       cFCBBuckets = 257;
const ULONG     cFCBPurgeAboveThresholdInterval     = 500;      //  how often to purge FCBs above the preferred threshold

enum TERMTYPE {
        termtypeCleanUp,        //  Termination with Version clean up etc
        termtypeNoCleanUp,      //  Termination without any clean up
        termtypeError,          //  Terminate with error, no clean up
                                //  no flush buffers, db header
        termtypeRecoveryQuitKeepAttachments
                                //  Termination which leaves buffers unflushed
                                //  and dbs attached and dirty
    };



INLINE UINT UiHashIfmpPgnoFDP( IFMP ifmp, PGNO pgnoFDP )
{

    //  OLD HASHING ALGORITHM: return ( ifmp + pgnoFDP ) % cFCBBuckets;

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

//#define OS_SNAPSHOT_TRACE

//#define OS_SNAPSHOT_ALLOW_TRUNCATE_ONLY

// Class describing the status of a snapshot operation
class CESESnapshotSession
{
// Constructors and destructors
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

    // do not allow other constructors other then the one with snapId
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

    // start the snapshot thread
    ERR     ErrStartSnapshotThreadProc( );

    // signal the thread to stop
    void    StopSnapshotThreadProc( const BOOL fWait = fFalse );

    // snapshot thread: the only that moves the state to Ended or TimeOut
    void    SnapshotThreadProc( const ULONG ulTimeOut );

    // truncate logs after Thaw
    ERR     ErrTruncateLogs( INST * pinstTruncate, const JET_GRBIT grbit);

    // stamp databases with snapshot info
    void    SaveSnapshotInfo(const JET_GRBIT grbit);

    void    ResetBackupInProgress( const INST * pinstLastBackupInProgress );

private:
    ERR     ErrFreeze();
    void    Thaw( const BOOL fTimeOut );

    // freeze/thaw operations on instance(s)
    ERR     ErrFreezeInstance();
    void    ThawInstance( const INST * pinstLastAPI = NULL, const INST * pinstLastCheckpoint = NULL, const INST * pinstLastLGFlush = NULL );


    // freeze/thaw operations on ALL databases from the frozen instance(s)
    ERR     ErrFreezeDatabase();
    void    ThawDatabase( const IFMP ifmpLast = g_ifmpMax );

    ERR     SetBackupInProgress();


private:
    
    JET_OSSNAPID        m_idSnapshot;   // identifer of the snapshot
    
    SNAPSHOT_STATE      m_state;        // state of the snapshot session
    THREAD              m_thread;
    ERR                 m_errFreeze;    // error at freeze start-up

    CAutoResetSignal    m_asigSnapshotThread;
    CAutoResetSignal    m_asigSnapshotStarted;


    // members and methods to control the instances involved in snapshot
    // type of snapshot: incremental or full / copy or no-copy (normal)
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
    INT                 m_ipinstCurrent;    // used to iterate with the next 2 functions on instnaces to snapshot
                                            // if m_pinstToFreeze is set, we return just that one (then NULL)
                                            // otherwise we return each instance in g_rgpinst
    INST *              GetFirstInstance();
    INST *              GetNextInstance();
    INST *              GetNextNotNullInstance();

    void                SetFreezeInstances();



    // global list related elements
    //
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

// static members
public:
    static void     SnapshotCritEnter() { CESESnapshotSession::g_critOSSnapshot.Enter(); }
    static void     SnapshotCritLeave() { CESESnapshotSession::g_critOSSnapshot.Leave(); }

    static ERR      ErrInit();
    static void     Term();
    
private:
    //  critical section protecting all OS level snapshot information
    static CCriticalSection g_critOSSnapshot;

    // counter used to keep a global identifer of the snapshots
    static JET_OSSNAPID g_idSnapshot;


    // used to specify no instance as (INST *)NULL stand for "all instances"
    static const INST * const g_pinstInvalid;

};

ERR ErrSNAPInit();
void SNAPTerm();

//  forward declarations
class SCRUBDB;
typedef struct tagLGSTATUSINFO LGSTATUSINFO;
PERSISTED struct CHECKPOINT_FIXED;
struct LOG_VERIFY_STATE;

//  Recovery Handle for File Structure:
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
    WCHAR *         wszFileName;        // used only to report an error, set in ErrIsamOpenFile
    LONG            lGeneration;    // generation number, if the file is a log file
    DBFILEHDR *     pdbfilehdr;     // Saved file header for database
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
    BOOL            m_fBackupFull;              // set for Full Backup

    PIB             *m_ppibBackup;

    CCriticalSection    m_critBackupInProgress; // critical section blocking state changes to m_fBackupInProgress ?and a few other things?
    BOOL            m_fBackupInProgressAny; // set when any type of backup is started, for backup synchronization and
                                                // attach / detach blocking.
    BOOL            m_fBackupInProgressLocal;   // set when a local variety of backup (ergo JetBackup, external backup,
                                                // OS snapshot backup), also implies some disk load / burden, and
                                                // checkpoint advancement issues.

    LONG            m_lgenDeleteMic;
    LONG            m_lgenDeleteMac;

    BOOL            m_fLogsTruncated;           // during backup tells if the Truncate was called
                                                // during restore tells if the last found backup was with truncation or not

    BOOL            m_fBackupIsInternal;        // used only with regular full backups.
    LGPOS           m_lgposFullBackupMark;
    LOGTIME         m_logtimeFullBackupMark;
    SIZE_T          m_cGenCopyDone;             // count of generation# for which copy is done
    BOOL            m_fBackupBeginNewLogFile;

    BOOL            m_fCtxIsSuspended;          // used to tell if backups are suspended,
                                                // it is updated by ErrLGDbDetachingCallback and ErrLGDbAttachedCallback.

    //  these are used for online zeroing during backup

    SCRUBDB         *m_pscrubdb;            //  used to process pages after they are read in
    ULONG_PTR       m_ulSecsStartScrub;     //  the time we started the scrub
    DBTIME          m_dbtimeLastScrubNew;   //  the dbtime we started the scrub

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


//  Note: the initial beginning sequence must always be 0, and the first done phase
//  must always be 1.
enum : BYTE
{
    eSequenceStart = 0
};

enum : BYTE     //  for Event ID: 105 / START_INSTANCE_DONE_ID
{
    eInitOslayerDone = 1,
    eInitIsamSystemDone,
    eInitSelectAllocInstDone,
    eInitVariableInitDone,      // just random inst allocations, variable inits, res pools (note: also can contain time between JetCreateInstance and JetInit API calls)
    eInitConfigInitDone,        // user config ... START_INSTANCE_ID logged here
    eInitBaseLogInitDone,       // not much done here
    eInitLogReadAndPatchFirstLog,
    //  formerly: eInitLogRecoveryRedoDone, split into next two phases
    eInitLogRecoverySilentRedoDone,
    eInitLogRecoveryVerboseRedoDone,
    eInitLogRecoveryReopenDone,
    eInitLogRecoveryUndoDone,
    eInitLogRecoveryBfFlushDone,
    eInitLogInitDone,
    eInitSubManagersDone,       // IO, PIB, FCB, SCB, FUCB type inst init is done + m_fSTInit  wait
    eInitBigComponentsDone,     // Bigger things, TM, VER, LV ...

    eInitDone,

    eInitSeqMax
};

enum : BYTE     //  for Event ID: 103 \ 104 / STOP_INSTANCE_ID \ STOP_INSTANCE_ID_WITH_ERROR
{
    eTermWaitedForUserWorkerThreads = 1,
    eTermWaitedForSystemWorkerThreads,  // note we include the m_fSTInitDone wait in here
    eTermVersionStoreDone,
    eTermRestNoReturning,
    eTermBufferManagerFlushDone,
    eTermBufferManagerPurgeDone,    // sometime BFPurge happens in ErrIOTerm due to keep-cache-alive
    eTermVersionStoreDoneAgain,
    eTermCleanupAndResetting,   // includes ErrIOTerm()
    eTermLogFlushed,
    eTermLogFsFlushed,
    eTermCheckpointFlushed,
    eTermLogTasksTermed,
    eTermLogFilesClosed,
    eTermLogTermDone,

    eTermDone,
    //  note since we lose our inst earlier, we leave the context that has the term sequence info
    //  before we get to OSUTerm/OSTerm() unfortunately (so this doens't include that).

    eTermSeqMax
};

enum : BYTE     //  for Event ID: 325 / CREATE_DATABASE_DONE_ID
{
    //  note since we don't have an FMP yet, we don't catch the timings of FMP::ErrNewAndWriteLatch()
    eCreateInitVariableDone = 1,
    eCreateLogged,
    eCreateHeaderInitialized,       //  first IO
    eCreateDatabaseZeroed,          //  ErrIOSetSize, second IO burst
    eCreateDatabaseSpaceInitialized,
    eCreateDatabaseCatalogInitialized,
    eCreateDatabaseInitialized,     //  base database flushed, third IO burst, still just created in header though
    eCreateFinishLogged,
    eCreateDatabaseDirty,           //  marked dirty
    eCreateLatentUpgradesDone,

    eCreateDone,

    eCreateSeqMax
};

enum : BYTE     //  for Event ID: 326 / ATTACH_DATABASE_DONE_ID
{
    //  note since we don't have an FMP yet, we don't catch the timings of FMP::ErrNewAndWriteLatch()
    eAttachInitVariableDone = 1,
    eAttachReadDBHeader,            //  first IO
    eAttachToLogStream,             //  LR
    eAttachIOOpenDatabase,
    eAttachFastReAttachAcquireFmp,
    eAttachLeakReclaimerDone,
    eAttachShrinkDone,
    eAttachFastBaseReAttachDone,    //  fast path version of next step, skips next step - goes to upgrades next (e.g. eAttachCreateMSysObjids, etc)
    eAttachSlowBaseAttachDone,      //  basic variables set, no "upgrades" processed
    eAttachCreateMSysObjids,
    eAttachCreateMSysLocales,
    eAttachDeleteMSysUnicodeFixup,
    eAttachCheckForOutOfDateLocales,
    eAttachDeleteUnicodeIndexes,
    eAttachUpgradeUnicodeIndexes,

    eAttachDone,

    eAttachSeqMax
};

enum : BYTE     //  for Event ID: 327 / DETACH_DATABASE_DONE_ID
{
    //  note does not have initial acquiring backup/restore critical section
    eDetachQuiesceSystemTasks = 1,  //
    eDetachSetDetaching,
    eDetachVersionStoreDone,        //  first ErrVERClean()
    eDetachStopSystemTasks,
    eDetachVersionStoreReallyDone,  //  second ErrVERClean() ... cleans up anything system tasks input
    eDetachBufferManagerFlushDone,
    eDetachBufferManagerPurgeDone,
    eDetachLogged,
    eDetachUpdateHeader,            //  header marked clean
    eDetachIOClose,                 //  includes closing the DB file handle, but not last IO we'll do

    eDetachDone,

    eDetachSeqMax
};

//  This should probably really be = eInitLogReadAndPatchFirstLog ), BUT I found it isn't always 
//  triggered ... and ErrLGSoftStart is such a mess, I didn't want to try to debug which if/elseif
//  /else it came through.  If we fix eInitLogReadAndPatchFirstLog to be consistently set before
//  we start redo, then change this to eInitLogReadAndPatchFirstLog instead of eInitBaseLogInitDone,
//  to only marginally improve the timing range and stats of redo to forward logs.
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


//  ================================================================
//  Sequence Helper
//  ================================================================
class CIsamSequenceDiagLog : CZeroInit  // isdl
{

    /*

        Example 1 (of Event 105):
        
            Additional Data:
            lgpos[] = 00000040:0001:0000 - 00000086:0001:0000 - 00000115:0008:0FBF - 00000116:0001:0000 (00000115:0001:0000)
            f = 76.655691 s - 213 lgens
            
            Internal Timing Sequence: I
            [1] 0.002821 +J(0) +M(C:0K, Fs:646, WS:2248K # 40K, PF:2508K # 0K, P:2508K)
            [2] 0.002112 +J(0) +M(C:8K, Fs:237, WS:940K # 940K, PF:548K # 488K, P:548K)
            [3] 0.000042 +J(0) +M(C:0K, Fs:18, WS:72K # 72K, PF:64K # 64K, P:64K)
            [4] 0.008936 +J(0) +M(C:0K, Fs:141, WS:556K # 556K, PF:276K # 276K, P:276K)
            [5] 0.000800 +J(0) +M(C:0K, Fs:15, WS:60K # 60K, PF:24K # 24K, P:24K)
            [6] 0.001101 +J(0) +M(C:0K, Fs:23, WS:92K # 92K, PF:28K # 28K, P:28K)
            [7] 0.000603 +J(0) +M(C:0K, Fs:44, WS:176K # 176K, PF:64K # 64K, P:64K)
            [8] 25.224397 -0.485739 (843) CM -2.939594 (143) CB -18.430438 (1215) WT +J(CM:843, PgRf:23623, Rd:1831/828, Dy:295/20637, Lg:3959176/30889) +M(C:1568K, Fs:4718, WS:15848K # 15848K, PF:10228K # 10228K, P:10228K) + 70 lgens
            [9] 51.822035 -2.335206 (1217) CM -5.874344 (287) CB -33.576494 (2176) WT +J(CM:1217, PgRf:53098, Rd:3347/1161, Dy:1378/84127, Lg:8100024/68128) +M(C:4536K, Fs:10500, WS:29720K # 29720K, PF:23256K # 24028K, P:23256K) + 143 lgens
            [10] 0.021178 -0.020581 (1) CB +J(0) +M(C:0K, Fs:1, WS:-56K # 4K, PF:-60K # 0K, P:-60K)
            [11] 17.614491 -4.003910 (832) CM +J(CM:832, PgRf:148294, Rd:0/832, Dy:22004/148604, Lg:9046609/67179) +M(C:90564K, Fs:26805, WS:88996K # 103080K, PF:91708K # 134988K, P:91708K) + 1 lgens
            [12] 16.402431 +J(0) +M(C:0K, Fs:69, WS:236K # 0K, PF:212K # 0K, P:212K)
            [13] 0.374828 -0.021711 (2) CM -0.040737 (2) CB +J(CM:2, PgRf:2, Rd:0/2, Dy:0/0, Lg:4905/5) +M(C:0K, Fs:53, WS:80K # 0K, PF:-20K # 0K, P:-20K)
            [14] 0.000280 +J(0) +M(C:0K, Fs:1, WS:4K # 0K, PF:0K # 0K, P:0K)
            [15] 0.000088 +J(0) +M(C:0K, Fs:9, WS:36K # 0K, PF:0K # 0K, P:0K)
            [16] 0.016479 +J(0) +M(C:0K, Fs:2, WS:0K # 0K, PF:0K # 0K, P:0K).

            Best to unpack line [8], as it's busiest.
                [8] 25.224397 -0.485739 (843) CM -2.939594 (143) CB -18.430438 (1215) WT +J(CM:843, PgRf:23623, Rd:1831/828, Dy:295/20637, Lg:3959176/30889) +M(C:1568K, Fs:4718, WS:15848K # 15848K, PF:10228K # 10228K, P:10228K) + 70 lgens
                    - This step took ~25.2 seconds in total.
                    - CM: We spent 0.485739 seconds in 843 individual cache misses.
                    - CB: We spent 2.939594 seconds in 143 individual callbacks to the application.
                    - WT: We spent 18.430438 seconds in 1215 incidents of busy waiting so recovery could continue.
                    - +J(x,y,z...) - JET_THREADSTATS data ...
                            - CM = CacheMiss, PgRf = PageReferences, Rd = PagePrereads / PageSyncReads, Dy = Dirty / Redirties,
                                Lg = Log Bytes / Log Records
                    - +M(x,y,z...) - OS Memory stats
                            - C = Cache Memory (BF), Fs = OS Page Faults, WS = Working Set size delta / and delta of Peak WS,
                                PF = Page File usage delta / and peak page file usage, P = private bytes delta.
                    - Lgens at end, 70 logs consumed for this phase.

            Also the lgpos[] above in additional data gives some key lgpos's, the order is:
                    = lgposRecoveryStartMin - lgposRecoverySilentRedoEnd - lgposRecoveryVerboseRedoEnd - lgposRecoveryUndoEnd (lgposRecoveryForwardLogs)

            And f = the time to get to forward or future logs, or another way to think of it - the time to perform catch up...
                In this case ~76.6 seconds, and 213 log generations of replay to get caught up.

        Example 2 (of Event 105):

            Internal Timing Sequence: I
            [1] 0.004076 +J(0) +M(C:0K, Fs:834, WS:3312K # 3312K, PF:4972K # 4672K, P:4972K)
            [2] 0.002110 +J(0) +M(C:8K, Fs:307, WS:1032K # 1032K, PF:628K # 628K, P:628K)
            [3] 0.000053 +J(0) +M(C:0K, Fs:18, WS:72K # 72K, PF:64K # 64K, P:64K)
            [4] 0.015550 +J(0) +M(C:0K, Fs:246, WS:680K # 680K, PF:300K # 416K, P:300K)
            [5] 0.000941 +J(0) +M(C:0K, Fs:18, WS:72K # 72K, PF:4K # 0K, P:4K)
            [6] 0.000608 +J(0) +M(C:0K, Fs:16, WS:64K # 64K, PF:12K # 0K, P:12K)
            [7] -
            [8] -
            [9] -
            [10] -
            [11] -
            [12] -
            [13] 0.102435 +J(CM:0, PgRf:0, Rd:0/0, Dy:0/0, Lg:616/1) +M(C:0K, Fs:602, WS:860K # 1324K, PF:92K # 688K, P:92K)
            [14] 0.000441 +J(0) +M(C:0K, Fs:248, WS:992K # 528K, PF:952K # 256K, P:952K)
            [15] 0.000149 +J(0) +M(C:0K, Fs:33, WS:128K # 128K, PF:68K # 68K, P:68K)
            [16] 0.013316 +J(0) +M(C:0K, Fs:7, WS:20K # 24K, PF:0K # 4K, P:0K).

            In this case the blank steps are because eInitLogReadAndPatchFirstLog, through Undo, through recovery BF flush were
            all skipped because this wasn't a dirty shutdown ... this is how you know you recovered from a "clean" / shutdown log
            with no DBs that needed / or that we could recover.
            
    */

public:

    //  This is the fixed data(s) and sequence array of stuff we might collect at any step, 
    //  and is the on-demand allocated pieces of our diag data.  New elements for tracking 
    //  stuff should be placed in one of these sub-structures (and not in CIsamSequenceDiagInfo
    //  itself).

    struct FixedInitData    //  'I'
    {
        LGPOS           lgposRecoveryStartMin;
        LGPOS           lgposRecoverySilentRedoEnd;
        LGPOS           lgposRecoveryVerboseRedoEnd;
        LGPOS           lgposRecoveryUndoEnd;
        HRT             hrtRecoveryForwardLogs;
        LGPOS           lgposRecoveryForwardLogs;
        INT             cReInits;                           //  Tells us how many times we see an init LR.

        // Other things I can imagine being interesting:
        //  cdbAttachedMax - as in actually concurrently attached, 1 for most clients, 2 for Ex + MCDB for instance.
        //  Some form of stats on user RO transactions during redo.
        //  Initial state of the DBs as in dirty vs. dirty and patched vs. clean.
    };

    struct FixedTermData    //  'T'
    {
        LGPOS           lgposTerm;                          // like many of these fixed data, lgposes only valid if logging is on.
    };

    struct FixedCreateData  //  'C'
    {
        LGPOS           lgposCreate;
    };

    struct FixedAttachData  //  'A'
    {
        LGPOS           lgposAttach;
    };

    struct FixedDetachData  //  'D'
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

    static SEQFIXEDDATA         s_uDummyFixedData;      //  Used as dummy variable to shunt code to write to in case of memory failure.

    struct SEQDIAGINFO
    {
        INT                 seq;
        HRT                 hrtEnd;
        __int64             cCallbacks;
        double              secInCallback;      //  So far only implemented for redo/undo phase.
        __int64             cThrottled;
        double              secThrottled;
        DATETIME            dtm;
        JET_THREADSTATS4    thstat;
        __int64             cbCacheMem;
        MEMSTAT             memstat;
    };

private:

    IsamSequenceDiagLogType             m_isdltype;     //  "Type" of sequence.

    BYTE                                m_fPreAllocatedSeqDiagInfo:1;
    BYTE                                m_fAllocFailure:1;
    BYTE                                m_fInitialized:1;
    BYTE                                m_grbitReserved:6;
    BYTE                                m_cseqMax;      //  Max is the maximum reserved / expected by the sequence we're executing.
    BYTE                                m_cseqMac;      //  Mac is one higher than the maximum sequence value triggered, but also
                                                        //      it is the value of the currently accumulating sequence.
    BYTE                                rgbReserved[2]; //  (round to pointer)

    SEQFIXEDDATA *                      m_pFixedData;
    SEQDIAGINFO *                       m_rgDiagInfo;

    //  16 to 24 bytes (x86/amd64 retail) when inactive.

    //  DO NOT add new member variables, should be in Fixed*Data or FIXEDDATA or SEQDIAGINFO above.

#ifdef DEBUG
    DWORD                               m_tidOwner;
#endif

    //  DO NOT add new member variables here either - see above.


    inline bool FValidSequence_( _In_ const INT seq ) const
    {
        Assert( m_cseqMax >= 2 );   //  should be initialized
        return seq >= 0 && seq < (INT)m_cseqMax;
    }

    inline bool FTriggeredSequence_( _In_ const INT seq ) const
    {
        Expected( FActiveSequence() );  //  should be initialized
        Assert( m_rgDiagInfo ); //  such cases should have already been handled ...
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
        //  This should be all setup by the CZeroInit ...
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
        Assert( m_rgDiagInfo == NULL ); // no leaks
        Assert( !FAllocatedFixedData_() ); // no leaks
        Assert( m_pFixedData == &s_uDummyFixedData ); // just to double check
    }

    //
    //  Intialization - 3 options:
    //
    //   1. Basic case - InitSequence(), where it allocates the sequence array and initializes it 
    //      to zero.  This is the most typical method and used for most sequences.
    //   2. Seed case - where we consume a previous InitSequence structure to seed this one.  This 
    //      one is used for Init to preserve / consume the timing data from OS layer operations that
    //      execute before the INST timing sequence is created.
    //   3. Single Step - where we take a preallocated buffer so analysis can't fail and it only    
    //      tracks one step operation.
    //

    void InitSequence( _In_ const IsamSequenceDiagLogType isdltype, _In_range_( 2, 128 ) const BYTE cseqMax );

    //  Note: We destroy / .TermSequence the seed sequence after we cooy the data out.
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

    //
    //  Active Sequence and Triggered Step State accessors
    //

    //  Active from InitSequence() to TermSequence().
    BOOL FActiveSequence() const
    {
        return m_fInitialized;
    }

    //  If the step has been Trigger( seq )'d or not.  Returns fFalse on failed allocs though.
    BOOL FTriggeredStep( _In_ const INT seq ) const
    {
        if ( m_fAllocFailure && m_rgDiagInfo == NULL )
        {
            return fFalse;
        }
        return FTriggeredSequence_( seq );
    }

    //  The configured final step, not the current final step.
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

    //
    //  Update Functions
    //

    SEQFIXEDDATA& FixedData() const
    {
        Expected( FActiveSequence() );  //  if sequence isn't active, like the data you're saving won't be logged properly.
        return *m_pFixedData;
    }

    void Trigger( _In_ const BYTE seqTrigger );
    void AddCallbackTime( const double secsCallback, const __int64 cCallbacks = 1 );
    void AddThrottleTime( const double secsThrottle, const __int64 cThrottled = 1 );
    double GetCallbackTime( __int64 *pcCallbacks );
    double GetThrottleTime( __int64 *pcThrottled );

    //
    //  Data Analysis Functions
    //

    __int64 UsecTimer( _In_ INT seqBegin, _In_ const INT seqEnd ) const;

    ULONG CbSprintFixedData() const
    {
        Assert( m_tidOwner == DwUtilThreadId() );
        //  This is a VERY worst case ... something that allocated on demand would be way 
        //  better, because we have a lot of cases of zeros that we drop from the sprintf
        //  timing stats.
        const ULONG cb = 600; //  from SprintFixedData() Init case ~230 chars
        return cb;
    }
    void SprintFixedData( _Out_writes_bytes_(cbFixedData) WCHAR * wszFixedData, _In_range_( 4, 4000 ) ULONG cbFixedData) const;

    ULONG CbSprintTimings() const
    {
        Assert( m_tidOwner == DwUtilThreadId() );
        //  This is a VERY worst case ... something that allocated on demand would be way 
        //  better, because we have a lot of cases of zeros that we drop form the sprintf
        //  timing stats.
        const ULONG cb = ( (ULONG)m_cseqMax * 213 /* sum of "Worst case:" from below */ + 10 /* prefix/suffix fuzz */ ) * sizeof(WCHAR);
        return cb;
    }

    void SprintTimings( _Out_writes_bytes_(cbTimeSeq) WCHAR * wszTimeSeq, _In_range_( 4, 4000 ) ULONG cbTimeSeq ) const;
    
};


static const INT perfStatusRecoveryRedo     = 1;
static const INT perfStatusRecoveryUndo     = 2;
static const INT perfStatusRuntime          = 3;
static const INT perfStatusTerm             = 4;
static const INT perfStatusError            = 5;


//  ================================================================
class INST
    :   public CZeroInit
//  ================================================================
{
    friend class APICALL_INST;
public:

    INST( INT iInstance );
    ~INST();

#pragma push_macro( "new" )
#undef new
    private:
        void* operator new[]( size_t );         //  not supported
        void operator delete[]( void* );        //  not supported
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
    INT                 m_perfstatusEvent;  //  Redo, Undo, Runtime/Do-time, and Term.
    
    BOOL                m_fBackupAllowed;
    volatile BOOL       m_fStopJetService;
    ERR                 m_errInstanceUnavailable;
    BOOL                m_fNoWaypointLatency;
    CESESnapshotSession *       m_pOSSnapshotSession;   // NULL if not participating in snapshot
    __int64             m_ftInit;

    //  instance configuration

    CJetParam*          m_rgparam;

    JET_GRBIT           m_grbitCommitDefault;
    CJetParam::Config   m_config;

    //  LV

    CCriticalSection    m_critLV;
    PIB *               m_ppibLV;

    //  ver and log

    VER                 *m_pver;
    LOG                 *m_plog;
    BACKUP_CONTEXT      *m_pbackup;

    //  IO related. Mapping ifmp to ifmp for log records.

    IFMP                m_mpdbidifmp[ dbidMax ];

    //  update id of this instance

    UPDATEID            m_updateid;

    //  pib related

    volatile TRX        m_trxNewest;
    CCriticalSection    m_critPIB;
    RWLPOOL< PIB >      m_rwlpoolPIBTrx;
    CResource           m_cresPIB;
    PIB* volatile       m_ppibGlobal;
    // During recovery read-only transactions get a trxBegin0 relative to the ones seen in recovery. Have
    // we seen any trxid's during recovery yet to allow us to assign an appropriate trxBegin0 yet?
    CAutoResetSignal    m_sigTrxOldestDuringRecovery;
    BOOL                m_fTrxNewestSetByRecovery;

    // RBS related
    CRevertSnapshot     *m_prbs;                    // revert snapshot instance.
    RBSCleaner          *m_prbscleaner;             // Manages deleting of older revert snapshots that either age out or have to be removed due to disk space pressure

private:
    volatile TRX        m_trxOldestCached;      // may be out-of-date
public:
    
    //  FCB Pool

    CResource           m_cresFCB;
    CResource           m_cresTDB;
    CResource           m_cresIDB;

    //  FCB Hash table

    FCBHash             *m_pfcbhash;

    //  LRU list of all file FCBs whose wRefCnt == 0.

    CCriticalSection    m_critFCBList;
    FCB                 *m_pfcbList;            //  list of ALL FCBs (both available and unavailable)
    FCB                 *m_pfcbAvailMRU;        //  list of available FCBs
    FCB                 *m_pfcbAvailLRU;
    ULONG               m_cFCBAvail;            //  number of available FCBs
    volatile ULONG      m_cFCBAlloc;            //  number of allocated FCBs

    //  FCB creation mutex (using critical section for deadlock detection info)

    CCriticalSection    m_critFCBCreate;

    //  Reference count tracing for FCBs

    POSTRACEREFLOG      m_pFCBRefTrace;

    //  FUCB

    CResource           m_cresFUCB;

    //  SCB

    CResource           m_cresSCB;

    //  task manager for PIB based tasks

    CGPTaskManager      m_taskmgr;
    volatile LONG       m_cOpenedSystemPibs;
    volatile LONG       m_cUsedSystemPibs;  //  instantaneous count of PIBs in use

    //  OLD
    CCriticalSection    m_critOLD;
    OLDDB_STATUS        *m_rgoldstatDB;

    //  file-system

    IFileSystemConfiguration*   m_pfsconfig;
    IFileSystemAPI*     m_pfsapi;

    //  Inc-ReSeed

    CIrsOpContext **    m_rgpirs;   //  note: dbidMax large IF allocated, free'd at term.
    CRBSRevertContext*             m_prbsrc;   //  revert snapshot revert context

    JET_PFNINITCALLBACK m_pfnInitCallback;
    void *              m_pvInitCallbackContext;
    BOOL                m_fAllowAPICallDuringRecovery;

    //  BF variables

    BOOL                m_fFlushLogForCleanThread;

    volatile BOOL       m_fTempDBCreated;

private:
    volatile LONG       m_cIOReserved;
    LONG                m_cNonLoggedIndexCreators;

    //  4 bytes wasted here.

    //  keep a copy of these globals to facilitate debugging
    //
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

    //  critical section for temp db create
    CCriticalSection    m_critTempDbCreate;

public:
    static LONG         iInstanceActiveMin;
    static LONG         iInstanceActiveMac;

    static LONG         cInstancesCounter;

    //  Processor Local Storage for this Instance

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

    //  Stop / Cancel Service

    TICK                        m_tickStopped;
    TICK                        m_tickStopCanceled;
    JET_GRBIT                   m_grbitStopped;
    BOOL                        m_fCheckpointQuiesce;

    COSEventTraceIdCheck        m_traceidcheckInst;

    //  LARGE data in the INST.

    BOOL                        m_rgfStaticBetaFeatures[EseFeatureMax];

    ULONG                       m_dwLCMapFlagsDefault;          //  LC map flags
    WCHAR                       m_wszLocaleNameDefault[NORM_LOCALE_NAME_MAX_LENGTH];    // Locale name

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

    // After E15/win8, temp DB is no longer created during init phase.
    // temp DB is created only when it may be used.
    ERR ErrINSTCreateTempDatabase();

    VOID SaveDBMSParams( DBMS_PARAM *pdbms_param );
    VOID RestoreDBMSParams( DBMS_PARAM *pdbms_param );
    ERR ErrAPIEnter( const BOOL fTerminating );
private:
    // Only APICALL_INST should be accessing these functions:
    ERR ErrAPIEnterFromTerm();
    ERR ErrAPIEnterForInit();
    ERR ErrAPIEnterWithoutInit( const BOOL fAllowInitInProgress );
public:
    VOID APILeave();

    BOOL    FInstanceUnavailable() const;
    ERR     ErrInstanceUnavailableErrorCode() const;
    VOID    SetInstanceUnavailable( const ERR err );

    ERR     ErrCheckForTermination() const;

    // These methods are used to control the performance counter and event status which reports the current status
    // of the instance.
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
            maskAPIReserved             = 0x80000000,   //  WARNING: don't use high bit to avoid sign problems
            fAPIInitializing            = 0x10000000,
            fAPITerminating             = 0x20000000,
            fAPIRestoring               = 0x40000000,
            fAPICheckpointing           = 0x01000000,
            fAPIDeleting                = 0x02000000,   //  deleting the pinst
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
    ULONG CAttachedUserDBs( ) const;    // not paticularlly efficient

    LONG CNonLoggedIndexCreators() const        { return m_cNonLoggedIndexCreators; }
    VOID IncrementCNonLoggedIndexCreators()     { AtomicIncrement( &m_cNonLoggedIndexCreators ); }
    VOID DecrementCNonLoggedIndexCreators()     { AtomicDecrement( &m_cNonLoggedIndexCreators ); }

    TRX TrxOldestCached() const { return m_trxOldestCached; }
    void SetTrxOldestCached(const TRX trx) { m_trxOldestCached = trx; }
    
    ERR ErrReserveIOREQ();
    VOID UnreserveIOREQ();

#ifdef DEBUGGER_EXTENSION
    VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif  //  DEBUGGER_EXTENSION

    static VOID EnterCritInst();
    static VOID LeaveCritInst();

#ifdef DEBUG
    static BOOL FOwnerCritInst();
#endif  //  DEBUG

    static INST * GetInstanceByName( PCWSTR wszInstanceName );
    static INST * GetInstanceByFullLogPath( PCWSTR wszLogPath );

    static LONG AllocatedInstances() { return cInstancesCounter; }
    static LONG IncAllocInstances() { return AtomicIncrement( &cInstancesCounter ); }
    static LONG DecAllocInstances() { return AtomicDecrement( &cInstancesCounter ); }
    static ERR  ErrINSTSystemInit();
    static VOID INSTSystemTerm();
}; // end of class INST

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

    //  verify we found entry in the instance array
    EnforceSz( ipinst < g_cpinstMax, "CorruptedInstArrayInIpinstFromPinst" );

    Assert( ipinst + 1 == (ULONG)pinst->m_iInstance );

    return ipinst;
}

PIB * const     ppibSurrogate   = (PIB *)( lBitsAllFlipped << 1 );  //  "Surrogate" or place-holder PIB for exclusive latch by proxy
const PROCID    procidSurrogate = (PROCID)0xFFFE;                   //  "Surrogate" or place-holder PROCID for exclusive latch by proxy
const PROCID    procidReadDuringRecovery = (PROCID)0xFFFD;          //  procid to use for read from passive transactions
const PROCID    procidRCEClean = (PROCID)0xFFFC;                    //  procid to use for RCE clean
const PROCID    procidRCECleanCallback = (PROCID)0xFFFB;            //  procid to use for RCE clean callback


INLINE void UtilAssertNotInAnyCriticalSection()
{
#ifdef SYNC_DEADLOCK_DETECTION
    AssertSz( !Pcls()->pownerLockHead || Ptls()->fInCallback, "Still in a critical section" );
#endif  //  SYNC_DEADLOCK_DETECTION
}

INLINE void UtilAssertCriticalSectionCanDoIO()
{
#ifdef SYNC_DEADLOCK_DETECTION
    AssertSz(   !Pcls()->pownerLockHead || Pcls()->pownerLockHead->m_plddiOwned->Info().Rank() > rankIO,
                "In a critical section we cannot hold over I/O" );
#endif  //  SYNC_DEADLOCK_DETECTION
}


//  ================================================================
INLINE BOOL INST::FInstanceUnavailable() const
//  ================================================================
{
    return ( JET_errSuccess != m_errInstanceUnavailable );
}

//  ================================================================
INLINE ERR INST::ErrInstanceUnavailableErrorCode() const
//  ================================================================
{
    //  should only call this function after a
    //  call to FInstanceUnavailable() returns TRUE
    //  to return the appropriate instance-unavailable
    //  error code
    //
    Assert( FInstanceUnavailable() );

    return ErrERRCheck(
            ( JET_errLogDiskFull == m_errInstanceUnavailable ) ?
                                JET_errInstanceUnavailableDueToFatalLogDiskFull :
                                JET_errInstanceUnavailable );
}


//  ================================================================
INLINE ERR INST::ErrCheckForTermination() const
//  ================================================================
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
                //  tried to create an empty hash table - something is amiss
                //
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

            //  should never reach the end of the list without
            //  finding the desired entry
            //
            Assert( fFalse );
        }

        CEntry * PentryOfHash( const ULONG ulHash ) const
        {
            return m_rgpentries[ulHash];
        }
};


//  ================================================================
//      JET PARAMETER ACCESS
//  ================================================================

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

    //  if no instance was specified then either there must not be any instances
    //  initialized or the parameter must be intrinsically global
    //
    Assert( pinst != pinstNil ||
            ( 0 == INST::AllocatedInstances() ) ||
            g_rgparam[ paramid ].FGlobal() );

    //  lookup the storage location for this paramid.  if no instance was
    //  specified or if the parameter is intrinsically global then we will
    //  always go to the global array
    //
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

    //  we cannot use this for a custom parameter because their value is not
    //  directly managed by CJetParam
    //  JET_paramConfigStoreSpec is kind of hybrid, it stores the string in the parameter
    //  for diagnostics, but it actually stores it's real data in a CConfigStore object
    //  at various stages.
    //
    Assert( !g_rgparam[ paramid ].FCustomStorage() || paramid == JET_paramConfigStoreSpec );

    //  in theory I suppose it could be a RO param w/ NULL setting func, as then it could
    //  never change ... so add the or clause if necessary ...
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

// Returns the number of ESE-instance objects that will show up in perfmon.
INT CPERFESEInstanceObjects( const INT cUsedInstances );

// Updates the list of cached database names to be used as performance counter instance names.
VOID PERFSetDatabaseNames( IFileSystemAPI* const pfsapi );

// Returns the number of TCE objects that will show up in perfmon, including internal and user-defined ones.
LONG CPERFTCEObjects();

// Returns the number of database objects that will show up in perfmon.
ULONG CPERFDatabaseObjects( const ULONG cIfmpsInUse );

// Returns the number of objects that need to be pre-allocated (i.e., minimum required).
ULONG CPERFObjectsMinReq();

// This class is responsible for deleting the global variable wszTableClassNames.
// The destructor is called during DLL detach.
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

            // This should never change between calls.
            Assert( m_cchTableClassNames == 0 || m_cchTableClassNames == cchTableClassNames );

            m_cchTableClassNames = cchTableClassNames;
        HandleError:
            return err;
        }

    private:
        WCHAR* m_wszTableClassNames;
        size_t m_cchTableClassNames;
};

//  the following are some VERY commonly used parameters
//


//
//     DB Page Size
//

//  BF needs to know global / multi-inst worst case page size we may use.

LONG CbSYSMaxPageSize();

//  Log needs to know inst-wide what is the worst case page size we may use.

LONG CbINSTMaxPageSize( const INST * const pinst );

//  Moving off of a global constant for page size ... eventually we would like to move uses 
//  of this variable to FMP specific calls pfmp->CbPage().
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

//  Generic count of blocks to capture a given cb (that isn't an even block size).
INLINE CPG CpgEnclosingCb( ULONG cbPage, QWORD cb )   { return (CPG) ( ( cb / cbPage ) + ( ( cb % cbPage ) ? 1 : 0 ) ); }

//  A place with all the storage / disk / controller induced errors that occur against the database.
//  Note: this is for the database only, another set of errors is in FErrIsLogCorruption().

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
