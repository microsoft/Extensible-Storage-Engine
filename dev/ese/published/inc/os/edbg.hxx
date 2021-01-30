// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

extern "C" {


ERR ErrEDBGCacheQueryLocal(
        __in const INT argc,
        __in_ecount(argc) const CHAR * const argv[],
        __inout void * pvResult );

}


#ifdef DEBUGGER_EXTENSION
struct EDBGGlobals
{
    const CHAR  *szName;
    const ULONG_PTR pvData;
    const CHAR  *szDebuggerCommand;

    EDBGGlobals(
        _In_ const CHAR* szNameIn,
        _In_ const ULONG_PTR pvDataIn,
        _In_ const CHAR *szDebuggerCommandIn
        )
        : szName( szNameIn ),
        pvData( pvDataIn ),
        szDebuggerCommand( szDebuggerCommandIn )
    {
    }
};

extern const EDBGGlobals * rgEDBGGlobalsArray;

HRESULT
EDBGPrintf(
    __in PCSTR szFormat,
    ...
)
;
#endif



#pragma warning(disable:4296)

extern const CHAR * SzEDBGHexDump( const VOID * pv, INT cbMax );

#define SYMBOL_LEN_MAX      24
#define VOID_CB_DUMP        8


INLINE QWORD QwMaskForSmallerTypes( const size_t cbType )
{
    return ( cbType < sizeof(QWORD) ?
                    ( ( QWORD( 1 ) << ( cbType * 8 ) ) - 1 ) :
                    QWORD( ~0 ) );
}


#define FORMAT_( CLASS, pointer, member, offset )   \
    "\t%*.*s <0x%0*I64X,%3I64u>:  ", \
    SYMBOL_LEN_MAX, \
    SYMBOL_LEN_MAX, \
    #member, \
    INT( 2 * sizeof( pointer ) ), \
    __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) ), \
    __int64( sizeof( (pointer)->member ) )

#define FORMAT_VOID( CLASS, pointer, member, offset )   \
    "\t%*.*s <0x%0*I64X,%3I64u>:  %s", \
    SYMBOL_LEN_MAX, \
    SYMBOL_LEN_MAX, \
    #member, \
    INT( 2 * sizeof( pointer ) ), \
    __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) ), \
    __int64( sizeof( (pointer)->member ) ), \
    SzEDBGHexDump( (VOID *)(&((pointer)->member)), ( VOID_CB_DUMP > sizeof( (pointer)->member ) ) ? sizeof( (pointer)->member ) : VOID_CB_DUMP )

#define FORMAT_POINTER( CLASS, pointer, member, offset )    \
    "\t%*.*s <0x%0*I64X,%3I64u>:  0x%0*I64X\n", \
    SYMBOL_LEN_MAX, \
    SYMBOL_LEN_MAX, \
    #member, \
    INT( 2 * sizeof( pointer ) ), \
    __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) ), \
    __int64( sizeof( (pointer)->member ) ), \
    INT( 2 * sizeof( (pointer)->member ) ), \
    __int64( (pointer)->member )

#define FORMAT_POINTER_NOLINE( CLASS, pointer, member, offset ) \
    "\t%*.*s <0x%0*I64X,%3I64u>:  0x%0*I64X", \
    SYMBOL_LEN_MAX, \
    SYMBOL_LEN_MAX, \
    #member, \
    INT( 2 * sizeof( pointer ) ), \
    __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) ), \
    __int64( sizeof( (pointer)->member ) ), \
    INT( 2 * sizeof( (pointer)->member ) ), \
    __int64( (pointer)->member )

