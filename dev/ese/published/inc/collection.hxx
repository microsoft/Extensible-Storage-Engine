// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _COLLECTION_HXX_INCLUDED
#define _COLLECTION_HXX_INCLUDED

//  asserts
//
//  #define COLLAssert to point to your favorite assert function per #include

#ifdef COLLAssert
#else  //  !COLLAssert
#define COLLAssert Assert

#endif  //  COLLAssert

#ifdef COLLAssertSz
#else  //  !COLLAssertsz
#define COLLAssertSz AssertSz
#endif  //  COLLAssertSz

#ifdef COLLEnforce
#else
#define COLLEnforce Enforce
#endif

#ifdef COLLAssertTrack
#else
#define COLLAssertTrack( X ) AssertTrack( ( X ), OSFormat( "COLLECTION_LINE:%d", __LINE__ ) )
#endif

// May want to compile it out at some point ... but for now leaving it in, as it is pretty light weight.
#define APPROXIMATE_INDEX_STATS 1

#include "dht.hxx"

#include <memory.h>
#include <minmax.h>

#pragma warning ( disable : 4786 )  //  we allow huge symbol names


typedef SIZE_T ( *PfnOffsetOf )();

NAMESPACE_BEGIN( COLL );

//  CInvasiveConcurrentModSet
//
//  Implements a mathematical Set of pointers to a type.  The Set is optimized for
//  shared-lock insertion/removal.  Enumeration of the members of the Set is single-threaded,
//  requiring that the caller hold an exclusive lock.
//
//  All modifications to the membership of the Set must be done while holding the
//  shared modification lock.  All read access to the Set should be done while holding
//  the enumeration lock.  Especially note that subscript access (i.e. Set[X] ) is not
//  guaranteed to be accurate or even safe unless the enumeration lock is held because the
//  Set is only guaranteed to be enumerable if that lock is held.  Capacity methods such as
//  Size() and Count() may be called with no lock, but are only guaranteed to be accurate
//  if the enumeration lock is held.
//
//  The Set is implemented as either a single pointer variable or an array of pointers.
//  The Set starts out using just the single pointer and switches to using an array the
//  first time a second element is added.   The slots of the array hold either a pointer to an
//  object or an embedded singly linked list of free slots.
//
//  The Set is invasive in that each object in the Set holds space for a CElement. The CElement
//  structure holds the value F(X), where X is the actual index of the object in the array that
//  implements Set.  Therefore, mapping between an object in the Set and the array slot holding
//  the pointer to the object is O(1) in both directions.
//
//  The array elements that don't hold pointers to objects hold a singly linked list where
//  array[X]=F(Y) means the next free slot after array[X] is array[Y].   The Set member variable
//  "Set.m_flhFreeListHead" holds F(Z) where Z is the first free element in the array.
//
//  Insertion into and removal from the Set is therefore O(1), modulo races to
//  modify the free list head.
//
//  The order of elements in the Set is mutable and may change at run-time.
//  When the enumeration lock is taken, the Set is compacted.  That is, the elements of the
//  array that implement the Set are rearranged such that the valid pointers are all at the
//  beginning of the array and the embedded list of free slots all come after the slots holding
//  pointers.  Therefore, when holding the enumeration lock, the Set acts as a list
//  rather than a Set.
//
//  The 0th slot of the array is not used.  This is so an index value of 0 can be used as
//  an "initialized but unused" marker.
//
//  Part of the contract of this data structure is that once an order has been (temporarily)
//  imposed on the Set due to the enumeration lock, the first element of that order will remain
//  the first element after subsequent reorderings until it is removed from the Set.  That is,
//  taking the enumeration lock and compacting the array may reorder any element except the first one.
//
//  Several variables of this implementation are conceptually linked together, but are
//  modified under the shared lock.  Because of this, those member variables can't be assumed
//  to be self-consistent unless the enumeration lock is held.
//    * Contents of the invasive portion of the object, linking the object to the Set.
//    * Contents of the array slot or embedded single pointer, linking the Set to the object.
//    * The member variable m_ulArrayUsed, counting the number of objects in the Set.
//    * The member variable m_flhFreeListHead, referring to the first free array slot.
// 
//  No attempt is made to make this data structure aware of processors.  An instance may slosh
//  around unacceptably.
//
//  CObject         = class representing objects in the array.  Each class must contain
//                    storage for a CElement for embedded list state
//  OffsetOfIAE     = Inline function returning the offset of the CElement contained
//                    in the CObject

// Not a subclass of CZeroInit to avoid introducing a new dependency.
template< class CObject, PfnOffsetOf OffsetOfIAE >
class CInvasiveConcurrentModSet
{
private:
    class CMarkedIndex
    {
        // Holds an index into the array that implements the set.  Uses a ULONG_PTR to
        // so it fits nicely into an ARRAY_VALUE structure (see below), but
        // with the one bit stolen to differentiate between uninitialized and 0.  We
        // use the low bit for this so that an initialized CMarkedIndex can not
        // be a valid pointer to a naturally aligned CObject where CObject is bigger
        // than 1 byte.
        //
        // We use the high bit for a marker set when we don't expect the object to become
        // uninitialized.  This is a debugging facility that triggers asserts, but is not
        // otherwise enforced.
        //
    public:

        //  ctor / dtor

        CMarkedIndex( ) : m_ulpMarkedIndex( 0 ) {}; // Starts out uninitialized.
        ~CMarkedIndex() {}

        // static const for highest bit of member variable to use as a flag.
        static const ULONG_PTR m_ulpProtectedFlag = ( (ULONG_PTR)1 << ( sizeof(ULONG_PTR) * 8 - 1 ) );

        VOID operator = ( const ULONG &ulRealIndex )
        {
            // We can be setting a new value into an uninitialized CMarkedIndex, or we
            // can be overwriting an existing value, so we can't assert on anything.
            m_ulpMarkedIndex = ( ( ulRealIndex << 1 ) | 1 ) & ~m_ulpProtectedFlag;
        };

        operator ULONG() const
        {
            // Reading an uninitialized value silently looks like an initialized 0
            // was stored, which will corrupt the set.
            COLLEnforce( FInitialized() );

            return ( ULONG )( ( ~m_ulpProtectedFlag & m_ulpMarkedIndex ) >> 1 );
        };

        VOID Uninitialize()
        {
            COLLEnforce( FInitialized() );
            COLLAssertTrack( !FProtected() );
            m_ulpMarkedIndex = 0;
        };

        BOOL FInitialized() const { return !!( m_ulpMarkedIndex & 1 ); }

        // Set a bit saying we don't expect an uninitialize call to happen.  Only triggers AssertTrack()s.
        // Note that we're thread-safe here in that we're using atomic set/reset to make
        // sure we get clean control of that bit, but NOT threadsafe in that we're checking
        // FInitialized, FProtected, and the value of m_ulpMarkedIndex in AssertTrack without any sort
        // of lock to protect against simultaneous modifications.
        VOID SetProtected()
        {
            COLLAssertTrack( FInitialized() );
            COLLAssertTrack( !FProtected() );
            AtomicExchangeSet( &m_ulpMarkedIndex, m_ulpProtectedFlag );
            // We don't expect any close races here.
            COLLAssertTrack( FProtected() );
        };

        VOID ResetProtected()
        {
            COLLAssertTrack( FInitialized() );
            COLLAssertTrack( FProtected() );
            AtomicExchangeReset( &m_ulpMarkedIndex, m_ulpProtectedFlag );
            // We don't expect any close races here.
            COLLAssertTrack( !FProtected() );
        };

        BOOL FProtected() const
        {
            COLLAssert( FInitialized() || m_ulpMarkedIndex == 0 );

            return !!( m_ulpMarkedIndex & m_ulpProtectedFlag );
        }

    private:
        ULONG_PTR m_ulpMarkedIndex;
    };

    struct ARRAY_VALUE
    {
        //  The elements that make up the array that instantiates the set.
        //  Each element is either a pointer to a CObject or is an index
        //  into the array making up a linked list of free nodes.

    public:
        ARRAY_VALUE() { m_miNextFree = 0; } // Initialize element to End-Of-List marker.
        ~ARRAY_VALUE() {}

        VOID operator = ( CObject *pObject )
        {
            // In general, we don't store NULL pointers.  We only do so for the 0th slot in
            // the array in a Set, and we set that NULL pointer via the method Uninitialize(), not
            // via simple assignment.
            COLLAssertTrack( nullptr != pObject );

            // We can only ever be overwriting an index in the free list, not a pointer.  Overwriting
            // a pointer would mean we're losing a value from the set.
            COLLAssertTrack( m_miNextFree.FInitialized() );

            m_pObject = pObject;
        }

        VOID operator = ( const ULONG &ulIndex )
        {
            if ( m_miNextFree.FInitialized() )
            {
                // Overwriting an index value with another.  No restrictions on the values.
                // We might be renumbering the free list.
            }
            else
            {
                // Overwriting a pointer with an index, thus removing it from the array.
                // In this implementation, we must always be writing 0 in that case.
                COLLAssertTrack( 0 == ulIndex );
            }
            m_miNextFree = ulIndex;
        }

        operator CObject *() const
        {
            // There isn't an index in the array slot, right?  That would mean we're about
            // to return a bad (and probably very low valued) pointer.
            COLLAssertTrack( FIsObject() );

            // Return the object in this slot.
            return m_pObject;
        }

        operator ULONG() const
        {
            // There isn't an object in the array slot, right?  That would mean we're about
            // to return a bad (and probably very large) index value.
            COLLAssertTrack( !FIsObject() );

            // Return the unwrapped index in this slot.
            return m_miNextFree;
        }

        BOOL FIsObject() const
        {
            // This returns fTrue if we're holding a pointer, even a NULL pointer.  However, we shouldn't
            // be holding a NULL pointer.
            COLLAssert( nullptr != m_pObject );

            return !(m_miNextFree.FInitialized());
        }

        VOID Uninitialize()
        {
            // Only used to set the 0th slot of the array in a Set to a constant NULL pointer.
            m_pObject = nullptr;
        }

        BOOL FInitialized() const
        {
            // This should only be true for the array_value that is the 0th slot of the array in the Set.
            return ( nullptr != m_pObject );
        }

    private:
        //  This union is diffentiated as follows:
        //  * 0 shouldn't happen.  That's either a null pointer or an uninitialized MarkedIndex.
        //  * low bit set means it's an initialized MarkedIndex that's part of the list of free
        //       elements in the array.
        //  * non-zero but low bit not set means it's a pointer to a CObject.  This assumes that
        //       the natural alignment of CObject is at least 2, which is trivially true because
        //       CObject must contain a CElement, which does not have natural alignment of 1.
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
            ULONG ulSequence;       // Monotonically increasing sequence.  Used to solve some race conditions
                                    // that arise while updating the list head.  No problem with wrapping,
                                    // it's used to make ullFreeListHead unique for a brief period of time
                                    // For example, differentiate between (6, 5) and (8, 5) where in both cases
                                    // the index of the first free element is 5, but the change in sequence number means
                                    // that there were operations on the list (presumably by another thread)
                                    // that may render your calculations invalid.
            ULONG ulIndexFirstFree; // Index of first free element in the array.  The value 0 is a special marker
                                    // meaning there are no free elements available.
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
        // The invasive portion of the data structure.  Include one of these in the
        // declaration of the objects you intend to hold in the collection.
        //
        // Privately holds an index back into the array that instantiates the set.
        // CInvasiveConcurrentModSet is a friend so it can directly manipulate that
        // index.
        //
    public:

        //  ctor / dtor

        CElement() {}
        ~CElement() {}

        BOOL FUninitialized() const { return !m_miArrayIndex.FInitialized(); }

        VOID SetProtected()     { m_miArrayIndex.SetProtected(); }
        VOID ResetProtected()   { m_miArrayIndex.ResetProtected(); }
        BOOL FProtected() const { return m_miArrayIndex.FProtected(); }

    private:

        CElement& operator=( CElement& );  //  disallowed

        friend class CInvasiveConcurrentModSet< CObject, OffsetOfIAE >;

        VOID _Uninitialize() { m_miArrayIndex.Uninitialize(); }

        VOID _SetArrayIndex( const ULONG ulArrayIndex )
        {
            BOOL fProtect = !FUninitialized() && FProtected();
            m_miArrayIndex = ulArrayIndex;
            if ( fProtect )
            {
                SetProtected();
            }
        }

        ULONG _GetArrayIndex() { return m_miArrayIndex; }

        CMarkedIndex m_miArrayIndex;
    };

    //  API Error Codes

    enum class ERR
    {
        errSuccess = 0,
        errOutOfMemory
    };

private:
    // Fields ordered for optimal packing.

    // These members are only modified under the enumeration lock.
    union
    {
        ARRAY_VALUE *m_prgValueArray;    // Allocating this only occurs under the enumeration lock.
        CObject *m_pObject;              // No array is allocated when we start, we directly use this.
    };

    ULONG   m_ulArrayAllocated;   // ULONG type implies a limit to the number of elements in array.

    // This is atomically incremented/decremented under the modification lock, but only
    //   truly safe to read under the enumeration lock (since inserting/removing pointers in
    //   the array is not atomic with incrementing/decrementing this member).  We
    //   can assert, however, that it should always be between 0 and m_ulArrayAllocated,
    //   regardless of what (if any) lock is currently held. 
    ULONG  m_ulArrayUsed;

    CReaderWriterLock m_rwl;

    FREE_LIST_HEAD m_flhFreeListHead;

    // The array starts at 16 elements and doubles until it has enough elements to fit 
    // into 4K, then grows by 4K chunks.  The code that grows the array assumes that
    // m_ulArrayPageSize is a power of 2 * m_ulInitialArrayCount.
    static const ULONG m_ulArrayPageSize = 0x1000;     // Size in bytes of chunks allocated for array after initial growth.
    static const ULONG m_ulInitialArrayCount = 0x10;   // Count of elements in first memory allocation.
    static const ULONG m_ulArrayPageCount = ( m_ulArrayPageSize / sizeof( CObject * ) ); // Count of elements in a chunk.

    static_assert( 0 == ( (m_ulArrayPageCount / m_ulInitialArrayCount) & ( ( m_ulArrayPageCount / m_ulInitialArrayCount) - 1 ) ),
                   "Number of elements in a page must be a power of 2 times initial array size." );

    // Static variables to control validation behavior, modified by tests.  Since they are static variables, they
    // affect all instances of the class.    Not implemented as normal dynamic member variables because we want
    // to keep the class small for (indirect) perf reasons.

    // Control how aggressive AssertValid_ is for all instances of the class. Defaults to false.
    static BOOL fDoExtensiveValidation;

    // Control whether we do lock assertions in Count() and Size() methods. Defaults to true.
    static BOOL fAssertOnCountAndSize;

public:
    CInvasiveConcurrentModSet( const char* szRWLName, const INT iRWLRank );
    ~CInvasiveConcurrentModSet( );

    // Return the number of elements in use in the array.
    // Count() is not guaranteed to be accurate unless locked for enumeration.
    // Updating count and inserting/removing the array element are not atomic.
    // Remember that if we're not using the embedded pointer, we set the 0th
    // element of the array to a constant nullptr, so don't include it in the count.
    INT Count()        { COLLAssert( !fAssertOnCountAndSize || FLockedForEnumeration() ); return ( FEmbeddedSinglePointer_() ? m_ulArrayUsed : (m_ulArrayUsed - 1) ); }; 

    // Return the number of elements allocated for the array.
    // Size() is not guaranteed to be accurate unless locked.  The memory allocations are done with
    // the enumeration lock.
    INT Size() { COLLAssert( !fAssertOnCountAndSize || FLockedForModification() || FLockedForEnumeration() ); return ( FEmbeddedSinglePointer_() ? 0 : m_ulArrayAllocated ); };

    VOID LockForModification();      // Takes a shared lock.
    VOID UnlockForModification();
#ifdef DEBUG
    BOOL FLockedForModification();
    BOOL FNotLockedForModification();
#endif

    VOID LockForEnumeration();       // Takes a non-shared lock.
    VOID UnlockForEnumeration();
#ifdef DEBUG
    BOOL FLockedForEnumeration();
    BOOL FNotLockedForEnumeration();
#endif

    // These require the modification lock.
    ERR ErrInsert( CObject *pObject );      
    ERR ErrInsertReserveSpace();
    VOID Remove( CObject *pObject );

    // This requires the enumeration lock.
    CObject* operator[] ( ULONG ulArrayIndex );

#ifdef DEBUGGER_EXTENSION
    VOID Dump( CPRINTF * pcprintf, const DWORD_PTR dwOffset = 0 ) const;
#endif

    // Enable/disable the test-centric behaviors controlled by static variables.
    VOID EnableExtensiveValidation() { fDoExtensiveValidation = fTrue; }
    VOID DisableExtensiveValidation() { fDoExtensiveValidation = fFalse; }
    VOID EnableAssertOnCountAndSize() { fAssertOnCountandSize = fTrue; }
    VOID DisableAssertOnCountAndSize() { fAssertOnCountAndSize = fFalse; }

private:
    // Compacts the array to move all non-NULL pointers to the beginning of the array.
    // This requires the enumeration lock be held.    
    VOID Compact_();

    // If the array is full, adds more slots to the array.
    // This requires the enumeration lock be held.    
    ERR Grow_();

    // True if we are just storing one pointer directly in the structure.
    // False if we are using an allocated array.
    BOOL FEmbeddedSinglePointer_();

    // Renumber all the marked indices in the embedded free list.
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
    // No array is actually allocated.  We directly store 1 slot in the class itself.
    // We indicate this by setting m_ulArrayAllocated = 1;
    m_ulArrayAllocated = 1;

