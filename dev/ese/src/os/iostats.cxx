// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"


const static LONG dtickSampleUpdateRate     = OnDebugOrRetail( 5 * 1000, 5 * 60 * 1000 );

const static INT dtickInitialDeferralToLatencySpikeBaseline = OnDebugOrRetail( 6 * 1000, 24 * 60 * 60 * 1000 );
const static INT dtickSubsequentDeferralToLatencySpikeBaseline  = OnDebugOrRetail( 3 * 1000, 12 * 60 * 60 * 1000 );
const static QWORD cusecMinIoLatencySpikeToReport           = 100 * 1000;



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
            Assert( qwMin != qwMax );

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
            SAMPLE qwPctSample;
            CStats::ERR errStats = m_histoLatencies.ErrGetPercentileHits( &pctT, &qwPctSample );
            Assert( errStats == CStats::ERR::errSuccess );

#ifdef TEST_DOUBLE_CHECK_AGAINST_PERFECT_HISTO
            SAMPLE qwPctTrue = 42;
            ULONG pctIn = pct;
            if ( m_phistoTrue )
                m_phistoTrue->ErrGetPercentileHits( &pctIn, &qwPctTrue );
            wprintf( L"  %d%% = %I64u == %I64u\n", pct, qwPctSample, qwPctTrue );
#endif

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

    const static INT usHighRes =      50;  const static INT usHighResMax =     2500;
    const static INT usMedmRes =    2500;  const static INT usMedmResMax =   100000;
    const static INT usPokyRes =   10000;  const static INT usPokyResMax =   500000;
    const static INT usSlowRes =   50000;  const static INT usSlowResMax =  2500000;
    const static INT usHungRes =  500000;  const static INT usHungResMax = 20000000;

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
        new( rgbHighResSsd )  CFixedLinearHistogram( sizeof( rgbHighResSsd ),             0, usHighRes, cHighResBuckets, flhfFullRoundUp | flhfHardMin | flhfHardMax );
        new( rgbMedResHd )    CFixedLinearHistogram( sizeof( rgbMedResHd ),    usHighResMax, usMedmRes, cMedmResBuckets, flhfFullRoundUp | flhfHardMin | flhfHardMax );
        new( rgbPokyResHd )   CFixedLinearHistogram( sizeof( rgbPokyResHd ),   usMedmResMax, usPokyRes, cPokyResBuckets, flhfFullRoundUp | flhfHardMin | flhfHardMax );
        new( rgbLowResSlow )  CFixedLinearHistogram( sizeof( rgbLowResSlow ),  usPokyResMax, usSlowRes, cSlowResBuckets, flhfFullRoundUp | flhfHardMin | flhfHardMax );
        new( rgbSlowResHung ) CFixedLinearHistogram( sizeof( rgbSlowResHung ), usSlowResMax, usHungRes, cHungResBuckets, flhfFullRoundUp | flhfHardMin |   flhfSoftMax );
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
    const static INT usRes1 =       1000;  const static INT usRes1Max =     1000;
    const static INT usRes2 =      10000;  const static INT usRes2Max =    50000;
    const static INT usRes3 =     100000;  const static INT usRes3Max =   300000;
    const static INT usRes4 =    1000000;  const static INT usRes4Max =  3000000;
    const static INT usRes5 =   15000000;  const static INT usRes5Max = 60000000;

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
        new( rgbHisto1 ) CFixedLinearHistogram( sizeof( rgbHisto1 ),         0, usRes1, cBuckets1, flhfFullRoundUp | flhfHardMin | flhfHardMax );
        new( rgbHisto2 ) CFixedLinearHistogram( sizeof( rgbHisto2 ), usRes1Max, usRes2, cBuckets2, flhfFullRoundUp | flhfHardMin | flhfHardMax );
        new( rgbHisto3 ) CFixedLinearHistogram( sizeof( rgbHisto3 ), usRes2Max, usRes3, cBuckets3, flhfFullRoundUp | flhfHardMin | flhfHardMax );
        new( rgbHisto4 ) CFixedLinearHistogram( sizeof( rgbHisto4 ), usRes3Max, usRes4, cBuckets4, flhfFullRoundUp | flhfHardMin | flhfHardMax );
        new( rgbHisto5 ) CFixedLinearHistogram( sizeof( rgbHisto5 ), usRes4Max, usRes5, cBuckets5, flhfFullRoundUp | flhfHardMin |   flhfSoftMax );
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
#ifdef TEST_DOUBLE_CHECK_AGAINST_PERFECT_HISTO
    delete piostatsOut->m_piostats_->m_phistoTrue;
    piostatsOut->m_piostats_->m_phistoTrue = NULL;
#endif
#endif

    if ( fDatacenterGranularStats )
    {
        CIoStatsLarge_ * pT = new CIoStatsLarge_();
        Alloc( piostatsOut->m_pvLargeAlloc = pT );
        pT->AddCompounds( &piostatsOut->m_piostats_->m_histoLatencies );
    }
    else
    {
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
        return fFalse;
    }
    m_rwl.EnterAsWriter();
    if ( DtickDelta( m_tickLastFastDataUpdate, tickNow ) < dtickSampleUpdateRate )
    {
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
    m_tickSpikeBaselineCompletedTime = m_tickLastFastDataUpdate + dtickInitialDeferralToLatencySpikeBaseline;
    m_rwl.LeaveAsWriter();
}

