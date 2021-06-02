// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include "_logredomap.hxx"

#ifndef ENABLE_JET_UNIT_TEST
#error This file should only be compiled with the unit tests!
#endif // ENABLE_JET_UNIT_TEST

static const LGPOS lgposAny1 = { 0x1, 0x2, 0x3 };
static const LGPOS lgposAny2 = { 0x4, 0x5, 0x6 };
static const LGPOS lgposAny3 = { 0x7, 0x8, 0x9 };
static const LGPOS lgposAny4 = { 0xA, 0xB, 0xC };
static const LGPOS lgposAny5 = { 0xD, 0xE, 0xF };

JETUNITTEST( CLogRedoMap, SetGetSinglePgnoClearSinglePageRange )
{
    CLogRedoMap rm;
    PGNO pgno;
    RedoMapEntry rme;
    CPG cpg;

    CHECK( JET_errSuccess == rm.ErrInitLogRedoMap( ifmpNil ) );
    CHECK( !rm.FAnyPgnoSet() );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny1 ) );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 665, 665 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 667, 667 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 666, 666 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FAnyPgnoSet() );

    rm.TermLogRedoMap();
}

JETUNITTEST( CLogRedoMap, SetGetSinglePgnoClearTwoPageRange )
{
    CLogRedoMap rm;
    PGNO pgno;
    RedoMapEntry rme;
    CPG cpg;

    CHECK( JET_errSuccess == rm.ErrInitLogRedoMap( ifmpNil ) );
    CHECK( !rm.FAnyPgnoSet() );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny1 ) );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 664, 665 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 667, 668 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 665, 666 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FAnyPgnoSet() );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny1 ) );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 666, 667 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FAnyPgnoSet() );

    rm.TermLogRedoMap();
}

JETUNITTEST( CLogRedoMap, SetGetSinglePgnoClearMultiPageRange )
{
    CLogRedoMap rm;
    PGNO pgno;
    RedoMapEntry rme;
    CPG cpg;

    CHECK( JET_errSuccess == rm.ErrInitLogRedoMap( ifmpNil ) );
    CHECK( !rm.FAnyPgnoSet() );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny1 ) );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 660, 665 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 667, 672 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 661, 666 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FAnyPgnoSet() );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny1 ) );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 666, 671 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FAnyPgnoSet() );

    rm.TermLogRedoMap();
}

JETUNITTEST( CLogRedoMap, SetGetTwoPgnoClearSinglePageRange )
{
    CLogRedoMap rm;
    PGNO pgno;
    RedoMapEntry rme;
    CPG cpg;

    CHECK( JET_errSuccess == rm.ErrInitLogRedoMap( ifmpNil ) );
    CHECK( !rm.FAnyPgnoSet() );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny1 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny2 ) );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 665 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 664, 664 );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 665 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 667, 667 );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 665 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 665, 665 );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny2 ) == 0 );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny3 ) );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny2 ) == 0 );

    rm.ClearPgno( 666, 666 );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 665 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny3 ) == 0 );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny2 ) );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny2 ) == 0 );

    rm.ClearPgno( 665, 665 );
    rm.ClearPgno( 666, 666 );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FAnyPgnoSet() );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny2 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny3 ) );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 665 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny2 ) == 0 );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny1 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 665 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny2 ) == 0 );

    rm.ClearPgno( 666, 666 );
    rm.ClearPgno( 665, 665 );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FAnyPgnoSet() );

    rm.TermLogRedoMap();
}

JETUNITTEST( CLogRedoMap, SetGetTwoPgnoClearTwoPageRange )
{
    CLogRedoMap rm;
    PGNO pgno;
    RedoMapEntry rme;
    CPG cpg;

    CHECK( JET_errSuccess == rm.ErrInitLogRedoMap( ifmpNil ) );
    CHECK( !rm.FAnyPgnoSet() );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny2 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny1 ) );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 663, 664 );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 667, 668 );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 664, 665 );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny3 ) );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 666, 667 );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 665 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny3 ) == 0 );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny1 ) );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 665, 666 );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FAnyPgnoSet() );

    rm.TermLogRedoMap();
}

