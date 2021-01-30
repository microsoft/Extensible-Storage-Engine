// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _DHT_HXX_INCLUDED
#define _DHT_HXX_INCLUDED


#pragma warning ( disable : 4200 )



#ifndef RFS2


#define FOSSetCleanupState( fInCleanupState ) (fInCleanupState)

#endif


#ifdef DHTAssert
#else
#define DHTAssert Assert
#endif


#include <sync.hxx>


#ifdef DEBUG


#define UNIQUE_BUCKET_NAMES

#ifdef UNIQUE_BUCKET_NAMES
#include <stdio.h>
#endif

#endif

#ifdef DEBUGGER_EXTENSION
class CPRINTF;
#endif


NAMESPACE_BEGIN( DHT );



template< class CKey, class CEntry >
class CDynamicHashTable
{
    public:


        typedef ULONG_PTR NativeCounter;


        class CKeyEntry
        {
            public:


                static NativeCounter Hash( const CKey& key );


                NativeCounter Hash() const;


                BOOL FEntryMatchesKey( const CKey& key ) const;


                void SetEntry( const CEntry& entry );


                void GetEntry( CEntry* const pentry ) const;

            public:
                CEntry  m_entry;
                ~CKeyEntry();

            private:

                CKeyEntry();
                CKeyEntry *operator =( const CKeyEntry & );
        };


        enum class ERR
        {
            errSuccess,
            errOutOfMemory,
            errInvalidParameter,
            errEntryNotFound,
            errNoCurrentEntry,
            errKeyDuplicate,
        };


        class CLock;

    public:

        CDynamicHashTable( const NativeCounter rankDHTrwlBucket );
        ~CDynamicHashTable();

        ERR     ErrInit(    const double        dblLoadFactor,
                            const double        dblUniformity,
                            const NativeCounter cBucketMinimum = 0 );
        void    Term();

        void    ReadLockKey( const CKey& key, CLock* const plock );
        void    ReadUnlockKey( CLock* const plock );

        void    WriteLockKey( const CKey& key, CLock* const plock );
        BOOL    FTryWriteLockKey( const CKey& key, CLock* const plock, BOOL fCanFail = fTrue );
        void    WriteUnlockKey( CLock* const plock );

        ERR     ErrRetrieveEntry( CLock* const plock, CEntry* const pentry );
        ERR     ErrReplaceEntry( CLock* const plock, const CEntry& entry );
        ERR     ErrInsertEntry( CLock* const plock, const CEntry& entry );
        ERR     ErrDeleteEntry( CLock* const plock );

        void    BeginHashScan( CLock* const plock );
        void    BeginHashScanFromKey( const CKey& key, CLock* const plock );
        ERR     ErrMoveNext( CLock* const plock, BOOL* const pfNewBucket = NULL );
        void    EndHashScan( CLock* const plock );

#ifdef DEBUGGER_EXTENSION
        VOID    Dump( CPRINTF * pcprintf, const DWORD_PTR dwOffset = 0 ) const;
        VOID    Scan( CPRINTF * pcprintf, VOID * pv ) const;
#endif

#ifdef DHT_STATS
        LONG    CBucketOverflow() const     { return m_cBucketOverflowInsert; }
        LONG    CBucketSplit() const        { return m_cBucketSplit; }
        LONG    CBucketMerge() const        { return m_cBucketMerge; }
        LONG    CDirectorySplit() const     { return m_cDirSplit; }
        LONG    CDirectoryMerge() const     { return m_cDirMerge; }
        LONG    CStateTransition() const    { return m_cTransition; }
        LONG    CPolicySelection() const    { return m_cSelection; }
        LONG    CSplitContend() const       { return m_cSplitContend; }
        LONG    CMergeContend() const       { return m_cMergeContend; }
#else
        LONG    CBucketOverflow() const     { return 0; }
        LONG    CBucketSplit() const        { return 0; }
        LONG    CBucketMerge() const        { return 0; }
        LONG    CDirectorySplit() const     { return 0; }
        LONG    CDirectoryMerge() const     { return 0; }
        LONG    CStateTransition() const    { return 0; }
        LONG    CPolicySelection() const    { return 0; }
        LONG    CSplitContend() const       { return 0; }
        LONG    CMergeContend() const       { return 0; }
#endif

        BOOL    FHasCurrentRecord( CLock* const plock) const
    {
            if ( plock->m_pEntry )
                return fTrue;
            else
                return fFalse;
    }

    private:


        enum ENUMSTATE
        {
            stateNil                = 0,
            stateShrinkFromGrow     = 1,
            stateShrinkFromGrow2    = 2,
            stateGrowFromShrink     = 3,
            stateGrowFromShrink2    = 4,
            stateSplitFromGrow      = 5,
            stateSplitFromGrow2     = 6,
            stateGrowFromSplit      = 7,
            stateGrowFromSplit2     = 8,
            stateMergeFromShrink    = 9,
            stateMergeFromShrink2   = 10,
            stateShrinkFromMerge    = 11,
            stateShrinkFromMerge2   = 12,
            stateUnused             = 13,
            stateGrow               = 14,
            stateShrink             = 15,
            stateSplit              = 16,
            stateMerge              = 17,
        };



        enum { cbitByte             = 8 };
        enum { cbitNativeCounter    = sizeof( NativeCounter ) * cbitByte };



        struct BUCKET
        {
            public:

                BUCKET() {}
                ~BUCKET() {}


                union
                {
                    BYTE            m_rgbRWL[ sizeof( OSSYNC::CReaderWriterLock ) ];
                    BUCKET          *m_pBucketPrev;
                };


                union
                {
                    BYTE            *m_pb;
                    BUCKET          *m_pBucketNext;
                    CKeyEntry       *m_pEntryLast;
                };


                CKeyEntry           m_rgEntry[];

            public:


                OSSYNC::CReaderWriterLock& CRWL() const
                {
                    return (OSSYNC::CReaderWriterLock &)m_rgbRWL;
                }
        };
        typedef BUCKET* PBUCKET;



        class BUCKETPool
        {
            public:

                PBUCKET             m_pReserve;
                LONG                m_cReserve;
                OSSYNC::CSemaphore  m_semReserve;
#ifdef _WIN64
                BYTE                m_rgbRsvd[ 40 ];
#else
                BYTE                m_rgbRsvd[ 20 ];
#endif

            public:

                BUCKETPool()
                    :   m_semReserve( CSyncBasicInfo( "CDynamicHashTable::BUCKETPool::m_semReserve" ) )
                {


                    m_pReserve = NULL;
                    m_cReserve = 0;


                    m_semReserve.Release();

#ifdef DEBUG
                    memset( m_rgbRsvd, 0, sizeof( m_rgbRsvd ) );
#endif
                }



                ~BUCKETPool()
                {
                    while ( m_pReserve )
                    {
                        PBUCKET pBucket;

                        pBucket = m_pReserve;
                        m_pReserve = m_pReserve->m_pBucketNext;
                        MEMFree( pBucket );
                    }
                    m_cReserve = 0;
                }



                BOOL FPOOLReserve( const NativeCounter cbBucket )
                {

                    if ( AtomicDecrement( (LONG*)&m_cReserve ) >= 0 )
                    {
                        return fTrue;
                    }


                    else
                    {
                        return FPOOLReserve_( cbBucket );
                    }
                }

                BOOL FPOOLReserve_( const NativeCounter cbBucket )
                {

                    AtomicIncrement( (LONG*)&m_cReserve );


                    const PBUCKET pBucket = PBUCKET( PvMEMAlloc( cbBucket ) );

                    if ( pBucket )
                    {


                        m_semReserve.Acquire();
                        pBucket->m_pBucketNext = m_pReserve;
                        m_pReserve = pBucket;
                        m_semReserve.Release();


                        return fTrue;
                    }


                    return fFalse;
                }



                BUCKET *PbucketPOOLCommit()
                {
                    PBUCKET pBucketReserve;


                    m_semReserve.Acquire();
                    pBucketReserve = m_pReserve;
                    DHTAssert( pBucketReserve );
                    m_pReserve = m_pReserve->m_pBucketNext;
                    m_semReserve.Release();


                    return pBucketReserve;
                }



                void POOLUnreserve()
                {


                    AtomicIncrement( (LONG*)&m_cReserve );
                }
        };



        struct HOTSTUFF
        {
            public:

                NativeCounter               m_cEntry;
                NativeCounter               m_cOp;
                OSSYNC::CMeteredSection     m_cms;
#ifdef _WIN64
                BYTE                        m_rgbRsvd[ 24 ];
#else
                BYTE                        m_rgbRsvd[ 12 ];
#endif

                BUCKETPool                  m_bucketpool;

                HOTSTUFF()
                    :   m_cms()
                {
                    m_cEntry    = 0;
                    m_cOp       = 0;
#ifdef DEBUG
                    memset( m_rgbRsvd, 0, sizeof( m_rgbRsvd ) );
#endif
                }

        };



        struct DIRPTRS
        {
            NativeCounter       m_cBucketMax;
            NativeCounter       m_cBucket;
#ifdef _WIN64
            BYTE                m_rgbRsvd[ 16 ];
#else
            BYTE                m_rgbRsvd[ 8 ];
#endif
        };



    public:

        class CLock
        {

            friend class CDynamicHashTable< CKey, CEntry >;

            public:


                enum ENUMLOCKSTATE
                {
                    lsNil   = 0,
                    lsRead  = 1,
                    lsWrite = 2,
                    lsScan  = 3,
                };

            public:

                CLock()
                {
                    m_ls = lsNil;
                    m_pBucketHead = NULL;
                }


                ~CLock()
                {
                    DHTAssert( m_pBucketHead == NULL );
                }

                BOOL IsSameBucketHead(CLock *pLock)
                {
                    if ( pLock->m_pBucketHead == this->m_pBucketHead )
                        return fTrue;
                    else
                        return fFalse;
                }

            private:


                ENUMLOCKSTATE       m_ls;
                BOOL                m_fInsertOrDelete;


                HOTSTUFF            *m_phs;

#ifdef DEBUG

                CKey                m_key;
                NativeCounter       m_iHash;
                NativeCounter       m_iBucketDebug;
#endif


                BUCKET              *m_pBucketHead;


                BUCKET              *m_pBucket;


                CKeyEntry           *m_pEntryPrev;
                CKeyEntry           *m_pEntry;
                CKeyEntry           *m_pEntryNext;


                NativeCounter       m_iBucket;
        };




        const INT UiSTEnter( HOTSTUFF **pphs )
        {

            *pphs = HOTSTUFFHash();


            return ( *pphs )->m_cms.Enter();
        }


        void STLeave( const INT group, HOTSTUFF *phs )
        {
            phs->m_cms.Leave( group );
        }


        const ENUMSTATE EsSTGetState() const
        {
            return m_stateCur;
        }


        void STTransition( const ENUMSTATE esNew )
        {

            m_stateCur = esNew;

            m_cCompletions = 0;
            for ( NativeCounter ihs = 0; ihs < m_chs; ihs++ )
            {
                m_rghs[ ihs ].m_cms.Partition( OSSYNC::CMeteredSection::PFNPARTITIONCOMPLETE( STCompletion_ ), DWORD_PTR( this ) );
            }
        }


        static void STCompletion_( CDynamicHashTable< CKey, CEntry >* pdht )
        {
            pdht->STCompletion();
        }


