// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.



#include "std.hxx"

#include "PageSizeClean.hxx"


#ifdef MINIMAL_FUNCTIONALITY
#else




struct UPGRADEPAGESTATS
{
    LONG        err;
    LONG        cpgSeen;
};




PERSISTED
struct LV
{
    union
    {
        BYTE    bFlags;
        struct
        {
            BYTE    fSeparated:1;
            BYTE    fReserved:6;
        };
    };

    UnalignedLittleEndian< _LID32 >     m_lid;
};




class CONVERTPAGETASK : public DBTASK
{
    public:
        CONVERTPAGETASK( const IFMP ifmp, const PGNO pgnoFirst, const CPG cpg, UPGRADEPAGESTATS * const pstats );
        virtual ~CONVERTPAGETASK();

        virtual ERR ErrExecuteDbTask( PIB * const ppib );
        VOID HandleError( const ERR err );

    protected:
        const PGNO      m_pgnoFirst;
        const CPG       m_cpg;

        UPGRADEPAGESTATS * const m_pstats;

    private:
        CONVERTPAGETASK( const CONVERTPAGETASK& );
        CONVERTPAGETASK& operator=( const CONVERTPAGETASK& );
};


class CONVERTPAGETASKPOOL
{
    public:
        CONVERTPAGETASKPOOL( const IFMP ifmp );
        ~CONVERTPAGETASKPOOL();

        ERR ErrInit( PIB * const ppib, const INT cThreads );
        ERR ErrTerm();

        ERR ErrConvertPages( const PGNO pgnoFirst, const CPG cpg );

        const volatile UPGRADEPAGESTATS& Stats() const;

    private:
        CONVERTPAGETASKPOOL( const CONVERTPAGETASKPOOL& );
        CONVERTPAGETASKPOOL& operator=( const CONVERTPAGETASKPOOL& );

    private:
        const IFMP  m_ifmp;
        TASKMGR     m_taskmgr;

        UPGRADEPAGESTATS    m_stats;
};





VOID UPGRADECheckConvertNode(
    const VOID * const pvRecOld,
    const INT cbRecOld,
    const VOID * const pvRecNew,
    const INT cbRecNew );
ERR ErrUPGRADEConvertNode(
    CPAGE * const pcpage,
    const INT iline,
    VOID * const pvBuf );
ERR ErrUPGRADEConvertPage(
    CPAGE * const pcpage,
    const PGNO pgno,
    VOID * const pvBuf );



CONVERTPAGETASKPOOL::CONVERTPAGETASKPOOL( const IFMP ifmp ) :
    m_ifmp( ifmp )
{
    m_stats.err                 = JET_errSuccess;
    m_stats.cpgSeen             = 0;
}


CONVERTPAGETASKPOOL::~CONVERTPAGETASKPOOL()
{
}


ERR CONVERTPAGETASKPOOL::ErrInit( PIB * const ppib, const INT cThreads )
{
    ERR err;

    INST * const pinst = PinstFromIfmp( m_ifmp );

    Call( m_taskmgr.ErrInit( pinst, cThreads ) );

    return err;

HandleError:
    CallS( ErrTerm() );
    return err;
}


ERR CONVERTPAGETASKPOOL::ErrTerm()
{
    ERR err;

    Call( m_taskmgr.ErrTerm() );

    err = m_stats.err;

HandleError:
    return err;
}


ERR CONVERTPAGETASKPOOL::ErrConvertPages( const PGNO pgnoFirst, const CPG cpg )
{
    CONVERTPAGETASK * ptask = new CONVERTPAGETASK( m_ifmp, pgnoFirst, cpg, &m_stats );
    if( NULL == ptask )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }
    const ERR err = m_taskmgr.ErrPostTask( TASK::Dispatch, (ULONG_PTR)ptask );
    if( err < JET_errSuccess )
    {
        delete ptask;
    }
    return err;
}


const volatile UPGRADEPAGESTATS& CONVERTPAGETASKPOOL::Stats() const
{
    return m_stats;
}


CONVERTPAGETASK::CONVERTPAGETASK(
    const IFMP ifmp,
    const PGNO pgnoFirst,
    const CPG cpg,
    UPGRADEPAGESTATS * const pstats ) :
    DBTASK( ifmp ),
    m_pgnoFirst( pgnoFirst ),
    m_cpg( cpg ),
    m_pstats( pstats )
{
    AssertRTL( !FFMPIsTempDB( ifmp ) );
}


