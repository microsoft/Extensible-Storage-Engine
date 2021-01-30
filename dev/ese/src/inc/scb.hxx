// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef _SCB_H
#error SCB.HXX already included
#endif
#define _SCB_H



const LONG cbSortMemFast = ( 16 * ( 4088 + 1 ) );

const LONG cbSortMemNorm = ( 1024 * 1024 );

const LONG cbResidentTTMax = ( 64 * 1024 );

const LONG cspairQSortMin = 32;

const LONG cpartQSortMax = 16;

const LONG crunFanInMax = 16;

const LONG cpgClusterSize = 16;

#include <pshpack1.h>

PERSISTED
struct SREC
{
    UnalignedLittleEndian< USHORT >     cbRec;
    UnalignedLittleEndian< USHORT >     cbKey;
    BYTE                                rgbKey[];
};

const INT cbSrecHeader = sizeof( USHORT ) + sizeof( USHORT );

#include <poppack.h>


#include <pshpack1.h>

PERSISTED
struct SPAGE_FIX
{
    LittleEndian< ULONG >       ulChecksum;
    LittleEndian< PGNO >        pgnoThisPage;
};

#define cbFreeSPAGE ( max( g_cbPage, g_cbPageMin ) - sizeof( SPAGE_FIX ) )

#include <poppack.h>

INLINE BYTE *PbDataStartPspage( SPAGE_FIX *pspage )
{
    return (BYTE *)( pspage ) + sizeof( SPAGE_FIX );
}

INLINE BYTE *PbDataEndPspage( SPAGE_FIX *pspage )
{
    return (BYTE *)( pspage ) + g_cbPage;
}

#define cspageSortMax (cbSortMemNorm / cbFreeSPAGE)

#define cbSortMemNormUsed ( cspageSortMax * cbFreeSPAGE )



const INT cbKeyPrefix = 14;

#include <pshpack1.h>

struct SPAIR
{
    USHORT      irec;
    BYTE        rgbKey[cbKeyPrefix];
};

#include <poppack.h>

#define cbIndexGran (( cbSortMemNormUsed + 0xFFFFL ) / 0x10000L )

#define irecSortMax ( cbSortMemNormUsed / cbIndexGran )

const INT cspairSortMax = cbSortMemFast / sizeof( SPAIR ) - 1;

const INT cbSortMemFastUsed = ( cspairSortMax + 1 ) * sizeof( SPAIR );

INLINE LONG CirecToStoreCb( LONG cb )
{
    return ( cb + cbIndexGran - 1 ) / cbIndexGran;
}



typedef PGNO        RUN;

const RUN runNull = (RUN) pgnoNull;
const RUN crunAll = 0x7FFFFFFFL;



struct RUNINFO
{
    RUN     run;
    CPG     cpg;
    QWORD   cbRun;
    ULONG   crecRun;
    CPG     cpgUsed;
};



struct RUNLINK
{
    RUNLINK             *prunlinkNext;
    RUNINFO             runinfo;
};

RUNLINK * const prunlinkNil = 0;



INLINE RUNLINK * PrunlinkRUNLINKAlloc()  { return new RUNLINK; }

INLINE VOID RUNLINKReleasePrunlink( RUNLINK *& prunlink )
{
    delete prunlink;
#ifdef DEBUG
    prunlink = 0;
#endif
}


struct RUNLIST
{
    RUNLINK         *prunlinkHead;
    LONG            crun;
};



struct MTNODE
{
    struct RCB      *prcb;
    MTNODE  *pmtnodeExtUp;

    SREC            *psrec;
    MTNODE          *pmtnodeSrc;
    MTNODE          *pmtnodeIntUp;
};

SREC * const psrecNegInf    = ( SREC * ) -1;
SREC * const psrecInf       = ( SREC * ) 0;



struct OTNODE
{
    RUNLIST     runlist;
    OTNODE      *rgpotnode[crunFanInMax];
    OTNODE      *potnodeAllocNext;
    OTNODE      *potnodeLevelNext;
};