        void STCompletion()
        {

            typedef void (CDynamicHashTable::*PfnCompletion)();

            struct StateTransitionTable
            {
                PfnCompletion   m_pfnCompletion;
                ENUMSTATE       m_stNext;
            };

            static const StateTransitionTable rgstt[] =
            {
                  { NULL,                                         stateNil,               },
                  { NULL,                                         stateShrinkFromGrow2,   },
                  { NULL,                                         stateShrink,            },
                  { NULL,                                         stateGrowFromShrink2,   },
                  { NULL,                                         stateGrow,              },
                  { NULL,                                         stateSplitFromGrow2,    },
                  { &CDynamicHashTable::STCompletionCopyDir,      stateSplit,             },
                  { NULL,                                         stateGrowFromSplit2,    },
                  { NULL,                                         stateGrow,              },
                  { NULL,                                         stateMergeFromShrink2,  },
                  { &CDynamicHashTable::STCompletionCopyDir,      stateMerge,             },
                  { NULL,                                         stateShrinkFromMerge2,  },
                  { NULL,                                         stateShrink,            },
                  { NULL,                                         stateNil,               },
                  { &CDynamicHashTable::STCompletionGrowShrink,   stateNil,               },
                  { &CDynamicHashTable::STCompletionGrowShrink,   stateNil,               },
                  { &CDynamicHashTable::STCompletionSplit,        stateGrowFromSplit,     },
                  { &CDynamicHashTable::STCompletionMerge,        stateShrinkFromMerge,   },
            };


            if ( NativeCounter( AtomicIncrement( &m_cCompletions ) ) >= m_chs )
            {
                STATStateTransition();


                const ENUMSTATE esCurrent = EsSTGetState();


                if ( rgstt[ esCurrent ].m_pfnCompletion )
                {
                    (this->*rgstt[ esCurrent ].m_pfnCompletion)();
                }


                if ( rgstt[ esCurrent ].m_stNext )
                {
                    STTransition( rgstt[ esCurrent ].m_stNext );
                }
            }
        }


        void STCompletionCopyDir()
        {

            memcpy( &m_dirptrs[ 1 ], &m_dirptrs[ 0 ], sizeof( DIRPTRS ) );
        }


        void STCompletionGrowShrink()
        {

            m_semPolicy.Release();
        }


        void STCompletionSplit()
        {

            DIRISplit();
        }


        void STCompletionMerge()
        {

            DIRIMerge();
        }







        ERR ErrDIRInit( const NativeCounter cLoadFactor, const NativeCounter cbucketMin )
        {
            ERR             err;
            NativeCounter   iExponent;
            NativeCounter   iRemainder;


            if ( cLoadFactor < 1 )
            {
                return ERR::errInvalidParameter;
            }


            m_cLoadFactor   = cLoadFactor;


            m_cbBucket  =   sizeof( BUCKET ) + ( cLoadFactor * 2 + 1 ) * sizeof( CKeyEntry );
            m_cbBucket  =   ( ( m_cbBucket + cbCacheLine - 1 ) / cbCacheLine ) * cbCacheLine;


            m_centryBucket  = ( m_cbBucket - sizeof( BUCKET ) ) / sizeof( CKeyEntry );


            m_cbucketMin    = max( cbucketMin, NativeCounter( OSSYNC::OSSyncGetProcessorCountMax() ) );
            m_cbucketMin    = max( m_cbucketMin, 2 );


            DIRILog2( m_cbucketMin, &iExponent, &iRemainder );

            if ( iRemainder )
            {
                if ( ++iExponent >= cbitNativeCounter )
                {
                    return ERR::errInvalidParameter;
                }
            }

            m_cbucketMin    = (ULONG_PTR)1 << iExponent;


            m_dirptrs[ 0 ].m_cBucketMax = m_cbucketMin / 2;
            m_dirptrs[ 0 ].m_cBucket    = m_cbucketMin / 2;


            err = ErrDIRInitBucketArray( 2, 0, &m_rgrgBucket[ 0 ] );
            if ( ERR::errSuccess != err )
            {
                return err;
            }


            for ( iExponent = 1; ( NativeCounter( 1 ) << iExponent ) < m_cbucketMin; iExponent++ )
            {
                err = ErrDIRInitBucketArray( NativeCounter( 1 ) << iExponent, NativeCounter( 1 ) << iExponent, &m_rgrgBucket[ iExponent ] );
                if ( ERR::errSuccess != err )
                {
                    return err;
                }
            }


            memset( &m_dirptrs[ 1 ], 0, sizeof( DIRPTRS ) );

            return ERR::errSuccess;
        }



        void DIRTerm()
        {
            NativeCounter   iExponent;


            if ( m_rgrgBucket[ 0 ] )
            {
                DIRTermBucketArray( m_rgrgBucket[ 0 ], 2 );
                m_rgrgBucket[ 0 ] = NULL;
            }


            for ( iExponent = 1; iExponent < cbitNativeCounter; iExponent++ )
            {
                if ( m_rgrgBucket[ iExponent ] )
                {
                    DIRTermBucketArray( m_rgrgBucket[ iExponent ], NativeCounter( 1 ) << iExponent );
                    m_rgrgBucket[ iExponent ] = NULL;
                }
            }


            memset( m_dirptrs, 0, sizeof( m_dirptrs ) );
        }



        void DIRReadLockKey( const ENUMSTATE esCurrent, const CKey &key, CLock * const plock ) const
        {
            NativeCounter   iHash;
            NativeCounter   iBucket;
            NativeCounter   cBucketBefore;
            NativeCounter   cBucketAfter;
            NativeCounter   cBucketMax;


            DHTAssert( FBKTRead( plock ) );
            DHTAssert( plock->m_pBucketHead == NULL );

#ifdef DEBUG

            plock->m_key    = key;
#endif

            NativeCounter cBucketMaxBefore = NcDIRIGetBucketMax( esCurrent );

            iHash                   = CKeyEntry::Hash( key );
            plock->m_pBucketHead    = PbucketDIRIHash( esCurrent, iHash, &iBucket, &cBucketBefore );


            plock->m_pBucketHead->CRWL().EnterAsReader();


            cBucketAfter    = NcDIRIGetBucket( esCurrent );
            cBucketMax      = NcDIRIGetBucketMax( esCurrent );

            DHTAssert( cBucketMax == cBucketMaxBefore );
            
            if (    cBucketBefore != cBucketAfter &&
                    (   cBucketBefore <= iBucket && iBucket < cBucketAfter ||
                        cBucketMax + cBucketAfter <= iBucket && iBucket < cBucketMax + cBucketBefore ) )
            {

                plock->m_pBucketHead->CRWL().LeaveAsReader();


                plock->m_pBucketHead = PbucketDIRIHash( esCurrent, iHash );


                plock->m_pBucketHead->CRWL().EnterAsReader();
            }


            DHTAssert( plock->m_pBucketHead == PbucketDIRIHash( esCurrent, iHash ) );
        }



        void DIRReadUnlockKey( CLock * const plock ) const
        {


            DHTAssert( FBKTRead( plock ) );
            DHTAssert( plock->m_pBucketHead != NULL );


            plock->m_pBucketHead->CRWL().LeaveAsReader();
            plock->m_pBucketHead = NULL;
        }


        void DIRWriteLockKey( const ENUMSTATE esCurrent, const CKey &key, CLock * const plock) const
        {
            BOOL fLockSucceeded = FDIRTryWriteLockKey( esCurrent, key, plock, fFalse );
            DHTAssert( fLockSucceeded );
            return;
        }

        BOOL FDIRTryWriteLockKey( const ENUMSTATE esCurrent, const CKey &key, CLock * const plock, BOOL fCanFail = fTrue ) const
        {
            NativeCounter   iHash;
            NativeCounter   iBucket;
            NativeCounter   cBucketBefore;
            NativeCounter   cBucketAfter;
            NativeCounter   cBucketMax;


            DHTAssert( FBKTWrite( plock ) || FBKTScan( plock ) );
            DHTAssert( plock->m_pBucketHead == NULL );

#ifdef DEBUG

            plock->m_key    = key;
#endif

            NativeCounter cBucketMaxBefore = NcDIRIGetBucketMax( esCurrent );

            iHash                   = CKeyEntry::Hash( key );
            plock->m_pBucketHead    = PbucketDIRIHash( esCurrent, iHash, &iBucket, &cBucketBefore );

#ifdef DEBUG
            plock->m_iHash   = iHash;
            plock->m_iBucket = iBucket;
#endif



            if ( fCanFail )
            {
                BOOL fLockSucceeded = plock->m_pBucketHead->CRWL().FTryEnterAsWriter();
                if ( !fLockSucceeded )
                {
                    plock->m_pBucketHead = NULL;
                    return fLockSucceeded;
                }
            }
            else
            {
                plock->m_pBucketHead->CRWL().EnterAsWriter();
            }


            cBucketAfter    = NcDIRIGetBucket( esCurrent );
            cBucketMax      = NcDIRIGetBucketMax( esCurrent );

            DHTAssert( cBucketMax == cBucketMaxBefore );

            if (    cBucketBefore != cBucketAfter &&
                    (   cBucketBefore <= iBucket && iBucket < cBucketAfter ||
                        cBucketMax + cBucketAfter <= iBucket && iBucket < cBucketMax + cBucketBefore ) )
            {

                plock->m_pBucketHead->CRWL().LeaveAsWriter();


                plock->m_pBucketHead = PbucketDIRIHash( esCurrent, iHash );


                if ( fCanFail )
                {
                    BOOL fLockSucceeded = plock->m_pBucketHead->CRWL().FTryEnterAsWriter();
                    if ( !fLockSucceeded )
                    {
                        plock->m_pBucketHead = NULL;
                        return fLockSucceeded;
                    }
                }
                else
                {
                    plock->m_pBucketHead->CRWL().EnterAsWriter();
                }
            }


            DHTAssert( plock->m_pBucketHead == PbucketDIRIHash( esCurrent, iHash ) );

            return fTrue;
        }



        void DIRWriteUnlockKey( CLock * const plock ) const
        {


            DHTAssert( FBKTWrite( plock ) );
            DHTAssert( plock->m_pBucketHead != NULL );


            plock->m_pBucketHead->CRWL().LeaveAsWriter();
            plock->m_pBucketHead = NULL;
        }



        ERR ErrDIRInitBucketArray(  const NativeCounter cbucketAlloc,
                                    const NativeCounter ibucketFirst,
                                    BYTE** const        prgbBucket )
        {
#ifdef UNIQUE_BUCKET_NAMES
            char                *psz;
            NativeCounter       cbsz;
#endif
            NativeCounter       cb;
            BYTE                *rgb;
            NativeCounter       ibucket;

            DHTAssert( cbucketAlloc > 0 );
            DHTAssert( prgbBucket );


#ifdef UNIQUE_BUCKET_NAMES
            cb = cbucketAlloc * ( m_cbBucket + 60 );
#else
            cb = cbucketAlloc * m_cbBucket;
#endif


            rgb = (BYTE*)PvMEMAlloc( cb );
            if ( !rgb )
            {
                *prgbBucket = NULL;
                return ERR::errOutOfMemory;
            }


            for ( ibucket = 0; ibucket < cbucketAlloc; ibucket++ )
            {


                PBUCKET const   pbucket = PBUCKET( rgb + ( ibucket * m_cbBucket ) );


#ifdef UNIQUE_BUCKET_NAMES
                psz = (char*)( rgb + ( cbucketAlloc * m_cbBucket ) + ( ibucket * 60 ) );
                cbsz = cb - ( ( cbucketAlloc * m_cbBucket ) + ( ibucket * 60 )  );
                OSStrCbFormatA( psz, cbsz, "CDynamicHashTable::BUCKET[0x%016I64X]::m_rwlBucket", QWORD( ibucketFirst + ibucket ) );
                DHTAssert( strlen( psz ) < 60 );

                new( &pbucket->CRWL() ) OSSYNC::CReaderWriterLock( CLockBasicInfo( CSyncBasicInfo( psz ), INT( m_rankDHTrwlBucket ), 0 ) );
#else
                new( &pbucket->CRWL() ) OSSYNC::CReaderWriterLock( CLockBasicInfo( CSyncBasicInfo( "CDynamicHashTable::BUCKET::m_rwlBucket" ), INT( m_rankDHTrwlBucket ), 0 ) );
#endif


                pbucket->m_pb = NULL;
            }

            *prgbBucket = rgb;
            return ERR::errSuccess;
        }



        void DIRTermBucketArray(    BYTE* const         rgbBucket,
                                    const NativeCounter cbucketTerm )
        {
            NativeCounter   ibucket;
            PBUCKET         pbucketNext;


            DHTAssert( rgbBucket );
            for ( ibucket = 0; ibucket < cbucketTerm; ibucket++ )
            {


                PBUCKET pbucket = PBUCKET( rgbBucket + ( ibucket * m_cbBucket ) );


                pbucket->CRWL().CReaderWriterLock::~CReaderWriterLock();


                pbucket = PbucketBKTNext( pbucket );
                while ( pbucket )
                {
                    pbucketNext = PbucketBKTNext( pbucket );
                    MEMFree( pbucket );
                    pbucket = pbucketNext;
                }
            }

            MEMFree( rgbBucket );
        }



