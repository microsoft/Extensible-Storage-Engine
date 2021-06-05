// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//  SvcInitMainGoo.c : Application Sample of ESE
//

#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <strsafe.h>

#include <esent.h>

#include "SvcInit.h"

//
//              Multi-Service Init and Parameter Configuration Sample
//

//  Services are often started in service hosts, where multiple ESE client instances may
//  need to co-exist peacefully, and potentially start and initialize thier instance(s)
//  with no direct runtime (and little overall) coordination.  This sample covers the 
//  different ways to achieve this aim.

//  The sample is broken into 4 files:
//      SvcInitMainGoo.c    - This file is just connector "goo", and some overall notes.
//      SvcInit.h           - This file is more connector "goo".  Defn of CONN_DB_STATE.
//      SvcInitSampleA.c    - This implements Method A described below.
//      SvcInitSampleB.c    - This implements Method B described below.
//      SvcInitTermSample.c - Shutsdown a CONN_DB_STATE.
//
//  Either of the two *Sample.c files are meant to by copied, but review, remove and 
//  address all comments starting with ESE_CLIENT_TODO.


//      Pre Information:
//
//  ESE parameters come in two scopes and two lifetimes: global or instance specific scope,
//  and fixed or dynamic lifetime.
//
//      Global Fixed - Affect the whole process and are not reconfigurable after
//          first instannce has started or JetEnableMultiInstance is called.  Such as
//          JET_paramDatabasePageSize, or JET_paramEnableFileCache.
//
//      Global Dynamic - Affect the whole process and are switchable at anytime.  Such as 
//          JET_paramCacheSizeMax.
//
//      Instance Specific Fixed - Affect only one instance, but can not be reconfigured
//          after the instance starts / calls JetInit().  Such as JET_paramLogFileSize.
//
//      Instance Specific Dynamic - The most flexible of all types, these affect only one 
//          instance, and further can be reconfigured at anytime during the instances 
//          lifetime.  Such as JET_paramCheckpointDepthMax.
//
//  When setting a Global parameter, simply pass a JET_INSTANCE instNil = JET_instanceNil
//  into JetSetSystemParameter( &instNil, .... ) as the first argument.  When setting an 
//  Instance Specific parameter pass in the instance created with JetCreateInstance() or 
//  JetInit().
//
//  It is also possible to set Instance Specific parameters like globals by passing in the
//  &instNil (from above) in ... when done like this, it effectively is like setting the 
//  default for that parameter that will be utilized when the any new instances are created
//  or initialized.  It does not however "set it globally" in an override sense, the instance
//  specific parameter will be pulled from the instance param table, not the global param 
//  table.
//
//  A couple parameters of note: JET_paramConfiguration and JET_paramConfigStoreSpec, are
//  special in scope in that they both have a Global Fixed and a Instance Specific Fixed
//  scope depending upon if an active instance is passed into JetSetSystemParameter() ...
//  and as such the value is used in both scopes.  These parameters are also a bit different
//  in that they affect many other parameter values.
//
//  This creates a couple conundrums for svchost-based ESE clients:
//
//      1.  There is no way for two ESE instances / clients to run concurrently in the same 
//          process with different values for a Global-Fixed parameter, such as a different 
//          JET_paramDatabasePageSize.  So all ESE instances must agree on page size (and a
//          hanful of other parameters), or move to a different process / svchost.
//
//      2.  This means there is no way today to free an ESE client completely from some 
//          form of coordination with other instances in thier process ... such as an agreed 
//          upon set of global parameters (Method A below), or indirecting to a common 
//          location in the registry (Method B below).
//
//      3.  Further, even Global Dynamic paramters are a bit of a issue, because there is not
//          a way to say "increase" the global cache size.  It is recommended that two
//          instances in a single svchost agree on a good median values for there usage.
//
//  The rest of this sample discusses primarily how to achieve #2 in the least painful way,
//  and simultaneously ensuring that the two clients can operation on thier initialization
//  sequences un-coordinated and without knowledge of when the other might run (potentially
//  even running completely concurrently).


//      Multi-Service Init and Parameter Configuration Options:
//
//  Two services in the same svchost, must still agree upon all the _global_ ESE parameters
//  of the svchost process that they intend to use, because ESE's parameter system is 
//  not 100% instance specific.
//
//  There are two methods that can achieve the minimal coordination necessary:
//
//      A. JetEnableMultiInstance() common parameter set method.
//
//          If the two pieces of code can simply agree on a set of parameters, then they
//          can simply pass thier matching set of parameters to this API.  The API co-ordinates
//          so the two services do not have to agree / create a matching named critical section, 
//          or other synchronization object.
//
//          implemented in: SvcInitSampleA.c : ErrEnableMultiInstStartSample()
//
//      B. JET_paramConfigStoreSpec registry method.
//
//          This method builds off the previous method ... if the two only agree on the path
//          for the common ESE configuration store, then all other global parameters can be
//          picked up from that location.
//
//          It is assumed the simpliest way for the registry to be setup is via a manifest 
//          declared registry keys at Windows or Application install time.
//
//          implemented in: SvcInitSampleB.c : ErrConfigStorePathStartSample()
//          availability: Only available in WinBlue+1 / Windows Phone 8.
//
//  To compare these strategies, run:
//      windiff SvcInitSampleA.c SvcInitSampleB.c
//
//  Note in this diff: That Service X is a simple transistion to the new model for global system 
//  parameters, and Service Y also transistions its instance specific / scoped system parameters.
//


//      Futures:
//
//  In an ideal world, there would only be instance specific parameters, so any two ESE
//  clients can exist in the process, completely without coordination or agreement on any
//  of the ESE parameters.
//
//  It is also recommended that you ask the ESE team (SOMEONE@microsoft.com) to make your
//  favorite parameters instance specific, so we can solve this problem with the most
//  painful parameters at a time.
//

