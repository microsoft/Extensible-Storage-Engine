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
                class CContainer
                {
                    public:

                        CContainer( Object^ o )
                            :   m_qwSignature( s_qwSignature ),
                                m_idAppDomain( System::AppDomain::CurrentDomain->Id ),
                                m_o( o )
                        {
                        }

                        ~CContainer()
                        {
                            m_qwSignature = 0;
                        }

                        Object^ O() const;

                    private:

                        Object^ OFromOtherAppDomain() const;
                        AppDomain^ GetOtherAppDomain( Int32 appDomainId ) const;

                    private:

                        static const QWORD                  s_qwSignature   = 0x4C796B6165757153;

                        QWORD                               m_qwSignature; 
                        int                                 m_idAppDomain;
                        mutable msclr::auto_gcroot<Object^> m_o;
                };

                INLINE Object^ CContainer::O() const
                {
                    if ( !this )
                    {
                        return nullptr;
                    }

                    if ( m_qwSignature != s_qwSignature )
                    {
                        return nullptr;
                    }

                    if ( m_idAppDomain == System::AppDomain::CurrentDomain->Id )
                    {
                        return m_o.get();
                    }

                    return OFromOtherAppDomain();
                }

                INLINE Object^ CContainer::OFromOtherAppDomain() const
                {
                    AppDomain^          otherAppDomain          = nullptr;
                    IObjectMarshaller^  otherObjectMarshaller   = nullptr;

                    try
                    {
                        otherAppDomain = GetOtherAppDomain( m_idAppDomain );

                        otherObjectMarshaller = (IObjectMarshaller^)otherAppDomain->CreateInstanceAndUnwrap(
                            ObjectMarshaller::typeid->Assembly->FullName,
                            ObjectMarshaller::typeid->FullName );

                        return otherObjectMarshaller->Get( (IntPtr)(void*)static_cast<const CContainer*>( this ) );
                    }
                    finally
                    {
                        if ( otherObjectMarshaller != nullptr )
                        {
                            delete otherObjectMarshaller;
                        }

                        if ( otherAppDomain != nullptr )
                        {
                            delete otherAppDomain;
                        }
                    }
                }

                INLINE AppDomain^ CContainer::GetOtherAppDomain( Int32 appDomainId ) const
                {
                    ICorRuntimeHost*    phost       = NULL;
                    HDOMAINENUM         hEnum       = NULL;
                    IUnknown*           pUnknown    = NULL;

                    try
                    {
                        phost = reinterpret_cast<ICorRuntimeHost*>(
                            RuntimeEnvironment::GetRuntimeInterfaceAsIntPtr(
                                msclr::_detail::FromGUID( __uuidof( CorRuntimeHost ) ),
                                msclr::_detail::FromGUID( __uuidof( ICorRuntimeHost ) ) ).ToPointer() );

                        phost->EnumDomains( &hEnum );

                        while ( phost->NextDomain( hEnum, &pUnknown ) == S_OK )
                        {
                            AppDomain^ appDomain = (AppDomain^)Marshal::GetObjectForIUnknown( (IntPtr)(void*)pUnknown );
                            pUnknown->Release();
                            pUnknown = NULL;

                            if ( appDomain != nullptr && appDomain->Id == appDomainId )
                            {
                                return appDomain;
                            }
                        }

                        return nullptr;
                    }
                    finally
                    {
                        if ( pUnknown )
                        {
                            pUnknown->Release();
                        }

                        if ( phost )
                        {
                            if ( hEnum )
                            {
                                phost->CloseEnum( hEnum );
                            }

                            phost->Release();
                        }
                    }
                }
            }
        }
    }
}