// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  Log Redo Map
//
//  The log redo map is used to track pages referred to
//  in redo operations that are not present in the database
//  file or failed otherwise as it was possibly shrunk/trimmed.
//
//  Once a page is in the redo map, any further operations
//  against that page will be automatically discarded. The page
//  reference is removed from the map if the page is part of a
//  region which gets shrunk/trimmed. At this point, we say
//  that the page reference has been reconciled.
//
//  At the end of recovery, the log redo map is inspected
//  to ensure that no unreconciled (uncleared) pages are
//  present. If such a page or more are present, it means
//  that there were invalid redo operations that cannot
//  be justified by shrink/trim and thus must fail recovery.
//


struct RedoMapEntry
{
    public:
        LGPOS lgpos;          // Log position where the earliest mismatch occurred.
        ERR err;              // Error where the easliest mismatch occurred.
        DBTIME dbtimeBefore;  // "Before" DBTIME of the log record that triggered the mismatch.
        DBTIME dbtimePage;    // DBTIME of the page at the time of the mismatch.
        DBTIME dbtimeAfter;   // "After" DBTIME of the log record that triggered the mismatch.

    public:
        // Default ctr.
        RedoMapEntry();

        // Ctr.
        RedoMapEntry(
            _In_ const LGPOS& lgposNew,
            _In_ const ERR errNew,
            _In_ const DBTIME dbtimeBeforeNew,
            _In_ const DBTIME dbtimePageNew,
            _In_ const DBTIME dbtimeAfterNew );
};

class CLogRedoMap
{
    private:
        typedef CRedBlackTree< PGNO, RedoMapEntry > RedoMapTree;
        static ERR ErrToErr( _In_ const RedoMapTree::ERR err );

    private:
        RedoMapTree*    m_rmt;
        IFMP            m_ifmp;

    public:
        CLogRedoMap();
        ~CLogRedoMap();

        VOID ClearLogRedoMap();
        ERR ErrInitLogRedoMap( const IFMP ifmp );
        VOID TermLogRedoMap();
        BOOL FLogRedoMapEnabled() const;

        BOOL FPgnoSet( _In_ const PGNO pgno ) const;
        BOOL FAnyPgnoSet() const;
        ERR ErrSetPgno( _In_ const PGNO pgno );
        ERR ErrSetPgno( _In_ const PGNO pgno, _In_ const LGPOS& lgpos );
        ERR ErrSetPgno(
            _In_ const PGNO pgno,
            _In_ const LGPOS& lgpos,
            _In_ const ERR errNew,
            _In_ const DBTIME dbtimeBefore,
            _In_ const DBTIME dbtimePage,
            _In_ const DBTIME dbtimeAfter );
        VOID ClearPgno( _In_ PGNO pgnoStart, _In_ PGNO pgnoEnd );
        VOID ClearPgno( _In_ PGNO pgno );
        VOID GetOldestLgposEntry( _Out_ PGNO* const ppgno, _Out_ RedoMapEntry* const prme, _Out_ CPG* const pcpg ) const;
};
