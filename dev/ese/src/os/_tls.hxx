// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS__TLS_HXX_INCLUDED
#define _OS__TLS_HXX_INCLUDED


//  Internal TLS structure

struct _TLS
{

    //  Elements used for thread and TLS itself support.
    //
    DWORD               dwThreadId;     //  thread ID
    HANDLE              hThread;        //  thread handle

    _TLS*               ptlsNext;       //  next TLS in global list
    _TLS**              pptlsNext;      //  pointer to the pointer to this TLS

    //  OS Library TLS support.
    //
    OSTLS               ostls;          //  OS Library TLS structure

    //  User TLS space.
    //
    BYTE                rgUserTLS[];    //  external TLS structure

};

OSTLS* const PostlsFromIntTLS( _TLS * const ptls );
TLS* const PtlsFromIntTLS( _TLS * const ptls );

#endif  //  _OS__TLS_HXX_INCLUDED

