// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "stdafx.h"

#pragma once

namespace Internal
{
    namespace Ese
    {
        namespace BlockCache
        {
            namespace Interop
            {
                template< class TM, class TN, class TW >
                public ref class CachedBlockSlabManagerBase : Base<TM, TN, TW>, ICachedBlockSlabManager
                {
                    public:

                        CachedBlockSlabManagerBase( TM^ cbsm ) : Base( cbsm ) { }

                        CachedBlockSlabManagerBase( TN** const ppcbsm ) : Base( ppcbsm ) {}

                        CachedBlockSlabManagerBase( TN* const pcbsm ) : Base( pcbsm ) {}

                    public:

                        virtual ICachedBlockSlab^ GetSlab( CachedBlockId^ cachedBlockId );

                        virtual bool IsSlabForCachedBlock( CachedBlockSlab^ slab, CachedBlockId^ cachedBlockId );

                        virtual ICachedBlockSlab^ GetSlab(  UInt64 offsetInBytes, 
                                                            bool wait, 
                                                            bool ignoreVerificationErrors, 
                                                            [Out] EsentErrorException^% ex );

                        virtual void VisitSlabs( ICachedBlockSlabManager::VisitSlab^ visitSlab );
                };

                template<class TM, class TN, class TW>
                inline ICachedBlockSlab^ CachedBlockSlabManagerBase<TM, TN, TW>::GetSlab( CachedBlockId^ cachedBlockId )
                {
                    ERR                 err     = JET_errSuccess;
                    ::ICachedBlockSlab* pcbs    = NULL;

                    Call( Pi->ErrGetSlab( *cachedBlockId->Pcbid(), &pcbs ) );

                    return pcbs ? gcnew CachedBlockSlab( &pcbs ) : nullptr;

                HandleError:
                    delete pcbs;
                    throw EseException( err );
                }

                template<class TM, class TN, class TW>
                inline bool CachedBlockSlabManagerBase<TM, TN, TW>::IsSlabForCachedBlock(
                    CachedBlockSlab^ slab,
                    CachedBlockId^ cachedBlockId )
                {
                    ERR     err                     = JET_errSuccess;
                    BOOL    fIsSlabForCachedBlock   = fFalse;

                    Call( Pi->ErrIsSlabForCachedBlock( slab == nullptr ? NULL : slab->Pi,
                                                        *cachedBlockId->Pcbid(),
                                                        &fIsSlabForCachedBlock ) );

                    return fIsSlabForCachedBlock;

                HandleError:
                    throw EseException( err );
                }

                template<class TM, class TN, class TW>
                inline ICachedBlockSlab^ CachedBlockSlabManagerBase<TM, TN, TW>::GetSlab(
                    UInt64 offsetInBytes,
                    bool wait,
                    bool ignoreVerificationErrors, 
                    [Out] EsentErrorException^% ex )
                {
                    ERR                 err     = JET_errSuccess;
                    ::ICachedBlockSlab* pcbs    = NULL;

                    ex = nullptr;

                    Call( Pi->ErrGetSlab(   offsetInBytes, 
                                            wait ? fTrue : fFalse, 
                                            ignoreVerificationErrors ? fTrue : fFalse, 
                                            &pcbs ) );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        if ( !pcbs )
                        {
                            throw EseException( err );
                        }
                        else
                        {
                            ex = EseException( err );
                        }
                    }
                    return pcbs ? gcnew CachedBlockSlab( &pcbs ) : nullptr;
                }

                template<class TM, class TN, class TW>
                inline void CachedBlockSlabManagerBase<TM, TN, TW>::VisitSlabs( ICachedBlockSlabManager::VisitSlab^ visitSlab )
                {
                    ERR                 err                 = JET_errSuccess;
                    VisitSlabInverse^   visitSlabInverse    = nullptr;

                    if ( visitSlab != nullptr )
                    {
                        visitSlabInverse = gcnew VisitSlabInverse( visitSlab );
                    }

                    Call( Pi->ErrVisitSlabs(    visitSlabInverse == nullptr ? NULL : visitSlabInverse->PfnVisitSlab,
                                                visitSlabInverse == nullptr ? NULL : visitSlabInverse->KeyVisitSlab ) );

                HandleError:
                    delete visitSlabInverse;
                    if ( err < JET_errSuccess )
                    {
                        throw EseException( err );
                    }
                }
            }
        }
    }
}