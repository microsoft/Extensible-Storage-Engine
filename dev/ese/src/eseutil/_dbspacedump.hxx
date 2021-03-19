// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  Space utilities / dumping.
//
//  The intended calling order is:
//
//      ErrSpaceDumpCtxInit( &pCtx )
//
//      ErrSpaceDumpCtxSetFields( pCtx, wszArgs )   NOTE: must be before call to SpaceDumpCtxGetGRBIT().
//
//      ErrSpaceDumpCtxSetOptions( pCtx, ... various options ... )
//
//      pdbutil->grbitOptions = SpaceDumpCtxGetGRBIT( pCtx );   // used for JetDBUtilities
//
//      JetDBUtilities( pdbutil )
//          --> EseutilEvalBTreeData( pBTreesStats, pCtx ) x N times
//
//      ErrSpaceDumpCtxComplete( pCtx, fTrue|fFalse ) // prints final stats.
//

void PrintSpaceDumpHelp(
    _In_ const ULONG                    cchLineWidth,
    _In_ const WCHAR                    wchNewLine,
    __in_z const WCHAR *                wszTab1,
    __in_z const WCHAR *                wszTab2 );


JET_ERR ErrSpaceDumpCtxInit(
    _Out_ void **                       ppvContext );

JET_ERR ErrSpaceDumpCtxSetFields(
    __inout void *                      pvContext,
    __in_z const WCHAR *                wszFields  );

enum SPDUMPOPTS
{
    fSPDumpNoOpts           = 0x0,
    fSPDumpPrintSmallTrees  = 0x1,
    fSPDumpPrintSpaceTrees  = 0x2,
    fSPDumpPrintSpaceNodes  = 0x4,      //  the OE/AE space nodes w/in the OE/AE space trees
    fSPDumpPrintSpaceLeaks  = 0x8,
    fSPDumpSelectOneTable   = 0x10
};

JET_ERR ErrSpaceDumpCtxSetOptions(
    __inout void *                      pvContext,
    __in_opt const ULONG *              pcbPageSize,        // trinary state, NULL means no change.
    __in_opt const JET_GRBIT *          pgrbitAdditional,   // trinary state, NULL means no change.
    __in_z_opt const PWSTR              wszSeparator,       // trinary state, NULL means no change.
    _In_ const SPDUMPOPTS               fSPDumpOpts         // "trinary" state, 0x0 means no change, can't unset.
    );

JET_ERR ErrSpaceDumpCtxGetGRBIT(
    __inout void *                      pvContext,
    _Out_ JET_GRBIT *                   pgrbit );

JET_ERR EseutilEvalBTreeData(
    _In_ const BTREE_STATS * const      pBTreeStats,
    _In_ JET_API_PTR                    pvContext );

JET_ERR ErrSpaceDumpCtxComplete(
    __inout void *                      pvContext,
    _In_ JET_ERR                        err );


