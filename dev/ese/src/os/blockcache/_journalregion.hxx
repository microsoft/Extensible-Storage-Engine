// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once


#include <pshpack1.h>

class CJournalRegion
{
    public:

        CJournalRegion( _In_ const ULONG cbPayload )
            :   m_le_cbPayload( cbPayload )
        {
            C_ASSERT( 0 == offsetof( CJournalRegion, m_le_cbPayload ) );
        }

        ULONG CbRegion() const { return sizeof( *this ) + CbPayload(); }
        ULONG CbPayload() const { return m_le_cbPayload; }
        const BYTE* RgbPayload() const { return &m_rgbPayload[ 0 ]; }
        CJournalBuffer JbPayload() const { return CJournalBuffer( CbPayload(), RgbPayload() ); }

    private:

        const UnalignedLittleEndian<ULONG>  m_le_cbPayload;
        BYTE                                m_rgbPayload[ 0 ];
};

#include <poppack.h>