        void DIRISplit()
        {


            DHTAssert( m_stateCur == stateSplit );
            DHTAssert( m_dirptrs[ 0 ].m_cBucketMax > 0 );
            DHTAssert( m_dirptrs[ 0 ].m_cBucket == m_dirptrs[ 0 ].m_cBucketMax );


            m_dirptrs[ 0 ].m_cBucketMax = m_dirptrs[ 0 ].m_cBucketMax * 2;
            m_dirptrs[ 0 ].m_cBucket    = 0;

            STATSplitDirectory();
        }



        void DIRIMerge()
        {


            DHTAssert( m_stateCur == stateMerge );
            DHTAssert( m_dirptrs[ 0 ].m_cBucketMax > 1 );
            DHTAssert( m_dirptrs[ 0 ].m_cBucket == 0 );


            NativeCounter   iExponent;
            NativeCounter   iRemainder;

            DIRILog2( m_dirptrs[ 0 ].m_cBucketMax, &iExponent, &iRemainder );

            DHTAssert( NativeCounter( 1 ) << iExponent == m_dirptrs[ 0 ].m_cBucketMax );
            DHTAssert( 0 == iRemainder );


            if ( m_rgrgBucket[ iExponent ] )
            {
                DIRTermBucketArray( m_rgrgBucket[ iExponent ], m_dirptrs[ 0 ].m_cBucketMax );
                m_rgrgBucket[ iExponent ] = NULL;
            }

#ifdef DEBUG

            while ( ++iExponent < cbitNativeCounter )
            {
                DHTAssert( !m_rgrgBucket[ iExponent ] );
            }
#endif


            m_dirptrs[ 0 ].m_cBucketMax = m_dirptrs[ 0 ].m_cBucketMax / 2;
            m_dirptrs[ 0 ].m_cBucket    = m_dirptrs[ 0 ].m_cBucketMax;

            STATMergeDirectory();
        }



        void DIRILog2(  const NativeCounter     iValue,
                        NativeCounter* const    piExponent,
                        NativeCounter* const    piRemainder ) const
        {
            NativeCounter   iExponent;
            NativeCounter   iMask;
            NativeCounter   iMaskLast;

            iExponent       = 0;
            iMaskLast       = 1;
            iMask           = 1;

            while ( iMask < iValue )
            {
                iExponent++;
                iMaskLast   = iMask;
                iMask       = ( iMask << 1 ) + 1;
            }

            DHTAssert( iExponent < cbitNativeCounter );

            *piExponent     = iExponent;
            *piRemainder    = iMaskLast & iValue;
        }



        const NativeCounter NcDIRIGetBucketMax( const ENUMSTATE esCurrent ) const
        {
            return m_dirptrs[ esCurrent >> 4 ].m_cBucketMax;
        }



        const NativeCounter NcDIRIGetBucket( const ENUMSTATE esCurrent ) const
        {
            return m_dirptrs[ esCurrent >> 4 ].m_cBucket;
        }



        PBUCKET const PbucketDIRIResolve(   const NativeCounter ibucketIndex,
                                            const NativeCounter ibucketOffset ) const
        {
            BYTE* const         pb          = m_rgrgBucket[ ibucketIndex ];
            const NativeCounter ibOffset    = ibucketOffset * m_cbBucket;

            DHTAssert( NULL != pb );

            return PBUCKET( pb + ibOffset );
        }



        const PBUCKET PbucketDIRIHash(  const ENUMSTATE         esCurrent,
                                        const NativeCounter     iHash,
                                        NativeCounter* const    piBucket,
                                        NativeCounter* const    pcBucket ) const
        {
            NativeCounter&      iBucket     = *piBucket;
            NativeCounter&      cBucket     = *pcBucket;
            NativeCounter       cBucketMax;
            NativeCounter       iExponent;
            NativeCounter       iRemainder;


            cBucket     = NcDIRIGetBucket( esCurrent );
            cBucketMax  = NcDIRIGetBucketMax( esCurrent );


            iBucket = iHash & ( ( cBucketMax - 1 ) + cBucketMax );
            if ( iBucket >= cBucketMax + cBucket )
            {
                iBucket -= cBucketMax;
            }


            DIRILog2( iBucket, &iExponent, &iRemainder );


            return PbucketDIRIResolve( iExponent, iRemainder );
        }

        const PBUCKET PbucketDIRIHash(  const ENUMSTATE         esCurrent,
                                        const NativeCounter     iHash ) const
        {
            NativeCounter   iBucket;
            NativeCounter   cBucket;

            return PbucketDIRIHash( esCurrent, iHash, &iBucket, &cBucket );
        }





        ERR ErrSCANMoveNext( CLock *const plock )
        {
            DHTAssert( plock->m_pEntryPrev == NULL );
            DHTAssert( plock->m_pEntry == NULL );
            DHTAssert( plock->m_pEntryNext == NULL );


            if ( plock->m_pBucketHead )
            {
                plock->m_pBucketHead->CRWL().LeaveAsWriter();
                plock->m_pBucketHead = NULL;


                if ( plock->m_fInsertOrDelete )
                {

                    MaintainTable( plock->m_phs );
                }
            }


            const INT iGroup = UiSTEnter( &plock->m_phs );
            const ENUMSTATE esCurrent = EsSTGetState();

            while ( plock->m_iBucket + 1 < NcDIRIGetBucketMax( esCurrent ) + NcDIRIGetBucket( esCurrent ) )
            {



                plock->m_iBucket++;


                plock->m_pBucketHead = PbucketDIRIHash( esCurrent, plock->m_iBucket );
                plock->m_pBucketHead->CRWL().EnterAsWriter();

                if ( plock->m_iBucket < NcDIRIGetBucketMax( esCurrent ) + NcDIRIGetBucket( esCurrent ) )
                {


                    if ( plock->m_pBucketHead->m_pb != NULL )
                    {



                        plock->m_pBucket = plock->m_pBucketHead;
                        plock->m_pEntry = &plock->m_pBucketHead->m_rgEntry[0];


                        break;
                    }


                    DHTAssert( !plock->m_pBucketHead->m_pb );
                }
                else
                {
                    DHTAssert( stateShrink == esCurrent );


                    DHTAssert( plock->m_iBucket >= NcDIRIGetBucketMax( esCurrent ) );
                    DHTAssert(  PbucketDIRIHash( esCurrent, plock->m_iBucket ) ==
                                PbucketDIRIHash( esCurrent, plock->m_iBucket - NcDIRIGetBucketMax( esCurrent ) ) );


                    DHTAssert( !plock->m_pEntry );

                }


                plock->m_pBucketHead->CRWL().LeaveAsWriter();
                plock->m_pBucketHead = NULL;
            }


            STLeave( iGroup, plock->m_phs );


            DHTAssert( !plock->m_pEntry || plock->m_pBucketHead );
            return plock->m_pEntry ? ERR::errSuccess : ERR::errNoCurrentEntry;
        }






        const BOOL FBKTRead( CLock *const plock ) const
        {
            return plock->m_ls == CLock::lsRead;
        }



        const BOOL FBKTWrite( CLock *const plock ) const
        {
            return plock->m_ls == CLock::lsWrite;
        }



        const BOOL FBKTScan( CLock *const plock ) const
        {
            return plock->m_ls == CLock::lsScan;
        }



        CKeyEntry *PentryBKTNextMost( const PBUCKET pBucket ) const
        {
            const BYTE *pb = pBucket->m_pb;

            if ( BOOL( ( pb >= (BYTE*)&pBucket->m_rgEntry[ 0 ] ) &
                        ( pb < (BYTE*)&pBucket->m_rgEntry[ m_centryBucket ] ) ) )
            {


                return (CKeyEntry*)pb + 1;
            }
            else if ( NULL == pb )
            {


                return &pBucket->m_rgEntry[ 0 ];
            }


            return &pBucket->m_rgEntry[ m_centryBucket ];
        }



        PBUCKET PbucketBKTNext( const PBUCKET pBucket ) const
        {
            const BYTE *pb = pBucket->m_pb;

            if ( BOOL( ( pb <= (BYTE*)pBucket - m_cbBucket ) |
                       ( pb >= (BYTE*)pBucket + m_cbBucket ) ) )
            {


                DHTAssert( !pb || PBUCKET( pb )->m_pBucketPrev == pBucket );
                return PBUCKET( pb );
            }


            return NULL;
        }




        void BKTSeek( CLock *const plock, const CKey &key ) const
        {

            plock->m_pBucket    = plock->m_pBucketHead;
            plock->m_pEntryPrev = NULL;
            plock->m_pEntryNext = NULL;


            CKeyEntry* const pEntryLast = plock->m_pBucketHead->m_pEntryLast;

            if ( DWORD_PTR( pEntryLast ) - DWORD_PTR( plock->m_pBucketHead ) < m_cbBucket )
            {
                CKeyEntry* pEntry = plock->m_pBucketHead->m_rgEntry;
                do
                {
                    if ( pEntry->FEntryMatchesKey( key ) )
                    {
                        plock->m_pEntry = pEntry;
                        return;
                    }
                }
                while ( ++pEntry <= pEntryLast );

                plock->m_pEntry = NULL;
            }


            else if ( !pEntryLast )
            {
                plock->m_pEntry = NULL;
            }


            else
            {
                BKTISeek( plock, key );
            }
        }

        void BKTISeek( CLock *const plock, const CKey &key ) const
        {
            PBUCKET         pBucket;
            PBUCKET         pBucketPrev;
            CKeyEntry       *pEntryThis;
            CKeyEntry       *pEntryMost;

            DHTAssert( FBKTRead( plock ) || FBKTWrite( plock ) );
            DHTAssert( plock->m_pBucketHead != NULL );


            pBucket = plock->m_pBucketHead;
            do
            {


                pEntryThis = &pBucket->m_rgEntry[ 0 ];
                pEntryMost = PentryBKTNextMost( pBucket );
                while ( pEntryThis < pEntryMost )
                {


                    if ( !pEntryThis->FEntryMatchesKey( key ) )
                    {

                    }
                    else
                    {


                        goto SetupCurrency;
                    }


                    pEntryThis++;
                }


                pBucketPrev = pBucket;
                pBucket = PbucketBKTNext( pBucket );
            }
            while ( pBucket );


            pBucket = pBucketPrev;
            pEntryThis = NULL;

    SetupCurrency:


            plock->m_pBucket    = pBucket;
            plock->m_pEntryPrev = NULL;
            plock->m_pEntry     = pEntryThis;
            plock->m_pEntryNext = NULL;
        }


#ifdef DEBUG

        void BKTGetEntry( CLock *const plock, CKeyEntry **ppKeyEntry ) const
        {
            DHTAssert( FBKTRead( plock ) || FBKTWrite( plock ) );
            DHTAssert( plock->m_pBucketHead != NULL );
            DHTAssert( plock->m_pBucket != NULL );

            *ppKeyEntry = plock->m_pEntry;
            return;
        }
#endif



        const ERR ErrBKTGetEntry( CLock *const plock, CEntry *pentry ) const
        {
            DHTAssert( FBKTRead( plock ) || FBKTWrite( plock ) || FBKTScan( plock ) );
            DHTAssert( plock->m_pBucketHead != NULL );
            DHTAssert( plock->m_pBucket != NULL );

            if ( plock->m_pEntry )
            {


                plock->m_pEntry->GetEntry( pentry );
                return ERR::errSuccess;
            }


            return ERR::errEntryNotFound;
        }



        const ERR ErrBKTReplaceEntry( CLock *const plock, const CEntry &entry ) const
        {
            DHTAssert( FBKTWrite( plock ) || FBKTScan( plock ) );
            DHTAssert( plock->m_pBucketHead != NULL );
            DHTAssert( plock->m_pBucket != NULL );

            if ( plock->m_pEntry )
            {



                plock->m_pEntry->SetEntry( entry );
                return ERR::errSuccess;
            }


            return ERR::errNoCurrentEntry;
        }



