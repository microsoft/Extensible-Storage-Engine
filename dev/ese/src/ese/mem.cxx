// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"


const ULONG         MEMPOOL::cbChunkSize        = 256;
const USHORT        MEMPOOL::ctagChunkSize      = 4;
const USHORT        MEMPOOL::cMaxEntries        = 0xfff0;
const MEMPOOL::ITAG MEMPOOL::itagTagArray       = 0;
const MEMPOOL::ITAG MEMPOOL::itagFirstUsable    = 1;


INLINE BOOL MEMPOOL::FResizeBuf( ULONG cbNewBufSize )
{
    BYTE        *pbuf;

    Assert( IbBufFree() <= CbBufSize() );
    Assert( cbNewBufSize >= IbBufFree() );

    pbuf = (BYTE *)PvOSMemoryHeapAlloc( cbNewBufSize );
    if ( pbuf == NULL )
        return fFalse;

    UtilMemCpy( pbuf, Pbuf(), IbBufFree() );
    OSMemoryHeapFree( Pbuf() );
    
    SetPbuf( pbuf );
    SetCbBufSize( cbNewBufSize );

    return fTrue;
}


INLINE BOOL MEMPOOL::FGrowBuf( ULONG cbNeeded )
{
    return FResizeBuf( CbBufSize() + cbNeeded + cbChunkSize );
}


INLINE ERR MEMPOOL::ErrGrowEntry( ITAG itag, ULONG cbNew )
{
    MEMPOOLTAG  *rgbTags = (MEMPOOLTAG *)Pbuf();
    ITAG        itagCurrent;

    Assert( itag >= itagFirstUsable || itag == itagTagArray );


    const ULONG ibEntry = rgbTags[itag].ib;
    const ULONG cbOld   = rgbTags[itag].cb;


    Assert( cbNew > cbOld );


    ULONG cbOldAlign = cbOld + sizeof( QWORD ) - 1;
    cbOldAlign -= cbOldAlign % sizeof( QWORD );
    ULONG cbNewAlign = cbNew + sizeof( QWORD ) - 1;
    cbNewAlign -= cbNewAlign % sizeof( QWORD );


    const ULONG cbAdditional = cbNewAlign - cbOldAlign;

    
    if ( CbBufSize() - IbBufFree() < cbAdditional )
    {
        if ( !FGrowBuf( cbAdditional - ( CbBufSize() - IbBufFree() ) ) )
            return ErrERRCheck( JET_errOutOfMemory );

        rgbTags = (MEMPOOLTAG *)Pbuf();
    }

    Assert( ( ibEntry + cbOld ) <= IbBufFree() );
    Assert( ( ibEntry + cbNew ) <= CbBufSize() );
    memmove( Pbuf() + ibEntry + cbNewAlign, Pbuf() + ibEntry + cbOldAlign, IbBufFree() - ( ibEntry + cbOldAlign ) );

    SetIbBufFree( IbBufFree() + cbAdditional );
    Assert( IbBufFree() <= CbBufSize() );

    for ( itagCurrent = itagFirstUsable; itagCurrent < ItagUnused(); itagCurrent++ )
    {
        if ( rgbTags[itagCurrent].cb > 0  &&  rgbTags[itagCurrent].ib > ibEntry )
        {
            Assert( rgbTags[itagCurrent].ib >= ibEntry + cbOldAlign );
            rgbTags[itagCurrent].ib += cbAdditional;
            Assert( rgbTags[itagCurrent].ib + rgbTags[itagCurrent].cb <= IbBufFree() );
        }
    }
    Assert( itagCurrent == ItagUnused() );


    rgbTags[itag].cb = cbNew;

    return JET_errSuccess;
}


