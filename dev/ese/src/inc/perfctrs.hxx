// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef PERFCTRS_H_INCLUDED
#define PERFCTRS_H_INCLUDED

//  Table Class type. This is the raw information provided by callers through JetOpenTable

typedef BYTE TABLECLASS;

const BYTE tableclassMin    = 0;
const BYTE tableclassNone   = 0;

//  Begin Reserved Range
const BYTE tableclassReservedMin    = 1;
const BYTE tableclassCatalog        = tableclassReservedMin;    // Reserved1
const BYTE tableclassShadowCatalog  = 2;    // Reserved2
const BYTE tableclassReservedMost   = tableclassShadowCatalog;
//  End Reserved Range

//  Begin Public Range
const BYTE tableClassPublic = tableclassReservedMost + 1;   // 3
const BYTE tableclass1      = tableClassPublic;             // 3
const BYTE tableclass2      = tableClassPublic + 1;         // 4
const BYTE tableclass3      = tableClassPublic + 2;         // 5
const BYTE tableclass4      = tableClassPublic + 3;         // 6
const BYTE tableclass5      = tableClassPublic + 4;         // 7
const BYTE tableclass6      = tableClassPublic + 5;         // 8
const BYTE tableclass7      = tableClassPublic + 6;         // 9
const BYTE tableclass8      = tableClassPublic + 7;         // 10
const BYTE tableclass9      = tableClassPublic + 8;         // 11
const BYTE tableclass10     = tableClassPublic + 9;         // 12
const BYTE tableclass11     = tableClassPublic + 10;        // 13
const BYTE tableclass12     = tableClassPublic + 11;        // 14
const BYTE tableclass13     = tableClassPublic + 12;        // 15
const BYTE tableclass14     = tableClassPublic + 13;        // 16
const BYTE tableclass15     = tableClassPublic + 14;        // 17
const BYTE tableclass16     = tableClassPublic + 15;        // 18
const BYTE tableclass17     = tableClassPublic + 16;        // 19
const BYTE tableclass18     = tableClassPublic + 17;        // 20
const BYTE tableclass19     = tableClassPublic + 18;        // 21
const BYTE tableclass20     = tableClassPublic + 19;        // 22
const BYTE tableclass21     = tableClassPublic + 20;        // 23
const BYTE tableclass22     = tableClassPublic + 21;        // 24
const BYTE tableclass23     = tableClassPublic + 22;        // 25
const BYTE tableclass24     = tableClassPublic + 23;        // 26
const BYTE tableclass25     = tableClassPublic + 24;        // 27
const BYTE tableclass26     = tableClassPublic + 25;        // 28
const BYTE tableclass27     = tableClassPublic + 26;        // 29
const BYTE tableclass28     = tableClassPublic + 27;        // 30
const BYTE tableclass29     = tableClassPublic + 28;        // 31
const BYTE tableclass30     = tableClassPublic + 29;        // 32
const BYTE tableclass31     = tableClassPublic + 30;        // 33
//  End Public Range

const BYTE tableclassMost   = tableclass31;                 // 33
const BYTE tableclassMax    = tableclassMost + 1;           // 34

//  TCE (TableClassEx) is used internally to collect and display perfmon data per tableclass.
//  It expands the "space" of tableclass in 2 ways.
//  1) Index 0 is special and means "_Unknown". This is used for all B-trees
//  for which we are unable to determine its actual tableclass
//  2) Primary B-tree vs LV B-tree vs Index B-tree(s) vs Space B-tree(s) are distinguished in 
///this extended space. For tableclassN, the TCE values 4N, 4N-1, 4N-2, and 4N-3 
///correspond to the Primary, LV, Index, and the Space B-trees
//  respectively.
//      So the TCE space is something like the following:
//  TCE = 0 : Unknown
//
//  TCE = 1 : Space B-trees for catalog
//  TCE = 2 : Index B-trees for catalog
//  TCE = 3 : LV B-Trees for catalog
//  TCE = 4 : Primary B-trees for catalog
//
//  TCE = 5 : Space B-trees for shadow catalog
//  TCE = 6 : Index B-trees for shadow catalog
//  TCE = 7 : LV B-Trees for shadow catalog
//  TCE = 8 : Primary B-trees for shadow catalog
//
//  TCE = 9 : Space B-trees for tableclass1
//  TCE = 10 : Index B-trees for tableclass1
//  TCE = 11: LV B-Trees for tableclass1
//  TCE = 12 : Primary B-trees for tableclass1
//
//  TCE = 13 : Space B-trees for tableclass2
//  TCE = 14 : Index B-trees for tableclass2
//  TCE = 15 : LV B-Trees for tableclass2
//  TCE = 16 : Primary B-trees for tableclass2
//
//  And so on.
//
//  TCE = 65 : Space B-trees for tableclass15
//  TCE = 66 : Index B-trees for tableclass15
//  TCE = 67 : LV B-Trees for tableclass15
//  TCE = 68 : Primary B-trees for tableclass15
//

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

    //  If there is no offset, no tableclass grbit
    //  was set...
    if ( 0 == tableClassOffset )
    {
        tableclass = tableclassNone;
    }
    else
    {
        //  ... otherwise calculate it given the tableclass public offset.
        tableclass = TABLECLASS( tableClassPublic + tableClassOffset - 1 );
    }

    return tableclass;
}

