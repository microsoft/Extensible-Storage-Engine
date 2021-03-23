// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_HXX_INCLUDED
#define _OS_HXX_INCLUDED

#include "cc.hxx"

//  
//  ESE OS Abstraction Layer / Library 
//

// -----------------------------------------------------------------------------
//  Using the OS Layer / Library
//

//  Step 1) #include "os.hxx", and ensure your sources includes can find it and
//          all similarly includes headers, types.hxx, osfs.hxx, sync.hxx, etc.
//  Step 2) Must link to the OS Library / oslite.lib.
//  Step 3) Must link to the ESE sync.lib library (required by oslite.lib).
//  Step 4) Must decide how you want to initialize and terminate the OS Layer,
//          see "Initializing and Terminating the library".
//
//  Optional Steps:
//
//  Step 5) If the user wants a Thread Local State (TLS) with the OS Layer's
//          Ptls() capability, then the user must define a struct TLS {...}
//          (see "OS Interface" below), so that linking can properly occur with
//          Ptls() is called.  User must also call OSPrepreinitSetUserTLSSize()
//          to inform the OS layer about the size of the user TLS.
//

// -----------------------------------------------------------------------------
//  Temporary: OS Layer Violations.
//  User should not define OS_LAYER_VIOLATIONs.

#ifdef OS_LAYER_VIOLATIONS
class INST;
#else
//  This redefining INST to an unsigned char works because nearly every last 
//  reference to an INST in the OS Layer is an 'INST *', and nothing in the OS
//  Layer really derefs it, and calls a member of INST::*, mostly just passes 
//  it through as context to things like UtilReportEvent().
typedef unsigned char INST;
#endif

//  This is used to enable datacenter level optics that we manage / monitor for
//  O365, and/or that are too expensive to enable by default for windows.
#ifndef ESENT
#define ENABLE_MICROSOFT_MANAGED_DATACENTER_LEVEL_OPTICS 1
#endif

#ifdef ENABLE_MICROSOFT_MANAGED_DATACENTER_LEVEL_OPTICS
#define OnMsftDatacenterOptics( x ) x
#else
#define OnMsftDatacenterOptics( x )
#endif // ENABLE_MICROSOFT_MANAGED_DATACENTER_LEVEL_OPTICS




// -----------------------------------------------------------------------------
//  Include required helper libraries.

#include "sync.hxx"

// -----------------------------------------------------------------------------
//  Include the OSAL functionality.

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

// -----------------------------------------------------------------------------
//  Include more required helper libraries.

#include "stat.hxx"
#include "memfile.hxx"

// -----------------------------------------------------------------------------
//  Windows NT Extension Headers

#include "edbg.hxx"
#include "perfmon.hxx"
#include "oseventtrace.hxx"
#include "eseeventtrace.g.hxx"
#include "hapublish.hxx"

// -----------------------------------------------------------------------------
//  OS layer pre-preinit
//
//  user TLS
//
void OSPrepreinitSetUserTLSSize( const ULONG cbUserTLSSize );

#ifdef OS_LAYER_VIOLATIONS
#if defined(_WIN64)
#define ESE_USER_TLS_SIZE 184
#else
#define ESE_USER_TLS_SIZE 160
#endif
#endif  // OS_LAYER_VIOLATIONS

// -----------------------------------------------------------------------------
//  Initializing and Terminating the library.
//

//  The OS Layer / Library is initialized in three distinct steps, and 
//  terminated in two different distinct steps.  The initialization steps are:
//
//      0. OSPreinitSetUserTlsSize() - Optional, must be set if Ptls() facility is used.
//      1. FOSPreinit()
//      2. Configuring certain OS Layer globals.
//      3. ErrOSInit()
//
//  The termination steps:
//
//      4. ErrOSTerm()
//      5. OSPostterm()
//

