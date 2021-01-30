// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef PERFCTRS_H_INCLUDED
#define PERFCTRS_H_INCLUDED


typedef BYTE TABLECLASS;

const BYTE tableclassMin    = 0;
const BYTE tableclassNone   = 0;

const BYTE tableclassReservedMin    = 1;
const BYTE tableclassCatalog        = tableclassReservedMin;
const BYTE tableclassShadowCatalog  = 2;
const BYTE tableclassReservedMost   = tableclassShadowCatalog;

const BYTE tableClassPublic = tableclassReservedMost + 1;
const BYTE tableclass1      = tableClassPublic;
const BYTE tableclass2      = tableClassPublic + 1;
const BYTE tableclass3      = tableClassPublic + 2;
const BYTE tableclass4      = tableClassPublic + 3;
const BYTE tableclass5      = tableClassPublic + 4;
const BYTE tableclass6      = tableClassPublic + 5;
const BYTE tableclass7      = tableClassPublic + 6;
const BYTE tableclass8      = tableClassPublic + 7;
const BYTE tableclass9      = tableClassPublic + 8;
const BYTE tableclass10     = tableClassPublic + 9;
const BYTE tableclass11     = tableClassPublic + 10;
const BYTE tableclass12     = tableClassPublic + 11;
const BYTE tableclass13     = tableClassPublic + 12;
const BYTE tableclass14     = tableClassPublic + 13;
const BYTE tableclass15     = tableClassPublic + 14;
const BYTE tableclass16     = tableClassPublic + 15;
const BYTE tableclass17     = tableClassPublic + 16;
const BYTE tableclass18     = tableClassPublic + 17;
const BYTE tableclass19     = tableClassPublic + 18;
const BYTE tableclass20     = tableClassPublic + 19;
const BYTE tableclass21     = tableClassPublic + 20;
const BYTE tableclass22     = tableClassPublic + 21;
const BYTE tableclass23     = tableClassPublic + 22;
const BYTE tableclass24     = tableClassPublic + 23;
const BYTE tableclass25     = tableClassPublic + 24;
const BYTE tableclass26     = tableClassPublic + 25;
const BYTE tableclass27     = tableClassPublic + 26;
const BYTE tableclass28     = tableClassPublic + 27;
const BYTE tableclass29     = tableClassPublic + 28;
const BYTE tableclass30     = tableClassPublic + 29;
const BYTE tableclass31     = tableClassPublic + 30;

const BYTE tableclassMost   = tableclass31;
const BYTE tableclassMax    = tableclassMost + 1;


typedef BYTE TCE;

#define tceNone ( 0 )
#define tceMin  ( 1 )
#define tcePerTableClass ( 4 )
#define tceMost ( tableclassMost * tcePerTableClass )
#define tceMax  ( 1 + tceMost )

C_ASSERT( tceMost <= bMax );

INLINE TABLECLASS TableClassFromTCE( TCE tce )
{
    Assert( tce <= tceMost );
    return (TABLECLASS) ( ( tce + tcePerTableClass - 1 ) / tcePerTableClass );
}

INLINE TCE TCEFromTableClass( TABLECLASS tableclass, BOOL fLV, BOOL fIndex, BOOL fSpace )
{
    Assert( !( fLV && fIndex ) );
    Assert( tableclass <= tableclassMost );

    if ( tableclass == tableclassNone )
    {
        return tceNone;
    }
    else
    {
        TCE tce = tableclass * tcePerTableClass;
        if ( fSpace )
            tce -= 3;
        else if ( fIndex )
            tce -= 2;
        else if ( fLV )
            tce -= 1;
        return tce;
    }
}

INLINE BOOL FIsLV( TCE tce )
{
    INT remainder = tce % tcePerTableClass;
    return ( remainder == 3 );
}

INLINE BOOL FIsIndex( TCE tce )
{
    INT remainder = tce % tcePerTableClass;
    return ( remainder == 2 );
}
    
INLINE BOOL FIsSpace( TCE tce )
{
    INT remainder = tce % tcePerTableClass;
    return ( remainder == 1 );
}

INLINE TABLECLASS TableClassFromSysTable( BOOL fShadowSystemTable )
{
    if ( fShadowSystemTable )
    {
        return tableclassShadowCatalog;
    }

    return tableclassCatalog;
}

