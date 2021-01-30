// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _STAT_HXX_INCLUDED
#define _STAT_HXX_INCLUDED




#ifdef STATAssert
#else
#define STATAssert Assert
#endif
#ifdef STATAssertSz
#else
#define STATAssertSz AssertSz
#endif

#ifndef INLINE
#define INLINE inline
#endif

#include "sync.hxx"

#define roundup( val, align ) ( ( ( ( val ) + ( align ) - 1 ) / ( align ) ) * ( align ) )



inline ULONG UlFunctionalMin( ULONG ulVal1, ULONG ulVal2 )
{
    return ulVal1 < ulVal2 ? ulVal1 : ulVal2;
}
inline ULONG UlFunctionalMax( ULONG ulVal1, ULONG ulVal2 )
{
    return ulVal1 > ulVal2 ? ulVal1 : ulVal2;
}

inline LONG LFunctionalMin( LONG lVal1, LONG lVal2 )
{
    return lVal1 < lVal2 ? lVal1 : lVal2;
}
inline LONG LFunctionalMax( LONG lVal1, LONG lVal2 )
{
    return lVal1 > lVal2 ? lVal1 : lVal2;
}


#define UlBound( ulIn, ulLow, ulHigh )          ( UlFunctionalMax( UlFunctionalMin( ulIn, ulHigh ), ulLow ) )

#define LBound( lIn, lLow, lHigh )              ( LFunctionalMax( LFunctionalMin( lIn, lHigh ), lLow ) )

inline ULONG_PTR UlpFunctionalMin( ULONG_PTR ulpVal1, ULONG_PTR ulpVal2 )
{
    return ulpVal1 < ulpVal2 ? ulpVal1 : ulpVal2;
}
inline ULONG_PTR UlpFunctionalMax( ULONG_PTR ulpVal1, ULONG_PTR ulpVal2 )
{
    return ulpVal1 > ulpVal2 ? ulpVal1 : ulpVal2;
}

#define UlpBound( ulpIn, ulpLow, ulpHigh )          ( UlpFunctionalMax( UlpFunctionalMin( ulpIn, ulpHigh ), ulpLow ) )

inline LONG_PTR LpFunctionalMin( LONG_PTR lpVal1, LONG_PTR lpVal2 )
{
    return lpVal1 < lpVal2 ? lpVal1 : lpVal2;
}
inline LONG_PTR LpFunctionalMax( LONG_PTR lpVal1, LONG_PTR lpVal2 )
{
    return lpVal1 > lpVal2 ? lpVal1 : lpVal2;
}

#define LpBound( lpIn, lpLow, lpHigh )              ( LpFunctionalMax( LpFunctionalMin( lpIn, lpHigh ), lpLow ) )



namespace STAT {



typedef  QWORD SAMPLE;
typedef  __int64 CHITS;

enum HistogramCurrencyResetFlags
{
    hgcrfResetSampleEnumeration = 0x1,
    hgcrfResetHitsEnumeration   = 0x2,

    hgcrfDefault                = ( hgcrfResetSampleEnumeration | hgcrfResetHitsEnumeration ),
};




class CStats {

public:


    enum class ERR
    {
        errSuccess = 0,
        errInvalidParameter,
        errOutOfMemory,
        wrnOutOfSamples,
        errFuncNotSupported,
    };


    virtual ~CStats() {}


    virtual void Zero( void ) = 0;

    virtual ERR ErrAddSample( _In_ const SAMPLE qwSample ) = 0;



    virtual ERR ErrReset( const HistogramCurrencyResetFlags hgcrf = hgcrfDefault ) = 0;

    virtual ERR ErrGetSampleValues( __out SAMPLE * pSamples ) = 0;

    virtual ERR ErrGetSampleHits( __in const SAMPLE qwSample, __out CHITS * pcHits ) = 0;

    virtual ERR ErrGetPercentileHits( __inout ULONG * pPercentile, __out SAMPLE * pSample ) = 0;

    virtual CHITS C() const = 0;
    virtual SAMPLE Min() const = 0;
    virtual SAMPLE Max() const = 0;
    virtual SAMPLE Total() const = 0;
    virtual SAMPLE Ave() const = 0;
    virtual double DblAve( void ) const = 0;


};


#define CStatsFromPv( pv )      ((CStats*)(pv))


class CMinMaxTotStats : public CStats {

protected:
    SAMPLE  qwMin;
    SAMPLE  qwMax;
    SAMPLE  qwTotal;
    CHITS   c;

public:

#ifdef DEBUG
    void AssertValid ( void ) const
    {
        if ( c )
        {
            STATAssert( qwMin <= qwMax );
            if ( qwMax < qwTotal )
            {
                STATAssert( c > 1 );
            }
        }
        else
        {
            STATAssert( qwMin == 0xFFFFFFFFFFFFFFFF );
            STATAssert( qwMax == 0 );
            STATAssert( qwTotal == 0 );
        }
    }
#else
    void AssertValid( void ) const { ; }
#endif

public:
    void Zero( void )
    {
        c = 0;
        qwMin = 0xFFFFFFFFFFFFFFFF;
        qwMax = 0;
        qwTotal = 0;
    }
    CMinMaxTotStats()
    {
        Zero();
        AssertValid();
    }
    CStats::ERR ErrAddSample( _In_ const SAMPLE qwSample )
    {
        if ( qwSample < qwMin )
        {
            qwMin = qwSample;
        }
        if ( qwSample > qwMax )
        {
            qwMax = qwSample;
        }
        qwTotal += qwSample;
        c++;
        AssertValid();
        return ERR::errSuccess;
    }

    CHITS C() const         { AssertValid(); return c; }
    SAMPLE Min() const      { AssertValid(); return qwMin;  }
    SAMPLE Max() const      { AssertValid(); return qwMax; }
    SAMPLE Total() const    { AssertValid(); return qwTotal; }

    SAMPLE Ave() const
    {
        AssertValid();
        const CHITS cT = c;
        return cT ? ( qwTotal / cT ) : 0;
    }

    double  DblAve() const
    {
        AssertValid();
        const CHITS cT = c;
        return cT ? ( (double)qwTotal / (double)cT ) : (double)0.0;
    }

    CStats::ERR ErrReset( const HistogramCurrencyResetFlags hgcrf = hgcrfDefault )
    {
        return CStats::ERR::errFuncNotSupported;
    }
    CStats::ERR ErrGetSampleValues( __out SAMPLE * pSamples )
    {
        return CStats::ERR::errFuncNotSupported;
    }
    CStats::ERR ErrGetSampleHits( __in const SAMPLE qwSample, __out CHITS * pcHits )
    {
        return CStats::ERR::errFuncNotSupported;
    }
    CStats::ERR ErrGetPercentileHits( __inout ULONG * pPercentile, __out SAMPLE * pSample )
    {
        return CStats::ERR::errFuncNotSupported;
    }

};

class CReloadableAveStats : public CMinMaxTotStats {
public:
    void Init(CHITS cInitial, SAMPLE qwInitial)
    {
        Zero();
        AssertValid();

        if ( cInitial > 0 )
            {
            qwTotal = qwInitial;
            c = cInitial;

            qwMin = 0;
            qwMax = qwInitial;
            }
        else
            {
            STATAssert( qwInitial == 0 );
            }
    }

