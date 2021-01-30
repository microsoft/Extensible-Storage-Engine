// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

unsigned __int64 Ui64FNVHash(   _In_reads_bytes_opt_( cb )  const void* const   pv,
                                _In_                        const INT           cb )
{
    static const __int64            FNV_PRIME           = 1099511628211;
    static const unsigned __int64   FNV_OFFSET_BASIS    = 14695981039346656037;

    const BYTE* const   pb      = (const BYTE*)pv;
    unsigned __int64    hash    = FNV_OFFSET_BASIS;

    if ( pb )
    {
        for ( INT ib = 0; ib < cb; ib++ )
        {
            hash = hash ^ pb[ ib ];
            hash = hash * FNV_PRIME;
        }
    }

    return hash;
}

double ChiSquaredSignificanceTest(
    _In_reads_bytes_( cbSample ) const BYTE* const pbSample,
    const INT cbSample,
    _Out_writes_bytes_( sizeof( WORD ) * CHISQUARED_FREQTABLE_SIZE ) WORD* rgwFreqTable )
{
    for ( const BYTE* pb = pbSample; pb < pbSample + cbSample; pb++ )
    {
        rgwFreqTable[ *pb ]++;
    }

    double dChiSquared = 0.0;
    double E = cbSample / 256.0;
    for ( INT i = 0; i < CHISQUARED_FREQTABLE_SIZE; i++ )
    {
        double numerator = ( rgwFreqTable[ i ] - E );
        dChiSquared += ( numerator * numerator ) / E;
    }

    return dChiSquared;
}

