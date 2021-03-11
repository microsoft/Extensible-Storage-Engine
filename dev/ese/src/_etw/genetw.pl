# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# This file processes the Micrsoft-ETW-ESE.mc file into Microsoft-ESE-ETW.man, while merging in 
# additional ETW events, tasks, strings, etc that we auto-generate from ESE's JET APIs and the 
# JET_tracetags.

# Are we in ESENT or ESE ..
#
if( uc( $ARGV[0] ) eq uc("ESENT") ) {
	$szEseEtwProvGuid		= "478EA8A8-00BE-4BA6-8E75-8B9DC7DB9F78";
	$szEseEtwProvName		= "Microsoft-Windows-ESE";
	$szEseEtwProvSymbol		= "Microsoft_Windows_ESE";
	$szEseEtwChannelName	= "Microsoft-Windows-ESE";
	$szEseEtwAsmID			= "Microsoft-ETW-ESE";
	$szEseEtwChannelEnabled		= "false";
	$szEseEtwChannelSize		= "655360";
} else {
	$szEseEtwProvGuid		= "EF2AAEDE-57B4-431F-8F82-0BD11A8D30F8";
	$szEseEtwProvName		= "Microsoft-Exchange-ESE";
	$szEseEtwProvSymbol		= "Microsoft_Exchange_ESE";
	$szEseEtwChannelName	= "Microsoft-Exchange-ESE";
	$szEseEtwAsmID			= "Microsoft-Exchange-ETW-ESE";
	$szEseEtwChannelEnabled		= "true";
	$szEseEtwChannelSize		= "41943040";
}