    SAMPLE Min() const { AssertValid(); return 0; }
    SAMPLE Max() const { AssertValid(); return 0; }
};

class CAtomicMinMaxTotStats : public CMinMaxTotStats
{
public:
    CStats::ERR ErrAddSample( _In_ const SAMPLE qwSample )
    {
        (void)AtomicExchangeMin( &qwMin, qwSample );
        (void)AtomicExchangeMax( &qwMax, qwSample );
        (void)AtomicAdd( &qwTotal, qwSample );
        (void)AtomicAdd( (QWORD*)&c, (QWORD)1 );
        return ERR::errSuccess;
    }
};


enum PerfectHistogramStatsFlags
{
    phsfReadSyncExternalControlled = 1,
};

class CPerfectHistogramStats : public CStats {

private:

    CMinMaxTotStats         cmmts;
    SAMPLE *                m_pValues;
    CHITS                   m_cValues;
    CHITS *                 m_pcValues;
    CReaderWriterLock       m_rwlGrowing;

    BOOL FValidIndex( __int64 i ) { return i >= 0 && i < m_cValues; }

    BOOL FFindSampleBucket( __in const SAMPLE qwSample, __out ULONG * piVal )
    {
        STATAssert( m_rwlGrowing.FReader() || m_rwlGrowing.FWriter() );
        STATAssert( piVal );

        if ( m_cValues == 0 )
        {
            STATAssert( m_pValues == NULL );
            STATAssert( m_pcValues == NULL );
            *piVal = 0;
            return fFalse;
        }

        ULONG iVal;
        ULONG fExactMatch = fFalse;
        for ( iVal = 0; iVal < m_cValues; iVal++ )
        {
            if( qwSample <= m_pValues[iVal] )
            {
                fExactMatch = ( qwSample == m_pValues[iVal] );
                break;
            }
        }

        STATAssert( iVal != m_cValues || !fExactMatch );

        *piVal = iVal;
        return fExactMatch;
    }

public:
    
#ifdef DEBUG
    void AssertValid ( void ) const
    {
        if ( m_cValues )
        {
            STATAssert( m_pValues && m_pcValues );
        }
        if ( m_pValues || m_pcValues )
        {
            STATAssert( m_cValues );
        }
        cmmts.AssertValid();
    }
#else
    void AssertValid ( void ) const { ; }
#endif

public:


    void Zero( void )
    {
        ErrReset();
        m_rwlGrowing.EnterAsWriter();
        cmmts.Zero();
        m_iValueLastQuery = 0;
        m_iSampleValueNext = 0;
        m_iValueLastPercentile = 0;
        m_cValueLastPercentile = 0;
        if( m_pValues )
        {
            free( m_pValues );
        }
        m_pValues = NULL;
        m_cValues = 0;
        if( m_pcValues )
        {
            free( m_pcValues );
        }
        m_pcValues = NULL;
        AssertValid();
        m_rwlGrowing.LeaveAsWriter();
    }

    CPerfectHistogramStats() :
        m_rwlGrowing( CLockBasicInfo( CSyncBasicInfo( "CPerfectHistogramStats" ), 10 , 0 ) )
    {
        m_pValues = NULL;
        m_pcValues = NULL;
        m_fInReadValues = fFalse;
        Zero();
        AssertValid();
        STATAssert( cmmts.C() == 0 );
    }

    CPerfectHistogramStats( PerfectHistogramStatsFlags pfhs ) :
        m_rwlGrowing( CLockBasicInfo( CSyncBasicInfo( "CPerfectHistogramStats" ), 10 , 0 ) )
    {
        m_pValues = NULL;
        m_pcValues = NULL;
        m_fInReadValues = fFalse;
        Zero();
        AssertValid();
        m_fDisableEnumerationLocking = ( pfhs & phsfReadSyncExternalControlled );
    }

    ~CPerfectHistogramStats()
    {
        AssertValid();
        ErrReset();
        AssertValid();
        Zero();
        AssertValid();
    }

    CStats::ERR ErrAddSample( _In_ const SAMPLE qwSample )
    {
        CStats::ERR err = ERR::errSuccess;

        m_rwlGrowing.EnterAsReader();
        AssertValid();

        err = cmmts.ErrAddSample( qwSample );

        if ( err != ERR::errSuccess )
        {
            m_rwlGrowing.LeaveAsReader();
            AssertValid();
            return err;
        }

        ULONG iVal;
        ULONG fExactMatch = FFindSampleBucket( qwSample, &iVal );

        if ( !fExactMatch )
        {
            STATAssert( m_cValues < 10000000 );

            m_rwlGrowing.LeaveAsReader();
            m_rwlGrowing.EnterAsWriter();

            if ( FFindSampleBucket( qwSample, &iVal ) )
            {
                AssertValid();
                m_rwlGrowing.LeaveAsWriter();
                return ErrAddSample( qwSample );
            }

            SAMPLE * pNewValues = (SAMPLE*) malloc( (ULONG)( m_cValues + 1 ) * sizeof( SAMPLE ) );
            CHITS * pcNewCounts = (CHITS*) malloc( (ULONG)( m_cValues + 1 ) * sizeof( CHITS ) );
            if ( pNewValues == NULL ||
                pcNewCounts == NULL )
            {
                AssertValid();
                m_rwlGrowing.LeaveAsWriter();
                goto HandleError;
            }
            if( iVal != 0 )
            {
                memcpy( pNewValues, m_pValues, (ULONG)iVal * sizeof( SAMPLE ) );
                memcpy( pcNewCounts, m_pcValues, (ULONG)iVal * sizeof( CHITS ) );
            }
            if( iVal != m_cValues )
            {
                memcpy( &(pNewValues[iVal+1]), &(m_pValues[iVal]), (ULONG)( m_cValues - iVal ) * sizeof( SAMPLE ) );
                memcpy( &(pcNewCounts[iVal+1]), &(m_pcValues[iVal]), (ULONG)( m_cValues - iVal ) * sizeof( CHITS ) );
            }
            pNewValues[iVal] = qwSample;
            pcNewCounts[iVal] = 1;
            free( m_pValues );
            m_pValues = pNewValues;
            free( m_pcValues );
            m_pcValues = pcNewCounts;
            m_cValues++;

            AssertValid();
            m_rwlGrowing.LeaveAsWriter();
        }
        else
        {
            STATAssert( FValidIndex(iVal) );
            m_pcValues[iVal]++;
            AssertValid();
            m_rwlGrowing.LeaveAsReader();
        }

        return ERR::errSuccess;
    HandleError:
    
        return ERR::errOutOfMemory;
    }

private:

    BOOL    m_fDisableEnumerationLocking;
    BOOL    m_fInReadValues;
    ULONG   m_iSampleValueNext;
    ULONG   m_iValueLastQuery;
    ULONG   m_iValueLastPercentile;
    CHITS   m_cValueLastPercentile;

public:

    
    CStats::ERR ErrReset( const HistogramCurrencyResetFlags hgcrf = hgcrfDefault )
    {
        STATAssert( ( hgcrf & ( hgcrfResetSampleEnumeration | hgcrfResetHitsEnumeration ) ) != 0 );

        if ( hgcrf & hgcrfResetSampleEnumeration )
        {
            m_iSampleValueNext = 0;
        }

        if ( hgcrf & hgcrfResetHitsEnumeration )
        {
            m_iValueLastQuery = 0;
            m_iValueLastPercentile = 0;
            m_cValueLastPercentile = 0;
        }

        if ( m_fInReadValues )
        {
            STATAssert( m_rwlGrowing.FReader() || m_fDisableEnumerationLocking );
            AssertValid();
            if ( !m_fDisableEnumerationLocking )
            {
                m_rwlGrowing.LeaveAsReader();
            }
            m_fInReadValues = fFalse;
        }
        STATAssert( m_rwlGrowing.FNotReader() );
        return ERR::errSuccess;
    }