INLINE TABLECLASS TableClassFromTableClassOffset( const ULONG tableClassOffset )
{
    TABLECLASS tableclass;

    if ( 0 == tableClassOffset )
    {
        tableclass = tableclassNone;
    }
    else
    {
        tableclass = TABLECLASS( tableClassPublic + tableClassOffset - 1 );
    }

    return tableclass;
}

extern LONG g_cbPlsMemRequiredPerPerfInstance;

const INT perfinstGlobal = 0;
static_assert( perfinstGlobal == 0, "Please, carefully review all references to perfinstGlobal because some of them use the symbolic const, but implicitly assume zero." );

#define CPerfinstMaxToUse( cpinstMaxT, ifmpMaxT )   ( max( max( cpinstMaxT, ifmpMaxT ), tceMax ) + 1 )
#define cPerfinstMax                                CPerfinstMaxToUse( g_cpinstMax, g_ifmpMax )

template < class TData = LONG, BOOL fHashPerProc = fTrue >
class PERFInstance
{
#ifdef PERFMON_SUPPORT
    protected:
        LONG m_ibOffset;

        INLINE BYTE* GetDataBuffer( const INT iInstance )
        {
            PLS* const ppls = ( fHashPerProc ? Ppls() : Ppls( 0 ) );
            const ULONG ibOffset = iInstance * g_cbPlsMemRequiredPerPerfInstance + m_ibOffset;
            BYTE* const pbDataBuffer = ppls->PbGetPerfCounterBuffer( ibOffset, sizeof( TData ) );
            return pbDataBuffer;
        }

        INLINE TData GetData( const size_t iProc, const INT iInstance )
        {
            const BYTE* const pbDataBuffer = Ppls( iProc )->PbGetPerfCounterBuffer( iInstance * g_cbPlsMemRequiredPerPerfInstance + m_ibOffset, sizeof( TData ) );
            return *( (TData*)pbDataBuffer );
        }

        INLINE void SetData( const size_t iProc, const INT iInstance, const TData data )
        {
            PLS* const ppls = Ppls( iProc );
            const ULONG ibOffset = iInstance * g_cbPlsMemRequiredPerPerfInstance + m_ibOffset;
            BYTE* const pbDataBuffer = ppls->PbGetPerfCounterBuffer( ibOffset, sizeof( TData ) );
            *(TData*)pbDataBuffer = data;
        }

        INLINE void IncData( const INT iInstance, const TData data )
        {
            PLS* const ppls = ( fHashPerProc ? Ppls() : Ppls( 0 ) );
            const ULONG ibOffset = iInstance * g_cbPlsMemRequiredPerPerfInstance + m_ibOffset;
            BYTE* const pbDataBuffer = ppls->PbGetPerfCounterBuffer( ibOffset, sizeof( TData ) );
            *(TData*)pbDataBuffer += data;
        }

    public:
        PERFInstance()
        {
            m_ibOffset = AtomicExchangeAdd(
                            &g_cbPlsMemRequiredPerPerfInstance,
                            LONG( roundup( sizeof( TData ), sizeof( DWORD_PTR ) ) ) );
        }

        ~PERFInstance()
        {
        }

        VOID Clear( INT iInstance )
        {
            Set( iInstance, TData(0) );
        }

        VOID Inc( INT iInstance )
        {
            Add( iInstance, TData(1) );
        }

        VOID Add( INT iInstance, const TData lValue )
        {
            if ( g_fDisablePerfmon )
            {
                AssertSz( fFalse, "Perf counters disabled" );
                return;
            }
            Assert( iInstance >= 0 );
            Assert( (ULONG)iInstance < cPerfinstMax );
            IncData( iInstance, lValue );
        }

        VOID Set( INT iInstance, const TData lValue )
        {
            if ( g_fDisablePerfmon )
            {
                AssertSz( fFalse, "Perf counters disabled" );
                return;
            }

            AssertSz( !fHashPerProc || lValue == 0, "Performance issue - .Set should not be used with hashed counters, except for resetting to 0." );

            Assert( iInstance >= 0 );
            Assert( (ULONG)iInstance < cPerfinstMax );
            SetData( 0, iInstance, lValue );
            if ( fHashPerProc )
            {
                const size_t cProcs = (size_t)OSSyncGetProcessorCountMax();
                for ( size_t iProc = 1; iProc < cProcs; iProc++ )
                {
                    SetData( iProc, iInstance, TData(0) );
                }
            }
        }

