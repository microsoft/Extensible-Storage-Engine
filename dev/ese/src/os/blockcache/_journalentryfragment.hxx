// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  Journal Entry Fragment

#include <pshpack1.h>

//PERSISTED
class CJournalEntryFragment
{
    public:

        enum { cbEntryMax = lMax };

        CJournalEntryFragment( _In_ const BOOL fFirstFragment, _In_ const QWORD cbEntryRem )
            :   m_le_cbEntryOrCbEntryRem( fFirstFragment ? (int)cbEntryRem : -(int)cbEntryRem )
        {
            C_ASSERT( 0 == offsetof( CJournalEntryFragment, m_le_cbEntryOrCbEntryRem ) );
            C_ASSERT( sizeof( m_le_cbEntryOrCbEntryRem ) >= sizeof( cbEntryMax ) );
            C_ASSERT( sizeof( m_le_cbEntryOrCbEntryRem ) >= sizeof( -cbEntryMax ) );
        }

        BOOL FFirstFragment() const { return m_le_cbEntryOrCbEntryRem >= 0; }
        DWORD CbEntryRemaining() const { return (DWORD)labs( m_le_cbEntryOrCbEntryRem ); }
        const BYTE* RgbFragment() const { return &m_rgbFragment[ 0 ]; }

    private:

        UnalignedLittleEndian<int>  m_le_cbEntryOrCbEntryRem;   //  entry size of (negative) entry bytes remaining
        BYTE                        m_rgbFragment[ 0 ];         //  fragment data
};

#include <poppack.h>