    CStats::ERR ErrGetSampleValues( __out SAMPLE * pSamples )
    {
        if ( !m_fInReadValues )
        {
            if ( !m_fDisableEnumerationLocking )
            {
                m_rwlGrowing.EnterAsReader();
            }
            m_fInReadValues = fTrue;
        }
        STATAssert( m_rwlGrowing.FReader() || m_fDisableEnumerationLocking );
        AssertValid();

        if ( pSamples == NULL )
        {
            return ERR::errInvalidParameter;
        }

        if ( m_iSampleValueNext >= m_cValues )
        {
            return ERR::wrnOutOfSamples;
        }

        *pSamples = m_pValues[m_iSampleValueNext];

        m_iSampleValueNext++;

        return CStats::ERR::errSuccess;
    }

    CStats::ERR ErrGetSampleHits( __in const SAMPLE qwValue, __out CHITS * pcHits )
    {
        CHITS cHits = 0;

        if ( !m_fInReadValues )
        {
            if ( !m_fDisableEnumerationLocking )
            {
                m_rwlGrowing.EnterAsReader();
            }
            m_fInReadValues = fTrue;
        }
        STATAssert( m_rwlGrowing.FReader() || m_fDisableEnumerationLocking );
        AssertValid();

        if ( pcHits == NULL )
        {
            return ERR::errInvalidParameter;
        }

        *pcHits = 0;

        if ( m_iValueLastQuery >= m_cValues )
        {
            return ERR::wrnOutOfSamples;
        }
        else if ( m_iValueLastQuery > 1 &&
                    qwValue < m_pValues[m_iValueLastQuery-1] )
        {
            return ERR::errInvalidParameter;
        }

        for( ; m_iValueLastQuery < m_cValues; m_iValueLastQuery++ )
        {
            if ( qwValue < m_pValues[m_iValueLastQuery] )
            {
                break;
            }
            cHits += m_pcValues[m_iValueLastQuery];
        }
        
        *pcHits = cHits;
        return ERR::errSuccess;
    }

    CStats::ERR ErrGetPercentileHits( __inout ULONG * pPercentile, __out SAMPLE * pSample )
    {
        CHITS cHits;
        CHITS cTotHits;
        
        if ( !m_fInReadValues )
        {
            if ( !m_fDisableEnumerationLocking )
            {
                m_rwlGrowing.EnterAsReader();
            }
            m_fInReadValues = fTrue;
        }
        STATAssert( m_rwlGrowing.FReader() || m_fDisableEnumerationLocking );

        if ( 0 == m_cValues )
        {
            return ERR::wrnOutOfSamples;
        }

        if ( pPercentile == NULL || pSample == NULL ||
                *pPercentile < 1 || *pPercentile > 100 )
        {
            return ERR::errInvalidParameter;
        }

        cTotHits = ( C() * *pPercentile ) / 100;

        if ( m_cValueLastPercentile == 0 )
        {
            m_cValueLastPercentile = m_pcValues[ 0 ];
        }
        cHits = m_cValueLastPercentile;
        if ( cTotHits != cHits )
        {
            if ( cTotHits < m_cValueLastPercentile )
            {
                while ( m_iValueLastPercentile > 0 )
                {
                    if ( ( cHits - m_pcValues[m_iValueLastPercentile] ) < cTotHits )
                    {
                        break;
                    }
                    cHits -= m_pcValues[m_iValueLastPercentile];
                    m_iValueLastPercentile--;
                }
            }
            else
            {
                m_iValueLastPercentile++;
                while ( m_iValueLastPercentile < m_cValues )
                {
                    cHits += m_pcValues[m_iValueLastPercentile];
                    if ( cHits >= cTotHits )
                    {
                        break;
                    }
                    m_iValueLastPercentile++;
                }
                
                if ( m_iValueLastPercentile == m_cValues )
                {
                    m_iValueLastPercentile--;
                }
            }
        }
        m_cValueLastPercentile = cHits;
        *pPercentile = static_cast<ULONG>( ( m_cValueLastPercentile * 100 ) / C() );
        *pSample = m_pValues[m_iValueLastPercentile];

        return ERR::errSuccess;
    }

    SAMPLE Mode()
    {
        if ( m_cValues == 0 )
        {
            return 0x4242424242424242;
        }
        if ( !m_fDisableEnumerationLocking )
        {
            m_rwlGrowing.EnterAsReader();
        }
        CHITS cHighCount = 0;
        ULONG iMode = 0;
        for( ULONG iVal = 0; iVal < m_cValues; iVal++ )
        {
            if ( cHighCount < m_pcValues[iVal] )
            {
                cHighCount = m_pcValues[iVal];
                iMode = iVal;
            }
        }

        STATAssert( FValidIndex( iMode ) );
        SAMPLE qwMode = m_pValues[iMode];
        if ( !m_fDisableEnumerationLocking )
        {
            m_rwlGrowing.LeaveAsReader();
        }
        return qwMode;
    }

    CHITS C() const         { AssertValid(); return cmmts.C(); }
    SAMPLE Min() const      { AssertValid(); return cmmts.Min(); }
    SAMPLE Max() const      { AssertValid(); return cmmts.Max(); }
    SAMPLE Total() const    { AssertValid(); return cmmts.Total(); }
    SAMPLE Ave() const      { AssertValid(); return cmmts.Ave(); }
    SAMPLE Mean() const     { AssertValid(); return Ave(); }
    double DblAve() const   { AssertValid(); return cmmts.DblAve(); }

};


class CLinearHistogramStats : public CStats {

private:

    CPerfectHistogramStats  m_stats;

    SAMPLE                  m_cSection;
    BOOL                    m_fRoundUp;

public:
    
#ifdef DEBUG
    void AssertValid ( void ) const
    {
        m_stats.AssertValid();
    }
#else
    void AssertValid ( void ) const { ; }
#endif

public:


    void Zero( void )
    {
        m_stats.Zero();
    }

    CLinearHistogramStats( ULONG cSection )
    {
        m_cSection = cSection;
        m_fRoundUp = fTrue;
        Zero();
        AssertValid();
        STATAssert( m_stats.C() == 0 );
    }
    ~CLinearHistogramStats()
    {
        AssertValid();
        ErrReset();
        AssertValid();
        Zero();
        AssertValid();
    }

    CStats::ERR ErrAddSample( const SAMPLE qwSample )
    {
        SAMPLE qwSampleNormalized;


        if ( 0 == qwSample )
        {
            qwSampleNormalized = 0;
        }
        else if ( 0 == ( qwSample % m_cSection ) )
        {
            qwSampleNormalized = qwSample;
        }
        else
        {
            qwSampleNormalized = ( ( m_fRoundUp ? 1 : 0 ) + qwSample / m_cSection ) * m_cSection;
        }


        return m_stats.ErrAddSample( qwSampleNormalized );
    }

public:

    
    CStats::ERR ErrReset( const HistogramCurrencyResetFlags hgcrf = hgcrfDefault )
    {
        return m_stats.ErrReset( hgcrf );
    }

    CStats::ERR ErrGetSampleValues( __out SAMPLE * pSamples )
    {
        return m_stats.ErrGetSampleValues( pSamples );
    }

