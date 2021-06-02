// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

/*******************************************************************

Each database page and database header page contains a 8-byte checksum.
The checksum is the first 8 bytes on the page. The checksum incorporates
the page number so that getting back the wrong page will generate a
checksum error.

There are two different formats of page checksums, called (imaginatively
enough) old and new:

- Old:  this is the format used by 5.5 through Exchange 2003 SP1, and
        Windows 2000 through Longhorn. The first DWORD of the checksum
        is an XOR of all DWORDs on the page (except the checksum) along
        with the seed value of 0x89abcdef. The second DWORD is the pgno
        of the page. XOR is used because it's very easy to optimize on
        processors with wide words lengths.

        The old checksum format is actually a 4-byte format, we expand
        it to 8-bytes for pages by making the pgno check part of checksum
        validation. Database and logfile headers don't have a pgno, so
        they in fact have a 4-byte checksum.

- New:  XOR checksums can only detect problems, not correct them. ECC
        checksums can correct single-bit errors in pages, which are actually
        a significant percentage of problems we see. Calculating and verifying
        ECC checksums is more expensive though.

        The new checksum format uses an ECC checksum to allow correction of
        single-bit errors in pages.

In an I/O intensive scenario checksum calculation and verification can become
a performance problem. To optimize checksumming as much as possible we take
advantage of different processor features (64-bit words, SSE instructions, SSE2
instructions). This leads to several implementations of the checksum routine,
for different architectures. All checksum calls are indirected through a function
pointer, which is set at runtime, depending on the capabilities of the processor
we are running on.

In order to determine which type of checksum should be used, there is a
bit in the database header/database page which tells us what type of
checksum to use. We always have to consider that a corruption could flip
that bit, so the actual flow of checksumming a page is a little complex to
deal with that.

*******************************************************************/

#include "osustd.hxx"
#include "esestd.hxx"