CONVERTPAGETASK::~CONVERTPAGETASK()
{
}


ERR CONVERTPAGETASK::ErrExecuteDbTask( PIB * const ppib )
{
    ERR err = JET_errSuccess;

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = tceNone;

    CSR csr;

    PGNO pgno;
    for( pgno = m_pgnoFirst; pgno < m_pgnoFirst + m_cpg; ++pgno )
    {
        AtomicExchangeAdd( &m_pstats->cpgSeen, 1 );

        err = csr.ErrGetRIWPage( ppib, m_ifmp, pgno );
        if( JET_errPageNotInitialized == err )
        {
            err = JET_errSuccess;
            continue;
        }
        Call( err );

        csr.ReleasePage( fTrue );
        csr.Reset();
    }

HandleError:
    return err;
}


VOID CONVERTPAGETASK::HandleError( const ERR err )
{
    AssertSz( fFalse, "Unable to run a CONVERTPAGETASK" );
}


ERR ErrDBUTLConvertRecords( JET_SESID sesid, const JET_DBUTIL_W * const pdbutil )
{
    ERR err = JET_errSuccess;
    PIB * const ppib = reinterpret_cast<PIB *>( sesid );
    CONVERTPAGETASKPOOL * pconverttasks = NULL;
    PIBTraceContextScope tcScope = ((PIB*)sesid)->InitTraceContextScope();

    Call( ErrIsamAttachDatabase( sesid, pdbutil->szDatabase, fFalse, NULL, 0, NO_GRBIT ) );

    IFMP ifmp;
    ifmp = 0;
    Call( ErrIsamOpenDatabase(
        sesid,
        pdbutil->szDatabase,
        NULL,
        reinterpret_cast<JET_DBID *>( &ifmp ),
        NO_GRBIT
        ) );

    PGNO pgnoLast;
    pgnoLast = g_rgfmp[ifmp].PgnoLast();

    pconverttasks = new CONVERTPAGETASKPOOL( ifmp );
    if( NULL == pconverttasks )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    Call( pconverttasks->ErrInit( ppib, CUtilProcessProcessor() ) );

    JET_SNPROG snprog;
    memset( &snprog, 0, sizeof( snprog ) );
    snprog.cunitTotal   = pgnoLast;
    snprog.cunitDone    = 0;

    JET_PFNSTATUS pfnStatus;
    pfnStatus = reinterpret_cast<JET_PFNSTATUS>( pdbutil->pfnCallback );

    if ( NULL != pfnStatus )
    {
        (VOID)pfnStatus( sesid, JET_snpUpgradeRecordFormat, JET_sntBegin, NULL );
    }

    PGNO pgno;
    pgno = 1;

    while( pgnoLast >= pgno && JET_errSuccess == pconverttasks->Stats().err )
    {
        const CPG cpgChunk = 256;
        const CPG cpgPreread = min( cpgChunk, pgnoLast - pgno + 1 );
        BFPrereadPageRange( ifmp, pgno, cpgPreread, bfprfDefault, ppib->BfpriPriority( ifmp ), *tcScope );

        Call( pconverttasks->ErrConvertPages( pgno, cpgPreread ) );
        pgno += cpgPreread;

        snprog.cunitDone = pconverttasks->Stats().cpgSeen;
        if ( NULL != pfnStatus )
        {
            (VOID)pfnStatus( sesid, JET_snpUpgradeRecordFormat, JET_sntProgress, &snprog );
        }

        while(  ULONG( pconverttasks->Stats().cpgSeen + ( cpgChunk * 4 ) ) < pgno
                && JET_errSuccess == pconverttasks->Stats().err )
        {
            snprog.cunitDone = pconverttasks->Stats().cpgSeen;
            if ( NULL != pfnStatus )
            {
                (VOID)pfnStatus( sesid, JET_snpUpgradeRecordFormat, JET_sntProgress, &snprog );
            }
            UtilSleep( cmsecWaitGeneric );
        }
    }

    snprog.cunitDone = pconverttasks->Stats().cpgSeen;
    if ( NULL != pfnStatus )
    {
        (VOID)pfnStatus( sesid, JET_snpUpgradeRecordFormat, JET_sntProgress, &snprog );
    }

    Call( pconverttasks->ErrTerm() );

    if ( NULL != pfnStatus )
    {
        (VOID)pfnStatus( sesid, JET_snpUpgradeRecordFormat, JET_sntComplete, NULL );
    }

    (*CPRINTFSTDOUT::PcprintfInstance())( "%d pages seen\n", pconverttasks->Stats().cpgSeen );

    err = pconverttasks->Stats().err;

    delete pconverttasks;
    pconverttasks = NULL;

    Call( err );

HandleError:
    if( NULL != pconverttasks )
    {
        const ERR errT = pconverttasks->ErrTerm();
        if( err >= 0 && errT < 0 )
        {
            err = errT;
        }
        delete pconverttasks;
    }
    return err;
}


