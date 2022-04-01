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
                public ref class CachedBlockWriteCountsManagerBase : Base<TM, TN, TW>, ICachedBlockWriteCountsManager
                {
                    public:

                        CachedBlockWriteCountsManagerBase( TM^ cbwcm ) : Base( cbwcm ) { }

                        CachedBlockWriteCountsManagerBase( TN** const ppcbwcm ) : Base( ppcbwcm ) {}

                        CachedBlockWriteCountsManagerBase( TN* const pcbwcm ) : Base( pcbwcm ) {}

                    public:

                        virtual CachedBlockWriteCount GetWriteCount( Int64 index );

                        virtual void SetWriteCount( Int64 index, CachedBlockWriteCount writeCount );

                        virtual void Save();
                };

                template<class TM, class TN, class TW>
                inline CachedBlockWriteCount CachedBlockWriteCountsManagerBase<TM, TN, TW>::GetWriteCount( Int64 index )
                {
                    ERR                     err     = JET_errSuccess;
                    ::CachedBlockWriteCount cbwc    = ::cbwcUnknown;

                    Call( Pi->ErrGetWriteCount( index, &cbwc ) );

                    return (CachedBlockWriteCount)cbwc;

                HandleError:
                    throw EseException( err );
                }

                template<class TM, class TN, class TW>
                inline void CachedBlockWriteCountsManagerBase<TM, TN, TW>::SetWriteCount( Int64 index, CachedBlockWriteCount writeCount )
                {
                    ERR err = JET_errSuccess;

                    Call( Pi->ErrSetWriteCount( index, (::CachedBlockWriteCount)writeCount ) );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        throw EseException( err );
                    }
                }

                template<class TM, class TN, class TW>
                inline void CachedBlockWriteCountsManagerBase<TM, TN, TW>::Save()
                {
                    ERR err = JET_errSuccess;

                    Call( Pi->ErrSave() );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        throw EseException( err );
                    }
                }
            }
        }
    }
}