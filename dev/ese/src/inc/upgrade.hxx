// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

ERR ErrUPGRADEConvertDatabase(
    PIB * const ppib,
    const IFMP ifmp,
    const JET_GRBIT grbit );


ERR ErrUPGRADEPossiblyConvertPage(
    CPAGE * const pcpage,
    const PGNO pgno,
    VOID * const pvWorkBuf );


ERR ErrUPGRADEConvertNode(
    CPAGE * const pcpage,
    const INT iline,
    VOID * const pvBuf );


ERR ErrDBUTLConvertRecords( JET_SESID sesid, const JET_DBUTIL_W * const pdbutil );

