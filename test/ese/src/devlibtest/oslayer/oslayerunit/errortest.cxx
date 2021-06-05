// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

//  ACHTUNG! This test is (unfortuately necessarily) VERY sensitive to code 
//  motility, try to only append to the test, to avoid breaking previous test
//  cases.  We also put in some buffer, and spacing to allow changes later.

// buffer...






//  ================================================================
class ERRORHANDLING : public UNITTEST
//  ================================================================
{

    private:

        static ERRORHANDLING s_instance;


    protected:

        ERRORHANDLING() {}


    public:

        ~ERRORHANDLING() {}


    public:

        const char * SzName() const;

        const char * SzDescription() const;


        bool FRunUnderESE98() const;

        bool FRunUnderESENT() const;

        bool FRunUnderESE97() const;


        ERR ErrTest();

};

ERRORHANDLING ERRORHANDLING::s_instance;

const char * ERRORHANDLING::SzName() const      { return "Error Handling"; };
const char * ERRORHANDLING::SzDescription() const   { return 
            "Tests the OS Layer error handling.";
        }
bool ERRORHANDLING::FRunUnderESE98() const      { return true; }
bool ERRORHANDLING::FRunUnderESENT() const      { return true; }
bool ERRORHANDLING::FRunUnderESE97() const      { return true; }


#ifdef DEBUG
    // The TLS info is only valid in checked / debug builds ...
    const BOOL g_fDebug = true;
#else
    const BOOL g_fDebug = false;
#endif


JET_ERR ErrFunction( JET_ERR err )
{
    return err;
}

bool FTestCallR()
{
    JET_ERR err;

    wprintf( L"\t\tTesting CallR()\n");

    Call( ErrFunction( JET_errSuccess ) );  // line 86 according to notepad.exe
    OSTestCheck( JET_errSuccess == err );
    OSTestCheck( !g_fDebug || fTrue );

    return true;

HandleError:

    return false;
}




//  ================================================================
ERR ERRORHANDLING::ErrTest()
//  ================================================================
{
    JET_ERR err = JET_errSuccess;

    wprintf( L"\tTesting error handling ...\n");

    COSLayerPreInit     oslayer;

    // FOSPreinit()
    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }

    //  Important: By not calling ErrOSInit() we are asserting that the
    //  OS Layer error handling facilities work pre-ErrOSInit().
    //OSTestCall( ErrOSInit() );

    // We use FXxxx, instead of ErrXxxx, b/c there is alot of errors 
    // flying around here....
    OSTestCheck( FTestCallR() );

HandleError:

//  OSTerm();

    return err;
}



//  ================================================================
class SzSourceFileNameFuncValidation : public UNITTEST
//  ================================================================
{

    private:

        static SzSourceFileNameFuncValidation s_instance;


    protected:

        SzSourceFileNameFuncValidation() {}


    public:

        ~SzSourceFileNameFuncValidation() {}


    public:

        const char * SzName() const;

        const char * SzDescription() const;


        bool FRunUnderESE98() const;

        bool FRunUnderESENT() const;

        bool FRunUnderESE97() const;


        ERR ErrTest();

};

SzSourceFileNameFuncValidation SzSourceFileNameFuncValidation::s_instance;

const char * SzSourceFileNameFuncValidation::SzName() const     { return "SzSourceFileNameFuncValidation"; };
const char * SzSourceFileNameFuncValidation::SzDescription() const  { return 
            "Tests the SzSourceFileName() functional behavior.";
        }
bool SzSourceFileNameFuncValidation::FRunUnderESE98() const     { return true; }
bool SzSourceFileNameFuncValidation::FRunUnderESENT() const     { return true; }
bool SzSourceFileNameFuncValidation::FRunUnderESE97() const     { return true; }


//  someone defined JETUNITTEST() at the ESE layer, not the OS Layer, so we'll define tests
//  here for now, until it can be moved.

//  ================================================================
ERR SzSourceFileNameFuncValidation::ErrTest()
//  ================================================================
{
    JET_ERR err = JET_errSuccess;

    COSLayerPreInit     oslayer;

    // FOSPreinit()
    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }

    //  "good" cases

    OSTestCheck( 0 == strcmp( "blah.cxx", SzSourceFileName( "C:\\mysrc\\blah.cxx" ) ) );
    OSTestCheck( 0 == strcmp( "blah", SzSourceFileName( "C:\\mysrc\\blah" ) ) );
    OSTestCheck( 0 == strcmp( "a", SzSourceFileName( "\\a" ) ) );
    OSTestCheck( 0 == strcmp( "b", SzSourceFileName( "\\a\\b" ) ) );
    OSTestCheck( 0 == strcmp( "blah.cxx", SzSourceFileName( "blah.cxx" ) ) );

    //  "null" / empty file name cases

    OSTestCheck( 0 == strcmp( "", SzSourceFileName( "" ) ) );
    OSTestCheck( 0 == strcmp( "", SzSourceFileName( NULL ) ) ); // should not AV

    //  "bad" cases

    UINT wAssertActionSaved = COSLayerPreInit::SetAssertAction( JET_AssertSkipAll );

    extern const CHAR * g_szBadSourceFileName;

    OSTestCheck( 0 == strcmp( g_szBadSourceFileName, SzSourceFileName( "C:\\mysrc\\blah.cxx\\" ) ) );
    OSTestCheck( 0 == strcmp( g_szBadSourceFileName, SzSourceFileName( "\\" ) ) );

    (void)COSLayerPreInit::SetAssertAction( wAssertActionSaved );

HandleError:
    return err;
}

