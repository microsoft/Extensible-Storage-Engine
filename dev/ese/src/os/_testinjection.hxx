// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

LOCAL const ULONG ulIDInvalid = ~(0UL);

class TESTINJECTION
{
public:
    TESTINJECTION() :
        m_ulID( ulIDInvalid ),
        m_pv( NULL ),
        m_ulProb( 0 ),
        m_grbit( 0x0 ),
        m_cEvals( 0 ),
        m_cHits( 0 )
    {
    }

    TESTINJECTION( const ULONG ulID, const JET_API_PTR pv, const ULONG ulProbability, const DWORD grbit ) :
        m_ulID( ulID ),
        m_pv( pv ),
        m_ulProb( ulProbability ),
        m_grbit( grbit ),
        m_cEvals( 0 ),
        m_cHits( 0 )
    {
    }
    
    TESTINJECTION& operator=( const TESTINJECTION& rhs )
    {
        m_ulID      = rhs.m_ulID;
        m_pv        = rhs.m_pv;
        m_ulProb    = rhs.m_ulProb;
        m_grbit     = rhs.m_grbit;
        m_cEvals    = rhs.m_cEvals;
        m_cHits     = rhs.m_cHits;
        return *this;
    }

    bool operator==( const TESTINJECTION& rhs ) const
    {
        return( m_ulID == rhs.m_ulID );
    }

#ifdef DEBUGGER_EXTENSION
    void DumpLite( const ULONG iinj );
#endif

    ULONG Id() const
    {
        return m_ulID;
    }

    bool FProbable()
    {
        AtomicAdd( &m_cEvals, 1 );

        Assert( m_grbit );

        if ( m_grbit & JET_bitInjectionProbabilitySuppress )
        {
            return false;
        }
        if ( m_grbit & JET_bitInjectionProbabilityPct )
        {
            if ( (ULONG)( rand() % 100 ) < m_ulProb )
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else if( m_grbit & JET_bitInjectionProbabilityCount )
        {
            if ( m_grbit & JET_bitInjectionProbabilityPermanent )
            {
                return m_cEvals >= (QWORD)m_ulProb;
            }
            else if ( m_grbit & JET_bitInjectionProbabilityFailUntil )
            {
                return m_cEvals <= (QWORD)m_ulProb;
            }
            else
            {
                return m_cEvals == (QWORD)m_ulProb;
            }
        }

        AssertSz( fFalse, "Test injection FProbable() mis-calculated, grbit unknown" );
        return false;
    }

    JET_API_PTR Pv()
    {
        AtomicAdd( &m_cHits, 1 );
        return m_pv;
    }

    void TraceStats() const
    {
    }

    void Suppress()
    {
        m_grbit |= JET_bitInjectionProbabilitySuppress;
    }

    void Unsuppress()
    {
        m_grbit &= ~JET_bitInjectionProbabilitySuppress;
    }

    void Disable()
    {
        m_ulID = ulIDInvalid;
    }

    QWORD Chits() const
    {
        return m_cHits;
    }
    
private:
    ULONG   m_ulID;
    JET_API_PTR     m_pv;
    ULONG   m_ulProb;
    DWORD           m_grbit;
    QWORD           m_cEvals;
    QWORD           m_cHits;
};

LOCAL const ULONG g_cTestInjectionsMax = 128;
extern TESTINJECTION g_rgTestInjections[g_cTestInjectionsMax];
