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
    MJET_COLTYP     Coltyp;         //  the coltyp isn't always filled it, it may be JET_ColtypNil
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

    //JET_COLUMNID  columnid;   // returned column id
    //JET_ERR       err;            // returned error code

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
    System::Int64   Data;               //  user data in record
    System::Int64   LongValueData;      //  user data associated with the record but stored in the long-value tree (NOTE: does NOT count intrinsic long-values)
    System::Int64   Overhead;           //  record overhead
    System::Int64   LongValueOverhead;  //  overhead of long-value data (NOTE: does not count intrinsic long-values)
    System::Int64   NonTaggedColumns;   //  total number of fixed/variable columns
    System::Int64   TaggedColumns;      //  total number of tagged columns
    System::Int64   LongValues;         //  total number of values stored in the long-value tree for this record (NOTE: does NOT count intrinsic long-values)
    System::Int64   MultiValues;        //  total number of values beyond the first for each column in the record
    System::Int64   CompressedColumns;          //  total number of columns which are compressed
    System::Int64   DataCompressed;             //  compressed size of user data in record (same as Data if no intrinsic long-values are compressed)
    System::Int64   LongValueDataCompressed;    //  compressed size of user data in the long-value tree (same as LongValueData if no separated long values are compressed)

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

/// <summary>
/// Information about a page
/// </summary>
MSINTERNAL value struct MJET_PAGEINFO
{
    /// <summary>
    /// Page number
    /// </summary>
    System::Int64   PageNumber;

    /// <summary>
    /// False if the page is zeroed, true otherwise
    /// </summary>
    System::Boolean IsInitialized;

    /// <summary>
    /// True if a correctable error was found on the page, false otherwise
    /// </summary>
    System::Boolean IsCorrectableError;

    /// <summary>
    /// Checksum that is calculated for this page
    /// </summary>
    array<System::Byte>^  ActualChecksum;

    /// <summary>
    /// Checksum stored on the page
    /// </summary>
    array<System::Byte>^  ExpectedChecksum;

    /// <summary>
    /// Dbtime on the page
    /// </summary>
    System::UInt64  Dbtime;

    /// <summary>
    /// Checksum of the page structure
    /// </summary>
    System::UInt64  StructureChecksum;

    /// <summary>
    /// Flags
    /// </summary>
    /// <remarks>
    /// Currently unused
    /// </remarks>
    System::Int64   Flags;

    /// <summary>
    /// Convert to string
    /// </summary>
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

/// <summary>
/// 'Unique' signature for a database or log stream.
/// </summary>
/// <remarks>
/// The non-managed version has a Computername field, but it is always empty.
/// </remarks>
MSINTERNAL value struct MJET_SIGNATURE
{
    /// <summary>
    /// A random number (for uniqueness)
    /// </summary>
    System::Int64       Random;

    /// <summary>
    /// The creation time of the database or the first logfile in the stream as UInt64
    /// </summary>
    System::UInt64      CreationTimeUInt64;

    /// <summary>
    /// The creation time of the database or the first logfile in the stream.
    /// </summary>
    System::DateTime    CreationTime;

    virtual String ^ ToString() override
        {
        return String::Format(
                "MJET_SIGNATURE(Random = {0},CreationTime = {1},CreationTimeUint64 = 0x{2})",
                Random, CreationTime, CreationTimeUInt64.ToString("X") );
        }
};

/// <summary>
/// Information about a logfile. Returned by JetGetLogFileInfo.
/// </summary>
MSINTERNAL value struct MJET_LOGINFOMISC
{
    /// <summary>
    /// Generation of the logfile.
    /// </summary>
    System::Int64       Generation;

    /// <summary>
    /// Logfile signature
    /// </summary>
    /// <remarks>
    /// Unique per logfile sequence, not per logfile.
    /// </remarks>
    MJET_SIGNATURE      Signature;

    /// <summary>
    /// Creation time of this logfile.
    /// </summary>
    System::DateTime    CreationTime;

    /// <summary>
    /// Creation time of the previous logfile.
    /// </summary>
    System::DateTime    PreviousGenerationCreationTime;

    /// <summary>
    /// </summary>
    System::Int64       Flags;

    /// <summary>
    /// </summary>
    System::Int64       VersionMajor;

    /// <summary>
    /// </summary>
    System::Int64       VersionMinor;

    /// <summary>
    /// </summary>
    System::Int64       VersionUpdate;

    /// <summary>
    /// </summary>
    System::Int64       SectorSize;

    /// <summary>
    /// </summary>
    System::Int64       HeaderSize;

    /// <summary>
    /// </summary>
    System::Int64       FileSize;

    /// <summary>
    /// </summary>
    System::Int64       DatabasePageSize;

    /// <summary>
    /// Checkpoint generation at the time this logfile was created
    /// </summary>
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

/// <summary>
/// A position in the stream of log records generated by ESE
/// </summary>
MSINTERNAL value struct MJET_LGPOS
{
    /// <summary>
    /// Generation number. This maps to an individual logfile
    /// </summary>
    System::Int64   Generation;

    /// <summary>
    /// Sector offset in logfile.
    /// </summary>
    System::Int32   Sector;

    /// <summary>
    /// Byte offset in sector
    /// </summary>
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

/// <summary>
/// Database states which can be present in the MJET_DBINFO structure
/// </summary>
MSINTERNAL enum class MJET_DBSTATE
{
    /// <summary>
    /// </summary>
    JustCreated = 1,

    /// <summary>
    /// </summary>
    DirtyShutdown = 2,

    /// <summary>
    /// </summary>
    CleanShutdown = 3,

    /// <summary>
    /// </summary>
    BeingConverted = 4,

    /// <summary>
    /// </summary>
    ForceDetach = 5,

    /// <summary>
    /// </summary>
    IncrementalReseedInProgress = 6,

    /// <summary>
    /// </summary>
    DirtyAndPatchedShutdown = 7,

    /// <summary>
    /// </summary>
    Unknown = 99,
};

/// <summary>
/// Information about a backup
/// </summary>
MSINTERNAL value struct MJET_BKINFO
{
    /// <summary>
    /// </summary>
    MJET_LGPOS          Lgpos;

    /// <summary>
    /// </summary>
    System::DateTime    BackupTime;

    /// <summary>
    /// True if this is a snapshot backup.
    /// </summary>
    System::Boolean     SnapshotBackup;

    /// <summary>
    /// </summary>
    System::Int64       LowestGeneration;

    /// <summary>
    /// </summary>
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

/// <summary>
/// Returned by JetGetDatabaseFileInfo
/// </summary>
MSINTERNAL value struct MJET_DBINFO
{
    /// <summary>
    /// Version of DAE the database was created with.
    /// </summary>
    System::Int64   Version;

    /// <summary>
    /// Used to track incremental database format updates that are backward-
    /// compatible.
    /// </summary>
    System::Int64   Update;

    /// <summary>
    /// Signature of the database. Includes creation time
    /// </summary>
    MJET_SIGNATURE  Signature;

    /// <summary>
    /// Database state (e.g. consistent/inconsistent)
    /// </summary>
    MJET_DBSTATE        State;

    /// <summary>
    /// Time of last detach/shutdown.
    /// </summary>
    /// <remarks>
    /// 0 if in inconsistent state
    /// </remarks>
    MJET_LGPOS      ConsistentLgpos;

    /// <summary>
    /// Lgpos of last detach/shutdown record.
    /// </summary>
    /// <remarks>
    /// 0 if in inconsistent state
    /// </remarks>
    System::DateTime    ConsistentTime;

    /// <summary>
    /// Lgpos of last attach record.
    /// </summary>
    MJET_LGPOS      AttachLgpos;

    /// <summary>
    /// Last attach time
    /// </summary>
    System::DateTime    AttachTime;

    /// <summary>
    /// Lgpos of last detach record.
    /// </summary>
    MJET_LGPOS      DetachLgpos;

    /// <summary>
    /// Last detach time
    /// </summary>
    System::DateTime    DetachTime;

    /// <summary>
    /// Log signature for log stream that uses this database
    /// </summary>
    MJET_SIGNATURE  LogSignature;

    /// <summary>
    /// Information about that last successful full backup.
    /// </summary>
    MJET_BKINFO     LastFullBackup;

    /// <summary>
    /// Information about the last successful incremental backup.
    /// </summary>
    /// <remarks>
    /// Reset when LastFullBackup is set.
    /// </remarks>
    MJET_BKINFO     LastIncrementalBackup;

    /// <summary>
    /// Current full backup information. If this is set then the database
    /// is the product of a backup.
    /// </summary>
    MJET_BKINFO     CurrentFullBackup;

    /// <summary>
    /// Is the shadow catalog disabled for this database.
    /// </summary>
    /// <remarks>
    /// The shadow catalog is always enabled.
    /// </remarks>
    System::Boolean ShadowCatalogDisabled;

    /// <summary>
    /// </summary>
    System::Boolean UpgradeDatabase;

    /// <summary>
    /// Major version of the OS that the database was last used on.
    /// </summary>
    /// <remarks>
    /// This information is needed to decide if an index needs
    /// be recreated due to sort table changes.
    /// </remarks>
    System::Int64   OSMajorVersion;

    /// <summary>
    /// Minor version of the OS that the database was last used on.
    /// </summary>
    /// <remarks>
    /// This information is needed to decide if an index needs
    /// be recreated due to sort table changes.
    /// </remarks>
    System::Int64   OSMinorVersion;

    /// <summary>
    /// Build number of the OS that the database was last used on.
    /// </summary>
    /// <remarks>
    /// This information is needed to decide if an index needs
    /// be recreated due to sort table changes.
    /// </remarks>
    System::Int64   OSBuild;

    /// <summary>
    /// Service pack number of the OS that the database was last used on.
    /// </summary>
    /// <remarks>
    /// This information is needed to decide if an index needs
    /// be recreated due to sort table changes.
    /// </remarks>
    System::Int64   OSServicePackNumber;

    /// <summary>
    /// Page size of the database.
    /// </summary>
    System::Int32   DatabasePageSize;

    /// <summary>
    /// The minimum log generation required for replaying the logs. Typically the checkpoint generation
    /// </summary>
    System::Int64   MinimumLogGenerationRequired;

    /// <summary>
    /// The maximum log generation required for replaying the logs. Typically the current log generation
    /// </summary>
    System::Int64   MaximumLogGenerationRequired;

    /// <summary>
    /// Creation time of the MaximumLogGenerationRequired logfile.
    /// </summary>
    System::DateTime MaximumLogGenerationCreationTime;

    /// <summary>
    /// Number of times repair has been called on this database
    /// </summary>
    System::Int64   RepairCount;

    /// <summary>
    /// The date of the last time repair was called on this database.
    /// </summary>
    System::DateTime    LastRepairTime;

    /// <summary>
    /// Number of times ErrREPAIRAttachForRepair has been called on this database before the last defrag
    /// </summary>
    System::Int64   RepairCountOld;

    /// <summary>
    /// Number of times a one bit error was fixed and resulted in a good page
    /// </summary>
    System::Int64   SuccessfulECCPageFixes;

    /// <summary>
    /// The date of the last time that a one bit error was fixed and resulted in a good page
    /// </summary>
    System::DateTime LastSuccessfulECCPageFix;

    /// <summary>
    /// Number of times a one bit error was fixed and resulted in a good page before last repair
    /// </summary>
    /// <remarks>
    /// Reset by repair.
    /// </remarks>
    System::Int64   SuccessfulECCPageFixesOld;

    /// <summary>
    /// Number of times a one bit error was fixed and resulted in a bad page
    /// </summary>
    System::Int64   UnsuccessfulECCPageFixes;

    /// <summary>
    /// The date of the last time that a one bit error was fixed and resulted in a bad page
    /// </summary>
    System::DateTime LastUnsuccessfulECCPageFix;

    /// <summary>
    /// Number of times a one bit error was fixed and resulted in a bad page before last repair
    /// </summary>
    /// <remarks>
    /// Reset by repair.
    /// </remarks>
    System::Int64   UnsuccessfulECCPageFixesOld;

    /// <summary>
    /// Number of times a non-correctable ECC/checksum error was found
    /// </summary>
    System::Int64   BadChecksums;

    /// <summary>
    /// The date of the last time that a non-correctable ECC/checksum error was found
    /// </summary>
    System::DateTime LastBadChecksum;

    /// <summary>
    /// Number of times a non-correctable ECC/checksum error was found before last repair
    /// </summary>
    /// <remarks>
    /// Reset by repair.
    /// </remarks>
    System::Int64   BadChecksumsOld;

    /// <summary>
    /// The maximum log generation committed to the database. Typically the current log generation
    /// </summary>
    System::Int64   CommittedGeneration;

    /// <summary>
    /// Information about that last successful copy backup.
    /// </summary>
    MJET_BKINFO     LastCopyBackup;

    /// <summary>
    /// Information about the last successful differential backup.
    /// </summary>
    /// <remarks>
    /// Reset when LastFullBackup is set.
    /// </remarks>
    MJET_BKINFO     LastDifferentialBackup;

    /// <summary>
    /// Number of times incremental reseed has been invoked on this database
    /// </summary>
    System::Int64   IncrementalReseedCount;

    /// <summary>
    /// The date of the last time incremental reseed was invoked on this database.
    /// </summary>
    System::DateTime    LastIncrementalReseedTime;

    /// <summary>
    /// Number of times incremental reseed was invoked on this database before the last defrag
    /// </summary>
    System::Int64   IncrementalReseedCountOld;

    /// <summary>
    /// Number of pages patched in the database as a part of incremental reseed
    /// </summary>
    System::Int64   PagePatchCount;

    /// <summary>
    /// The date of the last time that a page was patched in the database as a part of incremental reseed
    /// </summary>
    System::DateTime    LastPagePatchTime;

    /// <summary>
    /// Number of pages patched in the database as a part of incremental reseed before the last defrag
    /// </summary>
    System::Int64   PagePatchCountOld;

    /// <summary>
    /// The date of the last time that the database scan finished during recovery
    /// </summary>
    System::DateTime    LastChecksumTime;

    /// <summary>
    /// The date of the start time of the current database scan during recovery
    /// </summary>
    System::DateTime    ChecksumStartTime;

    /// <summary>
    /// Number of pages checked in the current database scan during recovery
    /// </summary>
    System::Int64 PageCheckedCount;

    /// <summary>
    /// Lgpos of last attach record.
    /// </summary>
    MJET_LGPOS      LastReAttachLgpos;

    /// <summary>
    /// Last attach time
    /// </summary>
    System::DateTime    LastReAttachTime;

    /// <summary>
    /// Random signature generated at the time of the last DB header flush.
    /// </summary>
    MJET_SIGNATURE DbHeaderFlushSignature;

    /// <summary>
    /// Random signature generated at the time of the last FM header flush.
    /// </summary>
    MJET_SIGNATURE FlushMapHeaderFlushSignature;

    /// <summary>
    /// The minimum log generation required to bring the database to a clean state.
    /// </summary>
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

/// <summary>
/// The MJET_INSTANCE_INFO structure receives information about running database instances when used
/// with the MJetGetInstanceInfo and MJetOSSnapshotFreeze functions.
/// </summary>
/// <remarks>
/// Each database instance can have several databases attached to it.
/// For a given MJET_INSTANCE_INFO structure, the array of strings that is returned for the databases
/// are in the same order. For example, "DatabaseDisplayName[ i ]" and "DatabaseFileName[ i ]" both
/// refer to the same database.
/// </remarks>
MSINTERNAL value struct MJET_INSTANCE_INFO
{
    /// <summary>
    /// The MJET_INSTANCE of the given instance.
    /// </summary>
    MJET_INSTANCE           Instance;

    /// <summary>
    /// The name of the database instance. This value can be null if the instance does not have a name.
    /// </summary>
    System::String^         InstanceName;

    /// <summary>
    /// An array of strings, each holding the file name of a database that is attached to the database instance.
    /// </summary>
    array<System::String^>^ DatabaseFileName;

    /// <summary>
    /// An array of strings, each holding the display name of a database. Currently the strings can be null.
    /// </summary>
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

    //  void *      pfnCallback;
    //  void *      pvCallback;

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
    Langid          = 3,    // OBSOLETE: use JET_DbInfoLCID instead
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
    HasSLVFile      = 16,   // not_PubEsent
    PageSize        = 17,
    StreamingFileSpace  = 18,   // SLV owned and available space (may be slow because this sequentially scans the SLV space tree)
    FileType        = 19,
    StreamingFileSize   = 20,   // SLV owned space only (fast because it does NOT scan the SLV space tree)
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

// The following flags are combinable
#ifdef INTERNALUSE
MSINTERNAL enum class MJET_HUNG_IO_ACTION
{
    Event = JET_bitHungIOEvent, // Log event when an IO appears to be hung for over the IO threshold.
    CancelIo = JET_bitHungIOCancel, // Cancel an IO when an IO appears to be hung for over 2 x the IO threshhold.
    Debug = JET_bitHungIODebug, // Crash the process when an IO appears to be hung for over 3 x the IO threshhold.
    Enforce = JET_bitHungIOEnforce, // Crash the process when an IO appears to be hung for over 3 x the IO threshhold.
    Timeout = JET_bitHungIOTimeout, // Failure item when an IO appears to be hung for over 4 x the IO threshhold (considered timed out).
};

MSINTERNAL enum class MJET_NEGATIVE_TESTING_BITS
{
    DeletingLogFiles                    = fDeletingLogFiles,
    CorruptingLogFiles                  = fCorruptingLogFiles,
    LockingCheckpointFile               = fLockingCheckpointFile,
    CorruptingDbHeaders                 = fCorruptingDbHeaders,
    CorruptingPagePgnos                 = fCorruptingPagePgnos,             // but checksum is ok.
    LeakStuff                           = fLeakStuff,
    CorruptingWithLostFlush             = fCorruptingWithLostFlush,
    DisableTimeoutDeadlockDetection     = fDisableTimeoutDeadlockDetection,
    CorruptingPages                     = fCorruptingPages,
    DiskIOError                         = fDiskIOError, // Injecting ERROR_IO_DEVICE (results in Jet_errDiskIO)
    InvalidAPIUsage                     = fInvalidAPIUsage, // invalid usage of the JET API
    InvalidUsage                        = fInvalidUsage, // invalid usage of some sub-component during unit testing
    CorruptingPageLogically             = fCorruptingPageLogically, // but checksum (and perhaps structure / consistency) is ok.
    OutOfMemory                         = fOutOfMemory,
    LeakingUnflushedIos                 = fLeakingUnflushedIos, // for internal tests that do not flush file buffers before deleting the pfapi.
    HangingIOs                          = fHangingIOs, // Simulate hung IOs
};

MSINTERNAL enum class MJET_TRACEOP
{
    Null = JET_traceopNull,
    SetGlobal = JET_traceopSetGlobal,               //  enable/disable tracing ("ul" param cast to BOOL)
    SetTag = JET_traceopSetTag,                 //  enable/disable tracing for specified tag ("ul" param cast to BOOL)
    SetAllTags = JET_traceopSetAllTags,             //  enable/disable tracing for all tags ("ul" param cast to BOOL)
    SetMessagePrefix = JET_traceopSetMessagePrefix,     //  text which should prefix all emitted messages ("ul" param cast to CHAR*)
    RegisterTag = JET_traceopRegisterTag,           //  callback to register a trace tag ("ul" param cast to JET_PFNTRACEREGISTER)
    RegisterAllTags = JET_traceopRegisterAllTags,       //  callback to register all trace tags ("ul" param cast to JET_PFNTRACEREGISTER)
    SetEmitCallback = JET_traceopSetEmitCallback,       //  override default trace emit function with specified function, or pass NULL to revert to default ("ul" param ast to JET_PFNTRACEEMIT)
    SetThreadidFilter = JET_traceopSetThreadidFilter,       //  threadid to use to filter traces (0==all threads, -1==no threads)
    SetDbidFilter = JET_traceopSetDbidFilter,           //  JET_DBID to use to filter traces (0x7fffffff==all db's, JET_dbidNil==no db's)
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
