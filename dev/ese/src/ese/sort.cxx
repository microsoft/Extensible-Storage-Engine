// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"


INLINE VOID SrecToKeydataflags( const SREC * psrec, FUCB * pfucb );
LOCAL LONG IspairSORTISeekByKey(
    const BYTE * const rgbRec,
    const SPAIR * const rgspair,
    const INT ispairMac,
    const KEY& keySeek,
    INT * const plastCompare);
INLINE INT ISORTICmpKey( const SREC * psrec1, const SREC * psrec2 );
INLINE INT ISORTICmpKeyData( const SREC * psrec1, const SREC * psrec2 );
INLINE BOOL FSORTIDuplicate( SCB* const pscb, const SREC * psrec1, const SREC * psrec2 );
LOCAL LONG CspairSORTIUnique( SCB* pscb, BYTE * rgbRec, __inout_ecount(ispairMac) SPAIR * rgspair, const LONG ispairMac );
LOCAL ERR ErrSORTIOutputRun( SCB * pscb );
INLINE INT ISORTICmpPspairPspair( const SCB * pscb, const SPAIR * pspair1, const SPAIR * pspair2 );
LOCAL INT ISORTICmp2PspairPspair( const SCB * pscb, const SPAIR * pspair1, const SPAIR * pspair2 );
INLINE VOID SWAPPspair( SPAIR **ppspair1, SPAIR **ppspair2 );
INLINE VOID SWAPSpair( SPAIR *pspair1, SPAIR *pspair2 );
INLINE VOID SWAPPsrec( SREC **ppsrec1, SREC **ppsrec2 );
INLINE VOID SWAPPmtnode( MTNODE **ppmtnode1, MTNODE **ppmtnode2 );
LOCAL VOID SORTIInsertionSort( SCB *pscb, SPAIR *pspairMinIn, SPAIR *pspairMaxIn );
LOCAL VOID SORTIQuicksort( SCB * pscb, SPAIR *pspairMinIn, SPAIR *pspairMaxIn );
LOCAL ERR ErrSORTIRunStart( SCB *pscb, QWORD cb, RUNINFO *pruninfo );
LOCAL ERR ErrSORTIRunInsert( SCB *pscb, RUNINFO* pruninfo, SREC *psrec );
INLINE VOID SORTIRunEnd( SCB * pscb, RUNINFO* pruninfo );
INLINE VOID SORTIRunDelete( SCB * pscb, const RUNINFO * pruninfo );
LOCAL VOID  SORTIRunDeleteList( SCB *pscb, RUNLINK **pprunlink, LONG crun );
LOCAL VOID  SORTIRunDeleteListMem( SCB *pscb, RUNLINK **pprunlink, LONG crun );
LOCAL ERR ErrSORTIRunOpen( SCB *pscb, RUNINFO *pruninfo, RCB **pprcb );
LOCAL ERR ErrSORTIRunNext( RCB * prcb, SREC **ppsrec );
LOCAL VOID SORTIRunClose( RCB *prcb );
INLINE ERR ErrSORTIRunReadPage( RCB *prcb, PGNO pgno, LONG ipbf );
LOCAL ERR ErrSORTIMergeToRun( SCB *pscb, RUNLINK *prunlinkSrc, RUNLINK **pprunlinkDest );
LOCAL ERR ErrSORTIMergeStart( SCB *pscb, RUNLINK *prunlinkSrc );
LOCAL ERR ErrSORTIMergeFirst( SCB *pscb, SREC **ppsrec );
LOCAL ERR ErrSORTIMergeNext( SCB *pscb, SREC **ppsrec );
LOCAL VOID SORTIMergeEnd( SCB *pscb );
LOCAL ERR ErrSORTIMergeNextChamp( SCB *pscb, SREC **ppsrec );
INLINE VOID SORTIOptTreeInit( SCB *pscb );
LOCAL ERR ErrSORTIOptTreeAddRun( SCB *pscb, RUNINFO *pruninfo );
LOCAL ERR ErrSORTIOptTreeMerge( SCB *pscb );
INLINE VOID SORTIOptTreeTerm( SCB *pscb );
LOCAL ERR ErrSORTIOptTreeBuild( SCB *pscb, OTNODE **ppotnode );
LOCAL ERR ErrSORTIOptTreeMergeDF( SCB *pscb, OTNODE *potnode, RUNLINK **pprunlink );
LOCAL VOID SORTIOptTreeFree( SCB *pscb, OTNODE *potnode );

INLINE VOID SrecToKeydataflags( const SREC * psrec, FUCB * pfucb )
{
    pfucb->locLogical               = locOnCurBM;
    pfucb->kdfCurr.key.prefix.Nullify();
    pfucb->kdfCurr.key.suffix.SetCb( CbSRECKeyPsrec( psrec ) );
    pfucb->kdfCurr.key.suffix.SetPv( PbSRECKeyPsrec( psrec ) );
    pfucb->kdfCurr.data.SetCb( CbSRECDataPsrec( psrec ) );
    pfucb->kdfCurr.data.SetPv( PbSRECDataPsrec( psrec ) );
    pfucb->kdfCurr.fFlags           = 0;
}


ERR ErrSORTOpen( PIB *ppib, FUCB **ppfucb, const BOOL fRemoveDuplicateKey, const BOOL fRemoveDuplicateKeyData )
{
    ERR     err         = JET_errSuccess;
    FUCB    * pfucb     = pfucbNil;
    SCB     * pscb      = pscbNil;
    SPAIR   * rgspair   = 0;
    BYTE    * rgbRec    = 0;

    
    INST *pinst = PinstFromPpib( ppib );

    CallR( ErrEnsureTempDatabaseOpened( pinst, ppib ) );
    
    IFMP ifmpTemp = pinst->m_mpdbidifmp[ dbidTemp ];
    CallR( ErrFUCBOpen( ppib, ifmpTemp, &pfucb ) );
    if ( ( pscb = new( pinst ) SCB( ifmpTemp, pgnoNull ) ) == pscbNil )
    {
        Error( ErrERRCheck( JET_errTooManySorts ) );
    }

    pscb->fcb.Lock();

    
    pscb->fcb.SetTypeSort();
    pscb->fcb.SetFixedDDL();
    pscb->fcb.SetPrimaryIndex();
    pscb->fcb.SetSequentialIndex();
    pscb->fcb.SetIntrinsicLVsOnly();


    pscb->fcb.CreateComplete();

    pscb->fcb.Unlock();

    FUCBSetSort( pfucb );
    SCBSetInsert( pscb );
    SCBResetRemoveDuplicateKey( pscb );
    SCBResetRemoveDuplicateKeyData( pscb );
    if ( fRemoveDuplicateKey )
    {
        SCBSetRemoveDuplicateKey( pscb );
    }
    if ( fRemoveDuplicateKeyData )
    {
        SCBSetRemoveDuplicateKeyData( pscb );
    }
    pscb->cRecords  = 0;

    
    Alloc( rgspair = ( SPAIR * )( PvOSMemoryPageAlloc( cbSortMemFastUsed, NULL ) ) );
    pscb->rgspair   = rgspair;

    Alloc( rgbRec = ( BYTE * )( PvOSMemoryPageAlloc( cbSortMemNormUsed, NULL ) ) );
    pscb->rgbRec    = rgbRec;

    
    pscb->ispairMac = 0;

    
    pscb->irecMac   = 0;
    pscb->crecBuf   = 0;
    pscb->cbData    = 0;

    
    pscb->crun = 0;

    
    Call( pscb->fcb.ErrLink( pfucb ) );

    
    Assert( pscb->fcb.PgnoFDP() == pgnoNull );

    
    *ppfucb = pfucb;

    err = JET_errSuccess;

HandleError:
    if ( JET_errSuccess != err )
    {
        delete pscb;
        if ( pfucb != pfucbNil )
        {
            FUCBClose( pfucb );
        }
    }

    return err;
}



