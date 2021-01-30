// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include "_comp.hxx"


const INT fCOLSDELETEDNone      = 0;
const INT fCOLSDELETEDFixedVar  = (1<<0);
const INT fCOLSDELETEDTagged    = (1<<1);

INLINE BOOL FCOLSDELETEDNone( const INT fColumnsDeleted )
{
    return ( fColumnsDeleted == fCOLSDELETEDNone );
}
INLINE BOOL FCOLSDELETEDFixedVar( const INT fColumnsDeleted )
{
    return ( fColumnsDeleted & fCOLSDELETEDFixedVar );
}
INLINE BOOL FCOLSDELETEDTagged( const INT fColumnsDeleted )
{
    return ( fColumnsDeleted & fCOLSDELETEDTagged );
}

INLINE VOID FCOLSDELETEDSetNone( INT& fColumnsDeleted )
{
    fColumnsDeleted = fCOLSDELETEDNone;
    Assert( FCOLSDELETEDNone( fColumnsDeleted ) );
}
INLINE VOID FCOLSDELETEDSetFixedVar( INT& fColumnsDeleted )
{
    fColumnsDeleted |= fCOLSDELETEDFixedVar;
    Assert( FCOLSDELETEDFixedVar( fColumnsDeleted ) );
}
INLINE VOID FCOLSDELETEDSetTagged( INT& fColumnsDeleted )
{
    fColumnsDeleted |= fCOLSDELETEDTagged;
    Assert( FCOLSDELETEDTagged( fColumnsDeleted ) );
}

LOCAL ERR ErrSORTTableOpen(
    PIB             *ppib,
    JET_COLUMNDEF   *rgcolumndef,
    ULONG           ccolumndef,
    JET_UNICODEINDEX2 *pidxunicode,
    JET_GRBIT       grbit,
    FUCB            **ppfucb,
    JET_COLUMNID    *rgcolumnid,
    ULONG           cbKeyMost,
    ULONG           cbVarSegMac )
{
    ERR             err                     = JET_errSuccess;
    INT             icolumndefMax           = (INT)ccolumndef;
    FUCB            *pfucb                  = pfucbNil;
    TDB             *ptdb                   = ptdbNil;
    JET_COLUMNDEF   *pcolumndef             = NULL;
    JET_COLUMNID    *pcolumnid              = NULL;
    JET_COLUMNDEF   *pcolumndefMax          = rgcolumndef+icolumndefMax;
    TCIB            tcib                    = { fidFixedLeast-1, fidVarLeast-1, fidTaggedLeast-1 };
    WORD            ibRec;
    BOOL            fTruncate;
    BOOL            fIndexOnLocalizedText   = fFalse;
    IDXSEG          rgidxseg[JET_ccolKeyMost];
    ULONG           iidxsegMac;
    IDB             idb( PinstFromPpib( ppib ) );
    idb.SetCidxsegConditional( 0 );

    CheckPIB( ppib );

    INST            *pinst = PinstFromPpib( ppib );

    BOOL fRemoveDuplicates = !( ( grbit & JET_bitTTForwardOnly ) && !( grbit & JET_bitTTUnique ) );

    CallJ( ErrSORTOpen( ppib, &pfucb, fRemoveDuplicates, fFalse ), SimpleError );
    *ppfucb = pfucb;

    pfucb->u.pscb->grbit = grbit;

    if ( cbKeyMost != 0 && cbKeyMost < JET_cbKeyMostMin || cbKeyMost > (ULONG) CbKeyMostForPage() )
    {
        CallJ( ErrERRCheck( JET_errInvalidParameter ), SimpleError );
    }
    if ( cbKeyMost != 0 && cbVarSegMac > cbKeyMost || cbKeyMost == 0 && cbVarSegMac > JET_cbKeyMostMin )
    {
        CallJ( ErrERRCheck( JET_errInvalidParameter ), SimpleError );
    }


    for ( pcolumndef = rgcolumndef, pcolumnid = rgcolumnid; pcolumndef < pcolumndefMax; pcolumndef++, pcolumnid++ )
    {
        if ( ( pcolumndef->grbit & JET_bitColumnTagged )
            || FRECLongValue( pcolumndef->coltyp ) )
        {
            if ( ( *pcolumnid = ++tcib.fidTaggedLast ) > fidTaggedMost )
            {
                Error( ErrERRCheck( JET_errTooManyColumns ) );
            }
        }
        else if ( pcolumndef->coltyp == JET_coltypBit ||
            pcolumndef->coltyp == JET_coltypUnsignedByte ||
            pcolumndef->coltyp == JET_coltypShort ||
            pcolumndef->coltyp == JET_coltypLong ||
            pcolumndef->coltyp == JET_coltypCurrency ||
            pcolumndef->coltyp == JET_coltypIEEESingle ||
            pcolumndef->coltyp == JET_coltypIEEEDouble ||
            pcolumndef->coltyp == JET_coltypDateTime ||
            pcolumndef->coltyp == JET_coltypUnsignedShort ||
            pcolumndef->coltyp == JET_coltypUnsignedLong ||
            pcolumndef->coltyp == JET_coltypLongLong ||
            pcolumndef->coltyp == JET_coltypUnsignedLongLong ||
            pcolumndef->coltyp == JET_coltypGUID ||
            ( pcolumndef->grbit & JET_bitColumnFixed ) )
        {
            if ( ( *pcolumnid = ++tcib.fidFixedLast ) > fidFixedMost )
            {
                Error( ErrERRCheck( JET_errTooManyColumns ) );
            }
        }
        else if ( ( *pcolumnid = ++tcib.fidVarLast ) > fidVarMost )
        {
            Error( ErrERRCheck( JET_errTooManyColumns ) );
        }
    }

    Assert( pfucb->u.pscb->fcb.FTypeSort() );
    Assert( pfucb->u.pscb->fcb.FPrimaryIndex() );
    Assert( pfucb->u.pscb->fcb.FSequentialIndex() );

    Call( ErrTDBCreate( pinst, pfucb->ifmp, &ptdb, &tcib ) );

    Assert( ptdb->ItagTableName() == 0 );

    pfucb->u.pscb->fcb.SetPtdb( ptdb );
    Assert( ptdbNil != pfucb->u.pscb->fcb.Ptdb() );
    Assert( pidbNil == pfucb->u.pscb->fcb.Pidb() );

    ibRec = ibRECStartFixedColumns;

    iidxsegMac = 0;
    for ( pcolumndef = rgcolumndef, pcolumnid = rgcolumnid; pcolumndef < pcolumndefMax; pcolumndef++, pcolumnid++ )
    {
        FIELD   field;
        BOOL    fLocalizedText  = fFalse;

        memset( &field, 0, sizeof(FIELD) );
        field.coltyp = FIELD_COLTYP( pcolumndef->coltyp );
        if ( FRECTextColumn( field.coltyp ) )
        {
            switch( pcolumndef->cp )
            {
                case usUniCodePage:
                    fLocalizedText = fTrue;
                    field.cp = usUniCodePage;
                    break;

                case 0:
                case usEnglishCodePage:
                    field.cp = usEnglishCodePage;
                    break;

                default:
                    Error( ErrERRCheck( JET_errInvalidCodePage ) );
                    break;
            }
        }

        Assert( field.coltyp != JET_coltypNil );
        field.cbMaxLen = UlCATColumnSize( field.coltyp, pcolumndef->cbMax, &fTruncate );

        if ( FCOLUMNIDFixed( *pcolumnid ) )
        {
            field.ibRecordOffset = ibRec;
            ibRec = WORD( ibRec + field.cbMaxLen );
        }

        Assert( !FCOLUMNIDTemplateColumn( *pcolumnid ) );
        Call( ErrRECAddFieldDef( ptdb, *pcolumnid, &field ) );

        if ( ( pcolumndef->grbit & JET_bitColumnTTKey ) && iidxsegMac < JET_ccolKeyMost )
        {
            rgidxseg[iidxsegMac].ResetFlags();
            rgidxseg[iidxsegMac].SetFid( FidOfColumnid( *pcolumnid ) );

            if ( pcolumndef->grbit & JET_bitColumnTTDescending )
                rgidxseg[iidxsegMac].SetFDescending();

            if ( fLocalizedText )
                fIndexOnLocalizedText = fTrue;

            iidxsegMac++;
        }
    }
    
    Assert( ibRec >= REC::cbRecordMin );
    Assert( ibRec <= REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) );
    Assert( ibRec <= REC::CbRecordMost( pfucb ) );
    if ( ibRec > REC::CbRecordMost( pfucb ) )
    {
        FireWall( "SORTTableOpenRecTooBig9.2" );
    }
    if ( ibRec < REC::cbRecordMin || ibRec > REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) )
    {
        FireWall( "SORTTableOpenRecTooBig9.1" );
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }
    
    ptdb->SetIbEndFixedColumns( ibRec, ptdb->FidFixedLast() );

    Assert( iidxsegMac <= JET_ccolKeyMost );
    idb.SetCidxseg( (BYTE)iidxsegMac );
    if ( iidxsegMac > 0 )
    {
        idb.SetCbKeyMost( (USHORT)( cbKeyMost ? cbKeyMost : JET_cbKeyMostMin ) );
        idb.SetCbVarSegMac( (USHORT) ( cbVarSegMac ? cbVarSegMac : idb.CbKeyMost() ) );

        idb.ResetFlags();

        if ( NULL != pidxunicode )
        {
            const BOOL fUppercaseTextNormalization = ( pidxunicode->dwMapFlags & LCMAP_UPPERCASE ) &&
                ( g_rgfmp[pfucb->ifmp].ErrDBFormatFeatureEnabled( JET_efvUppercaseTextNormalization ) >= JET_errSuccess );

            Call( ErrNORMCheckLocaleName( pinst, pidxunicode->szLocaleName ) );
            Call( ErrNORMCheckLCMapFlags( pinst, &pidxunicode->dwMapFlags, fUppercaseTextNormalization ) );
            idb.SetWszLocaleName( pidxunicode->szLocaleName );
            idb.SetDwLCMapFlags( pidxunicode->dwMapFlags );
            idb.SetFLocaleSet();
        }
        else
        {
            Assert( !idb.FLocaleSet() );
            idb.SetWszLocaleName( pinst->m_wszLocaleNameDefault );
            idb.SetDwLCMapFlags( pinst->m_dwLCMapFlagsDefault );
        }

        if ( fIndexOnLocalizedText )
            idb.SetFLocalizedText();
        if ( grbit & JET_bitTTSortNullsHigh )
            idb.SetFSortNullsHigh();
        if ( grbit & JET_bitTTDotNetGuid )
            idb.SetFDotNetGuid();

        idb.SetFPrimary();
        idb.SetFAllowAllNulls();
        idb.SetFAllowFirstNull();
        idb.SetFAllowSomeNulls();
        idb.SetFUnique();
        idb.SetItagIndexName( 0 );

        Call( ErrIDBSetIdxSeg( &idb, ptdb, rgidxseg ) );

        Call( ErrFILEIGenerateIDB( &( pfucb->u.pscb->fcb ), ptdb, &idb ) );
        Assert( pidbNil != pfucb->u.pscb->fcb.Pidb() );

        if ( ibRec > REC::CbRecordMost( g_rgfmp[ pfucb->ifmp ].CbPage(), &idb ) )
        {
            Error( ErrERRCheck( JET_errRecordTooBig ) );
        }

        pfucb->u.pscb->fcb.Lock();
        pfucb->u.pscb->fcb.ResetSequentialIndex();
        pfucb->u.pscb->fcb.Unlock();
    }

    Assert( ptdbNil != pfucb->u.pscb->fcb.Ptdb() );

    pfucb->pvWorkBuf = NULL;
    pfucb->dataWorkBuf.SetPv( NULL );
    FUCBResetUpdateFlags( pfucb );

    pfucb->dataSearchKey.Nullify();
    pfucb->cColumnsInSearchKey = 0;
    KSReset( pfucb );

    return JET_errSuccess;

HandleError:
    SORTClose( pfucb );
SimpleError:
    *ppfucb = pfucbNil;
    return err;
}


ERR VTAPI ErrIsamSortOpen(
    PIB                 *ppib,
    JET_COLUMNDEF       *rgcolumndef,
    ULONG               ccolumndef,
    JET_UNICODEINDEX2   *pidxunicode,
    JET_GRBIT           grbit,
    FUCB                **ppfucb,
    JET_COLUMNID        *rgcolumnid,
    ULONG               cbKeyMost,
    ULONG               cbVarSegMac )
{
    ERR             err;
    FUCB            *pfucb;

    CallR( ErrPIBOpenTempDatabase( ppib ) );
    CallR( ErrSORTTableOpen( ppib, rgcolumndef, ccolumndef, pidxunicode, grbit, &pfucb, rgcolumnid, cbKeyMost, cbVarSegMac ) );
    Assert( pfucb->u.pscb->fcb.WRefCount() == 1 );
    SORTBeforeFirst( pfucb );

    pfucb->pvtfndef = &vtfndefTTSortIns;

    FUCBSetUpdatable( pfucb );
    *ppfucb = pfucb;

    return err;
}



ERR VTAPI ErrIsamSortSetIndexRange( JET_SESID sesid, JET_VTID vtid, JET_GRBIT grbit )
{
    PIB     * const ppib    = ( PIB * )( sesid );
    FUCB    * const pfucb   = ( FUCB * )( vtid );
    ERR     err = JET_errSuccess;

    CallR( ErrPIBCheck( ppib ) );
    CheckSort( ppib, pfucb );
    if ( !( pfucb->u.pscb->grbit & (JET_bitTTScrollable|JET_bitTTIndexed) ) )
    {
        CallR( ErrERRCheck( JET_errIllegalOperation ) );
    }

    if ( !FKSPrepared( pfucb ) )
    {
        return ErrERRCheck( JET_errKeyNotMade );
    }

    FUCBSetIndexRange( pfucb, grbit );
    err =  ErrSORTCheckIndexRange( pfucb );

    KSReset( pfucb );

    if ( grbit & JET_bitRangeInstantDuration )
    {
        DIRResetIndexRange( pfucb );
    }

    return err;
}


ERR VTAPI ErrIsamSortMove( JET_SESID sesid, JET_VTID vtid, LONG csrid, JET_GRBIT grbit )
{
    PIB     * const ppib    = ( PIB * )( sesid );
    FUCB    * const pfucb   = ( FUCB * )( vtid );
    ERR     err = JET_errSuccess;

    Assert( !FSCBInsert( pfucb->u.pscb ) );

    Assert( !pfucb->pmoveFilterContext );

    CallR( ErrPIBCheck( ppib ) );
    CheckSort( ppib, pfucb );

    Assert( !FFUCBUpdatePrepared( pfucb ) );

    if ( csrid > 0 )
    {
        if ( csrid == JET_MoveLast )
        {
            err = ErrSORTLast( pfucb );
        }
        else
        {
            while ( csrid-- > 0 )
            {
                if ( ( err = ErrSORTNext( pfucb ) ) < 0 )
                    break;
            }
        }
    }
    else if ( csrid < 0 )
    {
        if ( csrid == JET_MoveFirst )
        {
            err = ErrSORTFirst( pfucb );
        }
        else
        {
            if ( !( pfucb->u.pscb->grbit & (JET_bitTTScrollable|JET_bitTTIndexed) ) )
            {
                CallR( ErrERRCheck( JET_errIllegalOperation ) );
            }

            while ( csrid++ < 0 )
            {
                if ( ( err = ErrSORTPrev( pfucb ) ) < 0 )
                    break;
            }
        }
    }
    else
    {
        Assert( csrid == 0 );
        if ( pfucb->u.pscb->ispairMac > 0
            && pfucb->ispairCurr < pfucb->u.pscb->ispairMac
            && pfucb->ispairCurr >= 0 )
        {
            err = JET_errSuccess;
        }
        else
        {
            err = ErrERRCheck( JET_errNoCurrentRecord );
        }
    }

    return err;
}


