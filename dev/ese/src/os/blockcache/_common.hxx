// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  Block Cache Lock Ranks

const INT rankFilePathHash = 0;
const INT rankCachedFileHash = 0;
const INT rankCacheThreadLocalStorage = 0;
const INT rankCacheRepository = 0;
const INT rankFileFilter = 0;
const INT rankFileIdentification = 0;
const INT rankFileWrapperIOComplete = 0;
const INT rankJournalSegment = 0;
const INT rankJournalAppend = 1;

//  IO offset range.

class COffsets
{
    public:

        COffsets( _In_ const QWORD ibStart, _In_ const QWORD ibEnd )
            :   m_ibStart( ibStart ),
                m_ibEnd( ibEnd )
        {
        }

        COffsets() : COffsets( 0, 0 ) {}

        COffsets& operator=( _In_ const COffsets& other )
        {
            m_ibStart = other.m_ibStart;
            m_ibEnd = other.m_ibEnd;
            return *this;
        }

        QWORD IbStart() const { return m_ibStart; }
        QWORD IbEnd() const { return m_ibEnd; }
        QWORD Cb() const { return min( qwMax - 1, m_ibEnd - m_ibStart ) + 1; }

        BOOL FOverlaps( _In_ const COffsets& other ) const
        {
            return IbEnd() >= other.IbStart() && IbStart() <= other.IbEnd();
        }

    private:

        QWORD  m_ibStart;
        QWORD  m_ibEnd;
};


//  Buffer of the same size as another type.

template< class T >
class Buffer
{
    private:

        BYTE    m_rgb[sizeof( T )];
};
