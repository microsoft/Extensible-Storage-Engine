// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

enum IOREASONPRIMARY : BYTE
{
    // iorpNone publically defined
    iorpInvalid = 0,
    iorpOSUnitTest
};

//  This creates a file that is 256 commit granularity (4 KB) pages long with the 0th page having all 0s, and
//  the 1st(really 2nd) page having all 1s, and the 2nd(really 3rd) page having all 2s, etc. 
//  If fBigFile is specified, the file will be 2 x as big as physical memory

ERR ErrCreateIncrementFile( PCWSTR wszFile, const BOOL fBigFile )
{
    JET_ERR err;

    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapi = NULL;

    wprintf( L"\tErrCreateIncrementFile() Start:\n" );

    QWORD cbPage = fBigFile ? ( 256 * 1024 ) : OSMemoryPageCommitGranularity();
    QWORD cpage = CbIncrementalFileSize( fBigFile ) / cbPage;
    BYTE* pvPattern = new BYTE[ size_t( cbPage ) ];

    Call( ErrOSInit() );
    Call( ErrOSFSCreate( NULL/* pinstNil */, &pfsapi ) );

    if ( !fBigFile )
    {
        //  Wish there was a better way to ensure sane format ...
        wprintf( L"Resetting/deleting pre-existing file () --> %d", pfsapi->ErrFileDelete( wszFile ) );
    }

    wprintf( L"\t\tErrFileCreate() --> " );
    err = pfsapi->ErrFileCreate( wszFile, 
                                IFileAPI::fmfLossyWriteBack,
                                &pfapi );
    wprintf( L"%d\n", err );
#ifdef DEBUG    //  Useful for prototyping to not have to re-create the file ...
    if ( fBigFile && JET_errDiskIO == err )
    {
        wprintf( L"\t\t\tTreating JET_errDiskIO as existing file, succeeding.\n" );
        err = JET_errSuccess;
        goto HandleError;
    }
#endif
    Call( err );

    wprintf( L"\t\tErrSetSize( %I64d /* = %I64d pages */ ) --> ", cbPage * cpage, cpage );
    err = pfapi->ErrSetSize( *TraceContextScope( iorpOSUnitTest ), cbPage * cpage, fFalse, qosIONormal );
    wprintf( L"%d\n", err );
    Call( err );

    wprintf( L"\t\tBegin Writing ... %I64d bytes", cbPage * cpage );
    const TICK tickBegin = TickOSTimeCurrent();
    for( QWORD ipage = 0; ipage < cpage; ipage++ )
    {
        memset( pvPattern, (BYTE)ipage, (INT)cbPage );
        Call( pfapi->ErrIOWrite( *TraceContextScope( iorpOSUnitTest ), ipage * cbPage, (DWORD)cbPage, pvPattern, qosIONormal ) );
    }
    wprintf( L"...Done (%.3f msec)\n", double( TickOSTimeCurrent() - tickBegin ) / 1000.0 );

HandleError:

    wprintf( L"\tErrCreateIncrementFile() END --> %d\n", err );

    delete pfapi;
    delete pfsapi;
    delete[] pvPattern;
    OSTerm();

    return err;
}


#define ExpMap( e )     { e, #e }
typedef struct ExplicitMap_
{
    QWORD               eValue;
    char *              szValue;
} ExplicitMap;

ExplicitMap empstatesz[] =
{
    ExpMap( MEM_COMMIT ),
    ExpMap( MEM_FREE ),
    ExpMap( MEM_RESERVE ),
};

ExplicitMap emptypesz[] = 
{
    ExpMap( MEM_IMAGE ),
    ExpMap( MEM_MAPPED ),
    ExpMap( MEM_PRIVATE ),
};

ExplicitMap empprotectsz[] =
{
    ExpMap( PAGE_EXECUTE ),
    ExpMap( PAGE_EXECUTE_READ ),
    ExpMap( PAGE_EXECUTE_READWRITE ),
    ExpMap( PAGE_EXECUTE_WRITECOPY ),
    ExpMap( PAGE_NOACCESS ),
    ExpMap( PAGE_READONLY ),
    ExpMap( PAGE_READWRITE ),
    ExpMap( PAGE_WRITECOPY ),
    ExpMap( PAGE_GUARD ),
    ExpMap( PAGE_NOCACHE ),
    ExpMap( PAGE_WRITECOMBINE ),
};