ERR ErrSORTInsert( FUCB *pfucb, const KEY& key, const DATA& data )
{
    SCB     * const pscb    = pfucb->u.pscb;
    LONG    cbKey;
    LONG    cbData;
    LONG    irec;
    SREC    * psrec         = 0;
    SPAIR   * pspair        = 0;
    BYTE    * pbSrc         = 0;
    BYTE    * pbSrcMac      = 0;
    BYTE    * pbDest        = 0;
    BYTE    * pbDestMic     = 0;
    ERR     err             = JET_errSuccess;

    Assert( key.Cb() <= cbKeyMostMost );
    Assert( FSCBInsert( pscb ) );


    Assert( pscb->crecBuf <= cspairSortMax );
    Assert( (SIZE_T)pscb->irecMac <= irecSortMax );


    INT cbNormNeeded = CbSRECSizeCbCb( key.Cb(), data.Cb() );
    INT cirecNeeded = CirecToStoreCb( cbNormNeeded );


    if (    pscb->irecMac * cbIndexGran + cbNormNeeded > cbSortMemNormUsed ||
            pscb->crecBuf == cspairSortMax )
    {

        SORTIQuicksort( pscb, pscb->rgspair, pscb->rgspair + pscb->ispairMac );


        Call( ErrSORTIOutputRun( pscb ) );
    }


    irec = pscb->irecMac;
    psrec = PsrecFromPbIrec( pscb->rgbRec, irec );
    pscb->irecMac += cirecNeeded;
    pscb->crecBuf++;
    pscb->cbData += cbNormNeeded;
    SRECSizePsrecCb( psrec, cbNormNeeded );
    SRECKeySizePsrecCb( psrec, key.Cb() );
    key.CopyIntoBuffer( PbSRECKeyPsrec( psrec ) );
    UtilMemCpy( PbSRECDataPsrec( psrec ), data.Pv(), data.Cb() );



    pspair = pscb->rgspair + pscb->ispairMac++;


    cbKey = CbSRECKeyPsrec( psrec );
    pbSrc = PbSRECKeyPsrec( psrec );
    pbSrcMac = pbSrc + min( cbKey, cbKeyPrefix );
    pbDest = pspair->rgbKey + cbKeyPrefix - 1;

    while ( pbSrc < pbSrcMac )
        *( pbDest-- ) = *( pbSrc++ );


    if ( pbDest >= pspair->rgbKey )
    {

        cbData = (LONG)min( pbDest - pspair->rgbKey + 1, CbSRECDataPsrec( psrec ) );
        pbSrc = PbSRECDataPsrec( psrec );
        pbDestMic = max( pspair->rgbKey, pbDest - cbData + 1 );

        while ( pbDest >= pbDestMic )
            *( pbDest-- ) = *( pbSrc++ );


        if ( pbDest >= pspair->rgbKey )
            memset( pspair->rgbKey, 0, pbDest - pspair->rgbKey + 1 );
    }



    pspair->irec = (USHORT) irec;


    pscb->cRecords++;


    Assert( pscb->crecBuf <= cspairSortMax );
    Assert( (SIZE_T)pscb->irecMac <= irecSortMax );

HandleError:
    return err;
}



ERR ErrSORTEndInsert( FUCB *pfucb )
{
    SCB     * const pscb    = pfucb->u.pscb;
    ERR     err     = JET_errSuccess;


    Assert( FSCBInsert( pscb ) );


    SCBResetInsert( pscb );

    SORTBeforeFirst( pfucb );


    if ( !pscb->cRecords )
        return JET_errSuccess;


    SORTIQuicksort( pscb, pscb->rgspair, pscb->rgspair + pscb->ispairMac );


    if ( pscb->crun )
    {

        Call( ErrSORTIOutputRun( pscb ) );


        OSMemoryPageFree( pscb->rgspair );
        pscb->rgspair = NULL;
        OSMemoryPageFree( pscb->rgbRec );
        pscb->rgbRec = NULL;


        Call( ErrSORTIOptTreeMerge( pscb ) );


        Call( ErrSORTIMergeStart( pscb, pscb->runlist.prunlinkHead ) );
    }


    else
    {
        pscb->ispairMac = CspairSORTIUnique(    pscb,
                                                pscb->rgbRec,
                                                pscb->rgspair,
                                                pscb->ispairMac );
        pscb->cRecords = pscb->ispairMac;
    }


    err = ( pscb->crun > 0 || pscb->irecMac * cbIndexGran > cbResidentTTMax ) ?
                ErrERRCheck( JET_wrnSortOverflow ) :
                JET_errSuccess;

HandleError:
    return err;
}


ERR ErrSORTFirst( FUCB * pfucb )
{
    SCB     * const pscb    = pfucb->u.pscb;
    SREC    * psrec         = 0;
    LONG    irec;
    ERR     err             = JET_errSuccess;


    Assert( !FSCBInsert( pscb ) );


    FUCBResetLimstat( pfucb );


    if ( !pscb->cRecords )
    {
        DIRBeforeFirst( pfucb );
        return ErrERRCheck( JET_errNoCurrentRecord );
    }

    Assert( pscb->crun > 0 || pscb->ispairMac > 0 );


    if ( pscb->crun )
    {
        CallR( ErrSORTIMergeFirst( pscb, &psrec ) );
    }


    else
    {
        pfucb->ispairCurr = 0L;
        irec = pscb->rgspair[pfucb->ispairCurr].irec;
        psrec = PsrecFromPbIrec( pscb->rgbRec, irec );
    }


    SrecToKeydataflags( psrec, pfucb );

    return JET_errSuccess;
}


ERR ErrSORTLast( FUCB *pfucb )
{
    SCB     *pscb   = pfucb->u.pscb;
    SREC    *psrec  = 0;
    LONG    irec;
    ERR     err = JET_errSuccess;


    Assert( !FSCBInsert( pscb ) );


    FUCBResetLimstat( pfucb );


    if ( !pscb->cRecords )
    {
        DIRAfterLast( pfucb );
        return ErrERRCheck( JET_errNoCurrentRecord );
    }

    Assert( pscb->crun > 0 || pscb->ispairMac > 0 );


    if ( pscb->crun )
    {
        err = ErrSORTIMergeNext( pscb, &psrec );
        while ( err >= 0 )
        {
            CallS( err );

            SrecToKeydataflags( psrec, pfucb );

            err = ErrSORTIMergeNext( pscb, &psrec );
        }

        if ( JET_errNoCurrentRecord == err )
        {
            err = JET_errSuccess;
        }
        else
        {
            DIRAfterLast( pfucb );
        }
    }


    else
    {
        pfucb->ispairCurr = pscb->ispairMac - 1;
        irec = pscb->rgspair[pfucb->ispairCurr].irec;
        psrec = PsrecFromPbIrec( pscb->rgbRec, irec );

        SrecToKeydataflags( psrec, pfucb );
    }

    return err;
}


ERR ErrSORTNext( FUCB *pfucb )
{
    SCB     *pscb   = pfucb->u.pscb;
    SREC    *psrec  = 0;
    LONG    irec;
    ERR     err = JET_errSuccess;


    Assert( !FSCBInsert( pscb ) );


    if ( pscb->crun )
    {
        Call( ErrSORTIMergeNext( pscb, &psrec ) );
    }
    else
    {
        Assert( pfucb->ispairCurr <= pscb->ispairMac );


        if ( pfucb->ispairCurr > pscb->ispairMac )
        {
            AssertSz( fFalse, "pfucb->ispairCurr set after the end of the sort" );
            Error( ErrERRCheck( JET_errInternalError ) );
        }
        
        if ( pfucb->ispairCurr > ( pfucb->ispairCurr + 1 ) )
        {
            AssertSz( fFalse, "pfucb->ispairCurr overflow" );
            Error( ErrERRCheck( JET_errInternalError ) );
        }
        
        if ( ++pfucb->ispairCurr < pscb->ispairMac )
        {
            irec = pscb->rgspair[pfucb->ispairCurr].irec;
            psrec = PsrecFromPbIrec( pscb->rgbRec, irec );
        }


        else
        {
            pfucb->ispairCurr = pscb->ispairMac;
            Call( ErrERRCheck( JET_errNoCurrentRecord ) );
        }
    }

    SrecToKeydataflags( psrec, pfucb );


    if ( FFUCBLimstat( pfucb ) && FFUCBUpper( pfucb ) )
        CallR( ErrSORTCheckIndexRange( pfucb ) );

    return JET_errSuccess;

HandleError:
    Assert( err < 0 );
    if ( JET_errNoCurrentRecord == err )
        DIRAfterLast( pfucb );

    return err;
}



ERR ErrSORTPrev( FUCB *pfucb )
{
    SCB     * const pscb    = pfucb->u.pscb;
    ERR     err             = JET_errSuccess;


    Assert( !pscb->crun );


    Assert( !FSCBInsert( pscb ) );

    Assert( pfucb->ispairCurr >= -1L );
    if ( pfucb->ispairCurr <= 0L )
    {
        SORTBeforeFirst( pfucb );
        return ErrERRCheck( JET_errNoCurrentRecord );
    }

    pfucb->ispairCurr--;
    Assert( pfucb->ispairCurr >= 0L );

    const LONG  irec = pscb->rgspair[pfucb->ispairCurr].irec;
    const SREC  *psrec = PsrecFromPbIrec( pscb->rgbRec, irec );

    SrecToKeydataflags( psrec, pfucb );


    if ( FFUCBLimstat( pfucb ) && FFUCBUpper( pfucb ) )
        CallR( ErrSORTCheckIndexRange( pfucb ) );

    return JET_errSuccess;
}



ERR ErrSORTSeek( FUCB * const pfucb, const KEY& key )
{
    const SCB * const pscb  = pfucb->u.pscb;


    Assert( FFUCBSort( pfucb ) );
    Assert( !pscb->crun );


    Assert( !FSCBInsert( pscb ) );


    Assert( ( pfucb->u.pscb->grbit & JET_bitTTScrollable ) ||
        ( pfucb->u.pscb->grbit & JET_bitTTIndexed ) );


    if ( !pscb->cRecords )
    {
        return ErrERRCheck( JET_errRecordNotFound );
    }

    Assert( key.Cb() <= cbKeyMostMost );


    INT lastCompare;
    const INT ispair = IspairSORTISeekByKey(    pscb->rgbRec,
                                                pscb->rgspair,
                                                pscb->ispairMac - 1,
                                                key,
                                                &lastCompare );

    ERR err = JET_errSuccess;
    if ( ispair < 0 )
    {
        pfucb->ispairCurr = pscb->ispairMac - 1;
        err = ErrERRCheck( wrnNDFoundLess );
    }
    else if ( 0 == lastCompare )
    {
        pfucb->ispairCurr = ispair;
        err = JET_errSuccess;
    }
    else
    {
        Assert( lastCompare > 0 );
        pfucb->ispairCurr = ispair;
        err = ErrERRCheck( wrnNDFoundGreater );
    }


    const INT irec = pscb->rgspair[pfucb->ispairCurr].irec;
    const SREC * const psrec = PsrecFromPbIrec( pscb->rgbRec, irec );
    SrecToKeydataflags( psrec, pfucb );

    return err;
}