JETUNITTEST( CLogRedoMap, SetGetTwoPgnoClearMultiPageRange )
{
    CLogRedoMap rm;
    PGNO pgno;
    RedoMapEntry rme;
    CPG cpg;

    CHECK( JET_errSuccess == rm.ErrInitLogRedoMap( ifmpNil ) );
    CHECK( !rm.FAnyPgnoSet() );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny1 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny2 ) );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 665 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 659, 664 );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 665 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 667, 672 );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 665 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 660, 665 );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny2 ) == 0 );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny3 ) );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );

    rm.ClearPgno( 666, 671 );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 665 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny3 ) == 0 );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny1 ) );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 661, 666 );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FAnyPgnoSet() );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny2 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny1 ) );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 665, 670 );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FAnyPgnoSet() );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny1 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny2 ) );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 665 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 663, 668 );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FAnyPgnoSet() );

    rm.TermLogRedoMap();
}

JETUNITTEST( CLogRedoMap, SetGetMultiPgnoClearSinglePageRange )
{
    CLogRedoMap rm;
    PGNO pgno;
    RedoMapEntry rme;
    CPG cpg;

    CHECK( JET_errSuccess == rm.ErrInitLogRedoMap( ifmpNil ) );
    CHECK( !rm.FAnyPgnoSet() );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny2, JET_errPageNotInitialized, 10, 20, 30 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny4, JET_errFileIOBeyondEOF, 40, 50, 60 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 667, lgposAny1, JET_errRecoveryVerifyFailure, 70, 80, 90 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 668, lgposAny5, JET_errDbTimeTooOld, 100, 110, 120 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 669, lgposAny3, JET_errDbTimeTooNew, 130, 140, 150 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 5 );
    CHECK( pgno == 667 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );
    CHECK( rme.err == JET_errRecoveryVerifyFailure );
    CHECK( rme.dbtimeBefore == 70 );
    CHECK( rme.dbtimePage == 80 );
    CHECK( rme.dbtimeAfter == 90 );

    rm.ClearPgno( 667, 667 );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FPgnoSet( 668 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 4 );
    CHECK( pgno == 665 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny2 ) == 0 );
    CHECK( rme.err == JET_errPageNotInitialized );
    CHECK( rme.dbtimeBefore == 10 );
    CHECK( rme.dbtimePage == 20 );
    CHECK( rme.dbtimeAfter == 30 );

    rm.ClearPgno( 665, 665 );
    CHECK( !rm.FPgnoSet( 664 ) );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 3 );
    CHECK( pgno == 669 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny3 ) == 0 );
    CHECK( rme.err == JET_errDbTimeTooNew );
    CHECK( rme.dbtimeBefore == 130 );
    CHECK( rme.dbtimePage == 140 );
    CHECK( rme.dbtimeAfter == 150 );

    rm.ClearPgno( 669, 669 );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FPgnoSet( 668 ) );
    CHECK( !rm.FPgnoSet( 669 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny4 ) == 0 );
    CHECK( rme.err == JET_errFileIOBeyondEOF );
    CHECK( rme.dbtimeBefore == 40 );
    CHECK( rme.dbtimePage == 50 );
    CHECK( rme.dbtimeAfter == 60 );

    rm.ClearPgno( 666, 666 );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FPgnoSet( 668 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 668 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny5 ) == 0 );
    CHECK( rme.err == JET_errDbTimeTooOld );
    CHECK( rme.dbtimeBefore == 100 );
    CHECK( rme.dbtimePage == 110 );
    CHECK( rme.dbtimeAfter == 120 );

    rm.ClearPgno( 668, 668 );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FPgnoSet( 668 ) );
    CHECK( !rm.FAnyPgnoSet() );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny2 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 668, lgposAny1 ) );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FPgnoSet( 668 ) );
    CHECK( !rm.FPgnoSet( 669 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 668 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );
    CHECK( rme.err == JET_errSuccess );

    rm.ClearPgno( 668, 668 );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FPgnoSet( 668 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny2 ) == 0 );
    CHECK( rme.err == JET_errSuccess );

    rm.ClearPgno( 666, 666 );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FPgnoSet( 668 ) );
    CHECK( !rm.FAnyPgnoSet() );

    rm.TermLogRedoMap();
}

