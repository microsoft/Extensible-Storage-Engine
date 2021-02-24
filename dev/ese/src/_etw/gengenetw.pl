# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# helpful for extra info on what might be going wrong, but too noisy
# use warnings;
# use strict;

#
#	Process Args and Open input / output files.
#

my $iPrintLevel = 0;

# $#ARGV is the index of the last element.
if ( $#ARGV < 2 )
{
	print "Provided number of arguments $#ARGV is too small.\n";
	die "Usage: genetw.pl [dev root path] [test root path] [TagForGenGenTempFile]";
}

my $szDevRoot = $ARGV[0];
my $szTestRoot = $ARGV[1];
my $szEseBuildTempExt = $ARGV[2];

# Input files
#
my $szEtwPregenData  = $szDevRoot . "\\src\\_etw\\EseEtwEventsPregen.txt";
my $szEtwBaseMc      = $szDevRoot . "\\src\\_etw\\Microsoft-ETW-ESE.mc";

open( hEtwPregenData, "<$szEtwPregenData" ) or die "Cannot open the ESE ETW data file ($szEtwPregenData).";
open( hEtwBaseMc, "<$szEtwBaseMc" ) or die "Cannot open the ESE ETW Base .mc file ($szEtwBaseMc).";

# Output files
#
my $szEtwNewMc                     = "src\\_etw\\Microsoft-ETW-ESE.mc.$szEseBuildTempExt";	# The main ETW event .mc file.

my $szOsEventTraceHxxFileName      = "oseventtrace.g.hxx";
my $szOsEventTraceImplHxxFileName  = "_oseventtraceimpl.g.hxx";
my $szEseEventTraceTypeHxxFileName = "EseEventTraceType.g.hxx";
my $szEseEventTraceHxxFileName     = "EseEventTrace.g.hxx";
my $szEseEtwParserForCPRFilename   = "Microsoft-ETW-ESE.g.cs";

my $szOsEventTraceHxxHdrIns        = "published\\inc\\os\\$szOsEventTraceHxxFileName.$szEseBuildTempExt";       #  Just the enum constant names to be included in oseventtrace.hxx.
my $szOsEventTraceHxxImplIns       = "src\\os\\_oseventtraceimpl.g.hxx.$szEseBuildTempExt";                     #  The weakly-typed ETW_Xxxx_Trace() functions and the g_rgOSEventTraceGUID table entries.
my $szEseEventTraceTypeHxx         = "published\\inc\\os\\$szEseEventTraceTypeHxxFileName.$szEseBuildTempExt";  #  The EseXxxxTrace structures for easying passing of arguments in test processing.
my $szEseEventTraceHxx             = "published\\inc\\os\\$szEseEventTraceHxxFileName.$szEseBuildTempExt";      #  The strongly-typed ETXxxxx() trace functions and infrastructure.
my $szEseEtwParserForCPR           = "src\\_etw\\$szEseEtwParserForCPRFilename.$szEseBuildTempExt";             # The stronlgy-typed parser code for parsing ESE Etw events being profiled by CPR agent

open( hEtwNewMc, ">$szEtwNewMc" ) or die "Cannot open the destination file ($szEtwNewMc).";
open( hOsEventTraceHxxHdrIns, ">$szOsEventTraceHxxHdrIns" ) or die "Cannot open the destination file ($szOsEventTraceHxxHdrIns).";
open( hOsEventTraceHxxImplIns, ">$szOsEventTraceHxxImplIns" ) or die "Cannot open the destination file ($szOsEventTraceHxxImplIns).";
open( hEseEventTraceTypeHxx, ">$szEseEventTraceTypeHxx" ) or die "Cannot open the destination file ($szEseEventTraceTypeHxx).";
open( hEseEventTraceHxx, ">$szEseEventTraceHxx" ) or die "Cannot open the destination file ($szEseEventTraceHxx).";
open( hEseEtwParserForCPR, ">$szEseEtwParserForCPR" ) or die "Cannot open the destination file ($szEseEtwParserForCPR).";

if ( $iPrintLevel >= 1 ){
	print "Opened all files.\n";
}


#
#	Helper Sub-Routines
#

sub roundup {
    my $number = shift;
    my $round = shift;

    if ($number % $round) {
        return (1 + int($number/$round)) * $round;
    } else {
        return $number;
    }
}

sub szpad {
    my $szIn = shift;
    my $cchPad = shift;

    $cchPad = roundup( length( $szIn ), $cchPad ) - length( $szIn );
    for ( my $i = 0; $i < $cchPad; $i++ ){
        $szIn .= ' ';
    }

    return $szIn;
}


#
#	Build our bags o' types.
#

my $EtwTypes = {};

$EtwTypes->{"win:Int8"}          = "BYTE";
$EtwTypes->{"win:Int32"}         = "INT";
$EtwTypes->{"win:Int64"}         = "__int64";
$EtwTypes->{"win:UInt8"}         = "BYTE";
$EtwTypes->{"win:UInt16"}        = "USHORT";
$EtwTypes->{"win:UInt32"}        = "DWORD";
$EtwTypes->{"win:UInt64"}        = "QWORD";
$EtwTypes->{"win:Pointer"}       = "const void *";
$EtwTypes->{"win:Double"}        = "double";
# IMPORTANT: Do note these three types are checked below as literal strings.
$EtwTypes->{"win:UnicodeString"} = "PCWSTR"; # "const WCHAR *";
$EtwTypes->{"win:AnsiString"}    = "PCSTR";  # "const CHAR *";
$EtwTypes->{"win:Binary"}        = "const BYTE *";
$EtwTypes->{"win:GUID"}          = "const GUID *";

my $EtwManagedTypes = {};

$EtwManagedTypes->{"win:Int8"}          = "byte";
$EtwManagedTypes->{"win:Int32"}         = "int";
$EtwManagedTypes->{"win:Int64"}         = "long";
# Testing showed that Microsoft.Diagnostics.Tracing always uses signed types (even if the event template specifies unsigned types)
$EtwManagedTypes->{"win:UInt8"}         = "byte";
$EtwManagedTypes->{"win:UInt16"}        = "short";
$EtwManagedTypes->{"win:UInt32"}        = "int";
$EtwManagedTypes->{"win:UInt64"}        = "long";
$EtwManagedTypes->{"win:Pointer"}       = "IntPtr";
$EtwManagedTypes->{"win:Double"}        = "double";
$EtwManagedTypes->{"win:UnicodeString"} = "string";
$EtwManagedTypes->{"win:AnsiString"}    = "string";
$EtwManagedTypes->{"win:Binary"}        = "byte[]";
$EtwManagedTypes->{"win:GUID"}          = "Guid";

if ( $iPrintLevel >= 1 ){
	print "Inited types.\n";
}


#
#		Main Processing happens in these steps:
#
#   1-A. We build the list of events (in memory / @rgsEvents) by processing the first half of EseEtwEventsPregen.txt
#        file to get each ETW events and it's various fields in $rgsEvents[$i]{EventId|Keywords|etc}.
#   1-B. We build the list of fields for each event (in memory / @@rgrgsEventFields) and count of fields (@rgcEventFieldCounts)
#        by processing the second half of EseEtwEventsPregen.txt. Reference with: $rgrgsEventFields[$i]{FieldName|TypeEtw|Keywords|etc}.
#   2-*. Then walk the existing .mc file, dumping each line out to the new .mc file, and re-generating
#        code that is between ESE_ETW_AUTOGEN_*_LIST_BEGIN/_LIST_END pairs for four sub-steps to gen
#        that portion of the .mc file.  See step for more details.
#   4-*. Then we reprocess the events list from #1 to generate all other various auto-generated files in
#        our system one at a time.  There are several of these sub-steps.
#


#   -----------------------------------------------------------------------------------------------------------------------------------------
#
#		Step 1. - Build list of events and event fields for processing.
#
#   -----------------------------------------------------------------------------------------------------------------------------------------

#
#	Step 1-A. - Build list of events (consuming first half of the EseEtwEventsPregen.txt data).
#

if ( $iPrintLevel >= 1 ){
	print "Begin pregen event list data processing ...\n";
}

my $fProcessingTraceList = 0;

while( $szPregenDataLine = <hEtwPregenData> ) {

	chomp( $szPregenDataLine );

	#
	#	Process the product specific (ESE or ESENT) provider inserts ...
	#

	if( $szPregenDataLine =~ /START_TRACE_DEFNS:/ ){
		if ( $iPrintLevel >= 1 ){
			print "\tFinished processing trace list.\n";
		}
		$fProcessingTraceList = 0;
		if ( $iPrintLevel >= 1 ){
			print "\tStarting processing of trace definitions pass 1 ...\n";
		}

		#	We have hit the next section, so bail to 2nd loop / 1-B.
		#
		last;
	}


	#
	#	Add event descriptor to array of events
	#

	if ( $fProcessingTraceList ) {
		if ( $szPregenDataLine=~/^\s*$/ ){
			# all blank lines, skip.
		} elsif( $szPregenDataLine=~/^\/\/.*$/ ){
			# // coment, skip.
		} else {
			@EvtFields = split /\s*,\s*/, $szPregenDataLine;
			@EvtFields[0] =~ s/^\s*//;
			@EvtFields[3] =~ s/\s*$//;
			if ( $iPrintLevel >= 2 ){
				# perhaps should be at print level 3, but very noisy print level ...
				print "\t\t\tEVT{ @EvtFields[0], $EvtFields[1], $EvtFields[2], @EvtFields[3], @EvtFields[4] }\n";
			}

			my $evt = {};
			$evt->{BaseName}  = @EvtFields[0];
			$evt->{EventId}   = @EvtFields[1];
			$evt->{Verbosity} = @EvtFields[2];
			$evt->{Keywords}  = @EvtFields[3];
			if ( @EvtFields[4] =~ /fDeprecated/ ) {
				$evt->{fDeprecated} = 1;
			}
			if ( @EvtFields[4] =~ /fIODiagnoseChannel/ ) {
				$evt->{fIODiagnoseChannel} = 1;
			}
			if ( @EvtFields[4] =~ /fOpChannel/ ) {
				$evt->{fOpChannel} = 1;
			}
			if ( @EvtFields[4] =~ /fStopCodeNoTask/ ) {
				$evt->{fStopCodeNoTask} = 1;
				if ( $iPrintLevel >= 2 ){
					print "\t\t\t\t Found fStopCodeNoTask flag for this / $evt->{EventId} event.\n";
				}
			}
			# $evt->{FieldCount} = x; - will be post computed in trace definition processing below.

			push @rgsEvents, $evt;
		}
	}

	if( $szPregenDataLine =~ /START_TRACE_LIST:/ ){
		$fProcessingTraceList = 1;
		if ( $iPrintLevel >= 1 ){
			print "\tStarting processing of trace list ...\n";
		}
	}

}


#
#	Step 1-B - Build list of events's field arrays (consuming second half of the EseEtwEventsPregen.txt data).
#

my $iEvt = 0;
my $cEvtArg = 0;

# Note: we're already positioned half way through the file
while( $szPregenDataLine = <hEtwPregenData> ) {

	#
	#	Process each line of file - identifying between "Define:xxx" event headers and "<data ...> event fields.
	#

	if ( $iPrintLevel >= 4 ){
		print "INPUT:\t\t trace defn input line: $szPregenDataLine";
	}

	if ( $szPregenDataLine=~/^\s*$/ ){
		# all blank lines, skip.
	} elsif( $szPregenDataLine=~/^\s*\/\/.*$/ ){
		# // coment, skip.
	} elsif( $szPregenDataLine=~/^\s*Define:\ .*$/ ){

		chomp( $szPregenDataLine );

		my $szTraceName = $szPregenDataLine;
		$szTraceName =~ s/^\s*Define:\ //;

		if ( $iPrintLevel >= 3 ){
			print "DEBUG:\t\t start processing:$szTraceName:.\n";
		}

		if ( $rgsEvents[$iEvt]{BaseName} ne $szTraceName ){

			#
			#		Done recieving field specifiers for current event: $rgsEvents[$iEvt]{BaseName} and starting a NEW event: $szTraceName
			#

			#	Update the structured lists for the current event
			#

			my $evtCheck = {};
			$evtCheck->{BaseName}     = $rgsEvents[$iEvt]{BaseName};
			$evtCheck->{FieldCount}   = $cEvtArg;

			# Can't seem to update this, sure it has to do with some clever perl reference thing. Just use a 2nd array. :P
			#$rgsEvents[$iEvt]{FieldCount} = $cEvtArg;
			$rgcEventFieldCounts[$iEvt] = $cEvtArg;
			push @rgsEventsCheck, $evtCheck;
			push @rgrgsEventFields, [ @rgsFieldsT ];

			#	Reset accumluated fields
			#

			@rgsFieldsT=();
			$cEvtArg = 0;

			#	Increment to next event in @rgsEvents array
			#

			$iEvt = $iEvt + 1;

			#	Check to make sure still on track for %sz.
			#

			if ( $szTraceName eq "InvalidEndDefinitionsMarker" ){
				# Note: Dealing with last event, not a real Trace defn
			} elsif ( $rgsEvents[$iEvt]{BaseName} ne $szTraceName ){

				die "ERROR: The $rgsEvents[$iEvt]{BaseName} does not match $szTraceName - sure both lists in data file are in same order!?\n\n";
			}

		} elsif( $iEvt == 0 ) {
			# This is fine, first iteration around, it correctly skips previous clause.
		} else {
			die "ERROR: Unknown trace name at $iEvt : $rgsEvents[$iEvt]{BaseName} == $szTraceName.  Are you sure both lists in data file are in the same order? \n";
		}

		if ( $szTraceName eq "InvalidEndDefinitionsMarker" ){
			# DONE DONE with per event + all field trace processing.
			if ( $iPrintLevel >= 2 ){
				print "DONE PHASE TWO.\n";
			}
			last;
		}

	} else {
		#
		#		Got new field specifier for $szTraceName / $rgsEvents[$iEvt]
		#

		# This will (or SHOULD ;) be a <data xxx /> type field spec, turn it into a field structure for appending
		# to the @rgrgsEventFields array later (though above here in code).

		if ( $szPregenDataLine =~ /^\s*\<data / ){

			chomp( $szPregenDataLine );

			# printf "DEBUG: \t args: $iArg : $szPregenDataLine \n";

			$szEtwType = $szPregenDataLine;
			$szEtwType =~ s/^.*inType=\"//;
			$szEtwType =~ s/\".*$//;

			$szEseVar = $szPregenDataLine;
			$szEseVar =~ s/^.*name=\"//;
			$szEseVar =~ s/\".*$//;

			$szEseType = $szEseVar;
			$szEseType =~ s/[A-Z].*$//;

			my $fldT = {};
			$fldT->{FullMcLine}    = $szPregenDataLine;
			$fldT->{FieldName}     = $szEseVar;
			$fldT->{TypeEtw}       = $szEtwType;
			$fldT->{TypeEse}       = $szEseType;

			push @rgsFieldsT, $fldT;

			$cEvtArg = $cEvtArg + 1;

		} else {
			# Perhaps some day, someone will want to pass through a:
			#	<!-- comment -->
			# type XML line to the .mc file.  Would then suggest you append the comment line to the previously
			# accumulated $fldT->{FullMcLine} with line return so when we print it to the .mc file, it just
			# flows through
			print "Unknown input line from Pregen.txt file: $szPregenDataLine \n";
			die "ERROR: Did not understand input field line from $szEtwPregenData.\n";
		}

	}

} # while

if ( $iPrintLevel >= 1 ){
	printf "\tValidating Events+Fields collected from ...Pregen.txt file.\n";
}
for $i ( 0 ... $#rgsEventsCheck ){

	if ( $iPrintLevel >= 2 ){
		print "\t\tEtwEvent[$i]       = { $rgsEvents[$i]{BaseName}, $rgsEvents[$i]{EventId}, $rgsEvents[$i]{Verbosity}, $rgsEvents[$i]{Keywords},  }\n";
	}

	if ( 0 != ( $rgsEvents[$i]{BaseName} cmp $rgsEventsCheck[$i]{BaseName} ) ){
		die "ERROR: The rgsEvents->BaseName and rgsEventsCheck->BaseName do not have the same event name, but they should be.\n";
	}

	if ( $rgsEventsCheck[$i]{FieldCount} != $rgcEventFieldCounts[$i] ){
		die "ERROR: The rgsEventsCheck->FieldCount and rgcEventFieldCounts do not have the same field count, but they should be.\n";
	}

	# Note these are off by 1, $#{$rgrgsEventFields[$i]} being the max value - 1
	if ( $rgsEventsCheck[$i]{FieldCount} != ( $#{$rgrgsEventFields[$i]} + 1 ) ){
		print "ERROR: The $rgsEventsCheck[$i]{FieldCount} and $#{$rgrgsEventFields[$i]} do not have the same field count, but they should be.\n";
		die "ERROR: The rgsEventsCheck->FieldCount and # { rgrgsEventFields[ i ] } do not have the same field count, but they should be.\n";
	}

	for $iField ( 0 ... $#{$rgrgsEventFields[$i]} ){
		if ( $iPrintLevel >= 2 ){
			print "\t\t\tEtwEvent[$i].EvtFld[$iField] = \{ $rgrgsEventFields[$i][$iField]{FieldName}, $rgrgsEventFields[$i][$iField]{TypeEse}, $rgrgsEventFields[$i][$iField]{TypeEtw} \} \n";
		}
	}

}

if ( $#rgsEvents != $#rgsEventsCheck ){
	die "ERROR: The rgsEvents and rgsEventsCheck are not the same array size, but they should be.\n";
}
if ( $#rgsEvents != $#rgcEventFieldCounts ){
	die "ERROR: The rgsEvents and rgcEventFieldCounts are not the same array size, but they should be.\n";
}


#   -----------------------------------------------------------------------------------------------------------------------------------------
#
#		Step 2 - Main Processing (for .mc file)
#
#   -----------------------------------------------------------------------------------------------------------------------------------------

# It walks through the .mc file looking for four insert markers ...
#   A. ESE_ETW_AUTOGEN_TASK_LIST_BEGIN - The <task name="xxx" value="yyy" /> entries.
#   B. ESE_ETW_AUTOGEN_TEMPLATE_LIST_BEGIN - The templates and where we process the rest of the hEtwPregenData for the template
#      arguments.
#   C. ESE_ETW_AUTOGEN_EVENT_LIST_BEGIN - The <event ...> entries.
#   D. ESE_ETW_AUTOGEN_STRING_LIST_BEGIN - The event's matching <string ...> entries.
#


if ( $iPrintLevel >= 1 ){
	print "\tBegin .mc file insert processing ...\n";
}


# Note: The event value boost is 99 before / lower than task 46 / event 145, and then 100 after because of the weird ApiCall task is
# split to two events with different opcode= 'start' and 'stop' signifiers.
my $iEventValueBoost = 99;

my $fInTaskList = 0;
my $fInTemplateList = 0;
my $fInEventList = 0;
my $fInStringList = 0;

my $fPastFirst = 0;
my $fCurrInsertDone = 0;

while( $szMcLine = <hEtwBaseMc> ) {

	#	We pass through almost all lines, without any alteration.

	# We have to pass through to new.mc all these marker lines, as we need it the next time this script runs as well. ;)
	if ( $szMcLine =~ /<!--ESE_ETW_AUTOGEN_TASK_LIST_BEGIN-->/ ){
		print hEtwNewMc $szMcLine;
		$fInTaskList = 1;
	}
	if ( $szMcLine =~ /<!--ESE_ETW_AUTOGEN_TEMPLATE_LIST_BEGIN-->/ ){
		print hEtwNewMc $szMcLine;
		$fInTemplateList = 1;
	}
	if ( $szMcLine =~ /<!--ESE_ETW_AUTOGEN_EVENT_LIST_BEGIN-->/ ){
		print hEtwNewMc $szMcLine;
		$fInEventList = 1;
	}
	if ( $szMcLine =~ /<!--ESE_ETW_AUTOGEN_STRING_LIST_BEGIN-->/ ){
		print hEtwNewMc $szMcLine;
		$fInStringList = 1;
	}

	if ( $fInTaskList ){

		if ( !$fCurrInsertDone ){

			print hEtwNewMc "            <!-- DO NOT EDIT = THESE TASKS AUTO-GEN (from EseEtwEventsPregen.txt) -->\n";
			for $i ( 0 .. $#rgsEvents ){

				# I do not know why or if it is important that all these values are -99 or -100 under their event id values but for
				# for now we'll just match it.  Or maybe it is important the event ids are +100 / non-overlapping with task ids.

				$iTaskId = $rgsEvents[$i]{EventId} - $iEventValueBoost;

				if ( $rgsEvents[$i]{fStopCodeNoTask} eq 1 ){
					$iEventValueBoost++;
				} else {

					$szTaskName = $rgsEvents[$i]{BaseName};
					$szTaskName =~ s/_Start//;

					print hEtwNewMc "          <task\n";
					print hEtwNewMc "              name=\"ESE_$szTaskName\_Trace\"\n";
					print hEtwNewMc "              value=\"$iTaskId\"\n";
					print hEtwNewMc "              />\n";
					# Migrate to better list format?  Be helpful to know width of trace BaseName so we could size it consistently though.
					# print hEtwNewMc "              <task  name=\"ESE_$szTaskName\_Trace\"  value=\"$iTaskId\"  />\n";
				}
			}
			$fCurrInsertDone = 1;
		}

		if ( $szMcLine =~ /<!--ESE_ETW_AUTOGEN_TASK_LIST_END-->/ ){
			print hEtwNewMc $szMcLine;
			$fInTaskList = 0;
			$fCurrInsertDone = 0; # reset this for the next list to be processed
		}

	} elsif ( $fInTemplateList ){

		if ( !$fCurrInsertDone ){

			print hEtwNewMc "          <!-- DO NOT EDIT = THESE TEMPLATES AUTO-GEN (from EseEtwEventsPregen.txt) -->\n";

			for $i ( 0 ... $#rgsEvents ){

				if ( $rgcEventFieldCounts[$i] ){

					print hEtwNewMc "          <template tid=\"t$rgsEventsCheck[$i]{BaseName}" . "Trace\">\n";

					if ( $iPrintLevel >= 2 ){
						print "\t\tEVENT[$i]: ... Name = $rgsEventsCheck[$i]{BaseName} == $rgsEvents[$i]{BaseName}, FieldCount = $rgsEventsCheck[$i]{FieldCount}.\n";
					}

					for $iField ( 0 ... $#{$rgrgsEventFields[$i]} ){
						print hEtwNewMc "$rgrgsEventFields[$i][$iField]{FullMcLine}\n";
					}

					print hEtwNewMc "          </template>\n";

				} else {
					if ( $iPrintLevel >= 2 ){
						print "\t\tEVENT[$i]: ... Name = $rgsEventsCheck[$i]{BaseName} == $rgsEvents[$i]{BaseName}, FieldCount = $rgsEventsCheck[$i]{FieldCount}      -------------->  SKIP TEMPLATE OF DUE TO 0 FIELDS.\n";
					}
				}

			}

			$fCurrInsertDone = 1;
		}

		if ( $szMcLine =~ /<!--ESE_ETW_AUTOGEN_TEMPLATE_LIST_END-->/ ){
			print hEtwNewMc $szMcLine;
			$fInTemplateList = 0;
			$fCurrInsertDone = 0; # reset this for the next list to be processed
		}


	} elsif ( $fInEventList ){

		if ( !$fCurrInsertDone ){
			print hEtwNewMc "            <!-- DO NOT EDIT = THESE EVENTS AUTO-GEN (from EseEtwEventsPregen.txt) -->\n";

			for $i ( 0 .. $#rgsEvents ){

				print hEtwNewMc "          <event\n";
				print hEtwNewMc "              keywords=$rgsEvents[$i]{Keywords}\n";	# note the keywords field already has quotes.
				print hEtwNewMc "              level=\"$rgsEvents[$i]{Verbosity}\"\n";
				print hEtwNewMc "              message=\"\$(string.Event.ESE_$rgsEvents[$i]{BaseName}_Trace)\"\n";
				print hEtwNewMc "              symbol=\"ESE_$rgsEvents[$i]{BaseName}_Trace\"\n";
				$szTaskName = $rgsEvents[$i]{BaseName};
				$szTaskName =~ s/_Start//;
				$szTaskName =~ s/_Stop//;
				print hEtwNewMc "              task=\"ESE_$szTaskName\_Trace\"\n";
				if ( $rgsEvents[$i]{BaseName} =~ /_Start/ ) {
					print hEtwNewMc "              opcode=\"win:Start\"\n";
				}
				if ( $rgsEvents[$i]{BaseName} =~ /_Stop/ ) {
					print hEtwNewMc "              opcode=\"win:Stop\"\n";
				}
				if ( $rgcEventFieldCounts[$i] > 0 ){
					print hEtwNewMc "              template=\"t$rgsEvents[$i]{BaseName}Trace\"\n";
				}
				print hEtwNewMc "              value=\"$rgsEvents[$i]{EventId}\"\n";
				if ( $rgsEvents[$i]{fIODiagnoseChannel} eq 1 ){
					print hEtwNewMc "              channel=\"IODiagnoseChannel\"\n";
				}
				if ( $rgsEvents[$i]{fOpChannel} eq 1 ){
					print hEtwNewMc "              channel=\"OpChannel\"\n";
				}
				print hEtwNewMc "              />\n";
			}
			$fCurrInsertDone = 1;
		}

		if ( $szMcLine =~ /<!--ESE_ETW_AUTOGEN_EVENT_LIST_END-->/ ){
			print hEtwNewMc $szMcLine;
			$fInEventList = 0;
			$fCurrInsertDone = 0; # reset this for the next list to be processed
		}

	} elsif  ( $fInStringList ){

		if ( !$fCurrInsertDone ){
			print hEtwNewMc "            <!-- DO NOT EDIT = THESE STRINGS AUTO-GEN (from EseEtwEventsPregen.txt) -->\n";

			for $i ( 0 .. $#rgsEvents ){

				# NOTE: Slight regression here, for some value this generated value is much less friendly.
				print hEtwNewMc "        <string\n";
				print hEtwNewMc "            id=\"Event.ESE_$rgsEvents[$i]{BaseName}_Trace\"\n";
				print hEtwNewMc "            value=\"ESE $rgsEvents[$i]{BaseName} Trace\"\n";
				print hEtwNewMc "            />\n";
				# Migrate to better list format?  Be helpful to know width of trace BaseName so we could size it consistently though.
				# print hEtwNewMc "        <string  id=\"Event.ESE_$rgsEvents[$i]{BaseName}_Trace\"  value=\"ESE $rgsEvents[$i]{BaseName} Trace\"  />\n";

			}
			$fCurrInsertDone = 1;
		}

		if ( $szMcLine =~ /<!--ESE_ETW_AUTOGEN_STRING_LIST_END-->/ ){
			print hEtwNewMc $szMcLine;
			$fInStringList = 0;
			$fCurrInsertDone = 0; # reset this for the next list to be processed
		}

	} else {
		# else - we pass the $line through unaltered for printing
		print hEtwNewMc $szMcLine;
	}

}


#   -----------------------------------------------------------------------------------------------------------------------------------------
#
#		Step 3 - Reprocessing events / fields for other code files.
#
#   -----------------------------------------------------------------------------------------------------------------------------------------

#
#	Step 3-A - Create oseventtrace.g.hxx.
#

print hOsEventTraceHxxHdrIns <<__OSEVENTTRACEHXXPROLOG__;
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// This file $szOsEventTraceHxxFileName is generated by gengen.bat / gengenetw.pl using EseEtwEventsPregen.txt

//  Event Tracing Enum
//

enum OSEventTraceGUID
{

__OSEVENTTRACEHXXPROLOG__

for $i ( 0 .. $#rgsEvents ){
	if ( $iPrintLevel >= 3 ){
		print "\tDEBUG: id= Event.ESE_$rgsEvents[$i]{BaseName}  - $rgsEvents[$i]{FieldCound} - $rgcEventFieldCounts[$i] done.\n";
	}

	# example: etguidFirstDirtyPage,
	print hOsEventTraceHxxHdrIns "    _etguid$rgsEvents[$i]{BaseName},\n";

}

print hOsEventTraceHxxHdrIns <<__OSEVENTTRACEHXXEPILOG__;

    etguidOsTraceBase   // general tags autogen'd before this one
};

__OSEVENTTRACEHXXEPILOG__


#
#	Step 3-B - Create EseEventTraceType.g.hxx
#

print hEseEventTraceTypeHxx <<__ESEEVENTTRACETYPEHXXPROLOG__;
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// This file $szEseEventTraceTypeHxxFileName is generated by gengen.bat / gengenetw.pl using EseEtwEventsPregen.txt

__ESEEVENTTRACETYPEHXXPROLOG__

for $i ( 0 ... $#rgsEventsCheck ){



	$szTraceStructType = "Ese$rgsEventsCheck[$i]{BaseName}" . "Native";
	$szEseEventTraceTypeHxxStruct = "typedef struct _$szTraceStructType\n{\n";

	for $iField ( 0 ... $#{$rgrgsEventFields[$i]} ){
		$szEseEventTraceTypeHxxStruct = $szEseEventTraceTypeHxxStruct . szpad( "    $EtwTypes->{$rgrgsEventFields[$i][$iField]{TypeEtw}} ", 4 ) . "    $rgrgsEventFields[$i][$iField]{FieldName};\n";
	}

	$szEseEventTraceTypeHxxStruct = $szEseEventTraceTypeHxxStruct . "} $szTraceStructType;\n";

	print hEseEventTraceTypeHxx "$szEseEventTraceTypeHxxStruct\n";
}

#
#	Step 3-C - Create EseEventTrace.g.hxx
#

print hEseEventTraceHxx <<__ESEEVENTTRACEHXXPROLOG__;
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// This file $szEseEventTraceHxxFileName is generated by gengen.bat / gengenetw.pl using EseEtwEventsPregen.txt

#include "EseEventTraceType.g.hxx"

#ifdef DEBUG
inline void FakeUseTrace( void * pv )
{
}
#endif

__ESEEVENTTRACEHXXPROLOG__

for $iEvt ( 0 ... $#rgsEventsCheck ){

	# example: of $szTraceHeader and $szEseEventTraceHxxUntypedTraceCall,
	#     INLINE void ETFirstDirtyPage(
	#         DWORD    tick,
	#         DWORD    ifmp,
	#         DWORD    pgno,
	#         ...
	#         BYTE    iorf,
	#         BYTE    tce )
	#         {
	#
	##ifdef DEBUG
	#     EseCacheFirstDirtyPageNative ettT = {
	#         tick,
	#         ifmp,
	#         pgno,
	#         ...
	#         iorf,
	#         tce };
	#     FakeUseTrace( (void*)&ettT );
	##endif // DEBUG
	#	     OSEventTrace(
	#             etguidFirstDirtyPage,
	#             19,
	#             *(DWORD *)&tick,
	#             *(DWORD *)&ifmp,
	#             *(DWORD *)&pgno,
	#             ...
	#             *(BYTE *)&iorf,
	#             *(BYTE *)&tce );
	#         }

	$szTraceName = $rgsEventsCheck[$iEvt]{BaseName};

	$szTraceHeader = "INLINE void ET$szTraceName(\n";

	$szTraceStructType = "Ese$szTraceName" . "Native";
	$szEseEventTraceHxxDebugCheckTrace = "#ifdef DEBUG\n";
	$szEseEventTraceHxxDebugCheckTrace = $szEseEventTraceHxxDebugCheckTrace . "    $szTraceStructType ettT = {\n";

	$szEseEventTraceHxxUntypedTraceCall = "    OSEventTrace(\n";

	$szEseEventTraceHxxUntypedTraceCall = $szEseEventTraceHxxUntypedTraceCall . "        _etguid$szTraceName";

	if ( $rgcEventFieldCounts[$iEvt] > 0 ){
		$szEseEventTraceHxxUntypedTraceCall = $szEseEventTraceHxxUntypedTraceCall . ",\n";
		$szEseEventTraceHxxUntypedTraceCall = $szEseEventTraceHxxUntypedTraceCall . "        $rgcEventFieldCounts[$iEvt],\n";
	}

	for $iField ( 0 ... $#{$rgrgsEventFields[$iEvt]} ){

		$szEseVar = $rgrgsEventFields[$iEvt][$iField]{FieldName};
		$szEtwType = $rgrgsEventFields[$iEvt][$iField]{TypeEtw};

		if ( $iField >= 1 ){
			$szTraceHeader = $szTraceHeader . ",\n";
		}
		$szTraceHeader = $szTraceHeader . "    $EtwTypes->{$szEtwType}    $szEseVar";

		if ( $iField >= 1 ){
			$szEseEventTraceHxxDebugCheckTrace = $szEseEventTraceHxxDebugCheckTrace . ",\n";
		}
		$szEseEventTraceHxxDebugCheckTrace = $szEseEventTraceHxxDebugCheckTrace . "        $szEseVar";

		if ( $iField >= 1 ){
			$szEseEventTraceHxxUntypedTraceCall = $szEseEventTraceHxxUntypedTraceCall . ",\n";
		}
		if ( ( $EtwTypes->{$szEtwType} eq "PCWSTR" ) ||
			( $EtwTypes->{$szEtwType} eq "PCSTR" ) ||
			( $EtwTypes->{$szEtwType} eq "const BYTE *" ) ||
			( $EtwTypes->{$szEtwType} eq "const GUID *" ) ){
			# These types are pure pass-through, no deref necessary.
			$szEseEventTraceHxxUntypedTraceCall = $szEseEventTraceHxxUntypedTraceCall . "        $szEseVar";
		} else {
			$szEseEventTraceHxxUntypedTraceCall = $szEseEventTraceHxxUntypedTraceCall . "        ($EtwTypes->{$szEtwType} *)&$szEseVar";
		}
	}

	$szTraceHeader = $szTraceHeader . " )\n{\n";

	$szEseEventTraceHxxDebugCheckTrace = $szEseEventTraceHxxDebugCheckTrace . " };\n    FakeUseTrace( (void*)&ettT );\n#endif // DEBUG\n";

	$szEseEventTraceHxxUntypedTraceCall = $szEseEventTraceHxxUntypedTraceCall . " );\n}\n";

	print hEseEventTraceHxx "$szTraceHeader\n";
	print hEseEventTraceHxx "$szEseEventTraceHxxDebugCheckTrace\n";
	print hEseEventTraceHxx "$szEseEventTraceHxxUntypedTraceCall\n";

}


#
#	Step 3-D - Create _oseventtraceimpl.g.hxx
#

print hOsEventTraceHxxImplIns <<__OSEVENTTRACEIMPLHXXPROLOG__;
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// This file $szOsEventTraceImplHxxFileName is generated by gengen.bat / gengenetw.pl using EseEtwEventsPregen.txt

__OSEVENTTRACEIMPLHXXPROLOG__

for $iEvt ( 0 ... $#rgsEventsCheck ){

	# example:
	#     void ETW_FirstDirtyPage_Trace( const MOF_FIELD * rgparg )
	#         {
	#         EventWriteESE_DirtyPage_Trace(
	#             *(DWORD *)rgparg[0].DataPtr,
	#             *(ULONG *)rgparg[1].DataPtr,
	#             *(DWORD *)rgparg[2].DataPtr,
	#             ...
	#             *(BYTE *)rgparg[17].DataPtr,
	#             *(BYTE *)rgparg[18].DataPtr );
	#         }

	$szTraceName = $rgsEventsCheck[$iEvt]{BaseName};

	$szOsEventTraceHxxUntypedTraceDefn = "void ETW_$szTraceName" . "_Trace( const MOF_FIELD * rgparg )\n";
	$szOsEventTraceHxxUntypedTraceDefn = $szOsEventTraceHxxUntypedTraceDefn . "{\n";
	$szOsEventTraceHxxUntypedTraceDefn = $szOsEventTraceHxxUntypedTraceDefn . "    EventWriteESE_$szTraceName" . "_Trace(\n";

	for $iField ( 0 ... $#{$rgrgsEventFields[$iEvt]} ){

		$szEseVar = $rgrgsEventFields[$iEvt][$iField]{FieldName};
		$szEtwType = $rgrgsEventFields[$iEvt][$iField]{TypeEtw};

		if ( $iField >= 1 ){
			$szOsEventTraceHxxUntypedTraceDefn = $szOsEventTraceHxxUntypedTraceDefn . ",\n";
		}
		if ( ( $EtwTypes->{$szEtwType} eq "PCWSTR" ) ||
			( $EtwTypes->{$szEtwType} eq "PCSTR" ) ||
			( $EtwTypes->{$szEtwType} eq "const BYTE *" ) ||
			( $EtwTypes->{$szEtwType} eq "const GUID *" ) ){
			# These types are pure pass-through, no deref necessary.
			$szOsEventTraceHxxUntypedTraceDefn = $szOsEventTraceHxxUntypedTraceDefn . "        ($EtwTypes->{$szEtwType})rgparg[$iField].DataPtr";
		} else {
			$szOsEventTraceHxxUntypedTraceDefn = $szOsEventTraceHxxUntypedTraceDefn . "        *($EtwTypes->{$szEtwType} *)rgparg[$iField].DataPtr";
		}
	}

	$szOsEventTraceHxxUntypedTraceDefn = $szOsEventTraceHxxUntypedTraceDefn . " );\n}\n";


	print hOsEventTraceHxxImplIns "$szOsEventTraceHxxUntypedTraceDefn\n";
}


#
#	Step 3-E - Append to _oseventtraceimpl.g.hxx (the PFN_ARRAY part)
#

print hOsEventTraceHxxImplIns "#define ESE_ETW_AUTOGEN_PFN_ARRAY \\\n";

for $i ( 0 .. $#rgsEvents ){
	if ( $iPrintLevel >= 3 ){
		print "\tDEBUG: id= Event.ESE_$rgsEvents[$i]{BaseName}  - $rgsEvents[$i]{FieldCound} - $rgcEventFieldCounts[$i] done.\n";
	}

	# example: { 19, &ETW_FirstDirtyPage_Trace },
	print hOsEventTraceHxxImplIns "        { $rgcEventFieldCounts[$i], &ETW_$rgsEvents[$i]{BaseName}_Trace }, \\\n";
}

print hOsEventTraceHxxImplIns "\n\n";


#
#	Step 4 - Create Microsoft-Exchange-ESE.g.cs (for CPR parser)
#

print hEseEtwParserForCPR <<__ESEETWPARSERFORCPRPROLOG__;
// ---------------------------------------------------------------------------
// <copyright file="$szEseEtwParserForCPRFilename" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// ---------------------------------------------------------------------------

// Auto-generated, DON'T EDIT MANUALLY.
// This file $szEseEtwParserForCPRFilename is generated by gengen.bat / gengenetw.pl using EseEtwEventsPregen.txt

namespace Microsoft.Exchange.Diagnostics.Profiling.Agent.Analyze.EseTraceParser
{
    using System;
    using Microsoft.Diagnostics.Tracing;

__ESEETWPARSERFORCPRPROLOG__

	# example: 
	#    public enum EseTraceEventIds
	#    {
	#        TraceBaseId = 100,
	#        // and so on
	#    }
	#
	#    /// <summary>
	#    /// CacheFirstDirtyPage parser
	#    /// EventId = 154
	#    /// Level = Informational
	#    /// Keyword(s): BF DataWorkingSet 
	#    /// </summary>
	#    [System.CodeDom.Compiler.GeneratedCode("gengenetw.pl", "1")]
	#    public sealed class CacheFirstDirtyPage
	#    {
	#        public int tick { get { return (int)traceEvent.PayloadValue(0); } }
	#        public int ifmp { get { return (int)traceEvent.PayloadValue(1); } }
	#        public int pgno { get { return (int)traceEvent.PayloadValue(2); } }
	#        // and so on
	#
	#        public TraceEvent TraceEvent { get; }
	#
	#        public FirstDirtyPageTraceParser(TraceEvent _traceEvent)
	#        {
	#            this.TraceEvent = _traceEvent;
	#        }
	#    }

$szEseEtwParserEnumDef = "    public enum EseTraceEventIds\n" . "    {\n";

for $iEvt ( 0 ... $#rgsEventsCheck ){
		$szEseEtwParserEnumDef = $szEseEtwParserEnumDef . "        $rgsEvents[$iEvt]{BaseName} = $rgsEvents[$iEvt]{EventId},\n";
}

$szEseEtwParserEnumDef = $szEseEtwParserEnumDef . "    }\n";
print hEseEtwParserForCPR "$szEseEtwParserEnumDef\n";

for $iEvt ( 0 ... $#rgsEventsCheck ){
	# no need to emit classes for traces that don't have custom fields
	if ( 0 < $#{$rgrgsEventFields[$iEvt]} ){
		$szTraceName = $rgsEventsCheck[$iEvt]{BaseName};
		$szEseEtwParserTypeDef = "    /// <summary>\n" .
			"    /// $szTraceName parser\n" .
			"    /// EventId = $rgsEvents[$iEvt]{EventId}\n" .
			"    /// Level = $rgsEvents[$iEvt]{Verbosity}\n" .
			"    /// Keyword(s): $rgsEvents[$iEvt]{Keywords}\n" .
			"    /// </summary>\n";
	
		$szEseEtwParserTypeDef = $szEseEtwParserTypeDef . "    [System.CodeDom.Compiler.GeneratedCode(\"gengenetw.pl\", \"1\")]\n";
		$szEseEtwParserTypeDef = $szEseEtwParserTypeDef . "    public sealed class $szTraceName" . "TraceParser\n";
		$szEseEtwParserTypeDef = $szEseEtwParserTypeDef . "    {\n";
	
		$szPayloadNames = "        public static readonly string[] PayloadNames = {";
		for $iField ( 0 ... $#{$rgrgsEventFields[$iEvt]} ){
			$szPayloadNames = $szPayloadNames . " \"$rgrgsEventFields[$iEvt][$iField]{FieldName}\",";
			if ( length($szPayloadNames) >= 160 and $iField < $#{$rgrgsEventFields[$iEvt]} - 1 ){
				$szEseEtwParserTypeDef = $szEseEtwParserTypeDef . $szPayloadNames;
				$szPayloadNames = "\n                                                        ";
			}
		}
	
		$szEseEtwParserTypeDef = $szEseEtwParserTypeDef . $szPayloadNames . " };\n\n";
	
		for $iField ( 0 ... $#{$rgrgsEventFields[$iEvt]} ){
	
			$szPropName = $rgrgsEventFields[$iEvt][$iField]{FieldName};
			$szEtwType = $rgrgsEventFields[$iEvt][$iField]{TypeEtw};
			$szEtwManagedType = $EtwManagedTypes->{$szEtwType};
			$szEseEtwParserTypeDef = $szEseEtwParserTypeDef . "        public $szEtwManagedType $szPropName { get { return ($szEtwManagedType)this.TraceEvent.PayloadValue($iField); } }\n"
		}
	
		if ( $#{$rgrgsEventFields[$iEvt]} > 0 ){
			$szEseEtwParserTypeDef = $szEseEtwParserTypeDef . "\n";
		}
	
		$szEseEtwParserTypeDef = $szEseEtwParserTypeDef .
			"        public TraceEvent TraceEvent { get; }\n\n" .
			"        public $szTraceName" . "TraceParser(TraceEvent _traceEvent)\n" .
			"        {\n" .
			"            this.TraceEvent = _traceEvent;\n" .
			"        }\n" .
			"    }\n";
	
		print hEseEtwParserForCPR "$szEseEtwParserTypeDef\n";
	}
}

print hEseEtwParserForCPR "}\n";

#   -----------------------------------------------------------------------------------------------------------------------------------------
#
#	Step N+1 - Close out the files and such.
#
#   -----------------------------------------------------------------------------------------------------------------------------------------

if ( $iPrintLevel >= 1 ){
	print "\tFinished all ETW trace processing.\n";
}

close( hEtwPregenData );
close( hEtwBaseMc );

close( hEtwNewMc );
close( hOsEventTraceHxxHdrIns );
close( hOsEventTraceHxxImplIns );
close( hEseEventTraceTypeHxx );
close( hEseEventTraceHxx );
close( hEseEtwParserForCPR );