        const ERR ErrBKTInsertEntry( CLock *const plock, const CEntry &entry )
        {
            DHTAssert( FBKTWrite( plock ) );
            DHTAssert( plock->m_pBucketHead != NULL );
            DHTAssert( plock->m_pBucket != NULL );

            if ( plock->m_pEntry )
            {


                return ERR::errKeyDuplicate;
            }

#ifdef DEBUG


            const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );

            PBUCKET     *rgBucketCheck = NULL, pbucketTX;
            size_t      cBucketCheck = 0, iT;

            pbucketTX = plock->m_pBucketHead;
            while ( pbucketTX )
            {
                cBucketCheck++;
                pbucketTX = PbucketBKTNext( pbucketTX );
            }
            cBucketCheck++;

            rgBucketCheck = (PBUCKET *)PvMEMAlloc( cBucketCheck * sizeof( PBUCKET ) );
            if ( NULL != rgBucketCheck )
            {
                iT = 0;
                pbucketTX = plock->m_pBucketHead;
                while ( pbucketTX )
                {
                    rgBucketCheck[ iT++ ] = pbucketTX;
                    pbucketTX = PbucketBKTNext( pbucketTX );
                }
                rgBucketCheck[ iT++ ] = NULL;
            }


            size_t          cEntriesTotal = 0;
            PBUCKET         pbktT, pbktNextT;

            pbktT = plock->m_pBucketHead;
            if ( pbktT->m_pb != NULL )
            {
                while ( pbktT )
                {
                    pbktNextT = PbucketBKTNext( pbktT );
                    if ( pbktNextT )
                    {
                        cEntriesTotal += size_t( m_centryBucket );
                    }
                    else
                    {
                        cEntriesTotal += 1 + ( pbktT->m_pEntryLast - &pbktT->m_rgEntry[0] );
                    }
                    pbktT = pbktNextT;
                }
            }
            

            FOSSetCleanupState( fCleanUpStateSaved );
#endif



            PBUCKET     pBucketThis = plock->m_pBucket;
            CKeyEntry   *pEntryThis;


            PBUCKET     pBucketT;



            pBucketT = PbucketBKTNext( pBucketThis );
            while ( pBucketT )
            {
                pBucketThis = pBucketT;
                pBucketT = PbucketBKTNext( pBucketT );
            }
            pEntryThis = PentryBKTNextMost( pBucketThis );

            if ( pEntryThis != &pBucketThis->m_rgEntry[ m_centryBucket ] )
            {


            }
            else
            {



                const BOOL fCleanUpStateSaved2 = FOSSetCleanupState( fFalse );
                

                pBucketT = (BUCKET *)PvMEMAlloc( m_cbBucket );


                FOSSetCleanupState( fCleanUpStateSaved2 );

                if ( !pBucketT )
                {


#ifdef DEBUG

                    if ( NULL != rgBucketCheck )
                    {
                        MEMFree( rgBucketCheck );
                    }
#endif

                    return ERR::errOutOfMemory;
                }

                STATInsertOverflowBucket();

#ifdef DEBUG

                if ( NULL != rgBucketCheck )
                {
                    DHTAssert( rgBucketCheck[cBucketCheck-1] == NULL );
                    rgBucketCheck[cBucketCheck-1] = pBucketT;
                }
#endif


                pBucketThis->m_pBucketNext = pBucketT;
                pBucketT->m_pBucketPrev = pBucketThis;


                pBucketThis = pBucketT;
                pEntryThis = &pBucketT->m_rgEntry[0];
            }


            pEntryThis->SetEntry( entry );


            pBucketThis->m_pEntryLast = pEntryThis;


            plock->m_pBucket = pBucketThis;
            plock->m_pEntry = pEntryThis;


#ifdef DEBUG

            if ( NULL != rgBucketCheck )
            {


                pbucketTX = plock->m_pBucketHead;
                DHTAssert( pbucketTX );


                while ( pbucketTX )
                {
                    for ( iT = 0; iT < cBucketCheck; iT++ )
                    {
                        if ( rgBucketCheck[iT] == pbucketTX )
                        {
                            rgBucketCheck[iT] = NULL;
                            break;
                        }
                    }
                    DHTAssert( iT < cBucketCheck );
                    pbucketTX = PbucketBKTNext( pbucketTX );
                }


                for ( iT = 0; iT < cBucketCheck; iT++ )
                {
                    DHTAssert( rgBucketCheck[iT] == NULL );
                }


                MEMFree( rgBucketCheck );
            }



            size_t  cEntriesAfterwards = 0;

            pbktT = plock->m_pBucketHead;
            if ( pbktT->m_pb != NULL )
            {
                while ( pbktT )
                {
                    pbktNextT = PbucketBKTNext( pbktT );
                    if ( pbktNextT )
                    {
                        cEntriesAfterwards += size_t( m_centryBucket );
                    }
                    else
                    {
                        cEntriesAfterwards += 1 + ( pbktT->m_pEntryLast - &pbktT->m_rgEntry[0] );
                    }
                    pbktT = pbktNextT;
                }
            }


            DHTAssert( cEntriesAfterwards == cEntriesTotal + 1 );
#endif

            return ERR::errSuccess;
        }



        const ERR ErrBKTDeleteEntry( CLock *const plock )
        {
            DHTAssert( FBKTWrite( plock ) || FBKTScan( plock ) );
            DHTAssert( plock->m_pBucketHead != NULL );
            DHTAssert( plock->m_pBucket != NULL );

            if ( !plock->m_pEntry )
            {


                return ERR::errNoCurrentEntry;
            }

#ifdef DEBUG

            const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );
            
            PBUCKET     *rgBucketCheck = NULL;
            PBUCKET     pbucketT;
            size_t      cBucketCheck = 0, iT;

            pbucketT = plock->m_pBucketHead;
            while ( pbucketT )
            {
                cBucketCheck++;
                pbucketT = PbucketBKTNext( pbucketT );
            }

            rgBucketCheck = (PBUCKET *)PvMEMAlloc( cBucketCheck * sizeof( PBUCKET ) );
            if ( NULL != rgBucketCheck )
            {
                iT = 0;
                pbucketT = plock->m_pBucketHead;
                while ( pbucketT )
                {
                    rgBucketCheck[ iT++ ] = pbucketT;
                    pbucketT = PbucketBKTNext( pbucketT );
                }
            }



            size_t          cEntriesTotal = 0;
            PBUCKET         pbktT, pbktNextT;

            pbktT = plock->m_pBucketHead;
            if ( pbktT->m_pb != NULL )
            {
                while ( pbktT )
                {
                    pbktNextT = PbucketBKTNext( pbktT );
                    if ( pbktNextT )
                    {
                        cEntriesTotal += size_t( m_centryBucket );
                    }
                    else
                    {
                        cEntriesTotal += 1 + ( pbktT->m_pEntryLast - &pbktT->m_rgEntry[0] );
                    }
                    pbktT = pbktNextT;
                }
            }
            

            FOSSetCleanupState( fCleanUpStateSaved );
#endif



            PBUCKET     pBucketThis = plock->m_pBucket;
            CKeyEntry   *pEntryThis = plock->m_pEntry;
            PBUCKET     pBucketFree = NULL;

            if ( pEntryThis != pBucketThis->m_pEntryLast )
            {



                PBUCKET pBucketT = PbucketBKTNext( pBucketThis );
                while ( pBucketT )
                {
                    pBucketThis = pBucketT;
                    pBucketT = PbucketBKTNext( pBucketT );
                }


                pEntryThis = pBucketThis->m_pEntryLast;


                plock->m_pEntry->SetEntry( pEntryThis->m_entry );
            }


            plock->m_pEntry = NULL;



            if ( pEntryThis != &pBucketThis->m_rgEntry[0] )
            {


                DHTAssert( pBucketThis->m_pEntryLast == pEntryThis );
                pBucketThis->m_pEntryLast--;

#ifdef DEBUG
                goto DoValidation;
#else
                return ERR::errSuccess;
#endif
            }


            if ( pBucketThis == plock->m_pBucketHead )
            {


                pBucketThis->m_pb = NULL;

#ifdef DEBUG
                goto DoValidation;
#else
                return ERR::errSuccess;
#endif
            }


            pBucketFree = pBucketThis;


            DHTAssert( pBucketThis->m_pBucketPrev->m_pBucketNext == pBucketThis );
            pBucketThis = pBucketThis->m_pBucketPrev;
            pBucketThis->m_pEntryLast = &pBucketThis->m_rgEntry[ m_centryBucket - 1 ];


            MEMFree( pBucketFree );

            if ( plock->m_pBucket == pBucketFree )
            {


                plock->m_pBucket = pBucketThis;
            }

            STATDeleteOverflowBucket();

#ifdef DEBUG
        DoValidation:

            if ( NULL != rgBucketCheck )
            {

                pbucketT = plock->m_pBucketHead;
                DHTAssert( pbucketT );


                while ( pbucketT )
                {
                    for ( iT = 0; iT < cBucketCheck; iT++ )
                    {
                        if ( rgBucketCheck[iT] == pbucketT )
                        {
                            rgBucketCheck[iT] = NULL;
                            break;
                        }
                    }
                    DHTAssert( iT < cBucketCheck );
                    pbucketT = PbucketBKTNext( pbucketT );
                }


                if ( pBucketFree )
                {
                    for ( iT = 0; iT < cBucketCheck; iT++ )
                    {
                        if ( rgBucketCheck[iT] == pBucketFree )
                        {
                            rgBucketCheck[iT] = NULL;
                            break;
                        }
                    }
                    DHTAssert( iT < cBucketCheck );
                }


                for ( iT = 0; iT < cBucketCheck; iT++ )
                {
                    DHTAssert( rgBucketCheck[iT] == NULL );
                }


                MEMFree( rgBucketCheck );
            }



            size_t  cEntriesAfterwards = 0;

            pbktT = plock->m_pBucketHead;
            if ( pbktT->m_pb != NULL )
            {
                while ( pbktT )
                {
                    pbktNextT = PbucketBKTNext( pbktT );
                    if ( pbktNextT )
                    {
                        cEntriesAfterwards += size_t( m_centryBucket );
                    }
                    else
                    {
                        cEntriesAfterwards += 1 + ( pbktT->m_pEntryLast - &pbktT->m_rgEntry[0] );
                    }
                    pbktT = pbktNextT;
                }
            }


            DHTAssert( cEntriesAfterwards == cEntriesTotal - 1 );
