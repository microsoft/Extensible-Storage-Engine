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
                ref class FileFilterRemotable : FileRemotable, IFileFilter
                {
                    public:

                        FileFilterRemotable( IFileFilter^ target )
                            :   FileRemotable( target ),
                                target( target )
                        {
                        }

                        ~FileFilterRemotable() {}

                    public:

                        virtual void GetPhysicalId(
                            [Out] VolumeId% volumeid,
                            [Out] FileId% fileid,
                            [Out] FileSerial% fileserial )
                        {
                            this->target->GetPhysicalId( volumeid, fileid, fileserial );
                        }

                        virtual void Read(
                            Int64 offsetInBytes,
                            MemoryStream^ data,
                            FileQOS fileQOS,
                            IOMode ioMode,
                            IFile::IOComplete^ ioComplete,
                            IFile::IOHandoff^ ioHandoff )
                        {
                            this->target->Read( offsetInBytes, data, fileQOS, ioMode, ioComplete, ioHandoff );
                        }

                        virtual void Write(
                            Int64 offsetInBytes,
                            MemoryStream^ data,
                            FileQOS fileQOS,
                            IOMode ioMode,
                            IFile::IOComplete^ ioComplete,
                            IFile::IOHandoff^ ioHandoff )
                        {
                            this->target->Write( offsetInBytes, data, fileQOS, ioMode, ioComplete, ioHandoff );
                        }

                        virtual void Issue( IOMode ioMode )
                        {
                            this->target->Issue( ioMode );
                        }

                        virtual void Flush( IOMode ioMode )
                        {
                            this->target->Flush( ioMode );
                        }

                    private:

                        IFileFilter^ target;
                };
            }
        }
    }
}