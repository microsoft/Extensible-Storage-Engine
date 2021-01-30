// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osustd.hxx"
#include "esestd.hxx"


IOFLUSHREASON g_iofrSuppressedFlushed = (IOFLUSHREASON)0;

ERR ErrUtilFlushFileBuffers( _Inout_ IFileAPI * const pfapi, _In_ const IOFLUSHREASON iofr )
{
    Assert( !Ptls()->fFfbFreePath );

    return pfapi->ErrFlushFileBuffers( iofr );
}

static BYTE*    g_rgbDebugHeader = NULL;
static BYTE*    g_rgbDebugShadowHeader = NULL;
static ERR  g_errDebugHeaderIO = JET_errSuccess;
static ERR  g_errDebugShadowHeaderIO = JET_errSuccess;
static TICK g_tickDebugHeader = 0;

LOCAL INLINE BOOL FValidCbPage( const ULONG cb )
{
    return (    0 == cb ||
                g_cbPageDefault == cb ||
                2 * 1024 == cb ||
                4 * 1024 == cb ||
                8 * 1024 == cb ||
                16 * 1024 == cb ||
                32 * 1024 == cb );
}

LOCAL INLINE BOOL FRangeContains( const DWORD ibContainer, const DWORD cbContainer, const DWORD ibContainee, const DWORD cbContainee )
{
    if ( ibContainee < ibContainer ||
            ( ibContainee - ibContainer ) >= cbContainer ||
            cbContainee > cbContainer )
    {
        return fFalse;
    }

    return ( ( ibContainer + cbContainer ) - ibContainee ) >= cbContainee;
}

LOCAL INLINE VOID MergeRange( DWORD* const pibCurrent, DWORD* const pcbCurrent, const DWORD ibNew, const DWORD cbNew )
{
    if ( FRangeContains( *pibCurrent, *pcbCurrent, ibNew, cbNew ) )
    {
        return;
    }
    if ( *pibCurrent < ibNew )
    {
        if ( ( ibNew - *pibCurrent ) > *pcbCurrent )
        {
            *pcbCurrent = cbNew;
            *pibCurrent = ibNew;
        }
        else
        {
            *pcbCurrent = max( *pcbCurrent, ibNew + cbNew - *pibCurrent );
        }
    }
    else
    {
        if ( ( *pibCurrent - ibNew ) > cbNew )
        {
            *pcbCurrent = cbNew;
            *pibCurrent = ibNew;
        }
        else
        {
            *pcbCurrent = max( cbNew, *pibCurrent + *pcbCurrent - ibNew );
            *pibCurrent = ibNew;
        }
    }
}

