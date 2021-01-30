// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "stdafx.h"

#pragma once

#undef interface

#pragma managed

namespace Internal
{
    namespace Ese
    {
        namespace BlockCache
        {
            namespace Interop
            {
                public interface class IFileIdentification : IDisposable
                {
                    void GetFileId( String^ path, [Out] VolumeId% volumeid, [Out] FileId% fileid );

                    String^ GetFileKeyPath( String^ path );

                    void GetFilePathById(
                        VolumeId volumeid,
                        FileId fileid,
                        [Out] String^% anyAbsPath,
                        [Out] String^% keyPath );
                };
            }
        }
    }
}
