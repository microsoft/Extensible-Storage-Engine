// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  types of split
//
enum SPLITTYPE  {
    splittypeMin = 0,
    splittypeVertical = 0,
    splittypeRight = 1,
    splittypeAppend = 2,
    splittypeMax = 3
};

//  types of merge
//
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


//  operation to be performed with split
//
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
    ULONG           cbSize;             //  size of node
    ULONG           cbSizeMax;          //  maximum size of node

    ULONG           cbSizeLE;           //  sum of size of all nodes
                                        //  <= this node in page
    ULONG           cbSizeMaxLE;        //  includes reserved space
                                        //  in version store

    ULONG           cbCommonPrev;       //  common bytes with previous key
    INT             cbPrefix;           //  common bytes with prefix

    KEYDATAFLAGS    kdf;
    FLAG32          fVerActive : 1;     //  used by OLC
};


const INT           ilineInvalid = -1;

///typedef struct _prefixinfo
class PREFIXINFO
{
    public:
        INT     ilinePrefix;        //  line of optimal prefix [-1 if no prefix]
        INT     cbSavings;          //  bytes saved due to prefix compression
        INT     ilineSegBegin;      //  set of lines that contribute
        INT     ilineSegEnd;        //  to selected prefix

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


//  split structures
//
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
    void* operator new[]( size_t );         //  not supported
    void operator delete[]( void* );        //  not supported
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
    CSR                 csrRight;           //  only for leaf level of horizontal split
    LittleEndian<PGNO>  pgnoSplit;
    PGNO                pgnoNew;

    // This dbtime/fFlags pair must be updated atomically so it can be reverted to
    // a consistent state if neeed.
    DBTIME              dbtimeRightBefore;  // dbtime of the right page before dirty
    ULONG               fFlagsRightBefore;  // flags on the right page before dirty

    SPLITPATH*          psplitPath;

    SPLITTYPE           splittype:8;
    SPLITOPER           splitoper:8;
    ULONG               fAllocParent:1;     //  is kdfParent allocated? [only for leaf pages]
    ULONG               fAppend:1;          //  append insert
    ULONG               fHotpoint:1;        //  hotpoint insert

    INT                 ilineOper;          //  line of insert/replace
    INT                 clines;             //  number of lines in rglineinfo
                                            //   == number of lines in page + 1 for operinsert
                                            //   == number of lines in page otherwise

    ULONG               fNewPageFlags;      //  page flags for split and new pages
    ULONG               fSplitPageFlags;

    SHORT               cbUncFreeDest;
    SHORT               cbUncFreeSrc;

    INT                 ilineSplit;         //  line to move from

    PREFIXINFO          prefixinfoSplit;    //  info for prefix selected
                                            //  for split page
    DATA                prefixSplitOld;     //  old prefix in split page
    DATA                prefixSplitNew;     //  new prefix for split page

    PREFIXINFO          prefixinfoNew;      //  info for prefix seleceted for
                                            //  new page

    KEYDATAFLAGS        kdfParent;          //  separator key to insert at parent

    LINEINFO            *rglineinfo;        //  info on all lines in page
                                            //  plus the line inserted/replaced
};


//  stores csrPath and related split info for split
//
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
    void* operator new[]( size_t );         //  not supported
    void operator delete[]( void* );        //  not supported
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

    CSR         csr;                //  page at current level

    // This dbtime/fFlags pair must be updated atomically so it can be reverted to
    // a consistent state if neeed.
    DBTIME      dbtimeBefore;       //  dbtime of the page before dirty
    ULONG       fFlagsBefore;       //  flags on the page before dirty

    SPLITPATH*  psplitPathParent;   //  split path to parent level
    SPLITPATH*  psplitPathChild;    //  split path to child level
    SPLIT*      psplit;             //  if not NULL, split is occurning
                                    //  at this level
};

INLINE BOOL FBTISplitPageBeforeImageRequired( const SPLIT * const psplit )
{
    BOOL    fDependencyRequired;

    //  if nodes will be moving to the new page, depend the
    //  new page on the split page so that the data moved
    //  from the split page to the new page will always be
    //  available no matter when we crash
    //
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
                //  this is either a hotpoint split, or a
                //  natural split where the only node moving
                //  to the new page is the node being inserted
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


//  merge structure -- only for leaf page
//
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
    void* operator new[]( size_t );         //  not supported
    void operator delete[]( void* );        //  not supported
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
    CSR             csrNew;                 // newly allocated page used for page moves
    PGNO            pgnoNew;

    // Each of these dbtime/fFlags pairs must be updated atomically so they can be reverted to
    // a consistent state if neeed.
    DBTIME          dbtimeLeftBefore;       // dbtime of the left page before dirty
    DBTIME          dbtimeRightBefore;      // dbtime of the right page before dirty
    ULONG           fFlagsLeftBefore;       // flags on the left page before dirty
    ULONG           fFlagsRightBefore;      // flags on the right page before dirty

    MERGEPATH*      pmergePath;

    KEYDATAFLAGS    kdfParentSep;       //  separator key to insert at grand parent
                                        //  only if last page pointer is removed

    MERGETYPE       mergetype:8;
    UINT            fAllocParentSep:1;  //  is kdfParent allocated? [only for internal pages]

    INT             ilineMerge;     //  line to move from

    INT             cbSavings;      //  savings due to key compression on nodes to move
    INT             cbSizeTotal;    //  total size of nodes to move
    INT             cbSizeMaxTotal; //  max size of nodes to move

    INT             cbUncFreeDest;  //  uncommitted freed space in right page
    INT             clines;         //  number of lines in merged page
    LINEINFO        *rglineinfo;    //  info on all lines in page
};


//  stores csrPath and related merge info for merge/empty page operations
//
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
    void* operator new[]( size_t );         //  not supported
    void operator delete[]( void* );        //  not supported
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

    CSR             csr;                //  page at current level

    // This dbtime/fFlags pair must be updated atomically so it can be reverted to
    // a consistent state if neeed.
    ULONG           fFlagsBefore;       //  flags on the page before dirty
    DBTIME          dbtimeBefore;       //  dbtime of the page before dirty

    MERGEPATH*      pmergePathParent;   //  merge path to parent level
    MERGEPATH*      pmergePathChild;    //  merge path to child level
    MERGE*          pmerge;             //  if not NULL, split is occurring
                                        //  at this level

    SHORT           iLine;              //  seeked node in page
    USHORT          fKeyChange:1;       //  is there a key change in this page?
    USHORT          fDeleteNode:1;      //  should node iLine in page be deleted?
    USHORT          fEmptyPage:1;       //  is this page empty?
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
    UnalignedLittleEndian<ULONG>    ulFlags;            //  copy page flags for easier debugging
};

#include <poppack.h>

