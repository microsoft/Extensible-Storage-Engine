// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// BookStoreSample.c : console application sample of ESE
//

#include <windows.h>
#include <stdio.h>

#include <esent.h>

//  CallJ and Call both expect a local variable err, and a lable HandleError to be defined
//
#define CallJ( fn, label )  {                                           \
                            if ( ( err = (fn) ) < 0 )                       \
                                {                                   \
                                goto label;                         \
                                }                                   \
                            }
#define Call( fn )          CallJ( fn, HandleError )

#define NO_GRBIT                            0
#define CP_ANSI                                 1252
#define CP_UNICODE                          1200

//  user name and password are ignored by ESE and can be any string value including the empty string
//
char szUser []                              = "";
char szPassword []                          = "";

char szDatabase []                          = "database.mdb";
char szCustomersTable []                        = "Customers";
char szCustomerID []                            = "CustomerID";
char szCustomerName []                      = "CustomerName";
char szOrdersTable []                           = "Orders";
char szOrderID []                               = "OrderID";
char szOrderCustomerID []                   = "OrderCustomerID";
char szOrderDate []                         = "OrderDate";
char szOrderAmount []                       = "OrderAmount";
char szOrderCancelled []                        = "OrderCancelled";
char szOrderItems []                            = "OrderItems";
char szBooksTable []                            = "Books";
char szBookID []                                = "BookID";
char szBookTitle []                         = "BookTitle";
char szBookPrice []                         = "BookPrice";
char szBookAuthors []                       = "BookAuthors";
char szBookCover []                         = "BookCover";
char szBookDescription []                       = "BookDescription";

char szCustomerIDIndex []                   = "CustomerID";
char szCustomerNameIndex []                 = "CustomerName";
char szOrderIDIndex []                      = "OrderID";
char szOrderCustomerIDIndex []              = "OrderCustomerIDNotCancelled";
char szBookIDIndex []                       = "BookID";
char szBookAuthorsIndex []                  = "BookAuthors";
char szBookTitleAuthorsIndex []             = "BookTitleAuthors";

//  index definitions, each index segment is followed by a string terminator, \0, and 
//  the whole index definition is also followed by a string terminator, \0.  This is why
//  each index definition ends in \0\0.  Counts of characters include all terminators.
//
char rgbCustomerIDIndex []                  = "+CustomerID\0\0";
int cchCustomerIDIndex                      = 13;
char rgbCustomerNameIndex []                = "+CustomerName\0\0";
int cchCustomerNameIndex                    = 15;
char rgbOrderIDIndex []                     = "+OrderID\0\0";
int cchOrderIDIndex                     = 10;
char rgbOrderCustomerIDIndex []             = "+OrderCustomerID\0\0";
int cchOrderCustomerIDIndex             = 18;
char rgbBookIDIndex []                      = "+BookID\0\0";
int cchBookIDIndex                          = 9;
char rgbBookAuthorsIndex []                 = "+BookAuthors\0\0";
int cchBookAuthorsIndex                 = 14;
char rgbBookTitleAuthorsIndex []                = "+BookTitle\0+BookAuthors\0\0";
int cchBookTitleAuthorsIndex                = 25;

////////////////////////////////////////////////////////////
//  DATA
//
//  Customers
//
char szCustomerName1 []                 = "Owen May";

char szCustomerName2 []                 = "Darrin DeYoung";

char szCustomerName3 []                 = "Detlef Wagner";

char szCustomerName4 []                 = "Mathias Kjeldsen";

char szCustomerName5 []                 = "Pentti Hietalahti";


//  Books
//
wchar_t wszBookTitle1 []                    = L"All About Coffee";
wchar_t wszBookAuthor1 []               = L"William Harrison Ukers";
//  currency units are 10,000ths of cents
//
__int64 llBookPrice1                        = 4250000;
wchar_t wszBookDescription1 []          = L"Published in 1935";

wchar_t wszBookTitle2 []                    = L"Espresso Coffee: The Chemistry of Quality";
wchar_t wszBookAuthor2First []          = L"Andrea Illy";
wchar_t wszBookAuthor2Second []     = L"Rinantonio Viani";
__int64 llBookPrice2                        = 300000;
wchar_t wszBookDescription2 []          = L"Not Available";

wchar_t wszBookTitle3 []                    = L"Espresso: Ultimate Coffee";
wchar_t wszBookAuthor3 []               = L"Kenneth Davids";
__int64 llBookPrice3                        = 117100;
wchar_t wszBookDescription3 []          = L"A great historical, cultural and sensual review of the brew";

//  Orders
//
long lOrderCustomerID1                  = 5;
long rglOrderItems1[]                       = { 1 };

long lOrderCustomerID2                  = 1;
long rglOrderItems2[]                       = { 2, 3 };

long lOrderCustomerID3                  = 2;
long rglOrderItems3[]                       = { 3, 3 };

long lOrderCustomerID4                  = 4;
long rglOrderItems4[]                       = { 1, 2, 3 };


////////////////////////////////////////////////////////////
//


//  column ids are like global constants after they are set.
//  They can even be made constants for a stable schema and then
//  asserted in startup.  The addition of subsequent columns to
//  an existing schema does not change the column ids of pre-existing
//  columns.
//
JET_COLUMNID columnidCustomerID     = 0;
JET_COLUMNID columnidCustomerName       = 0;
JET_COLUMNID columnidOrderID            = 0;
JET_COLUMNID columnidOrderCustomerID    = 0;
JET_COLUMNID columnidOrderDate          = 0;
JET_COLUMNID columnidOrderAmount        = 0;
JET_COLUMNID columnidOrderCancelled = 0;
JET_COLUMNID columnidOrderItems     = 0;
JET_COLUMNID columnidBookID         = 0;
JET_COLUMNID columnidBookTitle          = 0;
JET_COLUMNID columnidBookAuthors        = 0;
JET_COLUMNID columnidBookCover          = 0;
JET_COLUMNID columnidBookPrice          = 0;
JET_COLUMNID columnidBookDescription    = 0;


