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
                template< class TM, class TN, class TW, class TR >
                static ERR ErrWrap( TM^% tm, TN** const pptn )
                {
                    ERR err         = JET_errSuccess;
                    TM^ remotable   = nullptr;
                    TN* ptn         = NULL;

                    *pptn = NULL;

                    if ( tm )
                    {
                        remotable = tm;

                        if ( dynamic_cast<MarshalByRefObject^>( remotable ) == nullptr )
                        {
                            remotable = gcnew TR( remotable );
                        }

                        Alloc( ptn = new TW( remotable ) );
                    }

                    *pptn = ptn;
                    ptn = NULL;

                HandleError:
                    delete ptn;
                    if ( err < JET_errSuccess )
                    {
                        delete* pptn;
                        *pptn = NULL;
                    }
                    return err;
                }


                template< class TM, class TN >
                public class CWrapper : public TN
                {
                    public:

                        CWrapper( [Out] TM^% tm )
                            :   m_pContainer( (CContainer*)m_rgbContainer )
                        {
                            new( m_pContainer ) CContainer( tm );
                            tm = nullptr;
                        }

                        ~CWrapper()
                        {
                            m_pContainer->Release();
                        }

                        TM^ I() const
                        {
                            return (TM^)m_pContainer->O();
                        }

                    private:

                        CContainer* const   m_pContainer;
                        BYTE                m_rgbContainer[ sizeof( CContainer ) ];
                };
            }
        }
    }
}
