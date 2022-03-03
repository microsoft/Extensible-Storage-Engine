// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  A cached file table entry for the hashed LRUK cache.

template<class I>
class CHashedLRUKCachedFileTableEntry  //  cfte
    :   public CCachedFileTableEntryBase
{
    public:

        class CIORangeLockBase;

        CHashedLRUKCachedFileTableEntry(    _In_ ICache* const      pc,
                                            _In_ const VolumeId     volumeid,
                                            _In_ const FileId       fileid,
                                            _In_ const FileSerial   fileserial )
            :   CCachedFileTableEntryBase( pc, volumeid, fileid, fileserial ),
                m_critIORangeLock( CLockBasicInfo( CSyncBasicInfo( "CHashedLRUKCachedFileTableEntry<I>::m_critIORangeLock" ), rankIORangeLock, 0 ) ),
                m_critCachedFileSparseMap( CLockBasicInfo( CSyncBasicInfo( "CHashedLRUKCachedFileTableEntry<I>::m_critCachedFileSparseMap" ), rankCachedFileSparseMap, 0 ) )
        {
        }

        ~CHashedLRUKCachedFileTableEntry()
        {
            if ( Pff() )
            {
                Pff()->SetNoFlushNeeded();
            }
        }

        BOOL FSparse() const { return m_arraysparseseg.Size(); }

        BOOL FSparse( _In_ const QWORD ib, _In_ const QWORD cb );

        ERR ErrLoadSparseMap();

        ERR ErrUpdateSparseMapForInvalidate( _In_ const COffsets invalidate );
        ERR ErrUpdateSparseMapForSetSize( _In_ const COffsets invalidate );
        ERR ErrUpdateSparseMapForTrim( _In_ const COffsets invalidate );
        ERR ErrUpdateSparseMapForWrite( _In_ const COffsets write );

        BOOL FTryRequestIORangeLock( _In_ CIORangeLockBase* const piorl, _In_ const BOOL fWait = fTrue );

        void AcquireIORangeLock( _In_ CIORangeLockBase* const piorl )
        {
            //  request the IO range lock

            const BOOL fIORangeLockPending = FTryRequestIORangeLock( piorl );

            //  the request must succeed

            EnforceSz( fIORangeLockPending, "AcquireIORangeLock_pending" );

            //  wait to acquire the IO range lock

            piorl->Wait();

            //  the IO range lock must be acquired

            EnforceSz( piorl->FLocked(), "AcquireIORangeLock_locked" );
        }

    public:

        //  An IO Range Lock

        class CIORangeLockBase  //  iorl
        {
            public:

                CIORangeLockBase( _In_ CHashedLRUKCachedFileTableEntry<I>* const pcfte )
                    :   m_pcfte( pcfte ),
                        m_msig( CSyncBasicInfo( "CHashedLRUKCachedFileTableEntry<I>::CIORangeLockBase::m_msig" ) ),
                        m_fLocked( fFalse )
                {
                }

                CHashedLRUKCachedFileTableEntry<I>* const Pcfte() const { return m_pcfte; }
                virtual QWORD QwContext() const { return 0; }
                virtual COffsets Offsets() const = 0;
                BOOL FLocked() const { return m_fLocked; }

                void Wait()
                {
                    m_msig.Wait();
                }

                virtual void Grant()
                {
                    if ( !AtomicCompareExchange( (LONG*)&m_fLocked, fFalse, fTrue ) )
                    {
                        OSTrace(    JET_tracetagBlockCacheOperations,
                                    OSFormat(   "C=%s R=0x%016I64x F=%s IORangeLock ib=%llu cb=%llu Grant",
                                                OSFormatFileId( m_pcfte->Pc() ),
                                                QwContext(),
                                                OSFormatFileId( Pcfte()->Pff() ),
                                                Offsets().IbStart(),
                                                Offsets().Cb() ) );

                        m_msig.Set();
                    }
                }

                void Release()
                {
                    if ( AtomicCompareExchange( (LONG*)&m_fLocked, fTrue, fFalse ) )
                    {
                        OSTrace(    JET_tracetagBlockCacheOperations,
                                    OSFormat(   "C=%s R=0x%016I64x F=%s IORangeLock ib=%llu cb=%llu Release",
                                                OSFormatFileId( m_pcfte->Pc() ),
                                                QwContext(),
                                                OSFormatFileId( Pcfte()->Pff() ),
                                                Offsets().IbStart(),
                                                Offsets().Cb() ) );

                        Pcfte()->ReleaseIORangeLock( this );
                    }
                }

                static SIZE_T OffsetOfILE() { return OffsetOf( CIORangeLockBase, m_ile ); }
                static SIZE_T OffsetOfWaitQueue() { return OffsetOf( CIORangeLockBase, m_ileWaitQueue ); }

                CInvasiveList<CIORangeLockBase, OffsetOfWaitQueue>& IlWaitQueue() { return m_ilWaitQueue; }

            private:

                CHashedLRUKCachedFileTableEntry<I>* const                               m_pcfte;
                typename CInvasiveList<CIORangeLockBase, OffsetOfILE>::CElement         m_ile;
                CManualResetSignal                                                      m_msig;
                volatile BOOL                                                           m_fLocked;
                CInvasiveList<CIORangeLockBase, OffsetOfWaitQueue>                      m_ilWaitQueue;
                typename CInvasiveList<CIORangeLockBase, OffsetOfWaitQueue>::CElement   m_ileWaitQueue;
        };

        class CIORangeLock : public CIORangeLockBase
        {
            public:

                CIORangeLock(   _In_ CHashedLRUKCachedFileTableEntry<I>* const  pcfte,
                                _In_ const COffsets                             offsets )
                    :   CIORangeLockBase( pcfte ),
                        m_offsets( offsets )
                {
                }

                COffsets Offsets() const override { return m_offsets; }

            private:

                const COffsets m_offsets;
        };

    protected:

        ERR ErrOpenCachedFile(  _In_ IFileSystemFilter* const           pfsf,
                                _In_ IFileIdentification* const         pfident,
                                _In_ IBlockCacheConfiguration* const    pbcconfig,
                                _In_ IFileFilter* const                 pffCaching ) override;

    private:

        CIORangeLockBase* PiorlGetConflictingIORangeLock( _In_ CIORangeLockBase* const piorl );
        void ReleaseIORangeLock( _In_ CIORangeLockBase* const piorl );

        BOOL FConflicting( _In_ CIORangeLockBase* const piorlA, _In_ CIORangeLockBase* const piorlB );

    private:

        CCriticalSection                                                m_critIORangeLock;
        CInvasiveList<CIORangeLockBase, CIORangeLockBase::OffsetOfILE>  m_ilIORangeLock;
        CCriticalSection                                                m_critCachedFileSparseMap;
        CArray<SparseFileSegment>                                       m_arraysparseseg;
};


