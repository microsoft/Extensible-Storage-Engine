# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# Are we in ESENT or ESE ..
#

# Process the rest of the in args ...
$OUTDIR         	= $ARGV[1];
$DATAFILE_nopath	= "perfdata.cxx";
$DATAFILE			= $OUTDIR . $DATAFILE_nopath;
$DISPFILE_nopath	= "perfdisp.cxx";
$DISPFILE			= $OUTDIR . $DISPFILE_nopath;
$SYMFILE_nopath		= $ARGV[2];					# "eseperf.hxx" in Ex, "esentprf.hxx" in NT
$SYMFILE			= $OUTDIR . $SYMFILE_nopath;
$INIFILE_nopath		= $ARGV[3];					# "eseperf.ini" in Ex, "esentprf.ini" in NT
$INIFILE			= $OUTDIR . $INIFILE_nopath;
$inputfile			= $ARGV[4];						# usually "perfdata.txt"
$modulename			= $ARGV[5];					# either "ESE" or "ESENT"

print( "$SYMFILE $INIFILE $inputfile\n" );

$ModuleInfo{"MaxInstNameSize"} = "0";
$ModuleInfo{"SchemaVersion"} = "0";

$ProcessInfo{"ProcessInfoFunction"} = "NULL";

$line = 0;
$section = "";
$errors = 0;

$RootObjectRead = 0;

$CurLang = 0;
$CurObj = 0;
$CurCtr = 0;
$CurNameText = 0;
$CurHelpText = 0;

open( INPUTFILE, "<$inputfile" ) || die "Cannot open $inputfile: ";

