// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace Microsoft
{
#if (ESENT)
namespace Windows
#else
namespace Exchange
#endif
{
namespace Isam
{

MSINTERNAL enum class MJET_PREP
{
    Insert          = 0,
    Replace         = 2,
    Cancel          = 3,
    ReplaceNoLock   = 4,
    InsertCopy      = 5,
    InsertCopyWithoutSLVColumns = 6,
    InsertCopyDeleteOriginal    = 7,
    ReadOnlyCopy    = 8,
};

public value struct MJET_INSTANCE
{
internal:
    System::Int64   NativeValue;

public:
    virtual String ^ ToString() override
        {
        return String::Format( L"MJET_INSTANCE(NativeValue = {0})", NativeValue );
        }

    bool Equals(MJET_INSTANCE rhs)
        {
        return (NativeValue == rhs.NativeValue);
        }
    
    virtual bool Equals(System::Object ^o) override
        {
        MJET_INSTANCE^ inst = safe_cast<MJET_INSTANCE^>( o );
        if(o == nullptr)
            return false;
        return (NativeValue == inst->NativeValue);
        }

};

MSINTERNAL value struct MJET_SESID
{
internal:
    MJET_INSTANCE   Instance;
    System::Int64   NativeValue;

public:
    virtual String ^ ToString() override
        {
        return String::Format( L"MJET_SESID(NativeValue = {0})", NativeValue );
        }
};

MSINTERNAL value struct MJET_DBID
{
internal:
    System::Int64   NativeValue;

public:
    virtual String ^ ToString() override
        {
        return String::Format( L"MJET_DBID(NativeValue = {0})", NativeValue );
        }
};

MSINTERNAL value struct MJET_TABLEID
{
internal:
    MJET_SESID      Sesid;
    MJET_DBID       Dbid;
    System::Int64   NativeValue;

public:
    virtual String ^ ToString() override
        {
        return String::Format( L"MJET_TABLEID(Sesid = {0}, Dbid = {1}, NativeValue = {2})", Sesid, Dbid, NativeValue );
        }
};

MSINTERNAL value struct MJET_COLUMNID
{
internal:
    MJET_COLTYP     Coltyp;
    System::Boolean isASCII;
    System::Int64   NativeValue;

public:

    property System::Int64 Value
        {
        System::Int64 get( )
            {
            return NativeValue;
            }
        }

    property MJET_COLTYP Type
        {
        MJET_COLTYP get ( )
            {
            return Coltyp;
            }
        }

    property System::Boolean IsASCII
        {
        System::Boolean get( )
            {
            return isASCII;
            }
        }

    virtual String ^ ToString() override
        {
        return String::Format( L"MJET_COLUMNID(Coltyp = {0}, NativeValue = {1}, isASCII = {2})",
            Coltyp , NativeValue , isASCII );
        }
};

MSINTERNAL value struct MJET_COLUMNDEF
{
    MJET_COLUMNID   Columnid;
    MJET_COLTYP     Coltyp;
    System::Int32   Langid;
    System::Boolean ASCII;
    System::Int32   MaxLength;
    MJET_GRBIT      Grbit;

    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams = {
            Columnid ,
            Coltyp ,
            Langid ,
            ASCII ,
            MaxLength ,
            Grbit };
        return String::Format(
            L"MJET_COLUMNDEF("
            L"Columnid = {0}, "
            L"Coltyp = {1},"
            L"Langid = {2},"
            L"ASCII = {3},"
            L"MaxLength = {4},"
            L"Grbit = {5})",
            TempParams );
        }
};

MSINTERNAL value struct MJET_SETINFO
{
    System::Int64   IbLongValue;
    System::Int64   ItagSequence;

    virtual String ^ ToString() override
        {
        return String::Format(
            L"MJET_SETINFO("
            L"IbLongValue = {0},"
            L"ItagSequence = {1})",
            IbLongValue ,
            ItagSequence );
        }
    };

MSINTERNAL value struct MJET_RETINFO
{
    System::Int64   IbLongValue;
    System::Int64   ItagSequence;

    virtual String ^ ToString() override
        {
        return String::Format(
            L"MJET_RETINFO("
            L"IbLongValue = {0},"
            L"ItagSequence = {1})",
            IbLongValue ,
            ItagSequence );
        }
};

MSINTERNAL value struct MJET_RECPOS
{
    System::Int64   EntriesLessThan;
    System::Int64   TotalEntries;

    virtual String ^ ToString() override
        {
        return String::Format(
            L"MJET_RECPOS("
            L"EntriesLessThan = {0},"
            L"TotalEntries = {1})",
            EntriesLessThan ,
            TotalEntries );
        }
};

MSINTERNAL value struct MJET_RSTMAP
{
    System::String ^ DatabaseName;
    System::String ^ NewDatabaseName;

    virtual String ^ ToString() override
        {
        return String::Format(
            L"MJET_RSTMAP("
            L"DatabaseName = {0},"
            L"NewDatabaseName = {1})",
            DatabaseName,
            NewDatabaseName );
        }
    };

MSINTERNAL value struct MJET_COLUMNBASE
{
    MJET_COLUMNID   Columnid;
    MJET_COLTYP     Coltyp;
    System::Int32   Langid;
    System::Boolean ASCII;
    System::Int64   MaxLength;
    MJET_GRBIT      Grbit;
    System::String ^    TableName;
    System::String ^    ColumnName;

    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams =    {
            Columnid ,
            Coltyp ,
            Langid ,
            ASCII ,
            MaxLength ,
            Grbit ,
            TableName,
            ColumnName };
        return String::Format(
            L"MJET_COLUMNBASE("
            L"Columnid = {0}, "
            L"Coltyp = {1},"
            L"Langid = {2},"
            L"ASCII = {3},"
            L"MaxLength = {4},"
            L"Grbit = {5},"
            L"TableName = {6},"
            L"ColumnName = {7})",
            TempParams );
        }
};

MSINTERNAL value struct MJET_COLUMNLIST
{
    MJET_TABLEID        Tableid;
    System::Int64   Records;
    MJET_COLUMNID   ColumnidColumnName;
    MJET_COLUMNID   ColumnidMaxLength;
    MJET_COLUMNID   ColumnidGrbit;
    MJET_COLUMNID   ColumnidDefault;
    MJET_COLUMNID   ColumnidTableName;

    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams =    {
                Tableid ,
                Records ,
                ColumnidColumnName ,
                ColumnidMaxLength ,
                ColumnidGrbit ,
                ColumnidDefault ,
                ColumnidTableName };
        return String::Format(
                L"MJET_COLUMNLIST("
                L"Tableid = {0},"
                L"Records = {1},"
                L"ColumnidColumnName = {2},"
                L"ColumnidMaxLength = {3},"
                L"ColumnidGrbit = {4},"
                L"ColumnidDefault = {5},"
                L"ColumnidTableName = {6})",
                TempParams );
        }

};

