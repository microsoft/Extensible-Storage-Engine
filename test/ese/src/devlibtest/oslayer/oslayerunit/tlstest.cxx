// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

//  ================================================================
class TLSTESTS : public UNITTEST
//  ================================================================
{
    private:
        static TLSTESTS s_instance;

    protected:
        TLSTESTS() {}

    public:
        ~TLSTESTS() {}

    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();
};

TLSTESTS TLSTESTS::s_instance;

const char * TLSTESTS::SzName() const       { return "TLS Test"; };
const char * TLSTESTS::SzDescription() const    { return 
            "Tests the OS Layer thread local state / TLS functionality.";
        }
bool TLSTESTS::FRunUnderESE98() const       { return true; }
bool TLSTESTS::FRunUnderESENT() const       { return true; }
bool TLSTESTS::FRunUnderESE97() const       { return true; }


struct TLS {
    ULONG   i;
};

void SetTLSValue1( ULONG ulValue )
{
    TLS * ptls = Ptls();
    ptls->i = ulValue;
}

//  ================================================================
ERR TLSTESTS::ErrTest()
//  ================================================================
{
    JET_ERR         err = JET_errSuccess;

    OSPrepreinitSetUserTLSSize( sizeof( TLS ) );
    COSLayerPreInit     oslayer;

    // FOSPreinit()
    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }

// Note: The TLS functionality requires preinit, but SHOULD work without / before
// ErrOSInit() is called, this validates this assumption.
// FYI - if we don't call preinit, we AV.  So good to know where our boundaries are.
//  OSTestCall( ErrOSInit() );

    wprintf( L"\tTesting TLS functionality ...\n");

    SetTLSValue1( 0x42 );
    OSTestCheck( 0x42 == Ptls()-> i );

    SetTLSValue1( 42 );
    OSTestCheck( 0x42 != Ptls()-> i );
    OSTestCheck( 42 == Ptls()-> i );

HandleError:

//  OSTerm();

    return err;
}

