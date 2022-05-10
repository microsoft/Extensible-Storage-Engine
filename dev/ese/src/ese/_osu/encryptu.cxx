// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osustd.hxx"
#include "esestd.hxx"

#ifdef PERFMON_SUPPORT

PERFInstanceLiveTotalWithClass<> cbEncryptionBytes;
LONG LEncryptionBytesCEFLPv( LONG iInstance, void *pvBuf )
{
    cbEncryptionBytes.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cEncryptionCalls;
LONG LEncryptionCEFLPv( LONG iInstance, void *pvBuf )
{
    cEncryptionCalls.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<QWORD> cEncryptionTotalDhrts;
LONG LEncryptionLatencyCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*)pvBuf = CusecHRTFromDhrt( cEncryptionTotalDhrts.Get( iInstance ) );
    }
    return 0;
}

PERFInstanceLiveTotalWithClass<> cbDecryptionBytes;
LONG LDecryptionBytesCEFLPv( LONG iInstance, void *pvBuf )
{
    cbDecryptionBytes.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<> cDecryptionCalls;
LONG LDecryptionCEFLPv( LONG iInstance, void *pvBuf )
{
    cDecryptionCalls.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass<QWORD> cDecryptionTotalDhrts;
LONG LDecryptionLatencyCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf != NULL )
    {
        *(QWORD*)pvBuf = CusecHRTFromDhrt( cDecryptionTotalDhrts.Get( iInstance ) );
    }
    return 0;
}

#endif

ERR
ErrOSUEncrypt(
    _Inout_updates_bytes_to_(cbDataBufLen, *pcbDataLen)     BYTE *pbData,
    _Inout_                                                 ULONG *pcbDataLen,
    _In_                                                    ULONG cbDataBufLen,
    _In_                                            const   FUCB *pfucb,
    _In_                                            const   FUCB *pfucbTable )
{
    if ( pfucbTable == NULL )
    {
        pfucbTable = pfucb;
    }
    INT iInstance = PinstFromPfucb( pfucb )->m_iInstance;
    TCE tce = pfucb->u.pfcb->TCE();

    PERFOpt( cbEncryptionBytes.Add( iInstance, tce, *pcbDataLen ) );
    PERFOpt( cEncryptionCalls.Inc( iInstance, tce ) );

    const HRT hrtStart = HrtHRTCount();
    const ERR err = ErrOSEncryptWithAes256( pbData, pcbDataLen, cbDataBufLen, pfucbTable->pbEncryptionKey, pfucbTable->cbEncryptionKey );
    PERFOpt( cEncryptionTotalDhrts.Add( iInstance, tce, HrtHRTCount() - hrtStart ) );

    return err;
}

ERR
ErrOSUDecrypt(
    _In_reads_( *pcbDataLen )                           BYTE *pbDataIn,
    _Out_writes_bytes_to_(*pcbDataLen, *pcbDataLen)     BYTE *pbDataOut,
    _Inout_                                             ULONG *pcbDataLen,
    _In_                                        const   FUCB *pfucb,
    _In_                                        const   FUCB *pfucbTable )
{
    if ( pfucbTable == NULL )
    {
        pfucbTable = pfucb;
    }
    INT iInstance = PinstFromPfucb( pfucb )->m_iInstance;
    TCE tce = pfucb->u.pfcb->TCE();

    PERFOpt( cbDecryptionBytes.Add( iInstance, tce, *pcbDataLen ) );
    PERFOpt( cDecryptionCalls.Inc( iInstance, tce ) );

    const HRT hrtStart = HrtHRTCount();
    const ERR err = ErrOSDecryptWithAes256( pbDataIn, pbDataOut, pcbDataLen, pfucbTable->pbEncryptionKey, pfucbTable->cbEncryptionKey );
    PERFOpt( cDecryptionTotalDhrts.Add( iInstance, tce, HrtHRTCount() - hrtStart ) );

    if ( err == JET_errDecryptionFailed )
    {
        const WCHAR *rgsz[5];
        INT irgsz = 0;

        WCHAR wszpgno[16];
        WCHAR wsziline[16];
        WCHAR wszObjid[16];
        WCHAR wszpgnoFDP[16];

        OSStrCbFormatW( wszpgno, sizeof(wszpgno), L"%d", Pcsr( pfucb )->Pgno() );
        OSStrCbFormatW( wsziline, sizeof(wsziline), L"%d", Pcsr( pfucb )->ILine() );
        OSStrCbFormatW( wszObjid, sizeof(wszObjid), L"%u", ObjidFDP( pfucb ) );
        OSStrCbFormatW( wszpgnoFDP, sizeof(wszpgnoFDP), L"%d", PgnoFDP( pfucb ) );

        rgsz[irgsz++] = g_rgfmp[ pfucb->u.pfcb->Ifmp() ].WszDatabaseName();
        rgsz[irgsz++] = wsziline;
        rgsz[irgsz++] = wszpgno;
        rgsz[irgsz++] = wszObjid;
        rgsz[irgsz++] = wszpgnoFDP;

        UtilReportEvent(
            eventError,
            DATABASE_CORRUPTION_CATEGORY,
            DECRYPTION_FAILED_ID,
            irgsz,
            rgsz,
            0,
            NULL,
            PinstFromPfucb( pfucb ) );
    }

    return err;
}