ERR ErrUtilReadSpecificShadowedHeader( const INST* const pinst, DB_HEADER_READER* const pdbHdrReader )
{
    ERR                     err                         = JET_errSuccess;
    ShadowedHeaderStatus&   shadowedHeaderStatus        = pdbHdrReader->shadowedHeaderStatus;
    ShadowedHeaderRequest   shadowedHeaderRequest       = pdbHdrReader->shadowedHeaderRequest;
    const DWORD             cbHeader                    = pdbHdrReader->cbHeader;
    DWORD&                  cbHeaderActual              = pdbHdrReader->cbHeaderActual;
    DWORD                   cbHeaderElected             = 0;
    IFileAPI*               pfapiRead                   = NULL;
    QWORD                   cbFileSize                  = 0;
    DWORD                   cbIOSize                    = 0;
    QWORD                   cbReadT                     = 0;
    DWORD                   cbAlloc                     = 0;
    DWORD                   ibRead                      = 0;
    DWORD                   cbRead                      = 0;
    BYTE*                   pbRead                      = NULL;
    ULONG                   cOffsetOfPbRead             = 0;
    ULONG                   cbPageCandidate             = 0;
    const ULONG             cbPageDownlevelMin          = 2048;
    const ULONG             fNoAutoDetectPageSize       = pdbHdrReader->fNoAutoDetectPageSize;
    const ULONG             cbPageCandidateMin          = fNoAutoDetectPageSize ? cbHeader : cbPageDownlevelMin;
    const ULONG             cbPageCandidateMax          = fNoAutoDetectPageSize ? cbHeader : g_cbPageMax;
    const LONG              ibPageSize                  = pdbHdrReader->ibPageSize;
    ERR                     errRead                     = JET_errSuccess;
    PAGECHECKSUM            checksumExpected            = 0;
    PAGECHECKSUM            checksumActual              = 0;
    ULONG                   cbPagePrimary               = 0;
    ULONG                   cbPageSecondary             = 0;
    const OSFILEQOS         qos                         = ( pdbHdrReader->pfsapi && pinst ) ?
                                                            QosSyncDefault( pinst ) :
                                                            qosIONormal;
    TraceContextScope       tcHeader                    ( iorpHeader );

    memset( pdbHdrReader->pbHeader, 0, pdbHdrReader->cbHeader );
    shadowedHeaderStatus = shadowedHeaderCorrupt;
    cbHeaderActual = 0;

    if ( !pdbHdrReader->pfapi )
    {
        Call( pdbHdrReader->pfsapi->ErrFileOpen(    pdbHdrReader->wszFileName,
                                                    (   IFileAPI::fmfReadOnly |
                                                        ( BoolParam( JET_paramEnableFileCache ) ?
                                                            IFileAPI::fmfCached :
                                                            IFileAPI::fmfNone ) ),
                                                    &pfapiRead ) );
    }
    else
    {
        pfapiRead = pdbHdrReader->pfapi;
    }

    Call( pfapiRead->ErrSize( &cbFileSize, IFileAPI::filesizeLogical ) );
    Call( pfapiRead->ErrIOSize( &cbIOSize ) );

    cbReadT = min( 2 * cbPageCandidateMax, ( cbFileSize / cbIOSize ) * cbIOSize );
    cbAlloc = (DWORD)cbReadT;
    Assert( QWORD( cbAlloc ) == cbReadT );

    if ( cbAlloc > 0 )
    {
        Alloc( pbRead = (BYTE*)PvOSMemoryPageAlloc( cbAlloc, NULL ) );
        errRead = pfapiRead->ErrIORead( *tcHeader, QWORD( 0 ), cbAlloc, pbRead, qos );

        if ( errRead >= JET_errSuccess )
        {
            MergeRange( &ibRead, &cbRead, 0, cbAlloc );
        }
    }

    for ( cbPageCandidate = cbPageCandidateMin ; cbPageCandidate <= cbPageCandidateMax && cbAlloc > 0 ; cbPageCandidate *= 2 )
    {
        if ( FRangeContains( ibRead, cbRead, 0 * cbPageCandidate, cbPageCandidate ) ||
                ( cbAlloc >= cbPageCandidate &&
                    pfapiRead->ErrIORead( *tcHeader, QWORD( 0 * cbPageCandidate ), cbPageCandidate, pbRead + 0 * cbPageCandidate, qos ) >= JET_errSuccess ) )
        {
            ChecksumPage(   pbRead + 0 * cbPageCandidate,
                            cbPageCandidate,
                            databaseHeader,
                            0,
                            &checksumExpected,
                            &checksumActual );
            if (    checksumActual == checksumExpected ||
                    BoolParam( JET_paramDisableBlockVerification ) )
            {
                if ( ibPageSize >= 0 )
                {
                    if ( cbPageCandidate >= ibPageSize + sizeof( UnalignedLittleEndian<ULONG> ) )
                    {
                        ULONG cbPageHeader = *( (UnalignedLittleEndian<ULONG> *)( pbRead + 0 * cbPageCandidate + ibPageSize ) );
                        if ( FValidCbPage( cbPageHeader ) )
                        {
                            if ( cbPageHeader == 0 )
                            {
                                cbPageHeader = g_cbPageDefault;
                            }
                            if ( cbPageHeader == cbPageCandidate )
                            {
                                cbPagePrimary = cbPageCandidate;
                                MergeRange( &ibRead, &cbRead, 0 * cbPageCandidate, cbPageCandidate );
                            }
                        }
                    }
                }
                else
                {
                    cbPagePrimary = cbPageCandidate;
                }
            }
        }
        if ( FRangeContains( ibRead, cbRead, 1 * cbPageCandidate, cbPageCandidate ) ||
                ( cbAlloc >= 2 * cbPageCandidate &&
                    pfapiRead->ErrIORead( *tcHeader, QWORD( 1 * cbPageCandidate ), cbPageCandidate, pbRead + 1 * cbPageCandidate, qos ) >= JET_errSuccess ) )
        {
            ChecksumPage(   pbRead + 1 * cbPageCandidate,
                            cbPageCandidate,
                            databaseHeader,
                            0,
                            &checksumExpected,
                            &checksumActual );
            if (    checksumActual == checksumExpected ||
                    BoolParam( JET_paramDisableBlockVerification ) )
            {
                if ( ibPageSize >= 0 )
                {
                    if ( cbPageCandidate >= ibPageSize + sizeof( UnalignedLittleEndian<ULONG> ) )
                    {
                        ULONG cbPageHeader = *( (UnalignedLittleEndian<ULONG> *)( pbRead + 1 * cbPageCandidate + ibPageSize ) );
                        if ( FValidCbPage( cbPageHeader ) )
                        {
                            if ( cbPageHeader == 0 )
                            {
                                cbPageHeader = g_cbPageDefault;
                            }
                            if ( cbPageHeader == cbPageCandidate )
                            {
                                cbPageSecondary = cbPageCandidate;
                                MergeRange( &ibRead, &cbRead, 1 * cbPageCandidate, cbPageCandidate );
                            }
                        }
                    }
                }
                else
                {
                    cbPageSecondary = cbPageCandidate;
                }
            }
        }
    }

    if ( !cbPagePrimary && !cbPageSecondary &&
            ( shadowedHeaderRequest == headerRequestGoodOnly || !fNoAutoDetectPageSize ) )
    {

        g_tickDebugHeader = TickOSTimeCurrent();
        
        if ( NULL == g_rgbDebugHeader )
        {
            BYTE * pbAlloc = (BYTE*)PvOSMemoryPageAlloc( cbPageCandidate, NULL );
            if ( NULL != AtomicExchangePointer( (void **)&g_rgbDebugHeader, (void*)pbAlloc ) )
            {
                OSMemoryPageFree( pbAlloc );
            }
        }

        if ( NULL == g_rgbDebugShadowHeader )
        {
            BYTE * pbAlloc = (BYTE*)PvOSMemoryPageAlloc( cbPageCandidate, NULL );
            if ( NULL != AtomicExchangePointer( (void **)&g_rgbDebugShadowHeader, (void*)pbAlloc ) )
            {
                OSMemoryPageFree( pbAlloc );
            }
        }

        if ( NULL != g_rgbDebugHeader )
        {
            g_errDebugHeaderIO = pfapiRead->ErrIORead( *tcHeader, QWORD( 0 * cbPageCandidate ), cbPageCandidate, g_rgbDebugHeader, qos );
        }

        if ( NULL != g_rgbDebugShadowHeader )
        {
            g_errDebugShadowHeaderIO = pfapiRead->ErrIORead( *tcHeader, QWORD( 1 * cbPageCandidate ), cbPageCandidate, g_rgbDebugShadowHeader, qos );
        }

        Error( ErrERRCheck( JET_errReadVerifyFailure ) );
    }

    if (    ( cbPagePrimary && cbPageSecondary ) &&
            (   cbPagePrimary != cbPageSecondary ||
                memcmp( pbRead + 0 * cbPagePrimary, pbRead + 1 * cbPagePrimary, cbPagePrimary ) != 0 ) )
    {
        cbHeaderElected = cbPagePrimary;
        cOffsetOfPbRead = 0;
        shadowedHeaderStatus = shadowedHeaderSecondaryBad;
        Error( JET_errSuccess );
    }

    if ( cbPagePrimary && !cbPageSecondary )
    {
        cbHeaderElected = cbPagePrimary;
        cOffsetOfPbRead = 0;
        shadowedHeaderStatus = shadowedHeaderSecondaryBad;
        Error( JET_errSuccess );
    }

    if ( !cbPagePrimary && cbPageSecondary )
    {
        cbHeaderElected = cbPageSecondary;
        cOffsetOfPbRead = 1;
        shadowedHeaderStatus = shadowedHeaderPrimaryBad;
        Error( JET_errSuccess );
    }

    if ( cbPagePrimary && cbPageSecondary )
    {
        cbHeaderElected = cbPagePrimary;
        cOffsetOfPbRead = 0;
        shadowedHeaderStatus = shadowedHeaderOK;
        Error( JET_errSuccess );
    }

    if ( !cbPagePrimary && !cbPageSecondary )
    {
        Assert( fNoAutoDetectPageSize );
        cbHeaderElected = cbHeader;
        cOffsetOfPbRead = ( shadowedHeaderRequest == headerRequestPrimaryOnly ) ? 0 : 1;
        Error( JET_errSuccess );
    }

    AssertSz( fFalse, "Unexpected case when validating a shadowed header!" );
    Error( ErrERRCheck( JET_errInternalError ) );

HandleError:
    Assert( !(  err >= JET_errSuccess &&
                shadowedHeaderStatus == shadowedHeaderCorrupt &&
                shadowedHeaderRequest != headerRequestPrimaryOnly &&
                shadowedHeaderRequest != headerRequestSecondaryOnly ) );

    if ( cbAlloc > 0 && err == JET_errSuccess )
    {
        Assert( cOffsetOfPbRead == 0 || cOffsetOfPbRead == 1 );

        switch ( shadowedHeaderRequest )
        {
            case headerRequestPrimaryOnly:
                cOffsetOfPbRead                 = 0;
                cbHeaderElected                 = cbPagePrimary ? cbPagePrimary : cbHeaderElected;
                break;

            case headerRequestSecondaryOnly:
                cOffsetOfPbRead                 = 1;
                cbHeaderElected                 = cbPageSecondary ? cbPageSecondary : cbHeaderElected;
                break;

            default:
                AssertSz( fFalse, "This case must be taken care of!" );
                err = ErrERRCheck( JET_errInternalError );
            case headerRequestGoodOnly:
                break;
        }

        const DWORD cbReadToCopyOffset  = cOffsetOfPbRead * cbHeaderElected;
        BYTE* const pbReadToCopy        = pbRead + cbReadToCopyOffset;
        const DWORD cbSrcMax            = ( cbAlloc < cbReadToCopyOffset ) ? 0 : ( cbAlloc - cbReadToCopyOffset );
        const DWORD cbDstMax            = pdbHdrReader->cbHeader;
        const DWORD cbReadToCopy        = min( cbHeaderElected, min( cbSrcMax, cbDstMax ) );
        const DWORD cbReRead            = min( cbHeaderElected, cbSrcMax );

        Assert( pbReadToCopy >= pbRead );
        Assert( cbReadToCopy <= pdbHdrReader->cbHeader );
        Assert( cbReadToCopy <= cbAlloc );

        errRead = JET_errSuccess;
        const BOOL fRangeContainsBytesToCopy = FRangeContains( ibRead, cbRead, cbReadToCopyOffset, cbReadToCopy );
        if ( fRangeContainsBytesToCopy ||
                ( cbReRead > 0 &&
                    ( errRead = pfapiRead->ErrIORead( *tcHeader, QWORD( cbReadToCopyOffset ), cbReRead, pbReadToCopy, qos ) ) >= JET_errSuccess ) )
        {
            if ( !fRangeContainsBytesToCopy )
            {
                MergeRange( &ibRead, &cbRead, cbReadToCopyOffset, cbReRead );
            }
            
            memcpy( pdbHdrReader->pbHeader, pbReadToCopy, cbReadToCopy );

            if ( FRangeContains( ibRead, cbRead, cbReadToCopyOffset, cbHeaderElected ) )
            {
                ChecksumPage(   pbReadToCopy,
                                cbHeaderElected,
                                databaseHeader,
                                0,
                                &pdbHdrReader->checksumExpected,
                                &pdbHdrReader->checksumActual );
            }
            else
            {
                pdbHdrReader->checksumExpected  = 0;
                pdbHdrReader->checksumActual    = 0;
            }

            if ( fNoAutoDetectPageSize )
            {
                cbHeaderActual = cbReadToCopy;
            }
            else
            {
                cbHeaderActual = cbHeaderElected;
            }
        }
        else
        {
            if ( errRead < JET_errSuccess )
            {
                err = ErrERRCheck( JET_errReadVerifyFailure );
            }
        }
    }

    OSMemoryPageFree( pbRead );
    if ( pfapiRead != pdbHdrReader->pfapi )
    {
        delete pfapiRead;
    }
    return err;
}

