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
                ERR ErrWrap( TM^% tm, TN** const pptn )
                {
                    ERR err = JET_errSuccess;

                    *pptn = NULL;

                    if ( tm )
                    {
                        Alloc( *pptn = new TW( tm ) );
                        tm = nullptr;
                    }

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        delete *pptn;
                        *pptn = NULL;
                    }
                    return err;
                }

                template< class TM, class TN >
                public class CWrapper : public TN
                {
                    public:

                        CWrapper( TM^ tm ) : m_container( tm ) { }

                        TM^ I() const
                        {
                            return m_container.I();
                        }

                    private:

                        class CContainer
                        {
                            public:

                                CContainer( TM^ tm ) : m_tm( tm ) { }

                                TM^ I() const { return m_tm.get(); }

                            private:

                                mutable msclr::auto_gcroot<TM^> m_tm;
                        };

                    private:

                        const CContainer m_container;
                };
            }
        }
    }
}