template<class I>
ERR CHashedLRUKCachedFileTableEntry<I>::ErrOpenCachedFile(  _In_ IFileSystemFilter* const           pfsf,
                                                            _In_ IFileIdentification* const         pfident,
                                                            _In_ IBlockCacheConfiguration* const    pbcconfig,
                                                            _In_ IFileFilter* const                 pffCaching )
{
    ERR err = JET_errSuccess;

    //  call the base implementation first

    Call( CCachedFileTableEntryBase::ErrOpenCachedFile( pfsf, pfident, pbcconfig, pffCaching ) );

    //  cache the sparse extents of this file, if any

    Call( ErrLoadSparseMap() );

HandleError:
    return err;
}

template<class I>
BOOL CHashedLRUKCachedFileTableEntry<I>::FSparse( _In_ const QWORD ib, _In_ const QWORD cb )
{
    BOOL            fResult         = fFalse;
    const COffsets  offsetsToTest( ib, ib + ( cb - 1 ) );

    if ( FSparse() )
    {
        m_critCachedFileSparseMap.Enter();

        for ( int i = 0; !fResult && i < m_arraysparseseg.Size(); i++ )
        {
            const SparseFileSegment sparseseg       = m_arraysparseseg[ i ];
            const COffsets          offsetsSparse( sparseseg.ibFirst, sparseseg.ibLast );

            fResult = fResult || offsetsSparse.FContains( offsetsToTest );
        }

        m_critCachedFileSparseMap.Leave();
    }

    return fResult;
}

