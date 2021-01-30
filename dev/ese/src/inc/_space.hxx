// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _SPACE_H_INCLUDED
#define _SPACE_H_INCLUDED


 typedef enum class SpacePool : ULONG
{
    MinPool                          = 0x00,
    AvailExtLegacyGeneralPool        = 0x00,
    ContinuousPool                   = 0x01,
    ShelvedPool                      = 0x02,
    MaxPool,

    PrimaryExt                       = 0x00010000,
    OwnedTreeAvail                   = 0x00020000,
    AvailTreeAvail                   = 0x00030000,
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
        return L"UnknMax";
    }

    return L"Unkn";
}

INLINE SpacePool& operator++( SpacePool &spp )
{
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

ERR ErrSPIOpenOwnExt(
    PIB     *ppib,
    FCB     *pfcb,
    FUCB    **ppfucbOE );

ERR ErrSPIGetExtentInfo(
    __in const FUCB             *pfucb,
    __out PGNO                  *ppgnoLast,
    __out CPG                   *pcpgSize,
    __out SpacePool             *psppPool );

ERR ErrSPIGetExtentInfo(
    __in const KEYDATAFLAGS     *pkdf,
    __out PGNO                  *ppgnoLast,
    __out CPG                   *pcpgSize,
    __out SpacePool             *psppPool );

ERR ErrSPITrimUpdateDatabaseHeader( const IFMP ifmp );

ERR ErrSPIREPAIRValidateSpaceNode(
    __in const  KEYDATAFLAGS * pkdf,
    __out       PGNO *          ppgnoLast,
    __out       CPG *           pcpgExtent,
    __out       SpacePool *     sppPool );

const CPG   cpgSmallFDP                 = 16;
const CPG   cpgSmallGrow                = 4;

const CPG   cpgSmallDbSpaceSize         = 254;
const PGNO  pgnoSmallDbSpaceStart       = 128;

const CPG   cpgSmallSpaceAvailMost      = 32;

const CPG   cpgMultipleExtentConvert    = 2;

const CPG   cpgMaxRootPageSplit         = 2;
const CPG   cpgPrefRootPageSplit        = 2;

const CPG   cpgMaxParentOfLeafRootSplit = 3;
const CPG   cpgPrefParentOfLeafRootSplit = 8;

const CPG   cpgMaxSpaceTreeSplit        = 4;
const CPG   cpgPrefSpaceTreeSplit       = 16;

#endif


#endif