VOID SORTICloseRun( PIB * const ppib, SCB * const pscb )
{
    Assert( pscb->fcb.WRefCount() == 1 );
    Assert( pscb->fcb.PgnoFDP() != pgnoNull );
    Assert( !pscb->fcb.FInList() );

    
    if ( pscb->crunMerge )
        SORTIMergeEnd( pscb );

    
    SORTIOptTreeTerm( pscb );

    
    if ( pscb->bflOut.pv != NULL )
    {
        BFWriteUnlatch( &pscb->bflOut );
        pscb->bflOut.pv         = NULL;
        pscb->bflOut.dwContext  = NULL;
    }

    Assert( pscb->fcb.PrceOldest() == prceNil );
    Assert( pscb->fcb.PrceNewest() == prceNil );

    Assert( !pscb->fcb.FDeleteCommitted() );
    pscb->fcb.Lock();
    pscb->fcb.SetDeleteCommitted();
    pscb->fcb.Unlock();
    SCBDeleteHashTable( pscb );

    Assert( FFMPIsTempDB( pscb->fcb.Ifmp() ) );
    (VOID)ErrSPFreeFDP( ppib, &pscb->fcb, pgnoSystemRoot );

    pscb->fcb.ResetSortSpace();
}



VOID SORTClose( FUCB *pfucb )
{
    SCB * const pscb    = pfucb->u.pscb;

    Assert( FFMPIsTempDB( pfucb->ifmp ) );

    Assert( !pscb->fcb.FInList() );
    Assert( pscb->fcb.WRefCount() >= 1 );
    if ( pscb->fcb.WRefCount() == 1 )
    {

        if ( pscb->crun > 0 )
        {
            SORTICloseRun( pfucb->ppib, pscb );
        }


        pfucb->u.pfcb->Unlink( pfucb, fTrue );


        FUCBClose( pfucb );

        
        Assert( pscb->fcb.WRefCount() == 0 );
        SORTClosePscb( pscb );
    }
    else
    {

        pfucb->u.pfcb->Unlink( pfucb, fTrue );


        FUCBClose( pfucb );
    }
}



VOID SORTClosePscb( SCB *pscb )
{
    Assert( FFMPIsTempDB( pscb->fcb.Ifmp() ) );
    Assert( pscb->fcb.PgnoFDP() == pgnoNull );
    if ( pscb->fcb.Pidb() != pidbNil )
    {
        Assert( 0 == pscb->fcb.Pidb()->ItagIndexName() );

        pscb->fcb.ReleasePidb();
    }
    Assert( pscb->fcb.Ptdb() == ptdbNil || pscb->fcb.Ptdb()->PfcbLV() == pfcbNil );
    delete pscb->fcb.Ptdb();

    delete pscb;
}



ERR ErrSORTCheckIndexRange( FUCB *pfucb )
{
    SCB     * const pscb = pfucb->u.pscb;


    ERR err =  ErrFUCBCheckIndexRange( pfucb, pfucb->kdfCurr.key );
    Assert( JET_errSuccess == err || JET_errNoCurrentRecord == err );


    if ( err == JET_errNoCurrentRecord )
    {
        DIRUp( pfucb );

        if ( FFUCBUpper( pfucb ) )
        {
            pfucb->ispairCurr = pscb->ispairMac;
            DIRAfterLast( pfucb );
        }


        else
        {
            SORTBeforeFirst( pfucb );
        }
    }


    Assert( pfucb->locLogical == locBeforeFirst ||
            pfucb->locLogical == locOnCurBM ||
            pfucb->locLogical == locAfterLast );
    Assert( pfucb->ispairCurr >= -1 );
    Assert( pfucb->ispairCurr <= pscb->ispairMac );

    return err;
}



LOCAL LONG IspairSORTISeekByKey(
    const BYTE * const rgbRec,
    const SPAIR * const rgspair,
    const INT ispairMac,
    const KEY& keySeek,
    INT * const plastCompare )
{
    Assert( rgbRec );
    Assert( rgspair );
    Assert( plastCompare );

    INT ispairFirst = 0;
    INT ispairLast  = ispairMac;

    INT compare = -1;

    INT ispair = -1;
    while( ispairFirst <= ispairLast )
    {

        const INT ispairMid     = (ispairFirst + ispairLast) / 2;


        const INT irec              = rgspair[ispairMid].irec;
        const SREC * const psrec    = PsrecFromPbIrec( rgbRec, irec );

        KEY keyCurr;
        keyCurr.prefix.Nullify();
        keyCurr.suffix.SetPv( PbSRECKeyPsrec( psrec ) );
        keyCurr.suffix.SetCb( CbSRECKeyPsrec( psrec ) );
        compare = CmpKey( keyCurr, keySeek);

        if ( compare < 0 )
        {
            ispairFirst = ispairMid + 1;
        }
        else if ( 0 == compare )
        {
            ispair = ispairMid;
            break;
        }
        else
        {
            Assert( compare > 0 );
            ispairLast = ispairMid;
            if ( ispairLast == ispairFirst )
            {
                ispair = ispairMid;
                break;
            }
        }
    }

    *plastCompare = compare;

    return ispair;
}



INLINE INT ISORTICmpKey( const SREC * psrec1, const SREC * psrec2 )
{
    const BYTE*     stKey1  = StSRECKeyPsrec( psrec1 );
    const USHORT    cbKey1  = *( UnalignedLittleEndian< USHORT > *)stKey1;
    const BYTE*     stKey2  = StSRECKeyPsrec( psrec2 );
    const USHORT    cbKey2  = *( UnalignedLittleEndian< USHORT > *)stKey2;

    Assert( cbKey1 <= cbKeyMostMost );
    Assert( cbKey2 <= cbKeyMostMost );

    const INT w = memcmp(   stKey1 + cbKeyCount,
                            stKey2 + cbKeyCount,
                            min( cbKey1, cbKey2 ) );

    return w ? w : cbKey1 - cbKey2;
}


INLINE INT ISORTICmpKeyData( const SREC * psrec1, const SREC * psrec2 )
{

    INT cmp = ISORTICmpKey( psrec1, psrec2 );
    if ( 0 == cmp )
    {
        const LONG  cbData1 = CbSRECDataPsrec( psrec1 );
        const LONG  cbData2 = CbSRECDataPsrec( psrec2 );
        cmp = memcmp(   PbSRECDataPsrec( psrec1 ),
                        PbSRECDataPsrec( psrec2 ),
                        min( cbData1, cbData2 ) );
        if ( 0 == cmp )
        {
            cmp = cbData1 - cbData2;
        }
    }

    return cmp;
}


INLINE BOOL FSORTIDuplicate( SCB* const pscb, const SREC * psrec1, const SREC * psrec2 )
{
    if ( FSCBRemoveDuplicateKey( pscb ) )
    {
        return !ISORTICmpKey( psrec1, psrec2 );
    }
    else if ( FSCBRemoveDuplicateKeyData( pscb ) )
    {
        return !ISORTICmpKeyData( psrec1, psrec2 );
    }
    else
    {
        return fFalse;
    }
}



LOCAL LONG CspairSORTIUnique( SCB* pscb, BYTE * rgbRec, __inout_ecount(ispairMac) SPAIR * rgspair, const LONG ispairMac )
{

    if (    !ispairMac ||
            !FSCBRemoveDuplicateKeyData( pscb ) && !FSCBRemoveDuplicateKey( pscb ) )
    {
        return ispairMac;
    }


    LONG    ispairDest;
    LONG    ispairSrc;
    for ( ispairDest = 0, ispairSrc = 1; ispairSrc < ispairMac; ispairSrc++ )
    {

        const LONG      irecDest    = rgspair[ispairDest].irec;
        SREC * const    psrecDest   = PsrecFromPbIrec( rgbRec, irecDest );
        const LONG      irecSrc     = rgspair[ispairSrc].irec;
        SREC * const    psrecSrc    = PsrecFromPbIrec( rgbRec, irecSrc );


        if ( !FSORTIDuplicate( pscb, psrecSrc, psrecDest ) )
        {
            rgspair[++ispairDest] = rgspair[ispairSrc];
        }
    }

    Assert( ispairDest + 1 <= ispairMac );

    return ispairDest + 1;
}