template<class I>
ERR CHashedLRUKCachedFileTableEntry<I>::ErrLoadSparseMap()
{
    ERR                         err             = JET_errSuccess;
    BOOL                        fLeave          = fTrue;
    QWORD                       cb              = 0;
    CArray<SparseFileSegment>   arraysparseseg;

    m_critCachedFileSparseMap.Enter();
    fLeave = fTrue;

    Call( Pff()->ErrSize( &cb, IFileAPI::filesizeLogical ) );

    if ( cb )
    {
        Call( ErrIORetrieveSparseSegmentsInRegion( Pff(), 0, cb - 1, &arraysparseseg ) );
    }

    Call( ErrToErr<CArray<SparseFileSegment>>( m_arraysparseseg.ErrClone( arraysparseseg ) ) );

HandleError:
    if ( fLeave )
    {
        m_critCachedFileSparseMap.Leave();
    }
    return err;
}

template<class I>
ERR CHashedLRUKCachedFileTableEntry<I>::ErrUpdateSparseMapForInvalidate( _In_ const COffsets invalidate )
{
    ERR err = JET_errSuccess;

    if ( invalidate.IbEnd() == qwMax )
    {
        Call( ErrUpdateSparseMapForSetSize( invalidate ) );
    }
    else
    {
        Call( ErrUpdateSparseMapForTrim( invalidate ) );
    }

HandleError:
    return err;
}

template<class I>
ERR CHashedLRUKCachedFileTableEntry<I>::ErrUpdateSparseMapForSetSize( _In_ const COffsets invalidate )
{
    ERR                         err             = JET_errSuccess;
    BOOL                        fLeave          = fTrue;
    CArray<SparseFileSegment>   arraysparseseg;

    m_critCachedFileSparseMap.Enter();
    fLeave = fTrue;

    //  remove all the sparse segments in the invalidation range which corresponds to the new file size configured by
    //  IFileAPI::ErrSetSize to the max possible offset.  these sparse ranges must be deleted because if we grow back
    //  into this offset range then the file will not be sparse.  COSFile::ErrSetSize forces newly allocated offset
    //  ranges in the file to be backed by storage so that insufficient storage errors are encountered only by the call
    //  to ErrSetSize and not by later calls to IFileAPI::ErrIOWrite.

    for ( size_t i = 0; i < m_arraysparseseg.Size(); i++ )
    {
        if ( m_arraysparseseg[ i ].ibLast < invalidate.IbStart() )
        {
            Call( ErrToErr<CArray<SparseFileSegment>>( arraysparseseg.ErrSetEntry( arraysparseseg.Size(), m_arraysparseseg[ i ] ) ) );
        }
        else if ( m_arraysparseseg[ i ].ibFirst < invalidate.IbStart() )
        {
            SparseFileSegment sparsesegNew;
            sparsesegNew.ibFirst = m_arraysparseseg[ i ].ibFirst;
            sparsesegNew.ibLast = invalidate.IbStart() - 1;
            Call( ErrToErr<CArray<SparseFileSegment>>( arraysparseseg.ErrSetEntry( arraysparseseg.Size(), sparsesegNew ) ) );
        }
    }

    Call( ErrToErr<CArray<SparseFileSegment>>( m_arraysparseseg.ErrClone( arraysparseseg ) ) );

HandleError:
    if ( fLeave )
    {
        m_critCachedFileSparseMap.Leave();
    }
    return err;
}