#define FORMAT_POINTER_DUMPLINK_DML( CLASS, pointer, PTRCLASS, member, offset ) \
    "\t%*c<link cmd=\"!ese dump %hs 0x%I64X\">%0.*s</link> &lt;0x%0*I64X,%3I64u&gt;:  <link cmd=\"dt %ws!%hs 0x%0*I64X\">0x%0*I64X</link>\n", \
    max( 0, LONG( SYMBOL_LEN_MAX - strlen( #member ) ) ), \
    ' ', \
    #PTRCLASS,  \
    __int64( (pointer)->member ), \
    min( SYMBOL_LEN_MAX - 1, strlen( #member ) ), \
    #member, \
    INT( 2 * sizeof( pointer ) ), \
    __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) ), \
    __int64( sizeof( (pointer)->member ) ), \
    WszUtilImageName(), \
    #PTRCLASS,  \
    INT( 2 * sizeof( (pointer)->member ) ), \
    __int64( (pointer)->member ), \
    INT( 2 * sizeof( (pointer)->member ) ), \
    __int64( (pointer)->member )

#define EDBGDumplinkDml( CLASS, pointer, PTRCLASS, member, offset )     \
    if ( g_DebugControl == NULL || (pointer)->member == NULL )          \
    {                                                                   \
        (*pcprintf)( FORMAT_POINTER( CLASS, pointer, member, offset ) );    \
    }                                                                   \
    else                                                                \
    {                                                                   \
        EDBGPrintfDml( FORMAT_POINTER_DUMPLINK_DML( CLASS, pointer, PTRCLASS, member, offset ) ); \
    }

#define FORMAT_POINTER_DUMPTYPE_DML( CLASS, pointer, PTRCLASS, member, offset ) \
    "\t%*.*s &lt;0x%0*I64X,%3I64u&gt;:  <link cmd=\"dt %ws!%hs 0x%0*I64X\">0x%0*I64X</link>\n", \
    SYMBOL_LEN_MAX, \
    SYMBOL_LEN_MAX, \
    #member, \
    INT( 2 * sizeof( pointer ) ), \
    __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) ), \
    __int64( sizeof( (pointer)->member ) ), \
    WszUtilImageName(), \
    #PTRCLASS,  \
    INT( 2 * sizeof( (pointer)->member ) ), \
    __int64( (pointer)->member ), \
    INT( 2 * sizeof( (pointer)->member ) ), \
    __int64( (pointer)->member )

#define EDBGDumptypeDml( CLASS, pointer, PTRCLASS, member, offset )     \
    if ( g_DebugControl == NULL || (pointer)->member == NULL )          \
    {                                                                   \
        (*pcprintf)( FORMAT_POINTER( CLASS, pointer, member, offset ) );    \
    }                                                                   \
    else                                                                \
    {                                                                   \
        EDBGPrintfDml( FORMAT_POINTER_DUMPTYPE_DML( CLASS, pointer, PTRCLASS, member, offset ) ); \
    }

#define EDBGDprintfDumplinkDml( CLASS, pointer, PTRCLASS, member, offset )      \
    if ( g_DebugControl == NULL || (pointer)->member == NULL )          \
    {                                                                   \
        dprintf( FORMAT_POINTER( CLASS, pointer, member, offset ) );    \
    }                                                                   \
    else                                                                \
    {                                                                   \
        EDBGPrintfDml( FORMAT_POINTER_DUMPLINK_DML( CLASS, pointer, PTRCLASS, member, offset ) ); \
    }


#define FORMAT_0ARRAY( CLASS, pointer, member, offset ) \
    "\t%*.*s <0x%0*I64X,  0>\n", \
    SYMBOL_LEN_MAX, \
    SYMBOL_LEN_MAX, \
    #member, \
    INT( 2 * sizeof( pointer ) ), \
    __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) )

#define FORMAT_INT( CLASS, pointer, member, offset )    \
        "\t%*.*s <0x%0*I64X,%3I64u>:  %I64i (0x%I64X)\n", \
        SYMBOL_LEN_MAX, \
        SYMBOL_LEN_MAX, \
        #member, \
        INT( 2 * sizeof( pointer ) ), \
        __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) ), \
        __int64( sizeof( (pointer)->member ) ), \
        __int64( (pointer)->member ), \
        __int64( (pointer)->member ) & QwMaskForSmallerTypes( sizeof( (pointer)->member ) )
    
#define FORMAT_INT_NOLINE( CLASS, pointer, member, offset ) \
        "\t%*.*s <0x%0*I64X,%3I64u>:  %I64i (0x%I64X)", \
        SYMBOL_LEN_MAX, \
        SYMBOL_LEN_MAX, \
        #member, \
        INT( 2 * sizeof( pointer ) ), \
        __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) ), \
        __int64( sizeof( (pointer)->member ) ), \
        __int64( (pointer)->member ), \
        __int64( (pointer)->member ) & QwMaskForSmallerTypes( sizeof( (pointer)->member ) )
    
#define FORMAT_UINT( CLASS, pointer, member, offset )   \
    "\t%*.*s <0x%0*I64X,%3I64u>:  %I64u (0x%I64X)\n", \
    SYMBOL_LEN_MAX, \
    SYMBOL_LEN_MAX, \
    #member, \
    INT( 2 * sizeof( pointer ) ), \
    __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) ), \
    __int64( sizeof( (pointer)->member ) ), \
    unsigned __int64( (pointer)->member ), \
    unsigned __int64( (pointer)->member )