#endif

            return ERR::errSuccess;
        }




        void BKTISplit( HOTSTUFF* const phs )
        {


            const NativeCounter cBucketMax  = NcDIRIGetBucketMax( stateGrow );
            const NativeCounter cBucket     = NcDIRIGetBucket( stateGrow );

            if ( cBucketMax + cBucket >= m_cBucketPreferred || cBucket == cBucketMax )
            {
                return;
            }


            if ( !phs->m_bucketpool.FPOOLReserve( m_cbBucket ) )
            {
                return;
            }


            const PBUCKET       pbucketGrowSrc      = PbucketDIRIHash( stateGrow, cBucket );


            if (    pbucketGrowSrc->CRWL().FWritersQuiesced() ||
                    !pbucketGrowSrc->CRWL().FTryEnterAsWriter() )
            {
                STATSplitContention();
                phs->m_bucketpool.POOLUnreserve();
                return;
            }


            if ( cBucket != NcDIRIGetBucket( stateGrow ) )
            {
                DHTAssert( cBucket < NcDIRIGetBucket( stateGrow ) );
                pbucketGrowSrc->CRWL().LeaveAsWriter();
                phs->m_bucketpool.POOLUnreserve();
                return;
            }


            NativeCounter   iExponent;
            NativeCounter   iRemainder;
            DIRILog2( cBucketMax + cBucket, &iExponent, &iRemainder );


            if ( !m_rgrgBucket[ iExponent ] )
            {

                if ( ErrDIRInitBucketArray( cBucketMax, cBucketMax, &m_rgrgBucket[ iExponent ] ) != ERR::errSuccess )
                {
                    pbucketGrowSrc->CRWL().LeaveAsWriter();
                    phs->m_bucketpool.POOLUnreserve();
                    return;
                }
            }
            DHTAssert( m_rgrgBucket[ iExponent ] );


            const PBUCKET pbucketGrowDst = PbucketDIRIResolve( iExponent, iRemainder );


            BOOL fDstLockSucceeded = pbucketGrowDst->CRWL().FTryEnterAsWriter();
            DHTAssert( fDstLockSucceeded );


            DHTAssert( cBucket == NcDIRIGetBucket( stateGrow ) );
            m_dirptrs[ 0 ].m_cBucket++;


            BKTIDoSplit( phs, pbucketGrowSrc, pbucketGrowDst, cBucket );


            pbucketGrowSrc->CRWL().LeaveAsWriter();
            pbucketGrowDst->CRWL().LeaveAsWriter();
        }



        void BKTIMerge( HOTSTUFF* const phs )
        {


            const NativeCounter cBucketMax  = NcDIRIGetBucketMax( stateShrink );
            NativeCounter       cBucket     = NcDIRIGetBucket( stateShrink );

            if ( cBucketMax + cBucket <= m_cBucketPreferred || cBucket == 0 )
            {
                return;
            }

            cBucket--;


            if ( !phs->m_bucketpool.FPOOLReserve( m_cbBucket ) )
            {
                return;
            }


            const PBUCKET pbucketShrinkDst = PbucketDIRIHash( stateShrink, cBucket );


            if (    pbucketShrinkDst->CRWL().FWritersQuiesced() ||
                    !pbucketShrinkDst->CRWL().FTryEnterAsWriter() )
            {
                STATMergeContention();
                phs->m_bucketpool.POOLUnreserve();
                return;
            }


            if ( cBucket + 1 != NcDIRIGetBucket( stateShrink ) )
            {
                DHTAssert( cBucket + 1 > NcDIRIGetBucket( stateShrink ) );
                pbucketShrinkDst->CRWL().LeaveAsWriter();
                phs->m_bucketpool.POOLUnreserve();
                return;
            }


            NativeCounter   iExponent;
            NativeCounter   iRemainder;
            DIRILog2( cBucket + NcDIRIGetBucketMax( stateShrink ), &iExponent, &iRemainder );


            const PBUCKET pbucketShrinkSrc = PbucketDIRIResolve( iExponent, iRemainder );


            if (    pbucketShrinkSrc->CRWL().FWritersQuiesced() ||
                    !pbucketShrinkSrc->CRWL().FTryEnterAsWriter() )
            {
                STATMergeContention();
                pbucketShrinkDst->CRWL().LeaveAsWriter();
                phs->m_bucketpool.POOLUnreserve();
                return;
            }


            DHTAssert( cBucket + 1 == NcDIRIGetBucket( stateShrink ) );
            m_dirptrs[ 0 ].m_cBucket--;


            BKTIDoMerge( phs, pbucketShrinkSrc, pbucketShrinkDst );


            pbucketShrinkDst->CRWL().LeaveAsWriter();
            pbucketShrinkSrc->CRWL().LeaveAsWriter();
        }



        void BKTIDoSplit(   HOTSTUFF* const         phs,
                            PBUCKET                 pBucketSrcSrc,
                            PBUCKET                 pBucketDst,
                            const NativeCounter     iHashSrc )
        {

#ifdef DEBUG


            const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );

            PBUCKET         pBucketSrcSrcOriginal = pBucketSrcSrc;
            PBUCKET         pBucketDstOriginal = pBucketDst;
            size_t          cEntriesTotal = 0, cEntriesTotalRunning = 0;
            PBUCKET         pbktT, pbktNextT;


            PBUCKET     *rgBucketCheck = NULL, pbucketTX;
            size_t      cBucketCheck = 0, iT;

            pbucketTX = pBucketSrcSrc;
            while ( pbucketTX )
            {
                cBucketCheck++;
                pbucketTX = PbucketBKTNext( pbucketTX );
            }
            pbucketTX = pBucketDst;
            DHTAssert( PbucketBKTNext( pbucketTX ) == NULL );
            while ( pbucketTX )
            {
                cBucketCheck++;
                pbucketTX = PbucketBKTNext( pbucketTX );
            }
            cBucketCheck++;

            rgBucketCheck = (PBUCKET *)PvMEMAlloc( cBucketCheck * sizeof( PBUCKET ) );
            if ( NULL != rgBucketCheck )
            {
                iT = 0;
                pbucketTX = pBucketSrcSrc;
                while ( pbucketTX )
                {
                    rgBucketCheck[ iT++ ] = pbucketTX;
                    pbucketTX = PbucketBKTNext( pbucketTX );
                }
                pbucketTX = pBucketDst;
                while ( pbucketTX )
                {
                    rgBucketCheck[ iT++ ] = pbucketTX;
                    pbucketTX = PbucketBKTNext( pbucketTX );
                }
                rgBucketCheck[ iT++ ] = NULL;
                DHTAssert( iT == cBucketCheck );
            }


            pbktT = pBucketSrcSrc;
            if ( pbktT->m_pb != NULL )
            {
                while ( pbktT )
                {
                    pbktNextT = PbucketBKTNext( pbktT );
                    if ( pbktNextT )
                    {
                        cEntriesTotal += size_t( m_centryBucket );
                    }
                    else
                    {
                        cEntriesTotal += 1 + ( pbktT->m_pEntryLast - &pbktT->m_rgEntry[0] );
                    }
                    pbktT = pbktNextT;
                }
            }
            

            FOSSetCleanupState( fCleanUpStateSaved );
#endif


            PBUCKET         pBucketNextSrc;
            CKeyEntry       *pEntryThisSrc;
            CKeyEntry       *pEntryMostSrc;


            PBUCKET         pBucketThis[2];
            CKeyEntry       *pEntryThis[2];
            CKeyEntry       *pEntryMost[2];
            CKeyEntry       *pEntryLast[2];
            size_t          iIndex;


            PBUCKET         pBucketAvail = NULL;


            BOOL            fBucketFromHeap = fFalse;


            NativeCounter   iHashMask;

            DHTAssert( pBucketSrcSrc );
            DHTAssert( pBucketDst );
            DHTAssert( pBucketDst->m_pb == NULL );


            DHTAssert( NcDIRIGetBucketMax( stateGrow ) > 0 );
            iHashMask       = ( NcDIRIGetBucketMax( stateGrow ) - 1 ) + NcDIRIGetBucketMax( stateGrow );


            pBucketNextSrc      = PbucketBKTNext( pBucketSrcSrc );
            pEntryThisSrc       = &pBucketSrcSrc->m_rgEntry[ 0 ];
            pEntryMostSrc       = PentryBKTNextMost( pBucketSrcSrc );


            pBucketThis[ 0 ]    = pBucketSrcSrc;
            pEntryThis[ 0 ]     = &pBucketSrcSrc->m_rgEntry[ 0 ];
            pEntryMost[ 0 ]     = &pBucketSrcSrc->m_rgEntry[ m_centryBucket ];
            pEntryLast[ 0 ]     = NULL;


            pBucketThis[ 1 ]    = pBucketDst;
            pEntryThis[ 1 ]     = &pBucketDst->m_rgEntry[ 0 ];
            pEntryMost[ 1 ]     = &pBucketDst->m_rgEntry[ m_centryBucket ];
            pEntryLast[ 1 ]     = NULL;


            while ( fTrue )
            {


                if ( pEntryThisSrc < pEntryMostSrc )
                {

                }
                else if ( NULL == pBucketNextSrc )
                {


                    break;
                }
                else
                {


                    if ( pBucketSrcSrc != pBucketThis[ 0 ] )
                    {



                        DHTAssert( pBucketThis[ 0 ]->m_pBucketNext == pBucketSrcSrc );
                        DHTAssert( pBucketSrcSrc->m_pBucketNext == pBucketNextSrc );
                        DHTAssert( pBucketNextSrc->m_pBucketPrev == pBucketSrcSrc );
                        DHTAssert( pBucketSrcSrc->m_pBucketPrev == pBucketThis[ 0 ] );


                        pBucketThis[ 0 ]->m_pBucketNext = pBucketNextSrc;
                        pBucketNextSrc->m_pBucketPrev   = pBucketThis[ 0 ];


                        pBucketSrcSrc->m_pBucketNext    = pBucketAvail;
                        pBucketAvail                    = pBucketSrcSrc;
                    }


                    pEntryThisSrc   = &pBucketNextSrc->m_rgEntry[ 0 ];
                    pEntryMostSrc   = PentryBKTNextMost( pBucketNextSrc );
                    pBucketSrcSrc   = pBucketNextSrc;
                    pBucketNextSrc  = PbucketBKTNext( pBucketNextSrc );
                }


                iIndex = BOOL( ( pEntryThisSrc->Hash() & iHashMask ) != iHashSrc );
                DHTAssert( iIndex == 0 || iIndex == 1 );
#ifdef DEBUG
                cEntriesTotalRunning++;
#endif


                if ( pEntryThis[ iIndex ] < pEntryMost[ iIndex ] )
                {

                }
                else
                {


                    if ( 0 == iIndex )
                    {


                        DHTAssert( pBucketThis[ 0 ]->m_pBucketNext->m_pBucketPrev == pBucketThis[ 0 ] );
                        pBucketThis[ 0 ]    = pBucketThis[ 0 ]->m_pBucketNext;


                        pEntryThis[ 0 ]     = &pBucketThis[ 0 ]->m_rgEntry[ 0 ];
                        pEntryMost[ 0 ]     = &pBucketThis[ 0 ]->m_rgEntry[ m_centryBucket ];
                    }
                    else
                    {


                        if ( pBucketAvail )
                        {


                            const PBUCKET pBucketNew        = pBucketAvail;
                            pBucketAvail                    = pBucketAvail->m_pBucketNext;


                            pBucketThis[ 1 ]->m_pBucketNext = pBucketNew;
                            pBucketNew->m_pBucketPrev       = pBucketThis[ 1 ];


                            pBucketThis[ 1 ]                = pBucketNew;
                        }
                        else
                        {


                            DHTAssert( !fBucketFromHeap );
                            fBucketFromHeap = fTrue;


                            const PBUCKET pBucketReserve    = phs->m_bucketpool.PbucketPOOLCommit();
                            DHTAssert( pBucketReserve );

                            STATInsertOverflowBucket();

#ifdef DEBUG

                            if ( NULL != rgBucketCheck )
                            {
                                DHTAssert( NULL == rgBucketCheck[ cBucketCheck - 1 ] );
                                rgBucketCheck[ cBucketCheck - 1 ] = pBucketReserve;
                            }
#endif


                            pBucketThis[ 1 ]->m_pBucketNext = pBucketReserve;
                            pBucketReserve->m_pBucketPrev   = pBucketThis[ 1 ];


                            pBucketThis[ 1 ]                = pBucketReserve;
                        }


                        pEntryThis[ 1 ] = &pBucketThis[ 1 ]->m_rgEntry[ 0 ];
                        pEntryMost[ 1 ] = &pBucketThis[ 1 ]->m_rgEntry[ m_centryBucket ];
                    }
                }


                pEntryThis[ iIndex ]->SetEntry( pEntryThisSrc->m_entry );


                pEntryLast[ iIndex ] = pEntryThis[ iIndex ];
                pEntryThis[ iIndex ]++;


                pEntryThisSrc++;
            }

            if ( pBucketSrcSrc == pBucketThis[ 0 ] )
            {

            }
            else
            {



                DHTAssert( pBucketThis[ 0 ]->m_pBucketNext == pBucketSrcSrc );
                DHTAssert( pBucketSrcSrc->m_pBucketPrev == pBucketThis[ 0 ] );


                MEMFree( pBucketSrcSrc );

                STATDeleteOverflowBucket();

#ifdef DEBUG

                if ( NULL != rgBucketCheck )
                {
                    for ( iT = 0; iT < cBucketCheck; iT++ )
                    {
                        if ( rgBucketCheck[iT] == pBucketSrcSrc )
                        {
                            rgBucketCheck[iT] = NULL;
                            break;
                        }
                    }
                    DHTAssert( iT < cBucketCheck );
                }
#endif
            }


            pBucketThis[ 0 ]->m_pEntryLast = pEntryLast[ 0 ];
            pBucketThis[ 1 ]->m_pEntryLast = pEntryLast[ 1 ];


