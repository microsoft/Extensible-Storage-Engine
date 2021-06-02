// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  Journal Region

#include <pshpack1.h>

//PERSISTED
class CJournalRegion  //  jreg
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

        const UnalignedLittleEndian<ULONG>  m_le_cbPayload;     //  payload size;  0 indicates the last region in a segment
        BYTE                                m_rgbPayload[ 0 ];  //  payload
};

#include <poppack.h>
