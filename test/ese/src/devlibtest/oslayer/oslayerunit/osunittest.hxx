// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#define _CRT_RAND_S

// needed for JET errors
#if defined(BUILD_ENV_IS_NT) || defined(BUILD_ENV_IS_WPHONE)
#include <esent_x.h>
#endif
#ifdef BUILD_ENV_IS_EX
#include <jet.h>
#endif

#include "ntstatus.h"   //  for access to STATUS_CRC_ERROR in MemoryMappedIoSuite.cxx
#define WIN32_NO_STATUS
#include <windows.h>
#include <cstdio>
#include <stdlib.h>

#include <functional>

#include "testerr.h"
#include "bstf.hxx"


//  This is OS Layer Unit test Call/CallJ macros, because sometimes we want
//  to test the Call()/CallJ()/etc macros as they're part of the OS Layer.
//

#define OSTestCallJ( func, label )                      TestCallJ( func, label )

#define OSTestCall( func )                              TestCall( func )

#define OSTestCheck( _condition )                       TestCheck( _condition )

#define OSTestCheckErr( func )                          TestCheckErr( func )

#define OSTestCheckExpectedErr( errExpected, func )     TestCheckExpectedErr( errExpected, func )


#define NO_GRBIT 0
#define CP_ASCII    1252
#define CP_UNICODE  1200

VOID RandomizeBuffer( void * const pv, const size_t cb );

VOID ZeroBuffer_( BYTE * pb, ULONG cb );

#pragma prefast(suppress: 6260, "calculation of size is correct.")
#define ZeroBuffer( rgLocal )       ZeroBuffer_( rgLocal, _countof(rgLocal)*sizeof(rgLocal[0]) )

#include <tchar.h>
#define OS_LAYER_UNIT_TESTING_ACCESS 1
#include "os.hxx"


//  ================================================================
//  OS MM (MemoryManager) Helpers
//  ================================================================

inline QWORD CbIncrementalFileSize( const BOOL fBigFile )
{
    //  The small size is choosen b/c it gives 1 segment with each byte value for the
    //  memset(), so all 256 segments have simple unique data.
    return fBigFile ? ( OSMemoryTotal() * 2 ) : 256 * 4096;
}
ERR ErrCreateIncrementFile( PCWSTR wszFile, const BOOL fBigFile = fFalse );

#define pbMemInfoHeaders        ((BYTE*)-1)
void PrintMemInfo_( const ULONG ibPrefix, const BYTE * const pbMemory, PCSTR szMemVarName );
#define PrintMemInfo( ibPrefix, pbMemory )      PrintMemInfo_( ibPrefix, pbMemory, #pbMemory )


//  ================================================================
//  OS Revision Helpers
//  ================================================================


BOOL FUtilOsIsGeq( const ULONG dwMajorOsVerRequired, const ULONG dwMinorOsVerRequired = 0 );

INLINE BOOL FUtilOsIsGeq( const ULONG dwMajorOsVerRequired, const ULONG dwMinorOsVerRequired )
{
    Assert( dwMajorOsVerRequired != 0 );
    if( DwUtilSystemVersionMajor() > dwMajorOsVerRequired )
    {
        return fTrue;
    }
    if( DwUtilSystemVersionMajor() == dwMajorOsVerRequired  &&
        DwUtilSystemVersionMinor() >= dwMinorOsVerRequired )
    {
        return fTrue;
    }
    return fFalse;
}

class COSFilePerfTest
    : public IFilePerfAPI
{
    public:
        COSFilePerfTest() : IFilePerfAPI( 0, 0xFFFFFFFFFFFFFFFF ), m_cIOCompletions(0), m_cbTransfer(0), m_cIOIssues(0) {}
        virtual ~COSFilePerfTest()                  {}

        virtual DWORD_PTR IncrementIOIssue(
            const DWORD     diskQueueDepth,
            const BOOL      fWrite )
        {
            m_cIOIssues++;
            return NULL;
        }

        virtual VOID IncrementIOCompletion(
            const DWORD_PTR iocontext,
            const DWORD     dwDiskNumber,
            const OSFILEQOS qosIo,
            const HRT       dhrtIOElapsed,
            const DWORD     cbTransfer,
            const BOOL      fWrite )
        {
                m_cIOCompletions++;
                m_cbTransfer += cbTransfer;
        }

        virtual VOID IncrementIOInHeap( const BOOL fWrite )
        {
        }

        virtual VOID DecrementIOInHeap( const BOOL fWrite )
        {
        }

        virtual VOID IncrementIOAsyncPending( const BOOL fWrite )
        {
        }

        virtual VOID DecrementIOAsyncPending( const BOOL fWrite )
        {
        }

        virtual VOID IncrementIOAbnormalLatency( const BOOL fWrite )
        {
        }

        virtual VOID IncrementFlushFileBuffer( const HRT dhrtElapsed )
        {
        }

        virtual VOID SetCurrentQueueDepths( const LONG cioMetedReadQueue, const LONG cioAllowedMetedOps, const LONG cioAsyncRead )
        {
        }

        INT NumberOfIOCompletions()
        {
            return m_cIOCompletions;
        }

        INT NumberOfBytesWritten()
        {
            return m_cbTransfer;
        }

        INT NumberOfIOIssues()
        {
            return m_cIOIssues;
        }
    private:
        INT m_cIOCompletions;
        INT m_cbTransfer;
        INT m_cIOIssues;
};