ERR VTAPI ErrIsamSortSeek( JET_SESID sesid, JET_VTID vtid, JET_GRBIT grbit )
{
    PIB     * const ppib    = ( PIB * )( sesid );
    FUCB    * const pfucb   = ( FUCB * )( vtid );
    ERR     err;

    CallR( ErrPIBCheck( ppib ) );
    CheckSort( ppib, pfucb );
    if ( !( pfucb->u.pscb->grbit & (JET_bitTTScrollable|JET_bitTTIndexed) ) )
    {
        CallR( ErrERRCheck( JET_errIllegalOperation ) );
    }

    Assert( !FFUCBUpdatePrepared( pfucb ) );

    if ( !( FKSPrepared( pfucb ) ) )
    {
        return ErrERRCheck( JET_errKeyNotMade );
    }

    FUCBAssertValidSearchKey( pfucb );

    KEY key;
    key.prefix.Nullify();
    key.suffix.SetPv( pfucb->dataSearchKey.Pv() );
    key.suffix.SetCb( pfucb->dataSearchKey.Cb() );

    err = ErrSORTSeek( pfucb, key );
    if ( err >= 0 )
    {
        KSReset( pfucb );
    }

    Assert( JET_errSuccess == err
        || wrnNDFoundGreater == err
        || wrnNDFoundLess == err
        || JET_errRecordNotFound == err );

    const JET_GRBIT bitSeekAll = ( JET_bitSeekEQ
                            | JET_bitSeekGE
                            | JET_bitSeekGT
                            | JET_bitSeekLE
                            | JET_bitSeekLT );

    switch ( grbit & bitSeekAll )
    {
        case JET_bitSeekLT:
            if ( JET_errSuccess == err || wrnNDFoundGreater == err )
            {
                err = ErrIsamSortMove( ppib, pfucb, JET_MovePrevious, NO_GRBIT );
                if ( JET_errNoCurrentRecord == err )
                {
                    err = ErrERRCheck( JET_errRecordNotFound );
                }
            }
            else if ( wrnNDFoundLess == err )
            {
                err = JET_errSuccess;
            }
            break;

        case JET_bitSeekLE:
            if ( wrnNDFoundGreater == err )
            {
                err = ErrIsamSortMove( ppib, pfucb, JET_MovePrevious, NO_GRBIT );
                if ( JET_errNoCurrentRecord == err )
                {
                    err = ErrERRCheck( JET_errRecordNotFound );
                }
            }
            else if ( wrnNDFoundLess == err )
            {
                err = JET_errSuccess;
            }
            break;

        case JET_bitSeekEQ:
            if ( wrnNDFoundLess == err || wrnNDFoundGreater == err )
            {
                err = ErrERRCheck( JET_errRecordNotFound );
            }
            break;

        case JET_bitSeekGE:
            if ( wrnNDFoundGreater == err )
            {
                err = JET_errSuccess;
            }
            else if ( wrnNDFoundLess == err )
            {
                err = ErrERRCheck( JET_errRecordNotFound );
            }
            break;

        case JET_bitSeekGT:
            if ( JET_errSuccess == err )
            {
                err = ErrIsamSortMove( ppib, pfucb, JET_MoveNext, NO_GRBIT );
                if ( JET_errNoCurrentRecord == err )
                {
                    err = ErrERRCheck( JET_errRecordNotFound );
                }
            }
            if ( wrnNDFoundGreater == err )
            {
                err = JET_errSuccess;
            }
            else if ( wrnNDFoundLess == err )
            {
                err = ErrERRCheck( JET_errRecordNotFound );
            }
            break;

        default:
            AssertSz( fFalse, "Unknown grbit" );
            break;
    }

    return err;
}


ERR VTAPI ErrIsamSortGetBookmark(
    JET_SESID       sesid,
    JET_VTID        vtid,
    VOID * const    pvBookmark,
    const ULONG     cbMax,
    ULONG * const   pcbActual )
{
    PIB     * const ppib    = ( PIB * )( sesid );
    FUCB    * const pfucb   = ( FUCB * )( vtid );
    SCB     * const pscb    = pfucb->u.pscb;
    ERR     err = JET_errSuccess;
    LONG    ipb;

    CallR( ErrPIBCheck( ppib ) );
    CheckSort( ppib, pfucb );
    Assert( pvBookmark != NULL );
    Assert( pscb->crun == 0 );

    if ( cbMax < sizeof( ipb ) )
    {
        return ErrERRCheck( JET_errBufferTooSmall );
    }

    ipb = pfucb->ispairCurr;
    if ( ipb < 0 || ipb >= pfucb->u.pscb->ispairMac )
        return ErrERRCheck( JET_errNoCurrentRecord );

    *(LONG *)pvBookmark = ipb;

    if ( pcbActual )
    {
        *pcbActual = sizeof(ipb);
    }

    Assert( err == JET_errSuccess );
    return err;
}


ERR VTAPI ErrIsamSortGotoBookmark(
    JET_SESID           sesid,
    JET_VTID            vtid,
    const VOID * const  pvBookmark,
    const ULONG         cbBookmark )
{
    ERR                 err;
    PIB * const         ppib    = ( PIB * )( sesid );
    FUCB * const        pfucb   = ( FUCB * )( vtid );

    CallR( ErrPIBCheck( ppib ) );
    CheckSort( ppib, pfucb );
    Assert( pfucb->u.pscb->crun == 0 );

    Assert( !FFUCBUpdatePrepared( pfucb ) );

    if ( cbBookmark != sizeof( LONG )
        || NULL == pvBookmark )
    {
        return ErrERRCheck( JET_errInvalidBookmark );
    }

    Assert( *( LONG *)pvBookmark < pfucb->u.pscb->ispairMac );
    Assert( *( LONG *)pvBookmark >= 0 );

    pfucb->ispairCurr = *(LONG *)pvBookmark;

    Assert( err == JET_errSuccess );
    return err;
}


#ifdef DEBUG

ERR VTAPI ErrIsamSortRetrieveKey(
    JET_SESID       sesid,
    JET_VTID        vtid,
    VOID*           pv,
    const ULONG     cbMax,
    ULONG*          pcbActual,
    JET_GRBIT       grbit )
{
    PIB* const      ppib    = ( PIB * )( sesid );
    FUCB* const     pfucb   = ( FUCB * )( vtid );

    return ErrIsamRetrieveKey( ppib, pfucb, (BYTE *)pv, cbMax, pcbActual, NO_GRBIT );
}

#endif



ERR VTAPI ErrIsamSortUpdate(
    JET_SESID       sesid,
    JET_VTID        vtid,
    VOID            * pv,
    ULONG           cbMax,
    ULONG           * pcbActual,
    JET_GRBIT       grbit )
{
    PIB     * const ppib    = (PIB *)sesid;
    FUCB    * const pfucb   = (FUCB *)vtid;

    ERR             err                     = JET_errSuccess;
    ULONG           cbKeyMost;
    const size_t    cbKeyStack              = 256;
    BYTE            rgbKeyStack[ cbKeyStack ];
    BYTE            *pbKeyRes               = NULL;
    BYTE            *pbKeyAlloc             = NULL;
    KEY             key;
    ULONG           iidxsegT;

    CallR( ErrPIBCheck( ppib ) );
    CheckSort( ppib, pfucb );

    Assert( FFUCBSort( pfucb ) );
    Assert( pfucb->u.pscb->fcb.FTypeSort() );
    if ( !( FFUCBInsertPrepared( pfucb ) ) )
    {
        return ErrERRCheck( JET_errUpdateNotPrepared );
    }
    Assert( pfucb->u.pscb != pscbNil );
    Assert( pfucb->u.pscb->fcb.Ptdb() != ptdbNil );
    

    
    if ( pfucb->dataWorkBuf.FNull() )
    {
        return ErrERRCheck( JET_errRecordNoCopy );
    }
    else
    {
        CallR( ErrRECIIllegalNulls( pfucb ) )
    }

    Assert( pfucb->u.pscb->fcb.Pidb() != pidbNil );
    cbKeyMost = pfucb->u.pscb->fcb.Pidb()->CbKeyMost();
    if ( cbKeyMost > cbKeyStack )
    {
        Alloc( pbKeyRes = (BYTE *)RESKEY.PvRESAlloc() );
        pbKeyAlloc = pbKeyRes;
    }
    else
    {
        pbKeyAlloc = rgbKeyStack;
    }
    key.prefix.Nullify();
    key.suffix.SetPv( pbKeyAlloc );
    key.suffix.SetCb( cbKeyMost );

    CallR( ErrRECRetrieveKeyFromCopyBuffer(
        pfucb,
        pfucb->u.pscb->fcb.Pidb(),
        &key,
        rgitagBaseKey,
        0,
        prceNil,
        &iidxsegT ) );

    CallS( ErrRECValidIndexKeyWarning( err ) );
    Assert( wrnFLDOutOfKeys != err );
    Assert( wrnFLDOutOfTuples != err );
    Assert( wrnFLDNotPresentInIndex != err );

    
    if ( pfucb->u.pscb->fcb.Pidb()->FNoNullSeg()
        && ( wrnFLDNullSeg == err || wrnFLDNullFirstSeg == err || wrnFLDNullKey == err ) )
    {
        return ErrERRCheck( JET_errNullKeyDisallowed );
    }

    
    if ( JET_errSuccess == err
        || ( wrnFLDNullKey == err && pfucb->u.pscb->fcb.Pidb()->FAllowAllNulls() )
        || ( wrnFLDNullFirstSeg == err && pfucb->u.pscb->fcb.Pidb()->FAllowFirstNull() )
        || ( wrnFLDNullSeg == err && pfucb->u.pscb->fcb.Pidb()->FAllowFirstNull() ) )
    {
        Assert( (ULONG) key.Cb() <= cbKeyMost );
        CallR( ErrSORTInsert( pfucb, key, pfucb->dataWorkBuf ) );
    }

    RECIFreeCopyBuffer( pfucb );
    FUCBResetUpdateFlags( pfucb );

HandleError:
    RESKEY.Free( pbKeyRes );
    return err;
}


ERR VTAPI ErrIsamSortDupCursor(
    JET_SESID       sesid,
    JET_VTID        vtid,
    JET_TABLEID     *ptableid,
    JET_GRBIT       grbit )
{
    PIB     * const ppib    = ( PIB * )( sesid );
    FUCB    * const pfucb   = ( FUCB * )( vtid );

    FUCB            **ppfucbDup = (FUCB **)ptableid;
    FUCB            *pfucbDup = pfucbNil;
    ERR             err = JET_errSuccess;

    CallR( ErrPIBOpenTempDatabase( ppib ) );
    
    if ( FFUCBIndex( pfucb ) )
    {
        err = ErrIsamDupCursor( ppib, pfucb, ppfucbDup, grbit );
        return err;
    }

    INST *pinst = PinstFromPpib( ppib );

    Call( ErrEnsureTempDatabaseOpened( pinst, ppib ) );
    
    Call( ErrFUCBOpen( ppib, pinst->m_mpdbidifmp[ dbidTemp ], &pfucbDup ) );
    Call( pfucb->u.pscb->fcb.ErrLink( pfucbDup ) );
    pfucbDup->pvtfndef = &vtfndefTTSortIns;

    pfucbDup->ulFlags = pfucb->ulFlags;

    pfucbDup->dataSearchKey.Nullify();
    pfucbDup->cColumnsInSearchKey = 0;
    KSReset( pfucbDup );

    
    pfucbDup->pvWorkBuf = NULL;
    pfucbDup->dataWorkBuf.SetPv( NULL );
    FUCBResetUpdateFlags( pfucbDup );

    
    err = ErrIsamSortMove( ppib, pfucbDup, (ULONG)JET_MoveFirst, 0 );
    if ( err < 0  )
    {
        if ( err != JET_errNoCurrentRecord )
            goto HandleError;
        err = JET_errSuccess;
    }

    *ppfucbDup = pfucbDup;

    return JET_errSuccess;

HandleError:
    if ( pfucbDup != pfucbNil )
    {
        if ( pfucbDup->u.pfcb != pfcbNil )
        {
            pfucbDup->u.pfcb->Unlink( pfucbDup );
        }
        
        FUCBClose( pfucbDup );
    }
    return err;
}


ERR VTAPI ErrIsamSortClose(
    JET_SESID       sesid,
    JET_VTID        vtid
    )
{
    PIB     * const ppib    = ( PIB * )( sesid );
    FUCB    * const pfucb   = ( FUCB * )( vtid );
    ERR     err = JET_errSuccess;

    CallR( ErrPIBCheck( ppib ) );

    Assert( pfucb->pvtfndef == &vtfndefTTSortIns
        || pfucb->pvtfndef == &vtfndefTTSortRet
        || pfucb->pvtfndef == &vtfndefTTBase
        || pfucb->pvtfndef == &vtfndefTTBaseMustRollback );
    pfucb->pvtfndef = &vtfndefInvalidTableid;

    if ( FFUCBIndex( pfucb ) )
    {
        CheckTable( ppib, pfucb );
        CallS( ErrFILECloseTable( ppib, pfucb ) );
    }
    else
    {
        CheckSort( ppib, pfucb );
        Assert( FFUCBSort( pfucb ) );

        
        RECReleaseKeySearchBuffer( pfucb );

        
        RECIFreeCopyBuffer( pfucb );

        SORTClose( pfucb );
    }

    return err;
}


