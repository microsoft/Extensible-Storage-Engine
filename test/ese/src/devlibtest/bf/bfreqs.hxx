// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  This header thunks out all the requisite structures for including various
//  bf (and resmgr) headers and linking to said.

#ifndef _BFREQS_HXX_INCLUDED
#define _BFREQS_HXX_INCLUDED


//
//  BF Elements
//

typedef ULONG PGNO;

typedef LONG CPG;

typedef UINT            IFMP;

static const IFMP ifmpMax = 200;

enum BFDirtyFlags  //  bfdf
{
    bfdfMin     = 0,
    bfdfClean   = 0,        //  the page will not be written
    bfdfUntidy  = 1,        //  the page will be written only when idle
    bfdfDirty   = 2,        //  the page will be written only when necessary
    bfdfFilthy  = 3,        //  the page will be written as soon as possible
    bfdfMax     = 4,
};

//
//  ResMgr Elements
//

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

//
//  Trace Constants
//

#include "tcconst.hxx"

#endif  //  _BFREQS_HXX_INCLUDED

