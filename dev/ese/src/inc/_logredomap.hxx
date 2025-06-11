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
            __in const LGPOS& lgposNew,
            __in const ERR errNew,
            __in const DBTIME dbtimeBeforeNew,
            __in const DBTIME dbtimePageNew,
            __in const DBTIME dbtimeAfterNew );
};

class CLogRedoMap
{
    private:
        typedef CRedBlackTree< PGNO, RedoMapEntry > RedoMapTree;
        static ERR ErrToErr( __in const RedoMapTree::ERR err );

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

        BOOL FPgnoSet( __in const PGNO pgno ) const;
        BOOL FAnyPgnoSet() const;
        ERR ErrSetPgno( __in const PGNO pgno );
        ERR ErrSetPgno( __in const PGNO pgno, __in const LGPOS& lgpos );
        ERR ErrSetPgno(
            __in const PGNO pgno,
            __in const LGPOS& lgpos,
            __in const ERR errNew,
            __in const DBTIME dbtimeBefore,
            __in const DBTIME dbtimePage,
            __in const DBTIME dbtimeAfter );
        VOID ClearPgno( __in PGNO pgnoStart, __in PGNO pgnoEnd );
        VOID ClearPgno( __in PGNO pgno );
        VOID GetOldestLgposEntry( __out PGNO* const ppgno, __out RedoMapEntry* const prme, __out CPG* const pcpg ) const;
};