INPUT: while(<INPUTFILE>) {
	
	$line++;

	$key = "";
	$value = "";

	if( /^;/ ) {
		next INPUT;					# throw away comment in INPUTFILE
	}

	if( /^\[([_a-zA-Z]\w*)\]/ ) {		# match section names, like "[ModuleInfo]"
		$section = $1;
		$type = "";					# clear $type upon entering a new section
		next INPUT;
	}

	if( /^(\w+)=(.*)$/ ) {				# match key=value pairs
		if( $section eq "" ) {
			warn "$inputfile($line): error: unexpected KEY=VALUE (\"$_\")";
			$errors++;
		}
		$key = $1;
		$value = $2;
	}

	if( $section eq "" ) {
		if(  $key ne "" ) {
			warn "$inputfile($line): error: Key \"$key\" defined under no section";
			$errors++;
		}
		next INPUT;
	}

	if( $section eq "ModuleInfo" ) {
		$ModuleInfo{$key} = $value;
		next INPUT;
	}

	if( $section eq "ProcessInfo" ) {
		$ProcessInfo{$key} = $value;
		next INPUT;
	}

	if( $section eq "Languages" && $key ne "" ) {
		$CurLang++;
		$Language_ID[$CurLang] = $key;
		$Language_Name[$CurLang] = $value;
		next INPUT;
	}

	if( $section ne "" && $key eq "Type" && $type ne "" ) {
		warn "$inputfile($line): error: Key \"$key\" multiply defined under section $section";
		$errors++;
		next INPUT;
	}
	 
	if( $key eq "Type" && $value =~ /RootObject/ ) {
		if( 0 != $RootObjectRead ) {
			warn "$inputfile($line): error: Key \"$key\" multiply defined under section $section";
			$errors++;
			}
		elsif( 0 != $CurObj ) {
			warn "$inputfile($line): error: Root Object must be first object/counter";
			$errors++;
		}
		$RootObjectRead = 1;

		# FALLTHRU to case below
	}

	if( $key eq "Type" && $value =~ /Object/ ) {
		$type = "Object";
		$CurObj++;

		$Object_SymName[$CurObj] = $section;

		next INPUT;
	}

	if( $key eq "Type" && $value =~ /Counter/ ) {
		$type = "Counter";
		$CurCtr++;

		$Counter_SymName[$CurCtr] = $section;
		$Counter_Size[$CurCtr] = 0;

		next INPUT;
	}

	if( $key eq "Type" ) {	# valid types are only "RootObject", "Object" and "Counter"
		warn "$inputfile($line): error: Illegal value for section \"$section\", key \"$key\":  \"$value\"";
		$errors++;
		next INPUT;
	}

	if( $key eq "InstanceCatalogFunction" && $type eq "Object" ) {
		$Object_ICF[$CurObj] = $value;
		next INPUT;
	}

	if( $key eq "Object" && $type eq "Counter" ) {
		$Counter_Object[$CurCtr] = $value;
		next INPUT;
	}

	if( $key eq "Object" && $type eq "Counter" ) {
		$Counter_Object[$CurCtr] = $value;
		next INPUT;
	}

	if( $key eq "DetailLevel" && $type eq "Counter" ) {
		$Counter_DetailLevel[$CurCtr] = $value;
		next INPUT;
	}

	if( $key eq "DefaultScale" && $type eq "Counter" ) {
		$Counter_DefaultScale[$CurCtr] = $value;
		next INPUT;
	}

	if( $key eq "CounterType" && $type eq "Counter" ) {
		$Counter_Type[$CurCtr] = $value;
		next INPUT;
	}

	if( $key eq "Size" && $type eq "Counter" ) {
		$Counter_Size[$CurCtr] = $value;
		next INPUT;
	}

	if( $key eq "EvaluationFunction" && $type eq "Counter" ) {
		$Counter_CEF[$CurCtr] = $value;
		next INPUT;
	}

	if( substr( $key, 3 ) eq "_Name" ) {
		$tag = $section."_".$key;

		# subst $PerformanceObjectName$ to real name
		$value =~ s/\$PerformanceObjectName\$/$ModuleInfo{"PerformanceObjectName_".$modulename}/;
		if ( $value eq "" ) {
			$name = "No name";
		}
		else {
			$name = $value;
		}

		$CurNameText++;
		$NameText_Tag[$CurNameText] = $tag;
		$NameText_Name[$CurNameText] = $name;
		if( $type eq "Object" ) {
			$Object_NameText_Tag[$CurObj] = $tag;
			$Object_NameText_Name[$CurObj] = $name;
		}
		next INPUT;
	}

	if( substr( $key, 3 ) eq "_Help" ) {
		$tag = $section."_".$key;
		if( $HelpText_Tag[$CurHelpText] eq $tag ) {
			$HelpText_Text[$CurHelpText] = $HelpText_Text[$CurHelpText].$value;
		}
		else {
			if ( $value eq "" ) {
				$text = "No text";
			}
			else {
				$text = $value;
			}
			$CurHelpText++;
			$HelpText_Tag[$CurHelpText] = $tag;
			$HelpText_Text[$CurHelpText] = $text;
			$CurCtrHelpMap[$CurCtr] = $CurHelpText;
		}
		next INPUT;
	}

	if( $key ne "" && $type eq "" ) {
		warn "$inputfile($line): error: Unknown key \"$key\" defined for section \"$section\"";
		$errors++;
		next INPUT;
	}

	if( $key ne "" && $type ne "" ) {
		warn "$inputfile($line): error: Unknown key \"$key\" defined for $type \"$section\"";
		$errors++;
		next INPUT;
	}

}

if( 0 == $RootObjectRead ) {
	warn "$inputfile($line): error: Root Object was not specified";
	$errors++;
}

$nLang = $CurLang;
$nObj = $CurObj;
$nCtr = $CurCtr;
$nNameText = $CurNameText;
$nHelpText = $CurHelpText;

for( $CurObj = 1; $CurObj <= $nObj; $CurObj++) {
	if ( $Object_ICF[$CurObj] eq "") {
		warn "$inputfile($line): error: InstanceCatalogFunction for Object \"$Object_SymName[$CurObj]\" was not specified";
		$errors++;
	}
}


