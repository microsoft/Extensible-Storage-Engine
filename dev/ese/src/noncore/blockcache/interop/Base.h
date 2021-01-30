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
                public ref class Base : public SafeHandle
                {
                    public:

                        Base( TM^ tm )
                            :   SafeHandle( (IntPtr)( new TW( tm ) ), true ),
                                isWrapper( true )
                        {
                            if ( IsInvalid )
                            {
                                throw gcnew OutOfMemoryException();
                            }
                        }

                        Base( TN** ppi )
                            :   SafeHandle( (IntPtr)*ppi, true ),
                                isWrapper( false )
                        {
                            *ppi = NULL;
                        }

                        Base( TN* pi )
                            :   SafeHandle( (IntPtr)pi, false ),
                                isWrapper( false )
                        {
                        }

                        property bool IsInvalid
                        {
                            virtual bool get() override
                            {
                                return ( IntPtr::Zero == this->handle );
                            }
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

                        virtual bool ReleaseHandle() override
                        {
                            if ( this->isWrapper )
                            {
                                TW* wrapper = (TW*)(void*)this->handle;
                                this->handle = IntPtr::Zero;
                                delete wrapper;
                            }
                            else
                            {
                                TN* pi = (TN*)(void*)this->handle;
                                this->handle = IntPtr::Zero;
                                delete pi;
                            }
                            return true;
                        }

                    private:

                        const bool isWrapper;
                };
            }
        }
    }
}
