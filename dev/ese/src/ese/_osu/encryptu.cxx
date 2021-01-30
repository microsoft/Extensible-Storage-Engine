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
    _In_reads_bytes_(cbKey)                         const   BYTE *pbKey,
    _In_                                                    ULONG cbKey,
    _In_                                            const   INT iInstance,
    _In_                                            const   TCE tce )
{
    PERFOpt( cbEncryptionBytes.Add( iInstance, tce, *pcbDataLen ) );
    PERFOpt( cEncryptionCalls.Inc( iInstance, tce ) );

    const HRT hrtStart = HrtHRTCount();
    const ERR err = ErrOSEncryptWithAes256( pbData, pcbDataLen, cbDataBufLen, pbKey, cbKey );
    PERFOpt( cEncryptionTotalDhrts.Add( iInstance, tce, HrtHRTCount() - hrtStart ) );

    return err;
}

ERR
ErrOSUDecrypt(
    _In_reads_( *pcbDataLen )                           BYTE *pbDataIn,
    _Out_writes_bytes_to_(*pcbDataLen, *pcbDataLen)     BYTE *pbDataOut,
    _Inout_                                             ULONG *pcbDataLen,
    _In_reads_bytes_(cbKey)                     const   BYTE *pbKey,
    _In_                                                ULONG cbKey,
    _In_                                        const   INT iInstance,
    _In_                                        const   TCE tce )
{
    PERFOpt( cbDecryptionBytes.Add( iInstance, tce, *pcbDataLen ) );
    PERFOpt( cDecryptionCalls.Inc( iInstance, tce ) );

    const HRT hrtStart = HrtHRTCount();
    const ERR err = ErrOSDecryptWithAes256( pbDataIn, pbDataOut, pcbDataLen, pbKey, cbKey );
    PERFOpt( cDecryptionTotalDhrts.Add( iInstance, tce, HrtHRTCount() - hrtStart ) );

    return err;
}