MSINTERNAL value struct MJET_SECONDARYINDEXBOOKMARK
{
    array<System::Byte>^    SecondaryBookmark;
    array<System::Byte>^    PrimaryBookmark;
};

MSINTERNAL value struct MJET_CONDITIONALCOLUMN
{
    System::String^ ColumnName;
    MJET_GRBIT      Grbit;

    virtual String ^ ToString( void ) override
        {
        return String::Format(
            L"MJET_CONDITIONALCOLUMN("
            L"ColumnName = {0}, "
            L"Grbit = {1})",
            ColumnName,
            Grbit );
        }
};

MSINTERNAL value struct MJET_UNICODEINDEX
{
    System::Int32       Lcid;
    System::Int32       MapFlags;

    virtual String ^ ToString( void ) override
        {
        return String::Format(
            L"MJET_UNICODEINDEX("
            L"Lcid = {0}, "
            L"MapFlags = {1})",
            Lcid ,
            MapFlags );
        }
};

MSINTERNAL value struct MJET_TUPLELIMITS
{
    System::Int32       LengthMin;
    System::Int32       LengthMax;
    System::Int32       ToIndexMax;
    System::Int32       OffsetIncrement;
    System::Int32       OffsetStart;

    virtual String ^ ToString( void ) override
        {
        return String::Format(
            L"MJET_TUPLELIMITS("
            L"LengthMin = {0}, "
            L"LengthMax = {1}, "
            L"ToIndexMax = {2}, "
            L"OffsetIncrement = {3}, "
            L"OffsetStart = {4})",
            LengthMin ,
            LengthMax ,
            ToIndexMax,
            OffsetIncrement,
            OffsetStart
             );
        }
};

MSINTERNAL value struct MJET_SPACEHINTS
{
    System::Int32       InitialDensity;
    System::Int32       InitialSize;
    MJET_GRBIT          Grbit;
    System::Int32       MaintenanceDensity;
    System::Int32       GrowthRate;
    System::Int32       MinimumExtentSize;
    System::Int32       MaximumExtentSize;

    virtual String ^ ToString( void ) override
        {
        return String::Format(
            L"MJET_SPACEHINTS("
            L"InitialDensity = {0}, "
            L"InitialSize = {1}, "
            L"Grbit = {2}, "
            L"MaintenanceDensity = {3}, "
            L"GrowthRate = {4}, "
            L"MinimumExtentSize = {5}, "
            L"MaximumExtentSize = {6})",
            InitialDensity,
            InitialSize,
            Grbit,
            MaintenanceDensity,
            GrowthRate,
            MinimumExtentSize,
            MaximumExtentSize );
        }
};

MSINTERNAL value struct MJET_INDEXCREATE
{
    System::String ^    IndexName;
    System::String ^    Key;

    MJET_GRBIT      Grbit;
    System::Int32       Density;

    System::Int32       Lcid;
    System::Int32       MapFlags;

    System::Int32       VarSegMac;
    System::Int32       LengthMin;
    System::Int32       LengthMax;
    System::Int32       ToIndexMax;

    array<MJET_CONDITIONALCOLUMN>^  ConditionalColumns;

    System::Int32       KeyLengthMax;

    MJET_SPACEHINTS ^   SpaceHints;

    MJET_TUPLELIMITS ^  TupleLimits;

    virtual String ^ ToString( void ) override
        {
        array<System::Object^>^ TempParams =    {
            IndexName,
            Key,
            Grbit ,
            Density ,
            Lcid ,
            MapFlags ,
            VarSegMac ,
            LengthMin ,
            LengthMax ,
            ToIndexMax ,
            ConditionalColumns,
            KeyLengthMax,
            SpaceHints,
            TupleLimits };
        return String::Format(
            L"MJET_INDEXCREATE("
            L"IndexName = {0}, "
            L"Key = {1},"
            L"Grbit = {2},"
            L"Density = {3},"
            L"Lcid = {4},"
            L"MapFlags = {5},"
            L"cbVarSegMac = {6},"
            L"LengthMin = {7},"
            L"LengthMax = {8},"
            L"ToIndexMax = {9},"
            L"ConditionalColumns = {10},"
            L"KeyLengthMax = {11}, "
            L"SpaceHints = {12}, "
            L"TupleLimits = {13})",
            TempParams );
        }
};

MSINTERNAL value struct MJET_COLUMNCREATE
{
    System::String ^    ColumnName;
    MJET_COLTYP     Coltyp;
    System::Int32       MaxLength;
    MJET_GRBIT      Grbit;

    array<System::Byte>^        DefaultValue;
    System::Int32       Cp;


    virtual String ^ ToString( void ) override
        {
        array<System::Object^>^ TempParams =    {
            ColumnName,
            Coltyp ,
            MaxLength ,
            Grbit ,
            DefaultValue,
            Cp };
        return String::Format(
            L"MJET_COLUMNCREATE("
            L"ColumnName = {0}, "
            L"Coltyp = {1},"
            L"MaxLength = {2},"
            L"Grbit = {3},"
            L"DefaultValue = {4},"
            L"Cp = {5})",
            TempParams );
        }
};