JET_ERR ErrCreateTablesColumnsAndIndexes( JET_SESID sesidT, JET_DBID dbidDatabase )
{
    JET_ERR                 err = JET_errSuccess;
    JET_TABLECREATE         tablecreateT ;
    JET_COLUMNCREATE        rgcolumncreateT[10];
    JET_INDEXCREATE         rgindexcreateT[10];
    JET_CONDITIONALCOLUMN   rgconditionalcolumnT[3];
        
    //  perform all DDL operations in single transaction so either all tables, columns and indexes are created,
    //  or none are created.
    //
    err = JetBeginTransaction( sesidT );
    if ( err < 0 )
    {
        return err;
    }
    
    //  Customers table
    //
    
    //  column for customer id
    //  autoincrement column ensures unique value for each row
    //
    rgcolumncreateT[0].cbStruct = sizeof(rgcolumncreateT[0]);
    rgcolumncreateT[0].szColumnName = szCustomerID;
    rgcolumncreateT[0].coltyp = JET_coltypLong;
    rgcolumncreateT[0].cbMax = 0;
    rgcolumncreateT[0].grbit = JET_bitColumnAutoincrement;
    rgcolumncreateT[0].pvDefault = NULL;
    rgcolumncreateT[0].cbDefault = 0;
    rgcolumncreateT[0].cp = CP_ANSI;
    rgcolumncreateT[0].columnid = 0;
    rgcolumncreateT[0].err = JET_errSuccess;

    //  customer name column
    //  non-unicode text
    //
    rgcolumncreateT[1].cbStruct = sizeof(rgcolumncreateT[1]);
    rgcolumncreateT[1].szColumnName = szCustomerName;
    rgcolumncreateT[1].coltyp = JET_coltypText;
    rgcolumncreateT[1].cbMax = 0;
    rgcolumncreateT[1].grbit = NO_GRBIT;
    rgcolumncreateT[1].pvDefault = NULL;
    rgcolumncreateT[1].cbDefault = 0;
    rgcolumncreateT[1].cp = CP_ANSI;
    rgcolumncreateT[1].columnid = 0;
    rgcolumncreateT[1].err = JET_errSuccess;

    //  primary index on CustomerID
    //
    rgindexcreateT[0].cbStruct = sizeof(rgindexcreateT[0]);
    rgindexcreateT[0].szIndexName = szCustomerIDIndex;
    rgindexcreateT[0].szKey = rgbCustomerIDIndex;
    rgindexcreateT[0].cbKey = cchCustomerIDIndex;
    rgindexcreateT[0].grbit = JET_bitIndexPrimary;
    rgindexcreateT[0].ulDensity = 100;
    rgindexcreateT[0].lcid = MAKELCID( 0x409, SORT_DEFAULT );
    rgindexcreateT[0].cbVarSegMac = 0;
    rgindexcreateT[0].rgconditionalcolumn = NULL;
    rgindexcreateT[0].cConditionalColumn = 0;
    rgindexcreateT[0].err = JET_errSuccess;
    
    //  secondary index on CustomerName
    //  with ignore NULL to remove defunct customers
    //
    rgindexcreateT[1].cbStruct = sizeof(rgindexcreateT[0]);
    rgindexcreateT[1].szIndexName = szCustomerNameIndex;
    rgindexcreateT[1].szKey = rgbCustomerNameIndex;
    rgindexcreateT[1].cbKey = cchCustomerNameIndex;
    rgindexcreateT[1].grbit = JET_bitIndexIgnoreNull;
    rgindexcreateT[1].ulDensity = 80;
    rgindexcreateT[1].lcid = MAKELCID( 0x409, SORT_DEFAULT );
    rgindexcreateT[1].cbVarSegMac = 0;
    rgindexcreateT[1].rgconditionalcolumn = NULL;
    rgindexcreateT[1].cConditionalColumn = 0;
    rgindexcreateT[1].err = JET_errSuccess;
    
    tablecreateT.cbStruct = sizeof(tablecreateT);
    tablecreateT.szTableName = szCustomersTable;
    tablecreateT.szTemplateTableName = NULL;
    tablecreateT.ulPages = 16;
    tablecreateT.ulDensity = 80;
    tablecreateT.rgcolumncreate = rgcolumncreateT;
    tablecreateT.cColumns = 2;
    tablecreateT.rgindexcreate = rgindexcreateT;
    tablecreateT.cIndexes = 2;
    tablecreateT.grbit = NO_GRBIT;
    tablecreateT.tableid =  JET_tableidNil;
    tablecreateT.cCreated = 0;

    Call( JetCreateTableColumnIndex( sesidT, dbidDatabase, &tablecreateT ) );
    //  close open table handle implicitly created by create table
    //
    Call( JetCloseTable( sesidT, tablecreateT.tableid ) );

    //  Orders table
    //
    //  column for order id
    //  autoincrement column ensures unique value for each row
    //
    rgcolumncreateT[0].cbStruct = sizeof(rgcolumncreateT[0]);
    rgcolumncreateT[0].szColumnName = szOrderID;
    rgcolumncreateT[0].coltyp = JET_coltypLong;
    rgcolumncreateT[0].cbMax = 0;
    rgcolumncreateT[0].grbit = JET_bitColumnAutoincrement;
    rgcolumncreateT[0].pvDefault = NULL;
    rgcolumncreateT[0].cbDefault = 0;
    rgcolumncreateT[0].cp = CP_ANSI;
    rgcolumncreateT[0].columnid = 0;
    rgcolumncreateT[0].err = JET_errSuccess;

    //  column for order customer id
    //  non-NULL
    //
    rgcolumncreateT[1].cbStruct = sizeof(rgcolumncreateT[1]);
    rgcolumncreateT[1].szColumnName = szOrderCustomerID;
    rgcolumncreateT[1].coltyp = JET_coltypLong;
    rgcolumncreateT[1].cbMax = 0;
    rgcolumncreateT[1].grbit = JET_bitColumnNotNULL;
    rgcolumncreateT[1].pvDefault = NULL;
    rgcolumncreateT[1].cbDefault = 0;
    rgcolumncreateT[1].cp = CP_ANSI;
    rgcolumncreateT[1].columnid = 0;
    rgcolumncreateT[1].err = JET_errSuccess;

    //  column for order completion date time
    //
    //
    rgcolumncreateT[2].cbStruct = sizeof(rgcolumncreateT[2]);
    rgcolumncreateT[2].szColumnName = szOrderDate;
    rgcolumncreateT[2].coltyp = JET_coltypDateTime;
    rgcolumncreateT[2].cbMax = 0;
    rgcolumncreateT[2].grbit = NO_GRBIT;
    rgcolumncreateT[2].pvDefault = NULL;
    rgcolumncreateT[2].cbDefault = 0;
    rgcolumncreateT[2].cp = CP_ANSI;
    rgcolumncreateT[2].columnid = 0;
    rgcolumncreateT[2].err = JET_errSuccess;

    //  column for order amount
    //
    //
    rgcolumncreateT[3].cbStruct = sizeof(rgcolumncreateT[3]);
    rgcolumncreateT[3].szColumnName = szOrderAmount;
    rgcolumncreateT[3].coltyp = JET_coltypCurrency;
    rgcolumncreateT[3].cbMax = 0;
    rgcolumncreateT[3].grbit = NO_GRBIT;
    rgcolumncreateT[3].pvDefault = NULL;
    rgcolumncreateT[3].cbDefault = 0;
    rgcolumncreateT[3].cp = CP_ANSI;
    rgcolumncreateT[3].columnid = 0;
    rgcolumncreateT[3].err = JET_errSuccess;

    //  column for order cancelled
    //  tagged becuase it is usually NULL
    //
    rgcolumncreateT[4].cbStruct = sizeof(rgcolumncreateT[4]);
    rgcolumncreateT[4].szColumnName = szOrderCancelled;
    rgcolumncreateT[4].coltyp = JET_coltypBit;
    rgcolumncreateT[4].cbMax = 0;
    rgcolumncreateT[4].grbit = JET_bitColumnTagged;
    rgcolumncreateT[4].pvDefault = NULL;
    rgcolumncreateT[4].cbDefault = 0;
    rgcolumncreateT[4].cp = CP_ANSI;
    rgcolumncreateT[4].columnid = 0;
    rgcolumncreateT[4].err = JET_errSuccess;

    //  column for order items
    //  multi-valued since order can contain multiple items
    //
    rgcolumncreateT[5].cbStruct = sizeof(rgcolumncreateT[5]);
    rgcolumncreateT[5].szColumnName = szOrderItems;
    rgcolumncreateT[5].coltyp = JET_coltypLong;
    rgcolumncreateT[5].cbMax = 0;
    rgcolumncreateT[5].grbit = JET_bitColumnTagged|JET_bitColumnMultiValued;
    rgcolumncreateT[5].pvDefault = NULL;
    rgcolumncreateT[5].cbDefault = 0;
    rgcolumncreateT[5].cp = CP_ANSI;
    rgcolumncreateT[5].columnid = 0;
    rgcolumncreateT[5].err = JET_errSuccess;

    //  primary index on OrderID
    //
    rgindexcreateT[0].cbStruct = sizeof(rgindexcreateT[0]);
    rgindexcreateT[0].szIndexName = szOrderIDIndex;
    rgindexcreateT[0].szKey = rgbOrderIDIndex;
    rgindexcreateT[0].cbKey = cchOrderIDIndex;
    rgindexcreateT[0].grbit = JET_bitIndexPrimary;
    rgindexcreateT[0].ulDensity = 80;
    rgindexcreateT[0].lcid = MAKELCID( 0x409, SORT_DEFAULT );
    rgindexcreateT[0].cbVarSegMac = 0;
    rgindexcreateT[0].rgconditionalcolumn = NULL;
    rgindexcreateT[0].cConditionalColumn = 0;
    rgindexcreateT[0].err = JET_errSuccess;

    //  secondary index on OrderName
    //  with ignore NULL to remove defunct customers
    //
    rgconditionalcolumnT[0].cbStruct = sizeof(rgconditionalcolumnT[0]);
    rgconditionalcolumnT[0].szColumnName = szOrderCancelled;
    rgconditionalcolumnT[0].grbit = JET_bitIndexColumnMustBeNull;

    rgindexcreateT[1].cbStruct = sizeof(rgindexcreateT[0]);
    rgindexcreateT[1].szIndexName = szOrderCustomerIDIndex;
    rgindexcreateT[1].szKey = rgbOrderCustomerIDIndex;
    rgindexcreateT[1].cbKey = cchOrderCustomerIDIndex;
    rgindexcreateT[1].grbit = NO_GRBIT;
    rgindexcreateT[1].ulDensity = 80;
    rgindexcreateT[1].lcid = MAKELCID( 0x409, SORT_DEFAULT );
    rgindexcreateT[1].cbVarSegMac = 0;
    rgindexcreateT[1].rgconditionalcolumn = rgconditionalcolumnT;
    rgindexcreateT[1].cConditionalColumn = 1;
    rgindexcreateT[1].err = JET_errSuccess;
    
    tablecreateT.cbStruct = sizeof(tablecreateT);
    tablecreateT.szTableName = szOrdersTable;
    tablecreateT.szTemplateTableName = NULL;
    tablecreateT.ulPages = 16;
    tablecreateT.ulDensity = 80;
    tablecreateT.rgcolumncreate = rgcolumncreateT;
    tablecreateT.cColumns = 6;
    tablecreateT.rgindexcreate = rgindexcreateT;
    tablecreateT.cIndexes = 2;
    tablecreateT.grbit = NO_GRBIT;
    tablecreateT.tableid =  JET_tableidNil;
    tablecreateT.cCreated = 0;

    Call( JetCreateTableColumnIndex( sesidT, dbidDatabase, &tablecreateT ) );
    //  close open table handle implicitly created by create table
    //
    Call( JetCloseTable( sesidT, tablecreateT.tableid ) );

    //  Books table
    //
    
    //  column for book id
    //  autoincrement column ensures unique value for each row
    //
    rgcolumncreateT[0].cbStruct = sizeof(rgcolumncreateT[0]);
    rgcolumncreateT[0].szColumnName = szBookID;
    rgcolumncreateT[0].coltyp = JET_coltypLong;
    rgcolumncreateT[0].cbMax = 0;
    rgcolumncreateT[0].grbit = JET_bitColumnAutoincrement;
    rgcolumncreateT[0].pvDefault = NULL;
    rgcolumncreateT[0].cbDefault = 0;
    rgcolumncreateT[0].cp = CP_ANSI;
    rgcolumncreateT[0].columnid = 0;
    rgcolumncreateT[0].err = JET_errSuccess;

    //  column for book title
    //
    rgcolumncreateT[1].cbStruct = sizeof(rgcolumncreateT[1]);
    rgcolumncreateT[1].szColumnName = szBookTitle;
    rgcolumncreateT[1].coltyp = JET_coltypText;
    rgcolumncreateT[1].cbMax = 0;
    rgcolumncreateT[1].grbit = NO_GRBIT;
    rgcolumncreateT[1].pvDefault = NULL;
    rgcolumncreateT[1].cbDefault = 0;
    rgcolumncreateT[1].cp = CP_UNICODE;
    rgcolumncreateT[1].columnid = 0;
    rgcolumncreateT[1].err = JET_errSuccess;

    //  column for book author
    //  multi-valued
    //
    rgcolumncreateT[2].cbStruct = sizeof(rgcolumncreateT[1]);
    rgcolumncreateT[2].szColumnName = szBookAuthors;
    rgcolumncreateT[2].coltyp = JET_coltypText;
    rgcolumncreateT[2].cbMax = 0;
    rgcolumncreateT[2].grbit = JET_bitColumnTagged|JET_bitColumnMultiValued;
    rgcolumncreateT[2].pvDefault = NULL;
    rgcolumncreateT[2].cbDefault = 0;
    rgcolumncreateT[2].cp = CP_UNICODE;
    rgcolumncreateT[2].columnid = 0;
    rgcolumncreateT[2].err = JET_errSuccess;

    //  column for book cover
    //
    //
    rgcolumncreateT[3].cbStruct = sizeof(rgcolumncreateT[2]);
    rgcolumncreateT[3].szColumnName = szBookCover;
    rgcolumncreateT[3].coltyp = JET_coltypLongBinary;
    rgcolumncreateT[3].cbMax = 0;
    rgcolumncreateT[3].grbit = JET_bitColumnTagged|JET_bitColumnMultiValued;
    rgcolumncreateT[3].pvDefault = NULL;
    rgcolumncreateT[3].cbDefault = 0;
    rgcolumncreateT[3].cp = CP_ANSI;
    rgcolumncreateT[3].columnid = 0;
    rgcolumncreateT[3].err = JET_errSuccess;

    //  column for book price
    //
    //
    rgcolumncreateT[4].cbStruct = sizeof(rgcolumncreateT[3]);
    rgcolumncreateT[4].szColumnName = szBookPrice;
    rgcolumncreateT[4].coltyp = JET_coltypCurrency;
    rgcolumncreateT[4].cbMax = 0;
    rgcolumncreateT[4].grbit = NO_GRBIT;
    rgcolumncreateT[4].pvDefault = NULL;
    rgcolumncreateT[4].cbDefault = 0;
    rgcolumncreateT[4].cp = CP_ANSI;
    rgcolumncreateT[4].columnid = 0;
    rgcolumncreateT[4].err = JET_errSuccess;

    //  column for book description
    //
    //
    rgcolumncreateT[5].cbStruct = sizeof(rgcolumncreateT[4]);
    rgcolumncreateT[5].szColumnName = szBookDescription;
    rgcolumncreateT[5].coltyp = JET_coltypLongText;
    rgcolumncreateT[5].cbMax = 0;
    rgcolumncreateT[5].grbit = NO_GRBIT;
    rgcolumncreateT[5].pvDefault = NULL;
    rgcolumncreateT[5].cbDefault = 0;
    rgcolumncreateT[5].cp = CP_ANSI;
    rgcolumncreateT[5].columnid = 0;
    rgcolumncreateT[5].err = JET_errSuccess;

    //  primary index on BookID
    //
    rgindexcreateT[0].cbStruct = sizeof(rgindexcreateT[0]);
    rgindexcreateT[0].szIndexName = szBookIDIndex;
    rgindexcreateT[0].szKey = rgbBookIDIndex;
    rgindexcreateT[0].cbKey = cchBookIDIndex;
    rgindexcreateT[0].grbit = JET_bitIndexPrimary;
    rgindexcreateT[0].ulDensity = 80;
    rgindexcreateT[0].lcid = MAKELCID( 0x409, SORT_DEFAULT );
    rgindexcreateT[0].cbVarSegMac = 0;
    rgindexcreateT[0].rgconditionalcolumn = NULL;
    rgindexcreateT[0].cConditionalColumn = 0;
    rgindexcreateT[0].err = JET_errSuccess;

    //  secondary index on BookAuthors
    //
    //
    rgindexcreateT[1].cbStruct = sizeof(rgindexcreateT[0]);
    rgindexcreateT[1].szIndexName = szBookAuthorsIndex;
    rgindexcreateT[1].szKey = rgbBookAuthorsIndex;
    rgindexcreateT[1].cbKey = cchBookAuthorsIndex;
    rgindexcreateT[1].grbit = NO_GRBIT;
    rgindexcreateT[1].ulDensity = 80;
    rgindexcreateT[1].lcid = MAKELCID( 0x409, SORT_DEFAULT );
    rgindexcreateT[1].cbVarSegMac = 0;
    rgindexcreateT[1].rgconditionalcolumn = NULL;
    rgindexcreateT[1].cConditionalColumn = 0;
    rgindexcreateT[1].err = JET_errSuccess;

    //  secondary index on BookTitle, BookAuthors
    //
    //
    rgindexcreateT[2].cbStruct = sizeof(rgindexcreateT[2]);
    rgindexcreateT[2].szIndexName = szBookTitleAuthorsIndex;
    rgindexcreateT[2].szKey = rgbBookTitleAuthorsIndex;
    rgindexcreateT[2].cbKey = cchBookTitleAuthorsIndex;
    rgindexcreateT[2].grbit = NO_GRBIT;
    rgindexcreateT[2].ulDensity = 80;
    rgindexcreateT[2].lcid = MAKELCID( 0x409, SORT_DEFAULT );
    rgindexcreateT[2].cbVarSegMac = 0;
    rgindexcreateT[2].rgconditionalcolumn = NULL;
    rgindexcreateT[2].cConditionalColumn = 0;
    rgindexcreateT[2].err = JET_errSuccess;
    
    tablecreateT.cbStruct = sizeof(tablecreateT);
    tablecreateT.szTableName = szBooksTable;
    tablecreateT.szTemplateTableName = NULL;
    tablecreateT.ulPages = 16;
    tablecreateT.ulDensity = 80;
    tablecreateT.rgcolumncreate = rgcolumncreateT;
    tablecreateT.cColumns = 6;
    tablecreateT.rgindexcreate = rgindexcreateT;
    tablecreateT.cIndexes = 3;
    tablecreateT.grbit = NO_GRBIT;
    tablecreateT.tableid =  JET_tableidNil;
    tablecreateT.cCreated = 0;

    Call( JetCreateTableColumnIndex( sesidT, dbidDatabase, &tablecreateT ) );
    //  close table handle opened implicitly by create table
    //
    Call( JetCloseTable( sesidT, tablecreateT.tableid ) );

    //  here JET_bitCommitLazyFlush is used to commit the transaction without synchronously flushing the log.
    //  This is a means of ensuring write ordering without incurring the cost of flushing changes synchronously.
    //
    Call( JetCommitTransaction( sesidT, JET_bitCommitLazyFlush) );

HandleError:
    if ( err < 0 )
    {
        (void)JetRollback( sesidT, 0 );
    }

    return err;
}