#define FORMAT_GUID( CLASS, pointer, member, offset )   \
    "\t%*.*s <0x%0*I64X,%3I64u>:  {%08lx-%04hx-%04hx-%02hx%02hx-%02hx%02hx%02hx%02hx%02hx%02hx}\n", \
    SYMBOL_LEN_MAX, \
    SYMBOL_LEN_MAX, \
    #member, \
    INT( 2 * sizeof( pointer ) ), \
    __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) ), \
    __int64( sizeof( (pointer)->member ) ), \
    ( (pointer)->member.Data1 ), \
    ( (pointer)->member.Data2 ), \
    ( (pointer)->member.Data3 ), \
    ( (USHORT) (pointer)->member.Data4[0] ), \
    ( (USHORT) (pointer)->member.Data4[1] ), \
    ( (USHORT) (pointer)->member.Data4[2] ), \
    ( (USHORT) (pointer)->member.Data4[3] ), \
    ( (USHORT) (pointer)->member.Data4[4] ), \
    ( (USHORT) (pointer)->member.Data4[5] ), \
    ( (USHORT) (pointer)->member.Data4[6] ), \
    ( (USHORT) (pointer)->member.Data4[7] )

#define FORMAT_UINT_NOLINE( CLASS, pointer, member, offset )    \
    "\t%*.*s <0x%0*I64X,%3I64u>:  %I64u (0x%I64X)", \
    SYMBOL_LEN_MAX, \
    SYMBOL_LEN_MAX, \
    #member, \
    INT( 2 * sizeof( pointer ) ), \
    __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) ), \
    __int64( sizeof( (pointer)->member ) ), \
    unsigned __int64( (pointer)->member ), \
    unsigned __int64( (pointer)->member )

#define FORMAT_BOOL( CLASS, pointer, member, offset )   \
    "\t%*.*s <0x%0*I64X,%3I64u>:  %s  (0x%I64X)\n", \
    SYMBOL_LEN_MAX, \
    SYMBOL_LEN_MAX, \
    #member, \
    INT( 2 * sizeof( pointer ) ), \
    __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) ), \
    __int64( sizeof( (pointer)->member ) ), \
    ( (pointer)->member ) ? \
        "fTrue" : \
        "fFalse", \
    unsigned __int64( (pointer)->member )

#define FORMAT_ENUM( CLASS, pointer, member, offset, mpenumsz, enumMin, enumMax )   \
    "\t%*.*s <0x%0*I64X,%3I64u>:  %s (0x%I64X)\n", \
    SYMBOL_LEN_MAX, \
    SYMBOL_LEN_MAX, \
    #member, \
    INT( 2 * sizeof( pointer ) ), \
    __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) ), \
    __int64( sizeof( (pointer)->member ) ), \
    ( unsigned __int64( (pointer)->member ) < enumMin || unsigned __int64( (pointer)->member ) >= enumMax ) ? \
        "<Illegal>" : \
        mpenumsz[ unsigned __int64( (pointer)->member ) - enumMin ], \
    unsigned __int64( (pointer)->member )

#define FORMAT_INT_BF( CLASS, pointer, member, offset ) \
    "\t%*.*s <%-*.*s>:  %I64i (0x%I64X)\n", \
    SYMBOL_LEN_MAX, \
    SYMBOL_LEN_MAX, \
    #member, \
    INT( 2 + 2 * sizeof( pointer ) + 1 + 3 ), \
    INT( 2 + 2 * sizeof( pointer ) + 1 + 3 ), \
    "Bit-Field", \
    __int64( (pointer)->member ), \
    __int64( (pointer)->member ) & QwMaskForSmallerTypes( sizeof( DWORD_PTR ) )

#define FORMAT_UINT_BF( CLASS, pointer, member, offset )    \
    "\t%*.*s <%-*.*s>:  %I64u (0x%I64X)\n", \
    SYMBOL_LEN_MAX, \
    SYMBOL_LEN_MAX, \
    #member, \
    INT( 2 + 2 * sizeof( pointer ) + 1 + 3 ), \
    INT( 2 + 2 * sizeof( pointer ) + 1 + 3 ), \
    "Bit-Field", \
    unsigned __int64( (pointer)->member ), \
    unsigned __int64( (pointer)->member )

