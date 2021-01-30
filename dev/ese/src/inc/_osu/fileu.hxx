// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OSU_FILE_HXX_INCLUDED
#define _OSU_FILE_HXX_INCLUDED

struct RBSFILEHDR;
struct RBSREVERTCHECKPOINT;

ERR ErrUtilFlushFileBuffers( _Inout_ IFileAPI * const pfapi, _In_ const IOFLUSHREASON iofr );

enum ShadowedHeaderStatus
{
    shadowedHeaderOK,
    shadowedHeaderPrimaryBad,
    shadowedHeaderSecondaryBad,
    shadowedHeaderCorrupt,
};

enum ShadowedHeaderRequest
{
    headerRequestGoodOnly,
    headerRequestPrimaryOnly,
    headerRequestSecondaryOnly,
};

typedef struct tagDbHeaderReader
{
    ShadowedHeaderRequest       shadowedHeaderRequest;
    const WCHAR*                wszFileName;
    BYTE*                       pbHeader;
    DWORD                       cbHeader;
    LONG                        ibPageSize;
    IFileSystemAPI*             pfsapi;
    IFileAPI*                   pfapi;
    BOOL                        fNoAutoDetectPageSize;
    DWORD                       cbHeaderActual;
    PAGECHECKSUM                checksumExpected;
    PAGECHECKSUM                checksumActual;
    ShadowedHeaderStatus        shadowedHeaderStatus;
} DB_HEADER_READER;

enum UtilReadHeaderFlags
{
    urhfNone                    = 0x00000000,
    urhfDefault                 = urhfNone,
    urhfReadOnly                = 0x00000001,
    urhfNoEventLogging          = 0x00000002,
    urhfNoFailOnPageMismatch    = 0x00000004,
    urhfNoAutoDetectPageSize    = 0x00000008,
};

DEFINE_ENUM_FLAG_OPERATORS_BASIC( UtilReadHeaderFlags );

ERR ErrUtilReadSpecificShadowedHeader( const INST* const pinst, DB_HEADER_READER* const pdbHdrReader );

ERR ErrUtilReadShadowedHeader(
    const INST* const               pinst,
    IFileSystemAPI* const           pfsapi,
    const WCHAR* const              wszFilePath,
    __out_bcount( cbHeader ) BYTE*  pbHeader,
    const DWORD                     cbHeader,
    const LONG                      ibPageSize,
    const UtilReadHeaderFlags       urhf                    = urhfDefault,
    DWORD* const                    pcbHeaderActual         = NULL,
    ShadowedHeaderStatus* const     pShadowedHeaderStatus   = NULL );

ERR ErrUtilReadShadowedHeader(
    const INST* const               pinst,
    IFileSystemAPI* const           pfsapi,
    IFileAPI* const                 pfapi,
    __out_bcount( cbHeader ) BYTE*  pbHeader,
    const DWORD                     cbHeader,
    const LONG                      ibPageSize,
    const UtilReadHeaderFlags       urhf                    = urhfDefault,
    DWORD* const                    pcbHeaderActual         = NULL,
    ShadowedHeaderStatus* const     pShadowedHeaderStatus   = NULL );

ERR ErrUtilFullPathOfFile( IFileSystemAPI* const pfsapi, __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const wszPathOnly, const WCHAR* const wszFile );

ERR ErrUtilCreatePathIfNotExist(
    IFileSystemAPI *const   pfsapi,
    const WCHAR *           wszPath,
    _Out_opt_bytecap_(cbAbsPath)  WCHAR *const          wszAbsPath,
    _In_range_(0, cbOSFSAPI_MAX_PATHW) ULONG    cbAbsPath );


ERR ErrUtilPathExists(  IFileSystemAPI* const   pfsapi,
                        const WCHAR* const      wszPath,
                        __out_bcount_opt(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const           wszAbsPath  = NULL );


ERR ErrUtilGetLogicalFileSize(  IFileSystemAPI* const   pfsapi,
                                const WCHAR* const      wszPath,
                                QWORD* const            pcbFileSize );

ERR ErrUtilPathComplete(
        IFileSystemAPI      * const pfsapi,
        const WCHAR         * const wszPath,
        __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR              * const wszAbsPath,
        const BOOL          fPathMustExist );


ERR ErrUtilPathReadOnly(    IFileSystemAPI* const   pfsapi,
                            const WCHAR* const      wszPath,
                            BOOL* const             pfReadOnly );


ERR ErrUtilDirectoryValidate(   IFileSystemAPI* const   pfsapi,
                                const WCHAR* const      wszPath,
                                __out_bcount_opt(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const           wszAbsPath  = NULL );


extern ULONG    cbLogExtendPattern;

#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
extern BYTE     *rgbLogExtendPattern;
const BYTE bLOGUnusedFill = 0xDA;
#endif



ERR ErrUtilFormatLogFile(
        IFileSystemAPI* const   pfsapi,
        IFileAPI * const        pfapi,
        const QWORD             qwSize,
        const QWORD             ibFormatted,
        const OSFILEQOS         grbitQOS,
        const BOOL              fPermitTruncate,
        const TraceContext&     tc );


ERR ErrUtilWriteRBSHeaders( 
        const INST* const           pinst,
        IFileSystemAPI *const       pfsapi,
        const WCHAR                 *wszFileName,
        RBSFILEHDR                  *prbsfilehdr,
        IFileAPI *const             pfapi );

ERR ErrUtilWriteRBSRevertCheckpointHeaders(
    const INST* const      pinst,
    IFileSystemAPI* const   pfsapi,
    const WCHAR* wszFileName,
    RBSREVERTCHECKPOINT* prbsrchkHeader,
    IFileAPI* const         pfapi );


ERR ErrOSUFileInit();


void OSUFileTerm();


#endif