//  Print a bitfield in a nice format like (note this is a made up case, OS MM wouldn't return this):
//          0x1000A = ( 0x10000 | PAGE_READONLY | PAGE_WRITECOPY )
//      where 
//          PAGE_READONLY = 0x2 and so is mapped from that part of the value.
//          PAGE_WRITECOPY = 0xA and so is mapped from that part of the value.
//          0x10000 = some unknown bit I made up for pretend.

#define PrintBitField( emp, eValue )    PrintBitField_( emp, _countof(emp), eValue )
void PrintBitField_( const ExplicitMap * const empenumsz, const ULONG cenumsz, const QWORD eValue )
{
    BOOL fFirstField = fTrue;
    wprintf( L"0x%I64x", eValue );
    if ( eValue != 0 )
    {
        wprintf( L" = ( " );
        QWORD eLeftOver = eValue;
        QWORD eToBeMapped = 0;
        QWORD eCheck = 0;
        for( ULONG ienum = 0; ienum < cenumsz; ienum++ )
        {
            // All bits in map must be exclusive, or probably this won't work right at all...
            Assert( 0 == ( eCheck & empenumsz[ienum].eValue ) );
            eCheck |= empenumsz[ienum].eValue;

            eToBeMapped |= ( eValue & empenumsz[ienum].eValue );    //  add this value to the mappable values
            eLeftOver &= ~empenumsz[ienum].eValue;                  //  strip known value from left overs
        }
        Assert( eValue == ( eLeftOver | eToBeMapped ) );
        if ( eLeftOver )
        {
            wprintf( L"0x%I64x ", eLeftOver );
            fFirstField = fFalse;
        }
        if ( eToBeMapped )
        {
            for( ULONG ienum = 0; ienum < cenumsz; ienum++ )
            {
                if ( eToBeMapped & empenumsz[ienum].eValue )
                {
                    if ( !fFirstField )
                    {
                        wprintf( L"| " );
                    }
                    wprintf( L"%hs ", empenumsz[ienum].szValue );
                    eToBeMapped &= ~empenumsz[ienum].eValue;
                }
            }
        }
        wprintf( L")" );
    }
}

#include <psapi.h>
#define wszPsapi                L"psapi.dll"
#define wszWorkingSet           L"api-ms-win-core-psapi-l1-1-0.dll"
const wchar_t * const g_mwszzWorkingSetLibs     = wszWorkingSet L"\0" /* downlevel */ wszPsapi L"\0";
NTOSFuncStd( g_pfnQueryWorkingSetEx2, g_mwszzWorkingSetLibs, QueryWorkingSetEx, oslfExpectedOnWin6 );