//  helper function, converts local time to little-endian 8 byte integer
//
void GetLocalTimeAsLongLong( unsigned char *rgbLocalTime )
{
    SYSTEMTIME  st;
    int         msecT;

    GetLocalTime( &st );

    msecT = (((((st.wHour * 60) + st.wMinute) * 60) + st.wSecond) * 1000) + st.wMilliseconds;

    //  normalize local time into little-endian binary
    //
    rgbLocalTime[7]=(unsigned char)(st.wYear>>8);
    rgbLocalTime[6]=(unsigned char)(st.wYear&0xff);
    rgbLocalTime[5]=(unsigned char)(st.wMonth&0xff);
    rgbLocalTime[4]=(unsigned char)(st.wDay&0xff);
    rgbLocalTime[3]=(unsigned char)(msecT>>24);
    rgbLocalTime[2]=(unsigned char)((msecT>>16)&0xff);
    rgbLocalTime[1]=(unsigned char)((msecT>>8)&0xff);
    rgbLocalTime[0]=(unsigned char)((msecT)&0xff);
}


JET_ERR ErrProcessOrder( JET_SESID sesidT, JET_TABLEID tableidOrders, JET_TABLEID tableidBooks, long lCustomerID, long *rglItems, long clItems )
{
    JET_ERR                 err = JET_errSuccess;
    int                     ilT = 0;
    __int64                 llSumTotalAmount = 0;
    __int64                 llBookPrice = 0;
    unsigned long               ulT = 0;
    JET_SETINFO             setinfoT;
    unsigned char               rgbDate[8];

    setinfoT.cbStruct = sizeof(setinfoT);
    setinfoT.ibLongValue = 0;
    setinfoT.itagSequence = 0;

    err = JetBeginTransaction( sesidT );
    if ( err < 0 )
    {
        return err;
    }

    //  set table handle on Books table to be on BookID index, e.g. the primary index
    //
    Call( JetSetCurrentIndex2( sesidT, tableidBooks, szBookIDIndex, NO_GRBIT ) );

    //  begin to create order record.  As we go through all the items computing the sum
    //  we will also add each item to the forming record.  Records are constructed in a copy buffer
    //  before being added to the table.
    //
    Call( JetPrepareUpdate( sesidT, tableidOrders, JET_prepInsert ) );
    Call( JetSetColumn( sesidT, tableidOrders, columnidOrderCustomerID, &lCustomerID, sizeof(lCustomerID), 0, NULL ) );

    //  compute sum total amount for order by seeking into Books table for book price
    //  As each item is summed, the item itself is set in the order copy buffer.
    //
    for ( ; ilT < clItems; ilT++ )
    {
        //  seek into Books table and retrieve book price, and add it to order sum total amount
        //
        Call( JetMakeKey( sesidT, tableidBooks, &rglItems[ilT], sizeof(rglItems[0]), JET_bitNewKey ) );
        Call( JetSeek( sesidT, tableidBooks, JET_bitSeekEQ ) );
        //  should have found equal
        //
        if ( JET_errSuccess != err )
        {
            goto HandleError;
        }

        Call( JetRetrieveColumn( 
            sesidT, 
            tableidBooks, 
            columnidBookPrice, 
            &llBookPrice, 
            sizeof(llBookPrice), 
            &ulT, 
            0, 
            NULL ) );
        llSumTotalAmount += llBookPrice;

        //  add item to Order record
        //  itagSequence is 1 based, so must add 1 to 0 based offset. 
        //
        setinfoT.itagSequence = ilT + 1;
        Call( JetSetColumn( sesidT, tableidOrders, columnidOrderItems, &rglItems[ilT], sizeof(rglItems[0]), 0, &setinfoT ) );
    }
    
    //  set sum total amount in Order record and insert Order record
    //
    Call( JetSetColumn( sesidT, tableidOrders, columnidOrderAmount, &llSumTotalAmount, sizeof(llSumTotalAmount), 0, NULL ) );
    GetLocalTimeAsLongLong( rgbDate );
    Call( JetSetColumn( sesidT, tableidOrders, columnidOrderDate, rgbDate, sizeof(rgbDate), 0, NULL ) );
    Call( JetUpdate( sesidT, tableidOrders, NULL, 0, NULL ) );

    //  here JET_bitCommitLazyFlush is not used because we may want to ensure that an update will be made as soon as we see
    //  success returned from this function.
    //
    Call( JetCommitTransaction( sesidT, NO_GRBIT ) );

HandleError:
    if ( err < 0 )
    {
        (void)JetRollback( sesidT, NO_GRBIT );
    }
    return err;
}


