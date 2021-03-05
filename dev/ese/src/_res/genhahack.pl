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
	die "Usage: genhahack.pl [location-of-jetmsg.mc] [output-file]";
}
my $inputFile = $ARGV[0];
my $outputFile = $ARGV[1];

open( INPUTFILE, "<$inputFile" ) or die "Cannot open the jetmsg.mc file ($inputFile).";
open( OUTPUTFILE, ">$outputFile" ) or die "Cannot open the destination file ($outputFile).";

# Currently we do not insert a URL in our eventlog messages.  We are preserving this functionality for the future.
$URL = "";
$sawPrev = 0;
$sawMsgId = 0;

while( $line = <INPUTFILE> ) {

	if( $line =~ /^%1 \(%2\) %3/ ) {
		$line =~ s/^%1 \(%2\) %3//;

	# This is a bit sketchy, b/c I'm not the best at perl, but we want to add 25,000 to the event IDs
	# to give HA a separate HA / failure tag number space.
	} elsif ( $line =~ /^MessageId\=([0-9]+)\s*$/ ) {
		$eventid = $1;
		$eventid += 25000;
		$line =~ s/[0-9]+/$eventid/;  # wow, I didn't even expect this to work, that I could use a variable substr.  perl rocks.
	} elsif ( $line =~ /^MessageId=(.*)/ ) {
		$sawMsgId = 1;
	} elsif ( $line =~ /^SymbolicName=/ ) {
		$line =~ s/=/=HA_/;
	} elsif ( $line =~ /^LanguageNames=/ ) {
		$line =~ s/jetmsg/exdbmsg_ese/;
	} elsif ( $line =~ /^%n%t/ ) {
		$sawPrev = 1;
	} elsif ( $line =~ /^%[0-9]/ ) {
		$sawPrev = 1;
	} elsif ( $sawPrev ) {
		$sawPrev = 0;
	}

	# This rebases the insert args to have 3 fewer args.

	$line =~ s/%4/%1/;
	$line =~ s/%5/%2/;
	$line =~ s/%6/%3/;
	$line =~ s/%7/%4/;
	$line =~ s/%8/%5/;
	$line =~ s/%9/%6/;
	$line =~ s/%10/%7/;
	$line =~ s/%11/%8/;
	$line =~ s/%12/%9/;
	$line =~ s/%13/%10/;
	$line =~ s/%14/%11/;
	$line =~ s/%15/%12/;
	$line =~ s/%16/%13/;
	$line =~ s/%17/%14/;
	$line =~ s/%18/%15/;
	$line =~ s/%19/%16/;
	$line =~ s/%20/%17/;

	print OUTPUTFILE $line;
}