//  The starting offset in the Processor-Local-Storage.
extern LONG g_cbPlsMemRequiredPerPerfInstance;

const INT perfinstGlobal = 0;
static_assert( perfinstGlobal == 0, "Please, carefully review all references to perfinstGlobal because some of them use the symbolic const, but implicitly assume zero." );

#define CPerfinstMaxToUse( cpinstMaxT, ifmpMaxT )   ( max( max( cpinstMaxT, ifmpMaxT ), tceMax ) + 1 )
#define cPerfinstMax                                CPerfinstMaxToUse( g_cpinstMax, g_ifmpMax )

//  Maintains per-instance counters, though not necessarily ESE instances.
//
//  NOTE: iInstance == 0 is used by PERFInstanceDelayedTotal and PERFInstanceLiveTotal to retrieve (but never set)
//  the accumulated value os the aggregated type.
//
//  We may not want to hash certain counters, particularly those that only update the counter via the Set() method,
//  because as the number of procs increases, Set()/Get() become more and more expensive.
//
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
            //  register the counter and reserve space for it
            //  (rounding to a DWORD_PTR boundary)
            //
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

            //  The .Set hits all CPU hashes, so if this is commonly how this perf counter is updated, you 
            //  should unhash this perf counter.  We of course let through lValue == 0, as .Clear() uses 
            //  this value ... so this AssertSz() does not completely close the hole, if someone is repeatedly
            //  calling .Clear() - but reduces our chances of making this mistake by an order of magnitude.
            AssertSz( !fHashPerProc || lValue == 0, "Performance issue - .Set should not be used with hashed counters, except for resetting to 0." );

            Assert( iInstance >= 0 );
            Assert( (ULONG)iInstance < cPerfinstMax );
            SetData( 0, iInstance, lValue );
            if ( fHashPerProc )
            {
                const size_t cProcs = (size_t)OSSyncGetProcessorCountMax();
                for ( size_t iProc = 1; iProc < cProcs; iProc++ )
                {
                    //  WARNING: don't use this method with really hot counters, because
                    //  for lots of CPU's, we'll spend a lot of time spinning here
                    //
                    SetData( iProc, iInstance, TData(0) );
                }
            }
        }

        VOID Max( INT iInstance, const TData lValue )
        {
            //  with hashing this perf counter would get effectively multiplied by the number
            //  of CPUs
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
                //  nop
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
#endif //   PERFMON_SUPPORT
};


