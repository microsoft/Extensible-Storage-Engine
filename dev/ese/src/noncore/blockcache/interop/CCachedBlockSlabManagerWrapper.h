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
                template< class TM, class TN >
                class CCachedBlockSlabManagerWrapper : public CWrapper<TM, TN>
                {
                    public:

                        CCachedBlockSlabManagerWrapper( TM^ cbsm ) : CWrapper( cbsm ) { }

                    public:

                        ERR ErrGetSlab( _In_    const ::CCachedBlockId&     cbid,
                                        _Out_   ::ICachedBlockSlab** const  ppcbs ) override;

                        ERR ErrIsSlabForCachedBlock(    _In_    ::ICachedBlockSlab* const   pcbs,
                                                        _In_    const ::CCachedBlockId&     cbid,
                                                        _Out_   BOOL* const                 pfIsSlabForCachedBlock ) override;

                        ERR ErrGetSlab( _In_    const QWORD                 ib,
                                        _In_    const BOOL                  fWait,
                                        _In_    const BOOL                  fIgnoreVerificationErrors,
                                        _Out_   ::ICachedBlockSlab** const  ppcbs ) override;

                        ERR ErrVisitSlabs(  _In_ const ::ICachedBlockSlabManager::PfnVisitSlab  pfnVisitSlab,
                                            _In_ const DWORD_PTR                                keyVisitSlab ) override;
                };

                template<class TM, class TN>
                inline ERR CCachedBlockSlabManagerWrapper<TM, TN>::ErrGetSlab(  _In_    const ::CCachedBlockId&     cbid,
                                                                                _Out_   ::ICachedBlockSlab** const  ppcbs )
                {
                    ERR                 err     = JET_errSuccess;
                    ::CCachedBlockId*   pcbid   = NULL;
                    ICachedBlockSlab^   cbs     = nullptr;

                    *ppcbs = NULL;

                    Alloc( pcbid = new ::CCachedBlockId() );
                    UtilMemCpy( pcbid, &cbid, sizeof( *pcbid ) );

                    ExCall( cbs = I()->GetSlab( gcnew CachedBlockId( &pcbid ) ) );

                    Call( CachedBlockSlab::ErrWrap( cbs, ppcbs ) );

                HandleError:
                    delete cbs;
                    delete pcbid;
                    if ( err < JET_errSuccess )
                    {
                        delete *ppcbs;
                        *ppcbs = NULL;
                    }
                    return err;
                }

                template<class TM, class TN>
                inline ERR CCachedBlockSlabManagerWrapper<TM, TN>::ErrIsSlabForCachedBlock( _In_    ::ICachedBlockSlab* const   pcbs,
                                                                                            _In_    const ::CCachedBlockId&     cbid,
                                                                                            _Out_   BOOL* const                 pfIsSlabForCachedBlock )
                {
                    ERR                 err                     = JET_errSuccess;
                    ::CCachedBlockId*   pcbid                   = NULL;
                    bool                isSlabForCachedBlock    = false;

                    *pfIsSlabForCachedBlock = fFalse;

                    Alloc( pcbid = new ::CCachedBlockId() );
                    UtilMemCpy( pcbid, &cbid, sizeof( *pcbid ) );

                    ExCall( isSlabForCachedBlock = I()->IsSlabForCachedBlock(   pcbs ? gcnew CachedBlockSlab( pcbs ) : nullptr,
                                                                                gcnew CachedBlockId( &pcbid ) ) );

                    *pfIsSlabForCachedBlock = isSlabForCachedBlock ? fTrue : fFalse;

                HandleError:
                    delete pcbid;
                    if ( err < JET_errSuccess )
                    {
                        *pfIsSlabForCachedBlock = fFalse;
                    }
                    return err;
                }

                template<class TM, class TN>
                inline ERR CCachedBlockSlabManagerWrapper<TM, TN>::ErrGetSlab(  _In_    const QWORD                 ib,
                                                                                _In_    const BOOL                  fWait,
                                                                                _In_    const BOOL                  fIgnoreVerificationErrors,
                                                                                _Out_   ::ICachedBlockSlab** const  ppcbs )
                {
                    ERR                     err     = JET_errSuccess;
                    ICachedBlockSlab^       cbs     = nullptr;
                    EsentErrorException^    ex      = nullptr;

                    *ppcbs = NULL;

                    ExCall( cbs = I()->GetSlab( ib, fWait ? true : false, fIgnoreVerificationErrors ? true : false, ex ) );

                    Call( CachedBlockSlab::ErrWrap( cbs, ppcbs ) );

                HandleError:
                    delete cbs;
                    if ( err < JET_errSuccess )
                    {
                        delete *ppcbs;
                        *ppcbs = NULL;
                    }
                    return err < JET_errSuccess || ex == nullptr ? err : (ERR)(int)ex->Error;
                }

                template<class TM, class TN>
                inline ERR CCachedBlockSlabManagerWrapper<TM, TN>::ErrVisitSlabs(   _In_ const ::ICachedBlockSlabManager::PfnVisitSlab  pfnVisitSlab,
                                                                                    _In_ const DWORD_PTR                                keyVisitSlab )
                {
                    ERR         err         = JET_errSuccess;
                    VisitSlab^  visitSlab   = pfnVisitSlab ? gcnew VisitSlab( pfnVisitSlab, keyVisitSlab ) : nullptr;

                    ExCall( I()->VisitSlabs( pfnVisitSlab ? gcnew Internal::Ese::BlockCache::Interop::ICachedBlockSlabManager::VisitSlab( visitSlab, &VisitSlab::VisitSlab_ ) : nullptr ) );

                HandleError:
                    return err;
                }
            }
        }
    }
}