JETUNITTEST( CLogRedoMap, SetGetMultiPgnoClearTwoPageRange )
{
    CLogRedoMap rm;
    PGNO pgno;
    RedoMapEntry rme;
    CPG cpg;

    CHECK( JET_errSuccess == rm.ErrInitLogRedoMap( ifmpNil ) );
    CHECK( !rm.FAnyPgnoSet() );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny3 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny4 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 667, lgposAny1 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 668, lgposAny2 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 669, lgposAny5 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 5 );
    CHECK( pgno == 667 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 667, 668 );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FPgnoSet( 668 ) );
    CHECK( rm.FPgnoSet( 669 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 3 );
    CHECK( pgno == 665 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny3 ) == 0 );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 667, lgposAny1 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 668, lgposAny2 ) );
    CHECK( rm.FPgnoSet( 667 ) );
    CHECK( rm.FPgnoSet( 668 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 5 );
    CHECK( pgno == 667 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 665, 666 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( rm.FPgnoSet( 667 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 3 );
    CHECK( pgno == 667 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny3 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny4 ) );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 5 );
    CHECK( pgno == 667 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 668, 669 );
    CHECK( rm.FPgnoSet( 667 ) );
    CHECK( !rm.FPgnoSet( 668 ) );
    CHECK( !rm.FPgnoSet( 669 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 3 );
    CHECK( pgno == 667 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 668, lgposAny2 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 669, lgposAny5 ) );
    CHECK( rm.FPgnoSet( 668 ) );
    CHECK( rm.FPgnoSet( 669 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 5 );
    CHECK( pgno == 667 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 664, 665 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 4 );
    CHECK( pgno == 667 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny3 ) );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 5 );
    CHECK( pgno == 667 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 669, 670 );
    CHECK( rm.FPgnoSet( 668 ) );
    CHECK( !rm.FPgnoSet( 669 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 4 );
    CHECK( pgno == 667 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 669, lgposAny5 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 5 );
    CHECK( pgno == 667 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 666, 668 );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FPgnoSet( 668 ) );
    CHECK( rm.FPgnoSet( 669 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 665 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny3 ) == 0 );

    rm.ClearPgno( 664, 665 );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 669 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny5 ) == 0 );
    rm.ClearPgno( 669, 670 );
    CHECK( !rm.FAnyPgnoSet() );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny3 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 669, lgposAny5 ) );

    rm.ClearPgno( 665, 666 );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 669 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny5 ) == 0 );
    rm.ClearPgno( 668, 669 );
    CHECK( !rm.FAnyPgnoSet() );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny3 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 669, lgposAny5 ) );

    rm.ClearPgno( 669, 670 );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 665 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny3 ) == 0 );
    rm.ClearPgno( 664, 665 );
    CHECK( !rm.FAnyPgnoSet() );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny3 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 669, lgposAny5 ) );

    rm.ClearPgno( 668, 669 );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 665 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny3 ) == 0 );
    rm.ClearPgno( 665, 666 );
    CHECK( !rm.FAnyPgnoSet() );

    rm.TermLogRedoMap();
}

