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
                ref class JournalRemotable : Remotable, IJournal
                {
                    public:

                        JournalRemotable( IJournal^ target )
                            :   target( target )
                        {
                        }

                        ~JournalRemotable() {}

                    public:

                        virtual void GetProperties(
                            [Out] JournalPosition% journalPositionReplay,
                            [Out] JournalPosition% journalPositionDurableForWriteBack,
                            [Out] JournalPosition% journalPositionDurable,
                            [Out] JournalPosition% journalPositionAppend,
                            [Out] JournalPosition% journalPositionFull )
                        {
                            this->target->GetProperties( journalPositionReplay, journalPositionDurableForWriteBack, journalPositionDurable, journalPositionAppend, journalPositionFull );
                        }

                        virtual void VisitEntries( IJournal::VisitEntry^ visitEntry )
                        {
                            return this->target->VisitEntries( visitEntry );
                        }

                        virtual JournalPosition Repair( JournalPosition journalPositionInvalidate )
                        {
                            return this->target->Repair( journalPositionInvalidate );
                        }

                        virtual JournalPosition AppendEntry(
                            array<ArraySegment<BYTE>>^ payload,
                            [Out] JournalPosition% journalPositionEnd )
                        {
                            return this->target->AppendEntry( payload, journalPositionEnd );
                        }

                        virtual void Flush()
                        {
                            return this->target->Flush();
                        }

                        virtual void Truncate( JournalPosition journalPositionReplay )
                        {
                            return this->target->Truncate( journalPositionReplay );
                        }

                    private:

                        IJournal^ target;
                };
            }
        }
    }
}