        VOID Max( INT iInstance, const TData lValue )
        {
            if ( g_fDisablePerfmon )
            {
                AssertSz( fFalse, "Perf counters disabled" );
                return;
            }
            Assert( !fHashPerProc );
            TData* const pPerfCounter = (TData*)GetDataBuffer( iInstance );
            if ( *pPerfCounter < lValue )
            {
                *pPerfCounter = lValue;
            }
        }

        VOID Dec( INT iInstance )
        {
            Add( iInstance, TData(-1) );
        }

        TData Get( INT iInstance )
        {
            if ( g_fDisablePerfmon )
            {
                AssertSz( fFalse, "Perf counters disabled" );
                return TData(0);
            }
            Assert( iInstance >= 0 );
            Assert( (ULONG)iInstance < cPerfinstMax );
            TData counter = TData(0);


            TRY
            {
                if ( fHashPerProc )
                {
                    const size_t    cProcs  = (size_t)OSSyncGetProcessorCountMax();
                    counter = TData(0);
                    for ( size_t iProc = 0; iProc < cProcs; iProc++ )
                    {
                        counter += GetData( iProc, iInstance );
                    }
                }
                else
                {
                    counter = GetData( 0, iInstance );
                }
            }
            EXCEPT( efaExecuteHandler )
            {
            }
            ENDEXCEPT;

            return counter;
        }

        VOID PassTo( INT iInstance, VOID *pvBuf )
        {
            if ( NULL != pvBuf )
            {
                *(TData*)pvBuf = Get( iInstance );
            }
        }
#endif
};


template < class TData = LONG, class TAggregation = INST, BOOL fHashPerProc = fTrue, BOOL fComputeTotal = fTrue >
class PERFInstanceDelayedTotal : public PERFInstance<TData, fHashPerProc>
{
#ifdef PERFMON_SUPPORT
    public:
        TData Get( INT iInstance )
        {
            if ( g_fDisablePerfmon )
            {
                AssertSz( fFalse, "Perf counters disabled" );
                return TData(0);
            }

            Assert( iInstance >= 0 );
            Assert( (ULONG)iInstance < cPerfinstMax );

            if ( perfinstGlobal == iInstance )
            {
                const LONG lInstanceActiveMin = TAggregation::iInstanceActiveMin;
                const LONG lInstanceActiveMac = TAggregation::iInstanceActiveMac;
                const BOOL fSingleInstance = ( lInstanceActiveMin == lInstanceActiveMac );

                if ( fSingleInstance || fComputeTotal )
                {
                    TData counter = TData(0);
                    for ( INT i = lInstanceActiveMin; i <= lInstanceActiveMac; i++ )
                    {
                        counter += Get( i );
                    }
                    return counter;
                }
            }

            return PERFInstance<TData, fHashPerProc>::Get( iInstance );
        }

        using PERFInstance::Clear;
        VOID Clear( TAggregation const * const pobj )
        {
            Assert( NULL != pobj );
            Clear( pobj->m_iInstance );
        }

        using PERFInstance::Inc;
        VOID Inc( TAggregation const * const pobj )
        {
            Assert( NULL != pobj );
            const INT iInstance = pobj->m_iInstance;
            Assert( iInstance != perfinstGlobal );
            Inc( pobj->m_iInstance );
        }

        using PERFInstance::Add;
        VOID Add( TAggregation const * const pobj, const TData lValue )
        {
            Assert( NULL != pobj );
            const INT iInstance = pobj->m_iInstance;
            Assert( iInstance != perfinstGlobal );
            Add( pobj->m_iInstance, lValue );
        }

        using PERFInstance::Set;
        VOID Set( TAggregation const * const pobj, const TData lValue )
        {
            Assert( NULL != pobj );
            const INT iInstance = pobj->m_iInstance;
            Assert( iInstance != perfinstGlobal );
            Set( pobj->m_iInstance, lValue );
        }

        using PERFInstance::Dec;
        VOID Dec( TAggregation const * const pobj )
        {
            Assert( NULL != pobj );
            const INT iInstance = pobj->m_iInstance;
            Assert( iInstance != perfinstGlobal );
            Dec( pobj->m_iInstance );
        }

        TData Get( TAggregation const * const pobj )
        {
            return Get( NULL != pobj ? pobj->m_iInstance : perfinstGlobal );
        }

        VOID PassTo( INT iInstance, VOID *pvBuf )
        {
            if ( NULL != pvBuf )
            {
                *(TData*)pvBuf = Get( iInstance );
            }
        }
#endif
};