#define FORMAT_UINT_BF_NOLINE( CLASS, pointer, member, offset ) \
    "\t%*.*s <%-*.*s>:  %I64u (0x%I64X)", \
    SYMBOL_LEN_MAX, \
    SYMBOL_LEN_MAX, \
    #member, \
    INT( 2 + 2 * sizeof( pointer ) + 1 + 3 ), \
    INT( 2 + 2 * sizeof( pointer ) + 1 + 3 ), \
    "Bit-Field", \
    unsigned __int64( (pointer)->member ), \
    unsigned __int64( (pointer)->member )
    

#define FORMAT_ENUM_BF( CLASS, pointer, member, offset, mpenumsz, enumMin, enumMax )    \
    "\t%*.*s <%-*.*s>:  %s (0x%I64X)\n", \
    SYMBOL_LEN_MAX, \
    SYMBOL_LEN_MAX, \
    #member, \
    INT( 2 + 2 * sizeof( pointer ) + 1 + 3 ), \
    INT( 2 + 2 * sizeof( pointer ) + 1 + 3 ), \
    "Bit-Field", \
    ( unsigned __int64( (pointer)->member ) < enumMin || unsigned __int64( (pointer)->member ) >= enumMax ) ? \
        "<Illegal>" : \
        mpenumsz[ unsigned __int64( (pointer)->member ) - enumMin ], \
    unsigned __int64( (pointer)->member )

#define FORMAT_ENUM_BF_WSZ( CLASS, pointer, member, offset, mpenumwsz, enumMin, enumMax )   \
    "\t%*.*s <%-*.*s>:	%ws (0x%I64X)\n", \
    SYMBOL_LEN_MAX, \
    SYMBOL_LEN_MAX, \
    #member, \
    INT( 2 + 2 * sizeof( pointer ) + 1 + 3 ), \
    INT( 2 + 2 * sizeof( pointer ) + 1 + 3 ), \
    "Bit-Field", \
    ( unsigned __int64( (pointer)->member ) < enumMin || unsigned __int64( (pointer)->member ) >= enumMax ) ? \
        L"<Illegal>" : \
        mpenumwsz[ unsigned __int64( (pointer)->member ) - enumMin ], \
    unsigned __int64( (pointer)->member )

#define FORMAT_BOOL_BF( CLASS, pointer, member, offset )    \
    "\t%*.*s <%-*.*s>:  %s  (0x%I64X)\n", \
    SYMBOL_LEN_MAX, \
    SYMBOL_LEN_MAX, \
    #member, \
    INT( 2 + 2 * sizeof( pointer ) + 1 + 3 ), \
    INT( 2 + 2 * sizeof( pointer ) + 1 + 3 ), \
    "Bit-Field", \
    ( (pointer)->member ) ? \
        "fTrue" : \
        "fFalse", \
    unsigned __int64( (pointer)->member )

#define PRINT_METHOD_FLAG( pprintf, method )            \
    if ( method() )                                     \
    {                                                   \
        (*pprintf)( "\t\t\t\tTrue: %s\n", #method );        \
    }                                                   \
    else                                                \
    {                                                   \
        (*pprintf)( "\t\t\t\tFalse: %s\n", #method );   \
    }


#define FORMAT_SZ( CLASS, pointer, member, offset ) \
    "\t%*.*s <0x%0*I64X,%3I64u>:  %hs\n", \
    SYMBOL_LEN_MAX, \
    SYMBOL_LEN_MAX, \
    #member, \
    INT( 2 * sizeof( pointer ) ), \
    __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) ), \
    __int64( sizeof( (pointer)->member ) ), \
    ( (pointer)->member ) ? (char *)( (pointer)->member ) : "(null)"


#define FORMAT_WSZ( CLASS, pointer, member, offset )    \
    "\t%*.*s <0x%0*I64X,%3I64u>:  %ws\n", \
    SYMBOL_LEN_MAX, \
    SYMBOL_LEN_MAX, \
    #member, \
    INT( 2 * sizeof( pointer ) ), \
    __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) ), \
    __int64( sizeof( (pointer)->member ) ), \
    (pointer)->member ? (wchar_t *)( (pointer)->member ) : L"(null)"

#define FORMAT_WSZ_IN_TARGET( CLASS, pointer, member, offset )  \
    "\t%*.*s <0x%0*I64X,%3I64u>:  %mu\n", \
    SYMBOL_LEN_MAX, \
    SYMBOL_LEN_MAX, \
    #member, \
    INT( 2 * sizeof( pointer ) ), \
    __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) ), \
    __int64( sizeof( (pointer)->member ) ), \
    (ULONG64)(wchar_t *)( (pointer)->member )
    

