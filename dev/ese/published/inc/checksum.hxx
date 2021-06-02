// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _CHECKSUM_HXX_INCLUDED
#define _CHECKSUM_HXX_INCLUDED

//
//  Database pages contain a checksum which can be used to check for,
//  and possibly correct, data corruption. There are multiple types
//  of database pages and we need to know which type as a bit of the
//  internal page format has to be deciphered
//

// typedef for a xor-ecc checksum
// an xor-ecc checksum will cover 2/4/8kiB.
typedef unsigned __int64 XECHECKSUM;

ULONG ChecksumOldFormat( const unsigned char * const pb, const ULONG cb );
XECHECKSUM ChecksumNewFormat( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue );

ULONG DwECCChecksumFromXEChecksum( const XECHECKSUM checksum );
ULONG DwXORChecksumFromXEChecksum( const XECHECKSUM checksum );

BOOL FECCErrorIsCorrectable( const UINT cb, const XECHECKSUM xeChecksumExpected, const XECHECKSUM xeChecksumActual );
BOOL FECCErrorIsCorrectable( const UINT cb, const ULONG dwECCChecksumExpected, const ULONG dwECCChecksumActual );

UINT IbitCorrupted( const UINT cb, const XECHECKSUM xeChecksumExpected, const XECHECKSUM xeChecksumActual );
UINT IbitCorrupted( const UINT cb, const ULONG dwECCChecksumExpected, const ULONG dwECCChecksumActual );

#endif  //  _CHECKSUM_HXX_INCLUDED
