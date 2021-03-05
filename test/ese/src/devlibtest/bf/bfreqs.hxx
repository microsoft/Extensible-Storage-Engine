// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#ifndef _BFREQS_HXX_INCLUDED
#define _BFREQS_HXX_INCLUDED



typedef ULONG PGNO;

typedef LONG CPG;

typedef UINT            IFMP;

static const IFMP ifmpMax = 200;

enum BFDirtyFlags
{
    bfdfMin     = 0,
    bfdfClean   = 0,
    bfdfUntidy  = 1,
    bfdfDirty   = 2,
    bfdfFilthy  = 3,
    bfdfMax     = 4,
};


struct IFMPPGNO
{
    IFMPPGNO() {}
    IFMPPGNO( IFMP ifmpIn, PGNO pgnoIn )
        :   ifmp( ifmpIn ),
            pgno( pgnoIn )
    {
    }

    BOOL operator==( const IFMPPGNO& ifmppgno ) const
    {
        return ifmp == ifmppgno.ifmp && pgno == ifmppgno.pgno;
    }

    const IFMPPGNO& operator=( const IFMPPGNO& ifmppgno )
    {
        ifmp = ifmppgno.ifmp;
        pgno = ifmppgno.pgno;

        return *this;
    }

    ULONG_PTR Hash() const
    {
        return (ULONG_PTR)( pgno + ( ifmp << 13 ) + ( pgno >> 17 ) );
    }

    IFMP    ifmp;
    PGNO    pgno;
};


#include "tcconst.hxx"

#endif