JET_ERR ErrAddDataToTables( JET_SESID sesidT, JET_DBID dbidDatabase, JET_TABLEID tableidCustomers, JET_TABLEID tableidOrders, JET_TABLEID tableidBooks )
{
    JET_ERR                 err = JET_errSuccess;
    
    //  common variable for data backing
    //
    JET_SETCOLUMN           rgsetcolumnT[10];
    unsigned char               rgbBookCoverT[32768];

    memset( rgbBookCoverT, 0, sizeof(rgbBookCoverT) );

    err = JetBeginTransaction( sesidT );
    if ( err < 0 )
    {
        return err;
    }

    //  create five customers
    //  use per-column set column operation since only one column is being set
    //
    Call( JetPrepareUpdate( sesidT, tableidCustomers, JET_prepInsert ) );
    Call( JetSetColumn( sesidT, tableidCustomers, columnidCustomerName, szCustomerName1, sizeof(szCustomerName1), 0, NULL ) );
    Call( JetUpdate( sesidT, tableidCustomers, NULL, 0, NULL ) );

    Call( JetPrepareUpdate( sesidT, tableidCustomers, JET_prepInsert ) );
    Call( JetSetColumn( sesidT, tableidCustomers, columnidCustomerName, szCustomerName2, sizeof(szCustomerName2), 0, NULL ) );
    Call( JetUpdate( sesidT, tableidCustomers, NULL, 0, NULL ) );

    Call( JetPrepareUpdate( sesidT, tableidCustomers, JET_prepInsert ) );
    Call( JetSetColumn( sesidT, tableidCustomers, columnidCustomerName, szCustomerName3, sizeof(szCustomerName3), 0, NULL ) );
    Call( JetUpdate( sesidT, tableidCustomers, NULL, 0, NULL ) );

    Call( JetPrepareUpdate( sesidT, tableidCustomers, JET_prepInsert ) );
    Call( JetSetColumn( sesidT, tableidCustomers, columnidCustomerName, szCustomerName4, sizeof(szCustomerName4), 0, NULL ) );
    Call( JetUpdate( sesidT, tableidCustomers, NULL, 0, NULL ) );

    Call( JetPrepareUpdate( sesidT, tableidCustomers, JET_prepInsert ) );
    Call( JetSetColumn( sesidT, tableidCustomers, columnidCustomerName, szCustomerName5, sizeof(szCustomerName5), 0, NULL ) );
    Call( JetUpdate( sesidT, tableidCustomers, NULL, 0, NULL ) );


    //  create three books
    //  use multi-column set columns operation since multiple columns will be set
    //
    Call( JetPrepareUpdate( sesidT, tableidBooks, JET_prepInsert ) );
    rgsetcolumnT[0].columnid = columnidBookTitle;
    rgsetcolumnT[0].pvData = (void *)wszBookTitle1;
    rgsetcolumnT[0].cbData = sizeof(wszBookTitle1);
    rgsetcolumnT[0].grbit = NO_GRBIT;
    rgsetcolumnT[0].ibLongValue = 0;
    rgsetcolumnT[0].itagSequence = 1;
    rgsetcolumnT[0].err = JET_errSuccess;
    rgsetcolumnT[1].columnid = columnidBookAuthors;
    rgsetcolumnT[1].pvData = (void *)wszBookAuthor1;
    rgsetcolumnT[1].cbData = sizeof(wszBookAuthor1);
    rgsetcolumnT[1].grbit = NO_GRBIT;
    rgsetcolumnT[1].ibLongValue = 0;
    rgsetcolumnT[1].itagSequence = 1;
    rgsetcolumnT[1].err = JET_errSuccess;
    rgsetcolumnT[2].columnid = columnidBookCover;
    rgsetcolumnT[2].pvData = (void *)rgbBookCoverT;
    rgsetcolumnT[2].cbData = sizeof(rgbBookCoverT);
    rgsetcolumnT[2].grbit = NO_GRBIT;
    rgsetcolumnT[2].ibLongValue = 0;
    rgsetcolumnT[2].itagSequence = 1;
    rgsetcolumnT[2].err = JET_errSuccess;
    rgsetcolumnT[3].columnid = columnidBookPrice;
    rgsetcolumnT[3].pvData = (void *)&llBookPrice1;
    rgsetcolumnT[3].cbData = sizeof(llBookPrice1);
    rgsetcolumnT[3].grbit = NO_GRBIT;
    rgsetcolumnT[3].ibLongValue = 0;
    rgsetcolumnT[3].itagSequence = 1;
    rgsetcolumnT[3].err = JET_errSuccess;
    rgsetcolumnT[4].columnid = columnidBookDescription;
    rgsetcolumnT[4].pvData = (void *)wszBookDescription1;
    rgsetcolumnT[4].cbData = sizeof(wszBookDescription1);
    rgsetcolumnT[4].grbit = NO_GRBIT;
    rgsetcolumnT[4].ibLongValue = 0;
    rgsetcolumnT[4].itagSequence = 1;
    rgsetcolumnT[4].err = JET_errSuccess;
    Call( JetSetColumns( sesidT, tableidBooks, rgsetcolumnT, 5 ) );
    Call( JetUpdate( sesidT, tableidBooks, NULL, 0, NULL ) );

    Call( JetPrepareUpdate( sesidT, tableidBooks, JET_prepInsert ) );
    rgsetcolumnT[0].columnid = columnidBookTitle;
    rgsetcolumnT[0].pvData = (void *)wszBookTitle2;
    rgsetcolumnT[0].cbData = sizeof(wszBookTitle2);
    rgsetcolumnT[0].grbit = NO_GRBIT;
    rgsetcolumnT[0].ibLongValue = 0;
    rgsetcolumnT[0].itagSequence = 1;
    rgsetcolumnT[0].err = JET_errSuccess;
    rgsetcolumnT[1].columnid = columnidBookAuthors;
    rgsetcolumnT[1].pvData = (void *)wszBookAuthor2First;
    rgsetcolumnT[1].cbData = sizeof(wszBookAuthor2First);
    rgsetcolumnT[1].grbit = NO_GRBIT;
    rgsetcolumnT[1].ibLongValue = 0;
    rgsetcolumnT[1].itagSequence = 1;
    rgsetcolumnT[1].err = JET_errSuccess;
    rgsetcolumnT[2].columnid = columnidBookAuthors;
    rgsetcolumnT[2].pvData = (void *)wszBookAuthor2Second;
    rgsetcolumnT[2].cbData = sizeof(wszBookAuthor2Second);
    rgsetcolumnT[2].grbit = NO_GRBIT;
    rgsetcolumnT[2].ibLongValue = 0;
    rgsetcolumnT[2].itagSequence = 2 /* itagSequence set to 2 to set second value of multi-valued column */;
    rgsetcolumnT[2].err = JET_errSuccess;
    rgsetcolumnT[3].columnid = columnidBookCover;
    rgsetcolumnT[3].pvData = (void *)rgbBookCoverT;
    rgsetcolumnT[3].cbData = sizeof(rgbBookCoverT);
    rgsetcolumnT[3].grbit = NO_GRBIT;
    rgsetcolumnT[3].ibLongValue = 0;
    rgsetcolumnT[3].itagSequence = 1;
    rgsetcolumnT[3].err = JET_errSuccess;
    rgsetcolumnT[4].columnid = columnidBookPrice;
    rgsetcolumnT[4].pvData = (void *)&llBookPrice2;
    rgsetcolumnT[4].cbData = sizeof(llBookPrice2);
    rgsetcolumnT[4].grbit = NO_GRBIT;
    rgsetcolumnT[4].ibLongValue = 0;
    rgsetcolumnT[4].itagSequence = 1;
    rgsetcolumnT[4].err = JET_errSuccess;
    rgsetcolumnT[5].columnid = columnidBookDescription;
    rgsetcolumnT[5].pvData = (void *)wszBookDescription2;
    rgsetcolumnT[5].cbData = sizeof(wszBookDescription2);
    rgsetcolumnT[5].grbit = NO_GRBIT;
    rgsetcolumnT[5].ibLongValue = 0;
    rgsetcolumnT[5].itagSequence = 1;
    rgsetcolumnT[5].err = JET_errSuccess;
    Call( JetSetColumns( sesidT, tableidBooks, rgsetcolumnT, 6 ) );
    Call( JetUpdate( sesidT, tableidBooks, NULL, 0, NULL ) );

    Call( JetPrepareUpdate( sesidT, tableidBooks, JET_prepInsert ) );
    rgsetcolumnT[0].columnid = columnidBookTitle;
    rgsetcolumnT[0].pvData = (void *)wszBookTitle3;
    rgsetcolumnT[0].cbData = sizeof(wszBookTitle3);
    rgsetcolumnT[0].grbit = NO_GRBIT;
    rgsetcolumnT[0].ibLongValue = 0;
    rgsetcolumnT[0].itagSequence = 1;
    rgsetcolumnT[0].err = JET_errSuccess;
    rgsetcolumnT[1].columnid = columnidBookAuthors;
    rgsetcolumnT[1].pvData = (void *)wszBookAuthor3;
    rgsetcolumnT[1].cbData = sizeof(wszBookAuthor3);
    rgsetcolumnT[1].grbit = NO_GRBIT;
    rgsetcolumnT[1].ibLongValue = 0;
    rgsetcolumnT[1].itagSequence = 1;
    rgsetcolumnT[1].err = JET_errSuccess;
    rgsetcolumnT[2].columnid = columnidBookCover;
    rgsetcolumnT[2].pvData = NULL;
    rgsetcolumnT[2].cbData = 0;
    rgsetcolumnT[2].grbit = NO_GRBIT;
    rgsetcolumnT[2].ibLongValue = 0;
    rgsetcolumnT[2].itagSequence = 1;
    rgsetcolumnT[2].err = JET_errSuccess;
    rgsetcolumnT[3].columnid = columnidBookPrice;
    rgsetcolumnT[3].pvData = (void *)&llBookPrice3;
    rgsetcolumnT[3].cbData = sizeof(llBookPrice3);
    rgsetcolumnT[3].grbit = NO_GRBIT;
    rgsetcolumnT[3].ibLongValue = 0;
    rgsetcolumnT[3].itagSequence = 1;
    rgsetcolumnT[3].err = JET_errSuccess;
    rgsetcolumnT[4].columnid = columnidBookDescription;
    rgsetcolumnT[4].pvData = (void *)wszBookDescription3;
    rgsetcolumnT[4].cbData = sizeof(wszBookDescription3);
    rgsetcolumnT[4].grbit = NO_GRBIT;
    rgsetcolumnT[4].ibLongValue = 0;
    rgsetcolumnT[4].itagSequence = 1;
    rgsetcolumnT[4].err = JET_errSuccess;
    Call( JetSetColumns( sesidT, tableidBooks, rgsetcolumnT, 5 ) );
    Call( JetUpdate( sesidT, tableidBooks, NULL, 0, NULL ) );
    
    //  process four orders
    //
    Call( ErrProcessOrder( sesidT, tableidOrders, tableidBooks, lOrderCustomerID1, rglOrderItems1, sizeof(rglOrderItems1)/sizeof(rglOrderItems1[0]) ) );
    Call( ErrProcessOrder( sesidT, tableidOrders, tableidBooks, lOrderCustomerID2, rglOrderItems2, sizeof(rglOrderItems2)/sizeof(rglOrderItems2[0]) ) );
    Call( ErrProcessOrder( sesidT, tableidOrders, tableidBooks, lOrderCustomerID3, rglOrderItems3, sizeof(rglOrderItems3)/sizeof(rglOrderItems3[0]) ) );
    Call( ErrProcessOrder( sesidT, tableidOrders, tableidBooks, lOrderCustomerID4, rglOrderItems4, sizeof(rglOrderItems4)/sizeof(rglOrderItems4[0]) ) );

    Call( JetCommitTransaction( sesidT, 0 ) );

HandleError:
    if ( err < 0 )
    {
        (void)JetRollback( sesidT, 0 );
    }
    return err;
}


