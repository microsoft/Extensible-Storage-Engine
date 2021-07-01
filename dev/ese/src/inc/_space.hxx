// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _SPACE_H_INCLUDED
#define _SPACE_H_INCLUDED


/*PERSISTED*/ typedef enum class SpacePool : ULONG
{
    //  Explicit Pools - except the 0 / AvailExtLegacyGeneralPool, these are stored/persisted
    //  explicitly in the byte at front of 5-byte avail pool key.    
    MinPool                          = 0x00,
    AvailExtLegacyGeneralPool        = 0x00, //  General / Legacy Free Pool.
    ContinuousPool                   = 0x01, //  Reserved for contiguous allocations.
    ShelvedPool                      = 0x02, //  Shelved page (available space beyond EOF, which can't be used).
    MaxPool,

    //   Virtual Pools - these pools exist by the context in where the extent was found, and only used to
    //     communicate this fact out the space APIs.  They are not persisted.
    PrimaryExt                       = 0x00010000,   //  Used to mark an extent that is within the primary extent.
    OwnedTreeAvail                   = 0x00020000,   //  Available pages reserved for use to split (i.e. in spbuf) the Owned Extents Tree.
    AvailTreeAvail                   = 0x00030000,   //  Available pages reserved for use to split (i.e. in spbuf) the Avail Extents Tree.
} spp;

static_assert( 0 == (ULONG)spp::AvailExtLegacyGeneralPool, "Persisted and so must be immutable" );
static_assert( 1 == (ULONG)spp::ContinuousPool,            "Persisted and so must be immutable" );
static_assert( 2 == (ULONG)spp::ShelvedPool,               "Persisted and so must be immutable" );

inline BOOL FSPIValidExplicitSpacePool( SpacePool spp ) { return ( ( spp::MinPool <= spp ) && ( spp < spp::MaxPool ) ); };

inline const WCHAR * const WszPoolName( _In_ const SpacePool spp )
{
    switch( spp )
    {
        case spp::AvailExtLegacyGeneralPool:  return L"Gen";
        case spp::ContinuousPool:             return L"Cont";
        case spp::ShelvedPool:                return L"Shvd";
        case spp::PrimaryExt:                 return L"Pri";
        case spp::OwnedTreeAvail:             return L"OeRes";
        case spp::AvailTreeAvail:             return L"AeRes";
    }

    Assert( fFalse );
    if ( spp == spp::MaxPool )
    {
        // A special case of unknown.  But you shouldn't be asking for the name of this, it
        // doesn't really have one.
        return L"UnknMax";
    }

    return L"Unkn";
}

// Incrementors for SpacePool (i.e. ++SpacePool and SpacePool++) 
INLINE SpacePool& operator++( SpacePool &spp )
{
    // The value you're incrementing is in the normal range, right?
    Assert( ( spp::MinPool <= spp ) && ( spp::MaxPool > spp) );

    using IntType = typename std::underlying_type<SpacePool>::type;
    spp = static_cast<SpacePool>( static_cast<IntType>(spp) + 1 );
    return spp;
}

INLINE SpacePool operator++( SpacePool &spp, int )
{
    SpacePool result = spp;
    ++spp;
    return result;
}


#ifndef SPACE_ONLY_DIAGNOSTIC_CONSTANTS
//  internal space functions called by recovery
//
VOID SPIInitPgnoFDP(
    FUCB                *pfucb,
    CSR                 *pcsr,
    const SPACE_HEADER& sph  );

VOID SPIPerformCreateMultiple(
    FUCB            *pfucb,
    CSR             *pcsrFDP,
    CSR             *pcsrOE,
    CSR             *pcsrAE,
    PGNO            pgnoLast,
    CPG             cpgPrimary );

VOID SPICreateExtentTree(
    FUCB            *pfucb,
    CSR             *pcsr,
    PGNO            pgnoLast,
    CPG             cpgExtent,
    BOOL            fAvail );

