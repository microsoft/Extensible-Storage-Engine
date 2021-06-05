// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  A cached file table entry for the pass through cache.

class CPassThroughCachedFileTableEntry  //  cfte
    :   public CCachedFileTableEntryBase
{
    public:

        CPassThroughCachedFileTableEntry(   _In_ const VolumeId     volumeid,
                                            _In_ const FileId       fileid,
                                            _In_ const FileSerial   fileserial )
            :   CCachedFileTableEntryBase( volumeid, fileid, fileserial ),
                m_pffDisplacedData( NULL )
        {
        }

        ~CPassThroughCachedFileTableEntry()
        {
            delete m_pffDisplacedData;
        }

        IFileFilter* PffDisplacedData() const { return m_pffDisplacedData; }

        ERR ErrOpenCachedFile(  _In_ IFileSystemFilter* const           pfsf,
                                _In_ IFileIdentification* const         pfident,
                                _In_ IBlockCacheConfiguration* const    pbcconfig,
                                _In_ IFileFilter* const                 pffCaching ) override;

    private:
        const WCHAR* const  s_wszDisplacedDataExtension = L"JBCAUX";

        IFileFilter*        m_pffDisplacedData;
};


INLINE ERR CPassThroughCachedFileTableEntry::ErrOpenCachedFile( _In_ IFileSystemFilter* const           pfsf,
                                                                _In_ IFileIdentification* const         pfident,
                                                                _In_ IBlockCacheConfiguration* const    pbcconfig,
                                                                _In_ IFileFilter* const                 pffCaching )
{
    ERR             err                                                     = JET_errSuccess;
    WCHAR           wszAbsCachingFilePath[ IFileSystemAPI::cchPathMax ]     = { 0 };
    WCHAR           wszCachingFileFolder[ IFileSystemAPI::cchPathMax ]      = { 0 };
    WCHAR           wszDisplacedDataFileName[ IFileSystemAPI::cchPathMax ]  = { 0 };
    WCHAR           wszAbsDisplacedDataPath[ IFileSystemAPI::cchPathMax ]   = { 0 };
    const int       cAttemptMax                                             = 10;
    int             cAttempt                                                = 0;
    IFileFilter*    pffDisplacedData                                        = NULL;
    BOOL            fCreated                                                = fFalse;

    //  call the base implementation first

    Call( CCachedFileTableEntryBase::ErrOpenCachedFile( pfsf, pfident, pbcconfig, pffCaching ) );

    //  create/open the file for the storage holding the displaced data.  eventually this will be in the caching file

    Call( pffCaching->ErrPath( wszAbsCachingFilePath ) );
    Call( pfsf->ErrPathParse( wszAbsCachingFilePath, wszCachingFileFolder, NULL, NULL ) );
    swprintf_s( wszDisplacedDataFileName,
                _countof( wszDisplacedDataFileName ),
                L"0x%08x-0x%016I64x-0x%08x",
                Volumeid(),
                Fileid(),
                Fileserial() );
    Call( pfsf->ErrPathBuild(   wszCachingFileFolder,
                                wszDisplacedDataFileName, 
                                s_wszDisplacedDataExtension,
                                wszAbsDisplacedDataPath ) );

    err = JET_errFileAlreadyExists;  //  ErrERRCheck( JET_errFileAlreadyExists ) not appropriate here
    while ( err == JET_errFileAlreadyExists )
    {
        //  if we have retried too many times then fail

        if ( ++cAttempt >= cAttemptMax )
        {
            Call( ErrERRCheck( JET_errInternalError ) );
        }

        //  try to open the file

        err = pfsf->ErrFileOpen(    wszAbsDisplacedDataPath, 
                                    IFileAPI::fmfStorageWriteBack,
                                    (IFileAPI**)&pffDisplacedData );

        //  if we couldn't find the file then try to create it

        if ( err == JET_errFileNotFound )
        {
            err = pfsf->ErrFileCreate(  wszAbsDisplacedDataPath,
                                        IFileAPI::fmfStorageWriteBack, 
                                        (IFileAPI**)&pffDisplacedData );
            fCreated = err >= JET_errSuccess;
        }
    }
    if ( err == JET_errInvalidPath )
    {
        err = ErrERRCheck( JET_errDiskIO );
    }
    Call( err );

    //  make the displaced data file available for other opens

    m_pffDisplacedData = pffDisplacedData;

    pffDisplacedData = NULL;

HandleError:
    delete pffDisplacedData;
    if ( err < JET_errSuccess )
    {
        if ( fCreated )
        {
            (void)pfsf->ErrFileDelete( wszAbsDisplacedDataPath );
        }
    }
    return err;
}
