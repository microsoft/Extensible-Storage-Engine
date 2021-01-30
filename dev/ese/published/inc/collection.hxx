// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _COLLECTION_HXX_INCLUDED
#define _COLLECTION_HXX_INCLUDED


#ifdef COLLAssert
#else
#define COLLAssert Assert

#endif

#ifdef COLLAssertSz
#else
#define COLLAssertSz AssertSz
#endif

#ifdef COLLEnforce
#else
#define COLLEnforce Enforce
#endif

#ifdef COLLAssertTrack
#else
#define COLLAssertTrack( X ) AssertTrack( ( X ), OSFormat( "COLLECTION_LINE:%d", __LINE__ ) )
#endif

#define APPROXIMATE_INDEX_STATS 1

#include "dht.hxx"

#include <memory.h>
#include <minmax.h>

#pragma warning ( disable : 4786 )


typedef SIZE_T ( *PfnOffsetOf )();

NAMESPACE_BEGIN( COLL );


template< class CObject, PfnOffsetOf OffsetOfIAE >
class CInvasiveConcurrentModSet
{
private:
    class CMarkedIndex
    {
    public:


        CMarkedIndex( ) : m_ulpMarkedIndex( 0 ) {};
        ~CMarkedIndex() {}

        VOID operator = ( const ULONG &ulRealIndex )
        {
            m_ulpMarkedIndex = ( ( ulRealIndex << 1 ) | 1 );
        };

        operator ULONG() const
        {
            COLLEnforce( FInitialized() );

            return ( ULONG )( m_ulpMarkedIndex >> 1 );
        };

        BOOL FInitialized() const { return ( m_ulpMarkedIndex & 1 ); }
        VOID Uninitialize() { COLLEnforce( FInitialized() ); m_ulpMarkedIndex = 0; };

    private:
        ULONG_PTR m_ulpMarkedIndex;
    };

    struct ARRAY_VALUE
    {

    public:
        ARRAY_VALUE() { m_miNextFree = 0; }
        ~ARRAY_VALUE() {}

        VOID operator = ( CObject *pObject )
        {
            COLLAssertTrack( nullptr != pObject );
            
            COLLAssertTrack( m_miNextFree.FInitialized() );
            
            m_pObject = pObject;
        }

        VOID operator = ( const ULONG &ulIndex )
        {
            if ( m_miNextFree.FInitialized() )
            {
            }
            else
            {
                COLLAssertTrack( 0 == ulIndex );
            }
            m_miNextFree = ulIndex;
        }

        operator CObject *() const
        {
            COLLAssertTrack( FIsObject() );

            return m_pObject;
        }

        operator ULONG() const
        {
            COLLAssertTrack( !FIsObject() );

            return m_miNextFree;
        }

        BOOL FIsObject() const
        {
            COLLAssert( nullptr != m_pObject );

            return !(m_miNextFree.FInitialized());
        }

        VOID Uninitialize()
        {
            m_pObject = nullptr;
        }
        
        BOOL FInitialized() const
        {
            return ( nullptr != m_pObject );
        }
        
    private:
        union {
            CMarkedIndex m_miNextFree;
            CObject      *m_pObject;
        };
    };

    enum class VALIDATION_LEVEL {
        vlModificationLocked = 0,
        vlEnumerationLocked = 1,
        vlEnumerationLockedAndCompact = 2
    };

    union FREE_LIST_HEAD {
        struct {
            ULONG ulSequence;
            ULONG ulIndexFirstFree;
        };
        unsigned __int64 ullFreeListHead;

        bool operator==(const FREE_LIST_HEAD &flhOther) const { return ( ullFreeListHead == flhOther.ullFreeListHead ); };
        bool operator!=(const FREE_LIST_HEAD &flhOther) const { return ( ullFreeListHead != flhOther.ullFreeListHead ); };

        FREE_LIST_HEAD( unsigned __int64 ullValue = 0 ) { ullFreeListHead = ullValue; };
        FREE_LIST_HEAD( ULONG ulIndexIn, ULONG ulSequenceIn ) { ulSequence = ulSequenceIn; ulIndexFirstFree = ulIndexIn; };
    };

public:
    class CElement
    {
    public:


        CElement() {}
        ~CElement() {}

        BOOL FUninitialized() const { return !m_miArrayIndex.FInitialized(); }

    private:

        CElement& operator=( CElement& );

        friend class CInvasiveConcurrentModSet< CObject, OffsetOfIAE >;

        VOID _Uninitialize() { m_miArrayIndex.Uninitialize(); }

        VOID _SetArrayIndex( const ULONG &ulArrayIndex ) { m_miArrayIndex = ulArrayIndex; }

        ULONG _GetArrayIndex() { return m_miArrayIndex; }

        CMarkedIndex m_miArrayIndex;
    };


    enum class ERR
    {
        errSuccess = 0,
        errOutOfMemory
    };

private:

    union
    {
        ARRAY_VALUE *m_prgValueArray;
        CObject *m_pObject;
    };

    ULONG   m_ulArrayAllocated;

    ULONG  m_ulArrayUsed;

    CReaderWriterLock m_rwl;

    FREE_LIST_HEAD m_flhFreeListHead;

    static const ULONG m_ulArrayPageSize = 0x1000;
    static const ULONG m_ulInitialArrayCount = 0x10;
    static const ULONG m_ulArrayPageCount = ( m_ulArrayPageSize / sizeof( CObject * ) );

    static_assert( 0 == ( (m_ulArrayPageCount / m_ulInitialArrayCount) & ( ( m_ulArrayPageCount / m_ulInitialArrayCount) - 1 ) ),
                   "Number of elements in a page must be a power of 2 times initial array size." );

    
    static BOOL fDoExtensiveValidation;

    static BOOL fAssertOnCountAndSize;

public:
    CInvasiveConcurrentModSet( const char* szRWLName, const INT iRWLRank );
    ~CInvasiveConcurrentModSet( );

    INT Count()        { COLLAssert( !fAssertOnCountAndSize || FLockedForEnumeration() ); return ( FEmbeddedSinglePointer_() ? m_ulArrayUsed : (m_ulArrayUsed - 1) ); }; 

    INT Size() { COLLAssert( !fAssertOnCountAndSize || FLockedForModification() || FLockedForEnumeration() ); return ( FEmbeddedSinglePointer_() ? 0 : m_ulArrayAllocated ); };

    VOID LockForModification();
    VOID UnlockForModification();
#ifdef DEBUG
    BOOL FLockedForModification();
    BOOL FNotLockedForModification();
#endif

    VOID LockForEnumeration();
    VOID UnlockForEnumeration();
#ifdef DEBUG
    BOOL FLockedForEnumeration();
    BOOL FNotLockedForEnumeration();
#endif

    ERR ErrInsert( CObject *pObject );      
    ERR ErrInsertReserveSpace();
    VOID Remove( CObject *pObject );

    CObject* operator[] ( ULONG ulArrayIndex );

#ifdef DEBUGGER_EXTENSION
    VOID Dump( CPRINTF * pcprintf, const DWORD_PTR dwOffset = 0 ) const;
#endif

    VOID EnableExtensiveValidation() { fDoExtensiveValidation = fTrue; }
    VOID DisableExtensiveValidation() { fDoExtensiveValidation = fFalse; }
    VOID EnableAssertOnCountAndSize() { fAssertOnCountandSize = fTrue; }
    VOID DisableAssertOnCountAndSize() { fAssertOnCountAndSize = fFalse; }

private:
    VOID Compact_();

    ERR Grow_();

    BOOL FEmbeddedSinglePointer_();

    VOID RenumberFreeList_();

    VOID AssertValid_( VALIDATION_LEVEL eLevel );

    VOID EnforceValidIndex_( ULONG ulArrayIndex );

    CElement* _PiaeFromPobj( const CObject* const pobj ) const;    
};

template< class CObject, PfnOffsetOf OffsetOfIAE >
BOOL CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::fDoExtensiveValidation = fFalse;

template< class CObject, PfnOffsetOf OffsetOfIAE >
BOOL CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::fAssertOnCountAndSize = fTrue;

template < class CObject, PfnOffsetOf OffsetOfIAE >
inline VOID CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::EnforceValidIndex_( ULONG ulArrayIndex )
{
    CObject *pObject;

    if ( 0 == ulArrayIndex )
    {
        COLLAssertTrack( FEmbeddedSinglePointer_() );
        pObject = m_pObject;
    }
    else
    {
        COLLAssertTrack( !FEmbeddedSinglePointer_() );
        pObject = m_prgValueArray[ ulArrayIndex ];
    }

    COLLAssertTrack( ulArrayIndex < m_ulArrayAllocated );
    COLLEnforce( !(_PiaeFromPobj( pObject )->FUninitialized() ) );
    COLLAssertTrack( ulArrayIndex == _PiaeFromPobj( pObject )->_GetArrayIndex() );
}

template< class CObject, PfnOffsetOf OffsetOfIAE >
inline CInvasiveConcurrentModSet<CObject, OffsetOfIAE>::CInvasiveConcurrentModSet( const char* szRWLName, const INT iRWLRank )
    : m_pObject( nullptr ),
      m_ulArrayAllocated( 0 ),
      m_ulArrayUsed( 0 ),
      m_flhFreeListHead( 0 ),
      m_rwl( CLockBasicInfo( CSyncBasicInfo( szRWLName ), iRWLRank, 0 ) )
{
    m_ulArrayAllocated = 1;

    static_assert( sizeof( m_flhFreeListHead ) == sizeof( unsigned __int64 ), "Expected" );
    static_assert( sizeof( m_flhFreeListHead ) == sizeof( m_flhFreeListHead.ullFreeListHead ), "Expected" );
}

template< class CObject, PfnOffsetOf OffsetOfIAE >
inline CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::~CInvasiveConcurrentModSet()
{
    COLLAssert( FNotLockedForEnumeration() );
    COLLAssert( FNotLockedForModification() );
    AssertValid_( VALIDATION_LEVEL::vlEnumerationLocked );

    if ( FEmbeddedSinglePointer_() )
    {
        if ( nullptr != m_pObject ) {
            EnforceValidIndex_( 0 );
            _PiaeFromPobj( m_pObject )->_Uninitialize();
        }
    }
    else
    {
        ULONG ulHits = 0;
        ulHits++;

        for ( ULONG ulArrayIndex = 1; ulArrayIndex < m_ulArrayAllocated; ulArrayIndex++ )
        {
            if ( m_prgValueArray[ ulArrayIndex ].FIsObject() )
            {
                EnforceValidIndex_( ulArrayIndex );
                _PiaeFromPobj( m_prgValueArray[ ulArrayIndex ] )->_Uninitialize();

                ulHits++;
                if ( ulHits >= m_ulArrayUsed )
                {
                    break;
                }
            }
        }

        delete [] m_prgValueArray;
    }
}


template < class CObject, PfnOffsetOf OffsetOfIAE >
inline VOID CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::AssertValid_( VALIDATION_LEVEL vlLevel )
{

    if ( !fDoExtensiveValidation )
    {
        return;
    }
    
    BOOL fEmbedded = FEmbeddedSinglePointer_();

    COLLAssert( m_flhFreeListHead.ulIndexFirstFree < m_ulArrayAllocated );

    switch ( vlLevel )
    {
        case VALIDATION_LEVEL::vlModificationLocked:
            break;

        case VALIDATION_LEVEL::vlEnumerationLocked:
        case VALIDATION_LEVEL::vlEnumerationLockedAndCompact:

            COLLAssert( m_ulArrayUsed <= m_ulArrayAllocated );

            if ( fEmbedded )
            {
                if ( nullptr == m_pObject ) {
                    COLLAssert( 0 == m_ulArrayUsed );
                }
                else
                {
                    COLLAssert( 1 == m_ulArrayUsed );
                    COLLAssert( 0 == _PiaeFromPobj( m_pObject )->_GetArrayIndex() );
                }

            }
            else
            {
                ULONG ulUsed = 0;
                ULONG ulFree = 0;

                ulUsed++;

                for ( ULONG ulArrayIndex = 1; ulArrayIndex < m_ulArrayAllocated; ulArrayIndex++ )
                {
                    if ( m_prgValueArray[ ulArrayIndex ].FIsObject() )
                    {
                        COLLAssert( nullptr != m_prgValueArray[ ulArrayIndex ] );
                        COLLAssert( ulArrayIndex == _PiaeFromPobj( m_prgValueArray[ ulArrayIndex ] )->_GetArrayIndex() );
                        ulUsed++;
                    }
                    else
                    {
                        COLLAssert( m_ulArrayAllocated > m_prgValueArray[ ulArrayIndex ] );
                        ulFree++;
                    }

                }

                COLLAssert( ulUsed == m_ulArrayUsed );
                COLLAssert( ( ulUsed + ulFree ) == m_ulArrayAllocated );

                ULONG ulArrayIndex = m_flhFreeListHead.ulIndexFirstFree;
                ULONG ulWalked = 0;
                while ( ulArrayIndex != 0 ) {
                    if ( vlLevel == VALIDATION_LEVEL::vlEnumerationLockedAndCompact )
                    {
                        COLLAssert( ulArrayIndex >= m_ulArrayUsed );
                    }
                    COLLAssert( !m_prgValueArray[ ulArrayIndex ].FIsObject() );
                    ulArrayIndex = m_prgValueArray[ ulArrayIndex];
                    ulWalked++;
                    if ( ulWalked > ulFree )
                    {
                        break;
                    }
                }

                if ( ulWalked < ulFree )
                {
                    COLLAssertSz( fFalse, "Early termination in the embedded free list." );
                }
                else if ( ulWalked > ulFree )
                {
                    COLLAssertSz( fFalse, "Loop in the embedded free list." );
                }
            }
            break;

        default:
            COLLAssertSz( fFalse, "Unknown validation level." );
            break;
    }
}

template < class CObject, PfnOffsetOf OffsetOfIAE >
inline BOOL CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::FEmbeddedSinglePointer_()
{


    switch ( m_ulArrayAllocated ) {
        case 0:
            COLLEnforce( fFalse );
            break;

        case 1:
            COLLAssertTrack( 0 == m_flhFreeListHead.ulIndexFirstFree );
            return fTrue;

        case 2:
            COLLEnforce( fFalse );
            break;

        default:
            COLLAssertTrack( nullptr != m_prgValueArray );
            COLLAssertTrack( !(m_prgValueArray[ 0 ].FInitialized()) );
            COLLAssertTrack( 0 < m_ulArrayUsed );
            break;
    }


    return fFalse;
}

template < class CObject, PfnOffsetOf OffsetOfIAE >
inline VOID CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::RenumberFreeList_()
{
    COLLAssert( FLockedForEnumeration() );
    COLLAssert( FNotLockedForModification() );

    COLLAssertTrack( !FEmbeddedSinglePointer_() );

    m_flhFreeListHead.ulIndexFirstFree = m_ulArrayUsed;
    m_flhFreeListHead.ulSequence += 1;

    AssertValid_( VALIDATION_LEVEL::vlModificationLocked );

    ULONG ulArrayIndex;

    for ( ulArrayIndex = m_ulArrayUsed; ulArrayIndex < ( m_ulArrayAllocated - 1 ) ; ulArrayIndex++ )
    {
        m_prgValueArray[ ulArrayIndex ] = ( ulArrayIndex + 1 );
    }

    m_prgValueArray[ ulArrayIndex ] = (ULONG)0;
}

template < class CObject, PfnOffsetOf OffsetOfIAE >
inline typename CInvasiveConcurrentModSet< CObject, OffsetOfIAE>::ERR CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::Grow_()
{
    COLLAssert( FLockedForEnumeration() );
    COLLAssert( FNotLockedForModification() );

    if ( !FEmbeddedSinglePointer_() && ( m_ulArrayAllocated > m_ulArrayUsed ) )
    {
        return ERR::errSuccess;
    }

    ULONG ulArrayAllocated;
    ARRAY_VALUE * prgValueArray;

    if ( FEmbeddedSinglePointer_() )
    {
        ulArrayAllocated = m_ulInitialArrayCount;
    }
    else
    {
        if ( ( m_ulArrayAllocated * 2 ) <= m_ulArrayPageCount )
        {
            ulArrayAllocated = m_ulArrayAllocated * 2;
        }
        else
        {
            ulArrayAllocated = m_ulArrayAllocated + m_ulArrayPageCount;
        }
    }

    {
        CMarkedIndex miTemp;
        miTemp = ( ulArrayAllocated - 1 );
        if ( miTemp != ( ulArrayAllocated - 1 ) )
        {
            return ERR::errOutOfMemory;
        }
    }

    prgValueArray = new ARRAY_VALUE [ ulArrayAllocated ];
    if ( nullptr == prgValueArray )
    {
        return ERR::errOutOfMemory;
    }

    if ( FEmbeddedSinglePointer_() )
    {

        prgValueArray[ 0 ].Uninitialize();

        if ( nullptr == m_pObject )
        {
            COLLAssertTrack( 0 == m_ulArrayUsed );
            m_ulArrayUsed = 1;
        }
        else
        {
            COLLAssertTrack( 1 == m_ulArrayUsed );


            m_ulArrayAllocated = 2;
            m_ulArrayUsed = 2;

            prgValueArray[ 1 ] = m_pObject;
            _PiaeFromPobj( m_pObject )->_SetArrayIndex( 1 );
        }
    }
    else
    {
        memcpy( prgValueArray, m_prgValueArray, m_ulArrayAllocated * sizeof( m_prgValueArray[ 0 ] ) );
        delete [] m_prgValueArray;
    }

    m_prgValueArray = prgValueArray;
    m_ulArrayAllocated = ulArrayAllocated;
    RenumberFreeList_();
    return ERR::errSuccess;
}

template < class CObject, PfnOffsetOf OffsetOfIAE >
inline VOID CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::Compact_()
{
    COLLAssert( FLockedForEnumeration() );
    COLLAssert( FNotLockedForModification() );

    if ( FEmbeddedSinglePointer_() )
    {
        return;
    }

    if ( m_ulArrayAllocated == m_ulArrayUsed )
    {
        return;
    }


    ULONG iTail = m_ulArrayAllocated - 1;
    ULONG iHead = 1;


    for ( ; ; )
    {
        for ( ; ( iHead < iTail ) && ( m_prgValueArray[ iHead ].FIsObject() ) ; iHead++ );

        for ( ; ( iTail > iHead ) && ( !m_prgValueArray[ iTail ].FIsObject() ); iTail-- );

        if ( iHead == iTail )
        {
            break;
        }

        EnforceValidIndex_( iTail );

        m_prgValueArray[ iHead ] = m_prgValueArray[ iTail ];
        _PiaeFromPobj( m_prgValueArray[ iHead ] )->_SetArrayIndex( iHead );
        m_prgValueArray[ iTail ] = (ULONG)0;

        EnforceValidIndex_( iHead );

        if ( iTail == m_ulArrayUsed )
        {
            break;
        }
    }

    if ( m_ulArrayAllocated > m_ulArrayPageCount )
    {
        ULONG ulArrayAllocated = 0;

        if ( m_ulArrayUsed == 1 )
        {
            ulArrayAllocated = m_ulArrayPageCount;
        }
        else if ( ( 3 * ( INT )m_ulArrayUsed ) < ( INT )m_ulArrayAllocated )
        {
            ulArrayAllocated = ( ( ( m_ulArrayAllocated / m_ulArrayPageCount ) + 1 ) / 2 ) * m_ulArrayPageCount;
        }

        if ( 0 != ulArrayAllocated )
        {
            ARRAY_VALUE *prgValueArray =  new ARRAY_VALUE [ ulArrayAllocated ];
            if ( nullptr != prgValueArray )
            {
                memcpy( prgValueArray, m_prgValueArray, ulArrayAllocated * sizeof( m_prgValueArray[ 0 ] ) );
                delete [] m_prgValueArray;
                m_prgValueArray = prgValueArray;
                m_ulArrayAllocated = ulArrayAllocated;
            }
        }
    }

    RenumberFreeList_();
}


template < class CObject, PfnOffsetOf OffsetOfIAE >
inline VOID CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::LockForModification()
{
    COLLAssert( FNotLockedForEnumeration() );
    COLLAssert( FNotLockedForModification() );

    m_rwl.EnterAsReader();

    AssertValid_( VALIDATION_LEVEL::vlModificationLocked );
    COLLAssert( FNotLockedForEnumeration() );
    COLLAssert( FLockedForModification() );
}