ERR ErrSPICreateSingle(
    FUCB            *pfucb,
    CSR             *pcsr,
    const PGNO      pgnoParent,
    const PGNO      pgnoFDP,
    const OBJID     objidFDP,
    CPG             cpgPrimary,
    const BOOL      fUnique,
    const ULONG     fPageFlags,
    const DBTIME    dbtime );

VOID SPIConvertGetExtentinfo(
    FUCB            *pfucb,
    CSR             *pcsrRoot,
    SPACE_HEADER    *psph,
    EXTENTINFO      *rgext,
    INT             *piextMac );

VOID SPIConvertCalcExtents(
    const SPACE_HEADER& sph,
    const PGNO          pgnoFDP,
    EXTENTINFO          *rgext,
    INT                 *pcext );

VOID SPIPerformConvert(
    FUCB            *pfucb,
    CSR             *pcsrRoot,
    CSR             *pcsrAE,
    CSR             *pcsrOE,
    SPACE_HEADER    *psph,
    PGNO            pgnoSecondaryFirst,
    CPG             cpgSecondary,
    EXTENTINFO      *rgext,
    INT             iextMac );

ERR ErrSPIOpenAvailExt(
    PIB     *ppib,
    FCB     *pfcb,
    FUCB    **ppfucbAE );

ERR ErrSPIOpenAvailExt(
    FUCB *pfucb,
    FUCB **ppfucbAE );

ERR ErrSPIOpenOwnExt(
    PIB     *ppib,
    FCB     *pfcb,
    FUCB    **ppfucbOE );

ERR ErrSPIOpenOwnExt(
    FUCB    *pfucb,
    FUCB    **ppfucbOE );

ERR ErrSPIGetExtentInfo(
    _In_ const FUCB             *pfucb,
    _Out_ PGNO                  *ppgnoLast,
    _Out_ CPG                   *pcpgSize,
    _Out_ SpacePool             *psppPool );

ERR ErrSPIGetExtentInfo(
    _In_ const KEYDATAFLAGS     *pkdf,
    _Out_ PGNO                  *ppgnoLast,
    _Out_ CPG                   *pcpgSize,
    _Out_ SpacePool             *psppPool );

ERR ErrSPITrimUpdateDatabaseHeader( const IFMP ifmp );

ERR ErrSPIREPAIRValidateSpaceNode(
    _In_ const  KEYDATAFLAGS * pkdf,
    _Out_       PGNO *          ppgnoLast,
    _Out_       CPG *           pcpgExtent,
    _Out_       SpacePool *     sppPool );

const CPG   cpgSmallFDP                 = 16;   //  count of owned pages below which an FDP
const CPG   cpgSmallGrow                = 4;    //  minimum count of pages to grow a small FDP

const CPG   cpgSmallDbSpaceSize         = 254;  //  use small DB alloc policies, while DB is smaller than this
const PGNO  pgnoSmallDbSpaceStart       = 128;  //  use small DB alloc policies, when allocation starts in this area of DB

const CPG   cpgSmallSpaceAvailMost      = 32;   //  maximum number of pages allocatable from single extent space format

const CPG   cpgMultipleExtentConvert    = 2;    //  minimum pages to preallocate when converting to multiple extent
                                                //  (enough for OE/AE)

const CPG   cpgMaxRootPageSplit         = 2;    //  max. theoretical pages required to satisfy split on single-level tree
const CPG   cpgPrefRootPageSplit        = 2;    //  preferred pages to satisfy split on single-level tree

const CPG   cpgMaxParentOfLeafRootSplit = 3;    //  max. theoretical pages required to satisfy split on 2-level tree
const CPG   cpgPrefParentOfLeafRootSplit = 8;   //  preferred pages to satisfy split on 2-level tree

const CPG   cpgMaxSpaceTreeSplit        = 4;    //  max. theoretical pages required to satisfy space tree split (because max. depth is 4)
const CPG   cpgPrefSpaceTreeSplit       = 16;   //  preferred pages to satisfy space tree split

#endif // SPACE_ONLY_DIAGNOSTIC_CONSTANTS


#endif  // _SPACE_H_INCLUDED