JETUNITTEST( CLogRedoMap, SetGetMultiPgnoClearMultiPageRange )
{
    CLogRedoMap rm;
    PGNO pgno;
    RedoMapEntry rme;
    CPG cpg;

    CHECK( JET_errSuccess == rm.ErrInitLogRedoMap( ifmpNil ) );
    CHECK( !rm.FAnyPgnoSet() );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny5 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny4 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 667, lgposAny3 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 668, lgposAny2 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 669, lgposAny1 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 5 );
    CHECK( pgno == 669 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );

    rm.ClearPgno( 667, 667 );
    rm.ClearPgno( 664, 666 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FPgnoSet( 668 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 669 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );
    rm.ClearPgno( 668, 668 );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 669 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );
    rm.ClearPgno( 669, 669 );
    CHECK( !rm.FAnyPgnoSet() );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny5 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny4 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 667, lgposAny3 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 668, lgposAny2 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 669, lgposAny1 ) );

    rm.ClearPgno( 667, 667 );
    rm.ClearPgno( 664, 667 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( rm.FPgnoSet( 668 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 669 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );
    rm.ClearPgno( 668, 668 );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 669 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );
    rm.ClearPgno( 669, 669 );
    CHECK( !rm.FAnyPgnoSet() );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny5 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny4 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 667, lgposAny3 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 668, lgposAny2 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 669, lgposAny1 ) );

    rm.ClearPgno( 667, 667 );
    rm.ClearPgno( 664, 668 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FPgnoSet( 668 ) );
    CHECK( rm.FPgnoSet( 669 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 669 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );
    rm.ClearPgno( 669, 669 );
    CHECK( !rm.FAnyPgnoSet() );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny5 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny4 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 667, lgposAny3 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 668, lgposAny2 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 669, lgposAny1 ) );

    rm.ClearPgno( 667, 667 );
    rm.ClearPgno( 665, 669 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FPgnoSet( 668 ) );
    CHECK( !rm.FPgnoSet( 669 ) );
    CHECK( !rm.FAnyPgnoSet() );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny5 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny4 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 667, lgposAny3 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 668, lgposAny2 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 669, lgposAny1 ) );

    rm.ClearPgno( 665, 669 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FPgnoSet( 668 ) );
    CHECK( !rm.FPgnoSet( 669 ) );
    CHECK( !rm.FAnyPgnoSet() );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny5 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny4 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 667, lgposAny3 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 668, lgposAny2 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 669, lgposAny1 ) );

    rm.ClearPgno( 664, 669 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FPgnoSet( 668 ) );
    CHECK( !rm.FPgnoSet( 669 ) );
    CHECK( !rm.FAnyPgnoSet() );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny5 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny4 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 667, lgposAny3 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 668, lgposAny2 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 669, lgposAny1 ) );

    rm.ClearPgno( 665, 670 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FPgnoSet( 668 ) );
    CHECK( !rm.FPgnoSet( 669 ) );
    CHECK( !rm.FAnyPgnoSet() );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny5 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny4 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 667, lgposAny3 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 668, lgposAny2 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 669, lgposAny1 ) );

    rm.ClearPgno( 665, 670 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FPgnoSet( 668 ) );
    CHECK( !rm.FPgnoSet( 669 ) );
    CHECK( !rm.FAnyPgnoSet() );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny5 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny4 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 667, lgposAny3 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 668, lgposAny2 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 669, lgposAny1 ) );

    rm.ClearPgno( 666, 669 );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FPgnoSet( 668 ) );
    CHECK( !rm.FPgnoSet( 669 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 665 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny5 ) == 0 );
    rm.ClearPgno( 665, 669 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FPgnoSet( 668 ) );
    CHECK( !rm.FPgnoSet( 669 ) );
    CHECK( !rm.FAnyPgnoSet() );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny5 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny4 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 667, lgposAny3 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 668, lgposAny2 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 669, lgposAny1 ) );

    rm.ClearPgno( 666, 680 );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FPgnoSet( 668 ) );
    CHECK( !rm.FPgnoSet( 669 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 665 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny5 ) == 0 );
    rm.ClearPgno( 665, 669 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FPgnoSet( 668 ) );
    CHECK( !rm.FPgnoSet( 669 ) );
    CHECK( !rm.FAnyPgnoSet() );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny5 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny4 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 667, lgposAny3 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 668, lgposAny2 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 669, lgposAny1 ) );

    rm.ClearPgno( 667, 669 );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FPgnoSet( 668 ) );
    CHECK( !rm.FPgnoSet( 669 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny4 ) == 0 );
    rm.ClearPgno( 665, 669 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FPgnoSet( 668 ) );
    CHECK( !rm.FPgnoSet( 669 ) );
    CHECK( !rm.FAnyPgnoSet() );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny5 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny4 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 667, lgposAny3 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 668, lgposAny2 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 669, lgposAny1 ) );

    rm.ClearPgno( 667, 680 );
    CHECK( rm.FPgnoSet( 665 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FPgnoSet( 668 ) );
    CHECK( !rm.FPgnoSet( 669 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 2 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny4 ) == 0 );
    rm.ClearPgno( 665, 669 );
    CHECK( !rm.FPgnoSet( 665 ) );
    CHECK( !rm.FPgnoSet( 666 ) );
    CHECK( !rm.FPgnoSet( 667 ) );
    CHECK( !rm.FPgnoSet( 668 ) );
    CHECK( !rm.FPgnoSet( 669 ) );
    CHECK( !rm.FAnyPgnoSet() );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 665, lgposAny5 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny4 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 667, lgposAny3 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 668, lgposAny2 ) );
    CHECK( JET_errSuccess == rm.ErrSetPgno( 669, lgposAny1 ) );

    rm.TermLogRedoMap();
}