    CStats::ERR ErrGetSampleHits( __in const SAMPLE qwValue, __out CHITS * pcHits )
    {
        return m_stats.ErrGetSampleHits( qwValue, pcHits );
    }

    CStats::ERR ErrGetPercentileHits( __inout ULONG * pPercentile, __out SAMPLE * pSample )
    {
        return m_stats.ErrGetPercentileHits( pPercentile, pSample );
    }

    SAMPLE Mode()
    {
        return m_stats.Mode();
    }

    CHITS C() const         { AssertValid(); return m_stats.C(); }
    SAMPLE Min() const      { AssertValid(); return m_stats.Min(); }
    SAMPLE Max() const      { AssertValid(); return m_stats.Max(); }
    SAMPLE Total() const    { AssertValid(); return m_stats.Total(); }
    SAMPLE Ave() const      { AssertValid(); return m_stats.Ave(); }
    SAMPLE Mean() const     { AssertValid(); return Ave(); }
    double DblAve() const   { AssertValid(); return m_stats.DblAve(); }

};



#define CbCSegmentedHistogram( rgSegments )     ( sizeof(CSegmentedHistogram) +                         \
                                                        sizeof(SAMPLE) +                                \
                                                        sizeof(rgSegments)  )

class CSegmentedHistogram : public CStats {

    private:
        CHITS                       m_cValues;
        SAMPLE *                    m_pValues;

        volatile CHITS * volatile PcHits_() const
        {
            CHITS * pcHits = (CHITS*) ( (BYTE*)this + sizeof(*this) );
            pcHits = (CHITS*)roundup( (LONG_PTR)pcHits, (LONG_PTR)sizeof(SAMPLE) );
            return pcHits;
        }

        BOOL FInit_() const
        {
            return m_cValues != 0;
        }

        BOOL FValidIndex_( __int64 i ) const
        {
            return i >= 0 && i < m_cValues;
        }

        BOOL FFindSampleBucket_( __in const SAMPLE qwSample, __out ULONG * piVal ) const
        {
            STATAssert( piVal );

            if ( m_cValues == 0 )
            {
                STATAssert( m_pValues == NULL );
                *piVal = 0;
                return fFalse;
            }

            ULONG iVal;
            for ( iVal = 0; iVal < m_cValues; iVal++ )
            {
                if( qwSample <= m_pValues[iVal] )
                {
                    break;
                }
            }

            *piVal = iVal;
            return iVal < m_cValues;
        }

#ifdef DEBUG
        void AssertValid ( void ) const
        {
            STATAssert( FInit_() );
            if ( m_cValues )
            {
                STATAssert( m_pValues && PcHits_() );
            }
            if ( m_pValues || PcHits_() )
            {
                STATAssert( m_cValues );
            }
        }
#else
        void AssertValid ( void ) const { ; }
#endif


    private:

        CSegmentedHistogram( );

    public:


        CSegmentedHistogram( SAMPLE * rgSampleSegments, ULONG cSampleSegments, void * pHitsData, ULONG cbHitsData ) :
            m_pValues ( NULL ),
            m_cValues ( 0 )
        {
            STATAssert( cSampleSegments );
            STATAssert( rgSampleSegments );
            STATAssert( cbHitsData );
            STATAssert( pHitsData );

            STATAssert( cSampleSegments * sizeof(CHITS) <= cbHitsData );

            const BOOL fValid = ( ( cbHitsData - sizeof(*this) ) % sizeof(SAMPLE) ) == 0 &&
                                ( ( cbHitsData - sizeof(*this) ) / sizeof(SAMPLE) ) == ( 1 + cSampleSegments );

            if ( fValid )
            {
                m_pValues = rgSampleSegments;
                m_cValues = cSampleSegments;
            }

            STATAssert( FInit_() );

            STATAssert( sizeof(*this) % 8 == 0 );
            STATAssert( (LONG_PTR)PcHits_() % 8 == 0 );
            STATAssert( ((BYTE*)(&PcHits_()[m_cValues-1])) + sizeof(SAMPLE) <= ((BYTE*)this) + cbHitsData );

            Zero();

            CStats::ERR errReset = ErrReset();
            STATAssert( ERR::errSuccess == errReset );
        }

        ~CSegmentedHistogram()
        {
        }

        void Zero( void )
        {
            STATAssert( FInit_() );
            if ( !FInit_() )
            {
                return;
            }

            for( ULONG iValues = 0; iValues < m_cValues; iValues++ )
            {
                AtomicExchange( &PcHits_()[iValues], 0 );
            }
            ErrReset();
        }

        CStats::ERR ErrAddSample( const SAMPLE qwSample )
        {
            STATAssert( FInit_() );

            if ( !FInit_() )
            {
                return ERR::errInvalidParameter;
            }

            ULONG iVal;
            if ( !FFindSampleBucket_( qwSample, &iVal ) )
            {
                STATAssertSz( fFalse, "The sample was outside any buckets, consider adding a 0xFFFFFFFFFFFFFFFF bucket." );
                return ERR::errInvalidParameter;
            }

            AtomicAdd( (QWORD*)&PcHits_()[iVal], (QWORD)1 );

            return ERR::errSuccess;
        }



    private:

        ULONG               m_iSampleValueNext;
        ULONG               m_iValueLastQuery;

    public:

        CStats::ERR ErrReset( const HistogramCurrencyResetFlags hgcrf = hgcrfDefault )
        {
            STATAssert( hgcrf == hgcrfDefault );
            m_iSampleValueNext = 0;
            m_iValueLastQuery = 0;
            return ERR::errSuccess;
        }

        CStats::ERR ErrGetSampleValues( __out SAMPLE * pSamples )
        {
            AssertValid();

            if ( pSamples == NULL )
            {
                return ERR::errInvalidParameter;
            }

            if ( m_iSampleValueNext >= m_cValues )
            {
                return ERR::wrnOutOfSamples;
            }

            *pSamples = m_pValues[m_iSampleValueNext];

            m_iSampleValueNext++;

            return ERR::errSuccess;
        }


        CStats::ERR ErrCaptureSampleValues( CSegmentedHistogram * phistoNew )
        {
            if ( !FInit_() || !phistoNew->FInit_() )
            {
                return ERR::errInvalidParameter;
            }

            if ( m_cValues != phistoNew->m_cValues )
            {
                return ERR::errInvalidParameter;
            }

            for( ULONG iVal = 0; iVal < m_cValues; iVal++ )
            {
                if ( m_pValues[iVal] != phistoNew->m_pValues[iVal] )
                {
                    return ERR::errInvalidParameter;
                }
            }

            for( ULONG iVal = 0; iVal < m_cValues; iVal++ )
            {
                phistoNew->PcHits_()[iVal] = AtomicExchange( &(PcHits_()[iVal]), 0 );
            }
            
            return ERR::errSuccess;
        }

        CStats::ERR ErrGetSampleHits( __in const SAMPLE qwValue, __out CHITS * pcHits )
        {
            CHITS cHits = 0;

            AssertValid();

            if ( pcHits == NULL )
            {
                return ERR::errInvalidParameter;
            }

            *pcHits = 0;

            if ( m_iValueLastQuery >= m_cValues )
            {
                return ERR::wrnOutOfSamples;
            }
            else if ( m_iValueLastQuery > 1 &&
                        qwValue < m_pValues[m_iValueLastQuery-1] )
            {
                return ERR::errInvalidParameter;
            }

            for( ; m_iValueLastQuery < m_cValues; m_iValueLastQuery++ )
            {
                if ( qwValue < m_pValues[m_iValueLastQuery] )
                {
                    break;
                }
                cHits += PcHits_()[m_iValueLastQuery];
            }

            if ( 0 == cHits && m_iValueLastQuery == m_cValues )
            {
                return ERR::wrnOutOfSamples;
            }
            
            *pcHits = cHits;
            return ERR::errSuccess;
        }

