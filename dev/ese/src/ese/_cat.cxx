// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  version number embedded in name
//  if the table is changed in an incompatible way, increase the version string
//  and delete the old ones
const CHAR  szMSU1[]                = "MSysUnicodeFixupVer1";
const CHAR  szMSU[]                 = "MSysUnicodeFixupVer2";   //  also referenced in eseutil.cxx
const CHAR  szMSO[]                 = "MSysObjects";
const CHAR  szMSOShadow[]           = "MSysObjectsShadow";
const CHAR  szMSOIdIndex[]          = "Id";
const CHAR  szMSONameIndex[]        = "Name";
const CHAR  szMSORootObjectsIndex[] = "RootObjects";

const CHAR  szMSObjids[]            = "MSysObjids";

const CHAR  szMSLocales[]           = "MSysLocales";
const WCHAR wszMSLocales[]          = L"MSysLocales";   // MUST match

const CHAR  szLVRoot[]              = "LV";
const ULONG cbLVRoot                = sizeof(szLVRoot) - 1;

const CHAR  szTableCallback[]       = "CB";
const ULONG cbTableCallback         = sizeof(szTableCallback) - 1;


struct CDESC                            // Column Description
{
    CHAR                *szColName;     // column name
    COLUMNID            columnid;       // hard-coded fid
    JET_COLTYP          coltyp;         // column type
    JET_GRBIT           grbit;          // flag bits
};

struct IDESC                            // Index Description
{
    CHAR                *szIdxName;     // index name
    CHAR                *szIdxKeys;     // key string
    JET_GRBIT           grbit;          // flag bits
};


//  This array tracks the original columns used in the MSO
//  table before we had versioning for the CreateDB log record. 
LOCAL const COLUMNID rgcolumnidOldMSO[] =
{
    fidMSO_ObjidTable,
    fidMSO_Type,
    fidMSO_Id,
    fidMSO_Name,
    fidMSO_Coltyp,
    fidMSO_SpaceUsage,
    fidMSO_Flags,
    fidMSO_Pages,
    fidMSO_Stats,
    fidMSO_RootFlag,
    fidMSO_TemplateTable,
    fidMSO_RecordOffset,
    fidMSO_DefaultValue,
    fidMSO_KeyFldIDs,
    fidMSO_VarSegMac,
    fidMSO_ConditionalColumns,
    fidMSO_LCMapFlags,
    fidMSO_KeyMost,
    fidMSO_TupleLimits,
    fidMSO_Version,
    fidMSO_CallbackData,
    fidMSO_CallbackDependencies,
    fidMSO_SeparateLVThreshold,
    fidMSO_SpaceHints,
    fidMSO_SpaceLVDeferredHints,

    //  You should not add any columns to this array, period.   
};

C_ASSERT( _countof( rgcolumnidOldMSO ) == 25 );