#ifdef DEBUG

            if ( NULL != rgBucketCheck )
            {



                pbucketTX = pBucketSrcSrcOriginal;
                DHTAssert( pbucketTX );
                while ( pbucketTX )
                {
                    for ( iT = 0; iT < cBucketCheck; iT++ )
                    {
                        if ( rgBucketCheck[iT] == pbucketTX )
                        {
                            rgBucketCheck[iT] = NULL;
                            break;
                        }
                    }
                    DHTAssert( iT < cBucketCheck );
                    pbucketTX = PbucketBKTNext( pbucketTX );
                }


                pbucketTX = pBucketDstOriginal;
                DHTAssert( pbucketTX );
                while ( pbucketTX )
                {
                    for ( iT = 0; iT < cBucketCheck; iT++ )
                    {
                        if ( rgBucketCheck[iT] == pbucketTX )
                        {
                            rgBucketCheck[iT] = NULL;
                            break;
                        }
                    }
                    DHTAssert( iT < cBucketCheck );
                    pbucketTX = PbucketBKTNext( pbucketTX );
                }


                pbucketTX = pBucketAvail;
                while ( pbucketTX )
                {
                    for ( iT = 0; iT < cBucketCheck; iT++ )
                    {
                        if ( rgBucketCheck[iT] == pbucketTX )
                        {
                            rgBucketCheck[iT] = NULL;
                            break;
                        }
                    }
                    DHTAssert( iT < cBucketCheck );
                    pbucketTX = pbucketTX->m_pBucketNext;
                }


                for ( iT = 0; iT < cBucketCheck; iT++ )
                {
                    DHTAssert( rgBucketCheck[iT] == NULL );
                }


                MEMFree( rgBucketCheck );
            }


            size_t  cEntriesAfterwards = 0;


            DHTAssert( cEntriesTotal == cEntriesTotalRunning );


            pbktT = pBucketSrcSrcOriginal;
            if ( pbktT->m_pb != NULL )
            {
                while ( pbktT )
                {
                    pbktNextT = PbucketBKTNext( pbktT );
                    if ( pbktNextT )
                    {
                        cEntriesAfterwards += size_t( m_centryBucket );
                    }
                    else
                    {
                        cEntriesAfterwards += 1 + ( pbktT->m_pEntryLast - &pbktT->m_rgEntry[0] );
                    }
                    pbktT = pbktNextT;
                }
            }

            pbktT = pBucketDstOriginal;
            if ( pbktT->m_pb != NULL )
            {
                while ( pbktT )
                {
                    pbktNextT = PbucketBKTNext( pbktT );
                    if ( pbktNextT )
                    {
                        cEntriesAfterwards += size_t( m_centryBucket );
                    }
                    else
                    {
                        cEntriesAfterwards += 1 + ( pbktT->m_pEntryLast - &pbktT->m_rgEntry[0] );
                    }
                    pbktT = pbktNextT;
                }
            }

            DHTAssert( cEntriesAfterwards == cEntriesTotal );
#endif



            while ( pBucketAvail )
            {
                PBUCKET pBucketT;

                pBucketT = pBucketAvail;
                pBucketAvail = pBucketAvail->m_pBucketNext;
                MEMFree( pBucketT );
                STATDeleteOverflowBucket();
            }

            if ( !fBucketFromHeap )
            {
                phs->m_bucketpool.POOLUnreserve();
            }

            STATSplitBucket();
        }



        void BKTIDoMerge(   HOTSTUFF* const phs,
                            PBUCKET         pBucketSrc,
                            PBUCKET         pBucketDst )
        {
#ifdef DEBUG


            const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );


            PBUCKET     pBucketDstOriginal = pBucketDst;
            PBUCKET     *rgBucketCheck = NULL, pbucketT;
            size_t      cBucketCheck = 0, iT;

            pbucketT = pBucketSrc;
            while ( pbucketT )
            {
                cBucketCheck++;
                pbucketT = PbucketBKTNext( pbucketT );
            }
            pbucketT = pBucketDst;
            while ( pbucketT )
            {
                cBucketCheck++;
                pbucketT = PbucketBKTNext( pbucketT );
            }
            cBucketCheck++;

            rgBucketCheck = (PBUCKET *)PvMEMAlloc( cBucketCheck * sizeof( PBUCKET ) );
            if ( NULL != rgBucketCheck )
            {
                iT = 0;
                pbucketT = pBucketSrc;
                while ( pbucketT )
                {
                    rgBucketCheck[ iT++ ] = pbucketT;
                    pbucketT = PbucketBKTNext( pbucketT );
                }
                pbucketT = pBucketDst;
                while ( pbucketT )
                {
                    rgBucketCheck[ iT++ ] = pbucketT;
                    pbucketT = PbucketBKTNext( pbucketT );
                }
                rgBucketCheck[ iT++ ] = NULL;
                DHTAssert( iT == cBucketCheck );
            }


            size_t          cEntriesTotal = 0;
            PBUCKET         pbktT, pbktNextT;

            pbktT = pBucketSrc;
            if ( pbktT->m_pb != NULL )
            {
                while ( pbktT )
                {
                    pbktNextT = PbucketBKTNext( pbktT );
                    if ( pbktNextT )
                    {
                        cEntriesTotal += size_t( m_centryBucket );
                    }
                    else
                    {
                        cEntriesTotal += 1 + ( pbktT->m_pEntryLast - &pbktT->m_rgEntry[0] );
                    }
                    pbktT = pbktNextT;
                }
            }

            pbktT = pBucketDst;
            if ( pbktT->m_pb != NULL )
            {
                while ( pbktT )
                {
                    pbktNextT = PbucketBKTNext( pbktT );
                    if ( pbktNextT )
                    {
                        cEntriesTotal += size_t( m_centryBucket );
                    }
                    else
                    {
                        cEntriesTotal += 1 + ( pbktT->m_pEntryLast - &pbktT->m_rgEntry[0] );
                    }
                    pbktT = pbktNextT;
                }
            }


            FOSSetCleanupState( fCleanUpStateSaved );
#endif


            CKeyEntry   *pEntryThisSrc;
            CKeyEntry   *pEntryMostSrc;


            CKeyEntry   *pEntryThisDst;
            CKeyEntry   *pEntryMostDst;


            BOOL        fSetEndPtr;


            BOOL        fBucketFromHeap = fFalse;


            PBUCKET     pBucketT;


            pBucketT = PbucketBKTNext( pBucketDst );
            while ( pBucketT )
            {
                pBucketDst  = pBucketT;
                pBucketT    = PbucketBKTNext( pBucketT );
            }

            pEntryThisDst   = PentryBKTNextMost( pBucketDst );
            pEntryMostDst   = &pBucketDst->m_rgEntry[ m_centryBucket ];

            if ( !PbucketBKTNext( pBucketSrc ) )
            {



                pEntryThisSrc   = &pBucketSrc->m_rgEntry[ 0 ];
                pEntryMostSrc   = PentryBKTNextMost( pBucketSrc );


                fSetEndPtr      = BOOL( pEntryThisSrc < pEntryMostSrc );
            }
            else
            {



                pBucketDst->m_pBucketNext                   = pBucketSrc->m_pBucketNext;
                pBucketDst->m_pBucketNext->m_pBucketPrev    = pBucketDst;


                pEntryThisSrc                               = &pBucketSrc->m_rgEntry[ 0 ];
                pEntryMostSrc                               = &pBucketSrc->m_rgEntry[ m_centryBucket ];


                fSetEndPtr                                  = fFalse;
            }


            while ( pEntryThisSrc < pEntryMostSrc )
            {


                if ( pEntryThisDst < pEntryMostDst )
                {

                }
                else
                {


                    if ( !fSetEndPtr )
                    {


                        pBucketT = PbucketBKTNext( pBucketDst );
                        DHTAssert( pBucketT );
                        do
                        {
                            pBucketDst  = pBucketT;
                            pBucketT    = PbucketBKTNext( pBucketT );
                        }
                        while ( pBucketT );


                        pEntryThisDst   = pBucketDst->m_pEntryLast + 1;
                        pEntryMostDst   = &pBucketDst->m_rgEntry[ m_centryBucket ];


                        fSetEndPtr      = fTrue;


                        continue;
                    }



                    DHTAssert( !fBucketFromHeap );
                    fBucketFromHeap = fTrue;


                    pBucketT = phs->m_bucketpool.PbucketPOOLCommit();
                    DHTAssert( pBucketT );

                    STATInsertOverflowBucket();


                    pBucketDst->m_pBucketNext   = pBucketT;
                    pBucketT->m_pBucketPrev     = pBucketDst;


                    pBucketDst                  = pBucketT;
                    pEntryThisDst               = &pBucketDst->m_rgEntry[ 0 ];
                    pEntryMostDst               = &pBucketDst->m_rgEntry[ m_centryBucket ];

#ifdef DEBUG

                    if ( NULL != rgBucketCheck )
                    {
                        DHTAssert( rgBucketCheck[cBucketCheck - 1] == NULL );
                        rgBucketCheck[cBucketCheck - 1] = pBucketT;
                    }
#endif
                }


                pEntryThisDst->SetEntry( pEntryThisSrc->m_entry );


                pEntryThisSrc++;
                pEntryThisDst++;
            }


            pBucketSrc->m_pb = NULL;

            if ( fSetEndPtr )
            {


                DHTAssert( pEntryThisDst != &pBucketDst->m_rgEntry[ 0 ] );
                pBucketDst->m_pEntryLast = pEntryThisDst - 1;
            }
            else
            {


            }

            if ( !fBucketFromHeap )
            {


                phs->m_bucketpool.POOLUnreserve();
            }


#ifdef DEBUG

            if ( NULL != rgBucketCheck )
            {


                pbucketT = pBucketDstOriginal;
                DHTAssert( pbucketT );


                while ( pbucketT )
                {
                    for ( iT = 0; iT < cBucketCheck; iT++ )
                    {
                        if ( rgBucketCheck[iT] == pbucketT )
                        {
                            rgBucketCheck[iT] = NULL;
                            break;
                        }
                    }
                    DHTAssert( iT < cBucketCheck );
                    pbucketT = PbucketBKTNext( pbucketT );
                }


                for ( iT = 0; iT < cBucketCheck; iT++ )
                {
                    if ( rgBucketCheck[iT] == pBucketSrc )
                    {
                        rgBucketCheck[iT] = NULL;
                        break;
                    }
                }
                DHTAssert( iT < cBucketCheck );


                for ( iT = 0; iT < cBucketCheck; iT++ )
                {
                    DHTAssert( rgBucketCheck[iT] == NULL );
                }


                MEMFree( rgBucketCheck );
            }



            size_t  cEntriesAfterwards = 0;

            pbktT = pBucketDstOriginal;
            if ( pbktT->m_pb != NULL )
            {
                while ( pbktT )
                {
                    pbktNextT = PbucketBKTNext( pbktT );
                    if ( pbktNextT )
                    {
                        cEntriesAfterwards += size_t( m_centryBucket );
                    }
                    else
                    {
                        cEntriesAfterwards += 1 + ( pbktT->m_pEntryLast - &pbktT->m_rgEntry[0] );
                    }
                    pbktT = pbktNextT;
                }
            }

            DHTAssert( cEntriesAfterwards == cEntriesTotal );

