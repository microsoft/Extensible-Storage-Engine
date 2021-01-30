// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _CHECKSUM_HXX_INCLUDED
#define _CHECKSUM_HXX_INCLUDED


typedef unsigned __int64 XECHECKSUM;

ULONG ChecksumOldFormat( const unsigned char * const pb, const ULONG cb );
XECHECKSUM ChecksumNewFormat( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue );

ULONG DwECCChecksumFromXEChecksum( const XECHECKSUM checksum );
ULONG DwXORChecksumFromXEChecksum( const XECHECKSUM checksum );

BOOL FECCErrorIsCorrectable( const UINT cb, const XECHECKSUM xeChecksumExpected, const XECHECKSUM xeChecksumActual );
BOOL FECCErrorIsCorrectable( const UINT cb, const ULONG dwECCChecksumExpected, const ULONG dwECCChecksumActual );

UINT IbitCorrupted( const UINT cb, const XECHECKSUM xeChecksumExpected, const XECHECKSUM xeChecksumActual );
UINT IbitCorrupted( const UINT cb, const ULONG dwECCChecksumExpected, const ULONG dwECCChecksumActual );

#endif
