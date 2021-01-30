// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

enum SPLITTYPE  {
    splittypeMin = 0,
    splittypeVertical = 0,
    splittypeRight = 1,
    splittypeAppend = 2,
    splittypeMax = 3
};

enum MERGETYPE  {
    mergetypeMin = 0,
    mergetypeNone = 0,
    mergetypeEmptyPage = 1,
    mergetypeFullRight = 2,
    mergetypePartialRight = 3,
    mergetypeEmptyTree = 4,
    mergetypeFullLeft = 5,
    mergetypePartialLeft = 6,
    mergetypePageMove = 7,
    mergetypeMax = 8
};


enum SPLITOPER  {
    splitoperMin = 0,
    splitoperNone = 0,
    splitoperInsert = 1,
    splitoperReplace = 2,
    splitoperFlagInsertAndReplaceData = 3,
    splitoperMax = 4
};


struct LINEINFO
{
    ULONG           cbSize;
    ULONG           cbSizeMax;

    ULONG           cbSizeLE;
    ULONG           cbSizeMaxLE;

    ULONG           cbCommonPrev;
    INT             cbPrefix;

    KEYDATAFLAGS    kdf;
    FLAG32          fVerActive : 1;
};


const INT           ilineInvalid = -1;

class PREFIXINFO
{
    public:
        INT     ilinePrefix;
        INT     cbSavings;
        INT     ilineSegBegin;
        INT     ilineSegEnd;

    public:
        VOID    Nullify( );
        BOOL    FNull()     const;

        VOID    operator=( const PREFIXINFO& );
};

INLINE VOID PREFIXINFO::Nullify( )
{
    ilinePrefix = ilineInvalid;
    cbSavings = 0;
    ilineSegBegin = 0;
    ilineSegEnd = 0;
}

INLINE BOOL PREFIXINFO::FNull( ) const
{
    const BOOL fNull = ( ilineInvalid == ilinePrefix );

    Assert( !fNull ||
            0 == cbSavings && 0 == ilineSegBegin && 0 == ilineSegEnd );
    return fNull;
}

INLINE VOID PREFIXINFO::operator= ( const PREFIXINFO& prefixinfo )
{
    ilinePrefix = prefixinfo.ilinePrefix;
    cbSavings       = prefixinfo.cbSavings;
    ilineSegBegin   = prefixinfo.ilineSegBegin;
    ilineSegEnd     = prefixinfo.ilineSegEnd;
    return;
}


struct SPLIT
    :   public CZeroInit
{
    SPLIT()
        :   CZeroInit( sizeof( SPLIT ) ),
            dbtimeRightBefore( dbtimeInvalid )
    {
        prefixinfoSplit.Nullify();
        prefixSplitOld.Nullify();
        prefixSplitNew.Nullify();
        prefixinfoNew.Nullify();
        kdfParent.Nullify();
    }
    ~SPLIT()
    {
        if ( fAllocParent )
        {
            RESBOOKMARK.Free( kdfParent.key.suffix.Pv() );
        }
        RESBOOKMARK.Free( prefixSplitOld.Pv() );
        RESBOOKMARK.Free( prefixSplitNew.Pv() );
        csrNew.ReleasePage();
        csrRight.ReleasePage();
        delete [] rglineinfo;
    }

#pragma push_macro( "new" )
#undef new
private:
    void* operator new[]( size_t );
    void operator delete[]( void* );
public:
    void* operator new( size_t cbAlloc )
    {
        return RESSPLIT.PvRESAlloc_( SzNewFile(), UlNewLine() );
    }
    void operator delete( void* pv )
    {
        RESSPLIT.Free( pv );
    }
#pragma pop_macro( "new" )

    CSR                 csrNew;
    CSR                 csrRight;
    LittleEndian<PGNO>  pgnoSplit;
    PGNO                pgnoNew;

    DBTIME              dbtimeRightBefore;
    ULONG               fFlagsRightBefore;

    SPLITPATH*          psplitPath;

    SPLITTYPE           splittype:8;
    SPLITOPER           splitoper:8;
    ULONG               fAllocParent:1;
    ULONG               fAppend:1;
    ULONG               fHotpoint:1;

    INT                 ilineOper;
    INT                 clines;

    ULONG               fNewPageFlags;
    ULONG               fSplitPageFlags;

    SHORT               cbUncFreeDest;
    SHORT               cbUncFreeSrc;

    INT                 ilineSplit;

    PREFIXINFO          prefixinfoSplit;
    DATA                prefixSplitOld;
    DATA                prefixSplitNew;

    PREFIXINFO          prefixinfoNew;

    KEYDATAFLAGS        kdfParent;

    LINEINFO            *rglineinfo;
};


struct SPLITPATH
{
    SPLITPATH()
        :   dbtimeBefore( dbtimeInvalid ),
            fFlagsBefore( 0 ),
            psplitPathParent( NULL ),
            psplitPathChild( NULL ),
            psplit( NULL )
    {
    }
    ~SPLITPATH()
    {
        delete psplit;
        csr.ReleasePage();
        if ( psplitPathParent )
        {
            psplitPathParent->psplitPathChild = NULL;
        }
        if ( psplitPathChild )
        {
            psplitPathChild->psplitPathParent = NULL;
        }
    }

#pragma push_macro( "new" )
#undef new
private:
    void* operator new[]( size_t );
    void operator delete[]( void* );
public:
    void* operator new( size_t cbAlloc )
    {
        return RESSPLITPATH.PvRESAlloc_( SzNewFile(), UlNewLine() );
    }
    void operator delete( void* pv )
    {
        RESSPLITPATH.Free( pv );
    }
#pragma pop_macro( "new" )