LOCAL ERR ErrSORTIOutputRun( SCB * pscb )
{
    ERR     err;
    RUNINFO runinfo;
    LONG    ispair;
    LONG    irec;
    SREC*   psrec       = NULL;
    SREC*   psrecLast;

    Assert( pscb->ispairMac );

    if ( pscb->fcb.PgnoFDP() == pgnoNull )
    {
        FUCB    * const pfucb   = pscb->fcb.Pfucb();
        PGNO    pgnoFDP         = pgnoNull;
        OBJID   objidFDP        = objidNil;

        const CPG cpgMin = (PGNO) ( ( pscb->cbData + cbFreeSPAGE - 1 ) / cbFreeSPAGE * crunFanInMax );
        CPG cpgReq = cpgMin;

        CallR( ErrSPGetExt( pfucb,
                pgnoSystemRoot,
                &cpgReq,
                cpgMin,
                &pgnoFDP,
                fSPNewFDP|fSPMultipleExtent|fSPUnversionedExtent,
                CPAGE::fPagePrimary,
                &objidFDP ) );

        Assert( pgnoNull != pgnoFDP );
        Assert( objidFDP > objidSystemRoot );
        Assert( !pscb->fcb.FSpaceInitialized() );
        pscb->fcb.SetSortSpace( pgnoFDP, objidFDP );
        SCBInsertHashTable( pscb );

        SORTIOptTreeInit( pscb );

        pscb->bflOut.pv         = NULL;
        pscb->bflOut.dwContext  = NULL;

        pscb->crunMerge = 0;
    }

    CallR( ErrSORTIRunStart( pscb, QWORD( pscb->cbData ), &runinfo ) );

    for ( ispair = 0; ispair < pscb->ispairMac; ispair++ )
    {
        psrecLast = psrec;

        irec = pscb->rgspair[ispair].irec;
        psrec = PsrecFromPbIrec( pscb->rgbRec, irec );

        if (    !psrecLast ||
                !FSORTIDuplicate( pscb, psrec, psrecLast ) )
        {
            CallJ( ErrSORTIRunInsert( pscb, &runinfo, psrec ), EndRun );
        }
    }

    SORTIRunEnd( pscb, &runinfo );
    CallJ( ErrSORTIOptTreeAddRun( pscb, &runinfo ), DeleteRun );

    pscb->ispairMac = 0;
    pscb->irecMac   = 0;
    pscb->crecBuf   = 0;
    pscb->cbData    = 0;

    return JET_errSuccess;

EndRun:
    SORTIRunEnd( pscb, &runinfo );
DeleteRun:
    SORTIRunDelete( pscb, &runinfo );
    return err;
}

#ifdef DEBUG
LOCAL INT IDBGICmp2PspairPspair( const SCB * pscb, const SPAIR * pspair1, const SPAIR * pspair2 )
{
    INT cmp;
    INT db;


    SREC * const psrec1 = PsrecFromPbIrec( pscb->rgbRec, pspair1->irec );
    SREC * const psrec2 = PsrecFromPbIrec( pscb->rgbRec, pspair2->irec );

    const LONG cbKey1   = CbSRECKeyPsrec( psrec1 );
    const LONG cbKey2   = CbSRECKeyPsrec( psrec2 );

    if ( cbKey1 > cbKeyPrefix )
    {
        db = cbKey1 - cbKey2;
        cmp = memcmp( PbSRECKeyPsrec( psrec1 ) + cbKeyPrefix,
                            PbSRECKeyPsrec( psrec2 ) + cbKeyPrefix,
                            ( db < 0 ? cbKey1 : cbKey2 ) - cbKeyPrefix );
        if ( 0 == cmp )
            cmp = db;
        if ( 0 != cmp )
            return cmp;
    }
    else
    {
        Assert( cbKey1 == cbKey2 );
        Assert( memcmp( PbSRECKeyPsrec( psrec1 ), PbSRECKeyPsrec( psrec2 ), cbKey1 ) == 0 );
    }


    const LONG cbData1 = CbSRECDataPsrec( psrec1 );
    const LONG cbData2 = CbSRECDataPsrec( psrec2 );

    db = cbData1 - cbData2;
    cmp = memcmp( PbSRECDataPsrec( psrec1 ),
                    PbSRECDataPsrec( psrec2 ),
                    db < 0 ? cbData1 : cbData2 );

    return ( 0 == cmp ? db : cmp );
}
#endif

INLINE INT ISORTICmpPspairPspair( const SCB * pscb, const SPAIR * pspair1, const SPAIR * pspair2 )
{
    const BYTE  *rgb1   = (BYTE *) pspair1;
    const BYTE  *rgb2   = (BYTE *) pspair2;



    Assert( cbKeyPrefix == 14 );
    Assert( OffsetOf( SPAIR, irec ) == 0 );

#ifdef _X86_

    if ( *( (DWORD *) ( rgb1 + 12 ) ) < *( (DWORD *) ( rgb2 + 12 ) ) )
        return -1;
    if ( *( (DWORD *) ( rgb1 + 12 ) ) > *( (DWORD *) ( rgb2 + 12 ) ) )
        return 1;

    if ( *( (DWORD *) ( rgb1 + 8 ) ) < *( (DWORD *) ( rgb2 + 8 ) ) )
        return -1;
    if ( *( (DWORD *) ( rgb1 + 8 ) ) > *( (DWORD *) ( rgb2 + 8 ) ) )
        return 1;

    if ( *( (DWORD *) ( rgb1 + 4 ) ) < *( (DWORD *) ( rgb2 + 4 ) ) )
        return -1;
    if ( *( (DWORD *) ( rgb1 + 4 ) ) > *( (DWORD *) ( rgb2 + 4 ) ) )
        return 1;

    if ( *( (USHORT *) ( rgb1 + 2 ) ) < *( (USHORT *) ( rgb2 + 2 ) ) )
        return -1;
    if ( *( (USHORT *) ( rgb1 + 2 ) ) > *( (USHORT *) ( rgb2 + 2 ) ) )
        return 1;

#else

    if ( *( (LittleEndian<QWORD> *) ( rgb1 + 8 ) ) < *( (LittleEndian<QWORD> *) ( rgb2 + 8 ) ) )
        return -1;
    if ( *( (LittleEndian<QWORD> *) ( rgb2 + 8 ) ) < *( (LittleEndian<QWORD> *) ( rgb1 + 8 ) ) )
        return 1;

    if (    ( *( (LittleEndian<QWORD> *) ( rgb1 + 0 ) ) & 0xFFFFFFFFFFFF0000 ) <
            ( *( (LittleEndian<QWORD> *) ( rgb2 + 0 ) ) & 0xFFFFFFFFFFFF0000 ) )
        return -1;
    if (    ( *( (LittleEndian<QWORD> *) ( rgb1 + 0 ) ) & 0xFFFFFFFFFFFF0000 ) >
            ( *( (LittleEndian<QWORD> *) ( rgb2 + 0 ) ) & 0xFFFFFFFFFFFF0000 ) )
        return 1;

#endif


    INT i = ISORTICmp2PspairPspair( pscb, pspair1, pspair2 );

#ifdef DEBUG
    INT j = IDBGICmp2PspairPspair( pscb, pspair1, pspair2 );

    Assert( ( i < 0 && j < 0 )
            || ( i == 0 && j == 0 )
            || ( i > 0 && j > 0 ) );
#endif

    return i;
}


LOCAL INT ISORTICmp2PspairPspair( const SCB * pscb, const SPAIR * pspair1, const SPAIR * pspair2 )
{

    SREC * const psrec1 = PsrecFromPbIrec( pscb->rgbRec, pspair1->irec );
    SREC * const psrec2 = PsrecFromPbIrec( pscb->rgbRec, pspair2->irec );


    const LONG cbKey1 = CbSRECKeyDataPsrec( psrec1 );
    const LONG cbKey2 = CbSRECKeyDataPsrec( psrec2 );

    INT w = min( cbKey1, cbKey2 ) - cbKeyPrefix;


    if ( w > 0 )
    {
        Assert( cbKey1 > cbKeyPrefix );
        Assert( cbKey2 > cbKeyPrefix );
        w = memcmp( PbSRECKeyPsrec( psrec1 ) + cbKeyPrefix,
                    PbSRECKeyPsrec( psrec2 ) + cbKeyPrefix,
                    w );
        if ( 0 == w )
        {
            w = cbKey1 - cbKey2;
        }
    }
    else
    {
        w = cbKey1 - cbKey2;
    }

    return w;
}



INLINE VOID SWAPPspair( SPAIR **ppspair1, SPAIR **ppspair2 )
{
    SPAIR *pspairT;

    pspairT = *ppspair1;
    *ppspair1 = *ppspair2;
    *ppspair2 = pspairT;
}



INLINE VOID SWAPSpair( SPAIR *pspair1, SPAIR *pspair2 )
{
    SPAIR spairT;

    spairT = *pspair1;
    *pspair1 = *pspair2;
    *pspair2 = spairT;
}


INLINE VOID SWAPPsrec( SREC **ppsrec1, SREC **ppsrec2 )
{
    SREC *psrecT;

    psrecT = *ppsrec1;
    *ppsrec1 = *ppsrec2;
    *ppsrec2 = psrecT;
}


INLINE VOID SWAPPmtnode( MTNODE **ppmtnode1, MTNODE **ppmtnode2 )
{
    MTNODE *pmtnodeT;

    pmtnodeT = *ppmtnode1;
    *ppmtnode1 = *ppmtnode2;
    *ppmtnode2 = pmtnodeT;
}