LOCAL ERR ErrUtilIReadShadowedHeader(
        const INST* const               pinst,
        IFileSystemAPI* const           pfsapi,
        const WCHAR* const              wszFileName,
        __out_bcount( cbHeader ) BYTE*  pbHeader,
        const DWORD                     cbHeader,
        const LONG                      ibPageSize,
        ShadowedHeaderStatus* const     pShadowedHeaderStatus,
        IFileAPI* const                 pfapi,
        DWORD* const                    pcbHeaderActual,
        const UtilReadHeaderFlags       urhf )
{
    ERR                     err                     = JET_errSuccess;
    DWORD                   cbHeaderActualT = 0;
    DWORD&                  cbHeaderActual          = pcbHeaderActual ? *pcbHeaderActual : cbHeaderActualT;
    const BOOL              fNoEventLogging         = ( urhf & urhfNoEventLogging ) != 0;
    const BOOL              fNoFailOnPageMismatch   = ( urhf & urhfNoFailOnPageMismatch ) != 0;
    const BOOL              fNoAutoDetectPageSize   = ( urhf & urhfNoAutoDetectPageSize ) != 0;

    DB_HEADER_READER dbHeaderReader =
    {
        headerRequestGoodOnly,
        wszFileName,
        pbHeader,
        cbHeader,
        ibPageSize,
        pfsapi,
        pfapi,
        fNoAutoDetectPageSize,

        0,
        0,
        0,
        shadowedHeaderCorrupt
    };

    err = ErrUtilReadSpecificShadowedHeader( pinst, &dbHeaderReader );
    if ( !fNoEventLogging && ( err == JET_errReadVerifyFailure ) )
    {
        OSUHAEmitFailureTag( pinst, HaDbFailureTagCorruption, L"922f6ba1-c846-42d8-bd57-72844b4dd49e" );
    }
    Call( err );
    *pShadowedHeaderStatus = dbHeaderReader.shadowedHeaderStatus;
    cbHeaderActual = dbHeaderReader.cbHeaderActual;

    Assert( *pShadowedHeaderStatus != shadowedHeaderCorrupt );

    if ( !fNoEventLogging &&
            ( ( *pShadowedHeaderStatus == shadowedHeaderPrimaryBad ) ||
            ( *pShadowedHeaderStatus == shadowedHeaderSecondaryBad ) ) )
    {
        WCHAR wszPageSize[12];
        OSStrCbFormatW( wszPageSize, sizeof(wszPageSize), L"%u", cbHeaderActual );

        const WCHAR* rgwsz [] =
        {
            wszFileName,
            wszPageSize
        };
        
        UtilReportEvent(
            eventWarning,
            LOGGING_RECOVERY_CATEGORY,
            ( shadowedHeaderPrimaryBad == *pShadowedHeaderStatus ?
                PRIMARY_PAGE_READ_FAIL_ID :
                SHADOW_PAGE_READ_FAIL_ID ),
            _countof(rgwsz),
            rgwsz,
            0,
            NULL,
            pinst );
    }

    if ( !fNoFailOnPageMismatch && ( cbHeader != cbHeaderActual ) )
    {
        if ( !fNoEventLogging )
        {
            OSUHAEmitFailureTag( pinst, HaDbFailureTagConfiguration, L"4b225aae-fd65-4f3d-82e3-c50eb5902bff" );
        }
        Error( ErrERRCheck( JET_errPageSizeMismatch ) );
    }

HandleError:
    return err;
}

ERR ErrUtilReadShadowedHeader(
    const INST* const               pinst,
    IFileSystemAPI* const           pfsapi,
    const WCHAR* const              wszFilePath,
    __out_bcount( cbHeader ) BYTE*  pbHeader,
    const DWORD                     cbHeader,
    const LONG                      ibPageSize,
    const UtilReadHeaderFlags       urhf,
    DWORD* const                    pcbHeaderActual,
    ShadowedHeaderStatus* const     pShadowedHeaderStatus )
{
    ERR         err         = JET_errSuccess;
    IFileAPI*   pfapi       = NULL;
    const BOOL  fReadOnly   = ( urhf & urhfReadOnly ) != 0;

    Call( pfsapi->ErrFileOpen(  wszFilePath,
                                (   ( fReadOnly ? IFileAPI::fmfReadOnly : IFileAPI::fmfNone ) |
                                    ( BoolParam( JET_paramEnableFileCache ) ?
                                        IFileAPI::fmfCached :
                                        IFileAPI::fmfNone ) ),
                                &pfapi ) );
    Call( ErrUtilReadShadowedHeader( pinst, pfsapi, pfapi, pbHeader, cbHeader, ibPageSize, urhf, pcbHeaderActual, pShadowedHeaderStatus ) );

HandleError:
    delete pfapi;

    return err;
}

ERR ErrUtilReadShadowedHeader(
    const INST* const               pinst,
    IFileSystemAPI* const           pfsapi,
    IFileAPI* const                 pfapi,
    __out_bcount( cbHeader ) BYTE*  pbHeader,
    const DWORD                     cbHeader,
    const LONG                      ibPageSize,
    const UtilReadHeaderFlags       urhf,
    DWORD* const                    pcbHeaderActual,
    ShadowedHeaderStatus* const     pShadowedHeaderStatus )
{
    ERR                     err                     = JET_errSuccess;
    ShadowedHeaderStatus    shadowedHeaderStatusT;
    ShadowedHeaderStatus&   shadowedHeaderStatus    = pShadowedHeaderStatus ? *pShadowedHeaderStatus : shadowedHeaderStatusT;
    const OSFILEQOS         qos                     = ( pfsapi && pinst ) ?
                                                            QosSyncDefault( pinst ) :
                                                            qosIONormal;
    const BOOL              fReadOnly               = ( urhf & urhfReadOnly ) != 0;
    const BOOL              fNoEventLogging         = ( urhf & urhfNoEventLogging ) != 0;
    WCHAR                   wszFilePath[OSFSAPI_MAX_PATH] = { L'\0' };
    BOOL                    fPatchedHeader          = fFalse;

    Assert( pfapi != NULL );

    CallS( pfapi->ErrPath( wszFilePath ) );

    Call( ErrUtilIReadShadowedHeader(   pinst,
                                        pfsapi,
                                        wszFilePath,
                                        pbHeader,
                                        cbHeader,
                                        ibPageSize,
                                        &shadowedHeaderStatus,
                                        pfapi,
                                        pcbHeaderActual,
                                        urhf ) );

    if ( !fReadOnly )
    {
        Assert( shadowedHeaderStatus != shadowedHeaderCorrupt );

        if ( ( shadowedHeaderStatus == shadowedHeaderPrimaryBad ) ||
                ( shadowedHeaderStatus == shadowedHeaderSecondaryBad ) )
        {
            TraceContextScope tcScope( iorpPatchFix );
            tcScope->iorReason.SetIors( iorsHeader );

            err = pfapi->ErrIOWrite(    *tcScope,
                                        QWORD( ( shadowedHeaderStatus == shadowedHeaderPrimaryBad ) ?
                                            0 :
                                            cbHeader ),
                                        cbHeader,
                                        pbHeader,
                                        qos );
            fPatchedHeader = fTrue;
            
            if ( !fNoEventLogging && ( err < JET_errSuccess ) )
            {
                WCHAR szT[12];
                const WCHAR* rgszT[2] = { wszFilePath, szT };
                OSStrCbFormatW( szT, sizeof( szT ), L"%d", err );

                UtilReportEvent(
                        eventError,
                        LOGGING_RECOVERY_CATEGORY,
                        SHADOW_PAGE_WRITE_FAIL_ID,
                        2,
                        rgszT,
                        0,
                        NULL,
                        pinst );
            }
        }
    }

HandleError:

    if ( fPatchedHeader )
    {
        const ERR errFlush = pfapi->ErrFlushFileBuffers( iofrUtility );
        if ( err >= JET_errSuccess )
        {
            err = errFlush;
        }
    }

    return err;
}

