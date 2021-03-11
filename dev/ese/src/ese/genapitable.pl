# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# This takes the src\inc\_jet.hxx file, and generates the string table for the JET APIs that
# the logapi needs.

# $#ARGV is the index of the last element.
if ( $#ARGV < 1 )
{
   die "Usage: genapitable.pl [location-of-_jet.hxx] [output-file]";
}
my $inputFile = $ARGV[0];
my $outputFile = $ARGV[1];

open( JETAPILIST, "<$inputFile" ) or die "Cannot open the ESE _jet.hxx file ($inputFile).";
open( OUTPUTFILE, ">$outputFile" ) or die "Cannot open the destination file ($outputFile).";

print OUTPUTFILE <<EOFHDR;
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#ifdef DEBUG

extern const CHAR * const mpopsz [];

const CHAR * const mpopsz [] =
	{
	0,									//	0
EOFHDR


$iJetApi = 1;

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
		
		if( $szApiName ne "Max" ){
			print OUTPUTFILE "\t\"$szJetApiName\",\t\t\t\t\t\t\t//\t$szApiNum\n";

			if( $iJetApi != $szApiNum ){
				print OUTPUTFILE "\t#error \"The JET API number stream is not continuous!!!\"\n";
			}

			$iJetApi++;
		}
	}
}
print OUTPUTFILE <<EOFHDR;
	};

C_ASSERT( _countof( mpopsz ) == opMax );

#endif // DEBUG

EOFHDR

close( JETAPILIST );
close( OUTPUTFILE );

