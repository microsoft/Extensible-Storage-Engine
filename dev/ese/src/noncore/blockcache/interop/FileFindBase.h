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
                public ref class FileFindBase : public Base<TM,TN,TW>, IFileFind
                {
                    public:

                        FileFindBase( TM^ ff ) : Base( ff ) { }

                        FileFindBase( TN** const ppffapi ) : Base( ppffapi ) {}

                    public:

                        virtual bool Next();

                        virtual bool IsFolder();

                        virtual String^ Path();

                        virtual Int64 Size( FileSize fileSize );

                        virtual bool IsReadOnly();
                };

                template< class TM, class TN, class TW >
                inline bool FileFindBase<TM,TN,TW>::Next()
                {
                    ERR err = JET_errSuccess;

                    err = Pi->ErrNext();

                    if ( err == JET_errFileNotFound )
                    {
                        return false;
                    }

                    Call( err );

                    return true;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline bool FileFindBase<TM,TN,TW>::IsFolder()
                {
                    ERR     err     = JET_errSuccess;
                    BOOL    fFolder = fFalse;

                    Call( Pi->ErrIsFolder( &fFolder ) );

                    return fFolder ? true : false;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline String^ FileFindBase<TM,TN,TW>::Path()
                {
                    ERR     err                                 = JET_errSuccess;
                    WCHAR   wszFoundPath[ OSFSAPI_MAX_PATH ]    = { 0 };

                    Call( Pi->ErrPath( wszFoundPath ) );

                    return gcnew String( wszFoundPath );

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline Int64 FileFindBase<TM,TN,TW>::Size( FileSize fileSize )
                {
                    ERR     err         = JET_errSuccess;
                    QWORD   cbSize      = 0;

                    Call( Pi->ErrSize( &cbSize, (IFileAPI::FILESIZE)fileSize ) );

                    return cbSize;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline bool FileFindBase<TM,TN,TW>::IsReadOnly()
                {
                    ERR     err         = JET_errSuccess;
                    BOOL    fReadOnly   = fFalse;

                    Call( Pi->ErrIsReadOnly( &fReadOnly ) );

                    return fReadOnly ? true : false;

                HandleError:
                    throw EseException( err );
                }
            }
        }
    }
}