LOCAL VOID SORTIInsertionSort( SCB *pscb, SPAIR *pspairMinIn, SPAIR *pspairMaxIn )
{
    SPAIR   *pspairLast;
    SPAIR   *pspairFirst;
    SPAIR   *pspairKey = pscb->rgspair + cspairSortMax;


    for (   pspairFirst = pspairMinIn, pspairLast = pspairMinIn + 1;
            pspairLast < pspairMaxIn;
            pspairFirst = pspairLast++ )
        if ( ISORTICmpPspairPspair( pscb, pspairFirst, pspairLast ) > 0 )
        {

            *pspairKey = *pspairLast;


            *pspairLast = *pspairFirst;


            while ( --pspairFirst >= pspairMinIn &&
                    ( ISORTICmpPspairPspair( pscb, pspairFirst, pspairKey ) ) > 0 )
            {
                *( pspairFirst + 1 ) = *pspairFirst;
            }
            *( pspairFirst + 1 ) = *pspairKey;
        }
}



LOCAL VOID SORTIQuicksort( SCB * pscb, SPAIR *pspairMinIn, SPAIR *pspairMaxIn )
{
    struct _part
    {
        SPAIR   *pspairMin;
        SPAIR   *pspairMax;
    }   rgpart[cpartQSortMax];
    LONG    cpart       = 0;

    SPAIR   *pspairFirst;
    SPAIR   *pspairLast;


    SPAIR   *pspairMin  = pspairMinIn;
    SPAIR   *pspairMax  = pspairMaxIn;


    forever
    {

        if ( pspairMax - pspairMin < cspairQSortMin )
        {
            SORTIInsertionSort( pscb, pspairMin, pspairMax );


            if ( !cpart )
                break;


            pspairMin = rgpart[--cpart].pspairMin;
            pspairMax = rgpart[cpart].pspairMax;
            continue;
        }


        pspairFirst = pspairMin + ( ( pspairMax - pspairMin ) >> 1 );
        pspairLast  = pspairMax - 1;

        if ( ISORTICmpPspairPspair( pscb, pspairFirst, pspairMin ) > 0 )
            SWAPSpair( pspairFirst, pspairMin );
        if ( ISORTICmpPspairPspair( pscb, pspairFirst, pspairLast ) > 0 )
            SWAPSpair( pspairFirst, pspairLast );
        if ( ISORTICmpPspairPspair( pscb, pspairMin, pspairLast ) > 0 )
            SWAPSpair( pspairMin, pspairLast );


        pspairFirst = pspairMin + 1;
        pspairLast--;

        Assert( pspairFirst <= pspairLast );

        forever
        {

            while ( pspairFirst <= pspairLast &&
                    ISORTICmpPspairPspair( pscb, pspairFirst, pspairMin ) <= 0 )
                pspairFirst++;


            while ( pspairFirst <= pspairLast &&
                    ISORTICmpPspairPspair( pscb, pspairLast, pspairMin ) > 0 )
                pspairLast--;


            Assert( pspairFirst != pspairLast );

            if ( pspairFirst < pspairLast )
                SWAPSpair( pspairFirst++, pspairLast-- );


            else
                break;
        }


        if ( pspairLast != pspairMin )
            SWAPSpair( pspairMin, pspairLast );


        if ( pspairMax - pspairLast - 1 > pspairLast - pspairMin )
        {
            pspairFirst = pspairLast + 1;
            SWAPPspair( &pspairLast, &pspairMax );
        }
        else
        {
            pspairFirst = pspairMin;
            pspairMin   = pspairLast + 1;
        }


        if ( cpart < cpartQSortMax )
        {
            rgpart[cpart].pspairMin     = pspairFirst;
            rgpart[cpart++].pspairMax   = pspairLast;
        }
        else
            SORTIQuicksort( pscb, pspairFirst, pspairLast );
    }
}


LOCAL ERR ErrSORTIRunStart( SCB *pscb, QWORD cb, RUNINFO *pruninfo )
{
    ERR             err;
    const QWORD     cpgAlloc    = ( cb + cbFreeSPAGE - 1 ) / cbFreeSPAGE;

    if ( cpgAlloc > lMax )
    {
        CallR( ErrERRCheck( JET_errOutOfDatabaseSpace ) );
    }


    pruninfo->run       = runNull;
    pruninfo->cpg       = CPG( cpgAlloc );
    pruninfo->cbRun     = 0;
    pruninfo->crecRun   = 0;
    pruninfo->cpgUsed   = pruninfo->cpg;

    CallR( ErrSPGetExt(
                pscb->fcb.Pfucb(),
                pscb->fcb.PgnoFDP(),
                &pruninfo->cpg,
                pruninfo->cpgUsed,
                &pruninfo->run ) );
    Assert( pruninfo->cpg >= pruninfo->cpgUsed );


    pscb->pgnoNext          = pruninfo->run;
    pscb->bflOut.pv         = NULL;
    pscb->bflOut.dwContext  = NULL;
    pscb->pbOutMac          = NULL;
    pscb->pbOutMax          = NULL;

    return JET_errSuccess;
}



LOCAL ERR ErrSORTIRunInsert( SCB *pscb, RUNINFO* pruninfo, SREC *psrec )
{
    ERR         err;
    ULONG       cb;
    PGNO        pgnoNext;
    SPAGE_FIX   *pspage;
    LONG        cbToWrite;


    Assert( CbSRECSizePsrec( psrec ) > CbSRECSizeCbCb( 0, 0 ) &&
            (SIZE_T)CbSRECSizePsrec( psrec ) < cbFreeSPAGE );


    cb = (ULONG)min(pscb->pbOutMax - pscb->pbOutMac, (LONG)CbSRECSizePsrec( psrec ) );


    if ( cb )
    {
        UtilMemCpy( pscb->pbOutMac, psrec, cb );
        pscb->pbOutMac += cb;
    }


    if ( cb < (ULONG) CbSRECSizePsrec( psrec ) )
    {

        if ( pscb->bflOut.pv != NULL )
        {
            BFWriteUnlatch( &pscb->bflOut );
            pscb->bflOut.pv         = NULL;
            pscb->bflOut.dwContext  = NULL;
        }

        FUCB* pFucb = pscb->fcb.Pfucb( );

        PIBTraceContextScope tcRef = pFucb->ppib->InitTraceContextScope();
        tcRef->nParentObjectClass = tceNone;
        tcRef->iorReason.SetIort( iortSort );


        pgnoNext = pscb->pgnoNext++;

        CallR( ErrBFWriteLatchPage( &pscb->bflOut,
                                    pFucb->ifmp,
                                    pgnoNext,
                                    bflfNew,
                                    pFucb->ppib->BfpriPriority( pFucb->ifmp ),
                                    *tcRef ) );
        BFDirty( &pscb->bflOut, bfdfDirty, *tcRef );


        pspage = (SPAGE_FIX *) pscb->bflOut.pv;

        pspage->pgnoThisPage = pgnoNext;


        pscb->pbOutMac = PbDataStartPspage( pspage );
        pscb->pbOutMax = PbDataEndPspage( pspage );


        cbToWrite = CbSRECSizePsrec( psrec ) - cb;
        UtilMemCpy( pscb->pbOutMac, ( (BYTE *) psrec ) + cb, cbToWrite );
        pscb->pbOutMac += cbToWrite;
    }


    pruninfo->cbRun += CbSRECSizePsrec( psrec );
    pruninfo->crecRun++;
    return JET_errSuccess;
}



INLINE VOID SORTIRunEnd( SCB * pscb, RUNINFO* pruninfo )
{

    if ( pscb->bflOut.pv != NULL )
    {
        BFWriteUnlatch( &pscb->bflOut );
        pscb->bflOut.pv         = NULL;
        pscb->bflOut.dwContext  = NULL;
    }


    pruninfo->cpgUsed = CPG( ( pruninfo->cbRun + cbFreeSPAGE - 1 ) / cbFreeSPAGE );

    if ( pruninfo->cpg - pruninfo->cpgUsed > 0 )
    {
        FUCB * const    pfucbT      = pscb->fcb.Pfucb();
        Assert( !FFUCBSpace( pfucbT ) );

        const ERR       errFreeExt  = ErrSPFreeExt(
                                            pfucbT,
                                            pruninfo->run + pruninfo->cpgUsed,
                                            pruninfo->cpg - pruninfo->cpgUsed,
                                            "SortEnd" );
#ifdef DEBUG
        if ( !FSPExpectedError( errFreeExt ) )
        {
            CallS( errFreeExt );
        }
#endif
    }

    pruninfo->cpg = pruninfo->cpgUsed;
}



INLINE VOID SORTIRunDelete( SCB * pscb, const RUNINFO * pruninfo )
{

    if ( pruninfo->cpg )
    {
        FUCB * const    pfucbT      = pscb->fcb.Pfucb();
        Assert( !FFUCBSpace( pfucbT ) );

        const ERR       errFreeExt  = ErrSPFreeExt(
                                            pfucbT,
                                            pruninfo->run,
                                            pruninfo->cpg,
                                            "SortDelete" );
#ifdef DEBUG
        if ( !FSPExpectedError( errFreeExt ) )
        {
            CallS( errFreeExt );
        }
#endif
    }
}



LOCAL VOID  SORTIRunDeleteList( SCB *pscb, RUNLINK **pprunlink, LONG crun )
{
    LONG    irun;


    for ( irun = 0; *pprunlink != prunlinkNil && irun < crun; irun++ )
    {

        SORTIRunDelete( pscb, &( *pprunlink )->runinfo );


        RUNLINK * prunlinkT = *pprunlink;
        *pprunlink = ( *pprunlink )->prunlinkNext;


        RUNLINKReleasePrunlink( prunlinkT );
    }
}



