// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once


class CHashedLRUKCachedFileTableEntry
    :   public CCachedFileTableEntryBase
{
    public:

        CHashedLRUKCachedFileTableEntry(    _In_ const VolumeId     volumeid,
                                            _In_ const FileId       fileid,
                                            _In_ const FileSerial   fileserial )
            :   CCachedFileTableEntryBase( volumeid, fileid, fileserial )
        {
        }
};