ERR VTAPI ErrIsamSortGetTableInfo(
    JET_SESID       sesid,
    JET_VTID        vtid,
    _Out_bytecap_(cbOutMax) VOID            * pv,
    ULONG           cbOutMax,
    ULONG           lInfoLevel )
{
    FUCB    * const pfucb   = ( FUCB * )( vtid );

    if ( lInfoLevel != JET_TblInfo )
    {
        return ErrERRCheck( JET_errInvalidOperation );
    }

    
    if ( cbOutMax < sizeof(JET_OBJECTINFO) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    memset( (BYTE *)pv, 0x00, (SHORT)cbOutMax );
    ( (JET_OBJECTINFO *)pv )->cbStruct = sizeof(JET_OBJECTINFO);
    ( (JET_OBJECTINFO *)pv )->objtyp   = JET_objtypTable;

    ( (JET_OBJECTINFO *)pv )->cRecord  = ULONG( min( pfucb->u.pscb->cRecords, ulMax ) );

    return JET_errSuccess;
}


INLINE ERR ErrSORTCopyProgress(
    STATUSINFO  * pstatus,
    const ULONG cPagesTraversed )
{
    JET_SNPROG  snprog;

    Assert( pstatus->pfnStatus );
    Assert( pstatus->snt == JET_sntProgress );

    pstatus->cunitDone += ( cPagesTraversed * pstatus->cunitPerProgression );

    Assert( pstatus->cunitProjected <= pstatus->cunitTotal );
    if ( pstatus->cunitDone > pstatus->cunitProjected )
    {
        Assert( g_fRepair );
        pstatus->cunitPerProgression = 0;
        pstatus->cunitDone = pstatus->cunitProjected;
    }

    Assert( pstatus->cunitDone <= pstatus->cunitTotal );

    snprog.cbStruct = sizeof( JET_SNPROG );
    snprog.cunitDone = pstatus->cunitDone;
    snprog.cunitTotal = pstatus->cunitTotal;

    return ( ERR )( *pstatus->pfnStatus )(
        pstatus->sesid,
        pstatus->snp,
        pstatus->snt,
        &snprog );
}


ERR ErrSORTIncrementLVRefcountDest(
    FUCB            * const pfucbSrc,
    const LvId      lidSrc,
    LvId            * const plidDest )
{
    LIDMAP* const plidmap = g_rgfmp[pfucbSrc->ifmp].Plidmap();
    Assert( NULL != plidmap );
    if ( plidmap == NULL )
    {
        return ErrERRCheck( JET_errDatabaseCorrupted );
    }
    Assert( JET_tableidNil != plidmap->Tableid() );
    return plidmap->ErrIncrementLVRefcountDest(
                pfucbSrc->ppib,
                lidSrc,
                plidDest );
}

INLINE ERR ErrSORTUpdateSeparatedLVs(
    FUCB                * pfucbSrc,
    FUCB                * pfucbDest,
    JET_COLUMNID        * mpcolumnidcolumnidTagged,
    STATUSINFO          * pstatus )
{
    TAGFIELDS           tagfields( pfucbDest->dataWorkBuf );
    return tagfields.ErrUpdateSeparatedLongValuesAfterCopy(
                        pfucbSrc,
                        pfucbDest,
                        mpcolumnidcolumnidTagged,
                        pstatus );
}

INLINE ERR ErrSORTCopyTaggedColumns(
    FUCB                * pfucbSrc,
    FUCB                * pfucbDest,
    BYTE                * pbRecBuf,
    JET_COLUMNID        * mpcolumnidcolumnidTagged,
    STATUSINFO          * pstatus )
{
    Assert( Pcsr( pfucbSrc )->FLatched() );

    TAGFIELDS           tagfields( pfucbSrc->kdfCurr.data );

    tagfields.Migrate( pbRecBuf );
    CallS( ErrDIRRelease( pfucbSrc ) );

    tagfields.CopyTaggedColumns(
                        pfucbSrc,
                        pfucbDest,
                        mpcolumnidcolumnidTagged );

    return ErrSORTUpdateSeparatedLVs(
                        pfucbSrc,
                        pfucbDest,
                        mpcolumnidcolumnidTagged,
                        pstatus );
}


INLINE SIZE_T CbSORTCopyFixedVarColumns(
    TDB             *ptdbSrc,
    TDB             *ptdbDest,
    CPCOL           *rgcpcol,
    ULONG           ccpcolMax,
    BYTE            * const pbRecSrc,
    BYTE            * const pbRecDest )
{
    REC                                     * const precSrc = (REC *)pbRecSrc;
    REC                                     * const precDest = (REC *)pbRecDest;
    UnalignedLittleEndian< REC::VAROFFSET > *pibVarOffsSrc;
    UnalignedLittleEndian< REC::VAROFFSET > *pibVarOffsDest;
    BYTE                                    *prgbitNullSrc;
    BYTE                                    *prgbitNullDest;
    BYTE                                    *prgbitNullT;
    FID                                     fidFixedLastDest;
    FID                                     fidVarLastDest;
    FID                                     fidT;
    BYTE                                    *pbChunkSrc = 0;
    BYTE                                    *pbChunkDest = 0;
    INT                                     cbChunk;

    const FID       fidFixedLastSrc = precSrc->FidFixedLastInRec();
    const FID       fidVarLastSrc   = precSrc->FidVarLastInRec();

    prgbitNullSrc = precSrc->PbFixedNullBitMap();
    pibVarOffsSrc = precSrc->PibVarOffsets();

    Assert( (BYTE *)pibVarOffsSrc > pbRecSrc );
    Assert( (BYTE *)pibVarOffsSrc < pbRecSrc + REC::CbRecordMostWithGlobalPageSize() );

    const INT   cFixedColumnsDest = ( ptdbDest->FidFixedLast() - fidFixedLeast + 1 );
    Assert( ptdbSrc->IbEndFixedColumns() < REC::CbRecordMostWithGlobalPageSize() );
    Assert( ptdbDest->IbEndFixedColumns() <= ptdbSrc->IbEndFixedColumns() );
    Assert( ptdbDest->IbEndFixedColumns() + ( ( cFixedColumnsDest + 7 ) / 8 )
            <= REC::CbRecordMostWithGlobalPageSize() );
    prgbitNullT = pbRecDest + ptdbDest->IbEndFixedColumns();
    memset( prgbitNullT, 0, ( cFixedColumnsDest + 7 ) / 8 );

    pbChunkSrc = pbRecSrc + ibRECStartFixedColumns;
    pbChunkDest = pbRecDest + ibRECStartFixedColumns;
    cbChunk = 0;

#ifdef DEBUG
    BOOL            fSawPlaceholderColumn   = fFalse;
#endif

    Assert( !ptdbSrc->FInitialisingDefaultRecord() );

    fidFixedLastDest = fidFixedLeast-1;
    for ( fidT = fidFixedLeast; fidT <= fidFixedLastSrc; fidT++ )
    {
        const BOOL  fTemplateColumn = ptdbSrc->FFixedTemplateColumn( fidT );
        const WORD  ibNextOffset    = ptdbSrc->IbOffsetOfNextColumn( fidT );
        const FIELD *pfieldFixedSrc = ptdbSrc->PfieldFixed( ColumnidOfFid( fidT, fTemplateColumn ) );

        if ( JET_coltypNil == pfieldFixedSrc->coltyp
            || FFIELDPrimaryIndexPlaceholder( pfieldFixedSrc->ffield ) )
        {
#ifdef DEBUG
            if ( FFIELDPrimaryIndexPlaceholder( pfieldFixedSrc->ffield ) )
                fSawPlaceholderColumn = fTrue;
            else
                Assert( !fTemplateColumn );
#endif
            if ( cbChunk > 0 )
            {
                UtilMemCpy( pbChunkDest, pbChunkSrc, cbChunk );
                pbChunkDest += cbChunk;
            }

            pbChunkSrc = pbRecSrc + ibNextOffset;
            cbChunk = 0;
        }
        else
        {
#ifdef DEBUG
            BOOL    fFound = fFalse;

            for ( ULONG icol = 0; icol < ccpcolMax; icol++ )
            {
                if ( rgcpcol[icol].columnidSrc == ColumnidOfFid( fidT, fTemplateColumn )  )
                {
                    const COLUMNID  columnidT   = ColumnidOfFid(
                                                        FID( fidFixedLastDest+1 ),
                                                        ptdbDest->FFixedTemplateColumn( FID( fidFixedLastDest+1 ) ) );
                    Assert( rgcpcol[icol].columnidDest == columnidT );
                    fFound = fTrue;
                    break;
                }
            }
            if ( fidT >= ptdbSrc->FidFixedFirst() )
            {
                Assert( fFound );
            }
            else
            {
                Assert( !fFound );
                Assert( fidT == fidFixedLastDest+1
                    || ( fSawPlaceholderColumn && fidT == fidFixedLastDest+1+1 ) );
            }
#endif

            const UINT  ifid    = fidT - fidFixedLeast;

            Assert( FFixedNullBit( prgbitNullSrc + ( ifid/8 ), ifid )
                || !FFixedNullBit( prgbitNullT + ( fidFixedLastDest / 8 ), fidFixedLastDest ) );
            if ( FFixedNullBit( prgbitNullSrc + ( ifid/8 ), ifid ) )
            {
                SetFixedNullBit(
                    prgbitNullT + ( fidFixedLastDest / 8 ),
                    fidFixedLastDest );
            }


            fidFixedLastDest++;

#ifdef DEBUG
            const COLUMNID  columnidT       = ColumnidOfFid(
                                                    fidFixedLastDest,
                                                    ptdbDest->FFixedTemplateColumn( fidFixedLastDest ) );
            const FIELD     * const pfieldT = ptdbDest->PfieldFixed( columnidT );
            Assert( ibNextOffset > pfieldFixedSrc->ibRecordOffset );
            Assert( pfieldT->ibRecordOffset == pbChunkDest + cbChunk - pbRecDest );
            Assert( pfieldT->ibRecordOffset >= ibRECStartFixedColumns );
            Assert( pfieldT->ibRecordOffset < REC::CbRecordMostWithGlobalPageSize() );
            Assert( pfieldT->ibRecordOffset < ptdbDest->IbEndFixedColumns() );
#endif

            Assert( pfieldFixedSrc->cbMaxLen ==
                        (ULONG)( ibNextOffset - pfieldFixedSrc->ibRecordOffset ) );
            cbChunk += pfieldFixedSrc->cbMaxLen;
        }
    }

    Assert( fidFixedLastDest <= ptdbDest->FidFixedLast() );

    if ( cbChunk > 0 )
    {
        UtilMemCpy( pbChunkDest, pbChunkSrc, cbChunk );
    }

    Assert( prgbitNullT == pbRecDest + ptdbDest->IbEndFixedColumns() );
    if ( fidFixedLastDest < ptdbDest->FidFixedLast() )
    {
        const COLUMNID  columnidT       = ColumnidOfFid(
                                                FID( fidFixedLastDest+1 ),
                                                ptdbDest->FFixedTemplateColumn( FID( fidFixedLastDest+1 ) ) );
        const FIELD     * const pfieldT = ptdbDest->PfieldFixed( columnidT );
        prgbitNullDest = pbRecDest + pfieldT->ibRecordOffset;

        memmove( prgbitNullDest, prgbitNullT, ( fidFixedLastDest + 7 ) / 8 );
    }
    else
    {
        prgbitNullDest = prgbitNullT;
    }

    Assert( pbChunkDest + cbChunk == prgbitNullDest );


    pibVarOffsDest = (UnalignedLittleEndian<REC::VAROFFSET> *)(
                        prgbitNullDest + ( ( fidFixedLastDest + 7 ) / 8 ) );

    fidVarLastDest = fidVarLeast-1;
    for ( fidT = fidVarLeast; fidT <= fidVarLastSrc; fidT++ )
    {
        const COLUMNID  columnidVarSrc          = ColumnidOfFid( fidT, ptdbSrc->FVarTemplateColumn( fidT ) );
        const FIELD     * const pfieldVarSrc    = ptdbSrc->PfieldVar( columnidVarSrc );

        Assert( pfieldVarSrc->coltyp != JET_coltypNil
            || !FCOLUMNIDTemplateColumn( columnidVarSrc ) );
        if ( pfieldVarSrc->coltyp != JET_coltypNil )
        {
#ifdef DEBUG
            BOOL        fFound = fFalse;

            for ( ULONG icol = 0; icol < ccpcolMax; icol++ )
            {
                if ( rgcpcol[icol].columnidSrc == columnidVarSrc )
                {
                    const COLUMNID  columnidT       = ColumnidOfFid(
                                                            FID( fidVarLastDest+1 ),
                                                            ptdbDest->FVarTemplateColumn( FID( fidVarLastDest+1 ) ) );
                    Assert( rgcpcol[icol].columnidDest == columnidT );
                    fFound = fTrue;
                    break;
                }
            }
            if ( fidT >= ptdbSrc->FidVarFirst() )
            {
                Assert( fFound );
            }
            else
            {
                Assert( !fFound );
                Assert( fidT == fidVarLastDest+1 );
            }
#endif

            fidVarLastDest++;
        }
    }
    Assert( fidVarLastDest <= ptdbDest->FidVarLast() );

    pbChunkSrc = (BYTE *)( pibVarOffsSrc + ( fidVarLastSrc - fidVarLeast + 1 ) );
    Assert( pbChunkSrc == precSrc->PbVarData() );
    pbChunkDest = (BYTE *)( pibVarOffsDest + ( fidVarLastDest - fidVarLeast + 1 ) );
    cbChunk = 0;

#ifdef DEBUG
    const FID   fidVarLastSave = fidVarLastDest;
#endif

    fidVarLastDest = fidVarLeast-1;
    for ( fidT = fidVarLeast; fidT <= fidVarLastSrc; fidT++ )
    {
        const COLUMNID  columnidVarSrc          = ColumnidOfFid( fidT, ptdbSrc->FVarTemplateColumn( fidT ) );
        const FIELD     * const pfieldVarSrc    = ptdbSrc->PfieldVar( columnidVarSrc );
        const UINT      ifid                    = fidT - fidVarLeast;

        Assert( pfieldVarSrc->coltyp != JET_coltypNil
            || !FCOLUMNIDTemplateColumn( columnidVarSrc ) );
        if ( pfieldVarSrc->coltyp == JET_coltypNil )
        {
            if ( cbChunk > 0 )
            {
                UtilMemCpy( pbChunkDest, pbChunkSrc, cbChunk );
                pbChunkDest += cbChunk;
            }

            pbChunkSrc = precSrc->PbVarData() + IbVarOffset( pibVarOffsSrc[ifid] );
            cbChunk = 0;
        }
        else
        {
            const REC::VAROFFSET    ibStart = ( fidVarLeast-1 == fidVarLastDest ?
                                                    REC::VAROFFSET( 0 ) :
                                                    IbVarOffset( pibVarOffsDest[fidVarLastDest-fidVarLeast] ) );
            INT cb;

            fidVarLastDest++;
            if ( FVarNullBit( pibVarOffsSrc[ifid] ) )
            {
                pibVarOffsDest[fidVarLastDest-fidVarLeast] = ibStart;
                SetVarNullBit( *( UnalignedLittleEndian< WORD >*)(&pibVarOffsDest[fidVarLastDest-fidVarLeast]) );
                cb = 0;
            }
            else
            {
                if ( ifid > 0 )
                {
                    Assert( IbVarOffset( pibVarOffsSrc[ifid] ) >=
                                IbVarOffset( pibVarOffsSrc[ifid-1] ) );
                    cb = IbVarOffset( pibVarOffsSrc[ifid] )
                            - IbVarOffset( pibVarOffsSrc[ifid-1] );
                }
                else
                {
                    Assert( IbVarOffset( pibVarOffsSrc[ifid] ) >= 0 );
                    cb = IbVarOffset( pibVarOffsSrc[ifid] );
                }

                pibVarOffsDest[fidVarLastDest-fidVarLeast] = REC::VAROFFSET( ibStart + cb );
                Assert( !FVarNullBit( pibVarOffsDest[fidVarLastDest-fidVarLeast] ) );
            }

            cbChunk += cb;
        }
    }

    Assert( fidVarLastDest == fidVarLastSave );

    if ( cbChunk > 0 )
    {
        UtilMemCpy( pbChunkDest, pbChunkSrc, cbChunk );
    }

    precDest->SetFidFixedLastInRec( fidFixedLastDest );
    precDest->SetFidVarLastInRec( fidVarLastDest );
    precDest->SetIbEndOfFixedData( REC::RECOFFSET( (BYTE *)pibVarOffsDest - pbRecDest ) );

    Assert( precDest->PbTaggedData() == pbChunkDest + cbChunk );

    Assert( precDest->IbEndOfVarData() <= precSrc->IbEndOfVarData() );

    return precDest->PbVarData() + precDest->IbEndOfVarData() - pbRecDest;
}


INLINE VOID SORTCheckVarTaggedCols( const REC *prec, const ULONG cbRec, const TDB *ptdb )
{
#if 0
    const BYTE          *pbRecMax           = (BYTE *)prec + cbRec;
    TAGFLD              *ptagfld;
    TAGFLD              *ptagfldPrev;
    FID                 fid;
    const WORD          wCorruptBit         = 0x0400;

    Assert( prec->FidFixedLastInRec() <= ptdb->FidFixedLast() );
    Assert( prec->FidVarLastInRec() <= ptdb->FidVarLast() );


    UnalignedLittleEndian<REC::VAROFFSET>   *pibVarOffs     = prec->PibVarOffsets();

    Assert( (BYTE *)pibVarOffs <= pbRecMax );

    Assert( !FVarNullBit( pibVarOffs[fidVarLastInRec+1-fidVarLeast] ) );
    Assert( ibVarOffset( pibVarOffs[fidVarLastInRec+1-fidVarLeast] ) ==
        pibVarOffs[fidVarLastInRec+1-fidVarLeast] );
    Assert( pibVarOffs[fidVarLastInRec+1-fidVarLeast] > sizeof(RECHDR) );
    Assert( pibVarOffs[fidVarLastInRec+1-fidVarLeast] <= cbRec );

    for ( fid = fidVarLeast; fid <= fidVarLastInRec; fid++ )
    {
        WORD    db;
        Assert( ibVarOffset( pibVarOffs[fid-fidVarLeast] ) > sizeof(RECHDR) );
        Assert( (ULONG)ibVarOffset( pibVarOffs[fid-fidVarLeast] ) <= cbRec );
        Assert( ibVarOffset( pibVarOffs[fid+1-fidVarLeast] ) > sizeof(RECHDR) );
        Assert( (ULONG)ibVarOffset( pibVarOffs[fid+1-fidVarLeast] ) <= cbRec );
        Assert( ibVarOffset( pibVarOffs[fid-fidVarLeast] ) <= ibVarOffset( pibVarOffs[fid+1-fidVarLeast] ) );

        db = ibVarOffset( pibVarOffs[fid+1-fidVarLeast] ) - ibVarOffset( pibVarOffs[ fid-fidVarLeast] );
        Assert( db >= 0 );
        if ( db > JET_cbColumnMost && ( pibVarOffs[fid+1-fidVarLeast] & wCorruptBit ) )
        {
            pibVarOffs[fid+1-fidVarLeast] &= ~wCorruptBit;
            db = ibVarOffset( pibVarOffs[fid+1-fidVarLeast] ) - ibVarOffset( pibVarOffs [fid-fidVarLeast] );
            printf( "\nReset corrupt bit in VarOffset entry." );
        }
        Assert( db <= JET_cbColumnMost );
    }

CheckTagged:
    ptagfldPrev = (TAGFLD*)( pbRec + pibVarOffs[fidVarLastInRec+1-fidVarLeast] );
    for ( ptagfld = (TAGFLD*)( pbRec + pibVarOffs[fidVarLastInRec+1-fidVarLeast] );
        (BYTE *)ptagfld < pbRecMax;
        ptagfld = PtagfldNext( ptagfld ) )
    {
        if ( ptagfld->fid > pfdb->fidTaggedLast && ( ptagfld->fid & wCorruptBit ) )
        {
            ptagfld->fid &= ~wCorruptBit;
            printf( "\nReset corrupt bit in Fid of TAGFLD." );
        }
        Assert( ptagfld->fid <= pfdb->fidTaggedLast );
        Assert( FTaggedFid( ptagfld->fid ) );
        if ( (BYTE *)PtagfldNext( ptagfld ) > pbRecMax
            && ( ptagfld->cbData & wCorruptBit ) )
        {
            ptagfld->cbData &= ~wCorruptBit;
            printf( "\nReset corrupt bit in Cb of TAGFLD." );
        }

        if ( ptagfld->fid < ptagfldPrev->fid )
        {
            BYTE    rgb[g_cbPageMax];
            ULONG   cbCopy          = sizeof(TAGFLD)+ptagfldPrev->cb;
            BYTE    *pbNext         = (BYTE *)PtagfldNext( ptagfld );

            Assert( cbCopy-sizeof(TAGFLD) <= cbLVIntrinsicMost );
            UtilMemCpy( rgb, ptagfldPrev, cbCopy );
            memmove( ptagfldPrev, ptagfld, sizeof(TAGFLD)+ptagfld->cb );

            ptagfld = PtagfldNext( ptagfldPrev );
            UtilMemCpy( ptagfld, rgb, cbCopy );
            Assert( PtagfldNext( ptagfld ) == m_pbNext );
            printf( "\nFixed TAGFLD out of order." );

            goto CheckTagged;
        }
        ptagfldPrev = ptagfld;

        Assert( (BYTE *)PtagfldNext( ptagfld ) <= pbRecMax );
    }
    Assert( (BYTE *)ptagfld == pbRecMax );
#endif
}


LOCAL ERR ErrSORTCopyOneRecord(
    FUCB            * pfucbSrc,
    FUCB            * pfucbDest,
    BOOKMARK        * const pbmPrimary,
    const INT       fColumnsDeleted,
    BYTE            * pbRecBuf,
    CPCOL           * rgcpcol,
    ULONG           ccpcolMax,
    JET_COLUMNID    * mpcolumnidcolumnidTagged,
    STATUSINFO      * pstatus,
    _Inout_ QWORD   * pqwAutoIncMax )
{
    ERR             err             = JET_errSuccess;
    BYTE            * pbRecSrc      = 0;
    BYTE            * pbRecDest     = 0;
    ULONG           cbRecSrc;
    SIZE_T          cbRecSrcFixedVar;
    SIZE_T          cbRecDestFixedVar;
#ifdef DEBUG
    FID             fidAutoInc;
#endif

    Assert( Pcsr( pfucbSrc )->FLatched() );
    CallS( ErrDIRRelease( pfucbSrc ) );

    Assert( NULL != pfucbDest->pvWorkBuf );
    Assert( pfucbDest->dataWorkBuf.Pv() == pfucbDest->pvWorkBuf );

    Call( ErrIsamPrepareUpdate( pfucbDest->ppib, pfucbDest, JET_prepInsert ) );

    Assert( !Pcsr( pfucbSrc )->FLatched() );
    Call( ErrDIRGet( pfucbSrc ) );

    pbRecSrc = (BYTE *)pfucbSrc->kdfCurr.data.Pv();
    cbRecSrc = pfucbSrc->kdfCurr.data.Cb();
    pbRecDest = (BYTE *)pfucbDest->dataWorkBuf.Pv();

    if ( pfucbSrc->u.pfcb->Ptdb()->FidAutoincrement() != 0 )
    {
        const TDB * const ptdbSrc = pfucbSrc->u.pfcb->Ptdb();
        Assert( ptdbSrc->FidAutoincrement() );
        const BOOL f8BytesAutoInc = ptdbSrc->F8BytesAutoInc();
        DATA dataAutoInc;

        if ( ErrRECRetrieveNonTaggedColumn(
                pfucbSrc->u.pfcb,
                ColumnidOfFid( ptdbSrc->FidAutoincrement(), ptdbSrc->FFixedTemplateColumn( ptdbSrc->FidAutoincrement() ) ),
                pfucbSrc->kdfCurr.data,
                &dataAutoInc,
                pfieldNil ) >= JET_errSuccess )
        {
            if ( (QWORD)dataAutoInc.Cb() < ( f8BytesAutoInc ? sizeof(QWORD) : sizeof(DWORD) ) )
            {
                AssertSz( fFalse, "Retrieved fixed column size didn't match the schema's claimed auto-inc size." );
                Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }
            const QWORD qwCurrAutoInc = f8BytesAutoInc ?
                                            ( *( (UnalignedLittleEndian< QWORD > *)dataAutoInc.Pv() ) ) :
                                            ( (QWORD)( *( (UnalignedLittleEndian< ULONG > *)dataAutoInc.Pv() ) ) );
            if ( *pqwAutoIncMax < qwCurrAutoInc )
            {
                *pqwAutoIncMax = qwCurrAutoInc;
            }
        }
    }

    Assert( cbRecSrc >= REC::cbRecordMin );
    Assert( cbRecSrc <= (SIZE_T)REC::CbRecordMostCHECK( g_rgfmp[ pfucbDest->ifmp ].CbPage() ) );
    Assert( cbRecSrc <= (SIZE_T)REC::CbRecordMost( pfucbDest ) );
    if ( cbRecSrc > (SIZE_T)REC::CbRecordMost( pfucbDest ) )
    {
        FireWall( "SORTCopyRecTooBig10.2" );
    }
    if ( cbRecSrc < REC::cbRecordMin || cbRecSrc > (SIZE_T)REC::CbRecordMostCHECK( g_rgfmp[ pfucbDest->ifmp ].CbPage() ) )
    {
        FireWall( "SORTCopyRecTooBig10.1" );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    Assert( ( (REC *)pbRecSrc )->FidFixedLastInRec() <= pfucbSrc->u.pfcb->Ptdb()->FidFixedLast() );
    Assert( ( (REC *)pbRecSrc )->FidVarLastInRec() <= pfucbSrc->u.pfcb->Ptdb()->FidVarLast() );

    cbRecSrcFixedVar = ( (REC *)pbRecSrc )->PbTaggedData() - pbRecSrc;
    Assert( cbRecSrcFixedVar >= REC::cbRecordMin );
    Assert( cbRecSrcFixedVar <= cbRecSrc );
    Assert( cbRecSrcFixedVar <= (SIZE_T)REC::CbRecordMostCHECK( g_rgfmp[ pfucbDest->ifmp ].CbPage() ) );
    Assert( cbRecSrcFixedVar <= (SIZE_T)REC::CbRecordMost( pfucbDest ) );
    if ( cbRecSrcFixedVar > (SIZE_T)REC::CbRecordMost( pfucbDest ) )
    {
        FireWall( "SORTCopyRecTooBig11.2" );
    }
    if ( cbRecSrcFixedVar < REC::cbRecordMin
        || cbRecSrcFixedVar > cbRecSrc
        || cbRecSrcFixedVar > (SIZE_T)REC::CbRecordMostCHECK( g_rgfmp[ pfucbDest->ifmp ].CbPage() ) )
    {
        FireWall( "SORTCopyRecTooBig11.1" );
        Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    SORTCheckVarTaggedCols( (REC *)pbRecSrc, cbRecSrc, pfucbSrc->u.pfcb->Ptdb() );

    if ( FCOLSDELETEDNone( fColumnsDeleted ) )
    {
        UtilMemCpy( pbRecDest, pbRecSrc, cbRecSrc );
        pfucbDest->dataWorkBuf.SetCb( cbRecSrc );
        cbRecDestFixedVar = cbRecSrcFixedVar;
    }
    else
    {
        if ( FCOLSDELETEDFixedVar( fColumnsDeleted ) )
        {
            cbRecDestFixedVar = CbSORTCopyFixedVarColumns(
                                        pfucbSrc->u.pfcb->Ptdb(),
                                        pfucbDest->u.pfcb->Ptdb(),
                                        rgcpcol,
                                        ccpcolMax,
                                        pbRecSrc,
                                        pbRecDest );
        }

        else
        {
            UtilMemCpy( pbRecDest, pbRecSrc, cbRecSrcFixedVar );
            cbRecDestFixedVar = cbRecSrcFixedVar;
        }

        Assert( cbRecDestFixedVar >= REC::cbRecordMin );
        Assert( cbRecDestFixedVar <= cbRecSrcFixedVar );

        if ( FCOLSDELETEDTagged( fColumnsDeleted ) )
        {
            pfucbDest->dataWorkBuf.SetCb( cbRecDestFixedVar );
            Call( ErrSORTCopyTaggedColumns(
                pfucbSrc,
                pfucbDest,
                pbRecBuf,
                mpcolumnidcolumnidTagged,
                pstatus ) );
            AssertDIRNoLatch( pfucbSrc->ppib );

            Assert( (SIZE_T)pfucbDest->dataWorkBuf.Cb() >= cbRecDestFixedVar );
            Assert( (SIZE_T)pfucbDest->dataWorkBuf.Cb() <= cbRecSrc );

            goto InsertRecord;
        }
        else
        {
            UtilMemCpy(
                pbRecDest+cbRecDestFixedVar,
                pbRecSrc+cbRecSrcFixedVar,
                cbRecSrc - cbRecSrcFixedVar );
            pfucbDest->dataWorkBuf.SetCb( cbRecDestFixedVar + ( cbRecSrc - cbRecSrcFixedVar ) );

            Assert( (SIZE_T)pfucbDest->dataWorkBuf.Cb() >= cbRecDestFixedVar );
            Assert( (SIZE_T)pfucbDest->dataWorkBuf.Cb() <= cbRecSrc );
        }
    }

    Assert( Pcsr( pfucbSrc )->FLatched() );
    CallS( ErrDIRRelease( pfucbSrc ) );

    Call( ErrSORTUpdateSeparatedLVs(
        pfucbSrc,
        pfucbDest,
        mpcolumnidcolumnidTagged,
        pstatus ) );

InsertRecord:
    if ( pstatus != NULL )
    {
        const FID   fidFixedLast    = ( (REC *)pbRecDest )->FidFixedLastInRec();
        const FID   fidVarLast      = ( (REC *)pbRecDest )->FidVarLastInRec();

        Assert( fidFixedLast >= fidFixedLeast-1 );
        Assert( fidFixedLast <= pfucbDest->u.pfcb->Ptdb()->FidFixedLast() );
        Assert( fidVarLast >= fidVarLeast-1 );
        Assert( fidVarLast <= pfucbDest->u.pfcb->Ptdb()->FidVarLast() );

        const INT   cbOverhead =
                        ibRECStartFixedColumns
                        + ( ( fidFixedLast + 1 - fidFixedLeast ) + 7 ) / 8
                        + ( fidVarLast + 1 - fidVarLeast ) * sizeof(WORD);
        Assert( cbRecDestFixedVar >= (SIZE_T)cbOverhead );

        pstatus->cbRawData += ( cbRecDestFixedVar - cbOverhead );
    }

#ifdef DEBUG
    BOOL f8BytesAutoInc;
    fidAutoInc = pfucbDest->u.pfcb->Ptdb()->FidAutoincrement();
    f8BytesAutoInc = pfucbDest->u.pfcb->Ptdb()->F8BytesAutoInc();
    if ( fidAutoInc != 0 )
    {
        Assert( FFixedFid( fidAutoInc ) );
        Assert( fidAutoInc <= pfucbDest->u.pfcb->Ptdb()->FidFixedLast() );
        Assert( FFUCBColumnSet( pfucbDest, fidAutoInc ) );
        
        pfucbDest->u.pfcb->Ptdb()->ResetFidAutoincrement();
    }
    
    err = ErrRECInsert( pfucbDest, pbmPrimary );
    
    if ( fidAutoInc != 0 )
    {
        pfucbDest->u.pfcb->Ptdb()->SetFidAutoincrement( fidAutoInc, f8BytesAutoInc );
    }
        
    Call( err );

#else
    Call( ErrRECInsert( pfucbDest, pbmPrimary ) );
#endif

HandleError:
    Assert( NULL != pfucbDest->pvWorkBuf || err < 0 );

    return err;
}


INLINE VOID SORTAssertColumnidMaps(
    TDB             *ptdb,
    CPCOL           *rgcpcol,
    ULONG           ccpcolMax,
    JET_COLUMNID    *mpcolumnidcolumnidTagged,
    const INT       fColumnsDeleted,
    const BOOL      fTemplateTable )
{
#ifdef DEBUG

    INT i;
    if ( FCOLSDELETEDFixedVar( fColumnsDeleted ) )
    {
        for ( i = 0; i < (INT)ccpcolMax; i++ )
        {
            if ( FCOLUMNIDTemplateColumn( rgcpcol[i].columnidSrc ) )
            {
                Assert( fTemplateTable );
                Assert( FCOLUMNIDTemplateColumn( rgcpcol[i].columnidDest ) );
            }
            else
            {
                Assert( !fTemplateTable );
                Assert( !FCOLUMNIDTemplateColumn( rgcpcol[i].columnidDest ) );
            }

            Assert( FidOfColumnid( rgcpcol[i].columnidDest ) <= FidOfColumnid( rgcpcol[i].columnidSrc ) );
            if ( FCOLUMNIDFixed( rgcpcol[i].columnidSrc ) )
            {
                Assert( FCOLUMNIDFixed( rgcpcol[i].columnidDest ) );
                if ( i > 0 )
                {
                    Assert( rgcpcol[i].columnidDest == rgcpcol[i-1].columnidDest + 1 );
                }
            }
            else
            {
                Assert( FCOLUMNIDVar( rgcpcol[i].columnidSrc ) );
                Assert( FCOLUMNIDVar( rgcpcol[i].columnidDest ) );
                if ( i > 0 )
                {
                    if ( FCOLUMNIDVar( rgcpcol[i-1].columnidDest ) )
                    {
                        Assert( rgcpcol[i].columnidDest == rgcpcol[i-1].columnidDest + 1 );
                    }
                    else
                    {
                        Assert( FCOLUMNIDFixed( rgcpcol[i-1].columnidDest ) );
                    }
                }
            }
        }
    }
    else
    {
        for ( i = 0; i < (INT)ccpcolMax; i++ )
        {
            Assert( rgcpcol[i].columnidDest == rgcpcol[i].columnidSrc );

            if ( FCOLUMNIDFixed( rgcpcol[i].columnidSrc ) )
            {
                Assert( i == 0 ?
                    FidOfColumnid( rgcpcol[i].columnidDest ) == ptdb->FidFixedFirst() :
                    rgcpcol[i].columnidDest == rgcpcol[i-1].columnidDest + 1 );
            }
            else
            {
                Assert( FCOLUMNIDVar( rgcpcol[i].columnidSrc ) );
                if ( i == 0 )
                {
                    Assert( FidOfColumnid( rgcpcol[i].columnidDest ) == ptdb->FidVarFirst() );
                    Assert( ptdb->FidFixedLast() == ptdb->FidFixedFirst() - 1 );
                }
                else if ( FCOLUMNIDVar( rgcpcol[i-1].columnidDest ) )
                {
                    Assert( rgcpcol[i].columnidDest == rgcpcol[i-1].columnidDest + 1 );
                }
                else
                {
                    Assert( FidOfColumnid( rgcpcol[i].columnidDest ) == ptdb->FidVarFirst() );
                    Assert( FidOfColumnid( rgcpcol[i-1].columnidDest ) == ptdb->FidFixedLast() );
                }
            }
        }
    }


    FID         fidT        = ptdb->FidTaggedFirst();
    const FID   fidLast     = ptdb->FidTaggedLast();
    if ( FCOLSDELETEDTagged( fColumnsDeleted ) )
    {
        for ( ; fidT <= fidLast; fidT++ )
        {
            const FIELD *pfieldTagged = ptdb->PfieldTagged( ColumnidOfFid( fidT, fTemplateTable ) );
            if ( pfieldTagged->coltyp != JET_coltypNil )
            {
                Assert( FCOLUMNIDTagged( mpcolumnidcolumnidTagged[fidT-fidTaggedLeast] ) );
                Assert( FidOfColumnid( mpcolumnidcolumnidTagged[fidT-fidTaggedLeast] ) <= fidT );
            }
        }
    }
    else
    {
        for ( ; fidT <= fidLast; fidT++ )
        {
            Assert( ptdb->PfieldTagged( ColumnidOfFid( fidT, fTemplateTable ) )->coltyp != JET_coltypNil );
            if ( fidT > ptdb->FidTaggedFirst() )
            {
                Assert( mpcolumnidcolumnidTagged[fidT-fidTaggedLeast]
                        == mpcolumnidcolumnidTagged[fidT-fidTaggedLeast-1] + 1 );
            }
            Assert( FidOfColumnid( mpcolumnidcolumnidTagged[fidT-fidTaggedLeast] ) == fidT );
        }
    }

#endif
}


ERR ErrSORTCopyRecords(
    PIB             *ppib,
    FUCB            *pfucbSrc,
    FUCB            *pfucbDest,
    CPCOL           *rgcpcol,
    ULONG           ccpcolMax,
    LONG            crecMax,
    ULONG           *pcsridCopied,
    _Out_ QWORD     *pqwAutoIncMax,
    BYTE            *pbLVBuf,
    size_t          cbLVBuf,
    JET_COLUMNID    *mpcolumnidcolumnidTagged,
    STATUSINFO      *pstatus )
{
    ERR             err                     = JET_errSuccess;
    TDB             *ptdb                   = ptdbNil;
    INT             fColumnsDeleted         = 0;
    LONG            dsrid                   = 0;
    BYTE            *pbRecBuf               = NULL;
    VOID            *pvWorkBuf              = NULL;
    BOOL            fDoAll                  = ( 0 == crecMax );
    PGNO            pgnoCurrPage;
    FCB             *pfcbSecondaryIndexes   = pfcbNil;
    const BOOL      fOldSeq                 = FFUCBSequential( pfucbSrc );
    ULONG           ibDeleteOnZeroColumn    = 0;
    BOOL            fInTrx                  = fFalse;
    FCB             *pfcbNextIndexBatch     = pfcbNil;
    BOOL            fBuildIndexWhileCopying = fFalse;
    FUCB            *rgpfucbSort[cFILEIndexBatchSizeDefault];
    ULONG           rgcRecInput[cFILEIndexBatchSizeDefault];
    BOOKMARK        *pbmPrimary             = NULL;
    BOOKMARK        bmPrimary;
    KEY             keyBuffer;
    BYTE            *pbPrimaryKey           = NULL;
    BYTE            *pbSecondaryKey         = NULL;
    ULONG           cIndexesToBuild         = 0;
    ULONG           iindex;

    Assert( pqwAutoIncMax );
    *pqwAutoIncMax = 0;

    Assert( !g_rgfmp[pfucbSrc->ifmp].Plidmap() );
    err = ErrCMPCopyLVTree(
                ppib,
                pfucbSrc,
                pfucbDest,
                pbLVBuf,
                cbLVBuf,
                pstatus );
    if ( err < 0 )
    {
        if ( g_rgfmp[pfucbSrc->ifmp].Plidmap() )
        {
            LIDMAP* const plidmap = g_rgfmp[pfucbSrc->ifmp].Plidmap();
            Assert( JET_tableidNil != plidmap->Tableid() );
            CallS( plidmap->ErrTerm( ppib ) );
            Assert( JET_tableidNil == plidmap->Tableid() );
            delete plidmap;
            g_rgfmp[pfucbSrc->ifmp].SetLidmap( NULL );
        }
        return err;
    }

    FUCBSetSequential( pfucbSrc );


    BFAlloc( bfasIndeterminate, (VOID **)&pbRecBuf );
    Assert ( NULL != pbRecBuf );
    Assert( NULL != pbLVBuf );

    Assert( NULL == pfucbDest->pvWorkBuf );
    RECIAllocCopyBuffer( pfucbDest );
    pvWorkBuf = pfucbDest->pvWorkBuf;
    Assert ( NULL != pvWorkBuf );

    Assert( ppib == pfucbSrc->ppib );
    Assert( ppib == pfucbDest->ppib );


    ptdb = pfucbSrc->u.pfcb->Ptdb();

    if ( ptdb->FTableHasDeleteOnZeroColumn() )
    {
        for ( FID fid = fidFixedLeast; fid <= ptdb->FidFixedLast(); ++fid )
        {
            const BOOL          fTemplateColumn = ptdb->FFixedTemplateColumn( fid );
            const COLUMNID      columnid        = ColumnidOfFid( fid, fTemplateColumn );
            const FIELD * const pfield          = ptdb->PfieldFixed( columnid );

            if ( FFIELDDeleteOnZero( pfield->ffield ) && !FFIELDDeleted( pfield->ffield ) )
            {
                Assert( FFIELDEscrowUpdate( pfield->ffield ) );
                Assert( pfield->ibRecordOffset >= REC::cbRecordMin );
                ibDeleteOnZeroColumn = pfield->ibRecordOffset;
                break;
            }
        }
    }

    FCOLSDELETEDSetNone( fColumnsDeleted );

    Assert( ccpcolMax <=
        (ULONG)( ( ptdb->FidFixedLast() + 1 - ptdb->FidFixedFirst() )
                    + ( ptdb->FidVarLast() + 1 - ptdb->FidVarFirst() ) ) );
    if ( ccpcolMax < (ULONG)( ( ptdb->FidFixedLast() + 1 - ptdb->FidFixedFirst() )
                                + ( ptdb->FidVarLast() + 1 - ptdb->FidVarFirst() ) ) )
    {
        FCOLSDELETEDSetFixedVar( fColumnsDeleted );
    }

    extern BOOL g_fCompactTemplateTableColumnDropped;
    if( g_fCompactTemplateTableColumnDropped && pfcbNil != ptdb->PfcbTemplateTable() )
    {
        FCOLSDELETEDSetFixedVar( fColumnsDeleted );
    }

    
    FID         fidT        = ptdb->FidTaggedFirst();
    const FID   fidLast     = ptdb->FidTaggedLast();
    Assert( fidLast == fidT-1 || FTaggedFid( fidLast ) );

    if ( ptdb->FESE97DerivedTable() )
    {
        FCOLSDELETEDSetTagged( fColumnsDeleted );
    }
    else
    {
        for ( ; fidT <= fidLast; fidT++ )
        {
            const FIELD *pfieldTagged = ptdb->PfieldTagged( ColumnidOfFid( fidT, ptdb->FTemplateTable() ) );
            if ( pfieldTagged->coltyp == JET_coltypNil )
            {
                FCOLSDELETEDSetTagged( fColumnsDeleted );
                break;
            }
        }
    }

    SORTAssertColumnidMaps(
        ptdb,
        rgcpcol,
        ccpcolMax,
        mpcolumnidcolumnidTagged,
        fColumnsDeleted,
        ptdb->FTemplateTable() );

    Assert( crecMax >= 0 );

    Call( ErrDIRBeginTransaction( ppib, 50213, NO_GRBIT ) );
    fInTrx = fTrue;

    DIRBeforeFirst( pfucbSrc );
    err = ErrDIRNext( pfucbSrc, fDIRNull );
    if ( err < 0 )
    {
        Assert( JET_errRecordNotFound != err );
        if ( JET_errNoCurrentRecord == err )
            err = JET_errSuccess;
        goto HandleError;
    }

    pfcbSecondaryIndexes = pfucbDest->u.pfcb->PfcbNextIndex();
    pfucbDest->u.pfcb->SetPfcbNextIndex( pfcbNil );

    if ( pfcbNil != pfcbSecondaryIndexes )
    {
        for ( iindex = 0; iindex < cFILEIndexBatchSizeDefault; iindex++ )
            rgpfucbSort[iindex] = pfucbNil;

        Call( ErrFILEIndexBatchInit(
                    ppib,
                    rgpfucbSort,
                    pfcbSecondaryIndexes,
                    &cIndexesToBuild,
                    rgcRecInput,
                    &pfcbNextIndexBatch,
                    cFILEIndexBatchSizeDefault ) );

#ifdef PARALLEL_BATCH_INDEX_BUILD
        fBuildIndexWhileCopying = ( pfcbNil == pfcbNextIndexBatch );
#else
        fBuildIndexWhileCopying = fTrue;
#endif

        if ( fBuildIndexWhileCopying )
        {
            Alloc( pbPrimaryKey = (BYTE *)RESKEY.PvRESAlloc() );
            Alloc( pbSecondaryKey = (BYTE *)RESKEY.PvRESAlloc() );

            bmPrimary.key.prefix.Nullify();
            bmPrimary.key.suffix.SetCb( cbKeyAlloc );
            bmPrimary.key.suffix.SetPv( pbPrimaryKey );
            bmPrimary.data.Nullify();
            pbmPrimary = &bmPrimary;

            keyBuffer.prefix.Nullify();
            keyBuffer.suffix.SetCb( cbKeyAlloc );
            keyBuffer.suffix.SetPv( pbSecondaryKey );
        }
        else
        {
#ifdef PARALLEL_BATCH_INDEX_BUILD
            for ( iindex = 0; iindex < cFILEIndexBatchSizeDefault; iindex++ )
            {
                Assert( pfucbNil != rgpfucbSort[iindex] );
                SORTClose( rgpfucbSort[iindex] );
                rgpfucbSort[iindex] = pfucbNil;
            }
#endif
        }
    }

    pgnoCurrPage = Pcsr( pfucbSrc )->Pgno();
    forever
    {
        Assert( Pcsr( pfucbSrc )->FLatched() );

        Assert( 0 == ibDeleteOnZeroColumn
            || pfucbSrc->kdfCurr.data.Cb() > LONG( ibDeleteOnZeroColumn + sizeof(LONG) ) );

        if ( 0 != ibDeleteOnZeroColumn
            && 0 == *(UnalignedLittleEndian< ULONG > *)( (BYTE *)pfucbSrc->kdfCurr.data.Pv() + ibDeleteOnZeroColumn ) )
        {
            CallS( ErrDIRRelease( pfucbSrc ) );
            Assert( !Pcsr( pfucbDest )->FLatched() );
        }
        else
        {
            err = ErrSORTCopyOneRecord(
                        pfucbSrc,
                        pfucbDest,
                        pbmPrimary,
                        fColumnsDeleted,
                        pbRecBuf,
                        rgcpcol,
                        ccpcolMax,
                        mpcolumnidcolumnidTagged,
                        pstatus,
                        pqwAutoIncMax );
            if ( err < 0 )
            {
                if ( g_fRepair )
                {
                    UtilReportEvent( eventWarning, REPAIR_CATEGORY, REPAIR_BAD_RECORD_ID, 0, NULL );
                }
                else
                {
                    goto HandleError;
                }
            }

            else
            {
                Assert( !Pcsr( pfucbSrc )->FLatched() );
                Assert( !Pcsr( pfucbDest )->FLatched() );

                Assert( pfucbDest->dataWorkBuf.Pv() == pvWorkBuf );
                Assert( pfucbDest->dataWorkBuf.Cb() >= REC::cbRecordMin );
                Assert( pfucbDest->dataWorkBuf.Cb() <= REC::CbRecordMostCHECK( g_rgfmp[ pfucbDest->ifmp ].CbPage() ) );
                Assert( pfucbDest->dataWorkBuf.Cb() <= REC::CbRecordMost( pfucbDest ) );

                if ( fBuildIndexWhileCopying )
                {
                    Assert( pfcbNil != pfcbSecondaryIndexes );
                    Assert( cIndexesToBuild > 0 );
                    Assert( cIndexesToBuild <= cFILEIndexBatchSizeDefault );
                    Assert( pbmPrimary == &bmPrimary );
                    Call( ErrFILEIndexBatchAddEntry(
                                rgpfucbSort,
                                pfucbDest,
                                pbmPrimary,
                                pfucbDest->dataWorkBuf,
                                pfcbSecondaryIndexes,
                                cIndexesToBuild,
                                rgcRecInput,
                                keyBuffer ) );
                }
            }
        }

        dsrid++;

        

        if ( !fDoAll && --crecMax == 0 )
            break;

        err = ErrDIRNext( pfucbSrc, fDIRNull );
        if ( err < 0 )
        {
            Assert( JET_errRecordNotFound != err );
            if ( err != JET_errNoCurrentRecord  )
                goto HandleError;

            if ( pstatus != NULL )
            {
                pstatus->cLeafPagesTraversed++;
                Call( ErrSORTCopyProgress( pstatus, 1 ) );
            }
            err = JET_errSuccess;
            break;
        }

        else if ( pstatus != NULL && pgnoCurrPage != Pcsr( pfucbSrc )->Pgno() )
        {
            pgnoCurrPage = Pcsr( pfucbSrc )->Pgno();
            pstatus->cLeafPagesTraversed++;
            Call( ErrSORTCopyProgress( pstatus, 1 ) );
        }
    }

    Assert( fInTrx );
    Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
    fInTrx = fFalse;

    if ( g_rgfmp[pfucbSrc->ifmp].Plidmap() )
    {
        Call( g_rgfmp[pfucbSrc->ifmp].Plidmap()->ErrUpdateLVRefcounts( ppib, pfucbDest ) );
    }


    Assert( pfucbDest->u.pfcb->PfcbNextIndex() == pfcbNil );
    pfucbDest->u.pfcb->SetPfcbNextIndex( pfcbSecondaryIndexes );

    if ( pfcbNil != pfcbSecondaryIndexes )
    {
        if ( pstatus )
        {
            Assert( pstatus->cSecondaryIndexes > 0 );

            Assert( pstatus->cunitPerProgression > 0 || g_fRepair );
            if ( pstatus->cunitPerProgression > 0 )
            {
                Assert( pstatus->cunitDone <= pstatus->cunitProjected );
                Assert( pstatus->cunitProjected <= pstatus->cunitTotal );
                const ULONG cpgRemaining = pstatus->cunitProjected - pstatus->cunitDone;

                Assert( cpgRemaining >= pstatus->cSecondaryIndexes );

                pstatus->cunitPerProgression = cpgRemaining / pstatus->cSecondaryIndexes;
                Assert( pstatus->cunitPerProgression >= 1 );
                Assert( pstatus->cunitPerProgression * pstatus->cSecondaryIndexes <= cpgRemaining );
            }
        }

#ifdef PARALLEL_BATCH_INDEX_BUILD
        if ( fBuildIndexWhileCopying )
        {
            Call( ErrFILEIndexBatchTerm(
                        ppib,
                        rgpfucbSort,
                        pfcbSecondaryIndexes,
                        cIndexesToBuild,
                        rgcRecInput,
                        pstatus ) );
        }
        else
        {
            ULONG   cIndexes    = 0;
            for ( FCB * pfcbT = pfcbSecondaryIndexes; pfcbNil != pfcbT; pfcbT = pfcbT->PfcbNextIndex() )
            {
                cIndexes++;
            }
            Assert( cIndexes > cFILEIndexBatchSizeDefault );

            Call( ErrFILEBuildAllIndexes(
                        ppib,
                        pfucbDest,
                        pfcbSecondaryIndexes,
                        pstatus,
                        cIndexes ) );
        }
#else
        Call( ErrFILEIndexBatchTerm(
                    ppib,
                    rgpfucbSort,
                    pfcbSecondaryIndexes,
                    cIndexesToBuild,
                    rgcRecInput,
                    pstatus ) );

        if ( pfcbNil != pfcbNextIndexBatch )
        {
            Assert( cFILEIndexBatchSizeDefault == cIndexesToBuild );
            Call( ErrFILEBuildAllIndexes(
                            ppib,
                            pfucbDest,
                            pfcbNextIndexBatch,
                            pstatus,
                            cFILEIndexBatchSizeDefault ) );
        }
#endif
    }

HandleError:
    if ( pcsridCopied )
        *pcsridCopied = dsrid;

    if ( err < 0 && pfcbNil != pfcbSecondaryIndexes )
    {
        for ( iindex = 0; iindex < cFILEIndexBatchSizeDefault; iindex++ )
        {
            if ( pfucbNil != rgpfucbSort[iindex] )
            {
                SORTClose( rgpfucbSort[iindex] );
                rgpfucbSort[iindex] = pfucbNil;
            }
        }

        pfucbDest->u.pfcb->SetPfcbNextIndex( pfcbSecondaryIndexes );
    }
#ifdef DEBUG
    else if ( pfcbNil != pfcbSecondaryIndexes )
    {
        for ( iindex = 0; iindex < cFILEIndexBatchSizeDefault; iindex++ )
        {
            Assert( pfucbNil == rgpfucbSort[iindex] );
        }
    }
#endif

    if ( g_rgfmp[pfucbSrc->ifmp].Plidmap() )
    {
        LIDMAP* const plidmap = g_rgfmp[pfucbSrc->ifmp].Plidmap();
        Assert( JET_tableidNil != plidmap->Tableid() );
        CallS( plidmap->ErrTerm( ppib ) );
        Assert( JET_tableidNil == plidmap->Tableid() );
        delete plidmap;
        g_rgfmp[pfucbSrc->ifmp].SetLidmap( NULL );
    }

    if ( fInTrx )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    Assert( NULL != pvWorkBuf );
    Assert( pvWorkBuf == pfucbDest->pvWorkBuf
        || ( err < 0 && NULL == pfucbDest->pvWorkBuf ) );
    Assert( pfucbDest->dataWorkBuf.Pv() == pfucbDest->pvWorkBuf );
    RECIFreeCopyBuffer( pfucbDest );

    Assert ( NULL != pbRecBuf );
    BFFree( pbRecBuf );

    RESKEY.Free( pbPrimaryKey );
    RESKEY.Free( pbSecondaryKey );

    if ( !fOldSeq )
        FUCBResetSequential( pfucbSrc );

    return err;
}


INLINE ULONG ulSORTTempNameGen( VOID )
{
    static ULONG ulTempNum = 0;
    return ulTempNum++;
}

LOCAL ERR ErrIsamSortMaterialize( PIB * const ppib, FUCB * const pfucbSort, const BOOL fIndex )
{
    ERR     err;
    FUCB    *pfucbTable         = pfucbNil;
    FCB     *pfcbTable          = pfcbNil;
    TDB     *ptdbTable          = ptdbNil;
    FCB     *pfcbSort           = pfcbNil;
    TDB     *ptdbSort           = ptdbNil;
    MEMPOOL mempoolSave;
    CHAR    szName[JET_cbNameMost + 1];

    JET_TABLECREATE5_A tablecreate  = {
        sizeof(JET_TABLECREATE5_A),
        szName,
        NULL,
        16,
        100,
        NULL, 0, NULL, 0, NULL, 0,
        NO_GRBIT,
        NULL,
        NULL,
        0,
        0,
        JET_TABLEID( pfucbNil ),
        0 };

    CallR( ErrPIBCheck( ppib ) );
    CheckSort( ppib, pfucbSort );

    Assert( ppib->Level() < levelMax );
    Assert( pfucbSort->ppib == ppib );
    Assert( !( FFUCBIndex( pfucbSort ) ) );

    if ( pfucbSort->u.pscb->grbit & JET_bitTTIntrinsicLVsOnly )
    {
        tablecreate.cbSeparateLV = g_cbPage;
    }

    INST *pinst = PinstFromPpib( ppib );
    CallR( ErrPIBOpenTempDatabase( ppib ) );
    
    
    if ( FSCBInsert( pfucbSort->u.pscb ) )
    {
        Assert( 0 == pfucbSort->u.pscb->cRecords );
        CallR( ErrSORTEndInsert( pfucbSort ) );
    }


    
    OSStrCbFormatA( szName, sizeof(szName), "TEMP%lu", ulSORTTempNameGen() );

    CallR( ErrEnsureTempDatabaseOpened( pinst, ppib ) );

    
    CallR( ErrFILECreateTable( ppib, pinst->m_mpdbidifmp[ dbidTemp ], &tablecreate ) );
    pfucbTable = (FUCB *)( tablecreate.tableid );
    Assert( pfucbNil != pfucbTable );
    
    Assert( tablecreate.cCreated == 1 );

    Call( pfucbTable->u.pfcb->ErrLinkReserveSpace() );

    
    const INT   fDIRFlags = ( fDIRNoVersion|fDIRAppend );
    DIRGotoRoot( pfucbTable );
    Call( ErrDIRInitAppend( pfucbTable ) );

    pfcbSort = &pfucbSort->u.pscb->fcb;
    Assert( pfcbSort->FTypeSort() );

    ptdbSort = pfcbSort->Ptdb();
    Assert( ptdbNil != ptdbSort );

    pfcbTable = pfucbTable->u.pfcb;
    Assert( pfcbTable->FTypeTemporaryTable() );

    ptdbTable = pfcbTable->Ptdb();
    Assert( ptdbNil != ptdbTable );

    err = ErrSORTFirst( pfucbSort );

    if ( fIndex )
    {
        while ( err >= 0 )
        {
            Call( ErrDIRAppend(
                        pfucbTable,
                        pfucbSort->kdfCurr.key,
                        pfucbSort->kdfCurr.data,
                        fDIRFlags ) );

            Assert( Pcsr( pfucbTable )->FLatched() );

            err = ErrSORTNext( pfucbSort );
        }
    }
    else
    {
        KEY     key;
        DBK     dbk = 0;
        BYTE    rgb[4];

        key.prefix.Nullify();
        key.suffix.SetPv( rgb );
        key.suffix.SetCb( sizeof(DBK) );

        while ( err >= 0 )
        {
            rgb[0] = (BYTE)(dbk >> 24);
            rgb[1] = (BYTE)((dbk >> 16) & 0xff);
            rgb[2] = (BYTE)((dbk >> 8) & 0xff);
            rgb[3] = (BYTE)(dbk & 0xff);
            dbk++;

            Call( ErrDIRAppend(
                pfucbTable,
                key,
                pfucbSort->kdfCurr.data,
                fDIRFlags ) );
            err = ErrSORTNext( pfucbSort );
        }

        dbk++;

        Assert( ptdbTable->DbkMost() == 0 );
        ptdbTable->InitDbkMost( dbk );
        Assert( ptdbTable->DbkMost() == dbk );
    }

    Assert( err < 0 );
    if ( err != JET_errNoCurrentRecord )
        goto HandleError;

    Call( ErrDIRTermAppend( pfucbTable ) );

    Assert( pfcbTable->PfcbNextIndex() == pfcbNil );
    Assert( FFMPIsTempDB( pfcbTable->Ifmp() ) );
    pfcbTable->SetSpaceHints( PSystemSpaceHints(eJSPHDefaultHeavyWater) );


    Assert( pfcbTable->FDomainDenyReadByUs( ppib ) );
    Assert( pfcbTable->FTypeTemporaryTable() );
    Assert( pfcbTable->FPrimaryIndex() );
    Assert( pfcbTable->FInitialized() );
    Assert( pfcbTable->ErrErrInit() == JET_errSuccess );
    Assert( pfcbTable->FInList() );

    pfcbTable->ResetFlags();

    pfcbTable->SetDomainDenyRead( ppib );

    pfcbTable->Lock();

    pfcbTable->SetTypeTemporaryTable();
    pfcbTable->SetFixedDDL();
    pfcbTable->SetPrimaryIndex();
    pfcbTable->CreateComplete();
    pfcbTable->SetInList();

    if ( pfucbSort->u.pscb->grbit & JET_bitTTIntrinsicLVsOnly )
    {
        Assert( ptdbTable->CbPreferredIntrinsicLV() == (ULONG)g_cbPage );
        pfcbTable->SetIntrinsicLVsOnly();
    }
    pfcbTable->Unlock();

    Assert( ptdbSort != ptdbNil );
    Assert( ptdbSort->PfcbTemplateTable() == pfcbNil );
    Assert( ptdbSort->IbEndFixedColumns() >= ibRECStartFixedColumns );
    Assert( ptdbTable != ptdbNil );
    Assert( ptdbTable->FidFixedLast() == fidFixedLeast-1 );
    Assert( ptdbTable->FidVarLast() == fidVarLeast-1 );
    Assert( ptdbTable->FidTaggedLast() == fidTaggedLeast-1 );
    Assert( ptdbTable->IbEndFixedColumns() == ibRECStartFixedColumns );
    Assert( ptdbTable->FidVersion() == 0 );
    Assert( ptdbTable->FidAutoincrement() == 0 );
    Assert( pfieldNil == ptdbTable->PfieldsInitial() );
    Assert( ptdbTable->PfcbTemplateTable() == pfcbNil );

    Assert( 0 == ptdbSort->CDynamicColumns() );
    Assert( 0 == ptdbTable->CInitialColumns() );
    Assert( 0 == ptdbTable->CDynamicColumns() );
    const ULONG     cCols   = ptdbSort->CInitialColumns();
    if ( cCols > 0 )
    {
        Call( ptdbSort->MemPool().ErrReplaceEntry(
                    itagTDBFields,
                    (BYTE *)ptdbSort->PfieldsInitial(),
                    cCols * sizeof(FIELD) ) );
    }


    Assert( ptdbSort->ItagTableName() == 0 );
    MEMPOOL::ITAG   itagNew;
    Call( ptdbSort->MemPool().ErrAddEntry(
                (BYTE *)szName,
                (ULONG)strlen( szName ) + 1,
                &itagNew ) );
    if ( fIndex
        || pfcbSort->Pidb() != pidbNil )
    {
        Assert( pfcbSort->Pidb() != pidbNil );
        if ( pfcbSort->Pidb()->FIsRgidxsegInMempool() )
        {
            Assert( pfcbSort->Pidb()->ItagRgidxseg() == itagTDBTempTableIdxSeg );
            Assert( itagNew == itagTDBTempTableNameWithIdxSeg );
        }
        else
        {
            Assert( pfcbSort->Pidb()->Cidxseg() > 0 );
            Assert( itagNew == itagTDBTableName );
        }
    }
    else
    {
        Assert( pfcbSort->Pidb() == pidbNil );
        Assert( itagNew == itagTDBTableName );
    }
    ptdbSort->SetItagTableName( itagNew );

    ptdbSort->MemPool().FCompact();

    mempoolSave = ptdbSort->MemPool();
    ptdbSort->SetMemPool( ptdbTable->MemPool() );


    ptdbTable->SetMemPool( mempoolSave );
    ptdbTable->MaterializeFids( ptdbSort );
    ptdbTable->SetItagTableName( ptdbSort->ItagTableName() );

    Assert( NULL == ptdbSort->PdataDefaultRecord() );
    Assert( NULL == ptdbTable->PdataDefaultRecord() );
    ptdbTable->SetPdataDefaultRecord( ptdbSort->PdataDefaultRecord() );
    ptdbSort->SetPdataDefaultRecord( NULL );

    Assert( pfcbTable->Pidb() == pidbNil );
    if ( fIndex )
    {
        Assert( pfcbSort->Pidb() != pidbNil );
        Assert( pfcbSort->Pidb()->ItagIndexName() == 0 );
        pfcbTable->SetPidb( pfcbSort->Pidb() );

        FILESetAllIndexMask( pfcbTable );

        pfcbSort->SetPidb( pidbNil );
    }

    Assert( FFMPIsTempDB( pfucbSort->ifmp ) );
    Assert( pfucbSort->pfucbCurIndex == pfucbNil );
    FUCBSetIndex( pfucbSort );
    FUCBResetSort( pfucbSort );
    FUCBSetMayCacheLVCursor( pfucbSort );

    Assert( pfucbSort->u.pscb->fcb.WRefCount() == 1 );
    Assert( pfucbSort->u.pscb->fcb.Pfucb() == pfucbSort );
    if ( pfucbSort->u.pscb->fcb.PgnoFDP() != pgnoNull )
        SORTICloseRun( ppib, pfucbSort->u.pscb );
    SORTClosePscb( pfucbSort->u.pscb );
    err = pfcbTable->ErrLink( pfucbSort );
    Enforce( JET_errSuccess == err );
    DIRBeforeFirst( pfucbSort );

    CheckTable( ppib, pfucbTable );
    Assert( pfucbTable->pvtfndef == &vtfndefInvalidTableid );
    Assert( !FFUCBUpdatePrepared( pfucbTable ) );
    Assert( NULL == pfucbTable->pvWorkBuf );
    FUCBAssertNoSearchKey( pfucbTable );
    Assert( pfucbNil == pfucbTable->pfucbCurIndex );
    Assert( pfucbTable->u.pfcb->FTypeTemporaryTable() );
    DIRClose( pfucbTable );
    pfucbTable = pfucbNil;

    if ( ppib->Level() > 0 )
    {
        pfucbSort->u.pfcb->Lock();
        pfucbSort->u.pfcb->SetUncommitted();
        pfucbSort->u.pfcb->Unlock();
    }

    pfucbSort->pvtfndef = &vtfndefTTBase;
    return JET_errSuccess;


HandleError:
    if ( pfucbNil != pfucbTable )
    {
        Assert( err < JET_errSuccess );
        Assert( pfucbTable->u.pfcb->FTypeTemporaryTable() );
        CallS( ErrFILECloseTable( ppib, pfucbTable ) );
    }

    return err;
}


#pragma warning(disable:4028 4030)

#ifndef DEBUG
#define ErrIsamSortRetrieveKey      ErrIsamRetrieveKey
#endif

#ifdef DB_DISPATCHING
extern VDBFNCapability              ErrIsamCapability;
extern VDBFNCloseDatabase           ErrIsamCloseDatabase;
extern VDBFNCreateObject            ErrIsamCreateObject;
extern VDBFNCreateTable             ErrIsamCreateTable;
extern VDBFNDeleteObject            ErrIsamDeleteObject;
extern VDBFNDeleteTable             ErrIsamDeleteTable;
extern VDBFNGetColumnInfo           ErrIsamGetColumnInfo;
extern VDBFNGetDatabaseInfo         ErrIsamGetDatabaseInfo;
extern VDBFNGetIndexInfo            ErrIsamGetIndexInfo;
extern VDBFNGetObjectInfo           ErrIsamGetObjectInfo;
extern VDBFNOpenTable               ErrIsamOpenTable;
extern VDBFNRenameTable             ErrIsamRenameTable;
extern VDBFNGetObjidFromName        ErrIsamGetObjidFromName;
extern VDBFNRenameObject            ErrIsamRenameObject;


CODECONST(VDBFNDEF) vdbfndefIsam =
{
    sizeof(VDBFNDEF),
    0,
    NULL,
    ErrIsamCapability,
    ErrIsamCloseDatabase,
    ErrIsamCreateObject,
    ErrIsamCreateTable,
    ErrIsamDeleteObject,
    ErrIsamDeleteTable,
    ErrIllegalExecuteSql,
    ErrIsamGetColumnInfo,
    ErrIsamGetDatabaseInfo,
    ErrIsamGetIndexInfo,
    ErrIsamGetObjectInfo,
    ErrIsamOpenTable,
    ErrIsamRenameObject,
    ErrIsamRenameTable,
    ErrIsamGetObjidFromName,
};
#endif


extern VTFNAddColumn                ErrIsamAddColumn;
extern VTFNCloseTable               ErrIsamCloseTable;
extern VTFNComputeStats             ErrIsamComputeStats;
extern VTFNCreateIndex              ErrIsamCreateIndex;
extern VTFNDelete                   ErrIsamDelete;
extern VTFNDeleteColumn             ErrIsamDeleteColumn;
extern VTFNDeleteIndex              ErrIsamDeleteIndex;
extern VTFNDupCursor                ErrIsamDupCursor;
extern VTFNEscrowUpdate             ErrIsamEscrowUpdate;
extern VTFNGetBookmark              ErrIsamGetBookmark;
extern VTFNGetIndexBookmark         ErrIsamGetIndexBookmark;
extern VTFNGetChecksum              ErrIsamGetChecksum;
extern VTFNGetCurrentIndex          ErrIsamGetCurrentIndex;
extern VTFNGetCursorInfo            ErrIsamGetCursorInfo;
extern VTFNGetRecordPosition        ErrIsamGetRecordPosition;
extern VTFNGetTableColumnInfo       ErrIsamGetTableColumnInfo;
extern VTFNGetTableIndexInfo        ErrIsamGetTableIndexInfo;
extern VTFNGetTableInfo             ErrIsamGetTableInfo;
extern VTFNSetTableInfo             ErrIsamSetTableInfo;
extern VTFNGotoBookmark             ErrIsamGotoBookmark;
extern VTFNGotoIndexBookmark        ErrIsamGotoIndexBookmark;
extern VTFNGotoPosition             ErrIsamGotoPosition;
extern VTFNMakeKey                  ErrIsamMakeKey;
extern VTFNMove                     ErrIsamMove;
extern VTFNPrepareUpdate            ErrIsamPrepareUpdate;
extern VTFNRenameColumn             ErrIsamRenameColumn;
extern VTFNRenameIndex              ErrIsamRenameIndex;
extern VTFNRetrieveColumn           ErrIsamRetrieveColumn;
extern VTFNRetrieveColumns          ErrIsamRetrieveColumns;
extern VTFNRetrieveKey              ErrIsamRetrieveKey;
extern VTFNSeek                     ErrIsamSeek;
extern VTFNSeek                     ErrIsamSortSeek;
extern VTFNSetCurrentIndex          ErrIsamSetCurrentIndex;
extern VTFNSetColumn                ErrIsamSetColumn;
extern VTFNSetColumns               ErrIsamSetColumns;
extern VTFNSetIndexRange            ErrIsamSetIndexRange;
extern VTFNSetIndexRange            ErrIsamSortSetIndexRange;
extern VTFNUpdate                   ErrIsamUpdate;
extern VTFNGetLock                  ErrIsamGetLock;
extern VTFNEnumerateColumns         ErrIsamEnumerateColumns;
extern VTFNGetRecordSize            ErrIsamGetRecordSize;

extern VTFNDupCursor                ErrIsamSortDupCursor;
extern VTFNGetTableInfo             ErrIsamSortGetTableInfo;
extern VTFNCloseTable               ErrIsamSortClose;
extern VTFNMove                     ErrIsamSortMove;
extern VTFNGetBookmark              ErrIsamSortGetBookmark;
extern VTFNGotoBookmark             ErrIsamSortGotoBookmark;
extern VTFNRetrieveKey              ErrIsamSortRetrieveKey;
extern VTFNUpdate                   ErrIsamSortUpdate;

extern VTFNDupCursor                ErrTTSortRetDupCursor;

extern VTFNDupCursor                ErrTTBaseDupCursor;
extern VTFNMove                     ErrTTSortInsMove;
extern VTFNSeek                     ErrTTSortInsSeek;


CODECONST(VTFNDEF) vtfndefIsam =
{
    sizeof(VTFNDEF),
    0,
    NULL,
    ErrIsamAddColumn,
    ErrIsamCloseTable,
    ErrIsamComputeStats,
    ErrIsamCreateIndex,
    ErrIsamDelete,
    ErrIsamDeleteColumn,
    ErrIsamDeleteIndex,
    ErrIsamDupCursor,
    ErrIsamEscrowUpdate,
    ErrIsamGetBookmark,
    ErrIsamGetIndexBookmark,
    ErrIsamGetChecksum,
    ErrIsamGetCurrentIndex,
    ErrIsamGetCursorInfo,
    ErrIsamGetRecordPosition,
    ErrIsamGetTableColumnInfo,
    ErrIsamGetTableIndexInfo,
    ErrIsamGetTableInfo,
    ErrIsamSetTableInfo,
    ErrIsamGotoBookmark,
    ErrIsamGotoIndexBookmark,
    ErrIsamGotoPosition,
    ErrIsamMakeKey,
    ErrIsamMove,
    ErrIsamSetCursorFilter,
    ErrIsamPrepareUpdate,
    ErrIsamRenameColumn,
    ErrIsamRenameIndex,
    ErrIsamRetrieveColumn,
    ErrIsamRetrieveColumns,
    ErrIsamRetrieveKey,
    ErrIsamSeek,
    ErrIsamPrereadKeys,
    ErrIsamPrereadIndexRanges,
    ErrIsamSetCurrentIndex,
    ErrIsamSetColumn,
    ErrIsamSetColumns,
    ErrIsamSetIndexRange,
    ErrIsamUpdate,
    ErrIsamGetLock,
    ErrIsamRegisterCallback,
    ErrIsamUnregisterCallback,
    ErrIsamSetLS,
    ErrIsamGetLS,
    ErrIsamIndexRecordCount,
    ErrIsamRetrieveTaggedColumnList,
    ErrIsamSetSequential,
    ErrIsamResetSequential,
    ErrIsamEnumerateColumns,
    ErrIsamGetRecordSize,
    ErrIsamPrereadIndexRange,
    ErrIsamRetrieveColumnByReference,
    ErrIsamPrereadColumnsByReference,
    ErrIsamStreamRecords,
};

const VTFNDEF vtfndefIsamMustRollback =
{
    sizeof(VTFNDEF),
    0,
    NULL,
    ErrIllegalAddColumn,
    ErrIsamCloseTable,
    ErrIllegalComputeStats,
    ErrIllegalCreateIndex,
    ErrIllegalDelete,
    ErrIllegalDeleteColumn,
    ErrIllegalDeleteIndex,
    ErrIllegalDupCursor,
    ErrIllegalEscrowUpdate,
    ErrIllegalGetBookmark,
    ErrIllegalGetIndexBookmark,
    ErrIllegalGetChecksum,
    ErrIllegalGetCurrentIndex,
    ErrIllegalGetCursorInfo,
    ErrIllegalGetRecordPosition,
    ErrIllegalGetTableColumnInfo,
    ErrIllegalGetTableIndexInfo,
    ErrIllegalGetTableInfo,
    ErrIllegalSetTableInfo,
    ErrIllegalGotoBookmark,
    ErrIllegalGotoIndexBookmark,
    ErrIllegalGotoPosition,
    ErrIllegalMakeKey,
    ErrIllegalMove,
    ErrIllegalSetCursorFilter,
    ErrIllegalPrepareUpdate,
    ErrIllegalRenameColumn,
    ErrIllegalRenameIndex,
    ErrIllegalRetrieveColumn,
    ErrIllegalRetrieveColumns,
    ErrIllegalRetrieveKey,
    ErrIllegalSeek,
    ErrIllegalPrereadKeys,
    ErrIllegalPrereadIndexRanges,
    ErrIllegalSetCurrentIndex,
    ErrIllegalSetColumn,
    ErrIllegalSetColumns,
    ErrIllegalSetIndexRange,
    ErrIllegalUpdate,
    ErrIllegalGetLock,
    ErrIllegalRegisterCallback,
    ErrIllegalUnregisterCallback,
    ErrIllegalSetLS,
    ErrIllegalGetLS,
    ErrIllegalIndexRecordCount,
    ErrIllegalRetrieveTaggedColumnList,
    ErrIllegalSetSequential,
    ErrIllegalResetSequential,
    ErrIllegalEnumerateColumns,
    ErrIllegalGetRecordSize,
    ErrIllegalPrereadIndexRange,
    ErrIllegalRetrieveColumnByReference,
    ErrIllegalPrereadColumnsByReference,
    ErrIllegalStreamRecords,
};

CODECONST(VTFNDEF) vtfndefTTSortIns =
{
    sizeof(VTFNDEF),
    0,
    NULL,
    ErrIllegalAddColumn,
    ErrIsamSortClose,
    ErrIllegalComputeStats,
    ErrIllegalCreateIndex,
    ErrIllegalDelete,
    ErrIllegalDeleteColumn,
    ErrIllegalDeleteIndex,
    ErrIllegalDupCursor,
    ErrIsamEscrowUpdate,
    ErrIllegalGetBookmark,
    ErrIllegalGetIndexBookmark,
    ErrIllegalGetChecksum,
    ErrIllegalGetCurrentIndex,
    ErrIllegalGetCursorInfo,
    ErrIllegalGetRecordPosition,
    ErrIllegalGetTableColumnInfo,
    ErrIllegalGetTableIndexInfo,
    ErrIllegalGetTableInfo,
    ErrIllegalSetTableInfo,
    ErrIllegalGotoBookmark,
    ErrIllegalGotoIndexBookmark,
    ErrIllegalGotoPosition,
    ErrIsamMakeKey,
    ErrTTSortInsMove,
    ErrIllegalSetCursorFilter,
    ErrIsamPrepareUpdate,
    ErrIllegalRenameColumn,
    ErrIllegalRenameIndex,
    ErrIllegalRetrieveColumn,
    ErrIllegalRetrieveColumns,
    ErrIsamSortRetrieveKey,
    ErrTTSortInsSeek,
    ErrIllegalPrereadKeys,
    ErrIllegalPrereadIndexRanges,
    ErrIllegalSetCurrentIndex,
    ErrIsamSetColumn,
    ErrIsamSetColumns,
    ErrIllegalSetIndexRange,
    ErrIsamSortUpdate,
    ErrIllegalGetLock,
    ErrIllegalRegisterCallback,
    ErrIllegalUnregisterCallback,
    ErrIllegalSetLS,
    ErrIllegalGetLS,
    ErrIllegalIndexRecordCount,
    ErrIllegalRetrieveTaggedColumnList,
    ErrIllegalSetSequential,
    ErrIllegalResetSequential,
    ErrIllegalEnumerateColumns,
    ErrIllegalGetRecordSize,
    ErrIllegalPrereadIndexRange,
    ErrIllegalRetrieveColumnByReference,
    ErrIllegalPrereadColumnsByReference,
    ErrIllegalStreamRecords,
};

CODECONST(VTFNDEF) vtfndefTTSortRet =
{
    sizeof(VTFNDEF),
    0,
    NULL,
    ErrIllegalAddColumn,
    ErrIsamSortClose,
    ErrIllegalComputeStats,
    ErrIllegalCreateIndex,
    ErrIllegalDelete,
    ErrIllegalDeleteColumn,
    ErrIllegalDeleteIndex,
    ErrTTSortRetDupCursor,
    ErrIllegalEscrowUpdate,
    ErrIsamSortGetBookmark,
    ErrIllegalGetIndexBookmark,
    ErrIllegalGetChecksum,
    ErrIllegalGetCurrentIndex,
    ErrIllegalGetCursorInfo,
    ErrIllegalGetRecordPosition,
    ErrIllegalGetTableColumnInfo,
    ErrIllegalGetTableIndexInfo,
    ErrIsamSortGetTableInfo,
    ErrIllegalSetTableInfo,
    ErrIsamSortGotoBookmark,
    ErrIllegalGotoIndexBookmark,
    ErrIllegalGotoPosition,
    ErrIsamMakeKey,
    ErrIsamSortMove,
    ErrIllegalSetCursorFilter,
    ErrIllegalPrepareUpdate,
    ErrIllegalRenameColumn,
    ErrIllegalRenameIndex,
    ErrIsamRetrieveColumn,
    ErrIsamRetrieveColumns,
    ErrIsamSortRetrieveKey,
    ErrIsamSortSeek,
    ErrIllegalPrereadKeys,
    ErrIllegalPrereadIndexRanges,
    ErrIllegalSetCurrentIndex,
    ErrIllegalSetColumn,
    ErrIllegalSetColumns,
    ErrIsamSortSetIndexRange,
    ErrIllegalUpdate,
    ErrIllegalGetLock,
    ErrIllegalRegisterCallback,
    ErrIllegalUnregisterCallback,
    ErrIsamSetLS,
    ErrIsamGetLS,
    ErrIllegalIndexRecordCount,
    ErrIllegalRetrieveTaggedColumnList,
    ErrIllegalSetSequential,
    ErrIllegalResetSequential,
    ErrIsamEnumerateColumns,
    ErrIsamGetRecordSize,
    ErrIllegalPrereadIndexRange,
    ErrIllegalRetrieveColumnByReference,
    ErrIllegalPrereadColumnsByReference,
    ErrIllegalStreamRecords,
};

CODECONST(VTFNDEF) vtfndefTTBase =
{
    sizeof(VTFNDEF),
    0,
    NULL,
    ErrIllegalAddColumn,
    ErrIsamSortClose,
    ErrIllegalComputeStats,
    ErrIllegalCreateIndex,
    ErrIsamDelete,
    ErrIllegalDeleteColumn,
    ErrIllegalDeleteIndex,
    ErrTTBaseDupCursor,
    ErrIsamEscrowUpdate,
    ErrIsamGetBookmark,
    ErrIllegalGetIndexBookmark,
    ErrIsamGetChecksum,
    ErrIllegalGetCurrentIndex,
    ErrIsamGetCursorInfo,
    ErrIllegalGetRecordPosition,
    ErrIllegalGetTableColumnInfo,
    ErrIllegalGetTableIndexInfo,
    ErrIsamSortGetTableInfo,
    ErrIllegalSetTableInfo,
    ErrIsamGotoBookmark,
    ErrIllegalGotoIndexBookmark,
    ErrIllegalGotoPosition,
    ErrIsamMakeKey,
    ErrIsamMove,
    ErrIllegalSetCursorFilter,
    ErrIsamPrepareUpdate,
    ErrIllegalRenameColumn,
    ErrIllegalRenameIndex,
    ErrIsamRetrieveColumn,
    ErrIsamRetrieveColumns,
    ErrIsamRetrieveKey,
    ErrIsamSeek,
    ErrIllegalPrereadKeys,
    ErrIllegalPrereadIndexRanges,
    ErrIllegalSetCurrentIndex,
    ErrIsamSetColumn,
    ErrIsamSetColumns,
    ErrIsamSetIndexRange,
    ErrIsamUpdate,
    ErrIllegalGetLock,
    ErrIllegalRegisterCallback,
    ErrIllegalUnregisterCallback,
    ErrIsamSetLS,
    ErrIsamGetLS,
    ErrIllegalIndexRecordCount,
    ErrIllegalRetrieveTaggedColumnList,
    ErrIllegalSetSequential,
    ErrIllegalResetSequential,
    ErrIsamEnumerateColumns,
    ErrIsamGetRecordSize,
    ErrIllegalPrereadIndexRange,
    ErrIllegalRetrieveColumnByReference,
    ErrIllegalPrereadColumnsByReference,
    ErrIllegalStreamRecords,
};

const VTFNDEF vtfndefTTBaseMustRollback =
{
    sizeof(VTFNDEF),
    0,
    NULL,
    ErrIllegalAddColumn,
    ErrIsamSortClose,
    ErrIllegalComputeStats,
    ErrIllegalCreateIndex,
    ErrIllegalDelete,
    ErrIllegalDeleteColumn,
    ErrIllegalDeleteIndex,
    ErrIllegalDupCursor,
    ErrIllegalEscrowUpdate,
    ErrIllegalGetBookmark,
    ErrIllegalGetIndexBookmark,
    ErrIllegalGetChecksum,
    ErrIllegalGetCurrentIndex,
    ErrIllegalGetCursorInfo,
    ErrIllegalGetRecordPosition,
    ErrIllegalGetTableColumnInfo,
    ErrIllegalGetTableIndexInfo,
    ErrIllegalGetTableInfo,
    ErrIllegalSetTableInfo,
    ErrIllegalGotoBookmark,
    ErrIllegalGotoIndexBookmark,
    ErrIllegalGotoPosition,
    ErrIllegalMakeKey,
    ErrIllegalMove,
    ErrIllegalSetCursorFilter,
    ErrIllegalPrepareUpdate,
    ErrIllegalRenameColumn,
    ErrIllegalRenameIndex,
    ErrIllegalRetrieveColumn,
    ErrIllegalRetrieveColumns,
    ErrIllegalRetrieveKey,
    ErrIllegalSeek,
    ErrIllegalPrereadKeys,
    ErrIllegalPrereadIndexRanges,
    ErrIllegalSetCurrentIndex,
    ErrIllegalSetColumn,
    ErrIllegalSetColumns,
    ErrIllegalSetIndexRange,
    ErrIllegalUpdate,
    ErrIllegalGetLock,
    ErrIllegalRegisterCallback,
    ErrIllegalUnregisterCallback,
    ErrIllegalSetLS,
    ErrIllegalGetLS,
    ErrIllegalIndexRecordCount,
    ErrIllegalRetrieveTaggedColumnList,
    ErrIllegalSetSequential,
    ErrIllegalResetSequential,
    ErrIllegalEnumerateColumns,
    ErrIllegalGetRecordSize,
    ErrIllegalPrereadIndexRange,
    ErrIllegalRetrieveColumnByReference,
    ErrIllegalPrereadColumnsByReference,
    ErrIllegalStreamRecords,
};

LOCAL CODECONST(VTFNDEF) vtfndefTTSortClose =
{
    sizeof(VTFNDEF),
    0,
    NULL,
    ErrIllegalAddColumn,
    ErrIsamSortClose,
    ErrIllegalComputeStats,
    ErrIllegalCreateIndex,
    ErrIllegalDelete,
    ErrIllegalDeleteColumn,
    ErrIllegalDeleteIndex,
    ErrIllegalDupCursor,
    ErrIllegalEscrowUpdate,
    ErrIllegalGetBookmark,
    ErrIllegalGetIndexBookmark,
    ErrIllegalGetChecksum,
    ErrIllegalGetCurrentIndex,
    ErrIllegalGetCursorInfo,
    ErrIllegalGetRecordPosition,
    ErrIllegalGetTableColumnInfo,
    ErrIllegalGetTableIndexInfo,
    ErrIllegalGetTableInfo,
    ErrIllegalSetTableInfo,
    ErrIllegalGotoBookmark,
    ErrIllegalGotoIndexBookmark,
    ErrIllegalGotoPosition,
    ErrIllegalMakeKey,
    ErrIllegalMove,
    ErrIllegalSetCursorFilter,
    ErrIllegalPrepareUpdate,
    ErrIllegalRenameColumn,
    ErrIllegalRenameIndex,
    ErrIllegalRetrieveColumn,
    ErrIllegalRetrieveColumns,
    ErrIllegalRetrieveKey,
    ErrIllegalSeek,
    ErrIllegalPrereadKeys,
    ErrIllegalPrereadIndexRanges,
    ErrIllegalSetCurrentIndex,
    ErrIllegalSetColumn,
    ErrIllegalSetColumns,
    ErrIllegalSetIndexRange,
    ErrIllegalUpdate,
    ErrIllegalGetLock,
    ErrIllegalRegisterCallback,
    ErrIllegalUnregisterCallback,
    ErrIllegalSetLS,
    ErrIllegalGetLS,
    ErrIllegalIndexRecordCount,
    ErrIllegalRetrieveTaggedColumnList,
    ErrIllegalSetSequential,
    ErrIllegalResetSequential,
    ErrIllegalEnumerateColumns,
    ErrIllegalGetRecordSize,
    ErrIllegalPrereadIndexRange,
    ErrIllegalRetrieveColumnByReference,
    ErrIllegalPrereadColumnsByReference,
    ErrIllegalStreamRecords,
};



ERR VDBAPI ErrIsamOpenTempTable(
    JET_SESID               sesid,
    const JET_COLUMNDEF     *rgcolumndef,
    ULONG                   ccolumndef,
    JET_UNICODEINDEX2       *pidxunicode,
    JET_GRBIT               grbit,
    JET_TABLEID             *ptableid,
    JET_COLUMNID            *rgcolumnid,
    const ULONG             cbKeyMost,
    const ULONG             cbVarSegMac )
{
    ERR                     err;
    FUCB                    *pfucb;

    Assert( ptableid );
    *ptableid = JET_tableidNil;

    if ( !UlParam( PinstFromPpib( (PIB*)sesid ), JET_paramMaxTemporaryTables ) )
    {
        CallR( ErrERRCheck( JET_errTooManySorts ) );
    }

    if (    ( grbit & JET_bitTTForwardOnly ) &&
            ( grbit & ( JET_bitTTIndexed | JET_bitTTUpdatable | JET_bitTTScrollable | JET_bitTTForceMaterialization ) ) )
    {
        CallR( ErrERRCheck( JET_errCannotMaterializeForwardOnlySort ) );
    }

    CallR( ErrPIBOpenTempDatabase ( (PIB *)sesid ) );

    CallR( ErrIsamSortOpen(
                (PIB *)sesid,
                (JET_COLUMNDEF *)rgcolumndef,
                ccolumndef,
                pidxunicode,
                grbit,
                &pfucb,
                rgcolumnid,
                cbKeyMost,
                cbVarSegMac ) );
    Assert( pfucbNil != pfucb );
    Assert( &vtfndefTTSortIns == pfucb->pvtfndef );
    Assert( ptdbNil != pfucb->u.pscb->fcb.Ptdb() );

    BOOL    fIndexed = fFalse;
    BOOL    fLongValues = fFalse;
    for ( UINT iColumndef = 0; iColumndef < (INT)ccolumndef; iColumndef++ )
    {
        fIndexed |= ( ( rgcolumndef[iColumndef].grbit & JET_bitColumnTTKey ) != 0);
        fLongValues |= FRECLongValue( rgcolumndef[iColumndef].coltyp );
    }

    if ( !fIndexed
        || ( fLongValues && !( grbit & JET_bitTTIntrinsicLVsOnly ) )
        || ( grbit & JET_bitTTForceMaterialization ) )
    {
        if ( grbit & JET_bitTTForwardOnly )
        {
            CallS( ErrIsamSortClose( sesid, (JET_VTID)pfucb ) );
            return ErrERRCheck( JET_errCannotMaterializeForwardOnlySort );
        }
        err = ErrIsamSortMaterialize( (PIB *)sesid, pfucb, fIndexed );
        Assert( err <= 0 );
        if ( err < 0 )
        {
            CallS( ErrIsamSortClose( sesid, (JET_VTID)pfucb ) );
            return err;
        }
    }

    *ptableid = (JET_TABLEID)pfucb;
    return err;
}


ERR ErrTTEndInsert( JET_SESID sesid, JET_TABLEID tableid, BOOL *pfMovedToFirst )
{
    ERR             err;
    FUCB            *pfucb          = (FUCB *)tableid;
    BOOL            fOverflow;
    BOOL            fMaterialize;
    const JET_GRBIT grbitOpen       = pfucb->u.pscb->grbit;

    Assert( &vtfndefTTSortIns == pfucb->pvtfndef );

    Assert( pfMovedToFirst );
    *pfMovedToFirst = fFalse;

    Call( ErrSORTEndInsert( pfucb ) );

    fOverflow = ( JET_wrnSortOverflow == err );
    Assert( JET_errSuccess == err || fOverflow );

    fMaterialize = ( grbitOpen & JET_bitTTUpdatable )
                    || ( ( grbitOpen & ( JET_bitTTScrollable|JET_bitTTIndexed ) )
                        && fOverflow );
    if ( fMaterialize )
    {
        Assert( !( grbitOpen & JET_bitTTForwardOnly ) );
        Call( ErrIsamSortMaterialize( (PIB *)sesid, pfucb, ( grbitOpen & JET_bitTTIndexed ) != 0 ) );
        Assert( JET_errSuccess == err );
        pfucb->pvtfndef = &vtfndefTTBase;
    }
    else
    {
        err = ErrSORTFirst( pfucb );
        Assert( err <= JET_errSuccess );
        if ( err < JET_errSuccess )
        {
            if ( JET_errNoCurrentRecord != err )
                goto HandleError;
        }

        *pfMovedToFirst = fTrue;
        pfucb->pvtfndef = &vtfndefTTSortRet;
    }

    Assert( JET_errSuccess == err
        || ( JET_errNoCurrentRecord == err && *pfMovedToFirst ) );
    return err;

HandleError:
    Assert( err < 0 );
    Assert( JET_errNoCurrentRecord != err );

    Assert( &vtfndefTTSortIns == pfucb->pvtfndef );
    pfucb->pvtfndef = &vtfndefTTSortClose;

    return err;
}



ERR VTAPI ErrTTSortInsMove( JET_SESID sesid, JET_TABLEID tableid, LONG crow, JET_GRBIT grbit )
{
    ERR     err;
    BOOL    fMovedToFirst;

    if ( FFUCBUpdatePrepared( (FUCB *)tableid ) )
    {
        CallR( ErrIsamPrepareUpdate( (PIB *)sesid, (FUCB *)tableid, JET_prepCancel ) );
    }

    err = ErrTTEndInsert( sesid, tableid, &fMovedToFirst );
    Assert( JET_errNoCurrentRecord != err || fMovedToFirst );
    CallR( err );
    Assert( JET_errSuccess == err );

    if ( fMovedToFirst )
    {
        if ( crow > 0 && crow < JET_MoveLast )
            crow--;

        if ( JET_MoveFirst == crow || 0 == crow )
            return JET_errSuccess;
    }

    err = ErrDispMove( sesid, tableid, crow, grbit );
    return err;
}



ERR VTAPI ErrTTSortInsSeek( JET_SESID sesid, JET_TABLEID tableid, JET_GRBIT grbit )
{
    ERR     err;
    BOOL    fMovedToFirst;

    if ( FFUCBUpdatePrepared( (FUCB *)tableid ) )
    {
        CallR( ErrIsamPrepareUpdate( (PIB *)sesid, (FUCB *)tableid, JET_prepCancel ) );
    }

    err = ErrTTEndInsert( sesid, tableid, &fMovedToFirst );
    Assert( err <= JET_errSuccess );
    if ( err < JET_errSuccess )
    {
        if ( JET_errNoCurrentRecord == err )
        {
            Assert( fMovedToFirst );
            err = ErrERRCheck( JET_errRecordNotFound );
        }
    }
    else
    {
        err = ErrDispSeek( sesid, tableid, grbit );
    }

    Assert( JET_errNoCurrentRecord != err );
    return err;
}


ERR VTAPI ErrTTSortRetDupCursor( JET_SESID sesid, JET_TABLEID tableid, JET_TABLEID *ptableidDup, JET_GRBIT grbit )
{
    ERR err = ErrIsamSortDupCursor( sesid, tableid, ptableidDup, grbit );
    if ( err >= 0 )
    {
        *(const VTFNDEF **)(*ptableidDup) = &vtfndefTTSortRet;
    }

    return err;
}


ERR VTAPI ErrTTBaseDupCursor( JET_SESID sesid, JET_TABLEID tableid, JET_TABLEID *ptableidDup, JET_GRBIT grbit )
{
    ERR err = ErrIsamSortDupCursor( sesid, tableid, ptableidDup, grbit );
    if ( err >= 0 )
    {
        *(const VTFNDEF **)(*ptableidDup) = &vtfndefTTBase;
    }

    return err;
}





ERR ErrCMPCopyLVTree(
    PIB         *ppib,
    FUCB        *pfucbSrc,
    FUCB        *pfucbDest,
    BYTE        *pbBuf,
    ULONG       cbBufSize,
    STATUSINFO  *pstatus )
{
    ERR         err;
    FUCB        *pfucbGetLV     = pfucbNil;
    FUCB        *pfucbCopyLV    = pfucbNil;
    LvId        lidSrc;
    LVROOT2     lvroot;
    DATA        dataNull;
    PGNO        pgnoLastLV      = pgnoNull;
    LIDMAP*     plidmapAlloc    = NULL;

    dataNull.Nullify();

    Assert( !g_rgfmp[pfucbSrc->ifmp].Plidmap() );

    err =  ErrCMPGetSLongFieldFirst(
                    pfucbSrc,
                    &pfucbGetLV,
                    &lidSrc,
                    &lvroot );
    if ( err < 0 )
    {
        Assert( pfucbNil == pfucbGetLV );
        if ( JET_errRecordNotFound == err )
            err = JET_errSuccess;
        return err;
    }

    Assert( pfucbNil != pfucbGetLV );

    Alloc( plidmapAlloc = new LIDMAP() );
    Call( plidmapAlloc->ErrLIDMAPInit( ppib ) );
    Assert( JET_tableidNil != plidmapAlloc->Tableid() );
    g_rgfmp[pfucbSrc->ifmp].SetLidmap( plidmapAlloc );
    plidmapAlloc = NULL;

    do
    {
        LvId    lidDest;
        ULONG   ibLongValue = 0;


        pfucbDest->u.pfcb->Ptdb()->SetFLid64( lidSrc.FIsLid64() );

        Assert( pgnoNull != Pcsr( pfucbGetLV )->Pgno() );

        if ( !FPartiallyDeletedLV( lvroot.ulReference ) )
        {
            Assert( lvroot.ulReference > 0 && !FPartiallyDeletedLV( lvroot.ulReference ) );
            Assert( pfucbNil == pfucbCopyLV );
            Call( ErrRECSeparateLV(
                        pfucbDest,
                        &dataNull,
                        compressNone,
                        fFalse,
                        &lidDest,
                        &pfucbCopyLV,
                        &lvroot ) );
            Assert( pfucbNil != pfucbCopyLV );

            EnforceSz( lidSrc.FIsLid64() == lidDest.FIsLid64(), "DataCorruptionLidSizeMismatch" );

            do {
                ULONG cbReturnedPhysical;

                const LONG cbLVChunkMost = pfucbSrc->u.pfcb->Ptdb()->CbLVChunkMost();
                Assert( ibLongValue % cbLVChunkMost == 0 );
                Assert( cbBufSize >= (ULONG)cbLVChunkMost );
                Call( ErrCMPRetrieveSLongFieldValueByChunk(
                            pfucbGetLV,
                            lidSrc,
                            lvroot.ulSize,
                            ibLongValue,
                            pbBuf,
                            CbOSEncryptAes256SizeNeeded( cbLVChunkMost ),
                            &cbReturnedPhysical ) );

                Assert( cbReturnedPhysical > 0 );
                Assert( cbReturnedPhysical <= cbBufSize );

                Call( ErrCMPAppendLVChunk(
                            pfucbCopyLV,
                            lidDest,
                            ibLongValue,
                            pbBuf,
                            cbReturnedPhysical ) );

                Assert( err != JET_wrnCopyLongValue );

                ibLongValue += cbLVChunkMost;

                Assert( Pcsr( pfucbGetLV )->Pgno() != pgnoNull );
                if ( NULL != pstatus )
                {
                    pstatus->cbRawDataLV += cbReturnedPhysical;
                    if( Pcsr( pfucbGetLV )->Pgno() != pgnoLastLV )
                    {
                        pgnoLastLV = Pcsr( pfucbGetLV )->Pgno();
                        pstatus->cLVPagesTraversed  += 1;
                        Call( ErrSORTCopyProgress( pstatus, 1 ) );
                    }
                }
            }
            while ( lvroot.ulSize > ibLongValue );

            Assert( pfucbNil != pfucbCopyLV );
            DIRClose( pfucbCopyLV );
            pfucbCopyLV = pfucbNil;

            Call( g_rgfmp[pfucbSrc->ifmp].Plidmap()->ErrInsert( ppib, lidSrc, lidDest, lvroot.ulReference ) );

            Assert( pgnoNull !=  Pcsr( pfucbGetLV )->Pgno() );
        }
        
        err = ErrCMPGetSLongFieldNext( pfucbGetLV, &lidSrc, &lvroot );
    }
    while ( err >= JET_errSuccess );

    if ( JET_errNoCurrentRecord == err )
        err = JET_errSuccess;

HandleError:
    delete plidmapAlloc;
    if ( pfucbNil != pfucbCopyLV )
        DIRClose( pfucbCopyLV );

    Assert( pfucbNil != pfucbGetLV );
    CMPGetSLongFieldClose( pfucbGetLV );

    return err;
}

