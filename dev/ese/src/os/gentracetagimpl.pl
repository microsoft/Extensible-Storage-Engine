# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# This file processes the JET_tracetags from the jethdr.w into a set of functions, and a couple sets
# of array slots to work this feature into the overall ETW infrastructure.

# The JET_tracetags will be of this format:
#	JET_tracetagFoo,	// [Error|Informational]
#

# $#ARGV is the index of the last element.
if ( $#ARGV < 1 )
{
   die "Usage: gentracetagtable.pl [location-of-jethdr.w] [output-file]";
}
my $inputFile = $ARGV[0];
my $outputFile = $ARGV[1];

open( JETHDR, "<$inputFile" ) or die "Cannot open the ESE jethdr.w file.";
open( OUTPUTFILE, ">$outputFile" ) or die "Cannot open the destination file ($outputFile).";

print OUTPUTFILE <<EOFHDR;
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef PROVIDE_TRACETAG_ETW_IMPL

EOFHDR


$idTraceTag = 200;
while($tagline = <JETHDR>) {
	if( $tagline =~ /^\s*JET_tracetag(\w+),/ ){
		$szTagName = $1;

		if ( $szTagName ne "Max" ){
			print OUTPUTFILE "void ETW_tag$szTagName"; print OUTPUTFILE "_Trace( const MOF_FIELD * rgparg )\n";
			print OUTPUTFILE "	{\n";
			print OUTPUTFILE "	EventWriteESE_tag$szTagName"; print OUTPUTFILE "_Trace( (const char *)rgparg[0].DataPtr );\n";
			print OUTPUTFILE "	}\n";
			$idTraceTag++;
		}
	}
}
print OUTPUTFILE "\n\n";

seek(JETHDR, 0, 0);
$idTraceTag = 200;

print OUTPUTFILE "#define ESE_ETW_TRACETAG_PFN_ARRAY	\\\n";

while($tagline = <JETHDR>) {
	if( $tagline =~ /^\s*JET_tracetag(\w+),/ ){
		$szTagName = $1;

		if ( $szTagName ne "Max" ){
			print OUTPUTFILE "\t\t{ 1, &ETW_tag$szTagName"; print OUTPUTFILE "_Trace }, \\\n";
			$idTraceTag++;
		}
	}
}

print OUTPUTFILE "\n";
print OUTPUTFILE "#endif // PROVIDE_TRACETAG_ETW_IMPL\n";
print OUTPUTFILE "\n";

seek(JETHDR, 0, 0);

print OUTPUTFILE "#define ESE_TRACETAG_SZ_ARRAY	\\\n";

while($tagline = <JETHDR>) {
	if( $tagline =~ /^\s*JET_tracetag(\w+),/ ){
		$szTagName = $1;

		if ( $szTagName ne "Max" ){
			if ( $szTagName =~ /Null/ ){
				$szTagName=OSTRACENULLPARAMW;
			} else {
				$szTagName="L\"$szTagName\"";
			}
			print OUTPUTFILE "\t$szTagName,\t\t  \\\n";
			$idTraceTag++;
		}
	}
}
print OUTPUTFILE "\n\n";

close(JETHDR);
close(OUTPUTFILE);
