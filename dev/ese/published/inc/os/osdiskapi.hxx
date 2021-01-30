// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OSDISKAPI_HXX_INCLUDED
#define _OSDISKAPI_HXX_INCLUDED



typedef void IDiskAPI;




enum OSDiskIoQueueManagement
{
    dioqmInvalid        = 0x0,

    dioqmTypeHeapA      = 0x1,
    dioqmTypeHeapB      = 0x2,
    dioqmTypeHeapAandB  = ( dioqmTypeHeapA | dioqmTypeHeapB ),
    dioqmTypeVIP        = 0x4,
    dioqmTypeMetedQ     = 0x8,

    mskQueueType        = ( dioqmTypeVIP | dioqmTypeHeapA | dioqmTypeHeapB | dioqmTypeMetedQ ),

    dioqmAccessStall    = 0x10,
    dioqmReEnqueued     = 0x20,
    dioqmMergedIorun    = 0x40,
    dioqmFreeVipUpgrade = 0x80,
    dioqmMetedQCycled   = 0x100,

    dioqmArtificialLimit = 0x1FF
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




class IVolumeAPI
{
    public:

        IVolumeAPI()            {}
        virtual ~IVolumeAPI()   {}



        virtual ERR ErrDiskSpace(   const WCHAR* const  wszPath,
                                    QWORD* const        pcbFreeForUser,
                                    QWORD* const        pcbTotalForUser = NULL,
                                    QWORD* const        pcbFreeOnDisk = NULL ) = 0;



        virtual ERR ErrFileSectorSize(  const WCHAR* const  wszPath,
                                        DWORD* const        pcbSize ) = 0;


        virtual ERR ErrFileAtomicWriteSize( const WCHAR* const  wszPath,
                                            DWORD* const        pcbSize ) = 0;


        virtual ULONG CRef() const = 0;

        
        virtual BOOL FDiskFixed() = 0;

        
        virtual ERR ErrDiskId( ULONG_PTR* const pulDiskId ) const = 0;


        virtual BOOL FSeekPenalty() const = 0;
};

class CIoStats_;
class CIoStats
{
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
    TICK                    m_tickSpikeBaselineCompletedTime;

private:
    CIoStats();

public:
    static ERR ErrCreateStats( CIoStats ** ppiostatsOut, const BOOL fServerMemoryConfig );
    ~CIoStats();

    void AddIoSample_( const QWORD cusecIoTime  );
    BOOL FStartUpdate_( const TICK tickNow );

    void AddIoSample( HRT dhrtIoTime );

    BOOL FStartUpdate();
    QWORD CusecAverage() const;
    QWORD CusecPercentile( const ULONG pct ) const;
    QWORD CioAccumulated() const;

    void FinishUpdate( const DWORD dwDiskNumber );

    void Tare();
};

#define bitUseMetedQ            0x1
#define bitAllowIoBoost         0x4
#define bitUseMetedQEseTasks	0x8
#define bitNewQueueOptionsMask  ( bitUseMetedQ | bitAllowIoBoost | bitUseMetedQEseTasks )

#endif