LOCAL VOID  SORTIRunDeleteListMem( SCB *pscb, RUNLINK **pprunlink, LONG crun )
{
    LONG    irun;


    for ( irun = 0; *pprunlink != prunlinkNil && irun < crun; irun++ )
    {

        RUNLINK * prunlinkT = *pprunlink;
        *pprunlink = ( *pprunlink )->prunlinkNext;


        RUNLINKReleasePrunlink( prunlinkT );
    }
}



LOCAL ERR ErrSORTIRunOpen( SCB *pscb, RUNINFO *pruninfo, RCB **pprcb )
{
    ERR     err;
    RCB     *prcb   = prcbNil;
    LONG    ipbf;
    CPG     cpgRead;


    Alloc( prcb = PrcbRCBAlloc() );


    prcb->pscb = pscb;
    prcb->runinfo = *pruninfo;

    for ( ipbf = 0; ipbf < cpgClusterSize; ipbf++ )
    {
        prcb->rgbfl[ipbf].pv        = NULL;
        prcb->rgbfl[ipbf].dwContext = NULL;
    }

    prcb->ipbf          = cpgClusterSize;
    prcb->pbInMac       = NULL;
    prcb->pbInMax       = NULL;
    prcb->cbRemaining   = prcb->runinfo.cbRun;
    prcb->pvAssy        = NULL;


    cpgRead = min( prcb->runinfo.cpgUsed, 2 * cpgClusterSize );

    {
        TraceContextScope tc( iortSort );
        tc->nParentObjectClass = tceNone;
        FUCB * pfucbT = pscb->fcb.Pfucb();

        BFPrereadPageRange( pfucbT->ifmp,
            (PGNO)prcb->runinfo.run,
            cpgRead,
            bfprfDefault,
            pfucbT->ppib->BfpriPriority( pfucbT->ifmp ),
            *tc );
    }


    *pprcb = prcb;
    return JET_errSuccess;

HandleError:
    *pprcb = prcbNil;
    return err;
}



LOCAL ERR ErrSORTIRunNext( RCB * prcb, SREC **ppsrec )
{
    ERR     err;
    SCB     *pscb = prcb->pscb;
    SIZE_T  cbUnread;
    SHORT   cbRec;
    SPAGE_FIX   *pspage;
    LONG    ipbf;
    PGNO    pgnoNext;
    CPG     cpgRead;
    SIZE_T  cbToRead;


    if ( pscb->pvAssyLast != NULL )
    {
        BFFree( pscb->pvAssyLast );
    }
    pscb->pvAssyLast = prcb->pvAssy;
    prcb->pvAssy = NULL;


    if ( pscb->bflLast.pv != NULL )
    {
        CLockDeadlockDetectionInfo::DisableOwnershipTracking();
        BFRenouncePage( &pscb->bflLast, fTrue );
        CLockDeadlockDetectionInfo::EnableOwnershipTracking();
        pscb->bflLast.pv        = NULL;
        pscb->bflLast.dwContext = NULL;
    }


    if ( !prcb->cbRemaining )
    {

        if ( prcb->rgbfl[prcb->ipbf].pv != NULL )
        {
            pscb->bflLast.pv        = prcb->rgbfl[prcb->ipbf].pv;
            pscb->bflLast.dwContext = prcb->rgbfl[prcb->ipbf].dwContext;
            prcb->rgbfl[prcb->ipbf].pv          = NULL;
            prcb->rgbfl[prcb->ipbf].dwContext   = NULL;
        }


        Error( ErrERRCheck( JET_errNoCurrentRecord ) );
    }


    cbUnread = prcb->pbInMax - prcb->pbInMac;


    if ( cbUnread )
    {

        if (    cbUnread > cbSRECReadMin &&
                CbSRECSizePsrec( (SREC *) prcb->pbInMac ) <= (LONG)cbUnread )
        {
            cbRec = (SHORT) CbSRECSizePsrec( (SREC *) prcb->pbInMac );
            *ppsrec = (SREC *) prcb->pbInMac;
            prcb->pbInMac += cbRec;
            prcb->cbRemaining -= cbRec;
            return JET_errSuccess;
        }


        BFAlloc( bfasTemporary, &prcb->pvAssy );


        UtilMemCpy( prcb->pvAssy, prcb->pbInMac, cbUnread );
        prcb->cbRemaining -= cbUnread;
    }


    if ( prcb->ipbf < cpgClusterSize )
    {

        pgnoNext = ( ( SPAGE_FIX * )prcb->rgbfl[prcb->ipbf].pv )->pgnoThisPage + 1;


        pscb->bflLast.pv        = prcb->rgbfl[prcb->ipbf].pv;
        pscb->bflLast.dwContext = prcb->rgbfl[prcb->ipbf].dwContext;
        prcb->rgbfl[prcb->ipbf].pv          = NULL;
        prcb->rgbfl[prcb->ipbf].dwContext   = NULL;
    }
    else
    {

        pgnoNext = (PGNO) prcb->runinfo.run;
    }


    if ( ++prcb->ipbf < cpgClusterSize )
    {

        Assert( prcb->rgbfl[prcb->ipbf].pv != NULL );
        Assert( prcb->rgbfl[prcb->ipbf].dwContext != NULL );


        pspage = (SPAGE_FIX *) prcb->rgbfl[prcb->ipbf].pv;
        prcb->pbInMac = PbDataStartPspage( pspage );
        prcb->pbInMax = PbDataEndPspage( pspage );
    }
    else
    {

        cpgRead = min(  (LONG) ( prcb->runinfo.run + prcb->runinfo.cpgUsed - pgnoNext ),
                        cpgClusterSize );
        Assert( cpgRead > 0 );

        for ( ipbf = 0; ipbf < cpgRead; ipbf++ )
            Call( ErrSORTIRunReadPage( prcb, pgnoNext + ipbf, ipbf ) );


        prcb->ipbf      = 0;
        pspage          = (SPAGE_FIX *) prcb->rgbfl[prcb->ipbf].pv;
        prcb->pbInMac   = PbDataStartPspage( pspage );
        prcb->pbInMax   = PbDataEndPspage( pspage );


        pgnoNext += cpgClusterSize;
        cpgRead = min(  (LONG) ( prcb->runinfo.run + prcb->runinfo.cpgUsed - pgnoNext ),
                        cpgClusterSize );
        if ( cpgRead > 0 )
        {
            Assert( pgnoNext >= prcb->runinfo.run );
            Assert( pgnoNext + cpgRead - 1 <= prcb->runinfo.run + prcb->runinfo.cpgUsed - 1 );
            TraceContextScope tc;
            tc->nParentObjectClass = tceNone;

            FUCB *pfucbT = pscb->fcb.Pfucb();
            BFPrereadPageRange( pfucbT->ifmp,
                                pgnoNext,
                                cpgRead,
                                bfprfDefault,
                                pfucbT->ppib->BfpriPriority( pfucbT->ifmp ),
                                *tc );
        }
    }


    if ( !cbUnread )
    {
        cbRec = (SHORT) CbSRECSizePsrec( (SREC *) prcb->pbInMac );
        Assert( cbRec > CbSRECSizeCbCb( 0, 0 ) );
        Assert( (SIZE_T)cbRec < cbFreeSPAGE );
        *ppsrec = (SREC *) prcb->pbInMac;
        prcb->pbInMac += cbRec;
        prcb->cbRemaining -= cbRec;
        return JET_errSuccess;
    }


    if ( cbUnread < cbSRECReadMin )
        UtilMemCpy( ( (BYTE *) prcb->pvAssy ) + cbUnread,
                prcb->pbInMac,
                cbSRECReadMin - cbUnread );


    cbToRead = CbSRECSizePsrec( (SREC *) prcb->pvAssy ) - cbUnread;
    UtilMemCpy( ( (BYTE *) prcb->pvAssy ) + cbUnread, prcb->pbInMac, cbToRead );
    prcb->pbInMac += cbToRead;
    prcb->cbRemaining -= cbToRead;


    *ppsrec = (SREC *) prcb->pvAssy;
    return JET_errSuccess;

HandleError:
    for ( ipbf = 0; ipbf < cpgClusterSize; ipbf++ )
        if ( prcb->rgbfl[ipbf].pv != NULL )
        {
            CLockDeadlockDetectionInfo::DisableOwnershipTracking();
            BFRenouncePage( &prcb->rgbfl[ipbf], fTrue );
            CLockDeadlockDetectionInfo::EnableOwnershipTracking();
            prcb->rgbfl[ipbf].pv        = NULL;
            prcb->rgbfl[ipbf].dwContext = NULL;
        }
    *ppsrec = NULL;
    return err;
}



LOCAL VOID SORTIRunClose( RCB *prcb )
{
    LONG    ipbf;


    if ( prcb->pvAssy != NULL )
    {
        BFFree( prcb->pvAssy );
    }


    for ( ipbf = 0; ipbf < cpgClusterSize; ipbf++ )
        if ( prcb->rgbfl[ipbf].pv != NULL )
        {
            CLockDeadlockDetectionInfo::DisableOwnershipTracking();
            BFRenouncePage( &prcb->rgbfl[ipbf], fTrue );
            CLockDeadlockDetectionInfo::EnableOwnershipTracking();
            prcb->rgbfl[ipbf].pv        = NULL;
            prcb->rgbfl[ipbf].dwContext = NULL;
        }


    RCBReleasePrcb( prcb );
}



