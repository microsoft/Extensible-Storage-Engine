// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

//  ================================================================
CFileFromMemory::CFileFromMemory( __in_bcount( cbData ) BYTE *pbData, QWORD cbData, _In_ PCWSTR wszPath ) :
//  ================================================================
    m_pbBuffer( pbData ),
    m_cbBuffer( cbData ),
    m_pbBufferToFree( NULL )
{
    OSStrCbCopyW( m_wszPath, sizeof(m_wszPath), wszPath );
}

//  ================================================================
CFileFromMemory::~CFileFromMemory()
//  ================================================================
{
    delete m_pbBufferToFree;
    m_pbBuffer       = NULL;
    m_cbBuffer       = 0;
    m_pbBufferToFree = NULL;
}

//  ================================================================
ERR CFileFromMemory::ErrPath( _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) WCHAR* const wszAbsPath )
//  ================================================================
{
    OSStrCbCopyW( wszAbsPath, OSFSAPI_MAX_PATH * sizeof(WCHAR), m_wszPath );

    return JET_errSuccess;
}

//  ================================================================
ERR CFileFromMemory::ErrSize(
    _Out_ QWORD* const pcbSize,
    _In_ const FILESIZE filesizeIgnored )
//  ================================================================
{
    UNREFERENCED_PARAMETER( filesizeIgnored );
    // No compressed/sparse files in memory; 
    *pcbSize = m_cbBuffer;
    return JET_errSuccess;
}

//  ================================================================
ERR CFileFromMemory::ErrIsReadOnly( BOOL* const pfReadOnly )
//  ================================================================
{
    *pfReadOnly = fTrue;
    return JET_errSuccess;
}

//  ================================================================
ERR CFileFromMemory::ErrIORead( const TraceContext& tc,
                                const QWORD         ibOffset,
                                const DWORD         cbData,
        __out_bcount( cbData )  BYTE* const         pbData,
                                const OSFILEQOS     grbitQOS,
                                const PfnIOComplete pfnIOComplete,
                                const DWORD_PTR     keyIOComplete,
                                const PfnIOHandoff  pfnIOHandoff,
                                const VOID *        pioreq )
//  ================================================================
{
    AssertRTL( tc.iorReason.FValid() );
    Assert( pfnIOComplete == NULL );
    Assert( pfnIOHandoff == NULL );
    Assert( pioreq == NULL );

    if ( ibOffset > m_cbBuffer ||
         ibOffset + cbData > m_cbBuffer )
    {

        return ErrERRCheck( JET_errFileIOBeyondEOF );
    }
    
    memcpy( pbData, m_pbBuffer + ibOffset, cbData);

    return JET_errSuccess;
}


