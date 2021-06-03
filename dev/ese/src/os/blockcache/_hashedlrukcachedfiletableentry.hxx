// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  A cached file table entry for the hashed LRUK cache.

class CHashedLRUKCachedFileTableEntry  //  cfte
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