INLINE VOID MEMPOOL::ShrinkEntry( ITAG itag, ULONG cbNew )
{
    BYTE        *pbuf = Pbuf();
    MEMPOOLTAG  *rgbTags = (MEMPOOLTAG *)pbuf;
    ITAG        itagCurrent;

    Assert( itag >= itagFirstUsable );
    Assert( cbNew < rgbTags[itag].cb );


    const ULONG ibEntry = rgbTags[itag].ib;
    const ULONG cbOld   = rgbTags[itag].cb;


    Assert( cbNew < cbOld );


    ULONG cbOldAlign = cbOld + sizeof( QWORD ) - 1;
    cbOldAlign -= cbOldAlign % sizeof( QWORD );
    ULONG cbNewAlign = cbNew + sizeof( QWORD ) - 1;
    cbNewAlign -= cbNewAlign % sizeof( QWORD );


    const ULONG ibNewEnd = ibEntry + cbNewAlign;
    const ULONG cbDelete = cbOldAlign - cbNewAlign;

    
    Assert( ibNewEnd > 0 );
    Assert( ibNewEnd >= rgbTags[itagTagArray].ib + rgbTags[itagTagArray].cb );
    Assert( ( ibNewEnd + cbDelete ) <= IbBufFree() );

    memmove( pbuf + ibNewEnd, pbuf + ibNewEnd + cbDelete, IbBufFree() - ( ibNewEnd + cbDelete ) );

    SetIbBufFree( IbBufFree() - cbDelete );
    Assert( IbBufFree() > 0 );
    Assert( IbBufFree() >= rgbTags[itagTagArray].ib + rgbTags[itagTagArray].cb );

    for ( itagCurrent = itagFirstUsable; itagCurrent < ItagUnused(); itagCurrent++ )
    {
        Assert( rgbTags[itagCurrent].ib != ibNewEnd  ||
            ( itagCurrent == itag  &&  cbNew == 0 ) );

        if ( rgbTags[itagCurrent].cb > 0  &&  rgbTags[itagCurrent].ib > ibNewEnd )
        {
            Assert( rgbTags[itagCurrent].ib >= ibNewEnd + cbDelete );
            rgbTags[itagCurrent].ib -= cbDelete;
            Assert( rgbTags[itagCurrent].ib >= ibNewEnd );
            Assert( rgbTags[itagCurrent].ib + rgbTags[itagCurrent].cb <= IbBufFree() );
        }
    }
    Assert( itagCurrent == ItagUnused() );

    rgbTags[itag].cb = cbNew;
}


ERR MEMPOOL::ErrMEMPOOLInit(
    ULONG       cbInitialSize,
    USHORT      cInitialEntries,
    BOOL        fPadding )
{
    MEMPOOLTAG  *rgbTags;
    BYTE        *pbuf;

    if ( cInitialEntries >= cMaxEntries )
        return ErrERRCheck( JET_errTooManyMempoolEntries );
        
    cInitialEntries++;

    if ( fPadding )
    {
        Assert ( cMaxEntries > ctagChunkSize );
        if ( cInitialEntries > cMaxEntries - ctagChunkSize )
        {
            fPadding = fFalse;
        }
        else
        {
            cInitialEntries += ctagChunkSize;
        }
    }
    
    Assert( cInitialEntries >= 1 );
    
    if ( cInitialEntries > cMaxEntries )
        return ErrERRCheck( JET_errTooManyMempoolEntries );

    ULONG cbTagArray        = cInitialEntries * sizeof( MEMPOOLTAG );
    ULONG cbTagArrayAlign   = cbTagArray + sizeof( QWORD ) - 1;
    cbTagArrayAlign         -= cbTagArrayAlign % sizeof( QWORD );

    if ( fPadding )
    {
        cbInitialSize += cbChunkSize;
        if( cbInitialSize <= cbChunkSize )
        {
            Assert( cbInitialSize > cbChunkSize );
            return ErrERRCheck( JET_errOutOfMemory );
        }
    }
    
    cbInitialSize += cbTagArrayAlign;
    Assert( cbInitialSize >= sizeof(MEMPOOLTAG) );
    if( cbInitialSize <= cbTagArrayAlign )
    {
        Assert( cbInitialSize > cbTagArrayAlign );
        return ErrERRCheck( JET_errOutOfMemory );
    }

    pbuf = (BYTE *)PvOSMemoryHeapAlloc( cbInitialSize );
    if ( pbuf == NULL )
        return ErrERRCheck( JET_errOutOfMemory );

    rgbTags = (MEMPOOLTAG *)pbuf;
    rgbTags[itagTagArray].ib = 0;
    rgbTags[itagTagArray].cb = cbTagArray;

    Assert( cbTagArrayAlign <= cbInitialSize );

    SetCbBufSize( cbInitialSize );
    SetIbBufFree( cbTagArrayAlign );
    SetItagUnused( itagFirstUsable );
    SetItagFreed( itagFirstUsable );
    SetPbuf( pbuf );
    
    return JET_errSuccess;
}


