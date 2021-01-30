// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _XSUM_HXX_INCLUDED
#define _XSUM_HXX_INCLUDED

#pragma warning ( disable : 4100 )
#pragma warning ( disable : 4127 )
#pragma warning ( disable : 4706 )

#include "cc.hxx"



typedef unsigned __int64 XECHECKSUM;

#define cxeChecksumPerPage 4

struct PGHDR2
{
    XECHECKSUM      checksum;
    unsigned char       reserved0[ 32 ];
    XECHECKSUM      rgChecksum[ cxeChecksumPerPage - 1 ];
    UINT        pgno;
};


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



enum PAGETYPE       { databasePage = 19, databaseHeader = 23, logfileHeader = 31 };


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

#endif
