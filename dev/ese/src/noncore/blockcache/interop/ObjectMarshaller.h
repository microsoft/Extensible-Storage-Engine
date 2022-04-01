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
                ref class ObjectMarshaller : Remotable, IObjectMarshaller
                {
                    public:

                        static void InitializeResolveHandler()
                        {
                            if ( isResolveHandlerInitialized == false )
                            {
                                isResolveHandlerInitialized = true;

                                SetupCurrentAppDomainResolveHandler();

                                SetupDefaultAppDomainResolveHandler();
                            }
                        }

                        ObjectMarshaller()
                        {
                        }

                        ~ObjectMarshaller()
                        {
                            this->!ObjectMarshaller();
                        }

                        virtual Object^ Get( IntPtr id );

                        virtual void SetupResolveHandler()
                        {
                            if ( isResolveHandlerInitialized == false )
                            {
                                isResolveHandlerInitialized = true;

                                SetupCurrentAppDomainResolveHandler();
                            }
                        }

                    protected:

                        !ObjectMarshaller()
                        {
                        }

                    private:

                        static void SetupCurrentAppDomainResolveHandler()
                        {
                            AppDomain::CurrentDomain->AssemblyResolve += gcnew ResolveEventHandler( ResolveHandler );
                        }

                        static void SetupDefaultAppDomainResolveHandler()
                        {
                            AppDomain^ defaultAppDomain = GetDefaultAppDomain();

                            if ( defaultAppDomain != nullptr && defaultAppDomain->Id != AppDomain::CurrentDomain->Id )
                            {
                                Object^ o = defaultAppDomain->CreateInstanceFromAndUnwrap(
                                    Assembly::GetExecutingAssembly()->Location,
                                    ObjectMarshaller::typeid->FullName );

                                IObjectMarshaller^ otherObjectMarshaller = (IObjectMarshaller^)o;
                                otherObjectMarshaller->SetupResolveHandler();
                            }
                        }

                        static Assembly^ ResolveHandler( Object^ Sender, ResolveEventArgs^ args )
                        {
                            Assembly^ assembly = nullptr;

                            if ( args->Name->Equals( Assembly::GetExecutingAssembly()->FullName ) )
                            {
                                try
                                {
                                    assembly = Assembly::LoadFrom( Assembly::GetExecutingAssembly()->Location );
                                }
                                catch ( Exception^ )
                                {
                                }
                            }

                            return assembly;
                        }

                        static AppDomain^ GetDefaultAppDomain()
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

                                if ( phost != NULL )
                                {
                                    phost->EnumDomains( &hEnum );

                                    while ( phost->NextDomain( hEnum, &pUnknown ) == S_OK )
                                    {
                                        AppDomain^ appDomain = (AppDomain^)Marshal::GetObjectForIUnknown( (IntPtr)(void*)pUnknown );
                                        pUnknown->Release();
                                        pUnknown = NULL;

                                        if ( appDomain != nullptr && appDomain->IsDefaultAppDomain() )
                                        {
                                            return appDomain;
                                        }
                                    }
                                }

                                return nullptr;
                            }
                            catch ( PlatformNotSupportedException^ )
                            {
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

                        static bool isResolveHandlerInitialized = false;
                };
            }
        }
    }
}