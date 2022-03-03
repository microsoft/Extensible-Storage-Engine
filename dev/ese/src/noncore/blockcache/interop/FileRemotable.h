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
                ref class FileRemotable : MarshalByRefObject, IFile
                {
                    public:

                        FileRemotable( IFile^ target )
                            :   target( target )
                        {
                        }

                        ~FileRemotable() {}

                    public:

                        virtual FileModeFlags FileModeFlags()
                        {
                            return this->target->FileModeFlags();
                        }

                        virtual void FlushFileBuffers()
                        {
                            this->target->FlushFileBuffers();
                        }

                        virtual void SetNoFlushNeeded()
                        {
                            this->target->SetNoFlushNeeded();
                        }

                        virtual String^ Path()
                        {
                            return this->target->Path();
                        }

                        virtual Int64 Size( FileSize fileSize )
                        {
                            return this->target->Size( fileSize );
                        }

                        virtual bool IsReadOnly()
                        {
                            return this->target->IsReadOnly();
                        }

                        virtual int CountLogicalCopies()
                        {
                            return this->target->CountLogicalCopies();
                        }

                        virtual void SetSize( Int64 sizeInBytes, bool zeroFill, FileQOS fileQOS )
                        {
                            return this->target->SetSize( sizeInBytes, zeroFill, fileQOS );
                        }

                        virtual void Rename( String^ pathDest, bool overwriteExisting )
                        {
                            return this->target->Rename( pathDest, overwriteExisting );
                        }

                        virtual void SetSparseness()
                        {
                            this->target->SetSparseness();
                        }

                        virtual void IOTrim( Int64 offsetInBytes, Int64 sizeInBytes )
                        {
                            return this->target->IOTrim( offsetInBytes, sizeInBytes );
                        }

                        virtual void RetrieveAllocatedRegion(
                            Int64 offsetToQuery,
                            [Out] Int64% offsetInBytes,
                            [Out] Int64% sizeInBytes )
                        {
                            return this->target->RetrieveAllocatedRegion( offsetToQuery, offsetInBytes, sizeInBytes );
                        }

                        virtual int IOSize()
                        {
                            return this->target->IOSize();
                        }

                        virtual int SectorSize()
                        {
                            return this->target->SectorSize();
                        }

                        virtual void IORead(
                            Int64 offsetInBytes,
                            MemoryStream^ data,
                            FileQOS fileQOS,
                            IFile::IOComplete^ ioComplete,
                            IFile::IOHandoff^ ioHandoff )
                        {
                            return this->target->IORead( offsetInBytes, data, fileQOS, ioComplete, ioHandoff );
                        }

                        virtual void IOWrite(
                            Int64 offsetInBytes,
                            MemoryStream^ data,
                            FileQOS fileQOS,
                            IFile::IOComplete^ ioComplete,
                            IFile::IOHandoff^ ioHandoff )
                        {
                            return this->target->IOWrite( offsetInBytes, data, fileQOS, ioComplete, ioHandoff );
                        }

                        virtual void IOIssue()
                        {
                            this->target->IOIssue();
                        }

                        virtual Int64 NTFSAttributeListSize()
                        {
                            return this->target->NTFSAttributeListSize();
                        }

                        virtual IntPtr DiskId()
                        {
                            return this->target->DiskId();
                        }

                        virtual Int64 CountIoNonFlushed()
                        {
                            return this->target->CountIoNonFlushed();
                        }

                        virtual bool SeekPenalty()
                        {
                            return this->target->SeekPenalty();
                        }

                    private:

                        IFile^ target;
                };
            }
        }
    }
}