MSINTERNAL value struct MJET_TABLECREATE
{
    System::String ^    TableName;
    System::String ^    TemplateTableName;

    System::Int32       Pages;
    System::Int32       Density;
    System::Int32       SeparateLVSize;

    array<MJET_COLUMNCREATE>^   ColumnCreates;
    array<MJET_INDEXCREATE>^        IndexCreates;

    MJET_GRBIT      Grbit;

    MJET_SPACEHINTS ^   SequentialSpaceHints;
    MJET_SPACEHINTS ^   LongValueSpaceHints;

    virtual String ^ ToString( void ) override
        {
        array<System::Object^>^ TempParams =    {
            TableName,
            TemplateTableName,
            Pages ,
            Density ,
            SeparateLVSize,
            ColumnCreates,
            IndexCreates,
            Grbit,
            SequentialSpaceHints,
            LongValueSpaceHints };
        return String::Format(
            L"MJET_TABLECREATE2("
            L"TableName = {0}, "
            L"TemplateTableName = {1},"
            L"Pages = {2},"
            L"Density = {3},"
            L"SeparateLVSize = {4}, "
            L"ColumnCreate = {5},"
            L"IndexCreate = {6},"
            L"Grbit = {7}, "
            L"SequentialSpaceHints = {8}, "
            L"LongValueSpaceHints = {9})",
            TempParams );
        }
};

MSINTERNAL value struct MJET_ENUMCOLUMNID
{
    MJET_COLUMNID           Columnid;
    array<System::Int32>^           itagSequences;

    virtual String ^ ToString() override
        {
        return String::Format(
            L"MJET_ENUMCOLUMNID("
            L"Columnid = {0}, "
            L"itagSequences = {1})",
            Columnid ,
            itagSequences );
        }
};

MSINTERNAL value struct MJET_ENUMCOLUMNVALUE
{
    System::Int32           itagSequence;
    array<System::Byte>^            data;

    virtual String ^ ToString() override
        {
        return String::Format(
            L"MJET_ENUMCOLUMNVALUE("
            L"itagSequence = {0}, "
            L"data = {1})",
            itagSequence ,
            data );
        }
};

MSINTERNAL value struct MJET_ENUMCOLUMN
{
    MJET_COLUMNID           Columnid;
    System::String^         ColumnName;
    array<MJET_ENUMCOLUMNVALUE>^        ColumnValues;

    virtual String ^ ToString() override
        {
        return String::Format(
            L"MJET_ENUMCOLUMN("
            L"Columnid = {0}, "
            L"ColumnName = {1}, "
            L"ColumnValues = {2})",
            Columnid ,
            ColumnName,
            ColumnValues );
        }
};

MSINTERNAL value struct MJET_INDEXLIST
{
    MJET_TABLEID        Tableid;
    System::Int64   Records;
    MJET_COLUMNID   ColumnidIndexName;
    MJET_COLUMNID   ColumnidGrbitIndex;
    MJET_COLUMNID   ColumnidCColumn;
    MJET_COLUMNID   ColumnidIColumn;
    MJET_COLUMNID   ColumnidGrbitColumn;
    MJET_COLUMNID   ColumnidColumnName;
    MJET_COLUMNID   ColumnidLCMapFlags;

    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams =    {
            Tableid ,
            Records ,
            ColumnidIndexName ,
            ColumnidGrbitIndex ,
            ColumnidCColumn ,
            ColumnidIColumn ,
            ColumnidGrbitColumn ,
            ColumnidColumnName ,
            ColumnidLCMapFlags };
        return String::Format(
                L"MJET_INDEXLIST("
                L"Tableid = {0},"
                L"Records = {1},"
                L"ColumnidIndexName = {2},"
                L"ColumnidGrbitIndex = {3},"
                L"ColumnidCColumn = {4},"
                L"ColumnidIColumn = {5},"
                L"ColumnidGrbitColumn = {6},"
                L"ColumnidColumnName = {7},"
                L"ColumnidLCMapFlags = {8})",
                TempParams );
        }
};

MSINTERNAL value struct MJET_OBJECTINFO
{
    MJET_OBJTYP     Objtyp;
    MJET_GRBIT      Grbit;
    MJET_GRBIT      Flags;

    virtual String ^ ToString() override
        {
        return String::Format(
                L"MJET_OBJECTINFO("
                L"Objtyp = {0},"
                L"Grbit = {1},"
                L"Flags = {2})",
                Objtyp ,
                Grbit ,
                Flags );
        }
};

MSINTERNAL value struct MJET_OBJECTLIST
{
    MJET_TABLEID        Tableid;
    System::Int64   Records;
    MJET_COLUMNID   ColumnidContainerName;
    MJET_COLUMNID   ColumnidObjectName;
    MJET_COLUMNID   ColumnidObjectType;
    MJET_COLUMNID   ColumnidGrbit;
    MJET_COLUMNID   ColumnidFlags;

    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams =    {
            Tableid ,
            Records ,
            ColumnidContainerName ,
            ColumnidObjectName ,
            ColumnidObjectType ,
            ColumnidGrbit ,
            ColumnidFlags };
        return String::Format(
                L"MJET_OBJECTLIST("
                L"Tableid = {0},"
                L"Records = {1},"
                L"ColumnidContainerName = {2},"
                L"ColumnidObjectName = {3},"
                L"ColumnidObjectType = {4},"
                L"ColumnidGrbit = {5},"
                L"ColumnidFlags = {6})",
                TempParams );
        }
};

MSINTERNAL value struct MJET_RECSIZE
{
    System::Int64   Data;
    System::Int64   LongValueData;
    System::Int64   Overhead;
    System::Int64   LongValueOverhead;
    System::Int64   NonTaggedColumns;
    System::Int64   TaggedColumns;
    System::Int64   LongValues;
    System::Int64   MultiValues;
    System::Int64   CompressedColumns;
    System::Int64   DataCompressed;
    System::Int64   LongValueDataCompressed;

    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams =    {
            (Data),
            (LongValueData),
            (Overhead),
            (LongValueOverhead),
            (NonTaggedColumns),
            (TaggedColumns),
            (LongValues),
            (MultiValues),
            (CompressedColumns),
            (DataCompressed),
            (LongValueDataCompressed)
            };
        return String::Format(
            L"MJET_RECSIZE("
            L"Data = {0},"
            L"LongValueData = {1},"
            L"Overhead = {2},"
            L"LongValueOverhead = {3},"
            L"NonTaggedColumns = {4},"
            L"TaggedColumns = {5},"
            L"LongValues = {6},"
            L"MultiValues = {7},"
            L"CompressedColumns = {8},"
            L"DataCompressed = {9},"
            L"LongValueDataCompressed = {10})",
            TempParams );
        }
};

