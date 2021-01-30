// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#include "PageSizeClean.hxx"



typedef struct                      
{
    JET_COLUMNID    columnid;
    JET_COLTYP      coltyp;
    USHORT          wCountry;
    LANGID          langid;
    USHORT          cp;
    USHORT          wCollate;
    ULONG           cbMax;
    JET_GRBIT       grbit;
    ULONG           cbDefault;
    BYTE            *pbDefault;
    CHAR            szName[JET_cbNameMost + 1];
} INFOCOLUMNDEF;




CODECONST( JET_COLUMNDEF ) rgcolumndefGetObjectInfo_A[] =
{
    { sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, JET_bitColumnTTKey },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, JET_bitColumnTTKey },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypDateTime, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypDateTime, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed }
};

CODECONST( JET_COLUMNDEF ) rgcolumndefGetObjectInfo_W[] =
{
    { sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, usUniCodePage, 0, 0, JET_bitColumnTTKey },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, usUniCodePage, 0, 0, JET_bitColumnTTKey },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypDateTime, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypDateTime, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed }
};

const ULONG ccolumndefGetObjectInfoMax  = ( sizeof(rgcolumndefGetObjectInfo_A) / sizeof(JET_COLUMNDEF) );


#define iContainerName      0
#define iObjectName         1
#define iObjectType         2
#define iCRecord            5
#define iCPage              6
#define iGrbit              7
#define iFlags              8



CODECONST( JET_COLUMNDEF ) rgcolumndefGetColumnInfo_A[] =
{
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed | JET_bitColumnTTKey },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, JET_bitColumnTTKey },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLongBinary, 0, 0, 0, 0, 0, 0 },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, 0 },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, 0 }
};

CODECONST( JET_COLUMNDEF ) rgcolumndefGetColumnInfo_W[] =
{
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed | JET_bitColumnTTKey },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, usUniCodePage, 0, 0, JET_bitColumnTTKey },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLongBinary, 0, 0, 0, 0, 0, 0 },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, usUniCodePage, 0, 0, 0 },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, usUniCodePage, 0, 0, 0 }
};

CODECONST( JET_COLUMNDEF ) rgcolumndefGetColumnInfoCompact_A[] =
{
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, 0 },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypBinary, 0, 0, 0, 0, 0, 0 },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, 0 },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, 0 }
};

CODECONST( JET_COLUMNDEF ) rgcolumndefGetColumnInfoCompact_W[] =
{
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, usUniCodePage, 0, 0, 0 },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypBinary, 0, 0, 0, 0, 0, 0 },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, usUniCodePage, 0, 0, 0 },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, usUniCodePage, 0, 0, 0 }
};

const ULONG ccolumndefGetColumnInfoMax  = ( sizeof( rgcolumndefGetColumnInfo_A ) / sizeof( JET_COLUMNDEF ) );

#define iColumnPOrder       0
#define iColumnName         1
#define iColumnId           2
#define iColumnType         3
#define iColumnCountry      4
#define iColumnLangid       5
#define iColumnCp           6
#define iColumnCollate      7
#define iColumnSize         8
#define iColumnGrbit        9
#define iColumnDefault      10
#define iColumnTableName    11
#define iColumnColumnName   12



CODECONST( JET_COLUMNDEF ) rgcolumndefGetIndexInfo_A[] =
{
    { sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, JET_bitColumnTTKey },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed | JET_bitColumnTTKey },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, 0, 0, 0, 0 },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed }
};

CODECONST( JET_COLUMNDEF ) rgcolumndefGetIndexInfo_W[] =
{
    { sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, usUniCodePage, 0, 0, JET_bitColumnTTKey },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed | JET_bitColumnTTKey },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypShort, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypText, 0, 0, usUniCodePage, 0, 0, 0 },
    { sizeof(JET_COLUMNDEF), 0, JET_coltypLong, 0, 0, 0, 0, 0, JET_bitColumnFixed }
};

const ULONG ccolumndefGetIndexInfoMax   = ( sizeof( rgcolumndefGetIndexInfo_A ) / sizeof( JET_COLUMNDEF ) );

#define iIndexName          0
#define iIndexGrbit         1
#define iIndexCKey          2
#define iIndexCEntry        3
#define iIndexCPage         4
#define iIndexCCol          5
#define iIndexICol          6
#define iIndexColId         7
#define iIndexColType       8
#define iIndexCountry       9
#define iIndexLangid        10
#define iIndexCp            11
#define iIndexCollate       12
#define iIndexColBits       13
#define iIndexColName       14
#define iIndexLCMapFlags    15

extern const ULONG  cbIDXLISTNewMembersSinceOriginalFormat  = 4;




LOCAL ERR ErrINFOGetTableColumnInfo(
    FUCB            *pfucb,             
    const CHAR      *szColumnName,      
    INFOCOLUMNDEF   *pcolumndef )       
{
    ERR             err;
    FCB             *pfcb                   = pfucb->u.pfcb;
    TDB             *ptdb                   = pfcb->Ptdb();
    FCB             * const pfcbTemplate    = ptdb->PfcbTemplateTable();
    COLUMNID        columnidT;
    FIELD           *pfield                 = pfieldNil;    
    JET_GRBIT       grbit;                  

    Assert( pcolumndef != NULL );

    Assert( szColumnName != NULL || pcolumndef->columnid != 0 );
    if ( szColumnName != NULL )
    {
        if ( *szColumnName == '\0' )
            return ErrERRCheck( JET_errColumnNotFound );

        BOOL    fColumnWasDerived;
        CallR( ErrFILEGetPfieldAndEnterDML(
                    pfucb->ppib,
                    pfcb,
                    szColumnName,
                    &pfield,
                    &columnidT,
                    &fColumnWasDerived,
                    fFalse ) );
        if ( fColumnWasDerived )
        {
            ptdb->AssertValidDerivedTable();

            Assert( FCOLUMNIDTemplateColumn( columnidT ) );
            pfcb = pfcbTemplate;
            ptdb = pfcbTemplate->Ptdb();
            pfcb->EnterDML();
        }
        else
        {
            if ( FCOLUMNIDTemplateColumn( columnidT ) )
            {
                Assert( pfcb->FTemplateTable() );
            }
            else
            {
                Assert( !pfcb->FTemplateTable() );
            }
        }
    }
    else
    {
        const FID   fid = FidOfColumnid( pcolumndef->columnid );

        columnidT = pcolumndef->columnid;

        if ( FCOLUMNIDTemplateColumn( columnidT ) && !pfcb->FTemplateTable() )
        {
            pfcb->Ptdb()->AssertValidDerivedTable();

            pfcb = pfcbTemplate;
            Assert( pfcbNil != pfcb );
            Assert( pfcb->FTemplateTable() );

            ptdb = pfcbTemplate->Ptdb();
            Assert( ptdbNil != ptdb );
        }

        pfcb->EnterDML();

        pfield = pfieldNil;
        if ( FCOLUMNIDTagged( columnidT ) )
        {
            if ( fid >= ptdb->FidTaggedFirst() && fid <= ptdb->FidTaggedLast() )
                pfield = ptdb->PfieldTagged( columnidT );
        }
        else if ( FCOLUMNIDFixed( columnidT ) )
        {
            if ( fid >= ptdb->FidFixedFirst() && fid <= ptdb->FidFixedLast() )
                pfield = ptdb->PfieldFixed( columnidT );
        }
        else if ( FCOLUMNIDVar( columnidT ) )
        {
            if ( fid >= ptdb->FidVarFirst() && fid <= ptdb->FidVarLast() )
                pfield = ptdb->PfieldVar( columnidT );
        }

        if ( pfieldNil == pfield )
        {
            pfcb->LeaveDML();
            return ErrERRCheck( JET_errColumnNotFound );
        }
        Assert( !FFIELDCommittedDelete( pfield->ffield ) );
    }

    pfcb->AssertDML();
    Assert( ptdb->Pfield( columnidT ) == pfield );

    
    if ( FCOLUMNIDTagged( columnidT ) )
    {
        grbit = JET_bitColumnTagged;
    }
    else if ( FCOLUMNIDVar( columnidT ) )
    {
        grbit = 0;
    }
    else
    {
        Assert( FCOLUMNIDFixed( columnidT ) );
        grbit = JET_bitColumnFixed;
    }

    if ( FFUCBUpdatable( pfucb ) )
        grbit |= JET_bitColumnUpdatable;

    if ( FFIELDNotNull( pfield->ffield ) )
        grbit |= JET_bitColumnNotNULL;

    if ( FFIELDAutoincrement( pfield->ffield ) )
        grbit |= JET_bitColumnAutoincrement;

    if ( FFIELDVersion( pfield->ffield ) )
        grbit |= JET_bitColumnVersion;

    if ( FFIELDMultivalued( pfield->ffield ) )
        grbit |= JET_bitColumnMultiValued;

    if ( FFIELDEscrowUpdate( pfield->ffield ) )
        grbit |= JET_bitColumnEscrowUpdate;

    if ( FFIELDFinalize( pfield->ffield ) )
        grbit |= JET_bitColumnFinalize;

    if ( FFIELDDeleteOnZero( pfield->ffield ) )
        grbit |= JET_bitColumnDeleteOnZero;

    if ( FFIELDUserDefinedDefault( pfield->ffield ) )
        grbit |= JET_bitColumnUserDefinedDefault;

    if ( FFIELDCompressed( pfield->ffield ) )
        grbit |= JET_bitColumnCompressed;

    if ( FFIELDEncrypted( pfield->ffield ) )
        grbit |= JET_bitColumnEncrypted;

    if ( FFIELDPrimaryIndexPlaceholder( pfield->ffield ) )
        grbit |= JET_bitColumnRenameConvertToPrimaryIndexPlaceholder;

    pcolumndef->columnid    = columnidT;
    pcolumndef->coltyp      = pfield->coltyp;
    pcolumndef->wCountry    = countryDefault;

    LCID lcid;
    CallS( ErrNORMLocaleToLcid( PinstFromPfucb( pfucb )->m_wszLocaleNameDefault, &lcid ) );

    pcolumndef->langid      = LangidFromLcid( lcid );
    pcolumndef->cp          = pfield->cp;
    pcolumndef->wCollate    = 0;
    pcolumndef->grbit       = grbit;
    pcolumndef->cbMax       = pfield->cbMaxLen;
    pcolumndef->cbDefault   = 0;

    OSStrCbCopyA( pcolumndef->szName, sizeof(pcolumndef->szName), ptdb->SzFieldName( pfield->itagFieldName, fFalse ) );

    if( NULL != pcolumndef->pbDefault )
    {
        if ( FFIELDUserDefinedDefault( pfield->ffield ) )
        {

            CHAR        szCallback[JET_cbNameMost+1];
            ULONG       cchSzCallback           = 0;
            BYTE        rgbUserData[JET_cbCallbackUserDataMost];
            ULONG       cbUserData              = 0;
            CHAR        szDependantColumns[ (JET_ccolKeyMost*(JET_cbNameMost+1)) + 1 ];
            ULONG       cchDependantColumns     = 0;
            COLUMNID    columnidCallback        = columnidT;

            pfcb->LeaveDML();

            COLUMNIDResetFTemplateColumn( columnidCallback );

            err = ErrCATGetColumnCallbackInfo(
                    pfucb->ppib,
                    pfucb->ifmp,
                    pfcb->ObjidFDP(),
                    ( NULL == pfcbTemplate ? objidNil : pfcbTemplate->ObjidFDP() ),
                    columnidCallback,
                    szCallback,
                    sizeof( szCallback ),
                    &cchSzCallback,
                    rgbUserData,
                    sizeof( rgbUserData ),
                    &cbUserData,
                    szDependantColumns,
                    sizeof( szDependantColumns ),
                    &cchDependantColumns );
            if( err < 0 )
            {
                return err;
            }

            Assert( cchSzCallback <= sizeof( szCallback ) );
            Assert( cbUserData <= sizeof( rgbUserData ) );
            Assert( cchDependantColumns <= sizeof( szDependantColumns ) );
            Assert( '\0' == szCallback[cchSzCallback-1] );
            Assert( 0 == cchDependantColumns ||
                    ( '\0' == szDependantColumns[cchDependantColumns-1]
                    && '\0' == szDependantColumns[cchDependantColumns-2] ) );

            BYTE * const pbMin                  = pcolumndef->pbDefault;
            BYTE * const pbUserdefinedDefault   = pbMin;
            BYTE * const pbSzCallback           = pbUserdefinedDefault + sizeof( JET_USERDEFINEDDEFAULT_A );
            BYTE * const pbUserData             = pbSzCallback + cchSzCallback;
            BYTE * const pbDependantColumns     = pbUserData + cbUserData;
            BYTE * const pbMax                  = pbDependantColumns + cchDependantColumns;

            JET_USERDEFINEDDEFAULT_A * const puserdefineddefault = (JET_USERDEFINEDDEFAULT_A *)pbUserdefinedDefault;
            memcpy( pbSzCallback, szCallback, cchSzCallback );
            memcpy( pbUserData, rgbUserData, cbUserData );
            memcpy( pbDependantColumns, szDependantColumns, cchDependantColumns );

            puserdefineddefault->szCallback         = (CHAR *)pbSzCallback;
            puserdefineddefault->pbUserData         = rgbUserData;
            puserdefineddefault->cbUserData         = cbUserData;
            if( 0 != cchDependantColumns )
            {
                puserdefineddefault->szDependantColumns = (CHAR *)pbDependantColumns;
            }
            else
            {
                puserdefineddefault->szDependantColumns = NULL;
            }

            pcolumndef->cbDefault = ULONG( pbMax - pbMin );

            pfcb->EnterDML();
        }
        else if ( FFIELDDefault( pfield->ffield ) )
        {
            DATA    dataT;

            Assert( pfcb->Ptdb() == ptdb );
            err = ErrRECIRetrieveDefaultValue( pfcb, columnidT, &dataT );
            Assert( err >= JET_errSuccess );
            Assert( wrnRECSeparatedLV != err );
            Assert( wrnRECLongField != err );

            pcolumndef->cbDefault = dataT.Cb();
            UtilMemCpy( pcolumndef->pbDefault, dataT.Pv(), dataT.Cb() );
        }
    }

    pfcb->LeaveDML();

    return JET_errSuccess;
}

LOCAL ERR ErrInfoGetObjectInfo(
    PIB                 *ppib,
    const IFMP          ifmp,
    const CHAR          *szObjectName,
    VOID                *pv,
    const ULONG         cbMax,
    const BOOL          fStats );
LOCAL ERR ErrInfoGetObjectInfoList(
    PIB                 *ppib,
    const IFMP          ifmp,
    const JET_OBJTYP    objtyp,
    VOID                *pv,
    const ULONG         cbMax,
    const BOOL          fStats,
    const BOOL          fUnicodeNames);

LOCAL ERR ErrInfoGetTableColumnInfo(
    PIB                 *ppib,
    FUCB                *pfucb,
    const CHAR          *szColumnName,
    const JET_COLUMNID  *pcolid,
    VOID                *pv,
    const ULONG         cbMax );
LOCAL ERR ErrInfoGetTableColumnInfoList(
    PIB                 *ppib,
    FUCB                *pfucb,
    VOID                *pv,
    const ULONG         cbMax,
    const JET_GRBIT     grbit,
    const BOOL          fUnicodeNames );
LOCAL ERR ErrInfoGetTableColumnInfoBase(
    PIB                 *ppib,
    FUCB                *pfucb,
    const CHAR          *szColumnName,
    const JET_COLUMNID  *pcolid,
    VOID                *pv,
    const ULONG         cbMax );

LOCAL ERR ErrINFOGetTableIndexInfo(
    PIB                 *ppib,
    FUCB                *pfucb,
    const CHAR          *szIndexName,
    VOID                *pv,
    const ULONG         cbMax,
    const BOOL          fUnicodeNames );