//  SOMEONETODO check if we need any more ESE_CLIENT_TODO markers ...?

//
//  We have excluded all read-only, private, and instance specific parameter ... then broken the params
//  into three groups:
//
//  Global Fixed (Commonly / Likely Used):
//
//      JET_paramConfiguration
//      JET_paramDisablePerfmon
//      JET_paramDatabasePageSize
//      JET_paramMaxInstances
//      JET_paramEnableFileCache
//      JET_paramEnableViewCache
//      JET_paramOutstandingIOMax
//      JET_paramHungIOThreshold
//      JET_paramHungIOActions
//
//  Global Dynamic (Commonly / Likely Used):
//  
//      JET_paramCacheSize
//      JET_paramCacheSizeMin
//      JET_paramCacheSizeMax
//      JET_paramStartFlushThreshold
//      JET_paramStopFlushThreshold
//
//  Rarely Used Global Params:
//
//      JET_paramAccessDeniedRetryPeriod
//      JET_paramOSSnapshotTimeout
//      JET_paramExceptionAction
//      JET_paramEventLogCache
//      JET_paramTableClass1Name
//      JET_paramTableClass2Name
//      JET_paramTableClass3Name
//      JET_paramTableClass4Name
//      JET_paramTableClass5Name
//      JET_paramTableClass6Name
//      JET_paramTableClass7Name
//      JET_paramTableClass8Name
//      JET_paramTableClass9Name
//      JET_paramTableClass10Name
//      JET_paramTableClass11Name
//      JET_paramTableClass12Name
//      JET_paramTableClass13Name
//      JET_paramTableClass14Name
//      JET_paramTableClass15Name
//      JET_paramProcessFriendlyName
//      JET_paramLRUKCorrInterval
//      JET_paramLRUKHistoryMax
//      JET_paramLRUKPolicy
//      JET_paramLRUKTimeout
//      JET_paramRecordUpgradeDirtyLevel
//      JET_paramIOThrottlingTimeQuanta
//      JET_paramGlobalMinVerPages
//      JET_paramPageHintCacheSize
//      JET_paramMaxCoalesceReadSize
//      JET_paramMaxCoalesceWriteSize
//      JET_paramMaxCoalesceReadGapSize
//      JET_paramMaxCoalesceWriteGapSize
//      JET_paramVerPageSize
//      JET_paramMinDataForXpress
//  


//  
//      Internal Non-Sample helpers
//

INT ErrPrintUsageFail( const char *szAppName, const WCHAR * const szReason )
{
    wprintf( L"Usage: %hs [JetEnableMultiInstance|JET_paramConfigStoreSpec [StrDifferentiator]]\n\n", szAppName );
    wprintf( L"Failed: %ws\n\n", szReason );
    return JET_errInvalidParameter;
}

JET_ERR ErrFromStrsafeHr( const HRESULT hr )
{
    JET_ERR err = (hr == SEC_E_OK) ?
        JET_errSuccess :
        (hr == STRSAFE_E_INSUFFICIENT_BUFFER) ?
            JET_errBufferTooSmall :
            (hr == STRSAFE_E_INVALID_PARAMETER) ?
                JET_errInvalidParameter :
                JET_errInternalError;
    assert( err == JET_errSuccess ||
            err == JET_errBufferTooSmall ); //  this is the only really expected error
    return(err);
}

int __cdecl main(int argc, char ** argv)
{
    JET_ERR         err = JET_errSuccess;
    const CHAR *    szRegDiff = "Default";
    const CHAR *    szSample = NULL;

    //
    //      Process the args ... this is just "goo".
    //

    if ( argc < 2 )
    {
        return ErrPrintUsageFail( argv[0], L"Not enough arguments, must specify the sub-sample to run (JetEnableMultiInstance|JET_paramConfigStoreSpec)." );
    }
    szSample = argv[1];

    if ( argc > 2 )
    {
        szRegDiff = argv[2];
    }

    //
    //      Run the sample specified
    //

    wprintf( L"Begining SvcInit Sample (argc = %d, %hs)\n", argc, szSample );
    if ( 0 == _stricmp( szSample, "JetEnableMultiInstance" ) )
    {
        err = ErrEnableMultiInstStartSample();
    }
    else if ( 0 == _stricmp( szSample, "JET_paramConfigStoreSpec" ) )
    {
        //  Configure the Registry path
        //
        WCHAR wszRegistryConfigPath[260];
        err = ErrFromStrsafeHr( StringCbPrintfW( wszRegistryConfigPath, sizeof(wszRegistryConfigPath), L"reg:HKLM\\SOFTWARE\\Microsoft\\Ese_SvcInitSample_%hs", szRegDiff ) );

        err = ErrConfigStorePathStartSample( wszRegistryConfigPath, FALSE );
    }
    else if ( 0 == _stricmp( szSample, "JET_paramConfigStoreSpec2" ) )
    {
        //  Configure the Registry path
        //
        WCHAR               wszRegistryConfigPath[260];
        err = ErrFromStrsafeHr( StringCbPrintfW( wszRegistryConfigPath, sizeof(wszRegistryConfigPath), L"reg:HKLM\\SOFTWARE\\Microsoft\\Ese_SvcInitSample_%hs", szRegDiff ) );

        err = ErrConfigStorePathStartSample( wszRegistryConfigPath, TRUE );
    }
    else
    {
        return ErrPrintUsageFail( argv[0], L"Unknown sub-sample specified, must run JetEnableMultiInstance|JET_paramConfigStoreSpec|JET_paramConfigStoreSpec2." );
    }

    wprintf( L"\nSample done.\n\n" );
    
    return err;
}