template<class I>
ERR CHashedLRUKCachedFileTableEntry<I>::ErrUpdateSparseMapForTrim( _In_ const COffsets invalidate )
{
    ERR                         err             = JET_errSuccess;
    BOOL                        fLeave          = fTrue;
    CArray<SparseFileSegment>   arraysparseseg;
    size_t                      i               = 0;
    SparseFileSegment           sparsesegNew;

    m_critCachedFileSparseMap.Enter();
    fLeave = fTrue;

    for ( i = 0; i < m_arraysparseseg.Size() && m_arraysparseseg[ i ].ibLast < invalidate.IbStart(); i++ )
    {
        Call( ErrToErr<CArray<SparseFileSegment>>( arraysparseseg.ErrSetEntry( arraysparseseg.Size(), m_arraysparseseg[ i ] ) ) );
    }

    sparsesegNew.ibFirst = (    i < m_arraysparseseg.Size() &&
                                m_arraysparseseg[ i ].ibFirst <= invalidate.IbStart() &&
                                invalidate.IbStart() <= m_arraysparseseg[ i ].ibLast ) ?
                            m_arraysparseseg[ i ].ibFirst :
                            invalidate.IbStart();

    for ( ; i < m_arraysparseseg.Size() && m_arraysparseseg[ i ].ibLast < invalidate.IbEnd(); i++ )
    {
    }

    sparsesegNew.ibLast = ( i < m_arraysparseseg.Size() &&
                            m_arraysparseseg[ i ].ibFirst <= invalidate.IbEnd() &&
                            invalidate.IbEnd() <= m_arraysparseseg[ i ].ibLast ) ?
                            m_arraysparseseg[ i ].ibLast :
                            invalidate.IbEnd();

    Call( ErrToErr<CArray<SparseFileSegment>>( arraysparseseg.ErrSetEntry( arraysparseseg.Size(), sparsesegNew ) ) );

    for ( ; i < m_arraysparseseg.Size(); i++ )
    {
        if ( invalidate.IbEnd() < m_arraysparseseg[ i ].ibFirst )
        {
            Call( ErrToErr<CArray<SparseFileSegment>>( arraysparseseg.ErrSetEntry( arraysparseseg.Size(), m_arraysparseseg[ i ] ) ) );
        }
    }

    Call( ErrToErr<CArray<SparseFileSegment>>( m_arraysparseseg.ErrClone( arraysparseseg ) ) );

HandleError:
    if ( fLeave )
    {
        m_critCachedFileSparseMap.Leave();
    }
    return err;
}

template<class I>
ERR CHashedLRUKCachedFileTableEntry<I>::ErrUpdateSparseMapForWrite( _In_ const COffsets write )
{
    ERR                         err             = JET_errSuccess;
    BOOL                        fLeave          = fTrue;
    CArray<SparseFileSegment>   arraysparseseg;
    SparseFileSegment           sparsesegNew;

    m_critCachedFileSparseMap.Enter();
    fLeave = fTrue;

    for ( int i = 0; i < m_arraysparseseg.Size(); i++ )
    {
        SparseFileSegment sparseseg = m_arraysparseseg[ i ];

        if ( sparseseg.ibLast < write.IbStart() )
        {
            Call( ErrToErr<CArray<SparseFileSegment>>( arraysparseseg.ErrSetEntry( arraysparseseg.Size(), sparseseg ) ) );
        }
        else if ( write.IbEnd() < sparseseg.ibFirst )
        {
            Call( ErrToErr<CArray<SparseFileSegment>>( arraysparseseg.ErrSetEntry( arraysparseseg.Size(), sparseseg ) ) );
        }
        else if ( write.IbStart() <= sparseseg.ibFirst && sparseseg.ibLast <= write.IbEnd() )
        {
        }
        else if ( sparseseg.ibFirst < write.IbStart() && write.IbEnd() < sparseseg.ibLast )
        {
            sparsesegNew.ibFirst = sparseseg.ibFirst;
            sparsesegNew.ibLast = write.IbStart() - 1;
            Call( ErrToErr<CArray<SparseFileSegment>>( arraysparseseg.ErrSetEntry( arraysparseseg.Size(), sparsesegNew ) ) );

            sparsesegNew.ibFirst = write.IbEnd() + 1;
            sparsesegNew.ibLast = sparseseg.ibLast;
            Call( ErrToErr<CArray<SparseFileSegment>>( arraysparseseg.ErrSetEntry( arraysparseseg.Size(), sparsesegNew ) ) );
        }
        else if ( write.IbStart() < sparseseg.ibLast && write.IbEnd() >= sparseseg.ibLast )
        {
            sparsesegNew.ibFirst = sparseseg.ibFirst;
            sparsesegNew.ibLast = write.IbStart() - 1;
            Call( ErrToErr<CArray<SparseFileSegment>>( arraysparseseg.ErrSetEntry( arraysparseseg.Size(), sparsesegNew ) ) );
        }
        else if ( write.IbStart() < sparseseg.ibFirst && write.IbEnd() >= sparseseg.ibFirst )
        {
            sparsesegNew.ibFirst = write.IbEnd() + 1;
            sparsesegNew.ibLast = sparseseg.ibLast;
            Call( ErrToErr<CArray<SparseFileSegment>>( arraysparseseg.ErrSetEntry( arraysparseseg.Size(), sparsesegNew ) ) );
        }
    }

    Call( ErrToErr<CArray<SparseFileSegment>>( m_arraysparseseg.ErrClone( arraysparseseg ) ) );

HandleError:
    if ( fLeave )
    {
        m_critCachedFileSparseMap.Leave();
    }
    return err;
}