//  dump Customer table information to output screen.
//
JET_ERR ErrDumpCustomersToScreen( JET_SESID sesidT, JET_TABLEID tableidCustomers )
{
    JET_ERR                 err = JET_errSuccess;
    JET_RETRIEVECOLUMN      rgretrievecolumnT[10];
    char                    szCustomerNameT[256];
    long                    lCustomerIDT = 0;

    rgretrievecolumnT[0].columnid = columnidCustomerName;
    rgretrievecolumnT[0].pvData = szCustomerNameT;
    rgretrievecolumnT[0].cbData = sizeof(szCustomerNameT);
    rgretrievecolumnT[0].cbActual = 0;
    rgretrievecolumnT[0].grbit = NO_GRBIT;
    rgretrievecolumnT[0].ibLongValue = 0;
    rgretrievecolumnT[0].itagSequence = 1;
    rgretrievecolumnT[0].columnidNextTagged = 0;
    rgretrievecolumnT[0].err = JET_errSuccess;
    rgretrievecolumnT[1].columnid = columnidCustomerID;
    rgretrievecolumnT[1].pvData = &lCustomerIDT;
    rgretrievecolumnT[1].cbData = sizeof(lCustomerIDT);
    rgretrievecolumnT[1].cbActual = 0;
    rgretrievecolumnT[1].grbit = NO_GRBIT;
    rgretrievecolumnT[1].ibLongValue = 0;
    rgretrievecolumnT[1].itagSequence = 1;
    rgretrievecolumnT[1].columnidNextTagged = 0;
    rgretrievecolumnT[1].err = JET_errSuccess;

    //  open transaction to get consistent view of data
    //
    err = JetBeginTransaction( sesidT );
    if ( err < 0 )
    {
        return err;
    }

    //  Customers table
    //  dump data in alphabetical order
    //
    printf( "Dump of %s table.\n\n", szCustomersTable );
    printf( "Customer Name\t\t\tCustomer ID\n" );
    printf( "--------------------------------------------------------\n" );
    Call( JetSetCurrentIndex2( sesidT, tableidCustomers, szCustomerNameIndex, NO_GRBIT ) );
    for ( err = JetMove( sesidT, tableidCustomers, JET_MoveFirst, NO_GRBIT );
        JET_errNoCurrentRecord != err;
        err = JetMove( sesidT, tableidCustomers, JET_MoveNext, NO_GRBIT ) )
    {
        Call( err );
    
        Call( JetRetrieveColumns( sesidT, tableidCustomers, rgretrievecolumnT, 2 ) );
            
        printf( "%-32s", szCustomerNameT );
        printf("%d\n", lCustomerIDT );
    }
    err = JET_errSuccess;
    printf( "--------------------------------------------------------\n" );
    printf( "\n\n" );

HandleError:
    //  no need to rollback on failure since no changes made.  However, rollback if commit fails.
    //
    err = JetCommitTransaction( sesidT, 0 );
    if ( err < 0 )
    {
        (void)JetRollback( sesidT, 0 );
    }

    return err;
}



