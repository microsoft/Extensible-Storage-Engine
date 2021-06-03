// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _CHECKSUMU_HXX_INCLUDED
#define _CHECKSUMU_HXX_INCLUDED

//
//  Database pages contain a checksum which can be used to check for,
//  and possibly correct, data corruption. There are multiple types
//  of database pages and we need to know which type as a bit of the
//  internal page format has to be deciphered
//

// for small pages (<=8kiB), an single XECHECKSUM is enough to protect whole page
// for large pages (16, 32kiB), we divide it into 4 sub-blocks and use 4 XECHECKSUM to protect it.
#define cxeChecksumPerPage 4

// page checksum structure
// a page checksum either has one XECHECKSUM to cover small pages (2/4/8kiB)
// or has FOUR XECHECKSUM to cover large pages (16/32kiB)
struct PAGECHECKSUM
{
    XECHECKSUM rgChecksum[ cxeChecksumPerPage ];

public:
    PAGECHECKSUM::PAGECHECKSUM( void ) { Init( 0, 0, 0, 0 ); }
    PAGECHECKSUM::PAGECHECKSUM( XECHECKSUM xe ) { Init( xe, 0, 0, 0 ); }
    PAGECHECKSUM::PAGECHECKSUM( XECHECKSUM xe0, XECHECKSUM xe1, XECHECKSUM xe2, XECHECKSUM xe3 ) { Init( xe0, xe1, xe2, xe3 ); }

private:
    void Init( XECHECKSUM xe0, XECHECKSUM xe1, XECHECKSUM xe2, XECHECKSUM xe3 )
    {
        rgChecksum[ 0 ] = xe0; rgChecksum[ 1 ] = xe1; rgChecksum[ 2 ] = xe2; rgChecksum[ 3 ] = xe3;
    }
};

inline BOOL operator ==( const PAGECHECKSUM& c1, const PAGECHECKSUM& c2 ) { return !memcmp( &c1, &c2, sizeof( c1 ) ); }
inline BOOL operator !=( const PAGECHECKSUM& c1, const PAGECHECKSUM& c2 ) { return !( c1 == c2 ); }


//  this describes the type of page we are checksumming. the enum is
//  given random values to make sure people don't pass the wrong argument

enum PAGETYPE
{
    databasePage = 19,
    databaseHeader = 23,
    logfileHeader = 31,
    logfilePage = 19, /* compatible with database page */
    flushmapHeaderPage = 37,
    flushmapDataPage = 19, /* compatible with database page */
    rbsPage = 19, /* compatible with database page */
};

//  ChecksumPage page validates a page

void ChecksumPage(
    const void * const pv,              //  pointer to the page
    const UINT cb,              //  size of the page (normally g_cbPage)
    const PAGETYPE pagetype,            //  type of the page
    const ULONG pgno,
    PAGECHECKSUM * const pchecksumExpected, //  set to the checksum the page should have
    PAGECHECKSUM * const pchecksumActual ); //  set the the actual checksum. if actual != expected, JET_errReadVerifyFailure is returned

//  ChecksumAndPossiblyFixPage page validates a page and, if possible, corrects errors in the page

void ChecksumAndPossiblyFixPage(
    void * const pv,                    //  pointer to the page
    const UINT cb,              //  size of the page (g_cbPage)
    const PAGETYPE pagetype,            //  type of the page
    const ULONG pgno,
    const BOOL fCorrectError,           //  fTrue if ECC should be used to correct errors
    PAGECHECKSUM * const pchecksumExpected, //  set to the checksum the page should have
    PAGECHECKSUM * const pchecksumActual,   //  set the the actual checksum. if actual != expected, JET_errReadVerifyFailure is returned
    BOOL * const pfCorrectableError,    //  set to fTrue if ECC could correct the error (set even if fCorrectError is fFalse)
    INT * const pibitCorrupted );       //  offset of the corrupted bit (meaningful only if *pfCorrectableError is fTrue)

//  Sets the checksum on a page

void SetPageChecksum( void * const pv, const UINT cb, const PAGETYPE pagetype, const ULONG pgno );


void DumpPageChecksumInfo(
    void * const pv,                    //  pointer to the page
    const UINT cb,              //  size of the page (normally g_cbPage)
    const PAGETYPE pagetype,            //  type of the page
    const ULONG pgno,
          CPRINTF * const pcprintf );
    
#ifndef RTM
ERR ErrChecksumUnitTest();
#endif

#endif
