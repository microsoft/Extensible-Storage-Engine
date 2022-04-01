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
                ref class FileIdentificationRemotable : Remotable, IFileIdentification
                {
                    public:

                        FileIdentificationRemotable( IFileIdentification^ target )
                            :   target( target )
                        {
                        }

                        ~FileIdentificationRemotable() {}

                    public:

                        virtual void GetFileId( String^ path, [Out] VolumeId% volumeid, [Out] FileId% fileid )
                        {
                            return this->target->GetFileId( path, volumeid, fileid );
                        }

                        virtual String^ GetFileKeyPath( String^ path )
                        {
                            return this->target->GetFileKeyPath( path );
                        }

                        virtual void GetFilePathById(
                            VolumeId volumeid,
                            FileId fileid,
                            [Out] String^% anyAbsPath,
                            [Out] String^% keyPath )
                        {
                            return this->target->GetFilePathById( volumeid, fileid, anyAbsPath, keyPath );
                        }

                    private:

                        IFileIdentification^ target;
                };
            }
        }
    }
}