ERR ErrUPGRADEPossiblyConvertPage(
        CPAGE * const pcpage,
        const PGNO pgno,
        VOID * const pvWorkBuf )
{
    ERR err = JET_errSuccess;

    if( pcpage->FNewRecordFormat() )
    {
        return JET_errSuccess;
    }

    if( pcpage->FPrimaryPage()
        && !pcpage->FRepairedPage()
        && pcpage->FLeafPage()
        && !pcpage->FSpaceTree()
        && !pcpage->FLongValuePage() )
    {
        Call( ErrUPGRADEConvertPage( pcpage, pgno, pvWorkBuf ) );
    }
    else
    {
        pcpage->SetFNewRecordFormat();
        BFDirty( pcpage->PBFLatch(), (BFDirtyFlags)GrbitParam( JET_paramRecordUpgradeDirtyLevel ), *TraceContextScope() );
    }

HandleError:
    return err;
}


LOCAL ERR ErrUPGRADECheckConvertNode(
    const LONG          cbPage,
    const VOID* const   pvRecOld,
    const SIZE_T        cbRecOld,
    const VOID* const   pvRecNew,
    const SIZE_T        cbRecNew )
{
    const REC* const    precOld     = reinterpret_cast<const REC *>( pvRecOld );
    const REC* const    precNew     = reinterpret_cast<const REC *>( pvRecNew );

    Assert( cbPage >= g_cbPageMin );

    if( cbRecOld < cbRecNew )
    {
        AssertSz( fFalse, "Record format conversion: the new record is larger" );
        return ErrERRCheck( JET_errRecordFormatConversionFailed );
    }

    if( precOld->FidFixedLastInRec() != precNew->FidFixedLastInRec() )
    {
        AssertSz( fFalse, "Record format conversion: fid fixed last changed" );
        return ErrERRCheck( JET_errRecordFormatConversionFailed );
    }

    if( precOld->FidVarLastInRec() != precNew->FidVarLastInRec() )
    {
        AssertSz( fFalse, "Record format conversion: fid var last changed" );
        return ErrERRCheck( JET_errRecordFormatConversionFailed );
    }

    if( precOld->IbEndOfFixedData() != precNew->IbEndOfFixedData() )
    {
        AssertSz( fFalse, "Record format conversion: end of fixed data changed" );
        return ErrERRCheck( JET_errRecordFormatConversionFailed );
    }

    if( precOld->CbFixedNullBitMap() != precNew->CbFixedNullBitMap() )
    {
        AssertSz( fFalse, "Record format conversion: size of fixed null bitmap changed" );
        return ErrERRCheck( JET_errRecordFormatConversionFailed );
    }

    if( precOld->IbEndOfVarData() != precNew->IbEndOfVarData() )
    {
        AssertSz( fFalse, "Record format conversion: end if var data changed" );
        return ErrERRCheck( JET_errRecordFormatConversionFailed );
    }

    if( !!precOld->FTaggedData( cbRecOld ) != !!precNew->FTaggedData( cbRecNew ) )
    {
        AssertSz( fFalse, "Record format conversion: tagged data missing/added!" );
        return ErrERRCheck( JET_errRecordFormatConversionFailed );
    }

    DATA dataRecNew;
    dataRecNew.SetPv( const_cast<VOID *>( pvRecNew ) );
    dataRecNew.SetCb( cbRecNew );

    if( !TAGFIELDS::FIsValidTagfields( cbPage, dataRecNew, CPRINTFNULL::PcprintfInstance() ) )
    {
        AssertSz( fFalse, "Record format conversion: new record is not valid" );
        return ErrERRCheck( JET_errRecordFormatConversionFailed );
    }


    

    return JET_errSuccess;
}