MSINTERNAL value struct MJET_PAGEINFO
{
    System::Int64   PageNumber;

    System::Boolean IsInitialized;

    System::Boolean IsCorrectableError;

    array<System::Byte>^  ActualChecksum;

    array<System::Byte>^  ExpectedChecksum;

    System::UInt64  Dbtime;

    System::UInt64  StructureChecksum;

    System::Int64   Flags;

    virtual String ^ ToString() override
        {
        array<System::String^>^ actualChecksums = gcnew array<System::String^>( 4 );
        array<System::String^>^ expectedChecksums = gcnew array<System::String^>( 4 );

        for ( int i = 0; i < actualChecksums->Length; ++i )
            {
            actualChecksums[ i ] = System::String::Format( "0x{0:x}", System::BitConverter::ToUInt64( ActualChecksum, i * sizeof( actualChecksums[ 0 ] ) ) );
            }
        for ( int i = 0; i < expectedChecksums->Length; ++i )
            {
            expectedChecksums[ i ] = System::String::Format( "0x{0:x}", System::BitConverter::ToUInt64( ExpectedChecksum, i * sizeof( expectedChecksums[ 0 ] ) ) );
            }

        return String::Format(
            L"MJET_PAGEINFO("
            L"PageNumber={0},"
            L"IsInitialized={1},"
            L"IsCorrectableError={2},"
            L"ActualChecksum=[0x{3}],"
            L"ExpectedChecksum=[0x{4}],"
            L"Dbtime={5},"
            L"StructureChecksum=0x{6:x})",
            PageNumber,
            IsInitialized,
            IsCorrectableError,
            System::String::Join( ",", actualChecksums ),
            System::String::Join( ",", expectedChecksums ),
            Dbtime,
            StructureChecksum
            );
        }
};

MSINTERNAL value struct MJET_SIGNATURE
{
    System::Int64       Random;

    System::UInt64      CreationTimeUInt64;

    System::DateTime    CreationTime;

    virtual String ^ ToString() override
        {
        return String::Format(
                "MJET_SIGNATURE(Random = {0},CreationTime = {1},CreationTimeUint64 = 0x{2})",
                Random, CreationTime, CreationTimeUInt64.ToString("X") );
        }
};

MSINTERNAL value struct MJET_LOGINFOMISC
{
    System::Int64       Generation;

    MJET_SIGNATURE      Signature;

    System::DateTime    CreationTime;

    System::DateTime    PreviousGenerationCreationTime;

    System::Int64       Flags;

    System::Int64       VersionMajor;

    System::Int64       VersionMinor;

    System::Int64       VersionUpdate;

    System::Int64       SectorSize;

    System::Int64       HeaderSize;

    System::Int64       FileSize;

    System::Int64       DatabasePageSize;

    System::Int64       CheckpointGeneration;

    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams =    {
            Generation,
            Signature,
            CreationTime,
            PreviousGenerationCreationTime,
            Flags,
            VersionMajor,
            VersionMinor,
            SectorSize,
            HeaderSize,
            FileSize,
            DatabasePageSize,
            CheckpointGeneration
            };
        return String::Format(
                L"MJET_LOGINFOMISC("
                L"Generation = {0},"
                L"Signature = {1},"
                L"CreationTime = {2},"
                L"PreviousGenerationCreationTime = {3},"
                L"Flags = {4:X},"
                L"VersionMajor = {5},"
                L"VersionMinor = {6},"
                L"SectorSize = {7},"
                L"HeaderSize = {8},"
                L"FileSize = {9},"
                L"DatabasePageSize = {10}"
                L"CheckpointGeneration = {11}"
                L")",
                TempParams );
        }

};

MSINTERNAL value struct MJET_LGPOS
{
    System::Int64   Generation;

    System::Int32   Sector;

    System::Int32   ByteOffset;

    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams =    {
            Generation,
            Sector,
            ByteOffset,
            };
        return String::Format(
                L"MJET_LGPOS("
                L"Generation = {0},"
                L"Sector = {1},"
                L"ByteOffset = {2}"
                L")",
                TempParams );
        }
};

MSINTERNAL enum class MJET_DBSTATE
{
    JustCreated = 1,

    DirtyShutdown = 2,

    CleanShutdown = 3,

    BeingConverted = 4,

    ForceDetach = 5,

    IncrementalReseedInProgress = 6,

    DirtyAndPatchedShutdown = 7,

    Unknown = 99,
};

MSINTERNAL value struct MJET_BKINFO
{
    MJET_LGPOS          Lgpos;

    System::DateTime    BackupTime;

    System::Boolean     SnapshotBackup;

    System::Int64       LowestGeneration;

    System::Int64       HighestGeneration;

    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams =    {
            Lgpos,
            BackupTime,
            SnapshotBackup,
            LowestGeneration,
            HighestGeneration
            };
        return String::Format(
                L"MJET_BKINFO("
                L"Lgpos = {0},"
                L"BackupTime = {1},"
                L"SnapshotBackup = {2},"
                L"LowestGeneration = {3},"
                L"HighestGeneration = {4}"
                L")",
                TempParams );
        }
};