        CStats::ERR ErrGetPercentileHits( __inout ULONG * pPercentile, __out SAMPLE * pSample )
        {
            return ERR::errInvalidParameter;
        }

        SAMPLE Mode()
        {
            STATAssert( FInit_() );

            STATAssert( m_cValues );
            if ( m_cValues == 0 )
            {
                return 0x4242424242424242;
            }

            CHITS cHighCount = 0;
            ULONG iMode = 0;
            for( ULONG iVal = 0; iVal < m_cValues; iVal++ )
            {
                if ( cHighCount < PcHits_()[iVal] )
                {
                    cHighCount = PcHits_()[iVal];
                    iMode = iVal;
                }
            }

            STATAssert( FValidIndex_( iMode ) );
            SAMPLE qwMode = m_pValues[iMode];

            return qwMode;
        }


        CHITS C() const
        {
            AssertValid();
            CHITS cTotal = 0;
            for( ULONG iVal = 0; iVal < m_cValues; iVal++ )
            {
                cTotal += PcHits_()[iVal];
            }
            return cTotal;
        }


        SAMPLE Min() const
        {
            AssertValid();
            for( ULONG iVal = 0; iVal < m_cValues; iVal++ )
            {
                if ( PcHits_()[iVal] != 0 )
                {
                    return m_pValues[iVal];
                }
            }
            return 0xFFFFFFFFFFFFFFFF;
        }
        SAMPLE Max() const
        {
            AssertValid();
            for( INT iVal = (ULONG)m_cValues - 1; iVal >= 0; iVal-- )
            {
                if ( PcHits_()[iVal] != 0 )
                {
                    return m_pValues[iVal];
                }
            }
            return 0;
        }
        SAMPLE Total() const
        {
            AssertValid();
            SAMPLE cTotal = 0;
            for( ULONG iVal = 0; iVal < m_cValues; iVal++ )
            {
                cTotal += PcHits_()[iVal] * m_pValues[iVal];
            }
            return cTotal;
        }
        SAMPLE Ave() const
        {
            AssertValid();
            const CHITS c = C();
            const SAMPLE t = Total();
            return c ? ( t / c ) : 0;
        }
        SAMPLE Mean() const
        {
            AssertValid();
            return Ave();
        }
        double DblAve() const
        {
            AssertValid();
            const double c = (double)C();
            const double t = (double)Total();
            return c ? ( t / c ) : 0.0;
        }


};


#define CbCFixedLinearHistogram( cSection )     ( sizeof( CFixedLinearHistogram ) +                 \
                                                        ( sizeof( CHITS ) * cSection ) )

#define STAT_DEFINE_ENUM_FLAG_OPERATORS_BASIC(ENUMTYPE) \
    extern "C++" {                                  \
        inline ENUMTYPE operator | (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((INT)a) | ((INT)b)); }               \
        inline ENUMTYPE operator & (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((INT)a) & ((INT)b)); }               \
        inline ENUMTYPE operator ~ (ENUMTYPE a) { return ENUMTYPE(~((INT)a)); }                                     \
    }

enum FixedLinearHistoFlags
{
    flhfNone            = 0x0,

    flhfFullRoundUp     = 0x1,
    flhfFullRoundDown   = 0x2,

    flhfSoftMax         = 0x10,
    flhfHardMax         = 0x20,
    flhfHardMin         = 0x40,
    flhfSoftMin         = 0x40,
};
STAT_DEFINE_ENUM_FLAG_OPERATORS_BASIC( FixedLinearHistoFlags );


class CFixedLinearHistogram : public CStats
{
private:

    SAMPLE                  m_qwSampleBase;
    SAMPLE                  m_dqwSamplesInSection;
    ULONG                   m_cSection;
    FixedLinearHistoFlags   m_flhf;
    
private:

    CFixedLinearHistogram()
    {
    }

private:
    
#ifdef DEBUG
    void AssertValid ( void ) const
    {
        STATAssert( m_iSampleValueNext <= m_cSection || m_iSampleValueNext == ulMax  );
        STATAssert( m_iSampleHitsNext <= m_cSection );
    }
#else
    void AssertValid ( void ) const { ; }
#endif

private:
    SAMPLE QwSampleFromSection_( ULONG iSection ) const
    {
        return m_qwSampleBase + iSection * m_dqwSamplesInSection;
    }


public:


    void Zero( void )
    {
        for( ULONG isection = 0; isection < m_cSection; isection++ )
        {
            AtomicExchange( (volatile __int64 * const)&m_rgcHits[isection], 0 );
        }
    }

    CFixedLinearHistogram( size_t cbSizeGiven, SAMPLE qwSampleBase, SAMPLE dqwSamplesInSection, ULONG cSection, const FixedLinearHistoFlags flhf )
    {
        STATAssert( ( ( cbSizeGiven - sizeof(CFixedLinearHistogram) ) / sizeof(SAMPLE) ) >= cSection );
        STATAssert( cSection < lMax );

        m_flhf = flhf;
        m_qwSampleBase = qwSampleBase;
        m_dqwSamplesInSection = dqwSamplesInSection;
        m_cSection = cSection;
        m_rgcHits = (CHITS*)roundup( ULONG_PTR((BYTE*)this) + sizeof( CFixedLinearHistogram ), sizeof( CHITS ) );

#ifdef DEBUG
        ULONG_PTR pbObjMax = ((ULONG_PTR)this) + cbSizeGiven + 1;
        ULONG_PTR pbLastSlot = (ULONG_PTR)&m_rgcHits[ m_cSection - 1 ];
        STATAssert( ( pbLastSlot + sizeof(CHITS) ) <= pbObjMax );
#endif
        STATAssert( ( (ULONG_PTR)m_rgcHits % sizeof( CHITS ) ) == 0 );

        ErrReset();
        Zero();

        AssertValid();
    }

    ~CFixedLinearHistogram()
    {
        AssertValid();
    }

    CStats::ERR ErrGetSampleSection_( _In_ const SAMPLE qwSample, _In_ const FixedLinearHistoFlags flhf, _Out_ ULONG * piSection )
    {
        SAMPLE qwSampleAdjusted = qwSample;

        if ( qwSampleAdjusted < m_qwSampleBase )
        {
            if ( !( flhf & flhfSoftMin ) )
            {
                STATAssert( !( flhf & flhfHardMin ) );
                return CStats::ERR::errInvalidParameter;
            }
            qwSampleAdjusted = m_qwSampleBase;
        }

        qwSampleAdjusted = qwSampleAdjusted - m_qwSampleBase;

        if ( flhf & flhfFullRoundUp )
        {
            SAMPLE qwSampleRounded = roundup( qwSampleAdjusted, m_dqwSamplesInSection );
            if ( qwSampleRounded >  qwSampleAdjusted )
            {
                qwSampleAdjusted = qwSampleRounded;
            }
        }
        else if ( !( flhf & flhfFullRoundDown ) )
        {
            STATAssertSz( fFalse, "Not implemented proper rounding - where if < m_dqwSamplesInSection / 2 above bucket boundary it would round down, else round up." );
        }

        SAMPLE iSection = ULONG( qwSampleAdjusted / m_dqwSamplesInSection );

        if ( iSection >= (SAMPLE)m_cSection )
        {
            if ( !( flhf & flhfSoftMax ) )
            {
                STATAssert( !( flhf & flhfHardMax ) );
                return CStats::ERR::errInvalidParameter;
            }
            iSection = m_cSection - 1;
        }
        STATAssert( iSection < (SAMPLE)ulMax );
        if ( iSection >= (SAMPLE)ulMax )
        {
            STATAssertSz( flhf & flhfHardMax, "This structure only allows ULONG array slots - which is like 32 GBs of RAM, use something else." );
            return CStats::ERR::errInvalidParameter;
        }

        *piSection = (ULONG)iSection;
        return CStats::ERR::errSuccess;
    }

