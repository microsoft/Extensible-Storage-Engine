// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "esefile.hxx"

#include "xsum.hxx"

static BOOL FIsSmallPage( ULONG cbPage )    { return cbPage <= 1024 * 8; }
static BOOL FIsSmallPage( void )            { return FIsSmallPage( g_cbPage ); }


//  ================================================================
static XECHECKSUM LongChecksumFromShortChecksum( const ULONG xorChecksum, const ULONG pgno )
//  ================================================================
{
    Assert( 0 != pgno );
    XECHECKSUM checksum = ( ( XECHECKSUM )pgno ) << 32 | xorChecksum;
    return checksum;
}

//  ================================================================
inline BOOL FGetBit( const void * const pv, const INT ibitOffset )
//  ================================================================
{
    const unsigned char * const pb  = (unsigned char *)pv;
    const INT   ibyte       = ibitOffset / 8;
    const INT   ibitInByte  = ibitOffset % 8;
    const unsigned char     bitMask     = (unsigned char)( 1 << ibitInByte );

    return ( pb[ibyte] & bitMask ) ? fTrue : fFalse;
}

//  ================================================================
static UINT IbitNewChecksumFormatFlag( const PAGETYPE pagetype )
//  ================================================================
{
    if( databasePage == pagetype )
    {
        //  for database pages, the page flags are stored in the 10th
        //  unsigned long. The format bit is 0x1000, which is the 13th bit.
        return ( 9 * 32 ) + 13;
    }
    return ( UINT )-1;
}

//  ================================================================
static BOOL FPageHasNewChecksumFormat( const void * const pv, const PAGETYPE pagetype )
//  ================================================================
//
//  Returns fTrue if the ECC format bit for the page is set. Database pages
//  and header pages have different formats, so we have to store the bit
//  in different places
//
//-
{
    if( databasePage == pagetype )
    {
        const INT ibit = IbitNewChecksumFormatFlag( pagetype );
        return FGetBit( pv, ibit );
    }
    return 0;
}

//  ================================================================
static BOOL FPageHasLongChecksum( const PAGETYPE pagetype )
//  ================================================================
{
    return ( databasePage == pagetype );
}

//  ================================================================
static ULONG ShortChecksumFromPage( const void * const pv )
//  ================================================================
{
    return *( const ULONG* )pv;
}

//  ================================================================
static PAGECHECKSUM LongChecksumFromPage( const void * const pv )
//  ================================================================
{
    const PGHDR2* const pPH = ( const PGHDR2* )pv;
    if ( FIsSmallPage() )
    {
        return PAGECHECKSUM( pPH->checksum );
    }
    else
    {
        return PAGECHECKSUM( pPH->checksum, pPH->rgChecksum[ 0 ], pPH->rgChecksum[ 1 ], pPH->rgChecksum[ 2 ] );
    }
}

//  ================================================================
static PAGECHECKSUM ChecksumFromPage( const void * const pv, const PAGETYPE pagetype )
//  ================================================================
{
    if( FPageHasLongChecksum( pagetype ) )
    {
        return LongChecksumFromPage( pv );
    }
    return ShortChecksumFromPage( pv );
}

//  ================================================================
static PAGECHECKSUM ComputePageChecksum(
    const void* const pv,
    const UINT cb,
    const PAGETYPE pagetype,
    const ULONG pgno,
    // set fNew to compute new ECC for a page (R/W wrt the large page!!)
    // reset fNew to computer ECC for verification purpose (R/O wrt the page)
    const BOOL fNew = fFalse )
