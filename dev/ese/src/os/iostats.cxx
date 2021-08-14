// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

//
//      Constants for timing sequences, sampling rate, etc.
//

const static LONG dtickSampleUpdateRate     = OnDebugOrRetail( 5 * 1000, 5 * 60 * 1000 );   // 5 min (or 5 sec in debug)

const static INT dtickInitialDeferralToLatencySpikeBaseline = OnDebugOrRetail( 6 * 1000, 24 * 60 * 60 * 1000 );
const static INT dtickSubsequentDeferralToLatencySpikeBaseline  = OnDebugOrRetail( 3 * 1000, 12 * 60 * 60 * 1000 ); // now allow +12 more hours of deferral
const static QWORD cusecMinIoLatencySpikeToReport           = 100 * 1000; // if our 99th percentile is < 100 ms, don't bother reporting it.


//  Internal implementation details.
//

LOCAL ERR ErrStatsErr( const CStats::ERR staterr )
{
    switch( staterr )
    {
    case CStats::ERR::errSuccess:                               return JET_errSuccess;
    case CStats::ERR::errInvalidParameter:   Assert( fFalse );  return ErrERRCheck( JET_errInvalidParameter );
    case CStats::ERR::errOutOfMemory:                           return ErrERRCheck( JET_errOutOfMemory );
    case CStats::ERR::wrnOutOfSamples:                          return ErrERRCheck( errNotFound );
    case CStats::ERR::errFuncNotSupported:   Assert( fFalse );  return ErrERRCheck( JET_errNyi );
    }
    Assert( fFalse );
    return ErrERRCheck( errCodeInconsistency );
}

class CIoStats_
{
    friend class CIoStats;

private:
    CCompoundHistogram      m_histoLatencies;
    ULONG                   m_pctLast;
    QWORD                   m_cusecWorstP99;

#ifdef TEST_DOUBLE_CHECK_AGAINST_PERFECT_HISTO
    CStats *                m_phistoTrue;
#endif

public:
    CIoStats_()
    {
        m_pctLast = 0;
        m_cusecWorstP99 = 0;
#ifdef TEST_DOUBLE_CHECK_AGAINST_PERFECT_HISTO
        m_phistoTrue = new CPerfectHistogramStats();
#endif
    }
    ~CIoStats_()
    {
#ifdef TEST_DOUBLE_CHECK_AGAINST_PERFECT_HISTO
        delete m_phistoTrue;
        m_phistoTrue = NULL;
#endif
    }

    void ResetCurrency()
    {
        CallS( ErrStatsErr( m_histoLatencies.ErrReset() ) );
        m_pctLast = 0;
    }

    void ZeroLatencies()
    {
        ResetCurrency();
        m_histoLatencies.Zero();
#ifdef TEST_DOUBLE_CHECK_AGAINST_PERFECT_HISTO
        if ( m_phistoTrue )
            m_phistoTrue->Zero();
#endif
    }
    
