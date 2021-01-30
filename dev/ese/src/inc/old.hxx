// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

class OLD_STATUS_
{
    protected:
        OLD_STATUS_() : m_asig( CSyncBasicInfo( _T( "asigOLD" ) ) )
                                                        { Reset_(); }

    public:
        virtual ~OLD_STATUS_()                          { return; }

    private:
        CAutoResetSignal    m_asig;
        THREAD              m_thread;

        union
        {
            ULONG           m_ulFlags;
            struct
            {
                FLAG32      m_fTermRequested:1;
                FLAG32      m_fAvailExtOnly:1;
                FLAG32      m_fNoPartialMerges:1;
            };
        };

        ULONG               m_cPasses;
        ULONG               m_cPassesMax;
        ULONG_PTR           m_csecStart;
        ULONG_PTR           m_csecEvent;
        ULONG_PTR           m_csecMax;
        JET_CALLBACK        m_callback;

    public:
        VOID Reset_();

        VOID SetSignal()                                { m_asig.Set(); }
        BOOL FWaitOnSignal( const INT cmsecTimeout )    { return m_asig.FWait( cmsecTimeout ); }

        ERR ErrThreadCreate(
                const IFMP ifmp,
                const PUTIL_THREAD_PROC pfn );
        VOID ThreadEnd();
        BOOL FRunning() const                           { return ( NULL != m_thread ); }

        VOID SetFTermRequested()                    { m_fTermRequested = fTrue; }
        BOOL FTermRequested() const                 { return m_fTermRequested; }

        VOID SetFAvailExtOnly()                     { m_fAvailExtOnly = fTrue; }
        BOOL FAvailExtOnly() const                      { return m_fAvailExtOnly; }

        VOID SetFNoPartialMerges()                      { m_fNoPartialMerges = fTrue; }
        BOOL FNoPartialMerges() const                   { return m_fNoPartialMerges; }
        
        VOID IncCPasses()                               { m_cPasses++; }
        ULONG CPasses() const                           { return m_cPasses; }

        VOID SetCPassesMax( const ULONG cPasses )       { m_cPassesMax = cPasses; }
        BOOL FReachedMaxPasses() const              { return ( 0 != m_cPassesMax && m_cPasses >= m_cPassesMax ); }
        BOOL FInfinitePasses() const                    { return ( 0 == m_cPassesMax ); }

        VOID SetCsecStart( const ULONG_PTR csec )       { m_csecStart = csec; }
        ULONG_PTR CsecStart() const                     { return m_csecStart; }

        VOID UpdateCsecEvent( VOID )                    { m_csecEvent = UlUtilGetSeconds( ); }
        ULONG_PTR CsecEvent( ) const                    { return m_csecEvent; }

        VOID SetCsecMax( const ULONG_PTR csec )     { m_csecMax = csec; }
        BOOL FReachedMaxElapsedTime() const     { return ( 0 != m_csecMax && UlUtilGetSeconds() > m_csecMax ); }

        VOID SetCallback( const JET_CALLBACK callback ) { m_callback = callback; }
        JET_CALLBACK Callback() const                   { return m_callback; }
};

INLINE VOID OLD_STATUS_::Reset_()
{
    m_thread = NULL;
    m_ulFlags = 0;
    m_cPasses = 0;
    m_cPassesMax = 0;
    m_csecStart = 0;
    m_csecEvent  = 0;
    m_csecMax = 0;
    m_callback = NULL;
}

INLINE ERR OLD_STATUS_::ErrThreadCreate( const IFMP ifmp, const PUTIL_THREAD_PROC pfn )
{
    Assert( !FRunning() );

    const ERR   err     = ErrUtilThreadCreate(
                                pfn,
                                0,
                                priorityNormal,
                                &m_thread,
                                (DWORD_PTR)ifmp );

    Assert( ( err >= 0 && FRunning() )
        || ( err < 0 && !FRunning() ) );
    return err;
}

INLINE VOID OLD_STATUS_::ThreadEnd()
{
    Assert( FRunning() );

    UtilThreadEnd( m_thread );
    m_thread = NULL;

    Assert( !FRunning() );
}


