// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"


VOID RandomizeBuffer( void * const pv, const size_t cb )
{
    BYTE * const pb = (BYTE *)pv;
    for( size_t ib = 0; ib < cb; ++ib )
    {
        pb[ib] = (BYTE)(rand() & 0xFF);
    }
}

VOID ZeroBuffer_( BYTE * pb, ULONG cb )
{
    memset( pb, 0, cb );
}


