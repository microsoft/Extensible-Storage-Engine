// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

const CHAR  szMSU1[]                = "MSysUnicodeFixupVer1";
const CHAR  szMSU[]                 = "MSysUnicodeFixupVer2";
const CHAR  szMSO[]                 = "MSysObjects";
const CHAR  szMSOShadow[]           = "MSysObjectsShadow";
const CHAR  szMSOIdIndex[]          = "Id";
const CHAR  szMSONameIndex[]        = "Name";
const CHAR  szMSORootObjectsIndex[] = "RootObjects";

const CHAR  szMSObjids[]            = "MSysObjids";

const CHAR  szMSLocales[]           = "MSysLocales";
const WCHAR wszMSLocales[]          = L"MSysLocales";

const CHAR  szLVRoot[]              = "LV";
const ULONG cbLVRoot                = sizeof(szLVRoot) - 1;

const CHAR  szTableCallback[]       = "CB";
const ULONG cbTableCallback         = sizeof(szTableCallback) - 1;


struct CDESC
{
    CHAR                *szColName;
    COLUMNID            columnid;
    JET_COLTYP          coltyp;
    JET_GRBIT           grbit;
};

struct IDESC
{
    CHAR                *szIdxName;
    CHAR                *szIdxKeys;
    JET_GRBIT           grbit;
};


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

};

C_ASSERT( _countof( rgcolumnidOldMSO ) == 25 );

LOCAL const CDESC   rgcdescMSO[]    =
{
    "ObjidTable",       fidMSO_ObjidTable,      JET_coltypLong,         JET_bitColumnNotNULL,

    "Type",             fidMSO_Type,            JET_coltypShort,        JET_bitColumnNotNULL,

    "Id",               fidMSO_Id,              JET_coltypLong,         JET_bitColumnNotNULL,

    "Name",                 fidMSO_Name,                JET_coltypText,         JET_bitColumnNotNULL,

    "ColtypOrPgnoFDP",      fidMSO_Coltyp,              JET_coltypLong,         JET_bitColumnNotNULL,
    
    "SpaceUsage",           fidMSO_SpaceUsage,          JET_coltypLong,         JET_bitColumnNotNULL,
    
    "Flags",                fidMSO_Flags,               JET_coltypLong,         JET_bitColumnNotNULL,

    "PagesOrLocale",        fidMSO_Pages,               JET_coltypLong,         JET_bitColumnNotNULL,

    "Stats",                fidMSO_Stats,               JET_coltypBinary,       NO_GRBIT,


    "RootFlag",             fidMSO_RootFlag,            JET_coltypBit,          NO_GRBIT,

    "TemplateTable",        fidMSO_TemplateTable,       JET_coltypText,         NO_GRBIT,

    "RecordOffset",         fidMSO_RecordOffset,        JET_coltypShort,        NO_GRBIT,
    "DefaultValue",         fidMSO_DefaultValue,        JET_coltypBinary,       NO_GRBIT,

    "KeyFldIDs",            fidMSO_KeyFldIDs,           JET_coltypBinary,       NO_GRBIT,
    "VarSegMac",            fidMSO_VarSegMac,           JET_coltypBinary,       NO_GRBIT,
    "ConditionalColumns",   fidMSO_ConditionalColumns,  JET_coltypBinary,       NO_GRBIT,
    "LCMapFlags",           fidMSO_LCMapFlags,          JET_coltypLong,         NO_GRBIT,
    "KeyMost",              fidMSO_KeyMost,             JET_coltypUnsignedShort,        NO_GRBIT,


    "TupleLimits",          fidMSO_TupleLimits,         JET_coltypBinary,       NO_GRBIT,
    "Version",              fidMSO_Version,             JET_coltypBinary,       NO_GRBIT,
    "SortID",               fidMSO_SortID,              JET_coltypBinary,       NO_GRBIT,

    "CallbackData",         fidMSO_CallbackData,        JET_coltypLongBinary,   JET_bitColumnTagged,
    "CallbackDependencies", fidMSO_CallbackDependencies,JET_coltypLongBinary,   JET_bitColumnTagged,

    "SeparateLV",           fidMSO_SeparateLVThreshold, JET_coltypLongBinary,           JET_bitColumnTagged,
    "SpaceHints",           fidMSO_SpaceHints,          JET_coltypLongBinary,   JET_bitColumnTagged,
    "SpaceDeferredLVHints", fidMSO_SpaceLVDeferredHints, JET_coltypLongBinary,  JET_bitColumnTagged,

    "LocaleName",           fidMSO_LocaleName,          JET_coltypLongBinary,   JET_bitColumnTagged,

    "LVChunkMax",           fidMSO_LVChunkMax,          JET_coltypLong,         NO_GRBIT,

};

LOCAL const ULONG   cColumnsMSO     = _countof(rgcdescMSO);

LOCAL const IDESC   rgidescMSO[]    =
{
    (CHAR *)szMSOIdIndex,           "+ObjidTable\0+Type\0+Id\0",    JET_bitIndexPrimary|JET_bitIndexDisallowNull,
    (CHAR *)szMSONameIndex,         "+ObjidTable\0+Type\0+Name\0",  JET_bitIndexUnique|JET_bitIndexDisallowNull,

    (CHAR *)szMSORootObjectsIndex,  "+RootFlag\0+Name\0",           JET_bitIndexUnique|JET_bitIndexIgnoreFirstNull
};

LOCAL const ULONG   cIndexesMSO     = _countof(rgidescMSO);


LOCAL const ULONG   cbMaxDeletedColumnStubName  = 32;
LOCAL const CHAR    szDeletedColumnStubPrefix[] = "JetStub_";