//  IMPORTANT: The entries here must match the same order as the iMSO_* constants in cat.hxx
LOCAL const CDESC   rgcdescMSO[]    =
{
    //  cluster system objects by table - all related table objects have
    //  the same clustering id, which will be the object id of the table
    "ObjidTable",       fidMSO_ObjidTable,      JET_coltypLong,         JET_bitColumnNotNULL,

    //  see SYSOBJ for valid types
    "Type",             fidMSO_Type,            JET_coltypShort,        JET_bitColumnNotNULL,

    //  object id for tables/indexes/LV, columnid for columns
    "Id",               fidMSO_Id,              JET_coltypLong,         JET_bitColumnNotNULL,

    //  table/column/index/LV name
    "Name",                 fidMSO_Name,                JET_coltypText,         JET_bitColumnNotNULL,

    //  column type for columns, pgnoFDP for tables/indexes/LV
    "ColtypOrPgnoFDP",      fidMSO_Coltyp,              JET_coltypLong,         JET_bitColumnNotNULL,
    
    //  density for tables/indexes/LV, column length for columns
    "SpaceUsage",           fidMSO_SpaceUsage,          JET_coltypLong,         JET_bitColumnNotNULL,
    
    "Flags",                fidMSO_Flags,               JET_coltypLong,         JET_bitColumnNotNULL,

    //  initial pages for table/LV, code page for columns, localeid for indexes
    "PagesOrLocale",        fidMSO_Pages,               JET_coltypLong,         JET_bitColumnNotNULL,

    //  unused by columns
    "Stats",                fidMSO_Stats,               JET_coltypBinary,       NO_GRBIT,


    //  set TableFlag to TRUE for tables, NULL otherwise
    "RootFlag",             fidMSO_RootFlag,            JET_coltypBit,          NO_GRBIT,

    //  name of the template table for table, callback name for callbacks and columns
    "TemplateTable",        fidMSO_TemplateTable,       JET_coltypText,         NO_GRBIT,
//  "Callback",             fidMSO_Callback,            JET_coltypText,         NO_GRBIT,

    //  COLUMN-specific items
    "RecordOffset",         fidMSO_RecordOffset,        JET_coltypShort,        NO_GRBIT,
    "DefaultValue",         fidMSO_DefaultValue,        JET_coltypBinary,       NO_GRBIT,

    //  INDEX-specific items
    //  max. # of fields in a key is (size of coltypBinary)/(size of JET_COLUMNID) = 255/8 = 31
    "KeyFldIDs",            fidMSO_KeyFldIDs,           JET_coltypBinary,       NO_GRBIT,
    "VarSegMac",            fidMSO_VarSegMac,           JET_coltypBinary,       NO_GRBIT,
    "ConditionalColumns",   fidMSO_ConditionalColumns,  JET_coltypBinary,       NO_GRBIT,
    "LCMapFlags",           fidMSO_LCMapFlags,          JET_coltypLong,         NO_GRBIT,
    "KeyMost",              fidMSO_KeyMost,             JET_coltypUnsignedShort,        NO_GRBIT,

    //  Variable columns

    "TupleLimits",          fidMSO_TupleLimits,         JET_coltypBinary,       NO_GRBIT,
    "Version",              fidMSO_Version,             JET_coltypBinary,       NO_GRBIT,
    "SortID",               fidMSO_SortID,              JET_coltypBinary,       NO_GRBIT,

    //  CALLBACK-related items
    //  These are used by columns with callbacks (user-defined default values)
    //  Added in database version 0x620,3
    "CallbackData",         fidMSO_CallbackData,        JET_coltypLongBinary,   JET_bitColumnTagged,
    "CallbackDependencies", fidMSO_CallbackDependencies,JET_coltypLongBinary,   JET_bitColumnTagged,

    //  Space hint-related items. 
    //  These are for the threshold for separating LVs, space hints for the B-Tree, and
    //  for the table deferred LV space hints.
    "SeparateLV",           fidMSO_SeparateLVThreshold, JET_coltypLongBinary,           JET_bitColumnTagged,
    "SpaceHints",           fidMSO_SpaceHints,          JET_coltypLongBinary,   JET_bitColumnTagged,
    "SpaceDeferredLVHints", fidMSO_SpaceLVDeferredHints, JET_coltypLongBinary,  JET_bitColumnTagged,

    //  Locale Name item used to store the locale for indices
    "LocaleName",           fidMSO_LocaleName,          JET_coltypLongBinary,   JET_bitColumnTagged,

    "LVChunkMax",           fidMSO_LVChunkMax,          JET_coltypLong,         NO_GRBIT,

    //  IMPORTANT: The entries here must match the same order as the iMSO_* constants in cat.hxx
};

LOCAL const ULONG   cColumnsMSO     = _countof(rgcdescMSO);

LOCAL const IDESC   rgidescMSO[]    =
{
    (CHAR *)szMSOIdIndex,           "+ObjidTable\0+Type\0+Id\0",    JET_bitIndexPrimary|JET_bitIndexDisallowNull,
    (CHAR *)szMSONameIndex,         "+ObjidTable\0+Type\0+Name\0",  JET_bitIndexUnique|JET_bitIndexDisallowNull,

    //  This index is only used to filter out duplicate table names (all root objects
    //  object -- currently only tables -- have the RootFlag field set, all non-root
    //  objects don't set it)
    (CHAR *)szMSORootObjectsIndex,  "+RootFlag\0+Name\0",           JET_bitIndexUnique|JET_bitIndexIgnoreFirstNull
};

LOCAL const ULONG   cIndexesMSO     = _countof(rgidescMSO);


LOCAL const ULONG   cbMaxDeletedColumnStubName  = 32;
LOCAL const CHAR    szDeletedColumnStubPrefix[] = "JetStub_";