    SAMPLE CusecPercentile( const ULONG pct )
    {
        Assert( pct == 0 || pct > m_pctLast );

        if ( pct == 0 )
        {
            const SAMPLE qwMin = m_histoLatencies.Min();
            Assert( qwMin != qwMax ); // we should have 3000 data points, so we should have a min value.

#ifdef TEST_DOUBLE_CHECK_AGAINST_PERFECT_HISTO
            SAMPLE qwMinTrue = 0x4242424242424242;
            if ( m_phistoTrue )
            {
                m_phistoTrue->ErrReset();
                qwMinTrue = m_phistoTrue->Min();
            }
            wprintf( L"  C = %I64d\n", m_histoLatencies.C() );
            wprintf( L"  min = %I64u == %I64u\n", qwMin, qwMinTrue );
#endif
            return qwMin;
        }
        else if ( pct < 100 )
        {
            Assert( pct > m_pctLast );

            ULONG pctT = pct;
            SAMPLE qwPctSample = 0;
            CStats::ERR errStats = m_histoLatencies.ErrGetPercentileHits( &pctT, &qwPctSample );
            Assert( errStats == CStats::ERR::errSuccess );

#ifdef TEST_DOUBLE_CHECK_AGAINST_PERFECT_HISTO
            SAMPLE qwPctTrue = 42;
            ULONG pctIn = pct;
            if ( m_phistoTrue )
                m_phistoTrue->ErrGetPercentileHits( &pctIn, &qwPctTrue );
            wprintf( L"  %d%% = %I64u == %I64u\n", pct, qwPctSample, qwPctTrue );
#endif

            //  Special: Update the worst 99th percentile tracker
            if ( pct == 99 )
            {
                m_cusecWorstP99 = max( m_cusecWorstP99, qwPctSample );
            }

            return qwPctSample;
        }
        else if ( pct == 100 )
        {
            m_pctLast = 100;
            const QWORD qwMaxLocal = m_histoLatencies.Max();
#ifdef OS_LAYER_VIOLATIONS
            //  we should have 3000 data points, I don't believe none of them aren't greater than 1 us - but infinitely fast and loadable 
            //  disks may happen some day!  Special case: OS Testing tries passing off a single 0 sample, just to make sure we work.
            Expected( qwMaxLocal != 0 );
#endif
#ifdef TEST_DOUBLE_CHECK_AGAINST_PERFECT_HISTO
            SAMPLE qwMaxTrue = 0;
            if ( m_phistoTrue )
            {
                m_phistoTrue->Min();
                m_phistoTrue->ErrReset();
            }
            wprintf( L"  max = %I64u == %I64u \n", qwMaxLocal, qwMaxTrue );
            //m_histoLatencies.Dump();
#endif
            return qwMaxLocal;
        }

        AssertSz( fFalse, "An unsupported pct (%d) sent in.\n", pct );
        return 0;
    }

};

class CIoStatsLarge_
{
private:

    //  Source of Truth (for Datacenter) - everything flows from here.
    //
    //  Formatted as table, starting with the "segments" sub-resolution, and then a highest resolution this segment 
    //  will accept.
    //
    const static INT usHighRes =      50;  const static INT usHighResMax =     2500;  // buckets = 50 -> 50 us res to 2.5 ms
    const static INT usMedmRes =    2500;  const static INT usMedmResMax =   100000;  // buckets = 40 -> 2.5 ms res to 100 ms
    const static INT usPokyRes =   10000;  const static INT usPokyResMax =   500000;  // buckets = 40 -> 10 ms res to 500 ms
    const static INT usSlowRes =   50000;  const static INT usSlowResMax =  2500000;  // buckets = 40 -> 100 ms res to 5 secs
    const static INT usHungRes =  500000;  const static INT usHungResMax = 20000000;  // buckets = 35 -> 500 ms res to 20 secs

    const static INT  cHighResBuckets    = usHighResMax / usHighRes + 1;
    const static INT  cMedmResBuckets    = usMedmResMax / usMedmRes + 1;
    const static INT  cPokyResBuckets    = usPokyResMax / usPokyRes + 1;
    const static INT  cSlowResBuckets    = usSlowResMax / usSlowRes + 1;
    const static INT  cHungResBuckets    = usHungResMax / usHungRes + 1;

    BYTE rgbHighResSsd[  CbCFixedLinearHistogram( cHighResBuckets ) ];
    BYTE rgbMedResHd[    CbCFixedLinearHistogram( cMedmResBuckets ) ];
    BYTE rgbPokyResHd[   CbCFixedLinearHistogram( cPokyResBuckets ) ];
    BYTE rgbLowResSlow[  CbCFixedLinearHistogram( cSlowResBuckets ) ];
    BYTE rgbSlowResHung[ CbCFixedLinearHistogram( cHungResBuckets ) ];

public:

    CIoStatsLarge_()
    {
        //                                                                    Not a mistake - the "baseline" / low is previous bucekts max, 
        new( rgbHighResSsd )  CFixedLinearHistogram( sizeof( rgbHighResSsd ),             0, usHighRes, cHighResBuckets, flhfFullRoundUp | flhfHardMin | flhfHardMax );
        new( rgbMedResHd )    CFixedLinearHistogram( sizeof( rgbMedResHd ),    usHighResMax, usMedmRes, cMedmResBuckets, flhfFullRoundUp | flhfHardMin | flhfHardMax );
        new( rgbPokyResHd )   CFixedLinearHistogram( sizeof( rgbPokyResHd ),   usMedmResMax, usPokyRes, cPokyResBuckets, flhfFullRoundUp | flhfHardMin | flhfHardMax );
        new( rgbLowResSlow )  CFixedLinearHistogram( sizeof( rgbLowResSlow ),  usPokyResMax, usSlowRes, cSlowResBuckets, flhfFullRoundUp | flhfHardMin | flhfHardMax );
        new( rgbSlowResHung ) CFixedLinearHistogram( sizeof( rgbSlowResHung ), usSlowResMax, usHungRes, cHungResBuckets, flhfFullRoundUp | flhfHardMin |   flhfSoftMax );
        // Note: soft cap on last to catch all overflow in the last bucket.
    }

    void AddCompounds( CCompoundHistogram * const pCompoundHisto )
    {
        CallS( ErrStatsErr( pCompoundHisto->ErrAddHistoRange( usHighResMax + 1, (CFixedLinearHistogram*)rgbHighResSsd ) ) );
        CallS( ErrStatsErr( pCompoundHisto->ErrAddHistoRange( usMedmResMax + 1, (CFixedLinearHistogram*)rgbMedResHd ) ) );
        CallS( ErrStatsErr( pCompoundHisto->ErrAddHistoRange( usPokyResMax + 1, (CFixedLinearHistogram*)rgbPokyResHd ) ) );
        CallS( ErrStatsErr( pCompoundHisto->ErrAddHistoRange( usSlowResMax + 1, (CFixedLinearHistogram*)rgbLowResSlow ) ) );
        CallS( ErrStatsErr( pCompoundHisto->ErrAddHistoRange(            qwMax, (CFixedLinearHistogram*)rgbSlowResHung ) ) );
    }
};

class CIoStatsSmall_
{
private:
    //  Source of Truth (for Windows) - everything flows from here.
    //
    //  The goal of this is to keep memory light, while still providing debugging level diagnostics of
    //  the last few moments of IO to be able to tell how the client should expect IO to behave.  So we
    //  keep this down to ~20 buckets / ~160 bytes of the spread across the most interesting times.
    //
    //  Formatted as table, starting with the "segments" sub-resolution, and then a highest resolution 
    //  this segment will accept.
    //
    const static INT usRes1 =       1000;  const static INT usRes1Max =     1000;  // buckets = 2 -> 1 ms res to 1 ms
    const static INT usRes2 =      10000;  const static INT usRes2Max =    50000;  // buckets = 5 -> 10 ms res to 50 ms
    const static INT usRes3 =     100000;  const static INT usRes3Max =   300000;  // buckets = 3 -> 100 ms res to 300 ms
    const static INT usRes4 =    1000000;  const static INT usRes4Max =  3000000;  // buckets = 3 -> 1 sec res to 3 secs
    const static INT usRes5 =   15000000;  const static INT usRes5Max = 60000000;  // buckets = 4 -> 15 sec res to 60 secs

    const static INT  cBuckets1  = usRes1Max / usRes1 + 1;
    const static INT  cBuckets2  = usRes2Max / usRes2 + 1;
    const static INT  cBuckets3  = usRes3Max / usRes3 + 1;
    const static INT  cBuckets4  = usRes4Max / usRes4 + 1;
    const static INT  cBuckets5  = usRes5Max / usRes5 + 1;

    BYTE rgbHisto1[ CbCFixedLinearHistogram( cBuckets1 ) ];
    BYTE rgbHisto2[ CbCFixedLinearHistogram( cBuckets2 ) ];
    BYTE rgbHisto3[ CbCFixedLinearHistogram( cBuckets3 ) ];
    BYTE rgbHisto4[ CbCFixedLinearHistogram( cBuckets4 ) ];
    BYTE rgbHisto5[ CbCFixedLinearHistogram( cBuckets5 ) ];

public:

    CIoStatsSmall_()
    {
        //                                                        Not a mistake - the baseline is previous buckets max.
        new( rgbHisto1 ) CFixedLinearHistogram( sizeof( rgbHisto1 ),         0, usRes1, cBuckets1, flhfFullRoundUp | flhfHardMin | flhfHardMax );
        new( rgbHisto2 ) CFixedLinearHistogram( sizeof( rgbHisto2 ), usRes1Max, usRes2, cBuckets2, flhfFullRoundUp | flhfHardMin | flhfHardMax );
        new( rgbHisto3 ) CFixedLinearHistogram( sizeof( rgbHisto3 ), usRes2Max, usRes3, cBuckets3, flhfFullRoundUp | flhfHardMin | flhfHardMax );
        new( rgbHisto4 ) CFixedLinearHistogram( sizeof( rgbHisto4 ), usRes3Max, usRes4, cBuckets4, flhfFullRoundUp | flhfHardMin | flhfHardMax );
        new( rgbHisto5 ) CFixedLinearHistogram( sizeof( rgbHisto5 ), usRes4Max, usRes5, cBuckets5, flhfFullRoundUp | flhfHardMin |   flhfSoftMax );
        // Note: soft cap on last to catch all overflow in the last bucket.
    }

    void AddCompounds( CCompoundHistogram * const pCompoundHisto )
    {
        CallS( ErrStatsErr( pCompoundHisto->ErrAddHistoRange( usRes1Max + 1, (CFixedLinearHistogram*)rgbHisto1 ) ) );
        CallS( ErrStatsErr( pCompoundHisto->ErrAddHistoRange( usRes2Max + 1, (CFixedLinearHistogram*)rgbHisto2 ) ) );
        CallS( ErrStatsErr( pCompoundHisto->ErrAddHistoRange( usRes3Max + 1, (CFixedLinearHistogram*)rgbHisto3 ) ) );
        CallS( ErrStatsErr( pCompoundHisto->ErrAddHistoRange( usRes4Max + 1, (CFixedLinearHistogram*)rgbHisto4 ) ) );
        CallS( ErrStatsErr( pCompoundHisto->ErrAddHistoRange(        qwMax, (CFixedLinearHistogram*)rgbHisto5 ) ) );
    }
};

ERR CIoStats::ErrCreateStats( CIoStats ** ppiostatsOut, const BOOL fDatacenterGranularStats )
{
    ERR err = JET_errSuccess;

    CIoStats * piostatsOut = NULL;

    AllocR( piostatsOut = new CIoStats() );

    Alloc( piostatsOut->m_piostats_ = new CIoStats_() );
#ifndef OS_LAYER_VIOLATIONS
    // in OsLayerUnit.exe there is a test that makes this take too long.
#ifdef TEST_DOUBLE_CHECK_AGAINST_PERFECT_HISTO
    delete piostatsOut->m_piostats_->m_phistoTrue;
    piostatsOut->m_piostats_->m_phistoTrue = NULL;
#endif
#endif

    if ( fDatacenterGranularStats )
    {
        //  Main Exchange / Datacenter ends up here ...
        CIoStatsLarge_ * pT = new CIoStatsLarge_();
        Alloc( piostatsOut->m_pvLargeAlloc = pT );
        pT->AddCompounds( &piostatsOut->m_piostats_->m_histoLatencies );
    }
    else
    {
        //  Windows ends up here.
        CIoStatsSmall_ * pT = new CIoStatsSmall_();
        Alloc( piostatsOut->m_pvSmallAlloc = pT );
        pT->AddCompounds( &piostatsOut->m_piostats_->m_histoLatencies );
    }

    piostatsOut->Tare();

    *ppiostatsOut = piostatsOut;
    piostatsOut = NULL;

HandleError:

    delete piostatsOut;

    return err;
}

CIoStats::CIoStats() :
#ifdef TEST_DOUBLE_CHECK_AGAINST_PERFECT_HISTO
    m_rwl( CLockBasicInfo( CSyncBasicInfo( "IO Stats Tracker" ), rankIoStats + 10, 0 ) )