template < class TData = LONG, class TAggregation = INST, BOOL fHashPerProc = fTrue >
class PERFInstanceLiveTotal : public PERFInstance<TData, fHashPerProc>
{
#ifdef PERFMON_SUPPORT
    public:
        VOID Inc( INT iInstance )
        {
            Add( iInstance, TData(1) );
        }

        VOID Add( INT iInstance, const TData lValue )
        {
            if ( g_fDisablePerfmon )
            {
                AssertSz( fFalse, "Perf counters disabled" );
                return;
            }
            IncData( iInstance, lValue );
            IncData( perfinstGlobal, lValue );
        }

        VOID Max( INT iInstance, const TData lValue )
        {
            if ( g_fDisablePerfmon )
            {
                AssertSz( fFalse, "Perf counters disabled" );
                return;
            }
            Assert( !fHashPerProc );

            TData* const pPerfCounter = (TData*)GetDataBuffer( iInstance );
            if ( *pPerfCounter < lValue )
            {
                *pPerfCounter = lValue;
            }

            Assert( iInstance != perfinstGlobal );

            TData* const pPerfCounterTotal = (TData*)GetDataBuffer( perfinstGlobal );
            if ( *pPerfCounterTotal < lValue )
            {
                *pPerfCounterTotal = lValue;
            }
        }

        VOID Dec( INT iInstance )
        {
            Add( iInstance, TData(-1) );
        }

        using PERFInstance::Clear;
        VOID Clear( TAggregation const * const pobj )
        {
            Assert( NULL != pobj );
            Clear( pobj->m_iInstance );
        }

        using PERFInstance::Inc;
        VOID Inc( TAggregation const * const pobj )
        {
            Assert( NULL != pobj );
            const INT iInstance = pobj->m_iInstance;
            Assert( iInstance != perfinstGlobal );
            Inc( pobj->m_iInstance );
        }

        using PERFInstance::Add;
        VOID Add( TAggregation const * const pobj, const TData lValue )
        {
            Assert( NULL != pobj );
            const INT iInstance = pobj->m_iInstance;
            Assert( iInstance != perfinstGlobal );
            Add( pobj->m_iInstance, lValue );
        }

        using PERFInstance::Set;
        VOID Set( TAggregation const * const pobj, const TData lValue )
        {
            Assert( NULL != pobj );
            const INT iInstance = pobj->m_iInstance;
            Assert( iInstance != perfinstGlobal );
            Set( pobj->m_iInstance, lValue );
        }

        using PERFInstance::Dec;
        VOID Dec( TAggregation const * const pobj )
        {
            Assert( NULL != pobj );
            const INT iInstance = pobj->m_iInstance;
            Assert( iInstance != perfinstGlobal );
            Dec( pobj->m_iInstance );
        }
#endif
};


    
template < class TData = LONG, class TAggregation = INST, INT cClassCount = 1, BOOL fHashPerProc = fTrue >
class PERFInstanceDelayedTotalWithClass
{
#ifdef PERFMON_SUPPORT
    static_assert( cClassCount > 0, "If you are aggregating 0 classes, then you don't need this object." );
    static_assert( cClassCount <= 2, "This function needs to be updated to cover a different number, look for layering violation comments below." );

    private:
        PERFInstanceDelayedTotal<TData, TAggregation, fHashPerProc> cInstance;
        PERFInstance<TData, fHashPerProc> cClass[ cClassCount ];

        TData GetInstance( INT iInstance )
        {
            Assert( ( DwPERFCurPerfObj() == PERF_OBJECT_ESE ) || ( DwPERFCurPerfObj() == PERF_OBJECT_INSTANCES ) );
            return cInstance.Get( iInstance );
        }

        TData GetClass( INT iClass, INT iInstanceClass )
        {
            OnDebug( const DWORD dwPERFCurPerfObj = DwPERFCurPerfObj() );
            Assert( ( iClass >= 0 ) && ( iClass < cClassCount ) );
            Assert( iInstanceClass >= 0 );
            Assert( ( ( dwPERFCurPerfObj == PERF_OBJECT_TABLECLASS ) && ( iClass == 0 ) ) || ( dwPERFCurPerfObj == PERF_OBJECT_DATABASES ) );
            return cClass[ iClass ].Get( iInstanceClass );
        }

    public:
        VOID Add( INT iInstance, INT iInstanceClass0, const TData lValue )
        {
            Add( iInstance, iInstanceClass0, -1, lValue );
        }