void AssertDatabaseHeaderConsistent( const DBFILEHDR * const pdbfilehdr, const DWORD cbDbHeader, const DWORD cbPage )
{
#ifdef DEBUG
    const BYTE* pbDbHeaderT         = NULL;

    Assert( pdbfilehdr != NULL );
    Assert( cbDbHeader >= offsetof( DBFILEHDR, rgbReserved ) );
    Assert( cbDbHeader <= cbPage );

    
    Assert( pdbfilehdr->le_filetype == JET_filetypeDatabase );

    if ( cbDbHeader < cbPage )
    {
        pbDbHeaderT = (BYTE*)PvOSMemoryPageAlloc( cbPage, NULL );

        if ( pbDbHeaderT != NULL )
        {
            memcpy( (VOID*)pbDbHeaderT, (VOID*)pdbfilehdr, cbDbHeader );

            Assert( cbDbHeader >= ( offsetof( DBFILEHDR, le_filetype ) + sizeof( pdbfilehdr->le_filetype ) ) );
        }
    }
    else
    {
        pbDbHeaderT = (const BYTE*)pdbfilehdr;
    }


    if ( pbDbHeaderT != NULL )
    {

        if ( CmpDbVer( pdbfilehdr->Dbv(), PfmtversEngineMax()->dbv ) <= 0 )
        {
            for ( DWORD ibReserved = 0; ibReserved < sizeof( pdbfilehdr->rgbReserved ); ibReserved++ )
            {
                Assert( pdbfilehdr->rgbReserved[ ibReserved ] == 0 );
                if ( pdbfilehdr->rgbReserved[ ibReserved ] != 0 )
                {
                    break;
                }
            }
            
            for ( DWORD ibHeader = sizeof( DBFILEHDR ); ibHeader < cbPage; ibHeader++ )
            {
                Assert( pbDbHeaderT[ ibHeader ] == 0 );
                if ( pbDbHeaderT[ ibHeader ] == 0 )
                {
                    break;
                }
            }
        }


        PAGECHECKSUM checksumExpected   = 0;
        PAGECHECKSUM checksumActual     = 0;

        ChecksumPage(
                    pbDbHeaderT,
                    cbPage,
                    databaseHeader,
                    0,
                    &checksumExpected,
                    &checksumActual );

        Assert( checksumExpected == checksumActual );

        if ( pbDbHeaderT != (const BYTE * const)pdbfilehdr )
        {
            OSMemoryPageFree( (VOID*)pbDbHeaderT );
        }
    }
#endif
}

LOCAL ERR ErrUtilWriteShadowedHeaderInternal(
    const INST* const       pinst,
    IFileSystemAPI* const   pfsapi,
    const IOREASONPRIMARY   iorp,
    const WCHAR*            wszFileName,
    BYTE*                   pbHeader,
    const DWORD             cbHeader,
    IFileAPI* const         pfapi )
{
    ERR             err         = JET_errSuccess;
    IFileAPI*       pfapiT      = NULL;
    const OSFILEQOS qos         = ( pfsapi && pinst ) ? QosSyncDefault( pinst ) : qosIONormal;
    BOOL            fHeaderRO   = fFalse;
    TraceContextScope tcHeader( iorp );
    tcHeader->iorReason.SetIors( iorsHeader );
    tcHeader->nParentObjectClass = tceNone;
    tcHeader->dwEngineObjid = 0;

    Assert( pfapi != NULL || wszFileName != NULL );

    if ( !pfapi )
    {
        pfapiT = NULL;


        err = pfsapi->ErrFileOpen(  wszFileName,
                                    BoolParam( JET_paramEnableFileCache ) ?
                                        IFileAPI::fmfCached :
                                        IFileAPI::fmfNone,
                                    &pfapiT );


        if ( JET_errFileNotFound == err )
        {

            Call( pfsapi->ErrFileCreate(    wszFileName,
                                            BoolParam( JET_paramEnableFileCache ) ?
                                                IFileAPI::fmfCached :
                                                IFileAPI::fmfNone,
                                            &pfapiT ) );
            Call( pfapiT->ErrSetSize( *tcHeader, 2 * QWORD( cbHeader ), fTrue, qos ) );
        }
        else
        {
            Call( err );
        }
    }
    else
    {
        pfapiT = pfapi;
    }

    ULONG cbSectorSize = 0;
    Call( pfapiT->ErrSectorSize( &cbSectorSize ) );
    if ( ( cbSectorSize == 0 ) ||
         ( ( cbHeader % cbSectorSize ) != 0 ) )
    {
        OSUHAEmitFailureTag( pinst, HaDbFailureTagConfiguration, L"fca51f8a-5caa-43b5-a50e-d56ed665dc3b" );

        Assert( cbSectorSize != 0 );
        Error( ErrERRCheck( JET_errSectorSizeNotSupported ) );
    }


    SetPageChecksum( pbHeader, cbHeader, databaseHeader, 0 );


    OSMemoryPageProtect( pbHeader, cbHeader );
    fHeaderRO = fTrue;


    Call( pfapiT->ErrIOWrite( *tcHeader, QWORD( 0 ), cbHeader, pbHeader, qos ) );
    Call( pfapiT->ErrIOWrite( *tcHeader, QWORD( cbHeader ), cbHeader, pbHeader, qos ) );

HandleError:
    if ( fHeaderRO )
    {
        OSMemoryPageUnprotect( pbHeader, cbHeader );
    }

    if ( err < 0 )
    {
        WCHAR wszFilePath[OSFSAPI_MAX_PATH] = { L'\0' };
        const WCHAR* pwszFilePath = wszFileName;
        if ( pwszFilePath == NULL )
        {
            const ERR errT = pfapiT->ErrPath( wszFilePath );
            if ( errT < JET_errSuccess )
            {
                pwszFilePath = L"";
            }
            else
            {
                pwszFilePath = wszFilePath;
            }
        }

        WCHAR szT[16];
        const WCHAR* rgszT[2] = { pwszFilePath, szT };
        OSStrCbFormatW( szT, sizeof( szT ), L"%d", err );

        UtilReportEvent(
                eventError,
                LOGGING_RECOVERY_CATEGORY,
                SHADOW_PAGE_WRITE_FAIL_ID,
                2,
                rgszT,
                0,
                NULL,
                pinst );
    }

    if ( pfapi == NULL && pfapiT != NULL )
    {
        const ERR errT = ErrUtilFlushFileBuffers( pfapiT, iofrDefensiveFileHdrWrite );
        if ( err == JET_errSuccess )
        {
            err = errT;
        }
        delete pfapiT;
    }

    return err;
}