#define FORMAT_LGPOS( CLASS, pointer, member, offset )  \
    "\t%*.*s <0x%0*I64X,%3I64u>:  %08lX:%04hX:%04hX\n", \
    SYMBOL_LEN_MAX, \
    SYMBOL_LEN_MAX, \
    #member, \
    INT( 2 * sizeof( pointer ) ), \
    __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) ), \
    __int64( sizeof( (pointer)->member ) ), \
    INT( (pointer)->member.lGeneration ), \
    SHORT( (pointer)->member.isec ), \
    SHORT( (pointer)->member.ib )

#define FORMAT_LE_LGPOS( CLASS, pointer, member, offset )   \
        "\t%*.*s <0x%0*I64X,%3I64u>:  %08lX:%04hX:%04hX\n", \
        SYMBOL_LEN_MAX, \
        SYMBOL_LEN_MAX, \
    #member, \
        INT( 2 * sizeof( pointer ) ), \
        __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) ), \
        __int64( sizeof( (pointer)->member ) ), \
        INT( (pointer)->member.le_lGeneration ), \
        SHORT( (pointer)->member.le_isec ), \
        SHORT( (pointer)->member.le_ib )

#define FORMAT_LOGTIME( CLASS, pointer, member, offset )    \
    "\t%*.*s <0x%0*I64X,%3I64u>:  %04d/%02d/%02d:%02d:%02d:%02d.%03d (%hs%hs)\n", \
    SYMBOL_LEN_MAX, \
    SYMBOL_LEN_MAX, \
    #member, \
    INT( 2 * sizeof( pointer ) ), \
    __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) ), \
    __int64( sizeof( (pointer)->member ) ), \
    ULONG( 1900 + (pointer)->member.bYear ), \
    ULONG( (pointer)->member.bMonth ), \
    ULONG( (pointer)->member.bDay ), \
    ULONG( (pointer)->member.bHours ), \
    ULONG( (pointer)->member.bMinutes ), \
    ULONG( (pointer)->member.bSeconds ), \
    ULONG( ( (pointer)->member.bMillisecondsHigh << 7 ) + (pointer)->member.bMillisecondsLow ), \
    ( ((pointer)->member).bYear ? ( ((pointer)->member).fTimeIsUTC ? "UTC Time" : "Local Time" ) : ( "" ) ), \
    ( (pointer)->member.fReserved ? ", fOSSnapshot=fTrue" : "" )

#define FORMAT_RBSPOS( CLASS, pointer, member, offset )  \
    "\t%*.*s <0x%0*I64X,%3I64u>:  %08lX:%08lX\n", \
    SYMBOL_LEN_MAX, \
    SYMBOL_LEN_MAX, \
    #member, \
    INT( 2 * sizeof( pointer ) ), \
    __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) ), \
    __int64( sizeof( (pointer)->member ) ), \
    INT( (pointer)->member.lGeneration ), \
    INT( (pointer)->member.iSegment )


#define _ALLOCATE_MEM_FOR_STRING_       256

#define PRINT_SZ_ON_HEAP( pprintf, CLASS, pointer, member, offset ) \
    if ( (pointer)->member ) \
    { \
        CHAR* szLocal; \
        if ( FFetchSz<CHAR>( const_cast<CHAR*>( (pointer)->member ), &szLocal ) ) \
        { \
            (*pprintf)( "\t%*.*s <0x%0*I64X,%3I64u>:  %s\n", \
                    SYMBOL_LEN_MAX, SYMBOL_LEN_MAX, \
                    #member, \
                    INT( 2 * sizeof( pointer ) ), \
                    __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) ), \
                    __int64( sizeof( (pointer)->member ) ), \
                    szLocal ); \
            Unfetch( szLocal ); \
        } \
    }

#define PRINT_WSZ_ON_HEAP( pprintf, CLASS, pointer, member, offset )    \
    if ( (pointer)->member ) \
    { \
        WCHAR* wszLocal; \
        if ( FFetchSz<WCHAR>( const_cast<WCHAR*>( (pointer)->member ), &wszLocal ) ) \
        { \
            (*pprintf)( "\t%*.*s <0x%0*I64X,%3I64u>:  %ws\n", \
                    SYMBOL_LEN_MAX, SYMBOL_LEN_MAX, \
                    #member, \
                    INT( 2 * sizeof( pointer ) ), \
                    __int64( (char*)(pointer) + offset + OffsetOf( CLASS, member ) ), \
                    __int64( sizeof( (pointer)->member ) ), \
                    wszLocal ); \
            Unfetch( wszLocal ); \
        } \
    }