for( $CurCtr = 1; $CurCtr <= $nCtr; $CurCtr++ ) {
	if( $Counter_Object[$CurCtr] eq "" ) {
		warn "$inputfile: error: Object for Counter \"$Counter_SymName[$CurCtr]\" was not specified";
		$errors++;
	}
	FINDOBJ: for( $CurObj = 1; $CurObj <= $nObj; $CurObj++ ) {
		if( $Counter_Object[$CurCtr] eq $Object_SymName[$CurObj] ) {
			last FINDOBJ;
		}
	}
	if( $CurObj > $nObj ) {
		warn "$inputfile: error: Object \"$CounterObject[$CurCtr]\" referenced by Counter \"$Counter_SymName[$CurCtr]\" does not exist";
		$errors++;
	}
	if( $Counter_DetailLevel[$CurCtr] eq "" ) {
		warn "$inputfile: error: DetailLevel for Counter \"$Counter_SymName[$CurCtr]\" was not specified";
		$errors++;
	}
	if( $Counter_DefaultScale[$CurCtr] eq "" ) {
		warn "$inputfile: error: DefaultScale for Counter \"$Counter_SymName[$CurCtr]\" was not specified";
		$errors++;
	}
	if( $Counter_Type[$CurCtr] eq "" ) {
		warn "$inputfile: error: Type for Counter \"$Counter_SymName[$CurCtr]\" was not specified";
		$errors++;
	}
	if( $Counter_CEF[$CurCtr] eq "" ) {
		warn "$inputfile: error: EvaluationFunction for Counter \"$Counter_SymName[$CurCtr]\" was not specified";
		$errors++;
	}

	if( ( $HelpText_Text[$CurCtrHelpMap[$CurCtr]] =~ /dev only/i ) &&
		!( $HelpText_Text[$CurCtrHelpMap[$CurCtr]] =~ /Dev Only/ ) ) {
		warn "$inputfile: error: The counter ($CurCtr) \"$Counter_SymName[$CurCtr]\" has bad capitalization on dev only marker, should be \"[Dev Only]\".";
		$errors++;
	}	

	if( ( $Counter_DetailLevel[$CurCtr] =~ /PERF_DETAIL_DEVONLY/ ) &&
		!( $HelpText_Text[$CurCtrHelpMap[$CurCtr]] =~ /Dev Only/ ) &&
		!( $HelpText_Text[$CurCtrHelpMap[$CurCtr]] eq "No text" ) ) {
		warn "$inputfile: error: The counter ($CurCtr) \"$Counter_SymName[$CurCtr]\" is specified as dev only in the programatic DetailLevel, BUT does not have the matching [Dev Only] marker in help text!  Should add help text marker.";
		$errors++;
	}
 
	if( ( $HelpText_Text[$CurCtrHelpMap[$CurCtr]] =~ /Dev Only/ ) &&
		!( $Counter_DetailLevel[$CurCtr] =~ /PERF_DETAIL_DEVONLY/ ) ) {
		warn "$inputfile: error: The counter ($CurCtr) \"$Counter_SymName[$CurCtr]\" is NOT specified as dev only in the programatic DetailLevel, BUT had a [Dev Only] marker in help text!  MUST make counter private or remove help text marker!";
		$errors++;
	}	
}

if( 0 != $errors ) {
	 die "$errors error(s) encountered reading source file \"$inputfile\":  no output files generated";
}