LOCAL ERR ErrUtilWriteDatabaseHeadersInternal(  const INST* const           pinst,
                                                IFileSystemAPI *const       pfsapi,
                                                const WCHAR                 *wszFileName,
                                                DBFILEHDR                   *pdbfilehdr,
                                                IFileAPI *const             pfapi )
{
    Assert( pdbfilehdr->le_filetype == JET_filetypeDatabase );

    ULONG cbPage = pdbfilehdr->le_cbPageSize;
    Assert( 1024 * 4 <= cbPage && cbPage <= 1024 * 32 );
    Assert( !( cbPage & ( cbPage - 1 ) ) );

    Assert( pdbfilehdr->le_ulVersion != 0 );
    Assert( CmpDbVer( pdbfilehdr->Dbv(), PfmtversEngineMax()->dbv ) <= 0 ||
            FUpdateMinorMismatch( CmpDbVer( pdbfilehdr->Dbv(), PfmtversEngineMax()->dbv ) ) ||
            ( FNegTest( fInvalidUsage ) && FNegTest( fCorruptingDbHeaders ) ) );

    Assert( pdbfilehdr->Dbstate() >= JET_dbstateJustCreated &&
            pdbfilehdr->Dbstate() <= JET_dbstateRevertInProgress );

    Assert( pdbfilehdr->le_lGenMinRequired <= pdbfilehdr->le_lGenMaxRequired );
    Assert( pdbfilehdr->le_lGenMaxRequired <= pdbfilehdr->le_lGenMaxCommitted );
    Assert( pdbfilehdr->le_lGenMinConsistent >= pdbfilehdr->le_lGenMinRequired ||
            pdbfilehdr->le_lGenMinConsistent == 0 );
    Assert( pdbfilehdr->le_lGenMinConsistent <= pdbfilehdr->le_lGenMaxRequired );
    Assert( pdbfilehdr->le_lGenRecovering == 0 ||
            pdbfilehdr->Dbstate() == JET_dbstateCleanShutdown ||
            pdbfilehdr->le_lGenRecovering <= pdbfilehdr->le_lGenMaxCommitted );

    Assert( ( pdbfilehdr->Dbstate() == JET_dbstateDirtyShutdown ||
              pdbfilehdr->Dbstate() == JET_dbstateDirtyAndPatchedShutdown ) ||
            ( pdbfilehdr->le_lGenPreRedoMinConsistent == 0 &&
              pdbfilehdr->le_lGenPreRedoMinRequired == 0 ) );
    Assert( ( pdbfilehdr->le_lGenPreRedoMinConsistent == 0 ) ||
            ( ( pdbfilehdr->le_lGenPreRedoMinConsistent > pdbfilehdr->le_lGenMinConsistent ) &&
              ( pdbfilehdr->le_lGenPreRedoMinConsistent <= pdbfilehdr->le_lGenMaxRequired ) ) );
    Assert( ( pdbfilehdr->le_lGenPreRedoMinRequired == 0 ) ||
            ( ( pdbfilehdr->le_lGenPreRedoMinRequired > pdbfilehdr->le_lGenMinRequired ) &&
              ( pdbfilehdr->le_lGenPreRedoMinRequired <= pdbfilehdr->le_lGenMaxRequired ) ) );

    Assert(
            ( CmpLgpos( pdbfilehdr->le_lgposLastResize, lgposMin ) == 0 ) ||

            ( ( CmpLgpos( pdbfilehdr->le_lgposLastResize, pdbfilehdr->le_lgposAttach ) >= 0 ) &&

              ( pdbfilehdr->Dbstate() != JET_dbstateCleanShutdown ) ||

                ( ( CmpLgpos( pdbfilehdr->le_lgposLastResize, pdbfilehdr->le_lgposConsistent ) == 0 ) ||

                  ( ( CmpLgpos( pdbfilehdr->le_lgposAttach, pdbfilehdr->le_lgposConsistent ) > 0 ) ) &&
                    ( g_fRepair || ( pdbfilehdr->le_ulRepairCount > 0 ) ) ) ) );


    const ERR err = ErrUtilWriteShadowedHeaderInternal( pinst,
                                                        pfsapi, 
                                                        iorpHeader,
                                                        wszFileName,
                                                        (BYTE*) pdbfilehdr,
                                                        g_cbPage,
                                                        pfapi );

    if ( err >= JET_errSuccess )
    {
        AssertDatabaseHeaderConsistent( pdbfilehdr, g_cbPage, g_cbPage );
    }

    return err;
}

ERR ErrUtilWriteAttachedDatabaseHeaders(    const INST* const           pinst,
                                            IFileSystemAPI *const       pfsapi,
                                            const WCHAR                 *wszFileName,
                                            FMP *const                  pfmp,
                                            IFileAPI *const             pfapi )
{
    ERR err                         = JET_errSuccess;
    DBFILEHDR* pdbfilehdr           = NULL;
    CFlushMap* const pfm            = pfmp->PFlushMap();
    CRevertSnapshot* const prbs     = pfmp->PRBS();
    BOOL fRBSFormatFeatureEnabled   = pfmp->ErrDBFormatFeatureEnabled( JET_efvRevertSnapshot ) >= JET_errSuccess;

    Assert( pfmp != NULL );
    Assert( pfmp->Pdbfilehdr() != NULL );
    Assert( ( pfm != NULL ) || ( pfmp->Dbid() == dbidTemp ) );

    SIGNATURE signDbHdrFlushNew, signFlushMapHdrFlushNew, signDbHdrFlushOld, signFlushMapHdrFlushOld, signRBSHdrFlushNew, signRBSHdrFlushOld;
    CFlushMap::EnterDbHeaderFlush( pfm, &signDbHdrFlushNew, &signFlushMapHdrFlushNew );
    CRevertSnapshot::EnterDbHeaderFlush( prbs, &signRBSHdrFlushNew );

    Alloc( pdbfilehdr = (DBFILEHDR*)PvOSMemoryPageAlloc( g_cbPage, NULL ) );
    UtilMemCpy( pdbfilehdr, pfmp->Pdbfilehdr().get(), sizeof( DBFILEHDR ) );

    UtilMemCpy( &signDbHdrFlushOld, &pdbfilehdr->signDbHdrFlush, sizeof( SIGNATURE ) );
    UtilMemCpy( &signFlushMapHdrFlushOld, &pdbfilehdr->signFlushMapHdrFlush, sizeof( SIGNATURE ) );
    UtilMemCpy( &signRBSHdrFlushOld, &pdbfilehdr->signRBSHdrFlush, sizeof( SIGNATURE ) );

    UtilMemCpy( &pdbfilehdr->signDbHdrFlush, &signDbHdrFlushNew, sizeof( SIGNATURE ) );
    UtilMemCpy( &pdbfilehdr->signFlushMapHdrFlush, &signFlushMapHdrFlushNew, sizeof( SIGNATURE ) );

    if ( fRBSFormatFeatureEnabled )
    {
        UtilMemCpy( &pdbfilehdr->signRBSHdrFlush, &signRBSHdrFlushNew, sizeof( SIGNATURE ) );
    }

    err = ErrUtilWriteDatabaseHeadersInternal( pinst, pfsapi, wszFileName, pdbfilehdr, pfapi );

    if ( err >= JET_errSuccess )
    {
        PdbfilehdrReadWrite pdbfilehdrUpdateable = pfmp->PdbfilehdrUpdateable();
        pdbfilehdrUpdateable->le_ulChecksum = pdbfilehdr->le_ulChecksum;
        UtilMemCpy( &pdbfilehdrUpdateable->signDbHdrFlush, &signDbHdrFlushNew, sizeof( SIGNATURE ) );
        UtilMemCpy( &pdbfilehdrUpdateable->signFlushMapHdrFlush, &signFlushMapHdrFlushNew, sizeof( SIGNATURE ) );

        if ( fRBSFormatFeatureEnabled )
        {
            UtilMemCpy( &pdbfilehdrUpdateable->signRBSHdrFlush, &signRBSHdrFlushNew, sizeof( SIGNATURE ) );
        }

#ifdef DEBUG
        if ( !pfmp->FHeaderUpdatePending() )
        {
            pfmp->SetDbHeaderUpdateState( FMP::DbHeaderUpdateState::dbhusUpdateFlushed );
        }
#endif
    }

    Call( err );

HandleError:

    OSMemoryPageFree( pdbfilehdr );
     
    CFlushMap::LeaveDbHeaderFlush( pfm );

    return err;
}

