// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OSDISKAPI_HXX_INCLUDED
#define _OSDISKAPI_HXX_INCLUDED

//  Disk API

//      Disk interface, opaque handle.

//  Today there is no extended features externally available, so it is an opaque handle.
typedef void IDiskAPI;


//  Disk Trace Mappings

//      For processing ETW trace files.

enum OSDiskIoQueueManagement // dioqm
{
    dioqmInvalid        = 0x0,

    dioqmTypeHeapA      = 0x1,
    dioqmTypeHeapB      = 0x2,
    dioqmTypeHeapAandB  = ( dioqmTypeHeapA | dioqmTypeHeapB ),  // heap A and B can get combined.
    dioqmTypeVIP        = 0x4,      //  Note: this cuts the line and skips right to the front.
    dioqmTypeMetedQ     = 0x8,

    mskQueueType        = ( dioqmTypeVIP | dioqmTypeHeapA | dioqmTypeHeapB | dioqmTypeMetedQ ),

    dioqmAccessStall    = 0x10,
    dioqmReEnqueued     = 0x20,     // usually because of OOM on issue, or (possibly) lack of scatter/gather IO. [only logged on enqueue.]
    dioqmMergedIorun    = 0x40,
    dioqmFreeVipUpgrade = 0x80,
    dioqmMetedQCycled   = 0x100,

    dioqmArtificialLimit = 0x1FF     // keep it lean if possible / down to 2 bytes (logged at full BOOL/32-bit for now).
};

DEFINE_ENUM_FLAG_OPERATORS_BASIC( OSDiskIoQueueManagement )

inline CHAR * SzOSDiskMapQueueType( _In_ const DWORD dioqm )
{
    switch( dioqm & mskQueueType )
    {
        case dioqmTypeHeapA:    return "Heap A";
        case dioqmTypeHeapB:    return "Heap B";
        case dioqmTypeHeapAandB:return "Heap A&B";
        case dioqmTypeVIP:      return "VIP List";
        case dioqmTypeMetedQ:   return "Meted Queue";
    }
    Assert( fFalse );
    return "UnknownQ";
}


//  Volume API

//      Useful Volume Functions

class IVolumeAPI  //  fsapi
{
    public:

        IVolumeAPI()            {}
        virtual ~IVolumeAPI()   {}

        //  Properties

        //  returns the amount of free/total disk space on the drive hosting the specified path

        virtual ERR ErrDiskSpace(   const WCHAR* const  wszPath,
                                    QWORD* const        pcbFreeForUser,
                                    QWORD* const        pcbTotalForUser = NULL,
                                    QWORD* const        pcbFreeOnDisk = NULL ) = 0;


        //  returns the sector size for the specified path

        virtual ERR ErrFileSectorSize(  const WCHAR* const  wszPath,
                                        DWORD* const        pcbSize ) = 0;

        //  returns the atomic write size for the specified path

        virtual ERR ErrFileAtomicWriteSize( const WCHAR* const  wszPath,
                                            DWORD* const        pcbSize ) = 0;

        //  returns the number of referrers

        virtual ULONG CRef() const = 0;

        //  get if disk is fixed (and therefore unlikely to suddenly disappear)
        
        virtual BOOL FDiskFixed() = 0;

        //  get disk ID for this file
        
        virtual ERR ErrDiskId( ULONG_PTR* const pulDiskId ) const = 0;

        //  get seek penalty (in order to identify SSD)

        virtual BOOL FSeekPenalty() const = 0;
};

//  Tracks Latency and Rate of IO operations
//
class CIoStats_;
class CIoStats // iostats
{
    //  Sampling 1000 out of 3000 assuming a normal distribution will will allow us to be 99% confident that 
    //  the values are within 3% of the real values.  Caveat, we're not ACTUALLY sampling when we take the 
    //  _first_ 1000.
    //
    //  So we build values off these two things ...
    //      cioMinAccumSampleRate - Once we have this many samples at a dtickSampleUpdateRate event, then
    //                  we reset the stats arrays, and accumulate new data for the next time 
    //                  slice ( dtickSampleUpdateRate ).  We consider this a typical min rate
    //                  the disk will generally fill within dtickSampleUpdateRate / 5 minutes.
    //      cioMinValidSampleRate - Any given period where we get at least this many samples, we will at
    //                  least update the performance counters, but continue to accumulate data.
    //                  Note: This value has to be public as this is where we check.
    //
    //  Other constants at top of iostats.cxx.  Of course the debug values are just to induce frequent testing.
    //
public:
    const static INT        cioMinValidSampleRate = OnDebugOrRetail( 50, 1000 );
private:
    const static INT        cioMinAccumSampleRate = OnDebugOrRetail( 120, 3000 );

private:
    CReaderWriterLock       m_rwl;
    CIoStats_ *             m_piostats_;
    void *                  m_pvLargeAlloc;
    void *                  m_pvSmallAlloc;
    TICK                    m_tickLastFastDataUpdate;
    TICK                    m_tickSpikeBaselineCompletedTime;  // initial baseline 24 hours, subsequent baselines every 12 hours

private:
    CIoStats();

public:
    static ERR ErrCreateStats( CIoStats ** ppiostatsOut, const BOOL fServerMemoryConfig );
    ~CIoStats();

    void AddIoSample_( const QWORD cusecIoTime  ); // used by test, maybe should be protected
    BOOL FStartUpdate_( const TICK tickNow ); // used by test

    void AddIoSample( HRT dhrtIoTime );

    BOOL FStartUpdate();
    QWORD CusecAverage() const;
    QWORD CusecPercentile( const ULONG pct ) const;
    QWORD CioAccumulated() const;

    void FinishUpdate( const DWORD dwDiskNumber );

    void Tare();
};

//  Flighting options used in JET_paramFlight_NewQueueOptions
#define bitUseMetedQ            0x1
//#define bitForegroundCombine  0x2 // turned on always now.
#define bitAllowIoBoost         0x4
#define bitUseMetedQEseTasks	0x8 // turn on meted Q for ESE's internal tasks of low priority.
#define bitNewQueueOptionsMask  ( bitUseMetedQ | bitAllowIoBoost | bitUseMetedQEseTasks )

#endif  //  _OSDISKAPI_HXX_INCLUDED