OTNODE * const potnodeNil = (OTNODE *) 0;

OTNODE * const potnodeLevel0 = (OTNODE *) -1;



INLINE OTNODE * PotnodeOTNODEAlloc() { return new OTNODE; }

INLINE VOID OTNODEReleasePotnode( OTNODE *& potnode )
{
    delete potnode;
#ifdef DEBUG                    
    potnode = 0;
#endif
}



struct SCB
{
    SCB( const IFMP ifmp, const PGNO pgnoFDP )
        :   fcb( ifmp, pgnoFDP ),
            rgbRec( NULL ),
            rgspair( NULL )
    {
    }
    ~SCB()
    {
        OSMemoryPageFree( rgbRec );
        OSMemoryPageFree( rgspair );
    }

#pragma push_macro( "new" )
#undef new
private:
    void* operator new( size_t );
    void* operator new[]( size_t );
    void operator delete[]( void* );
public:
    void* operator new( size_t cbAlloc, INST* pinst )
    {
        return pinst->m_cresSCB.PvRESAlloc_( SzNewFile(), UlNewLine() );
    }
    void operator delete( void* pv )
    {
        if ( pv )
        {
            SCB* pscb = reinterpret_cast< SCB* >( pv );
            PinstFromIfmp( pscb->fcb.Ifmp() )->m_cresSCB.Free( pv );
        }
    }
#pragma pop_macro( "new" )


    FCB         fcb;

    JET_GRBIT   grbit;
    INT         fFlags;

    QWORD       cRecords;

    SPAIR       *rgspair;
    LONG        ispairMac;

    BYTE        *rgbRec;
    LONG        irecMac;
    LONG        crecBuf;
    LONG        cbData;

    LONG        crun;
    RUNLIST     runlist;

    PGNO        pgnoNext;
    BFLatch     bflOut;
    BYTE        *pbOutMac;
    BYTE        *pbOutMax;

    LONG        crunMerge;
    BYTE        rgbReserved1[4];
    MTNODE      rgmtnode[crunFanInMax];

    BFLatch     bflLast;
    VOID        *pvAssyLast;

};

SCB * const pscbNil = 0;


INLINE VOID SCBTerm( INST *pinst )
{
    pinst->m_cresSCB.Term();
    if ( pinst->FRecovering() )
    {
        CallS( ErrRESSetResourceParam( pinst, JET_residSCB, JET_resoperEnableMaxUse, fTrue ) );
    }
}

INLINE ERR ErrSCBInit( INST *pinst )
{
    ERR err = JET_errSuccess;

    if ( UlParam( pinst, JET_paramMaxTemporaryTables ) == 0 )
    {
        return JET_errSuccess;
    }

    if ( pinst->FRecovering() )
    {
        CallS( ErrRESSetResourceParam( pinst, JET_residSCB, JET_resoperEnableMaxUse, fFalse ) );
    }
    Call( pinst->m_cresSCB.ErrInit( JET_residSCB ) );

HandleError:
    if ( err < JET_errSuccess )
    {
        SCBTerm( pinst );
    }
    return err;
}


const INT fSCBInsert                    = 0x0001;
const INT fSCBRemoveDuplicateKey        = 0x0002;
const INT fSCBRemoveDuplicateKeyData    = 0x0004;


INLINE BOOL FSCBInsert                      ( const SCB * pscb )    { return pscb->fFlags & fSCBInsert; }
INLINE VOID SCBSetInsert                    ( SCB * pscb )          { pscb->fFlags |= fSCBInsert;  Assert( FSCBInsert( pscb ) ); }
INLINE VOID SCBResetInsert                  ( SCB * pscb )          { pscb->fFlags &= ~fSCBInsert; Assert( !FSCBInsert( pscb ) ); }