ERR ErrUtilWriteUnattachedDatabaseHeaders(  const INST *const               pinst,
                                            IFileSystemAPI *const           pfsapi,
                                            const WCHAR                     *wszFileName,
                                            DBFILEHDR                       *pdbfilehdr,
                                            IFileAPI *const                 pfapi,
                                            CFlushMapForUnattachedDb *const pfm,
                                            BOOL                            fResetRBSHdrFlush )
{
    SIGNATURE signDbHdrFlushNew, signFlushMapHdrFlushNew, signDbHdrFlushOld, signFlushMapHdrFlushOld;
    CFlushMap::EnterDbHeaderFlush( pfm, &signDbHdrFlushNew, &signFlushMapHdrFlushNew );

    UtilMemCpy( &signDbHdrFlushOld, &pdbfilehdr->signDbHdrFlush, sizeof( SIGNATURE ) );
    UtilMemCpy( &signFlushMapHdrFlushOld, &pdbfilehdr->signFlushMapHdrFlush, sizeof( SIGNATURE ) );

    UtilMemCpy( &pdbfilehdr->signDbHdrFlush, &signDbHdrFlushNew, sizeof( SIGNATURE ) );
    UtilMemCpy( &pdbfilehdr->signFlushMapHdrFlush, &signFlushMapHdrFlushNew, sizeof( SIGNATURE ) );

    if ( fResetRBSHdrFlush )
    {
        SIGResetSignature( &pdbfilehdr->signRBSHdrFlush );
    }

    const ERR err = ErrUtilWriteDatabaseHeadersInternal( pinst, pfsapi, wszFileName, pdbfilehdr, pfapi );

    if ( err < JET_errSuccess )
    {
        UtilMemCpy( &pdbfilehdr->signDbHdrFlush, &signDbHdrFlushOld, sizeof( SIGNATURE ) );
        UtilMemCpy( &pdbfilehdr->signFlushMapHdrFlush, &signFlushMapHdrFlushOld, sizeof( SIGNATURE ) );
        
    }

    CFlushMap::LeaveDbHeaderFlush( pfm );

    return err;
}

ERR ErrUtilWriteCheckpointHeaders(  const INST * const      pinst,
                                    IFileSystemAPI *const   pfsapi,
                                    const WCHAR             *wszFileName,
                                    const IOFLUSHREASON     iofr,
                                    CHECKPOINT              *pChkptHeader,
                                    IFileAPI *const         pfapi )
{
    ERR err = JET_errSuccess;

    Assert( pChkptHeader->checkpoint.le_filetype == JET_filetypeCheckpoint );
    
    CallR( ErrUtilWriteShadowedHeaderInternal(  pinst,
                                                pfsapi,
                                                iorpCheckpoint,
                                                wszFileName,
                                                (BYTE*)pChkptHeader,
                                                sizeof( *pChkptHeader ),
                                                pfapi ) );

    if ( pfapi != NULL && iofr != 0 )
    {
        CallR( ErrUtilFlushFileBuffers( pfapi, iofr ) );
    }

    CallS( err );
    return err;
}

ERR ErrUtilFullPathOfFile(
    IFileSystemAPI* const   pfsapi,
    __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const           wszPathOnly,
    const WCHAR* const      wszFile )
{
    ERR                     err;
    WCHAR                   wszAbsPath[IFileSystemAPI::cchPathMax];
    WCHAR                   wszDir[IFileSystemAPI::cchPathMax];
    WCHAR                   wszT[IFileSystemAPI::cchPathMax];

    CallR( pfsapi->ErrPathComplete( wszFile, wszAbsPath ) );
    CallR( pfsapi->ErrPathParse( wszAbsPath, wszDir, wszT, wszT ) );
    wszT[0] = L'\0';
    CallR( pfsapi->ErrPathBuild( wszDir, wszT, wszT, wszPathOnly ) );
    return JET_errSuccess;
}

PWSTR OSSTRCharNext( PCWSTR const pwsz )
{
    return const_cast< WCHAR *const >( L'\0' != *pwsz ? pwsz + 1 : pwsz );
}
PWSTR OSSTRCharPrev( PCWSTR const pwszBase, PCWSTR const pwsz )
{
    return const_cast< WCHAR *const >( pwsz > pwszBase ? pwsz - 1 : pwsz );
}

