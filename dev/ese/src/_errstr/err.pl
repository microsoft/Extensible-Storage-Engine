# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

$cEntry = 0;
$section = 0;

# $#ARGV is the index of the last element.
if ( $#ARGV < 1 )
{
	die "Usage: err.pl [location-of-jethdr.w] [output-file]";
}
my $inputFile = $ARGV[0];
my $outputFile = $ARGV[1];

open( INPUTFILE, "<$inputFile" ) or die "Cannot open the ESE jethdr.w file ($inputFile).";
open( OUTPUTFILE, ">$outputFile" ) or die "Cannot open the destination file ($outputFile).";

INPUT: while ( $line = <INPUTFILE> )
	{
	if ( $line =~ /_USE_NEW_JET_H_/ )
		{
		$section++;
		}
	if ( 2 == $section )
		{ 
		last INPUT;
		}
		
	if ( $line =~ /^#define\s+(JET_(err|wrn)\w+)\s+(-*\d+).*\/\*\s+(.*)\s+\*\// )
		{
		$errName	= $1;
		$errNum		= $3;
		$errText	= $4;
		$ERR[$cEntry]		= $errName;
		$ERRTEXT[$cEntry]	= $errText;
		$cEntry++;
		}
	elsif ( $line =~ /^#define\s+(JET_(err|wrn)\w+)\s+(-*\d+).*\/\/\s+(.*)\s+/ )
		{
		$errName	= $1;
		$errNum		= $3;
		$errText	= $4;
		$ERR[$cEntry]		= $errName;
		$ERRTEXT[$cEntry]	= $errText;
		$cEntry++;
		}

	}

print OUTPUTFILE <<__EOS__;

#ifdef MINIMAL_FUNCTIONALITY
#else

#include "jet.h"

#include <cstring>
#include <algorithm>

using namespace std;

#include "sync.hxx"
#include "types.hxx"

#include "errdata.hxx"		//	header for this library

class FIND_ERRNUM
	{
	public:
		FIND_ERRNUM( JET_ERR err ) : m_err( err ) {}
		bool operator()( const ERRORMSGDATA& error ) { return m_err == error.err; }

	private:
		const JET_ERR m_err;
	};

class FIND_ERRSTR
	{
	public:
		FIND_ERRSTR( const char * szErr ) : m_szErr( szErr ) {}
		bool operator()( const ERRORMSGDATA& error ) { return !_stricmp( m_szErr, error.szError ); }

	private:
		const char * const m_szErr;
	};
	
__EOS__

print OUTPUTFILE "static const ERRORMSGDATA rgerror[] = \n\t{\n" ;
for ( $i = 0; $i < $cEntry; $i++ )
	{
	printf OUTPUTFILE "\t{ %s,\t\"%s\",\t\"%s\" },\n", $ERR[$i], $ERR[$i], $ERRTEXT[$i];
	}
print OUTPUTFILE "\t};";

print OUTPUTFILE <<__EOS2__;


//	returns true for success, false if a mapping could not be found

bool FErrStringToJetError( const char * szError, JET_ERR * perr )
	{
	const ERRORMSGDATA * const perror = find_if( rgerror, rgerror + _countof( rgerror ), FIND_ERRSTR( szError ) );
	if( ( rgerror + _countof( rgerror ) ) != perror )
		{
		*perr = perror->err;
		return true;
		}
	else
		{
		return false;
		}
	}


//	sets szError to "Unknown Error" if the error could not be found, otherwise gives error string

void  JetErrorToString( JET_ERR err, const char **szError, const char **szErrorText )
	{
	const ERRORMSGDATA * const perror = find_if( rgerror, rgerror + _countof( rgerror ), FIND_ERRNUM( err ) );
	if( ( rgerror + _countof( rgerror ) ) != perror )
		{
		*szError = perror->szError;
		*szErrorText = perror->szErrorText;
		}
	else
		{
		*szError = szUnknownError;
		*szErrorText = szUnknownError;
		}
	}

//	returns the ERRORMSGDATA struct for the index value, returns NULL if no more entries

const ERRORMSGDATA * PerrorEntryI( __in int iEntry )
	{
	if ( iEntry < _countof(rgerror) )
		{
		return ( &( rgerror[iEntry] ) );
		}
	return NULL;
	}

#endif // MINIMAL_FUNCTIONALITY

__EOS2__