ERR ErrUPGRADEConvertNode(
    CPAGE * const pcpage,
    const INT iline,
    VOID * const pvBuf )
{
    ERR err = JET_errSuccess;

    Assert( !pcpage->FNewRecordFormat() );


    KEYDATAFLAGS kdf;
    NDIGetKeydataflags( *pcpage, iline, &kdf );

    if( FNDDeleted( kdf ) )
    {
        return JET_errSuccess;
    }

    const INT cbRec             = kdf.data.Cb();
    const BYTE * const pbRec    = reinterpret_cast<const BYTE *>( kdf.data.Pv() );
    const BYTE * const pbRecMax = reinterpret_cast<const BYTE *>( kdf.data.Pv() ) + cbRec;

    const REC * const prec = reinterpret_cast<const REC *>( pbRec );


    const SIZE_T cbNonTaggedData    = prec->PbTaggedData() - pbRec;


    const TAGFLD_OLD * const ptagfldoldMin      = reinterpret_cast<const TAGFLD_OLD *>( prec->PbTaggedData() );
    const TAGFLD_OLD * const ptagfldoldMax      = reinterpret_cast<const TAGFLD_OLD *>( pbRecMax );
    const TAGFLD_OLD *  ptagfldold              = NULL;

    BOOL                fRecordHasMultivalues   = fFalse;
    INT                 cTAGFLD                 = 0;

    FID                 fidPrev                 = 0;
    for( ptagfldold = ptagfldoldMin; ptagfldold < ptagfldoldMax; ptagfldold = ptagfldold->PtagfldNext() )
    {
        if( ptagfldold->Fid() == fidPrev )
        {
            fRecordHasMultivalues = fTrue;
        }
        else
        {
            fidPrev = ptagfldold->Fid();
            ++cTAGFLD;
        }
    }


    BYTE * const pb = reinterpret_cast<BYTE *>( pvBuf );
    memcpy( pb, pbRec, cbNonTaggedData );


    BYTE * const pbTagfldsStart = pb + cbNonTaggedData;
    BYTE * pbTagflds            = pbTagfldsStart;
    BYTE * const pbDataStart    = pbTagfldsStart + ( cTAGFLD * sizeof(TAGFLD) );
    BYTE * pbData               = pbDataStart;

    INT ibCurr = cTAGFLD * sizeof(TAGFLD);

    ptagfldold = ptagfldoldMin;
    while( ptagfldold < ptagfldoldMax )
    {
        static const USHORT fDerived        = 0x8000;
        static const USHORT fExtendedInfo   = 0x4000;
        static const USHORT fNull           = 0x2000;

        const FID fid                           = ptagfldold->Fid();
        const TAGFLD_OLD * const ptagfldoldNext = ptagfldold->PtagfldNext();

        if( ptagfldoldNext < ptagfldoldMax
            && ptagfldoldNext->Fid() == fid
            && ptagfldoldNext->FDerived() == ptagfldold->FDerived() )
        {
            const TAGFLD_OLD *  ptagfldoldNextFid   = ptagfldold;
            INT                 cMULTIVALUES        = 0;
            INT                 cbDataTotal         = 0;

            while(  ptagfldoldNextFid < ptagfldoldMax
                    && ptagfldoldNextFid->Fid() == fid )
            {
                ++cMULTIVALUES;
                cbDataTotal += ptagfldoldNextFid->CbData();
                ptagfldoldNextFid = ptagfldoldNextFid->PtagfldNext();
            }

            if( 2 == cMULTIVALUES
                && !ptagfldold->FLongValue() )
            {

                Assert( !ptagfldold->FLongValue() );
                Assert( !ptagfldoldNext->FLongValue() );
                Assert( !ptagfldold->FNull() );
                Assert( !ptagfldoldNext->FNull() );


                USHORT ibFlags      = fExtendedInfo;
                if( ptagfldold->FDerived() )
                {
                    ibFlags |= fDerived;
                }

                const USHORT ib = static_cast<USHORT>( ibCurr | ibFlags );

                *((UnalignedLittleEndian<USHORT> *)pbTagflds) = fid;
                pbTagflds += sizeof( USHORT );
                *((UnalignedLittleEndian<USHORT> *)pbTagflds) = ib;
                pbTagflds += sizeof( USHORT );

                ibCurr += sizeof( TAGFLD_HEADER ) + sizeof( BYTE ) + cbDataTotal;


                *pbData = 0;
                TAGFLD_HEADER * const ptagfldheader = reinterpret_cast<TAGFLD_HEADER *>( pbData );
                ptagfldheader->SetFTwoValues();
                ptagfldheader->SetFMultiValues();
                pbData += sizeof( TAGFLD_HEADER );


                *pbData = (BYTE)ptagfldold->CbData();
                pbData += sizeof( BYTE );


                memcpy( pbData, ptagfldold->Rgb(), ptagfldold->CbData() );
                pbData      += ptagfldold->CbData();

                memcpy( pbData, ptagfldoldNext->Rgb(), ptagfldoldNext->CbData() );
                pbData      += ptagfldoldNext->CbData();
            }
            else
            {


                USHORT ibFlags      = fExtendedInfo;
                if( ptagfldold->FDerived() )
                {
                    ibFlags |= fDerived;
                }

                const USHORT ib = static_cast<USHORT>( ibCurr | ibFlags );

                *((UnalignedLittleEndian<USHORT> *)pbTagflds) = fid;
                pbTagflds += sizeof( USHORT );
                *((UnalignedLittleEndian<USHORT> *)pbTagflds) = ib;
                pbTagflds += sizeof( USHORT );

                if( !ptagfldold->FLongValue() )
                {
                    ibCurr += sizeof( TAGFLD_HEADER ) + ( sizeof( USHORT ) * cMULTIVALUES ) + cbDataTotal;
                }
                else
                {

                    ibCurr += sizeof( TAGFLD_HEADER ) + ( sizeof( USHORT ) * cMULTIVALUES ) + cbDataTotal - ( sizeof( BYTE ) * cMULTIVALUES );
                }


                *pbData = 0;
                TAGFLD_HEADER * const ptagfldheader = reinterpret_cast<TAGFLD_HEADER *>( pbData );
                ptagfldheader->SetFMultiValues();
                if( ptagfldold->FLongValue() )
                {
                    ptagfldheader->SetFLongValue();
                }
                pbData += sizeof( TAGFLD_HEADER );


                INT     ibOffsetCurr = sizeof( USHORT ) * cMULTIVALUES;
                BYTE * pbIbOffsets = pbData;
                pbData += sizeof( USHORT ) * cMULTIVALUES;

                const TAGFLD_OLD *  ptagfldoldT     = NULL;
                for(    ptagfldoldT = ptagfldold;
                        ptagfldoldT < ptagfldoldNextFid;
                        ptagfldoldT = ptagfldoldT->PtagfldNext() )
                {
                    Assert( ptagfldoldT->Fid() == fid );
                    if( ptagfldoldT->FLongValue() )
                    {


                        const LV * const plv = reinterpret_cast<const LV *>( ptagfldoldT->Rgb() );
                        if( plv->fSeparated )
                        {
                            static const USHORT fSeparatedInstance = 0x8000;
                            *((UnalignedLittleEndian<USHORT> *)pbIbOffsets) = static_cast<USHORT>( ibOffsetCurr | fSeparatedInstance );
                        }
                        else
                        {
                            *((UnalignedLittleEndian<USHORT> *)pbIbOffsets) = static_cast<USHORT>( ibOffsetCurr );
                        }
                        pbIbOffsets += sizeof( USHORT );

                        memcpy( pbData, ptagfldoldT->Rgb() + sizeof( BYTE ), ptagfldoldT->CbData() - sizeof( BYTE ) );
                        ibOffsetCurr += ptagfldoldT->CbData() - sizeof( BYTE );
                        pbData += ptagfldoldT->CbData() - sizeof( BYTE );
                    }
                    else
                    {
                        *((UnalignedLittleEndian<USHORT> *)pbIbOffsets) = static_cast<USHORT>( ibOffsetCurr );
                        pbIbOffsets += sizeof( USHORT );
                        memcpy( pbData, ptagfldoldT->Rgb(), ptagfldoldT->CbData() );
                        ibOffsetCurr += ptagfldoldT->CbData();
                        pbData += ptagfldoldT->CbData();
                    }
                }

            }

            ptagfldold  = ptagfldoldNextFid;
        }
        else
        {
            USHORT ibFlags      = 0;
            if( ptagfldold->FNull() )
            {
                ibFlags |= fNull;
            }
            else
            {
                if( ptagfldold->FLongValue() )
                {
                    ibFlags |= fExtendedInfo;
                }
            }

            if( ptagfldold->FDerived() )
            {
                ibFlags |= fDerived;
            }

            const USHORT ib = static_cast<USHORT>( ibCurr | ibFlags );

            *((UnalignedLittleEndian<USHORT> *)pbTagflds) = fid;
            pbTagflds += sizeof( USHORT );
            *((UnalignedLittleEndian<USHORT> *)pbTagflds) = ib;
            pbTagflds += sizeof( USHORT );

            ibCurr += ptagfldold->CbData();


            memcpy( pbData, ptagfldold->Rgb(), ptagfldold->CbData() );


            if( ibFlags & fExtendedInfo )
            {
                Assert( ptagfldold->FLongValue() );

                const LV * const plv = reinterpret_cast<const LV *>( ptagfldold->Rgb() );
                *pbData = 0;
                TAGFLD_HEADER * const ptagfldheader = reinterpret_cast<TAGFLD_HEADER *>( pbData );
                ptagfldheader->SetFLongValue();
                if( plv->fSeparated )
                {
                    ptagfldheader->SetFSeparated();
                }
            }

            pbData      += ptagfldold->CbData();
            ptagfldold  = ptagfldoldNext;
        }
    }

    const BYTE* const   pbMax       = pbData;
    const SIZE_T        cbRecNew    = pbMax - pb;

    CallR( ErrUPGRADECheckConvertNode( pcpage->CbPage(), kdf.data.Pv(), kdf.data.Cb(), pvBuf, cbRecNew ) );

    DATA data;
    data.SetPv( pvBuf );
    data.SetCb( cbRecNew );

    NDReplaceForUpgrade( pcpage, iline, &data, kdf );

    CallS( err );
    return err;
}