ERR MEMPOOL::ErrAddEntry( BYTE *rgb, ULONG cb, ITAG *piTag )
{
    MEMPOOLTAG  *rgbTags;
    ULONG       cTotalTags;

    Assert( cb > 0 );
    Assert( piTag );

    AssertValid();
    rgbTags = (MEMPOOLTAG *)Pbuf();
    cTotalTags = rgbTags[itagTagArray].cb / sizeof(MEMPOOLTAG);

    if ( ItagFreed() < ItagUnused() )
    {
        *piTag = ItagFreed();
        Assert( rgbTags[ItagFreed()].cb == 0 );
        Assert( rgbTags[ItagFreed()].ib >= itagFirstUsable );

        SetItagFreed( (ITAG)rgbTags[ItagFreed()].ib );
        Assert( rgbTags[ItagFreed()].cb == 0 || ItagFreed() == ItagUnused() );
    }

    else
    {
        Assert( ItagFreed() == ItagUnused() );

        if ( ItagUnused() == cTotalTags )
        {
            ERR err;

            if ( cTotalTags + ctagChunkSize > cMaxEntries )
                return ErrERRCheck( JET_errOutOfMemory );

            err = ErrGrowEntry(
                itagTagArray,
                rgbTags[itagTagArray].cb + ( ctagChunkSize * sizeof(MEMPOOLTAG) ) );
            if ( err != JET_errSuccess )
            {
                Assert( err == JET_errOutOfMemory );
                return err;
            }

            cTotalTags += ctagChunkSize;
            
            rgbTags = (MEMPOOLTAG *)Pbuf();
            Assert( rgbTags[itagTagArray].cb == cTotalTags * sizeof(MEMPOOLTAG) );
        }

        *piTag = ItagUnused();
        SetItagFreed( ITAG( ItagFreed() + 1 ) );
        SetItagUnused( ITAG( ItagUnused() + 1 ) );
    }

    Assert( ItagFreed() <= ItagUnused() );
    Assert( ItagUnused() <= cTotalTags );


    rgbTags[*piTag].ib = IbBufFree();
    rgbTags[*piTag].cb = 0;


    ERR err = ErrGrowEntry( *piTag, cb );
    rgbTags = (MEMPOOLTAG *)Pbuf();


    if ( err < JET_errSuccess )
    {
        Assert( err == JET_errOutOfMemory );
        
        Assert( rgbTags[*piTag].cb == 0 );
        rgbTags[*piTag].ib = ItagFreed();
        SetItagFreed( *piTag );
        *piTag = 0;
    
        return ErrERRCheck( JET_errOutOfMemory );
    }

    
    if ( rgb )
    {
        UtilMemCpy( Pbuf() + rgbTags[*piTag].ib, rgb, cb );
    }
    else
    {
        memset( Pbuf() + rgbTags[*piTag].ib, 0, cb );
    }
    
    return JET_errSuccess;
}


VOID MEMPOOL::DeleteEntry( ITAG itag )
{
    MEMPOOLTAG  *rgbTags;

    AssertValid();

    rgbTags = (MEMPOOLTAG *)Pbuf();

    Assert( itag >= itagFirstUsable );
    Assert( itag < ItagUnused() );
    Assert( itag != ItagFreed() );

    Assert( rgbTags[itag].cb > 0 );
    ShrinkEntry( itag, 0 );

    Assert( rgbTags[itag].cb == 0 );
    rgbTags[itag].ib = ItagFreed();
    SetItagFreed( itag );
}


ERR MEMPOOL::ErrReplaceEntry( ITAG itag, BYTE *rgb, ULONG cb )
{
    ERR         err = JET_errSuccess;
    MEMPOOLTAG  *rgbTags;

    Assert( cb > 0 );

    AssertValid();
    Assert( itag >= itagFirstUsable );
    Assert( itag < ItagUnused() );

    rgbTags = (MEMPOOLTAG *)Pbuf();

    Assert( rgbTags[itag].cb > 0 );
    Assert( rgbTags[itag].ib + rgbTags[itag].cb <= IbBufFree() );

    if ( cb < rgbTags[itag].cb )
    {
        ShrinkEntry( itag, cb );
    }
    else if ( cb > rgbTags[itag].cb )
    {
        err = ErrGrowEntry( itag, cb );
        rgbTags = (MEMPOOLTAG *)Pbuf();
    }

    if ( JET_errSuccess == err && rgb != NULL )
    {
        UtilMemCpy( Pbuf() + rgbTags[itag].ib, rgb, cb );
    }

    return err;
}


BOOL MEMPOOL::FCompact()
{
    AssertValid();

    BOOL    fCompactNeeded = ( CbBufSize() - IbBufFree() > 32 );
    return ( fCompactNeeded ? FResizeBuf( IbBufFree() ) : fTrue );
}

