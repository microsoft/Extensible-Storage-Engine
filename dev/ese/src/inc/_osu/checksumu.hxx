// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _CHECKSUMU_HXX_INCLUDED
#define _CHECKSUMU_HXX_INCLUDED


#define cxeChecksumPerPage 4

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



enum PAGETYPE
{
    databasePage = 19,
    databaseHeader = 23,
    logfileHeader = 31,
    logfilePage = 19, 
    flushmapHeaderPage = 37,
    flushmapDataPage = 19, 
    rbsPage = 19, 
};


void ChecksumPage(
    const void * const pv,
    const UINT cb,
    const PAGETYPE pagetype,
    const ULONG pgno,
    PAGECHECKSUM * const pchecksumExpected,
    PAGECHECKSUM * const pchecksumActual );


void ChecksumAndPossiblyFixPage(
    void * const pv,
    const UINT cb,
    const PAGETYPE pagetype,
    const ULONG pgno,
    const BOOL fCorrectError,
    PAGECHECKSUM * const pchecksumExpected,
    PAGECHECKSUM * const pchecksumActual,
    BOOL * const pfCorrectableError,
    INT * const pibitCorrupted );


void SetPageChecksum( void * const pv, const UINT cb, const PAGETYPE pagetype, const ULONG pgno );


void DumpPageChecksumInfo(
    void * const pv,
    const UINT cb,
    const PAGETYPE pagetype,
    const ULONG pgno,
          CPRINTF * const pcprintf );
    
#ifndef RTM
ERR ErrChecksumUnitTest();
#endif

#endif