//  ================================================================
XECHECKSUM LongChecksumFromShortChecksum( const ULONG xorChecksum, const ULONG pgno )
//  ================================================================
{
    Assert( 0 != pgno );
    XECHECKSUM checksum = ( (XECHECKSUM)pgno ) << 32 | xorChecksum;
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
inline void SetBit( void * const pv, const INT ibitOffset )
//  ================================================================
{
    unsigned char * const   pb      = (unsigned char *)pv;
    const INT   ibyte       = ibitOffset / 8;
    const INT   ibitInByte  = ibitOffset % 8;
    const unsigned char     bitMask     = (unsigned char)( 1 << ibitInByte );

    pb[ibyte] |= bitMask;
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
UINT IbitNewChecksumFormatFlag( const PAGETYPE pagetype )
//  ================================================================
{
    if( databasePage == pagetype )
    {
        //  for database pages, the page flags are stored in the 10th
        //  unsigned long. The format bit is 0x2000, which is the 14th bit
        //
        Assert( OffsetOf( CPAGE::PGHDR, fFlags ) * 8 == 9 * 32 );
        Assert( CPAGE::fPageNewChecksumFormat == ( 1 << 13 ) );

        return ( 9 * 32 ) + 13;
    }
    AssertSz( 0, "Only databasePage is supported" );
    return ( UINT )-1;
}


//  ================================================================
BOOL FPageHasNewChecksumFormat( const void * const pv, const PAGETYPE pagetype )
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
void SetNewChecksumFormatFlag( void * const pv, const PAGETYPE pagetype )
//  ================================================================
//
//  The ECC bit is only currently supported for database pages, not
//  headers
//
//-
{
    const INT ibit = IbitNewChecksumFormatFlag( pagetype );
    SetBit( pv, ibit );
}


//  ================================================================
BOOL FPageHasLongChecksum( const PAGETYPE pagetype )
//  ================================================================
{
    return ( databasePage == pagetype );
}

//  ================================================================
ULONG ShortChecksumFromPage( const void * const pv )
//  ================================================================
{
    return *( const ULONG* )pv;
}

//  ================================================================
void SetShortChecksumOnPage( void * const pv, const ULONG checksum )
//  ================================================================
{
    *( ULONG* )pv = checksum;
}



//  ================================================================
PAGECHECKSUM LongChecksumFromPage( const void * const pv, const ULONG cb )
//  ================================================================
{
    BOOL fSmallPage = FIsSmallPage( cb );

    const CPAGE::PGHDR2* const pPH2 = ( const CPAGE::PGHDR2* )pv;
    if ( fSmallPage )
    {
        return PAGECHECKSUM( pPH2->pghdr.checksum );
    }
    else
    {
        return PAGECHECKSUM( pPH2->pghdr.checksum, pPH2->rgChecksum[ 0 ], pPH2->rgChecksum[ 1 ], pPH2->rgChecksum[ 2 ] );
    }
}


//  ================================================================
void SetLongChecksumOnPage( void * const pv, const ULONG cb, const PAGECHECKSUM checksum )
//  ================================================================
{
    BOOL fSmallPage = FIsSmallPage( cb );

    CPAGE::PGHDR2* const pPH2 = ( CPAGE::PGHDR2* )pv;
    pPH2->pghdr.checksum = checksum.rgChecksum[ 0 ];

    if ( !fSmallPage )
    {
        pPH2->rgChecksum[ 0 ] = checksum.rgChecksum[ 1 ];
        pPH2->rgChecksum[ 1 ] = checksum.rgChecksum[ 2 ];
        pPH2->rgChecksum[ 2 ] = checksum.rgChecksum[ 3 ];
    }
}


//  ================================================================
PAGECHECKSUM ChecksumFromPage( const void * const pv, const ULONG cb, const PAGETYPE pagetype )
//  ================================================================
{
    if( FPageHasLongChecksum( pagetype ) )
    {
        return LongChecksumFromPage( pv, cb );
    }
    return ShortChecksumFromPage( pv );
}


//  ================================================================
void SetChecksumOnPage( void * const pv, const ULONG cb, const PAGETYPE pagetype, const PAGECHECKSUM checksum )
//  ================================================================
{
    if( FPageHasLongChecksum( pagetype ) )
    {
        SetLongChecksumOnPage( pv, cb, checksum );
    }
    else
    {
        XECHECKSUM xeChecksum = checksum.rgChecksum[ 0 ];
        Assert( xeChecksum <= 0xFFFFFFFF );
        const ULONG shortChecksum = (ULONG)xeChecksum;
        SetShortChecksumOnPage( pv, shortChecksum );
    }
}

//  ================================================================
ULONG CbBlockSize( const ULONG cb )
//  ================================================================
{
    // small pages only have 1 block
    // divide a large page into a max of 4 blocks, first block is header block which hosts the page header
    // because of backwards compatibility, 16k pages use 4k blocks. 32k pages use 8k blocks, with an ending 4k block if the page is not an 8k multiple
    ULONG cbT = cb;
    if ( !FIsSmallPage( cb ) )
    {
        AssertRTL( cb % 4096 == 0 );
        if ( cb <= 16 * 1024 )
        {
            cbT = 4096;
        }
        else if ( cb <= 32 * 1024 )
        {
            cbT = 8*1024;   // header block size
        }
        else
        {
            AssertSz( fFalse, "Invalid pagesize encountered during checksumming." );
        }
    }
    return cbT;
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

            // divide a large page into a max of 4 blocks, first block is header block which hosts the page header
            // because of backwards compatibility, 16k pages use 4k blocks. 32k pages use 8k blocks, with an ending 4k block if the page is not an 8k multiple
            unsigned cbBlock = cb;
            if ( !FIsSmallPage( cb ) )
            {
                cbBlock = CbBlockSize( cb );
                unsigned ibT = cbBlock; // Remove header block
                INT i = 1;
                
                while( ( ibT + cbBlock ) <= cb )
                {
                    Enforce( i <= 3 );  //  no one should pass in an array big enough to overflow the 3 slots
                    AssumePREFAST( i <= 3 );
                    pgChecksum.rgChecksum[ i ] = ChecksumNewFormat( pch + ibT, cbBlock, pgno, fFalse );
                    ibT += cbBlock;
                    i++;
                }

                AssertRTL( cb - ibT == 4 * 1024 || cb - ibT == 0 ); // only 4k or nothing should be left
                if ( cb - ibT > 0 )
                {
                    pgChecksum.rgChecksum[ i ] = ChecksumNewFormat( pch + ibT, cb - ibT, pgno, fFalse );
                }

                // write checksums into designated location in header block
                // so checksum for header block can protect them as well
                if ( fNew )
                {
                    // cast RO pv to RW pPgHdr2
                    CPAGE::PGHDR2* const pPgHdr2 = ( CPAGE::PGHDR2* )pv;
                    pPgHdr2->rgChecksum[ 0 ] = pgChecksum.rgChecksum[ 1 ];
                    pPgHdr2->rgChecksum[ 1 ] = pgChecksum.rgChecksum[ 2 ];
                    pPgHdr2->rgChecksum[ 2 ] = pgChecksum.rgChecksum[ 3 ];
                }
            }

            // whole small page or header block for large page
            pgChecksum.rgChecksum[ 0 ] = ChecksumNewFormat( pch, cbBlock, pgno, fTrue );
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
    PAGECHECKSUM checksumExpected,
    const PAGECHECKSUM checksumActual )
//  ================================================================
{
    Assert( checksumActual != checksumExpected );

    const BOOL fSmallPage = FIsSmallPage( cb );
    UINT cbBlock = CbBlockSize( cb );
    UINT ibT = 0;
    XECHECKSUMERROR rgErr[ cxeChecksumPerPage ] = { xeChecksumNoError, };

    UINT rgibitCorrupted[ cxeChecksumPerPage ] = { IbitNewChecksumFormatFlag( pagetype ), UINT_MAX, UINT_MAX, UINT_MAX, };
    UINT ibitCorrupted = UINT_MAX;

    // work out correction
    UINT iblk = 0;
    while( cbBlock > 0 )
    {
        if ( iblk >= cxeChecksumPerPage )
        {
            // something is wrong, we can't keep checking this page
            AssertSz( fFalse, "Too many blocks on the page" );
            return;
        }

        rgErr[ iblk ] = FTryFixBlock( cbBlock, &rgibitCorrupted[ iblk ], checksumExpected.rgChecksum[ iblk ], checksumActual.rgChecksum[ iblk ] );
        rgibitCorrupted[ iblk ] = rgibitCorrupted[ iblk ] + CHAR_BIT * ibT; // FTryFixBlock() stores the offset relative to the block; we adjust it so that it is relative to the start of the page instead.
        switch ( rgErr[ iblk ] )
        {
            case xeChecksumNoError:
                break;

            case xeChecksumCorrectableError:
                // save the location of first correctable error
                if ( UINT_MAX == ibitCorrupted )
                {
                    ibitCorrupted = rgibitCorrupted[ iblk ];

                    // does this correctable error actually hit other checksums
                    UINT ibitStart = CHAR_BIT * offsetof( CPAGE::PGHDR2, rgChecksum );
                    UINT ibitLength = CHAR_BIT * sizeof( checksumExpected.rgChecksum[ 0 ] );
                    if ( !fSmallPage &&                 // large page
                        0 == iblk &&                    // header block
                        ibitStart <= ibitCorrupted  &&  // hit other checksums
                        ibitCorrupted < ibitStart + ibitLength * ( cxeChecksumPerPage - 1 ) )
                    {
                        // fix the checksum
                        UINT ibit = ibitLength + ( ibitCorrupted - ibitStart );
                        FlipBit( ( void* )&checksumExpected, ibit );
                    }
                }
                break;

            case xeChecksumFatalError:
                *pfCorrectableError = fFalse;
                return;
        }

        iblk++;
        ibT += cbBlock;
        cbBlock = min( cb - ibT, cbBlock ); // for non-power of 2 pages bigger than 16k, we have multiple 8k blocks, with possibly the last block of 4k
    }

    // when multiple correctables, just report first one
    *pfCorrectableError = fTrue;
    *pibitCorrupted = ibitCorrupted;
    Assert( (UINT)*pibitCorrupted < ( 8 * cb ) );

    // carry out correction in page
    UINT cblk = iblk;
    for ( iblk = 0; fCorrectError && iblk < cblk; ++iblk )
    {
        unsigned char* pch = ( unsigned char* )pv;
        if ( xeChecksumCorrectableError == rgErr[ iblk ] )
        {
            FlipBit( pch, rgibitCorrupted[ iblk ]);
        }
    }

    return;
}


//  ================================================================
void ChecksumPage(
    const void * const pv,              //  pointer to the page
    const UINT cb,              //  size of the page (normally g_cbPage)
    const PAGETYPE pagetype,            //  type of the page
    const ULONG pgno,
    PAGECHECKSUM * const pchecksumExpected, //  set to the checksum the page should have / read out the header
    PAGECHECKSUM * const pchecksumActual )  //  set the the actual checksum. if actual != expected, JET_errReadVerifyFailure is returned
//  ================================================================
{
    *pchecksumExpected  = ChecksumFromPage( pv, cb, pagetype );
    *pchecksumActual    = ComputePageChecksum( pv, cb, pagetype, pgno );
}


//  ================================================================
void ChecksumAndPossiblyFixPage(
    void * const pv,                    //  pointer to the page
    const UINT cb,              //  size of the page (normally g_cbPage)
    const PAGETYPE pagetype,            //  type of the page
    const ULONG pgno,
    const BOOL fCorrectError,           //  fTrue if ECC should be used to correct errors
    PAGECHECKSUM * const pchecksumExpected, //  set to the checksum the page should have / read out the header
    PAGECHECKSUM * const pchecksumActual,   //  set the the actual checksum. if actual != expected, JET_errReadVerifyFailure is returned
    BOOL * const pfCorrectableError,    //  set to fTrue if ECC could correct the error (set even if fCorrectError is fFalse)
    INT * const pibitCorrupted )        //  offset of the corrupted bit (meaningful only if *pfCorrectableError is fTrue)
//  ================================================================
{
    *pfCorrectableError = fFalse;
    *pibitCorrupted     = -1;

    *pchecksumExpected  = ChecksumFromPage( pv, cb, pagetype );
    *pchecksumActual    = ComputePageChecksum( pv, cb, pagetype, pgno );
    
    const BOOL fNewChecksumFormat = FPageHasNewChecksumFormat( pv, pagetype );
    if( *pchecksumActual != *pchecksumExpected && fNewChecksumFormat )
    {
        TryFixPage( pv, cb, pagetype, fCorrectError, pfCorrectableError, pibitCorrupted, *pchecksumExpected, *pchecksumActual );
        Assert( ( *pfCorrectableError && *pibitCorrupted != -1 ) || ( !*pfCorrectableError && *pibitCorrupted == -1 ) );

        // no point in re-computing the checksum if we haven't done any changes
        if ( fCorrectError && *pfCorrectableError )
        {
            *pchecksumExpected = ChecksumFromPage( pv, cb, pagetype );
            *pchecksumActual = ComputePageChecksum( pv, cb, pagetype, pgno, fTrue );
        }
    }
    else if (ErrFaultInjection( 35349 ) != JET_errSuccess)
    {
        *pfCorrectableError = fTrue;
        *pibitCorrupted = 1066;
    }
}

//  ================================================================
void DumpLargePageChecksumInfo(
    void * const pv,                    //  pointer to the page
    const UINT cb,              //  size of the page (normally g_cbPage)
    const PAGETYPE pagetype,            //  type of the page
    const ULONG pgno,
          CPRINTF * const pcprintf )
//  ================================================================
{
    Assert( FPageHasNewChecksumFormat( pv, pagetype ) );

    TRY
    {
        PAGECHECKSUM checksumStoredInHeader     = ChecksumFromPage( pv, cb, pagetype );
        PAGECHECKSUM checksumComputedOffData    = ComputePageChecksum( pv, cb, pagetype, pgno );
        (*pcprintf)( _T(     "HEADER checksum     = 0x%016I64X:0x%016I64X:0x%016I64X:0x%016I64X\n" ),
            checksumStoredInHeader.rgChecksum[ 0 ], checksumStoredInHeader.rgChecksum[ 1 ], checksumStoredInHeader.rgChecksum[ 2 ], checksumStoredInHeader.rgChecksum[ 3 ] );

        if( checksumStoredInHeader != checksumComputedOffData )
        {
            (*pcprintf)( _T( "****** checksum mismatch ******\n" ) );
            (*pcprintf)( _T( "COMPUTED checksum   = 0x%016I64X:0x%016I64X:0x%016I64X:0x%016I64X\n" ),
                checksumComputedOffData.rgChecksum[ 0 ], checksumComputedOffData.rgChecksum[ 1 ], checksumComputedOffData.rgChecksum[ 2 ], checksumComputedOffData.rgChecksum[ 3 ] );

            BOOL fCorrectableError = fFalse;
            INT ibitCorrupted = -1;
            ChecksumAndPossiblyFixPage( pv, cb, pagetype, pgno, fTrue, &checksumStoredInHeader, &checksumComputedOffData, &fCorrectableError, &ibitCorrupted );

            if ( !fCorrectableError )
            {
                (*pcprintf)( _T( "error is NOT correctable by the checksum\n" ) );
            }
            else
            {
                Assert( 0 <= ibitCorrupted && ( unsigned )ibitCorrupted <= CHAR_BIT * cb );
                (*pcprintf)( _T( "a bit at offset %d (0x%X) was corrupted and can be corrected by the checksum\n" ), ibitCorrupted, ibitCorrupted );

                const PAGECHECKSUM checksumFixed = ComputePageChecksum( pv, cb, pagetype, pgno );
                (*pcprintf)( _T( "FIXED checksum    = 0x%016I64X:0x%016I64X:0x%016I64X:0x%016I64X\n" ),
                    checksumFixed.rgChecksum[ 0 ], checksumFixed.rgChecksum[ 1 ], checksumFixed.rgChecksum[ 2 ], checksumFixed.rgChecksum[ 3 ] );
                Assert( checksumFixed == checksumStoredInHeader );
            }
        }
    }
    EXCEPT( efaExecuteHandler )
    {
        (*pcprintf)( _T( "\t<unable to validate page checksum info>\n" ) );
    }
    ENDEXCEPT
}

//  ================================================================
void DumpPageChecksumInfo(
    void * const pv,                    //  pointer to the page
    const UINT cb,              //  size of the page (normally g_cbPage)
    const PAGETYPE pagetype,            //  type of the page
    const ULONG pgno,
          CPRINTF * const pcprintf )
//  ================================================================
{
    if ( !FIsSmallPage( cb ) && FPageHasNewChecksumFormat( pv, pagetype ) )
    {
        DumpLargePageChecksumInfo( pv, cb, pagetype, pgno, pcprintf );
        return;
    }

    TRY
    {
        const PAGECHECKSUM  checksumStoredInHeader  = ChecksumFromPage( pv, cb, pagetype );
        const PAGECHECKSUM  checksumComputedOffData = ComputePageChecksum( pv, cb, pagetype, pgno );
        const BOOL          fNewChecksumFormat      = FPageHasNewChecksumFormat( pv, pagetype );
        const BOOL          fBadChecksum            = ( checksumStoredInHeader != checksumComputedOffData );
        
        (*pcprintf)( _T(     "\theader checksum       = 0x%016I64x\n" ), checksumStoredInHeader.rgChecksum[ 0 ] );
        if( fBadChecksum )
        {
            (*pcprintf)( _T( "\t****** checksum mismatch ******\n" ) );
            (*pcprintf)( _T( "\tcomputed checksum     = 0x%016I64x\n" ), checksumComputedOffData );
        }
        if( !fNewChecksumFormat )
        {
            (*pcprintf)( _T( "\t\told checksum format\n" ) );
            const ULONG * pdw = (const ULONG * ) pv;
            const ULONG pgnoFromPage = pdw[1];
            (*pcprintf)( _T( "\t\t\tpgno = %d\n" ), pgnoFromPage );
        }
        else
        {
            (*pcprintf)( _T( "\t\tnew checksum format\n" ) );
            const ULONG eccChecksumComputed = DwECCChecksumFromXEChecksum( checksumComputedOffData.rgChecksum[ 0 ] );
            const ULONG xorChecksumComputed = DwXORChecksumFromXEChecksum( checksumComputedOffData.rgChecksum[ 0 ] );
            const ULONG eccChecksumHeader = DwECCChecksumFromXEChecksum( checksumStoredInHeader.rgChecksum[ 0 ] );
            const ULONG xorChecksumHeader = DwXORChecksumFromXEChecksum( checksumStoredInHeader.rgChecksum[ 0 ] );
            (*pcprintf)( _T( "\t\t\theader ECC checksum = 0x%08x\n" ), eccChecksumHeader );
            if( fBadChecksum )
            {
                (*pcprintf)( _T( "\t\t\tcomputed ECC checksum = 0x%08x\n" ), eccChecksumComputed );
            }
            (*pcprintf)( _T( "\t\t\theader XOR checksum   = 0x%08x\n" ), xorChecksumHeader );
            if( fBadChecksum )
            {
                (*pcprintf)( _T( "\t\t\tcomputed XOR checksum = 0x%08x\n" ), xorChecksumComputed );
            }

            if( fBadChecksum )
            {
                if( !FECCErrorIsCorrectable( cb, checksumStoredInHeader.rgChecksum[ 0 ], checksumComputedOffData.rgChecksum[ 0 ] ) )
                {
                    (*pcprintf)( _T( "\tchecksum error is NOT correctable\n" ) );
                    if( eccChecksumComputed == eccChecksumHeader )
                    {
                        const ULONG pgnoPossible = xorChecksumComputed ^ xorChecksumHeader ^ pgno;
                        (*pcprintf)( _T( "\tECC checksums match. perhaps this is actually page %d?\n" ), pgnoPossible );
                    }
                }
                else
                {
                    (*pcprintf)( _T( "\tchecksum error is correctable\n" ) );
                    const UINT ibitCorrupted = IbitCorrupted( cb, checksumStoredInHeader.rgChecksum[ 0 ], checksumComputedOffData.rgChecksum[ 0 ] );
                    (*pcprintf)( _T( "\t\tbit %d is corrupted\n" ), ibitCorrupted );
                    if( IbitNewChecksumFormatFlag( pagetype ) == ibitCorrupted )
                    {
                        (*pcprintf)( _T( "\t\tbit %d is the checksum format flag! this corruption is not fixable\n" ), ibitCorrupted );
                    }
                    else
                    {
                        FlipBit( pv, ibitCorrupted );
                        const PAGECHECKSUM  checksumFixed       = ComputePageChecksum( pv, cb, pagetype, pgno );
                        const ULONG eccChecksumFixed = DwECCChecksumFromXEChecksum( checksumFixed.rgChecksum[ 0 ] );
                        const ULONG xorChecksumFixed = DwXORChecksumFromXEChecksum( checksumFixed.rgChecksum[ 0 ] );
                        (*pcprintf)( _T( "\t\tfixed checksum is 0x%016I64x\n" ), checksumFixed );
                        (*pcprintf)( _T( "\t\tfixed ECC checksum is 0x%08x\n" ), eccChecksumFixed );
                        (*pcprintf)( _T( "\t\tfixed XOR checksum is 0x%08x\n" ), xorChecksumFixed );
                        if( checksumFixed == checksumStoredInHeader )
                        {
                            (*pcprintf)( _T( "\t****** page corruption was fixed ******\n" ) );
                        }
                        else
                        {
                            (*pcprintf)( _T( "\t****** page corruption fix FAILED! ******\n" ) );
                        }
                    }
                }
            }
        }
    }
    EXCEPT( efaExecuteHandler )
    {
        (*pcprintf)( _T( "\t<unable to validate page checksum info>\n" ) );
    }
    ENDEXCEPT
}

//  ================================================================
void SetPageChecksum( void * const pv, const UINT cb, const PAGETYPE pagetype, const ULONG pgno )
//  ================================================================
{
    if( FPageHasLongChecksum( pagetype ) )
    {
        SetNewChecksumFormatFlag( pv, pagetype );
    }
    
    PAGECHECKSUM checksum = ComputePageChecksum( pv, cb, pagetype, pgno, fTrue );
    SetChecksumOnPage( pv, cb, pagetype, checksum );
}