    CStats::ERR ErrAddSample( _In_ const SAMPLE qwSample )
    {


        ULONG iSection;
        CStats::ERR errNormalize = ErrGetSampleSection_( qwSample, m_flhf, &iSection );
        if ( errNormalize != CStats::ERR::errSuccess )
        {
            return errNormalize;
        }


        (void)AtomicAdd( (QWORD*)&m_rgcHits[iSection], (QWORD)1 );

        return CStats::ERR::errSuccess;
    }

private:
    ULONG   m_iSampleValueNext;
    ULONG   m_iSampleHitsNext;

public:

    
    CStats::ERR ErrReset( const HistogramCurrencyResetFlags hgcrf = hgcrfDefault )
    {
        if ( hgcrf & hgcrfResetSampleEnumeration )
        {
            m_iSampleValueNext = ulMax;
        }
        if ( hgcrf & hgcrfResetHitsEnumeration )
        {
            m_iSampleHitsNext = 0;
        }
        return CStats::ERR::errSuccess;
    }

    CStats::ERR ErrGetSampleValues( __out SAMPLE * psample )
    {
        m_iSampleValueNext++;


        while( m_iSampleValueNext < m_cSection && m_rgcHits[ m_iSampleValueNext ] == 0 )
        {
            m_iSampleValueNext++;
        }

        if ( m_iSampleValueNext >= m_cSection )
        {
            return CStats::ERR::wrnOutOfSamples;
        }

        *psample = QwSampleFromSection_( m_iSampleValueNext );

        return CStats::ERR::errSuccess;
    }

    CStats::ERR ErrGetSampleHits( __in const SAMPLE qwValue, __out CHITS * pcHits )
    {
        CHITS cHitsTotal = 0;

        *pcHits = 0;

        while( m_iSampleHitsNext < m_cSection && QwSampleFromSection_( m_iSampleHitsNext ) <= qwValue )
        {
            cHitsTotal += m_rgcHits[ m_iSampleHitsNext ];
            m_iSampleHitsNext++;
        }
    
        if ( cHitsTotal == 0 )
        {
            ULONG iNextHits = m_iSampleHitsNext;
            if ( iNextHits < m_cSection )
            {
                while( iNextHits < m_cSection )
                {
                    if ( m_rgcHits[ iNextHits ] )
                    {
                        break;
                    }
                    iNextHits++;
                }
            }
            if ( iNextHits >= m_cSection )
            {
                return CStats::ERR::wrnOutOfSamples;
            }
        }
        
        *pcHits = cHitsTotal;

        return CStats::ERR::errSuccess;
    }

    CStats::ERR ErrGetPercentileHits( __inout ULONG * pPercentile, __out SAMPLE * pSample )
    {
        STATAssertSz( fFalse, "Not yet implemented" );
        return CStats::ERR::errFuncNotSupported;
    }

    SAMPLE Mode()
    {
        return 0;
    }

    CHITS C() const
    {
        CHITS cHits = 0;
        for( ULONG isection = 0; isection < (INT)m_cSection; isection++ )
        {
            cHits += m_rgcHits[isection];
        }
        return cHits;
    }
    SAMPLE Min() const
    {
        for( ULONG isection = 0; isection < m_cSection; isection++ )
        {
            if ( m_rgcHits[isection] )
            {
                return QwSampleFromSection_( isection );
            }
        }
        return qwMax;
    }
    SAMPLE Max() const
    {
        for( LONG isection = (LONG)m_cSection - 1; isection >= 0; isection-- )
        {
            if ( m_rgcHits[isection] )
            {
                return QwSampleFromSection_( isection );
            }
        }
        return 0;
    }
    SAMPLE Total() const
    {
        SAMPLE qwTotal = 0;
        for( ULONG isection = 0; isection < m_cSection; isection++ )
        {
            if ( m_rgcHits[isection] )
            {
                STATAssert( ( qwTotal + ( m_rgcHits[isection] * QwSampleFromSection_( isection ) ) ) >= qwTotal );
                qwTotal += ( m_rgcHits[isection] * QwSampleFromSection_( isection ) );
            }
        }
        return qwTotal;
    }
    SAMPLE Ave() const
    {
        const SAMPLE cHitsTotal = C();
        return cHitsTotal ? ( Total() / cHitsTotal ) : 0;
    }
    SAMPLE Mean() const     { return Ave(); }
    double DblAve() const
    {
        const SAMPLE cHitsTotal = C();
        return C() ? ( (double)Total() / (double)cHitsTotal ) : 0.0;
    }

private:

    CHITS *                 m_rgcHits;

};

static_assert( ( sizeof( CFixedLinearHistogram ) % sizeof( SAMPLE ) ) == 0, "Basic CFixedLinearHistogram must be modulo 8 == 0 to ensure alignment of array at end." );

void PrintStats( CStats * pCS );

enum CompoundHistogramFlags
{
    chfInvalid = 0,
    chfDoNotFreeExternalMemory  = 0x1,
    chfDeleteOnDeleteYouOwn     = 0x2,
};


class CCompoundHistogram : public CStats {

    private:
        const static ULONG          cSegmentsMax = 5;
        ULONG                       m_cSegmentMac;
        CAtomicMinMaxTotStats       m_mmts;
        SAMPLE                      m_rgqwSampleSegmentMax[cSegmentsMax];
        CStats *                    m_rgphistoSegments[cSegmentsMax];

        BOOL FFindSampleHisto_( _In_ const SAMPLE qwSample, _Out_ CStats ** pphisto ) const
        {
            for ( ULONG iSeg  = 0; iSeg < m_cSegmentMac; iSeg++ )
            {
                if ( qwSample < m_rgqwSampleSegmentMax[iSeg] )
                {
                    *pphisto = m_rgphistoSegments[iSeg];
                    return fTrue;
                }
            }
            return fFalse;
        }

#ifdef DEBUG
        void AssertValid( const BOOL fAllowPastEndEnumState = fFalse ) const
        {
            STATAssert( m_cSegmentMac );
            STATAssert( m_cSegmentMac <= cSegmentsMax );
            if ( fAllowPastEndEnumState )
            {
                STATAssert( m_iSampleHistoSegmentCurr <= m_cSegmentMac );
                STATAssert( m_iHitsHistoSegmentStart <= m_cSegmentMac );
            }
            else
            {
                STATAssert( m_iSampleHistoSegmentCurr < m_cSegmentMac );
                STATAssert( m_iHitsHistoSegmentStart < m_cSegmentMac );
            }
            STATAssert( m_pctLast <= 100 );
            STATAssert( m_cPctHistAccumLast <= m_cHitsTotalPctStart );
        }
#else
        void AssertValid ( const BOOL fAllowPastEndEnumState = fFalse ) const { ; }
#endif


    public:

        CCompoundHistogram()
        {
            m_cSegmentMac = 0;
            memset( m_rgqwSampleSegmentMax, 0, sizeof( m_rgqwSampleSegmentMax ) );
            memset( m_rgphistoSegments, 0, sizeof( m_rgphistoSegments ) );
            ErrReset();
        }

        ERR ErrAddHistoRange( _In_ const SAMPLE qwSampleMax, _In_ CStats * phisto )
        {
            if ( m_cSegmentMac > 0 &&
                    m_rgqwSampleSegmentMax[ m_cSegmentMac - 1 ] >= qwSampleMax )
            {
                STATAssertSz( fFalse, "Must add histos in increasing sample value" );
                return ERR::errInvalidParameter;
            }
            if ( m_cSegmentMac >= _countof( m_rgqwSampleSegmentMax ) )
            {
                STATAssertSz( fFalse, "Either make the array of samples + histos variable or extend it a little." );
                return ERR::errInvalidParameter;
            }
            if ( phisto == NULL )
            {
                return ERR::errInvalidParameter;
            }

            m_rgqwSampleSegmentMax[ m_cSegmentMac ] = qwSampleMax;
            m_rgphistoSegments[ m_cSegmentMac ] = phisto;
            m_cSegmentMac++;

            AssertValid();
            return ERR::errSuccess;
        }

        ~CCompoundHistogram()
        {
        }

        void Zero( void )
        {
            for ( ULONG iSeg  = 0; iSeg < m_cSegmentMac; iSeg++ )
            {
                m_rgphistoSegments[ iSeg ]->Zero();
            }
            m_mmts.Zero();
            ErrReset();
        }

        CStats::ERR ErrAddSample( const SAMPLE qwSample )
        {
            CStats * phisto = NULL;
            if ( FFindSampleHisto_( qwSample, &phisto ) )
            {
                const CStats::ERR errStat = phisto->ErrAddSample( qwSample );
                if ( errStat >= CStats::ERR::errSuccess )
                {
                    m_mmts.ErrAddSample( qwSample );
                }
                return errStat;
            }
            
            return CStats::ERR::errInvalidParameter;
        }



    private:

        ULONG               m_iSampleHistoSegmentCurr;
        ULONG               m_iHitsHistoSegmentStart;

    public:

        CStats::ERR ErrReset( const HistogramCurrencyResetFlags hgcrf = hgcrfDefault )
        {
            if ( hgcrf & hgcrfResetSampleEnumeration )
            {
                m_iSampleHistoSegmentCurr = 0;
            }
            if ( hgcrf & hgcrfResetHitsEnumeration )
            {
                m_iHitsHistoSegmentStart = 0;
                
                m_pctLast = 0;
                m_cHitsTotalPctStart = 0;
                m_cPctHistAccumLast = 0;
                m_qwSampleLast = 0;
            }
            for ( ULONG iSeg  = 0; iSeg < m_cSegmentMac; iSeg++ )
            {
                CStats::ERR errReset = m_rgphistoSegments[ iSeg ]->ErrReset( hgcrf );
                STATAssert( errReset == CStats::ERR::errSuccess );
            }
            return ERR::errSuccess;
        }

        void Dump() const
        {
            for ( ULONG iSeg  = 0; iSeg < m_cSegmentMac; iSeg++ )
            {
                wprintf( L" Begin[%d] - < %I64u from %p:\n", iSeg, m_rgqwSampleSegmentMax[iSeg], m_rgphistoSegments[iSeg] );
                m_rgphistoSegments[iSeg]->ErrReset();
                PrintStats( m_rgphistoSegments[iSeg] );
            }
        }

        CStats::ERR ErrGetSampleValues( __out SAMPLE * pSamples )
        {
            AssertValid();

            if ( m_cSegmentMac == 0 )
            {
                STATAssertSz( fFalse, "What is the point of getting samples if you've added no histo ranges." );
                return ERR::errInvalidParameter;
            }

            if ( m_iSampleHistoSegmentCurr >= m_cSegmentMac )
            {
                return CStats::ERR::wrnOutOfSamples;
            }

            CStats::ERR errGet;
            do
            {
                STATAssert( m_rgphistoSegments[m_iSampleHistoSegmentCurr] );
                errGet = m_rgphistoSegments[m_iSampleHistoSegmentCurr]->ErrGetSampleValues( pSamples );
                if ( errGet != CStats::ERR::wrnOutOfSamples )
                {
                    STATAssert( errGet == CStats::ERR::errSuccess );
                    break;
                }

                m_rgphistoSegments[m_iSampleHistoSegmentCurr]->ErrReset( hgcrfResetSampleEnumeration );
                m_iSampleHistoSegmentCurr++;

                if ( m_iSampleHistoSegmentCurr >= m_cSegmentMac )
                {
                    return CStats::ERR::wrnOutOfSamples;
                }
            }
            while( errGet == ERR::wrnOutOfSamples );

            return errGet;
        }

        CStats::ERR ErrGetSampleHits( __in const SAMPLE qwValue, __out CHITS * pcHits )
        {
            *pcHits = 0;

            if ( m_cSegmentMac == 0 )
            {
                STATAssertSz( fFalse, "What is the point of getting samples if you've added no histo ranges." );
                return ERR::errInvalidParameter;
            }

            if ( m_iHitsHistoSegmentStart >= m_cSegmentMac )
            {
                return CStats::ERR::wrnOutOfSamples;
            }

            ULONG iSeg = m_iHitsHistoSegmentStart;
            BOOL fSuccess = fFalse;
            CStats::ERR errGet;
            do
            {
                STATAssert( m_rgphistoSegments[iSeg] );
                CHITS cHitsCurr = 0;
                errGet = m_rgphistoSegments[iSeg]->ErrGetSampleHits( qwValue, &cHitsCurr );

                STATAssert( errGet != CStats::ERR::wrnOutOfSamples || cHitsCurr == 0 );
                if ( errGet != CStats::ERR::wrnOutOfSamples && errGet != CStats::ERR::errSuccess )
                {
                    STATAssert( fFalse );
                    break;
                }

                if ( errGet == CStats::ERR::wrnOutOfSamples )
                {
                    m_iHitsHistoSegmentStart = iSeg + 1;
                }
                if ( !fSuccess )
                {
                    fSuccess = ( errGet == CStats::ERR::errSuccess );
                }

                *pcHits += cHitsCurr;

                iSeg++;

                if ( iSeg >= m_cSegmentMac )
                {
                    break;
                }
            }
            while( m_rgqwSampleSegmentMax[ iSeg - 1 ] <= qwValue );

            if ( errGet == CStats::ERR::wrnOutOfSamples &&
                    ( fSuccess ||
                        ( m_iHitsHistoSegmentStart < m_cSegmentMac &&
                            ( qwValue < Max() )
                        )
                    )
                )
            {
                errGet = CStats::ERR::errSuccess;
            }

            return errGet;
        }

        ULONG   m_pctLast;
        CHITS   m_cPctHistAccumLast;
        SAMPLE  m_qwSampleLast;
        CHITS   m_cHitsTotalPctStart;