#
# Some notes from someone smarter than me about Perl and Unicode ...
# ----
#
# Which encoding do you want to use? UTF16-LE is the standard on Windows (nearly
# all characters are encoded as 2 bytes), UTF8 is the standard everywhere else 
# (characters are variable length and all ASCII characters are a single byte).
#
# Here's what I've figured out after lots of experimentation. To get UTF16-LE 
# output you need to play a few games with perl...
#
#   open my $FH, ">:raw:encoding(UTF16-LE):crlf:utf8", "e:\\test.txt";
#   print $FH "\x{FEFF}";
#   print $FH "hello unicode world!\nThis is a test.\n";
#   close $FH;
# Reading the IO layers from right to left (the order that they will be applied 
# as they pass from perl to the file) ...
#
# Apply the :utf8 layer first. This doesn't do much except tell perl that we're 
# going to pass "characters" to this file handle instead of bytes so that it 
# doesn't give us "Wide character in print ..." warnings.
#
# Next, apply the :crlf layer as text goes from perl out to the file. This 
# transforms \n (0x0A) into \r\n (0x0D 0x0A) giving you DOS line endings. Perl 
# normally applies this by default on Windows but it would do it at the wrong 
# stage of the pipeline so we removed it (see below), this is where it ought to 
# be.
#
# Next apply the UTF16-LE (little endian) encoding. This takes the characters 
# and transforms them to that encoding. So 0x0A turns into 0x0A 0x00. Note that 
# if you just say 'UTF16' the default endianness is big endian which is 
# backwards from how Windows likes it. However, because we're explicitly 
# specifiying the endianness perl will not write a BOM (byte order mark) at the 
# beginning of the file. We have to make up for that later.
#
# Finally, the :raw psuedo layer just removes the default (on Windows) :crlf 
# layer that transforms \n into \r\n for DOS style line endings. This is 
# necessary because otherwise it would be applied at the wrong place in the 
# pipeline. Without this the encoding layer would turn 0x0A into 0x0A 0x00 and 
# then the crlf layer would turn that into 0x0D 0x0A 0x0A and that's just goofy.
#
# Now that we've got the file opened with the right IO layers in place we can 
# almost write to it. First we need to manually write the BOM that will tell 
# readers of this file what endianness it is in. That's what the 
# print $FH "\x{FEFF}" does.
#
# Finally we can just print text out.
#
# If you want UTF8, I'm pretty sure it's a lot easier. Also, this is also a lot 
# easier on unix, the CRLF ordering problem is definitely a bug but the default 
# to big endian (and ensuing games to get the BOM to output without a warning) 
# are by design. I'm pretty sure that none of the core perl maintainers use perl 
# on Windows (even though at least one keeps perl on VMS working...).
#

open( DATAFILE, ">$DATAFILE" ) || die "Cannot open $DATAFILE: ";
open( DISPFILE, ">$DISPFILE" ) || die "Cannot open $DISPFILE: ";
open( SYMFILE, ">$SYMFILE" ) || die "Cannot open $SYMFILE: ";

# Note this "open in Unicode mode" code doesn't work in perl 5.6.x-ish, but does 
# work as early as perl 5.8.1.  See above for an explanation of the ">:raw:enc..."
open( INIFILE, ">:raw:encoding(UTF16-LE):crlf:utf8", "$INIFILE" ) || die "Cannot open $INIFILE: ";
print INIFILE "\x{FEFF}";  # print BOM (Byte Order Mark) for the unicode file


print DATAFILE<<END_DATAFILE_PROLOG;

/*  Machine generated data file \"$DATAFILE_nopath\" from \"$inputfile\" */


#include <stddef.h>
#pragma prefast(push)
#pragma prefast(disable:26006, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:26007, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:28718, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:28726, "Dont bother us with tchar, someone else owns that.")
#include <tchar.h>
#pragma prefast(pop)
#include <windows.h>
#include <winperf.h>

#include "perfmon.hxx"

#define SIZEOF_INST_NAME QWORD_MULTIPLE($ModuleInfo{"MaxInstNameSize"})

#pragma pack(4)

const wchar_t wszPERFVersion[] =  L\"$modulename Performance Data Schema Version $ModuleInfo{"SchemaVersion"}\";

