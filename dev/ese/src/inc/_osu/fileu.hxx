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
    ShadowedHeaderRequest       shadowedHeaderRequest;      // in
    const WCHAR*                wszFileName;                // in
    BYTE*                       pbHeader;                   // in
    DWORD                       cbHeader;                   // in
    LONG                        ibPageSize;                 // in
    IFileSystemAPI*             pfsapi;                     // in
    IFileAPI*                   pfapi;                      // in
    BOOL                        fNoAutoDetectPageSize;      // in
    DWORD                       cbHeaderActual;             // out
    PAGECHECKSUM                checksumExpected;           // out
    PAGECHECKSUM                checksumActual;             // out
    ShadowedHeaderStatus        shadowedHeaderStatus;       // out
} DB_HEADER_READER;

enum UtilReadHeaderFlags    // urhf
{
    urhfNone                    = 0x00000000,   // no flags
    urhfDefault                 = urhfNone,     // default
    urhfReadOnly                = 0x00000001,   // read-only operation, i.e., header will not get fixed up
    urhfNoEventLogging          = 0x00000002,   // no events are generated in case of errors or warnings
    urhfNoFailOnPageMismatch    = 0x00000004,   // do not fail if the expected and actual header sizes mismatch
    urhfNoAutoDetectPageSize    = 0x00000008,   // do not try to auto-detect the page size (use the size provided by the caller)
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

//  tests if the given path exists.  the full path of the given path is
//  returned in szAbsPath if that path exists and szAbsPath is not NULL

ERR ErrUtilPathExists(  IFileSystemAPI* const   pfsapi,
                        const WCHAR* const      wszPath,
                        // UNDONE_BANAPI:
                        __out_bcount_opt(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const           wszAbsPath  = NULL );

//  returns the logical size of the file

ERR ErrUtilGetLogicalFileSize(  IFileSystemAPI* const   pfsapi,
                                const WCHAR* const      wszPath,
                                QWORD* const            pcbFileSize );

ERR ErrUtilPathComplete(
        IFileSystemAPI      * const pfsapi,
        const WCHAR         * const wszPath,
        // UNDONE_BANAPI:
        __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR              * const wszAbsPath,
        const BOOL          fPathMustExist );

//  tests if the given path is read only

ERR ErrUtilPathReadOnly(    IFileSystemAPI* const   pfsapi,
                            const WCHAR* const      wszPath,
                            BOOL* const             pfReadOnly );

//  checks whether or not the specified directory exists
//  if not, JET_errInvalidPath will be returned.
//  if so, JET_errSuccess will be returned and szAbsPath
//  will contain the full path to the directory.

ERR ErrUtilDirectoryValidate(   IFileSystemAPI* const   pfsapi,
                                const WCHAR* const      wszPath,
                                // UNDONE_BANAPI:
                                __out_bcount_opt(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const           wszAbsPath  = NULL );

//  log extend pattern buffer/size

extern ULONG    cbLogExtendPattern;

#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
extern BYTE     *rgbLogExtendPattern;
const BYTE bLOGUnusedFill = 0xDA;   // Used to fill portions of the log that should never be used as data
#endif


//  apply the format from ibFormat up to qwSize

ERR ErrUtilFormatLogFile(
        IFileSystemAPI* const   pfsapi,
        IFileAPI * const        pfapi,
        const QWORD             qwSize,
        const QWORD             ibFormatted,
        const OSFILEQOS         grbitQOS,
        const BOOL              fPermitTruncate,
        const TraceContext&     tc );

// Write revert snapshot header to the given rbs file.

ERR ErrUtilWriteRBSHeaders( 
        const INST* const           pinst,
        IFileSystemAPI *const       pfsapi,
        const WCHAR                 *wszFileName,
        RBSFILEHDR                  *prbsfilehdr,
        IFileAPI *const             pfapi );

// Write RBS revert checkpoint header to the corresponding checkpoint file.
ERR ErrUtilWriteRBSRevertCheckpointHeaders(
    const INST* const      pinst,
    IFileSystemAPI* const   pfsapi,
    const WCHAR* wszFileName,
    RBSREVERTCHECKPOINT* prbsrchkHeader,
    IFileAPI* const         pfapi );

//  init file subsystem

ERR ErrOSUFileInit();

//  terminate file subsystem

void OSUFileTerm();


#endif  //  _OSU_FILE_HXX_INCLUDED