LOCAL ERR ErrINFOGetTableIndexInfo(
    PIB             *ppib,
    FUCB            *pfucb,
    IFMP            ifmp,                   
    OBJID           objidTable,
    const CHAR      *szIndex,               
    void            *pb,
    ULONG           cbMax,
    ULONG           lInfoLevel,             
    const BOOL      fUnicodeNames );
LOCAL ERR ErrINFOGetTableIndexIdInfo(
    PIB                 * ppib,
    FUCB                * pfucb,
    const CHAR          * szIndexName,
    INDEXID             * pindexid );
LOCAL ERR ErrINFOGetTableIndexInfoForCreateIndex(
    PIB *               ppib,
    FUCB *              pfucb,
    const CHAR *        szIndexName,
    VOID *              pvResult,
    const ULONG         cbMax,
    const BOOL          fUnicodeNames,
    ULONG       lIdxVersion );


LOCAL const CHAR    szTcObject[]    = "Tables";
LOCAL const WCHAR   wszTcObject[]   = L"Tables";


ERR VDBAPI ErrIsamGetObjectInfo(
    JET_SESID       vsesid,             
    JET_DBID        vdbid,              
    JET_OBJTYP      objtyp,             
    const CHAR      *szContainer,       
    const CHAR      *szObject,          
    VOID            *pv,
    ULONG           cbMax,
    ULONG           lInfoLevel,         
    const BOOL      fUnicodeNames )
{
    ERR             err;
    PIB             *ppib           = (PIB *) vsesid;
    const IFMP      ifmp            = (IFMP)vdbid;
    CHAR            szObjectName[JET_cbNameMost+1];
    bool            fTransactionStarted = false;

    
    CallR( ErrPIBCheck( ppib ) );
    CallR( ErrPIBCheckIfmp( ppib, ifmp ) );

    if ( NULL != szContainer && '\0' != *szContainer )
    {
        CHAR    szContainerName[JET_cbNameMost+1];
        CallR( ErrUTILCheckName( szContainerName, szContainer, JET_cbNameMost+1 ) );
        if ( 0 != _stricmp( szContainerName, szTcObject ) )
        {
            err = ErrERRCheck( JET_errObjectNotFound );
            return err;
        }
    }

    if ( szObject == NULL || *szObject == '\0' )
        *szObjectName = '\0';
    else
        CallR( ErrUTILCheckName( szObjectName, szObject, JET_cbNameMost+1 ) );

    if ( ppib->Level() == 0 )
    {
        CallR( ErrDIRBeginTransaction( ppib, 42523, NO_GRBIT ) );
        fTransactionStarted = fTrue;
    }

    switch ( lInfoLevel )
    {
        case JET_ObjInfo:
        case JET_ObjInfoNoStats:
            err = ErrInfoGetObjectInfo(
                ppib,
                ifmp,
                szObjectName,
                pv,
                cbMax,
                JET_ObjInfo == lInfoLevel );
            break;

        case JET_ObjInfoList:
        case JET_ObjInfoListNoStats:
            err = ErrInfoGetObjectInfoList(
                ppib,
                ifmp,
                objtyp,
                pv,
                cbMax,
                JET_ObjInfoList == lInfoLevel,
                fUnicodeNames );
            break;

        case JET_ObjInfoSysTabCursor:
        case JET_ObjInfoSysTabReadOnly:
        case JET_ObjInfoListACM:
        case JET_ObjInfoRulesLoaded:
        default:
            Assert( fFalse );
            err = ErrERRCheck( JET_errInvalidParameter );
            break;
    }

    if( err >= 0 && fTransactionStarted )
    {
        err = ErrDIRCommitTransaction( ppib, NO_GRBIT );
    }

    if( err < 0 && fTransactionStarted )
    {
        const ERR   errT    = ErrDIRRollback( ppib );
        if ( JET_errSuccess != errT )
        {
            Assert( errT < 0 );
            Assert( PinstFromPpib( ppib )->FInstanceUnavailable() );
            Assert( JET_errSuccess != ppib->ErrRollbackFailure() );
        }
    }
    
    return err;
}


LOCAL ERR ErrInfoGetObjectInfo(
    PIB             *ppib,
    const IFMP      ifmp,
    const CHAR      *szObjectName,
    VOID            *pv,
    const ULONG     cbMax,
    const BOOL      fStats )
{
    ERR             err;
    FUCB            *pfucbInfo;
    JET_OBJECTINFO  objectinfo;

    
    if ( cbMax < sizeof( JET_OBJECTINFO ) )
    {
        return ErrERRCheck( JET_errBufferTooSmall );
    }

    CallR( ErrCATGetTableInfoCursor( ppib, ifmp, szObjectName, &pfucbInfo ) );

    
    objectinfo.cbStruct = sizeof( JET_OBJECTINFO );
    objectinfo.objtyp   = JET_objtypTable;

    
    objectinfo.grbit = JET_bitTableInfoBookmark | JET_bitTableInfoRollback;

    if ( FFUCBUpdatable( pfucbInfo ) )
    {
        objectinfo.grbit |= JET_bitTableInfoUpdatable;
    }

    ULONG   cbActual;
    Call( ErrIsamRetrieveColumn(
                ppib,
                pfucbInfo,
                fidMSO_Flags,
                &objectinfo.flags,
                sizeof( objectinfo.flags ),
                &cbActual,
                NO_GRBIT,
                NULL ) );
    CallS( err );
    Assert( sizeof(ULONG) == cbActual );

    
    if ( fStats )
    {
        LONG    cRecord, cPage;
        Call( ErrSTATSRetrieveTableStats(
                    ppib,
                    ifmp,
                    (CHAR *)szObjectName,
                    &cRecord,
                    NULL,
                    &cPage ) );

        objectinfo.cRecord  = cRecord;
        objectinfo.cPage    = cPage;
    }
    else
    {
        objectinfo.cRecord  = 0;
        objectinfo.cPage    = 0;
    }

    memcpy( pv, &objectinfo, sizeof( JET_OBJECTINFO ) );
    err = JET_errSuccess;

HandleError:
    CallS( ErrCATClose( ppib, pfucbInfo ) );
    return err;
}


LOCAL ERR ErrInfoGetObjectInfoList(
    PIB                 *ppib,
    const IFMP          ifmp,
    const JET_OBJTYP    objtyp,
    VOID                *pv,
    const ULONG         cbMax,
    const BOOL          fStats,
    const BOOL          fUnicodeNames )
{
    ERR                 err;
    const JET_SESID     sesid           = (JET_SESID)ppib;
    JET_TABLEID         tableid;
    JET_COLUMNID        rgcolumnid[ccolumndefGetObjectInfoMax];
    FUCB                *pfucbCatalog   = pfucbNil;
    const JET_OBJTYP    objtypTable     = JET_objtypTable;
    JET_GRBIT           grbitTable;
    ULONG               ulFlags;
    LONG                cRecord         = 0;        
    LONG                cPage           = 0;        
    ULONG               cRows           = 0;        
    ULONG               cbActual;
    CHAR                szObjectName[JET_cbNameMost+1];
    JET_OBJECTLIST      objectlist;

    
    C_ASSERT( sizeof(rgcolumndefGetObjectInfo_A) == sizeof(rgcolumndefGetObjectInfo_W) );
    CallR( ErrIsamOpenTempTable(
                sesid,
                (JET_COLUMNDEF *)( fUnicodeNames ? rgcolumndefGetObjectInfo_W : rgcolumndefGetObjectInfo_A ),
                ccolumndefGetObjectInfoMax,
                NULL,
                JET_bitTTScrollable|JET_bitTTIndexed,
                &tableid,
                rgcolumnid,
                JET_cbKeyMost_OLD,
                JET_cbKeyMost_OLD ) );

    if ( JET_objtypNil != objtyp && JET_objtypTable != objtyp )
    {
        goto ResetTempTblCursor;
    }

    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    Call( ErrIsamSetCurrentIndex( ppib, pfucbCatalog, szMSORootObjectsIndex ) );

    grbitTable = JET_bitTableInfoBookmark|JET_bitTableInfoRollback;

    if ( FFUCBUpdatable( pfucbCatalog ) )
        grbitTable |= JET_bitTableInfoUpdatable;


    err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveFirst, NO_GRBIT );
    while ( JET_errNoCurrentRecord != err )
    {
        Call( err );
        CallS( err );


#ifdef DEBUG
        SYSOBJ  sysobj;
        Call( ErrIsamRetrieveColumn(
                    ppib,
                    pfucbCatalog,
                    fidMSO_Type,
                    (BYTE *)&sysobj,
                    sizeof(sysobj),
                    &cbActual,
                    NO_GRBIT,
                    NULL ) );
        CallS( err );
        Assert( sizeof(SYSOBJ) == cbActual );
        Assert( sysobjTable == sysobj );
#endif

        Call( ErrIsamRetrieveColumn(
                    ppib,
                    pfucbCatalog,
                    fidMSO_Name,
                    szObjectName,
                    JET_cbNameMost,
                    &cbActual,
                    NO_GRBIT,
                    NULL ) );
        CallS( err );
        Assert( cbActual > 0 );
        Assert( cbActual <= JET_cbNameMost );
        if ( sizeof(szObjectName)/sizeof(szObjectName[0]) <= cbActual )
        {
            Error( ErrERRCheck(JET_errCatalogCorrupted) );
        }
        szObjectName[cbActual] = 0;

        Call( ErrIsamRetrieveColumn(
                    ppib,
                    pfucbCatalog,
                    fidMSO_Flags,
                    &ulFlags,
                    sizeof(ulFlags),
                    &cbActual,
                    NO_GRBIT,
                    NULL ) );
        CallS( err );
        Assert( sizeof(ULONG) == cbActual );

        if ( fStats )
        {
            Call( ErrSTATSRetrieveTableStats(
                        ppib,
                        ifmp,
                        szObjectName,
                        &cRecord,
                        NULL,
                        &cPage ) );
        }
        else
        {
            Assert( 0 == cRecord );
            Assert( 0 == cPage );
        }

        Call( ErrDispPrepareUpdate(
                    sesid,
                    tableid,
                    JET_prepInsert ) );

        Call( ErrDispSetColumn(
                    sesid,
                    tableid,
                    rgcolumnid[iContainerName],
                    fUnicodeNames ? (VOID *)wszTcObject : (VOID *)szTcObject,
                    (ULONG)( fUnicodeNames ? ( wcslen(wszTcObject) * sizeof( WCHAR ) ) : strlen(szTcObject) ),
                    NO_GRBIT,
                    NULL ) );
                    
        Call( ErrDispSetColumn(
                    sesid,
                    tableid,
                    rgcolumnid[iObjectType],
                    &objtypTable,
                    sizeof(objtypTable),
                    NO_GRBIT,
                    NULL ) );

        if ( fUnicodeNames )
        {
            WCHAR               wszObjectName[JET_cbNameMost+1];
            size_t              cwchActual;

            Call( ErrOSSTRAsciiToUnicode( szObjectName, wszObjectName, JET_cbNameMost + 1, &cwchActual ) );
            
            Call( ErrDispSetColumn(
                        sesid,
                        tableid,
                        rgcolumnid[iObjectName],
                        wszObjectName,
                        (ULONG) ( sizeof(WCHAR) * wcslen(wszObjectName) ),
                        NO_GRBIT,
                        NULL ) );
        }
        else
        {
            Call( ErrDispSetColumn(
                        sesid,
                        tableid,
                        rgcolumnid[iObjectName],
                        szObjectName,
                        (ULONG)strlen(szObjectName),
                        NO_GRBIT,
                        NULL ) );
        }
        
        Call( ErrDispSetColumn(
                    sesid,
                    tableid,
                    rgcolumnid[iFlags],
                    &ulFlags,
                    sizeof(ulFlags),
                    NO_GRBIT,
                    NULL ) );
        Call( ErrDispSetColumn(
                    sesid,
                    tableid,
                    rgcolumnid[iCRecord],
                    &cRecord,
                    sizeof(cRecord),
                    NO_GRBIT,
                    NULL ) );
        Call( ErrDispSetColumn(
                    sesid,
                    tableid,
                    rgcolumnid[iCPage],
                    &cPage,
                    sizeof(cPage),
                    NO_GRBIT,
                    NULL ) );
        Call( ErrDispSetColumn(
                    sesid,
                    tableid,
                    rgcolumnid[iGrbit],
                    &grbitTable,
                    sizeof(grbitTable),
                    NO_GRBIT,
                    NULL ) );

        Call( ErrDispUpdate(
                    sesid,
                    tableid,
                    NULL,
                    0,
                    NULL,
                    NO_GRBIT ) );

        cRows++;

        
        err = ErrIsamMove( ppib, pfucbCatalog, JET_MoveNext, NO_GRBIT );
    }

    CallS( ErrCATClose( ppib, pfucbCatalog ) );
    pfucbCatalog = pfucbNil;


ResetTempTblCursor:
    
    err = ErrDispMove( sesid, tableid, JET_MoveFirst, NO_GRBIT );
    if ( err < 0 )
    {
        if ( JET_errNoCurrentRecord != err )
            goto HandleError;
    }

    
    objectlist.cbStruct                 = sizeof(JET_OBJECTLIST);
    objectlist.tableid                  = tableid;
    objectlist.cRecord                  = cRows;
    objectlist.columnidcontainername    = rgcolumnid[iContainerName];
    objectlist.columnidobjectname       = rgcolumnid[iObjectName];
    objectlist.columnidobjtyp           = rgcolumnid[iObjectType];
    objectlist.columnidgrbit            = rgcolumnid[iGrbit];
    objectlist.columnidflags            = rgcolumnid[iFlags];
    objectlist.columnidcRecord          = rgcolumnid[iCRecord];
    objectlist.columnidcPage            = rgcolumnid[iCPage];

    AssertDIRNoLatch( ppib );
    Assert( pfucbNil == pfucbCatalog );

    memcpy( pv, &objectlist, sizeof( JET_OBJECTLIST ) );
    return JET_errSuccess;

HandleError:
    Assert( err < 0 );
    AssertDIRNoLatch( ppib );

    if ( pfucbNil != pfucbCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
    }

    (VOID)ErrDispCloseTable( sesid, tableid );

    return err;
}


LOCAL ERR ErrInfoGetTableAvailSpace(
    PIB * const ppib,
    FUCB * const pfucb,
    void * const pvResult,
    const ULONG cbMax )
{
    ERR         err         = JET_errSuccess;
    CPG* const  pcpg        = (CPG *)pvResult;
    CPG         cpgT        = 0;
    BOOL        fLeaveDML   = fFalse;
    FCB*        pfcbT       = pfcbNil;
    BOOL        fUnpinFCB   = fFalse;
    FUCB*       pfucbT      = pfucbNil;
    
    if ( sizeof( CPG ) != cbMax )
    {
        return ErrERRCheck( JET_errInvalidBufferSize );
    }
    *pcpg = 0;

    Call( ErrSPGetInfo( ppib, pfucb->ifmp, pfucb, (BYTE *)&cpgT, sizeof( cpgT ), fSPAvailExtent ) );
    *pcpg += cpgT;
    
    pfucb->u.pfcb->EnterDML();
    fLeaveDML = fTrue;
    for ( pfcbT = pfucb->u.pfcb->PfcbNextIndex(); pfcbNil != pfcbT; pfcbT = pfcbT->PfcbNextIndex() )
    {
        Assert( !pfcbT->Pidb()->FPrimary() );
        err = ErrFILEIAccessIndex( pfucb->ppib, pfucb->u.pfcb, pfcbT );
        if ( err != JET_errIndexNotFound )
        {
            Call( err );
        }
        
        if ( err != JET_errIndexNotFound )
        {
            if ( !pfcbT->Pidb()->FTemplateIndex() )
            {
                pfcbT->Pidb()->IncrementCurrentIndex();
                fUnpinFCB = fTrue;
            }
            pfucb->u.pfcb->LeaveDML();
            fLeaveDML = fFalse;

            Call( ErrDIROpen( ppib, pfcbT, &pfucbT ) );
            Call( ErrSPGetInfo( ppib, pfucbT->ifmp, pfucbT, (BYTE *)&cpgT, sizeof( cpgT ), fSPAvailExtent ) );
            *pcpg += cpgT;
            DIRClose( pfucbT );
            pfucbT = pfucbNil;

            pfucb->u.pfcb->EnterDML();
            fLeaveDML = fTrue;
            if ( fUnpinFCB )
            {
                pfcbT->Pidb()->DecrementCurrentIndex();
                fUnpinFCB = fFalse;
            }
        }
    }
    pfucb->u.pfcb->LeaveDML();
    fLeaveDML = fFalse;

    Call( ErrFILEOpenLVRoot( pfucb, &pfucbT, fFalse ) )
    if ( JET_errSuccess == err )
    {
        Call( ErrSPGetInfo( ppib, pfucbT->ifmp, pfucbT, (BYTE *)&cpgT, sizeof( cpgT ), fSPAvailExtent ) );
        *pcpg += cpgT;
        DIRClose( pfucbT );
        pfucbT = pfucbNil;
    }

    err = JET_errSuccess;

HandleError:
    if ( fUnpinFCB )
    {
        pfcbT->Pidb()->DecrementCurrentIndex();
    }
    if ( fLeaveDML )
    {
        pfucb->u.pfcb->LeaveDML();
    }
    if ( pfucbNil != pfucbT )
    {
        DIRClose( pfucbT );
    }
    return err;
}