template<class I>
BOOL CHashedLRUKCachedFileTableEntry<I>::FTryRequestIORangeLock(    _In_ CIORangeLockBase* const    piorl,
                                                                    _In_ const BOOL                 fWait )
{
    CIORangeLockBase*   piorlConflict   = NULL;
    BOOL                fAcquired       = fFalse;

    m_critIORangeLock.Enter();

    //  this IO range lock had better be referring to this cached file

    Assert( piorl );
    Assert( piorl->Pcfte() == this );

    //  this IO range lock had better not already be an owner or waiter for an IO range lock

    Assert( !m_ilIORangeLock.FMember( piorl ) );
    Assert( piorl->IlWaitQueue().FEmpty() );

    //  insert the IO range lock as the newest IO range lock

    m_ilIORangeLock.InsertAsNextMost( piorl );

    //  find any IO range lock that conflicts with this IO range lock

    piorlConflict = PiorlGetConflictingIORangeLock( piorl );

    //  if we conflicted with another IO range lock and we want to wait then register to wait for the conflicting IO
    //  range lock to be released.  if we cannot wait then remove the proposed IO range lock from our list

    if ( piorlConflict )
    {
        if ( fWait )
        {
            piorlConflict->IlWaitQueue().InsertAsNextMost( piorl );
        }
        else
        {
            m_ilIORangeLock.Remove( piorl );
        }
    }

    //  if we did not conflict with another IO range lock then we now own it

    else
    {
        piorl->Grant();
        fAcquired = fTrue;
    }

    m_critIORangeLock.Leave();

    //  indicate if we have requested the IO range lock

    return fAcquired || fWait;
}

template<class I>
typename CHashedLRUKCachedFileTableEntry<I>::CIORangeLockBase* CHashedLRUKCachedFileTableEntry<I>::PiorlGetConflictingIORangeLock( _In_ CIORangeLockBase* const piorl )
{
    CIORangeLockBase* piorlConflict = NULL;

    Assert( m_critIORangeLock.FOwner() );

    for (   piorlConflict = m_ilIORangeLock.PrevMost();
            piorlConflict != piorl && !FConflicting( piorlConflict, piorl );
            piorlConflict = m_ilIORangeLock.Next( piorlConflict ) )
    {
    }

    return piorlConflict == piorl ? NULL : piorlConflict;
}

template<class I>
void CHashedLRUKCachedFileTableEntry<I>::ReleaseIORangeLock( _In_ CIORangeLockBase* const piorl )
{
    m_critIORangeLock.Enter();

    //  this IO range lock had better already own the IO range lock on this cached file

    Assert( piorl );
    Assert( piorl->Pcfte() == this );
    Assert( m_ilIORangeLock.FMember( piorl ) );
    Assert( !PiorlGetConflictingIORangeLock( piorl ) );

    //  remove this IO range lock from the IO range lock list

    m_ilIORangeLock.Remove( piorl );

    //  grant any IO range locks that were blocked by this one

    while ( CIORangeLockBase* piorlWaiter = piorl->IlWaitQueue().PrevMost() )
    {
        CIORangeLockBase* piorlConflict = NULL;

        //  remove the released IO range lock from the wait queue

        piorl->IlWaitQueue().Remove( piorlWaiter );

        //  find any IO range lock that conflicts with this IO range lock

        piorlConflict = PiorlGetConflictingIORangeLock( piorlWaiter );

        //  if we conflicted with another IO range lock and we want to wait then register to wait for the conflicting IO
        //  range lock to be released.  otherwise, grant the IO range lock

        if ( piorlConflict )
        {
            piorlConflict->IlWaitQueue().InsertAsNextMost( piorlWaiter );
        }
        else
        {
            piorlWaiter->Grant();
        }
    }

    m_critIORangeLock.Leave();
}

template<class I>
BOOL CHashedLRUKCachedFileTableEntry<I>::FConflicting(  _In_ CIORangeLockBase* const    piorlA,
                                                        _In_ CIORangeLockBase* const    piorlB )
{
    //  IOs with overlapping offset ranges conflict

    if ( piorlA->Offsets().FOverlaps( piorlB->Offsets() ) )
    {
        return fTrue;
    }

    return fFalse;
}