#endif

            STATMergeBucket();
        }





        HOTSTUFF *HOTSTUFFHash() const
        {
            return m_rghs + OSSYNC::OSSyncGetCurrentProcessor();
        }



        void STATInsertEntry( HOTSTUFF* const phs )
        {
            AtomicExchangeAddPointer( (void**)&phs->m_cEntry, (void*)1 );
            phs->m_cOp++;
        }

        void STATDeleteEntry( HOTSTUFF* const phs )
        {
            AtomicExchangeAddPointer( (void**)&phs->m_cEntry, (void*)-1 );
            phs->m_cOp++;
        }

        void STATInsertOverflowBucket()
        {
#ifdef DHT_STATS
            m_cBucketOverflowInsert++;
#endif
        }

        void STATDeleteOverflowBucket()
        {
#ifdef DHT_STATS
            m_cBucketOverflowDelete++;
#endif
        }

        void STATSplitBucket()
        {
#ifdef DHT_STATS
            m_cBucketSplit++;
#endif
        }

        void STATMergeBucket()
        {
#ifdef DHT_STATS
            m_cBucketMerge++;
#endif
        }

        void STATSplitDirectory()
        {
#ifdef DHT_STATS
            m_cDirSplit++;
#endif
        }

        void STATMergeDirectory()
        {
#ifdef DHT_STATS
            m_cDirMerge++;
#endif
        }

        void STATStateTransition()
        {
#ifdef DHT_STATS
            m_cTransition++;
#endif
        }

        void STATPolicySelection()
        {
#ifdef DHT_STATS
            m_cSelection++;
#endif
        }

        void STATSplitContention()
        {
#ifdef DHT_STATS
            m_cSplitContend++;
#endif
        }

        void STATMergeContention()
        {
#ifdef DHT_STATS
            m_cMergeContend++;
#endif
        }



        void PerformMaintenance()
        {

            HOTSTUFF*       phs;
            const INT       iGroup      = UiSTEnter( &phs );
            const ENUMSTATE esCurrent   = EsSTGetState();


            const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );


            if ( esCurrent == stateGrow )
            {
                BKTISplit( phs );
            }
            else if ( esCurrent == stateShrink )
            {
                BKTIMerge( phs );
            }


            FOSSetCleanupState( fCleanUpStateSaved );


            STLeave( iGroup, phs );
        }

        void SelectMaintenancePolicy( HOTSTUFF* const phs )
        {

            const ENUMSTATE     esCurrent       = EsSTGetState();
            const NativeCounter cBucketMax      = NcDIRIGetBucketMax( esCurrent );
            const NativeCounter cBucket         = NcDIRIGetBucket( esCurrent );
            const NativeCounter cBucketActive   = cBucketMax + cBucket;
            const NativeCounter cOpLocal        = phs->m_cOp;


            NativeCounter   cEntry  = 0;
            NativeCounter   cOp     = 0;
            for ( NativeCounter ihs = 0; ihs < m_chs; ihs++ )
            {
                cEntry += m_rghs[ ihs ].m_cEntry;
                cOp += m_rghs[ ihs ].m_cOp;
                m_rghs[ ihs ].m_cOp = 0;
            }


            const NativeCounter cEntryIdeal = m_cLoadFactor * cBucketActive;


            const NativeCounter cEntryMax = m_centryBucket * cBucketActive;


            const NativeCounter cEntryFlexibility = max( m_centryBucket - m_cLoadFactor, cEntryMax / 2 - cEntryIdeal );


            const NativeCounter cOpSensitivity = max( 1, cEntryFlexibility / 2 );


            const NativeCounter ratio               = ( cOp + cOpLocal - 1 ) / cOpLocal;
            const NativeCounter cOpSensitivityLocal = max( 1, cOpSensitivity / ratio );


            NativeCounter cEntryPreferred = cEntry;

            if ( cEntryIdeal + ( cEntryFlexibility - cOpSensitivity ) < cEntry )
            {
                cEntryPreferred = cEntry - ( cEntryFlexibility - cOpSensitivity );
            }
            else if ( cEntryIdeal > cEntry + ( cEntryFlexibility - cOpSensitivity ) )
            {
                cEntryPreferred = cEntry + ( cEntryFlexibility - cOpSensitivity );
            }


            const NativeCounter cBucketPreferred = max( m_cbucketMin, ( cEntryPreferred + m_cLoadFactor - 1 ) / m_cLoadFactor );


            ENUMSTATE esNew = stateNil;

            if ( esCurrent == stateGrow )
            {
                if ( cBucketPreferred < cBucketActive )
                {
                    esNew = stateShrinkFromGrow;
                }
                else if ( cBucketPreferred > cBucketActive )
                {
                    if ( cBucket == cBucketMax )
                    {
                        esNew = stateSplitFromGrow;
                    }
                }
            }
            else
            {
                DHTAssert( esCurrent == stateShrink );

                if ( cBucketPreferred < cBucketActive )
                {
                    if ( cBucket == 0 )
                    {
                        esNew = stateMergeFromShrink;
                    }
                }
                else if ( cBucketPreferred > cBucketActive )
                {
                    esNew = stateGrowFromShrink;
                }
            }


            if ( m_cOpSensitivity != cOpSensitivityLocal )
            {
                m_cOpSensitivity = cOpSensitivityLocal;
            }

            if ( m_cBucketPreferred != cBucketPreferred )
            {
                m_cBucketPreferred = cBucketPreferred;
            }

            if ( esNew )
            {
                STTransition( esNew );
            }
            else
            {
                m_semPolicy.Release();
            }

            STATPolicySelection();
        }

        void MaintainTable( HOTSTUFF* const phs )
        {

            if (    phs->m_cOp > m_cOpSensitivity &&
                    m_semPolicy.CAvail() &&
                    m_semPolicy.FTryAcquire() )
            {
                if ( phs->m_cOp > m_cOpSensitivity )
                {
                    SelectMaintenancePolicy( phs );
                }
                else
                {
                    m_semPolicy.Release();
                }
            }


            if (    NcDIRIGetBucketMax( stateGrow ) + NcDIRIGetBucket( stateGrow ) < m_cBucketPreferred ||
                    m_cBucketPreferred < NcDIRIGetBucketMax( stateShrink ) + NcDIRIGetBucket( stateShrink ) )
            {
                PerformMaintenance();
            }
        }

    public:


        static void* PvMEMIAlign( void* const pv, const size_t cbAlign )
        {

            const ULONG_PTR ulp         = ULONG_PTR( pv );
            const ULONG_PTR ulpAligned  = ( ( ulp + cbAlign ) / cbAlign ) * cbAlign;
            const ULONG_PTR ulpOffset   = ulpAligned - ulp;

            DHTAssert( ulpOffset > 0 );
            DHTAssert( ulpOffset <= cbAlign );
            DHTAssert( ulpOffset == BYTE( ulpOffset ) );


            BYTE *const pbAligned       = (BYTE*)ulpAligned;
            pbAligned[ -1 ]             = BYTE( ulpOffset );


            return (void*)pbAligned;
        }



        static void* PvMEMIUnalign( void* const pv )
        {

            BYTE *const pbAligned       = (BYTE*)pv;
            const BYTE bOffset          = pbAligned[ -1 ];

            DHTAssert( bOffset > 0 );


            return (void*)( pbAligned - bOffset );
        }



        static void* PvMEMAlloc( const size_t cbSize, const size_t cbAlign = cbCacheLine )
        {
            void* const pv = new BYTE[ cbSize + cbAlign ];
            if ( pv )
            {
                return PvMEMIAlign( pv, cbAlign );
            }
            return NULL;
        }



        static void MEMFree( void* const pv )
        {
            if ( pv )
            {
                delete [] ((BYTE*)PvMEMIUnalign( pv ));
            }
        }

    private:


        NativeCounter       m_cLoadFactor;
        NativeCounter       m_centryBucket;
        NativeCounter       m_cbBucket;
        NativeCounter       m_rankDHTrwlBucket;
        HOTSTUFF            *m_rghs;
        NativeCounter       m_chs;
        NativeCounter       m_cbucketMin;
#ifdef _WIN64
        BYTE                m_rgbRsvdNever[ 8 ];
#else
        BYTE                m_rgbRsvdNever[ 4 ];
#endif


        DIRPTRS             m_dirptrs[ 2 ];
        BYTE                *m_rgrgBucket[ cbitNativeCounter ];


        NativeCounter       m_cOpSensitivity;
        NativeCounter       m_cBucketPreferred;
        ENUMSTATE           m_stateCur;
#ifdef _WIN64
        BYTE                m_rgbRsvdOften[ 44 ];
#else
        BYTE                m_rgbRsvdOften[ 20 ];
#endif


        OSSYNC::CSemaphore  m_semPolicy;
        LONG                m_cCompletions;
#ifdef _WIN64
        BYTE                m_rgbRsvdAlways[ 52 ];
#else
        BYTE                m_rgbRsvdAlways[ 24 ];
#endif

#ifdef DHT_STATS


        LONG                m_cBucketOverflowInsert;
        LONG                m_cBucketOverflowDelete;
        LONG                m_cBucketSplit;
        LONG                m_cBucketMerge;
        LONG                m_cDirSplit;
        LONG                m_cDirMerge;
        LONG                m_cTransition;
        LONG                m_cSelection;
        LONG                m_cSplitContend;
        LONG                m_cMergeContend;
#ifdef _WIN64
        BYTE                m_rgbRsvdPerf[ 24 ];
#else
        BYTE                m_rgbRsvdPerf[ 24 ];
#endif

#endif


#ifdef DEBUG
        BOOL                m_fInit;
#else
        static const BOOL   m_fInit = fTrue;
#endif

};










template< class CKey, class CEntry >
inline CDynamicHashTable< CKey, CEntry >::
CDynamicHashTable( const NativeCounter rankDHTrwlBucket )
    :   m_semPolicy( CSyncBasicInfo( "CDynamicHashTable::m_semPolicy" ) )
{
#ifdef DEBUG
    m_fInit = fFalse;


    memset( m_rgbRsvdNever, 0, sizeof( m_rgbRsvdNever ) );
    memset( m_rgbRsvdOften, 0, sizeof( m_rgbRsvdOften ) );
    memset( m_rgbRsvdAlways, 0, sizeof( m_rgbRsvdAlways ) );
#ifdef DHT_STATS
    memset( m_rgbRsvdPerf, 0, sizeof( m_rgbRsvdPerf ) );
#endif

#endif


#ifdef _WIN64
    DHTAssert( 8 == sizeof( NativeCounter ) );
#else
    DHTAssert( 4 == sizeof( NativeCounter ) );
#endif


    m_rankDHTrwlBucket = rankDHTrwlBucket;


    m_semPolicy.Release();
}



template< class CKey, class CEntry >
inline CDynamicHashTable< CKey, CEntry >::
~CDynamicHashTable()
{
}



template< class CKey, class CEntry >
inline typename CDynamicHashTable< CKey, CEntry >::ERR CDynamicHashTable< CKey, CEntry >::
ErrInit(    const double            dblLoadFactor,
            const double            dblUniformity,
            const NativeCounter     cBucketMinimum )
{
    ERR             err;
    NativeCounter   ihs;

#ifdef DEBUG
    DHTAssert( !m_fInit );
#endif



    m_cLoadFactor           = 0;
    m_centryBucket          = 0;
    m_cbBucket              = 0;
    m_rghs                  = NULL;
    m_chs                   = OSSYNC::OSSyncGetProcessorCountMax();
    m_cbucketMin            = 0;


    memset( m_dirptrs, 0, sizeof( m_dirptrs ) );
    memset( m_rgrgBucket, 0, sizeof( m_rgrgBucket ) );


    m_cOpSensitivity        = 0;
    m_cBucketPreferred      = cBucketMinimum;


    m_stateCur              = stateGrow;


    m_cCompletions          = 0;

#ifdef DHT_STATS


    m_cBucketOverflowInsert = 0;
    m_cBucketOverflowDelete = 0;
    m_cBucketSplit          = 0;
    m_cBucketMerge          = 0;
    m_cDirSplit             = 0;
    m_cDirMerge             = 0;
    m_cTransition           = 0;
    m_cSelection            = 0;
    m_cSplitContend         = 0;
    m_cMergeContend         = 0;

#endif


    m_rghs = (HOTSTUFF*)PvMEMAlloc( m_chs * sizeof( HOTSTUFF ), cbCacheLine );
    if ( !m_rghs )
    {
        err = ERR::errOutOfMemory;
        goto HandleError;
    }


    for ( ihs = 0; ihs < m_chs; ihs++ )
    {
        new( m_rghs + ihs ) HOTSTUFF();
    }


    err = ErrDIRInit( NativeCounter( dblLoadFactor * dblUniformity ), cBucketMinimum );
    if ( err != ERR::errSuccess )
    {
        goto HandleError;
    }

#ifdef DEBUG
    m_fInit = fTrue;
#endif

    return ERR::errSuccess;

HandleError:
    DHTAssert( err != ERR::errSuccess );
    Term();
    return err;
}



template< class CKey, class CEntry >
inline void CDynamicHashTable< CKey, CEntry >::
Term()
{
#ifdef DEBUG
    m_fInit = fFalse;
#endif


    DIRTerm();

    if ( NULL != m_rghs )
    {


        while ( m_chs )
        {


            m_chs--;
            m_rghs[ m_chs ].HOTSTUFF::~HOTSTUFF();
        }
        MEMFree( m_rghs );
        m_rghs = NULL;
    }
}