JETUNITTEST( CLogRedoMap, SettingPgnoTwiceDoesNotChangeLgposOrErrOrDbTime )
{
    CLogRedoMap rm;
    PGNO pgno;
    RedoMapEntry rme;
    CPG cpg;

    CHECK( JET_errSuccess == rm.ErrInitLogRedoMap( ifmpNil ) );
    CHECK( !rm.FAnyPgnoSet() );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny1, JET_errPageNotInitialized, 10, 20, 30 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );
    CHECK( rme.err == JET_errPageNotInitialized );
    CHECK( rme.dbtimeBefore == 10 );
    CHECK( rme.dbtimePage == 20 );
    CHECK( rme.dbtimeAfter == 30 );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny1, JET_errFileIOBeyondEOF, 40, 50, 60 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );
    CHECK( rme.err == JET_errPageNotInitialized );
    CHECK( rme.dbtimeBefore == 10 );
    CHECK( rme.dbtimePage == 20 );
    CHECK( rme.dbtimeAfter == 30 );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny2, JET_errRecoveryVerifyFailure, 10, 20, 30 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );
    CHECK( rme.err == JET_errPageNotInitialized );
    CHECK( rme.dbtimeBefore == 10 );
    CHECK( rme.dbtimePage == 20 );
    CHECK( rme.dbtimeAfter == 30 );

    CHECK( JET_errSuccess == rm.ErrSetPgno( 666, lgposAny2, JET_errDbTimeTooOld, 40, 50, 60 ) );
    CHECK( rm.FPgnoSet( 666 ) );
    CHECK( rm.FAnyPgnoSet() );
    rm.GetOldestLgposEntry( &pgno, &rme, &cpg );
    CHECK( cpg == 1 );
    CHECK( pgno == 666 );
    CHECK( CmpLgpos( rme.lgpos, lgposAny1 ) == 0 );
    CHECK( rme.err == JET_errPageNotInitialized );
    CHECK( rme.dbtimeBefore == 10 );
    CHECK( rme.dbtimePage == 20 );
    CHECK( rme.dbtimeAfter == 30 );

    rm.TermLogRedoMap();
}