typedef struct _PERF_DATA_TEMPLATE
	{

END_DATAFILE_PROLOG



print SYMFILE<<END_SYMFILE_PROLOG;
/*  Machine generated symbol file \"$SYMFILE_nopath\" from \"$inputfile\" */


END_SYMFILE_PROLOG



print INIFILE<<END_INIFILE_PROLOG;
;Machine generated INI file \"$INIFILE_nopath\" from \"$inputfile\"


[info]
drivername=$modulename
symbolfile=$SYMFILE_nopath

[languages]
END_INIFILE_PROLOG

for( $CurLang = 1; $CurLang <= $nLang; $CurLang++ ) {
	print INIFILE "${Language_ID[$CurLang]}=${Language_Name[$CurLang]}\n";
}
print INIFILE "000=Neutral\n";
print INIFILE "\n";
print INIFILE "[objects]\n";
for( $CurObj = 1; $CurObj <= $nObj; $CurObj++) {
	print INIFILE "$Object_NameText_Tag[$CurObj]=$Object_NameText_Name[$CurObj]\n";
}
print INIFILE "\n";
print INIFILE "[text]\n";
for( $CurNameText = 1; $CurNameText <= $nNameText; $CurNameText++ ) {
	print INIFILE "$NameText_Tag[$CurNameText]=$NameText_Name[$CurNameText]\n";
}
for( $CurHelpText = 1; $CurHelpText <= $nHelpText; $CurHelpText++ ) {
	print INIFILE "$HelpText_Tag[$CurHelpText]=$HelpText_Text[$CurHelpText]\n";
}
print INIFILE "\n";
for( $CurNameText = 1; $CurNameText <= $nNameText; $CurNameText++ ) {
	$temp = "$NameText_Tag[$CurNameText]=$NameText_Name[$CurNameText]\n";
	$temp =~ s/009/000/;
	print INIFILE $temp;
}
for( $CurHelpText = 1; $CurHelpText <= $nHelpText; $CurHelpText++ ) {
	$temp = "$HelpText_Tag[$CurHelpText]=$HelpText_Text[$CurHelpText]\n";
	$temp =~ s/009/000/;
	print INIFILE $temp;
}


print DISPFILE<<END_DISPFILE_PROLOG;
/*  Machine generated data file \"$DISPFILE_nopath\" from \"$inputfile\"  */


#ifdef MINIMAL_FUNCTIONALITY
#else

#include <stddef.h>
#include <tchar.h>

#include "perfmon.hxx"

#pragma pack(4)

END_DISPFILE_PROLOG

$CurIndex = 0;
for( $CurObj = 1; $CurObj <= $nObj; $CurObj++) {
	
	print DATAFILE "\tPERF_OBJECT_TYPE pot${Object_SymName[$CurObj]};\n";
	$Object_NameIndex[$CurObj] = $CurIndex;
	print SYMFILE "#define $Object_SymName[$CurObj] $Object_NameIndex[$CurObj]\n";
	$CurIndex = $CurIndex + 2;

	for( $CurCtr = 1; $CurCtr <= $nCtr; $CurCtr++ ) {
		if( $Counter_Object[$CurCtr] eq $Object_SymName[$CurObj]) {
			$Object_nCounters[$CurObj]++;
			print DATAFILE "\tPERF_COUNTER_DEFINITION pcd${Counter_SymName[$CurCtr]};\n";	
			$Counter_NameIndex[$CurCtr] = $CurIndex;
			print SYMFILE "#define $Counter_SymName[$CurCtr] $Counter_NameIndex[$CurCtr]\n";
			$CurIndex = $CurIndex + 2;
		}		
	}

	if( 0 == $Object_nCounters[$CurObj] ) {
		warn "$inputfile: error:  Object \"$Object_SymName[$CurObj]\" has no counters";
		$errors++;
	}
		

	print DATAFILE "\tPERF_INSTANCE_DEFINITION pid${Object_SymName[$CurObj]}0;\n";
	print DATAFILE "\twchar_t wsz${Object_SymName[$CurObj]}InstName0[SIZEOF_INST_NAME];\n";
}

print DATAFILE "\tDWORD EndStruct;\n";
print DATAFILE "\t} PERF_DATA_TEMPLATE;\n";
print DATAFILE "\n";

print DATAFILE "// Offset for the counters\n";
for( $CurObj = 1; $CurObj <= $nObj; $CurObj++ ) {
	FINDCTR: for( $CurCtr = 1; $CurCtr <= $nCtr; $CurCtr++) {
		if( $Counter_Object[$CurCtr] eq $Object_SymName[$CurObj] ) {
			last FINDCTR;
		}
	}
	
	# maintain the offset for most counters. This was done by perfmon.cxx\ErrUtilPerfInit()
	# but was moved here to prevent COW faults during dll initialization.
	# The code in ErrUtilPerfInit() was left in place and is used to
	# double check this perl script. The offset is the sum of the sizes
	# of all previous counters.
	$LastOff = "QWORD_MULTIPLE( sizeof(PERF_COUNTER_BLOCK) )";
	$LastCtr = $CurCtr;
	for( $CurCtr = $LastCtr + 1; $CurCtr <= $nCtr; $CurCtr++) {
		if( $Counter_Object[$CurCtr] eq $Object_SymName[$CurObj]) {
			print DATAFILE "const int off${Counter_SymName[$LastCtr]} = QWORD_MULTIPLE(CntrSize(${Counter_Type[$LastCtr]},${Counter_Size[$LastCtr]})) + $LastOff;\n";

			# offset for the next counter
			$LastOff = "off${Counter_SymName[$LastCtr]}";
			$LastCtr = $CurCtr;
		}
	}
}
print DATAFILE "\n";

sub PopulatePerfDataTemplate
{
   for( $CurObj = 1; $CurObj <= $nObj; $CurObj++ ) {
	if( $CurObj < $nObj) {
		$NextObjSym = "pot" . $Object_SymName[$CurObj+1];
	}
	else {
		$NextObjSym = "EndStruct";
	}

	FINDCTR: for( $CurCtr = 1; $CurCtr <= $nCtr; $CurCtr++) {
		if( $Counter_Object[$CurCtr] eq $Object_SymName[$CurObj] ) {
			last FINDCTR;
		}
	}

	print DATAFILE<<EOF2;
	{  //  PERF_OBJECT_TYPE pot${Object_SymName[$CurObj]};
		PerfOffsetOf(PERF_DATA_TEMPLATE,$NextObjSym)-PerfOffsetOf(PERF_DATA_TEMPLATE,pot${Object_SymName[$CurObj]}),
		PerfOffsetOf(PERF_DATA_TEMPLATE,pid${Object_SymName[$CurObj]}0)-PerfOffsetOf(PERF_DATA_TEMPLATE,pot${Object_SymName[$CurObj]}),
		PerfOffsetOf(PERF_DATA_TEMPLATE,pcd${Counter_SymName[$CurCtr]})-PerfOffsetOf(PERF_DATA_TEMPLATE,pot${Object_SymName[$CurObj]}),
		$Object_NameIndex[$CurObj],
		0,
		$Object_NameIndex[$CurObj],
		0,
		0,
		$Object_nCounters[$CurObj],
		0,
		0,
		0,
		0,
		0,
	},
EOF2

	# maintain the offset for first perf counters in each object type
	$LastOff = "QWORD_MULTIPLE( sizeof(PERF_COUNTER_BLOCK) )";
	$LastCtr = $CurCtr;
	for( $CurCtr = $LastCtr + 1; $CurCtr <= $nCtr; $CurCtr++) {
		if( $Counter_Object[$CurCtr] eq $Object_SymName[$CurObj]) {

			print DATAFILE<<EOF3;
	{  //  PERF_COUNTER_DEFINITION pcd${Counter_SymName[$LastCtr]};
		PerfOffsetOf(PERF_DATA_TEMPLATE,pcd${Counter_SymName[$CurCtr]})-PerfOffsetOf(PERF_DATA_TEMPLATE,pcd${Counter_SymName[$LastCtr]}),
		$Counter_NameIndex[$LastCtr],
		0,
		$Counter_NameIndex[$LastCtr],
		0,
		$Counter_DefaultScale[$LastCtr],
		$Counter_DetailLevel[$LastCtr],
		$Counter_Type[$LastCtr],
		CntrSize(${Counter_Type[$LastCtr]},${Counter_Size[$LastCtr]}),
		${LastOff},
	},
EOF3
			# offset for the next counter
			$LastOff = "off${Counter_SymName[$LastCtr]}";
			$LastCtr = $CurCtr;
		}
	}

	print DATAFILE<<EOF4;
	{  //  PERF_COUNTER_DEFINITION pcd${Counter_SymName[$LastCtr]};
		PerfOffsetOf(PERF_DATA_TEMPLATE,pid${Object_SymName[$CurObj]}0)-PerfOffsetOf(PERF_DATA_TEMPLATE,pcd${Counter_SymName[$LastCtr]}),
		$Counter_NameIndex[$LastCtr],
		0,
		$Counter_NameIndex[$LastCtr],
		0,
		$Counter_DefaultScale[$LastCtr],
		$Counter_DetailLevel[$LastCtr],
		$Counter_Type[$LastCtr],
		CntrSize(${Counter_Type[$LastCtr]},${Counter_Size[$LastCtr]}),
		${LastOff},
	},

	{  //  PERF_INSTANCE_DEFINITION pid${Object_SymName[$CurObj]}0;
		PerfOffsetOf(PERF_DATA_TEMPLATE,$NextObjSym)-PerfOffsetOf(PERF_DATA_TEMPLATE,pid${Object_SymName[$CurObj]}0),
		0,
		0,
		0,
		PerfOffsetOf(PERF_DATA_TEMPLATE,wsz${Object_SymName[$CurObj]}InstName0)-PerfOffsetOf(PERF_DATA_TEMPLATE,pid${Object_SymName[$CurObj]}0),
		0,
	},
	L"",  //  wchar_t wsz${Object_SymName[$CurObj]}InstName0[];
EOF4
   } # end for
} # end sub

print DATAFILE "// This copy of the PERF_DATA_TEMPLATE array will be linked in to ese/esent.dll, and\n";
print DATAFILE "// is a read-only version, so that it will not be COW'ed.\n";

print DATAFILE "const PERF_DATA_TEMPLATE PerfDataTemplateReadOnly = \n\t{\n";
PopulatePerfDataTemplate();

print DATAFILE<<EOF5;
	0,  //  DWORD EndStruct;
	};


