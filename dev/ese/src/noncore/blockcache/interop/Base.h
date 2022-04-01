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
                template<class TM, class TN, class TW>
                public ref class Base : Remotable
                {
                    public:

                        ~Base()
                        {
                            this->!Base();
                        }

                        property IntPtr Handle
                        {
                            IntPtr get()
                            {
                                return this->handle;
                            }
                        }

                    internal:

                        Base( TM^ tm )
                            :   handle( (IntPtr)( new TW( tm ) ) ),
                                isOwner( true ),
                                isWrapper( true )
                        {
                            if ( IsInvalid )
                            {
                                throw gcnew OutOfMemoryException();
                            }
                        }

                        Base( TN** ppi )
                            :   handle( (IntPtr)*ppi ),
                                isOwner( true ),
                                isWrapper( false )
                        {
                            *ppi = NULL;
                        }

                        Base( TN* pi )
                            :   handle( (IntPtr)pi ),
                                isOwner( false ),
                                isWrapper( false )
                        {
                        }

                        property TN* Pi
                        {
                            virtual TN* get()
                            {
                                return this->isWrapper ?
                                    (TN*)(TW*)(void*)this->handle :
                                    (TN*)(void*)this->handle;
                            }
                        }

                    protected:

                        !Base()
                        {
                            if ( !IsInvalid )
                            {
                                ReleaseHandle();
                            }
                        }

                    private:

                        property bool IsInvalid
                        {
                            bool get()
                            {
                                return this->handle == IntPtr::Zero;
                            }
                        }

                        void ReleaseHandle()
                        {
                            if ( !this->IsInvalid )
                            {
                                if ( this->isOwner )
                                {
                                    if ( this->isWrapper )
                                    {
                                        TW* wrapper = (TW*)(void*)this->handle;
                                        delete wrapper;
                                    }
                                    else
                                    {
                                        TN* pi = (TN*)(void*)this->handle;
                                        delete pi;
                                    }
                                }

                                this->handle = IntPtr::Zero;
                            }
                        }

                    private:

                        IntPtr handle;
                        const bool isOwner;
                        const bool isWrapper;
                };
            }
        }
    }
}