template < class CObject, PfnOffsetOf OffsetOfIAE >
inline VOID CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::UnlockForModification()
{
    COLLAssert( FNotLockedForEnumeration() );
    COLLAssert( FLockedForModification() );
    AssertValid_( VALIDATION_LEVEL::vlModificationLocked );

    m_rwl.LeaveAsReader();

    COLLAssert( FNotLockedForEnumeration() );
    COLLAssert( FNotLockedForModification() );
}


#ifdef DEBUG
template < class CObject, PfnOffsetOf OffsetOfIAE >
inline BOOL CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::FLockedForModification()
{
    return m_rwl.FReader();
}

template < class CObject, PfnOffsetOf OffsetOfIAE >
inline BOOL CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::FNotLockedForModification()
{
    return m_rwl.FNotReader();
}
#endif

template < class CObject, PfnOffsetOf OffsetOfIAE >
inline VOID CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::LockForEnumeration()
{
    COLLAssert( FNotLockedForEnumeration() );
    COLLAssert( FNotLockedForModification() );


    m_rwl.EnterAsWriter();
    AssertValid_( VALIDATION_LEVEL::vlEnumerationLocked );

    Compact_();
    AssertValid_( VALIDATION_LEVEL::vlEnumerationLockedAndCompact );
}

template < class CObject, PfnOffsetOf OffsetOfIAE >
inline VOID CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::UnlockForEnumeration()
{
    COLLAssert( FLockedForEnumeration() );
    COLLAssert( FNotLockedForModification() );
    AssertValid_( VALIDATION_LEVEL::vlEnumerationLockedAndCompact );

    m_rwl.LeaveAsWriter();

    COLLAssert( FNotLockedForEnumeration() );
    COLLAssert( FNotLockedForModification() );
}


#ifdef DEBUG
template < class CObject, PfnOffsetOf OffsetOfIAE >
inline BOOL CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::FLockedForEnumeration()
{
    return m_rwl.FWriter();
}

template < class CObject, PfnOffsetOf OffsetOfIAE >
inline BOOL CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::FNotLockedForEnumeration()
{
    return m_rwl.FNotWriter();
}
#endif

template < class CObject, PfnOffsetOf OffsetOfIAE >
inline typename CInvasiveConcurrentModSet< CObject, OffsetOfIAE>::ERR CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::ErrInsert( CObject *pObject )
{
    COLLAssert( FNotLockedForEnumeration() );
    COLLAssert( FLockedForModification() );
    AssertValid_( VALIDATION_LEVEL::vlModificationLocked );

    COLLEnforce( !( ( reinterpret_cast<CMarkedIndex *>( &pObject ) )->FInitialized() ) );

    COLLAssertTrack( _PiaeFromPobj( pObject )->FUninitialized() );


    for ( ; ; )
    {
        if ( FEmbeddedSinglePointer_() ) {
            CObject *pInitial = ( CObject * )AtomicCompareExchangePointer( ( PVOID * )&m_pObject, nullptr, pObject );
            if ( nullptr == pInitial ) {
                AtomicIncrement( &m_ulArrayUsed );
                _PiaeFromPobj( pObject )->_SetArrayIndex( 0 );
                EnforceValidIndex_( 0 );
                return ERR::errSuccess;
            }
        }
        else
        {
            OSSYNC_FOREVER
            {
                const FREE_LIST_HEAD flhOriginalHead = AtomicRead( &m_flhFreeListHead.ullFreeListHead ) ;
                if ( 0 == flhOriginalHead.ulIndexFirstFree )
                {
                    break;
                }

                COLLEnforce( flhOriginalHead.ulIndexFirstFree < m_ulArrayAllocated );

                const ARRAY_VALUE val = m_prgValueArray[ flhOriginalHead.ulIndexFirstFree ];

                if ( val.FIsObject() ) 
                {
                    COLLAssert( flhOriginalHead != AtomicRead( &m_flhFreeListHead.ullFreeListHead ) );
                    continue;
                }

                const FREE_LIST_HEAD flhNewHead = { val, ( flhOriginalHead.ulSequence + 1 ) };
                const FREE_LIST_HEAD flhCurrentHead = AtomicCompareExchange(
                    &m_flhFreeListHead.ullFreeListHead,
                    flhOriginalHead.ullFreeListHead,
                    flhNewHead.ullFreeListHead );

                if ( flhCurrentHead == flhOriginalHead )
                {
                    m_prgValueArray[ flhOriginalHead.ulIndexFirstFree ] = pObject;
                    AtomicIncrement( &m_ulArrayUsed );
                    _PiaeFromPobj( pObject )->_SetArrayIndex( flhOriginalHead.ulIndexFirstFree ); 
                    EnforceValidIndex_( flhOriginalHead.ulIndexFirstFree );
                    return ERR::errSuccess;
                }
            }
        }

        UnlockForModification();
        LockForEnumeration();
        ERR err = Grow_();
        UnlockForEnumeration();
        LockForModification();
        if ( ERR::errSuccess != err )
        {
            COLLAssertTrack( _PiaeFromPobj( pObject )->FUninitialized() );
            return err;
        }
    }
}

template < class CObject, PfnOffsetOf OffsetOfIAE >
inline typename CInvasiveConcurrentModSet< CObject, OffsetOfIAE>::ERR CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::ErrInsertReserveSpace( )
{
    COLLAssert( FNotLockedForEnumeration() );
    COLLAssert( FLockedForModification() );
    AssertValid_( VALIDATION_LEVEL::vlModificationLocked );


    if ( FEmbeddedSinglePointer_() )
    {
        if ( m_pObject == nullptr )
        {
            return ERR::errSuccess;
        }
    }
    else
    {
        const FREE_LIST_HEAD flhOriginalHead = AtomicRead( &m_flhFreeListHead.ullFreeListHead ) ;
        if ( 0 != flhOriginalHead.ulIndexFirstFree )
        {
            return ERR::errSuccess;
        }
    }

    UnlockForModification();
    LockForEnumeration();
    ERR err = Grow_();
    UnlockForEnumeration();
    LockForModification();
    return err;
}

template < class CObject, PfnOffsetOf OffsetOfIAE >
inline VOID CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::Remove( CObject *pObject )
{
    COLLAssert( FNotLockedForEnumeration() );
    COLLAssert( FLockedForModification() );
    AssertValid_( VALIDATION_LEVEL::vlModificationLocked );


    COLLEnforce( !(_PiaeFromPobj( pObject )->FUninitialized() ) );

    ULONG ulArrayIndex = _PiaeFromPobj( pObject )->_GetArrayIndex();

    COLLAssertTrack( ulArrayIndex < m_ulArrayAllocated );


    COLLEnforce( ulArrayIndex < m_ulArrayAllocated );

    if ( FEmbeddedSinglePointer_() ) {
        COLLEnforce( 0 == ulArrayIndex );
        COLLEnforce( m_pObject == pObject );
        EnforceValidIndex_( 0 );

        m_pObject = nullptr;
    }
    else
    {
        COLLEnforce( 0 != ulArrayIndex );
        COLLEnforce( m_prgValueArray[ ulArrayIndex ] == pObject );
        EnforceValidIndex_( ulArrayIndex );

        m_prgValueArray[ ulArrayIndex ] = (ULONG)0;

        OSSYNC_FOREVER
        {
            const FREE_LIST_HEAD flhOriginalHead = AtomicRead( &m_flhFreeListHead.ullFreeListHead );

            m_prgValueArray[ ulArrayIndex ] = flhOriginalHead.ulIndexFirstFree;

            const FREE_LIST_HEAD flhNewHead = { ulArrayIndex, ( flhOriginalHead.ulSequence + 1 ) };

            const FREE_LIST_HEAD flhCurrentHead = AtomicCompareExchange(
                &m_flhFreeListHead.ullFreeListHead,
                flhOriginalHead.ullFreeListHead,
                flhNewHead.ullFreeListHead );

            if ( flhCurrentHead == flhOriginalHead ) {
                break;
            }

        }
    }

    _PiaeFromPobj( pObject )->_Uninitialize();

    AtomicDecrement( &m_ulArrayUsed );
}


template < class CObject, PfnOffsetOf OffsetOfIAE >
inline CObject* CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::operator[] ( ULONG ulArrayIndex )
{
    COLLAssert( FLockedForEnumeration() );
    AssertValid_( VALIDATION_LEVEL::vlEnumerationLockedAndCompact );


    if ( FEmbeddedSinglePointer_() )
    {
        COLLAssert( 0 == ulArrayIndex );

        switch ( ulArrayIndex )
        {
        case 0:
            if ( nullptr != m_pObject )
            {
                EnforceValidIndex_( 0 );
                return m_pObject;
            }
            else
            {
                return nullptr;
            }

        default:
            return nullptr;
        }
    }
    else
    {
        ulArrayIndex += 1;

        if ( ulArrayIndex >= m_ulArrayUsed )
        {
            COLLAssert( 1 == ulArrayIndex );
            return nullptr;
        }

        EnforceValidIndex_( ulArrayIndex );

        return m_prgValueArray[ ulArrayIndex ];
    }
}


template< class CObject, PfnOffsetOf OffsetOfIAE >
inline typename CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::CElement* CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::
_PiaeFromPobj( const CObject* const pobj ) const
{
    return ( CElement* )( ( BYTE* )pobj + OffsetOfIAE() );
}


template< class CObject, PfnOffsetOf OffsetOfILE >
class CInvasiveList
{
    public:


        class CElement
        {
            public:


                CElement() : m_pilePrev( (CElement*)-1 ), m_pileNext( (CElement*)-1 ) {}
                ~CElement() {}

                BOOL FUninitialized() const { return ( ( m_pilePrev == (CElement*)-1 ) && ( m_pileNext == (CElement*)-1 ) ); }

            private:

                CElement& operator=( CElement& );

                friend class CInvasiveList< CObject, OffsetOfILE >;

                CElement*   m_pilePrev;
                CElement*   m_pileNext;
        };

    public:


        CInvasiveList();
        ~CInvasiveList();


        CInvasiveList& operator=( const CInvasiveList& il );


        BOOL FEmpty() const;

        BOOL FMember( const CObject* const pobj ) const;

        CObject* Prev( CObject* const pobj ) const;
        CObject* Next( CObject* const pobj ) const;

        CObject* PrevMost() const;
        CObject* NextMost() const;

        void InsertAsPrevMost( CObject* const pobj );
        void InsertAsNextMost( CObject* const pobj );

        void Insert( CObject* const pobj, CObject* const pobjNext );
        void Remove( CObject* const pobj );

        void Empty();

        CObject* Pnil() const { return _PobjFromPile( (COLL::CInvasiveList<CObject,OffsetOfILE>::CElement *)-1 ); }

    private:


        CObject* _PobjFromPile( CElement* const pile ) const;
        CElement* _PileFromPobj( const CObject* const pobj ) const;

    private:

        CElement*   m_pilePrevMost;
        CElement*   m_pileNextMost;
};


template< class CObject, PfnOffsetOf OffsetOfILE >
inline CInvasiveList< CObject, OffsetOfILE >::
CInvasiveList()
{

    Empty();
}


template< class CObject, PfnOffsetOf OffsetOfILE >
inline CInvasiveList< CObject, OffsetOfILE >::
~CInvasiveList()
{
}


template< class CObject, PfnOffsetOf OffsetOfILE >
inline CInvasiveList< CObject, OffsetOfILE >& CInvasiveList< CObject, OffsetOfILE >::
operator=( const CInvasiveList& il )
{
    m_pilePrevMost  = il.m_pilePrevMost;
    m_pileNextMost  = il.m_pileNextMost;
    return *this;
}


template< class CObject, PfnOffsetOf OffsetOfILE >
inline BOOL CInvasiveList< CObject, OffsetOfILE >::
FEmpty() const
{
    return m_pilePrevMost == _PileFromPobj( NULL );
}


template< class CObject, PfnOffsetOf OffsetOfILE >
inline BOOL CInvasiveList< CObject, OffsetOfILE >::
FMember( const CObject* const pobj ) const
{
#ifdef EXPENSIVE_DEBUG

    for ( CObject* pobjT = PrevMost(); pobjT && pobjT != pobj; pobjT = Next( pobjT ) )
    {
    }

    return pobjT == pobj;

#else

    return _PileFromPobj( pobj )->m_pilePrev != (CElement*)-1;

#endif
}


template< class CObject, PfnOffsetOf OffsetOfILE >
inline CObject* CInvasiveList< CObject, OffsetOfILE >::
Prev( CObject* const pobj ) const
{
    return _PobjFromPile( _PileFromPobj( pobj )->m_pilePrev );
}


template< class CObject, PfnOffsetOf OffsetOfILE >
inline CObject* CInvasiveList< CObject, OffsetOfILE >::
Next( CObject* const pobj ) const
{
    return _PobjFromPile( _PileFromPobj( pobj )->m_pileNext );
}


template< class CObject, PfnOffsetOf OffsetOfILE >
inline CObject* CInvasiveList< CObject, OffsetOfILE >::
PrevMost() const
{
    return _PobjFromPile( m_pilePrevMost );
}


template< class CObject, PfnOffsetOf OffsetOfILE >
inline CObject* CInvasiveList< CObject, OffsetOfILE >::
NextMost() const
{
    return _PobjFromPile( m_pileNextMost );
}


template< class CObject, PfnOffsetOf OffsetOfILE >
inline void CInvasiveList< CObject, OffsetOfILE >::
InsertAsPrevMost( CObject* const pobj )
{
    CElement* const pile = _PileFromPobj( pobj );


    COLLAssert( !FMember( pobj ) );


    COLLAssert( pile->m_pilePrev == (CElement*)-1 );
    COLLAssert( pile->m_pileNext == (CElement*)-1 );


    if ( m_pilePrevMost == _PileFromPobj( NULL ) )
    {

        pile->m_pilePrev            = _PileFromPobj( NULL );
        pile->m_pileNext            = _PileFromPobj( NULL );

        m_pilePrevMost              = pile;
        m_pileNextMost              = pile;
    }


    else
    {

        pile->m_pilePrev            = _PileFromPobj( NULL );
        pile->m_pileNext            = m_pilePrevMost;

        m_pilePrevMost->m_pilePrev  = pile;

        m_pilePrevMost              = pile;
    }
}


template< class CObject, PfnOffsetOf OffsetOfILE >
inline void CInvasiveList< CObject, OffsetOfILE >::
InsertAsNextMost( CObject* const pobj )
{
    CElement* const pile = _PileFromPobj( pobj );


    COLLAssert( !FMember( pobj ) );


    COLLAssert( pile->m_pilePrev == (CElement*)-1 );
    COLLAssert( pile->m_pileNext == (CElement*)-1 );


    if ( m_pileNextMost == _PileFromPobj( NULL ) )
    {

        pile->m_pilePrev            = _PileFromPobj( NULL );
        pile->m_pileNext            = _PileFromPobj( NULL );

        m_pilePrevMost              = pile;
        m_pileNextMost              = pile;
    }


    else
    {

        pile->m_pilePrev            = m_pileNextMost;
        pile->m_pileNext            = _PileFromPobj( NULL );

        m_pileNextMost->m_pileNext  = pile;

        m_pileNextMost              = pile;
    }
}


template< class CObject, PfnOffsetOf OffsetOfILE >
inline void CInvasiveList< CObject, OffsetOfILE >::
Insert( CObject* const pobj, CObject* const pobjNext )
{
    CElement* const pileNext = _PileFromPobj( pobjNext );


    COLLAssert( ( pileNext == _PileFromPobj( NULL ) ) || FMember( pobjNext ) );


    if ( pileNext == _PileFromPobj( NULL ) )
    {
        InsertAsNextMost( pobj );
    }
    else
    {

        if ( pileNext == m_pilePrevMost )
        {
            InsertAsPrevMost( pobj );
        }
        else
        {

            CElement* const pile = _PileFromPobj( pobj );


            COLLAssert( !FMember( pobj ) );


            COLLAssert( pile->m_pilePrev == (CElement*)-1 );
            COLLAssert( pile->m_pileNext == (CElement*)-1 );


            pile->m_pilePrev = pileNext->m_pilePrev;
            pile->m_pilePrev->m_pileNext = pile;


            pile->m_pileNext = pileNext;
            pileNext->m_pilePrev = pile;
        }
    }
}


template< class CObject, PfnOffsetOf OffsetOfILE >
inline void CInvasiveList< CObject, OffsetOfILE >::
Remove( CObject* const pobj )
{
    CElement* const pile = _PileFromPobj( pobj );


    COLLAssert( FMember( pobj ) );


    if ( pile->m_pileNext != _PileFromPobj( NULL ) )
    {

        pile->m_pileNext->m_pilePrev    = pile->m_pilePrev;
    }
    else
    {

        m_pileNextMost                  = pile->m_pilePrev;
    }


    if ( pile->m_pilePrev != _PileFromPobj( NULL ) )
    {

        pile->m_pilePrev->m_pileNext    = pile->m_pileNext;
    }
    else
    {

        m_pilePrevMost                  = pile->m_pileNext;
    }


    pile->m_pilePrev                    = (CElement*)-1;
    pile->m_pileNext                    = (CElement*)-1;
}


template< class CObject, PfnOffsetOf OffsetOfILE >
inline void CInvasiveList< CObject, OffsetOfILE >::
Empty()
{
    m_pilePrevMost  = _PileFromPobj( NULL );
    m_pileNextMost  = _PileFromPobj( NULL );
}


template< class CObject, PfnOffsetOf OffsetOfILE >
inline CObject* CInvasiveList< CObject, OffsetOfILE >::
_PobjFromPile( CElement* const pile ) const
{
    return (CObject*)( (BYTE*)pile - OffsetOfILE() );
}


template< class CObject, PfnOffsetOf OffsetOfILE >
inline typename CInvasiveList< CObject, OffsetOfILE >::CElement* CInvasiveList< CObject, OffsetOfILE >::
_PileFromPobj( const CObject* const pobj ) const
{
    return (CElement*)( (BYTE*)pobj + OffsetOfILE() );
}


template< class CObject, PfnOffsetOf OffsetOfILE >
class CCountedInvasiveList : public CInvasiveList<CObject, OffsetOfILE>
{
    public:


        CCountedInvasiveList()
            :   CInvasiveList<CObject, OffsetOfILE>(),
                m_c( 0 )
        {
        }


        CCountedInvasiveList& operator=( const CCountedInvasiveList& il )
        {
            m_c = il.m_c;

            CInvasiveList<CObject, OffsetOfILE>::operator=( il );

            return *this;
        }


        void InsertAsPrevMost( CObject* const pobj )
        {
            CInvasiveList<CObject, OffsetOfILE>::InsertAsPrevMost( pobj );
            m_c++;
        }

        void InsertAsNextMost( CObject* const pobj )
        {
            CInvasiveList<CObject, OffsetOfILE>::InsertAsNextMost( pobj );
            m_c++;
        }

        void Insert( CObject* const pobj, CObject* const pobjNext )
        {
            CInvasiveList<CObject, OffsetOfILE>::Insert( pobj, pobjNext );
            m_c++;
        }

        void Remove( CObject* const pobj )
        {
            CInvasiveList<CObject, OffsetOfILE>::Remove( pobj );
            m_c--;
        }

        void Empty()
        {
            CInvasiveList<CObject, OffsetOfILE>::Empty();
            m_c = 0;
        }


        size_t Count() const
        {
            return m_c;
        }

    private:

        size_t m_c;
};


template< class CObject >
class CLocklessLinkedList
{
    private:

        CObject * volatile m_pHead;

        CObject ** Ppnext_( CObject * pobj, ULONG cbOffsetOfNext )
        {
            COLLAssert( ( cbOffsetOfNext + sizeof(CObject*) ) <= sizeof(CObject) );

            COLLAssert( pobj != *( (CObject**)( ( (BYTE*)pobj ) + cbOffsetOfNext ) ) );

            return (CObject**)( ( (BYTE*)pobj ) + cbOffsetOfNext );
        }

    public:

        CLocklessLinkedList( ) : m_pHead( NULL ) { }

        ~CLocklessLinkedList( )
        {
        }

        void AtomicInsertAsPrevMost( CObject * pobj, const ULONG cbOffsetOfNext )
        {
            COLLAssert( pobj );
            COLLAssert( ( cbOffsetOfNext + sizeof(CObject*) ) <= sizeof(CObject) );

            COLLAssert( NULL == *Ppnext_( pobj, cbOffsetOfNext ) );

            AtomicAddLinklistNode( pobj, (void**)Ppnext_( pobj, cbOffsetOfNext ), (void**)&m_pHead );
        }


        CLocklessLinkedList( CObject* phead ) : m_pHead( phead ) { }

        CObject * AtomicRemoveList()
        {
            return (CObject*) PvAtomicRetrieveLinklist( (void *volatile *)&m_pHead );
        }


