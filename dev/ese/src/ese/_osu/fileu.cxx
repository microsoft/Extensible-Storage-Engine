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

// Keep a copy of the header and shadow header
// in case we hit corruption on both, which should
// not happen.
//
// IMPORTANT: THESE ARE NOT SAFE. We are not
// locking or guarding them in any way. This is a
// best effort debugging effort.
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

//  Returns whether or not range (ibContainer,cbContainer) contains range (ibContainee,cbContainee).
//
LOCAL INLINE BOOL FRangeContains( const DWORD ibContainer, const DWORD cbContainer, const DWORD ibContainee, const DWORD cbContainee )
{
    //  non-overlapping or containee is larger
    //
    if ( ibContainee < ibContainer ||
            ( ibContainee - ibContainer ) >= cbContainer ||
            cbContainee > cbContainer )
    {
        return fFalse;
    }

    return ( ( ibContainer + cbContainer ) - ibContainee ) >= cbContainee;
}

//  Merges range (ibNew,cbNew) into (*pibCurrent,*pcbCurrent). If the ranges don't overlap in any way, the resulting
//  range will be (ibNew,cbNew). A non-overlap with a gap of zero is also considered an overlap.
//
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

//  a resilient form of reading the shadowed header that will take any file and
//  do one of the following things:
//  - if shadowedHeaderRequest is headerRequestGoodOnly:
//    o  fail to recognize it as a valid ESE file with JET_errReadVerifyFailure
//    o  find a valid pair of shadowed headers
//    o  find either a valid header or a valid shadow
//    o  if fNoAutoDetectPageSize is note set, this function will try to auto-detect the page size among
//       the valid page sizes supported by ESE
//    o  if fNoAutoDetectPageSize is set, the search will be narrowed down to only cbHeader
//  - if shadowedHeaderRequest is either headerRequestPrimaryOnly or headerRequestSecondaryOnly:
//    o  either the primary or shadow header will be returned, respectively
//    o  if fNoAutoDetectPageSize is not set, page-size auto-detection will be enabled and JET_errReadVerifyFailure
//       will still be returned if a valid header can't be found
//    o  if fNoAutoDetectPageSize is set, this function should always succeed unless there is an IO failure
//       or and out-of-memory condition
//
//  on a success, the first cbHeader bytes of the valid header are returned.
//  also the actual size of the header is returned. The actual size of the header
//  will be the auto-detected page size if fNoAutoDetectPageSize is not set, or the actual number of
//  bytes copied o the output buffer if we explicitly passed the page size.
//
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
    const ULONG             fNoAutoDetectPageSize       = pdbHdrReader->fNoAutoDetectPageSize;
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

    //  reset out params
    //
    memset( pdbHdrReader->pbHeader, 0, pdbHdrReader->cbHeader );
    shadowedHeaderStatus = shadowedHeaderCorrupt;
    cbHeaderActual = 0;

    //  get the file to be read
    //
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

    ULONG                   cbPageMin;
    Call( pfapiRead->ErrSectorSize( &cbPageMin ) );
    cbPageMin = max( cbPageMin, 2048 ); // we supported 2 KB page sizes through Win 8.1, and removed in Threshold / Win X.
    const ULONG             cbPageCandidateMin          = fNoAutoDetectPageSize ? cbHeader : cbPageMin;

    //  get the current size of the file to be read and its I/O size
    //
    Call( pfapiRead->ErrSize( &cbFileSize, IFileAPI::filesizeLogical ) );
    Call( pfapiRead->ErrIOSize( &cbIOSize ) );

    //  read up to two times the worst case size of the database header or the
    //  entire file, which ever is smaller, from the start of the file
    //
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

    //  try to deduce the correct page header size and state from the file data
    //
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

    //  if we didn't find a valid header then return that the header is corrupt
    //
    //  NOTE:  we used to return JET_errDiskIO here.  we must now return
    //  JET_errReadVerifyFailure to enable better automated error recovery
    //
    if ( !cbPagePrimary && !cbPageSecondary &&
            ( shadowedHeaderRequest == headerRequestGoodOnly || !fNoAutoDetectPageSize ) )
    {
        // We should not be hitting this unless something serious happened. We will then fire two IO reads
        // to get the header and shadow header for debugging purposes.

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

    //  if both headers are valid but don't match then return that the secondary
    //  is corrupt
    //
    if (    ( cbPagePrimary && cbPageSecondary ) &&
            (   cbPagePrimary != cbPageSecondary ||
                memcmp( pbRead + 0 * cbPagePrimary, pbRead + 1 * cbPagePrimary, cbPagePrimary ) != 0 ) )
    {
        cbHeaderElected = cbPagePrimary;
        cOffsetOfPbRead = 0;
        shadowedHeaderStatus = shadowedHeaderSecondaryBad;
        Error( JET_errSuccess );
    }

    //  if the primary was good and the secondary was bad, return that
    //
    if ( cbPagePrimary && !cbPageSecondary )
    {
        cbHeaderElected = cbPagePrimary;
        cOffsetOfPbRead = 0;
        shadowedHeaderStatus = shadowedHeaderSecondaryBad;
        Error( JET_errSuccess );
    }

    //  if the primary was bad and the secondary was good, return that
    //
    if ( !cbPagePrimary && cbPageSecondary )
    {
        cbHeaderElected = cbPageSecondary;
        cOffsetOfPbRead = 1;
        shadowedHeaderStatus = shadowedHeaderPrimaryBad;
        Error( JET_errSuccess );
    }

    //  if both headers were good then return the primary
    //
    if ( cbPagePrimary && cbPageSecondary )
    {
        cbHeaderElected = cbPagePrimary;
        cOffsetOfPbRead = 0;
        shadowedHeaderStatus = shadowedHeaderOK;
        Error( JET_errSuccess );
    }

    //  if we got here, it means we know what we are doing: we want a header no matter what and
    //  we know what the page size is.
    //
    if ( !cbPagePrimary && !cbPageSecondary )
    {
        Assert( fNoAutoDetectPageSize );
        cbHeaderElected = cbHeader;
        cOffsetOfPbRead = ( shadowedHeaderRequest == headerRequestPrimaryOnly ) ? 0 : 1;
        Error( JET_errSuccess );
    }

    //  we should have trapped all valid cases above
    //
    AssertSz( fFalse, "Unexpected case when validating a shadowed header!" );
    Error( ErrERRCheck( JET_errInternalError ) );

HandleError:
    Assert( !(  err >= JET_errSuccess &&
                shadowedHeaderStatus == shadowedHeaderCorrupt &&
                shadowedHeaderRequest != headerRequestPrimaryOnly &&
                shadowedHeaderRequest != headerRequestSecondaryOnly ) );

    // Should we copy it to the output buffer?
    if ( cbAlloc > 0 && err == JET_errSuccess )
    {
        Assert( cOffsetOfPbRead == 0 || cOffsetOfPbRead == 1 );

        // Did we request a specific header?
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

        //  we may have to re-read what we need, since our merge algorithm was simplified to avoid
        //  having to add bitmaps to map valid data down to the IO size granularity
        //
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
            
            // finally, copy the data
            //
            memcpy( pdbHdrReader->pbHeader, pbReadToCopy, cbReadToCopy );

            //  it's not valid to checksum a binary blob that doesn't even have a valid page size.
            //
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

            // If the user knew what the page size was, returned the actual number of bytes written.
            //
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

//  Internal function to read a database, log or checkpoint header.
//
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

    //  attempt to read a shadowed header from this file
    //
    DB_HEADER_READER dbHeaderReader =
    {
        headerRequestGoodOnly,  // shadowedHeaderRequest
        wszFileName,            // wszFileName
        pbHeader,               // pbHeader
        cbHeader,               // cbHeader
        ibPageSize,             // ibPageSize
        pfsapi,                 // pfsapi
        pfapi,                  // pfapi
        fNoAutoDetectPageSize,  // fNoAutoDetectPageSize

        0,                      // cbHeaderActual
        0,                      // checksumExpected
        0,                      // checksumActual
        shadowedHeaderCorrupt   // shadowedHeaderStatus
    };

    err = ErrUtilReadSpecificShadowedHeader( pinst, &dbHeaderReader );
    if ( !fNoEventLogging && ( err == JET_errReadVerifyFailure ) )
    {
        OSUHAEmitFailureTag( pinst, HaDbFailureTagCorruption, L"922f6ba1-c846-42d8-bd57-72844b4dd49e" );
    }
    Call( err );
    *pShadowedHeaderStatus = dbHeaderReader.shadowedHeaderStatus;
    cbHeaderActual = dbHeaderReader.cbHeaderActual;

    Assert( *pShadowedHeaderStatus != shadowedHeaderCorrupt ); //  otherwise, we would have returned an error above.

    //  log an event if the file has one valid header and one damaged header
    //
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

    //  if the actual header size doesn't match the specified size then fail
    //  with a page size mismatch
    //
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

//  Public function to read a database, log or checkpoint header.
//  This overload takes in a file path and opens an IFileAPI handle to it.
//
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

//  Public function to read a database, log or checkpoint header.
//  This overload takes in an IFileAPI object and assumes it is not NULL.
//
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
        Assert( shadowedHeaderStatus != shadowedHeaderCorrupt ); //  otherwise, we would have returned an error above.

        if ( ( shadowedHeaderStatus == shadowedHeaderPrimaryBad ) ||
                ( shadowedHeaderStatus == shadowedHeaderSecondaryBad ) )
        {
            //  try to patch the bad page
            //  and if do not succeed then put the warning in the event log
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

//  This validates that the header passed in checksums correctly.  Note it takes a cbDbHeader because
//  it can be used for the FMP signature, which is only as large as it needs to be.
void AssertDatabaseHeaderConsistent( const DBFILEHDR * const pdbfilehdr, const DWORD cbDbHeader, const DWORD cbPage )
{
    //  retail does nothing
#ifdef DEBUG
    const BYTE* pbDbHeaderT         = NULL;

    Assert( pdbfilehdr != NULL );
    Assert( cbDbHeader >= offsetof( DBFILEHDR, rgbReserved ) );
    Assert( cbDbHeader <= cbPage );

    //  this must be a database header
    
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

    //  if we have a full header, check padding and checksum

    if ( pbDbHeaderT != NULL )
    {
        //  padding must be all zeroed out
        //  note: check disables itself if a newer version of the engine has modified the DB.

        if ( CmpDbVer( pdbfilehdr->Dbv(), PfmtversEngineMax()->dbv ) <= 0 )
        {
            for ( DWORD ibReserved = 0; ibReserved < sizeof( pdbfilehdr->rgbReserved ); ibReserved++ )
            {
                Assert( pdbfilehdr->rgbReserved[ ibReserved ] == 0 );
                if ( pdbfilehdr->rgbReserved[ ibReserved ] != 0 )
                {
                    break; // no need to belabor the point ...
                }
            }
            
            for ( DWORD ibHeader = sizeof( DBFILEHDR ); ibHeader < cbPage; ibHeader++ )
            {
                Assert( pbDbHeaderT[ ibHeader ] == 0 );
                if ( pbDbHeaderT[ ibHeader ] == 0 )
                {
                    break; // no need to belabor the point ...
                }
            }
        }

        //  it must also checksum

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

        //  open the specified file

        err = pfsapi->ErrFileOpen(  wszFileName,
                                    BoolParam( JET_paramEnableFileCache ) ?
                                        IFileAPI::fmfCached :
                                        IFileAPI::fmfNone,
                                    &pfapiT );

        //  we could not open the file

        if ( JET_errFileNotFound == err )
        {
            //  create the specified file

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
    if ( ( cbSectorSize == 0 ) ||  // protect against div by zero
         ( ( cbHeader % cbSectorSize ) != 0 ) )
    {
        //  indicate that this is a configuration error
        //
        OSUHAEmitFailureTag( pinst, HaDbFailureTagConfiguration, L"fca51f8a-5caa-43b5-a50e-d56ed665dc3b" );

        Assert( cbSectorSize != 0 );
        Error( ErrERRCheck( JET_errSectorSizeNotSupported ) );
    }

    //  compute the checksum of the header to write

    SetPageChecksum( pbHeader, cbHeader, databaseHeader, 0 );

    //  protect the page from updates while it is being written

    OSMemoryPageProtect( pbHeader, cbHeader );
    fHeaderRO = fTrue;

    //  write two copies of the header synchronously.  if one is corrupted,
    //  the other will be valid containing either the old or new header data

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
        //  then pfapiT is opened / allocated here
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
            //  JetTestHook() uses this to jack the headers up to higher than understood versions for testing.
            ( FNegTest( fInvalidUsage ) && FNegTest( fCorruptingDbHeaders ) ) );

    Assert( pdbfilehdr->Dbstate() >= JET_dbstateJustCreated &&
            pdbfilehdr->Dbstate() <= JET_dbstateRevertInProgress );

    Assert( pdbfilehdr->le_lGenMinRequired <= pdbfilehdr->le_lGenMaxRequired );
    Assert( pdbfilehdr->le_lGenMaxRequired <= pdbfilehdr->le_lGenMaxCommitted );
    Assert( pdbfilehdr->le_lGenMinConsistent >= pdbfilehdr->le_lGenMinRequired ||
            // le_lGenMinConsistent is 0 on databases from before the persistent lost flush feature was introduced.
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
            // Uninitialized.
            ( CmpLgpos( pdbfilehdr->le_lgposLastResize, lgposMin ) == 0 ) ||

            // It must always be ahead of le_lgposAttach if initialized.
            ( ( CmpLgpos( pdbfilehdr->le_lgposLastResize, pdbfilehdr->le_lgposAttach ) >= 0 ) &&

              ( pdbfilehdr->Dbstate() != JET_dbstateCleanShutdown ) ||

                // If the database is clean, le_lgposLastResize would normally match le_lgposConsistent...
                ( ( CmpLgpos( pdbfilehdr->le_lgposLastResize, pdbfilehdr->le_lgposConsistent ) == 0 ) ||

                  // ...  except if this is a clean detach of a post-repaired database, which was repaired from a dirty state.
                  // In that case, le_lgposConsistent is not updated at detach (repair isn't logged), but le_lgposAttach is kept
                  // at its original value.
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

    // Copy off header to avoid locking it through the entire I/O operation.
    Alloc( pdbfilehdr = (DBFILEHDR*)PvOSMemoryPageAlloc( g_cbPage, NULL ) );
    UtilMemCpy( pdbfilehdr, pfmp->Pdbfilehdr().get(), sizeof( DBFILEHDR ) );

    // Save off signatures.
    UtilMemCpy( &signDbHdrFlushOld, &pdbfilehdr->signDbHdrFlush, sizeof( SIGNATURE ) );
    UtilMemCpy( &signFlushMapHdrFlushOld, &pdbfilehdr->signFlushMapHdrFlush, sizeof( SIGNATURE ) );
    UtilMemCpy( &signRBSHdrFlushOld, &pdbfilehdr->signRBSHdrFlush, sizeof( SIGNATURE ) );

    // Copy in new signatures.
    UtilMemCpy( &pdbfilehdr->signDbHdrFlush, &signDbHdrFlushNew, sizeof( SIGNATURE ) );
    UtilMemCpy( &pdbfilehdr->signFlushMapHdrFlush, &signFlushMapHdrFlushNew, sizeof( SIGNATURE ) );

    if ( fRBSFormatFeatureEnabled )
    {
        UtilMemCpy( &pdbfilehdr->signRBSHdrFlush, &signRBSHdrFlushNew, sizeof( SIGNATURE ) );
    }

    // Write header.
    err = ErrUtilWriteDatabaseHeadersInternal( pinst, pfsapi, wszFileName, pdbfilehdr, pfapi );

    // Commit checksum and new signatures if write succeeded.
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

    // Save off signatures.
    UtilMemCpy( &signDbHdrFlushOld, &pdbfilehdr->signDbHdrFlush, sizeof( SIGNATURE ) );
    UtilMemCpy( &signFlushMapHdrFlushOld, &pdbfilehdr->signFlushMapHdrFlush, sizeof( SIGNATURE ) );

    // Copy in new signatures.
    UtilMemCpy( &pdbfilehdr->signDbHdrFlush, &signDbHdrFlushNew, sizeof( SIGNATURE ) );
    UtilMemCpy( &pdbfilehdr->signFlushMapHdrFlush, &signFlushMapHdrFlushNew, sizeof( SIGNATURE ) );

    // TODO SOMEONE: Reset RBS signatures so that when we init session after incremental reseed, snapshot is invalidated till we implement support for it.
    // ErrUtilWriteUnattachedDatabaseHeaders is also called while applying the revert snapshot in which case we don't want to reset the RBS header flush since we will
    // update the RBS header flush once we revert back using the snapshot signature.
    if ( fResetRBSHdrFlush )
    {
        SIGResetSignature( &pdbfilehdr->signRBSHdrFlush );
    }

    // Write header.
    const ERR err = ErrUtilWriteDatabaseHeadersInternal( pinst, pfsapi, wszFileName, pdbfilehdr, pfapi );

    // Revert signatures if write failed.
    if ( err < JET_errSuccess )
    {
        UtilMemCpy( &pdbfilehdr->signDbHdrFlush, &signDbHdrFlushOld, sizeof( SIGNATURE ) );
        UtilMemCpy( &pdbfilehdr->signFlushMapHdrFlush, &signFlushMapHdrFlushOld, sizeof( SIGNATURE ) );
        
        // TODO SOMEONE: Should we revert RBS signature here? Going to let it be reset in memory as we did perform incremental reseed.
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

    CallS( err ); // no expected warnings (or errors)
    return err;
}

ERR ErrUtilFullPathOfFile(
    IFileSystemAPI* const   pfsapi,
    // UNDONE_BANAPI:
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
//  returns a pointer to the previous character in the string.  when the first
//  character is reached, the given ptr is returned.
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

    //  copy the path, remove any trailing filename, and point to the ending path-delimiter
    //
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
            //  we cannot move backwards anymore
            //
            Assert( pwszMove == wszPathT );

            //  there were no path delimiters which means we were given only a filename.
            //  Resolve the path before returning.
            //
            Call( pfsapi->ErrPathComplete( wszPath, wszAbsPath ) );

            err = JET_errSuccess;
            goto HandleError;
        }
    }

    Assert( *pwszEnd == wchPathDelimiter );
    pwszEnd[1] = L'\0';

    //  loop until we find a directory that exists
    //
    pwsz = pwszEnd;
    do
    {
        //  try to validate the current path
        //
        Assert( *pwsz == wchPathDelimiter );
        ch = pwsz[1];
        pwsz[1] = L'\0';

        err = ErrUtilDirectoryValidate( pfsapi, wszPathT );
        pwsz[1] = ch;
        if ( err == JET_errInvalidPath )
        {
            //  path does not exist, so we will chop off a subdirectory
            //  and try to validate the parent.
            //
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
                    //  we cannot move backwards anymore
                    //
                    Assert( pwszMove == wszPathT );

                    //  none of the directories in the path exist
                    //  we need to start creating at this point
                    //  from the outer-most directory.
                    //
                    goto BeginCreation;
                }
            }

            //  move the real path ptr
            //
            pwsz = pwszT;
        }
        else
        {
            //  we found an existing directory
            //
            CallSx( err, JET_errFileAccessDenied );
            Call( err );
        }
    }
    while ( err == JET_errInvalidPath );

    //  loop until all directories are created
    //
    while ( pwsz < pwszEnd )
    {
        //  move forward to the next directory
        //
        Assert( *pwsz == wchPathDelimiter );
        pwsz++;
        while ( *pwsz != wchPathDelimiter )
        {
#ifdef DEBUG
            pwszMove = OSSTRCharNext( pwsz );

            //  if this assert fires, it means we scanned to the end
            //  of the path string and did not find a path
            //  delimiter; this should never happen because
            //  we append a path delimiter at the start of
            //  this function.
            //
            Assert( pwszMove <= pwszEnd );

            //  if this assert fires, the one before it should have
            //  fired as well; this means that we can no longer
            //  move to the next character because the string
            //  is completely exhausted.
            //
            Assert( pwszMove > pwsz );

            //  move next
            //
            pwsz = pwszMove;
#else   //  !DEBUG
            pwsz = OSSTRCharNext( pwsz );
#endif  //  DEBUG
        }

BeginCreation:
        Assert( pwsz <= pwszEnd );
        Assert( *pwsz == wchPathDelimiter );

        //  make sure the name of the directory we
        //  need to create is not already in use by a file
        //
        ch = pwsz[0];
        pwsz[0] = L'\0';
        err = ErrUtilPathExists( pfsapi, wszPathT );
        if ( err >= JET_errSuccess )
        {
            //  determine if the path that we found is a file or directory
            //
            BOOL fIsDirectory = fFalse;
            Call( pfsapi->ErrPathExists( wszPathT, &fIsDirectory ) );

            //  if this is a file then we don't allow that
            //
            if ( !fIsDirectory )
            {
                Error( ErrERRCheck( JET_errInvalidPath ) );
            }

            //  if this is a directory then we don't need to create it
            //
            pwsz[0] = ch;
            continue;
        }
        Call( err == JET_errFileNotFound ? JET_errSuccess : err );
        pwsz[0] = ch;

        //  create the directory, if needed.  don't fail if it already exists due to a race
        //
        ch = pwsz[1];
        pwsz[1] = L'\0';
        err = pfsapi->ErrFolderCreate( wszPathT );
        Call( err == JET_errFileAlreadyExists ? JET_errSuccess : err );
        pwsz[1] = ch;
    }

    //  verify the new path and prepare the absolute path
    //
    CallS( ErrUtilDirectoryValidate( pfsapi, wszPathT, wszAbsPath ) );
    if ( fFileName && wszAbsPath )
    {
        CallS( pfsapi->ErrPathFolderNorm( wszAbsPath, cbAbsPath ) );

        //  copy the filename over to the absolute path as well
        //
#ifdef DEBUG
        Assert( *pwszEnd == wchPathDelimiter );
        Assert( pwszEnd[1] == L'\0' );
        pwszT = const_cast< WCHAR * >( wszPath ) + ( pwszEnd - wszPathT );
        Assert( *pwszT == wchPathDelimiter );
        Assert( pwszT[1] != L'\0' );
#endif  //  DEBUG
        OSStrCbAppendW( wszAbsPath, cbAbsPath, wszPath + ( pwszEnd - wszPathT ) + 1 );
    }

HandleError:
    
    return err;
}

//  tests if the given path exists.  Full path of the given path is
//  returned in szAbsPath if that path exists and szAbsPath is not NULL.
//
//  This will fail if the process does not have permissions to the parent
//  directory (for example in packaged processes).
//
//  This is the legacy implementation, resurrected after we discovered that
//  the newer way (using GetFileAttributesEx) returned slightly different
//  error codes.
//
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


//  tests if the given path exists.  Full path of the given path is
//  returned in szAbsPath if that path exists and szAbsPath is not NULL.
//
ERR ErrUtilPathExists(
    IFileSystemAPI* const   pfsapi,
    const WCHAR* const      wszPath,
    // UNDONE_BANAPI:
    __out_bcount_opt(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const           wszAbsPath )
{
    ERR                     err     = JET_errSuccess;
    BOOL                    fIsDirectory = fFalse;
    WCHAR                   wszAbsPathLocal[ OSFSAPI_MAX_PATH ];
    WCHAR* const&           wszAbsPathWritable = wszAbsPath ? wszAbsPath : wszAbsPathLocal;

    wszAbsPathWritable[0] = L'\0';

    //  First check to see if the path exists by using FindFirst/FindNext. For legacy/backwards
    //  compat reasons we need to keep the identical error codes and can't change the implementation.
    //
    C_ASSERT( sizeof( wszAbsPathLocal ) == cbOSFSAPI_MAX_PATHW );
    err = ErrUtilPathExistsByFindFirst( pfsapi, wszPath, wszAbsPathWritable );

    if ( JET_errAccessDenied == err || JET_errFileAccessDenied == err )
    {
        //  However there are times when the legacy method fails. Most notably when the process
        //  doesn't have permissions to the parent directory (which happens in Packed Processes).
        //  In this case, we'll fail back to ErrPathExists(), which currently does a GetFileAttributes()
        //  call.
        //
        Call( pfsapi->ErrPathComplete( wszPath, wszAbsPathWritable ) );
        Call( pfsapi->ErrPathExists( wszAbsPathWritable, &fIsDirectory ) );
    }

HandleError:
    return err;
}


//  returns the logical size of the file

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

    //  at this point we have a "well-formed" path
    //

    //  do a FileFindExact to try an convert from an 8.3 equivalent to the full path
    //  but if the file doesn't exist then continue on for the temp database error-handling.
    //
    err = ErrUtilPathExists( pfsapi, wszName, wszAbsPath );
    if( JET_errFileNotFound == err && !fPathMustExist )
    {
        //  the file isn't there. we'll deal with this later
        //
        // UNDONE_BANAPI:
        OSStrCbCopyW( wszAbsPath, OSFSAPI_MAX_PATH*sizeof(WCHAR), wszName );
        err = JET_errSuccess;
    }
    Call( err );

HandleError:
    Assert( JET_errFileNotFound != err || fPathMustExist );
    return err;
}


//  tests if the given path is read only
//
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

//  checks whether or not the specified directory exists
//  if not, JET_errInvalidPath will be returned.
//  if so, JET_errSuccess will be returned and szAbsPath
//  will contain the full path to the directory.
//
ERR ErrUtilDirectoryValidate(
    IFileSystemAPI* const   pfsapi,
    const WCHAR* const      wszPath,
    __out_bcount_opt(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const           wszAbsPath )
{
    ERR                     err = JET_errSuccess;
    WCHAR                   wszFolder[ IFileSystemAPI::cchPathMax ];
    WCHAR                   wszT[ IFileSystemAPI::cchPathMax ];

    //  extract the folder from the path
    //
    Call( pfsapi->ErrPathParse( wszPath, wszFolder, wszT, wszT ) );

    //  see if the path exists
    //

    Call( ErrUtilPathExists( pfsapi, wszFolder, wszAbsPath ) );

    return JET_errSuccess;

HandleError:
    if ( JET_errFileNotFound == err )
    {
        err = ErrERRCheck( JET_errInvalidPath );
    }
    if ( wszAbsPath )
    {
        // UNDONE_BANAPI:
        wszAbsPath[0] = L'\0';
    }
    return err;
}


//  log file extension pattern buffer
//
ULONG           cbLogExtendPatternMin   = 512; //   this must be EXACTLY 512 bytes (not 1 sector)
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

            // even if the disk (or RFS) is telling us less then 512
            // we have this requirement for the pattern size,
            // otherwise we might end up with multiple parts
            // being only the partial start of the pattern
            //
            cbWriteMin = max( cbLogExtendPatternMin, cbWriteMin );

            cbWrite = 0;                            // no data written this iteration

            cbWriteMax /= 2;                        // exponentially backoff

            cbWriteMax += cbWriteMin - 1;           // round up
            cbWriteMax -= cbWriteMax % cbWriteMin;

            //  exponentially backed off to the minimum

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

//  re-format an existing log file
//
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

    //  apply the pattern to the log file
    //  (the file will expand if necessary)
    //
    CallR( ErrUtilIApplyLogExtendPattern( pfsapi, pfapi, qwSize, ibFormatted, grbitQOS ) );

    if ( fPermitTruncate )
    {
        QWORD   qwRealSize;

        //  we may need to truncate the rest of the file
        //
        CallR( pfapi->ErrSize( &qwRealSize, IFileAPI::filesizeLogical ) );
        Assert( qwRealSize >= qwSize );
        if ( qwRealSize > qwSize )
        {
            //  do the truncation
            //
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

    //  reset all pointers
    //
    rgbLogExtendPattern = NULL;

    //  init log file extension buffer
    //
    if ( cbLogExtendPattern < OSMemoryPageCommitGranularity() )
    {
        //  allocate the log file extension buffer from the heap.  our log files
        //  had better be cached or this won't work!

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
        //  allocate the log file extension buffer

        rgbLogExtendPattern = (BYTE*)PvOSMemoryPageAlloc( size_t( cbLogExtendPattern ), NULL );

        if ( !rgbLogExtendPattern  )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }

        cbFill = cbLogExtendPattern;
    }
    else
    {
        //  allocate log file extension buffer by allocating the smallest chunk of page
        //  store possible and remapping it consecutively in memory until we hit the
        //  desired chunk size
        //

        //  init the memory map
        //
        COSMemoryMap::ERR errOSMM;
        errOSMM = osmmOSUFile.ErrOSMMInit();
        if ( COSMemoryMap::ERR::errSuccess != errOSMM )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }

        //  allocate the pattern
        //
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

    cb = cbLogExtendPatternMin; //  this must be EXACTLY 512 bytes (not 1 sector)

    Assert( bLOGUnusedFill == 0xDA );   //  don't let this change
    Assert( cbLogExtendPattern >= cb );
    Assert( 0 == ( cbLogExtendPattern % cb ) );

    //  make the unique 512 byte logfile pattern
    //
    memset( rgbLogExtendPattern, bLOGUnusedFill, cb );
    for ( ib = 0; ib < cb; ib += 16 )
    {
        rgbLogExtendPattern[ib] = BYTE( bLOGUnusedFill + ib );
    }

    //  copy it until we fill the whole buffer
    //
    for ( ib = cb; ib < cbFill; ib += cb )
    {
        memcpy( rgbLogExtendPattern + ib, rgbLogExtendPattern, cb );
    }

    //  protect the log file extension buffer from modification

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
        //  free log file extension buffer

        OSMemoryHeapFree( rgbLogExtendPattern );
        rgbLogExtendPattern = NULL;
    }
    else if (   !COSMemoryMap::FCanMultiMap() ||
                cbLogExtendPattern < OSMemoryPageReserveGranularity() )
    {
        //  free log file extension buffer

        OSMemoryPageFree( rgbLogExtendPattern );
        rgbLogExtendPattern = NULL;
    }
    else
    {
        //  free log file extension buffer

        osmmOSUFile.OSMMPatternFree();
        rgbLogExtendPattern = NULL;

        //  term the memory map

        osmmOSUFile.OSMMTerm();
    }
}
#endif // ENABLE_LOG_V7_RECOVERY_COMPAT

//  init file subsystem
//
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


//  terminate file subsystem
//
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