ERR ErrUPGRADEConvertPage(
    CPAGE * const pcpage,
    const PGNO pgno,
    VOID * const pvBuf )
{
    ERR             err     = JET_errSuccess;
    INST * const    pinst   = PinstFromIfmp( pcpage->Ifmp() );

    Assert( NULL != pinst );
    Assert( !pcpage->FNewRecordFormat() );

#ifdef DEBUG
    memset( pvBuf, 0xff, pcpage->CbPage() );
#endif

    INT iline;
    for( iline = 0; iline < pcpage->Clines(); ++iline )
    {
        Call( ErrUPGRADEConvertNode( pcpage, iline, pvBuf ) );
    }

    pcpage->SetFNewRecordFormat();
    BFDirty( pcpage->PBFLatch(), (BFDirtyFlags)GrbitParam( JET_paramRecordUpgradeDirtyLevel ), *TraceContextScope() );

HandleError:
    if( err < 0 )
    {
        if( JET_errRecordFormatConversionFailed == err )
        {


            const WCHAR * rgcwsz[3];
            INT isz = 0;

            const OBJID objid = pcpage->ObjidFDP();
            WCHAR wszObjid[16];
            OSStrCbFormatW( wszObjid, sizeof( wszObjid ), L"%d", objid );

            WCHAR wszPgno[16];
            OSStrCbFormatW( wszPgno, sizeof( wszPgno ), L"%d", pgno );

            WCHAR wszIline[16];
            OSStrCbFormatW( wszIline, sizeof( wszIline ), L"%d", iline );

            rgcwsz[isz++] = wszObjid;
            rgcwsz[isz++] = wszPgno;
            rgcwsz[isz++] = wszIline;

            Assert( isz == _countof( rgcwsz ) );

            UtilReportEvent(
                eventError,
                CONVERSION_CATEGORY,
                RECORD_FORMAT_CONVERSION_FAILED_ID,
                isz,
                rgcwsz,
                0,
                NULL,
                pinst );

        }
        else
        {


            Assert( fFalse );

        }
    }
    else
    {
        CallS( err );
        Assert( pcpage->FNewRecordFormat() );
    }
    return err;
}

#endif