        VOID Add( INT iInstance, INT iInstanceClass0, INT iInstanceClass1, const TData lValue )
        {
            Assert( iInstance != perfinstGlobal );
            Assert( iInstanceClass0 >= 0 );
            Assert( ( iInstanceClass1 >= 0 ) || ( cClassCount < 2 ) );
            Assert( ( iInstanceClass1 < 0 ) || ( cClassCount > 1 ) );

            if ( iInstance != perfinstGlobal )
            {
                cInstance.Add( iInstance, lValue );
            }

            cClass[ 0 ].Add( iInstanceClass0, lValue );

            if ( iInstanceClass1 >= 0 )
            {
                cClass[ 1 ].Add( iInstanceClass1, lValue );
            }
        }
        
        VOID Inc( INT iInstance, INT iInstanceClass0, INT iInstanceClass1 = -1 )
        {
            Add( iInstance, iInstanceClass0, iInstanceClass1, TData(1) );
        }

        VOID Inc( TAggregation const * const pobj, INT iInstanceClass0, INT iInstanceClass1 = -1 )
        {
            Inc( pobj->m_iInstance, iInstanceClass0, iInstanceClass1 );
        }

        VOID Dec( INT iInstance, INT iInstanceClass0, INT iInstanceClass1 = -1 )
        {
            Add( iInstance, iInstanceClass0, iInstanceClass1, TData(-1) );
        }

        VOID Dec( TAggregation const * const pobj, INT iInstanceClass0, INT iInstanceClass1 = -1 )
        {
            Dec( pobj->m_iInstance, iInstanceClass0, iInstanceClass1 );
        }

        TData Get( INT iInstance )
        {
            const DWORD dwPERFCurPerfObj = DwPERFCurPerfObj();
            if ( dwPERFCurPerfObj == PERF_OBJECT_ESE )
            {
                Assert( iInstance == perfinstGlobal );
                return GetInstance( perfinstGlobal );
            }
            else if ( dwPERFCurPerfObj == PERF_OBJECT_INSTANCES )
            {
                return GetInstance( iInstance );
            }
            else if ( dwPERFCurPerfObj == PERF_OBJECT_TABLECLASS )
            {
                return GetClass( 0, iInstance );
            }
            else if ( dwPERFCurPerfObj == PERF_OBJECT_DATABASES )
            {
                return GetClass( ( cClassCount == 1 ) ? 0 : 1, iInstance );
            }
            else
            {
                AssertSz( fFalse, "Unhandled perf. object type %u.", dwPERFCurPerfObj );
                return GetInstance( perfinstGlobal );
            }
        }
        
        TData GetInstance( TAggregation const * pobj )
        {
            return GetInstance( pobj->m_iInstance );
        }
        
        VOID PassTo( INT iInstance, VOID *pvBuf )
        {
            if ( NULL != pvBuf )
            {
                *(TData*)pvBuf = Get( iInstance );
            }
        }
#endif
};

template < class TData = LONG, class TAggregation = INST, INT cClassCount = 1, BOOL fHashPerProc = fTrue >
class PERFInstanceLiveTotalWithClass
{
#ifdef PERFMON_SUPPORT
    static_assert( cClassCount > 0, "If you are aggregating 0 classes, then you don't need this object." );
    static_assert( cClassCount <= 2, "This function needs to be updated to cover a different number, look for layering violation comments below." );

    private:
        PERFInstanceLiveTotal<TData, TAggregation, fHashPerProc> cInstance;
        PERFInstance<TData, fHashPerProc> cClass[ cClassCount ];

        TData GetInstance( INT iInstance )
        {
            Assert( ( DwPERFCurPerfObj() == PERF_OBJECT_ESE ) || ( DwPERFCurPerfObj() == PERF_OBJECT_INSTANCES ) );
            return cInstance.Get( iInstance );
        }

        TData GetClass( INT iClass, INT iInstanceClass )
        {
            OnDebug( const DWORD dwPERFCurPerfObj = DwPERFCurPerfObj() );
            Assert( ( iClass >= 0 ) && ( iClass < cClassCount ) );
            Assert( iInstanceClass >= 0 );
            Assert( ( ( dwPERFCurPerfObj == PERF_OBJECT_TABLECLASS ) && ( iClass == 0 ) ) || ( dwPERFCurPerfObj == PERF_OBJECT_DATABASES ) );
            return cClass[ iClass ].Get( iInstanceClass );
        }