# $#ARGV is the index of the last element.
if ( $#ARGV < 5 )
{
   die "Usage: genetw.pl [Microsoft-ETW-ESE.mc] [path-to-jethdr.w] [path-to-_jet.hxx] [path-to-tcconst.hxx] [output-file]";
}
my $inputFile = $ARGV[1];
my $jethdrwFile = $ARGV[2];
my $apilistFile = $ARGV[3];
my $tcconstFile = $ARGV[4];
my $outputFile = $ARGV[5];

open( ETWDATA, "<$inputFile" ) or die "Cannot open the ESE ETW data file ($inputFile).";
open( JETHDR, "<$jethdrwFile" ) or die "Cannot open the jethdr.w data file ($jethdrwFile).";
open( JETAPILIST, "<$apilistFile" ) or die "Cannot open the _jet.hxx data file ($apilistFile).";
open( TCCONST, "<$tcconstFile" ) or die "Cannot open the tcconst.hxx data file ($tcconstFile).";
open( OUTPUTFILE, ">$outputFile" ) or die "Cannot open the destination file ($outputFile).";

while($line = <ETWDATA>) {

	#	We pass through almost all lines, without any alteration.

	#
	#	Process the product specific (ESE or ESENT) provider inserts ...
	#

	if( $line =~ /ESE_PRE_GEN_BASE_FILE:/ ){
		#	This line indicates we're dealing with the base file.
		$line = "<!--This file is processed by genetw.pl to generate the real Microsoft-ESE-ETW.man file-->\n";

	} elsif( $line =~ /xxxESE_ETW_PROV_GUIDxxx/ ){
		$line =~ s/xxxESE_ETW_PROV_GUIDxxx/$szEseEtwProvGuid/;
	} elsif( $line =~ /xxxESE_ETW_PROV_NAMExxx/ ){
		$line =~ s/xxxESE_ETW_PROV_NAMExxx/$szEseEtwProvName/;
	} elsif( $line =~ /xxxESE_ETW_PROV_SYMBOLxxx/ ){
		$line =~ s/xxxESE_ETW_PROV_SYMBOLxxx/$szEseEtwProvSymbol/;
	} elsif( $line =~ /xxxESE_ETW_CHANNEL_NAMExxx/ ){
		$line =~ s/xxxESE_ETW_CHANNEL_NAMExxx/$szEseEtwChannelName/;
	} elsif( $line =~ /xxxESE_ETW_ASM_IDxxx/ ){
		$line =~ s/xxxESE_ETW_ASM_IDxxx/$szEseEtwAsmID/;
 	} elsif( $line =~ /xxxESE_ETW_CHANNEL_ENABLEDxxx/ ){
		$line =~ s/xxxESE_ETW_CHANNEL_ENABLEDxxx/$szEseEtwChannelEnabled/;
	} elsif( $line =~ /xxxESE_ETW_CHANNEL_SIZExxx/ ){
		$line =~ s/xxxESE_ETW_CHANNEL_SIZExxx/$szEseEtwChannelSize/;
	}
	# else - we pass the $line through unaltered for printing 

	print OUTPUTFILE $line;

	#
	#	Process the JET_tracetag Inserts ...
	#

	if( $line =~ /<!--ESE_ETW_TRACETAG_NAMEVALUES:/ ){
		seek( JETHDR, 0, 0 );
		$idTraceTag = 100;
		while($tagline = <JETHDR>) {
			if( $tagline =~ /^(\s)*JET_tracetag/ ){
				$szTagName = $tagline;
				$szTagName =~ s/^(\s)*JET_tracetag//;
				$szTagName =~ s/,(.*)$//;
				chomp $szTagName;

				if ( $szTagName ne "Max" ){
					print OUTPUTFILE "          <task\n";
					print OUTPUTFILE "              name=\"ESE_tag$szTagName"; print OUTPUTFILE "_Trace\"\n";
					print OUTPUTFILE "              value=\"$idTraceTag\"\n";
					print OUTPUTFILE "              />\n";
					$idTraceTag++;
				}
			}
		}
	}

	if( $line =~ /<!--ESE_ETW_TRACETAG_EVENTS:/ ){
		seek( JETHDR, 0, 0 );
		$idTraceTag = 200;
		while($tagline = <JETHDR>) {
			if( $tagline =~ /^(\s)*JET_tracetag/ ){
				$szTagName = $tagline;
				$szTagName =~ s/^(\s)*JET_tracetag//;
				if ( $szTagName =~ /^(.*)\[(.*)\]/ ){
					# There is a level tag for this tracetag
					$szLevel = $szTagName;
					$szLevel =~ s/^(.*)\[//;
					$szLevel =~ s/\](.*)$//;
					chomp $szLevel;
				} else {
					# Default: level for the tracetag ETW events
					$szLevel = "Verbose";
				}
				$szTagName =~ s/,(.*)$//;
				chomp $szTagName;

				if ( $szTagName ne "Max" ){
					printf OUTPUTFILE "          <event\n";
					print OUTPUTFILE "              keywords=\"JETTraceTag\"\n";
					print OUTPUTFILE "              level=\"win:$szLevel\"\n";
					print OUTPUTFILE "              message=\"\$(string.Event.ESE_tag$szTagName"; print OUTPUTFILE "_Trace)\"\n";
					print OUTPUTFILE "              symbol=\"ESE_tag$szTagName"; print OUTPUTFILE "_Trace\"\n";
					print OUTPUTFILE "              task=\"ESE_tag$szTagName"; print OUTPUTFILE "_Trace\"\n";
					print OUTPUTFILE "              template=\"tStringTrace\"\n";
					print OUTPUTFILE "              value=\"$idTraceTag\"\n";
					print OUTPUTFILE "              />\n";
					$idTraceTag++;
				}
			}
		}
	}

	if( $line =~ /<!--ESE_ETW_TRACETAG_STRINGVALUES:/ ){
		seek( JETHDR, 0, 0 );
		$idTraceTag = 200;
		while($tagline = <JETHDR>) {
			if( $tagline =~ /^(\s)*JET_tracetag/ ){
				$szTagName = $tagline;
				$szTagName =~ s/^(\s)*JET_tracetag//;
				$szTagName =~ s/,(.*)$//;
				chomp $szTagName;

				if ( $szTagName ne "Max" ){
					print OUTPUTFILE "        <string\n";
					print OUTPUTFILE "            id=\"Event.ESE_tag$szTagName"; print OUTPUTFILE "_Trace\"\n";
					print OUTPUTFILE "            value=\"ESE tag$szTagName"; print OUTPUTFILE " Trace\"\n";
					print OUTPUTFILE "            />\n";
					$idTraceTag++;
				}
			}
		}
	}

	#
	#	Process the JET API Inserts ...
	#

	if( $line =~ /<!--ESE_ETW_JETAPI_NAMEMAP:/ ){
		seek(JETAPILIST, 0, 0);
		print OUTPUTFILE "            <valueMap name=\"ESE_ApiCallNameMap\">\n";
		while($apiline = <JETAPILIST>) {
			if( $apiline =~ /^#define op/ ){

				$szApiName = $apiline;
				$szApiName =~ s/^#define op//;
				$szApiName =~ s/(\s)+.*$//;
				chomp $szApiName;

				$szJetApiName = $szApiName;
				$szJetApiName =~ s/^/Jet/;

				$szApiNum = $apiline;
				$szApiNum =~ s/^#define op[a-zA-Z0-9_]*(\s)*//;
				$szApiNum =~ s/(\s)+.*$//;
				chomp $szApiNum;
		
				if ( $szApiName ne "Max" ){
					print OUTPUTFILE "                <map\n";
					print OUTPUTFILE "                    value=\"$szApiNum\"\n";
					print OUTPUTFILE "                    message=\"\$(string.ApiMap.ESE_fn_apiid_$szJetApiName)\"\n";
					print OUTPUTFILE "                />\n";
				}
			}
		}
		print OUTPUTFILE "            </valueMap>\n";
	}

	if( $line =~ /<!--ESE_ETW_JETAPI_STRINGVALUES:/ ){
		seek(JETAPILIST, 0, 0 );
		while($apiline = <JETAPILIST>) {
			if( $apiline =~ /^#define op/ ){

				$szApiName = $apiline;
				$szApiName =~ s/^#define op//;
				$szApiName =~ s/(\s)+.*$//;
				chomp $szApiName;

				$szJetApiName = $szApiName;
				$szJetApiName =~ s/^/Jet/;

				$szApiNum = $apiline;
				$szApiNum =~ s/^#define op[a-zA-Z0-9_]*(\s)*//;
				$szApiNum =~ s/(\s)+.*$//;
				chomp $szApiNum;
		
				if ( $szApiName ne "Max" ){
					print OUTPUTFILE "        <string\n";
					print OUTPUTFILE "            id=\"ApiMap.ESE_fn_apiid_$szJetApiName\"\n";
					print OUTPUTFILE "            value=\"$szJetApiName\"\n";
					print OUTPUTFILE "            />\n";
				}
			}
		}
	}

	#
	#	Process the IO File Inserts ...
	#

	if( $line =~ /<!--ESE_ETW_IOFILE_NAMEMAP:/ ){
		seek(TCCONST, 0, 0);
		print OUTPUTFILE "            <valueMap name=\"ESE_IofileMap\">\n";
		while($tcconstline = <TCCONST>) {
			if( $tcconstline =~ /^\s*iofile.*\s= / ){

				chomp $tcconstline;
				($iofileName, $equals, $iofileValue) = split(' ', $tcconstline);
				if( $iofileValue =~ /,/ ){
					$iofileValue = substr($iofileValue, 0, -1);
				}
				print OUTPUTFILE "              <map value=\"$iofileValue\" message=\"\$(string.IofileMap.$iofileName)\"/>\n";
			}
		}
		print OUTPUTFILE "            </valueMap>\n";
	}

	if( $line =~ /<!--ESE_ETW_IOFILE_STRINGVALUES:/ ){
		seek(TCCONST, 0, 0 );
		while($tcconstline = <TCCONST>) {
			if( $tcconstline =~ /^\s*iofile.*\s= / ){

				chomp $tcconstline;
				($iofileName) = split(' ', $tcconstline);
				print OUTPUTFILE "        <string id=\"IofileMap.$iofileName\" value=\"$iofileName\"/>\n";
			}
		}
	}

	#
	#	Process the IO Reason Primary Inserts ...
	#

	if( $line =~ /<!--ESE_ETW_IORP_NAMEMAP:/ ){
		seek(TCCONST, 0, 0);
		print OUTPUTFILE "            <valueMap name=\"ESE_IorpMap\">\n";
		while($tcconstline = <TCCONST>) {
			if( $tcconstline =~ /^\s*iorp/ ){

				chomp $tcconstline;
				($iorpName, $equals, $iorpValue) = split(' ', $tcconstline);
				if( $iorpValue =~ /,/ ){
					$iorpValue = substr($iorpValue, 0, -1);
				}
				print OUTPUTFILE "              <map value=\"$iorpValue\" message=\"\$(string.IorpMap.$iorpName)\"/>\n";
			}
		}
		print OUTPUTFILE "            </valueMap>\n";
	}

	if( $line =~ /<!--ESE_ETW_IORP_STRINGVALUES:/ ){
		seek(TCCONST, 0, 0 );
		while($tcconstline = <TCCONST>) {
			if( $tcconstline =~ /^\s*iorp/ ){

				chomp $tcconstline;
				($iorpName) = split(' ', $tcconstline);
				print OUTPUTFILE "        <string id=\"IorpMap.$iorpName\" value=\"$iorpName\"/>\n";
			}
		}
	}

	#
	#	Process the IO Reason Secondary Inserts ...
	#

	if( $line =~ /<!--ESE_ETW_IORS_NAMEMAP:/ ){
		seek(TCCONST, 0, 0);
		print OUTPUTFILE "            <valueMap name=\"ESE_IorsMap\">\n";
		print OUTPUTFILE "              <map value=\"0\" message=\"\$(string.IorsMap.iorsNone)\"/>\n";
		while($tcconstline = <TCCONST>) {
			if( $tcconstline =~ /^\s*iors/ ){

				chomp $tcconstline;
				($iorsName, $equals, $iorsValue) = split(' ', $tcconstline);
				if( $iorsValue =~ /,/ ){
					$iorsValue = substr($iorsValue, 0, -1);
				}
				print OUTPUTFILE "              <map value=\"$iorsValue\" message=\"\$(string.IorsMap.$iorsName)\"/>\n";
			}
		}
		print OUTPUTFILE "            </valueMap>\n";
	}

	if( $line =~ /<!--ESE_ETW_IORS_STRINGVALUES:/ ){
		seek(TCCONST, 0, 0 );
		print OUTPUTFILE "        <string id=\"IorsMap.iorsNone\" value=\"iorsNone\"/>\n";
		while($tcconstline = <TCCONST>) {
			if( $tcconstline =~ /^\s*iors/ ){

				chomp $tcconstline;
				($iorsName) = split(' ', $tcconstline);
				print OUTPUTFILE "        <string id=\"IorsMap.$iorsName\" value=\"$iorsName\"/>\n";
			}
		}
	}

	#
	#	Process the IO Reason Tertiary Inserts ...
	#

	if( $line =~ /<!--ESE_ETW_IORT_NAMEMAP:/ ){
		seek(TCCONST, 0, 0);
		print OUTPUTFILE "            <valueMap name=\"ESE_IortMap\">\n";
		while($tcconstline = <TCCONST>) {
			if( $tcconstline =~ /^\s*iort/ ){

				chomp $tcconstline;
				($iortName, $equals, $iortValue) = split(' ', $tcconstline);
				if( $iortValue =~ /,/ ){
					$iortValue = substr($iortValue, 0, -1);
				}
				print OUTPUTFILE "              <map value=\"$iortValue\" message=\"\$(string.IortMap.$iortName)\"/>\n";
			}
		}
		print OUTPUTFILE "            </valueMap>\n";
	}

	if( $line =~ /<!--ESE_ETW_IORT_STRINGVALUES:/ ){
		seek(TCCONST, 0, 0 );
		while($tcconstline = <TCCONST>) {
			if( $tcconstline =~ /^\s*iort/ ){

				chomp $tcconstline;
				($iortName) = split(' ', $tcconstline);
				print OUTPUTFILE "        <string id=\"IortMap.$iortName\" value=\"$iortName\"/>\n";
			}
		}
	}

	#
	#	Process the IO Reason Flag Inserts ...
	#

	if( $line =~ /<!--ESE_ETW_IORF_NAMEMAP:/ ){
		seek(TCCONST, 0, 0);
		print OUTPUTFILE "            <bitMap name=\"ESE_IorfMap\">\n";
		print OUTPUTFILE "              <map value=\"0x0\" message=\"\$(string.IorfMap.iorfNone)\"/>\n";
		while($tcconstline = <TCCONST>) {
			if( $tcconstline =~ /^\s*iorf/ ){

				chomp $tcconstline;
				($iorfName, $equals, $iorfValue) = split(' ', $tcconstline);
				if( $iorfValue =~ /,/ ){
					$iorfValue = substr($iorfValue, 0, -1);
				}
				print OUTPUTFILE "              <map value=\"$iorfValue\" message=\"\$(string.IorfMap.$iorfName)\"/>\n";
			}
		}
		print OUTPUTFILE "            </bitMap>\n";
	}

	if( $line =~ /<!--ESE_ETW_IORF_STRINGVALUES:/ ){
		seek(TCCONST, 0, 0 );
		print OUTPUTFILE "        <string id=\"IorfMap.iorfNone\" value=\"iorfNone\"/>\n";
		while($tcconstline = <TCCONST>) {
			if( $tcconstline =~ /^\s*iorf/ ){

				chomp $tcconstline;
				($iorfName) = split(' ', $tcconstline);
				print OUTPUTFILE "        <string id=\"IorfMap.$iorfName\" value=\"$iorfName\"/>\n";
			}
		}
	}

	#
	#	Process the IO Flush Reason Inserts ...
	#

	if( $line =~ /<!--ESE_ETW_IOFR_NAMEMAP:/ ){
		seek(TCCONST, 0, 0);
		print OUTPUTFILE "            <bitMap name=\"ESE_IofrMap\">\n";
		while($tcconstline = <TCCONST>) {
			if( $tcconstline =~ /^\s*iofr/ ){

				chomp $tcconstline;
				($iofrName, $equals, $iofrValue) = split(' ', $tcconstline);
				if( $iofrValue =~ /,/ ){
					$iofrValue = substr($iofrValue, 0, -1);
				}
				print OUTPUTFILE "              <map value=\"$iofrValue\" message=\"\$(string.IofrMap.$iofrName)\"/>\n";
			}
		}
		print OUTPUTFILE "            </bitMap>\n";
	}

	if( $line =~ /<!--ESE_ETW_IOFR_STRINGVALUES:/ ){
		seek(TCCONST, 0, 0 );
		while($tcconstline = <TCCONST>) {
			if( $tcconstline =~ /^\s*iofr/ ){

				chomp $tcconstline;
				($iofrName) = split(' ', $tcconstline);
				print OUTPUTFILE "        <string id=\"IofrMap.$iofrName\" value=\"$iofrName\"/>\n";
			}
		}
	}

}

close(ETWDATA);
close(JETHDR);
close(JETAPILIST);
close(TCCONST);
close(OUTPUTFILE);