INLINE BOOL FSCBRemoveDuplicateKey          ( const SCB * pscb )    { return pscb->fFlags & fSCBRemoveDuplicateKey; }
INLINE VOID SCBSetRemoveDuplicateKey        ( SCB * pscb )          { pscb->fFlags |= fSCBRemoveDuplicateKey;  Assert( FSCBRemoveDuplicateKey( pscb ) ); }
INLINE VOID SCBResetRemoveDuplicateKey      ( SCB * pscb )          { pscb->fFlags &= ~fSCBRemoveDuplicateKey; Assert( !FSCBRemoveDuplicateKey( pscb ) ); }

INLINE BOOL FSCBRemoveDuplicateKeyData      ( const SCB * pscb )    { return pscb->fFlags & fSCBRemoveDuplicateKeyData; }
INLINE VOID SCBSetRemoveDuplicateKeyData    ( SCB * pscb )          { pscb->fFlags |= fSCBRemoveDuplicateKeyData;  Assert( FSCBRemoveDuplicateKeyData( pscb ) ); }
INLINE VOID SCBResetRemoveDuplicateKeyData  ( SCB * pscb )          { pscb->fFlags &= ~fSCBRemoveDuplicateKeyData; Assert( !FSCBRemoveDuplicateKeyData( pscb ) ); }

const INT cbSRECReadMin = OffsetOf( SREC, cbKey );


INLINE VOID SCBInsertHashTable( SCB *pscb )
{
    pscb->fcb.InsertHashTable();
}
INLINE VOID SCBDeleteHashTable( SCB *pscb )
{
    pscb->fcb.DeleteHashTable();
}



INLINE LONG CbSRECSizePsrec( const SREC * psrec )
{
    return ( (SREC * ) psrec )->cbRec;
}

INLINE LONG CbSRECSizeCbCb( LONG cbKey, LONG cbData )
{
    return sizeof( SREC ) + cbKey + cbData;
}

INLINE VOID SRECSizePsrecCb( SREC * psrec, LONG cb )
{
    ( (SREC * ) psrec )->cbRec = (USHORT) cb;
}

INLINE LONG CbSRECKeyPsrec( const SREC * psrec )
{
    return ( (SREC * ) psrec )->cbKey;
}

INLINE VOID SRECKeySizePsrecCb( SREC * psrec, LONG cb )
{
    Assert( cb > 0 );
    Assert( cb <= cbKeyMostMost );
    psrec->cbKey = (SHORT)cb;
}

INLINE BYTE * PbSRECKeyPsrec( const SREC * psrec )
{
    return ( BYTE *) psrec->rgbKey;
}

INLINE BYTE * StSRECKeyPsrec( const SREC * psrec )
{
    return ( BYTE * ) &psrec->cbKey;
}

INLINE LONG CbSRECDataPsrec( const SREC * psrec )
{
    return psrec->cbRec - psrec->cbKey - sizeof( SREC );
}

INLINE BYTE * PbSRECDataPsrec( const SREC * psrec )
{
    return (BYTE *)(psrec->rgbKey + psrec->cbKey);
}

INLINE SREC * PsrecFromPbIrec( const BYTE * pb, LONG irec )
{
    return (SREC *) ( pb + irec * cbIndexGran );
}

INLINE LONG CbSRECKeyDataPsrec( const SREC * psrec )
{
    return psrec->cbRec - sizeof ( SREC );
}


struct RCB
{
    SCB             *pscb;
    RUNINFO         runinfo;
    BFLatch         rgbfl[cpgClusterSize];
    LONG            ipbf;
    BYTE            *pbInMac;
    BYTE            *pbInMax;
    VOID            *pvAssy;
    QWORD           cbRemaining;
};

RCB * const prcbNil = 0;


INLINE RCB * PrcbRCBAlloc() { return new RCB; }

INLINE VOID RCBReleasePrcb( RCB *& prcb )
{
    delete prcb;
#ifdef DEBUG
    prcb = 0;
#endif
}