    CSR         csr;

    DBTIME      dbtimeBefore;
    ULONG       fFlagsBefore;

    SPLITPATH*  psplitPathParent;
    SPLITPATH*  psplitPathChild;
    SPLIT*      psplit;
};

INLINE BOOL FBTISplitPageBeforeImageRequired( const SPLIT * const psplit )
{
    BOOL    fDependencyRequired;

    switch ( psplit->splittype )
    {
        case splittypeAppend:
            fDependencyRequired = fFalse;
            break;

        case splittypeRight:
            if ( splitoperInsert == psplit->splitoper
                && psplit->ilineSplit == psplit->ilineOper
                && psplit->ilineSplit == psplit->clines - 1 )
            {
                fDependencyRequired = fFalse;
            }
            else
            {
                Assert( !psplit->fHotpoint );
                fDependencyRequired = fTrue;
            }
            break;

        default:
            Assert( fFalse );
        case splittypeVertical:
            fDependencyRequired = fTrue;
            break;
    }

    return fDependencyRequired;
}


struct MERGE
    :   public CZeroInit
{
    MERGE()
        :   CZeroInit( sizeof( MERGE ) ),
            dbtimeLeftBefore( dbtimeInvalid ),
            dbtimeRightBefore( dbtimeInvalid )
    {
        kdfParentSep.Nullify();
    }
    ~MERGE()
    {
        if ( fAllocParentSep )
        {
            RESBOOKMARK.Free( kdfParentSep.key.suffix.Pv() );
        }
        csrLeft.ReleasePage();
        csrRight.ReleasePage();
        csrNew.ReleasePage();
        delete [] rglineinfo;
    }

#pragma push_macro( "new" )
#undef new
private:
    void* operator new[]( size_t );
    void operator delete[]( void* );
public:
    void* operator new( size_t cbAlloc )
    {
        return RESMERGE.PvRESAlloc_( SzNewFile(), UlNewLine() );
    }
    void operator delete( void* pv )
    {
        RESMERGE.Free( pv );
    }
#pragma pop_macro( "new" )

    CSR             csrLeft;
    CSR             csrRight;
    CSR             csrNew;
    PGNO            pgnoNew;

    DBTIME          dbtimeLeftBefore;
    DBTIME          dbtimeRightBefore;
    ULONG           fFlagsLeftBefore;
    ULONG           fFlagsRightBefore;

    MERGEPATH*      pmergePath;

    KEYDATAFLAGS    kdfParentSep;

    MERGETYPE       mergetype:8;
    UINT            fAllocParentSep:1;

    INT             ilineMerge;

    INT             cbSavings;
    INT             cbSizeTotal;
    INT             cbSizeMaxTotal;

    INT             cbUncFreeDest;
    INT             clines;
    LINEINFO        *rglineinfo;
};


struct MERGEPATH
    :   public CZeroInit
{
    MERGEPATH()
        :   CZeroInit( sizeof( MERGEPATH ) ),
            dbtimeBefore( dbtimeInvalid )
    {
    }
    ~MERGEPATH()
    {
        delete pmerge;
        csr.ReleasePage();
        if ( pmergePathParent )
        {
            pmergePathParent->pmergePathChild = NULL;
        }
        if ( pmergePathChild )
        {
            pmergePathChild->pmergePathParent = NULL;
        }
    }

#pragma push_macro( "new" )
#undef new
private:
    void* operator new[]( size_t );
    void operator delete[]( void* );
public:
    void* operator new( size_t cbAlloc )
    {
        return RESMERGEPATH.PvRESAlloc_( SzNewFile(), UlNewLine() );
    }
    void operator delete( void* pv )
    {
        RESMERGEPATH.Free( pv );
    }
#pragma pop_macro( "new" )

    CSR             csr;

    ULONG           fFlagsBefore;
    DBTIME          dbtimeBefore;

    MERGEPATH*      pmergePathParent;
    MERGEPATH*      pmergePathChild;
    MERGE*          pmerge;

    SHORT           iLine;
    USHORT          fKeyChange:1;
    USHORT          fDeleteNode:1;
    USHORT          fEmptyPage:1;
};

INLINE BOOL FRightMerge( const MERGETYPE mergetype )
{
    if( mergetypePartialLeft == mergetype
        || mergetypeFullLeft == mergetype )
    {
        return fFalse;
    }
    return fTrue;
}

INLINE BOOL FBTIMergePageBeforeImageRequired( const MERGE * const pmerge )
{
    return ( pmerge->mergetype == mergetypePartialRight
             || pmerge->mergetype == mergetypeFullRight
             || pmerge->mergetype == mergetypePartialLeft
             || pmerge->mergetype == mergetypeFullLeft );
    
}

#define cBTMaxDepth 8

#include <pshpack1.h>

PERSISTED
struct EMPTYPAGE
{
    UnalignedLittleEndian<DBTIME>   dbtimeBefore;
    UnalignedLittleEndian<PGNO>     pgno;
    UnalignedLittleEndian<ULONG>    ulFlags;
};

#include <poppack.h>