template< class CKey, class CEntry >
inline void CDynamicHashTable< CKey, CEntry >::
ReadLockKey( const CKey& key, CLock* const plock )
{
    DHTAssert( m_fInit );


    DHTAssert( plock->m_ls == CLock::lsNil );


    plock->m_ls = CLock::lsRead;


    const INT iGroup = UiSTEnter( &plock->m_phs );
    const ENUMSTATE esCurrent = EsSTGetState();


    DIRReadLockKey( esCurrent, key, plock );


    BKTSeek( plock, key );


    STLeave( iGroup, plock->m_phs );
}



template< class CKey, class CEntry >
inline void CDynamicHashTable< CKey, CEntry >::
ReadUnlockKey( CLock* const plock )
{
    DHTAssert( m_fInit );


    DHTAssert( FBKTRead( plock ) );
    DHTAssert( plock->m_pBucketHead != NULL );
    DHTAssert( plock->m_pBucketHead->CRWL().FReader() );


    DIRReadUnlockKey( plock );


    plock->m_ls = CLock::lsNil;
}



template< class CKey, class CEntry >
inline void CDynamicHashTable< CKey, CEntry >::
WriteLockKey( const CKey& key, CLock* const plock )
{
    BOOL fLockSucceeded = FTryWriteLockKey( key, plock, fFalse );
    DHTAssert( fLockSucceeded );
    return;
}

template< class CKey, class CEntry >
inline BOOL CDynamicHashTable< CKey, CEntry >::
FTryWriteLockKey( const CKey& key, CLock* const plock, BOOL fCanFail)
{
    DHTAssert( m_fInit );


    DHTAssert( plock->m_ls == CLock::lsNil );


    plock->m_ls                 = CLock::lsWrite;
    plock->m_fInsertOrDelete    = fFalse;


    const INT iGroup = UiSTEnter( &plock->m_phs );
    const ENUMSTATE esCurrent = EsSTGetState();


    BOOL fLockSucceeded = FDIRTryWriteLockKey( esCurrent, key, plock, fCanFail);
    if ( !fLockSucceeded )
    {
        plock->m_ls = CLock::lsNil;
        STLeave( iGroup, plock->m_phs );
        return fLockSucceeded;
    }
    

    BKTSeek( plock, key );


    STLeave( iGroup, plock->m_phs );

    return fTrue;
}



template< class CKey, class CEntry >
inline void CDynamicHashTable< CKey, CEntry >::
WriteUnlockKey( CLock* const plock )
{
    DHTAssert( m_fInit );


    DHTAssert( FBKTWrite( plock ) );
    DHTAssert( plock->m_pBucketHead != NULL );
    DHTAssert( plock->m_pBucketHead->CRWL().FWriter() );


    DIRWriteUnlockKey( plock );


    if ( plock->m_fInsertOrDelete )
    {

        MaintainTable( plock->m_phs );
    }


    plock->m_ls                 = CLock::lsNil;
    plock->m_fInsertOrDelete    = fFalse;
}



template< class CKey, class CEntry >
inline typename CDynamicHashTable< CKey, CEntry >::ERR CDynamicHashTable< CKey, CEntry >::
ErrRetrieveEntry( CLock* const plock, CEntry* const pentry )
{
    DHTAssert( m_fInit );


    DHTAssert( FBKTRead( plock ) || FBKTWrite( plock ) || FBKTScan( plock ) );
    DHTAssert( plock->m_pBucketHead != NULL );
#ifdef DEBUG
    if ( FBKTRead( plock ) )
    {
        DHTAssert( plock->m_pBucketHead->CRWL().FReader() );
    }
    else
    {
        DHTAssert( plock->m_pBucketHead->CRWL().FWriter() );
    }
    if ( FBKTRead( plock ) || FBKTWrite( plock ) )
    {
        CKeyEntry *pKeyEntry;
        BKTGetEntry( plock, &pKeyEntry );
        DHTAssert( pKeyEntry ? pKeyEntry->FEntryMatchesKey( plock->m_key ) : fTrue );
    }
#endif


    return ErrBKTGetEntry( plock, pentry );
}



template< class CKey, class CEntry >
inline typename CDynamicHashTable< CKey, CEntry >::ERR CDynamicHashTable< CKey, CEntry >::
ErrReplaceEntry( CLock* const plock, const CEntry& entry )
{
    DHTAssert( m_fInit );


    DHTAssert( FBKTWrite( plock ) || FBKTScan( plock ) );
    DHTAssert( plock->m_pBucketHead != NULL );
    DHTAssert( plock->m_pBucketHead->CRWL().FWriter() );
#ifdef DEBUG
    if ( FBKTWrite( plock ) )
    {
        CKeyEntry *pKeyEntry;
        BKTGetEntry( plock, &pKeyEntry );
        DHTAssert( pKeyEntry ? pKeyEntry->FEntryMatchesKey( plock->m_key ) : fTrue );
        DHTAssert( ((CKeyEntry &)entry).FEntryMatchesKey( plock->m_key ) );
    }
#endif


    return ErrBKTReplaceEntry( plock, entry );
}



template< class CKey, class CEntry >
inline typename CDynamicHashTable< CKey, CEntry >::ERR CDynamicHashTable< CKey, CEntry >::
ErrInsertEntry( CLock* const plock, const CEntry& entry )
{
    DHTAssert( m_fInit );


    DHTAssert( FBKTWrite( plock ) );
    DHTAssert( plock->m_pBucketHead != NULL );
    DHTAssert( plock->m_pBucketHead->CRWL().FWriter() );


    const ERR err = ErrBKTInsertEntry( plock, entry );

    if ( ERR::errSuccess == err )
    {


        STATInsertEntry( plock->m_phs );


        plock->m_fInsertOrDelete = fTrue;
    }

    return err;
}



template< class CKey, class CEntry >
inline typename CDynamicHashTable< CKey, CEntry >::ERR CDynamicHashTable< CKey, CEntry >::
ErrDeleteEntry( CLock* const plock )
{
    DHTAssert( m_fInit );


    DHTAssert( FBKTWrite( plock ) || FBKTScan( plock ) );
    DHTAssert( plock->m_pBucketHead != NULL );
    DHTAssert( plock->m_pBucketHead->CRWL().FWriter() );
#ifdef DEBUG
    if ( FBKTWrite( plock ) )
    {
        CKeyEntry *pKeyEntry;
        BKTGetEntry( plock, &pKeyEntry );
        DHTAssert( pKeyEntry ? pKeyEntry->FEntryMatchesKey( plock->m_key ) : fTrue );
    }
#endif

    if ( FBKTScan( plock ) )
    {


        DHTAssert( plock->m_pBucket != NULL );
        DHTAssert( plock->m_pEntryNext == NULL );
        plock->m_pEntryNext = ( plock->m_pEntry != plock->m_pBucket->m_pEntryLast ) ? plock->m_pEntry : NULL;
    }


    const ERR err = ErrBKTDeleteEntry( plock );

    if ( ERR::errSuccess == err )
    {


        STATDeleteEntry( plock->m_phs );


        plock->m_fInsertOrDelete = fTrue;
    }

    return err;
}




template< class CKey, class CEntry >
inline void CDynamicHashTable< CKey, CEntry >::
BeginHashScan( CLock* const plock )
{
    DHTAssert( m_fInit );


    DHTAssert( plock->m_ls == CLock::lsNil );


    plock->m_ls                 = CLock::lsScan;
    plock->m_fInsertOrDelete    = fFalse;
    plock->m_iBucket            = 0;


    const INT iGroup = UiSTEnter( &plock->m_phs );
    const ENUMSTATE esCurrent = EsSTGetState();


    DHTAssert( plock->m_pBucketHead == NULL );
    plock->m_pBucketHead = PbucketDIRIHash( esCurrent, plock->m_iBucket );


    plock->m_pBucketHead->CRWL().EnterAsWriter();



    STLeave( iGroup, plock->m_phs );


    plock->m_pBucket    = plock->m_pBucketHead;
    plock->m_pEntryPrev = NULL;
    plock->m_pEntry     = NULL;
    plock->m_pEntryNext = plock->m_pBucketHead->m_pb != NULL ? &plock->m_pBucketHead->m_rgEntry[0] : NULL;
}



template< class CKey, class CEntry >
inline void CDynamicHashTable< CKey, CEntry >::
BeginHashScanFromKey( const CKey& key, CLock* const plock )
{
    NativeCounter   cBucket;
    NativeCounter   cBucketMax;
    NativeCounter   iHash;

    DHTAssert( m_fInit );


    DHTAssert( plock->m_ls == CLock::lsNil );


    plock->m_ls                 = CLock::lsScan;
    plock->m_fInsertOrDelete    = fFalse;


    const INT iGroup = UiSTEnter( &plock->m_phs );
    const ENUMSTATE esCurrent = EsSTGetState();


    DIRWriteLockKey( esCurrent, key, plock );


    cBucket = NcDIRIGetBucket( esCurrent );
    cBucketMax = NcDIRIGetBucketMax( esCurrent );
    DHTAssert( cBucketMax != 0 );


    iHash = CKeyEntry::Hash( key );
    iHash = iHash & ( ( cBucketMax - 1 ) + cBucketMax );
    if ( iHash >= cBucketMax + cBucket )
        iHash -= cBucketMax;


    plock->m_iBucket = iHash;

#ifdef DEBUG
{

    NativeCounter   iExponent;
    NativeCounter   iRemainder;
    DIRILog2( iHash, &iExponent, &iRemainder );
    const PBUCKET pbucketT = PbucketDIRIResolve( iExponent, iRemainder );
    DHTAssert( pbucketT == plock->m_pBucketHead );
    DHTAssert( pbucketT->CRWL().FWriter() );
}
#endif


    STLeave( iGroup, plock->m_phs );


    plock->m_pBucket    = plock->m_pBucketHead;
    plock->m_pEntryPrev = NULL;
    plock->m_pEntry     = NULL;
    plock->m_pEntryNext = plock->m_pBucketHead->m_pb != NULL ? &plock->m_pBucketHead->m_rgEntry[0] : NULL;
}




template< class CKey, class CEntry >
inline typename CDynamicHashTable< CKey, CEntry >::ERR CDynamicHashTable< CKey, CEntry >::
ErrMoveNext( CLock* const plock, BOOL* const pfNewBucket )
{
    DHTAssert( m_fInit );


    DHTAssert( FBKTScan( plock ) );
    DHTAssert( plock->m_pEntryPrev == NULL );


    if ( plock->m_pEntry )
    {


        if ( plock->m_pEntry + 1 < PentryBKTNextMost( plock->m_pBucket ) )
        {


            plock->m_pEntry++;
        }
        else
        {


            plock->m_pBucket = PbucketBKTNext( plock->m_pBucket );
            if ( plock->m_pBucket )
            {


                plock->m_pEntry = &plock->m_pBucket->m_rgEntry[0];
            }
            else
            {


                plock->m_pEntry = NULL;
            }
        }
    }
    else
    {


        plock->m_pEntry = plock->m_pEntryNext;
    }
    plock->m_pEntryNext = NULL;

    if ( plock->m_pEntry != NULL )
    {


        DHTAssert( plock->m_pBucket );
        if ( pfNewBucket )
        {
            *pfNewBucket = fFalse;
        }
        return ERR::errSuccess;
    }


    if ( pfNewBucket )
    {
        *pfNewBucket = fTrue;
    }
    return ErrSCANMoveNext( plock );
}




template< class CKey, class CEntry >
inline void CDynamicHashTable< CKey, CEntry >::
EndHashScan( CLock* const plock )
{
    DHTAssert( m_fInit );


    DHTAssert( FBKTScan( plock ) );
    DHTAssert( plock->m_pEntryPrev == NULL );

    if ( plock->m_pBucketHead != NULL )
    {


        plock->m_pBucketHead->CRWL().LeaveAsWriter();
        plock->m_pBucketHead = NULL;


        if ( plock->m_fInsertOrDelete )
        {

            MaintainTable( plock->m_phs );
        }
    }


    plock->m_ls                 = CLock::lsNil;
    plock->m_fInsertOrDelete    = fFalse;
}


NAMESPACE_END( DHT );

using namespace DHT;


#endif