const DWORD dwPERFNumObjects = $nObj;

// This copy of the PERF_DATA_TEMPLATE array will be linked in to eseperf/esentprf.dll, and
// is a read/write copy.
EOF5


print DATAFILE "DECLSPEC_SELECTANY PERF_DATA_TEMPLATE PerfDataTemplateReadWrite = \n\t{\n";
PopulatePerfDataTemplate();

print DATAFILE<<EOF9;
	0,  //  DWORD EndStruct;
	};

// These two objects should be the same.
C_ASSERT( sizeof( PerfDataTemplateReadOnly ) == sizeof( PerfDataTemplateReadWrite ) );

EOF9

if( 0 == $nObj ) {
	$NumObj = 1;
} else {
	$NumObj = $nObj;
}

$MaxIndex = $CurIndex-2;
if( $MaxIndex < 0) {
	$MaxIndex = 0;
}

print DATAFILE<<EOF6;
long rglPERFNumInstances[$NumObj];
wchar_t* rgwszPERFInstanceList[$NumObj];
unsigned char* rgpbPERFInstanceAggregationIDs[$NumObj];

const DWORD dwPERFNumCounters = $nCtr;

const DWORD dwPERFMaxIndex = $MaxIndex;

EOF6

print DISPFILE "extern PM_PIF_PROC $ProcessInfo{\"ProcessInfoFunction\"};\n";
print DISPFILE "\n";