JET_ERR ErrQueryTopThreeOrdersByOrderAmount( JET_SESID sesidT, JET_DBID dbidDatabase, JET_TABLEID tableidCustomers, JET_TABLEID tableidOrders, JET_TABLEID tableidBooks )
{
    JET_ERR     err = JET_errSuccess;

    //  order amount is not indexed, and a temporary table is used to sort the data in descending order amount order
    //
    JET_RETRIEVECOLUMN      rgretrievecolumnT[10];
    long                        lOrderID                                = 0;
    long                        lOrderCustomerID                        = 0;
    __int64                 llOrderAmount                           = 0;
    unsigned char               bOrderCancelled                         = 0;
    char                        szCustomerNameT[256];
    unsigned long               ulT                                     = 0;
    int                     iCustomer                               = 0;

    //  result table variables
    //
    JET_TABLEID             tableidTemp                             = JET_tableidNil;
    JET_COLUMNDEF           rgcolumndefTemp[10];
    JET_COLUMNID            rgcolumnidTemp[10];
    JET_SETCOLUMN           rgsetcolumnTemp[10];
    JET_RETRIEVECOLUMN      rgretrievecolumnTemp[10];
    
    const int                   icolumndefOrderAmount                       = 0;
    const int                   icolumndefOrderID                           = 1;
    const int                   icolumndefOrderCustomerID                   = 2;

    err =  JetBeginTransaction( sesidT );
    if ( err < 0 )
        {
        return err;
        }

    //  create temporary table to sort by szOrderAmount
    //
    //  columns in temporary table include:
    //      order amount
    //      order ID (uniquifier for order amount)
    //      customer ID
    //
    rgcolumndefTemp[0].cbStruct = sizeof( JET_COLUMNDEF );
    rgcolumndefTemp[0].cp           = 0;
    rgcolumndefTemp[0].langid       = 0;
    rgcolumndefTemp[0].wCountry = 0;
    rgcolumndefTemp[0].columnid = 0;
    rgcolumndefTemp[0].coltyp       = JET_coltypCurrency;
    rgcolumndefTemp[0].grbit        = JET_bitColumnTTKey|JET_bitColumnTTDescending;
    rgcolumndefTemp[1].cbStruct = sizeof( JET_COLUMNDEF );
    rgcolumndefTemp[1].cp           = 0;
    rgcolumndefTemp[1].langid       = 0;
    rgcolumndefTemp[1].wCountry = 0;
    rgcolumndefTemp[1].columnid = 0;
    rgcolumndefTemp[1].coltyp       = JET_coltypLong;
    rgcolumndefTemp[1].grbit        = JET_bitColumnTTKey;
    rgcolumndefTemp[2].cbStruct = sizeof( JET_COLUMNDEF );
    rgcolumndefTemp[2].cp           = 0;
    rgcolumndefTemp[2].langid       = 0;
    rgcolumndefTemp[2].wCountry = 0;
    rgcolumndefTemp[2].columnid = 0;
    rgcolumndefTemp[2].coltyp       = JET_coltypLong;
    rgcolumndefTemp[2].grbit        = 0;

    Call( JetOpenTempTable( sesidT, rgcolumndefTemp, 3, 0, &tableidTemp, rgcolumnidTemp ) );
    
    //  create set column array to efficiently set data in TT
    //
    rgsetcolumnTemp[0].columnid = rgcolumnidTemp[icolumndefOrderAmount];
    rgsetcolumnTemp[0].pvData = &llOrderAmount;
    rgsetcolumnTemp[0].cbData = sizeof(llOrderAmount);
    rgsetcolumnTemp[0].grbit = NO_GRBIT;
    rgsetcolumnTemp[0].ibLongValue = 0;
    rgsetcolumnTemp[0].itagSequence = 1;
    rgsetcolumnTemp[0].err = 0;
    rgsetcolumnTemp[1].columnid = rgcolumnidTemp[icolumndefOrderID];
    rgsetcolumnTemp[1].pvData = &lOrderID;
    rgsetcolumnTemp[1].cbData = sizeof(lOrderID);
    rgsetcolumnTemp[1].grbit = NO_GRBIT;
    rgsetcolumnTemp[1].ibLongValue = 0;
    rgsetcolumnTemp[1].itagSequence = 1;
    rgsetcolumnTemp[1].err = 0;
    rgsetcolumnTemp[2].columnid = rgcolumnidTemp[icolumndefOrderCustomerID];
    rgsetcolumnTemp[2].pvData = &lOrderCustomerID;
    rgsetcolumnTemp[2].cbData = sizeof(lOrderCustomerID);
    rgsetcolumnTemp[2].grbit = NO_GRBIT;
    rgsetcolumnTemp[2].ibLongValue = 0;
    rgsetcolumnTemp[2].itagSequence = 1;
    rgsetcolumnTemp[2].err = 0;

    
    //  create retrieve column array which retrieves all columns at once
    //
    rgretrievecolumnT[0].columnid = columnidOrderID;
    rgretrievecolumnT[0].pvData = &lOrderID;
    rgretrievecolumnT[0].cbData = sizeof(lOrderID);
    rgretrievecolumnT[0].cbActual = 0;
    rgretrievecolumnT[0].grbit = NO_GRBIT;
    rgretrievecolumnT[0].ibLongValue = 0;
    rgretrievecolumnT[0].itagSequence = 1;
    rgretrievecolumnT[0].columnidNextTagged = 0;
    rgretrievecolumnT[0].err = JET_errSuccess;
    rgretrievecolumnT[1].columnid = columnidOrderCustomerID;
    rgretrievecolumnT[1].pvData = &lOrderCustomerID;
    rgretrievecolumnT[1].cbData = sizeof(lOrderCustomerID);
    rgretrievecolumnT[1].cbActual = 0;
    rgretrievecolumnT[1].grbit = NO_GRBIT;
    rgretrievecolumnT[1].ibLongValue = 0;
    rgretrievecolumnT[1].itagSequence = 1;
    rgretrievecolumnT[1].columnidNextTagged = 0;
    rgretrievecolumnT[1].err = JET_errSuccess;
    rgretrievecolumnT[2].columnid = columnidOrderAmount;
    rgretrievecolumnT[2].pvData = &llOrderAmount;
    rgretrievecolumnT[2].cbData = sizeof(llOrderAmount);
    rgretrievecolumnT[2].cbActual = 0;
    rgretrievecolumnT[2].grbit = NO_GRBIT;
    rgretrievecolumnT[2].ibLongValue = 0;
    rgretrievecolumnT[2].itagSequence = 1;
    rgretrievecolumnT[2].columnidNextTagged = 0;
    rgretrievecolumnT[2].err = JET_errSuccess;
    rgretrievecolumnT[3].columnid = columnidOrderCancelled;
    rgretrievecolumnT[3].pvData = &bOrderCancelled;
    rgretrievecolumnT[3].cbData = sizeof(bOrderCancelled);
    rgretrievecolumnT[3].cbActual = 0;
    rgretrievecolumnT[3].grbit = NO_GRBIT;
    rgretrievecolumnT[3].ibLongValue = 0;
    rgretrievecolumnT[3].itagSequence = 1;
    rgretrievecolumnT[3].columnidNextTagged = 0;
    rgretrievecolumnT[3].err = JET_errSuccess;

    //  since we don't have the index we want, browse in clustered index order to optimize cache locality.
    //  Skip orders that have been cancelled.
    //
    Call( JetSetCurrentIndex2( sesidT, tableidOrders, szOrderIDIndex, NO_GRBIT ) );
    for( err = JetMove( sesidT, tableidOrders, JET_MoveFirst, NO_GRBIT );
        JET_errNoCurrentRecord != err;
        err = JetMove( sesidT, tableidOrders, JET_MoveNext, NO_GRBIT ) )
    {
        Call( err );
        
        Call( JetRetrieveColumns( sesidT, tableidOrders, rgretrievecolumnT, 4 ) );

        //  process only if order is not cancelled
        //
        if ( rgretrievecolumnT[3].err == JET_wrnColumnNull )
            {
            //  for each row found, insert the retreived columns as a new row in the temporary table
            //
            Call( JetPrepareUpdate( sesidT, tableidTemp, JET_prepInsert ) );
            Call( JetSetColumns( sesidT, tableidTemp, rgsetcolumnTemp, 3 ) );
            Call( JetUpdate( sesidT, tableidTemp, NULL, 0, NULL ) );
            }
    }
    err = JET_errSuccess;

    //  dump result to screen
    //
    rgretrievecolumnTemp[0].columnid = rgcolumnidTemp[icolumndefOrderAmount];
    rgretrievecolumnTemp[0].pvData = &llOrderAmount;
    rgretrievecolumnTemp[0].cbData = sizeof(llOrderAmount);
    rgretrievecolumnTemp[0].cbActual = 0;
    rgretrievecolumnTemp[0].grbit = NO_GRBIT;
    rgretrievecolumnTemp[0].ibLongValue = 0;
    rgretrievecolumnTemp[0].itagSequence = 1;
    rgretrievecolumnTemp[0].columnidNextTagged = 0;
    rgretrievecolumnTemp[0].err = 0;
    rgretrievecolumnTemp[1].columnid = rgcolumnidTemp[icolumndefOrderCustomerID];
    rgretrievecolumnTemp[1].pvData = &lOrderCustomerID;
    rgretrievecolumnTemp[1].cbData = sizeof(lOrderCustomerID);
    rgretrievecolumnTemp[1].cbActual = 0;
    rgretrievecolumnTemp[1].grbit = NO_GRBIT;
    rgretrievecolumnTemp[1].ibLongValue = 0;
    rgretrievecolumnTemp[1].itagSequence = 1;
    rgretrievecolumnTemp[1].columnidNextTagged = 0;
    rgretrievecolumnTemp[1].err = 0;


    printf( "\n\nTop Three Orders by Order Amount.\n\n" );
    printf( "Customer Name\t\t\tAmount\n" );
    printf( "--------------------------------------------------------\n" );
    //  actual sort occurs on first move here
    //
    //  set table handle on Books table to be on BookID index, e.g. the primary index
    //
    Call( JetSetCurrentIndex2( sesidT, tableidCustomers, szCustomerIDIndex, 0 ) );
    for ( iCustomer = 0, err = JetMove( sesidT, tableidTemp, JET_MoveFirst, NO_GRBIT );
        JET_errNoCurrentRecord != err && iCustomer < 3;
         err = JetMove( sesidT, tableidTemp, JET_MoveNext, NO_GRBIT ) )
    {
        //  check other errors from JetMove
        //
        Call( err );

        //  retrieve all desired columns
        //
        Call( JetRetrieveColumns( sesidT, tableidTemp, rgretrievecolumnTemp, 2 ) );

        //  join to Customers table to get customer name from OrderCustomerID
        //
        Call( JetMakeKey( sesidT, tableidCustomers, &lOrderCustomerID, sizeof(lOrderCustomerID), JET_bitNewKey ) );
        Call( JetSeek( sesidT, tableidCustomers, JET_bitSeekEQ ) );
        //  should have found equal
        //
        if ( JET_errSuccess != err )
        {
            goto HandleError;
        }
        Call( JetRetrieveColumn( 
            sesidT, 
            tableidCustomers, 
            columnidCustomerName, 
            &szCustomerNameT, 
            sizeof(szCustomerNameT), 
            &ulT,
            0, 
            NULL ) );

        printf( "%-32s", szCustomerNameT );
        printf( "$%d\n", llOrderAmount/10000 );
    }
    err = JET_errSuccess;
    
    printf( "--------------------------------------------------------\n" );
    printf( "\n\n" );

    Call( JetCommitTransaction( sesidT, 0 ) );

HandleError:
    if ( JET_tableidNil != tableidTemp )
        {
        JetCloseTable( sesidT, tableidTemp );
        tableidTemp = JET_tableidNil;
        }
        
    if ( err < 0 )
        {
        JetRollback( sesidT, 0 );
        }

    return err;
}