    public:
        VOID Add( INT iInstance, INT iInstanceClass0, const TData lValue )
        {
            Add( iInstance, iInstanceClass0, -1, lValue );
        }

        VOID Add( INT iInstance, INT iInstanceClass0, INT iInstanceClass1, const TData lValue )
        {
            Assert( iInstance != perfinstGlobal );
            Assert( iInstanceClass0 >= 0 );
            Assert( ( iInstanceClass1 >= 0 ) || ( cClassCount < 2 ) );
            Assert( ( iInstanceClass1 < 0 ) || ( cClassCount > 1 ) );

            if ( iInstance != perfinstGlobal )
            {
                cInstance.Add( iInstance, lValue );
            }

            cClass[ 0 ].Add( iInstanceClass0, lValue );
            if ( iInstanceClass1 >= 0 )
            {
                cClass[ 1 ].Add( iInstanceClass1, lValue );
            }
        }
        
        VOID Inc( INT iInstance, INT iInstanceClass0, INT iInstanceClass1 = -1 )
        {
            Add( iInstance, iInstanceClass0, iInstanceClass1, TData(1) );
        }

        VOID Inc( TAggregation const * const pobj, INT iInstanceClass0, INT iInstanceClass1 = -1 )
        {
            Inc( pobj->m_iInstance, iInstanceClass0, iInstanceClass1 );
        }

        VOID Dec( INT iInstance, INT iInstanceClass0, INT iInstanceClass1 = -1 )
        {
            Add( iInstance, iInstanceClass0, iInstanceClass1, TData(-1) );
        }

        VOID Dec( TAggregation const * const pobj, INT iInstanceClass0, INT iInstanceClass1 = -1 )
        {
            Dec( pobj->m_iInstance, iInstanceClass0, iInstanceClass1 );
        }

        VOID Max( INT iInstance, INT iInstanceClass0, INT iInstanceClass1, const TData lValue )
        {
            Assert( iInstanceClass0 >= 0 );
            Assert( ( iInstanceClass1 >= 0 ) || ( cClassCount < 2 ) );
            Assert( ( iInstanceClass1 < 0 ) || ( cClassCount > 1 ) );
            
            if ( iInstance != perfinstGlobal )
            {
                cInstance.Max( iInstance, lValue );
            }

            cClass[ 0 ].Max( iInstanceClass0, lValue );
            if ( iInstanceClass1 >= 0 )
            {
                cClass[ 1 ].Max( iInstanceClass1, lValue );
            }
        }

        VOID Max( TAggregation const * const pobj, INT iInstanceClass0, const TData lValue )
        {
            Max( pobj->m_iInstance, iInstanceClass0, -1, lValue );
        }

        VOID Max( TAggregation const * const pobj, INT iInstanceClass0, INT iInstanceClass1, const TData lValue )
        {
            Max( pobj->m_iInstance, iInstanceClass0, iInstanceClass1, lValue );
        }

        TData Get( INT iInstance )
        {
            const DWORD dwPERFCurPerfObj = DwPERFCurPerfObj();
            if ( dwPERFCurPerfObj == PERF_OBJECT_ESE )
            {
                Assert( iInstance == perfinstGlobal );
                return GetInstance( perfinstGlobal );
            }
            else if ( dwPERFCurPerfObj == PERF_OBJECT_INSTANCES )
            {
                return GetInstance( iInstance );
            }
            else if ( dwPERFCurPerfObj == PERF_OBJECT_TABLECLASS )
            {
                return GetClass( 0, iInstance );
            }
            else if ( dwPERFCurPerfObj == PERF_OBJECT_DATABASES )
            {
                return GetClass( ( cClassCount == 1 ) ? 0 : 1, iInstance );
            }
            else
            {
                AssertSz( fFalse, "Unhandled perf. object type %u.", dwPERFCurPerfObj );
                return GetInstance( perfinstGlobal );
            }
        }
        
        TData GetInstance( TAggregation const * pobj )
        {
            return GetInstance( pobj->m_iInstance );
        }
        
        VOID PassTo( INT iInstance, VOID *pvBuf )
        {
            if ( NULL != pvBuf )
            {
                *(TData*)pvBuf = Get( iInstance );
            }
        }
#endif
};


#define PERFIncCounterTable( x, y, z )  \
{                                       \
    x.Inc( y, z );                      \
}

#endif

