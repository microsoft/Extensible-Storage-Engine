// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

class STRINGTESTS : public UNITTEST
{
    private:
        static STRINGTESTS s_instance;

    protected:
        STRINGTESTS() {}

    public:
        ~STRINGTESTS() {}

    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();
};

STRINGTESTS STRINGTESTS::s_instance;

const char * STRINGTESTS::SzName() const        { return "String Test"; };
const char * STRINGTESTS::SzDescription() const { return 
            "Tests the OS Layer String Library functionality.";
        }
bool STRINGTESTS::FRunUnderESE98() const        { return true; }
bool STRINGTESTS::FRunUnderESENT() const        { return true; }
bool STRINGTESTS::FRunUnderESE97() const        { return true; }


ERR STRINGTESTS::ErrTest()
{
    JET_ERR         err = JET_errSuccess;

    COSLayerPreInit     oslayer;

    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }


    wprintf( L"\tTesting String functionality ...\n");

    WCHAR wszUniString[256] = L"unittestWWW.edb";
    wszUniString[9]     = (WCHAR)(0x0234);

    CHAR szDestString[256];

    OSTestCheck( JET_errBufferTooSmall          == ErrOSStrCbFormatA( szDestString, sizeof(szDestString), "Hi.%ws", wszUniString ) );

    OSTestCheck( JET_errUnicodeTranslationFail  == ErrOSSTRUnicodeToAscii( wszUniString, szDestString, _countof(szDestString) ) );
    OSTestCheck( JET_errSuccess                 == ErrOSSTRUnicodeToAscii( wszUniString, szDestString, _countof(szDestString), NULL, OSSTR_ALLOW_LOSSY ) );

HandleError:


    return err;
}

