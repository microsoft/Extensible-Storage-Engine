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
                public interface class IJournal : IDisposable
                {
                    void GetProperties(
                        [Out] JournalPosition% journalPositionReplay,
                        [Out] JournalPosition% journalPositionDurableForWriteBack,
                        [Out] JournalPosition% journalPositionDurable );

                    delegate bool VisitEntry(
                        JournalPosition journalPosition,
                        ArraySegment<byte> entry );

                    void VisitEntries( IJournal::VisitEntry^ visitEntry );

                    void Repair( JournalPosition journalPositionInvalidate );

                    JournalPosition AppendEntry( array<ArraySegment<byte>>^ payload );

                    void Flush();

                    void Truncate( JournalPosition journalPositionReplay );
                };
            }
        }
    }
}