MSINTERNAL value struct MJET_DBINFO
{
    System::Int64   Version;

    System::Int64   Update;

    MJET_SIGNATURE  Signature;

    MJET_DBSTATE        State;

    MJET_LGPOS      ConsistentLgpos;

    System::DateTime    ConsistentTime;

    MJET_LGPOS      AttachLgpos;

    System::DateTime    AttachTime;

    MJET_LGPOS      DetachLgpos;

    System::DateTime    DetachTime;

    MJET_SIGNATURE  LogSignature;

    MJET_BKINFO     LastFullBackup;

    MJET_BKINFO     LastIncrementalBackup;

    MJET_BKINFO     CurrentFullBackup;

    System::Boolean ShadowCatalogDisabled;

    System::Boolean UpgradeDatabase;

    System::Int64   OSMajorVersion;

    System::Int64   OSMinorVersion;

    System::Int64   OSBuild;

    System::Int64   OSServicePackNumber;

    System::Int32   DatabasePageSize;

    System::Int64   MinimumLogGenerationRequired;

    System::Int64   MaximumLogGenerationRequired;

    System::DateTime MaximumLogGenerationCreationTime;

    System::Int64   RepairCount;

    System::DateTime    LastRepairTime;

    System::Int64   RepairCountOld;

    System::Int64   SuccessfulECCPageFixes;

    System::DateTime LastSuccessfulECCPageFix;

    System::Int64   SuccessfulECCPageFixesOld;

    System::Int64   UnsuccessfulECCPageFixes;

    System::DateTime LastUnsuccessfulECCPageFix;

    System::Int64   UnsuccessfulECCPageFixesOld;

    System::Int64   BadChecksums;

    System::DateTime LastBadChecksum;

    System::Int64   BadChecksumsOld;

    System::Int64   CommittedGeneration;

    MJET_BKINFO     LastCopyBackup;

    MJET_BKINFO     LastDifferentialBackup;

    System::Int64   IncrementalReseedCount;

    System::DateTime    LastIncrementalReseedTime;

    System::Int64   IncrementalReseedCountOld;

    System::Int64   PagePatchCount;

    System::DateTime    LastPagePatchTime;

    System::Int64   PagePatchCountOld;

    System::DateTime    LastChecksumTime;

    System::DateTime    ChecksumStartTime;

    System::Int64 PageCheckedCount;

    MJET_LGPOS      LastReAttachLgpos;

    System::DateTime    LastReAttachTime;

    MJET_SIGNATURE DbHeaderFlushSignature;

    MJET_SIGNATURE FlushMapHeaderFlushSignature;

    System::Int64 MinimumLogGenerationConsistent;

    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams =    {
            Version,
            Update,
            Signature,
            State,
            ConsistentLgpos,
            ConsistentTime,
            AttachLgpos,
            AttachTime,
            DetachLgpos,
            DetachTime,
            LogSignature,
            LastFullBackup,
            LastIncrementalBackup,
            CurrentFullBackup,
            ShadowCatalogDisabled,
            UpgradeDatabase,
            OSMajorVersion,
            OSMinorVersion,
            OSBuild,
            OSServicePackNumber,
            DatabasePageSize,
            MinimumLogGenerationRequired,
            MaximumLogGenerationRequired,
            MaximumLogGenerationCreationTime,
            RepairCount,
            LastRepairTime,
            RepairCountOld,
            SuccessfulECCPageFixes,
            LastSuccessfulECCPageFix,
            SuccessfulECCPageFixesOld,
            UnsuccessfulECCPageFixes,
            LastUnsuccessfulECCPageFix,
            UnsuccessfulECCPageFixesOld,
            BadChecksums,
            LastBadChecksum,
            BadChecksumsOld,
            CommittedGeneration,
            LastCopyBackup,
            LastDifferentialBackup,
            IncrementalReseedCount,
            LastIncrementalReseedTime,
            IncrementalReseedCountOld,
            PagePatchCount,
            LastPagePatchTime,
            PagePatchCountOld,
            LastChecksumTime,
            ChecksumStartTime,
            PageCheckedCount,
            LastReAttachLgpos,
            LastReAttachTime,
            DbHeaderFlushSignature,
            FlushMapHeaderFlushSignature,
            MinimumLogGenerationConsistent,
            };
        return String::Format(
                L"MJET_DBINFO("
                L"Version = {0},"
                L"Update = {1},"
                L"Signature = {2},"
                L"State = {3},"
                L"ConsistentLgpos = {4},"
                L"ConsistentTime = {5},"
                L"AttachLgpos = {6},"
                L"AttachTime = {7},"
                L"DetachLgpos = {8},"
                L"DetachTime = {9},"
                L"LogSignature = {10},"
                L"LastFullBackup = {11},"
                L"LastIncrementalBackup = {12},"
                L"CurrentFullBackup = {13},"
                L"ShadowCatalogDisables = {14},"
                L"UpgradeDatabase = {15},"
                L"OSMajorVersion = {16},"
                L"OSMinorVersion = {17},"
                L"OSBuild = {18},"
                L"OSServicePackNumber = {19},"
                L"DatabasePageSize = {20},"
                L"MinimumLogGenerationRequired = {21},"
                L"MaximumLogGenerationRequired = {22},"
                L"MaximumLogGenerationCreationTime = {23},"
                L"RepairCount = {24},"
                L"LastRepairTime = {25},"
                L"RepairCountOld = {26},"
                L"SuccessfulECCPageFixes = {27},"
                L"LastSuccessfulECCPageFix = {28},"
                L"SuccessfulECCPageFixesOld = {29},"
                L"UnsuccessfulECCPageFixes = {30},"
                L"LastUnsuccessfulECCPageFix = {31},"
                L"UnsuccessfulECCPageFixesOld = {32},"
                L"BadChecksums = {33},"
                L"LastBadChecksum = {34},"
                L"BadChecksumsOld = {35},"
                L"CommittedGeneration = {36},"
                L"LastCopyBackup = {37},"
                L"LastDifferentialBackup = {38},"
                L"IncrementalReseedCount = {39},"
                L"LastIncrementalReseedTime = {40},"
                L"IncrementalReseedCountOld = {41},"
                L"PagePatchCount = {42},"
                L"LastPagePatchTime = {43},"
                L"PagePatchCountOld = {44},"
                L"LastChecksumTime = {45},"
                L"ChecksumStartTime = {46},"
                L"PageCheckedCount = {47},"
                L"LastReAttachLgpos = {48},"
                L"LastReAttachTime = {49},"
                L"DbHeaderFlushSignature = {50},"
                L"FlushMapHeaderFlushSignature = {51},"
                L"MinimumLogGenerationConsistent = {52},"
                L")",
                TempParams );
        }

};

MSINTERNAL value struct MJET_INSTANCE_INFO
{
    MJET_INSTANCE           Instance;

    System::String^         InstanceName;

    array<System::String^>^ DatabaseFileName;

    array<System::String^>^ DatabaseDisplayName;

    virtual String ^ ToString() override
        {
        return String::Format(
            L"MJET_INSTANCE_INFO("
            L"Instance = {0},"
            L"InstanceName = {1},"
            L"DatabaseFileName = {2}{3},"
            L"DatabaseDisplayName = {4}{5}"
            L")",
            Instance,
            InstanceName ,
            DatabaseFileName,
            DatabaseFileName != nullptr ? String::Format( "({0})", DatabaseFileName->Length ) : "",
            DatabaseDisplayName,
            DatabaseDisplayName != nullptr ? String::Format( "({0})", DatabaseDisplayName->Length ) : ""
            );
        }
};