class OLDDB_STATUS : public OLD_STATUS_
{
    public:
        OLDDB_STATUS();
        ~OLDDB_STATUS()                                 { return; }

    private:

        ULONG_PTR           m_cTasksDispatched;

    public:
        VOID Reset( INST * const pinst );

        ULONG_PTR CTasksDispatched() const              { return m_cTasksDispatched; }
        VOID IncrementCTasksDispatched()                { m_cTasksDispatched++; }
};

INLINE OLDDB_STATUS::OLDDB_STATUS()
{
}

typedef USHORT      DEFRAGTYPE;
const DEFRAGTYPE    defragtypeNull      = 0;
const DEFRAGTYPE    defragtypeTable     = 1;
const DEFRAGTYPE    defragtypeLV        = 2;
const DEFRAGTYPE    defragtypeSpace     = 3;
const DEFRAGTYPE    defragtypeIndex     = 4;
const DEFRAGTYPE    defragtypeAll       = 5;

typedef USHORT      DEFRAGPASS;
const DEFRAGPASS    defragpassNull      = 0;
const DEFRAGPASS    defragpassFull      = 1;
const DEFRAGPASS    defragpassPartial   = 2;
const DEFRAGPASS    defragpassCompleted = 3;

class DEFRAG_STATUS
{
    public:
        DEFRAG_STATUS();
        ~DEFRAG_STATUS()    { return; }

    private:
        DEFRAGTYPE  m_defragtype;
        DEFRAGPASS  m_defragpass;
        CPG         m_cpgVisited;
        CPG         m_cpgFreed;
        CPG         m_cpgPartialMerges;
        OBJID       m_objidCurrentTable;
        ULONG       m_cbCurrentKey;
        BYTE        m_rgbCurrentKey[cbKeyAlloc];

        __int64     m_startTime;

#ifndef RTM
    private:
        VOID Test();
#endif

    public:
        DEFRAGTYPE DefragType() const;
        VOID SetType( const DEFRAGTYPE defragtype );
        
        BOOL FTypeNull() const;
        VOID SetTypeNull();
        
        BOOL FTypeTable() const;
        VOID SetTypeTable();
        
        BOOL FTypeLV() const;
        VOID SetTypeLV();

        BOOL FTypeIndex() const;
        VOID SetTypeIndex();
        
        BOOL FTypeSpace() const;
        VOID SetTypeSpace();

        BOOL FPassNull() const;
        VOID SetPassNull();

        BOOL FPassFull() const;
        VOID SetPassFull();

        BOOL FPassPartial() const;
        VOID SetPassPartial();

        BOOL FPassCompleted() const;
        VOID SetPassCompleted();

        CPG CpgVisited() const;
        VOID IncrementCpgVisited();
        VOID ResetCpgVisited();

        CPG CpgFreed() const;
        VOID IncrementCpgFreed();
        VOID ResetCpgFreed();

        CPG CpgPartialMerges() const;
        VOID IncrementCpgPartialMerges();
        VOID ResetCpgPartialMerges();
        
        OBJID ObjidCurrentTable() const;
        VOID SetObjidCurrentTable( const OBJID objidFDP );
        VOID SetObjidNextTable();

        ULONG CbCurrentKey() const;
        VOID SetCbCurrentKey( const ULONG cb );
        BYTE *RgbCurrentKey() const;

        __int64 StartTime() const;
        VOID SetStartTime( const __int64 startTime );
};

INLINE DEFRAG_STATUS::DEFRAG_STATUS()
{
    m_defragtype = defragtypeNull;
    m_defragpass = defragpassNull;
    m_cbCurrentKey = 0;
    SetStartTime(0);
    ResetCpgVisited();
    ResetCpgFreed();
    ResetCpgPartialMerges();

#ifndef RTM
    Test();
#endif
}

#ifndef RTM