for( $CurObj = 1; $CurObj <= $nObj; $CurObj++) {
	print DISPFILE "extern PM_ICF_PROC $Object_ICF[$CurObj];\n";
}
print DISPFILE "\n";
print DISPFILE "PM_ICF_PROC* const rgpicfPERFICF[] = \n\t{\n";
for( $CurObj = 1; $CurObj <= $nObj; $CurObj++ ) {
	print DISPFILE "\t${Object_ICF[$CurObj]},\n";
}
if ( !$nObj ) {
	print DISPFILE "\tNULL,\n";
}
print DISPFILE "\t};\n";
print DISPFILE "\n";
	
for( $CurCtr = 1; $CurCtr <= $nCtr; $CurCtr++ ) {
	print DISPFILE "extern PM_CEF_PROC ${Counter_CEF[$CurCtr]};\n";
}
print DISPFILE "\n";
print DISPFILE "PM_CEF_PROC* const rgpcefPERFCEF[] = \n\t{\n";
for( $CurCtr = 1; $CurCtr <= $nCtr; $CurCtr++) {
	print DISPFILE "\t${Counter_CEF[$CurCtr]},\n";
}
if ( !$nCtr ) {
	print DISPFILE "\tNULL,\n";
}
print DISPFILE "\t};\n";
print DISPFILE "\n";

print DISPFILE "#endif // MINIMAL_FUNCTIONALITY\n";
print DISPFILE "\n";

if( 0 != $errors ) {
	 die "$errors error(s) encountered creating output files\n";
}