//  Maintains per-instance counters (TAggregation) and infers a global counter by accumulating the per-instance counters
//  at the time the global value is requested.
//  Note that a valid iInstance must not be numerically equal to perfinstGlobal because that is a special value
//  used to retrieve (but not set) the global counter.
//
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
            Assert( iInstance != perfinstGlobal ); // Can't alter global instance explicitly.
            Inc( pobj->m_iInstance );
        }

        using PERFInstance::Add;
        VOID Add( TAggregation const * const pobj, const TData lValue )
        {
            Assert( NULL != pobj );
            const INT iInstance = pobj->m_iInstance;
            Assert( iInstance != perfinstGlobal ); // Can't alter global instance explicitly.
            Add( pobj->m_iInstance, lValue );
        }

        using PERFInstance::Set;
        VOID Set( TAggregation const * const pobj, const TData lValue )
        {
            Assert( NULL != pobj );
            const INT iInstance = pobj->m_iInstance;
            Assert( iInstance != perfinstGlobal ); // Can't alter global instance explicitly.
            Set( pobj->m_iInstance, lValue );
        }

        using PERFInstance::Dec;
        VOID Dec( TAggregation const * const pobj )
        {
            Assert( NULL != pobj );
            const INT iInstance = pobj->m_iInstance;
            Assert( iInstance != perfinstGlobal ); // Can't alter global instance explicitly.
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
#endif //   PERFMON_SUPPORT
};


//  Maintains per-instance counters (TAggregation) and infers a global counter by accumulating the per-instance counters
//  as the individual instances are updated.
//  Note that a valid iInstance must not be numerically equal to perfinstGlobal because that is a special value
//  used to retrieve (but not set) the global counter.
//
//  So what is the difference between PERFInstanceDelayedTotal and PERFInstanceLiveTotal?
//  PERFInstanceDelayedTotal computes the global value as the simple sum of the data
//  from each instance while PERFInstanceLiveTotal manually updates the global
//  value as each instance value is changed.  PERF_COUNTER_RAWCOUNT typed
//  counters should use PERFInstanceDelayedTotal and PERF_COUNTER_COUNTER typed counters
//  should use PERFInstanceLiveTotal.  this is done so that when instances come
//  and go, the resulting global value is maintained correctly.  _RAWCOUNT
//  counters will lose the value represented by that instance and _COUNTER
//  counters will not change (or else they would cause a huge negative spike in
//  their derivatives which is what perfmon displays).
//
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
            //  we could have called into the base class to do the work but on very busy servers
            //  very hot codepaths can benefit from not doing an extra method call.
            //
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
            //  with hashing this perf counter would get effectively multiplied by the number
            //  of CPUs
            Assert( !fHashPerProc );

            //  we could have called into the base class to do the work but on very busy servers
            //  very hot codepaths can benefit from not doing an extra method call.
            //
            TData* const pPerfCounter = (TData*)GetDataBuffer( iInstance );
            if ( *pPerfCounter < lValue )
            {
                *pPerfCounter = lValue;
            }

            Assert( iInstance != perfinstGlobal ); // Double-setting global instance.

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
            Assert( iInstance != perfinstGlobal ); // Can't alter global instance explicitly.
            Inc( pobj->m_iInstance );
        }

        using PERFInstance::Add;
        VOID Add( TAggregation const * const pobj, const TData lValue )
        {
            Assert( NULL != pobj );
            const INT iInstance = pobj->m_iInstance;
            Assert( iInstance != perfinstGlobal ); // Can't alter global instance explicitly.
            Add( pobj->m_iInstance, lValue );
        }

        using PERFInstance::Set;
        VOID Set( TAggregation const * const pobj, const TData lValue )
        {
            Assert( NULL != pobj );
            const INT iInstance = pobj->m_iInstance;
            Assert( iInstance != perfinstGlobal ); // Can't alter global instance explicitly.
            Set( pobj->m_iInstance, lValue );
        }

        using PERFInstance::Dec;
        VOID Dec( TAggregation const * const pobj )
        {
            Assert( NULL != pobj );
            const INT iInstance = pobj->m_iInstance;
            Assert( iInstance != perfinstGlobal ); // Can't alter global instance explicitly.
            Dec( pobj->m_iInstance );
        }
#endif //   PERFMON_SUPPORT
};


    
//  PERFInstanceDelayedTotalWithClass is a helper class to maintain a performance counter with instance specific values
//  as well as an additional class values to categorize the values upon. Internally it maintains one instance of
//  PERFInstanceDelayedTotal for TAggregation and one of PERFInstance for the additional class and re-routes calls to
//  either one as appropriate.
//  Note that a valid iInstance must not be numerically equal to perfinstGlobal because that is a special value
//  used to retrieve (but not set) the global counter.
//
template < class TData = LONG, class TAggregation = INST, INT cClassCount = 1, BOOL fHashPerProc = fTrue >
class PERFInstanceDelayedTotalWithClass
{
#ifdef PERFMON_SUPPORT
    static_assert( cClassCount > 0, "If you are aggregating 0 classes, then you don't need this object." );
    static_assert( cClassCount <= 2, "This function needs to be updated to cover a different number, look for layering violation comments below." );