INLINE ERR ErrSORTIRunReadPage( RCB *prcb, PGNO pgno, LONG ipbf )
{
    ERR     err;


    Assert( pgno >= prcb->runinfo.run );
    Assert( pgno < prcb->runinfo.run + prcb->runinfo.cpgUsed );


    FUCB* pFucb = prcb->pscb->fcb.Pfucb();
    
    PIBTraceContextScope tcScope = pFucb->ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = tceNone;
    tcScope->iorReason.SetIort( iortSort );

    CLockDeadlockDetectionInfo::DisableOwnershipTracking();
    err = ErrBFReadLatchPage(   &prcb->rgbfl[ipbf],
                                pFucb->ifmp,
                                pgno,
                                bflfNoTouch,
                                pFucb->ppib->BfpriPriority( pFucb->ifmp ),
                                *tcScope );
    CLockDeadlockDetectionInfo::EnableOwnershipTracking();

    return err;
}



LOCAL ERR ErrSORTIMergeToRun( SCB *pscb, RUNLINK *prunlinkSrc, RUNLINK **pprunlinkDest )
{
    ERR     err = JET_errSuccess;
    LONG    irun;
    QWORD   cbRun;
    RUNLINK *prunlink = prunlinkNil;
    SREC    *psrec;


    CallR( ErrSORTIMergeStart( pscb, prunlinkSrc ) );


    for ( cbRun = 0, irun = 0; irun < pscb->crunMerge; irun++ )
    {
        cbRun += pscb->rgmtnode[irun].prcb->runinfo.cbRun;
    }


    prunlink = PrunlinkRUNLINKAlloc();
    if ( NULL == prunlink )
    {
        err = ErrERRCheck( JET_errOutOfMemory );
        goto EndMerge;
    }

    CallJ( ErrSORTIRunStart( pscb, cbRun, &prunlink->runinfo ), FreeRUNLINK );


    while ( ( err = ErrSORTIMergeNext( pscb, &psrec ) ) >= 0 )
        CallJ( ErrSORTIRunInsert( pscb, &prunlink->runinfo, psrec ), DeleteRun );

    if ( err < 0 && err != JET_errNoCurrentRecord )
        goto DeleteRun;

    SORTIRunEnd( pscb, &prunlink->runinfo );
    SORTIMergeEnd( pscb );


    prunlink->prunlinkNext = *pprunlinkDest;
    *pprunlinkDest = prunlink;

    return JET_errSuccess;

DeleteRun:
    SORTIRunEnd( pscb, &prunlink->runinfo );
    SORTIRunDelete( pscb, &prunlink->runinfo );
FreeRUNLINK:
    RUNLINKReleasePrunlink( prunlink );
EndMerge:
    SORTIMergeEnd( pscb );
    return err;
}



LOCAL ERR ErrSORTIMergeStart( SCB *pscb, RUNLINK *prunlinkSrc )
{
    ERR     err;
    RUNLINK *prunlink;
    LONG    crun;
    LONG    irun;
    MTNODE  *pmtnode;

    
    if ( PinstFromIfmp( pscb->fcb.Ifmp() )->m_fTermInProgress )
        return ErrERRCheck( JET_errTermInProgress );

    
    prunlink = prunlinkSrc;
    crun = 1;
    while ( prunlink->prunlinkNext != prunlinkNil )
    {
        prunlink = prunlink->prunlinkNext;
        crun++;
    }

    
    Assert( crun > 1 );

    
    pscb->crunMerge             = crun;
    pscb->bflLast.pv            = NULL;
    pscb->bflLast.dwContext     = NULL;
    pscb->pvAssyLast            = NULL;

    OSTrace( JET_tracetagSortPerf, OSFormat( "MERGE:  %ld runs -(details to follow)", crun ) );

    
    prunlink = prunlinkSrc;
    for ( irun = 0; irun < crun; irun++ )
    {

        pmtnode = pscb->rgmtnode + irun;
        Call( ErrSORTIRunOpen( pscb, &prunlink->runinfo, &pmtnode->prcb ) );
        pmtnode->pmtnodeExtUp = pscb->rgmtnode + ( irun + crun ) / 2;


        pmtnode->psrec = psrecNegInf;
        pmtnode->pmtnodeSrc = pmtnode;
        pmtnode->pmtnodeIntUp = pscb->rgmtnode + irun / 2;

        OSTrace( JET_tracetagSortPerf, OSFormat( "  Run:  %ld(%ld)", pmtnode->prcb->runinfo.run, pmtnode->prcb->runinfo.cpgUsed ) );


        prunlink = prunlink->prunlinkNext;
    }

    return JET_errSuccess;

HandleError:
    pscb->crunMerge = 0;
    for ( irun--; irun >= 0; irun-- )
        SORTIRunClose( pscb->rgmtnode[irun].prcb );
    return err;
}



LOCAL ERR ErrSORTIMergeFirst( SCB *pscb, SREC **ppsrec )
{
    ERR     err;


    while ( pscb->rgmtnode[0].psrec == psrecNegInf )
        Call( ErrSORTIMergeNextChamp( pscb, ppsrec ) );


    *ppsrec = pscb->rgmtnode[0].psrec;

    return JET_errSuccess;

HandleError:
    Assert( err != JET_errNoCurrentRecord );
    *ppsrec = NULL;
    return err;
}



LOCAL ERR ErrSORTIMergeNext( SCB *pscb, SREC **ppsrec )
{
    ERR     err;
    SREC    *psrecLast;


    if ( pscb->rgmtnode[0].psrec == psrecNegInf )
        return ErrSORTIMergeFirst( pscb, ppsrec );


    do  {
        psrecLast = pscb->rgmtnode[0].psrec;
        CallR( ErrSORTIMergeNextChamp( pscb, ppsrec ) );
    }
    while ( FSORTIDuplicate( pscb, *ppsrec, psrecLast ) );

    return JET_errSuccess;
}



LOCAL VOID SORTIMergeEnd( SCB *pscb )
{
    LONG    irun;


    if ( pscb->bflLast.pv != NULL )
    {
        CLockDeadlockDetectionInfo::DisableOwnershipTracking();
        BFRenouncePage( &pscb->bflLast, fTrue );
        CLockDeadlockDetectionInfo::EnableOwnershipTracking();
        pscb->bflLast.pv        = NULL;
        pscb->bflLast.dwContext = NULL;
    }
    if ( pscb->pvAssyLast != NULL )
    {
        BFFree( pscb->pvAssyLast );
        pscb->pvAssyLast = NULL;
    }


    for ( irun = 0; irun < pscb->crunMerge; irun++ )
        SORTIRunClose( pscb->rgmtnode[irun].prcb );
    pscb->crunMerge = 0;
}



LOCAL ERR ErrSORTIMergeNextChamp( SCB *pscb, SREC **ppsrec )
{
    ERR     err;
    MTNODE  *pmtnodeChamp;
    MTNODE  *pmtnodeLoser;


    pmtnodeChamp = pscb->rgmtnode + 0;
    pmtnodeLoser = pmtnodeChamp->pmtnodeSrc;


    *ppsrec = NULL;
    err = ErrSORTIRunNext( pmtnodeLoser->prcb, &pmtnodeChamp->psrec );
    if ( err < 0 && err != JET_errNoCurrentRecord )
        return err;


    pmtnodeLoser = pmtnodeLoser->pmtnodeExtUp;


    do  {

        if ( pmtnodeLoser->psrec == psrecInf || pmtnodeChamp->psrec == psrecNegInf )
            continue;


        if (    pmtnodeChamp->psrec == psrecInf ||
                pmtnodeLoser->psrec == psrecNegInf ||
                ISORTICmpKeyData( pmtnodeLoser->psrec, pmtnodeChamp->psrec ) < 0 )
        {
            SWAPPsrec( &pmtnodeLoser->psrec, &pmtnodeChamp->psrec );
            SWAPPmtnode( &pmtnodeLoser->pmtnodeSrc, &pmtnodeChamp->pmtnodeSrc );
        }
    }
    while ( ( pmtnodeLoser = pmtnodeLoser->pmtnodeIntUp ) != pmtnodeChamp );


    if ( ( *ppsrec = pmtnodeChamp->psrec ) == NULL )
        return ErrERRCheck( JET_errNoCurrentRecord );

    return JET_errSuccess;
}



INLINE VOID SORTIOptTreeInit( SCB *pscb )
{

    pscb->runlist.prunlinkHead      = prunlinkNil;
    pscb->runlist.crun              = 0;
}



LOCAL ERR ErrSORTIOptTreeAddRun( SCB *pscb, RUNINFO *pruninfo )
{

    RUNLINK * const prunlink    = PrunlinkRUNLINKAlloc();
    if ( prunlinkNil == prunlink )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    prunlink->runinfo = *pruninfo;


    prunlink->prunlinkNext = pscb->runlist.prunlinkHead;
    pscb->runlist.prunlinkHead = prunlink;
    pscb->runlist.crun++;
    pscb->crun++;

    return JET_errSuccess;
}



