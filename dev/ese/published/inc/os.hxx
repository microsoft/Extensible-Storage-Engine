// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_HXX_INCLUDED
#define _OS_HXX_INCLUDED

#include "cc.hxx"





#ifdef OS_LAYER_VIOLATIONS
class INST;
#else
typedef unsigned char INST;
#endif

#ifndef ESENT
#define ENABLE_MICROSOFT_MANAGED_DATACENTER_LEVEL_OPTICS 1
#endif

#ifdef ENABLE_MICROSOFT_MANAGED_DATACENTER_LEVEL_OPTICS
#define OnMsftDatacenterOptics( x ) x
#else
#define OnMsftDatacenterOptics( x )
#endif





#include "sync.hxx"


#include "types.hxx"
#include "math.hxx"
#include "error.hxx"
#include "string.hxx"
#include "time.hxx"
#include "trace.hxx"
#include "config.hxx"
#include "cprintf.hxx"
#include "event.hxx"
#include "thread.hxx"
#include "ostimerqueue.hxx"
#include "memorynotification.hxx"
#include "norm.hxx"
#include "memory.hxx"
#include "osfs.hxx"
#include "library.hxx"
#include "osdiag.hxx"
#include "sysinfo.hxx"
#include "task.hxx"
#include "encrypt.hxx"
#include "osblockcache.hxx"


#include "stat.hxx"
#include "memfile.hxx"


#include "edbg.hxx"
#include "perfmon.hxx"
#include "oseventtrace.hxx"
#include "eseeventtrace.g.hxx"
#include "hapublish.hxx"

void OSPrepreinitSetUserTLSSize( const ULONG cbUserTLSSize );

#ifdef OS_LAYER_VIOLATIONS
#if defined(_WIN64)
#define ESE_USER_TLS_SIZE 184
#else
#define ESE_USER_TLS_SIZE 160
#endif
#endif





class COSLayerPreInit
{
    public:


        COSLayerPreInit();


        BOOL FInitd();


        ~COSLayerPreInit();

        
        static void SetDefaults();

        static UINT SetAssertAction( UINT AssertAction );
        static UINT SetExceptionAction( const UINT ExceptionAction );
        static void SetCatchExceptionsOnBackgroundThreads( const BOOL fCatch );
        static void SetRFSAlloc( ULONG cRFSAlloc );
        static void SetRFSIO( ULONG cRFSIO );

        static void SetZeroExtend( QWORD cbZeroExtend );
        static void SetIOMaxOutstanding( ULONG cIOs );
        static void SetIOMaxOutstandingBackground( ULONG cIOs );

        static void SetProcessFriendlyName( const WCHAR* const wszProcessFriendlyName );
        static void SetEventLogCache( ULONG cbEventCache );
        static void EnablePerfmon();
        static void DisablePerfmon();
        static void DisableTracing();
        static void SetRestrictIdleActivity( const BOOL fRestrictIdleActivity );

        typedef VOID ( *PfnThreadWait )( );
        static PfnThreadWait SetThreadWaitBeginNotification( PfnThreadWait pfnThreadWait );
        static PfnThreadWait SetThreadWaitEndNotification( PfnThreadWait pfnThreadWait );

    private:
        BOOL        m_fInitedSuccessfully;

        
    private:


#pragma push_macro( "new" )
#undef new
        void *  operator new[]( size_t );
        void    operator delete[]( void* );
        void *  operator new( size_t );
        void    operator delete( void* );
#pragma pop_macro( "new" )

        COSLayerPreInit& operator=( const COSLayerPreInit& );
};




ERR ErrOSInit();


void OSTerm();


BOOL FOSDllUp();
BOOL FOSLayerUp();



#endif