ERR ErrUtilCreatePathIfNotExist(
    IFileSystemAPI *const   pfsapi,
    const WCHAR *           wszPath,
    _Out_opt_bytecap_(cbAbsPath)  WCHAR *const          wszAbsPath,
    _In_range_(0, cbOSFSAPI_MAX_PATHW) ULONG    cbAbsPath )
{
    ERR                     err = JET_errSuccess;
    WCHAR                   wszPathT[IFileSystemAPI::cchPathMax];
    WCHAR                   *pwsz, *pwszT, *pwszEnd, *pwszMove;
    WCHAR                   ch;
    BOOL                    fFileName = fFalse;

    if ( wszAbsPath && cbAbsPath < cbOSFSAPI_MAX_PATHW )
    {
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if( LOSStrLengthW( wszPath ) >= IFileSystemAPI::cchPathMax )
    {
        Call( ErrERRCheck( JET_errInvalidPath ) );
    }
    
    OSStrCbCopyW( wszPathT, sizeof( wszPathT ), wszPath );
    pwszEnd = OSSTRCharPrev( wszPathT, wszPathT + LOSStrLengthW( wszPathT ) );
    while ( *pwszEnd != wchPathDelimiter && pwszEnd >= wszPathT )
    {
        fFileName = fTrue;
        pwszMove = OSSTRCharPrev( wszPathT, pwszEnd );
        if ( pwszMove < pwszEnd )
        {
            pwszEnd = pwszMove;
        }
        else
        {
            Assert( pwszMove == wszPathT );

            Call( pfsapi->ErrPathComplete( wszPath, wszAbsPath ) );

            err = JET_errSuccess;
            goto HandleError;
        }
    }

    Assert( *pwszEnd == wchPathDelimiter );
    pwszEnd[1] = L'\0';

    pwsz = pwszEnd;
    do
    {
        Assert( *pwsz == wchPathDelimiter );
        ch = pwsz[1];
        pwsz[1] = L'\0';

        err = ErrUtilDirectoryValidate( pfsapi, wszPathT );
        pwsz[1] = ch;
        if ( err == JET_errInvalidPath )
        {
            pwszT = OSSTRCharPrev( wszPathT, pwsz );
            while ( *pwszT != wchPathDelimiter && pwszT >= wszPathT )
            {
                pwszMove = OSSTRCharPrev( wszPathT, pwszT );
                if ( pwszMove < pwszT )
                {
                    pwszT = pwszMove;
                }
                else
                {
                    Assert( pwszMove == wszPathT );

                    goto BeginCreation;
                }
            }

            pwsz = pwszT;
        }
        else
        {
            CallSx( err, JET_errFileAccessDenied );
            Call( err );
        }
    }
    while ( err == JET_errInvalidPath );

    while ( pwsz < pwszEnd )
    {
        Assert( *pwsz == wchPathDelimiter );
        pwsz++;
        while ( *pwsz != wchPathDelimiter )
        {
#ifdef DEBUG
            pwszMove = OSSTRCharNext( pwsz );

            Assert( pwszMove <= pwszEnd );

            Assert( pwszMove > pwsz );

            pwsz = pwszMove;
#else
            pwsz = OSSTRCharNext( pwsz );
#endif
        }

BeginCreation:
        Assert( pwsz <= pwszEnd );
        Assert( *pwsz == wchPathDelimiter );

        ch = pwsz[0];
        pwsz[0] = L'\0';
        err = ErrUtilPathExists( pfsapi, wszPathT );
        if ( err >= JET_errSuccess )
        {
            BOOL fIsDirectory = fFalse;
            Call( pfsapi->ErrPathExists( wszPathT, &fIsDirectory ) );

            if ( !fIsDirectory )
            {
                Error( ErrERRCheck( JET_errInvalidPath ) );
            }

            pwsz[0] = ch;
            continue;
        }
        Call( err == JET_errFileNotFound ? JET_errSuccess : err );
        pwsz[0] = ch;

        ch = pwsz[1];
        pwsz[1] = L'\0';
        err = pfsapi->ErrFolderCreate( wszPathT );
        Call( err == JET_errFileAlreadyExists ? JET_errSuccess : err );
        pwsz[1] = ch;
    }

    CallS( ErrUtilDirectoryValidate( pfsapi, wszPathT, wszAbsPath ) );
    if ( fFileName && wszAbsPath )
    {
        CallS( pfsapi->ErrPathFolderNorm( wszAbsPath, cbAbsPath ) );

#ifdef DEBUG
        Assert( *pwszEnd == wchPathDelimiter );
        Assert( pwszEnd[1] == L'\0' );
        pwszT = const_cast< WCHAR * >( wszPath ) + ( pwszEnd - wszPathT );
        Assert( *pwszT == wchPathDelimiter );
        Assert( pwszT[1] != L'\0' );
#endif
        OSStrCbAppendW( wszAbsPath, cbAbsPath, wszPath + ( pwszEnd - wszPathT ) + 1 );
    }

HandleError:
    
    return err;
}

LOCAL ERR ErrUtilPathExistsByFindFirst(
    IFileSystemAPI* const   pfsapi,
    const WCHAR* const      wszPath,
    __out_bcount_opt(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const           wszAbsPath )
{
    ERR                     err     = JET_errSuccess;
    IFileFindAPI*           pffapi  = NULL;

    Call( pfsapi->ErrFileFind( wszPath, &pffapi ) );
    Call( pffapi->ErrNext() );
    if ( wszAbsPath )
    {
        Call( pffapi->ErrPath( wszAbsPath ) );
    }

    delete pffapi;
    return JET_errSuccess;

HandleError:
    delete pffapi;
    if ( wszAbsPath )
    {
        wszAbsPath[0] = L'\0';
    }
    return err;
}


ERR ErrUtilPathExists(
    IFileSystemAPI* const   pfsapi,
    const WCHAR* const      wszPath,
    __out_bcount_opt(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const           wszAbsPath )
{
    ERR                     err     = JET_errSuccess;
    BOOL                    fIsDirectory = fFalse;
    WCHAR                   wszAbsPathLocal[ OSFSAPI_MAX_PATH ];
    WCHAR* const&           wszAbsPathWritable = wszAbsPath ? wszAbsPath : wszAbsPathLocal;

    wszAbsPathWritable[0] = L'\0';

    C_ASSERT( sizeof( wszAbsPathLocal ) == cbOSFSAPI_MAX_PATHW );
    err = ErrUtilPathExistsByFindFirst( pfsapi, wszPath, wszAbsPathWritable );

    if ( JET_errAccessDenied == err || JET_errFileAccessDenied == err )
    {
        Call( pfsapi->ErrPathComplete( wszPath, wszAbsPathWritable ) );
        Call( pfsapi->ErrPathExists( wszAbsPathWritable, &fIsDirectory ) );
    }

HandleError:
    return err;
}



ERR ErrUtilGetLogicalFileSize(  IFileSystemAPI* const   pfsapi,
                                const WCHAR* const      wszPath,
                                QWORD* const            pcbFileSize )
{
    JET_ERR err = JET_errSuccess;
    IFileAPI* pfapi = NULL;

    Call( pfsapi->ErrFileOpen( wszPath, IFileAPI::fmfReadOnly, &pfapi ) );
    Call( pfapi->ErrSize( pcbFileSize, IFileAPI::filesizeLogical ) );

HandleError:

    delete pfapi;
    return err;
}


ERR ErrUtilPathComplete(
    IFileSystemAPI* const   pfsapi,
    const WCHAR* const      wszPath,
    __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const           wszAbsPath,
    const BOOL              fPathMustExist )
{
    ERR                     err;
    WCHAR                   wszName[IFileSystemAPI::cchPathMax];

    err = pfsapi->ErrPathComplete( wszPath, wszName );
    Assert( JET_errFileNotFound != err );
    Call( err );


    err = ErrUtilPathExists( pfsapi, wszName, wszAbsPath );
    if( JET_errFileNotFound == err && !fPathMustExist )
    {
        OSStrCbCopyW( wszAbsPath, OSFSAPI_MAX_PATH*sizeof(WCHAR), wszName );
        err = JET_errSuccess;
    }
    Call( err );

HandleError:
    Assert( JET_errFileNotFound != err || fPathMustExist );
    return err;
}


ERR ErrUtilPathReadOnly(
    IFileSystemAPI* const   pfsapi,
    const WCHAR* const      wszPath,
    BOOL* const             pfReadOnly )
{
    ERR                     err     = JET_errSuccess;
    IFileFindAPI*           pffapi  = NULL;

    Call( pfsapi->ErrFileFind( wszPath, &pffapi ) );
    Call( pffapi->ErrNext() );
    Call( pffapi->ErrIsReadOnly( pfReadOnly ) );

    delete pffapi;
    return JET_errSuccess;

HandleError:
    delete pffapi;
    *pfReadOnly = fFalse;
    return err;
}

ERR ErrUtilDirectoryValidate(
    IFileSystemAPI* const   pfsapi,
    const WCHAR* const      wszPath,
    __out_bcount_opt(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const           wszAbsPath )
{
    ERR                     err = JET_errSuccess;
    WCHAR                   wszFolder[ IFileSystemAPI::cchPathMax ];
    WCHAR                   wszT[ IFileSystemAPI::cchPathMax ];

    Call( pfsapi->ErrPathParse( wszPath, wszFolder, wszT, wszT ) );


    Call( ErrUtilPathExists( pfsapi, wszFolder, wszAbsPath ) );

    return JET_errSuccess;

HandleError:
    if ( JET_errFileNotFound == err )
    {
        err = ErrERRCheck( JET_errInvalidPath );
    }
    if ( wszAbsPath )
    {
        wszAbsPath[0] = L'\0';
    }
    return err;
}


ULONG           cbLogExtendPatternMin   = 512;
ULONG           cbLogExtendPattern;
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
BYTE*           rgbLogExtendPattern;
COSMemoryMap    osmmOSUFile;
#endif


INLINE ERR ErrUtilIApplyLogExtendPattern(
    IFileSystemAPI* const pfsapi,
    IFileAPI *const pfapi,
    const QWORD qwSize,
    const QWORD ibFormatted,
    const OSFILEQOS grbitQOS )
{
    ERR     err     = JET_errSuccess;
    QWORD   ib;
    QWORD   cbWrite;

    DWORD   cbWriteMax  = cbLogExtendPattern;

    for ( ib = ibFormatted; ib < qwSize; ib += cbWrite )
    {
        cbWrite = min( cbWriteMax, qwSize - ib );
        Assert( DWORD( cbWrite ) == cbWrite );

        TraceContextScope tcLogFill( iorpLog );
        tcLogFill->iorReason.AddFlag( iorfFill );
        err = pfapi->ErrIOWrite( *tcLogFill, ib, DWORD( cbWrite ), g_rgbZero, grbitQOS );
        if ( JET_errOutOfMemory == err )
        {
            const DWORD cbWriteMaxOrig  = cbWriteMax;
            DWORD       cbWriteMin      = 0;

            Call( pfapi->ErrSectorSize( &cbWriteMin ) );

            cbWriteMin = max( cbLogExtendPatternMin, cbWriteMin );

            cbWrite = 0;

            cbWriteMax /= 2;

            cbWriteMax += cbWriteMin - 1;
            cbWriteMax -= cbWriteMax % cbWriteMin;


            if ( cbWriteMax >= cbWriteMaxOrig )
            {
                Call( ErrERRCheck( JET_errOutOfMemory ) );
            }
        }
        else
        {
            Call( err );
        }
    }

    return JET_errSuccess;

HandleError:
    return err;
}

ERR ErrUtilFormatLogFile(
    IFileSystemAPI* const   pfsapi,
    IFileAPI * const        pfapi,
    const QWORD             qwSize,
    const QWORD             ibFormatted,
    const OSFILEQOS         grbitQOS,
    const BOOL              fPermitTruncate,
    const TraceContext&     tc )
{
    ERR                 err;

    CallR( ErrUtilIApplyLogExtendPattern( pfsapi, pfapi, qwSize, ibFormatted, grbitQOS ) );

    if ( fPermitTruncate )
    {
        QWORD   qwRealSize;

        CallR( pfapi->ErrSize( &qwRealSize, IFileAPI::filesizeLogical ) );
        Assert( qwRealSize >= qwSize );
        if ( qwRealSize > qwSize )
        {
            TraceContextScope tcScope( iorpLog );
            CallR( pfapi->ErrSetSize( *tcScope, qwSize, fTrue, grbitQOS ) );
        }
    }

    return JET_errSuccess;
}

ERR ErrUtilWriteRBSHeaders( 
    const INST* const           pinst,
    IFileSystemAPI *const       pfsapi,
    const WCHAR                 *wszFileName,
    RBSFILEHDR                  *prbsfilehdr,
    IFileAPI *const             pfapi )
{
    Assert( prbsfilehdr->rbsfilehdr.le_filetype == JET_filetypeSnapshot );

    Assert( prbsfilehdr->rbsfilehdr.le_ulMajor != 0 );
    Assert( prbsfilehdr->rbsfilehdr.le_ulMinor != 0 );
    Assert( prbsfilehdr->rbsfilehdr.le_lGeneration > 0 );

    ERR err = JET_errSuccess;

    for( const BYTE * pbT = prbsfilehdr->rgbAttach; 0 != *pbT; pbT += sizeof( RBSATTACHINFO ) )
    {
        RBSATTACHINFO* prbsattachinfo = (RBSATTACHINFO*) pbT;
        Assert( prbsattachinfo->FPresent() >= 0 );

        if ( prbsattachinfo->FPresent() == 0 )
        {
            break;
        }

        Assert( prbsattachinfo->LGenMinRequired() >= 0 );
        Assert( prbsattachinfo->LGenMaxRequired() >= 0 );
        Assert( prbsattachinfo->LGenMaxRequired() >= prbsattachinfo->LGenMinRequired() );
        Assert( prbsattachinfo->DbtimeDirtied() >= 0 );
        Assert( prbsattachinfo->wszDatabaseName[0] != L'\0' );
    }

    Call( ErrUtilWriteShadowedHeaderInternal(   pinst,
                                                pfsapi, 
                                                iorpRBS,
                                                wszFileName,
                                                (BYTE*) prbsfilehdr,
                                                sizeof( RBSFILEHDR ),
                                                pfapi ) );

    if ( pfapi != NULL )
    {
        Call( ErrUtilFlushFileBuffers( pfapi, iofrRBS ) );
    }

HandleError:
    return err;
}

ERR ErrUtilWriteRBSRevertCheckpointHeaders(  
    const INST * const      pinst,
    IFileSystemAPI *const   pfsapi,
    const WCHAR             *wszFileName,
    RBSREVERTCHECKPOINT     *prbsrchkHeader,
    IFileAPI *const         pfapi )
{
    ERR err = JET_errSuccess;

    Assert( prbsrchkHeader->rbsrchkfilehdr.le_filetype == JET_filetypeRBSRevertCheckpoint );

    Call( ErrUtilWriteShadowedHeaderInternal(  
                pinst,
                pfsapi,
                iorpRBSRevertCheckpoint,
                wszFileName,
                (BYTE*)prbsrchkHeader,
                sizeof( *prbsrchkHeader ),
                pfapi ) );

    if ( pfapi != NULL )
    {
        Call( ErrUtilFlushFileBuffers( pfapi, iofrRBSRevertUtil ) );
    }

HandleError:
    return err;
}

#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
LOCAL ERR ErrOSULogPatternInit()
{
    ERR     err     = JET_errSuccess;
    ULONG   cbFill  = 0;

    rgbLogExtendPattern = NULL;

    if ( cbLogExtendPattern < OSMemoryPageCommitGranularity() )
    {

        rgbLogExtendPattern = (BYTE*)PvOSMemoryHeapAlloc( size_t( cbLogExtendPattern ) );

        if ( !rgbLogExtendPattern )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }

        cbFill = cbLogExtendPattern;
    }
    else if (   !COSMemoryMap::FCanMultiMap() ||
                cbLogExtendPattern < OSMemoryPageReserveGranularity() )
    {

        rgbLogExtendPattern = (BYTE*)PvOSMemoryPageAlloc( size_t( cbLogExtendPattern ), NULL );

        if ( !rgbLogExtendPattern  )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }

        cbFill = cbLogExtendPattern;
    }
    else
    {

        COSMemoryMap::ERR errOSMM;
        errOSMM = osmmOSUFile.ErrOSMMInit();
        if ( COSMemoryMap::ERR::errSuccess != errOSMM )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }

        errOSMM = osmmOSUFile.ErrOSMMPatternAlloc(  OSMemoryPageReserveGranularity(),
                                                    cbLogExtendPattern,
                                                    (void**)&rgbLogExtendPattern );
        if ( COSMemoryMap::ERR::errSuccess != errOSMM )
        {
            AssertSz(   COSMemoryMap::ERR::errOutOfBackingStore == errOSMM ||
                        COSMemoryMap::ERR::errOutOfAddressSpace == errOSMM ||
                        COSMemoryMap::ERR::errOutOfMemory == errOSMM,
                        "unexpected error while allocating memory pattern" );
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }
        Assert( rgbLogExtendPattern );

        cbFill = OSMemoryPageReserveGranularity();
    }

    ULONG ib;
    ULONG cb;

    cb = cbLogExtendPatternMin;

    Assert( bLOGUnusedFill == 0xDA );
    Assert( cbLogExtendPattern >= cb );
    Assert( 0 == ( cbLogExtendPattern % cb ) );

    memset( rgbLogExtendPattern, bLOGUnusedFill, cb );
    for ( ib = 0; ib < cb; ib += 16 )
    {
        rgbLogExtendPattern[ib] = BYTE( bLOGUnusedFill + ib );
    }

    for ( ib = cb; ib < cbFill; ib += cb )
    {
        memcpy( rgbLogExtendPattern + ib, rgbLogExtendPattern, cb );
    }


    if ( cbLogExtendPattern >= OSMemoryPageCommitGranularity() )
    {
        for ( ib = 0; ib < cbLogExtendPattern; ib += cbFill )
        {
            OSMemoryPageProtect( rgbLogExtendPattern + ib, cbFill );
        }
    }

    return JET_errSuccess;

HandleError:
    return err;
}

LOCAL VOID OSULogPatternTerm()
{
    if ( cbLogExtendPattern < OSMemoryPageCommitGranularity() )
    {

        OSMemoryHeapFree( rgbLogExtendPattern );
        rgbLogExtendPattern = NULL;
    }
    else if (   !COSMemoryMap::FCanMultiMap() ||
                cbLogExtendPattern < OSMemoryPageReserveGranularity() )
    {

        OSMemoryPageFree( rgbLogExtendPattern );
        rgbLogExtendPattern = NULL;
    }
    else
    {

        osmmOSUFile.OSMMPatternFree();
        rgbLogExtendPattern = NULL;


        osmmOSUFile.OSMMTerm();
    }
}
#endif

ERR ErrOSUFileInit()
{
    ERR     err     = JET_errSuccess;
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    Call( ErrOSULogPatternInit() );

    return JET_errSuccess;

HandleError:
    OSUFileTerm();
#endif
    return err;
}


void OSUFileTerm()
{
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    OSULogPatternTerm();
#endif

    if ( NULL != g_rgbDebugHeader )
    {
        OSMemoryPageFree( g_rgbDebugHeader );
        g_rgbDebugHeader = NULL;
    }

    if ( NULL != g_rgbDebugShadowHeader )
    {
        OSMemoryPageFree( g_rgbDebugShadowHeader );
        g_rgbDebugShadowHeader = NULL;
    }

    g_errDebugHeaderIO = JET_errSuccess;
    g_errDebugShadowHeaderIO = JET_errSuccess;
    g_tickDebugHeader = 0L;
}

