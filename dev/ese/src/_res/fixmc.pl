# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# The eventlog messages in Ti RTM look something like this:
#
#	MessageId=100
#	SymbolicName=START_ID
#	Language=English
#	%1 (%2) %3The database engine %4.%5.%6.%7 started.
#	%n%nFor more information, click http://www.microsoft.com/contentredirect.asp.
#	.
#
#	In contrast, the eventlog messages in the merged code base no longer have the URL:
#
#	MessageId=100
#	SymbolicName=START_ID
#	Language=English
#	%1 (%2) %3The database engine %4.%5.%6.%7 started.
#	.
#
# This perl script puts that URL back into each message. We look for a line that starts with
# "%1 (%2)" followed by a line that only contains a period and then insert the URL in between # # those two lines.

# $#ARGV is the index of the last element.
if ( $#ARGV < 1 )
{
	die "Usage: fixmc.pl [location-of-jetmsg.mc] [output-file]";
}
my $inputFile = $ARGV[0];
my $outputFile = $ARGV[1];

open( INPUTFILE, "<$inputFile" ) or die "Cannot open the jetmsg.mc file ($inputFile).";
open( OUTPUTFILE, ">$outputFile" ) or die "Cannot open the destination file ($outputFile).";

# Currently we do not insert a URL in our eventlog messages.  We are preserving this functionality for the future.
$URL = "";
$sawPrev = 0;

while( <INPUTFILE> ) {
	if( /^%1 \(%2\)/ ) {
		$sawPrev = 1;
	} elsif ( /^%n%t/ ) {
		$sawPrev = 1;
	} elsif ( /^%[0-9]/ ) {
		$sawPrev = 1;
	} elsif ( $sawPrev ) {
		$sawPrev = 0;
		if( /^\.$/ ) {
			print OUTPUTFILE $URL;
		}
	}
	print OUTPUTFILE $_;
}

