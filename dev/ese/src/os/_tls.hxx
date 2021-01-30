// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS__TLS_HXX_INCLUDED
#define _OS__TLS_HXX_INCLUDED



struct _TLS
{

    DWORD               dwThreadId;
    HANDLE              hThread;

    _TLS*               ptlsNext;
    _TLS**              pptlsNext;

    OSTLS               ostls;

    BYTE                rgUserTLS[];

};

OSTLS* const PostlsFromIntTLS( _TLS * const ptls );
TLS* const PtlsFromIntTLS( _TLS * const ptls );

#endif