//  ================================================================
{
    if( FPageHasLongChecksum( pagetype ) )
    {
        if( FPageHasNewChecksumFormat( pv, pagetype ) )
        {
            // large pages (16/32kiB) always have new checksum format
            PAGECHECKSUM pgChecksum;
            const unsigned char* const pch = ( unsigned char* )pv;

            // divide a large page into 4 blocks, first block is header block which hosts the page header
            unsigned cbT = cb;
            if ( !FIsSmallPage( cb ) )
            {
                cbT = cb / cxeChecksumPerPage;
                pgChecksum.rgChecksum[ 1 ] = ChecksumNewFormat( pch + cbT * 1, cbT, pgno, fFalse );
                pgChecksum.rgChecksum[ 2 ] = ChecksumNewFormat( pch + cbT * 2, cbT, pgno, fFalse );
                pgChecksum.rgChecksum[ 3 ] = ChecksumNewFormat( pch + cbT * 3, cbT, pgno, fFalse );

                // write checksums into designated location in header block
                // so checksum for header block can protect them as well
                if ( fNew )
                {
                    // cast RO ( const void* ) to RW ( PGHDR2* )
                    PGHDR2* const pPgHdr2 = ( PGHDR2* )pv;
                    pPgHdr2->rgChecksum[ 0 ] = pgChecksum.rgChecksum[ 1 ];
                    pPgHdr2->rgChecksum[ 1 ] = pgChecksum.rgChecksum[ 2 ];
                    pPgHdr2->rgChecksum[ 2 ] = pgChecksum.rgChecksum[ 3 ];
                }
            }

            // whole small page or header block for large page
            pgChecksum.rgChecksum[ 0 ] = ChecksumNewFormat( pch, cbT, pgno, fTrue );
            return pgChecksum;
        }
        else
        {
            return LongChecksumFromShortChecksum( ChecksumOldFormat((unsigned char *)pv, cb), pgno );
        }
    }

    return ChecksumOldFormat((unsigned char *)pv, cb);
}

//  ================================================================
inline void FlipBit( void * const pv, const INT ibitOffset )
//  ================================================================
{
    unsigned char * const   pb      = (unsigned char *)pv;
    const INT   ibyte       = ibitOffset / 8;
    const INT   ibitInByte  = ibitOffset % 8;
    const unsigned char     bitMask     = (unsigned char)( 1 << ibitInByte );

    pb[ibyte] ^= bitMask;
}

//  ================================================================
enum XECHECKSUMERROR { xeChecksumNoError = 0, xeChecksumCorrectableError = -13, xeChecksumFatalError = -29, };

static XECHECKSUMERROR FTryFixBlock(
    const UINT cb,
    UINT* const pibitCorrupted,         // [in] bit offset of XECHECKSUM flag, [out] bit offset of proposed correction
    const XECHECKSUM xeChecksumExpected,
    const XECHECKSUM xeChecksumActual )
//  ================================================================
{
    if ( xeChecksumExpected == xeChecksumActual )
    {
        return xeChecksumNoError;
    }

    if ( FECCErrorIsCorrectable( cb, xeChecksumExpected, xeChecksumActual ) )
    {
        const UINT ibitCorrupted = IbitCorrupted( cb, xeChecksumExpected, xeChecksumActual );

        // *pibitCorrupted is the offset of the BIT we can NOT flip
        if ( ( *pibitCorrupted != ibitCorrupted ) && ( ibitCorrupted < 8 * cb ) )
        {
            *pibitCorrupted = ibitCorrupted;
            return xeChecksumCorrectableError;
        }
    }

    return xeChecksumFatalError;
}

//  ================================================================
static void TryFixPage(
    void * const pv,
    const UINT cb,
    const PAGETYPE pagetype,
    const BOOL fCorrectError,
    BOOL * const pfCorrectableError,
    INT * const pibitCorrupted,
    const PAGECHECKSUM checksumExpected,
    const PAGECHECKSUM checksumActual )