MSINTERNAL value struct MJET_SESSIONINFO
{
    System::Int64   TrxBegin0;
    System::Int64   TrxLevel;
    System::Int64   Procid;
    System::Int64   Flags;
    System::IntPtr  TrxContext;

    virtual String ^ ToString() override
        {
        return String::Format(
            L"MJET_SESSIONINFO("
            L"TrxBegin0 = {0},"
            L"TrxLevel = {1},"
            L"Procid = {2},"
            L"Flags = {3},"
            L"TrxContext = {4}"
            L")",
            TrxBegin0,
            TrxLevel,
            Procid,
            Flags,
            TrxContext
            );
        }
    
};

MSINTERNAL enum class MJET_DBUTIL_OPERATION
{
    Consistency             = 0,
    DumpData                = 1,
    DumpMetaData            = 2,
    DumpPage                = 3,
    DumpNode                = 4,
    DumpSpace               = 5,
    SetHeaderState          = 6,
    DumpHeader              = 7,
    DumpLogfile             = 8,
    DumpLogfileTrackNode    = 9,
    DumpCheckpoint          = 10,
    EDBDump                 = 11,
    EDBRepair               = 12,
    Munge                   = 13,
    EDBScrub                = 14,
    SLVMove                 = 15,
    DBConvertRecords        = 16,
    DBDefragment            = 17,
    DumpExchangeSLVInfo     = 18,
    DumpPageUsage           = 20,
};

MSINTERNAL enum class MJET_EDBDUMP_OPERATION
{
    Tables      = 0,
    Indexes     = 1,
    Columns     = 2,
    Callbacks   = 3,
    Page        = 4,
};

MSINTERNAL value struct MJET_DBUTIL
{
    MJET_SESID              Sesid;
    MJET_DBID               Dbid;
    MJET_TABLEID            Tableid;

    MJET_DBUTIL_OPERATION   Operation;
    MJET_EDBDUMP_OPERATION  EDBDumpOperation;
    MJET_GRBIT              Grbit;

    System::String ^        DatabaseName;
    System::String ^        SLVName;
    System::String ^        BackupName;
    System::String ^        TableName;
    System::String ^        IndexName;
    System::String ^        IntegPrefix;

    System::Int64           PageNumber;
    System::Int32           Line;

    System::Int64           Generation;
    System::Int32           Sector;
    System::Int32           SectorOffset;

    System::Int32           RetryCount;


    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams =    {
            Sesid,
            Dbid,
            Tableid,
            Operation,
            EDBDumpOperation,
            Grbit,
            DatabaseName,
            SLVName,
            BackupName,
            TableName,
            IndexName,
            IntegPrefix,
            PageNumber,
            Line,
            Generation,
            Sector,
            SectorOffset,
            RetryCount };
        return String::Format(
            L"MJET_DBUTIL("
            L"Sesid = {0}"
            L"Dbid = {1}"
            L"Tableid = {2}"
            L"Operation = {3}"
            L"EDBDumpOperation = {4}"
            L"Grbit = {5}"
            L"DatabaseName = {6}"
            L"SLVName = {7}"
            L"BackupName = {8}"
            L"TableName = {9}"
            L"IndexName = {10}"
            L"IntegPrefix = {11}"
            L"PageNumber = {12}"
            L"Line = {13}"
            L"Generation = {14}"
            L"Sector = {15}"
            L"SectorOffset = {16}"
            L"RetryCount = {17})",
            TempParams );
        }
};

MSINTERNAL value struct MJET_EMITDATACTX
{
    System::Int32               Version;
    System::Int64               SequenceNum;
    MJET_GRBIT                  GrbitOperationalFlags;
    DateTime                    EmitTime;
    MJET_LGPOS                  LogDataLgpos;
    unsigned long               LogDataSize;

    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams =    {
            Version,
            GrbitOperationalFlags,
            SequenceNum,
            LogDataLgpos.ToString(),
            LogDataSize };
        return String::Format(
            L"MJET_EMITDATACTX("
            L"Version = {0}"
            L"GrbitOperationalFlags = {1}"
            L"SequenceNum = {2}"
            L"LogDataPos = {3}"
            L"LogDataSize = {4}",
            TempParams );
        }
};

MSINTERNAL enum class MJET_SNT
{
    Progress                = 0,
    Fail                    = 3,
    Begin                   = 5,
    Complete                = 6,

    OpenLog                 = 1001,
    OpenCheckpoint          = 1002,
    MissingLog              = 1004,
    BeginUndo               = 1005,
    NotificationEvent       = 1006,
    SignalErrorCondition    = 1007,

    AttachedDb              = 1008,
    DetachingDb             = 1009,
    
    CommitCtx               = 1010,

    PagePatchRequest        = 1101,
    CorruptedPage           = 1102,

};

MSINTERNAL enum class MJET_DBINFO_LEVEL : unsigned long
{
    Filename        = 0,
    Connect         = 1,
    Country         = 2,
    LCID            = 3,
    Langid          = 3,
    Cp          = 4,
    Collate         = 5,
    Options         = 6,
    Transactions        = 7,
    Version         = 8,
    Isam            = 9,
    Filesize        = 10,
    SpaceOwned      = 11,
    SpaceAvailable      = 12,
    Upgrade         = 13,
    Misc            = 14,
    DBInUse         = 15,
    HasSLVFile      = 16,
    PageSize        = 17,
    StreamingFileSpace  = 18,
    FileType        = 19,
    StreamingFileSize   = 20,
};

MSINTERNAL enum class MJET_SNP
{
    Repair                      = 2,
    Compact                     = 4,
    Restore                     = 8,
    Backup                      = 9,
    Upgrade                     = 10,
    Scrub                       = 11,
    UpgradeRecordFormat         = 12,
    RecoveryControl             = 18,
    ExternalAutoHealing         = 19,

};

MSINTERNAL value struct MJET_SNPROG
{
    System::Int32       Done;
    System::Int32       Total;


    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams =    {
            Done,
            Total };
        return String::Format(
            L"MJET_SNPROG("
            L"Done = {0} "
            L"Total = {1})",
            TempParams );
        }

};

MSINTERNAL enum class MJET_RECOVERY_RETVALS
{
    Default = -9999,
    RecoveredWithoutUndo = JET_errRecoveredWithoutUndo,
    RecoveredWithoutUndoDatabaseConsistent = JET_errRecoveredWithoutUndoDatabasesConsistent,
};