ERR VTAPI ErrIsamSetTableInfo(
    JET_SESID sesid,
    JET_VTID vtid,
    _In_reads_bytes_opt_(cbParam) const void  *pvParam,
    ULONG cbParam,
    ULONG InfoLevel)
{
    ERR             err;
    PIB             *ppib       = (PIB *)sesid;
    FUCB            *pfucb      = (FUCB *)vtid;

    CallR( ErrPIBCheck( ppib ) );
    CheckTable( ppib, pfucb );

    OSTraceFMP(
        pfucb->ifmp,
        JET_tracetagDDLWrite,
        OSFormat(
            "Session=[0x%p:0x%x] setting table info for objid=[0x%x:0x%x] [level=0x%x]",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            (ULONG)pfucb->ifmp,
            pfucb->u.pfcb->ObjidFDP(),
            InfoLevel ) );

    switch( InfoLevel )
    {
        case JET_TblInfoEncryptionKey:
            FUCBRemoveEncryptionKey( pfucb );
            if ( cbParam > 0 )
            {
                err = ErrOSEncryptionVerifyKey( (BYTE*)pvParam, cbParam );
                if ( err < JET_errSuccess )
                {
                    AssertSz( fFalse, "Client is giving us a bad key" );
                    return err;
                }
                AllocR( pfucb->pbEncryptionKey = (BYTE*)PvOSMemoryHeapAlloc( cbParam ) );
                memcpy( pfucb->pbEncryptionKey, pvParam, cbParam );
                pfucb->cbEncryptionKey = cbParam;
            }
            return JET_errSuccess;

        default:
            Assert( fFalse );
            return ErrERRCheck( JET_errFeatureNotAvailable );
    }
}

ERR VTAPI ErrIsamGetTableInfo(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    void            *pvResult,
    ULONG   cbMax,
    ULONG   lInfoLevel )
{
    ERR             err;
    PIB             *ppib       = (PIB *)vsesid;
    FUCB            *pfucb      = (FUCB *)vtid;
    CHAR            szTableName[JET_cbNameMost+1];

    CallR( ErrPIBCheck( ppib ) );
    CheckTable( ppib, pfucb );

    OSTraceFMP(
        pfucb->ifmp,
        JET_tracetagDDLRead,
        OSFormat(
            "Session=[0x%p:0x%x] retrieving table info for objid=[0x%x:0x%x] [level=0x%x]",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            (ULONG)pfucb->ifmp,
            pfucb->u.pfcb->ObjidFDP(),
            lInfoLevel ) );

    
    switch( lInfoLevel )
    {
        case JET_TblInfo:
        case JET_TblInfoName:
        case JET_TblInfoTemplateTableName:
        case JET_TblInfoDbid:
            break;

        case JET_TblInfoOLC:
        case JET_TblInfoResetOLC:
            return ErrERRCheck( JET_errFeatureNotAvailable );

        case JET_TblInfoSpaceAlloc:
            
            Assert( cbMax >= sizeof(ULONG) * 2);
            err = ErrCATGetTableAllocInfo(
                    ppib,
                    pfucb->ifmp,
                    pfucb->u.pfcb->ObjidFDP(),
                    (ULONG *)pvResult,
                    ((ULONG *)pvResult) + 1);
            return err;

        case JET_TblInfoSpaceUsage:
        {
            BYTE    fSPExtents = fSPOwnedExtent|fSPAvailExtent;

            if ( cbMax > 2 * sizeof(CPG) )
                fSPExtents |= fSPExtentList;

            err = ErrSPGetInfo(
                        ppib,
                        pfucb->ifmp,
                        pfucb,
                        static_cast<BYTE *>( pvResult ),
                        cbMax,
                        fSPExtents );
            return err;
        }

        case JET_TblInfoSpaceOwned:
            err = ErrSPGetInfo(
                        ppib,
                        pfucb->ifmp,
                        pfucb,
                        static_cast<BYTE *>( pvResult ),
                        cbMax,
                        fSPOwnedExtent );
            return err;

        case JET_TblInfoSpaceAvailable:
            err = ErrInfoGetTableAvailSpace(
                    ppib,
                    pfucb,
                    pvResult,
                    cbMax );
            return err;

        case JET_TblInfoDumpTable:
            Assert( fFalse );
            return ErrERRCheck( JET_errFeatureNotAvailable );

        case JET_TblInfoLVChunkMax:
            if ( cbMax < sizeof(LONG) )
            {
                return ErrERRCheck( JET_errBufferTooSmall );
            }
            *(LONG *)pvResult = pfucb->u.pfcb->Ptdb()->CbLVChunkMost();
            return JET_errSuccess;

        case JET_TblInfoEncryptionKey:
            if ( cbMax < pfucb->cbEncryptionKey )
            {
                return ErrERRCheck( JET_errBufferTooSmall );
            }
#ifdef DEBUG
            if ( pfucb->cbEncryptionKey > 0 )
            {
                err = ErrOSEncryptionVerifyKey( pfucb->pbEncryptionKey, pfucb->cbEncryptionKey );
                if ( err < JET_errSuccess )
                {
                    AssertSz( fFalse, "Client should not have been able to save a bad encryption key" );
                }
            }
#endif
            memcpy( pvResult, pfucb->pbEncryptionKey, pfucb->cbEncryptionKey );
            return JET_errSuccess;

        default:
            Assert( fFalse );
            return ErrERRCheck( JET_errFeatureNotAvailable );
    }


    Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );
    pfucb->u.pfcb->EnterDML();
    Assert( strlen( pfucb->u.pfcb->Ptdb()->SzTableName() ) <= JET_cbNameMost );
    OSStrCbCopyA( szTableName, sizeof(szTableName), pfucb->u.pfcb->Ptdb()->SzTableName() );
    pfucb->u.pfcb->LeaveDML();

    switch ( lInfoLevel )
    {
        case JET_TblInfo:
        {
            JET_OBJECTINFO  objectinfo;
            LONG            cRecord;
            LONG            cPage;

            
            if ( cbMax < sizeof( JET_OBJECTINFO ) )
            {
                err = ErrERRCheck( JET_errBufferTooSmall );
                goto HandleError;
            }

            if ( pfucb->u.pfcb->FTypeTemporaryTable() )
            {
                err = ErrERRCheck( JET_errObjectNotFound );
                goto HandleError;
            }

            Assert( !FFMPIsTempDB( pfucb->u.pfcb->Ifmp() ) );

            
            objectinfo.cbStruct = sizeof(JET_OBJECTINFO);
            objectinfo.objtyp   = JET_objtypTable;
            objectinfo.flags    = 0;

            if ( FCATSystemTable( pfucb->u.pfcb->PgnoFDP() ) )
                objectinfo.flags |= JET_bitObjectSystem;
            else if ( FOLDSystemTable( szTableName ) )
                objectinfo.flags |= JET_bitObjectSystemDynamic;
            else if ( FCATUnicodeFixupTable( szTableName ) )
                objectinfo.flags |= JET_bitObjectSystemDynamic;
            else if ( FSCANSystemTable( szTableName ) )
                objectinfo.flags |= JET_bitObjectSystemDynamic;
            else if ( FCATObjidsTable( szTableName ) )
                objectinfo.flags |= JET_bitObjectSystemDynamic;
            else if ( MSysDBM::FIsSystemTable( szTableName ) )
                objectinfo.flags |= JET_bitObjectSystemDynamic;
            else if ( FCATLocalesTable( szTableName ) )
                objectinfo.flags |= JET_bitObjectSystemDynamic;

            if ( pfucb->u.pfcb->FFixedDDL() )
                objectinfo.flags |= JET_bitObjectTableFixedDDL;

            Assert( !( pfucb->u.pfcb->FTemplateTable() && pfucb->u.pfcb->FDerivedTable() ) );
            if ( pfucb->u.pfcb->FTemplateTable() )
                objectinfo.flags |= JET_bitObjectTableTemplate;
            else if ( pfucb->u.pfcb->FDerivedTable() )
                objectinfo.flags |= JET_bitObjectTableDerived;

            
            objectinfo.grbit = JET_bitTableInfoBookmark | JET_bitTableInfoRollback;
            if ( FFUCBUpdatable( pfucb ) )
                objectinfo.grbit |= JET_bitTableInfoUpdatable;

            Call( ErrSTATSRetrieveTableStats(
                        pfucb->ppib,
                        pfucb->ifmp,
                        szTableName,
                        &cRecord,
                        NULL,
                        &cPage ) );

            objectinfo.cRecord  = cRecord;
            objectinfo.cPage    = cPage;

            memcpy( pvResult, &objectinfo, sizeof( JET_OBJECTINFO ) );

            break;
        }

        case JET_TblInfoRvt:
            err = ErrERRCheck( JET_errQueryNotSupported );
            break;

        case JET_TblInfoName:
        case JET_TblInfoMostMany:
            if ( pfucb->u.pfcb->FTypeTemporaryTable() )
            {
                err = ErrERRCheck( JET_errInvalidOperation );
                goto HandleError;
            }
            if ( strlen( szTableName ) >= cbMax )
                err = ErrERRCheck( JET_errBufferTooSmall );
            else
            {
                OSStrCbCopyA( static_cast<CHAR *>( pvResult ), cbMax, szTableName );
            }
            break;

        case JET_TblInfoDbid:
            if ( pfucb->u.pfcb->FTypeTemporaryTable() )
            {
                err = ErrERRCheck( JET_errInvalidOperation );
                goto HandleError;
            }
            
            if ( cbMax < sizeof(JET_DBID) )
            {
                err = ErrERRCheck( JET_errBufferTooSmall );
                goto HandleError;
            }
            else
            {
                *(JET_DBID *)pvResult = (JET_DBID)pfucb->ifmp;
            }
            break;

        case JET_TblInfoTemplateTableName:
            if ( pfucb->u.pfcb->FTypeTemporaryTable() )
            {
                err = ErrERRCheck( JET_errInvalidOperation );
                goto HandleError;
            }

            if ( cbMax <= JET_cbNameMost )
                err = ErrERRCheck( JET_errBufferTooSmall );
            else if ( pfucb->u.pfcb->FDerivedTable() )
            {
                FCB     *pfcbTemplateTable = pfucb->u.pfcb->Ptdb()->PfcbTemplateTable();
                Assert( pfcbNil != pfcbTemplateTable );
                Assert( pfcbTemplateTable->FFixedDDL() );
                Assert( strlen( pfcbTemplateTable->Ptdb()->SzTableName() ) <= JET_cbNameMost );
                OSStrCbCopyA( (CHAR *)pvResult, cbMax, pfcbTemplateTable->Ptdb()->SzTableName() );
            }
            else
            {
                *( (CHAR *)pvResult ) = '\0';
            }
            break;

        default:
            err = ErrERRCheck( JET_errInvalidParameter );
    }

HandleError:
    return err;
}



ERR VDBAPI ErrIsamGetColumnInfo(
    JET_SESID       vsesid,                 
    JET_DBID        vdbid,                  
    const CHAR      *szTable,               
    const CHAR      *szColumnName,          
    JET_COLUMNID    *pcolid,                
    VOID            *pv,
    ULONG   cbMax,
    ULONG   lInfoLevel,
    const BOOL      fUnicodeNames )
{
    PIB             *ppib = (PIB *) vsesid;
    ERR             err;
    IFMP            ifmp;
    CHAR            szTableName[ JET_cbNameMost+1 ];
    FUCB            *pfucb;

    
    CallR( ErrPIBCheck( ppib ) );
    CallR( ErrPIBCheckIfmp( ppib, (IFMP)vdbid ) );
    ifmp = (IFMP) vdbid;
    if ( szTable == NULL )
        return ErrERRCheck( JET_errInvalidParameter );
    CallR( ErrUTILCheckName( szTableName, szTable, JET_cbNameMost+1 ) );

    CallR( ErrFILEOpenTable( ppib, ifmp, &pfucb, szTableName ) );
    Assert( pfucbNil != pfucb );

    Assert( ( g_rgfmp[ifmp].FReadOnlyAttach() && !FFUCBUpdatable( pfucb ) )
        || ( !g_rgfmp[ifmp].FReadOnlyAttach() && FFUCBUpdatable( pfucb ) ) );
    FUCBResetUpdatable( pfucb );

    Call( ErrIsamGetTableColumnInfo(
                (JET_SESID)ppib,
                (JET_VTID)pfucb,
                szColumnName,
                pcolid,
                pv,
                cbMax,
                lInfoLevel,
                fUnicodeNames ) );

HandleError:
    CallS( ErrFILECloseTable( ppib, pfucb ) );
    return err;
}



    ERR VTAPI
ErrIsamGetTableColumnInfo(
    JET_SESID           vsesid,         
    JET_VTID            vtid,           
    const CHAR          *szColumn,      
    const JET_COLUMNID  *pcolid,        
    void                *pb,
    ULONG       cbMax,
    ULONG       lInfoLevel,     
    const BOOL          fUnicodeNames )
{
    ERR                 err;
    PIB                 *ppib               = (PIB *)vsesid;
    FUCB                *pfucb              = (FUCB *)vtid;
    const ULONG         infolevelMask       = 0x0fffffff;
    const JET_GRBIT     grbitMask           = 0xf0000000;
    const JET_GRBIT     grbit               = ( lInfoLevel & grbitMask );
    CHAR                szColumnName[ (JET_cbNameMost + 1) ];
    BOOL                fTransactionStarted = fFalse;

    CallR( ErrPIBCheck( ppib ) );
    CheckTable( ppib, pfucb );
    if ( szColumn == NULL || *szColumn == '\0' )
    {
        szColumnName[0] = '\0';
    }
    else
    {
        CallR( ErrUTILCheckName( szColumnName, szColumn, ( JET_cbNameMost + 1 ) ) );
    }

    if ( ppib->Level() == 0 )
    {
        CallR( ErrDIRBeginTransaction( ppib, 33061, NO_GRBIT ) );
        fTransactionStarted = fTrue;
    }

    OSTraceFMP(
        pfucb->ifmp,
        JET_tracetagDDLRead,
        OSFormat(
            "Session=[0x%p:0x%x] retrieving info for column '%s' of objid=[0x%x:0x%x] [level=0x%x]",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            ( 0 != szColumnName[0] ? szColumnName : "<null>" ),
            (ULONG)pfucb->ifmp,
            pfucb->u.pfcb->ObjidFDP(),
            lInfoLevel ) );

    switch ( lInfoLevel & infolevelMask )
    {
        case JET_ColInfo:
        case JET_ColInfoByColid:
            err = ErrInfoGetTableColumnInfo( ppib, pfucb, szColumnName, pcolid, pb, cbMax );
            break;
        case JET_ColInfoList:
            err = ErrInfoGetTableColumnInfoList( ppib, pfucb, pb, cbMax, grbit, fUnicodeNames );
            break;
        case JET_ColInfoListSortColumnid:
            err = ErrInfoGetTableColumnInfoList( ppib, pfucb, pb, cbMax, ( grbit | JET_ColInfoGrbitSortByColumnid ), fUnicodeNames );
            break;
        case JET_ColInfoBase:
        case JET_ColInfoBaseByColid:
            err = ErrInfoGetTableColumnInfoBase( ppib, pfucb, szColumnName, pcolid, pb, cbMax );
            break;
        case JET_ColInfoListCompact:
            err = ErrInfoGetTableColumnInfoList( ppib, pfucb, pb, cbMax, ( grbit | JET_ColInfoGrbitCompacting ), fUnicodeNames );
            break;

        case JET_ColInfoSysTabCursor:
        default:
            Assert( fFalse );
            err = ErrERRCheck( JET_errInvalidParameter );
    }

    if( err >= 0 && fTransactionStarted )
    {
        err = ErrDIRCommitTransaction( ppib, NO_GRBIT );
    }

    if( err < 0 && fTransactionStarted )
    {
        const ERR   errT    = ErrDIRRollback( ppib );
        if ( JET_errSuccess != errT )
        {
            Assert( errT < 0 );
            Assert( PinstFromPpib( ppib )->FInstanceUnavailable() );
            Assert( JET_errSuccess != ppib->ErrRollbackFailure() );
        }
    }

    return err;
}