INLINE VOID DEFRAG_STATUS::Test()
{
    static BOOL fTested = false;

    if(!fTested)
    {
        AssertRTL( 0 == CpgVisited() );
        AssertRTL( 0 == CpgFreed() );
        AssertRTL( 0 == CpgPartialMerges() );
        AssertRTL( 0 == StartTime() );

        SetStartTime( 0x1234 );
        AssertRTL( 0x1234 == StartTime() );
        
        IncrementCpgVisited();
        IncrementCpgVisited();
        IncrementCpgPartialMerges();
        IncrementCpgVisited();
        IncrementCpgPartialMerges();
        IncrementCpgFreed();
        AssertRTL( 3 == CpgVisited() );
        AssertRTL( 2 == CpgPartialMerges() );
        AssertRTL( 1 == CpgFreed() );

        ResetCpgVisited();
        ResetCpgFreed();
        ResetCpgPartialMerges();
        SetStartTime( 0 );

        AssertRTL( 0 == CpgVisited() );
        AssertRTL( 0 == CpgFreed() );
        AssertRTL( 0 == CpgPartialMerges() );
        AssertRTL( 0 == StartTime() );
        fTested = fTrue;
    }
}

#endif

INLINE DEFRAGTYPE DEFRAG_STATUS::DefragType() const
{
    return m_defragtype;
}
INLINE VOID DEFRAG_STATUS::SetType( const DEFRAGTYPE defragtype )
{
    Assert( defragtypeNull == defragtype
        || defragtypeTable == defragtype
        || defragtypeIndex == defragtype
        || defragtypeLV == defragtype );
    m_defragtype = defragtype;
}

INLINE BOOL DEFRAG_STATUS::FTypeNull() const
{
    return ( defragtypeNull == m_defragtype );
}
INLINE VOID DEFRAG_STATUS::SetTypeNull()
{
    m_defragtype = defragtypeNull;
    m_cbCurrentKey = 0;
}

INLINE BOOL DEFRAG_STATUS::FTypeTable() const
{
    return ( defragtypeTable == m_defragtype );
}
INLINE VOID DEFRAG_STATUS::SetTypeTable()
{
    m_defragtype = defragtypeTable;
}

INLINE BOOL DEFRAG_STATUS::FTypeLV() const
{
    return ( defragtypeLV == m_defragtype );
}
INLINE VOID DEFRAG_STATUS::SetTypeLV()
{
    m_defragtype = defragtypeLV;
}

INLINE BOOL DEFRAG_STATUS::FTypeIndex() const
{
    return ( defragtypeIndex == m_defragtype );
}
INLINE VOID DEFRAG_STATUS::SetTypeIndex()
{
    m_defragtype = defragtypeIndex;
}

INLINE BOOL DEFRAG_STATUS::FTypeSpace() const
{
    return ( defragtypeSpace == m_defragtype );
}
INLINE VOID DEFRAG_STATUS::SetTypeSpace()
{
    m_defragtype = defragtypeSpace;
}

INLINE BOOL DEFRAG_STATUS::FPassNull() const
{
    return ( defragpassNull == m_defragpass );
}
INLINE VOID DEFRAG_STATUS::SetPassNull()
{
    m_defragpass = defragpassNull;
}

INLINE BOOL DEFRAG_STATUS::FPassFull() const
{
    return ( defragpassFull == m_defragpass );
}
INLINE VOID DEFRAG_STATUS::SetPassFull()
{
    m_defragpass = defragpassFull;
}

INLINE BOOL DEFRAG_STATUS::FPassPartial() const
{
    return ( defragpassPartial == m_defragpass );
}
INLINE VOID DEFRAG_STATUS::SetPassPartial()
{
    m_defragpass = defragpassPartial;
}

INLINE BOOL DEFRAG_STATUS::FPassCompleted() const
{
    return ( defragpassCompleted == m_defragpass );
}
INLINE VOID DEFRAG_STATUS::SetPassCompleted()
{
    m_defragpass = defragpassCompleted;
}

INLINE CPG DEFRAG_STATUS::CpgVisited() const
{
    return m_cpgVisited;
}
INLINE VOID DEFRAG_STATUS::IncrementCpgVisited()
{
    m_cpgVisited++;
}
INLINE VOID DEFRAG_STATUS::ResetCpgVisited()
{
    m_cpgVisited = 0;
}