//  ================================================================
{
    Assert( checksumActual != checksumExpected );

    const BOOL fSmallPage = FIsSmallPage( cb );
    const UINT cbT = fSmallPage ? cb : cb / cxeChecksumPerPage;
    const UINT cblk = fSmallPage ? 1 : cxeChecksumPerPage;
    XECHECKSUMERROR rgErr[ cxeChecksumPerPage ] = { xeChecksumNoError, };

    UINT rgibitCorrupted[ cxeChecksumPerPage ] = { IbitNewChecksumFormatFlag( pagetype ), UINT_MAX, UINT_MAX, UINT_MAX, };
    UINT ibitCorrupted = UINT_MAX;

    // work out correction
    for ( UINT iblk = 0; iblk < cblk; ++iblk )
    {
        rgErr[ iblk ] = FTryFixBlock( cbT, &rgibitCorrupted[ iblk ], checksumExpected.rgChecksum[ iblk ], checksumActual.rgChecksum[ iblk ] );
        switch ( rgErr[ iblk ] )
        {
            case xeChecksumNoError:
                break;

            case xeChecksumCorrectableError:
                // save the location of first correctable error
                if ( UINT_MAX == ibitCorrupted )
                {
                    ibitCorrupted = CHAR_BIT * cbT * iblk + rgibitCorrupted[ iblk ];

                    // does this correctable error actually hit other checksums
                    UINT ibitStart = CHAR_BIT * offsetof( PGHDR2, rgChecksum );
                    UINT ibitLength = CHAR_BIT * sizeof( checksumExpected.rgChecksum[ 0 ] );
                    if ( !fSmallPage &&                 // large page
                        0 == iblk &&                    // header block
                        ibitStart <= ibitCorrupted  &&  // hit one of the other checksums
                        ibitCorrupted < ibitStart + ibitLength * ( cxeChecksumPerPage - 1 ) )
                    {
                        // fix the other hit checksum
                        UINT ibit = ibitLength + ( ibitCorrupted - ibitStart );
                        FlipBit( ( void* )&checksumExpected, ibit );
                    }
                }
                break;

            case xeChecksumFatalError:
                *pfCorrectableError = fFalse;
                return;
        }
    }

    // when multiple correctables, just report first one
    *pfCorrectableError = fTrue;
    *pibitCorrupted = ibitCorrupted;
    Assert( (UINT)*pibitCorrupted < ( 8 * cb ) );

    // carry out correction in page
    for ( UINT iblk = 0; fCorrectError && iblk < cblk; ++iblk )
    {
        unsigned char* pch = ( unsigned char* )pv;
        if ( xeChecksumCorrectableError == rgErr[ iblk ] )
        {
            FlipBit( &pch[ cbT * iblk ], rgibitCorrupted[ iblk ]);
        }
    }

    return;
}

//  ================================================================
void ChecksumAndPossiblyFixPage(
    void * const pv,                    //  pointer to the page
    const UINT cb,              //  size of the page (normally g_cbPage)
    const PAGETYPE pagetype,            //  type of the page
    const ULONG pgno,
    const BOOL fCorrectError,           //  fTrue if ECC should be used to correct errors
    PAGECHECKSUM * const pchecksumExpected, //  set to the checksum the page should have
    PAGECHECKSUM * const pchecksumActual,   //  set the the actual checksum. if actual != expected, JET_errReadVerifyFailure is returned
    BOOL * const pfCorrectableError,    //  set to fTrue if ECC could correct the error (set even if fCorrectError is fFalse)
    INT * const pibitCorrupted )        //  offset of the corrupted bit (meaningful only if *pfCorrectableError is fTrue)
//  ================================================================
{
    *pfCorrectableError = fFalse;
    *pibitCorrupted     = -1;

    *pchecksumExpected  = ChecksumFromPage( pv, pagetype );
    *pchecksumActual    = ComputePageChecksum( pv, cb, pagetype, pgno );

    const BOOL fNewChecksumFormat = FPageHasNewChecksumFormat( pv, pagetype );
    if( *pchecksumActual != *pchecksumExpected && fNewChecksumFormat )
    {
        TryFixPage( pv, cb, pagetype, fCorrectError, pfCorrectableError, pibitCorrupted, *pchecksumExpected, *pchecksumActual );
        Assert( ( *pfCorrectableError && *pibitCorrupted != -1 ) || ( !*pfCorrectableError && *pibitCorrupted == -1 ) );

        // no point in re-computing  the checksum if we haven't done any changes
        if ( fCorrectError && *pfCorrectableError )
        {
            *pchecksumExpected = ChecksumFromPage( pv, pagetype );
            *pchecksumActual = ComputePageChecksum( pv, cb, pagetype, pgno, fTrue );
        }
    }
}