LOCAL ERR ErrInfoGetTableColumnInfo(
    PIB                 *ppib,
    FUCB                *pfucb,
    const CHAR          *szColumnName,
    const JET_COLUMNID  *pcolid,
    VOID                *pv,
    const ULONG         cbMax )
{
    ERR         err;
    INFOCOLUMNDEF   columndef;
    columndef.pbDefault = NULL;

    if ( cbMax < sizeof(JET_COLUMNDEF) || szColumnName == NULL )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( szColumnName[0] == '\0' )
    {
        if ( pcolid )
        {
            columndef.columnid = *pcolid;
            szColumnName = NULL;
        }
        else
        {
            columndef.columnid = 0;
        }
    }

    CallR( ErrINFOGetTableColumnInfo( pfucb, szColumnName, &columndef ) );

    ((JET_COLUMNDEF *)pv)->cbStruct = sizeof(JET_COLUMNDEF);
    ((JET_COLUMNDEF *)pv)->columnid = columndef.columnid;
    ((JET_COLUMNDEF *)pv)->coltyp   = columndef.coltyp;
    ((JET_COLUMNDEF *)pv)->cbMax    = columndef.cbMax;
    ((JET_COLUMNDEF *)pv)->grbit    = columndef.grbit;
    ((JET_COLUMNDEF *)pv)->wCollate = 0;
    ((JET_COLUMNDEF *)pv)->cp       = columndef.cp;
    ((JET_COLUMNDEF *)pv)->wCountry = columndef.wCountry;
    ((JET_COLUMNDEF *)pv)->langid   = columndef.langid;

    return JET_errSuccess;
}


LOCAL ERR ErrINFOSetTableColumnInfoList(
    PIB             *ppib,
    JET_TABLEID     tableid,
    const CHAR      *szTableName,
    COLUMNID        *rgcolumnid,
    INFOCOLUMNDEF   *pcolumndef,
    const JET_GRBIT grbit,
    const BOOL      fUnicodeNames )
{
    ERR             err;
    const BOOL      fMinimalInfo        = ( grbit & JET_ColInfoGrbitMinimalInfo );
    const BOOL      fOrderByColid       = ( grbit & JET_ColInfoGrbitSortByColumnid );

    WCHAR           wszName[JET_cbNameMost + 1];
    size_t          cwchActual;
    
    Call( ErrDispPrepareUpdate( (JET_SESID)ppib, tableid, JET_prepInsert ) );

    if ( fOrderByColid )
    {
        Call( ErrDispSetColumn(
                    (JET_SESID)ppib,
                    tableid,
                    rgcolumnid[iColumnPOrder],
                    (BYTE *)&pcolumndef->columnid,
                    sizeof(pcolumndef->columnid),
                    NO_GRBIT,
                    NULL ) );
    }

    if ( fUnicodeNames )
    {
        Call( ErrOSSTRAsciiToUnicode( pcolumndef->szName, wszName, _countof(wszName), &cwchActual ) );

        Call( ErrDispSetColumn(
                    (JET_SESID)ppib,
                    tableid,
                    rgcolumnid[iColumnName],
                    wszName,
                    (ULONG)( sizeof( WCHAR ) * wcslen( wszName ) ),
                    NO_GRBIT,
                    NULL ) );

    }
    else
    {
        Call( ErrDispSetColumn(
                    (JET_SESID)ppib,
                    tableid,
                    rgcolumnid[iColumnName],
                    pcolumndef->szName,
                    (ULONG)strlen( pcolumndef->szName ),
                    NO_GRBIT,
                    NULL ) );
    }
    
    Call( ErrDispSetColumn(
                (JET_SESID)ppib,
                tableid,
                rgcolumnid[iColumnId],
                (BYTE *)&pcolumndef->columnid,
                sizeof(pcolumndef->columnid),
                NO_GRBIT,
                NULL ) );

    if ( !fMinimalInfo )
    {
        Call( ErrDispSetColumn(
                    (JET_SESID)ppib,
                    tableid,
                    rgcolumnid[iColumnType],
                    (BYTE *)&pcolumndef->coltyp,
                    sizeof(pcolumndef->coltyp),
                    NO_GRBIT,
                    NULL ) );
        Call( ErrDispSetColumn(
                    (JET_SESID)ppib,
                    tableid,
                    rgcolumnid[iColumnCountry],
                    &pcolumndef->wCountry,
                    sizeof( pcolumndef->wCountry ),
                    NO_GRBIT,
                    NULL ) );
        Call( ErrDispSetColumn(
                    (JET_SESID)ppib,
                    tableid,
                    rgcolumnid[iColumnLangid],
                    &pcolumndef->langid,
                    sizeof( pcolumndef->langid ),
                    NO_GRBIT,
                    NULL ) );
        Call( ErrDispSetColumn(
                    (JET_SESID)ppib,
                    tableid,
                    rgcolumnid[iColumnCp],
                    &pcolumndef->cp,
                    sizeof(pcolumndef->cp),
                    NO_GRBIT,
                    NULL ) );
        Call( ErrDispSetColumn(
                    (JET_SESID)ppib,
                    tableid,
                    rgcolumnid[iColumnSize],
                    (BYTE *)&pcolumndef->cbMax,
                    sizeof(pcolumndef->cbMax),
                    NO_GRBIT,
                    NULL ) );
        Call( ErrDispSetColumn(
                    (JET_SESID)ppib,
                    tableid,
                    rgcolumnid[iColumnGrbit],
                    &pcolumndef->grbit,
                    sizeof(pcolumndef->grbit),
                    NO_GRBIT,
                    NULL ) );

        Call( ErrDispSetColumn(
                    (JET_SESID)ppib,
                    tableid,
                    rgcolumnid[iColumnCollate],
                    &pcolumndef->wCollate,
                    sizeof(pcolumndef->wCollate),
                    NO_GRBIT,
                    NULL ) );

        if ( pcolumndef->cbDefault > 0 )
        {
            Call( ErrDispSetColumn(
                    (JET_SESID)ppib,
                    tableid,
                    rgcolumnid[iColumnDefault],
                    pcolumndef->pbDefault,
                    pcolumndef->cbDefault,
                    NO_GRBIT,
                    NULL ) );
        }

        if ( fUnicodeNames )
        {
            Call( ErrOSSTRAsciiToUnicode( szTableName, wszName, _countof(wszName), &cwchActual ) );

            Call( ErrDispSetColumn(
                        (JET_SESID)ppib,
                        tableid,
                        rgcolumnid[iColumnTableName],
                        wszName,
                        (ULONG)( sizeof( WCHAR ) * ( wcslen( wszName ) ) ),
                        NO_GRBIT,
                        NULL ) );

            Call( ErrOSSTRAsciiToUnicode( pcolumndef->szName, wszName, _countof(wszName), &cwchActual ) );
            
            Call( ErrDispSetColumn(
                        (JET_SESID)ppib,
                        tableid,
                        rgcolumnid[iColumnColumnName],
                        wszName,
                        (ULONG)( sizeof( WCHAR ) * ( wcslen( wszName ) ) ),
                        NO_GRBIT,
                        NULL ) );

        }
        else
        {
            Call( ErrDispSetColumn(
                        (JET_SESID)ppib,
                        tableid,
                        rgcolumnid[iColumnTableName],
                        szTableName,
                        (ULONG)strlen( szTableName ),
                        NO_GRBIT,
                        NULL ) );

            Call( ErrDispSetColumn(
                        (JET_SESID)ppib,
                        tableid,
                        rgcolumnid[iColumnColumnName],
                        pcolumndef->szName,
                        (ULONG)strlen( pcolumndef->szName ),
                        NO_GRBIT,
                        NULL ) );

        }
    }

    Call( ErrDispUpdate( (JET_SESID)ppib, tableid, NULL, 0, NULL, NO_GRBIT ) );

HandleError:
    return err;
}


BOOL g_fCompactTemplateTableColumnDropped = fFalse;

LOCAL ERR ErrInfoGetTableColumnInfoList(
    PIB             *ppib,
    FUCB            *pfucb,
    VOID            *pv,
    const ULONG     cbMax,
    const JET_GRBIT grbit,
    const BOOL      fUnicodeNames )
{
    ERR             err;
    JET_TABLEID     tableid;
    COLUMNID        rgcolumnid[ccolumndefGetColumnInfoMax];
    FCB             *pfcb                   = pfucb->u.pfcb;
    TDB             *ptdb                   = pfcb->Ptdb();
    FID             fid;
    FID             fidFixedFirst;
    FID             fidFixedLast;
    FID             fidVarFirst;
    FID             fidVarLast;
    FID             fidTaggedFirst;
    FID             fidTaggedLast;
    CHAR            szTableName[JET_cbNameMost+1];
    INFOCOLUMNDEF   columndef;
    ULONG           cRows                   = 0;
    const BOOL      fNonDerivedColumnsOnly  = ( grbit & JET_ColInfoGrbitNonDerivedColumnsOnly );
    const BOOL      fCompacting             = ( grbit & JET_ColInfoGrbitCompacting );
    const BOOL      fTemplateTable          = pfcb->FTemplateTable();

    columndef.pbDefault = NULL;

    
    if ( cbMax < sizeof(JET_COLUMNLIST) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    C_ASSERT( sizeof( rgcolumndefGetColumnInfoCompact_A ) == sizeof( rgcolumndefGetColumnInfo_A ) );
    C_ASSERT( sizeof( rgcolumndefGetColumnInfoCompact_A ) == sizeof( rgcolumndefGetColumnInfo_W ) );
    C_ASSERT( sizeof( rgcolumndefGetColumnInfoCompact_A ) == sizeof( rgcolumndefGetColumnInfoCompact_W ) );
    
    CallR( ErrIsamOpenTempTable(
                (JET_SESID)ppib,
                (JET_COLUMNDEF *)( fCompacting ? ( fUnicodeNames ? rgcolumndefGetColumnInfoCompact_W : rgcolumndefGetColumnInfoCompact_A ) :
                    (  fUnicodeNames ? rgcolumndefGetColumnInfo_W : rgcolumndefGetColumnInfo_A ) ),
                ccolumndefGetColumnInfoMax,
                NULL,
                JET_bitTTScrollable|JET_bitTTIndexed,
                &tableid,
                rgcolumnid,
                JET_cbKeyMost_OLD,
                JET_cbKeyMost_OLD ) );

    C_ASSERT( (sizeof(JET_USERDEFINEDDEFAULT)
                + JET_cbNameMost + 1 
                + JET_cbCallbackUserDataMost
                + ( ( JET_ccolKeyMost * ( JET_cbNameMost + 1 ) ) + 1 ) 
                ) < JET_cbCallbackDataAllMost );

    columndef.pbDefault = (BYTE *)PvOSMemoryHeapAlloc( JET_cbCallbackDataAllMost );
    if( NULL == columndef.pbDefault )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    pfcb->EnterDML();

    fidFixedFirst = ptdb->FidFixedFirst();
    fidVarFirst = ptdb->FidVarFirst();
    fidTaggedFirst = ptdb->FidTaggedFirst();

    fidFixedLast = ptdb->FidFixedLast();
    fidVarLast = ptdb->FidVarLast();
    fidTaggedLast = ptdb->FidTaggedLast();

    Assert( fidFixedFirst.FFixed() || ( ( pfcbNil != ptdb->PfcbTemplateTable() ) && fidFixedFirst.FFixedNoneFull() ) );
    Assert( fidVarFirst.FVar() || ( ( pfcbNil != ptdb->PfcbTemplateTable() ) && fidVarFirst.FVarNoneFull() ) );
    Assert( fidTaggedFirst.FTagged() || ( (pfcbNil != ptdb->PfcbTemplateTable() ) && fidVarFirst.FTaggedNoneFull() ) );

    Assert( fidFixedLast.FFixedNone() || fidFixedLast.FFixed() );
    Assert( fidVarLast.FVarNone() || fidVarLast.FVar() );
    Assert( fidTaggedLast.FTaggedNone() || fidTaggedLast.FTagged() );

    Assert( strlen( ptdb->SzTableName() ) <= JET_cbNameMost );
    OSStrCbCopyA( szTableName, sizeof(szTableName), ptdb->SzTableName() );

    pfcb->LeaveDML();

    if ( !fNonDerivedColumnsOnly
        && !fCompacting
        && pfcbNil != ptdb->PfcbTemplateTable() )
    {
        ptdb->AssertValidDerivedTable();
        Assert( !fTemplateTable );

        const FID   fidTemplateFixedFirst   = FID( fidtypFixed, fidlimMin );
        const FID   fidTemplateVarFirst     = FID( fidtypVar, fidlimMin );
        const FID   fidTemplateTaggedFirst  = FID( fidtypTagged, fidlimMin );
        
        const FID   fidTemplateFixedLast    = ptdb->PfcbTemplateTable()->Ptdb()->FidFixedLast();
        const FID   fidTemplateVarLast      = ptdb->PfcbTemplateTable()->Ptdb()->FidVarLast();
        const FID   fidTemplateTaggedLast   = ptdb->PfcbTemplateTable()->Ptdb()->FidTaggedLast();

        Assert( fidTemplateFixedFirst.FFixed() );
        Assert( fidTemplateVarFirst.FVar() );
        Assert( fidTemplateTaggedFirst.FTagged() );

        Assert( fidTemplateFixedLast.FFixedNone() || fidTemplateFixedLast.FFixed() );
        Assert( fidTemplateVarLast.FVarNone() || fidTemplateVarLast.FVar() );
        Assert( fidTemplateTaggedLast.FTaggedNone() || fidTemplateTaggedLast.FTagged() );

        FID rgrgfidTemplateTableIterationBounds[][2] = {
            { fidTemplateFixedFirst, fidTemplateFixedLast },
            { fidTemplateVarFirst, fidTemplateVarLast },
            { fidTemplateTaggedFirst, fidTemplateTaggedLast },
        };

        static_assert( 3 == _countof( rgrgfidTemplateTableIterationBounds ), "3 elements." );
        static_assert( 2 == _countof( rgrgfidTemplateTableIterationBounds[0] ), "each element an array of 2.");
        
        for ( INT iBounds = 0; iBounds < _countof( rgrgfidTemplateTableIterationBounds ); iBounds++ )
        {
            for ( FID_ITERATOR itfid( rgrgfidTemplateTableIterationBounds[iBounds][0], rgrgfidTemplateTableIterationBounds[iBounds][1] ); !itfid.FEnd(); itfid++ )
            {
                columndef.columnid = ColumnidOfFid( *itfid, fTrue );
                CallS( ErrINFOGetTableColumnInfo( pfucb, NULL, &columndef ) );
                
                Call( ErrINFOSetTableColumnInfoList(
                          ppib,
                          tableid,
                          szTableName,
                          rgcolumnid,
                          &columndef,
                          grbit,
                          fUnicodeNames) );
                
                cRows++;
            }
        }
    }

    FID rgrgfidIterationBounds[][2] = {
        { fidFixedFirst, fidFixedLast },
        { fidVarFirst, fidVarLast },
        { fidTaggedFirst, fidTaggedLast }
    };

    
    for ( INT iBounds=0; iBounds < _countof( rgrgfidIterationBounds ); iBounds++ )
    {
        for ( FID_ITERATOR itfid( rgrgfidIterationBounds[iBounds][0], rgrgfidIterationBounds[iBounds][1] ); !itfid.FEnd(); ++itfid )
        {
            columndef.columnid = ColumnidOfFid( *itfid, fTemplateTable );

            if ( !fTemplateTable )
            {
                err = ErrRECIAccessColumn( pfucb, columndef.columnid );
                if ( err < 0 )
                {
                    if ( JET_errColumnNotFound == err )
                        continue;
                    goto HandleError;
                }
            }
            
            CallS( ErrINFOGetTableColumnInfo( pfucb, NULL, &columndef ) );
            
            if ( !fCompacting || !( columndef.grbit & JET_bitColumnRenameConvertToPrimaryIndexPlaceholder ) )
            {
                Call( ErrINFOSetTableColumnInfoList(
                          ppib,
                          tableid,
                          szTableName,
                          rgcolumnid,
                          &columndef,
                          grbit,
                          fUnicodeNames) );
                
                cRows++;
            }
            else if ( fCompacting && ( columndef.grbit & JET_bitColumnRenameConvertToPrimaryIndexPlaceholder ) )
            {
                
                
                g_fCompactTemplateTableColumnDropped = fTrue;
            }
            
        }
    }

    
    err = ErrDispMove( (JET_SESID)ppib, tableid, JET_MoveFirst, NO_GRBIT );
    if ( err < 0  )
    {
        if ( err != JET_errNoCurrentRecord )
            goto HandleError;
        err = JET_errSuccess;
    }

    JET_COLUMNLIST  *pcolumnlist;
    pcolumnlist = reinterpret_cast<JET_COLUMNLIST *>( pv );
    pcolumnlist->cbStruct = sizeof(JET_COLUMNLIST);
    pcolumnlist->tableid = tableid;
    pcolumnlist->cRecord = cRows;
    pcolumnlist->columnidPresentationOrder = rgcolumnid[iColumnPOrder];
    pcolumnlist->columnidcolumnname = rgcolumnid[iColumnName];
    pcolumnlist->columnidcolumnid = rgcolumnid[iColumnId];
    pcolumnlist->columnidcoltyp = rgcolumnid[iColumnType];
    pcolumnlist->columnidCountry = rgcolumnid[iColumnCountry];
    pcolumnlist->columnidLangid = rgcolumnid[iColumnLangid];
    pcolumnlist->columnidCp = rgcolumnid[iColumnCp];
    pcolumnlist->columnidCollate = rgcolumnid[iColumnCollate];
    pcolumnlist->columnidcbMax = rgcolumnid[iColumnSize];
    pcolumnlist->columnidgrbit = rgcolumnid[iColumnGrbit];
    pcolumnlist->columnidDefault =  rgcolumnid[iColumnDefault];
    pcolumnlist->columnidBaseTableName = rgcolumnid[iColumnTableName];
    pcolumnlist->columnidBaseColumnName = rgcolumnid[iColumnColumnName];
    pcolumnlist->columnidDefinitionName = rgcolumnid[iColumnName];

    Assert( NULL != columndef.pbDefault );
    OSMemoryHeapFree( columndef.pbDefault );

    return JET_errSuccess;

HandleError:
    (VOID)ErrDispCloseTable( (JET_SESID)ppib, tableid );

    OSMemoryHeapFree( columndef.pbDefault );

    return err;
}


LOCAL ERR ErrInfoGetTableColumnInfoBase(
    PIB                 *ppib,
    FUCB                *pfucb,
    const CHAR          *szColumnName,
    const JET_COLUMNID  *pcolid,
    VOID                *pv,
    const ULONG         cbMax )
{
    ERR             err;
    INFOCOLUMNDEF   columndef;
    columndef.pbDefault = NULL;

    if ( cbMax < sizeof(JET_COLUMNBASE_A) || szColumnName == NULL )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( szColumnName[0] == '\0' )
    {
        if ( pcolid )
        {
            columndef.columnid = *pcolid;
            szColumnName = NULL;
        }
        else
        {
            columndef.columnid = 0;
        }
    }

    CallR( ErrINFOGetTableColumnInfo( pfucb, szColumnName, &columndef ) );

    Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );

    ((JET_COLUMNBASE_A *)pv)->cbStruct      = sizeof(JET_COLUMNBASE_A);
    ((JET_COLUMNBASE_A *)pv)->columnid      = columndef.columnid;
    ((JET_COLUMNBASE_A *)pv)->coltyp            = columndef.coltyp;
    ((JET_COLUMNBASE_A *)pv)->wFiller           = 0;
    ((JET_COLUMNBASE_A *)pv)->cbMax         = columndef.cbMax;
    ((JET_COLUMNBASE_A *)pv)->grbit         = columndef.grbit;
    OSStrCbCopyA( ( ( JET_COLUMNBASE_A *)pv )->szBaseColumnName, sizeof(( ( JET_COLUMNBASE_A *)pv )->szBaseColumnName), columndef.szName );
    ((JET_COLUMNBASE_A *)pv)->wCountry      = columndef.wCountry;
    ((JET_COLUMNBASE_A *)pv)->langid        = columndef.langid;
    ((JET_COLUMNBASE_A *)pv)->cp                = columndef.cp;

    pfucb->u.pfcb->EnterDML();
    OSStrCbCopyA( ( ( JET_COLUMNBASE_A *)pv )->szBaseTableName,
        sizeof(( ( JET_COLUMNBASE_A *)pv )->szBaseTableName),
        pfucb->u.pfcb->Ptdb()->SzTableName() );
    pfucb->u.pfcb->LeaveDML();

    return JET_errSuccess;
}