        CStats::ERR ErrGetPercentileHits( __inout ULONG * pulPercentile, __out SAMPLE * pqwSample )
        {
            if ( m_cSegmentMac == 0 )
            {
                STATAssertSz( fFalse, "What is the point of getting samples if you've added no histo ranges." );
                return ERR::errInvalidParameter;
            }

            if ( m_cPctHistAccumLast == 0 )
            {
                m_cHitsTotalPctStart = C();
            }
            if ( m_cHitsTotalPctStart == 0 )
            {
                STATAssertSz( fFalse, "No samples added yet, really not sure what will happen." );
                return ERR::wrnOutOfSamples;
            }
            if ( m_cHitsTotalPctStart != C() )
            {
                STATAssertSz( fFalse, "You got last percent and then added more samples, you need to call ErrReset() in-between to restart pct enumeration." );
                return ERR::errInvalidParameter;
            }

            STATAssert( *pulPercentile > m_pctLast );
            m_pctLast = *pulPercentile;

            const CHITS cHitsPctTarget = ( ( m_cHitsTotalPctStart * (*pulPercentile) ) + 50 ) / 100;
            if ( cHitsPctTarget == 0 )
            {
                *pqwSample = m_mmts.Min();
                return CStats::ERR::errSuccess;
            }

            while( m_cPctHistAccumLast < cHitsPctTarget )
            {
                SAMPLE qwSampleNext;
                CHITS cHitsNext;
                CStats::ERR errGet = ErrGetSampleValues( &qwSampleNext );
                if ( errGet == CStats::ERR::wrnOutOfSamples )
                {
                    break;
                }
                else if ( errGet != CStats::ERR::errSuccess )
                {
                    STATAssert( fFalse );
                    return errGet;
                }
                STATAssert( qwSampleNext > m_qwSampleLast || qwSampleNext == 0  );

                CStats::ERR errHits = ErrGetSampleHits( qwSampleNext, &cHitsNext );
                STATAssert( errHits == CStats::ERR::errSuccess );

                m_qwSampleLast = qwSampleNext;
                m_cPctHistAccumLast += cHitsNext;
            }

            *pqwSample = m_qwSampleLast;

            return CStats::ERR::errSuccess;
        }

        SAMPLE Mode()
        {
            STATAssert( fFalse );
            return 0x4242424242424242;
        }

        CHITS C() const
        {
            AssertValid( fTrue );
            return m_mmts.C();
        }


        SAMPLE Min() const
        {
            AssertValid();
            if ( m_cSegmentMac == 0 )
            {
                STATAssertSz( fFalse, "What is the point of getting samples if you've added no histo ranges." );
                return qwMax;
            }
            return m_mmts.Min();
        }
        SAMPLE Max() const
        {
            AssertValid();
            if ( m_cSegmentMac == 0 )
            {
                STATAssertSz( fFalse, "What is the point of getting samples if you've added no histo ranges." );
                return 0;
            }
            return m_mmts.Max();
        }
        SAMPLE Total() const
        {
            AssertValid();
            return m_mmts.Total();
        }
        SAMPLE Ave() const
        {
            AssertValid();
            const CHITS c = C();
            const SAMPLE t = Total();
            return c ? ( t / c ) : 0;
        }
        SAMPLE Mean() const
        {
            AssertValid();
            return Ave();
        }
        double DblAve() const
        {
            AssertValid();
            const double c = (double)C();
            const double t = (double)Total();
            return c ? ( t / c ) : 0.0;
        }

};




inline void PrintStats( CStats * pCS )
{
    CStats::ERR csErr;

    if ( pCS == NULL )
    {
        wprintf(L"   NULL pointer to object of class CStats!\n");
        return;
    }
    
    if ( pCS->C() == 0 )
    {
        wprintf(L"   No samples!\n");
        return;
    }

    if ( CStats::ERR::errSuccess != ( csErr = pCS->ErrReset() ) )
    {
        wprintf(L" ERROR: %d reseting stat class.\n", csErr );
        return;
    }

    SAMPLE sample;
    while( CStats::ERR::errSuccess == ( csErr = pCS->ErrGetSampleValues( &sample ) ) )
    {
        CHITS hits;
        csErr = pCS->ErrGetSampleHits( sample, &hits );
        if ( csErr != CStats::ERR::errSuccess )
        {
            break;
        }
        wprintf(L"   %I64u = %I64u\n", sample, hits );
    }
    if ( csErr != CStats::ERR::wrnOutOfSamples )
    {
        wprintf(L" ERROR: %d enumerating samples.\n", csErr );
    }

    wprintf(L" C,Min,Ave(DblAve),Max,Total:	%I64u,%I64u,%I64u(%f),%I64u,%I64u\n",
        pCS->C(), pCS->Min(), pCS->Ave(), pCS->DblAve(), pCS->Max(), pCS->Total() );

    (void)pCS->ErrReset();
}

inline void PrintStats( CStats * pCS, SAMPLE dcChunk )
{
    CStats::ERR csErr;
    __int64 iChunk = 0;
    SAMPLE cChunk = 0;
    CHITS cHits = 0;

    if ( pCS == NULL )
    {
        wprintf(L"   NULL pointer to object of class CStats!\n");
        return;
    }
    
    if ( CStats::ERR::errSuccess != ( csErr = pCS->ErrReset() ) )
    {
        wprintf(L" ERROR: %d reseting stat class.\n", csErr );
        return;
    }

    while( CStats::ERR::errSuccess == ( csErr = pCS->ErrGetSampleHits( cChunk, &cHits ) ) )
    {
        wprintf(L"    [%I64d].%I64u = %I64u\n", iChunk, cChunk, cHits );
        iChunk++;
        cChunk += dcChunk;
    }
    if ( csErr != CStats::ERR::wrnOutOfSamples &&
            csErr != CStats::ERR::errInvalidParameter  )
    {
        wprintf(L" ERROR: %d enumerating samples.\n", csErr );
    }
    if ( iChunk == 0 )
    {
        wprintf(L"   No samples!\n");
    }

    wprintf(L" C,Min,Ave(DblAve),Max,Total:	%I64u,%I64u,%I64u(%f),%I64u,%I64u\n",
        pCS->C(), pCS->Min(), pCS->Ave(), pCS->DblAve(), pCS->Max(), pCS->Total() );

    (void)pCS->ErrReset();
}


template< class CType, LONG_PTR m_cSamples >
class CMovingAverage
{
    private:


        C_ASSERT( m_cSamples > 0 );
        CType           m_rgSamples[ m_cSamples ];
        LONG_PTR        m_iLastSample;
        CType           m_avg;
    
    public:


        CMovingAverage( const CType& initSample )   { Reset( initSample ); }

    public:


        INLINE void Reset( const CType& initSample )
        {
            for ( LONG_PTR iSample = 0; iSample < m_cSamples; iSample++ )
            {
                m_rgSamples[ iSample ] = initSample;
            }

            m_avg = CType( initSample * m_cSamples );
            m_iLastSample = 0;

            STATAssert( CType( m_avg / m_cSamples ) == initSample );
        }

        INLINE void AddSample( const CType& newSample )
        {
            m_iLastSample = ( m_iLastSample + 1 ) % m_cSamples;
            
            const CType& oldSample = m_rgSamples[ m_iLastSample ];
            const CType incSample = newSample - oldSample;

            STATAssert( !( newSample > oldSample ) || ( ( m_avg + incSample ) > m_avg ) );
            STATAssert( !( oldSample > newSample ) || ( m_avg > ( m_avg + incSample ) ) );

            m_avg = m_avg + incSample;
            m_rgSamples[ m_iLastSample ] = newSample;
        }
        
        INLINE const CType& GetLastSample() const
        {
            return m_rgSamples[ m_iLastSample ];
        }

        INLINE const CType GetAverage() const
        {
            return CType( m_avg / m_cSamples );
        }
};

};


using namespace STAT;


#endif