        CObject * RemovePrevMost( const ULONG cbOffsetOfNext )
        {
            COLLAssert( ( cbOffsetOfNext + sizeof(CObject*) ) <= sizeof(CObject) );

            CObject * pobj = m_pHead;
            if ( pobj )
            {
                m_pHead = *Ppnext_( pobj, cbOffsetOfNext );
                *Ppnext_( pobj, cbOffsetOfNext ) = NULL;
            }

            return pobj;
        }

        BOOL FEmpty() const
        {
            return NULL == m_pHead;
        }

        CObject * Head() const 
        {
            return m_pHead;
        }

};



template< class CObject >
class CSimpleQueue
{
    private:
        CLocklessLinkedList<CObject>        m_Head;
        CObject *                           m_pTail;
        ULONG                               m_cObject;

        CObject ** Ppnext_( CObject * pobj, ULONG cbOffsetOfNext )
        {
            COLLAssert( ( cbOffsetOfNext + sizeof(CObject*) ) <= sizeof(CObject) );

            COLLAssert( pobj != *( (CObject**)( ( (BYTE*)pobj ) + cbOffsetOfNext ) ) );

            return (CObject**)( ( (BYTE*)pobj ) + cbOffsetOfNext );
        }

        const CObject ** Ppnext_( const CObject * pobj, ULONG cbOffsetOfNext )
        {
            return (const CObject **)Ppnext_( (CObject*)pobj, cbOffsetOfNext );
        }

        bool FValid() const
        {
            return ( ( ( m_Head.FEmpty() && NULL == m_pTail && 0 == m_cObject ) ||
                       ( !m_Head.FEmpty() && m_pTail && m_cObject ) ) &&
                     ( ( 1 != m_cObject ) || ( m_Head.Head() == m_pTail ) ) &&
                     ( ( 2 != m_cObject ) || ( m_Head.Head() != m_pTail ) )
                    );
        }

    public:

        CSimpleQueue( ) : m_Head(), m_pTail( NULL ), m_cObject( 0 )
        {
            COLLAssert( m_Head.FEmpty() );
        }

        CSimpleQueue( CObject * pobj, ULONG cbOffsetOfNext ) :
                m_Head( pobj ),
                m_pTail( NULL ),
                m_cObject( 0 )
        {
            COLLAssert( pobj );
            COLLAssert( !m_Head.FEmpty() );

            if ( !m_Head.FEmpty() )
            {
                COLLAssert( m_Head.Head() );
                COLLAssert( 0 == m_cObject );

                m_cObject = 0;

                for( CObject * pobj2 = m_Head.Head(); pobj2; pobj2 = *Ppnext_( pobj2, cbOffsetOfNext ) )
                {
                    COLLAssert( pobj2 !=  *Ppnext_( pobj2, cbOffsetOfNext ) );

                    if ( NULL == *Ppnext_( pobj2, cbOffsetOfNext ) )
                    {
                        m_pTail = pobj2;
                    }
                    m_cObject++;
                }

                COLLAssert( CElements() );
            }
            else
            {
                COLLAssert( NULL == m_Head.Head() );
                COLLAssert( 0 == CElements() );
            }

            COLLAssert( FValid() );
            COLLAssert( pobj ? !FEmpty() : FEmpty() );
        }

        ~CSimpleQueue( )
        {
        }

        void InsertAsPrevMost( CObject * pobj, const ULONG cbOffsetOfNext )
        {
            COLLAssert( FValid() );
            COLLAssert( pobj );
            COLLAssert( ( cbOffsetOfNext + sizeof(CObject*) ) <= sizeof(CObject) );

            COLLAssert( NULL == *Ppnext_( pobj, cbOffsetOfNext ) );

            m_Head.AtomicInsertAsPrevMost( pobj, cbOffsetOfNext );
            if ( NULL == m_pTail )
            {
                COLLAssert( NULL == *Ppnext_( m_Head.Head(), cbOffsetOfNext ) );

                m_pTail = pobj;
            }
            m_cObject++;

            COLLAssert( FValid() );
            COLLAssert( !FEmpty() );
        }

        void InsertAsNextMost( CObject * pobj, const ULONG cbOffsetOfNext )
        {
            COLLAssert( FValid() );
            COLLAssert( pobj );
            COLLAssert( ( cbOffsetOfNext + sizeof(CObject*) ) <= sizeof(CObject) );

            COLLAssert( NULL == *Ppnext_( pobj, cbOffsetOfNext ) );

            if ( m_pTail )
            {
                COLLAssert( NULL == *Ppnext_( m_pTail, cbOffsetOfNext ) );

                *Ppnext_( m_pTail, cbOffsetOfNext ) = pobj;
                m_pTail = pobj;
            }
            else
            {
                COLLAssert( NULL == m_Head.Head() );
                COLLAssert( FEmpty() && m_Head.FEmpty() );

                m_Head.AtomicInsertAsPrevMost( pobj, cbOffsetOfNext );
                m_pTail = pobj;
            }
            m_cObject++;

            COLLAssert( FValid() );
            COLLAssert( !FEmpty() );
        }

        CObject * RemoveList()
        {
            COLLAssert( FValid() );
            CObject * pobj = m_Head.AtomicRemoveList();
            COLLAssert( m_Head.FEmpty() );
            m_pTail = NULL;
            m_cObject = 0;
            COLLAssert( FValid() );
            COLLAssert( FEmpty() );
            return pobj;
        }

        CObject * RemovePrevMost( const ULONG cbOffsetOfNext )
        {
            COLLAssert( FValid() );
            COLLAssert( ( cbOffsetOfNext + sizeof(CObject*) ) <= sizeof(CObject) );

            CObject * pobj = m_Head.RemovePrevMost( cbOffsetOfNext );

            if ( NULL == m_Head.Head() )
            {

                m_pTail = NULL;
            }

            if ( pobj )
            {

                m_cObject--;
            }

            return pobj;
        }

        BOOL FEmpty() const
        {
            COLLAssert( FValid() );
            COLLAssert( ( m_Head.FEmpty() && NULL == m_pTail && 0 == m_cObject ) ||
                        ( !m_Head.FEmpty() && m_pTail && m_cObject ) );
            return m_Head.FEmpty();
        }

        ULONG CElements() const
        {
            COLLAssert( FValid() );
            return m_cObject;
        }

        CObject * Head() const
        {
            COLLAssert( FValid() );
            return m_Head.Head();
        }
        CObject * Tail() const
        {
            COLLAssert( FValid() );
            return m_pTail;
        }

};



template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
class CApproximateIndex
{
    public:


        class CInvasiveContext
        {
            public:

                CInvasiveContext() {}
                ~CInvasiveContext() {}

                static SIZE_T OffsetOfILE() { return OffsetOfIC() + OffsetOf( CInvasiveContext, m_ile ); }
                BOOL FUninitialized() const { return m_ile.FUninitialized(); }

            private:

                typename CInvasiveList< CEntry, OffsetOfILE >::CElement m_ile;
        };


        enum class ERR
        {
            errSuccess,
            errInvalidParameter,
            errOutOfMemory,
            errEntryNotFound,
            errNoCurrentEntry,
            errKeyRangeExceeded,
        };


        class CLock;

    public:


        CApproximateIndex( const INT Rank );
        ~CApproximateIndex();


        ERR ErrInit(    const CKey      dkeyPrecision,
                        const CKey      dkeyUncertainty,
                        const double    dblSpeedSizeTradeoff );
        void Term();

        void LockKeyPtr( const CKey& key, CEntry* const pentry, CLock* const plock);
        BOOL FTryLockKeyPtr( const CKey& key, CEntry* const pentry, CLock* const plock, BOOL fCanFail = fTrue );
        void UnlockKeyPtr( CLock* const plock );

        LONG CmpKey( const CKey& key1, const CKey& key2 ) const;

        CKey KeyRangeFirst() const;
        CKey KeyRangeLast() const;

        CKey KeyInsertLeast() const;
        CKey KeyInsertMost() const;

        ERR ErrRetrieveEntry( CLock* const plock, CEntry** const ppentry ) const;
        ERR ErrInsertEntry( CLock* const plock, CEntry* const pentry, const BOOL fNextMost = fTrue );
        ERR ErrDeleteEntry( CLock* const plock );
        ERR ErrReserveEntry( CLock* const plock );
        void UnreserveEntry( CLock* const plock );

        void MoveBeforeFirst( CLock* const plock );
        ERR ErrMoveNext( CLock* const plock );
        ERR ErrMovePrev( CLock* const plock );
        void MoveAfterLast( CLock* const plock );

        void MoveBeforeKeyPtr( const CKey& key, CEntry* const pentry, CLock* const plock );
        void MoveAfterKeyPtr( const CKey& key, CEntry* const pentry, CLock* const plock );

#ifdef DEBUGGER_EXTENSION
        VOID Dump( CPRINTF * pcprintf, const DWORD_PTR dwOffset = 0, const DWORD grbit = 0x0 ) const;
        VOID Scan( CPRINTF * pcprintf, VOID * pv ) const    { m_bt.Scan( pcprintf, pv ); }
#endif

    public:


        class CBucket
        {
            public:


                typedef ULONG ID;

            public:

                CBucket() {}
                ~CBucket() {}

                CBucket& operator=( const CBucket& bucket )
                {
                    m_id    = bucket.m_id;
                    m_cPin  = bucket.m_cPin;
                    m_il    = bucket.m_il;
                    return *this;
                }

            public:

                ID                                                      m_id;
                ULONG                                           m_cPin;
                CInvasiveList< CEntry, CInvasiveContext::OffsetOfILE >  m_il;
        };


        typedef CDynamicHashTable< typename CBucket::ID, CBucket > CBucketTable;

    public:


        class CLock
        {
            public:

                CLock()
                {
#ifdef APPROXIMATE_INDEX_STATS
                    ResetStats();
#endif
                }
                ~CLock() {}

            BOOL IsSameBucketHead(CLock *pLock)
            {
                return m_lock.IsSameBucketHead( &pLock->m_lock );
            }

            private:

                friend class CApproximateIndex< CKey, CEntry, OffsetOfIC >;

                typename CBucketTable::CLock    m_lock;
                CBucket             m_bucket;
                CEntry*             m_pentryPrev;
                CEntry*             m_pentry;
                CEntry*             m_pentryNext;

#ifdef APPROXIMATE_INDEX_STATS

                typename CBucket::ID            m_idStart;
                typename CBucket::ID            m_idEnd;
                ULONG               m_cEnumeratedEntries;
                ULONG               m_cEnumeratedBuckets;
                ULONG               m_cEnumeratedEmptyBuckets;
                ULONG               m_cEnumeratedEntriesPerLastBucket;
                void ResetStats()
                {
                    m_idStart = 0;
                    m_idEnd = 0;
                    m_cEnumeratedEntries = 0;
                    m_cEnumeratedBuckets = 0;
                    m_cEnumeratedEmptyBuckets = 0;
                    m_cEnumeratedEntriesPerLastBucket = 0;
                }

            public:
                static const ULONG      cchStatsString = 140;
                void SPrintStats( __in_bcount(cchStatsString) char rgStatsString[] )
                {

                    ULONG cRealEmptyBuckets = m_cEnumeratedEntries == 0 ?
                                                m_cEnumeratedEmptyBuckets + 1 :
                                                m_cEnumeratedEmptyBuckets;
                    OSStrCbFormatA( rgStatsString, cchStatsString,
                                    "ID Range: %d - %d, Visited Entries = %d, Visited Buckets = %d, Skipped Buckets = %d",
                                    m_idStart, m_idEnd,
                                    m_cEnumeratedEntries,
                                    m_cEnumeratedBuckets - cRealEmptyBuckets,
                                    cRealEmptyBuckets );
                }

                LONG CEnumeratedEntries() const             { return m_cEnumeratedEntries; }
                LONG CEnumeratedBuckets() const             { return m_cEnumeratedBuckets; }
                LONG CEnumeratedEmptyBuckets() const        { return m_cEnumeratedEmptyBuckets; }

#else
            public:
                void SPrintStats( __in_bcount(cchStatsString) char rgStatsString[] )
                {
                    OSStrCbCopyA( rgStatsString, cchStatsString, "Stats compiled out / disabled." );
                }
#endif
        };

#ifdef APPROXIMATE_INDEX_STATS
        LONG CEnumeratedRangeVirtual( const CLock* const plock ) const      { COLLAssert( plock->m_idEnd == _DeltaId( plock->m_idStart, _SubId( plock->m_idEnd, plock->m_idStart ) ) ); return plock->m_idEnd != 0 ? _SubId( plock->m_idEnd, plock->m_idStart ) : 1; }
        LONG CEnumeratedRangeReal( const CLock* const plock ) const         { return _KeyFromId( CEnumeratedRangeVirtual( plock ) ); }
#endif

    private:

        typename CBucket::ID _IdFromKeyPtr( const CKey& key, CEntry* const pentry ) const;
        typename CBucket::ID _IdFromKey( const CKey& key ) const;
        CKey _KeyFromId( const typename CBucket::ID id ) const;
        typename CBucket::ID _DeltaId( const typename CBucket::ID id, const LONG did ) const;
        LONG _SubId( const typename CBucket::ID id1, typename const CBucket::ID id2 ) const;
        LONG _CmpId( const typename CBucket::ID id1, typename const CBucket::ID id2 ) const;
        CInvasiveContext* _PicFromPentry( CEntry* const pentry ) const;
        BOOL _FExpandIdRange( const typename CBucket::ID idNew );

        void _MoveBucketId( CLock* const plock, const LONG did );

        ERR _ErrInsertBucket( CLock* const plock );
        ERR _ErrInsertEntry( CLock* const plock, CEntry* const pentry );
        ERR _ErrMoveNext( CLock* const plock );
        ERR _ErrMovePrev( CLock* const plock );

    private:


        LONG                    m_shfKeyPrecision;
        LONG                    m_shfKeyUncertainty;
        LONG                    m_shfBucketHash;
        LONG                    m_shfFillMSB;
        typename CBucket::ID    m_maskBucketKey;
        typename CBucket::ID    m_maskBucketPtr;
        typename CBucket::ID    m_maskBucketID;
        LONG                    m_didRangeMost;


        CCriticalSection        m_critUpdateIdRange;
        LONG                    m_cidRange;
        typename CBucket::ID    m_idRangeFirst;
        typename CBucket::ID    m_idRangeLast;
        BYTE                    m_rgbReserved2[ 16 ];