#else
    m_rwl( CLockBasicInfo( CSyncBasicInfo( "IO Stats Tracker" ), rankIoStats, 0 ) )
#endif
{
    m_piostats_ = NULL;
    m_pvLargeAlloc = NULL;
    m_pvSmallAlloc = NULL;
    m_tickLastFastDataUpdate = 0;
    m_tickSpikeBaselineCompletedTime = 0;
}

CIoStats::~CIoStats()
{
    delete m_piostats_;
    delete ((CIoStatsLarge_*)m_pvLargeAlloc);
    delete ((CIoStatsSmall_*)m_pvSmallAlloc);
}

void CIoStats::AddIoSample_( const QWORD cusecIoTime  )
{
    m_rwl.EnterAsReader();

    CallS( ErrStatsErr( m_piostats_->m_histoLatencies.ErrAddSample( (SAMPLE)cusecIoTime ) ) );
#ifdef TEST_DOUBLE_CHECK_AGAINST_PERFECT_HISTO
    if ( m_piostats_->m_phistoTrue )
        m_piostats_->m_phistoTrue->ErrAddSample( cusecIoTime );
#endif

    m_rwl.LeaveAsReader();
}

void CIoStats::AddIoSample( HRT dhrtIoTime )
{
    AddIoSample_( CusecHRTFromDhrt( dhrtIoTime ) );
}

BOOL CIoStats::FStartUpdate_( const TICK tickNow )
{
    if ( DtickDelta( m_tickLastFastDataUpdate, tickNow ) < dtickSampleUpdateRate )
    {
        // fast out, not time yet ...
        return fFalse;
    }
    m_rwl.EnterAsWriter();
    if ( DtickDelta( m_tickLastFastDataUpdate, tickNow ) < dtickSampleUpdateRate )
    {
        // we lost the race to get to do the update, bail ...
        m_rwl.LeaveAsWriter();
        return fFalse;
    }
    m_piostats_->ResetCurrency();
    return fTrue;
}

BOOL CIoStats::FStartUpdate()
{
    return FStartUpdate_( TickOSTimeCurrent() );
}

QWORD CIoStats::CioAccumulated() const
{
    Assert( m_rwl.FWriter() );
    return m_piostats_->m_histoLatencies.C();
}

QWORD CIoStats::CusecAverage() const
{
    Assert( m_rwl.FWriter() );
    return m_piostats_->m_histoLatencies.Ave();
}

QWORD CIoStats::CusecPercentile( const ULONG pct ) const
{
    Assert( m_rwl.FWriter() );
    return m_piostats_->CusecPercentile( pct );
}

void CIoStats::FinishUpdate( const DWORD dwDiskNumber )
{
    if ( CioAccumulated() >= CIoStats::cioMinAccumSampleRate )
    {
        m_piostats_->ZeroLatencies();
    }

    if ( TickCmp( m_tickSpikeBaselineCompletedTime, TickOSTimeCurrent() ) < 0 )
    {
        if ( m_piostats_->m_cusecWorstP99 > cusecMinIoLatencySpikeToReport )
        {
            //  Some day if we get smarter, we can give an exact length of time for the spike length,
            //  but in the meantime we know it at least happened in the last sample time.
            ETIOLatencySpikeNotice( dwDiskNumber, dtickSampleUpdateRate );

            m_tickSpikeBaselineCompletedTime = TickOSTimeCurrent() + dtickSubsequentDeferralToLatencySpikeBaseline;
        }
    }

    m_tickLastFastDataUpdate = TickOSTimeCurrent();

    m_rwl.LeaveAsWriter();
}

void CIoStats::Tare()
{
    m_rwl.EnterAsWriter();
    m_piostats_->ZeroLatencies();
    m_tickLastFastDataUpdate = TickOSTimeCurrent();
    m_tickSpikeBaselineCompletedTime = m_tickLastFastDataUpdate + dtickInitialDeferralToLatencySpikeBaseline; // initially allow 24 hours
    m_rwl.LeaveAsWriter();
}