    // The free list head has to be 64 bits for our AtomicCompareExchange calls to work.
    static_assert( sizeof( m_flhFreeListHead ) == sizeof( unsigned __int64 ), "Expected" );
    static_assert( sizeof( m_flhFreeListHead ) == sizeof( m_flhFreeListHead.ullFreeListHead ), "Expected" );
}

template< class CObject, PfnOffsetOf OffsetOfIAE >
inline CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::~CInvasiveConcurrentModSet()
{
    COLLAssert( FNotLockedForEnumeration() );
    COLLAssert( FNotLockedForModification() );
    // We expect to have single-threaded access while destructing, which for validation
    // is equivalent to holding the enumeration lock.  We expect only well-behaved clients that
    // only destruct when they're truly done with the set; we don't ref-count or guard against
    // that sort of race in any way.
    AssertValid_( VALIDATION_LEVEL::vlEnumerationLocked );

    // We need to reset the invasive portion of the list in all the contained elements.
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

    //
    // All validations are only accurate if we're locked.
    // Some validations are only accurate if we're enum locked.
    // Some validations are only accurate if we're enum locked and compact.
    //
    // FEmbeddedSinglePointer_() does some validation assertions also, so make sure we
    // call it.
    //
    BOOL fEmbedded = FEmbeddedSinglePointer_();

    // This variable can change under the modification lock, but regardless, this should
    // always be true.
    COLLAssert( m_flhFreeListHead.ulIndexFirstFree < m_ulArrayAllocated );

    switch ( vlLevel )
    {
        case VALIDATION_LEVEL::vlModificationLocked:
            // We're only locked for modification, so we can't examine values that can
            // change under that lock.  That includes:
            //     * the element in the embedded slot.
            //     * the elements in the array other than the constant NULL in the 0th element.
            //     * m_ulArrayUsed
            //
            // Nothing we can safely assert that we didn't already assert in FEmbeddedSinglePointer_()
            break;

        case VALIDATION_LEVEL::vlEnumerationLocked:
        case VALIDATION_LEVEL::vlEnumerationLockedAndCompact:
            // Single threaded access.  That means we can examine any part of the data structure.
            // Don't Assert we're locked; we can get called from the destructor to validate as if
            // we're enumeration locked, even if we're not.

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

                // With at most a single element, the array is by definition compact.
            }
            else
            {
                ULONG ulUsed = 0;
                ULONG ulFree = 0;

                // Remember, the 0th element is a constant nullptr and is counted in m_ulArrayUsed.
                ulUsed++;

                // First, spin through the array:
                //   * count used vs. free.
                //   * verify the invasive data for used slots.
                //   * verify that the indices in free slots are in bounds.
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

                // Now, walk the embedded free list. 
                ULONG ulArrayIndex = m_flhFreeListHead.ulIndexFirstFree;
                ULONG ulWalked = 0;
                while ( ulArrayIndex != 0 ) {
                    if ( vlLevel == VALIDATION_LEVEL::vlEnumerationLockedAndCompact )
                    {
                        // Caller expects the array to be compacted.  If so, all free slot indices must
                        // be greater than or equal to the number of used slots.
                        COLLAssert( ulArrayIndex >= m_ulArrayUsed );
                    }
                    COLLAssert( !m_prgValueArray[ ulArrayIndex ].FIsObject() );
                    ulArrayIndex = m_prgValueArray[ ulArrayIndex];
                    ulWalked++;
                    if ( ulWalked > ulFree )
                    {
                        // There must be a loop in the free list.
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
    // True if we are just storing one pointer directly in the structure.
    // False if we are using an allocated array.

    // Since m_ulArrayAllocated is the decisive thing to check to see if it's an embedded single
    // pointer, take this opportunity to make some assertions about that value.
    //
    // m_ulArrayAllocated only changes under the enumeration lock (i.e. single threaded)

    switch ( m_ulArrayAllocated ) {
        case 0:
            // This should never happen.  It's set to 1 in the constructor, commonly incremented under
            // the enumeration lock, and very rarely decremented under the same lock, but never
            // decremented to less than m_ulArrayPageCount.
            COLLEnforce( fFalse );
            break;

        case 1:
            // Simple un-raced marker that we're using the embedded slot.
            COLLAssertTrack( 0 == m_flhFreeListHead.ulIndexFirstFree );
            // Remeber that the only valid thing to do with m_ulArrayUsed outside of an enumeration lock
            // is AtomicIncrement or AtomicDecrement; it can in theory have almost any value outside of
            // that lock due to shared multi-threaded access.
            return fTrue;

        case 2:
            // This is rarely a valid value.  It's set to 2 when we're growing out of the embedded
            // slot into an array, and the embedded slot has a value in it.  This is only done
            // under the enumeration lock, and we never call this routine during the time it has
            // this value.  So, if we're here, something truly unexpected has happened.
            COLLEnforce( fFalse );
            break;

        default:
            // Not an embedded single pointer.  There must be an array and the 0th element must
            // be used and must store a NULL (we only use the index 0 in the EmbeddedSinglePointer case).
            COLLAssertTrack( nullptr != m_prgValueArray );
            COLLAssertTrack( !(m_prgValueArray[ 0 ].FInitialized()) );
            // Normally not OK to read m_ulArrayUsed outside of an enumeration lock, but it should
            // never be 0, even outside of that lock, because of the uninitialized 0th slot.
            COLLAssertTrack( 0 < m_ulArrayUsed );
            break;
    }


    return fFalse;
}

template < class CObject, PfnOffsetOf OffsetOfIAE >
inline VOID CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::RenumberFreeList_()
{
    // We expect to be called while holding the EnumerationLock.
    COLLAssert( FLockedForEnumeration() );
    COLLAssert( FNotLockedForModification() );

    COLLAssertTrack( !FEmbeddedSinglePointer_() );

    // Reset the head of the free list to be the first element we're going to use.
    // Do that now before we AssertValid_() because its state right now is untrustworthy
    // and frankly uninteresting.  We KNOW the free list may not set up correctly yet,
    // that's why we're here.  For example, if we just shrunk the array, it may even
    // refer to a value outside of the current max.
    m_flhFreeListHead.ulIndexFirstFree = m_ulArrayUsed;
    m_flhFreeListHead.ulSequence += 1;

    // VALIDATION_LEVEL::vlEnumerationLocked and vlEnumerationLockedAndCompact imply the free list
    // is accurate.  We know it's not.  Just use the much weaker vlModificationLocked level.  We'll
    // AssertValid_ at the stronger level later.
    AssertValid_( VALIDATION_LEVEL::vlModificationLocked );

    ULONG ulArrayIndex;

    for ( ulArrayIndex = m_ulArrayUsed; ulArrayIndex < ( m_ulArrayAllocated - 1 ) ; ulArrayIndex++ )
    {
        m_prgValueArray[ ulArrayIndex ] = ( ulArrayIndex + 1 );
    }

    // End marker.
    m_prgValueArray[ ulArrayIndex ] = (ULONG)0;
}

template < class CObject, PfnOffsetOf OffsetOfIAE >
inline typename CInvasiveConcurrentModSet< CObject, OffsetOfIAE>::ERR CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::Grow_()
{
    // We expect to be called while holding the EnumerationLock.
    COLLAssert( FLockedForEnumeration() );
    COLLAssert( FNotLockedForModification() );

    if ( !FEmbeddedSinglePointer_() && ( m_ulArrayAllocated > m_ulArrayUsed ) )
    {
        // Presumably someone else already grew the array.  It's big enough.
        // Note that if we're using the embedded slot, we're going to
        // grow the array regardless.
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
        // Double until you have more than fit on a page, then grow by pages.
        if ( ( m_ulArrayAllocated * 2 ) <= m_ulArrayPageCount )
        {
            ulArrayAllocated = m_ulArrayAllocated * 2;
        }
        else
        {
            ulArrayAllocated = m_ulArrayAllocated + m_ulArrayPageCount;
        }
    }

    // We can't let the size of the array grow enough that a CMarkedArray can't
    // accurately store it.
    {
        CMarkedIndex miTemp;
        miTemp = ( ulArrayAllocated - 1 );
        if ( miTemp != ( ulArrayAllocated - 1 ) )
        {
            // Overflow happened.
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
        // We use index 0 only for the embedded single pointer case, we don't use element 0
        // in the array case in order to have a known invalid value.  Therefore,
        // we have to bump any embedded single pointer to the 1st slot, and update
        // the bookkeeping to show that.

        // Set the 0th slot in the array to be uninitialized.
        prgValueArray[ 0 ].Uninitialize();

        if ( nullptr == m_pObject )
        {
            // Growing, but currently not holding a pointer.  Valid in some race conditions.
            COLLAssertTrack( 0 == m_ulArrayUsed );
            m_ulArrayUsed = 1;
        }
        else
        {
            COLLAssertTrack( 1 == m_ulArrayUsed );

            // Move the object in the embedded 0th slot to the first slot.

            m_ulArrayAllocated = 2; // This is not really true but it's harmless right now
                                    // since we're single-threaded to grow the array.  If
                                    // we don't do this, though, we can occasionally trigger            
                                    // asserts in places that, outside of the lock, assert
                                    // that arrayUsed is always less than or equal to
                                    // arrayAllocated.  It'll only have this value for a
                                    // few more lines.
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
    // We need to be single threaded here.
    COLLAssert( FLockedForEnumeration() );
    COLLAssert( FNotLockedForModification() );

    if ( FEmbeddedSinglePointer_() )
    {
        // No array to compact.
        return;
    }

    if ( m_ulArrayAllocated == m_ulArrayUsed )
    {
        // Completely full array, nothing to compact.
        return;
    }


    ULONG iTail = m_ulArrayAllocated - 1;
    ULONG iHead = 1; // Remember to skip the unused nullptr at array[0].

    // The one constraint we have about moving values around is that if
    // m_prgValueArray[1] is not a free slot before this loop, then m_prgValueArray[1]
    // doesn't get moved from the 1st slot by compacting.

    // We definitely have at least one free slot.
    for ( ; ; )
    {
        // Move iHead forward to the next free slot.
        for ( ; ( iHead < iTail ) && ( m_prgValueArray[ iHead ].FIsObject() ) ; iHead++ );

        // Move iTail backward to the next used slot.
        for ( ; ( iTail > iHead ) && ( !m_prgValueArray[ iTail ].FIsObject() ); iTail-- );

        if ( iHead == iTail )
        {
            break;
        }

        // The used slot has correct bookkeping before the move, right?
        EnforceValidIndex_( iTail );

        // iHead is positioned on a free slot.  iTail is on a used slot. Put the value
        // from iTail into iHead, then mark iTail as no longer in use.
        m_prgValueArray[ iHead ] = m_prgValueArray[ iTail ];
        _PiaeFromPobj( m_prgValueArray[ iHead ] )->_SetArrayIndex( iHead );
        m_prgValueArray[ iTail ] = (ULONG)0;

        // The used slot has correct bookkeping after the move, right?
        EnforceValidIndex_( iHead );

        if ( iTail == m_ulArrayUsed )
        {
            break;
        }
    }

    //
    // Now see if we can release some memory by shrinking the array.
    // 
    if ( m_ulArrayAllocated > m_ulArrayPageCount )
    {
        // Not already minimally sized.
        ULONG ulArrayAllocated = 0;

        if ( m_ulArrayUsed == 1 )
        {
            // The array is actually empty.  Shrink to minimal size.
            ulArrayAllocated = m_ulArrayPageCount;
        }
        else if ( ( 3 * ( INT )m_ulArrayUsed ) < ( INT )m_ulArrayAllocated )
        {
            // We're using less than 1/3 of it.  Realloc to 1/2 of allocated
            // space, rounded up to be a multiple of m_ulArrayPageCount
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
            // else
            //    We can't allocate space to shrink.  Unfortunate.  Ignore it, just don't shrink.
        }
    }

    // Re-initialize the free list; it's all the slots at the end of the array.
    RenumberFreeList_();
}


template < class CObject, PfnOffsetOf OffsetOfIAE >
inline VOID CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::LockForModification()
{
    // Take a shared lock for modification.  We carefully use Atomic*() calls to
    // modify the array, but the array is not safe for normal enumeration while this
    // lock is held.  Enumeration requires the array be compacted, and you can
    // remove elements under this lock, leaving interspersed free slots.
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

    // Take a non-shared lock for enumeration.  Nothing can be inserted or removed from the
    // array while this lock is held.
    //
    // SOMEONE: CONSIDER: techically, we should be able to share this latch among Enumerators.
    //   However, this way is easy to implement and the behavior of the code this
    //   template replaced was a non-shared latch for all access, so we're no worse
    //   off than we were.

    m_rwl.EnterAsWriter();
    AssertValid_( VALIDATION_LEVEL::vlEnumerationLocked );

    // Compact the array, pushing any unused slots to the end.  Due to Remove(),
    // the array can have embedded unused slots.
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

    //  Pointer should NOT be be a valid CMarkedIndex ... OR disaster!    
    COLLEnforce( !( ( reinterpret_cast<CMarkedIndex *>( &pObject ) )->FInitialized() ) );

    //  Object should not already be in an array.
    COLLAssertTrack( _PiaeFromPobj( pObject )->FUninitialized() );
    
    // Since this is supposed to be inserting a new item, the item shouldn't already be protected.
    COLLAssert( !( _PiaeFromPobj( pObject )->FProtected() ) );
    
    // We expect well-behaved clients to never have multiple threads simultaneously manipulating the set membership of
    // a given object.  We do not guard against this, but a client that does so will eventually (and probably
    // sooner rather than later) get unpredictable failures.

    // Loop forever until we manage to insert.
    for ( ; ; )
    {
        if ( FEmbeddedSinglePointer_() ) {
            CObject *pInitial = ( CObject * )AtomicCompareExchangePointer( ( PVOID * )&m_pObject, nullptr, pObject );
            if ( nullptr == pInitial ) {
                // Successfully replaced a nullptr with the value to insert.
                // RACE_POTENTIAL:
                //   * m_ulArrayUsed is out of sync; it still needs to be incremented.  This window is why it's not
                //     safe to read m_ulArrayUsed without holding the enumeration lock.
                //   * The object is in the set, but is not yet marked as such in the invasive portion of the object.
                //     This window is part of why it's not safe to have multiple threads simultaneously manipulating
                //     the set membership of a given object on multiple threads.
                AtomicIncrement( &m_ulArrayUsed );
                _PiaeFromPobj( pObject )->_SetArrayIndex( 0 ); // 0 is a marker that we're in the single embedded pointer.
                EnforceValidIndex_( 0 );
                return ERR::errSuccess;
            }
        }
        else
        {
            // Now, in a threadsafe way with contention on pHead, do the equivalent of:
            //   pFreeSlot = pHead;
            //   pHead = pHead->pNext;
            //
            // Loop until we notice the array is out of free slots or we manage to insert.
            OSSYNC_FOREVER
            {
                // Safely read the head of the free list.
                const FREE_LIST_HEAD flhOriginalHead = AtomicRead( &m_flhFreeListHead.ullFreeListHead ) ;
                if ( 0 == flhOriginalHead.ulIndexFirstFree )
                {
                    // We think we're out of slots.  Break out of this loop and grow the array.
                    break;
                }

                // The free list head actually refers to an element in the array, right?
                COLLEnforce( flhOriginalHead.ulIndexFirstFree < m_ulArrayAllocated );

                // Retrieve next available slot from the array.  If no-one else interferes,
                // val contains the next free index.
                const ARRAY_VALUE val = m_prgValueArray[ flhOriginalHead.ulIndexFirstFree ];

                if ( val.FIsObject() ) 
                {
                    // Free list head we read didn't point to a free element when we checked.  That
                    // can happen in races, but in that case the head we read had better not still
                    // be the same head that's in place.
                    COLLAssert( flhOriginalHead != AtomicRead( &m_flhFreeListHead.ullFreeListHead ) );
                    continue;
                }

                // Attempt to grab the free element by doing the equivalent of
                //    pHead = pHead->pNext;
                // This will only succeed if pHead hasn't changed since we read it.  If it succeeds, we
                // have sole ownership of *pHead (i.e. the first currently available slot).
                //
                // The point of ulSequence is to guarantee that not only is the next free index value the same
                // as when we read it, it hasn't been modified by anyone since we read it.  Not this extra check
                // avoids some cases where we could orphan free elements.
                const FREE_LIST_HEAD flhNewHead = { val, ( flhOriginalHead.ulSequence + 1 ) };
                const FREE_LIST_HEAD flhCurrentHead = AtomicCompareExchange(
                    &m_flhFreeListHead.ullFreeListHead,
                    flhOriginalHead.ullFreeListHead,
                    flhNewHead.ullFreeListHead );

                if ( flhCurrentHead == flhOriginalHead )
                {
                    // We succeeded, the free list head has been updated and we now own the slot; it's no longer in the
                    // free list.
                    m_prgValueArray[ flhOriginalHead.ulIndexFirstFree ] = pObject;
                    // RACE_POTENTIAL:
                    //   * m_ulArrayUsed is out of sync with actual use; it still needs to be incremented.  This window
                    //     is why it's not safe to read m_ulArrayUsed without holding the enumeration lock.
                    //   * The object is in the set, but is not yet marked as such in the invasive portion of the object.
                    //     This window is part of why it's not safe to have multiple threads simultaneously manipulating
                    //     the set membership of a given object on multiple threads.
                    AtomicIncrement( &m_ulArrayUsed );
                    _PiaeFromPobj( pObject )->_SetArrayIndex( flhOriginalHead.ulIndexFirstFree ); 
                    EnforceValidIndex_( flhOriginalHead.ulIndexFirstFree );
                    return ERR::errSuccess;
                }
            }
        }

        // We failed to insert (we would already have returned if we had succeeded).  We must need to grow the array.
        // Note that growing is the only error-generating path here.
        UnlockForModification();
        LockForEnumeration();
        ERR err = Grow_();
        UnlockForEnumeration();
        LockForModification();
        if ( ERR::errSuccess != err )
        {
            //  Object should still not be in an array.
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

    // Assuming single threaded access (even though we're only modification locked), make sure the set
    // has a slot available for insertion of an object.

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

    // We failed to find space for insertion.  We must need to grow the list.
    // Note that growing is the only error-generating path here.
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

    // We don't tolerate being called to remove an object that is not in the Set, although we don't
    // explicitly guard against that; we will fail (or appear to succeed) non-deterministically if
    // we aren't in the set.
    // We DO enforce that several pieces of data are consistent, given that assumption.

    // Object must be in a Set.
    COLLEnforce( !(_PiaeFromPobj( pObject )->FUninitialized() ) );

    ULONG ulArrayIndex = _PiaeFromPobj( pObject )->_GetArrayIndex();

    COLLAssertTrack( ulArrayIndex < m_ulArrayAllocated );

    // RACE_POTENTIAL:
    //   The full bookkeeping data has three parts.
    //     = The pointer to pObject, stored in the Set.
    //     = The index in the Set of the pointer to pObject, stored in pObject.
    //     = The number of elements used in the Set.
    //
    //   We're setting them under the shared modification lock, so there is a window where they don't agree
    //   and other threads could see it.  This window is part of why it's not safe to have multiple threads
    //   simultaneously manipulating the Set membership of a given object on multiple threads.

    COLLEnforce( ulArrayIndex < m_ulArrayAllocated );

    if ( FEmbeddedSinglePointer_() ) {
        // I should be at the slot I was given
        COLLEnforce( 0 == ulArrayIndex );
        COLLEnforce( m_pObject == pObject );
        EnforceValidIndex_( 0 );

        // Remove the pointer to the object stored in the Set.
        m_pObject = nullptr;
    }
    else
    {
        // I should be at the slot I was given
        COLLEnforce( 0 != ulArrayIndex );
        COLLEnforce( m_prgValueArray[ ulArrayIndex ] == pObject );
        EnforceValidIndex_( ulArrayIndex );

        // Remove the pointer to the object stored in the Set in a way that prepares the slot to be in the free list.
        m_prgValueArray[ ulArrayIndex ] = (ULONG)0;

        // Link this array slot in to the free list.
        OSSYNC_FOREVER
        {
            // Safely get the current free list head.
            const FREE_LIST_HEAD flhOriginalHead = AtomicRead( &m_flhFreeListHead.ullFreeListHead );

            // Place the current free list head into the array slot.
            m_prgValueArray[ ulArrayIndex ] = flhOriginalHead.ulIndexFirstFree;

            // Calculate a value for the free list head that points to the array slot.
            const FREE_LIST_HEAD flhNewHead = { ulArrayIndex, ( flhOriginalHead.ulSequence + 1 ) };

            // Try to replace the free list head.  This only suceeds if the free list head hasn't changed
            // since we read it.
            const FREE_LIST_HEAD flhCurrentHead = AtomicCompareExchange(
                &m_flhFreeListHead.ullFreeListHead,
                flhOriginalHead.ullFreeListHead,
                flhNewHead.ullFreeListHead );

            if ( flhCurrentHead == flhOriginalHead ) {
                // We succeeded in making the head of the free list refer to us.  We're done.
                break;
            }

            // We didn't succeed in making the head of the free list refer to us.  Try again.
        }
    }

    // Unset the index of where the pointer to pObject used to be.
    _PiaeFromPobj( pObject )->_Uninitialize();

    // And finally note that we've freed up a slot.
    AtomicDecrement( &m_ulArrayUsed );
}


template < class CObject, PfnOffsetOf OffsetOfIAE >
inline CObject* CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::operator[] ( ULONG ulArrayIndex )
{
    COLLAssert( FLockedForEnumeration() );
    AssertValid_( VALIDATION_LEVEL::vlEnumerationLockedAndCompact );

    // Part of the contract is you can ask for array[X] where X is greater than
    // the number of used slots, and get back a nullptr in that case.  This
    // allows the idiom of reading array[0] in a loop until it returns nullptr.
    //
    // Although we handle it, we really don't expect people to ask for array[X]
    // where X is beyond the bounds of the array unless X is 0 (1 in 1-indexed).

    if ( FEmbeddedSinglePointer_() )
    {
        // If we're in this state, there is no reason to be asking for anything
        // but the 0th element.
        COLLAssert( 0 == ulArrayIndex );

        switch ( ulArrayIndex )
        {
        case 0:
            // m_pObject holds nullptr if the list is empty.
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
        // Remember, the 0th element is a constant nullptr and is counted in m_ulArrayUsed.
        // Adjust ulArrayIndex to be 1-indexed.
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

//  converts a pointer to an object to a pointer to the IAE

template< class CObject, PfnOffsetOf OffsetOfIAE >
inline typename CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::CElement* CInvasiveConcurrentModSet< CObject, OffsetOfIAE >::
_PiaeFromPobj( const CObject* const pobj ) const
{
    return ( CElement* )( ( BYTE* )pobj + OffsetOfIAE() );
}

//////////////////////////////////////////////////////////////////////////////////////////
//  CInvasiveList
//
//  Implements an "invasive" doubly linked list of objects.  The list is "invasive"
//  because part of its state is embedded directly in the objects it contains.  An
//  additional property of this list class is that the head of the list can be relocated
//  without updating the state of any of the contained objects.
//
//  CObject         = class representing objects in the list.  each class must contain
//                    storage for a CElement for embedded list state
//  OffsetOfILE     = inline function returning the offset of the CElement contained
//                    in the CObject

template< class CObject, PfnOffsetOf OffsetOfILE >
class CInvasiveList
{
    public:

        //  invasive list element state (embedded in linked objects)

        class CElement
        {
            public:

                //  ctor / dtor

                CElement() : m_pilePrev( (CElement*)-1 ), m_pileNext( (CElement*)-1 ) {}
                ~CElement() {}

                BOOL FUninitialized() const { return ( ( m_pilePrev == (CElement*)-1 ) && ( m_pileNext == (CElement*)-1 ) ); }

            private:

                CElement& operator=( CElement& );  //  disallowed

                friend class CInvasiveList< CObject, OffsetOfILE >;

                CElement*   m_pilePrev;
                CElement*   m_pileNext;
        };

    public:

        //  ctor / dtor

        CInvasiveList();
        ~CInvasiveList();

        //  operators

        CInvasiveList& operator=( const CInvasiveList& il );

        //  API

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

        //  internal functions

        CObject* _PobjFromPile( CElement* const pile ) const;
        CElement* _PileFromPobj( const CObject* const pobj ) const;

    private:

        CElement*   m_pilePrevMost;
        CElement*   m_pileNextMost;
};

//  ctor

template< class CObject, PfnOffsetOf OffsetOfILE >
inline CInvasiveList< CObject, OffsetOfILE >::
CInvasiveList()
{
    //  start with an empty list

    Empty();
}

//  dtor

template< class CObject, PfnOffsetOf OffsetOfILE >
inline CInvasiveList< CObject, OffsetOfILE >::
~CInvasiveList()
{
}

//  assignment operator

template< class CObject, PfnOffsetOf OffsetOfILE >
inline CInvasiveList< CObject, OffsetOfILE >& CInvasiveList< CObject, OffsetOfILE >::
operator=( const CInvasiveList& il )
{
    m_pilePrevMost  = il.m_pilePrevMost;
    m_pileNextMost  = il.m_pileNextMost;
    return *this;
}

//  returns fTrue if the list is empty

template< class CObject, PfnOffsetOf OffsetOfILE >
inline BOOL CInvasiveList< CObject, OffsetOfILE >::
FEmpty() const
{
    return m_pilePrevMost == _PileFromPobj( NULL );
}

//  returns fTrue if the specified object is a member of this list
//
//  NOTE:  this function currently returns fTrue if the specified object is a
//  member of any list!

template< class CObject, PfnOffsetOf OffsetOfILE >
inline BOOL CInvasiveList< CObject, OffsetOfILE >::
FMember( const CObject* const pobj ) const
{
#ifdef EXPENSIVE_DEBUG

    for ( CObject* pobjT = PrevMost(); pobjT && pobjT != pobj; pobjT = Next( pobjT ) )
    {
    }

    return pobjT == pobj;

#else  //  !DEBUG

    return _PileFromPobj( pobj )->m_pilePrev != (CElement*)-1;

#endif  //  DEBUG
}

//  returns the prev object to the given object in the list

template< class CObject, PfnOffsetOf OffsetOfILE >
inline CObject* CInvasiveList< CObject, OffsetOfILE >::
Prev( CObject* const pobj ) const
{
    return _PobjFromPile( _PileFromPobj( pobj )->m_pilePrev );
}

//  returns the next object to the given object in the list

template< class CObject, PfnOffsetOf OffsetOfILE >
inline CObject* CInvasiveList< CObject, OffsetOfILE >::
Next( CObject* const pobj ) const
{
    return _PobjFromPile( _PileFromPobj( pobj )->m_pileNext );
}

//  returns the prev-most object to the given object in the list

template< class CObject, PfnOffsetOf OffsetOfILE >
inline CObject* CInvasiveList< CObject, OffsetOfILE >::
PrevMost() const
{
    return _PobjFromPile( m_pilePrevMost );
}

//  returns the next-most object to the given object in the list

template< class CObject, PfnOffsetOf OffsetOfILE >
inline CObject* CInvasiveList< CObject, OffsetOfILE >::
NextMost() const
{
    return _PobjFromPile( m_pileNextMost );
}

//  inserts the given object as the prev-most object in the list

template< class CObject, PfnOffsetOf OffsetOfILE >
inline void CInvasiveList< CObject, OffsetOfILE >::
InsertAsPrevMost( CObject* const pobj )
{
    CElement* const pile = _PileFromPobj( pobj );

    //  this object had better not already be in the list

    COLLAssert( !FMember( pobj ) );

    //  this object had better not already be in any list

    COLLAssert( pile->m_pilePrev == (CElement*)-1 );
    COLLAssert( pile->m_pileNext == (CElement*)-1 );

    //  the list is empty

    if ( m_pilePrevMost == _PileFromPobj( NULL ) )
    {
        //  insert this element as the only element in the list

        pile->m_pilePrev            = _PileFromPobj( NULL );
        pile->m_pileNext            = _PileFromPobj( NULL );

        m_pilePrevMost              = pile;
        m_pileNextMost              = pile;
    }

    //  the list is not empty

    else
    {
        //  insert this element at the prev-most position in the list

        pile->m_pilePrev            = _PileFromPobj( NULL );
        pile->m_pileNext            = m_pilePrevMost;

        m_pilePrevMost->m_pilePrev  = pile;

        m_pilePrevMost              = pile;
    }
}

//  inserts the given object as the next-most object in the list

template< class CObject, PfnOffsetOf OffsetOfILE >
inline void CInvasiveList< CObject, OffsetOfILE >::
InsertAsNextMost( CObject* const pobj )
{
    CElement* const pile = _PileFromPobj( pobj );

    //  this object had better not already be in the list

    COLLAssert( !FMember( pobj ) );

    //  this object had better not already be in any list

    COLLAssert( pile->m_pilePrev == (CElement*)-1 );
    COLLAssert( pile->m_pileNext == (CElement*)-1 );

    //  the list is empty

    if ( m_pileNextMost == _PileFromPobj( NULL ) )
    {
        //  insert this element as the only element in the list

        pile->m_pilePrev            = _PileFromPobj( NULL );
        pile->m_pileNext            = _PileFromPobj( NULL );

        m_pilePrevMost              = pile;
        m_pileNextMost              = pile;
    }

    //  the list is not empty

    else
    {
        //  insert this element at the next-most position in the list

        pile->m_pilePrev            = m_pileNextMost;
        pile->m_pileNext            = _PileFromPobj( NULL );

        m_pileNextMost->m_pileNext  = pile;

        m_pileNextMost              = pile;
    }
}

//  inserts the given object into the list, before pobjNext
//  a NULL pobjNext will insert the object as NextMost

template< class CObject, PfnOffsetOf OffsetOfILE >
inline void CInvasiveList< CObject, OffsetOfILE >::
Insert( CObject* const pobj, CObject* const pobjNext )
{
    CElement* const pileNext = _PileFromPobj( pobjNext );

    //  the next object had better already be in the list or the NULL object (insert as NextMost)

    COLLAssert( ( pileNext == _PileFromPobj( NULL ) ) || FMember( pobjNext ) );

    //  insert as NextMost

    if ( pileNext == _PileFromPobj( NULL ) )
    {
        InsertAsNextMost( pobj );
    }
    else
    {
        //  insert as PrevMost

        if ( pileNext == m_pilePrevMost )
        {
            InsertAsPrevMost( pobj );
        }
        else
        {
            //  insert in the middle

            CElement* const pile = _PileFromPobj( pobj );

            //  this object had better not already be in the list

            COLLAssert( !FMember( pobj ) );

            //  this object had better not already be in any list

            COLLAssert( pile->m_pilePrev == (CElement*)-1 );
            COLLAssert( pile->m_pileNext == (CElement*)-1 );

            //  prev

            pile->m_pilePrev = pileNext->m_pilePrev;
            pile->m_pilePrev->m_pileNext = pile;

            //  next

            pile->m_pileNext = pileNext;
            pileNext->m_pilePrev = pile;
        }
    }
}

//  removes the given object from the list

template< class CObject, PfnOffsetOf OffsetOfILE >
inline void CInvasiveList< CObject, OffsetOfILE >::
Remove( CObject* const pobj )
{
    CElement* const pile = _PileFromPobj( pobj );

    //  this object had better already be in the list

    COLLAssert( FMember( pobj ) );

    //  there is an element after us in the list

    if ( pile->m_pileNext != _PileFromPobj( NULL ) )
    {
        //  fix up its prev element to be our prev element (if any)

        pile->m_pileNext->m_pilePrev    = pile->m_pilePrev;
    }
    else
    {
        //  set the next-most element to be our prev element (if any)

        m_pileNextMost                  = pile->m_pilePrev;
    }

    //  there is an element before us in the list

    if ( pile->m_pilePrev != _PileFromPobj( NULL ) )
    {
        //  fix up its next element to be our next element (if any)

        pile->m_pilePrev->m_pileNext    = pile->m_pileNext;
    }
    else
    {
        //  set the prev-most element to be our next element (if any)

        m_pilePrevMost                  = pile->m_pileNext;
    }

    //  mark ourself as not in any list

    pile->m_pilePrev                    = (CElement*)-1;
    pile->m_pileNext                    = (CElement*)-1;
}

//  resets the list to the empty state

template< class CObject, PfnOffsetOf OffsetOfILE >
inline void CInvasiveList< CObject, OffsetOfILE >::
Empty()
{
    m_pilePrevMost  = _PileFromPobj( NULL );
    m_pileNextMost  = _PileFromPobj( NULL );
}

//  converts a pointer to an ILE to a pointer to the object

template< class CObject, PfnOffsetOf OffsetOfILE >
inline CObject* CInvasiveList< CObject, OffsetOfILE >::
_PobjFromPile( CElement* const pile ) const
{
    return (CObject*)( (BYTE*)pile - OffsetOfILE() );
}

//  converts a pointer to an object to a pointer to the ILE

template< class CObject, PfnOffsetOf OffsetOfILE >
inline typename CInvasiveList< CObject, OffsetOfILE >::CElement* CInvasiveList< CObject, OffsetOfILE >::
_PileFromPobj( const CObject* const pobj ) const
{
    return (CElement*)( (BYTE*)pobj + OffsetOfILE() );
}

//////////////////////////////////////////////////////////////////////////////////////////
//  CCountedInvasiveList
//
//  Implements a version of CInvasiveList that also maintains an element count.
//
//  CObject         = class representing objects in the list.  each class must contain
//                    storage for a CElement for embedded list state
//  OffsetOfILE     = inline function returning the offset of the CElement contained
//                    in the CObject

template< class CObject, PfnOffsetOf OffsetOfILE >
class CCountedInvasiveList : public CInvasiveList<CObject, OffsetOfILE>
{
    public:

        //  ctor / dtor

        CCountedInvasiveList()
            :   CInvasiveList<CObject, OffsetOfILE>(),
                m_c( 0 )
        {
        }

        //  operators

        CCountedInvasiveList& operator=( const CCountedInvasiveList& il )
        {
            m_c = il.m_c;

            CInvasiveList<CObject, OffsetOfILE>::operator=( il );

            return *this;
        }

        //  API

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

        // returns the count if elements currently in the list

        size_t Count() const
        {
            return m_c;
        }

    private:

        size_t m_c;
};

//////////////////////////////////////////////////////////////////////////////////////////
//  CLocklessLinkedList / Singly Linked List (lockless operations)
//
//  This implements a rudimentary, singly linked list.  It supports two atomic operations
//  for lockless add and lockless retrieve (whole list, not top element).  It also supports
//  remove head object (not atomic/thread safe) and a few property functions FEmpty(), etc.
//
//  CObject         = class to be linked into the list.
//
//  cbOffsetOfNext  = Does not use the typical ILE function, but rather has user pass
//                    the cbOffset of pNext element in the structure to use for our IC.
//

template< class CObject >
class CLocklessLinkedList
{
    private:

        CObject * volatile m_pHead;

        CObject ** Ppnext_( CObject * pobj, ULONG cbOffsetOfNext )
        {
            COLLAssert( ( cbOffsetOfNext + sizeof(CObject*) ) <= sizeof(CObject) );

            // This would mean object is linked to itself, circularly
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

            // tried to insert something already w/ linked list next set?
            COLLAssert( NULL == *Ppnext_( pobj, cbOffsetOfNext ) );

            AtomicAddLinklistNode( pobj, (void**)Ppnext_( pobj, cbOffsetOfNext ), (void**)&m_pHead );
        }


        CLocklessLinkedList( CObject* phead ) : m_pHead( phead ) { }

        CObject * AtomicRemoveList()
        {
            return (CObject*) PvAtomicRetrieveLinklist( (void *volatile *)&m_pHead );
        }

        //  Note: Unlike the previous functions this is not thread safe / atomic.

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

        CObject * Head() const /* sort of const */
        {
            return m_pHead;
        }

};


//////////////////////////////////////////////////////////////////////////////////////////
//  CSimpleQueue (invasive-based) / Singly Linked List (with tail)
//
//  This implements a rudimentary, singly linked list with tail pointer.  In addition to 
//  non-thread-safe operations of CLocklessLinkedList, the tail pointer allows support for 
//  InsertAsNextMost() type operations.
//
//  CObject         = class to be linked into the list.
//
//  cbOffsetOfNext  = Does not use the typical ILE function, but rather has user pass
//                    the cbOffset of pNext element in the structure to use for our IC.
//

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

            // This would mean object is linked to itself, circularly
            COLLAssert( pobj != *( (CObject**)( ( (BYTE*)pobj ) + cbOffsetOfNext ) ) );

            return (CObject**)( ( (BYTE*)pobj ) + cbOffsetOfNext );
        }

        const CObject ** Ppnext_( const CObject * pobj, ULONG cbOffsetOfNext )
        {
            //  overriding const case, but we know it is ok...
            return (const CObject **)Ppnext_( (CObject*)pobj, cbOffsetOfNext );
        }

        bool FValid() const
        {
            return ( ( ( m_Head.FEmpty() && NULL == m_pTail && 0 == m_cObject ) ||
                       ( !m_Head.FEmpty() && m_pTail && m_cObject ) ) &&            // completely empty or "completely un-empty" ...
                     ( ( 1 != m_cObject ) || ( m_Head.Head() == m_pTail ) ) &&      // simple 1-element checks
                     ( ( 2 != m_cObject ) || ( m_Head.Head() != m_pTail ) )         // simple 2-element checks
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
            COLLAssert( pobj );             // not required, just what would be the point otherwise
            COLLAssert( !m_Head.FEmpty() ); // true b/c pobj != NULL

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
                        //  we've reached the end of the list, setup the tail.
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

            // tried to insert something already w/ linked list next set?
            COLLAssert( NULL == *Ppnext_( pobj, cbOffsetOfNext ) );

            m_Head.AtomicInsertAsPrevMost( pobj, cbOffsetOfNext );
            if ( NULL == m_pTail )
            {
                //  list is empty (well tail), head better have been empty tooo.
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

            // tried to insert something already w/ linked list next set?
            COLLAssert( NULL == *Ppnext_( pobj, cbOffsetOfNext ) );

            if ( m_pTail )
            {
                // next of tail should be NULL.
                COLLAssert( NULL == *Ppnext_( m_pTail, cbOffsetOfNext ) );

                *Ppnext_( m_pTail, cbOffsetOfNext ) = pobj;
                m_pTail = pobj;
            }
            else
            {
                // if tail is empty, head should be too.
                COLLAssert( NULL == m_Head.Head() );
                COLLAssert( FEmpty() && m_Head.FEmpty() );

                // list empty, fill first head, and tail.
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
                // we managed to empty the list, so empty this too.

                m_pTail = NULL;
            }

            if ( pobj )
            {
                //  If we're giving back an object, we need to decrement the count

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

};  // end of CSimpleQueue


//////////////////////////////////////////////////////////////////////////////////////////
//  CApproximateIndex
//
//  Implements a dynamically resizable table of entries indexed approximately by key
//  ranges of a specified uncertainty.  Accuracy and exact ordering are sacrificied for
//  improved performance and concurrency.  This index is optimized for a set of records
//  whose keys occupy a fairly dense range of values.  The index is designed to handle
//  key ranges that can wrap around zero.  As such, the indexed key range can not span
//  more than half the numerical precision of the key.
//
//  CKey            = class representing keys used to order entries in the mesh table.
//                    this class must support all the standard math operators.  wrap-
//                    around in the key values is supported
//  CEntry          = class indexed by the mesh table.  this class must contain storage
//                    for a CInvasiveContext class
//  OffsetOfIC      = inline function returning the offset of the CInvasiveContext
//                    contained in the CEntry
//
//  You must use the DECLARE_APPROXIMATE_INDEX macro to declare this class.

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
class CApproximateIndex
{
    public:

        //  class containing context needed per CEntry

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

        //  API Error Codes

        enum class ERR
        {
            errSuccess,
            errInvalidParameter,
            errOutOfMemory,
            errEntryNotFound,
            errNoCurrentEntry,
            errKeyRangeExceeded,
        };

        //  API Lock Context

        class CLock;

    public:

        //  ctor / dtor

        CApproximateIndex( const INT Rank );
        ~CApproximateIndex();

        //  API

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

        //  bucket used for containing index entries that have approximately
        //  the same key

        class CBucket
        {
            public:

                //  bucket ID

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

        //  table that contains our buckets

        typedef CDynamicHashTable< typename CBucket::ID, CBucket > CBucketTable;

    public:

        //  API Lock Context

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
                //  We keep statistics so we can know how efficient the approx idx is behaving.
                //

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
                static const ULONG      cchStatsString = 140; // from below string.
                void SPrintStats( __in_bcount(cchStatsString) char rgStatsString[] )
                {

                    ULONG cRealEmptyBuckets = m_cEnumeratedEntries == 0 ?
                                                m_cEnumeratedEmptyBuckets + 1 :
                                                m_cEnumeratedEmptyBuckets;
                    OSStrCbFormatA( rgStatsString, cchStatsString,
                                    // ~82 chars + 5 DWORDs + NUL ~= 133 use 140
                                    "ID Range: %d - %d, Visited Entries = %d, Visited Buckets = %d, Skipped Buckets = %d",
                                    m_idStart, m_idEnd,
                                    m_cEnumeratedEntries,
                                    m_cEnumeratedBuckets - cRealEmptyBuckets,
                                    cRealEmptyBuckets );
                }

                LONG CEnumeratedEntries() const             { return m_cEnumeratedEntries; }
                LONG CEnumeratedBuckets() const             { return m_cEnumeratedBuckets; }
                LONG CEnumeratedEmptyBuckets() const        { return m_cEnumeratedEmptyBuckets; }

#else   // !APPROXIMATE_INDEX_STATS
            public:
                void SPrintStats( __in_bcount(cchStatsString) char rgStatsString[] )
                {
                    OSStrCbCopyA( rgStatsString, cchStatsString, "Stats compiled out / disabled." );
                }
#endif  // APPROXIMATE_INDEX_STATS
        };

#ifdef APPROXIMATE_INDEX_STATS
        //  this first function is a bit of a layering violation
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

        //  never updated

        LONG                    m_shfKeyPrecision;
        LONG                    m_shfKeyUncertainty;
        LONG                    m_shfBucketHash;
        LONG                    m_shfFillMSB;
        typename CBucket::ID    m_maskBucketKey;
        typename CBucket::ID    m_maskBucketPtr;
        typename CBucket::ID    m_maskBucketID;
        LONG                    m_didRangeMost;
        //BYTE                  m_rgbReserved1[ 0 ];

        //  seldom updated

        CCriticalSection        m_critUpdateIdRange;
        LONG                    m_cidRange;
        typename CBucket::ID    m_idRangeFirst;
        typename CBucket::ID    m_idRangeLast;
        BYTE                    m_rgbReserved2[ 16 ];

        //  commonly updated

        CBucketTable            m_bt;
        //BYTE                  m_rgbReserved3[ 0 ];
};

//  ctor

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CApproximateIndex< CKey, CEntry, OffsetOfIC >::
CApproximateIndex( const INT Rank )
    :   m_critUpdateIdRange( CLockBasicInfo( CSyncBasicInfo( "CApproximateIndex::m_critUpdateIdRange" ), 1, 0 ) ),
        m_bt( Rank )
{
    //  we use a minimal rank for the internal UpdateIdRange critsec; the overall Rank has to be higher
    COLLAssert( Rank > 1 );
}

//  dtor

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CApproximateIndex< CKey, CEntry, OffsetOfIC >::
~CApproximateIndex()
{
}

//  initializes the approximate index using the given parameters.  if the index
//  cannot be initialized, errOutOfMemory is returned

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
ErrInit(    const CKey      dkeyPrecision,
            const CKey      dkeyUncertainty,
            const double    dblSpeedSizeTradeoff )
{
    //  validate all parameters

    if (    dkeyPrecision <= dkeyUncertainty ||
            dkeyUncertainty < CKey( 0 ) ||
            dblSpeedSizeTradeoff < 0.0 || dblSpeedSizeTradeoff > 1.0 && dblSpeedSizeTradeoff != 99.9 )
    {
        return ERR::errInvalidParameter;
    }

    //  init our parameters

    const CBucket::ID cbucketHashMin = ( dblSpeedSizeTradeoff == 99.9 ) ?
                                        CBucket::ID( 64 ) :  //  MAXIMUM_PROCESSORS for largest OS
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

    //  if our parameters leave us with too much or too little precision for
    //  our bucket IDs, fail.  "too much" precision would allow our bucket IDs
    //  to span more than half the precision of our bucket ID and cause our
    //  wrap-around-aware comparisons to fail.  "too little" precision would
    //  give us too few bucket IDs to allow us to hash efficiently
    //
    //  NOTE:  we check for hash efficiency in the worst case so that we don't
    //  suddenly return errInvalidParameter on some new monster machine

    const CBucket::ID cbucketHashMax = CBucket::ID( sizeof( void* ) * 8 );  //  MAXIMUM_PROCESSORS

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

    //  the uncertainty must be able to cover the worst case bucket hash

    if ( m_shfKeyUncertainty < shfBucketHashMax )
    {
        return ERR::errInvalidParameter;
    }

    //  limit the ID range to within half the precision of the bucket ID

    m_didRangeMost = m_maskBucketID >> 1;

    //  init our bucket ID range to be empty

    m_cidRange          = 0;
    m_idRangeFirst      = 0;
    m_idRangeLast       = 0;

    //  initialize the bucket table
    //  
    //  Parameters:
    //      dblLoadFactor = 5.0
    //
    //      dblUniformity = 1.0
    //
    //      cBucketMinimum = 4 * cbucketHashMin
    //          this number is chosen because the cBucketHashMin represents the
    //  number of buckets that a single key range is divided into. note that
    //  the approximate index divides a single range across buckets to improve
    //  concurrency in the case that all keys hash to the same bucket. by choosing
    //  a value greater that cbucketHashMin, we guarantee there are at least 4 
    //  distinct key ranges at any time. prior to 2009, this value was 0; but
    //  was changed to allow the FastEviction heuristic to work immediately i.e.
    //  without waiting for the DHT to need to grow.
    //
    //  FUTURE: KD: consider adjusting how we choose cbucketHashMin
    //

    if ( m_bt.ErrInit( 5.0, 1.0, 4 * cbucketHashMin ) != CBucketTable::ERR::errSuccess )
    {
        Term();
        return ERR::errOutOfMemory;
    }

    return ERR::errSuccess;
}

//  terminates the approximate index.  this function can be called even if the
//  index has never been initialized or is only partially initialized
//
//  NOTE:  any data stored in the index at this time will be lost!

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
Term()
{
    //  terminate the bucket table

    m_bt.Term();
}

//  acquires a lock on the specified key and entry pointer and returns the lock
//  in the provided lock context. waits forever.

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
LockKeyPtr( const CKey& key, CEntry* const pentry, CLock* const plock)
{
    BOOL fLockSucceeded = FTryLockKeyPtr( key,      // key
                                          pentry,   // Entry*
                                          plock,    // lock state
                                          fFalse);  // fCanFail
    COLLAssert( fLockSucceeded );
}

//  acquires a lock on the specified key and entry pointer and returns the lock
//  in the provided lock context. returns failure if can't be take immediately and
//  fCanFail is specified

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline BOOL CApproximateIndex< CKey, CEntry, OffsetOfIC >::
FTryLockKeyPtr( const CKey& key, CEntry* const pentry, CLock* const plock, BOOL fCanFail )
{
    //  compute the bucket ID for this key and entry pointer

    plock->m_bucket.m_id = _IdFromKeyPtr( key, pentry );

    //  write lock this bucket ID in the bucket table

    BOOL fLockSucceeded = m_bt.FTryWriteLockKey( plock->m_bucket.m_id, &plock->m_lock, fCanFail );
    if ( !fLockSucceeded )
    {
        return fLockSucceeded;
    }

    //  fetch this bucket from the bucket table if it exists.  if it doesn't
    //  exist, the bucket will start out empty and have the above bucket ID

    plock->m_bucket.m_cPin = 0;
    plock->m_bucket.m_il.Empty();
    (void)m_bt.ErrRetrieveEntry( &plock->m_lock, &plock->m_bucket );

    //  the entry is in this bucket
    //  WARNING: FMember doesn't actually work

    if ( plock->m_bucket.m_il.FMember( pentry ) )
    {
        //  set our currency to be on this entry in the bucket

        plock->m_pentryPrev = NULL;
        plock->m_pentry     = pentry;
        plock->m_pentryNext = NULL;
    }

    //  the entry is not in this bucket

    else
    {
        //  set our currency to be before the first entry in this bucket

        plock->m_pentryPrev = NULL;
        plock->m_pentry     = NULL;
        plock->m_pentryNext = plock->m_bucket.m_il.PrevMost();
    }

    //  if this bucket isn't pinned, it had better be represented by the valid
    //  bucket ID range of the index

    COLLAssert( !plock->m_bucket.m_cPin ||
            (   _CmpId( plock->m_bucket.m_id, m_idRangeFirst ) >= 0 &&
                _CmpId( plock->m_bucket.m_id, m_idRangeLast ) <= 0 ) );

    return fTrue;
}

//  releases the lock in the specified lock context

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
UnlockKeyPtr( CLock* const plock )
{
    //  if this bucket isn't pinned, it had better be represented by the valid
    //  bucket ID range of the index

    COLLAssert( !plock->m_bucket.m_cPin ||
            (   _CmpId( plock->m_bucket.m_id, m_idRangeFirst ) >= 0 &&
                _CmpId( plock->m_bucket.m_id, m_idRangeLast ) <= 0 ) );

    //  write unlock this bucket ID in the bucket table

    m_bt.WriteUnlockKey( &plock->m_lock );
}

//  compares two keys as they would be seen relative to each other by the
//  approximate index

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline LONG CApproximateIndex< CKey, CEntry, OffsetOfIC >::
CmpKey( const CKey& key1, const CKey& key2 ) const
{
    return _CmpId( _IdFromKey( key1 ), _IdFromKey( key2 ) );
}

//  returns the first key in the current key range.  this key is guaranteed to
//  be at least as small as the key of any record currently in the index given
//  the precision and uncertainty of the index. this value is not directly 
//  comparable to input keys which are masked based on the precision specified.

//  WARNING: read that last sentence carefully. Even though a CKey value is returned
//  it is no longer in the same space as CKey used by clients of the ApproximateIndex

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CKey CApproximateIndex< CKey, CEntry, OffsetOfIC >::
KeyRangeFirst() const
{
    return _KeyFromId( m_idRangeFirst );
}

//  returns the last key in the current key range.  this key is guaranteed to
//  be at least as large as the key of any record currently in the index given
//  the precision and uncertainty of the index. this value is not directly 
//  comparable to input keys which are masked based on the precision specified.

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CKey CApproximateIndex< CKey, CEntry, OffsetOfIC >::
KeyRangeLast() const
{
    return _KeyFromId( m_idRangeLast );
}

//  returns the smallest key that could be successfully inserted into the index

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CKey CApproximateIndex< CKey, CEntry, OffsetOfIC >::
KeyInsertLeast() const
{
    const CBucket::ID cBucketHash = 1 << m_shfBucketHash;

    CBucket::ID idFirstLeast = m_idRangeLast - m_didRangeMost;
    idFirstLeast = idFirstLeast + ( cBucketHash - idFirstLeast % cBucketHash ) % cBucketHash;

    return _KeyFromId( idFirstLeast );
}

//  returns the largest key that could be successfully inserted into the index.
//  warning: this does not take into account precision or uncertainty i.e. a key's
//  high bits and low bits gets masked out. 

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CKey CApproximateIndex< CKey, CEntry, OffsetOfIC >::
KeyInsertMost() const
{
    const CBucket::ID cBucketHash = 1 << m_shfBucketHash;

    CBucket::ID idLastMost = m_idRangeFirst + m_didRangeMost;
    idLastMost = idLastMost - ( idLastMost + 1 ) % cBucketHash;

    return _KeyFromId( idLastMost );
}

//  retrieves the entry corresponding to the key and entry pointer locked by the
//  specified lock context.  if there is no entry for this key, errEntryNotFound
//  will be returned

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
ErrRetrieveEntry( CLock* const plock, CEntry** const ppentry ) const
{
    //  return the current entry.  if the current entry is NULL, then there is
    //  no current entry

    if ( plock->m_pentry )
    {
        COLLAssert( m_bt.FHasCurrentRecord( &plock->m_lock ) );
    }

    *ppentry = plock->m_pentry;

    return *ppentry ? ERR::errSuccess : ERR::errEntryNotFound;
}

//  inserts a new entry corresponding to the key and entry pointer locked by the
//  specified lock context.  fNextMost biases the position the entry will take
//  when inserted in the index.  if the new entry cannot be inserted,
//  errOutOfMemory will be returned.  if inserting the new entry will cause the
//  key space to become too large, errKeyRangeExceeded will be returned
//
//  NOTE:  it is illegal to attempt to insert an entry into the index that is
//  already in the index

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
ErrInsertEntry( CLock* const plock, CEntry* const pentry, const BOOL fNextMost )
{
    CBucketTable::ERR err;

    //  this entry had better not already be in the index

    COLLAssert( !plock->m_bucket.m_il.FMember( pentry ) );

    //  pin the bucket on behalf of the entry to insert

    plock->m_bucket.m_cPin++;

    //  insert this entry at the selected end of the current bucket

    if ( fNextMost )
    {
        plock->m_bucket.m_il.InsertAsNextMost( pentry );
    }
    else
    {
        plock->m_bucket.m_il.InsertAsPrevMost( pentry );
    }

    //  try to update this bucket in the bucket table

    if ( ( err = m_bt.ErrReplaceEntry( &plock->m_lock, plock->m_bucket ) ) != CBucketTable::ERR::errSuccess )
    {
        COLLAssert( err == CBucketTable::ERR::errNoCurrentEntry );

        //  the bucket does not yet exist, so try to insert it in the bucket table

        return _ErrInsertEntry( plock, pentry );
    }

    //  we succeeded in updating the bucket

    else
    {
        //  set the current entry to the newly inserted entry

        plock->m_pentryPrev = NULL;
        plock->m_pentry     = pentry;
        plock->m_pentryNext = NULL;
        return ERR::errSuccess;
    }
}

//  deletes the entry corresponding to the key and entry pointer locked by the
//  specified lock context.  if there is no entry for this key, errNoCurrentEntry
//  will be returned

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
ErrDeleteEntry( CLock* const plock )
{
    //  there is a current entry

    if ( plock->m_pentry )
    {
        //  save the current entry's prev and next pointers so that we can
        //  recover our currency when it is deleted

        plock->m_pentryPrev = plock->m_bucket.m_il.Prev( plock->m_pentry );
        plock->m_pentryNext = plock->m_bucket.m_il.Next( plock->m_pentry );

        //  delete the current entry from this bucket

        plock->m_bucket.m_il.Remove( plock->m_pentry );

        //  unpin the bucket on behalf of this entry

        plock->m_bucket.m_cPin--;

        //  update the bucket in the bucket table.  it is OK if the bucket is
        //  empty because empty buckets are deleted in _ErrMoveNext/_ErrMovePrev

        const CBucketTable::ERR err = m_bt.ErrReplaceEntry( &plock->m_lock, plock->m_bucket );
        COLLAssert( err == CBucketTable::ERR::errSuccess );

        //  set our currency to no current entry

        plock->m_pentry = NULL;
        return ERR::errSuccess;
    }

    //  there is no current entry

    else
    {
        //  return no current entry

        return ERR::errNoCurrentEntry;
    }
}

//  reserves room to insert a new entry corresponding to the key and entry
//  pointer locked by the specified lock context.  if room for the new entry
//  cannot be reserved, errOutOfMemory will be returned.  if reserving the new
//  entry will cause the key space to become too large, errKeyRangeExceeded
//  will be returned
//
//  NOTE:  once room is reserved, it must be unreserved via UnreserveEntry()

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
ErrReserveEntry( CLock* const plock )
{
    //  pin the locked bucket

    plock->m_bucket.m_cPin++;

    //  we failed to update the pin count on the bucket in the index because the
    //  bucket doesn't exist

    CBucketTable::ERR errBT;

    if ( ( errBT = m_bt.ErrReplaceEntry( &plock->m_lock, plock->m_bucket ) ) != CBucketTable::ERR::errSuccess )
    {
        COLLAssert( errBT == CBucketTable::ERR::errNoCurrentEntry );

        //  insert this bucket in the bucket table

        ERR err;

        if ( ( err = _ErrInsertBucket( plock ) ) != ERR::errSuccess )
        {
            COLLAssert( err == ERR::errOutOfMemory || err == ERR::errKeyRangeExceeded );

            //  we cannot insert the bucket so unpin the locked bucket and fail
            //  the reservation

            plock->m_bucket.m_cPin--;
            return err;
        }
    }

    return ERR::errSuccess;
}

//  removes a reservation made with ErrReserveEntry()

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
UnreserveEntry( CLock* const plock )
{
    //  unpin the locked bucket

    COLLAssert( plock->m_bucket.m_cPin > 0 );
    plock->m_bucket.m_cPin--;

    //  update the pin count on the bucket in the index.  this cannot fail
    //  because we know the bucket exists because it is pinned

    CBucketTable::ERR errBT = m_bt.ErrReplaceEntry( &plock->m_lock, plock->m_bucket );
    COLLAssert( errBT == CBucketTable::ERR::errSuccess );
}

//  sets up the specified lock context in preparation for scanning all entries
//  in the index by ascending key value, give or take the key uncertainty
//
//  NOTE:  this function will acquire a lock that must eventually be released
//  via UnlockKeyPtr()

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
MoveBeforeFirst( CLock* const plock )
{
    //  we will start scanning at the first bucket ID believed to be present in
    //  the index (it could have been emptied by now)

    plock->m_bucket.m_id = m_idRangeFirst;

#ifdef APPROXIMATE_INDEX_STATS
    plock->m_idStart = plock->m_bucket.m_id;
    plock->m_cEnumeratedBuckets++;
#endif

    //  write lock this bucket ID in the bucket table

    m_bt.WriteLockKey( plock->m_bucket.m_id, &plock->m_lock );

    //  fetch this bucket from the bucket table if it exists.  if it doesn't
    //  exist, the bucket will start out empty and have the above bucket ID

    plock->m_bucket.m_cPin = 0;
    plock->m_bucket.m_il.Empty();
    (void)m_bt.ErrRetrieveEntry( &plock->m_lock, &plock->m_bucket );

    //  set our currency to be before the first entry in this bucket

    plock->m_pentryPrev = NULL;
    plock->m_pentry     = NULL;
    plock->m_pentryNext = plock->m_bucket.m_il.PrevMost();
}

//  moves the specified lock context to the next key and entry pointer in the
//  index by ascending key value, give or take the key uncertainty.  if the end
//  of the index is reached, errNoCurrentEntry is returned
//
//  NOTE:  this function will acquire a lock that must eventually be released
//  via UnlockKeyPtr()

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
ErrMoveNext( CLock* const plock )
{
    //  move to the next entry in this bucket

    plock->m_pentryPrev = NULL;
    plock->m_pentry     =   plock->m_pentry ?
                                plock->m_bucket.m_il.Next( plock->m_pentry ) :
                                plock->m_pentryNext;
    plock->m_pentryNext = NULL;

    //  we still have no current entry

    if ( !plock->m_pentry )
    {
        //  possibly advance to the next bucket

        return _ErrMoveNext( plock );
    }

    //  we now have a current entry

    else
    {
        //  we're done

#ifdef APPROXIMATE_INDEX_STATS
        plock->m_cEnumeratedEntries++;
#endif
        return ERR::errSuccess;
    }
}

//  moves the specified lock context to the next key and entry pointer in the
//  index by descending key value, give or take the key uncertainty.  if the
//  start of the index is reached, errNoCurrentEntry is returned
//
//  NOTE:  this function will acquire a lock that must eventually be released
//  via UnlockKeyPtr()

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
ErrMovePrev( CLock* const plock )
{
    //  move to the prev entry in this bucket

    plock->m_pentryNext = NULL;
    plock->m_pentry     =   plock->m_pentry ?
                                plock->m_bucket.m_il.Prev( plock->m_pentry ) :
                                plock->m_pentryPrev;
    plock->m_pentryPrev = NULL;

    //  we still have no current entry

    if ( !plock->m_pentry )
    {
        //  possibly advance to the prev bucket

        return _ErrMovePrev( plock );
    }

    //  we now have a current entry

    else
    {
        //  we're done

#ifdef APPROXIMATE_INDEX_STATS
        plock->m_cEnumeratedEntries++;
#endif
        return ERR::errSuccess;
    }
}

//  sets up the specified lock context in preparation for scanning all entries
//  in the index by descending key value, give or take the key uncertainty
//
//  NOTE:  this function will acquire a lock that must eventually be released
//  via UnlockKeyPtr()

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
MoveAfterLast( CLock* const plock )
{
    //  we will start scanning at the last bucket ID believed to be present in
    //  the index (it could have been emptied by now)

    plock->m_bucket.m_id = m_idRangeLast;

#ifdef APPROXIMATE_INDEX_STATS
    plock->m_idStart = plock->m_bucket.m_id;
    plock->m_cEnumeratedBuckets++;
#endif

    //  write lock this bucket ID in the bucket table

    m_bt.WriteLockKey( plock->m_bucket.m_id, &plock->m_lock );

    //  fetch this bucket from the bucket table if it exists.  if it doesn't
    //  exist, the bucket will start out empty and have the above bucket ID

    plock->m_bucket.m_cPin = 0;
    plock->m_bucket.m_il.Empty();
    (void)m_bt.ErrRetrieveEntry( &plock->m_lock, &plock->m_bucket );

    //  set our currency to be after the last entry in this bucket

    plock->m_pentryPrev = plock->m_bucket.m_il.NextMost();
    plock->m_pentry     = NULL;
    plock->m_pentryNext = NULL;
}

//  sets up the specified lock context in preparation for scanning all entries
//  greater than or approximately equal to the specified key and entry pointer
//  in the index by ascending key value, give or take the key uncertainty
//
//  NOTE:  this function will acquire a lock that must eventually be released
//  via UnlockKeyPtr()
//
//  NOTE:  even though this function may land between two valid entries in
//  the index, the currency will not be on one of those entries until
//  ErrMoveNext() or ErrMovePrev() has been called

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
MoveBeforeKeyPtr( const CKey& key, CEntry* const pentry, CLock* const plock )
{
    //  we will start scanning at the bucket ID formed from the given key and
    //  entry pointer

    plock->m_bucket.m_id = _IdFromKeyPtr( key, pentry );

#ifdef APPROXIMATE_INDEX_STATS
    plock->m_idStart = plock->m_bucket.m_id;
    plock->m_cEnumeratedBuckets++;
#endif

    //  write lock this bucket ID in the bucket table

    m_bt.WriteLockKey( plock->m_bucket.m_id, &plock->m_lock );

    //  fetch this bucket from the bucket table if it exists.  if it doesn't
    //  exist, the bucket will start out empty and have the above bucket ID

    plock->m_bucket.m_cPin = 0;
    plock->m_bucket.m_il.Empty();
    (void)m_bt.ErrRetrieveEntry( &plock->m_lock, &plock->m_bucket );

    //  set our currency to be before the first entry in this bucket

    plock->m_pentryPrev = NULL;
    plock->m_pentry     = NULL;
    plock->m_pentryNext = plock->m_bucket.m_il.PrevMost();
}

//  sets up the specified lock context in preparation for scanning all entries
//  less than or approximately equal to the specified key and entry pointer
//  in the index by descending key value, give or take the key uncertainty
//
//  NOTE:  this function will acquire a lock that must eventually be released
//  via UnlockKeyPtr()
//
//  NOTE:  even though this function may land between two valid entries in
//  the index, the currency will not be on one of those entries until
//  ErrMoveNext() or ErrMovePrev() has been called

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
MoveAfterKeyPtr( const CKey& key, CEntry* const pentry, CLock* const plock )
{
    //  we will start scanning at the bucket ID formed from the given key and
    //  entry pointer

    plock->m_bucket.m_id = _IdFromKeyPtr( key, pentry );

#ifdef APPROXIMATE_INDEX_STATS
    plock->m_idStart = plock->m_bucket.m_id;
    plock->m_cEnumeratedBuckets++;
#endif

    //  write lock this bucket ID in the bucket table

    m_bt.WriteLockKey( plock->m_bucket.m_id, &plock->m_lock );

    //  fetch this bucket from the bucket table if it exists.  if it doesn't
    //  exist, the bucket will start out empty and have the above bucket ID

    plock->m_bucket.m_cPin = 0;
    plock->m_bucket.m_il.Empty();
    (void)m_bt.ErrRetrieveEntry( &plock->m_lock, &plock->m_bucket );

    //  set our currency to be after the last entry in this bucket

    plock->m_pentryPrev = plock->m_bucket.m_il.NextMost();
    plock->m_pentry     = NULL;
    plock->m_pentryNext = NULL;
}

//  transforms the given key and entry pointer into a bucket ID

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::CBucket::ID CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_IdFromKeyPtr( const CKey& key, CEntry* const pentry ) const
{
    //  we compute the bucket ID such that each uncertainty range is split into
    //  several buckets, each of which are indexed by the pointer.  we do this
    //  to provide maximum concurrency while accessing any particular range of
    //  keys.  the reason we use the pointer in the calculation is that we want
    //  to minimize the number of times the user has to update the position of
    //  an entry due to a key change yet we need some property of the entry
    //  over which we can reproducibly hash

    const CBucket::ID   iBucketKey      = CBucket::ID( key >> m_shfKeyUncertainty );
    const CBucket::ID   iBucketPtr      = CBucket::ID( LONG_PTR( pentry ) / LONG_PTR( sizeof( CEntry ) ) );

    return ( ( iBucketKey & m_maskBucketKey ) << m_shfBucketHash ) + ( iBucketPtr & m_maskBucketPtr );
}

//  transforms the given key into a bucket ID

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::CBucket::ID CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_IdFromKey( const CKey& key ) const
{
    return _IdFromKeyPtr( key, NULL );
}

//  transforms the given bucket ID into a key

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CKey CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_KeyFromId( const typename CBucket::ID id ) const
{
    return CKey( id >> m_shfBucketHash ) << m_shfKeyUncertainty;
}

//  performs a wrap-around insensitive delta of a bucket ID by an offset

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::CBucket::ID CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_DeltaId( const typename CBucket::ID id, const LONG did ) const
{
    return ( id + CBucket::ID( did ) ) & m_maskBucketID;
}

//  performs a wrap-around insensitive subtraction of two bucket IDs

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline LONG CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_SubId( const typename CBucket::ID id1, const typename CBucket::ID id2 ) const
{
    //  munge bucket IDs to fill the Most Significant Bit of a long so that we
    //  can make a wrap-around aware subtraction

    const LONG lid1 = id1 << m_shfFillMSB;
    const LONG lid2 = id2 << m_shfFillMSB;

    //  munge the result back into the same scale as the bucket IDs

    return CBucket::ID( ( lid1 - lid2 ) >> m_shfFillMSB );
}

//  performs a wrap-around insensitive comparison of two bucket IDs

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline LONG CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_CmpId( const typename CBucket::ID id1, const typename CBucket::ID id2 ) const
{
    //  munge bucket IDs to fill the Most Significant Bit of a long so that we
    //  can make a wrap-around aware comparison

    const LONG lid1 = id1 << m_shfFillMSB;
    const LONG lid2 = id2 << m_shfFillMSB;

    return lid1 - lid2;
}

//  converts a pointer to an entry to a pointer to the invasive context

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::CInvasiveContext* CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_PicFromPentry( CEntry* const pentry ) const
{
    return (CInvasiveContext*)( (BYTE*)pentry + OffsetOfIC() );
}

//  tries to expand the bucket ID range by adding the new bucket ID.  if this
//  cannot be done without violating the range constraints, fFalse will be
//  returned

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline BOOL CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_FExpandIdRange( const typename CBucket::ID idNew )
{
    COLLAssert( m_critUpdateIdRange.FOwner() );

    //  fetch the current ID range

    const LONG          cidRange    = m_cidRange;
    const CBucket::ID   idFirst     = m_idRangeFirst;
    const CBucket::ID   idLast      = m_idRangeLast;
    const LONG          didRange    = _SubId( idLast, idFirst );

    COLLAssert( didRange >= 0 );
    COLLAssert( didRange <= m_didRangeMost );
    COLLAssert( cidRange >= 0 );
    COLLAssert( (ULONG)cidRange <= (ULONG)m_didRangeMost + 1 );

    //  if there are no entries in the ID range then simply set the ID range to
    //  exactly contain this new bucket ID

    if ( !cidRange )
    {
        m_cidRange      = 1;
        m_idRangeFirst  = idNew;
        m_idRangeLast   = idNew;

        return fTrue;
    }

    //  compute the valid range for the new first ID and new last ID.  these
    //  points and the above points form four ranges in a circular number
    //  line containing all possible bucket IDs:
    //
    //    ( idFirstMic, idFirst )    Possible extension of the ID range
    //    [ idFirst, idLast ]        The current ID range
    //    ( idLast, idLastMax )      Possible extension of the ID range
    //    [ idLastMax, idFirstMic ]  Cannot be part of the ID range
    //
    //  these ranges will never overlap due to the restriction that the
    //  ID range cannot meet or exceed half the number of bucket IDs
    //
    //  NOTE:  due to a quirk in 2's complement arithmetic where the 2's
    //  complement negative of the smallest negative number is itself, the
    //  inclusive range tests fail when idFirst == idLast and idNew ==
    //  idFirstMic == idLastMax or when idFirstMic == idLastMax and idnew ==
    //  idFirst == idLast.  we have added special logic to handle these
    //  cases correctly

    const CBucket::ID   idFirstMic  = _DeltaId( idFirst, -( m_didRangeMost - didRange + 1 ) );
    const CBucket::ID   idLastMax   = _DeltaId( idLast, m_didRangeMost - didRange + 1 );

    //  if the new bucket ID is already part of this ID range, no change
    //  is needed

    if (    _CmpId( idFirstMic, idNew ) != 0 && _CmpId( idLastMax, idNew ) != 0 &&
            _CmpId( idFirst, idNew ) <= 0 && _CmpId( idNew, idLast ) <= 0 )
    {
        m_cidRange = cidRange + 1;

        return fTrue;
    }

    //  if the new bucket ID cannot be a part of this ID range, fail the
    //  expansion

    if (    _CmpId( idFirst, idNew ) != 0 && _CmpId( idLast, idNew ) != 0 &&
            _CmpId( idLastMax, idNew ) <= 0 && _CmpId( idNew, idFirstMic ) <= 0 )
    {
        return fFalse;
    }

    //  compute the new ID range including this new bucket ID

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

    //  the new ID range should be larger than the old ID range and should
    //  include the new bucket ID

    COLLAssert( _CmpId( idFirstNew, idFirst ) <= 0 );
    COLLAssert( _CmpId( idLast, idLastNew ) <= 0 );
    COLLAssert( _SubId( idLastNew, idFirstNew ) > 0 );
    COLLAssert( _SubId( idLastNew, idFirstNew ) <= m_didRangeMost );
    COLLAssert( _CmpId( idFirstNew, idNew ) <= 0 );
    COLLAssert( _CmpId( idNew, idLastNew ) <= 0 );

    //  update the key range to include the new bucket ID

    m_cidRange      = cidRange + 1;
    m_idRangeFirst  = idFirstNew;
    m_idRangeLast   = idLastNew;

    return fTrue;
}

//  inserts a new bucket in the bucket table

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_ErrInsertBucket( CLock* const plock )
{
    //  try to update the bucket ID range and subrange of the index to include
    //  this new bucket ID

    m_critUpdateIdRange.Enter();
    const BOOL fRangeUpdated = _FExpandIdRange( plock->m_bucket.m_id );
    m_critUpdateIdRange.Leave();

    //  if the update failed, fail the bucket insertion

    if ( !fRangeUpdated )
    {
        return ERR::errKeyRangeExceeded;
    }

    //  the bucket does not yet exist, so try to insert it in the bucket table

    CBucketTable::ERR err;

    if ( ( err = m_bt.ErrInsertEntry( &plock->m_lock, plock->m_bucket ) ) != CBucketTable::ERR::errSuccess )
    {
        COLLAssert( err == CBucketTable::ERR::errOutOfMemory );

        //  we cannot do the insert so fail

        m_critUpdateIdRange.Enter();
        m_cidRange--;
        m_critUpdateIdRange.Leave();

        return ERR::errOutOfMemory;
    }

    return ERR::errSuccess;
}

//  performs an entry insertion that must insert a new bucket in the bucket table

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_ErrInsertEntry( CLock* const plock, CEntry* const pentry )
{
    ERR err;

    //  insert this bucket in the bucket table

    if ( ( err = _ErrInsertBucket( plock ) ) != ERR::errSuccess )
    {
        COLLAssert( err == ERR::errOutOfMemory || err == ERR::errKeyRangeExceeded );

        //  we cannot insert the bucket so undo the list insertion and fail

        plock->m_bucket.m_il.Remove( pentry );
        plock->m_bucket.m_cPin--;
        return err;
    }

    //  set the current entry to the newly inserted entry

    plock->m_pentryPrev = NULL;
    plock->m_pentry     = pentry;
    plock->m_pentryNext = NULL;
    return ERR::errSuccess;
}

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_MoveBucketId( CLock* const plock, const LONG did )
{
    //  advance or decrement to the next or prev bucket

    plock->m_bucket.m_id = _DeltaId( plock->m_bucket.m_id, did );

    //  update stats if appropriate
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

//  performs a move next that possibly goes to the next bucket.  we won't go to
//  the next bucket if we are already at the last bucket ID

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_ErrMoveNext( CLock* const plock )
{
    //  set our currency to be after the last entry in this bucket

    plock->m_pentryPrev = plock->m_bucket.m_il.NextMost();
    plock->m_pentry     = NULL;
    plock->m_pentryNext = NULL;

    //  scan forward until we have a current entry or we are at or beyond the
    //  last bucket ID

    while ( !plock->m_pentry && _CmpId( plock->m_bucket.m_id, m_idRangeLast ) < 0 )
    {
        //  we are currently at the first bucket ID and that bucket isn't pinned

        if ( !plock->m_bucket.m_cPin )
        {
            //  delete this empty bucket (if it exists)

            const CBucketTable::ERR err = m_bt.ErrDeleteEntry( &plock->m_lock );
            COLLAssert( err == CBucketTable::ERR::errSuccess ||
                    err == CBucketTable::ERR::errNoCurrentEntry );

            //  advance the first bucket ID by one so that subsequent searches
            //  do not scan through this empty bucket unnecessarily

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

        //  unlock the current bucket ID in the bucket table

        m_bt.WriteUnlockKey( &plock->m_lock );

        //  this bucket ID may not be in the valid bucket ID range

        if (    _CmpId( m_idRangeFirst, plock->m_bucket.m_id ) > 0 ||
                _CmpId( plock->m_bucket.m_id, m_idRangeLast ) > 0 )
        {
            //  we can get the critical section protecting the bucket ID range

            if ( m_critUpdateIdRange.FTryEnter() )
            {
                //  this bucket ID is not in the valid bucket ID range

                if (    _CmpId( m_idRangeFirst, plock->m_bucket.m_id ) > 0 ||
                        _CmpId( plock->m_bucket.m_id, m_idRangeLast ) > 0 )
                {
                    //  go to the first valid bucket ID

                    plock->m_bucket.m_id = m_idRangeFirst;
                }

                //  this bucket ID is in the valid bucket ID range

                else
                {
                    //  advance to the next bucket ID

                    _MoveBucketId( plock, 1 ); // increments plock->m_bucket.m_id
                }

                m_critUpdateIdRange.Leave();
            }

            //  we cannot get the critical section protecting the bucket ID range

            else
            {
                //  advance to the next bucket ID

                _MoveBucketId( plock, 1 ); // increments plock->m_bucket.m_id
            }
        }

        //  this bucket may be in the valid bucket ID range

        else
        {
            //  advance to the next bucket ID

            _MoveBucketId( plock, 1 ); // increments plock->m_bucket.m_id
        }

        //  write lock this bucket ID in the bucket table

        m_bt.WriteLockKey( plock->m_bucket.m_id, &plock->m_lock );

        //  fetch this bucket from the bucket table if it exists.  if it doesn't
        //  exist, the bucket will start out empty and have the above bucket ID

        plock->m_bucket.m_cPin = 0;
        plock->m_bucket.m_il.Empty();
        (void)m_bt.ErrRetrieveEntry( &plock->m_lock, &plock->m_bucket );

        //  set our currency to be the first entry in this bucket

        plock->m_pentryPrev = NULL;
        plock->m_pentry     = plock->m_bucket.m_il.PrevMost();
        plock->m_pentryNext = NULL;
    }

    //  return the status of our currency

#ifdef APPROXIMATE_INDEX_STATS
    if ( plock->m_pentry )
    {
        plock->m_cEnumeratedEntries++;
    }
#endif

    return plock->m_pentry ? ERR::errSuccess : ERR::errNoCurrentEntry;
}

//  performs a move prev that goes possibly to the prev bucket.  we won't go to
//  the prev bucket if we are already at the first bucket ID

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline typename CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_ErrMovePrev( CLock* const plock )
{
    //  set our currency to be before the first entry in this bucket

    plock->m_pentryPrev = NULL;
    plock->m_pentry     = NULL;
    plock->m_pentryNext = plock->m_bucket.m_il.PrevMost();

    //  scan backward until we have a current entry or we are at or before the
    //  first bucket ID

    while ( !plock->m_pentry && _CmpId( m_idRangeFirst, plock->m_bucket.m_id ) < 0 )
    {
        //  we are currently at the last bucket ID and that bucket isn't pinned

        if ( !plock->m_bucket.m_cPin )
        {
            //  delete this empty bucket (if it exists)

            const CBucketTable::ERR err = m_bt.ErrDeleteEntry( &plock->m_lock );
            COLLAssert( err == CBucketTable::ERR::errSuccess ||
                    err == CBucketTable::ERR::errNoCurrentEntry );

            //  retreat the last bucket ID by one so that subsequent searches
            //  do not scan through this empty bucket unnecessarily

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

        //  unlock the current bucket ID in the bucket table

        m_bt.WriteUnlockKey( &plock->m_lock );

        //  this bucket ID may not be in the valid bucket ID range

        if (    _CmpId( m_idRangeFirst, plock->m_bucket.m_id ) > 0 ||
                _CmpId( plock->m_bucket.m_id, m_idRangeLast ) > 0 )
        {
            //  we can get the critical section protecting the bucket ID range

            if ( m_critUpdateIdRange.FTryEnter() )
            {
                //  this bucket ID is not in the valid bucket ID range

                if (    _CmpId( m_idRangeFirst, plock->m_bucket.m_id ) > 0 ||
                        _CmpId( plock->m_bucket.m_id, m_idRangeLast ) > 0 )
                {
                    //  go to the last valid bucket ID

                    plock->m_bucket.m_id = m_idRangeLast;
                }

                //  this bucket ID is in the valid bucket ID range

                else
                {
                    //  retreat to the previous bucket ID

                    _MoveBucketId( plock, -1 ); // decrements plock->m_bucket.m_id
                }

                m_critUpdateIdRange.Leave();
            }

            //  we cannot get the critical section protecting the bucket ID range

            else
            {
                //  retreat to the previous bucket ID

                _MoveBucketId( plock, -1 ); // decrements plock->m_bucket.m_id
            }
        }

        //  this bucket may be in the valid bucket ID range

        else
        {
            //  retreat to the previous bucket ID

            _MoveBucketId( plock, -1 ); // decrements plock->m_bucket.m_id
        }

        //  write lock this bucket ID in the bucket table

        m_bt.WriteLockKey( plock->m_bucket.m_id, &plock->m_lock );

        //  fetch this bucket from the bucket table if it exists.  if it doesn't
        //  exist, the bucket will start out empty and have the above bucket ID

        plock->m_bucket.m_cPin = 0;
        plock->m_bucket.m_il.Empty();
        (void)m_bt.ErrRetrieveEntry( &plock->m_lock, &plock->m_bucket );

        //  set our currency to be the last entry in this bucket

        plock->m_pentryPrev = NULL;
        plock->m_pentry     = plock->m_bucket.m_il.NextMost();
        plock->m_pentryNext = NULL;
    }

    //  return the status of our currency

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


//////////////////////////////////////////////////////////////////////////////////////////
//  CPool
//
//  Implements a pool of objects that can be inserted and deleted quickly in arbitrary
//  order.
//
//  CObject         = class representing objects in the pool.  each class must contain
//                    storage for a CInvasiveContext for embedded pool state
//  OffsetOfIC      = inline function returning the offset of the CInvasiveContext
//                    contained in the CObject

template< class CObject, PfnOffsetOf OffsetOfIC >
class CPool
{
    public:

        //  class containing context needed per CObject

        class CInvasiveContext
        {
            public:

                CInvasiveContext() {}
                ~CInvasiveContext() {}

                static SIZE_T OffsetOfILE() { return OffsetOfIC() + OffsetOf( CInvasiveContext, m_ile ); }

            private:

                typename CInvasiveList< CObject, OffsetOfILE >::CElement    m_ile;
        };

        //  API Error Codes

        enum class ERR
        {
            errSuccess,
            errInvalidParameter,
            errOutOfMemory,
            errObjectNotFound,
            errOutOfObjects,
            errNoCurrentObject,
        };

        //  API Lock Context

        class CLock;

    public:

        //  ctor / dtor

        CPool();
        ~CPool();

        //  API

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

        //  bucket used for containing objects in the pool

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

        //  API Lock Context

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

        //  never updated

        DWORD           m_cbucket;
        CBucket*        m_rgbucket;
        BYTE            m_rgbReserved1[24];

        //  commonly updated

        CSemaphore      m_semObjectCount;
        DWORD           m_cInsert;
        DWORD           m_cRemove;
        DWORD           m_cRemoveWait;
        BYTE            m_rgbReserved2[16];
};

//  ctor

template< class CObject, PfnOffsetOf OffsetOfIC >
inline CPool< CObject, OffsetOfIC >::
CPool()
    :   m_semObjectCount( CSyncBasicInfo( "CPool::m_semObjectCount" ) )
{
}

//  dtor

template< class CObject, PfnOffsetOf OffsetOfIC >
inline CPool< CObject, OffsetOfIC >::
~CPool()
{
    //  nop
}

//  initializes the pool using the given parameters.  if the pool cannot be
//  initialized, errOutOfMemory is returned

template< class CObject, PfnOffsetOf OffsetOfIC >
inline typename CPool< CObject, OffsetOfIC >::ERR CPool< CObject, OffsetOfIC >::
ErrInit( const double dblSpeedSizeTradeoff )
{
    //  validate all parameters

    if ( dblSpeedSizeTradeoff < 0.0 || dblSpeedSizeTradeoff > 1.0 )
    {
        return ERR::errInvalidParameter;
    }

    //  allocate our bucket array, one per CPU, on a cache-line boundary

    m_cbucket = OSSyncGetProcessorCount();
    const SIZE_T cbrgbucket = sizeof( CBucket ) * m_cbucket;
    if ( !( m_rgbucket = (CBucket*)_PvMEMAlloc( cbrgbucket, cbCacheLine ) ) )
    {
        return ERR::errOutOfMemory;
    }

    //  setup our bucket array

    for ( DWORD ibucket = 0; ibucket < m_cbucket; ibucket++ )
    {
        new( m_rgbucket + ibucket ) CBucket;
    }

    //  init out stats

    m_cInsert       = 0;
    m_cRemove       = 0;
    m_cRemoveWait   = 0;

    return ERR::errSuccess;
}

//  terminates the pool.  this function can be called even if the pool has never
//  been initialized or is only partially initialized
//
//  NOTE:  any data stored in the pool at this time will be lost!

template< class CObject, PfnOffsetOf OffsetOfIC >
inline void CPool< CObject, OffsetOfIC >::
Term()
{
    //  free our bucket array

    if ( m_rgbucket )
    {
        for ( DWORD ibucket = 0; ibucket < m_cbucket; ibucket++ )
        {
            m_rgbucket[ ibucket ].~CBucket();
        }
        _MEMFree( m_rgbucket );
        m_rgbucket = NULL;
    }

    //  remove any free counts on our semaphore

    while ( m_semObjectCount.FTryAcquire() )
    {
    }
}

//  inserts the given object into the pool

template< class CObject, PfnOffsetOf OffsetOfIC >
inline void CPool< CObject, OffsetOfIC >::
Insert( CObject* const pobj, const BOOL fMRU )
{
    //  add the given object to the bucket for this CPU.  we use one bucket per
    //  CPU to reduce cache sloshing.  if we cannot lock the bucket for this CPU,
    //  we will try another bucket instead of blocking
    //
    //  NOTE:  if we are freeing to the LRU end of the list then we will hash
    //  evenly across all buckets to distribute cold objects evenly

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

    //  increment the object count

    AtomicIncrement( (LONG*)&m_cInsert );
    m_semObjectCount.Release();
}

//  removes an object from the pool, optionally waiting until an object can be
//  removed.  if an object can be removed, errSuccess is returned.  if an
//  object cannot be immediately removed and waiting is not desired,
//  errOutOfObjects will be returned

template< class CObject, PfnOffsetOf OffsetOfIC >
inline typename CPool< CObject, OffsetOfIC >::ERR CPool< CObject, OffsetOfIC >::
ErrRemove( CObject** const ppobj, const INT cmsecTimeout, const BOOL fMRU )
{
    //  reserve an object for removal from the pool by acquiring a count on the
    //  object count semaphore.  if we get a count, we are allowed to remove an
    //  object from the pool.  acquire a count in the requested mode, i.e. wait
    //  or do not wait for a count

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

    //  we are now entitled to an object from the pool, so scan all buckets for
    //  an object to remove until we find one.  start with the bucket for the
    //  current CPU to reduce cache sloshing

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

    //  return the object

    AtomicIncrement( (LONG*)&m_cRemove );
    return ERR::errSuccess;
}

//  sets up the specified lock context in preparation for scanning all objects
//  in the pool
//
//  NOTE:  this function will acquire a lock that must eventually be released
//  via EndPoolScan()

template< class CObject, PfnOffsetOf OffsetOfIC >
inline void CPool< CObject, OffsetOfIC >::
BeginPoolScan( CLock* const plock )
{
    //  we will start in the first bucket

    plock->m_pbucket = m_rgbucket;

    //  lock this bucket

    plock->m_pbucket->m_crit.Enter();

    //  set out currency to be before the first object in this bucket

    plock->m_pobj       = NULL;
    plock->m_pobjNext   = plock->m_pbucket->m_il.PrevMost();
}

//  retrieves the next object in the pool locked by the specified lock context.
//  if there are no more objects to be scanned, errNoCurrentObject is returned

template< class CObject, PfnOffsetOf OffsetOfIC >
inline typename CPool< CObject, OffsetOfIC >::ERR CPool< CObject, OffsetOfIC >::
ErrGetNextObject( CLock* const plock, CObject** const ppobj )
{
    //  move to the next object in this bucket

    plock->m_pobj       =   plock->m_pobj ?
                                plock->m_pbucket->m_il.Next( plock->m_pobj ) :
                                plock->m_pobjNext;
    plock->m_pobjNext   = NULL;

    //  we still have no current object

    if ( !plock->m_pobj )
    {
        //  possibly advance to the next bucket

        _GetNextObject( plock );
    }

    //  return the current object, if any

    *ppobj = plock->m_pobj;
    return plock->m_pobj ? ERR::errSuccess : ERR::errNoCurrentObject;
}

//  removes the current object in the pool locked by the specified lock context
//  from the pool.  if there is no current object, errNoCurrentObject will be
//  returned

template< class CObject, PfnOffsetOf OffsetOfIC >
inline typename CPool< CObject, OffsetOfIC >::ERR CPool< CObject, OffsetOfIC >::
ErrRemoveCurrentObject( CLock* const plock )
{
    //  there is a current object and we can remove that object from the pool
    //
    //  NOTE:  we must get a count from the semaphore to remove an object from
    //  the pool

    if ( plock->m_pobj && m_semObjectCount.FTryAcquire() )
    {
        //  save the current object's next pointer so that we can recover our
        //  currency when it is deleted

        plock->m_pobjNext   = plock->m_pbucket->m_il.Next( plock->m_pobj );

        //  delete the current object from this bucket

        plock->m_pbucket->m_il.Remove( plock->m_pobj );

        //  set our currency to no current object

        plock->m_pobj = NULL;

        AtomicIncrement( (LONG*)&m_cRemove );
        return ERR::errSuccess;
    }

    //  there is no current object

    else
    {
        //  return no current object

        return ERR::errNoCurrentObject;
    }
}

//  ends the scan of all objects in the pool associated with the specified lock
//  context and releases all locks held

template< class CObject, PfnOffsetOf OffsetOfIC >
inline void CPool< CObject, OffsetOfIC >::
EndPoolScan( CLock* const plock )
{
    //  unlock the current bucket

    plock->m_pbucket->m_crit.Leave();
}

//  returns the current count of objects in the pool

template< class CObject, PfnOffsetOf OffsetOfIC >
inline DWORD CPool< CObject, OffsetOfIC >::
Cobject()
{
    //  the number of objects in the pool is equal to the available count on the
    //  object count semaphore

    return m_semObjectCount.CAvail();
}

//  returns the number of waiters for objects in the pool

template< class CObject, PfnOffsetOf OffsetOfIC >
inline DWORD CPool< CObject, OffsetOfIC >::
CWaiter()
{
    //  the number of waiters on the pool is equal to the waiter count on the
    //  object count semaphore

    return m_semObjectCount.CWait();
}

//  returns the number of times on object has been successfully inserted into the
//  pool

template< class CObject, PfnOffsetOf OffsetOfIC >
inline DWORD CPool< CObject, OffsetOfIC >::
CInsert()
{
    return m_cInsert;
}

//  returns the number of times on object has been successfully removed from the
//  pool

template< class CObject, PfnOffsetOf OffsetOfIC >
inline DWORD CPool< CObject, OffsetOfIC >::
CRemove()
{
    return m_cRemove;
}

//  returns the number of waits that occurred while removing objects from the
//  pool

template< class CObject, PfnOffsetOf OffsetOfIC >
inline DWORD CPool< CObject, OffsetOfIC >::
CRemoveWait()
{
    return m_cRemoveWait;
}

//  performs a move next that possibly goes to the next bucket.  we won't go to
//  the next bucket if we are already at the last bucket

template< class CObject, PfnOffsetOf OffsetOfIC >
inline void CPool< CObject, OffsetOfIC >::
_GetNextObject( CLock* const plock )
{
    //  set our currency to be after the last object in this bucket

    plock->m_pobj       = NULL;
    plock->m_pobjNext   = NULL;

    //  scan forward until we have a current object or we are at or beyond the
    //  last bucket

    while ( !plock->m_pobj && plock->m_pbucket < m_rgbucket + m_cbucket - 1 )
    {
        //  unlock the current bucket

        plock->m_pbucket->m_crit.Leave();

        //  advance to the next bucket

        plock->m_pbucket++;

        //  lock this bucket

        plock->m_pbucket->m_crit.Enter();

        //  set our currency to be the first object in this bucket

        plock->m_pobj       = plock->m_pbucket->m_il.PrevMost();
        plock->m_pobjNext   = NULL;
    }
}

//  calculate the address of the aligned block and store its offset (for free)

template< class CObject, PfnOffsetOf OffsetOfIC >
inline void* CPool< CObject, OffsetOfIC >::
_PvMEMIAlign( void* const pv, const size_t cbAlign )
{
    //  round up to the nearest cache line
    //  NOTE: this formula always forces an offset of at least 1 byte

    const ULONG_PTR ulp         = ULONG_PTR( pv );
    const ULONG_PTR ulpAligned  = ( ( ulp + cbAlign ) / cbAlign ) * cbAlign;
    const ULONG_PTR ulpOffset   = ulpAligned - ulp;

    COLLAssert( ulpOffset > 0 );
    COLLAssert( ulpOffset <= cbAlign );
    COLLAssert( ulpOffset == BYTE( ulpOffset ) );   //  must fit into a single BYTE

    //  store the offset

    BYTE *const pbAligned   = (BYTE*)ulpAligned;
    pbAligned[ -1 ]         = BYTE( ulpOffset );

    //  return the aligned block

    return (void*)pbAligned;
}

//  retrieve the offset of the real block being freed

template< class CObject, PfnOffsetOf OffsetOfIC >
inline void* CPool< CObject, OffsetOfIC >::
_PvMEMIUnalign( void* const pv )
{
    //  read the offset of the real block

    BYTE *const pbAligned   = (BYTE*)pv;
    const BYTE bOffset      = pbAligned[ -1 ];

    COLLAssert( bOffset > 0 );

    //  return the real unaligned block

    return (void*)( pbAligned - bOffset );
}

template< class CObject, PfnOffsetOf OffsetOfIC >
inline void* CPool< CObject, OffsetOfIC >::
_PvMEMAlloc( const size_t cbSize, const size_t cbAlign )
{
    // Prevent integer overflow.
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


////////////////////////////////////////////////////////////////////////////////
//  CArray
//
//  Implements a dynamically resized array of entries stored for efficient
//  iteration.
//
//  CEntry          = class representing entries stored in the array
//
//  NOTE:  the user must provide CEntry::CEntry() and CEntry::operator=()

template< class CEntry >
class CArray
{
    public:

        //  API Error Codes

        enum class ERR
        {
            errSuccess,
            errOutOfMemory,
        };

        //  Special constants.

        static const size_t iEntryNotFound = SIZE_MAX;

        //  Comparison function signatures.

        typedef INT (__cdecl *PfnCompare)( const CEntry* pentry1, const CEntry* pentry2 );

    protected:

        typedef INT (__cdecl *PfnCompareCRT)( const void* pv1, const void* pv2 );

    public:

        //  ctor/dtor.

        CArray();
        CArray( const size_t centryGrowth );
        ~CArray();

    public:

        //  Public methods.

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

        //  private state.

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

//  clones an existing array

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

//  sets the size of the array, growing the array if necessary

template< class CEntry >
inline typename CArray< CEntry >::ERR CArray< CEntry >::
ErrSetSize( const size_t centry )
{
    ERR err = ERR::errSuccess;

    COLLAssert( m_centry <= m_centryMax );

    if ( m_centryMax < centry )
    {

        //  rounding up to the next bucket of growth.

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

//  sets the capacity (max. size) of the array
//  asking for a size of zero entries causes the underlying memory backing up
//  the array to be deallocated

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

//  sets the default entry value

template< class CEntry >
inline void CArray< CEntry >::
SetEntryDefault( const CEntry& entry )
{
    m_entryDefault = entry;
    m_fEntryDefault = true;
}

//  sets the Nth entry of the array, growing the array if necessary

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

//  sets an existing entry of the array. WARNING: the array size and capacity
//  are not adjusted accordingly, so this method is only supposed to be used
//  to set existing elements.

template< class CEntry >
inline void CArray< CEntry >::
SetEntry( const CEntry* const pentry, const CEntry& entry )
{
    COLLAssert( pentry >= PEntry( 0 ) );
    COLLAssert( pentry < ( PEntry( 0 ) + m_centry ) );
    *const_cast< CEntry* >( pentry ) = entry;
}

//  returns the current size of the array

template< class CEntry >
inline size_t CArray< CEntry >::
Size() const
{
    return m_centry;
}

//  returns the current capacity of the array

template< class CEntry >
inline size_t CArray< CEntry >::
Capacity() const
{
    return m_centryMax;
}

//  returns the Nth entry of the array. the index cannot point beyond the known size of the array

template< class CEntry >
inline const CEntry& CArray< CEntry >::
Entry( const size_t ientry ) const
{
    return *PEntry( ientry );
}

//  returns the Nth entry of the array. the index cannot point beyond the known size of the array

template< class CEntry >
inline CEntry& CArray< CEntry >::
operator[]( const size_t ientry ) const
{
    return (CEntry&)Entry( ientry );
}

//  returns a pointer to the Nth entry of the array or NULL if the index is beyond the known size of the array

template< class CEntry >
inline const CEntry* CArray< CEntry >::
PEntry( const size_t ientry ) const
{
    return ientry < m_centry ? m_rgentry + ientry : NULL;
}

//  sorts the array using the CRT function qsort. 

template< class CEntry >
inline void CArray< CEntry >::
Sort( PfnCompare pfnCompare )
{
    if ( m_rgentry != NULL )
    {
        qsort( m_rgentry, m_centry, sizeof( CEntry ), (PfnCompareCRT)pfnCompare );
    }
}

//  same as Sort(), and also removes duplicate entries at the end.

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

    //  We can't fail because we are shrinking.
    COLLAssert( errSetSize == ERR::errSuccess );
}

//  searches for an element using a linear search algorithm. the context in the callback will be set
//  to the 'this' pointer. CRT's _lfind_s could perhaps be used here, but it takes an int as the number
//  of elements (which we store as a size_t), so we're better off doing it ourselves, as the algorithm is
//  trivial. this function returns CArray::iEntryNotFound if the element can't be found and the index in the array
//  otherwise

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

//  searches for an element using the CRT function bsearch.

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


////////////////////////////////////////////////////////////////////////////////
//  CTable
//
//  Implements a table of entries identified by a key and stored for efficient
//  lookup and iteration.  The keys need not be unique.
//
//  CKey            = class representing keys used to identify entries
//  CEntry          = class representing entries stored in the table
//
//  NOTE:  the user must implement the CKeyEntry::Cmp() functions and provide
//  CEntry::CEntry() and CEntry::operator=()

template< class CKey, class CEntry >
class CTable
{
    public:

        class CKeyEntry
            :   public CEntry
        {
            public:

                //  Cmp() return values:
                //
                //      < 0  this entry < specified entry / key
                //      = 0  this entry = specified entry / key
                //      > 0  this entry > specified entry / key

                INT Cmp( const CKeyEntry& keyentry ) const;
                INT Cmp( const CKey& key ) const;
        };

        //  API Error Codes

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

//  loads the table over an existing array of entries.  if the entries are not
//  in order then they will be sorted in place

template< class CKey, class CEntry >
inline CTable< CKey, CEntry >::
CTable( const size_t centry, CEntry* const rgentry, const BOOL fInOrder )
    :   m_arrayKeyEntry( centry, reinterpret_cast< CKeyEntry* >( rgentry ) )
{
    _ConfigureSearch( fInOrder );
}

//  loads an array of entries into the table.  additional entries may also be
//  loaded into the table via this function

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

//  clones an existing table

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

//  sets the table's array to be a clone of an existing array

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

//  updates an existing entry in the table as long as it doesn't change
//  that entry's position in the table

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

//  returns the current size of the table

template< class CKey, class CEntry >
inline size_t CTable< CKey, CEntry >::
Size() const
{
    return m_arrayKeyEntry.Size();
}

//   empties all the entries and releases internal memory

template< class CKey, class CEntry >
inline void CTable< CKey, CEntry >::
Clear()
{
    CArray< CKeyEntry >::ERR err = m_arrayKeyEntry.ErrSetCapacity(0);
    COLLAssert( err == CArray< CKeyEntry >::ERR::errSuccess );
}


//  returns a pointer to the Nth entry of the table or NULL if it is empty

template< class CKey, class CEntry >
inline const CEntry* CTable< CKey, CEntry >::
PEntry( const size_t ientry ) const
{
    return static_cast< const CEntry* >( m_arrayKeyEntry.PEntry( ientry ) );
}

//  the following group of functions return a pointer to an entry whose key
//  matches the specified key according to the given criteria:
//
//      Suffix    Description                 Positional bias
//
//      LT        less than                   high
//      LE        less than or equal to       low
//      EQ        equal to                    low
//      HI        equal to                    high
//      GE        greater than or equal to    high
//      GT        greater than                low
//
//  if no matching entry was found then NULL will be returned
//
//  "positional bias" means that the function will land on a matching entry
//  whose position is closest to the low / high end of the table

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
    //  quicksort cutoff

    const size_t    ckeyentryMin    = 32;

    //  partition stack (used to reduce levels of recursion)

    const size_t    cpartMax        = 16;
    size_t          cpart           = 0;
    struct
    {
        size_t  ikeyentryMin;
        size_t  ikeyentryMax;
    }           rgpart[ cpartMax ];

    //  current partition = partition passed in arguments

    size_t  ikeyentryMin    = ikeyentryMinIn;
    size_t  ikeyentryMax    = ikeyentryMaxIn;

    //  _QuickSort current partition

    for ( ; ; )
    {
        //  if this partition is small enough, insertion sort it

        if ( ikeyentryMax - ikeyentryMin < ckeyentryMin )
        {
            _InsertionSort( ikeyentryMin, ikeyentryMax );

            //  if there are no more partitions to sort, we're done

            if ( !cpart )
            {
                break;
            }

            //  pop a partition off the stack and make it the current partition

            ikeyentryMin    = rgpart[ --cpart ].ikeyentryMin;
            ikeyentryMax    = rgpart[ cpart ].ikeyentryMax;
            continue;
        }

        //  determine divisor by sorting the first, middle, and last entries and
        //  taking the resulting middle entry as the divisor

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

        //  sort large partition into two smaller partitions (<=, >)

        do  {
            //  advance past all entries <= the divisor

            while ( ikeyentryFirst <= ikeyentryLast &&
                    _Entry( ikeyentryFirst ).Cmp( _Entry( ikeyentryMin ) ) <= 0 )
            {
                ikeyentryFirst++;
            }

            //  advance past all entries > the divisor

            while ( ikeyentryFirst <= ikeyentryLast &&
                    _Entry( ikeyentryLast ).Cmp( _Entry( ikeyentryMin ) ) > 0 )
            {
                ikeyentryLast--;
            }

            //  if we have found a pair to swap, swap them and continue

            if ( ikeyentryFirst < ikeyentryLast )
            {
                _SwapEntry( ikeyentryFirst++, ikeyentryLast-- );
            }
        }
        while ( ikeyentryFirst <= ikeyentryLast );

        //  move the divisor to the end of the <= partition

        _SwapEntry( ikeyentryMin, ikeyentryLast );

        //  determine the limits of the smaller and larger sub-partitions

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

        //  push the larger sub-partition or recurse if the stack is full

        if ( cpart < cpartMax )
        {
            rgpart[ cpart ].ikeyentryMin    = ikeyentryLargeMin;
            rgpart[ cpart++ ].ikeyentryMax  = ikeyentryLargeMax;
        }
        else
        {
            _QuickSort( ikeyentryLargeMin, ikeyentryLargeMax );
        }

        //  set our current partition to be the smaller sub-partition

        ikeyentryMin    = ikeyentrySmallMin;
        ikeyentryMax    = ikeyentrySmallMax;
    }
}


////////////////////////////////////////////////////////////////////////////////
//  CStupidQueue
//
//  Implements a "dynamically" resized queue of entries implemented for ease of initial
//  implementation and for efficiency of memory overhead (O(1)), but requires a large
//  contiguous chunk of memory.  Enque = O(n log n).  Dequeue = O(1).  Memory usage 
//  greedy (i.e. does not give up memory).
//

class CStupidQueue {

    public:

        //  API Error Codes

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
    DWORD mGrowth = 2; // growth factor, we increase rapidly...

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

    // double now and set m_rg, to use PvElement() and avoid assert() therein.
    m_cAlloc = cNew;
    m_rg = rgNew;

    //  So we're not quite right yet ... move front part (from 0 to iTail+1) of 
    //  queue into newly allocated last half ... 
    //      Note slightly inefficient O(1.5n) instead of O(n), more cunning coding
    //      could avoid this...
    DWORD iT = ( m_iTail + 1 ) % ( m_cAlloc / mGrowth );
    if ( iT != 0 )
    {
        memcpy( PvElement( m_cAlloc / mGrowth ), PvElement( 0 ), iT * m_cbElement );
        m_iTail = ( m_cAlloc / mGrowth ) + m_iTail;
        COLLAssert( m_iTail < m_cAlloc );
    }
    else
    {
        // Means we broke while m_iHead was at the front, so no adjustment needed...
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
        m_cAlloc = 32;  // initial allocation of 32 entries ... why? why not?
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

    //  Increment the tail w/ wrap ...
    m_iTail = ( m_iTail + 1 ) % m_cAlloc;
    COLLAssert( m_iHead != m_iTail );
    //  Copy in the new element
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


//  These templates are used to cache objects/memory which are
//  expensive to allocate but are needed for only a short time.
//
//  GetCachedPtr() takes an array of pointers and returns the
//  first non-null value it finds, replacing the array entry
//  with null. If all the array entries are null then null is returned.
//
//  FCachePtr() tries to add the given pointer to the array.
//  If a free (i.e. null) entry is found the pointer is added
//  to the array at that location and the function returns true.
//  If all array entries are non-full then false is returned.
//
//  Concurrency is handled inside of the functions through the
//  use of interlocked operations.
//
//  Remember to free all members of the array when ESE terminates!
//
//  Array searches don't start at zero, instead the processor
//  number is used as the starting index. This should have the 
//  effect of improving processor locality if multiple objects
//  are being cached.

//  ================================================================
template<class P>
P GetCachedPtr( P * const array, const INT arraySize )
//  ================================================================
{
    C_ASSERT( sizeof( void* ) == sizeof( P ) );
    const INT startIndex = OSSyncGetCurrentProcessor() % arraySize;
    INT index = startIndex;
    do
    {
        const P ptr = ( P )AtomicExchangePointer( ( void ** )( &array[index] ), 0 );
        if( 0 != ptr )
        {
            // we found a cached value
            return ptr;
        }
        index = ( index + 1 ) % arraySize;
    } while( startIndex != index );
    return ( P )0;
}

//  ================================================================
template<class P>
bool FCachePtr( P ptr, P * const array, const INT arraySize )
//  ================================================================
{
    C_ASSERT( sizeof( void* ) == sizeof( P ) );
    COLLAssert( 0 != ptr );

    const INT startIndex = OSSyncGetCurrentProcessor() % arraySize;
    INT index = startIndex;
    do
    {
        if( 0 == AtomicCompareExchangePointer( ( void ** )( &array[index] ), 0, ptr ) )
        {
            // we cached the ptr in an empty slot
            return true;
        }
        index = ( index + 1 ) % arraySize;
    } while( startIndex != index );
    return false;
}

//  ================================================================
template<class Object, INT NumObjectsInPool>
class ObjectPool
//  ================================================================
//
//  Provides a cache of object allocations. Initially 
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

private: // not implemented
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

/*
SOMEONEPOST_CHECKIN RedBlackTree renames to match style of collection.hxx
 o InvasiveRedBlackTree -> CInvasiveRedBlackTree // irbt
 o InvasiveContext -> CInvasiveContext
 o CObject -> CEntry ala approximate index, etc ... though CInvasiveList uses CObject, so reconsider.
 o KEY -> CKey ... ala approximate index again.  Matches CObject better as well.
 o Change pnode to picNode
 o Change to m_picLeftNode / m_picRightNode ... instead of m_pnodeLeft / m_pnodeRight b/c it is a pointer to the pic I _think_?  Check.
    o Do note:
            const CObject* PnodeRoot() const { return NodeToCObject( m_pnodeRoot ); }
        o is calling the CObject * Pnode here ... I think in various places it's pic vs. pentry ...
 o consider
    static RedBlackTreeNode* ICToNode( typename BaseType::InvasiveContext* pic ) { return (RedBlackTreeNode*) ( ( (BYTE*) pic ) - OffsetOfRedBlackTreeIC() ); }
    static const RedBlackTreeNode* ICToNode( const typename BaseType::InvasiveContext* const pic ) { return ICToNode( const_cast<BaseType::InvasiveContext*>( pic ) ); }
 o try to move RedBlackTreeColor into InvasiveRedBlackTree
 o errDuplicateEntry -> errKeyDuplicate
 o fix comment style
 o try moving RedBlackTreeNode into CRedBlackTree as a sub-member
 o CObjectToNode -> PnodeOfCObject()
     o Or really PicOfCObject() maybe
     o Also see what CApproximateIndex & CInvasiveList handle this
 o AND fix CObject* NodeToCObject( INode* pnode ) const similarly.
 o AND fix the redblacktree.cxx test code as well.
 o Note this is inconsistent: const CObject* PnodeRoot() const { return NodeToCObject(m_pnodeRoot); }
    o Claiming to return Pnode, but returns CObject*

*/

////////////////////////////////////////////////////////////////////////////////
//  RedBlackTree
//
//  A templatized red-black tree
// The color of a node in a Red-Black tree
enum RedBlackTreeColor { Red, Black };

//  A Red-Black tree for the given key/data-entries.
//
//  Supports:
//    insert, delete, seek and in-order traversal
//  
//  The KEY class must implement:
//      bool operator==( const KEY& rhs ) const
//      bool operator<( const KEY& rhs ) const
//      bool operator>( const KEY& rhs ) const
//
template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
class InvasiveRedBlackTree
{
public:
    // Context for one node in a Red-Black tree.
    // The node has a key, some data, left and right
    // children, a color and a parent.
    class InvasiveContext
    {
    public:
        InvasiveContext() :
            m_key(),
            m_pnodeLeft( 0 ),
            m_pnodeRight( 0 ),
            m_color( Red )    // new nodes are red
        {
        }

        InvasiveContext( const KEY& key ) :
            m_key( key ),
            m_pnodeLeft( 0 ),
            m_pnodeRight( 0 ),
            m_color( Red )    // new nodes are red
        {
        }

        ~InvasiveContext()
        {
#ifdef DEBUG
            // Scrub the invasive context in DEBUG to catch bugs
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

    private:    // member variables
        KEY                 m_key;
        RedBlackTreeColor   m_color;
        InvasiveContext*    m_pnodeLeft;
        InvasiveContext*    m_pnodeRight;
    };

    // Type of the underlying node
    typedef InvasiveContext Node;

    // API Error Codes
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

    // Find the key/data combination that is nearest to the specified key
    ERR ErrFindNearest( const KEY& key, KEY* const pkeyFound, CObject** const ppObjectFound ) const;

    // Find the smallest key/data combination which is greater than or equal to the specified key.
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

protected:  // implementation methods

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

protected:  // private member variables
    Node * m_pnodeRoot;

private:    // not implemented
    InvasiveRedBlackTree( const InvasiveRedBlackTree& );
    InvasiveRedBlackTree& operator=( const InvasiveRedBlackTree& ); //  Disabled. Use .Move() instead.

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
    // Cannot be red with two red children
    if( m_pnodeLeft && m_pnodeRight )
    {
        COLLAssert( Black == Color() || Black == m_pnodeLeft->Color() || Black == m_pnodeRight->Color() );
    }
    // This is a left-leaning red-black tree so the right child can only
    // be red if the left one is as well.
    if( m_pnodeRight && Red == m_pnodeRight->Color() )
    {
        COLLAssert( m_pnodeLeft );
        COLLAssert( Red == m_pnodeLeft->Color() );
    }
}

    // Create a new empty tree
template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::InvasiveRedBlackTree() :
    m_pnodeRoot( 0 )
{
}

// Destroy the tree and all its nodes
template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::~InvasiveRedBlackTree()
{
    // COLLAssert to help detect InvasiveRedBlackTree related memory leak
    COLLAssert( FEmpty() );
    MakeEmpty();
}

// Look for the key in the tree and return its data.
// This can return:
//  errSuccess
//  errEntryNotFound
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

// Find the key/data combination that is nearest to the specified key
// This can return:
//  errSuccess
//  errEntryNotFound (if the tree is empty)
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

// Find the smallest key/data combination which is greater than or equal to the specified key.
// This can return:
//  errSuccess
//  errEntryNotFound (if there are no elements which are greater than or equal to the key specified, including the case in
//  which the tree is empty).
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

// Insert a new node into the tree containing the given
// key and data. The key must be unique. This can return
//  errSuccess
//  errOutOfMemory
//  errDuplicateEntry
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

// Delete the given key from the tree. This can return
//  errSuccess
//  errEntryNotFound
template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
typename InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::ERR
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::ErrDelete( const KEY& key, CObject** const ppObject /* = NULL */ )
{
    ERR err;
    Node* pnodeDeleted = NULL;

    // make sure the key is in the tree
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

// Get rid of all nodes in the tree
template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
void InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::MakeEmpty()
{
    MakeEmpty_( m_pnodeRoot );
    m_pnodeRoot = NULL;
    COLLAssert( FEmpty() );
}

// Get rid of all nodes in the tree
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

// Check all nodes in the tree
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

// Merge all the nodes from provided tree into this one.
// If the source tree can not be completely emptied, left overs are left there and errDuplicateEntry is returned.
template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
typename InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::ERR
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::ErrMerge( class InvasiveRedBlackTree< KEY, CObject, OffsetOfILE > * prbtSrc )
{
    InvasiveRedBlackTree<KEY, CObject, OffsetOfILE> rbtLeftOvers;

    // if empty, use fast path

    if ( FEmpty() )
    {
        // since target is empty, can use fast path of just moving the root.
        Move( prbtSrc );
    }

    // else (and perhaps there is a faster way than brute force off root, but) for now move each node ...

    Node * pnodeMove;
    while( ( pnodeMove = prbtSrc->m_pnodeRoot ) != NULL )
    {
        KEY keyMove = pnodeMove->Key();

        const ERR errDel = prbtSrc->ErrDelete( keyMove );
        COLLAssert( errDel == ERR::errSuccess );
        COLLAssert( pnodeMove->PnodeLeft() == NULL ); // I would hope (for safety's sake).
        COLLAssert( pnodeMove->PnodeRight() == NULL );

        const ERR errMove = ErrInsert( keyMove, NodeToCObject( pnodeMove ) );
        if ( errMove != ERR::errSuccess )
        {
            COLLAssert( errMove == ERR::errDuplicateEntry );
            const ERR errLeftOver = rbtLeftOvers.ErrInsert( keyMove, NodeToCObject( pnodeMove ) );
            COLLAssert( errLeftOver == ERR::errSuccess ); // already existed in tree (without dups) should be fine in left overs tree
        }
    }

    COLLAssert( prbtSrc->FEmpty() );

    if ( !rbtLeftOvers.FEmpty() )
    {
        prbtSrc->Move( &rbtLeftOvers );
        COLLAssert( rbtLeftOvers.FEmpty() );
        //  used to indicate there are left overs in prbtSrc.
        return ERR::errDuplicateEntry;
    }

    return ERR::errSuccess;
}

// Move the root (pointer) from one RedBlackTree to another (destroying / null'ing out the src root pointer)
template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
void InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::Move( class InvasiveRedBlackTree< KEY, CObject, OffsetOfILE > * prbtSrc )
{
    COLLAssert( FEmpty() ); // more of an Expected, and handled pretty well next.
    if ( !FEmpty() )
    {
        // We can merge non-empty trees too, but has error potential ... so use ErrMerge() instead.  And don't 
        // forget to handle errDuplicateEntry!
        COLLAssert( fFalse );
        return;
    }
    COLLAssert( this != prbtSrc || m_pnodeRoot == NULL ); // should not move onto self, but technically ok if empty.

    m_pnodeRoot = prbtSrc->m_pnodeRoot;
    prbtSrc->m_pnodeRoot = NULL;

    COLLAssert( prbtSrc->FEmpty() );
}

// Determine if the given key is present in the tree
template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
bool InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::FContainsKey_( const KEY& key )
{
    return NULL != Find_( m_pnodeRoot, key );
}

// Find a node with the given key. Returns NULL if
// the node can't be found.
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

// Find the minimum (i.e. leftmost) node in the subtree of pnode
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

// Insert the given node into the tree. The key of the node
// must be unique. This can return:
//  errSuccess
//  errOutOfMemory
//  errDuplicateEntry
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

// Left rotation of the node
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

// Right rotation of the node
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

// Flip the colors of the node and its children
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

// Delete the given key from the tree. The key must be present.
// Returns the deleted node in ppnodeDeleted.
//
// If the node being deleted isn't a leaf node then we swap it
// with the smallest node in its subtree and delete that node.
//
// Deleting the smallest node is done by moving red links down
// the tree with rotations and color inversions to ensure that
// the node being deleted is red. As red nodes conceptually form
// part of a larger (2,3,4) node we can safely remove the node
// without unbalancing the tree.
template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
typename InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::Node*
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::Delete_( Node * const pnode, const KEY& key, Node** const ppnodeDeleted )
{
    COLLAssert( pnode );

    Node * pnodeT = pnode;
    if( key < pnodeT->Key() )
    {
        // We know the key is in the tree and is less than the current node
        // so there must be a left node.
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
            // This is the node we want to delete
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
            // This is the key we want to delete. Swap this node with the minimum node
            // in the subtree and then delete the minimum node.
            //pnodeT->SetKey(pnodeMin->Key());
            //pnodeT->SetData(pnodeMin->Data());
            //pnodeT->SetRight(DeleteMinNode_(pnodeT->PnodeRight()));

            // This is the node we want to delete. Find the minimum node in the subtree,
            // remove the minimum node from its location and move it into the deleted node's position.
            Node* pnodeDelete = pnodeT;
            Node * const pnodeMin = PnodeMin_( pnodeT->PnodeRight() );
            Node* pnodeRight = RemoveMinNode_( pnodeT->PnodeRight() );

            // Swap the minimun node in (parent will be fixed on return from recursion)
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

// Removes the minimum (leftmost) node from the subtree given by pnode (doesn't scrub it)
template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
typename InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::Node*
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::RemoveMinNode_( Node * const pnode )
{
    Node * pnodeT = pnode;
    if( NULL == pnodeT->PnodeLeft() )
    {
        // In left-leaning red-black tree: if left child is empty, right child must be empty.
        // This is because, by definition, right child can only be black and the number of black nodes 
        // on any path in the right subtree is the same as that in left subtree which is zero.
        COLLAssert( pnodeT->PnodeRight() == NULL );
        return NULL;    // node removed from tree (upon return from recursion)
    }

    if( !FIsNonNullAndRed( pnodeT->PnodeLeft() ) && !FIsNonNullAndRed( pnodeT->PnodeLeft()->PnodeLeft() ) )
    {
        pnodeT = MoveRedLeft_( pnodeT );
    }

    pnodeT->SetLeft( RemoveMinNode_( pnodeT->PnodeLeft() ) );
    return Fixup_( pnodeT );
}

// Deletion helper method
template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
typename InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::Node*
InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::Fixup_( Node * const pnode )
{
    COLLAssert( pnode );

    Node * pnodeT = pnode;
    // Rotate right-leaning reds to be left-leaning
    if( FIsNonNullAndRed( pnodeT->PnodeRight() ) )
    {
        pnodeT = RotateLeft_( pnodeT );
    }
    // Red-Red pairs should be rotated so that both children become red
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

// Deletion helper method
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

// Deletion helper method
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

// Allocate a new node containing the given key and data.
template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
void InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::InitNode_( const KEY& key, Node* pnode )
{
    new( pnode ) Node( key );
}

// Free the given node.
template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
void InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::ScrubNode_( Node * const pnode )
{
    // Scrub, so that we don't leak this state upon reuse
    pnode->~Node();
}

#ifdef DEBUG
// Print the given node (recursively).
template<class KEY, class CObject, PfnOffsetOf OffsetOfILE>
INT InvasiveRedBlackTree<KEY, CObject, OffsetOfILE>::CDepth_( const Node * const pic ) const
{
    if( pic == NULL )
    {
        return 0;
    }
    return 1 + max( CDepth_( pic->PnodeLeft() ), CDepth_( pic->PnodeRight() ) );
}

// Print the given node (recursively).
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

// Print the given tree.
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
#endif // DEBUG

////////////////////////////////////////////////////////////////////////////////
//  RedBlackTree (non-invasive)
//
//  A templatized red-black tree that is a thin Facade over InvasiveRedBlackTree.
//  Provides a simpler interface when you don't need to deal with invasive contexts.

// One node in a Red-Black tree. The node has
// an invasive context for InvasiveRedBlackTree
// and user-provided DATA.
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

private:    // member variables
    typename BaseType::InvasiveContext  m_icRedBlackTree;
    DATA                    m_data;
};

// A Red-Black tree for the given key/data. Supports
// insert, delete, seek and in-order traversal
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

    // Find the key/data combination that is nearest to the specified key
    ERR ErrFindNearest( const KEY& key, KEY * const pkeyFound, DATA * const pdataFound = NULL ) const;

    // Find the smallest key/data combination which is greater than or equal to the specified key.
    ERR ErrFindGreaterThanOrEqual( const KEY& key, KEY * const pkeyFound, DATA * const pdataFound = NULL ) const;

    ERR ErrInsert( const KEY& key, const DATA& data );
    ERR ErrDelete( const KEY& key );
    void MakeEmpty();

    bool FEmpty() const                             { return m_irbtBase.FEmpty(); }
    const Node* PnodeRoot() const                   { return m_irbtBase.PnodeRoot(); }
    const Node* FindNode( const KEY& key ) const    { return m_irbtBase.FindNode( key ); }
    void AssertValid() const                        { m_irbtBase.AssertValid(); }

private:    // implementation methods
    Node* PnodeAlloc_( const DATA& data );
    void Free_( Node * const pnode );

    void MakeEmpty_( Node * const pnode );
    void AssertValid_( const Node * const pnode ) const;

private:    // not implemented
    CRedBlackTree( const CRedBlackTree& );
    CRedBlackTree& operator=( const CRedBlackTree& );

private:    // member variables
    BaseType m_irbtBase;
};

// Destroy the tree and all its nodes
template<class KEY, class DATA>
CRedBlackTree<KEY, DATA>::~CRedBlackTree()
{
    // COLLAssert to help detect CRedBlackTree related memory leak
    COLLAssert( FEmpty() );
    MakeEmpty();
}

// Look for the key in the tree and return its data.
// This can return:
//  errSuccess
//  errEntryNotFound
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

// Find the key/data combination that is nearest to the specified key
// This can return:
//  errSuccess
//  errEntryNotFound (if the tree is empty)
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

// Find the smallest key/data combination which is greater than or equal to the specified key.
// This can return:
//  errSuccess
//  errEntryNotFound (if there are no elements which are greater than or equal to the key specified, including the case in
//  which the tree is empty).
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

// Insert a new node into the tree containing the given
// key and data. The key must be unique. This can return
//  errSuccess
//  errOutOfMemory
//  errDuplicateEntry
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

// Delete the given key from the tree. This can return
//  errSuccess
//  errEntryNotFound
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

// Get rid of all nodes in the tree
// Overrides BaseType::MakeEmpty() because it needs to free nodes
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

        m_irbtBase.MakeEmpty(); // just the root node left in the tree
        Free_( pnodeRoot );
    }

    COLLAssert( FEmpty() );
}

// Get rid of all nodes in the tree
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

// Allocate a new node containing the given key and data.
template<class KEY, class DATA>
CRedBlackTreeNode<KEY, DATA> * CRedBlackTree<KEY, DATA>::PnodeAlloc_( const DATA& data )
{
    return new CRedBlackTreeNode<KEY, DATA>( data );
}

// Free the given node.
template<class KEY, class DATA>
void CRedBlackTree<KEY, DATA>::Free_( Node * const pnode )
{
    delete pnode;
}


////////////////////////////////////////////////////////////////////////////////
//  Round-robin buffer helpers
//
//  Round-robin buffers are fairly common, these functions allow the equivalent of ++ and -- without worrying about
//  the boundary conditions ...

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

#endif  //  _COLLECTION_HXX_INCLUDED