ERR VDBAPI
ErrIsamGetIndexInfo(
    JET_SESID       vsesid,                 
    JET_DBID        vdbid,                  
    const CHAR      *szTable,               
    const CHAR      *szIndexName,           
    VOID            *pv,
    ULONG           cbMax,
    ULONG           lInfoLevel,             
    const BOOL      fUnicodeNames )
{
    ERR             err;
    PIB             *ppib = (PIB *)vsesid;
    IFMP            ifmp;
    CHAR            szTableName[ JET_cbNameMost+1 ];
    FUCB            *pfucb = pfucbNil;
    PGNO            pgnoFDP = pgnoNull;
    OBJID           objidTable = objidNil;

    
    CallR( ErrPIBCheck( ppib ) );
    CallR( ErrPIBCheckIfmp( ppib, (IFMP)vdbid ) );
    ifmp = (IFMP) vdbid;
    CallR( ErrUTILCheckName( szTableName, szTable, ( JET_cbNameMost + 1 ) ) );

    switch ( lInfoLevel )
    {
        case JET_IdxInfoSpaceAlloc:
        case JET_IdxInfoLCID:
        case JET_IdxInfoLocaleName:
        case JET_IdxInfoSortVersion:
        case JET_IdxInfoDefinedSortVersion:
        case JET_IdxInfoSortId:
        case JET_IdxInfoVarSegMac:
        case JET_IdxInfoKeyMost:
            if ( !FCATHashLookup( ifmp, szTable, &pgnoFDP, &objidTable ) )
            {
                CallR( ErrCATSeekTable( ppib, ifmp, szTable, &pgnoFDP, &objidTable ) );
            }
            break;

        default:
            CallR( ErrFILEOpenTable( ppib, ifmp, &pfucb, szTableName ) );
            Assert( pfucbNil != pfucb );
            Assert( ( g_rgfmp[ifmp].FReadOnlyAttach() && !FFUCBUpdatable( pfucb ) )
                || ( !g_rgfmp[ifmp].FReadOnlyAttach() && FFUCBUpdatable( pfucb ) ) );
            FUCBResetUpdatable( pfucb );
            break;
    }

    Call( ErrINFOGetTableIndexInfo(
                ppib,
                pfucb,
                ifmp,
                objidTable,
                szIndexName,
                pv,
                cbMax,
                lInfoLevel,
                fUnicodeNames ) );

HandleError:
    if ( pfucb != pfucbNil )
    {
        CallS( ErrFILECloseTable( ppib, pfucb ) );
    }
    return err;
}



ERR VTAPI
ErrIsamGetTableIndexInfo(
    JET_SESID       vsesid,                 
    JET_VTID        vtid,                   
    const CHAR      *szIndex,               
    void            *pb,
    ULONG           cbMax,
    ULONG           lInfoLevel,             
    const BOOL      fUnicodeNames )
{
    return ErrINFOGetTableIndexInfo( (PIB*)vsesid, (FUCB*)vtid, ifmpNil, objidNil, szIndex, pb, cbMax, lInfoLevel, fUnicodeNames );
}

ERR ErrINFOGetTableIndexInfo(
    PIB             *ppib,
    FUCB            *pfucb,
    IFMP            ifmp,                   
    OBJID           objidTable,
    const CHAR      *szIndex,               
    void            *pb,
    ULONG           cbMax,
    ULONG           lInfoLevel,             
    const BOOL      fUnicodeNames )
{
    ERR             err = JET_errSuccess;
    CHAR            szIndexName[JET_cbNameMost+1];
    bool            fTransactionStarted = false;

    
    CallR( ErrPIBCheck( ppib ) );
    if ( pfucb != pfucbNil )
    {
        CheckTable( ppib, pfucb );
        ifmp = pfucb->ifmp;
        objidTable = pfucb->u.pfcb->ObjidFDP();
    }
    if ( szIndex == NULL || *szIndex == '\0' )
    {
        *szIndexName = '\0';
    }
    else
    {
        CallR( ErrUTILCheckName( szIndexName, szIndex, ( JET_cbNameMost + 1 ) ) );
    }

    OSTraceFMP(
        ifmp,
        JET_tracetagDDLRead,
        OSFormat(
            "Session=[0x%p:0x%x] retrieving info for index '%s' of objid=[0x%x:0x%x] [level=0x%x]",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            ( 0 != szIndexName[0] ? szIndexName : "<null>" ),
            (ULONG)ifmp,
            objidTable,
            lInfoLevel ) );

    if ( ppib->Level() == 0 )
    {
        CallR( ErrDIRBeginTransaction( ppib, 54811, NO_GRBIT ) );
        fTransactionStarted = fTrue;
    }

    switch ( lInfoLevel )
    {
        case JET_IdxInfo:
        case JET_IdxInfoList:
            Call( ErrINFOGetTableIndexInfo( ppib, pfucb, szIndexName, pb, cbMax, fUnicodeNames ) );
            break;
        case JET_IdxInfoIndexId:
            Assert( sizeof(JET_INDEXID) <= cbMax );
            Call( ErrINFOGetTableIndexIdInfo( ppib, pfucb, szIndexName, (INDEXID *)pb ) );
            break;
        case JET_IdxInfoSpaceAlloc:
            Assert( sizeof(ULONG) == cbMax );
            Call( ErrCATGetIndexAllocInfo(
                        ppib,
                        ifmp,
                        objidTable,
                        szIndexName,
                        (ULONG *)pb ) );
            break;
        case JET_IdxInfoLCID:
        {
            LCID    lcid    = lcidNone;
            Assert( sizeof(LANGID) == cbMax
                || sizeof(LCID) == cbMax );
            Call( ErrCATGetIndexLcid(
                        ppib,
                        ifmp,
                        objidTable,
                        szIndexName,
                        &lcid ) );
            if ( cbMax < sizeof(LCID) )
            {
                *(LANGID *)pb = LangidFromLcid( lcid );
            }
            else
            {
                *(LCID *)pb = lcid;
            }
        }
            break;
        case JET_IdxInfoLocaleName:
        {
            WCHAR* wszLocaleName = (WCHAR*) pb;

            Call( ErrCATGetIndexLocaleName(
                        ppib,
                        ifmp,
                        objidTable,
                        szIndexName,
                        wszLocaleName,
                        cbMax ) );
        }
            break;
        case JET_IdxInfoSortVersion:
        {
            Call( ErrCATGetIndexSortVersion(
                        ppib,
                        ifmp,
                        objidTable,
                        szIndexName,
                        (DWORD*) pb ) );
        }
            break;
        case JET_IdxInfoDefinedSortVersion:
        {
            Call( ErrCATGetIndexDefinedSortVersion(
                        ppib,
                        ifmp,
                        objidTable,
                        szIndexName,
                        (DWORD*) pb ) );
        }
            break;
        case JET_IdxInfoSortId:
        {
            Call( ErrCATGetIndexSortid(
                        ppib,
                        ifmp,
                        objidTable,
                        szIndexName,
                        (SORTID*) pb ) );
        }
            break;
        case JET_IdxInfoVarSegMac:
            Assert( sizeof(USHORT) == cbMax );
            Call( ErrCATGetIndexVarSegMac(
                        ppib,
                        ifmp,
                        objidTable,
                        szIndexName,
                        (USHORT *)pb ) );
            break;
        case JET_IdxInfoKeyMost:
            Assert( sizeof(USHORT) == cbMax );
            Call( ErrCATGetIndexKeyMost(
                        ppib,
                        ifmp,
                        objidTable,
                        szIndexName,
                        (USHORT *)pb ) );
            break;
        case JET_IdxInfoCount:
        {
            INT cIndexes = 1;
            FCB *pfcbT;
            FCB * const pfcbTable = pfucb->u.pfcb;

            pfcbTable->EnterDML();
            for ( pfcbT = pfcbTable->PfcbNextIndex();
                pfcbT != pfcbNil;
                pfcbT = pfcbT->PfcbNextIndex() )
            {
                err = ErrFILEIAccessIndex( pfucb->ppib, pfcbTable, pfcbT );
                if ( err < 0 )
                {
                    if ( JET_errIndexNotFound != err )
                    {
                        pfcbTable->LeaveDML();
                        goto HandleError;
                    }
                }
                else
                {
                    cIndexes++;
                }
            }
            pfcbTable->LeaveDML();

            Assert( sizeof(INT) == cbMax );
            *( (INT *)pb ) = cIndexes;

            err = JET_errSuccess;
            break;
        }
        case JET_IdxInfoCreateIndex:
        case JET_IdxInfoCreateIndex2:
        case JET_IdxInfoCreateIndex3:
            Call( ErrINFOGetTableIndexInfoForCreateIndex( ppib, pfucb, szIndexName, pb, cbMax, fUnicodeNames, lInfoLevel ) );
            break;
        case JET_IdxInfoSysTabCursor:
        case JET_IdxInfoOLC:
        case JET_IdxInfoResetOLC:
        default:
            Assert( fFalse );
            err = ErrERRCheck( JET_errInvalidParameter );
            break;
    }

HandleError:
    if( err >= 0 && fTransactionStarted )
    {
        err = ErrDIRCommitTransaction( ppib, NO_GRBIT );
    }

    if( err < 0 && fTransactionStarted )
    {
        const ERR   errT    = ErrDIRRollback( ppib );
        if ( JET_errSuccess != errT )
        {
            Assert( errT < 0 );
            Assert( PinstFromPpib( ppib )->FInstanceUnavailable() );
            Assert( JET_errSuccess != ppib->ErrRollbackFailure() );
        }
    }

    return err;
}