    private:
        PERFInstanceDelayedTotal<TData, TAggregation, fHashPerProc> cInstance;  //  to maintain instance specific counter values
        PERFInstance<TData, fHashPerProc> cClass[ cClassCount ];                //  to maintain class specific counter values

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
            Assert( iInstance != perfinstGlobal );  //  this will cause this data point to be dropped at the instance-level aggegation.
            Assert( iInstanceClass0 >= 0 ); //  first class should always exist.
            Assert( ( iInstanceClass1 >= 0 ) || ( cClassCount < 2 ) );  //  have to add to all classes.
            Assert( ( iInstanceClass1 < 0 ) || ( cClassCount > 1 ) );   //  too few classes.

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
            //  Note this is the last layering violation remaining in this libary (in addition some asserts,
            //  but those can be removed for the sake of removing the layering violation if we can ever
            //  get there).
            //
            //  The architecturally correct way to do this would be to have different perf counter callback
            //  functions for each of the perf counter categories. Each of those callbacks would then
            //  call a different method or use different parameters to retrieve the specific value that they
            //  wish. This is doable and just a matter of mechanical work, but it would be too time consuming,
            //  so we'll leave that for some other day.
            //
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
                //  TableClasses is always the first class.
                return GetClass( 0, iInstance );
            }
            else if ( dwPERFCurPerfObj == PERF_OBJECT_DATABASES )
            {
                //  Databases is the second class if more than one class is used.
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
#endif //   PERFMON_SUPPORT
};

//  PERFInstanceLiveTotalWithClass is a helper class to maintain a performance counter with instance specific values
//  as well as an additional class values to categorize the values upon. Internally it maintains one instance of
//  PERFInstanceLiveTotal for TAggregation and one of PERFInstance for the additional class and re-routes calls to
//  either one as appropriate.
//  Note that a valid iInstance must not be numerically equal to perfinstGlobal because that is a special value
//  used to retrieve (but not set) the global counter.
//
template < class TData = LONG, class TAggregation = INST, INT cClassCount = 1, BOOL fHashPerProc = fTrue >
class PERFInstanceLiveTotalWithClass
{
#ifdef PERFMON_SUPPORT
    static_assert( cClassCount > 0, "If you are aggregating 0 classes, then you don't need this object." );
    static_assert( cClassCount <= 2, "This function needs to be updated to cover a different number, look for layering violation comments below." );

    private:
        PERFInstanceLiveTotal<TData, TAggregation, fHashPerProc> cInstance; //  to maintain instance specific counter values
        PERFInstance<TData, fHashPerProc> cClass[ cClassCount ];            //  to maintain class specific counter values

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
            Assert( iInstance != perfinstGlobal );  //  this will cause this data point to be dropped at the instance-level aggegation.
            Assert( iInstanceClass0 >= 0 ); //  first class should always exist.
            Assert( ( iInstanceClass1 >= 0 ) || ( cClassCount < 2 ) );  //  have to add to all classes.
            Assert( ( iInstanceClass1 < 0 ) || ( cClassCount > 1 ) );   //  too few classes.

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
            Assert( iInstanceClass0 >= 0 ); //  first class should always exist.
            Assert( ( iInstanceClass1 >= 0 ) || ( cClassCount < 2 ) );  //  have to add to all classes.
            Assert( ( iInstanceClass1 < 0 ) || ( cClassCount > 1 ) );   //  too few classes.
            
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
            //  See comment in PERFInstanceDelayedTotalWithClass::Get() for how to remove the layering
            //  violation below.
            //
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
                //  TableClasses is always the first class.
                return GetClass( 0, iInstance );
            }
            else if ( dwPERFCurPerfObj == PERF_OBJECT_DATABASES )
            {
                //  Databases is the second class if more than one class is used.
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
#endif //   PERFMON_SUPPORT
};


#define PERFIncCounterTable( x, y, z )  \
{                                       \
    x.Inc( y, z );                      \
}

#endif //   PERFCTRS_H_INCLUDED

