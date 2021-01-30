// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


void PrintSpaceDumpHelp(
    __in const ULONG                    cchLineWidth,
    __in const WCHAR                    wchNewLine,
    __in_z const WCHAR *                wszTab1,
    __in_z const WCHAR *                wszTab2 );


JET_ERR ErrSpaceDumpCtxInit(
    __out void **                       ppvContext );

JET_ERR ErrSpaceDumpCtxSetFields(
    __inout void *                      pvContext,
    __in_z const WCHAR *                wszFields  );

enum SPDUMPOPTS
{
    fSPDumpNoOpts           = 0x0,
    fSPDumpPrintSmallTrees  = 0x1,
    fSPDumpPrintSpaceTrees  = 0x2,
    fSPDumpPrintSpaceNodes  = 0x4,
    fSPDumpPrintSpaceLeaks  = 0x8,
    fSPDumpSelectOneTable   = 0x10
};

JET_ERR ErrSpaceDumpCtxSetOptions(
    __inout void *                      pvContext,
    __in_opt const ULONG *              pcbPageSize,
    __in_opt const JET_GRBIT *          pgrbitAdditional,
    __in_z_opt const PWSTR              wszSeparator,
    __in const SPDUMPOPTS               fSPDumpOpts
    );

JET_ERR ErrSpaceDumpCtxGetGRBIT(
    __inout void *                      pvContext,
    __out JET_GRBIT *                   pgrbit );

JET_ERR EseutilEvalBTreeData(
    __in const BTREE_STATS * const      pBTreeStats,
    __in JET_API_PTR                    pvContext );

JET_ERR ErrSpaceDumpCtxComplete(
    __inout void *                      pvContext,
    __in JET_ERR                        err );