LOCAL ERR ErrINFOGetTableIndexInfo(
    PIB             *ppib,
    FUCB            *pfucb,
    const CHAR      *szIndexName,
    VOID            *pv,
    const ULONG     cbMax,
    const BOOL      fUnicodeNames)
{
    ERR             err;            
    FCB             *pfcbTable;
    FCB             *pfcbIndex;     
    TDB             *ptdb;          

    LONG            cRecord;        
    LONG            cKey;           
    LONG            cPage;          
    LONG            cRows;          

    JET_TABLEID     tableid;        
    JET_COLUMNID    columnid;       
    JET_GRBIT       grbit = 0;      
    JET_GRBIT       grbitColumn;    
    JET_COLUMNID    rgcolumnid[ccolumndefGetIndexInfoMax];

    WORD            wCollate = 0;
    WORD            wT;
    LANGID          langid;         
    DWORD           dwMapFlags;     

    Assert( NULL != szIndexName );
    BOOL            fIndexList = ( '\0' == *szIndexName );
    BOOL            fUpdatingLatchSet = fFalse;

    
    if ( cbMax < sizeof(JET_INDEXLIST) - cbIDXLISTNewMembersSinceOriginalFormat )
        return ErrERRCheck( JET_wrnBufferTruncated );

    const ULONG     cbIndexList     = sizeof(JET_INDEXLIST) -
                                        ( cbMax < sizeof(JET_INDEXLIST) ? cbIDXLISTNewMembersSinceOriginalFormat : 0 );

    C_ASSERT( sizeof(rgcolumndefGetIndexInfo_W) == sizeof(rgcolumndefGetIndexInfo_A) );
    
    CallR( ErrIsamOpenTempTable(
                (JET_SESID)ppib,
                (JET_COLUMNDEF *)( fUnicodeNames ? rgcolumndefGetIndexInfo_W : rgcolumndefGetIndexInfo_A ),
                ccolumndefGetIndexInfoMax,
                NULL,
                JET_bitTTScrollable|JET_bitTTIndexed,
                &tableid,
                rgcolumnid,
                JET_cbKeyMost_OLD,
                JET_cbKeyMost_OLD ) );

    cRows = 0;

    
    pfcbTable = pfucb->u.pfcb;
    Assert( pfcbTable != pfcbNil );

    ptdb = pfcbTable->Ptdb();
    Assert( ptdbNil != ptdb );

    Call( pfcbTable->ErrSetUpdatingAndEnterDML( ppib, fTrue ) );
    fUpdatingLatchSet = fTrue;

    
    pfcbTable->AssertDML();
    for ( pfcbIndex = pfcbTable; pfcbIndex != pfcbNil; pfcbIndex = pfcbIndex->PfcbNextIndex() )
    {
        Assert( pfcbIndex->Pidb() != pidbNil || pfcbIndex == pfcbTable );

        if ( pfcbIndex->Pidb() != pidbNil )
        {
            if ( fIndexList )
            {
                err = ErrFILEIAccessIndex( ppib, pfcbTable, pfcbIndex );
            }
            else
            {
                Assert( NULL != szIndexName );
                err = ErrFILEIAccessIndexByName( ppib, pfcbTable, pfcbIndex, szIndexName );
            }
            if ( err < 0 )
            {
                if ( JET_errIndexNotFound != err )
                {
                    pfcbTable->LeaveDML();
                    goto HandleError;
                }
            }
            else
                break;
        }
    }

    pfcbTable->AssertDML();

    if ( pfcbNil == pfcbIndex && !fIndexList )
    {
        pfcbTable->LeaveDML();
        err = ErrERRCheck( JET_errIndexNotFound );
        goto HandleError;
    }

    
    while ( pfcbIndex != pfcbNil )
    {
        CHAR    szCurrIndex[JET_cbNameMost+1];
        IDXSEG  rgidxseg[JET_ccolKeyMost];

        pfcbTable->AssertDML();

        const IDB       *pidb = pfcbIndex->Pidb();
        Assert( pidbNil != pidb );

        Assert( pidb->ItagIndexName() != 0 );
        OSStrCbCopyA(
            szCurrIndex,
            sizeof(szCurrIndex),
            pfcbTable->Ptdb()->SzIndexName( pidb->ItagIndexName(), pfcbIndex->FDerivedIndex() ) );

        const ULONG     cColumn = pidb->Cidxseg();      
        UtilMemCpy( rgidxseg, PidxsegIDBGetIdxSeg( pidb, pfcbTable->Ptdb() ), cColumn * sizeof(IDXSEG) );

        
        grbit = pidb->GrbitFromFlags();
        LCID lcid;
        Call( ErrNORMLocaleToLcid( pidb->WszLocaleName(), &lcid ) );
        langid = LangidFromLcid( lcid );
        dwMapFlags = pidb->DwLCMapFlags();

        pfcbTable->LeaveDML();

        
        for ( ULONG iidxseg = 0; iidxseg < cColumn; iidxseg++ )
        {
            FIELD   field;
            CHAR    szFieldName[JET_cbNameMost+1];
            WCHAR   wszName[JET_cbNameMost+1];
            size_t  cwchActual;

            Call( ErrDispPrepareUpdate( (JET_SESID)ppib, tableid, JET_prepInsert ) );

            if ( fUnicodeNames )
            {
                Call( ErrOSSTRAsciiToUnicode( szCurrIndex, wszName, _countof(wszName), &cwchActual ) );
            
                
                Call( ErrDispSetColumn(
                            (JET_SESID)ppib,
                            tableid,
                            rgcolumnid[iIndexName],
                            wszName,
                            (ULONG)( sizeof(WCHAR) * wcslen( wszName ) ),
                            NO_GRBIT,
                            NULL ) );
            }
            else
            {
                
                Call( ErrDispSetColumn(
                            (JET_SESID)ppib,
                            tableid,
                            rgcolumnid[iIndexName],
                            szCurrIndex,
                            (ULONG)strlen( szCurrIndex ),
                            NO_GRBIT,
                            NULL ) );
            }

            
            Call( ErrDispSetColumn(
                        (JET_SESID)ppib,
                        tableid,
                        rgcolumnid[iIndexGrbit],
                        &grbit,
                        sizeof( grbit ),
                        NO_GRBIT,
                        NULL ) );

            
            Call( ErrSTATSRetrieveIndexStats(
                        pfucb,
                        szCurrIndex,
                        pfcbIndex->FPrimaryIndex(),
                        &cRecord,
                        &cKey,
                        &cPage ) );
            Call( ErrDispSetColumn(
                        (JET_SESID)ppib,
                        tableid,
                        rgcolumnid[iIndexCKey],
                        &cKey,
                        sizeof( cKey ),
                        NO_GRBIT,
                        NULL ) );
            Call( ErrDispSetColumn(
                        (JET_SESID)ppib,
                        tableid,
                        rgcolumnid[iIndexCEntry],
                        &cRecord,
                        sizeof( cRecord ),
                        NO_GRBIT,
                        NULL ) );
            Call( ErrDispSetColumn(
                        (JET_SESID)ppib,
                        tableid,
                        rgcolumnid[iIndexCPage],
                        &cPage,
                        sizeof( cPage ),
                        NO_GRBIT,
                        NULL ) );

            
            Call( ErrDispSetColumn(
                        (JET_SESID)ppib,
                        tableid,
                        rgcolumnid[iIndexCCol],
                        &cColumn,
                        sizeof( cColumn ),
                        NO_GRBIT,
                        NULL ) );

            
            Call( ErrDispSetColumn(
                        (JET_SESID)ppib,
                        tableid,
                        rgcolumnid[iIndexICol],
                        &iidxseg,
                        sizeof( iidxseg ),
                        NO_GRBIT,
                        NULL ) );

            
            grbitColumn = ( rgidxseg[iidxseg].FDescending() ?
                                JET_bitKeyDescending :
                                JET_bitKeyAscending );

            
            columnid  = rgidxseg[iidxseg].Columnid();
            Call( ErrDispSetColumn(
                        (JET_SESID)ppib,
                        tableid,
                        rgcolumnid[iIndexColId],
                        &columnid,
                        sizeof( columnid ),
                        0,
                        NULL ) );

            
            if ( FCOLUMNIDTemplateColumn( columnid ) && !ptdb->FTemplateTable() )
            {
                ptdb->AssertValidDerivedTable();

                const TDB   * const ptdbTemplateTable = ptdb->PfcbTemplateTable()->Ptdb();

                field = *( ptdbTemplateTable->Pfield( columnid ) );
                OSStrCbCopyA( szFieldName, sizeof(szFieldName), ptdbTemplateTable->SzFieldName( field.itagFieldName, fFalse ) );
            }
            else
            {
                pfcbTable->EnterDML();
                field = *( ptdb->Pfield( columnid ) );
                OSStrCbCopyA( szFieldName, sizeof(szFieldName), ptdb->SzFieldName( field.itagFieldName, fFalse ) );
                pfcbTable->LeaveDML();
            }

            
            {
                JET_COLTYP coltyp = field.coltyp;
                Call( ErrDispSetColumn(
                            (JET_SESID)ppib,
                            tableid,
                            rgcolumnid[iIndexColType],
                            &coltyp,
                            sizeof( coltyp ),
                            NO_GRBIT,
                            NULL ) );
            }

            
            wT = countryDefault;
            Call( ErrDispSetColumn(
                        (JET_SESID)ppib,
                        tableid,
                        rgcolumnid[iIndexCountry],
                        &wT,
                        sizeof( wT ),
                        NO_GRBIT,
                        NULL ) );

            
            Call( ErrDispSetColumn(
                        (JET_SESID)ppib,
                        tableid,
                        rgcolumnid[iIndexLangid],
                        &langid,
                        sizeof( langid ),
                        NO_GRBIT,
                        NULL ) );

            
            Call( ErrDispSetColumn(
                        (JET_SESID)ppib,
                        tableid,
                        rgcolumnid[iIndexLCMapFlags],
                        &dwMapFlags,
                        sizeof( dwMapFlags ),
                        NO_GRBIT,
                        NULL ) );

            
            Call( ErrDispSetColumn(
                        (JET_SESID)ppib,
                        tableid,
                        rgcolumnid[iIndexCp],
                        &field.cp,
                        sizeof(field.cp),
                        NO_GRBIT,
                        NULL ) );

            
            Call( ErrDispSetColumn(
                        (JET_SESID)ppib,
                        tableid,
                        rgcolumnid[iIndexCollate],
                        &wCollate,
                        sizeof(wCollate),
                        NO_GRBIT,
                        NULL ) );

            
            Call( ErrDispSetColumn(
                        (JET_SESID)ppib,
                        tableid,
                        rgcolumnid[iIndexColBits],
                        &grbitColumn,
                        sizeof( grbitColumn ),
                        NO_GRBIT,
                        NULL ) );

            
            if ( fUnicodeNames )
            {
                Call( ErrOSSTRAsciiToUnicode( szFieldName, wszName, _countof(wszName), &cwchActual ) );
                
                Call( ErrDispSetColumn(
                            (JET_SESID)ppib,
                            tableid,
                            rgcolumnid[iIndexColName],
                            wszName,
                            (ULONG)( sizeof(WCHAR) * wcslen( wszName ) ),
                            NO_GRBIT,
                            NULL ) );
            }
            else
            {
                Call( ErrDispSetColumn(
                            (JET_SESID)ppib,
                            tableid,
                            rgcolumnid[iIndexColName],
                            szFieldName,
                            (ULONG)strlen( szFieldName ),
                            NO_GRBIT,
                            NULL ) );
            }

            Call( ErrDispUpdate( (JET_SESID)ppib, tableid, NULL, 0, NULL, NO_GRBIT ) );

            
            cRows++;
        }

        
        pfcbTable->EnterDML();
        if ( fIndexList )
        {
            for ( pfcbIndex = pfcbIndex->PfcbNextIndex(); pfcbIndex != pfcbNil; pfcbIndex = pfcbIndex->PfcbNextIndex() )
            {
                err = ErrFILEIAccessIndex( ppib, pfcbTable, pfcbIndex );
                if ( err < 0 )
                {
                    if ( JET_errIndexNotFound != err )
                    {
                        pfcbTable->LeaveDML();
                        goto HandleError;
                    }
                }
                else
                    break;
            }
        }
        else
        {
            pfcbIndex = pfcbNil;
        }
    }

    pfcbTable->ResetUpdatingAndLeaveDML();
    fUpdatingLatchSet = fFalse;

    
    err = ErrDispMove( (JET_SESID)ppib, tableid, JET_MoveFirst, NO_GRBIT );
    if ( err < 0  )
    {
        if ( err != JET_errNoCurrentRecord )
            goto HandleError;
        err = JET_errSuccess;
    }

    
    ((JET_INDEXLIST *)pv)->cbStruct = cbIndexList;
    ((JET_INDEXLIST *)pv)->tableid = tableid;
    ((JET_INDEXLIST *)pv)->cRecord = cRows;
    ((JET_INDEXLIST *)pv)->columnidindexname = rgcolumnid[iIndexName];
    ((JET_INDEXLIST *)pv)->columnidgrbitIndex = rgcolumnid[iIndexGrbit];
    ((JET_INDEXLIST *)pv)->columnidcEntry = rgcolumnid[iIndexCEntry];
    ((JET_INDEXLIST *)pv)->columnidcKey = rgcolumnid[iIndexCKey];
    ((JET_INDEXLIST *)pv)->columnidcPage = rgcolumnid[iIndexCPage];
    ((JET_INDEXLIST *)pv)->columnidcColumn = rgcolumnid[iIndexCCol];
    ((JET_INDEXLIST *)pv)->columnidiColumn = rgcolumnid[iIndexICol];
    ((JET_INDEXLIST *)pv)->columnidcolumnid = rgcolumnid[iIndexColId];
    ((JET_INDEXLIST *)pv)->columnidcoltyp = rgcolumnid[iIndexColType];
    ((JET_INDEXLIST *)pv)->columnidCountry = rgcolumnid[iIndexCountry];
    ((JET_INDEXLIST *)pv)->columnidLangid = rgcolumnid[iIndexLangid];
    ((JET_INDEXLIST *)pv)->columnidCp = rgcolumnid[iIndexCp];
    ((JET_INDEXLIST *)pv)->columnidCollate = rgcolumnid[iIndexCollate];
    ((JET_INDEXLIST *)pv)->columnidgrbitColumn = rgcolumnid[iIndexColBits];
    ((JET_INDEXLIST *)pv)->columnidcolumnname = rgcolumnid[iIndexColName];

    if ( cbIndexList < sizeof(JET_INDEXLIST) )
    {
        Assert( cbMax >= sizeof(JET_INDEXLIST) - cbIDXLISTNewMembersSinceOriginalFormat );
    }
    else
    {
        Assert( cbMax >= sizeof(JET_INDEXLIST) );
        ((JET_INDEXLIST *)pv)->columnidLCMapFlags = rgcolumnid[iIndexLCMapFlags];
    }

    return JET_errSuccess;

HandleError:
    if ( fUpdatingLatchSet )
    {
        pfcbTable->ResetUpdating();
    }
    (VOID)ErrDispCloseTable( (JET_SESID)ppib, tableid );
    return err;
}

LOCAL ERR ErrINFOGetTableIndexIdInfo(
    PIB                 * ppib,
    FUCB                * pfucb,
    const CHAR          * szIndexName,
    INDEXID             * pindexid )
{
    ERR                 err;
    FCB                 * const pfcbTable   = pfucb->u.pfcb;
    FCB                 * pfcbIndex;

    Assert( NULL != szIndexName );

    
    Assert( pfcbTable != pfcbNil );

    CallR( pfcbTable->ErrSetUpdatingAndEnterDML( ppib, fTrue ) );

    
    pfcbTable->AssertDML();
    for ( pfcbIndex = pfcbTable; pfcbIndex != pfcbNil; pfcbIndex = pfcbIndex->PfcbNextIndex() )
    {
        Assert( pfcbIndex->Pidb() != pidbNil || pfcbIndex == pfcbTable );

        if ( pfcbIndex->Pidb() != pidbNil )
        {
            err = ErrFILEIAccessIndexByName(
                        ppib,
                        pfcbTable,
                        pfcbIndex,
                        szIndexName );
            if ( err < 0 )
            {
                if ( JET_errIndexNotFound != err )
                {
                    goto HandleError;
                }
            }
            else
            {
                CallS( err );
                break;
            }
        }
    }

    pfcbTable->AssertDML();

    if ( pfcbNil == pfcbIndex )
    {
        err = ErrERRCheck( JET_errIndexNotFound );
    }
    else
    {
        CallS( err );

        Assert( pfcbIndex->FValid() );

        Assert( sizeof(INDEXID) == sizeof(JET_INDEXID) );
        pindexid->cbStruct = sizeof(INDEXID);
        pindexid->pfcbIndex = pfcbIndex;
        pindexid->objidFDP = pfcbIndex->ObjidFDP();
        pindexid->pgnoFDP = pfcbIndex->PgnoFDP();
    }

HandleError:
    pfcbTable->AssertDML();
    pfcbTable->ResetUpdatingAndLeaveDML();

    return err;
}