LOCAL ERR ErrSORTIOptTreeMerge( SCB *pscb )
{
    ERR     err;
    OTNODE  *potnode = potnodeNil;


    if ( pscb->runlist.crun <= crunFanInMax )
        return JET_errSuccess;


    CallR( ErrSORTIOptTreeBuild( pscb, &potnode ) );


    Call( ErrSORTIOptTreeMergeDF( pscb, potnode, NULL ) );


    Assert( pscb->runlist.crun == 0 );
    Assert( pscb->runlist.prunlinkHead == prunlinkNil );
    Assert( potnode->runlist.crun == crunFanInMax );
    Assert( potnode->runlist.prunlinkHead != prunlinkNil );
    pscb->runlist = potnode->runlist;


    OTNODEReleasePotnode( potnode );
    return JET_errSuccess;

HandleError:
    if ( potnode != potnodeNil )
    {
        SORTIOptTreeFree( pscb, potnode );
        OTNODEReleasePotnode( potnode );
    }
    return err;
}



INLINE VOID SORTIOptTreeTerm( SCB *pscb )
{

    SORTIRunDeleteListMem( pscb, &pscb->runlist.prunlinkHead, crunAll );
}



LOCAL ERR ErrSORTIOptTreeBuild( SCB *pscb, OTNODE **ppotnode )
{
    ERR     err;
    OTNODE  *potnodeAlloc   = potnodeNil;
    OTNODE  *potnodeT;
    OTNODE  *potnodeLast2;
    LONG    crunLast2;
    OTNODE  *potnodeLast;
    LONG    crunLast;
    OTNODE  *potnodeThis;
    LONG    crunThis;
    LONG    crunFanInFirst;
    OTNODE  *potnodeFirst;
    LONG    ipotnode;
    LONG    irun;


    potnodeLast2    = potnodeNil;
    crunLast2       = 0;
    potnodeLast     = potnodeLevel0;
    crunLast        = pscb->crun;
    potnodeThis     = potnodeNil;
    crunThis        = 0;


    do  {


        if ( crunLast2 + crunLast <= crunFanInMax )
            crunFanInFirst = crunLast2 + crunLast;
        else
            crunFanInFirst = 2 + ( crunLast2 + crunLast - crunFanInMax - 1 ) % ( crunFanInMax - 1 );
        Assert( potnodeLast == potnodeLevel0 || crunFanInFirst == crunFanInMax );


        Alloc( potnodeT = PotnodeOTNODEAlloc() );
        memset( potnodeT, 0, sizeof( OTNODE ) );
        potnodeT->potnodeAllocNext = potnodeAlloc;
        potnodeAlloc = potnodeT;
        ipotnode = 0;


        Assert( crunLast2 < crunFanInMax );

        if ( potnodeLast2 == potnodeLevel0 )
        {
            Assert( potnodeT->runlist.crun == 0 );
            potnodeT->runlist.crun = crunLast2;
        }
        else
        {
            while ( potnodeLast2 != potnodeNil )
            {
                Assert( ipotnode < crunFanInMax );
                if ( ipotnode >= _countof( potnodeT->rgpotnode ) )
                {
                    AssertSz( false, "ipotnode should never overflow fixed size array" );
                    Error( ErrERRCheck( JET_errInternalError ) );
                }
                potnodeT->rgpotnode[ipotnode++] = potnodeLast2;
                potnodeLast2 = potnodeLast2->potnodeLevelNext;
            }
        }
        crunFanInFirst -= crunLast2;
        crunLast2 = 0;


        if ( potnodeLast == potnodeLevel0 )
        {
            Assert( potnodeT->runlist.crun == 0 );
            potnodeT->runlist.crun = crunFanInFirst;
        }
        else
        {
            for ( irun = 0; irun < crunFanInFirst; irun++ )
            {
                Assert( ipotnode < crunFanInMax );
                if ( ipotnode >= _countof( potnodeT->rgpotnode ) )
                {
                    AssertSz( false, "ipotnode should never overflow fixed size array" );
                    Error( ErrERRCheck( JET_errInternalError ) );
                }
                potnodeT->rgpotnode[ipotnode++] = potnodeLast;
                potnodeLast = potnodeLast->potnodeLevelNext;
            }
        }
        crunLast -= crunFanInFirst;


        potnodeFirst = potnodeT;


        while ( crunLast >= crunFanInMax )
        {

            Alloc( potnodeT = PotnodeOTNODEAlloc() );
            memset( potnodeT, 0, sizeof( OTNODE ) );
            potnodeT->potnodeAllocNext = potnodeAlloc;
            potnodeAlloc = potnodeT;
            ipotnode = 0;


            if ( potnodeLast == potnodeLevel0 )
            {
                Assert( potnodeT->runlist.crun == 0 );
                potnodeT->runlist.crun = crunFanInMax;
            }
            else
            {
                for ( irun = 0; irun < crunFanInMax; irun++ )
                {
                    Assert( ipotnode < crunFanInMax );
                    if ( ipotnode >= _countof( potnodeT->rgpotnode ) )
                    {
                        AssertSz( false, "ipotnode should never overflow fixed size array" );
                        Error( ErrERRCheck( JET_errInternalError ) );
                    }
                    potnodeT->rgpotnode[ipotnode++] = potnodeLast;
                    potnodeLast = potnodeLast->potnodeLevelNext;
                }
            }
            crunLast -= crunFanInMax;


            potnodeT->potnodeLevelNext = potnodeThis;
            potnodeThis = potnodeT;
            crunThis++;
        }


        potnodeFirst->potnodeLevelNext = potnodeThis;
        potnodeThis = potnodeFirst;
        crunThis++;


        Assert( potnodeLast2 == potnodeNil || potnodeLast2 == potnodeLevel0 );
        Assert( crunLast2 == 0 );

        potnodeLast2    = potnodeLast;
        crunLast2       = crunLast;
        potnodeLast     = potnodeThis;
        crunLast        = crunThis;
        potnodeThis     = potnodeNil;
        crunThis        = 0;
    }
    while ( crunLast2 + crunLast > 1 );


    Assert( potnodeLast2 == potnodeNil || potnodeLast2 == potnodeLevel0 );
    Assert( crunLast2 == 0 );
    Assert( potnodeLast != potnodeNil
            && potnodeLast->potnodeLevelNext == potnodeNil );
    Assert( crunLast == 1 );


    *ppotnode = potnodeLast;
    return JET_errSuccess;

HandleError:
    while ( potnodeAlloc != potnodeNil )
    {
        SORTIRunDeleteListMem( pscb, &potnodeAlloc->runlist.prunlinkHead, crunAll );
        potnodeT = potnodeAlloc->potnodeAllocNext;
        OTNODEReleasePotnode( potnodeAlloc );
        potnodeAlloc = potnodeT;
    }
    *ppotnode = potnodeNil;
    return err;
}


LOCAL ERR ErrSORTIOptTreeMergeDF( SCB *pscb, OTNODE *potnode, RUNLINK **pprunlink )
{
    ERR     err;
    LONG    crunPhantom = 0;
    LONG    ipotnode;
    LONG    irun;
    RUNLINK *prunlinkNext;


    if ( potnode->runlist.prunlinkHead == prunlinkNil )
        crunPhantom = potnode->runlist.crun;


    for ( ipotnode = 0; ipotnode < crunFanInMax; ipotnode++ )
    {

        if ( potnode->rgpotnode[ipotnode] == potnodeNil )
            continue;


        CallR( ErrSORTIOptTreeMergeDF(  pscb,
                                        potnode->rgpotnode[ipotnode],
                                        &potnode->runlist.prunlinkHead ) );
        OTNODEReleasePotnode( potnode->rgpotnode[ipotnode] );
        potnode->rgpotnode[ipotnode] = potnodeNil;
        potnode->runlist.crun++;
    }


    if ( crunPhantom > 0 )
    {
        for ( irun = 0; irun < crunPhantom; irun++ )
        {
            prunlinkNext = pscb->runlist.prunlinkHead->prunlinkNext;
            pscb->runlist.prunlinkHead->prunlinkNext = potnode->runlist.prunlinkHead;
            potnode->runlist.prunlinkHead = pscb->runlist.prunlinkHead;
            pscb->runlist.prunlinkHead = prunlinkNext;
        }
        pscb->runlist.crun -= crunPhantom;
    }


    if ( pprunlink != NULL )
    {

        CallR( ErrSORTIMergeToRun(  pscb,
                                    potnode->runlist.prunlinkHead,
                                    pprunlink ) );
        SORTIRunDeleteList( pscb, &potnode->runlist.prunlinkHead, crunAll );
        potnode->runlist.crun = 0;
    }

    return JET_errSuccess;
}



LOCAL VOID SORTIOptTreeFree( SCB *pscb, OTNODE *potnode )
{
    LONG    ipotnode;


    for ( ipotnode = 0; ipotnode < crunFanInMax; ipotnode++ )
    {
        if ( potnode->rgpotnode[ipotnode] == potnodeNil )
            continue;

        SORTIOptTreeFree( pscb, potnode->rgpotnode[ipotnode] );
        OTNODEReleasePotnode( potnode->rgpotnode[ipotnode] );
    }


    SORTIRunDeleteListMem( pscb, &potnode->runlist.prunlinkHead, crunAll );
}