#ifdef INTERNALUSE
MSINTERNAL enum class MJET_RECOVERY_ACTIONS
{
    Invalid = 0,
    MissingLogMustFail = JET_MissingLogMustFail,
    MissingLogContinueToUndo = JET_MissingLogContinueToUndo,
    MissingLogContinueToRedo = JET_MissingLogContinueToRedo,
    MissingLogContinueTryCurrentLog = JET_MissingLogContinueTryCurrentLog,
    MissingLogCreateNewLogStream = JET_MissingLogCreateNewLogStream,
};
#endif

#ifdef INTERNALUSE
MSINTERNAL enum class MJET_OPENLOG_REASONS
{
    Invalid = 0,
    RecoveryCheckingAndPatching = JET_OpenLogForRecoveryCheckingAndPatching,
    Redo = JET_OpenLogForRedo,
    Undo = JET_OpenLogForUndo,
    Do = JET_OpenLogForDo,
};
#endif

#ifdef INTERNALUSE
MSINTERNAL ref struct MJET_RECOVERY_CONTROL abstract
{
    IsamErrorException ^    ErrDefault;
    MJET_INSTANCE           Instance;
};

MSINTERNAL ref struct MJET_SNRECOVERY : MJET_RECOVERY_CONTROL
{
    System::Int32           GenNext;
    System::String ^        LogFileName;
    System::Boolean         IsCurrentLog;
    MJET_OPENLOG_REASONS    Reason;
    array<MJET_DBINFO>^     DatabaseInfo;

    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams =    {
            GenNext,
            LogFileName,
            IsCurrentLog,
            Reason,
            Instance,
            DatabaseInfo};

        return String::Format(
            L"MJET_SNRECOVERY("
            L"GenNext = {0} "
            L"LogFileName = {1} "
            L"IsCurrentLog = {2} "
            L"Reason = {3}"
            L"Instance = {4} "
            L"DatabaseInfo = {5})",
            TempParams );
        }
};

MSINTERNAL ref struct MJET_SNOPEN_CHECKPOINT : MJET_RECOVERY_CONTROL
{
    System::String ^    Checkpoint;

    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams =    {
            Checkpoint,
            Instance};

        return String::Format(
            L"MJET_SNOPEN_CHECKPOINT("
            L"Checkpoint = {0}"
            L"Instance = {1} ",
            TempParams );
        }
};

MSINTERNAL ref struct MJET_SNMISSING_LOG : MJET_RECOVERY_CONTROL
{
    System::Int32           GenMissing;
    System::Boolean         IsCurrentLog;
    MJET_RECOVERY_ACTIONS   NextAction;
    System::String ^        LogFileName;
    array<MJET_DBINFO> ^    DatabaseInfo;
    
    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams =    {
            GenMissing,
            LogFileName,
            IsCurrentLog,
            NextAction,
            Instance,
            DatabaseInfo};

        return String::Format(
            L"MJET_SNMISSING_LOG("
            L"GenMissing = {0} "
            L"LogFileName = {1} "
            L"IsCurrentLog = {2} "
            L"NextAction = {3}"
            L"Instance = {4} "
            L"DatabaseInfo = {5})",
            TempParams );
        }
};

MSINTERNAL ref struct MJET_SNBEGIN_UNDO : MJET_RECOVERY_CONTROL
{
    array<MJET_DBINFO> ^    DatabaseInfo;

    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams =    {
            DatabaseInfo,
            Instance};

        return String::Format(
            L"MJET_SNBEGIN_UNDO("
            L"DatabaseInfo = {0}"
            L"Instance = {1} ",
            TempParams );
        }
};

MSINTERNAL ref struct MJET_SNNOTIFICATION_EVENT : MJET_RECOVERY_CONTROL
{
    System::Int32   EventId;
    
    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams =    {
            EventId,
            Instance};

        return String::Format(
            L"MJET_SNNOTIFICATION_EVENT("
            L"EventId = {0}"
            L"Instance = {1} ",
            TempParams );
        }
};

MSINTERNAL ref struct MJET_SNSIGNAL_ERROR : MJET_RECOVERY_CONTROL
{
    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams =    {
            Instance};

        return String::Format(
            L"MJET_SNSIGNAL_ERROR("
            L"Instance = {0} ",
            TempParams );
        }
};

MSINTERNAL ref struct MJET_SNATTACHED_DB : MJET_RECOVERY_CONTROL
{
    System::String ^        DatabaseName;

    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams =    {
            DatabaseName,
            Instance};

        return String::Format(
            L"MJET_SNATTACHED_DB("
            L"DatabaseName = {0} "
            L"Instance = {1} ",
            TempParams );
        }
};

MSINTERNAL ref struct MJET_SNDETACHING_DB : MJET_RECOVERY_CONTROL
{
    System::String ^        DatabaseName;

    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams =    {
            DatabaseName,
            Instance};

        return String::Format(
            L"MJET_SNDETACHING_DB("
            L"DatabaseName = {0} "
            L"Instance = {1} ",
            TempParams );
        }
};

MSINTERNAL ref struct MJET_SNCOMMIT_CTX : MJET_RECOVERY_CONTROL
{
    array<System::Byte>^    Data;

    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams =    {
            Instance,
            Data};

        return String::Format(
            L"MJET_SNCOMMIT_CTX("
            L"Instance = {0} ",
            L"Data = {1} ",
            TempParams );
        }
};
#endif

MSINTERNAL value struct MJET_SNPATCHREQUEST
{
    System::Int64           PageNumber;
    System::String^         LogFileName;
    MJET_INSTANCE           Instance;
    MJET_DBINFO             DatabaseInfo;
    array<System::Byte>^    Token;
    array<System::Byte>^    Data;
    System::Int32           DatabaseId;
    
    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams =    {
            PageNumber,
            LogFileName,
            Instance,
            DatabaseInfo,
            Token,
            Data,
            DatabaseId
            };

        return String::Format(
            L"MJET_SNPATCHREQUEST("
            L"PageNumber = {0} "
            L"LogFileName = {1} "
            L"Instance = {2} "
            L"DatabaseInfo = {3} ",
            L"Token = {4} ",
            L"Data = {5} ",
            L"DatabaseId = {6})",
            TempParams );
        }
};

MSINTERNAL value struct MJET_DDLCALLBACKDLL
{
    System::String ^    OldDLL;
    System::String ^    NewDLL;

    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams = {
            OldDLL,
            NewDLL,
            };

        return String::Format(
            L"MJET_DDLCALLBACKDLL("
            L"OldDLL = {0} "
            L"NewDLL = {1})",
            TempParams );
        }
};