//  ----- As an DLL -----
//
//  If you link to a DLL, the OS layer defines a DLLEntryPoint() for you (this
//  is something we may have to fix someday if some other DLL doesn't like this),
//  and this calls FOSPreinit() / OSPostterm for the consumer DLL.  Steps
//  2, and 4 are the DLLs responsibility.
//

//  ----- As an EXE -----
//
//  While as a DLL the DLLEntryPoint calls pre-init, for an EXE this helper class 
//  will FOSPreinit() / OSPostterm the OS library properly.  Merely define this
//  on the stack of the main function:
//
//      COSLayerPreInit     oslayer( fTrue );
//      Then MUST check oslayer.FInitd().
//
//  Steps 2, 3 and 4 are the EXEs responsibility.
//

class COSLayerPreInit
{
    public:

        //  This is the only allowed constructor, meant for calling as 
        //  instructed above, doing several "Preinit" tasks.

        COSLayerPreInit();

        //  Immediately after .ctor must check this to ensure there was
        //  no disasterous memory allocation failures during init.  If
        //  this is false, the OS library is not safe to use.

        BOOL FInitd();

        //  This is the destructor, and "Postterms" the library.
        //  Note: Things such as memory leaks will be caught in here.

        ~COSLayerPreInit();

        //  OS Layer parameters
        //
        
        //  Reset the parameters to the default configuration
        //
        static void SetDefaults();

        //  debugging
        //
        static UINT SetAssertAction( UINT AssertAction );
        static UINT SetExceptionAction( const UINT ExceptionAction );
        static void SetCatchExceptionsOnBackgroundThreads( const BOOL fCatch );
        static void SetRFSAlloc( ULONG cRFSAlloc );
        static void SetRFSIO( ULONG cRFSIO );

        //  os file settings
        //
        static void SetZeroExtend( QWORD cbZeroExtend );
        static void SetIOMaxOutstanding( ULONG cIOs );
        static void SetIOMaxOutstandingBackground( ULONG cIOs );

        //  analysis sub-systems, logging, perfmon, etc...
        //
        static void SetProcessFriendlyName( const WCHAR* const wszProcessFriendlyName );
        static void SetEventLogCache( ULONG cbEventCache ); //
        static void EnablePerfmon();                            // by default perfmon is enabled
        static void DisablePerfmon();
        static void DisableTracing();
        static void SetRestrictIdleActivity( const BOOL fRestrictIdleActivity );

        //  performance data
        //
        typedef VOID ( *PfnThreadWait )( );
        static PfnThreadWait SetThreadWaitBeginNotification( PfnThreadWait pfnThreadWait );
        static PfnThreadWait SetThreadWaitEndNotification( PfnThreadWait pfnThreadWait );

    private:
        BOOL        m_fInitedSuccessfully;

        
    private:

        // These are forbidden methods.
        // non-implemented functions. these objects should be allocated off the
        // stack using the public constructor

#pragma push_macro( "new" )
#undef new
        void *  operator new[]( size_t );
        void    operator delete[]( void* );
        void *  operator new( size_t );
        void    operator delete( void* );
#pragma pop_macro( "new" )

        COSLayerPreInit& operator=( const COSLayerPreInit& );
};

//  In case some one needs explicit preinit/postterm we can uncomment these lines,
//  but for now consumers get these trhough DLLEntryPoint() or COSLayerPreInit.
//BOOL FOSPreinit();
//void OSPostterm();


//  init OS subsystem

ERR ErrOSInit();

//  terminate OS subsystem

void OSTerm();

//  lifecycle 

BOOL FOSDllUp();
BOOL FOSLayerUp();

//  Turns on FFB/StorageWriteBack mode always.
//#define FORCE_STORAGE_WRITEBACK       1

// Turn off some APIs that we don't want directly supported
#define wcscmp __USE_LOSSTRCOMPAREW__
// ISSUE[SOMEONE] - Github side of build cannot handle this due to stdlib includes in rmemulator.hxx.  Some day figure this out for real.
//#define wcslen __USE_LOSSTRLENGTHW__

#endif  //  _OS_HXX_INCLUDED


