// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  ================================================================
namespace PagePatching
//  ================================================================
{
    INLINE BOOL FIsPatchableError( const ERR err );
    void TryToRequestPatch( const IFMP ifmp, const PGNO pgno );
    void CancelPatchRequest( const IFMP ifmp, const PGNO pgno );
    bool FHasRequest( const IFMP ifmp, const PGNO pgno );
    void RequestPagePatchOnNewThread( IFMP ifmp, LONG pgno );
    void TryPatchFromCopy( const IFMP ifmp, const PGNO pgno, void *pv, SHORT *perr );

    ERR ErrDoPatch(
        __in const IFMP                     ifmp,
        __in const PGNO                     pgno,
        __inout BFLatch * const             pbfl,
        __in_bcount(cbToken) const void *   pvToken,
        __in ULONG                  cbToken,
        __in_bcount(cbData) const void *    pvData,
        __in ULONG                  cbData,
        __out BOOL *                        pfPatched);

    ERR ErrPRLInit(
        _In_ INST* pinst );

    void TermFmp( const IFMP ifmp );
    void TermInst( INST * const pinst );
}

