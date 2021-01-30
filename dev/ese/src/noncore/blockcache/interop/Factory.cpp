// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "stdafx.h"

namespace Internal
{
    namespace Ese
    {
        namespace BlockCache
        {
            namespace Interop
            {
                public ref class Factory
                {
                    public:

                        static FileSystemConfiguration^ WrapFileSystemConfiguration( IFileSystemConfiguration^ ifsconfig )
                        {
                            ERR                         err         = JET_errSuccess;
                            ::IFileSystemConfiguration* pfsconfig   = NULL;

                            Call( FileSystemConfiguration::ErrWrap( ifsconfig, &pfsconfig ) );

                            return gcnew FileSystemConfiguration( &pfsconfig );

                        HandleError:
                            delete pfsconfig;
                            throw EseException( err );
                        }

                        static FileSystem^ WrapFileSystem( IFileSystem^ ifs )
                        {
                            ERR                 err     = JET_errSuccess;
                            ::IFileSystemAPI*   pfsapi  = NULL;

                            Call( FileSystem::ErrWrap( ifs, &pfsapi ) );

                            return gcnew FileSystem( &pfsapi );

                        HandleError:
                            delete pfsapi;
                            throw EseException( err );
                        }

                        static File^ WrapFile( IFile^ f )
                        {
                            ERR         err     = JET_errSuccess;
                            ::IFileAPI* pfapi   = NULL;

                            Call( File::ErrWrap( f, &pfapi ) );

                            return gcnew File( &pfapi );

                        HandleError:
                            delete pfapi;
                            throw EseException( err );
                        }

                        static FileIdentification^ WrapFileIdentification( IFileIdentification^ ifident )
                        {
                            ERR                     err     = JET_errSuccess;
                            ::IFileIdentification*  pfident = NULL;

                            Call( FileIdentification::ErrWrap( ifident, &pfident ) );

                            return gcnew FileIdentification( &pfident );

                        HandleError:
                            delete pfident;
                            throw EseException( err );
                        }

                        static FileSystemFilter^ WrapFileSystemFilter( IFileSystemFilter^ ifsf )
                        {
                            ERR                     err     = JET_errSuccess;
                            ::IFileSystemFilter*    pfsd    = NULL;

                            Call( FileSystemFilter::ErrWrap( ifsf, &pfsd ) );

                            return gcnew FileSystemFilter( &pfsd );

                        HandleError:
                            delete pfsd;
                            throw EseException( err );
                        }

                        static FileFilter^ WrapFileFilter( IFileFilter^ iff )
                        {
                            ERR             err = JET_errSuccess;
                            ::IFileFilter*  pff = NULL;

                            Call( FileFilter::ErrWrap( iff, &pff ) );

                            return gcnew FileFilter( &pff );

                        HandleError:
                            delete pff;
                            throw EseException( err );
                        }

                        static BlockCacheConfiguration^ WrapBlockCacheConfiguration( IBlockCacheConfiguration^ ibcconfig )
                        {
                            ERR                         err         = JET_errSuccess;
                            ::IBlockCacheConfiguration* pbcconfig   = NULL;

                            Call( BlockCacheConfiguration::ErrWrap( ibcconfig, &pbcconfig ) );

                            return gcnew BlockCacheConfiguration( &pbcconfig );

                        HandleError:
                            delete pbcconfig;
                            throw EseException( err );
                        }

                        static CacheConfiguration^ WrapCacheConfiguration( ICacheConfiguration^ icconfig )
                        {
                            ERR                     err         = JET_errSuccess;
                            ::ICacheConfiguration*  pcconfig    = NULL;

                            Call( CacheConfiguration::ErrWrap( icconfig, &pcconfig ) );

                            return gcnew CacheConfiguration( &pcconfig );

                        HandleError:
                            delete pcconfig;
                            throw EseException( err );
                        }

                        static CachedFileConfiguration^ WrapCachedFileConfiguration( ICachedFileConfiguration^ icfconfig )
                        {
                            ERR                         err         = JET_errSuccess;
                            ::ICachedFileConfiguration* pcfconfig   = NULL;

                            Call( CachedFileConfiguration::ErrWrap( icfconfig, &pcfconfig ) );

                            return gcnew CachedFileConfiguration( &pcfconfig );

                        HandleError:
                            delete pcfconfig;
                            throw EseException( err );
                        }

                        static Cache^ WrapCache( ICache^ ic )
                        {
                            ERR         err = JET_errSuccess;
                            ::ICache*   pc  = NULL;

                            Call( Cache::ErrWrap( ic, &pc ) );

                            return gcnew Cache( &pc );

                        HandleError:
                            delete pc;
                            throw EseException( err );
                        }

                        static CacheRepository^ WrapCacheRepository( ICacheRepository^ icrep )
                        {
                            ERR                 err     = JET_errSuccess;
                            ::ICacheRepository* pcrep   = NULL;

                            Call( CacheRepository::ErrWrap( icrep, &pcrep ) );

                            return gcnew CacheRepository( &pcrep );

                        HandleError:
                            delete pcrep;
                            throw EseException( err );
                        }

                        static IDisposable^ CreateOSLayer()
                        {
                            return gcnew OSLayer();
                        }

                        static FileSystem^ CreateFileSystem( FileSystemConfiguration^ fsconfig )
                        {
                            ERR             err     = JET_errSuccess;
                            IFileSystemAPI* pfsapi  = NULL;

                            Call( ErrOSFSCreate( fsconfig->Pi, &pfsapi ) );

                            return gcnew FileSystem( &pfsapi );

                        HandleError:
                            delete pfsapi;
                            throw EseException( err );
                        }

                        static FileSystem^ CreateFileSystemWrapper( IFileSystem^ ifsInner )
                        {
                            ERR             err         = JET_errSuccess;
                            IFileSystemAPI* pfsapiInner = NULL;
                            IFileSystemAPI* pfsapi      = NULL;

                            Call( FileSystem::ErrWrap( ifsInner, &pfsapiInner ) );

                            Call( ErrOSBCCreateFileSystemWrapper( &pfsapiInner, &pfsapi ) );

                            return gcnew FileSystem( &pfsapi );

                        HandleError:
                            delete pfsapi;
                            delete pfsapiInner;
                            throw EseException( err );
                        }

                        static FileSystemFilter^ CreateFileSystemFilter(    FileSystemConfiguration^ fsconfig,
                                                                            IFileSystem^ ifsInner,
                                                                            FileIdentification^ fident,
                                                                            CacheTelemetry^ ctm,
                                                                            CacheRepository^ crep )
                        {
                            ERR                     err         = JET_errSuccess;
                            IFileSystemAPI*         pfsapiInner = NULL;
                            ::IFileSystemFilter*    pfsf        = NULL;

                            Call( FileSystem::ErrWrap( ifsInner, &pfsapiInner ) );

                            Call( ErrOSBCCreateFileSystemFilter(    fsconfig->Pi,
                                                                    &pfsapiInner, 
                                                                    fident->Pi, 
                                                                    ctm->Pi,
                                                                    crep->Pi, 
                                                                    &pfsf ) );

                            return gcnew FileSystemFilter( &pfsf );

                        HandleError:
                            delete pfsapiInner;
                            delete pfsf;
                            throw EseException( err );
                        }

                        static File^ CreateFileWrapper( IFile^ ifInner )
                        {
                            ERR         err         = JET_errSuccess;
                            IFileAPI*   pfapiInner  = NULL;
                            IFileAPI*   pfapi       = NULL;

                            Call( File::ErrWrap( ifInner, &pfapiInner ) );

                            Call( ErrOSBCCreateFileWrapper( &pfapiInner, &pfapi ) );

                            return gcnew File( &pfapi );

                        HandleError:
                            delete pfapiInner;
                            delete pfapi;
                            throw EseException( err );
                        }

                        static FileFilter^ CreateFileFilter(    IFile^ fInner,
                                                                FileSystemFilter^ fsf, 
                                                                FileSystemConfiguration^ fsconfig,
                                                                CacheTelemetry^ ctm,
                                                                VolumeId volumeid,
                                                                FileId fileid,
                                                                ICachedFileConfiguration^ icfconfig,
                                                                ICache^ ic,
                                                                ArraySegment<byte> header )
                        {
                            ERR                         err         = JET_errSuccess;
                            IFileAPI*                   pfapiInner  = NULL;
                            ::ICachedFileConfiguration* pcfconfig   = NULL;
                            ::ICache*                   pc          = NULL;
                            ::IFileFilter*              pff         = NULL;

                            Call( File::ErrWrap( fInner, &pfapiInner ) );
                            Call( CachedFileConfiguration::ErrWrap( icfconfig, &pcfconfig ) );
                            Call( Cache::ErrWrap( ic, &pc ) );

                            pin_ptr<const byte> pbHeader = header.Count == 0 ? nullptr : &header.Array[ header.Offset ];
                            int cbHeader = header.Count;

                            Call( ErrOSBCCreateFileFilter(  &pfapiInner,
                                                            fsf->Pi,
                                                            fsconfig->Pi,
                                                            ctm->Pi,
                                                            (::VolumeId)volumeid,
                                                            (::FileId)fileid,
                                                            &pcfconfig,
                                                            &pc,
                                                            pbHeader, 
                                                            cbHeader,
                                                            &pff ) );

                            return gcnew FileFilter( &pff );

                        HandleError:
                            delete pff;
                            delete pcfconfig;
                            delete pc;
                            delete pfapiInner;
                            throw EseException( err );
                        }

                        static FileFilter^ CreateFileFilterWrapper( IFileFilter^ iffInner, IOMode ioMode )
                        {
                            ERR             err         = JET_errSuccess;
                            ::IFileFilter*  pffInner    = NULL;
                            ::IFileFilter*  pff         = NULL;

                            Call( FileFilter::ErrWrap( iffInner, &pffInner ) );

                            Call( ErrOSBCCreateFileFilterWrapper( &pffInner, (::IFileFilter::IOMode)ioMode, &pff ) );

                            return gcnew FileFilter( &pff );

                        HandleError:
                            delete pffInner;
                            delete pff;
                            throw EseException( err );
                        }

                        static FileIdentification^ CreateFileIdentification()
                        {
                            ERR                     err     = JET_errSuccess;
                            ::IFileIdentification*  pfident = NULL;

                            Call( ErrOSBCCreateFileIdentification( &pfident ) );

                            return gcnew FileIdentification( &pfident );

                        HandleError:
                            delete pfident;
                            throw EseException( err );
                        }

                        static Cache^ CreateCache(  FileSystemFilter^ fsf,
                                                    FileIdentification^ fident,
                                                    FileSystemConfiguration^ fsconfig,
                                                    ICacheConfiguration^ cconfig,
                                                    CacheTelemetry^ ctm,
                                                    IFileFilter^ iff )
                        {
                            ERR                     err         = JET_errSuccess;
                            ::ICacheConfiguration*  pcconfig    = NULL;
                            ::IFileFilter*          pff         = NULL;
                            ::ICache*               pc          = NULL;

                            Call( CacheConfiguration::ErrWrap( cconfig, &pcconfig ) );
                            Call( FileFilter::ErrWrap( iff, &pff ) );

                            Call( ErrOSBCCreateCache( fsf->Pi, fident->Pi, fsconfig->Pi, &pcconfig, ctm->Pi, &pff, &pc ) );

                            return gcnew Cache( &pc );

                        HandleError:
                            delete pff;
                            delete pcconfig;
                            delete pc;
                            throw EseException( err );
                        }

                        static Cache^ CreateCacheWrapper( ICache^ icInner )
                        {
                            ERR         err     = JET_errSuccess;
                            ::ICache*   pcInner = NULL;
                            ::ICache*   pc      = NULL;

                            Call( Cache::ErrWrap( icInner, &pcInner ) );

                            Call( ErrOSBCCreateCacheWrapper( &pcInner, &pc ) );

                            return gcnew Cache( &pc );

                        HandleError:
                            delete pcInner;
                            delete pc;
                            throw EseException( err );
                        }

                        static CacheRepository^ CreateCacheRepository(  FileIdentification^ fident,
                                                                        CacheTelemetry^ ctm )
                        {
                            ERR                 err     = JET_errSuccess;
                            ::ICacheRepository* pcrep   = NULL;

                            Call( ErrOSBCCreateCacheRepository( fident->Pi, ctm->Pi, &pcrep ) );

                            return gcnew CacheRepository( &pcrep );

                        HandleError:
                            delete pcrep;
                            throw EseException( err );
                        }

                        static CacheTelemetry^ CreateCacheTelemetry()
                        {
                            ERR                 err     = JET_errSuccess;
                            ::ICacheTelemetry*  pctm    = NULL;

                            Call( ErrOSBCCreateCacheTelemetry( &pctm ) );

                            return gcnew CacheTelemetry( &pctm );

                        HandleError:
                            delete pctm;
                            throw EseException( err );
                        }

                        static CacheTelemetry^ WrapCacheTelemetry( ICacheTelemetry^ ictm )
                        {
                            ERR                 err     = JET_errSuccess;
                            ::ICacheTelemetry*  pctm    = NULL;

                            Call( CacheTelemetry::ErrWrap( ictm, &pctm ) );

                            return gcnew CacheTelemetry( &pctm );

                        HandleError:
                            delete pctm;
                            throw EseException( err );
                        }
                        
                        static JournalSegment^ CreateJournalSegment(
                            FileFilter^ ff,
                            Int64 offsetInBytes,
                            SegmentPosition segmentPosition,
                            Int32 uniqueIdPrevious,
                            SegmentPosition segmentPositionReplay,
                            SegmentPosition segmentPositionDurable )
                        {
                            ERR                 err = JET_errSuccess;
                            ::IJournalSegment*  pjs = NULL;

                            Call( ErrOSBCCreateJournalSegment(  ff->Pi,
                                                                offsetInBytes,
                                                                (::SegmentPosition)segmentPosition,
                                                                uniqueIdPrevious,
                                                                (::SegmentPosition)segmentPositionReplay,
                                                                (::SegmentPosition)segmentPositionDurable,
                                                                &pjs ) );

                            return gcnew JournalSegment( &pjs );

                        HandleError:
                            delete pjs;
                            throw EseException( err );
                        }
                        
                        static JournalSegment^ LoadJournalSegment(
                            FileFilter^ ff,
                            Int64 offsetInBytes )
                        {
                            ERR                 err = JET_errSuccess;
                            ::IJournalSegment*  pjs = NULL;

                            Call( ErrOSBCLoadJournalSegment( ff->Pi, offsetInBytes, &pjs ) );

                            return gcnew JournalSegment( &pjs );

                        HandleError:
                            delete pjs;
                            throw EseException( err );
                        }
                        
                        static JournalSegment^ WrapJournalSegment( IJournalSegment^ ijs )
                        {
                            ERR                 err = JET_errSuccess;
                            ::IJournalSegment*  pjs = NULL;

                            Call( JournalSegment::ErrWrap( ijs, &pjs ) );

                            return gcnew JournalSegment( &pjs );

                        HandleError:
                            delete pjs;
                            throw EseException( err );
                        }
                        
                        static JournalSegmentManager^ CreateJournalSegmentManager(
                            FileFilter^ ff,
                            Int64 offsetInBytes,
                            Int64 sizeInBytes )
                        {
                            ERR                         err     = JET_errSuccess;
                            ::IJournalSegmentManager*   pjsm    = NULL;

                            Call( ErrOSBCCreateJournalSegmentManager(   ff->Pi,
                                                                        offsetInBytes,
                                                                        sizeInBytes,
                                                                        &pjsm ) );

                            return gcnew JournalSegmentManager( &pjsm );

                        HandleError:
                            delete pjsm;
                            throw EseException( err );
                        }
                        
                        static JournalSegmentManager^ WrapJournalSegmentManager( IJournalSegmentManager^ ijsm )
                        {
                            ERR                         err     = JET_errSuccess;
                            ::IJournalSegmentManager*   pjsm    = NULL;

                            Call( JournalSegmentManager::ErrWrap( ijsm, &pjsm ) );

                            return gcnew JournalSegmentManager( &pjsm );

                        HandleError:
                            delete pjsm;
                            throw EseException( err );
                        }
                        
                        static Journal^ CreateJournal(
                            IJournalSegmentManager^ ijsm,
                            Int64 cacheSizeInBytes )
                        {
                            ERR                         err     = JET_errSuccess;
                            ::IJournalSegmentManager*   pjsm    = NULL;
                            ::IJournal*                 pj      = NULL;

                            Call( JournalSegmentManager::ErrWrap( ijsm, &pjsm ) );

                            Call( ErrOSBCCreateJournal( &pjsm,
                                                        cacheSizeInBytes,
                                                        &pj ) );

                            return gcnew Journal( &pj );

                        HandleError:
                            delete pj;
                            delete pjsm;
                            throw EseException( err );
                        }
                        
                        static Journal^ WrapJournal( IJournal^ ij )
                        {
                            ERR         err = JET_errSuccess;
                            ::IJournal* pj  = NULL;

                            Call( Journal::ErrWrap( ij, &pj ) );

                            return gcnew Journal( &pj );

                        HandleError:
                            delete pj;
                            throw EseException( err );
                        }

                    private:

                        ref class OSLayer : IDisposable
                        {
                            public:
                            
                                OSLayer()
                                {
                                    COSLayerPreInit::SetDefaults();
                                    ERR err = ErrOSInit();
                                    if ( err < JET_errSuccess )
                                    {
                                        throw EsentExceptionHelper::JetErrToException( (JET_err)err );
                                    }
                                }

                                ~OSLayer()
                                {
                                    OSTerm();
                                }
                        };

                };
            }
        }
    }
}