void PrintMemInfo_( const ULONG ibPrefix, const BYTE * const pbMemory, PCSTR szMemVarName )
{
    if ( (ULONG_PTR)pbMemory == (ULONG_PTR)pbMemInfoHeaders )
    {
        MEMORY_BASIC_INFORMATION membasic = { 0 };
        const size_t cbResult = VirtualQueryEx( GetCurrentProcess(), (LPCVOID)pbMemory, &membasic, sizeof(membasic) );
        wprintf( L"%10hs: fRes=    [offset]  pbMemory        /  pbBaseAdd       .  cbMem    prot=0x%04x / 0x%x    state=0x%x   Type=0x%x\n", 
                    szMemVarName,
                    membasic.AllocationProtect, membasic.Protect, membasic.State, membasic.Type );
    }
    else
    {
        MEMORY_BASIC_INFORMATION membasic = { 0 };
        const size_t cbResult = VirtualQueryEx( GetCurrentProcess(), (LPCVOID)pbMemory, &membasic, sizeof(membasic) );
        Assert( membasic.BaseAddress == pbMemory );

//      wprintf( L"fRes=%d  [%4dKB] %p / %p . %6I64d    prot=0x%04x / 0x%x    state=0x%x    Type=0x%x\n", 
//                  (DWORD)cbResult, ibPrefix/1024, membasic.BaseAddress, membasic.AllocationBase, membasic.RegionSize,
//                  membasic.AllocationProtect, membasic.Protect, membasic.State, membasic.Type );

        wprintf( L"%10hs: fRes=%d  [%4dKB] %p / %p . %6I64d    prot=", 
                    szMemVarName, (DWORD)cbResult, ibPrefix/1024, membasic.BaseAddress, membasic.AllocationBase, membasic.RegionSize );
        PrintBitField( empprotectsz, membasic.AllocationProtect );
        wprintf( L" / " );
        PrintBitField( empprotectsz, membasic.Protect );
        wprintf( L"  state=" );
        PrintBitField( empstatesz, membasic.State );
        wprintf( L"  type=" );
        PrintBitField( emptypesz, membasic.Type );

        wprintf( L"  QWS:" );
        PSAPI_WORKING_SET_EX_INFORMATION    mwsexinfo;
        mwsexinfo.VirtualAddress = (PVOID)pbMemory;
        mwsexinfo.VirtualAttributes.Flags = 0;
        if( g_pfnQueryWorkingSetEx2( GetCurrentProcess(), (void*)&mwsexinfo, sizeof(mwsexinfo) ) )
        {
            if ( mwsexinfo.VirtualAttributes.Valid )
            {
                Assert( mwsexinfo.VirtualAttributes.Win32Protection == membasic.Protect );
                wprintf( L" cShares=%d cNode=%d", mwsexinfo.VirtualAttributes.ShareCount, mwsexinfo.VirtualAttributes.Node );
                if ( mwsexinfo.VirtualAttributes.Locked )
                {
                    wprintf( L" fLocked" );
                }
                if ( mwsexinfo.VirtualAttributes.LargePage )
                {
                    wprintf( L" fLargePage" );
                }
                if ( mwsexinfo.VirtualAttributes.Shared )
                {
                    wprintf( L" fShared" );
                }
                if ( mwsexinfo.VirtualAttributes.Bad )
                {
                    wprintf( L" fBad" );
                }
                if ( mwsexinfo.VirtualAttributes.Reserved )
                {
                    wprintf( L" Res=0x%x", mwsexinfo.VirtualAttributes.Reserved );
                }
                if ( mwsexinfo.VirtualAttributes.Invalid.Valid )
                {
                    wprintf( L" Ex:" );
                    if ( mwsexinfo.VirtualAttributes.Invalid.Shared )
                    {
                        wprintf( L" fShared" );
                    }
                    if ( mwsexinfo.VirtualAttributes.Invalid.Bad )
                    {
                        wprintf( L" fBad" );
                    }
                    if ( mwsexinfo.VirtualAttributes.Invalid.Reserved0 )
                    {
                        wprintf( L" Res0=0x%x", mwsexinfo.VirtualAttributes.Invalid.Reserved0 );
                    }
                    if ( mwsexinfo.VirtualAttributes.Invalid.Reserved1 )
                    {
                        wprintf( L" Res1=0x%x", mwsexinfo.VirtualAttributes.Invalid.Reserved1 );
                    }
                    //  Not apparently present in x86 definition of this struct.
#ifdef _AMD64_
                    if ( mwsexinfo.VirtualAttributes.Invalid.ReservedUlong )
                    {
                        wprintf( L" ResUl=0x%x", mwsexinfo.VirtualAttributes.Invalid.ReservedUlong );
                    }
#endif
                }
            }
            else
            {
                wprintf( L" NotResident?" );
            }
        }
        else
        {
            wprintf( L"QWSExFailed??" );
        }

        wprintf( L"\n" );
    }
}

void PrintMemInfo_( const ULONG ibPrefix, const void * const pbMemory, PCSTR szMemVarName )
{
    PrintMemInfo_( ibPrefix, (BYTE*)pbMemory, szMemVarName );
}