LOCAL ERR ErrINFOICopyAsciiName(
    _Out_writes_(cwchNameDest) WCHAR * const        wszNameDest,
    const size_t        cwchNameDest,
    const CHAR * const  szNameSrc,
    size_t *            pcbActual )
{
    size_t              cwchActual  = 0;
    const ERR           err         = ErrOSSTRAsciiToUnicode( szNameSrc, wszNameDest, cwchNameDest, &cwchActual );

    if ( NULL != pcbActual )
        *pcbActual = cwchActual * sizeof( WCHAR );

    return err;
}
LOCAL ERR ErrINFOICopyAsciiName(
    _Out_writes_(cchNameDest) CHAR * const      szNameDest,
    const size_t        cchNameDest,
    const CHAR * const  szNameSrc,
    size_t *            pcbActual )
{
    const ERR           err         = ErrOSStrCbCopyA( szNameDest, cchNameDest, szNameSrc );
    CallSx( err, JET_errBufferTooSmall );

    if ( NULL != pcbActual )
        *pcbActual = ( strlen( JET_errSuccess == err ? szNameDest : szNameSrc ) + 1 ) * sizeof( CHAR );
    return err;
}

INLINE VOID INFOISetKeySegmentDescendingFlag(
    __in WCHAR * const      pwch,
    const BOOL          fDescending )
{
    Assert( NULL != pwch );
    *pwch = ( fDescending ? L'-' : L'+' );
}
INLINE VOID INFOISetKeySegmentDescendingFlag(
    __in CHAR * const       pch,
    const BOOL          fDescending )
{
    Assert( NULL != pch );
    *pch = ( fDescending ? '-' : '+' );
}

template< class JET_INDEXCREATE_T, class JET_CONDITIONALCOLUMN_T, class JET_UNICODEINDEX_T, class T, ULONG cbV2Reserve >
ERR ErrINFOIBuildIndexCreateVX(
    FCB * const             pfcbTable,
    FCB * const             pfcbIndex,
    VOID * const            pvResult,
    const ULONG             cbMax,
    VOID **                 ppvV2Reserve,
    WCHAR **                pszLocaleName,
    ULONG                   lIdxVersion )
{
    ERR                     err                 = JET_errSuccess;
    TDB *                   ptdb                = ptdbNil;
    IDB *                   pidb                = pidbNil;
    JET_INDEXCREATE_T *     pindexcreate        = NULL;
    CHAR *                  szIndexName         = NULL;
    const IDXSEG *          rgidxseg            = NULL;
    size_t                  cbKey               = 0;
    size_t                  cbActual            = 0;
    BYTE *                  pbBuffer            = (BYTE *)pvResult;
    size_t                  cbBufferRemaining   = cbMax;


    Assert( pfcbNil != pfcbTable );
    Assert( pfcbNil != pfcbIndex );
    Assert( lIdxVersion == JET_IdxInfoCreateIndex || lIdxVersion == JET_IdxInfoCreateIndex2 || lIdxVersion == JET_IdxInfoCreateIndex3);
    if ( cbV2Reserve > 0 )
    {
        Assert( ppvV2Reserve != NULL );
    }
    else
    {
        Assert( ppvV2Reserve == NULL );
    }

    ptdb = pfcbTable->Ptdb();
    Assert( ptdbNil != ptdb );

    pidb = pfcbIndex->Pidb();
    Assert( pidbNil != pidb );

    Assert( NULL != pbBuffer );
    Assert( cbBufferRemaining >= sizeof( JET_INDEXCREATE_T ) );
    if ( cbBufferRemaining < sizeof( JET_INDEXCREATE_T ) )
    {
        Error( ErrERRCheck( JET_errBufferTooSmall ) );
    }

    pindexcreate = (JET_INDEXCREATE_T *)pbBuffer;
    memset( pindexcreate, 0, sizeof( JET_INDEXCREATE_T ) );
    pindexcreate->cbStruct = sizeof( JET_INDEXCREATE_T );

    pbBuffer += sizeof( JET_INDEXCREATE_T );
    cbBufferRemaining -= sizeof( JET_INDEXCREATE_T );

    if ( cbV2Reserve )
    {
        if ( cbBufferRemaining < cbV2Reserve )
        {
            Error( ErrERRCheck( JET_errBufferTooSmall ) );
        }
        *ppvV2Reserve = pbBuffer;
        pbBuffer += cbV2Reserve;
        cbBufferRemaining -= cbV2Reserve;
    }

    pindexcreate->szIndexName = (T *)pbBuffer;

    szIndexName = ptdb->SzIndexName( pidb->ItagIndexName() );
    Assert( NULL != szIndexName );
    Assert( strlen( szIndexName ) > 0 );
    Assert( strlen( szIndexName ) <= JET_cbNameMost );

    Call( ErrINFOICopyAsciiName(
                pindexcreate->szIndexName,
                cbBufferRemaining / sizeof( T ),
                szIndexName,
                &cbActual ) );
    Assert( '\0' == pindexcreate->szIndexName[ ( cbActual / sizeof( T ) ) - 1 ] );

    Assert( cbBufferRemaining >= cbActual );
    pbBuffer += cbActual;
    cbBufferRemaining -= cbActual;
    pindexcreate->szKey = (T *)pbBuffer;

    rgidxseg = PidxsegIDBGetIdxSeg( pidb, ptdb );
    Assert( NULL != rgidxseg );
    
    Assert( pidb->Cidxseg() > 0 );
    Assert( pidb->Cidxseg() <= JET_ccolKeyMost );
    for ( size_t iidxseg = 0; iidxseg < pidb->Cidxseg(); iidxseg++ )
    {
        const COLUMNID      columnid    = rgidxseg[ iidxseg ].Columnid();
        const FIELD * const pfield      = ptdb->Pfield( columnid );

        Assert( pfieldNil != pfield );

        if ( !FFIELDPrimaryIndexPlaceholder( pfield->ffield ) )
        {
            const BOOL      fDerived        = ( FCOLUMNIDTemplateColumn( columnid ) && !ptdb->FTemplateTable() );
            const CHAR *    szColumnName    = ptdb->SzFieldName( pfield->itagFieldName, fDerived );

            Assert( NULL != szColumnName );
            Assert( strlen( szColumnName ) > 0 );
            Assert( strlen( szColumnName ) <= JET_cbNameMost );

            if ( cbBufferRemaining < sizeof( T ) )
            {
                Error( ErrERRCheck( JET_errBufferTooSmall ) );
            }

            INFOISetKeySegmentDescendingFlag( (T *)pbBuffer, rgidxseg[ iidxseg ].FDescending() );

            pbBuffer += sizeof( T );
            cbBufferRemaining -= sizeof( T );

            Call( ErrINFOICopyAsciiName(
                        (T *)pbBuffer,
                        cbBufferRemaining / sizeof( T ),
                        szColumnName,
                        &cbActual ) );
            Assert( '\0' == ( (T *)pbBuffer )[ ( cbActual / sizeof( T ) ) - 1 ] );

            Assert( cbBufferRemaining >= cbActual );
            pbBuffer += cbActual;
            cbBufferRemaining -= cbActual;

            cbKey += ( sizeof( T ) + cbActual );
        }
        else
        {
            Assert( pidb->FPrimary() );
            Assert( 0 == iidxseg );
        }
    }

    Assert( cbKey > sizeof( T ) + sizeof( T ) );

    if ( cbBufferRemaining < sizeof( T ) )
    {
        Error( ErrERRCheck( JET_errBufferTooSmall ) );
    }

    *(T *)pbBuffer = 0;
    cbKey += sizeof( T );

    pbBuffer += sizeof( T );
    cbBufferRemaining -= sizeof( T );

    pindexcreate->cbKey = cbKey;

    pindexcreate->grbit = pidb->GrbitFromFlags();

    pindexcreate->ulDensity = pfcbIndex->UlDensity();

    if ( cbBufferRemaining < sizeof( JET_UNICODEINDEX_T ) )
    {
        Error( ErrERRCheck( JET_errBufferTooSmall ) );
    }

    pindexcreate->grbit |= JET_bitIndexUnicode;

    if ( lIdxVersion != JET_IdxInfoCreateIndex3 )
    {
        JET_UNICODEINDEX idxUnicode;

        pindexcreate->pidxunicode = (JET_UNICODEINDEX_T *)pbBuffer;

        Call( ErrNORMLocaleToLcid( pidb->WszLocaleName(), &idxUnicode.lcid ) );
        idxUnicode.dwMapFlags = pidb->DwLCMapFlags();

#pragma warning(suppress:26000)
        *( pindexcreate->pidxunicode ) = *( (JET_UNICODEINDEX_T *)&idxUnicode );

        pbBuffer += sizeof( JET_UNICODEINDEX );
        cbBufferRemaining -= sizeof( JET_UNICODEINDEX );
    }
    else
    {
        pindexcreate->pidxunicode = (JET_UNICODEINDEX_T *)pbBuffer;
        pbBuffer += sizeof(WCHAR*);
        cbBufferRemaining -= sizeof(WCHAR*);

        pindexcreate->pidxunicode->dwMapFlags = pidb->DwLCMapFlags();
        pbBuffer += sizeof( pidb->DwLCMapFlags() );
        cbBufferRemaining -= sizeof( pidb->DwLCMapFlags() );

        *pszLocaleName = (WCHAR *)pbBuffer;
        Call( ErrOSStrCbCopyW( *pszLocaleName, cbBufferRemaining, pidb->WszLocaleName() ) );

        Assert( NULL != *pszLocaleName );

        const ULONG_PTR cbLocaleName = ( wcslen( *pszLocaleName ) + 1 ) * sizeof(WCHAR);

        pbBuffer += cbLocaleName;
        cbBufferRemaining -= cbLocaleName;
    }

    if ( pidb->FTuples() )
    {
        JET_TUPLELIMITS *   ptuplelimits    = (JET_TUPLELIMITS *)pbBuffer;

        if ( cbBufferRemaining < sizeof( JET_TUPLELIMITS ) )
        {
            Error( ErrERRCheck( JET_errBufferTooSmall ) );
        }

        pindexcreate->grbit |= JET_bitIndexTupleLimits;
        ptuplelimits->chLengthMin = pidb->CchTuplesLengthMin();
        ptuplelimits->chLengthMax = pidb->CchTuplesLengthMax();
        ptuplelimits->chToIndexMax = pidb->IchTuplesToIndexMax();
        ptuplelimits->cchIncrement = pidb->CchTuplesIncrement();
        ptuplelimits->ichStart = pidb->IchTuplesStart();
        pindexcreate->ptuplelimits = ptuplelimits;

        pbBuffer += sizeof( JET_TUPLELIMITS );
        cbBufferRemaining -= sizeof( JET_TUPLELIMITS );
    }
    else
    {
        pindexcreate->cbVarSegMac = pidb->CbVarSegMac();
    }

    if ( pidb->CidxsegConditional() > 0 )
    {
        JET_CONDITIONALCOLUMN_T *   rgconditionalcolumn     = (JET_CONDITIONALCOLUMN_T *)pbBuffer;

        if ( cbBufferRemaining < sizeof( JET_CONDITIONALCOLUMN_T ) * pidb->CidxsegConditional() )
        {
            Error( ErrERRCheck( JET_errBufferTooSmall ) );
        }

        pbBuffer += ( sizeof( JET_CONDITIONALCOLUMN_T ) * pidb->CidxsegConditional() );
        cbBufferRemaining -= ( sizeof( JET_CONDITIONALCOLUMN_T ) * pidb->CidxsegConditional() );

        rgidxseg = PidxsegIDBGetIdxSegConditional( pidb, ptdb );
        Assert( NULL != rgidxseg );

        for ( size_t iidxseg = 0; iidxseg < pidb->CidxsegConditional(); iidxseg++ )
        {
            const COLUMNID  columnid        = rgidxseg[ iidxseg ].Columnid();
            const FIELD *   pfield          = ptdb->Pfield( columnid );
            const BOOL      fDerived        = ( FCOLUMNIDTemplateColumn( columnid ) && !ptdb->FTemplateTable() );
            const CHAR *    szColumnName    = ptdb->SzFieldName( pfield->itagFieldName, fDerived );

            Assert( NULL != szColumnName );
            Assert( strlen( szColumnName ) > 0 );
            Assert( strlen( szColumnName ) <= JET_cbNameMost );

            Call( ErrINFOICopyAsciiName(
                        (T *)pbBuffer,
                        cbBufferRemaining / sizeof( T ),
                        szColumnName,
                        &cbActual ) );
            Assert( '\0' == ( (T *)pbBuffer )[ ( cbActual / sizeof( T ) ) - 1 ] );

            rgconditionalcolumn[ iidxseg ].cbStruct = sizeof( JET_CONDITIONALCOLUMN_T );
            rgconditionalcolumn[ iidxseg ].szColumnName = (T *)pbBuffer;
            rgconditionalcolumn[ iidxseg ].grbit = ( rgidxseg[ iidxseg ].FMustBeNull() ?
                                                                JET_bitIndexColumnMustBeNull :
                                                                JET_bitIndexColumnMustBeNonNull );

            Assert( cbBufferRemaining >= cbActual );
            pbBuffer += cbActual;
            cbBufferRemaining -= cbActual;
        }

        pindexcreate->rgconditionalcolumn = rgconditionalcolumn;
        pindexcreate->cConditionalColumn = pidb->CidxsegConditional();
    }

    pindexcreate->cbKeyMost = max( JET_cbKeyMost_OLD, pidb->CbKeyMost() );

    CallS( err );


HandleError:
    return err;
}