void __cdecl main2(int argc, char ** argv)
{

    JET_ERR                 err                                     = JET_errSuccess;
    JET_INSTANCE            instance                                    = 0;
    JET_SESID               sesidT                                  = JET_sesidNil;
    JET_DBID                dbidDatabase                            = JET_dbidNil;

    //  table variables
    //
    JET_TABLEID                 tableidCustomers                        = JET_tableidNil;
    JET_TABLEID                 tableidOrders                           = JET_tableidNil;
    JET_TABLEID                 tableidBooks                            = JET_tableidNil;
    JET_COLUMNDEF           columndefT;

    ////////////////////////////////////////////////////////
    //  INITIALIZATION
    //

    //  some system parameters must be set prior to calling JetInit
    //
    Call( JetSetSystemParameter( &instance, 0, JET_paramSystemPath, 0, ".\\" ) );
    Call( JetSetSystemParameter( &instance, 0, JET_paramTempPath, 0, ".\\" ) );
    Call( JetSetSystemParameter( &instance, 0, JET_paramLogFilePath, 0, ".\\" ) );
    Call( JetSetSystemParameter( &instance, 0, JET_paramBaseName, 0, "edb" ) );
    Call( JetSetSystemParameter( &instance, 0, JET_paramEventSource, 0, "query.exe" ) );

    //  set logging option such that logs unneeded for crash recovery are automatically deleted.
    //  This prevents replaying a backup all the way to the current point in time 
    //  but reduces the need for regular backup to remove log files.
    //
    Call( JetSetSystemParameter( &instance, 0, JET_paramCircularLog, 1, NULL ) );

    //  below system parameters should not need to be set in most cases
    //
    //  Call( JetSetSystemParameter( &instance, 0, JET_paramMaxSessions, 16, NULL ) );
    //  Call( JetSetSystemParameter( &instance, 0, JET_paramMaxOpenTables, 300, NULL ) );
    //  Call( JetSetSystemParameter( &instance, 0, JET_paramMaxCursors, 1024, NULL ) );
    //  Call( JetSetSystemParameter( &instance, 0, JET_paramMaxVerPages, 64, NULL ) );
    //  Call( JetSetSystemParameter( &instance, 0, JET_paramPreferredVerPages, 64, NULL ) );
    //  Call( JetSetSystemParameter( &instance, 0, JET_paramGlobalMinVerPages, 64, NULL ) );
    //  Call( JetSetSystemParmaeter( &instance, 0, JET_paramVersionStoreTaskQueueMax, 32, NULL ) );
    //  Call( JetSetSystemParameter( &instance, 0, JET_paramMaxTemporaryTables, 30, NULL ) );
    //  Call( JetSetSystemParameter( &instance, 0, JET_paramLogFileSize, 5120, NULL ) );
    //  Call( JetSetSystemParameter( &instance, 0, JET_paramLogBuffers, 80, NULL ) );
    //
    //  note skip JET_paramStartFlushThreshold, JET_paramStopFlushThreshold, and JetSetSystemParameter
    //  since they are now set automatically via dynamic buffer allocation algorithm in ESE/NT.
    //
    //  Call( JetSetSystemParameter( &instance, 0, JET_paramCommitDefault, 0, NULL ) );

    //  call JetInit to initialize ESE
    //
    Call( JetInit( &instance ) );

    //  one or more sessions should be begun since all subsequent ESE operations are performed in the context of a session.
    //  A separate session should be opened for each thread using ESE.
    //
    Call( JetBeginSession( instance, &sesidT, szUser, szPassword ) );


    ////////////////////////////////////////////////////////
    //  DATA DEFINITION
    //
    //  create schema as required
    //

    //  look for database and create if not found
    //
    err = JetAttachDatabase( sesidT, szDatabase, 0 );
    if ( JET_errFileNotFound == err )
    {
        Call( JetCreateDatabase( sesidT, szDatabase, NULL, &dbidDatabase, 0 ) );
    }
    else
        {
        Call( err );
        Call( JetOpenDatabase( sesidT, szDatabase, "", &dbidDatabase, 0 ) );
        }

    //  look for table and create if not found
    //
    err = JetOpenTable( sesidT, dbidDatabase, szCustomersTable, NULL, 0, 0L, &tableidCustomers );
    if ( JET_errObjectNotFound == err )
    {
        Call( ErrCreateTablesColumnsAndIndexes( sesidT, dbidDatabase ) );
        Call( JetOpenTable( sesidT, dbidDatabase, szCustomersTable, NULL, 0, 0L, &tableidCustomers ) );
    }
    Call( err );

    //  by now all tables, columns and indexes should exist.  
    //  Open tables to get open table handle, and set column ids for all columns.
    //
    Call( JetGetTableColumnInfo(
        sesidT,
        tableidCustomers,
        szCustomerID,
        &columndefT,
        sizeof(columndefT),
        JET_ColInfo ) );
    columnidCustomerID = columndefT.columnid;
    Call( JetGetTableColumnInfo(
        sesidT,
        tableidCustomers,
        szCustomerName,
        &columndefT,
        sizeof(columndefT),
        JET_ColInfo ) );
    columnidCustomerName = columndefT.columnid;
    
    Call( JetOpenTable( sesidT, dbidDatabase, szOrdersTable, NULL, 0, 0L, &tableidOrders ) );
    Call( JetGetTableColumnInfo(
        sesidT,
        tableidOrders,
        szOrderID,
        &columndefT,
        sizeof(columndefT),
        JET_ColInfo ) );
    columnidOrderID = columndefT.columnid;
    Call( JetGetTableColumnInfo(
        sesidT,
        tableidOrders,
        szOrderCustomerID,
        &columndefT,
        sizeof(columndefT),
        JET_ColInfo ) );
    columnidOrderCustomerID = columndefT.columnid;
    Call( JetGetTableColumnInfo(
        sesidT,
        tableidOrders,
        szOrderDate,
        &columndefT,
        sizeof(columndefT),
        JET_ColInfo ) );
    columnidOrderDate = columndefT.columnid;
    Call( JetGetTableColumnInfo(
        sesidT,
        tableidOrders,
        szOrderAmount,
        &columndefT,
        sizeof(columndefT),
        JET_ColInfo ) );
    columnidOrderAmount = columndefT.columnid;
    Call( JetGetTableColumnInfo(
        sesidT,
        tableidOrders,
        szOrderCancelled,
        &columndefT,
        sizeof(columndefT),
        JET_ColInfo ) );
    columnidOrderCancelled = columndefT.columnid;
    Call( JetGetTableColumnInfo(
        sesidT,
        tableidOrders,
        szOrderItems,
        &columndefT,
        sizeof(columndefT),
        JET_ColInfo ) );
    columnidOrderItems = columndefT.columnid;

    Call( JetOpenTable( sesidT, dbidDatabase, szBooksTable, NULL, 0, 0L, &tableidBooks ) );
    Call( JetGetTableColumnInfo(
        sesidT,
        tableidBooks,
        szBookID,
        &columndefT,
        sizeof(columndefT),
        JET_ColInfo ) );
    columnidBookID = columndefT.columnid;
    Call( JetGetTableColumnInfo(
        sesidT,
        tableidBooks,
        szBookTitle,
        &columndefT,
        sizeof(columndefT),
        JET_ColInfo ) );
    columnidBookTitle = columndefT.columnid;
    Call( JetGetTableColumnInfo(
        sesidT,
        tableidBooks,
        szBookAuthors,
        &columndefT,
        sizeof(columndefT),
        JET_ColInfo ) );
    columnidBookAuthors = columndefT.columnid;
    Call( JetGetTableColumnInfo(
        sesidT,
        tableidBooks,
        szBookCover,
        &columndefT,
        sizeof(columndefT),
        JET_ColInfo ) );
    columnidBookCover = columndefT.columnid;
    Call( JetGetTableColumnInfo(
        sesidT,
        tableidBooks,
        szBookPrice,
        &columndefT,
        sizeof(columndefT),
        JET_ColInfo ) );
    columnidBookPrice = columndefT.columnid;
    Call( JetGetTableColumnInfo(
        sesidT,
        tableidBooks,
        szBookDescription,
        &columndefT,
        sizeof(columndefT),
        JET_ColInfo ) );
    columnidBookDescription = columndefT.columnid;


    ////////////////////////////////////////////////////////
    //  DATA MANIPULATION
    //
    //  create data as required
    //
    err = JetMove( sesidT, tableidCustomers, JET_MoveFirst, NO_GRBIT );
    if ( JET_errNoCurrentRecord == err )
    {
        Call( ErrAddDataToTables( sesidT, dbidDatabase, tableidCustomers, tableidOrders, tableidBooks ) );
    }

    //  dump data to screen
    //
    Call( ErrDumpCustomersToScreen( sesidT, tableidCustomers ) );

    //  query data
    //
    Call( ErrQueryTopThreeOrdersByOrderAmount( sesidT, dbidDatabase, tableidCustomers, tableidOrders, tableidBooks ) );

HandleError:
    if ( err < 0 )
    {
        printf( "Sample failed with error %d.\n", err );
    }
    else
    {
        printf( "Sample succeeded.\n" );
    }

    ////////////////////////////////////////////////////////
    //  TERMINATION
    //
    if ( JET_tableidNil != tableidBooks )
    {
        JetCloseTable( sesidT, tableidBooks );
        tableidBooks = JET_tableidNil;
    }

    if ( JET_tableidNil != tableidOrders )
    {
        JetCloseTable( sesidT, tableidOrders );
        tableidOrders = JET_tableidNil;
    }

    if ( JET_tableidNil != tableidCustomers )
    {
        JetCloseTable( sesidT, tableidCustomers );
        tableidCustomers = JET_tableidNil;
    }

    if ( JET_dbidNil != dbidDatabase )
    {
        JetCloseDatabase( sesidT, dbidDatabase, 0 );
        dbidDatabase = JET_dbidNil;
    }

    if ( JET_sesidNil != sesidT )
    {
        JetEndSession( sesidT, 0 );
        sesidT = JET_sesidNil;
    }

    //  always call JetTerm!!!
    //
    if ( 0 /* JET_instanceNil */ != instance )
    {
        JetTerm( instance );
        instance = 0;
    }


    return;
}
