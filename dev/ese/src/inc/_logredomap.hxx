// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.



struct RedoMapEntry
{
    public:
        LGPOS lgpos;
        ERR err;
        DBTIME dbtimeBefore;
        DBTIME dbtimePage;
        DBTIME dbtimeAfter;

    public:
        RedoMapEntry();

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
        VOID GetOldestLgposEntry( __out PGNO* const ppgno, __out RedoMapEntry* const prme, __out CPG* const pcpg ) const;
};