LOCAL ERR ErrINFOGetTableIndexInfoForCreateIndex(
    PIB *           ppib,
    FUCB *          pfucb,
    const CHAR *    szIndex,
    VOID *          pvResult,
    const ULONG     cbMax,
    const BOOL      fUnicodeNames,
    ULONG   lIdxVersion )               
{
    ERR             err                 = JET_errSuccess;
    FCB *           pfcbTable           = pfcbNil;
    FCB *           pfcbIndex           = pfcbNil;
    BOOL            fUpdatingLatchSet   = fFalse;

    Assert( ppibNil != ppib );
    Assert( pfucbNil != pfucb );
    
    pfcbTable = pfucb->u.pfcb;
    Assert( pfcbNil != pfcbTable );
    Assert( lIdxVersion == JET_IdxInfoCreateIndex || lIdxVersion == JET_IdxInfoCreateIndex2 || lIdxVersion == JET_IdxInfoCreateIndex3 );
    
    Call( pfcbTable->ErrSetUpdatingAndEnterDML( ppib, fTrue ) );
    fUpdatingLatchSet = fTrue;
    
    if ( NULL == szIndex || '\0' == *szIndex )
    {
        if ( pidbNil != pfcbTable->Pidb() )
        {
            pfcbIndex = pfcbTable;
        }
    }
    else
    {
        pfcbTable->AssertDML();
        for ( pfcbIndex = pfcbTable; pfcbIndex != pfcbNil; pfcbIndex = pfcbIndex->PfcbNextIndex() )
        {
            Assert( pfcbIndex->Pidb() != pidbNil || pfcbIndex == pfcbTable );

            if ( pfcbIndex->Pidb() != pidbNil )
            {
                err = ErrFILEIAccessIndexByName( ppib, pfcbTable, pfcbIndex, szIndex );
                if ( err < JET_errSuccess )
                {
                    if ( JET_errIndexNotFound == err )
                    {
                        err = JET_errSuccess;
                    }
                    else
                    {
                        goto HandleError;
                    }
                }
                else
                    break;
            }
        }
    }

    pfcbTable->AssertDML();
    CallS( err );

    if ( pfcbNil == pfcbIndex )
    {
        Error( ErrERRCheck( JET_errIndexNotFound ) );
        goto HandleError;
    }

    if ( lIdxVersion == JET_IdxInfoCreateIndex )
        if ( fUnicodeNames )
        {
            err = ErrINFOIBuildIndexCreateVX< JET_INDEXCREATE_W, JET_CONDITIONALCOLUMN_W, JET_UNICODEINDEX, WCHAR, 0 >(
                            pfcbTable,
                            pfcbIndex,
                            pvResult,
                            cbMax,
                            NULL,
                            NULL,
                            lIdxVersion );
        }
        else
        {
            err = ErrINFOIBuildIndexCreateVX< JET_INDEXCREATE_A, JET_CONDITIONALCOLUMN_A, JET_UNICODEINDEX, CHAR, 0 >(
                            pfcbTable,
                            pfcbIndex,
                            pvResult,
                            cbMax,
                            NULL,
                            NULL,
                            lIdxVersion );
        }
    else if ( lIdxVersion == JET_IdxInfoCreateIndex2 )
    {
        JET_SPACEHINTS * pSPHints = NULL;
        if ( fUnicodeNames )
        {
            err = ErrINFOIBuildIndexCreateVX< JET_INDEXCREATE2_W, JET_CONDITIONALCOLUMN_W, JET_UNICODEINDEX, WCHAR, sizeof(JET_SPACEHINTS) >(
                            pfcbTable,
                            pfcbIndex,
                            pvResult,
                            cbMax,
                            (void**)&pSPHints,
                            NULL,
                            lIdxVersion );
            
        }
        else
        {
            err = ErrINFOIBuildIndexCreateVX< JET_INDEXCREATE2_A, JET_CONDITIONALCOLUMN_A, JET_UNICODEINDEX, CHAR, sizeof(JET_SPACEHINTS) >(
                            pfcbTable,
                            pfcbIndex,
                            pvResult,
                            cbMax,
                            (void**)&pSPHints,
                            NULL,
                            lIdxVersion );
        }
        if ( err >= JET_errSuccess )
        {

            C_ASSERT( OffsetOf(JET_INDEXCREATE2_W, pSpacehints) == OffsetOf(JET_INDEXCREATE2_A, pSpacehints) );
            Assert( OffsetOf(JET_INDEXCREATE2_W, pSpacehints) == OffsetOf(JET_INDEXCREATE2_A, pSpacehints) );
            Assert( pvResult );

            Assert( pSPHints > pvResult );
            Assert( ((BYTE*)(pSPHints + sizeof(JET_SPACEHINTS))) <= ((BYTE*)pvResult) + cbMax );
            Assert( (size_t)pSPHints % sizeof(ULONG) == 0 );

            pfcbIndex->GetAPISpaceHints( pSPHints );
            Assert( pSPHints->cbStruct == sizeof(JET_SPACEHINTS) );
            if ( fUnicodeNames )
            {
                ((JET_INDEXCREATE2_W*)pvResult)->pSpacehints = pSPHints;
                Assert( ((JET_INDEXCREATE2_W*)pvResult)->pSpacehints->ulInitialDensity == ((JET_INDEXCREATE2_W*)pvResult)->ulDensity );
            }
            else
            {
                ((JET_INDEXCREATE2_A*)pvResult)->pSpacehints = pSPHints;
                Assert( ((JET_INDEXCREATE2_A*)pvResult)->pSpacehints->ulInitialDensity == ((JET_INDEXCREATE2_A*)pvResult)->ulDensity );
            }
            
        }
    }
    else if ( lIdxVersion == JET_IdxInfoCreateIndex3 )
    {
        
        WCHAR *szLocaleName = NULL;
        JET_SPACEHINTS * pSPHints = NULL;

        if ( fUnicodeNames )
        {
            err = ErrINFOIBuildIndexCreateVX< JET_INDEXCREATE3_W, JET_CONDITIONALCOLUMN_W, JET_UNICODEINDEX2, WCHAR, sizeof(JET_SPACEHINTS) >(
                            pfcbTable,
                            pfcbIndex,
                            pvResult,
                            cbMax,
                            (void**)&pSPHints,
                            &szLocaleName,
                            lIdxVersion );
        }
        else
        {
            err = ErrINFOIBuildIndexCreateVX< JET_INDEXCREATE3_A, JET_CONDITIONALCOLUMN_A, JET_UNICODEINDEX2, CHAR, sizeof(JET_SPACEHINTS) >(
                            pfcbTable,
                            pfcbIndex,
                            pvResult,
                            cbMax,
                            (void**)&pSPHints,
                            &szLocaleName,
                            lIdxVersion );
        }
        if ( err >= JET_errSuccess )
        {
            C_ASSERT( OffsetOf(JET_INDEXCREATE3_W, pSpacehints) == OffsetOf(JET_INDEXCREATE3_A, pSpacehints) );
            Assert( OffsetOf(JET_INDEXCREATE3_W, pSpacehints) == OffsetOf(JET_INDEXCREATE3_A, pSpacehints) );
            
            Assert( pvResult );


            Assert( szLocaleName > pvResult );
            const ULONG_PTR cbLocaleName = ( wcslen( szLocaleName ) + 1 ) * sizeof(WCHAR);
            Assert( ((BYTE*)szLocaleName) + cbLocaleName <= ((BYTE*)pvResult) + cbMax );
            ((JET_INDEXCREATE3_W*)pvResult)->pidxunicode->szLocaleName = szLocaleName;
            
            Assert( pSPHints > pvResult );
            Assert( (((BYTE*)pSPHints) + sizeof(JET_SPACEHINTS)) <= ((BYTE*)pvResult) + cbMax );
            Assert( (size_t)pSPHints % sizeof(ULONG) == 0 );

            pfcbIndex->GetAPISpaceHints( pSPHints );
            Assert( pSPHints->cbStruct == sizeof(JET_SPACEHINTS) );
            if ( fUnicodeNames )
            {
                ((JET_INDEXCREATE3_W*)pvResult)->pSpacehints = pSPHints;
                Assert( ((JET_INDEXCREATE3_W*)pvResult)->pSpacehints->ulInitialDensity == ((JET_INDEXCREATE3_W*)pvResult)->ulDensity );
            }
            else
            {
                ((JET_INDEXCREATE3_A*)pvResult)->pSpacehints = pSPHints;
                Assert( ((JET_INDEXCREATE3_A*)pvResult)->pSpacehints->ulInitialDensity == ((JET_INDEXCREATE3_A*)pvResult)->ulDensity );
            }
        }
    }
    else
    {
        AssertSz( fFalse, "Unknown lIdxVersion (%d)!\n", lIdxVersion );
    }
    
    Call( err );
    CallS( err );


HandleError:
    if ( fUpdatingLatchSet )
    {
        pfcbTable->ResetUpdatingAndLeaveDML();
    }
    return err;
}


ERR VDBAPI ErrIsamGetDatabaseInfo(
    JET_SESID       vsesid,
    JET_DBID        vdbid,
    VOID            *pvResult,
    const ULONG     cbMax,
    const ULONG     ulInfoLevel )
{
    PIB             *ppib = (PIB *)vsesid;
    ERR             err;
    IFMP            ifmp;
    WORD            cp          = usEnglishCodePage;
    WORD            wCountry    = countryDefault;
    WORD            wCollate    = 0;
    bool            fUseCachedResult = 0;
    ULONG           ulT         = 0;
    
    

    
    CallR( ErrPIBCheck( ppib ) );
    CallR( ErrPIBCheckIfmp( ppib, (IFMP)vdbid ) );
    ifmp = (IFMP) vdbid;

    Assert ( cbMax == 0 || pvResult != NULL );


    
    Expected( FInRangeIFMP( ifmp ) );
    if ( !FInRangeIFMP( ifmp ) || !g_rgfmp[ifmp].FInUse()  )
    {
        err = ErrERRCheck( JET_errInvalidParameter );
        goto HandleError;
    }

    fUseCachedResult = (bool)((ulInfoLevel & JET_DbInfoUseCachedResult) ? 1 : 0 );
    ulT = (ulInfoLevel & ~(JET_DbInfoUseCachedResult));
    switch ( ulT )
    {
        case JET_DbInfoFilename:
            if ( sizeof( WCHAR ) * ( LOSStrLengthW( g_rgfmp[ifmp].WszDatabaseName() ) + 1UL ) > cbMax )
            {
                err = ErrERRCheck( JET_errBufferTooSmall );
                goto HandleError;
            }
            OSStrCbCopyW( (WCHAR  *)pvResult, cbMax, g_rgfmp[ifmp].WszDatabaseName() );
            break;

        case JET_DbInfoConnect:
            if ( 1UL > cbMax )
            {
                err = ErrERRCheck( JET_errBufferTooSmall );
                goto HandleError;
            }
            *(CHAR *)pvResult = '\0';
            break;

        case JET_DbInfoCountry:
            if ( cbMax != sizeof(LONG) )
                return ErrERRCheck( JET_errInvalidBufferSize );
            *(LONG  *)pvResult = wCountry;
            break;

        case JET_DbInfoLCID:
            if ( cbMax != sizeof(LONG) )
                return ErrERRCheck( JET_errInvalidBufferSize );
            Call( ErrNORMLocaleToLcid( PinstFromPpib( ppib )->m_wszLocaleNameDefault, (LCID *)pvResult ) );
            break;

        case JET_DbInfoCp:
            if ( cbMax != sizeof(LONG) )
                return ErrERRCheck( JET_errInvalidBufferSize );
            *(LONG  *)pvResult = cp;
            break;

        case JET_DbInfoCollate:
            
            if ( cbMax != sizeof(LONG) )
                return ErrERRCheck( JET_errInvalidBufferSize );
            *(LONG *)pvResult = wCollate;
            break;

        case JET_DbInfoOptions:
            
            if ( cbMax != sizeof(JET_GRBIT) )
                return ErrERRCheck( JET_errInvalidBufferSize );

            
            *(JET_GRBIT *)pvResult = g_rgfmp[ifmp].FExclusiveBySession( ppib ) ? JET_bitDbExclusive : 0;
            break;

        case JET_DbInfoTransactions:
            
            if ( cbMax != sizeof(LONG) )
                return ErrERRCheck( JET_errInvalidBufferSize );

            *(LONG*)pvResult = levelUserMost;
            break;

        case JET_DbInfoVersion:
            
            if ( cbMax != sizeof(LONG) )
                return ErrERRCheck( JET_errInvalidBufferSize );

            *(LONG *)pvResult = ulDAEVersionMax;
            break;

        case JET_DbInfoIsam:
            
            return ErrERRCheck( JET_errFeatureNotAvailable );

        case JET_DbInfoMisc:
            if (    sizeof( JET_DBINFOMISC ) != cbMax &&
                    sizeof( JET_DBINFOMISC2 ) != cbMax &&
                    sizeof( JET_DBINFOMISC3 ) != cbMax &&
                    sizeof( JET_DBINFOMISC4 ) != cbMax &&
                    sizeof( JET_DBINFOMISC5 ) != cbMax &&
                    sizeof( JET_DBINFOMISC6 ) != cbMax &&
                    sizeof( JET_DBINFOMISC7 ) != cbMax )
            {
                return ErrERRCheck( JET_errInvalidBufferSize );
            }

            ProbeClientBuffer( pvResult, cbMax );
        {
            UtilLoadDbinfomiscFromPdbfilehdr( ( JET_DBINFOMISC7* )pvResult, cbMax, g_rgfmp[ ifmp ].Pdbfilehdr().get() );
        }
            break;

        case JET_DbInfoPageSize:
            if ( sizeof( ULONG ) != cbMax )
            {
                err = ErrERRCheck( JET_errBufferTooSmall );
                goto HandleError;
            }

        {
            const ULONG cbPageSize = g_rgfmp[ifmp].Pdbfilehdr()->le_cbPageSize;
            *(ULONG *)pvResult = ( 0 != cbPageSize ? cbPageSize : g_cbPageDefault );
        }
            break;

        case JET_DbInfoFilesize:
        {
            QWORD cbFileSize = 0;
            Call( g_rgfmp[ifmp].Pfapi()->ErrSize( &cbFileSize, IFileAPI::filesizeLogical ) );
            *(ULONG *)pvResult = static_cast<ULONG>( g_rgfmp[ifmp].CpgOfCb( cbFileSize ) );
        }
            break;

        case JET_DbInfoFilesizeOnDisk:
        {
            QWORD cbFileSize = 0;
            Call( g_rgfmp[ifmp].Pfapi()->ErrSize( &cbFileSize, IFileAPI::filesizeOnDisk ) );
            *(ULONG *)pvResult = static_cast<ULONG>( g_rgfmp[ifmp].CpgOfCb( cbFileSize ) );
        }
            break;

        case JET_DbInfoSpaceOwned:
            if ( cbMax != sizeof(ULONG) )
                return ErrERRCheck( JET_errInvalidBufferSize );

            err = ErrSPGetDatabaseInfo(
                        ppib,
                        ifmp,
                        static_cast<BYTE *>( pvResult ),
                        cbMax,
                        fSPOwnedExtent,
                        fUseCachedResult );
            return err;

        case JET_DbInfoSpaceAvailable:
            err = ErrSPGetDatabaseInfo(
                        ppib,
                        ifmp,
                        static_cast<BYTE *>( pvResult ),
                        cbMax,
                        fSPAvailExtent,
                        fUseCachedResult );
            return err;

        case dbInfoSpaceShelved:
        {
            Expected( !fUseCachedResult );
            ULONG rgcpg[2] = { 0 };

            if ( cbMax < sizeof(*rgcpg) )
            {
                return ErrERRCheck( JET_errInvalidBufferSize );
            }

            err = ErrSPGetDatabaseInfo(
                        ppib,
                        ifmp,
                        (BYTE*)rgcpg,
                        sizeof(rgcpg),
                        fSPAvailExtent | fSPShelvedExtent,
                        fFalse );
            *( (ULONG*)pvResult ) = rgcpg[1];
            return err;
        }

        default:
            return ErrERRCheck( JET_errInvalidParameter );
    }

    err = JET_errSuccess;
HandleError:
    return err;
}


ERR VTAPI ErrIsamGetCursorInfo(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    void            *pvResult,
    ULONG   cbMax,
    ULONG   InfoLevel )
{
    ERR             err;
    PIB             *ppib   = (PIB *)vsesid;
    FUCB            *pfucb  = (FUCB *)vtid;
    VS              vs;

    Unused( ppib );

    CallR( ErrPIBCheck( ppib ) );
    CheckFUCB( pfucb->ppib, pfucb );

    if ( cbMax != 0 || InfoLevel != 0 )
        return ErrERRCheck( JET_errInvalidParameter );

    if ( pfucb->locLogical != locOnCurBM )
        return ErrERRCheck( JET_errNoCurrentRecord );

    Assert( !Pcsr( pfucb )->FLatched() );

    Call( ErrDIRGet( pfucb ) );
    if ( FNDVersion( pfucb->kdfCurr ) )
    {
        vs = VsVERCheck( pfucb, pfucb->bmCurr );
        if ( vs == vsUncommittedByOther )
        {
            CallS( ErrDIRRelease( pfucb ) );

            OSTraceFMP(
                pfucb->ifmp,
                JET_tracetagDMLConflicts,
                OSFormat(
                    "Session=[0x%p:0x%x] on objid=[0x%x:0x%x] is attempting to retrieve cursor info, but currency is on a node with uncommitted changes",
                    ppib,
                    ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
                    (ULONG)pfucb->ifmp,
                    pfucb->u.pfcb->ObjidFDP() ) );
            return ErrERRCheck( JET_errWriteConflict );
        }
    }

    CallS( ErrDIRRelease( pfucb ) );

    if ( pfucb->u.pfcb->FTypeTemporaryTable() )
        err = JET_errSuccess;

HandleError:
    return err;
}