MSINTERNAL value struct MJET_OPENTEMPORARYTABLE
{
    array<MJET_COLUMNDEF>^  Columndefs;
    MJET_UNICODEINDEX       UnicodeIndex;
    MJET_GRBIT              Grbit;
    array<MJET_COLUMNID>^   Columnids;
    System::Int32           KeyLengthMax;
    System::Int32           VarSegMac;

    virtual String ^ ToString() override
        {
        array<System::Object^>^ TempParams =    {
            Columndefs,
            UnicodeIndex,
            Grbit,
            Columnids,
            KeyLengthMax,
            VarSegMac };
        return String::Format(
            L"MJET_OPENTEMPORARYTABLE("
            L"Columndefs = {0} "
            L"UnicodeIndex = {1} "
            L"Grbit = {2} "
            L"Columnids = {3} "
            L"KeyLengthMax = {4} "
            L"VarSegMac = {5})",
            TempParams );
        }
};

#ifdef INTERNALUSE
MSINTERNAL enum class MJET_HUNG_IO_ACTION
{
    Event = JET_bitHungIOEvent,
    CancelIo = JET_bitHungIOCancel,
    Debug = JET_bitHungIODebug,
    Enforce = JET_bitHungIOEnforce,
    Timeout = JET_bitHungIOTimeout,
};

MSINTERNAL enum class MJET_NEGATIVE_TESTING_BITS
{
    DeletingLogFiles                    = fDeletingLogFiles,
    CorruptingLogFiles                  = fCorruptingLogFiles,
    LockingCheckpointFile               = fLockingCheckpointFile,
    CorruptingDbHeaders                 = fCorruptingDbHeaders,
    CorruptingPagePgnos                 = fCorruptingPagePgnos,
    LeakStuff                           = fLeakStuff,
    CorruptingWithLostFlush             = fCorruptingWithLostFlush,
    DisableTimeoutDeadlockDetection     = fDisableTimeoutDeadlockDetection,
    CorruptingPages                     = fCorruptingPages,
    DiskIOError                         = fDiskIOError,
    InvalidAPIUsage                     = fInvalidAPIUsage,
    InvalidUsage                        = fInvalidUsage,
    CorruptingPageLogically             = fCorruptingPageLogically,
    OutOfMemory                         = fOutOfMemory,
    LeakingUnflushedIos                 = fLeakingUnflushedIos,
    HangingIOs                          = fHangingIOs,
};

MSINTERNAL enum class MJET_TRACEOP
{
    Null = JET_traceopNull,
    SetGlobal = JET_traceopSetGlobal,
    SetTag = JET_traceopSetTag,
    SetAllTags = JET_traceopSetAllTags,
    SetMessagePrefix = JET_traceopSetMessagePrefix,
    RegisterTag = JET_traceopRegisterTag,
    RegisterAllTags = JET_traceopRegisterAllTags,
    SetEmitCallback = JET_traceopSetEmitCallback,
    SetThreadidFilter = JET_traceopSetThreadidFilter,
    SetDbidFilter = JET_traceopSetDbidFilter,
};

MSINTERNAL enum class MJET_TRACETAG
{
    Null = JET_tracetagNull,
    Information = JET_tracetagInformation,
    Errors = JET_tracetagErrors,
    Asserts = JET_tracetagAsserts,
    API = JET_tracetagAPI,
    InitTerm = JET_tracetagInitTerm,
    BufferManager = JET_tracetagBufferManager,
    BufferManagerHashedLatches = JET_tracetagBufferManagerHashedLatches,
    IO = JET_tracetagIO,
    Memory = JET_tracetagMemory,
    VersionStore = JET_tracetagVersionStore,
    VersionStoreOOM = JET_tracetagVersionStoreOOM,
    VersionCleanup = JET_tracetagVersionCleanup,
    Catalog = JET_tracetagCatalog,
    DDLRead = JET_tracetagDDLRead,
    DDLWrite = JET_tracetagDDLWrite,
    DMLRead = JET_tracetagDMLRead,
    DMLWrite = JET_tracetagDMLWrite,
    DMLConflicts = JET_tracetagDMLConflicts,
    Instances = JET_tracetagInstances,
    Databases = JET_tracetagDatabases,
    Sessions = JET_tracetagSessions,
    Cursors = JET_tracetagCursors,
    CursorNavigation = JET_tracetagCursorNavigation,
    CursorPageRefs = JET_tracetagCursorPageRefs,
    Btree = JET_tracetagBtree,
    Space = JET_tracetagSpace,
    FCBs = JET_tracetagFCBs,
    Transactions = JET_tracetagTransactions,
    Logging = JET_tracetagLogging,
    Recovery = JET_tracetagRecovery,
    Backup = JET_tracetagBackup,
    Restore = JET_tracetagRestore,
    OLD = JET_tracetagOLD,
    Eventlog = JET_tracetagEventlog,
    BufferManagerMaintTasks = JET_tracetagBufferManagerMaintTasks,
    SpaceManagement = JET_tracetagSpaceManagement,
    SpaceInternal = JET_tracetagSpaceInternal,
    IOQueue = JET_tracetagIOQueue,
    DiskVolumeManagement = JET_tracetagDiskVolumeManagement,
    Callbacks = JET_tracetagCallbacks,
    IOProblems = JET_tracetagIOProblems,
    Upgrade = JET_tracetagUpgrade,
    RecoveryValidation = JET_tracetagRecoveryValidation,
    BufferManagerBufferCacheState = JET_tracetagBufferManagerBufferCacheState,
    BufferManagerBufferDirtyState = JET_tracetagBufferManagerBufferDirtyState,
    TimerQueue = JET_tracetagTimerQueue,
    SortPerf = JET_tracetagSortPerf,
};

MSINTERNAL enum class MJET_EVENTLOGGINGLEVEL
{
    Disable = JET_EventLoggingDisable,
    Min     = JET_EventLoggingLevelMin,
    Low     = JET_EventLoggingLevelLow,
    Medium  = JET_EventLoggingLevelMedium,
    High    = JET_EventLoggingLevelHigh,
    Max     = JET_EventLoggingLevelMax,
};
#endif

}
}
}