        CBucketTable            m_bt;
};


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CApproximateIndex< CKey, CEntry, OffsetOfIC >::
CApproximateIndex( const INT Rank )
    :   m_critUpdateIdRange( CLockBasicInfo( CSyncBasicInfo( "CApproximateIndex::m_critUpdateIdRange" ), 1, 0 ) ),
        m_bt( Rank )
{
    COLLAssert( Rank > 1 );
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CApproximateIndex< CKey, CEntry, OffsetOfIC >::
~CApproximateIndex()
{
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
ErrInit(    const CKey      dkeyPrecision,
            const CKey      dkeyUncertainty,
            const double    dblSpeedSizeTradeoff )
{

    if (    dkeyPrecision <= dkeyUncertainty ||
            dkeyUncertainty < CKey( 0 ) ||
            dblSpeedSizeTradeoff < 0.0 || dblSpeedSizeTradeoff > 1.0 && dblSpeedSizeTradeoff != 99.9 )
    {
        return ERR::errInvalidParameter;
    }


    const CBucket::ID cbucketHashMin = ( dblSpeedSizeTradeoff == 99.9 ) ?
                                        CBucket::ID( 64 ) :
                                        CBucket::ID( ( 1.0 - dblSpeedSizeTradeoff ) * OSSyncGetProcessorCount() );

    CKey maskKey;
    for (   m_shfKeyPrecision = 0, maskKey = 0;
            dkeyPrecision > CKey( 1 ) << m_shfKeyPrecision && m_shfKeyPrecision < sizeof( CKey ) * 8;
            maskKey |= CKey( 1 ) << m_shfKeyPrecision++ )
    {
    }
    for (   m_shfKeyUncertainty = 0;
            dkeyUncertainty > CKey( 1 ) << m_shfKeyUncertainty && m_shfKeyUncertainty < sizeof( CKey ) * 8;
            m_shfKeyUncertainty++ )
    {
    }
    for (   m_shfBucketHash = 0, m_maskBucketPtr = 0;
            cbucketHashMin > CBucket::ID( 1 ) << m_shfBucketHash && m_shfBucketHash < sizeof( CBucket::ID ) * 8;
            m_maskBucketPtr |= CBucket::ID( 1 ) << m_shfBucketHash++ )
    {
    }

    m_maskBucketKey = CBucket::ID( maskKey >> m_shfKeyUncertainty );

    m_shfFillMSB = sizeof( CBucket::ID ) * 8 - m_shfKeyPrecision + m_shfKeyUncertainty - m_shfBucketHash;
    m_shfFillMSB = max( m_shfFillMSB, 0 );

    m_maskBucketID = ( ~CBucket::ID( 0 ) ) >> m_shfFillMSB;


    const CBucket::ID cbucketHashMax = CBucket::ID( sizeof( void* ) * 8 );

    LONG shfBucketHashMax;
    for (   shfBucketHashMax = 0;
            cbucketHashMax > CBucket::ID( 1 ) << shfBucketHashMax && shfBucketHashMax < sizeof( CBucket::ID ) * 8;
            shfBucketHashMax++ )
    {
    }

    LONG shfFillMSBMin;
    shfFillMSBMin = sizeof( CBucket::ID ) * 8 - m_shfKeyPrecision + m_shfKeyUncertainty - shfBucketHashMax;
    shfFillMSBMin = max( shfFillMSBMin, 0 );

    if (    shfFillMSBMin < 0 ||
            shfFillMSBMin > LONG( sizeof( CBucket::ID ) * 8 - shfBucketHashMax ) )
    {
        return ERR::errInvalidParameter;
    }

    
    if ( m_shfKeyUncertainty < shfBucketHashMax )
    {
        return ERR::errInvalidParameter;
    }


    m_didRangeMost = m_maskBucketID >> 1;


    m_cidRange          = 0;
    m_idRangeFirst      = 0;
    m_idRangeLast       = 0;


    if ( m_bt.ErrInit( 5.0, 1.0, 4 * cbucketHashMin ) != CBucketTable::ERR::errSuccess )
    {
        Term();
        return ERR::errOutOfMemory;
    }

    return ERR::errSuccess;
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
Term()
{

    m_bt.Term();
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
LockKeyPtr( const CKey& key, CEntry* const pentry, CLock* const plock)
{
    BOOL fLockSucceeded = FTryLockKeyPtr( key,
                                          pentry,
                                          plock,
                                          fFalse);
    COLLAssert( fLockSucceeded );
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline BOOL CApproximateIndex< CKey, CEntry, OffsetOfIC >::
FTryLockKeyPtr( const CKey& key, CEntry* const pentry, CLock* const plock, BOOL fCanFail )
{

    plock->m_bucket.m_id = _IdFromKeyPtr( key, pentry );


    BOOL fLockSucceeded = m_bt.FTryWriteLockKey( plock->m_bucket.m_id, &plock->m_lock, fCanFail );
    if ( !fLockSucceeded )
    {
        return fLockSucceeded;
    }


    plock->m_bucket.m_cPin = 0;
    plock->m_bucket.m_il.Empty();
    (void)m_bt.ErrRetrieveEntry( &plock->m_lock, &plock->m_bucket );


    if ( plock->m_bucket.m_il.FMember( pentry ) )
    {

        plock->m_pentryPrev = NULL;
        plock->m_pentry     = pentry;
        plock->m_pentryNext = NULL;
    }


    else
    {

        plock->m_pentryPrev = NULL;
        plock->m_pentry     = NULL;
        plock->m_pentryNext = plock->m_bucket.m_il.PrevMost();
    }


    COLLAssert( !plock->m_bucket.m_cPin ||
            (   _CmpId( plock->m_bucket.m_id, m_idRangeFirst ) >= 0 &&
                _CmpId( plock->m_bucket.m_id, m_idRangeLast ) <= 0 ) );

    return fTrue;
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
UnlockKeyPtr( CLock* const plock )
{

    COLLAssert( !plock->m_bucket.m_cPin ||
            (   _CmpId( plock->m_bucket.m_id, m_idRangeFirst ) >= 0 &&
                _CmpId( plock->m_bucket.m_id, m_idRangeLast ) <= 0 ) );


    m_bt.WriteUnlockKey( &plock->m_lock );
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline LONG CApproximateIndex< CKey, CEntry, OffsetOfIC >::
CmpKey( const CKey& key1, const CKey& key2 ) const
{
    return _CmpId( _IdFromKey( key1 ), _IdFromKey( key2 ) );
}



template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CKey CApproximateIndex< CKey, CEntry, OffsetOfIC >::
KeyRangeFirst() const
{
    return _KeyFromId( m_idRangeFirst );
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CKey CApproximateIndex< CKey, CEntry, OffsetOfIC >::
KeyRangeLast() const
{
    return _KeyFromId( m_idRangeLast );
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CKey CApproximateIndex< CKey, CEntry, OffsetOfIC >::
KeyInsertLeast() const
{
    const CBucket::ID cBucketHash = 1 << m_shfBucketHash;

    CBucket::ID idFirstLeast = m_idRangeLast - m_didRangeMost;
    idFirstLeast = idFirstLeast + ( cBucketHash - idFirstLeast % cBucketHash ) % cBucketHash;

    return _KeyFromId( idFirstLeast );
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CKey CApproximateIndex< CKey, CEntry, OffsetOfIC >::
KeyInsertMost() const
{
    const CBucket::ID cBucketHash = 1 << m_shfBucketHash;

    CBucket::ID idLastMost = m_idRangeFirst + m_didRangeMost;
    idLastMost = idLastMost - ( idLastMost + 1 ) % cBucketHash;

    return _KeyFromId( idLastMost );
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
ErrRetrieveEntry( CLock* const plock, CEntry** const ppentry ) const
{

    if ( plock->m_pentry )
    {
        COLLAssert( m_bt.FHasCurrentRecord( &plock->m_lock ) );
    }

    *ppentry = plock->m_pentry;
    
    return *ppentry ? ERR::errSuccess : ERR::errEntryNotFound;
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
ErrInsertEntry( CLock* const plock, CEntry* const pentry, const BOOL fNextMost )
{
    CBucketTable::ERR err;


    COLLAssert( !plock->m_bucket.m_il.FMember( pentry ) );


    plock->m_bucket.m_cPin++;


    if ( fNextMost )
    {
        plock->m_bucket.m_il.InsertAsNextMost( pentry );
    }
    else
    {
        plock->m_bucket.m_il.InsertAsPrevMost( pentry );
    }


    if ( ( err = m_bt.ErrReplaceEntry( &plock->m_lock, plock->m_bucket ) ) != CBucketTable::ERR::errSuccess )
    {
        COLLAssert( err == CBucketTable::ERR::errNoCurrentEntry );


        return _ErrInsertEntry( plock, pentry );
    }


    else
    {

        plock->m_pentryPrev = NULL;
        plock->m_pentry     = pentry;
        plock->m_pentryNext = NULL;
        return ERR::errSuccess;
    }
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
ErrDeleteEntry( CLock* const plock )
{

    if ( plock->m_pentry )
    {

        plock->m_pentryPrev = plock->m_bucket.m_il.Prev( plock->m_pentry );
        plock->m_pentryNext = plock->m_bucket.m_il.Next( plock->m_pentry );


        plock->m_bucket.m_il.Remove( plock->m_pentry );


        plock->m_bucket.m_cPin--;


        const CBucketTable::ERR err = m_bt.ErrReplaceEntry( &plock->m_lock, plock->m_bucket );
        COLLAssert( err == CBucketTable::ERR::errSuccess );


        plock->m_pentry = NULL;
        return ERR::errSuccess;
    }


    else
    {

        return ERR::errNoCurrentEntry;
    }
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
ErrReserveEntry( CLock* const plock )
{

    plock->m_bucket.m_cPin++;


    CBucketTable::ERR errBT;

    if ( ( errBT = m_bt.ErrReplaceEntry( &plock->m_lock, plock->m_bucket ) ) != CBucketTable::ERR::errSuccess )
    {
        COLLAssert( errBT == CBucketTable::ERR::errNoCurrentEntry );


        ERR err;

        if ( ( err = _ErrInsertBucket( plock ) ) != ERR::errSuccess )
        {
            COLLAssert( err == ERR::errOutOfMemory || err == ERR::errKeyRangeExceeded );


            plock->m_bucket.m_cPin--;
            return err;
        }
    }

    return ERR::errSuccess;
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
UnreserveEntry( CLock* const plock )
{

    COLLAssert( plock->m_bucket.m_cPin > 0 );
    plock->m_bucket.m_cPin--;


    CBucketTable::ERR errBT = m_bt.ErrReplaceEntry( &plock->m_lock, plock->m_bucket );
    COLLAssert( errBT == CBucketTable::ERR::errSuccess );
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
MoveBeforeFirst( CLock* const plock )
{

    plock->m_bucket.m_id = m_idRangeFirst;

#ifdef APPROXIMATE_INDEX_STATS
    plock->m_idStart = plock->m_bucket.m_id;
    plock->m_cEnumeratedBuckets++;
#endif


    m_bt.WriteLockKey( plock->m_bucket.m_id, &plock->m_lock );


    plock->m_bucket.m_cPin = 0;
    plock->m_bucket.m_il.Empty();
    (void)m_bt.ErrRetrieveEntry( &plock->m_lock, &plock->m_bucket );


    plock->m_pentryPrev = NULL;
    plock->m_pentry     = NULL;
    plock->m_pentryNext = plock->m_bucket.m_il.PrevMost();
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
ErrMoveNext( CLock* const plock )
{

    plock->m_pentryPrev = NULL;
    plock->m_pentry     =   plock->m_pentry ?
                                plock->m_bucket.m_il.Next( plock->m_pentry ) :
                                plock->m_pentryNext;
    plock->m_pentryNext = NULL;


    if ( !plock->m_pentry )
    {

        return _ErrMoveNext( plock );
    }


    else
    {

#ifdef APPROXIMATE_INDEX_STATS
        plock->m_cEnumeratedEntries++;
#endif
        return ERR::errSuccess;
    }
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
ErrMovePrev( CLock* const plock )
{

    plock->m_pentryNext = NULL;
    plock->m_pentry     =   plock->m_pentry ?
                                plock->m_bucket.m_il.Prev( plock->m_pentry ) :
                                plock->m_pentryPrev;
    plock->m_pentryPrev = NULL;


    if ( !plock->m_pentry )
    {

        return _ErrMovePrev( plock );
    }


    else
    {

#ifdef APPROXIMATE_INDEX_STATS
        plock->m_cEnumeratedEntries++;
#endif
        return ERR::errSuccess;
    }
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
MoveAfterLast( CLock* const plock )
{

    plock->m_bucket.m_id = m_idRangeLast;

#ifdef APPROXIMATE_INDEX_STATS
    plock->m_idStart = plock->m_bucket.m_id;
    plock->m_cEnumeratedBuckets++;
#endif


    m_bt.WriteLockKey( plock->m_bucket.m_id, &plock->m_lock );


    plock->m_bucket.m_cPin = 0;
    plock->m_bucket.m_il.Empty();
    (void)m_bt.ErrRetrieveEntry( &plock->m_lock, &plock->m_bucket );


    plock->m_pentryPrev = plock->m_bucket.m_il.NextMost();
    plock->m_pentry     = NULL;
    plock->m_pentryNext = NULL;
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
MoveBeforeKeyPtr( const CKey& key, CEntry* const pentry, CLock* const plock )
{

    plock->m_bucket.m_id = _IdFromKeyPtr( key, pentry );

#ifdef APPROXIMATE_INDEX_STATS
    plock->m_idStart = plock->m_bucket.m_id;
    plock->m_cEnumeratedBuckets++;
#endif


    m_bt.WriteLockKey( plock->m_bucket.m_id, &plock->m_lock );


    plock->m_bucket.m_cPin = 0;
    plock->m_bucket.m_il.Empty();
    (void)m_bt.ErrRetrieveEntry( &plock->m_lock, &plock->m_bucket );


    plock->m_pentryPrev = NULL;
    plock->m_pentry     = NULL;
    plock->m_pentryNext = plock->m_bucket.m_il.PrevMost();
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
MoveAfterKeyPtr( const CKey& key, CEntry* const pentry, CLock* const plock )
{

    plock->m_bucket.m_id = _IdFromKeyPtr( key, pentry );

#ifdef APPROXIMATE_INDEX_STATS
    plock->m_idStart = plock->m_bucket.m_id;
    plock->m_cEnumeratedBuckets++;
#endif


    m_bt.WriteLockKey( plock->m_bucket.m_id, &plock->m_lock );


    plock->m_bucket.m_cPin = 0;
    plock->m_bucket.m_il.Empty();
    (void)m_bt.ErrRetrieveEntry( &plock->m_lock, &plock->m_bucket );


    plock->m_pentryPrev = plock->m_bucket.m_il.NextMost();
    plock->m_pentry     = NULL;
    plock->m_pentryNext = NULL;
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::CBucket::ID CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_IdFromKeyPtr( const CKey& key, CEntry* const pentry ) const
{

    const CBucket::ID   iBucketKey      = CBucket::ID( key >> m_shfKeyUncertainty );
    const CBucket::ID   iBucketPtr      = CBucket::ID( LONG_PTR( pentry ) / LONG_PTR( sizeof( CEntry ) ) );

    return ( ( iBucketKey & m_maskBucketKey ) << m_shfBucketHash ) + ( iBucketPtr & m_maskBucketPtr );
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::CBucket::ID CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_IdFromKey( const CKey& key ) const
{
    return _IdFromKeyPtr( key, NULL );
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CKey CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_KeyFromId( const typename CBucket::ID id ) const
{
    return CKey( id >> m_shfBucketHash ) << m_shfKeyUncertainty;
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::CBucket::ID CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_DeltaId( const typename CBucket::ID id, const LONG did ) const
{
    return ( id + CBucket::ID( did ) ) & m_maskBucketID;
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline LONG CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_SubId( const typename CBucket::ID id1, const typename CBucket::ID id2 ) const
{

    const LONG lid1 = id1 << m_shfFillMSB;
    const LONG lid2 = id2 << m_shfFillMSB;


    return CBucket::ID( ( lid1 - lid2 ) >> m_shfFillMSB );
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline LONG CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_CmpId( const typename CBucket::ID id1, const typename CBucket::ID id2 ) const
{

    const LONG lid1 = id1 << m_shfFillMSB;
    const LONG lid2 = id2 << m_shfFillMSB;

    return lid1 - lid2;
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::CInvasiveContext* CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_PicFromPentry( CEntry* const pentry ) const
{
    return (CInvasiveContext*)( (BYTE*)pentry + OffsetOfIC() );
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline BOOL CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_FExpandIdRange( const typename CBucket::ID idNew )
{
    COLLAssert( m_critUpdateIdRange.FOwner() );


    const LONG          cidRange    = m_cidRange;
    const CBucket::ID   idFirst     = m_idRangeFirst;
    const CBucket::ID   idLast      = m_idRangeLast;
    const LONG          didRange    = _SubId( idLast, idFirst );

    COLLAssert( didRange >= 0 );
    COLLAssert( didRange <= m_didRangeMost );
    COLLAssert( cidRange >= 0 );
    COLLAssert( (ULONG)cidRange <= (ULONG)m_didRangeMost + 1 );


    if ( !cidRange )
    {
        m_cidRange      = 1;
        m_idRangeFirst  = idNew;
        m_idRangeLast   = idNew;

        return fTrue;
    }


    const CBucket::ID   idFirstMic  = _DeltaId( idFirst, -( m_didRangeMost - didRange + 1 ) );
    const CBucket::ID   idLastMax   = _DeltaId( idLast, m_didRangeMost - didRange + 1 );


    if (    _CmpId( idFirstMic, idNew ) != 0 && _CmpId( idLastMax, idNew ) != 0 &&
            _CmpId( idFirst, idNew ) <= 0 && _CmpId( idNew, idLast ) <= 0 )
    {
        m_cidRange = cidRange + 1;

        return fTrue;
    }


    if (    _CmpId( idFirst, idNew ) != 0 && _CmpId( idLast, idNew ) != 0 &&
            _CmpId( idLastMax, idNew ) <= 0 && _CmpId( idNew, idFirstMic ) <= 0 )
    {
        return fFalse;
    }


    CBucket::ID idFirstNew  = idFirst;
    CBucket::ID idLastNew   = idLast;

    if ( _CmpId( idFirstMic, idNew ) < 0 && _CmpId( idNew, idFirst ) < 0 )
    {
        idFirstNew = idNew;
    }
    else
    {
        COLLAssert( _CmpId( idLast, idNew ) < 0 && _CmpId( idNew, idLastMax ) < 0 );

        idLastNew = idNew;
    }


    COLLAssert( _CmpId( idFirstNew, idFirst ) <= 0 );
    COLLAssert( _CmpId( idLast, idLastNew ) <= 0 );
    COLLAssert( _SubId( idLastNew, idFirstNew ) > 0 );
    COLLAssert( _SubId( idLastNew, idFirstNew ) <= m_didRangeMost );
    COLLAssert( _CmpId( idFirstNew, idNew ) <= 0 );
    COLLAssert( _CmpId( idNew, idLastNew ) <= 0 );


    m_cidRange      = cidRange + 1;
    m_idRangeFirst  = idFirstNew;
    m_idRangeLast   = idLastNew;

    return fTrue;
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_ErrInsertBucket( CLock* const plock )
{

    m_critUpdateIdRange.Enter();
    const BOOL fRangeUpdated = _FExpandIdRange( plock->m_bucket.m_id );
    m_critUpdateIdRange.Leave();


    if ( !fRangeUpdated )
    {
        return ERR::errKeyRangeExceeded;
    }


    CBucketTable::ERR err;

    if ( ( err = m_bt.ErrInsertEntry( &plock->m_lock, plock->m_bucket ) ) != CBucketTable::ERR::errSuccess )
    {
        COLLAssert( err == CBucketTable::ERR::errOutOfMemory );


        m_critUpdateIdRange.Enter();
        m_cidRange--;
        m_critUpdateIdRange.Leave();

        return ERR::errOutOfMemory;
    }

    return ERR::errSuccess;
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_ErrInsertEntry( CLock* const plock, CEntry* const pentry )
{
    ERR err;


    if ( ( err = _ErrInsertBucket( plock ) ) != ERR::errSuccess )
    {
        COLLAssert( err == ERR::errOutOfMemory || err == ERR::errKeyRangeExceeded );


        plock->m_bucket.m_il.Remove( pentry );
        plock->m_bucket.m_cPin--;
        return err;
    }


    plock->m_pentryPrev = NULL;
    plock->m_pentry     = pentry;
    plock->m_pentryNext = NULL;
    return ERR::errSuccess;
}

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_MoveBucketId( CLock* const plock, const LONG did )
{

    plock->m_bucket.m_id = _DeltaId( plock->m_bucket.m_id, did );

#ifdef APPROXIMATE_INDEX_STATS
    plock->m_idEnd = plock->m_bucket.m_id;
    if ( plock->m_cEnumeratedEntries == plock->m_cEnumeratedEntriesPerLastBucket )
    {
        plock->m_cEnumeratedEmptyBuckets++;
    }
    plock->m_cEnumeratedEntriesPerLastBucket = plock->m_cEnumeratedEntries;
    plock->m_cEnumeratedBuckets++;
#endif
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_ErrMoveNext( CLock* const plock )
{

    plock->m_pentryPrev = plock->m_bucket.m_il.NextMost();
    plock->m_pentry     = NULL;
    plock->m_pentryNext = NULL;


    while ( !plock->m_pentry && _CmpId( plock->m_bucket.m_id, m_idRangeLast ) < 0 )
    {

        if ( !plock->m_bucket.m_cPin )
        {

            const CBucketTable::ERR err = m_bt.ErrDeleteEntry( &plock->m_lock );
            COLLAssert( err == CBucketTable::ERR::errSuccess ||
                    err == CBucketTable::ERR::errNoCurrentEntry );


            m_critUpdateIdRange.Enter();

            if ( m_idRangeFirst == plock->m_bucket.m_id )
            {
                m_idRangeFirst = _DeltaId( m_idRangeFirst, 1 );
            }

            if ( err == CBucketTable::ERR::errSuccess )
            {
                m_cidRange--;
            }

            m_critUpdateIdRange.Leave();
        }


        m_bt.WriteUnlockKey( &plock->m_lock );


        if (    _CmpId( m_idRangeFirst, plock->m_bucket.m_id ) > 0 ||
                _CmpId( plock->m_bucket.m_id, m_idRangeLast ) > 0 )
        {

            if ( m_critUpdateIdRange.FTryEnter() )
            {

                if (    _CmpId( m_idRangeFirst, plock->m_bucket.m_id ) > 0 ||
                        _CmpId( plock->m_bucket.m_id, m_idRangeLast ) > 0 )
                {

                    plock->m_bucket.m_id = m_idRangeFirst;
                }


                else
                {

                    _MoveBucketId( plock, 1 );
                }

                m_critUpdateIdRange.Leave();
            }


            else
            {

                _MoveBucketId( plock, 1 );
            }
        }


        else
        {

            _MoveBucketId( plock, 1 );
        }


        m_bt.WriteLockKey( plock->m_bucket.m_id, &plock->m_lock );


        plock->m_bucket.m_cPin = 0;
        plock->m_bucket.m_il.Empty();
        (void)m_bt.ErrRetrieveEntry( &plock->m_lock, &plock->m_bucket );


        plock->m_pentryPrev = NULL;
        plock->m_pentry     = plock->m_bucket.m_il.PrevMost();
        plock->m_pentryNext = NULL;
    }


#ifdef APPROXIMATE_INDEX_STATS
    if ( plock->m_pentry )
    {
        plock->m_cEnumeratedEntries++;
    }
#endif

    return plock->m_pentry ? ERR::errSuccess : ERR::errNoCurrentEntry;
}


template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_ErrMovePrev( CLock* const plock )
{

    plock->m_pentryPrev = NULL;
    plock->m_pentry     = NULL;
    plock->m_pentryNext = plock->m_bucket.m_il.PrevMost();


    while ( !plock->m_pentry && _CmpId( m_idRangeFirst, plock->m_bucket.m_id ) < 0 )
    {

        if ( !plock->m_bucket.m_cPin )
        {

            const CBucketTable::ERR err = m_bt.ErrDeleteEntry( &plock->m_lock );
            COLLAssert( err == CBucketTable::ERR::errSuccess ||
                    err == CBucketTable::ERR::errNoCurrentEntry );


            m_critUpdateIdRange.Enter();

            if ( m_idRangeLast == plock->m_bucket.m_id )
            {
                m_idRangeLast = _DeltaId( m_idRangeLast, -1 );
            }

            if ( err == CBucketTable::ERR::errSuccess )
            {
                m_cidRange--;
            }

            m_critUpdateIdRange.Leave();
        }


        m_bt.WriteUnlockKey( &plock->m_lock );


        if (    _CmpId( m_idRangeFirst, plock->m_bucket.m_id ) > 0 ||
                _CmpId( plock->m_bucket.m_id, m_idRangeLast ) > 0 )
        {

            if ( m_critUpdateIdRange.FTryEnter() )
            {

                if (    _CmpId( m_idRangeFirst, plock->m_bucket.m_id ) > 0 ||
                        _CmpId( plock->m_bucket.m_id, m_idRangeLast ) > 0 )
                {

                    plock->m_bucket.m_id = m_idRangeLast;
                }


                else
                {

                    _MoveBucketId( plock, -1 );
                }

                m_critUpdateIdRange.Leave();
            }


            else
            {

                _MoveBucketId( plock, -1 );
            }
        }


        else
        {

            _MoveBucketId( plock, -1 );
        }


        m_bt.WriteLockKey( plock->m_bucket.m_id, &plock->m_lock );


        plock->m_bucket.m_cPin = 0;
        plock->m_bucket.m_il.Empty();
        (void)m_bt.ErrRetrieveEntry( &plock->m_lock, &plock->m_bucket );


        plock->m_pentryPrev = NULL;
        plock->m_pentry     = plock->m_bucket.m_il.NextMost();
        plock->m_pentryNext = NULL;
    }


#ifdef APPROXIMATE_INDEX_STATS
    if ( plock->m_pentry )
    {
        plock->m_cEnumeratedEntries++;
    }
#endif

    return plock->m_pentry ? ERR::errSuccess : ERR::errNoCurrentEntry;
}


#define DECLARE_APPROXIMATE_INDEX( CKey, CEntry, OffsetOfIC, Typedef )  \
                                                                        \
typedef CApproximateIndex< CKey, CEntry, OffsetOfIC > Typedef;          \
                                                                        \
inline ULONG_PTR Typedef::CBucketTable::CKeyEntry::                     \
Hash( const CBucket::ID& id )                                           \
{                                                                       \
    return id;                                                          \
}                                                                       \
                                                                        \
inline ULONG_PTR Typedef::CBucketTable::CKeyEntry::                     \
Hash() const                                                            \
{                                                                       \
    return m_entry.m_id;                                                \
}                                                                       \
                                                                        \
inline BOOL Typedef::CBucketTable::CKeyEntry::                          \
FEntryMatchesKey( const CBucket::ID& id ) const                         \
{                                                                       \
    return m_entry.m_id == id;                                          \
}                                                                       \
                                                                        \
inline void Typedef::CBucketTable::CKeyEntry::                          \
SetEntry( const CBucket& bucket )                                       \
{                                                                       \
    m_entry = bucket;                                                   \
}                                                                       \
                                                                        \
inline void Typedef::CBucketTable::CKeyEntry::                          \
GetEntry( CBucket* const pbucket ) const                                \
{                                                                       \
    *pbucket = m_entry;                                                 \
}



template< class CObject, PfnOffsetOf OffsetOfIC >
class CPool
{
    public:


        class CInvasiveContext
        {
            public:

                CInvasiveContext() {}
                ~CInvasiveContext() {}

                static SIZE_T OffsetOfILE() { return OffsetOfIC() + OffsetOf( CInvasiveContext, m_ile ); }

            private:

                typename CInvasiveList< CObject, OffsetOfILE >::CElement    m_ile;
        };


        enum class ERR
        {
            errSuccess,
            errInvalidParameter,
            errOutOfMemory,
            errObjectNotFound,
            errOutOfObjects,
            errNoCurrentObject,
        };


        class CLock;

    public:


        CPool();
        ~CPool();


        ERR ErrInit( const double dblSpeedSizeTradeoff );
        void Term();

        void Insert( CObject* const pobj, const BOOL fMRU = fTrue );
        ERR ErrRemove( CObject** const ppobj, const INT cmsecTimeout = cmsecInfinite, const BOOL fMRU = fTrue );

        void BeginPoolScan( CLock* const plock );
        ERR ErrGetNextObject( CLock* const plock, CObject** const ppobj );
        ERR ErrRemoveCurrentObject( CLock* const plock );
        void EndPoolScan( CLock* const plock );

        DWORD Cobject();
        DWORD CWaiter();
        DWORD CInsert();
        DWORD CRemove();
        DWORD CRemoveWait();

    private:


        class CBucket
        {
            public:

                CBucket() : m_crit( CLockBasicInfo( CSyncBasicInfo( "CPool::CBucket::m_crit" ), 0, 0 ) ) {}
                ~CBucket() {}

            public:

                CCriticalSection                                        m_crit;
                CInvasiveList< CObject, CInvasiveContext::OffsetOfILE > m_il;
                BYTE                                                    m_rgbReserved[20];
        };

    public:


        class CLock
        {
            public:

                CLock() {}
                ~CLock() {}

            private:

                friend class CPool< CObject, OffsetOfIC >;

                CBucket*    m_pbucket;
                CObject*    m_pobj;
                CObject*    m_pobjNext;
        };

    private:

        void _GetNextObject( CLock* const plock );
        static void* _PvMEMIAlign( void* const pv, const size_t cbAlign );
        static void* _PvMEMIUnalign( void* const pv );
        static void* _PvMEMAlloc( const size_t cbSize, const size_t cbAlign = 1 );
        static void _MEMFree( void* const pv );

    private:


        DWORD           m_cbucket;
        CBucket*        m_rgbucket;
        BYTE            m_rgbReserved1[24];


        CSemaphore      m_semObjectCount;
        DWORD           m_cInsert;
        DWORD           m_cRemove;
        DWORD           m_cRemoveWait;
        BYTE            m_rgbReserved2[16];
};


template< class CObject, PfnOffsetOf OffsetOfIC >
inline CPool< CObject, OffsetOfIC >::
CPool()
    :   m_semObjectCount( CSyncBasicInfo( "CPool::m_semObjectCount" ) )
{
}


template< class CObject, PfnOffsetOf OffsetOfIC >
inline CPool< CObject, OffsetOfIC >::
~CPool()
{
}


template< class CObject, PfnOffsetOf OffsetOfIC >
inline typename CPool< CObject, OffsetOfIC >::ERR CPool< CObject, OffsetOfIC >::
ErrInit( const double dblSpeedSizeTradeoff )
{

    if ( dblSpeedSizeTradeoff < 0.0 || dblSpeedSizeTradeoff > 1.0 )
    {
        return ERR::errInvalidParameter;
    }


    m_cbucket = OSSyncGetProcessorCount();
    const SIZE_T cbrgbucket = sizeof( CBucket ) * m_cbucket;
    if ( !( m_rgbucket = (CBucket*)_PvMEMAlloc( cbrgbucket, cbCacheLine ) ) )
    {
        return ERR::errOutOfMemory;
    }


    for ( DWORD ibucket = 0; ibucket < m_cbucket; ibucket++ )
    {
        new( m_rgbucket + ibucket ) CBucket;
    }


    m_cInsert       = 0;
    m_cRemove       = 0;
    m_cRemoveWait   = 0;

    return ERR::errSuccess;
}


template< class CObject, PfnOffsetOf OffsetOfIC >
inline void CPool< CObject, OffsetOfIC >::
Term()
{

    if ( m_rgbucket )
    {
        for ( DWORD ibucket = 0; ibucket < m_cbucket; ibucket++ )
        {
            m_rgbucket[ ibucket ].~CBucket();
        }
        _MEMFree( m_rgbucket );
        m_rgbucket = NULL;
    }


    while ( m_semObjectCount.FTryAcquire() )
    {
    }
}


template< class CObject, PfnOffsetOf OffsetOfIC >
inline void CPool< CObject, OffsetOfIC >::
Insert( CObject* const pobj, const BOOL fMRU )
{

    const DWORD_PTR ibucketBase = ( fMRU ? OSSyncGetCurrentProcessor() : ( DWORD_PTR( pobj ) / sizeof( CObject ) ) );
    DWORD           ibucket     = 0;

    do  {
        CBucket* const pbucket = m_rgbucket + ( ibucketBase + ibucket++ ) % m_cbucket;

        if ( ibucket < m_cbucket )
        {
            if ( !pbucket->m_crit.FTryEnter() )
            {
                continue;
            }
        }
        else
        {
            pbucket->m_crit.Enter();
        }

        if ( ( ibucket - 1 ) == 0 && fMRU )
        {
            pbucket->m_il.InsertAsPrevMost( pobj );
        }
        else
        {
            pbucket->m_il.InsertAsNextMost( pobj );
        }
        pbucket->m_crit.Leave();
        break;
    }
    while ( fTrue );


    AtomicIncrement( (LONG*)&m_cInsert );
    m_semObjectCount.Release();
}


template< class CObject, PfnOffsetOf OffsetOfIC >
inline typename CPool< CObject, OffsetOfIC >::ERR CPool< CObject, OffsetOfIC >::
ErrRemove( CObject** const ppobj, const INT cmsecTimeout, const BOOL fMRU )
{

    *ppobj = NULL;

    if ( !m_semObjectCount.FTryAcquire() )
    {
        if ( cmsecTimeout == cmsecTest )
        {
            return ERR::errOutOfObjects;
        }
        else
        {
            AtomicIncrement( (LONG*)&m_cRemoveWait );
            if ( !m_semObjectCount.FAcquire( cmsecTimeout ) )
            {
                return ERR::errOutOfObjects;
            }
        }
    }


    const DWORD ibucketBase = OSSyncGetCurrentProcessor();
    DWORD       ibucket     = 0;

    do  {
        CBucket* const pbucket = m_rgbucket + ( ibucketBase + ibucket++ ) % m_cbucket;

        if ( pbucket->m_il.FEmpty() )
        {
            continue;
        }

        if ( ibucket < m_cbucket )
        {
            if ( !pbucket->m_crit.FTryEnter() )
            {
                continue;
            }
        }
        else
        {
            pbucket->m_crit.Enter();
        }

        if ( !pbucket->m_il.FEmpty() )
        {
            if ( ( ibucket - 1 ) == 0 && fMRU )
            {
                *ppobj = pbucket->m_il.PrevMost();
            }
            else
            {
                *ppobj = pbucket->m_il.NextMost();
            }
            pbucket->m_il.Remove( *ppobj );
        }
        pbucket->m_crit.Leave();
    }
    while ( *ppobj == NULL );


    AtomicIncrement( (LONG*)&m_cRemove );
    return ERR::errSuccess;
}


template< class CObject, PfnOffsetOf OffsetOfIC >
inline void CPool< CObject, OffsetOfIC >::
BeginPoolScan( CLock* const plock )
{

    plock->m_pbucket = m_rgbucket;


    plock->m_pbucket->m_crit.Enter();


    plock->m_pobj       = NULL;
    plock->m_pobjNext   = plock->m_pbucket->m_il.PrevMost();
}


template< class CObject, PfnOffsetOf OffsetOfIC >
inline typename CPool< CObject, OffsetOfIC >::ERR CPool< CObject, OffsetOfIC >::
ErrGetNextObject( CLock* const plock, CObject** const ppobj )
{

    plock->m_pobj       =   plock->m_pobj ?
                                plock->m_pbucket->m_il.Next( plock->m_pobj ) :
                                plock->m_pobjNext;
    plock->m_pobjNext   = NULL;


    if ( !plock->m_pobj )
    {

        _GetNextObject( plock );
    }


    *ppobj = plock->m_pobj;
    return plock->m_pobj ? ERR::errSuccess : ERR::errNoCurrentObject;
}


template< class CObject, PfnOffsetOf OffsetOfIC >
inline typename CPool< CObject, OffsetOfIC >::ERR CPool< CObject, OffsetOfIC >::
ErrRemoveCurrentObject( CLock* const plock )
{

    if ( plock->m_pobj && m_semObjectCount.FTryAcquire() )
    {

        plock->m_pobjNext   = plock->m_pbucket->m_il.Next( plock->m_pobj );


        plock->m_pbucket->m_il.Remove( plock->m_pobj );


        plock->m_pobj = NULL;

        AtomicIncrement( (LONG*)&m_cRemove );
        return ERR::errSuccess;
    }


    else
    {

        return ERR::errNoCurrentObject;
    }
}


template< class CObject, PfnOffsetOf OffsetOfIC >
inline void CPool< CObject, OffsetOfIC >::
EndPoolScan( CLock* const plock )
{

    plock->m_pbucket->m_crit.Leave();
}


template< class CObject, PfnOffsetOf OffsetOfIC >
inline DWORD CPool< CObject, OffsetOfIC >::
Cobject()
{

    return m_semObjectCount.CAvail();
}


template< class CObject, PfnOffsetOf OffsetOfIC >
inline DWORD CPool< CObject, OffsetOfIC >::
CWaiter()
{

    return m_semObjectCount.CWait();
}


template< class CObject, PfnOffsetOf OffsetOfIC >
inline DWORD CPool< CObject, OffsetOfIC >::
CInsert()
{
    return m_cInsert;
}


template< class CObject, PfnOffsetOf OffsetOfIC >
inline DWORD CPool< CObject, OffsetOfIC >::
CRemove()
{
    return m_cRemove;
}


template< class CObject, PfnOffsetOf OffsetOfIC >
inline DWORD CPool< CObject, OffsetOfIC >::
CRemoveWait()
{
    return m_cRemoveWait;
}


template< class CObject, PfnOffsetOf OffsetOfIC >
inline void CPool< CObject, OffsetOfIC >::
_GetNextObject( CLock* const plock )
{

    plock->m_pobj       = NULL;
    plock->m_pobjNext   = NULL;


    while ( !plock->m_pobj && plock->m_pbucket < m_rgbucket + m_cbucket - 1 )
    {

        plock->m_pbucket->m_crit.Leave();


        plock->m_pbucket++;


        plock->m_pbucket->m_crit.Enter();


        plock->m_pobj       = plock->m_pbucket->m_il.PrevMost();
        plock->m_pobjNext   = NULL;
    }
}


template< class CObject, PfnOffsetOf OffsetOfIC >
inline void* CPool< CObject, OffsetOfIC >::
_PvMEMIAlign( void* const pv, const size_t cbAlign )
{

    const ULONG_PTR ulp         = ULONG_PTR( pv );
    const ULONG_PTR ulpAligned  = ( ( ulp + cbAlign ) / cbAlign ) * cbAlign;
    const ULONG_PTR ulpOffset   = ulpAligned - ulp;

    COLLAssert( ulpOffset > 0 );
    COLLAssert( ulpOffset <= cbAlign );
    COLLAssert( ulpOffset == BYTE( ulpOffset ) );


    BYTE *const pbAligned   = (BYTE*)ulpAligned;
    pbAligned[ -1 ]         = BYTE( ulpOffset );


    return (void*)pbAligned;
}


template< class CObject, PfnOffsetOf OffsetOfIC >
inline void* CPool< CObject, OffsetOfIC >::
_PvMEMIUnalign( void* const pv )
{

    BYTE *const pbAligned   = (BYTE*)pv;
    const BYTE bOffset      = pbAligned[ -1 ];

    COLLAssert( bOffset > 0 );


    return (void*)( pbAligned - bOffset );
}

template< class CObject, PfnOffsetOf OffsetOfIC >
inline void* CPool< CObject, OffsetOfIC >::
_PvMEMAlloc( const size_t cbSize, const size_t cbAlign )
{
    if ( cbSize +cbAlign < cbSize )
    {
        COLLAssertSz( fFalse, "Size request too big to fit w/ alignment, this is odd" );
        return NULL;
    }
    
    void* const pv = new BYTE[ cbSize + cbAlign ];
    if ( pv )
    {
        return _PvMEMIAlign( pv, cbAlign );
    }
    return NULL;
}

template< class CObject, PfnOffsetOf OffsetOfIC >
inline void CPool< CObject, OffsetOfIC >::
_MEMFree( void* const pv )
{
    if ( pv )
    {
        delete [] _PvMEMIUnalign( pv );
    }
}



template< class CEntry >
class CArray
{
    public:


        enum class ERR
        {
            errSuccess,
            errOutOfMemory,
        };


        static const size_t iEntryNotFound = SIZE_MAX;


        typedef INT (__cdecl *PfnCompare)( const CEntry* pentry1, const CEntry* pentry2 );

    protected:

        typedef INT (__cdecl *PfnCompareCRT)( const void* pv1, const void* pv2 );

    public:


        CArray();
        CArray( const size_t centryGrowth );
        ~CArray();

    public:


        ERR ErrClone( const CArray& array );

        ERR ErrSetSize( const size_t centry );
        ERR ErrSetCapacity( const size_t centryMax );
        void SetCapacityGrowth( const size_t centryGrowth );
        void SetEntryDefault( const CEntry& entry );
        
        ERR ErrSetEntry( const size_t ientry, const CEntry& entry );
        void SetEntry( const CEntry* const pentry, const CEntry& entry );
        
        size_t Size() const;
        size_t Capacity() const;
        const CEntry& Entry( const size_t ientry ) const;
        CEntry& operator[]( const size_t ientry ) const;
        const CEntry* PEntry( const size_t ientry ) const;

        void Sort( PfnCompare pfnCompare );
        void SortAndRemoveDuplicates( PfnCompare pfnCompare );
        size_t SearchLinear( const CEntry& entrySearch, PfnCompare pfnCompare );
        size_t SearchLinear( const CEntry* const pentrySearch, PfnCompare pfnCompare );
        size_t SearchBinary( const CEntry& entrySearch, PfnCompare pfnCompare );
        size_t SearchBinary( const CEntry* const pentrySearch, PfnCompare pfnCompare );

    private:


        size_t      m_centry;
        size_t      m_centryMax;
        CEntry*     m_rgentry;
        CEntry      m_entryDefault;
        bool        m_fEntryDefault;
        size_t      m_centryGrowth;
};

template< class CEntry >
inline CArray< CEntry >::
CArray()
    :   m_centry( 0 ),
        m_centryMax( 0 ),
        m_rgentry( NULL ),
        m_entryDefault(),
        m_fEntryDefault( false ),
        m_centryGrowth( 1 )
{
}

template< class CEntry >
inline CArray< CEntry >::
CArray( const size_t centryGrowth )
    :   m_centry( 0 ),
        m_centryMax( 0 ),
        m_rgentry( NULL ),
        m_entryDefault(),
        m_fEntryDefault( false ),
        m_centryGrowth( 0 )
{
    SetCapacityGrowth( centryGrowth );
}

template< class CEntry >
inline CArray< CEntry >::
~CArray()
{
    ErrSetCapacity( 0 );
    COLLAssert( m_rgentry == NULL );
}


template< class CEntry >
inline typename CArray< CEntry >::ERR CArray< CEntry >::
ErrClone( const CArray& array )
{
    CEntry*     rgentryNew  = NULL;

    COLLAssert( m_centry <= m_centryMax );
    COLLAssert( array.Size() <= array.Capacity() );

    if ( array.m_centryMax )
    {
        if ( !( rgentryNew = new CEntry[ array.m_centryMax ] ) )
        {
            return ERR::errOutOfMemory;
        }
    }

    for ( size_t ientryCopy = 0; ientryCopy < array.Size(); ientryCopy++ )
    {
        rgentryNew[ ientryCopy ] = array.Entry( ientryCopy );
    }

    delete [] m_rgentry;

    m_centry            = array.m_centry;
    m_centryMax         = array.m_centryMax;
    m_rgentry           = rgentryNew;
    m_entryDefault      = array.m_entryDefault;
    m_fEntryDefault     = array.m_fEntryDefault;
    m_centryGrowth      = array.m_centryGrowth;

    COLLAssert( m_centry <= m_centryMax );

    return ERR::errSuccess;
}


template< class CEntry >
inline typename CArray< CEntry >::ERR CArray< CEntry >::
ErrSetSize( const size_t centry )
{
    ERR err = ERR::errSuccess;

    COLLAssert( m_centry <= m_centryMax );

    if ( m_centryMax < centry )
    {

        
        size_t centryT = m_centryMax + ( ( centry - m_centryMax - 1 ) / m_centryGrowth ) * m_centryGrowth;
        if ( centryT <= ( SIZE_MAX - m_centryGrowth ) )
        {
            centryT += m_centryGrowth;
        }
        else
        {
            centryT = centry;
        }
        COLLAssert( centryT >= centry );
        if ( ( err = ErrSetCapacity( centryT ) ) != ERR::errSuccess )
        {
            return err;
        }
    }

    if ( m_fEntryDefault && centry > m_centry )
    {
        for ( size_t ientryDefault = m_centry; ientryDefault < centry; ientryDefault++ )
        {
            m_rgentry[ ientryDefault ] = m_entryDefault;
        }
    }

    m_centry = centry;

    COLLAssert( m_centry <= m_centryMax );

    return ERR::errSuccess;
}

template< class CEntry >
inline void CArray< CEntry >::
SetCapacityGrowth( const size_t centryGrowth )
{
    m_centryGrowth = ( centryGrowth == 0 ) ? 1 : centryGrowth;
}
    

template< class CEntry >
inline typename CArray< CEntry >::ERR CArray< CEntry >::
ErrSetCapacity( const size_t centryMax )
{
    COLLAssert( m_centry <= m_centryMax );

    if ( m_centryMax != centryMax )
    {
        if ( centryMax )
        {
            CEntry* rgentryNew = NULL;

            if ( ( rgentryNew = new CEntry[ centryMax ] ) == NULL )
            {
                return ERR::errOutOfMemory;
            }

            if ( m_centry > centryMax )
            {
                m_centry = centryMax;
            }

            for ( size_t ientryCopy = 0; ientryCopy < m_centry; ientryCopy++ )
            {
                rgentryNew[ ientryCopy ] = Entry( ientryCopy );
            }

            delete [] m_rgentry;

            m_centryMax = centryMax;
            m_rgentry   = rgentryNew;
        }
        else
        {
            delete [] m_rgentry;

            m_centry    = 0;
            m_centryMax = 0;
            m_rgentry   = NULL;
        }
    }

    COLLAssert( m_centry <= m_centryMax );

    return ERR::errSuccess;
}


template< class CEntry >
inline void CArray< CEntry >::
SetEntryDefault( const CEntry& entry )
{
    m_entryDefault = entry;
    m_fEntryDefault = true;
}


template< class CEntry >
inline typename CArray< CEntry >::ERR CArray< CEntry >::
ErrSetEntry( const size_t ientry, const CEntry& entry )
{
    ERR     err         = ERR::errSuccess;
    size_t  centryReq   = ientry + 1;

    if ( m_centry < centryReq )
    {
        if ( ( err = ErrSetSize( centryReq ) ) != ERR::errSuccess )
        {
            return err;
        }
    }

    SetEntry( PEntry( ientry ), entry );

    return ERR::errSuccess;
}


template< class CEntry >
inline void CArray< CEntry >::
SetEntry( const CEntry* const pentry, const CEntry& entry )
{
    COLLAssert( pentry >= PEntry( 0 ) );
    COLLAssert( pentry < ( PEntry( 0 ) + m_centry ) );
    *const_cast< CEntry* >( pentry ) = entry;
}


template< class CEntry >
inline size_t CArray< CEntry >::
Size() const
{
    return m_centry;
}


template< class CEntry >
inline size_t CArray< CEntry >::
Capacity() const
{
    return m_centryMax;
}


template< class CEntry >
inline const CEntry& CArray< CEntry >::
Entry( const size_t ientry ) const
{
    return *PEntry( ientry );
}


template< class CEntry >
inline CEntry& CArray< CEntry >::
operator[]( const size_t ientry ) const
{
    return (CEntry&)Entry( ientry );
}


template< class CEntry >
inline const CEntry* CArray< CEntry >::
PEntry( const size_t ientry ) const
{
    return ientry < m_centry ? m_rgentry + ientry : NULL;
}


template< class CEntry >
inline void CArray< CEntry >::
Sort( PfnCompare pfnCompare )
{
    if ( m_rgentry != NULL )
    {
        qsort( m_rgentry, m_centry, sizeof( CEntry ), (PfnCompareCRT)pfnCompare );
    }
}


template< class CEntry >
inline void CArray< CEntry >::
SortAndRemoveDuplicates( PfnCompare pfnCompare )
{
    Sort( pfnCompare );

    const size_t cInitialSize = m_centry;
    const CEntry* const rgEntryEnd = m_rgentry + cInitialSize;
    CEntry* pEntry = m_rgentry;
    CEntry* pEntryUnique = m_rgentry;

    while ( pEntry != rgEntryEnd )
    {
        if ( pfnCompare( pEntryUnique, pEntry ) != 0 )
        {
            pEntryUnique++;
            *pEntryUnique = *pEntry;
        }
        pEntry++;
    }

    const size_t cFinalSize = pEntryUnique - m_rgentry + 1;

    COLLAssert( cFinalSize <= cInitialSize || cInitialSize == 0 );

    const ERR errSetSize = ErrSetSize( min( cInitialSize, cFinalSize ) );

    COLLAssert( errSetSize == ERR::errSuccess );
}


template< class CEntry >
inline size_t CArray< CEntry >::
SearchLinear( const CEntry* const pentrySearch, PfnCompare pfnCompare )
{
    CEntry* pEntryFound = NULL;

    if ( m_rgentry != NULL )
    {
        for ( size_t iEntry = 0; iEntry < m_centry && pEntryFound == NULL; iEntry++ )
        {
            CEntry* const pEntryCompare = m_rgentry + iEntry;
            if ( pfnCompare( pentrySearch, pEntryCompare ) == 0 )
            {
                pEntryFound = pEntryCompare;
            }
        }
    }

    return ( pEntryFound == NULL ) ? iEntryNotFound : ( pEntryFound - m_rgentry );
}

template< class CEntry >
inline size_t CArray< CEntry >::
SearchLinear( const CEntry& entrySearch, PfnCompare pfnCompare )
{
    return SearchLinear( &entrySearch, pfnCompare );
}


template< class CEntry >
inline size_t CArray< CEntry >::
SearchBinary( const CEntry* const pentrySearch, PfnCompare pfnCompare )
{
    CEntry* pEntryFound = NULL;
    
    if ( m_rgentry != NULL )
    {
        pEntryFound = (CEntry*)bsearch( pentrySearch, m_rgentry, m_centry, sizeof( CEntry ), (PfnCompareCRT) pfnCompare );
    }

    return ( pEntryFound == NULL ) ? iEntryNotFound : ( pEntryFound - m_rgentry );
}

template< class CEntry >
inline size_t CArray< CEntry >::
SearchBinary( const CEntry& entrySearch, PfnCompare pfnCompare )
{
    return SearchBinary( &entrySearch, pfnCompare );
}



template< class CKey, class CEntry >
class CTable
{
    public:

        class CKeyEntry
            :   public CEntry
        {
            public:


                INT Cmp( const CKeyEntry& keyentry ) const;
                INT Cmp( const CKey& key ) const;
        };


        enum class ERR
        {
            errSuccess,
            errInvalidParameter,
            errOutOfMemory,
            errKeyChange,
        };

    public:

        CTable();
        CTable( const size_t centry, CEntry* const rgentry, const BOOL fInOrder = fFalse );

        ERR ErrLoad( const size_t centry, const CEntry* const rgentry );
        ERR ErrClone( const CTable& table );
        ERR ErrCloneArray(  const CArray< CEntry >& array );

        ERR ErrUpdateEntry( const CEntry* const pentry, const CEntry& entry );

        size_t Size() const;
        void  Clear();
        const CEntry* PEntry( const size_t ientry ) const;

        const CEntry* SeekLT( const CKey& key ) const;
        const CEntry* SeekLE( const CKey& key ) const;
        const CEntry* SeekEQ( const CKey& key ) const;
        const CEntry* SeekHI( const CKey& key ) const;
        const CEntry* SeekGE( const CKey& key ) const;
        const CEntry* SeekGT( const CKey& key ) const;

    private:

        typedef size_t (CTable< CKey, CEntry >::*PfnSearch)( const CKey& key, const BOOL fHigh ) const;

    private:

        void _ConfigureSearch( const BOOL fInOrder = fFalse );

        const CKeyEntry& _Entry( const size_t ikeyentry ) const;
        void _SetEntry( const size_t ikeyentry, const CKeyEntry& keyentry );
        void _SwapEntry( const size_t ikeyentry1, const size_t ikeyentry2 );

        size_t _LinearSearch( const CKey& key, const BOOL fHigh ) const;
        size_t _BinarySearch( const CKey& key, const BOOL fHigh ) const;
        void _InsertionSort( const size_t ikeyentryMinIn, const size_t ikeyentryMaxIn );
        void _QuickSort( const size_t ikeyentryMinIn, const size_t ikeyentryMaxIn );

    private:

        CArray< CKeyEntry >     m_arrayKeyEntry;
        PfnSearch               m_pfnSearch;
};

template< class CKey, class CEntry >
inline CTable< CKey, CEntry >::
CTable()
    :   m_pfnSearch( &CTable::_LinearSearch )
{
}


template< class CKey, class CEntry >
inline CTable< CKey, CEntry >::
CTable( const size_t centry, CEntry* const rgentry, const BOOL fInOrder )
    :   m_arrayKeyEntry( centry, reinterpret_cast< CKeyEntry* >( rgentry ) )
{
    _ConfigureSearch( fInOrder );
}


template< class CKey, class CEntry >
inline typename CTable< CKey, CEntry >::ERR CTable< CKey, CEntry >::
ErrLoad( const size_t centry, const CEntry* const rgentry )
{
    CArray< CKeyEntry >::ERR    err         = CArray< CKeyEntry >::ERR::errSuccess;
    size_t                      ientry      = 0;
    size_t                      ientryMin   = Size();
    size_t                      ientryMax   = Size() + centry;
    const CKeyEntry*            rgkeyentry  = reinterpret_cast< const CKeyEntry* >( rgentry );

    if ( ( err = m_arrayKeyEntry.ErrSetSize( Size() + centry ) ) != CArray< CKeyEntry >::ERR::errSuccess )
    {
        COLLAssert( err == CArray< CKeyEntry >::ERR::errOutOfMemory );
        return ERR::errOutOfMemory;
    }
    for ( ientry = ientryMin; ientry < ientryMax; ientry++ )
    {
        err = m_arrayKeyEntry.ErrSetEntry( ientry, rgkeyentry[ ientry - ientryMin ] );
        COLLAssert( err == CArray< CKeyEntry >::ERR::errSuccess );
    }

    _ConfigureSearch();

    return ERR::errSuccess;
}


template< class CKey, class CEntry >
inline typename CTable< CKey, CEntry >::ERR CTable< CKey, CEntry >::
ErrClone( const CTable& table )
{
    CArray< CKeyEntry >::ERR err = CArray< CKeyEntry >::ERR::errSuccess;

    if ( ( err = m_arrayKeyEntry.ErrClone( table.m_arrayKeyEntry ) ) != CArray< CKeyEntry >::ERR::errSuccess )
    {
        COLLAssert( err == CArray< CKeyEntry >::ERR::errOutOfMemory );
        return ERR::errOutOfMemory;
    }
    m_pfnSearch = table.m_pfnSearch;

    return ERR::errSuccess;
}


template< class CKey, class CEntry >
inline typename CTable< CKey, CEntry >::ERR CTable< CKey, CEntry >::
ErrCloneArray(  const CArray< CEntry >& array )
{
    CArray< CKeyEntry >::ERR    err             = CArray< CKeyEntry >::ERR::errSuccess;
    const CArray< CKeyEntry >&  arrayKeyEntry   = reinterpret_cast< const CArray< CKeyEntry >& >( array );

    if ( ( err = m_arrayKeyEntry.ErrClone( arrayKeyEntry ) ) != CArray< CKeyEntry >::ERR::errSuccess )
    {
        COLLAssert( err == CArray< CKeyEntry >::ERR::errOutOfMemory );
        return ERR::errOutOfMemory;
    }
    _ConfigureSearch();

    return ERR::errSuccess;
}


template< class CKey, class CEntry >
inline typename CTable< CKey, CEntry >::ERR CTable< CKey, CEntry >::
ErrUpdateEntry( const CEntry* const pentry, const CEntry& entry )
{
    ERR                 err         = ERR::errSuccess;
    const CKeyEntry*    pkeyentry   = reinterpret_cast< const CKeyEntry* >( pentry );
    const CKeyEntry&    keyentry    = reinterpret_cast< const CKeyEntry& >( entry );

    if ( !pkeyentry->Cmp( keyentry ) )
    {
        m_arrayKeyEntry.SetEntry( pkeyentry, keyentry );
        err = ERR::errSuccess;
    }
    else
    {
        err = ERR::errKeyChange;
    }

    return err;
}


template< class CKey, class CEntry >
inline size_t CTable< CKey, CEntry >::
Size() const
{
    return m_arrayKeyEntry.Size();
}


template< class CKey, class CEntry >
inline void CTable< CKey, CEntry >::
Clear()
{
    CArray< CKeyEntry >::ERR err = m_arrayKeyEntry.ErrSetCapacity(0);
    COLLAssert( err == CArray< CKeyEntry >::ERR::errSuccess );
}



template< class CKey, class CEntry >
inline const CEntry* CTable< CKey, CEntry >::
PEntry( const size_t ientry ) const
{
    return static_cast< const CEntry* >( m_arrayKeyEntry.PEntry( ientry ) );
}


template< class CKey, class CEntry >
inline const CEntry* CTable< CKey, CEntry >::
SeekLT( const CKey& key ) const
{
    const size_t ikeyentry = (this->*m_pfnSearch)( key, fFalse );

    if (    ikeyentry < Size() &&
            _Entry( ikeyentry ).Cmp( key ) < 0 )
    {
        return PEntry( ikeyentry );
    }
    else
    {
        return PEntry( ikeyentry - 1 );
    }
}

template< class CKey, class CEntry >
inline const CEntry* CTable< CKey, CEntry >::
SeekLE( const CKey& key ) const
{
    const size_t ikeyentry = (this->*m_pfnSearch)( key, fFalse );

    if (    ikeyentry < Size() &&
            _Entry( ikeyentry ).Cmp( key ) <= 0 )
    {
        return PEntry( ikeyentry );
    }
    else
    {
        return PEntry( ikeyentry - 1 );
    }
}

template< class CKey, class CEntry >
inline const CEntry* CTable< CKey, CEntry >::
SeekEQ( const CKey& key ) const
{
    const size_t ikeyentry = (this->*m_pfnSearch)( key, fFalse );

    if (    ikeyentry < Size() &&
            _Entry( ikeyentry ).Cmp( key ) == 0 )
    {
        return PEntry( ikeyentry );
    }
    else
    {
        return NULL;
    }
}

template< class CKey, class CEntry >
inline const CEntry* CTable< CKey, CEntry >::
SeekHI( const CKey& key ) const
{
    const size_t ikeyentry = (this->*m_pfnSearch)( key, fTrue );

    if (    ikeyentry > 0 &&
            _Entry( ikeyentry - 1 ).Cmp( key ) == 0 )
    {
        return PEntry( ikeyentry - 1 );
    }
    else
    {
        return NULL;
    }
}

template< class CKey, class CEntry >
inline const CEntry* CTable< CKey, CEntry >::
SeekGE( const CKey& key ) const
{
    const size_t ikeyentry = (this->*m_pfnSearch)( key, fTrue );

    if (    ikeyentry > 0 &&
            _Entry( ikeyentry - 1 ).Cmp( key ) == 0 )
    {
        return PEntry( ikeyentry - 1 );
    }
    else
    {
        return PEntry( ikeyentry );
    }
}

template< class CKey, class CEntry >
inline const CEntry* CTable< CKey, CEntry >::
SeekGT( const CKey& key ) const
{
    return PEntry( (this->*m_pfnSearch)( key, fTrue ) );
}

template< class CKey, class CEntry >
inline void CTable< CKey, CEntry >::
_ConfigureSearch( const BOOL fInOrder )
{
    size_t n;
    size_t log2n;
    for ( n = Size(), log2n = 0; n; n = n  / 2, log2n++ );

    if ( 2 * log2n < Size() )
    {
        if ( !fInOrder )
            _QuickSort( 0, Size() );
        m_pfnSearch = &COLL::CTable<CKey,CEntry>::_BinarySearch;
    }
    else
    {
        if ( !fInOrder )
            _InsertionSort( 0, Size() );
        m_pfnSearch = &COLL::CTable<CKey,CEntry>::_LinearSearch;
    }
}

template< class CKey, class CEntry >
inline const typename CTable< CKey, CEntry >::CKeyEntry& CTable< CKey, CEntry >::
_Entry( const size_t ikeyentry ) const
{
    return *( m_arrayKeyEntry.PEntry( ikeyentry ) );
}

template< class CKey, class CEntry >
inline void CTable< CKey, CEntry >::
_SetEntry( const size_t ikeyentry, const CKeyEntry& keyentry )
{
    m_arrayKeyEntry.SetEntry( m_arrayKeyEntry.PEntry( ikeyentry ), keyentry );
}

template< class CKey, class CEntry >
inline void CTable< CKey, CEntry >::
_SwapEntry( const size_t ikeyentry1, const size_t ikeyentry2 )
{
    CKeyEntry keyentryT;

    keyentryT = _Entry( ikeyentry1 );
    _SetEntry( ikeyentry1, _Entry( ikeyentry2 ) );
    _SetEntry( ikeyentry2, keyentryT );
}

template< class CKey, class CEntry >
inline size_t CTable< CKey, CEntry >::
_LinearSearch( const CKey& key, const BOOL fHigh ) const
{
    size_t ikeyentry;
    for ( ikeyentry = 0; ikeyentry < Size(); ikeyentry++ )
    {
        const INT cmp = _Entry( ikeyentry ).Cmp( key );

        if ( !( cmp < 0 || cmp == 0 && fHigh ) )
        {
            break;
        }
    }

    return ikeyentry;
}

template< class CKey, class CEntry >
inline size_t CTable< CKey, CEntry >::
_BinarySearch( const CKey& key, const BOOL fHigh ) const
{
    size_t  ikeyentryMin    = 0;
    size_t  ikeyentryMax    = Size();

    while ( ikeyentryMin < ikeyentryMax )
    {
        const size_t ikeyentryMid = ikeyentryMin + ( ikeyentryMax - ikeyentryMin ) / 2;

        const INT cmp = _Entry( ikeyentryMid ).Cmp( key );

        if ( cmp < 0 || cmp == 0 && fHigh )
        {
            ikeyentryMin = ikeyentryMid + 1;
        }
        else
        {
            ikeyentryMax = ikeyentryMid;
        }
    }

    return ikeyentryMax;
}

template< class CKey, class CEntry >
inline void CTable< CKey, CEntry >::
_InsertionSort( const size_t ikeyentryMinIn, const size_t ikeyentryMaxIn )
{
    size_t      ikeyentryLast;
    size_t      ikeyentryFirst;
    CKeyEntry   keyentryKey;

    for (   ikeyentryFirst = ikeyentryMinIn, ikeyentryLast = ikeyentryMinIn + 1;
            ikeyentryLast < ikeyentryMaxIn;
            ikeyentryFirst = ikeyentryLast++ )
    {
        if ( _Entry( ikeyentryFirst ).Cmp( _Entry( ikeyentryLast ) ) > 0 )
        {
            keyentryKey = _Entry( ikeyentryLast );

            _SetEntry( ikeyentryLast, _Entry( ikeyentryFirst ) );

            while ( ikeyentryFirst-- >= ikeyentryMinIn + 1 &&
                    _Entry( ikeyentryFirst ).Cmp( keyentryKey ) > 0 )
            {
                _SetEntry( ikeyentryFirst + 1, _Entry( ikeyentryFirst ) );
            }

            _SetEntry( ikeyentryFirst + 1, keyentryKey );
        }
    }
}

template< class CKey, class CEntry >
inline void CTable< CKey, CEntry >::
_QuickSort( const size_t ikeyentryMinIn, const size_t ikeyentryMaxIn )
{

    const size_t    ckeyentryMin    = 32;


    const size_t    cpartMax        = 16;
    size_t          cpart           = 0;
    struct
    {
        size_t  ikeyentryMin;
        size_t  ikeyentryMax;
    }           rgpart[ cpartMax ];


    size_t  ikeyentryMin    = ikeyentryMinIn;
    size_t  ikeyentryMax    = ikeyentryMaxIn;


    for ( ; ; )
    {

        if ( ikeyentryMax - ikeyentryMin < ckeyentryMin )
        {
            _InsertionSort( ikeyentryMin, ikeyentryMax );


            if ( !cpart )
            {
                break;
            }


            ikeyentryMin    = rgpart[ --cpart ].ikeyentryMin;
            ikeyentryMax    = rgpart[ cpart ].ikeyentryMax;
            continue;
        }


        size_t      ikeyentryFirst  = ikeyentryMin;
        size_t      ikeyentryMid    = ikeyentryMin + ( ikeyentryMax - ikeyentryMin ) / 2;
        size_t      ikeyentryLast   = ikeyentryMax - 1;

        if ( _Entry( ikeyentryFirst ).Cmp( _Entry( ikeyentryMid ) ) > 0 )
        {
            _SwapEntry( ikeyentryFirst, ikeyentryMid );
        }
        if ( _Entry( ikeyentryFirst ).Cmp( _Entry( ikeyentryLast ) ) > 0 )
        {
            _SwapEntry( ikeyentryFirst, ikeyentryLast );
        }
        if ( _Entry( ikeyentryMid ).Cmp( _Entry( ikeyentryLast ) ) > 0 )
        {
            _SwapEntry( ikeyentryMid, ikeyentryLast );
        }


        do  {

            while ( ikeyentryFirst <= ikeyentryLast &&
                    _Entry( ikeyentryFirst ).Cmp( _Entry( ikeyentryMin ) ) <= 0 )
            {
                ikeyentryFirst++;
            }


            while ( ikeyentryFirst <= ikeyentryLast &&
                    _Entry( ikeyentryLast ).Cmp( _Entry( ikeyentryMin ) ) > 0 )
            {
                ikeyentryLast--;
            }


            if ( ikeyentryFirst < ikeyentryLast )
            {
                _SwapEntry( ikeyentryFirst++, ikeyentryLast-- );
            }
        }
        while ( ikeyentryFirst <= ikeyentryLast );


        _SwapEntry( ikeyentryMin, ikeyentryLast );


        size_t  ikeyentrySmallMin;
        size_t  ikeyentrySmallMax;
        size_t  ikeyentryLargeMin;
        size_t  ikeyentryLargeMax;

        if ( ikeyentryMax - ikeyentryFirst == 0 )
        {
            ikeyentryLargeMin   = ikeyentryMin;
            ikeyentryLargeMax   = ikeyentryLast;
            ikeyentrySmallMin   = ikeyentryLast;
            ikeyentrySmallMax   = ikeyentryMax;
        }
        else if ( ikeyentryMax - ikeyentryFirst > ikeyentryFirst - ikeyentryMin )
        {
            ikeyentrySmallMin   = ikeyentryMin;
            ikeyentrySmallMax   = ikeyentryFirst;
            ikeyentryLargeMin   = ikeyentryFirst;
            ikeyentryLargeMax   = ikeyentryMax;
        }
        else
        {
            ikeyentryLargeMin   = ikeyentryMin;
            ikeyentryLargeMax   = ikeyentryFirst;
            ikeyentrySmallMin   = ikeyentryFirst;
            ikeyentrySmallMax   = ikeyentryMax;
        }


        if ( cpart < cpartMax )
        {
            rgpart[ cpart ].ikeyentryMin    = ikeyentryLargeMin;
            rgpart[ cpart++ ].ikeyentryMax  = ikeyentryLargeMax;
        }
        else
        {
            _QuickSort( ikeyentryLargeMin, ikeyentryLargeMax );
        }


        ikeyentryMin    = ikeyentrySmallMin;
        ikeyentryMax    = ikeyentrySmallMax;
    }
}



class CStupidQueue {

    public:


        enum class ERR
        {
            errSuccess,
            errInvalidParameter,
            errOutOfMemory,
            wrnEmptyQueue
        };

    public:

        CStupidQueue( DWORD cbElement );
        ~CStupidQueue( void );


        ERR ErrEnqueue( void * pvEntry );
        ERR ErrPeek( void * pvEntry );
        ERR ErrDequeue( void * pvEntry );

#ifdef DEBUG
        void AssertValid( void );
#else
        void AssertValid( void ) { ; }
#endif

    private:

        DWORD   m_cbElement;
        DWORD   m_iHead;
        DWORD   m_iTail;

        DWORD   m_cAlloc;
        void *  m_rg;

        ERR ErrAdjustSize( void );
        void * PvElement( const DWORD iElement );
};

#ifdef DEBUG
inline void CStupidQueue::AssertValid( void )
{
    COLLAssert( m_cbElement );
    if ( m_cAlloc )
    {
        COLLAssert( m_rg );
    }
    else
    {
        COLLAssert( m_rg == NULL );
    }
}
#endif

inline CStupidQueue::ERR CStupidQueue::ErrAdjustSize( void )
{
    ERR err = ERR::errSuccess;
    DWORD mGrowth = 2;

    AssertValid();

    DWORD cNew = m_cAlloc * mGrowth;
    void * rgNew = NULL;
    if ( NULL == ( rgNew = malloc( m_cbElement * cNew ) ) )
    {
        err = ERR::errOutOfMemory;
        goto HandleError;
    }

    memcpy( rgNew, m_rg, m_cAlloc * m_cbElement );
    free( m_rg );

    m_cAlloc = cNew;
    m_rg = rgNew;

    DWORD iT = ( m_iTail + 1 ) % ( m_cAlloc / mGrowth );
    if ( iT != 0 )
    {
        memcpy( PvElement( m_cAlloc / mGrowth ), PvElement( 0 ), iT * m_cbElement );
        m_iTail = ( m_cAlloc / mGrowth ) + m_iTail;
        COLLAssert( m_iTail < m_cAlloc );
    }
    else
    {
        COLLAssert( m_iHead == 0 );
    }

    err = ERR::errSuccess;

HandleError:
    AssertValid();
    
    return err;
}

inline void * CStupidQueue::PvElement( const DWORD iElement )
{
    AssertValid();
    COLLAssert( iElement < m_cAlloc );
    return ( &(((BYTE*)m_rg)[ iElement * m_cbElement ]) );
}

inline CStupidQueue::CStupidQueue( DWORD cbElement )
{
    m_cbElement = cbElement;

    m_cAlloc = 0;
    m_rg = NULL;

    m_iHead = 0;
    m_iTail = 0;

    AssertValid();
}
inline CStupidQueue::~CStupidQueue( void )
{
    AssertValid();
    if ( m_rg )
    {
        free( m_rg );
        m_rg = NULL;
        m_cAlloc = 0;
    }
    AssertValid();
    m_iHead = m_iTail = 0;
}

inline CStupidQueue::ERR CStupidQueue::ErrEnqueue( void * pvEntry )
{
    ERR err = ERR::errSuccess;

    AssertValid();

    if ( m_cAlloc == 0 )
    {
        COLLAssert( m_rg == NULL );
        m_cAlloc = 32;
        if ( NULL == ( m_rg = malloc( m_cbElement * m_cAlloc ) ) )
        {
            err = ERR::errOutOfMemory;
            goto HandleError;
        }
    }

    if( m_iHead == ( ( m_iTail + 1 ) % m_cAlloc ) )
    {
        err = ErrAdjustSize();
        if ( err != ERR::errSuccess )
        {
            goto HandleError;
        }
    }

    m_iTail = ( m_iTail + 1 ) % m_cAlloc;
    COLLAssert( m_iHead != m_iTail );
    memcpy( PvElement( m_iTail ), pvEntry, m_cbElement );

    err = ERR::errSuccess;

HandleError:
    AssertValid();

    return err;
}

inline CStupidQueue::ERR CStupidQueue::ErrPeek( void * pvEntry )
{
    if( m_iHead == m_iTail )
    {
        return ERR::wrnEmptyQueue;
    }
 
    DWORD iPeek = ( m_iHead + 1 ) % m_cAlloc;
    memcpy( pvEntry, PvElement( iPeek ), m_cbElement );

    AssertValid();
    return ERR::errSuccess;
}

inline CStupidQueue::ERR CStupidQueue::ErrDequeue( void * pvEntry )
{
    ERR err = ErrPeek( pvEntry );
    if ( err == ERR::errSuccess )
    {
        m_iHead = ( ++m_iHead ) % m_cAlloc;
    }
    AssertValid();
    return err;
}



template<class P>
P GetCachedPtr( P * const array, const INT arraySize )
{
    C_ASSERT( sizeof( void* ) == sizeof( P ) );
    const INT startIndex = OSSyncGetCurrentProcessor() % arraySize;
    INT index = startIndex;
    do
    {
        const P ptr = ( P )AtomicExchangePointer( ( void ** )( &array[index] ), 0 );
        if( 0 != ptr )
        {
            return ptr;
        }
        index = ( index + 1 ) % arraySize;
    } while( startIndex != index );
    return ( P )0;
}

template<class P>
bool FCachePtr( P ptr, P * const array, const INT arraySize )
{
    C_ASSERT( sizeof( void* ) == sizeof( P ) );
    COLLAssert( 0 != ptr );
    
    const INT startIndex = OSSyncGetCurrentProcessor() % arraySize;
    INT index = startIndex;
    do
    {
        if( 0 == AtomicCompareExchangePointer( ( void ** )( &array[index] ), 0, ptr ) )
        {
            return true;
        }
        index = ( index + 1 ) % arraySize;
    } while( startIndex != index );
    return false;
}

template<class Object, INT NumObjectsInPool>
class ObjectPool
{
public:
    ObjectPool();
    ~ObjectPool();
    
    Object * Allocate();
    void Free( Object * pobject );

    void FreeAllEntries();

private:
    Object * NewObject();
    void DeleteObject( Object * pobject );
    
private:
    Object * m_rgpobjects[NumObjectsInPool];
    LONG m_cobjectsAllocated;
    LONG m_cobjectsFreed;

private:
    ObjectPool( const ObjectPool& );
    ObjectPool& operator=( const ObjectPool& );
};

template<class Object, INT NumObjectsInPool>
ObjectPool<Object,NumObjectsInPool>::ObjectPool() :
    m_cobjectsAllocated( 0 ),
    m_cobjectsFreed( 0 )
{
    for( INT i = 0; i < NumObjectsInPool; ++i )
    {
        m_rgpobjects[i] = NULL;
    }
}

template<class Object, INT NumObjectsInPool>
ObjectPool<Object,NumObjectsInPool>::~ObjectPool()
{
    FreeAllEntries();
    COLLAssert( m_cobjectsAllocated == m_cobjectsFreed );
}

template<class Object, INT NumObjectsInPool>
Object * ObjectPool<Object,NumObjectsInPool>::Allocate()
{
    Object* pt = GetCachedPtr<Object*>( m_rgpobjects, NumObjectsInPool );
    if ( NULL == pt )
    {
        pt = NewObject();
    }
    return pt;
}

template<class Object, INT NumObjectsInPool>
void ObjectPool<Object,NumObjectsInPool>::Free( Object * pobject )
{
    if ( !FCachePtr<Object*>( pobject, m_rgpobjects, NumObjectsInPool ) )
    {
        DeleteObject( pobject );
    }
}

template<class Object, INT NumObjectsInPool>
void ObjectPool<Object,NumObjectsInPool>::FreeAllEntries()
{
    for ( INT i = 0; i < NumObjectsInPool; ++i )
    {
        DeleteObject( m_rgpobjects[i] );
    }
}

template<class Object, INT NumObjectsInPool>
Object * ObjectPool<Object,NumObjectsInPool>::NewObject()
{
    Object * const pobject = new Object();
    if ( pobject )
    {
        AtomicIncrement( &m_cobjectsAllocated );
    }
    return pobject;
}

template<class Object, INT NumObjectsInPool>
void ObjectPool<Object,NumObjectsInPool>::DeleteObject( Object * pobject )
{
    if ( pobject )
    {
        delete pobject;
        AtomicIncrement( &m_cobjectsFreed );
    }
}



enum RedBlackTreeColor { Red, Black };

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
class InvasiveRedBlackTree
{
public:
    class InvasiveContext
    {
    public:
        InvasiveContext() :
            m_key(),
            m_pnodeLeft( 0 ),
            m_pnodeRight( 0 ),
            m_color( Red )
        {
        }

        InvasiveContext( const KEY& key ) :
            m_key( key ),
            m_pnodeLeft( 0 ),
            m_pnodeRight( 0 ),
            m_color( Red )
        {
        }

        ~InvasiveContext()
        {
#ifdef DEBUG
            m_key = KEY();
            m_pnodeLeft = NULL;
            m_pnodeRight = NULL;
            m_color = Red;
#endif
        }

        KEY Key() const { return m_key; }
        RedBlackTreeColor Color() const { return m_color; }

        InvasiveContext* PnodeLeft() { return m_pnodeLeft; }
        const InvasiveContext* PnodeLeft() const { return m_pnodeLeft; }

        InvasiveContext* PnodeRight() { return m_pnodeRight; }
        const InvasiveContext* PnodeRight() const { return m_pnodeRight; }

        void SetKey( const KEY& key ) { m_key = key; }
        void SetColor( RedBlackTreeColor colorNew ) { m_color = colorNew; }
        void FlipColor() { m_color = ( Red == m_color ) ? Black : Red; }

        void SetLeft( InvasiveContext* const pnodeleftNew )
        {
            m_pnodeLeft = pnodeleftNew;
        }

        void SetRight( InvasiveContext* const pnoderightNew )
        {
            m_pnodeRight = pnoderightNew;
        }

        void AssertValid() const;

    private:
        KEY                 m_key;
        RedBlackTreeColor   m_color;
        InvasiveContext*    m_pnodeLeft;
        InvasiveContext*    m_pnodeRight;
    };

    typedef InvasiveContext Node;

    enum class ERR
    {
        errSuccess = 0,
        errOutOfMemory = -20,
        errEntryNotFound = -21,
        errDuplicateEntry = -22,
    };

    InvasiveRedBlackTree();
    ~InvasiveRedBlackTree();

    ERR ErrFind( const KEY& key, CObject** const ppObject ) const;

    ERR ErrFindNearest( const KEY& key, KEY* const pkeyFound, CObject** const ppObjectFound ) const;

    ERR ErrFindGreaterThanOrEqual( const KEY& key, KEY* const pkeyFound, CObject** const ppObjectFound ) const;

    ERR ErrInsert( const KEY& key, CObject* pObject );
    ERR ErrDelete( const KEY& key, CObject** const ppObject = NULL );

    bool FEmpty() const { return NULL == m_pnodeRoot; }
    void MakeEmpty();
    void AssertValid() const { AssertValid_( m_pnodeRoot ); }

    void Move( class InvasiveRedBlackTree< KEY, CObject, OffsetOfILE > * prbtFrom );
    ERR ErrMerge( class InvasiveRedBlackTree< KEY, CObject, OffsetOfILE > * prbtFrom );

    const CObject* PnodeRoot() const { return NodeToCObject( m_pnodeRoot ); }
    const CObject* FindNode( const KEY& key ) const { return NodeToCObject( Find_( m_pnodeRoot, key ) ); }

protected:

    void InitNode_( const KEY& key, Node* pnode );
    void ScrubNode_( Node * const pnode );

    void MakeEmpty_( Node * const pnode );
    void AssertValid_( const Node * const pnode ) const;

    bool FContainsKey_( const KEY& key );
    Node * Find_( Node * const pnode, const KEY& key ) const;
    Node * PnodeMin_( Node * const pnode );

    ERR ErrInsert_( const KEY& key, Node* pnodeNew, Node** ppnodeRoot );
    Node * Delete_( Node * const pnode, const KEY& key, Node** const ppnodeDeleted );
    Node * RemoveMinNode_( Node * const pnode );

    Node * RotateLeft_( Node * const pnode );
    Node * RotateRight_( Node * const pnode );
    void FlipColors_( Node * const pnode );

    Node * Fixup_( Node * const pnode );
    Node * MoveRedRight_( Node * const pnode );
    Node * MoveRedLeft_( Node * const pnode );

    bool FIsNonNullAndRed( const Node* const pnode )
    {
        return pnode && ( Red == pnode->Color() );
    }

#ifdef DEBUG
    INT CDepth_( const Node * const pic ) const;
    void PrintNode_( INT cLevel, const Node * const pic ) const;
public:
    void Print() const;
#endif

protected:
    Node * m_pnodeRoot;

private:
    InvasiveRedBlackTree( const InvasiveRedBlackTree& );
    InvasiveRedBlackTree& operator=( const InvasiveRedBlackTree& );

    Node* CObjectToNode( CObject* pObject ) const
    {
        return ( Node* ) ( ( ( BYTE* ) pObject ) + OffsetOfILE() );
    }

    CObject* NodeToCObject( Node* pnode ) const
    {
        return ( CObject* ) ( ( ( BYTE* ) pnode ) - OffsetOfILE() );
    }
};

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
void InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::InvasiveContext::AssertValid() const
{
    if( m_pnodeLeft )
    {
        COLLAssert( m_pnodeLeft->Key() < Key() );
        COLLAssert( this != m_pnodeLeft );
    }
    if( m_pnodeRight )
    {
        COLLAssert( m_pnodeRight->Key() > Key() );
        COLLAssert( this != m_pnodeRight );
    }
    if( m_pnodeLeft && m_pnodeRight )
    {
        COLLAssert( Black == Color() || Black == m_pnodeLeft->Color() || Black == m_pnodeRight->Color() );
    }
    if( m_pnodeRight && Red == m_pnodeRight->Color() )
    {
        COLLAssert( m_pnodeLeft );
        COLLAssert( Red == m_pnodeLeft->Color() );
    }
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::InvasiveRedBlackTree() :
    m_pnodeRoot( 0 )
{
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::~InvasiveRedBlackTree()
{
    COLLAssert( FEmpty() );
    MakeEmpty();
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
typename InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::ERR
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::ErrFind( const KEY& key, CObject** const ppObject ) const
{
    ERR err;

    Node * const pnode = Find_( m_pnodeRoot, key );
    if( NULL == pnode )
    {
        *ppObject = NULL;
        err = ERR::errEntryNotFound;
    }
    else
    {
        *ppObject = NodeToCObject( pnode );
        err = ERR::errSuccess;
    }

    COLLAssert( ERR::errSuccess == err || ERR::errEntryNotFound == err );
    return err;
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
typename InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::ERR
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::ErrFindNearest( const KEY& key, KEY * const pkeyFound, CObject** const ppObjectFound ) const
{
    if( NULL == m_pnodeRoot )
    {
        return ERR::errEntryNotFound;
    }

    Node * pnodeT = m_pnodeRoot;
    while ( true )
    {
        if( key == pnodeT->Key() )
        {
            break;
        }
        else if( key < pnodeT->Key() )
        {
            if( NULL == pnodeT->PnodeLeft() )
            {
                break;
            }
            pnodeT = pnodeT->PnodeLeft();
        }
        else
        {
            COLLAssert( key > pnodeT->Key() );
            if( NULL == pnodeT->PnodeRight() )
            {
                break;
            }
            pnodeT = pnodeT->PnodeRight();
        }
    }

    COLLAssert( pnodeT );
    *pkeyFound = pnodeT->Key();
    *ppObjectFound = NodeToCObject( pnodeT );
    return ERR::errSuccess;
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
typename InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::ERR
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::ErrFindGreaterThanOrEqual( const KEY& key, KEY* const pkeyFound, CObject** const ppObjectFound ) const
{
    Node * pnodeCurrent = m_pnodeRoot;
    Node * pnodeMatch = NULL;
    while ( NULL != pnodeCurrent )
    {
        if ( key == pnodeCurrent->Key() )
        {
            pnodeMatch = pnodeCurrent;
            break;
        }
        else if ( key < pnodeCurrent->Key() )
        {
            pnodeMatch = pnodeCurrent;
            pnodeCurrent = pnodeCurrent->PnodeLeft();
        }
        else
        {
            pnodeCurrent = pnodeCurrent->PnodeRight();
        }
    }

    COLLAssert( ( NULL == pnodeCurrent ) || ( key == pnodeCurrent->Key() ));

    if ( NULL == pnodeMatch )
    {
        return ERR::errEntryNotFound;
    }

    *pkeyFound = pnodeMatch->Key();
    *ppObjectFound = NodeToCObject( pnodeMatch );
    COLLAssert( *pkeyFound >= key );
    return ERR::errSuccess;
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
typename InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::ERR
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::ErrInsert( const KEY& key, CObject* pObject )
{
    Node* pnode = CObjectToNode( pObject );
    ERR err = ErrInsert_( key, pnode, &m_pnodeRoot );

    COLLAssert( ERR::errSuccess == err || ERR::errOutOfMemory == err || ERR::errDuplicateEntry == err );
    if( ERR::errSuccess == err || ERR::errDuplicateEntry == err )
    {
        COLLAssert( !FEmpty() );
        COLLAssert( FContainsKey_( key ) );
    }
    return err;
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
typename InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::ERR
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::ErrDelete( const KEY& key, CObject** const ppObject  )
{
    ERR err;
    Node* pnodeDeleted = NULL;

    if( !FContainsKey_( key ) )
    {
        err = ERR::errEntryNotFound;
    }
    else
    {
        COLLAssert( !FEmpty() );
        m_pnodeRoot = Delete_( m_pnodeRoot, key, &pnodeDeleted );
        if( ppObject )
        {
            *ppObject = NodeToCObject( pnodeDeleted );
        }
        err = ERR::errSuccess;
    }

    COLLAssert( ERR::errSuccess == err || ERR::errEntryNotFound == err );
    COLLAssert( !FContainsKey_( key ) );
    return err;
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
void InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::MakeEmpty()
{
    MakeEmpty_( m_pnodeRoot );
    m_pnodeRoot = NULL;
    COLLAssert( FEmpty() );
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
void InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::MakeEmpty_( Node * const pnode )
{
    if( pnode )
    {
        MakeEmpty_( pnode->PnodeLeft() );
        MakeEmpty_( pnode->PnodeRight() );
        ScrubNode_( pnode );
    }
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
void InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::AssertValid_( const Node * const pnode ) const
{
    if( pnode )
    {
        pnode->AssertValid();
        if( pnode->PnodeLeft() )
        {
            AssertValid_( pnode->PnodeLeft() );
        }
        if( pnode->PnodeRight() )
        {
            AssertValid_( pnode->PnodeRight() );
        }
    }
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
typename InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::ERR
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::ErrMerge( class InvasiveRedBlackTree< KEY, CObject, OffsetOfILE > * prbtSrc )
{
    InvasiveRedBlackTree<KEY, CObject, OffsetOfILE> rbtLeftOvers;


    if ( FEmpty() )
    {
        Move( prbtSrc );
    }


    Node * pnodeMove;
    while( ( pnodeMove = prbtSrc->m_pnodeRoot ) != NULL )
    {
        KEY keyMove = pnodeMove->Key();

        const ERR errDel = prbtSrc->ErrDelete( keyMove );
        COLLAssert( errDel == ERR::errSuccess );
        COLLAssert( pnodeMove->PnodeLeft() == NULL );
        COLLAssert( pnodeMove->PnodeRight() == NULL );

        const ERR errMove = ErrInsert( keyMove, NodeToCObject( pnodeMove ) );
        if ( errMove != ERR::errSuccess )
        {
            COLLAssert( errMove == ERR::errDuplicateEntry );
            const ERR errLeftOver = rbtLeftOvers.ErrInsert( keyMove, NodeToCObject( pnodeMove ) );
            COLLAssert( errLeftOver == ERR::errSuccess );
        }
    }

    COLLAssert( prbtSrc->FEmpty() );

    if ( !rbtLeftOvers.FEmpty() )
    {
        prbtSrc->Move( &rbtLeftOvers );
        COLLAssert( rbtLeftOvers.FEmpty() );
        return ERR::errDuplicateEntry;
    }

    return ERR::errSuccess;
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
void InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::Move( class InvasiveRedBlackTree< KEY, CObject, OffsetOfILE > * prbtSrc )
{
    COLLAssert( FEmpty() );
    if ( !FEmpty() )
    {
        COLLAssert( fFalse );
        return;
    }
    COLLAssert( this != prbtSrc || m_pnodeRoot == NULL );

    m_pnodeRoot = prbtSrc->m_pnodeRoot;
    prbtSrc->m_pnodeRoot = NULL;

    COLLAssert( prbtSrc->FEmpty() );
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
bool InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::FContainsKey_( const KEY& key )
{
    return NULL != Find_( m_pnodeRoot, key );
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
typename InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::Node*
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::Find_( Node * const pnode, const KEY& key ) const
{
    Node * pnodeT = pnode;
    while ( NULL != pnodeT )
    {
        if( key == pnodeT->Key() )
        {
            break;
        }
        else if( key < pnodeT->Key() )
        {
            pnodeT = pnodeT->PnodeLeft();
        }
        else
        {
            COLLAssert( key > pnodeT->Key() );
            pnodeT = pnodeT->PnodeRight();
        }
    }

    COLLAssert( NULL == pnodeT || key == pnodeT->Key() );
    return pnodeT;
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
typename InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::Node*
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::PnodeMin_( Node * const pnode )
{
    COLLAssert( NULL != pnode );

    Node * pnodeT = pnode;
    while ( NULL != pnodeT->PnodeLeft() )
    {
        pnodeT = pnodeT->PnodeLeft();
    }
    return pnodeT;
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
typename InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::ERR
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::ErrInsert_( const KEY& key, Node* pnodeNew, Node** ppnodeRoot )
{
    ERR err = ERR::errSuccess;

    if( NULL == *ppnodeRoot )
    {
        InitNode_( key, pnodeNew );
        *ppnodeRoot = pnodeNew;
        goto HandleError;
    }

    Node* pnodeRoot = *ppnodeRoot;

    if( key == pnodeRoot->Key() )
    {
        err = ERR::errDuplicateEntry;
    }
    else if( key < pnodeRoot->Key() )
    {
        Node * pnodeT = pnodeRoot->PnodeLeft();
        err = ErrInsert_( key, pnodeNew, &pnodeT );
        pnodeRoot->SetLeft( pnodeT );
    }
    else
    {
        COLLAssert( key > pnodeRoot->Key() );
        Node * pnodeT = pnodeRoot->PnodeRight();
        err = ErrInsert_( key, pnodeNew, &pnodeT );
        pnodeRoot->SetRight( pnodeT );
    }

    pnodeRoot = Fixup_( pnodeRoot );
    pnodeRoot->AssertValid();
    *ppnodeRoot = pnodeRoot;

HandleError:
    COLLAssert( ERR::errSuccess == err || ERR::errOutOfMemory == err || ERR::errDuplicateEntry == err );
    return err;
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
typename InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::Node*
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::RotateLeft_( Node * const pnode )
{
    COLLAssert( pnode );
    COLLAssert( pnode->PnodeRight() );

    Node * pnodeNewTop = pnode->PnodeRight();
    pnode->SetRight( pnodeNewTop->PnodeLeft() );
    pnodeNewTop->SetLeft( pnode );
    pnodeNewTop->SetColor( pnodeNewTop->PnodeLeft()->Color() );
    pnodeNewTop->PnodeLeft()->SetColor( Red );

    return pnodeNewTop;
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
typename InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::Node*
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::RotateRight_( Node * const pnode )
{
    COLLAssert( pnode );
    COLLAssert( pnode->PnodeLeft() );

    Node * pnodeNewTop = pnode->PnodeLeft();
    pnode->SetLeft( pnodeNewTop->PnodeRight() );
    pnodeNewTop->SetRight( pnode );
    pnodeNewTop->SetColor( pnodeNewTop->PnodeRight()->Color() );
    pnodeNewTop->PnodeRight()->SetColor( Red );

    return pnodeNewTop;
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
void InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::FlipColors_( Node * const pnode )
{
    COLLAssert( pnode );

    pnode->FlipColor();
    if( pnode->PnodeLeft() )
    {
        pnode->PnodeLeft()->FlipColor();
    }
    if( pnode->PnodeRight() )
    {
        pnode->PnodeRight()->FlipColor();
    }
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
typename InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::Node*
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::Delete_( Node * const pnode, const KEY& key, Node** const ppnodeDeleted )
{
    COLLAssert( pnode );

    Node * pnodeT = pnode;
    if( key < pnodeT->Key() )
    {
        COLLAssert( pnodeT->PnodeLeft() );
        if( !FIsNonNullAndRed( pnodeT->PnodeLeft() )
            && !FIsNonNullAndRed( pnodeT->PnodeLeft()->PnodeLeft() ) )
        {
            pnodeT = MoveRedLeft_( pnodeT );
        }
        pnodeT->SetLeft( Delete_( pnodeT->PnodeLeft(), key, ppnodeDeleted ) );
    }
    else
    {
        if( FIsNonNullAndRed( pnodeT->PnodeLeft() ) )
        {
            pnodeT = RotateRight_( pnodeT );
        }

        if( key == pnodeT->Key() && NULL == pnodeT->PnodeRight() )
        {
            COLLAssert( NULL == pnodeT->PnodeLeft() );
            COLLAssert( Red == pnodeT->Color() );
            ScrubNode_( pnodeT );
            *ppnodeDeleted = pnodeT;
            return NULL;
        }

        if( !FIsNonNullAndRed( pnodeT->PnodeRight() ) && !FIsNonNullAndRed( pnodeT->PnodeRight()->PnodeLeft() ) )
        {
            pnodeT = MoveRedRight_( pnodeT );
        }

        if( key == pnodeT->Key() )
        {

            Node* pnodeDelete = pnodeT;
            Node * const pnodeMin = PnodeMin_( pnodeT->PnodeRight() );
            Node* pnodeRight = RemoveMinNode_( pnodeT->PnodeRight() );

            pnodeT = pnodeMin;
            pnodeT->SetLeft( pnodeDelete->PnodeLeft() );
            pnodeT->SetRight( pnodeRight );
            pnodeT->SetColor( pnodeDelete->Color() );
            ScrubNode_( pnodeDelete );
            *ppnodeDeleted = pnodeDelete;
        }
        else
        {
            pnodeT->SetRight( Delete_( pnodeT->PnodeRight(), key, ppnodeDeleted ) );
        }
    }
    
    COLLAssert( *ppnodeDeleted != NULL );
    return Fixup_( pnodeT );
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
typename InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::Node*
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::RemoveMinNode_( Node * const pnode )
{
    Node * pnodeT = pnode;
    if( NULL == pnodeT->PnodeLeft() )
    {
        COLLAssert( pnodeT->PnodeRight() == NULL );
        return NULL;
    }

    if( !FIsNonNullAndRed( pnodeT->PnodeLeft() ) && !FIsNonNullAndRed( pnodeT->PnodeLeft()->PnodeLeft() ) )
    {
        pnodeT = MoveRedLeft_( pnodeT );
    }

    pnodeT->SetLeft( RemoveMinNode_( pnodeT->PnodeLeft() ) );
    return Fixup_( pnodeT );
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
typename InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::Node*
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::Fixup_( Node * const pnode )
{
    COLLAssert( pnode );

    Node * pnodeT = pnode;
    if( FIsNonNullAndRed( pnodeT->PnodeRight() ) )
    {
        pnodeT = RotateLeft_( pnodeT );
    }
    if( FIsNonNullAndRed( pnodeT->PnodeLeft() ) && FIsNonNullAndRed( pnodeT->PnodeLeft()->PnodeLeft() ) )
    {
        COLLAssert( !FIsNonNullAndRed( pnodeT->PnodeRight() ) );
        pnodeT = RotateRight_( pnodeT );
    }
    if( FIsNonNullAndRed( pnodeT->PnodeLeft() ) && FIsNonNullAndRed( pnodeT->PnodeRight() ) )
    {
        FlipColors_( pnodeT );
    }

    COLLAssert( pnodeT );
    return pnodeT;
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
typename InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::Node*
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::MoveRedRight_( Node * const pnode )
{
    COLLAssert( pnode );

    Node * pnodeT = pnode;
    FlipColors_( pnodeT );
    COLLAssert( pnodeT->PnodeLeft() );
    if( FIsNonNullAndRed( pnodeT->PnodeLeft()->PnodeLeft() ) )
    {
        pnodeT = RotateRight_( pnodeT );
        FlipColors_( pnodeT );
    }

    COLLAssert( pnodeT );
    return pnodeT;
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
typename InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::Node*
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::MoveRedLeft_( Node * const pnode )
{
    COLLAssert( pnode );

    Node * pnodeT = pnode;
    FlipColors_( pnode );
    COLLAssert( pnodeT->PnodeRight() );
    if( FIsNonNullAndRed( pnodeT->PnodeRight()->PnodeLeft() ) )
    {
        pnodeT->SetRight( RotateRight_( pnodeT->PnodeRight() ) );
        pnodeT = RotateLeft_( pnodeT );
        FlipColors_( pnodeT );
    }

    COLLAssert( pnodeT );
    return pnodeT;
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
void InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::InitNode_( const KEY& key, Node* pnode )
{
    new( pnode ) Node( key );
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
void InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::ScrubNode_( Node * const pnode )
{
    pnode->~Node();
}

#ifdef DEBUG
template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
INT InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::CDepth_( const Node * const pic ) const
{
    if( pic == NULL )
    {
        return 0;
    }
    return 1 + max( CDepth_( pic->PnodeLeft() ), CDepth_( pic->PnodeRight() ) );
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
void InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::PrintNode_( INT cLevel, const Node * const pic ) const
{
    cLevel++;
    if( pic->PnodeLeft() )
    {
        PrintNode_( cLevel, pic->PnodeLeft() );
    }
    
    CHAR szKeyT[100];
    wprintf( L"\t%*c picThis  = %p (%hs) / pnode = \t  { %hs }\n", 4 * ( cLevel + 1 ), L'.',
                pic, ( pic->Color() == Red ) ? "Red" : "Blk", pic->Key().Sz( sizeof( szKeyT ), szKeyT ) );
    if( pic->PnodeRight() )
    {
        PrintNode_( cLevel, pic->PnodeRight() );
    }
}

template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
void InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::Print() const
{
    const INT cDepth = CDepth_( m_pnodeRoot );
    INT cTabs = 1;
    wprintf( L"%*hc Printing irbt = %p.m_pnodeRoot = %p (%d levels)\n", cTabs + 1, '\t', this, m_pnodeRoot, cDepth );
    wprintf( L"%*hc --------------------------------\n", cTabs + 1, '\t' );
    if( m_pnodeRoot )
    {
        PrintNode_( 0, m_pnodeRoot );
    }
    else
    {
        wprintf(L"%*hc\tEmpty/Null Root.\n", cTabs + 1, '\t');
    }
    wprintf(L"%*hc --------------------------------\n", cTabs + 1, '\t');
}
#endif


template<class KEY, class DATA>
class CRedBlackTreeNode
{
public:
    static SIZE_T OffsetOfRedBlackTreeIC() { return OffsetOf( CRedBlackTreeNode, m_icRedBlackTree ); }
    typedef InvasiveRedBlackTree<KEY, CRedBlackTreeNode, OffsetOfRedBlackTreeIC> BaseType;

    CRedBlackTreeNode( const DATA& data ) :
        m_data( data )
    {
    }

    ~CRedBlackTreeNode() {}

    RedBlackTreeColor Color() const             { return m_icRedBlackTree.Color(); }

    CRedBlackTreeNode* PnodeLeft()              { return ICToNode( m_icRedBlackTree.PnodeLeft() ); }
    const CRedBlackTreeNode* PnodeLeft() const  { return ICToNode( m_icRedBlackTree.PnodeLeft() ); }

    CRedBlackTreeNode* PnodeRight()             { return ICToNode( m_icRedBlackTree.PnodeRight() ); }
    const CRedBlackTreeNode* PnodeRight() const { return ICToNode( m_icRedBlackTree.PnodeRight() ); }

    KEY Key() const                             { return m_icRedBlackTree.Key(); }
    DATA Data() const                           { return m_data; }
    void SetData( const DATA& data )            { m_data = data; }

private:
    static CRedBlackTreeNode* ICToNode( typename BaseType::InvasiveContext* pic ) { return (CRedBlackTreeNode*) ( ( (BYTE*) pic ) - OffsetOfRedBlackTreeIC() ); }
    static const CRedBlackTreeNode* ICToNode( const typename BaseType::InvasiveContext* const pic ) { return ICToNode( const_cast<BaseType::InvasiveContext*>( pic ) ); }

private:
    typename BaseType::InvasiveContext  m_icRedBlackTree;
    DATA                    m_data;
};

template<class KEY, class DATA>
class CRedBlackTree
{
public:
    typedef CRedBlackTreeNode<KEY, DATA> Node;
    typedef InvasiveRedBlackTree<KEY, Node, Node::OffsetOfRedBlackTreeIC> BaseType;
    typedef typename BaseType::ERR ERR;

    CRedBlackTree() {};
    ~CRedBlackTree();

    ERR ErrFind( const KEY& key, DATA * const pdata = NULL ) const;

    ERR ErrFindNearest( const KEY& key, KEY * const pkeyFound, DATA * const pdataFound = NULL ) const;

    ERR ErrFindGreaterThanOrEqual( const KEY& key, KEY * const pkeyFound, DATA * const pdataFound = NULL ) const;

    ERR ErrInsert( const KEY& key, const DATA& data );
    ERR ErrDelete( const KEY& key );
    void MakeEmpty();

    bool FEmpty() const                             { return m_irbtBase.FEmpty(); }
    const Node* PnodeRoot() const                   { return m_irbtBase.PnodeRoot(); }
    const Node* FindNode( const KEY& key ) const    { return m_irbtBase.FindNode( key ); }
    void AssertValid() const                        { m_irbtBase.AssertValid(); }

private:
    Node* PnodeAlloc_( const DATA& data );
    void Free_( Node * const pnode );

    void MakeEmpty_( Node * const pnode );
    void AssertValid_( const Node * const pnode ) const;

private:
    CRedBlackTree( const CRedBlackTree& );
    CRedBlackTree& operator=( const CRedBlackTree& );

private:
    BaseType m_irbtBase;
};

template<class KEY, class DATA>
CRedBlackTree<KEY, DATA>::~CRedBlackTree()
{
    COLLAssert( FEmpty() );
    MakeEmpty();
}

template<class KEY, class DATA>
typename CRedBlackTree<KEY, DATA>::ERR
CRedBlackTree<KEY, DATA>::ErrFind( const KEY& key, DATA * const pdata ) const
{
    ERR err;
    Node* pnode;
    err = m_irbtBase.ErrFind( key, &pnode );
    if ( pdata && ( err == ERR::errSuccess ) )
    {
        *pdata = pnode->Data();
    }

    COLLAssert( ERR::errSuccess == err || ERR::errEntryNotFound == err );
    return err;
}

template<class KEY, class DATA>
typename CRedBlackTree<KEY, DATA>::ERR
CRedBlackTree<KEY, DATA>::ErrFindNearest( const KEY& key, KEY * const pkeyFound, DATA * const pdataFound ) const
{
    ERR err;
    Node* pnode;
    err = m_irbtBase.ErrFindNearest( key, pkeyFound, &pnode );
    if ( pdataFound && ( err == ERR::errSuccess ) )
    {
        *pdataFound = pnode->Data();
    }

    COLLAssert( ERR::errSuccess == err || ERR::errEntryNotFound == err );
    return err;
}

template<class KEY, class DATA>
typename CRedBlackTree<KEY, DATA>::ERR
CRedBlackTree<KEY, DATA>::ErrFindGreaterThanOrEqual( const KEY& key, KEY * const pkeyFound, DATA * const pdataFound ) const
{
    ERR err;
    Node* pnode;
    err = m_irbtBase.ErrFindGreaterThanOrEqual( key, pkeyFound, &pnode );
    if ( pdataFound && ( err == ERR::errSuccess ) )
    {
        *pdataFound = pnode->Data();
    }

    COLLAssert( ERR::errSuccess == err || ERR::errEntryNotFound == err );
    return err;
}

template<class KEY, class DATA>
typename CRedBlackTree<KEY, DATA>::ERR
CRedBlackTree<KEY, DATA>::ErrInsert( const KEY& key, const DATA& data )
{
    Node* pnode = PnodeAlloc_( data );
    if ( pnode == NULL )
    {
        return ERR::errOutOfMemory;
    }

    ERR err = m_irbtBase.ErrInsert( key, pnode );
    COLLAssert( ERR::errSuccess == err || ERR::errOutOfMemory == err || ERR::errDuplicateEntry == err );
    if ( ERR::errSuccess != err )
    {
        Free_( pnode );
    }
    return err;
}

template<class KEY, class DATA>
typename CRedBlackTree<KEY, DATA>::ERR
CRedBlackTree<KEY, DATA>::ErrDelete( const KEY& key )
{
    ERR err;
    Node* pnode = NULL;

    err = m_irbtBase.ErrDelete( key, &pnode );
    if ( err == ERR::errSuccess )
    {
        COLLAssert( pnode != NULL );
        Free_( pnode );
    }

    COLLAssert( ERR::errSuccess == err || ERR::errEntryNotFound == err );
    return err;
}

template<class KEY, class DATA>
void CRedBlackTree<KEY, DATA>::MakeEmpty()
{

    if ( m_irbtBase.PnodeRoot() != NULL )
    {
        Node* pnodeRoot = const_cast<Node*>( m_irbtBase.PnodeRoot() );
        BaseType::InvasiveContext* picRoot = ( BaseType::InvasiveContext* ) ( ( (BYTE*) pnodeRoot ) + Node::OffsetOfRedBlackTreeIC() );

        MakeEmpty_( pnodeRoot->PnodeLeft() );
        MakeEmpty_( pnodeRoot->PnodeRight() );
        picRoot->SetLeft( NULL );
        picRoot->SetRight( NULL );

        m_irbtBase.MakeEmpty();
        Free_( pnodeRoot );
    }

    COLLAssert( FEmpty() );
}

template<class KEY, class DATA>
void CRedBlackTree<KEY, DATA>::MakeEmpty_( Node * const pnode )
{
    if ( pnode )
    {
        MakeEmpty_( pnode->PnodeLeft() );
        MakeEmpty_( pnode->PnodeRight() );
        Free_( pnode );
    }
}

template<class KEY, class DATA>
CRedBlackTreeNode<KEY, DATA> * CRedBlackTree<KEY, DATA>::PnodeAlloc_( const DATA& data )
{
    return new CRedBlackTreeNode<KEY, DATA>( data );
}

template<class KEY, class DATA>
void CRedBlackTree<KEY, DATA>::Free_( Node * const pnode )
{
    delete pnode;
}



#define FIrrInRange( iCurrent, cRoundRobinBufferElements )      ( ( iCurrent >= 0 ) && ( iCurrent < cRoundRobinBufferElements ) )

template<class T>
inline T IrrNext( T iCurrent, const size_t cRoundRobinBufferElements )
{
    COLLAssert( FIrrInRange( iCurrent, (T)cRoundRobinBufferElements ) );
    const T ret = ( iCurrent + 1 ) % cRoundRobinBufferElements;
    COLLAssert( FIrrInRange( ret, (T)cRoundRobinBufferElements ) );
    return ret;
}

template<class T>
inline T IrrPrev( T iCurrent, const size_t cRoundRobinBufferElements )
{
    COLLAssert( FIrrInRange( iCurrent, (T)cRoundRobinBufferElements ) );
    const T ret = ( ( iCurrent == 0 ) ? ( (T)cRoundRobinBufferElements - (T)1 ) : ( iCurrent - (T)1 ) );
    COLLAssert( FIrrInRange( ret, (T)cRoundRobinBufferElements ) );
    return ret;
}

NAMESPACE_END( COLL );

using namespace COLL;

#endif

