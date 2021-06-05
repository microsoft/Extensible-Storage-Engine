// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

enum IOREASONPRIMARY : BYTE
{
    //iorpNone = 0, defined generically by OS layer
    iorpInvalid = 0,

    iorpDefault,
    };

enum IOREASONSECONDARY : BYTE
{
    //iorsNone = 0, defined generically by OS layer
    };

enum IOREASONFLAGS : BYTE
{
    //iorfNone = 0, defined generically by OS layer
    };



//  ================================================================
class MEMFILE: public UNITTEST
//  ================================================================
{
    private:
        static MEMFILE s_instance;

    protected:
        MEMFILE() {}

    public:
        ~MEMFILE() {}

    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();
};

MEMFILE MEMFILE::s_instance;

const char * MEMFILE::SzName() const        { return "IFileAPI From Memory"; };
const char * MEMFILE::SzDescription() const { return 
            "Unit test for the CFileFromMemory class.";
        }
bool MEMFILE::FRunUnderESE98() const        { return true; }
bool MEMFILE::FRunUnderESENT() const        { return true; }
bool MEMFILE::FRunUnderESE97() const        { return true; }


//  ================================================================
ERR MEMFILE::ErrTest()
//  ================================================================
{
    JET_ERR         err     = JET_errSuccess;
    IFileAPI        *pfile  = NULL;

    COSLayerPreInit     oslayer;
    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }
    OSTestCall( ErrOSInit() );

    BYTE data[100];
    for (INT i = 0; i < sizeof( data ); i++)
    {
        data[i] = (BYTE)i;
    }
    
    WCHAR wszPath[] = L"MyPath";

    pfile = new CFileFromMemory( data, sizeof( data ), wszPath );
    if ( pfile == NULL )
    {
        return JET_errOutOfMemory;
    }

    // Test path
    WCHAR wszPathReturned[OSFSAPI_MAX_PATH]; 
    OSTestCheck( pfile->ErrPath( wszPathReturned ) == JET_errSuccess );
    OSTestCheck( memcmp( wszPath, wszPathReturned, sizeof( wszPath ) ) == 0 );

    // Check size
    QWORD cbSizeReturned;
    OSTestCheck( pfile->ErrSize( &cbSizeReturned, IFileAPI::filesizeLogical ) == JET_errSuccess );
    QWORD cbSizeReturnedOnDisk;
    OSTestCheck( cbSizeReturned == sizeof( data ) );
    OSTestCheckErr( pfile->ErrSize( &cbSizeReturnedOnDisk, IFileAPI::filesizeOnDisk ) );
    OSTestCheck( cbSizeReturned == cbSizeReturnedOnDisk );

    // Check state
    BOOL fReadOnly = fFalse;
    OSTestCheck( pfile->ErrIsReadOnly( &fReadOnly ) == JET_errSuccess );
    OSTestCheck( fReadOnly == fTrue );
    
    // Read data
    BYTE dataReturned[200];
    err = pfile->ErrIORead( *TraceContextScope( iorpDefault ), 0, sizeof( data ), dataReturned, qosIONormal );
    OSTestCheck( err == JET_errSuccess );
    OSTestCheck( memcmp( data, dataReturned, sizeof( data ) ) == 0 );

    // Read past end
    err = pfile->ErrIORead( *TraceContextScope( iorpDefault ), 1, sizeof( data ), dataReturned, qosIONormal );
    OSTestCheck( err == JET_errFileIOBeyondEOF );

    err = pfile->ErrIORead( *TraceContextScope( iorpDefault ), 0, 1 + sizeof( data ), dataReturned, qosIONormal );
    OSTestCheck( err == JET_errFileIOBeyondEOF );
    
    err = pfile->ErrIORead( *TraceContextScope( iorpDefault ), 0, 2 * sizeof( data ), dataReturned, qosIONormal );
    OSTestCheck( err == JET_errFileIOBeyondEOF );

    err = JET_errSuccess;

HandleError:
    if ( err )
    {
        wprintf( L"\tDone(error).\n");
    }
    else
    {
        wprintf( L"\tDone(success).\n");
    }

    delete pfile;
    pfile = NULL;

    OSTerm();

    return err;
}