INLINE CPG DEFRAG_STATUS::CpgFreed() const
{
    return m_cpgFreed;
}
INLINE VOID DEFRAG_STATUS::IncrementCpgFreed()
{
    m_cpgFreed++;
}
INLINE VOID DEFRAG_STATUS::ResetCpgFreed()
{
    m_cpgFreed = 0;
}

INLINE CPG DEFRAG_STATUS::CpgPartialMerges() const
{
    return m_cpgPartialMerges;
}
INLINE VOID DEFRAG_STATUS::IncrementCpgPartialMerges()
{
    m_cpgPartialMerges++;
}
INLINE VOID DEFRAG_STATUS::ResetCpgPartialMerges()
{
    m_cpgPartialMerges = 0;
}

INLINE __int64 DEFRAG_STATUS::StartTime() const
{
    return m_startTime;
}

INLINE VOID DEFRAG_STATUS::SetStartTime(const __int64 startTime)
{
    m_startTime = startTime;
}

INLINE OBJID DEFRAG_STATUS::ObjidCurrentTable() const
{
    Assert( m_objidCurrentTable >= objidFDPMSO );
    return m_objidCurrentTable;
}
INLINE VOID DEFRAG_STATUS::SetObjidCurrentTable( const OBJID objidFDP )
{
    Assert( objidFDP >= objidFDPMSO );
    m_objidCurrentTable = objidFDP;
}
INLINE VOID DEFRAG_STATUS::SetObjidNextTable()
{
    Assert( m_objidCurrentTable >= objidFDPMSO );
    m_objidCurrentTable++;
}

INLINE ULONG DEFRAG_STATUS::CbCurrentKey() const
{
    Assert( m_cbCurrentKey <= cbKeyMostMost );
    return m_cbCurrentKey;
}
INLINE VOID DEFRAG_STATUS::SetCbCurrentKey( const ULONG cb )
{
    Assert( cb <= cbKeyMostMost );
    m_cbCurrentKey = cb;
}
INLINE BYTE *DEFRAG_STATUS::RgbCurrentKey() const
{
    return (BYTE *)m_rgbCurrentKey;
}

INLINE BOOL FOLDPauseMark( void )
{
    const LONG lCacheCleanMark = 10;
    const LONG lCacheSizeMark = 50;
    const LONG lCachePinnedMark = 50;

    return LBFICacheCleanPercentage() < lCacheCleanMark &&
        lCacheSizeMark < LBFICacheSizePercentage() &&
        lCachePinnedMark < LBFICachePinnedPercentage();
}

INLINE BOOL FOLDResumeMark( void )
{
    const LONG lCacheCleanMark = 20;
    const LONG lCachePinnedMark = 40;

    return lCacheCleanMark <= LBFICacheCleanPercentage() &&
        LBFICachePinnedPercentage() < lCachePinnedMark;
}


VOID    OLDTermInst( INST *pinst );
VOID    OLDTermFmp( const IFMP ifmp );

BOOL    FOLDRunning( const IFMP ifmp );

ERR     ErrOLD2Resume( __in PIB * const ppib, const IFMP ifmp );
VOID    OLD2TermFmp( const IFMP ifmp );
VOID    OLD2TermInst( INST * const pinst );
ERR     ErrOLDRegisterObjectForOLD2(
    _In_ const IFMP ifmp,
    _In_z_ const CHAR * const szTable,
    _In_z_ const CHAR * const szIndex,
    _In_ DEFRAGTYPE defragtype );

BOOL    FOLDSystemTable( const CHAR * const szTableName );
ERR     ErrOLDDumpMSysDefrag( __in PIB * const ppib, const IFMP ifmp );
ERR     ErrOLDDefragment(
            const IFMP  ifmp,
            const CHAR  *szTableName,
            ULONG       *pcPasses,
            ULONG       *pcsec,
            JET_CALLBACK callback,
            JET_GRBIT   grbit );

ERR ErrOLDInit();
VOID OLDTerm();

