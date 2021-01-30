// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once


using namespace System;
using namespace System::Security::Permissions;

namespace Microsoft
{
#if (ESENT)
namespace Windows
#else
namespace Exchange
#endif
{
namespace Isam
{




    [Serializable]
    public ref class IsamErrorException : public SystemException, public System::Runtime::Serialization::ISerializable
    {
    private:
        JET_ERR     m_err;
        String ^        m_description;

    protected:
        IsamErrorException( String ^ description, Exception^ innerException ) :
            SystemException( description, innerException )
        {
        }

    public:
        property JET_ERR    Err
        {
            JET_ERR get()
                {
                return m_err;
                }
        }

        property String ^   Description
        {
            String ^    get ( )
                {
                return m_description;
                }
        }

        IsamErrorException( String ^ description, JET_ERR err ) :
            m_err(err),
            m_description( description ),
            SystemException( String::Format( L"{0} ({1})", description, err ) )
        {
        }

        IsamErrorException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : SystemException( info, context )
        {
            m_err = info->GetInt32( "m_err" );
            m_description = info->GetString( "m_description" );
        }

        [SecurityPermission(SecurityAction::LinkDemand, Flags = SecurityPermissionFlag::SerializationFormatter)]
        virtual void GetObjectData(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        ) override
        {
            __super::GetObjectData( info, context );
            info->AddValue( "m_err", m_err );
            info->AddValue( "m_description", m_description );
        }
    

    };




    [Serializable]
    public ref class IsamOperationException : public IsamErrorException
    {
    protected:
        IsamOperationException( String ^ description, Exception^ innerException ) :
            IsamErrorException( description, innerException )
        {
        }

    public:
        IsamOperationException( String ^ description, JET_ERR err ) : IsamErrorException( description, err ) { }

        IsamOperationException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamErrorException( info, context )
        {
        }
    };



    [Serializable]
    public ref class IsamDataException : public IsamErrorException
    {
    protected:
        IsamDataException( String ^ description, Exception^ innerException ) :
            IsamErrorException( description, innerException )
        {
        }

    public:
        IsamDataException( String ^ description, JET_ERR err ) : IsamErrorException( description, err ) { }

        IsamDataException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamErrorException( info, context )
        {
        }
    };



    [Serializable]
    public ref class IsamApiException : public IsamErrorException
    {
    protected:
        IsamApiException( String ^ description, Exception^ innerException ) :
            IsamErrorException( description, innerException )
        {
        }

    public:
        IsamApiException( String ^ description, JET_ERR err ) : IsamErrorException( description, err ) { }

        IsamApiException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamErrorException( info, context )
        {
        }
    };



    [Serializable]
    public ref class IsamFatalException : public IsamOperationException
    {
    protected:
        IsamFatalException( String ^ description, Exception^ innerException ) :
            IsamOperationException( description, innerException )
        {
        }

    public:
        IsamFatalException( String ^ description, JET_ERR err ) : IsamOperationException( description, err ) { }

        IsamFatalException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamOperationException( info, context )
        {
        }
    };



    [Serializable]
    public ref class IsamIOException : public IsamOperationException
    {
    protected:
        IsamIOException( String ^ description, Exception^ innerException ) :
            IsamOperationException( description, innerException )
        {
        }

    public:
        IsamIOException( String ^ description, JET_ERR err ) : IsamOperationException( description, err ) { }

        IsamIOException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamOperationException( info, context )
        {
        }
    };



    [Serializable]
    public ref class IsamResourceException : public IsamOperationException
    {
    protected:
        IsamResourceException( String ^ description, Exception^ innerException ) :
            IsamOperationException( description, innerException )
        {
        }

    public:
        IsamResourceException( String ^ description, JET_ERR err ) : IsamOperationException( description, err ) { }

        IsamResourceException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamOperationException( info, context )
        {
        }
    };



    [Serializable]
    public ref class IsamMemoryException : public IsamResourceException
    {
    protected:
        IsamMemoryException( String ^ description, Exception^ innerException ) :
            IsamResourceException( description, innerException )
        {
        }

    public:
        IsamMemoryException( String ^ description, JET_ERR err ) : IsamResourceException( description, err ) { }

        IsamMemoryException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamResourceException( info, context )
        {
        }
    };



    [Serializable]
    public ref class IsamQuotaException : public IsamResourceException
    {
    protected:
        IsamQuotaException( String ^ description, Exception^ innerException ) :
            IsamResourceException( description, innerException )
        {
        }

    public:
        IsamQuotaException( String ^ description, JET_ERR err ) : IsamResourceException( description, err ) { }

        IsamQuotaException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamResourceException( info, context )
        {
        }
    };



    [Serializable]
    public ref class IsamDiskException : public IsamResourceException
    {
    protected:
        IsamDiskException( String ^ description, Exception^ innerException ) :
            IsamResourceException( description, innerException )
        {
        }

    public:
        IsamDiskException( String ^ description, JET_ERR err ) : IsamResourceException( description, err ) { }

        IsamDiskException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamResourceException( info, context )
        {
        }
    };



    [Serializable]
    public ref class IsamCorruptionException : public IsamDataException
    {
    protected:
        IsamCorruptionException( String ^ description, Exception^ innerException ) :
            IsamDataException( description, innerException )
        {
        }

    public:
        IsamCorruptionException( String ^ description, JET_ERR err ) : IsamDataException( description, err ) { }

        IsamCorruptionException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamDataException( info, context )
        {
        }
    };



    [Serializable]
    public ref class IsamInconsistentException : public IsamDataException
    {
    protected:
        IsamInconsistentException( String ^ description, Exception^ innerException ) :
            IsamDataException( description, innerException )
        {
        }

    public:
        IsamInconsistentException( String ^ description, JET_ERR err ) : IsamDataException( description, err ) { }

        IsamInconsistentException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamDataException( info, context )
        {
        }
    };



    [Serializable]
    public ref class IsamFragmentationException : public IsamDataException
    {
    protected:
        IsamFragmentationException( String ^ description, Exception^ innerException ) :
            IsamDataException( description, innerException )
        {
        }

    public:
        IsamFragmentationException( String ^ description, JET_ERR err ) : IsamDataException( description, err ) { }

        IsamFragmentationException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamDataException( info, context )
        {
        }
    };



    [Serializable]
    public ref class IsamUsageException : public IsamApiException
    {
    protected:
        IsamUsageException( String ^ description, Exception^ innerException ) :
            IsamApiException( description, innerException )
        {
        }

    public:
        IsamUsageException( String ^ description, JET_ERR err ) : IsamApiException( description, err ) { }

        IsamUsageException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamApiException( info, context )
        {
        }
    };



    [Serializable]
    public ref class IsamStateException : public IsamApiException
    {
    protected:
        IsamStateException( String ^ description, Exception^ innerException ) :
            IsamApiException( description, innerException )
        {
        }

    public:
        IsamStateException( String ^ description, JET_ERR err ) : IsamApiException( description, err ) { }

        IsamStateException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamApiException( info, context )
        {
        }
    };



    [Serializable]
    public ref class IsamObsoleteException : public IsamApiException
    {
    protected:
        IsamObsoleteException( String ^ description, Exception^ innerException ) :
            IsamApiException( description, innerException )
        {
        }

    public:
        IsamObsoleteException( String ^ description, JET_ERR err ) : IsamApiException( description, err ) { }

        IsamObsoleteException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamApiException( info, context )
        {
        }
    };





    [Serializable]
    public ref class IsamOutOfThreadsException : public IsamMemoryException
    {
    public:
        IsamOutOfThreadsException() : IsamMemoryException( "Could not start thread", JET_errOutOfThreads)
        {
        }

        IsamOutOfThreadsException( String ^ description, Exception^ innerException ) :
            IsamMemoryException( description, innerException )
        {
        }

        IsamOutOfThreadsException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamMemoryException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTooManyIOException : public IsamResourceException
    {
    public:
        IsamTooManyIOException() : IsamResourceException( "System busy due to too many IOs", JET_errTooManyIO)
        {
        }

        IsamTooManyIOException( String ^ description, Exception^ innerException ) :
            IsamResourceException( description, innerException )
        {
        }

        IsamTooManyIOException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamResourceException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTaskDroppedException : public IsamResourceException
    {
    public:
        IsamTaskDroppedException() : IsamResourceException( "A requested async task could not be executed", JET_errTaskDropped)
        {
        }

        IsamTaskDroppedException( String ^ description, Exception^ innerException ) :
            IsamResourceException( description, innerException )
        {
        }

        IsamTaskDroppedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamResourceException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInternalErrorException : public IsamOperationException
    {
    public:
        IsamInternalErrorException() : IsamOperationException( "Fatal internal error", JET_errInternalError)
        {
        }

        IsamInternalErrorException( String ^ description, Exception^ innerException ) :
            IsamOperationException( description, innerException )
        {
        }

        IsamInternalErrorException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamOperationException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDisabledFunctionalityException : public IsamUsageException
    {
    public:
        IsamDisabledFunctionalityException() : IsamUsageException( "You are running MinESE, that does not have all features compiled in.  This functionality is only supported in a full version of ESE.", JET_errDisabledFunctionality)
        {
        }

        IsamDisabledFunctionalityException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamDisabledFunctionalityException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamUnloadableOSFunctionalityException : public IsamFatalException
    {
    public:
        IsamUnloadableOSFunctionalityException() : IsamFatalException( "The desired OS functionality could not be located and loaded / linked.", JET_errUnloadableOSFunctionality)
        {
        }

        IsamUnloadableOSFunctionalityException( String ^ description, Exception^ innerException ) :
            IsamFatalException( description, innerException )
        {
        }

        IsamUnloadableOSFunctionalityException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamFatalException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDeviceMissingException : public IsamFatalException
    {
    public:
        IsamDeviceMissingException() : IsamFatalException( "A required hardware device or functionality was missing.", JET_errDeviceMissing)
        {
        }

        IsamDeviceMissingException( String ^ description, Exception^ innerException ) :
            IsamFatalException( description, innerException )
        {
        }

        IsamDeviceMissingException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamFatalException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDeviceMisconfiguredException : public IsamFatalException
    {
    public:
        IsamDeviceMisconfiguredException() : IsamFatalException( "A required hardware device was misconfigured externally.", JET_errDeviceMisconfigured)
        {
        }

        IsamDeviceMisconfiguredException( String ^ description, Exception^ innerException ) :
            IsamFatalException( description, innerException )
        {
        }

        IsamDeviceMisconfiguredException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamFatalException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDeviceTimeoutException : public IsamOperationException
    {
    public:
        IsamDeviceTimeoutException() : IsamOperationException( "Timeout occurred while waiting for a hardware device to respond.", JET_errDeviceTimeout)
        {
        }

        IsamDeviceTimeoutException( String ^ description, Exception^ innerException ) :
            IsamOperationException( description, innerException )
        {
        }

        IsamDeviceTimeoutException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamOperationException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDeviceFailureException : public IsamOperationException
    {
    public:
        IsamDeviceFailureException() : IsamOperationException( "A required hardware device didn't function as expected.", JET_errDeviceFailure)
        {
        }

        IsamDeviceFailureException( String ^ description, Exception^ innerException ) :
            IsamOperationException( description, innerException )
        {
        }

        IsamDeviceFailureException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamOperationException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseBufferDependenciesCorruptedException : public IsamCorruptionException
    {
    public:
        IsamDatabaseBufferDependenciesCorruptedException() : IsamCorruptionException( "Buffer dependencies improperly set. Recovery failure", JET_errDatabaseBufferDependenciesCorrupted)
        {
        }

        IsamDatabaseBufferDependenciesCorruptedException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamDatabaseBufferDependenciesCorruptedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamBadPageLinkException : public IsamCorruptionException
    {
    public:
        IsamBadPageLinkException() : IsamCorruptionException( "Database corrupted", JET_errBadPageLink)
        {
        }

        IsamBadPageLinkException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamBadPageLinkException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamNTSystemCallFailedException : public IsamOperationException
    {
    public:
        IsamNTSystemCallFailedException() : IsamOperationException( "A call to the operating system failed", JET_errNTSystemCallFailed)
        {
        }

        IsamNTSystemCallFailedException( String ^ description, Exception^ innerException ) :
            IsamOperationException( description, innerException )
        {
        }

        IsamNTSystemCallFailedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamOperationException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamBadParentPageLinkException : public IsamCorruptionException
    {
    public:
        IsamBadParentPageLinkException() : IsamCorruptionException( "Database corrupted", JET_errBadParentPageLink)
        {
        }

        IsamBadParentPageLinkException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamBadParentPageLinkException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamSPAvailExtCorruptedException : public IsamCorruptionException
    {
    public:
        IsamSPAvailExtCorruptedException() : IsamCorruptionException( "AvailExt space tree is corrupt", JET_errSPAvailExtCorrupted)
        {
        }

        IsamSPAvailExtCorruptedException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamSPAvailExtCorruptedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamSPOwnExtCorruptedException : public IsamCorruptionException
    {
    public:
        IsamSPOwnExtCorruptedException() : IsamCorruptionException( "OwnExt space tree is corrupt", JET_errSPOwnExtCorrupted)
        {
        }

        IsamSPOwnExtCorruptedException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamSPOwnExtCorruptedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDbTimeCorruptedException : public IsamCorruptionException
    {
    public:
        IsamDbTimeCorruptedException() : IsamCorruptionException( "Dbtime on current page is greater than global database dbtime", JET_errDbTimeCorrupted)
        {
        }

        IsamDbTimeCorruptedException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamDbTimeCorruptedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamKeyTruncatedException : public IsamStateException
    {
    public:
        IsamKeyTruncatedException() : IsamStateException( "key truncated on index that disallows key truncation", JET_errKeyTruncated)
        {
        }

        IsamKeyTruncatedException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamKeyTruncatedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseLeakInSpaceException : public IsamStateException
    {
    public:
        IsamDatabaseLeakInSpaceException() : IsamStateException( "Some database pages have become unreachable even from the avail tree, only an offline defragmentation can return the lost space.", JET_errDatabaseLeakInSpace)
        {
        }

        IsamDatabaseLeakInSpaceException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamDatabaseLeakInSpaceException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamBadEmptyPageException : public IsamCorruptionException
    {
    public:
        IsamBadEmptyPageException() : IsamCorruptionException( "Database corrupted. Searching an unexpectedly empty page.", JET_errBadEmptyPage)
        {
        }

        IsamBadEmptyPageException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamBadEmptyPageException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamBadLineCountException : public IsamCorruptionException
    {
    public:
        IsamBadLineCountException() : IsamCorruptionException( "Number of lines on the page is too few compared to the line being operated on", JET_errBadLineCount)
        {
        }

        IsamBadLineCountException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamBadLineCountException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamPageTagCorruptedException : public IsamCorruptionException
    {
    public:
        IsamPageTagCorruptedException() : IsamCorruptionException( "A tag / line on page is logically corrupted, offset or size is bad, or tag count on page is bad.", JET_errPageTagCorrupted)
        {
        }

        IsamPageTagCorruptedException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamPageTagCorruptedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamNodeCorruptedException : public IsamCorruptionException
    {
    public:
        IsamNodeCorruptedException() : IsamCorruptionException( "A node or prefix node is logically corrupted, the key suffix size is larger than the node or line's size.", JET_errNodeCorrupted)
        {
        }

        IsamNodeCorruptedException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamNodeCorruptedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamCannotSeparateIntrinsicLVException : public IsamUsageException
    {
    public:
        IsamCannotSeparateIntrinsicLVException() : IsamUsageException( "illegal attempt to separate an LV which must be intrinsic", JET_errCannotSeparateIntrinsicLV)
        {
        }

        IsamCannotSeparateIntrinsicLVException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamCannotSeparateIntrinsicLVException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamSeparatedLongValueException : public IsamStateException
    {
    public:
        IsamSeparatedLongValueException() : IsamStateException( "Operation not supported on separated long-value", JET_errSeparatedLongValue)
        {
        }

        IsamSeparatedLongValueException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamSeparatedLongValueException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamMustBeSeparateLongValueException : public IsamUsageException
    {
    public:
        IsamMustBeSeparateLongValueException() : IsamUsageException( "Can only preread long value columns that can be separate, e.g. not size constrained so that they are fixed or variable columns", JET_errMustBeSeparateLongValue)
        {
        }

        IsamMustBeSeparateLongValueException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamMustBeSeparateLongValueException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidPrereadException : public IsamUsageException
    {
    public:
        IsamInvalidPrereadException() : IsamUsageException( "Cannot preread long values when current index secondary", JET_errInvalidPreread)
        {
        }

        IsamInvalidPrereadException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidPrereadException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidColumnReferenceException : public IsamStateException
    {
    public:
        IsamInvalidColumnReferenceException() : IsamStateException( "Column reference is invalid", JET_errInvalidColumnReference)
        {
        }

        IsamInvalidColumnReferenceException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamInvalidColumnReferenceException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamStaleColumnReferenceException : public IsamStateException
    {
    public:
        IsamStaleColumnReferenceException() : IsamStateException( "Column reference is stale", JET_errStaleColumnReference)
        {
        }

        IsamStaleColumnReferenceException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamStaleColumnReferenceException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamCompressionIntegrityCheckFailedException : public IsamCorruptionException
    {
    public:
        IsamCompressionIntegrityCheckFailedException() : IsamCorruptionException( "A compression integrity check failed. Decompressing data failed the integrity checksum indicating a data corruption in the compress/decompress pipeline.", JET_errCompressionIntegrityCheckFailed)
        {
        }

        IsamCompressionIntegrityCheckFailedException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamCompressionIntegrityCheckFailedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLogFileCorruptException : public IsamCorruptionException
    {
    public:
        IsamLogFileCorruptException() : IsamCorruptionException( "Log file is corrupt", JET_errLogFileCorrupt)
        {
        }

        IsamLogFileCorruptException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamLogFileCorruptException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamNoBackupDirectoryException : public IsamUsageException
    {
    public:
        IsamNoBackupDirectoryException() : IsamUsageException( "No backup directory given", JET_errNoBackupDirectory)
        {
        }

        IsamNoBackupDirectoryException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamNoBackupDirectoryException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamBackupDirectoryNotEmptyException : public IsamUsageException
    {
    public:
        IsamBackupDirectoryNotEmptyException() : IsamUsageException( "The backup directory is not empty", JET_errBackupDirectoryNotEmpty)
        {
        }

        IsamBackupDirectoryNotEmptyException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamBackupDirectoryNotEmptyException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamBackupInProgressException : public IsamStateException
    {
    public:
        IsamBackupInProgressException() : IsamStateException( "Backup is active already", JET_errBackupInProgress)
        {
        }

        IsamBackupInProgressException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamBackupInProgressException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRestoreInProgressException : public IsamStateException
    {
    public:
        IsamRestoreInProgressException() : IsamStateException( "Restore in progress", JET_errRestoreInProgress)
        {
        }

        IsamRestoreInProgressException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamRestoreInProgressException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamMissingPreviousLogFileException : public IsamCorruptionException
    {
    public:
        IsamMissingPreviousLogFileException() : IsamCorruptionException( "Missing the log file for check point", JET_errMissingPreviousLogFile)
        {
        }

        IsamMissingPreviousLogFileException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamMissingPreviousLogFileException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLogWriteFailException : public IsamIOException
    {
    public:
        IsamLogWriteFailException() : IsamIOException( "Failure writing to log file", JET_errLogWriteFail)
        {
        }

        IsamLogWriteFailException( String ^ description, Exception^ innerException ) :
            IsamIOException( description, innerException )
        {
        }

        IsamLogWriteFailException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamIOException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLogDisabledDueToRecoveryFailureException : public IsamFatalException
    {
    public:
        IsamLogDisabledDueToRecoveryFailureException() : IsamFatalException( "Try to log something after recovery failed", JET_errLogDisabledDueToRecoveryFailure)
        {
        }

        IsamLogDisabledDueToRecoveryFailureException( String ^ description, Exception^ innerException ) :
            IsamFatalException( description, innerException )
        {
        }

        IsamLogDisabledDueToRecoveryFailureException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamFatalException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLogGenerationMismatchException : public IsamInconsistentException
    {
    public:
        IsamLogGenerationMismatchException() : IsamInconsistentException( "Name of logfile does not match internal generation number", JET_errLogGenerationMismatch)
        {
        }

        IsamLogGenerationMismatchException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamLogGenerationMismatchException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamBadLogVersionException : public IsamInconsistentException
    {
    public:
        IsamBadLogVersionException() : IsamInconsistentException( "Version of log file is not compatible with Jet version", JET_errBadLogVersion)
        {
        }

        IsamBadLogVersionException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamBadLogVersionException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidLogSequenceException : public IsamCorruptionException
    {
    public:
        IsamInvalidLogSequenceException() : IsamCorruptionException( "Timestamp in next log does not match expected", JET_errInvalidLogSequence)
        {
        }

        IsamInvalidLogSequenceException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamInvalidLogSequenceException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLoggingDisabledException : public IsamUsageException
    {
    public:
        IsamLoggingDisabledException() : IsamUsageException( "Log is not active", JET_errLoggingDisabled)
        {
        }

        IsamLoggingDisabledException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamLoggingDisabledException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLogSequenceEndException : public IsamFragmentationException
    {
    public:
        IsamLogSequenceEndException() : IsamFragmentationException( "Maximum log file number exceeded", JET_errLogSequenceEnd)
        {
        }

        IsamLogSequenceEndException( String ^ description, Exception^ innerException ) :
            IsamFragmentationException( description, innerException )
        {
        }

        IsamLogSequenceEndException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamFragmentationException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamNoBackupException : public IsamStateException
    {
    public:
        IsamNoBackupException() : IsamStateException( "No backup in progress", JET_errNoBackup)
        {
        }

        IsamNoBackupException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamNoBackupException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidBackupSequenceException : public IsamUsageException
    {
    public:
        IsamInvalidBackupSequenceException() : IsamUsageException( "Backup call out of sequence", JET_errInvalidBackupSequence)
        {
        }

        IsamInvalidBackupSequenceException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidBackupSequenceException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamBackupNotAllowedYetException : public IsamStateException
    {
    public:
        IsamBackupNotAllowedYetException() : IsamStateException( "Cannot do backup now", JET_errBackupNotAllowedYet)
        {
        }

        IsamBackupNotAllowedYetException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamBackupNotAllowedYetException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDeleteBackupFileFailException : public IsamIOException
    {
    public:
        IsamDeleteBackupFileFailException() : IsamIOException( "Could not delete backup file", JET_errDeleteBackupFileFail)
        {
        }

        IsamDeleteBackupFileFailException( String ^ description, Exception^ innerException ) :
            IsamIOException( description, innerException )
        {
        }

        IsamDeleteBackupFileFailException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamIOException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamMakeBackupDirectoryFailException : public IsamIOException
    {
    public:
        IsamMakeBackupDirectoryFailException() : IsamIOException( "Could not make backup temp directory", JET_errMakeBackupDirectoryFail)
        {
        }

        IsamMakeBackupDirectoryFailException( String ^ description, Exception^ innerException ) :
            IsamIOException( description, innerException )
        {
        }

        IsamMakeBackupDirectoryFailException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamIOException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidBackupException : public IsamUsageException
    {
    public:
        IsamInvalidBackupException() : IsamUsageException( "Cannot perform incremental backup when circular logging enabled", JET_errInvalidBackup)
        {
        }

        IsamInvalidBackupException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidBackupException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRecoveredWithErrorsException : public IsamStateException
    {
    public:
        IsamRecoveredWithErrorsException() : IsamStateException( "Restored with errors", JET_errRecoveredWithErrors)
        {
        }

        IsamRecoveredWithErrorsException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamRecoveredWithErrorsException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamMissingLogFileException : public IsamCorruptionException
    {
    public:
        IsamMissingLogFileException() : IsamCorruptionException( "Current log file missing", JET_errMissingLogFile)
        {
        }

        IsamMissingLogFileException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamMissingLogFileException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLogDiskFullException : public IsamDiskException
    {
    public:
        IsamLogDiskFullException() : IsamDiskException( "Log disk full", JET_errLogDiskFull)
        {
        }

        IsamLogDiskFullException( String ^ description, Exception^ innerException ) :
            IsamDiskException( description, innerException )
        {
        }

        IsamLogDiskFullException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamDiskException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamBadLogSignatureException : public IsamInconsistentException
    {
    public:
        IsamBadLogSignatureException() : IsamInconsistentException( "Bad signature for a log file", JET_errBadLogSignature)
        {
        }

        IsamBadLogSignatureException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamBadLogSignatureException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamBadCheckpointSignatureException : public IsamInconsistentException
    {
    public:
        IsamBadCheckpointSignatureException() : IsamInconsistentException( "Bad signature for a checkpoint file", JET_errBadCheckpointSignature)
        {
        }

        IsamBadCheckpointSignatureException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamBadCheckpointSignatureException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamCheckpointCorruptException : public IsamCorruptionException
    {
    public:
        IsamCheckpointCorruptException() : IsamCorruptionException( "Checkpoint file not found or corrupt", JET_errCheckpointCorrupt)
        {
        }

        IsamCheckpointCorruptException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamCheckpointCorruptException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamBadPatchPageException : public IsamInconsistentException
    {
    public:
        IsamBadPatchPageException() : IsamInconsistentException( "Patch file page is not valid", JET_errBadPatchPage)
        {
        }

        IsamBadPatchPageException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamBadPatchPageException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRedoAbruptEndedException : public IsamCorruptionException
    {
    public:
        IsamRedoAbruptEndedException() : IsamCorruptionException( "Redo abruptly ended due to sudden failure in reading logs from log file", JET_errRedoAbruptEnded)
        {
        }

        IsamRedoAbruptEndedException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamRedoAbruptEndedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseLogSetMismatchException : public IsamInconsistentException
    {
    public:
        IsamDatabaseLogSetMismatchException() : IsamInconsistentException( "Database does not belong with the current set of log files", JET_errDatabaseLogSetMismatch)
        {
        }

        IsamDatabaseLogSetMismatchException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamDatabaseLogSetMismatchException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLogFileSizeMismatchException : public IsamUsageException
    {
    public:
        IsamLogFileSizeMismatchException() : IsamUsageException( "actual log file size does not match JET_paramLogFileSize", JET_errLogFileSizeMismatch)
        {
        }

        IsamLogFileSizeMismatchException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamLogFileSizeMismatchException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamCheckpointFileNotFoundException : public IsamInconsistentException
    {
    public:
        IsamCheckpointFileNotFoundException() : IsamInconsistentException( "Could not locate checkpoint file", JET_errCheckpointFileNotFound)
        {
        }

        IsamCheckpointFileNotFoundException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamCheckpointFileNotFoundException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRequiredLogFilesMissingException : public IsamInconsistentException
    {
    public:
        IsamRequiredLogFilesMissingException() : IsamInconsistentException( "The required log files for recovery is missing.", JET_errRequiredLogFilesMissing)
        {
        }

        IsamRequiredLogFilesMissingException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamRequiredLogFilesMissingException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamSoftRecoveryOnBackupDatabaseException : public IsamUsageException
    {
    public:
        IsamSoftRecoveryOnBackupDatabaseException() : IsamUsageException( "Soft recovery is intended on a backup database. Restore should be used instead", JET_errSoftRecoveryOnBackupDatabase)
        {
        }

        IsamSoftRecoveryOnBackupDatabaseException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamSoftRecoveryOnBackupDatabaseException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLogFileSizeMismatchDatabasesConsistentException : public IsamStateException
    {
    public:
        IsamLogFileSizeMismatchDatabasesConsistentException() : IsamStateException( "databases have been recovered, but the log file size used during recovery does not match JET_paramLogFileSize", JET_errLogFileSizeMismatchDatabasesConsistent)
        {
        }

        IsamLogFileSizeMismatchDatabasesConsistentException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamLogFileSizeMismatchDatabasesConsistentException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLogSectorSizeMismatchException : public IsamFragmentationException
    {
    public:
        IsamLogSectorSizeMismatchException() : IsamFragmentationException( "the log file sector size does not match the current volume's sector size", JET_errLogSectorSizeMismatch)
        {
        }

        IsamLogSectorSizeMismatchException( String ^ description, Exception^ innerException ) :
            IsamFragmentationException( description, innerException )
        {
        }

        IsamLogSectorSizeMismatchException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamFragmentationException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLogSectorSizeMismatchDatabasesConsistentException : public IsamStateException
    {
    public:
        IsamLogSectorSizeMismatchDatabasesConsistentException() : IsamStateException( "databases have been recovered, but the log file sector size (used during recovery) does not match the current volume's sector size", JET_errLogSectorSizeMismatchDatabasesConsistent)
        {
        }

        IsamLogSectorSizeMismatchDatabasesConsistentException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamLogSectorSizeMismatchDatabasesConsistentException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLogSequenceEndDatabasesConsistentException : public IsamFragmentationException
    {
    public:
        IsamLogSequenceEndDatabasesConsistentException() : IsamFragmentationException( "databases have been recovered, but all possible log generations in the current sequence are used; delete all log files and the checkpoint file and backup the databases before continuing", JET_errLogSequenceEndDatabasesConsistent)
        {
        }

        IsamLogSequenceEndDatabasesConsistentException( String ^ description, Exception^ innerException ) :
            IsamFragmentationException( description, innerException )
        {
        }

        IsamLogSequenceEndDatabasesConsistentException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamFragmentationException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseDirtyShutdownException : public IsamInconsistentException
    {
    public:
        IsamDatabaseDirtyShutdownException() : IsamInconsistentException( "Database was not shutdown cleanly. Recovery must first be run to properly complete database operations for the previous shutdown.", JET_errDatabaseDirtyShutdown)
        {
        }

        IsamDatabaseDirtyShutdownException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamDatabaseDirtyShutdownException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamConsistentTimeMismatchException : public IsamInconsistentException
    {
    public:
        IsamConsistentTimeMismatchException() : IsamInconsistentException( "Database last consistent time unmatched", JET_errConsistentTimeMismatch)
        {
        }

        IsamConsistentTimeMismatchException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamConsistentTimeMismatchException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamEndingRestoreLogTooLowException : public IsamInconsistentException
    {
    public:
        IsamEndingRestoreLogTooLowException() : IsamInconsistentException( "The starting log number too low for the restore", JET_errEndingRestoreLogTooLow)
        {
        }

        IsamEndingRestoreLogTooLowException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamEndingRestoreLogTooLowException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamStartingRestoreLogTooHighException : public IsamInconsistentException
    {
    public:
        IsamStartingRestoreLogTooHighException() : IsamInconsistentException( "The starting log number too high for the restore", JET_errStartingRestoreLogTooHigh)
        {
        }

        IsamStartingRestoreLogTooHighException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamStartingRestoreLogTooHighException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamGivenLogFileHasBadSignatureException : public IsamInconsistentException
    {
    public:
        IsamGivenLogFileHasBadSignatureException() : IsamInconsistentException( "Restore log file has bad signature", JET_errGivenLogFileHasBadSignature)
        {
        }

        IsamGivenLogFileHasBadSignatureException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamGivenLogFileHasBadSignatureException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamGivenLogFileIsNotContiguousException : public IsamInconsistentException
    {
    public:
        IsamGivenLogFileIsNotContiguousException() : IsamInconsistentException( "Restore log file is not contiguous", JET_errGivenLogFileIsNotContiguous)
        {
        }

        IsamGivenLogFileIsNotContiguousException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamGivenLogFileIsNotContiguousException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamMissingRestoreLogFilesException : public IsamInconsistentException
    {
    public:
        IsamMissingRestoreLogFilesException() : IsamInconsistentException( "Some restore log files are missing", JET_errMissingRestoreLogFiles)
        {
        }

        IsamMissingRestoreLogFilesException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamMissingRestoreLogFilesException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamMissingFullBackupException : public IsamStateException
    {
    public:
        IsamMissingFullBackupException() : IsamStateException( "The database missed a previous full backup before incremental backup", JET_errMissingFullBackup)
        {
        }

        IsamMissingFullBackupException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamMissingFullBackupException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseAlreadyUpgradedException : public IsamStateException
    {
    public:
        IsamDatabaseAlreadyUpgradedException() : IsamStateException( "Attempted to upgrade a database that is already current", JET_errDatabaseAlreadyUpgraded)
        {
        }

        IsamDatabaseAlreadyUpgradedException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamDatabaseAlreadyUpgradedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseIncompleteUpgradeException : public IsamStateException
    {
    public:
        IsamDatabaseIncompleteUpgradeException() : IsamStateException( "Attempted to use a database which was only partially converted to the current format -- must restore from backup", JET_errDatabaseIncompleteUpgrade)
        {
        }

        IsamDatabaseIncompleteUpgradeException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamDatabaseIncompleteUpgradeException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamMissingCurrentLogFilesException : public IsamInconsistentException
    {
    public:
        IsamMissingCurrentLogFilesException() : IsamInconsistentException( "Some current log files are missing for continuous restore", JET_errMissingCurrentLogFiles)
        {
        }

        IsamMissingCurrentLogFilesException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamMissingCurrentLogFilesException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDbTimeTooOldException : public IsamCorruptionException
    {
    public:
        IsamDbTimeTooOldException() : IsamCorruptionException( "dbtime on page smaller than dbtimeBefore in record", JET_errDbTimeTooOld)
        {
        }

        IsamDbTimeTooOldException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamDbTimeTooOldException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDbTimeTooNewException : public IsamCorruptionException
    {
    public:
        IsamDbTimeTooNewException() : IsamCorruptionException( "dbtime on page in advance of the dbtimeBefore in record", JET_errDbTimeTooNew)
        {
        }

        IsamDbTimeTooNewException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamDbTimeTooNewException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamMissingFileToBackupException : public IsamInconsistentException
    {
    public:
        IsamMissingFileToBackupException() : IsamInconsistentException( "Some log or patch files are missing during backup", JET_errMissingFileToBackup)
        {
        }

        IsamMissingFileToBackupException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamMissingFileToBackupException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLogTornWriteDuringHardRestoreException : public IsamCorruptionException
    {
    public:
        IsamLogTornWriteDuringHardRestoreException() : IsamCorruptionException( "torn-write was detected in a backup set during hard restore", JET_errLogTornWriteDuringHardRestore)
        {
        }

        IsamLogTornWriteDuringHardRestoreException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamLogTornWriteDuringHardRestoreException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLogTornWriteDuringHardRecoveryException : public IsamCorruptionException
    {
    public:
        IsamLogTornWriteDuringHardRecoveryException() : IsamCorruptionException( "torn-write was detected during hard recovery (log was not part of a backup set)", JET_errLogTornWriteDuringHardRecovery)
        {
        }

        IsamLogTornWriteDuringHardRecoveryException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamLogTornWriteDuringHardRecoveryException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLogCorruptDuringHardRestoreException : public IsamCorruptionException
    {
    public:
        IsamLogCorruptDuringHardRestoreException() : IsamCorruptionException( "corruption was detected in a backup set during hard restore", JET_errLogCorruptDuringHardRestore)
        {
        }

        IsamLogCorruptDuringHardRestoreException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamLogCorruptDuringHardRestoreException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLogCorruptDuringHardRecoveryException : public IsamCorruptionException
    {
    public:
        IsamLogCorruptDuringHardRecoveryException() : IsamCorruptionException( "corruption was detected during hard recovery (log was not part of a backup set)", JET_errLogCorruptDuringHardRecovery)
        {
        }

        IsamLogCorruptDuringHardRecoveryException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamLogCorruptDuringHardRecoveryException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamBadRestoreTargetInstanceException : public IsamUsageException
    {
    public:
        IsamBadRestoreTargetInstanceException() : IsamUsageException( "TargetInstance specified for restore is not found or log files don't match", JET_errBadRestoreTargetInstance)
        {
        }

        IsamBadRestoreTargetInstanceException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamBadRestoreTargetInstanceException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRecoveredWithoutUndoException : public IsamStateException
    {
    public:
        IsamRecoveredWithoutUndoException() : IsamStateException( "Soft recovery successfully replayed all operations, but the Undo phase of recovery was skipped", JET_errRecoveredWithoutUndo)
        {
        }

        IsamRecoveredWithoutUndoException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamRecoveredWithoutUndoException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamCommittedLogFilesMissingException : public IsamCorruptionException
    {
    public:
        IsamCommittedLogFilesMissingException() : IsamCorruptionException( "One or more logs that were committed to this database, are missing.  These log files are required to maintain durable ACID semantics, but not required to maintain consistency if the JET_bitReplayIgnoreLostLogs bit is specified during recovery.", JET_errCommittedLogFilesMissing)
        {
        }

        IsamCommittedLogFilesMissingException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamCommittedLogFilesMissingException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamSectorSizeNotSupportedException : public IsamFatalException
    {
    public:
        IsamSectorSizeNotSupportedException() : IsamFatalException( "The physical sector size reported by the disk subsystem, is unsupported by ESE for a specific file type.", JET_errSectorSizeNotSupported)
        {
        }

        IsamSectorSizeNotSupportedException( String ^ description, Exception^ innerException ) :
            IsamFatalException( description, innerException )
        {
        }

        IsamSectorSizeNotSupportedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamFatalException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRecoveredWithoutUndoDatabasesConsistentException : public IsamStateException
    {
    public:
        IsamRecoveredWithoutUndoDatabasesConsistentException() : IsamStateException( "Soft recovery successfully replayed all operations and intended to skip the Undo phase of recovery, but the Undo phase was not required", JET_errRecoveredWithoutUndoDatabasesConsistent)
        {
        }

        IsamRecoveredWithoutUndoDatabasesConsistentException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamRecoveredWithoutUndoDatabasesConsistentException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamCommittedLogFileCorruptException : public IsamCorruptionException
    {
    public:
        IsamCommittedLogFileCorruptException() : IsamCorruptionException( "One or more logs were found to be corrupt during recovery.  These log files are required to maintain durable ACID semantics, but not required to maintain consistency if the JET_bitIgnoreLostLogs bit and JET_paramDeleteOutOfRangeLogs is specified during recovery.", JET_errCommittedLogFileCorrupt)
        {
        }

        IsamCommittedLogFileCorruptException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamCommittedLogFileCorruptException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLogSequenceChecksumMismatchException : public IsamCorruptionException
    {
    public:
        IsamLogSequenceChecksumMismatchException() : IsamCorruptionException( "The previous log's accumulated segment checksum doesn't match the next log", JET_errLogSequenceChecksumMismatch)
        {
        }

        IsamLogSequenceChecksumMismatchException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamLogSequenceChecksumMismatchException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamPageInitializedMismatchException : public IsamCorruptionException
    {
    public:
        IsamPageInitializedMismatchException() : IsamCorruptionException( "Database divergence mismatch. Page was uninitialized on remote node, but initialized on local node.", JET_errPageInitializedMismatch)
        {
        }

        IsamPageInitializedMismatchException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamPageInitializedMismatchException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamUnicodeTranslationFailException : public IsamOperationException
    {
    public:
        IsamUnicodeTranslationFailException() : IsamOperationException( "Unicode normalization failed", JET_errUnicodeTranslationFail)
        {
        }

        IsamUnicodeTranslationFailException( String ^ description, Exception^ innerException ) :
            IsamOperationException( description, innerException )
        {
        }

        IsamUnicodeTranslationFailException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamOperationException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamUnicodeNormalizationNotSupportedException : public IsamUsageException
    {
    public:
        IsamUnicodeNormalizationNotSupportedException() : IsamUsageException( "OS does not provide support for Unicode normalisation (and no normalisation callback was specified)", JET_errUnicodeNormalizationNotSupported)
        {
        }

        IsamUnicodeNormalizationNotSupportedException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamUnicodeNormalizationNotSupportedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamUnicodeLanguageValidationFailureException : public IsamOperationException
    {
    public:
        IsamUnicodeLanguageValidationFailureException() : IsamOperationException( "Can not validate the language", JET_errUnicodeLanguageValidationFailure)
        {
        }

        IsamUnicodeLanguageValidationFailureException( String ^ description, Exception^ innerException ) :
            IsamOperationException( description, innerException )
        {
        }

        IsamUnicodeLanguageValidationFailureException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamOperationException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamExistingLogFileHasBadSignatureException : public IsamInconsistentException
    {
    public:
        IsamExistingLogFileHasBadSignatureException() : IsamInconsistentException( "Existing log file has bad signature", JET_errExistingLogFileHasBadSignature)
        {
        }

        IsamExistingLogFileHasBadSignatureException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamExistingLogFileHasBadSignatureException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamExistingLogFileIsNotContiguousException : public IsamInconsistentException
    {
    public:
        IsamExistingLogFileIsNotContiguousException() : IsamInconsistentException( "Existing log file is not contiguous", JET_errExistingLogFileIsNotContiguous)
        {
        }

        IsamExistingLogFileIsNotContiguousException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamExistingLogFileIsNotContiguousException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLogReadVerifyFailureException : public IsamCorruptionException
    {
    public:
        IsamLogReadVerifyFailureException() : IsamCorruptionException( "Checksum error in log file during backup", JET_errLogReadVerifyFailure)
        {
        }

        IsamLogReadVerifyFailureException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamLogReadVerifyFailureException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamCheckpointDepthTooDeepException : public IsamQuotaException
    {
    public:
        IsamCheckpointDepthTooDeepException() : IsamQuotaException( "too many outstanding generations between checkpoint and current generation", JET_errCheckpointDepthTooDeep)
        {
        }

        IsamCheckpointDepthTooDeepException( String ^ description, Exception^ innerException ) :
            IsamQuotaException( description, innerException )
        {
        }

        IsamCheckpointDepthTooDeepException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamQuotaException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRestoreOfNonBackupDatabaseException : public IsamUsageException
    {
    public:
        IsamRestoreOfNonBackupDatabaseException() : IsamUsageException( "hard recovery attempted on a database that wasn't a backup database", JET_errRestoreOfNonBackupDatabase)
        {
        }

        IsamRestoreOfNonBackupDatabaseException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamRestoreOfNonBackupDatabaseException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLogFileNotCopiedException : public IsamUsageException
    {
    public:
        IsamLogFileNotCopiedException() : IsamUsageException( "log truncation attempted but not all required logs were copied", JET_errLogFileNotCopied)
        {
        }

        IsamLogFileNotCopiedException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamLogFileNotCopiedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamSurrogateBackupInProgressException : public IsamStateException
    {
    public:
        IsamSurrogateBackupInProgressException() : IsamStateException( "A surrogate backup is in progress.", JET_errSurrogateBackupInProgress)
        {
        }

        IsamSurrogateBackupInProgressException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamSurrogateBackupInProgressException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTransactionTooLongException : public IsamQuotaException
    {
    public:
        IsamTransactionTooLongException() : IsamQuotaException( "Too many outstanding generations between JetBeginTransaction and current generation.", JET_errTransactionTooLong)
        {
        }

        IsamTransactionTooLongException( String ^ description, Exception^ innerException ) :
            IsamQuotaException( description, innerException )
        {
        }

        IsamTransactionTooLongException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamQuotaException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamEngineFormatVersionNoLongerSupportedTooLowException : public IsamUsageException
    {
    public:
        IsamEngineFormatVersionNoLongerSupportedTooLowException() : IsamUsageException( "The specified JET_ENGINEFORMATVERSION value is too low to be supported by this version of ESE.", JET_errEngineFormatVersionNoLongerSupportedTooLow)
        {
        }

        IsamEngineFormatVersionNoLongerSupportedTooLowException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamEngineFormatVersionNoLongerSupportedTooLowException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamEngineFormatVersionNotYetImplementedTooHighException : public IsamUsageException
    {
    public:
        IsamEngineFormatVersionNotYetImplementedTooHighException() : IsamUsageException( "The specified JET_ENGINEFORMATVERSION value is too high, higher than this version of ESE knows about.", JET_errEngineFormatVersionNotYetImplementedTooHigh)
        {
        }

        IsamEngineFormatVersionNotYetImplementedTooHighException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamEngineFormatVersionNotYetImplementedTooHighException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamEngineFormatVersionParamTooLowForRequestedFeatureException : public IsamUsageException
    {
    public:
        IsamEngineFormatVersionParamTooLowForRequestedFeatureException() : IsamUsageException( "Thrown by a format feature (not at JetSetSystemParameter) if the client requests a feature that requires a version higher than that set for the JET_paramEngineFormatVersion.", JET_errEngineFormatVersionParamTooLowForRequestedFeature)
        {
        }

        IsamEngineFormatVersionParamTooLowForRequestedFeatureException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamEngineFormatVersionParamTooLowForRequestedFeatureException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamEngineFormatVersionSpecifiedTooLowForLogVersionException : public IsamStateException
    {
    public:
        IsamEngineFormatVersionSpecifiedTooLowForLogVersionException() : IsamStateException( "The specified JET_ENGINEFORMATVERSION is set too low for this log stream, the log files have already been upgraded to a higher version.  A higher JET_ENGINEFORMATVERSION value must be set in the param.", JET_errEngineFormatVersionSpecifiedTooLowForLogVersion)
        {
        }

        IsamEngineFormatVersionSpecifiedTooLowForLogVersionException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamEngineFormatVersionSpecifiedTooLowForLogVersionException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamEngineFormatVersionSpecifiedTooLowForDatabaseVersionException : public IsamStateException
    {
    public:
        IsamEngineFormatVersionSpecifiedTooLowForDatabaseVersionException() : IsamStateException( "The specified JET_ENGINEFORMATVERSION is set too low for this database file, the database file has already been upgraded to a higher version.  A higher JET_ENGINEFORMATVERSION value must be set in the param.", JET_errEngineFormatVersionSpecifiedTooLowForDatabaseVersion)
        {
        }

        IsamEngineFormatVersionSpecifiedTooLowForDatabaseVersionException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamEngineFormatVersionSpecifiedTooLowForDatabaseVersionException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamBackupAbortByServerException : public IsamOperationException
    {
    public:
        IsamBackupAbortByServerException() : IsamOperationException( "Backup was aborted by server by calling JetTerm with JET_bitTermStopBackup or by calling JetStopBackup", JET_errBackupAbortByServer)
        {
        }

        IsamBackupAbortByServerException( String ^ description, Exception^ innerException ) :
            IsamOperationException( description, innerException )
        {
        }

        IsamBackupAbortByServerException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamOperationException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidGrbitException : public IsamUsageException
    {
    public:
        IsamInvalidGrbitException() : IsamUsageException( "Invalid flags parameter", JET_errInvalidGrbit)
        {
        }

        IsamInvalidGrbitException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidGrbitException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTermInProgressException : public IsamOperationException
    {
    public:
        IsamTermInProgressException() : IsamOperationException( "Termination in progress", JET_errTermInProgress)
        {
        }

        IsamTermInProgressException( String ^ description, Exception^ innerException ) :
            IsamOperationException( description, innerException )
        {
        }

        IsamTermInProgressException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamOperationException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamFeatureNotAvailableException : public IsamUsageException
    {
    public:
        IsamFeatureNotAvailableException() : IsamUsageException( "API not supported", JET_errFeatureNotAvailable)
        {
        }

        IsamFeatureNotAvailableException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamFeatureNotAvailableException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidNameException : public IsamUsageException
    {
    public:
        IsamInvalidNameException() : IsamUsageException( "Invalid name", JET_errInvalidName)
        {
        }

        IsamInvalidNameException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidNameException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidParameterException : public IsamUsageException
    {
    public:
        IsamInvalidParameterException() : IsamUsageException( "Invalid API parameter", JET_errInvalidParameter)
        {
        }

        IsamInvalidParameterException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidParameterException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseFileReadOnlyException : public IsamUsageException
    {
    public:
        IsamDatabaseFileReadOnlyException() : IsamUsageException( "Tried to attach a read-only database file for read/write operations", JET_errDatabaseFileReadOnly)
        {
        }

        IsamDatabaseFileReadOnlyException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamDatabaseFileReadOnlyException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidDatabaseIdException : public IsamUsageException
    {
    public:
        IsamInvalidDatabaseIdException() : IsamUsageException( "Invalid database id", JET_errInvalidDatabaseId)
        {
        }

        IsamInvalidDatabaseIdException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidDatabaseIdException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamOutOfMemoryException : public IsamMemoryException
    {
    public:
        IsamOutOfMemoryException() : IsamMemoryException( "Out of Memory", JET_errOutOfMemory)
        {
        }

        IsamOutOfMemoryException( String ^ description, Exception^ innerException ) :
            IsamMemoryException( description, innerException )
        {
        }

        IsamOutOfMemoryException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamMemoryException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamOutOfDatabaseSpaceException : public IsamQuotaException
    {
    public:
        IsamOutOfDatabaseSpaceException() : IsamQuotaException( "Maximum database size reached", JET_errOutOfDatabaseSpace)
        {
        }

        IsamOutOfDatabaseSpaceException( String ^ description, Exception^ innerException ) :
            IsamQuotaException( description, innerException )
        {
        }

        IsamOutOfDatabaseSpaceException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamQuotaException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamOutOfCursorsException : public IsamMemoryException
    {
    public:
        IsamOutOfCursorsException() : IsamMemoryException( "Out of table cursors", JET_errOutOfCursors)
        {
        }

        IsamOutOfCursorsException( String ^ description, Exception^ innerException ) :
            IsamMemoryException( description, innerException )
        {
        }

        IsamOutOfCursorsException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamMemoryException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamOutOfBuffersException : public IsamMemoryException
    {
    public:
        IsamOutOfBuffersException() : IsamMemoryException( "Out of database page buffers", JET_errOutOfBuffers)
        {
        }

        IsamOutOfBuffersException( String ^ description, Exception^ innerException ) :
            IsamMemoryException( description, innerException )
        {
        }

        IsamOutOfBuffersException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamMemoryException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTooManyIndexesException : public IsamUsageException
    {
    public:
        IsamTooManyIndexesException() : IsamUsageException( "Too many indexes", JET_errTooManyIndexes)
        {
        }

        IsamTooManyIndexesException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamTooManyIndexesException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTooManyKeysException : public IsamUsageException
    {
    public:
        IsamTooManyKeysException() : IsamUsageException( "Too many columns in an index", JET_errTooManyKeys)
        {
        }

        IsamTooManyKeysException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamTooManyKeysException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRecordDeletedException : public IsamStateException
    {
    public:
        IsamRecordDeletedException() : IsamStateException( "Record has been deleted", JET_errRecordDeleted)
        {
        }

        IsamRecordDeletedException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamRecordDeletedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamReadVerifyFailureException : public IsamCorruptionException
    {
    public:
        IsamReadVerifyFailureException() : IsamCorruptionException( "Checksum error on a database page", JET_errReadVerifyFailure)
        {
        }

        IsamReadVerifyFailureException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamReadVerifyFailureException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamPageNotInitializedException : public IsamCorruptionException
    {
    public:
        IsamPageNotInitializedException() : IsamCorruptionException( "Blank database page", JET_errPageNotInitialized)
        {
        }

        IsamPageNotInitializedException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamPageNotInitializedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamOutOfFileHandlesException : public IsamMemoryException
    {
    public:
        IsamOutOfFileHandlesException() : IsamMemoryException( "Out of file handles", JET_errOutOfFileHandles)
        {
        }

        IsamOutOfFileHandlesException( String ^ description, Exception^ innerException ) :
            IsamMemoryException( description, innerException )
        {
        }

        IsamOutOfFileHandlesException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamMemoryException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDiskReadVerificationFailureException : public IsamCorruptionException
    {
    public:
        IsamDiskReadVerificationFailureException() : IsamCorruptionException( "The OS returned ERROR_CRC from file IO", JET_errDiskReadVerificationFailure)
        {
        }

        IsamDiskReadVerificationFailureException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamDiskReadVerificationFailureException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDiskIOException : public IsamIOException
    {
    public:
        IsamDiskIOException() : IsamIOException( "Disk IO error", JET_errDiskIO)
        {
        }

        IsamDiskIOException( String ^ description, Exception^ innerException ) :
            IsamIOException( description, innerException )
        {
        }

        IsamDiskIOException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamIOException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidPathException : public IsamUsageException
    {
    public:
        IsamInvalidPathException() : IsamUsageException( "Invalid file path", JET_errInvalidPath)
        {
        }

        IsamInvalidPathException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidPathException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRecordTooBigException : public IsamStateException
    {
    public:
        IsamRecordTooBigException() : IsamStateException( "Record larger than maximum size", JET_errRecordTooBig)
        {
        }

        IsamRecordTooBigException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamRecordTooBigException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidDatabaseException : public IsamUsageException
    {
    public:
        IsamInvalidDatabaseException() : IsamUsageException( "Not a database file", JET_errInvalidDatabase)
        {
        }

        IsamInvalidDatabaseException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidDatabaseException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamNotInitializedException : public IsamUsageException
    {
    public:
        IsamNotInitializedException() : IsamUsageException( "Database engine not initialized", JET_errNotInitialized)
        {
        }

        IsamNotInitializedException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamNotInitializedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamAlreadyInitializedException : public IsamUsageException
    {
    public:
        IsamAlreadyInitializedException() : IsamUsageException( "Database engine already initialized", JET_errAlreadyInitialized)
        {
        }

        IsamAlreadyInitializedException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamAlreadyInitializedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInitInProgressException : public IsamOperationException
    {
    public:
        IsamInitInProgressException() : IsamOperationException( "Database engine is being initialized", JET_errInitInProgress)
        {
        }

        IsamInitInProgressException( String ^ description, Exception^ innerException ) :
            IsamOperationException( description, innerException )
        {
        }

        IsamInitInProgressException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamOperationException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamFileAccessDeniedException : public IsamIOException
    {
    public:
        IsamFileAccessDeniedException() : IsamIOException( "Cannot access file, the file is locked or in use", JET_errFileAccessDenied)
        {
        }

        IsamFileAccessDeniedException( String ^ description, Exception^ innerException ) :
            IsamIOException( description, innerException )
        {
        }

        IsamFileAccessDeniedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamIOException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamBufferTooSmallException : public IsamStateException
    {
    public:
        IsamBufferTooSmallException() : IsamStateException( "Buffer is too small", JET_errBufferTooSmall)
        {
        }

        IsamBufferTooSmallException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamBufferTooSmallException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTooManyColumnsException : public IsamUsageException
    {
    public:
        IsamTooManyColumnsException() : IsamUsageException( "Too many columns defined", JET_errTooManyColumns)
        {
        }

        IsamTooManyColumnsException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamTooManyColumnsException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidBookmarkException : public IsamUsageException
    {
    public:
        IsamInvalidBookmarkException() : IsamUsageException( "Invalid bookmark", JET_errInvalidBookmark)
        {
        }

        IsamInvalidBookmarkException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidBookmarkException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamColumnInUseException : public IsamUsageException
    {
    public:
        IsamColumnInUseException() : IsamUsageException( "Column used in an index", JET_errColumnInUse)
        {
        }

        IsamColumnInUseException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamColumnInUseException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidBufferSizeException : public IsamStateException
    {
    public:
        IsamInvalidBufferSizeException() : IsamStateException( "Data buffer doesn't match column size", JET_errInvalidBufferSize)
        {
        }

        IsamInvalidBufferSizeException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamInvalidBufferSizeException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamColumnNotUpdatableException : public IsamUsageException
    {
    public:
        IsamColumnNotUpdatableException() : IsamUsageException( "Cannot set column value", JET_errColumnNotUpdatable)
        {
        }

        IsamColumnNotUpdatableException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamColumnNotUpdatableException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamIndexInUseException : public IsamStateException
    {
    public:
        IsamIndexInUseException() : IsamStateException( "Index is in use", JET_errIndexInUse)
        {
        }

        IsamIndexInUseException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamIndexInUseException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamNullKeyDisallowedException : public IsamUsageException
    {
    public:
        IsamNullKeyDisallowedException() : IsamUsageException( "Null keys are disallowed on index", JET_errNullKeyDisallowed)
        {
        }

        IsamNullKeyDisallowedException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamNullKeyDisallowedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamNotInTransactionException : public IsamUsageException
    {
    public:
        IsamNotInTransactionException() : IsamUsageException( "Operation must be within a transaction", JET_errNotInTransaction)
        {
        }

        IsamNotInTransactionException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamNotInTransactionException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamMustRollbackException : public IsamUsageException
    {
    public:
        IsamMustRollbackException() : IsamUsageException( "Transaction must rollback because failure of unversioned update", JET_errMustRollback)
        {
        }

        IsamMustRollbackException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamMustRollbackException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTooManyActiveUsersException : public IsamUsageException
    {
    public:
        IsamTooManyActiveUsersException() : IsamUsageException( "Too many active database users", JET_errTooManyActiveUsers)
        {
        }

        IsamTooManyActiveUsersException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamTooManyActiveUsersException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidLanguageIdException : public IsamUsageException
    {
    public:
        IsamInvalidLanguageIdException() : IsamUsageException( "Invalid or unknown language id", JET_errInvalidLanguageId)
        {
        }

        IsamInvalidLanguageIdException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidLanguageIdException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidCodePageException : public IsamUsageException
    {
    public:
        IsamInvalidCodePageException() : IsamUsageException( "Invalid or unknown code page", JET_errInvalidCodePage)
        {
        }

        IsamInvalidCodePageException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidCodePageException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidLCMapStringFlagsException : public IsamUsageException
    {
    public:
        IsamInvalidLCMapStringFlagsException() : IsamUsageException( "Invalid flags for LCMapString()", JET_errInvalidLCMapStringFlags)
        {
        }

        IsamInvalidLCMapStringFlagsException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidLCMapStringFlagsException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamVersionStoreOutOfMemoryAndCleanupTimedOutException : public IsamUsageException
    {
    public:
        IsamVersionStoreOutOfMemoryAndCleanupTimedOutException() : IsamUsageException( "Version store out of memory (and cleanup attempt failed to complete)", JET_errVersionStoreOutOfMemoryAndCleanupTimedOut)
        {
        }

        IsamVersionStoreOutOfMemoryAndCleanupTimedOutException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamVersionStoreOutOfMemoryAndCleanupTimedOutException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamVersionStoreOutOfMemoryException : public IsamQuotaException
    {
    public:
        IsamVersionStoreOutOfMemoryException() : IsamQuotaException( "Version store out of memory (cleanup already attempted)", JET_errVersionStoreOutOfMemory)
        {
        }

        IsamVersionStoreOutOfMemoryException( String ^ description, Exception^ innerException ) :
            IsamQuotaException( description, innerException )
        {
        }

        IsamVersionStoreOutOfMemoryException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamQuotaException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamCannotIndexException : public IsamUsageException
    {
    public:
        IsamCannotIndexException() : IsamUsageException( "Cannot index escrow column", JET_errCannotIndex)
        {
        }

        IsamCannotIndexException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamCannotIndexException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRecordNotDeletedException : public IsamOperationException
    {
    public:
        IsamRecordNotDeletedException() : IsamOperationException( "Record has not been deleted", JET_errRecordNotDeleted)
        {
        }

        IsamRecordNotDeletedException( String ^ description, Exception^ innerException ) :
            IsamOperationException( description, innerException )
        {
        }

        IsamRecordNotDeletedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamOperationException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTooManyMempoolEntriesException : public IsamMemoryException
    {
    public:
        IsamTooManyMempoolEntriesException() : IsamMemoryException( "Too many mempool entries requested", JET_errTooManyMempoolEntries)
        {
        }

        IsamTooManyMempoolEntriesException( String ^ description, Exception^ innerException ) :
            IsamMemoryException( description, innerException )
        {
        }

        IsamTooManyMempoolEntriesException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamMemoryException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamOutOfObjectIDsException : public IsamFragmentationException
    {
    public:
        IsamOutOfObjectIDsException() : IsamFragmentationException( "Out of btree ObjectIDs (perform offline defrag to reclaim freed/unused ObjectIds)", JET_errOutOfObjectIDs)
        {
        }

        IsamOutOfObjectIDsException( String ^ description, Exception^ innerException ) :
            IsamFragmentationException( description, innerException )
        {
        }

        IsamOutOfObjectIDsException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamFragmentationException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamOutOfLongValueIDsException : public IsamFragmentationException
    {
    public:
        IsamOutOfLongValueIDsException() : IsamFragmentationException( "Long-value ID counter has reached maximum value. (perform offline defrag to reclaim free/unused LongValueIDs)", JET_errOutOfLongValueIDs)
        {
        }

        IsamOutOfLongValueIDsException( String ^ description, Exception^ innerException ) :
            IsamFragmentationException( description, innerException )
        {
        }

        IsamOutOfLongValueIDsException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamFragmentationException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamOutOfAutoincrementValuesException : public IsamFragmentationException
    {
    public:
        IsamOutOfAutoincrementValuesException() : IsamFragmentationException( "Auto-increment counter has reached maximum value (offline defrag WILL NOT be able to reclaim free/unused Auto-increment values).", JET_errOutOfAutoincrementValues)
        {
        }

        IsamOutOfAutoincrementValuesException( String ^ description, Exception^ innerException ) :
            IsamFragmentationException( description, innerException )
        {
        }

        IsamOutOfAutoincrementValuesException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamFragmentationException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamOutOfDbtimeValuesException : public IsamFragmentationException
    {
    public:
        IsamOutOfDbtimeValuesException() : IsamFragmentationException( "Dbtime counter has reached maximum value (perform offline defrag to reclaim free/unused Dbtime values)", JET_errOutOfDbtimeValues)
        {
        }

        IsamOutOfDbtimeValuesException( String ^ description, Exception^ innerException ) :
            IsamFragmentationException( description, innerException )
        {
        }

        IsamOutOfDbtimeValuesException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamFragmentationException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamOutOfSequentialIndexValuesException : public IsamFragmentationException
    {
    public:
        IsamOutOfSequentialIndexValuesException() : IsamFragmentationException( "Sequential index counter has reached maximum value (perform offline defrag to reclaim free/unused SequentialIndex values)", JET_errOutOfSequentialIndexValues)
        {
        }

        IsamOutOfSequentialIndexValuesException( String ^ description, Exception^ innerException ) :
            IsamFragmentationException( description, innerException )
        {
        }

        IsamOutOfSequentialIndexValuesException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamFragmentationException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRunningInOneInstanceModeException : public IsamUsageException
    {
    public:
        IsamRunningInOneInstanceModeException() : IsamUsageException( "Multi-instance call with single-instance mode enabled", JET_errRunningInOneInstanceMode)
        {
        }

        IsamRunningInOneInstanceModeException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamRunningInOneInstanceModeException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRunningInMultiInstanceModeException : public IsamUsageException
    {
    public:
        IsamRunningInMultiInstanceModeException() : IsamUsageException( "Single-instance call with multi-instance mode enabled", JET_errRunningInMultiInstanceMode)
        {
        }

        IsamRunningInMultiInstanceModeException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamRunningInMultiInstanceModeException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamSystemParamsAlreadySetException : public IsamStateException
    {
    public:
        IsamSystemParamsAlreadySetException() : IsamStateException( "Global system parameters have already been set", JET_errSystemParamsAlreadySet)
        {
        }

        IsamSystemParamsAlreadySetException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamSystemParamsAlreadySetException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamSystemPathInUseException : public IsamUsageException
    {
    public:
        IsamSystemPathInUseException() : IsamUsageException( "System path already used by another database instance", JET_errSystemPathInUse)
        {
        }

        IsamSystemPathInUseException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamSystemPathInUseException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLogFilePathInUseException : public IsamUsageException
    {
    public:
        IsamLogFilePathInUseException() : IsamUsageException( "Logfile path already used by another database instance", JET_errLogFilePathInUse)
        {
        }

        IsamLogFilePathInUseException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamLogFilePathInUseException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTempPathInUseException : public IsamUsageException
    {
    public:
        IsamTempPathInUseException() : IsamUsageException( "Temp path already used by another database instance", JET_errTempPathInUse)
        {
        }

        IsamTempPathInUseException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamTempPathInUseException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInstanceNameInUseException : public IsamUsageException
    {
    public:
        IsamInstanceNameInUseException() : IsamUsageException( "Instance Name already in use", JET_errInstanceNameInUse)
        {
        }

        IsamInstanceNameInUseException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInstanceNameInUseException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamSystemParameterConflictException : public IsamUsageException
    {
    public:
        IsamSystemParameterConflictException() : IsamUsageException( "Global system parameters have already been set, but to a conflicting or disagreeable state to the specified values.", JET_errSystemParameterConflict)
        {
        }

        IsamSystemParameterConflictException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamSystemParameterConflictException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInstanceUnavailableException : public IsamFatalException
    {
    public:
        IsamInstanceUnavailableException() : IsamFatalException( "This instance cannot be used because it encountered a fatal error", JET_errInstanceUnavailable)
        {
        }

        IsamInstanceUnavailableException( String ^ description, Exception^ innerException ) :
            IsamFatalException( description, innerException )
        {
        }

        IsamInstanceUnavailableException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamFatalException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInstanceUnavailableDueToFatalLogDiskFullException : public IsamFatalException
    {
    public:
        IsamInstanceUnavailableDueToFatalLogDiskFullException() : IsamFatalException( "This instance cannot be used because it encountered a log-disk-full error performing an operation (likely transaction rollback) that could not tolerate failure", JET_errInstanceUnavailableDueToFatalLogDiskFull)
        {
        }

        IsamInstanceUnavailableDueToFatalLogDiskFullException( String ^ description, Exception^ innerException ) :
            IsamFatalException( description, innerException )
        {
        }

        IsamInstanceUnavailableDueToFatalLogDiskFullException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamFatalException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidSesparamIdException : public IsamUsageException
    {
    public:
        IsamInvalidSesparamIdException() : IsamUsageException( "This JET_sesparam* identifier is not known to the ESE engine.", JET_errInvalidSesparamId)
        {
        }

        IsamInvalidSesparamIdException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidSesparamIdException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTooManyRecordsException : public IsamStateException
    {
    public:
        IsamTooManyRecordsException() : IsamStateException( "There are too many records to enumerate, switch to an API that handles 64-bit numbers", JET_errTooManyRecords)
        {
        }

        IsamTooManyRecordsException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamTooManyRecordsException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidDbparamIdException : public IsamUsageException
    {
    public:
        IsamInvalidDbparamIdException() : IsamUsageException( "This JET_dbparam* identifier is not known to the ESE engine.", JET_errInvalidDbparamId)
        {
        }

        IsamInvalidDbparamIdException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidDbparamIdException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamOutOfSessionsException : public IsamMemoryException
    {
    public:
        IsamOutOfSessionsException() : IsamMemoryException( "Out of sessions", JET_errOutOfSessions)
        {
        }

        IsamOutOfSessionsException( String ^ description, Exception^ innerException ) :
            IsamMemoryException( description, innerException )
        {
        }

        IsamOutOfSessionsException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamMemoryException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamWriteConflictException : public IsamStateException
    {
    public:
        IsamWriteConflictException() : IsamStateException( "Write lock failed due to outstanding write lock", JET_errWriteConflict)
        {
        }

        IsamWriteConflictException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamWriteConflictException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTransTooDeepException : public IsamUsageException
    {
    public:
        IsamTransTooDeepException() : IsamUsageException( "Transactions nested too deeply", JET_errTransTooDeep)
        {
        }

        IsamTransTooDeepException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamTransTooDeepException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidSesidException : public IsamUsageException
    {
    public:
        IsamInvalidSesidException() : IsamUsageException( "Invalid session handle", JET_errInvalidSesid)
        {
        }

        IsamInvalidSesidException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidSesidException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamWriteConflictPrimaryIndexException : public IsamStateException
    {
    public:
        IsamWriteConflictPrimaryIndexException() : IsamStateException( "Update attempted on uncommitted primary index", JET_errWriteConflictPrimaryIndex)
        {
        }

        IsamWriteConflictPrimaryIndexException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamWriteConflictPrimaryIndexException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInTransactionException : public IsamUsageException
    {
    public:
        IsamInTransactionException() : IsamUsageException( "Operation not allowed within a transaction", JET_errInTransaction)
        {
        }

        IsamInTransactionException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInTransactionException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTransReadOnlyException : public IsamUsageException
    {
    public:
        IsamTransReadOnlyException() : IsamUsageException( "Read-only transaction tried to modify the database", JET_errTransReadOnly)
        {
        }

        IsamTransReadOnlyException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamTransReadOnlyException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamSessionWriteConflictException : public IsamUsageException
    {
    public:
        IsamSessionWriteConflictException() : IsamUsageException( "Attempt to replace the same record by two different cursors in the same session", JET_errSessionWriteConflict)
        {
        }

        IsamSessionWriteConflictException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamSessionWriteConflictException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRecordTooBigForBackwardCompatibilityException : public IsamStateException
    {
    public:
        IsamRecordTooBigForBackwardCompatibilityException() : IsamStateException( "record would be too big if represented in a database format from a previous version of Jet", JET_errRecordTooBigForBackwardCompatibility)
        {
        }

        IsamRecordTooBigForBackwardCompatibilityException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamRecordTooBigForBackwardCompatibilityException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamCannotMaterializeForwardOnlySortException : public IsamUsageException
    {
    public:
        IsamCannotMaterializeForwardOnlySortException() : IsamUsageException( "The temp table could not be created due to parameters that conflict with JET_bitTTForwardOnly", JET_errCannotMaterializeForwardOnlySort)
        {
        }

        IsamCannotMaterializeForwardOnlySortException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamCannotMaterializeForwardOnlySortException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamSesidTableIdMismatchException : public IsamUsageException
    {
    public:
        IsamSesidTableIdMismatchException() : IsamUsageException( "This session handle can't be used with this table id", JET_errSesidTableIdMismatch)
        {
        }

        IsamSesidTableIdMismatchException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamSesidTableIdMismatchException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidInstanceException : public IsamUsageException
    {
    public:
        IsamInvalidInstanceException() : IsamUsageException( "Invalid instance handle", JET_errInvalidInstance)
        {
        }

        IsamInvalidInstanceException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidInstanceException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDirtyShutdownException : public IsamStateException
    {
    public:
        IsamDirtyShutdownException() : IsamStateException( "The instance was shutdown successfully but all the attached databases were left in a dirty state by request via JET_bitTermDirty", JET_errDirtyShutdown)
        {
        }

        IsamDirtyShutdownException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamDirtyShutdownException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamReadPgnoVerifyFailureException : public IsamCorruptionException
    {
    public:
        IsamReadPgnoVerifyFailureException() : IsamCorruptionException( "The database page read from disk had the wrong page number.", JET_errReadPgnoVerifyFailure)
        {
        }

        IsamReadPgnoVerifyFailureException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamReadPgnoVerifyFailureException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamReadLostFlushVerifyFailureException : public IsamCorruptionException
    {
    public:
        IsamReadLostFlushVerifyFailureException() : IsamCorruptionException( "The database page read from disk had a previous write not represented on the page.", JET_errReadLostFlushVerifyFailure)
        {
        }

        IsamReadLostFlushVerifyFailureException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamReadLostFlushVerifyFailureException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamFileSystemCorruptionException : public IsamCorruptionException
    {
    public:
        IsamFileSystemCorruptionException() : IsamCorruptionException( "File system operation failed with an error indicating the file system is corrupt.", JET_errFileSystemCorruption)
        {
        }

        IsamFileSystemCorruptionException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamFileSystemCorruptionException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRecoveryVerifyFailureException : public IsamCorruptionException
    {
    public:
        IsamRecoveryVerifyFailureException() : IsamCorruptionException( "One or more database pages read from disk during recovery do not match the expected state.", JET_errRecoveryVerifyFailure)
        {
        }

        IsamRecoveryVerifyFailureException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamRecoveryVerifyFailureException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamFilteredMoveNotSupportedException : public IsamUsageException
    {
    public:
        IsamFilteredMoveNotSupportedException() : IsamUsageException( "Attempted to provide a filter to JetSetCursorFilter() in an unsupported scenario.", JET_errFilteredMoveNotSupported)
        {
        }

        IsamFilteredMoveNotSupportedException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamFilteredMoveNotSupportedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseDuplicateException : public IsamUsageException
    {
    public:
        IsamDatabaseDuplicateException() : IsamUsageException( "Database already exists", JET_errDatabaseDuplicate)
        {
        }

        IsamDatabaseDuplicateException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamDatabaseDuplicateException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseInUseException : public IsamUsageException
    {
    public:
        IsamDatabaseInUseException() : IsamUsageException( "Database in use", JET_errDatabaseInUse)
        {
        }

        IsamDatabaseInUseException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamDatabaseInUseException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseNotFoundException : public IsamUsageException
    {
    public:
        IsamDatabaseNotFoundException() : IsamUsageException( "No such database", JET_errDatabaseNotFound)
        {
        }

        IsamDatabaseNotFoundException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamDatabaseNotFoundException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseInvalidNameException : public IsamUsageException
    {
    public:
        IsamDatabaseInvalidNameException() : IsamUsageException( "Invalid database name", JET_errDatabaseInvalidName)
        {
        }

        IsamDatabaseInvalidNameException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamDatabaseInvalidNameException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseInvalidPagesException : public IsamUsageException
    {
    public:
        IsamDatabaseInvalidPagesException() : IsamUsageException( "Invalid number of pages", JET_errDatabaseInvalidPages)
        {
        }

        IsamDatabaseInvalidPagesException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamDatabaseInvalidPagesException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseCorruptedException : public IsamCorruptionException
    {
    public:
        IsamDatabaseCorruptedException() : IsamCorruptionException( "Non database file or corrupted db", JET_errDatabaseCorrupted)
        {
        }

        IsamDatabaseCorruptedException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamDatabaseCorruptedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseLockedException : public IsamUsageException
    {
    public:
        IsamDatabaseLockedException() : IsamUsageException( "Database exclusively locked", JET_errDatabaseLocked)
        {
        }

        IsamDatabaseLockedException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamDatabaseLockedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamCannotDisableVersioningException : public IsamUsageException
    {
    public:
        IsamCannotDisableVersioningException() : IsamUsageException( "Cannot disable versioning for this database", JET_errCannotDisableVersioning)
        {
        }

        IsamCannotDisableVersioningException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamCannotDisableVersioningException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidDatabaseVersionException : public IsamInconsistentException
    {
    public:
        IsamInvalidDatabaseVersionException() : IsamInconsistentException( "Database engine is incompatible with database", JET_errInvalidDatabaseVersion)
        {
        }

        IsamInvalidDatabaseVersionException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamInvalidDatabaseVersionException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamPageSizeMismatchException : public IsamInconsistentException
    {
    public:
        IsamPageSizeMismatchException() : IsamInconsistentException( "The database page size does not match the engine", JET_errPageSizeMismatch)
        {
        }

        IsamPageSizeMismatchException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamPageSizeMismatchException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTooManyInstancesException : public IsamQuotaException
    {
    public:
        IsamTooManyInstancesException() : IsamQuotaException( "Cannot start any more database instances", JET_errTooManyInstances)
        {
        }

        IsamTooManyInstancesException( String ^ description, Exception^ innerException ) :
            IsamQuotaException( description, innerException )
        {
        }

        IsamTooManyInstancesException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamQuotaException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseSharingViolationException : public IsamUsageException
    {
    public:
        IsamDatabaseSharingViolationException() : IsamUsageException( "A different database instance is using this database", JET_errDatabaseSharingViolation)
        {
        }

        IsamDatabaseSharingViolationException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamDatabaseSharingViolationException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamAttachedDatabaseMismatchException : public IsamInconsistentException
    {
    public:
        IsamAttachedDatabaseMismatchException() : IsamInconsistentException( "An outstanding database attachment has been detected at the start or end of recovery, but database is missing or does not match attachment info", JET_errAttachedDatabaseMismatch)
        {
        }

        IsamAttachedDatabaseMismatchException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamAttachedDatabaseMismatchException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseInvalidPathException : public IsamUsageException
    {
    public:
        IsamDatabaseInvalidPathException() : IsamUsageException( "Specified path to database file is illegal", JET_errDatabaseInvalidPath)
        {
        }

        IsamDatabaseInvalidPathException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamDatabaseInvalidPathException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamForceDetachNotAllowedException : public IsamUsageException
    {
    public:
        IsamForceDetachNotAllowedException() : IsamUsageException( "Force Detach allowed only after normal detach errored out", JET_errForceDetachNotAllowed)
        {
        }

        IsamForceDetachNotAllowedException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamForceDetachNotAllowedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamCatalogCorruptedException : public IsamCorruptionException
    {
    public:
        IsamCatalogCorruptedException() : IsamCorruptionException( "Corruption detected in catalog", JET_errCatalogCorrupted)
        {
        }

        IsamCatalogCorruptedException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamCatalogCorruptedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamPartiallyAttachedDBException : public IsamUsageException
    {
    public:
        IsamPartiallyAttachedDBException() : IsamUsageException( "Database is partially attached. Cannot complete attach operation", JET_errPartiallyAttachedDB)
        {
        }

        IsamPartiallyAttachedDBException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamPartiallyAttachedDBException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseSignInUseException : public IsamUsageException
    {
    public:
        IsamDatabaseSignInUseException() : IsamUsageException( "Database with same signature in use", JET_errDatabaseSignInUse)
        {
        }

        IsamDatabaseSignInUseException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamDatabaseSignInUseException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseCorruptedNoRepairException : public IsamUsageException
    {
    public:
        IsamDatabaseCorruptedNoRepairException() : IsamUsageException( "Corrupted db but repair not allowed", JET_errDatabaseCorruptedNoRepair)
        {
        }

        IsamDatabaseCorruptedNoRepairException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamDatabaseCorruptedNoRepairException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidCreateDbVersionException : public IsamInconsistentException
    {
    public:
        IsamInvalidCreateDbVersionException() : IsamInconsistentException( "recovery tried to replay a database creation, but the database was originally created with an incompatible (likely older) version of the database engine", JET_errInvalidCreateDbVersion)
        {
        }

        IsamInvalidCreateDbVersionException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamInvalidCreateDbVersionException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseIncompleteIncrementalReseedException : public IsamInconsistentException
    {
    public:
        IsamDatabaseIncompleteIncrementalReseedException() : IsamInconsistentException( "The database cannot be attached because it is currently being rebuilt as part of an incremental reseed.", JET_errDatabaseIncompleteIncrementalReseed)
        {
        }

        IsamDatabaseIncompleteIncrementalReseedException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamDatabaseIncompleteIncrementalReseedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseInvalidIncrementalReseedException : public IsamUsageException
    {
    public:
        IsamDatabaseInvalidIncrementalReseedException() : IsamUsageException( "The database is not a valid state to perform an incremental reseed.", JET_errDatabaseInvalidIncrementalReseed)
        {
        }

        IsamDatabaseInvalidIncrementalReseedException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamDatabaseInvalidIncrementalReseedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseFailedIncrementalReseedException : public IsamStateException
    {
    public:
        IsamDatabaseFailedIncrementalReseedException() : IsamStateException( "The incremental reseed being performed on the specified database cannot be completed due to a fatal error.  A full reseed is required to recover this database.", JET_errDatabaseFailedIncrementalReseed)
        {
        }

        IsamDatabaseFailedIncrementalReseedException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamDatabaseFailedIncrementalReseedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamNoAttachmentsFailedIncrementalReseedException : public IsamStateException
    {
    public:
        IsamNoAttachmentsFailedIncrementalReseedException() : IsamStateException( "The incremental reseed being performed on the specified database cannot be completed because the min required log contains no attachment info.  A full reseed is required to recover this database.", JET_errNoAttachmentsFailedIncrementalReseed)
        {
        }

        IsamNoAttachmentsFailedIncrementalReseedException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamNoAttachmentsFailedIncrementalReseedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseNotReadyException : public IsamUsageException
    {
    public:
        IsamDatabaseNotReadyException() : IsamUsageException( "Recovery on this database has not yet completed enough to permit access.", JET_errDatabaseNotReady)
        {
        }

        IsamDatabaseNotReadyException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamDatabaseNotReadyException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseAttachedForRecoveryException : public IsamUsageException
    {
    public:
        IsamDatabaseAttachedForRecoveryException() : IsamUsageException( "Database is attached but only for recovery.  It must be explicitly attached before it can be opened. ", JET_errDatabaseAttachedForRecovery)
        {
        }

        IsamDatabaseAttachedForRecoveryException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamDatabaseAttachedForRecoveryException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTransactionsNotReadyDuringRecoveryException : public IsamStateException
    {
    public:
        IsamTransactionsNotReadyDuringRecoveryException() : IsamStateException( "Recovery has not seen any Begin0/Commit0 records and so does not know what trxBegin0 to assign to this transaction", JET_errTransactionsNotReadyDuringRecovery)
        {
        }

        IsamTransactionsNotReadyDuringRecoveryException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamTransactionsNotReadyDuringRecoveryException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTableLockedException : public IsamUsageException
    {
    public:
        IsamTableLockedException() : IsamUsageException( "Table is exclusively locked", JET_errTableLocked)
        {
        }

        IsamTableLockedException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamTableLockedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTableDuplicateException : public IsamStateException
    {
    public:
        IsamTableDuplicateException() : IsamStateException( "Table already exists", JET_errTableDuplicate)
        {
        }

        IsamTableDuplicateException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamTableDuplicateException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTableInUseException : public IsamStateException
    {
    public:
        IsamTableInUseException() : IsamStateException( "Table is in use, cannot lock", JET_errTableInUse)
        {
        }

        IsamTableInUseException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamTableInUseException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamObjectNotFoundException : public IsamStateException
    {
    public:
        IsamObjectNotFoundException() : IsamStateException( "No such table or object", JET_errObjectNotFound)
        {
        }

        IsamObjectNotFoundException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamObjectNotFoundException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDensityInvalidException : public IsamUsageException
    {
    public:
        IsamDensityInvalidException() : IsamUsageException( "Bad file/index density", JET_errDensityInvalid)
        {
        }

        IsamDensityInvalidException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamDensityInvalidException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTableNotEmptyException : public IsamUsageException
    {
    public:
        IsamTableNotEmptyException() : IsamUsageException( "Table is not empty", JET_errTableNotEmpty)
        {
        }

        IsamTableNotEmptyException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamTableNotEmptyException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidTableIdException : public IsamUsageException
    {
    public:
        IsamInvalidTableIdException() : IsamUsageException( "Invalid table id", JET_errInvalidTableId)
        {
        }

        IsamInvalidTableIdException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidTableIdException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTooManyOpenTablesException : public IsamQuotaException
    {
    public:
        IsamTooManyOpenTablesException() : IsamQuotaException( "Cannot open any more tables (cleanup already attempted)", JET_errTooManyOpenTables)
        {
        }

        IsamTooManyOpenTablesException( String ^ description, Exception^ innerException ) :
            IsamQuotaException( description, innerException )
        {
        }

        IsamTooManyOpenTablesException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamQuotaException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamIllegalOperationException : public IsamUsageException
    {
    public:
        IsamIllegalOperationException() : IsamUsageException( "Oper. not supported on table", JET_errIllegalOperation)
        {
        }

        IsamIllegalOperationException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamIllegalOperationException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTooManyOpenTablesAndCleanupTimedOutException : public IsamUsageException
    {
    public:
        IsamTooManyOpenTablesAndCleanupTimedOutException() : IsamUsageException( "Cannot open any more tables (cleanup attempt failed to complete)", JET_errTooManyOpenTablesAndCleanupTimedOut)
        {
        }

        IsamTooManyOpenTablesAndCleanupTimedOutException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamTooManyOpenTablesAndCleanupTimedOutException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamCannotDeleteTempTableException : public IsamUsageException
    {
    public:
        IsamCannotDeleteTempTableException() : IsamUsageException( "Use CloseTable instead of DeleteTable to delete temp table", JET_errCannotDeleteTempTable)
        {
        }

        IsamCannotDeleteTempTableException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamCannotDeleteTempTableException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamCannotDeleteSystemTableException : public IsamUsageException
    {
    public:
        IsamCannotDeleteSystemTableException() : IsamUsageException( "Illegal attempt to delete a system table", JET_errCannotDeleteSystemTable)
        {
        }

        IsamCannotDeleteSystemTableException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamCannotDeleteSystemTableException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamCannotDeleteTemplateTableException : public IsamUsageException
    {
    public:
        IsamCannotDeleteTemplateTableException() : IsamUsageException( "Illegal attempt to delete a template table", JET_errCannotDeleteTemplateTable)
        {
        }

        IsamCannotDeleteTemplateTableException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamCannotDeleteTemplateTableException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamExclusiveTableLockRequiredException : public IsamUsageException
    {
    public:
        IsamExclusiveTableLockRequiredException() : IsamUsageException( "Must have exclusive lock on table.", JET_errExclusiveTableLockRequired)
        {
        }

        IsamExclusiveTableLockRequiredException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamExclusiveTableLockRequiredException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamFixedDDLException : public IsamUsageException
    {
    public:
        IsamFixedDDLException() : IsamUsageException( "DDL operations prohibited on this table", JET_errFixedDDL)
        {
        }

        IsamFixedDDLException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamFixedDDLException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamFixedInheritedDDLException : public IsamUsageException
    {
    public:
        IsamFixedInheritedDDLException() : IsamUsageException( "On a derived table, DDL operations are prohibited on inherited portion of DDL", JET_errFixedInheritedDDL)
        {
        }

        IsamFixedInheritedDDLException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamFixedInheritedDDLException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamCannotNestDDLException : public IsamUsageException
    {
    public:
        IsamCannotNestDDLException() : IsamUsageException( "Nesting of hierarchical DDL is not currently supported.", JET_errCannotNestDDL)
        {
        }

        IsamCannotNestDDLException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamCannotNestDDLException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDDLNotInheritableException : public IsamUsageException
    {
    public:
        IsamDDLNotInheritableException() : IsamUsageException( "Tried to inherit DDL from a table not marked as a template table.", JET_errDDLNotInheritable)
        {
        }

        IsamDDLNotInheritableException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamDDLNotInheritableException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidSettingsException : public IsamUsageException
    {
    public:
        IsamInvalidSettingsException() : IsamUsageException( "System parameters were set improperly", JET_errInvalidSettings)
        {
        }

        IsamInvalidSettingsException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidSettingsException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamClientRequestToStopJetServiceException : public IsamOperationException
    {
    public:
        IsamClientRequestToStopJetServiceException() : IsamOperationException( "Client has requested stop service", JET_errClientRequestToStopJetService)
        {
        }

        IsamClientRequestToStopJetServiceException( String ^ description, Exception^ innerException ) :
            IsamOperationException( description, innerException )
        {
        }

        IsamClientRequestToStopJetServiceException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamOperationException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamIndexHasPrimaryException : public IsamUsageException
    {
    public:
        IsamIndexHasPrimaryException() : IsamUsageException( "Primary index already defined", JET_errIndexHasPrimary)
        {
        }

        IsamIndexHasPrimaryException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamIndexHasPrimaryException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamIndexDuplicateException : public IsamUsageException
    {
    public:
        IsamIndexDuplicateException() : IsamUsageException( "Index is already defined", JET_errIndexDuplicate)
        {
        }

        IsamIndexDuplicateException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamIndexDuplicateException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamIndexNotFoundException : public IsamStateException
    {
    public:
        IsamIndexNotFoundException() : IsamStateException( "No such index", JET_errIndexNotFound)
        {
        }

        IsamIndexNotFoundException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamIndexNotFoundException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamIndexMustStayException : public IsamUsageException
    {
    public:
        IsamIndexMustStayException() : IsamUsageException( "Cannot delete clustered index", JET_errIndexMustStay)
        {
        }

        IsamIndexMustStayException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamIndexMustStayException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamIndexInvalidDefException : public IsamUsageException
    {
    public:
        IsamIndexInvalidDefException() : IsamUsageException( "Illegal index definition", JET_errIndexInvalidDef)
        {
        }

        IsamIndexInvalidDefException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamIndexInvalidDefException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidCreateIndexException : public IsamUsageException
    {
    public:
        IsamInvalidCreateIndexException() : IsamUsageException( "Invalid create index description", JET_errInvalidCreateIndex)
        {
        }

        IsamInvalidCreateIndexException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidCreateIndexException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTooManyOpenIndexesException : public IsamMemoryException
    {
    public:
        IsamTooManyOpenIndexesException() : IsamMemoryException( "Out of index description blocks", JET_errTooManyOpenIndexes)
        {
        }

        IsamTooManyOpenIndexesException( String ^ description, Exception^ innerException ) :
            IsamMemoryException( description, innerException )
        {
        }

        IsamTooManyOpenIndexesException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamMemoryException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamMultiValuedIndexViolationException : public IsamUsageException
    {
    public:
        IsamMultiValuedIndexViolationException() : IsamUsageException( "Non-unique inter-record index keys generated for a multivalued index", JET_errMultiValuedIndexViolation)
        {
        }

        IsamMultiValuedIndexViolationException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamMultiValuedIndexViolationException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamIndexBuildCorruptedException : public IsamCorruptionException
    {
    public:
        IsamIndexBuildCorruptedException() : IsamCorruptionException( "Failed to build a secondary index that properly reflects primary index", JET_errIndexBuildCorrupted)
        {
        }

        IsamIndexBuildCorruptedException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamIndexBuildCorruptedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamPrimaryIndexCorruptedException : public IsamCorruptionException
    {
    public:
        IsamPrimaryIndexCorruptedException() : IsamCorruptionException( "Primary index is corrupt. The database must be defragmented or the table deleted.", JET_errPrimaryIndexCorrupted)
        {
        }

        IsamPrimaryIndexCorruptedException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamPrimaryIndexCorruptedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamSecondaryIndexCorruptedException : public IsamCorruptionException
    {
    public:
        IsamSecondaryIndexCorruptedException() : IsamCorruptionException( "Secondary index is corrupt. The database must be defragmented or the affected index must be deleted. If the corrupt index is over Unicode text, a likely cause is a sort-order change.", JET_errSecondaryIndexCorrupted)
        {
        }

        IsamSecondaryIndexCorruptedException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamSecondaryIndexCorruptedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidIndexIdException : public IsamUsageException
    {
    public:
        IsamInvalidIndexIdException() : IsamUsageException( "Illegal index id", JET_errInvalidIndexId)
        {
        }

        IsamInvalidIndexIdException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidIndexIdException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamIndexTuplesSecondaryIndexOnlyException : public IsamUsageException
    {
    public:
        IsamIndexTuplesSecondaryIndexOnlyException() : IsamUsageException( "tuple index can only be on a secondary index", JET_errIndexTuplesSecondaryIndexOnly)
        {
        }

        IsamIndexTuplesSecondaryIndexOnlyException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamIndexTuplesSecondaryIndexOnlyException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamIndexTuplesNonUniqueOnlyException : public IsamUsageException
    {
    public:
        IsamIndexTuplesNonUniqueOnlyException() : IsamUsageException( "tuple index must be a non-unique index", JET_errIndexTuplesNonUniqueOnly)
        {
        }

        IsamIndexTuplesNonUniqueOnlyException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamIndexTuplesNonUniqueOnlyException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamIndexTuplesTextBinaryColumnsOnlyException : public IsamUsageException
    {
    public:
        IsamIndexTuplesTextBinaryColumnsOnlyException() : IsamUsageException( "tuple index must be on a text/binary column", JET_errIndexTuplesTextBinaryColumnsOnly)
        {
        }

        IsamIndexTuplesTextBinaryColumnsOnlyException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamIndexTuplesTextBinaryColumnsOnlyException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamIndexTuplesVarSegMacNotAllowedException : public IsamUsageException
    {
    public:
        IsamIndexTuplesVarSegMacNotAllowedException() : IsamUsageException( "tuple index does not allow setting cbVarSegMac", JET_errIndexTuplesVarSegMacNotAllowed)
        {
        }

        IsamIndexTuplesVarSegMacNotAllowedException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamIndexTuplesVarSegMacNotAllowedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamIndexTuplesInvalidLimitsException : public IsamUsageException
    {
    public:
        IsamIndexTuplesInvalidLimitsException() : IsamUsageException( "invalid min/max tuple length or max characters to index specified", JET_errIndexTuplesInvalidLimits)
        {
        }

        IsamIndexTuplesInvalidLimitsException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamIndexTuplesInvalidLimitsException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamIndexTuplesCannotRetrieveFromIndexException : public IsamUsageException
    {
    public:
        IsamIndexTuplesCannotRetrieveFromIndexException() : IsamUsageException( "cannot call RetrieveColumn() with RetrieveFromIndex on a tuple index", JET_errIndexTuplesCannotRetrieveFromIndex)
        {
        }

        IsamIndexTuplesCannotRetrieveFromIndexException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamIndexTuplesCannotRetrieveFromIndexException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamIndexTuplesKeyTooSmallException : public IsamUsageException
    {
    public:
        IsamIndexTuplesKeyTooSmallException() : IsamUsageException( "specified key does not meet minimum tuple length", JET_errIndexTuplesKeyTooSmall)
        {
        }

        IsamIndexTuplesKeyTooSmallException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamIndexTuplesKeyTooSmallException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidLVChunkSizeException : public IsamUsageException
    {
    public:
        IsamInvalidLVChunkSizeException() : IsamUsageException( "Specified LV chunk size is not supported", JET_errInvalidLVChunkSize)
        {
        }

        IsamInvalidLVChunkSizeException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidLVChunkSizeException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamColumnCannotBeEncryptedException : public IsamUsageException
    {
    public:
        IsamColumnCannotBeEncryptedException() : IsamUsageException( "Only JET_coltypLongText and JET_coltypLongBinary columns without default values can be encrypted", JET_errColumnCannotBeEncrypted)
        {
        }

        IsamColumnCannotBeEncryptedException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamColumnCannotBeEncryptedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamCannotIndexOnEncryptedColumnException : public IsamUsageException
    {
    public:
        IsamCannotIndexOnEncryptedColumnException() : IsamUsageException( "Cannot index encrypted column", JET_errCannotIndexOnEncryptedColumn)
        {
        }

        IsamCannotIndexOnEncryptedColumnException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamCannotIndexOnEncryptedColumnException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamColumnNoChunkException : public IsamUsageException
    {
    public:
        IsamColumnNoChunkException() : IsamUsageException( "No such chunk in long value", JET_errColumnNoChunk)
        {
        }

        IsamColumnNoChunkException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamColumnNoChunkException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamColumnDoesNotFitException : public IsamUsageException
    {
    public:
        IsamColumnDoesNotFitException() : IsamUsageException( "Field will not fit in record", JET_errColumnDoesNotFit)
        {
        }

        IsamColumnDoesNotFitException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamColumnDoesNotFitException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamNullInvalidException : public IsamUsageException
    {
    public:
        IsamNullInvalidException() : IsamUsageException( "Null not valid", JET_errNullInvalid)
        {
        }

        IsamNullInvalidException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamNullInvalidException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamColumnTooBigException : public IsamUsageException
    {
    public:
        IsamColumnTooBigException() : IsamUsageException( "Field length is greater than maximum", JET_errColumnTooBig)
        {
        }

        IsamColumnTooBigException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamColumnTooBigException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamColumnNotFoundException : public IsamUsageException
    {
    public:
        IsamColumnNotFoundException() : IsamUsageException( "No such column", JET_errColumnNotFound)
        {
        }

        IsamColumnNotFoundException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamColumnNotFoundException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamColumnDuplicateException : public IsamUsageException
    {
    public:
        IsamColumnDuplicateException() : IsamUsageException( "Field is already defined", JET_errColumnDuplicate)
        {
        }

        IsamColumnDuplicateException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamColumnDuplicateException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamMultiValuedColumnMustBeTaggedException : public IsamUsageException
    {
    public:
        IsamMultiValuedColumnMustBeTaggedException() : IsamUsageException( "Attempted to create a multi-valued column, but column was not Tagged", JET_errMultiValuedColumnMustBeTagged)
        {
        }

        IsamMultiValuedColumnMustBeTaggedException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamMultiValuedColumnMustBeTaggedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamColumnRedundantException : public IsamUsageException
    {
    public:
        IsamColumnRedundantException() : IsamUsageException( "Second autoincrement or version column", JET_errColumnRedundant)
        {
        }

        IsamColumnRedundantException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamColumnRedundantException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidColumnTypeException : public IsamUsageException
    {
    public:
        IsamInvalidColumnTypeException() : IsamUsageException( "Invalid column data type", JET_errInvalidColumnType)
        {
        }

        IsamInvalidColumnTypeException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidColumnTypeException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamNoCurrentIndexException : public IsamUsageException
    {
    public:
        IsamNoCurrentIndexException() : IsamUsageException( "Invalid w/o a current index", JET_errNoCurrentIndex)
        {
        }

        IsamNoCurrentIndexException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamNoCurrentIndexException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamKeyIsMadeException : public IsamUsageException
    {
    public:
        IsamKeyIsMadeException() : IsamUsageException( "The key is completely made", JET_errKeyIsMade)
        {
        }

        IsamKeyIsMadeException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamKeyIsMadeException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamBadColumnIdException : public IsamUsageException
    {
    public:
        IsamBadColumnIdException() : IsamUsageException( "Column Id Incorrect", JET_errBadColumnId)
        {
        }

        IsamBadColumnIdException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamBadColumnIdException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamBadItagSequenceException : public IsamStateException
    {
    public:
        IsamBadItagSequenceException() : IsamStateException( "Bad itagSequence for tagged column", JET_errBadItagSequence)
        {
        }

        IsamBadItagSequenceException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamBadItagSequenceException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamCannotBeTaggedException : public IsamUsageException
    {
    public:
        IsamCannotBeTaggedException() : IsamUsageException( "AutoIncrement and Version cannot be tagged", JET_errCannotBeTagged)
        {
        }

        IsamCannotBeTaggedException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamCannotBeTaggedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDefaultValueTooBigException : public IsamUsageException
    {
    public:
        IsamDefaultValueTooBigException() : IsamUsageException( "Default value exceeds maximum size", JET_errDefaultValueTooBig)
        {
        }

        IsamDefaultValueTooBigException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamDefaultValueTooBigException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamMultiValuedDuplicateException : public IsamStateException
    {
    public:
        IsamMultiValuedDuplicateException() : IsamStateException( "Duplicate detected on a unique multi-valued column", JET_errMultiValuedDuplicate)
        {
        }

        IsamMultiValuedDuplicateException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamMultiValuedDuplicateException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLVCorruptedException : public IsamCorruptionException
    {
    public:
        IsamLVCorruptedException() : IsamCorruptionException( "Corruption encountered in long-value tree", JET_errLVCorrupted)
        {
        }

        IsamLVCorruptedException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamLVCorruptedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamMultiValuedDuplicateAfterTruncationException : public IsamStateException
    {
    public:
        IsamMultiValuedDuplicateAfterTruncationException() : IsamStateException( "Duplicate detected on a unique multi-valued column after data was normalized, and normalizing truncated the data before comparison", JET_errMultiValuedDuplicateAfterTruncation)
        {
        }

        IsamMultiValuedDuplicateAfterTruncationException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamMultiValuedDuplicateAfterTruncationException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDerivedColumnCorruptionException : public IsamCorruptionException
    {
    public:
        IsamDerivedColumnCorruptionException() : IsamCorruptionException( "Invalid column in derived table", JET_errDerivedColumnCorruption)
        {
        }

        IsamDerivedColumnCorruptionException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamDerivedColumnCorruptionException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidPlaceholderColumnException : public IsamUsageException
    {
    public:
        IsamInvalidPlaceholderColumnException() : IsamUsageException( "Tried to convert column to a primary index placeholder, but column doesn't meet necessary criteria", JET_errInvalidPlaceholderColumn)
        {
        }

        IsamInvalidPlaceholderColumnException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidPlaceholderColumnException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamColumnCannotBeCompressedException : public IsamUsageException
    {
    public:
        IsamColumnCannotBeCompressedException() : IsamUsageException( "Only JET_coltypLongText and JET_coltypLongBinary columns can be compressed", JET_errColumnCannotBeCompressed)
        {
        }

        IsamColumnCannotBeCompressedException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamColumnCannotBeCompressedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamColumnNoEncryptionKeyException : public IsamUsageException
    {
    public:
        IsamColumnNoEncryptionKeyException() : IsamUsageException( "Cannot retrieve/set encrypted column without an encryption key", JET_errColumnNoEncryptionKey)
        {
        }

        IsamColumnNoEncryptionKeyException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamColumnNoEncryptionKeyException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRecordNotFoundException : public IsamStateException
    {
    public:
        IsamRecordNotFoundException() : IsamStateException( "The key was not found", JET_errRecordNotFound)
        {
        }

        IsamRecordNotFoundException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamRecordNotFoundException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRecordNoCopyException : public IsamUsageException
    {
    public:
        IsamRecordNoCopyException() : IsamUsageException( "No working buffer", JET_errRecordNoCopy)
        {
        }

        IsamRecordNoCopyException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamRecordNoCopyException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamNoCurrentRecordException : public IsamStateException
    {
    public:
        IsamNoCurrentRecordException() : IsamStateException( "Currency not on a record", JET_errNoCurrentRecord)
        {
        }

        IsamNoCurrentRecordException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamNoCurrentRecordException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRecordPrimaryChangedException : public IsamUsageException
    {
    public:
        IsamRecordPrimaryChangedException() : IsamUsageException( "Primary key may not change", JET_errRecordPrimaryChanged)
        {
        }

        IsamRecordPrimaryChangedException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamRecordPrimaryChangedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamKeyDuplicateException : public IsamStateException
    {
    public:
        IsamKeyDuplicateException() : IsamStateException( "Illegal duplicate key", JET_errKeyDuplicate)
        {
        }

        IsamKeyDuplicateException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamKeyDuplicateException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamAlreadyPreparedException : public IsamUsageException
    {
    public:
        IsamAlreadyPreparedException() : IsamUsageException( "Attempted to update record when record update was already in progress", JET_errAlreadyPrepared)
        {
        }

        IsamAlreadyPreparedException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamAlreadyPreparedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamKeyNotMadeException : public IsamUsageException
    {
    public:
        IsamKeyNotMadeException() : IsamUsageException( "No call to JetMakeKey", JET_errKeyNotMade)
        {
        }

        IsamKeyNotMadeException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamKeyNotMadeException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamUpdateNotPreparedException : public IsamUsageException
    {
    public:
        IsamUpdateNotPreparedException() : IsamUsageException( "No call to JetPrepareUpdate", JET_errUpdateNotPrepared)
        {
        }

        IsamUpdateNotPreparedException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamUpdateNotPreparedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDecompressionFailedException : public IsamCorruptionException
    {
    public:
        IsamDecompressionFailedException() : IsamCorruptionException( "Internal error: data could not be decompressed", JET_errDecompressionFailed)
        {
        }

        IsamDecompressionFailedException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamDecompressionFailedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamUpdateMustVersionException : public IsamUsageException
    {
    public:
        IsamUpdateMustVersionException() : IsamUsageException( "No version updates only for uncommitted tables", JET_errUpdateMustVersion)
        {
        }

        IsamUpdateMustVersionException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamUpdateMustVersionException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDecryptionFailedException : public IsamCorruptionException
    {
    public:
        IsamDecryptionFailedException() : IsamCorruptionException( "Data could not be decrypted", JET_errDecryptionFailed)
        {
        }

        IsamDecryptionFailedException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamDecryptionFailedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamEncryptionBadItagException : public IsamUsageException
    {
    public:
        IsamEncryptionBadItagException() : IsamUsageException( "Cannot encrypt tagged columns with itag>1", JET_errEncryptionBadItag)
        {
        }

        IsamEncryptionBadItagException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamEncryptionBadItagException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTooManySortsException : public IsamMemoryException
    {
    public:
        IsamTooManySortsException() : IsamMemoryException( "Too many sort processes", JET_errTooManySorts)
        {
        }

        IsamTooManySortsException( String ^ description, Exception^ innerException ) :
            IsamMemoryException( description, innerException )
        {
        }

        IsamTooManySortsException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamMemoryException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTooManyAttachedDatabasesException : public IsamUsageException
    {
    public:
        IsamTooManyAttachedDatabasesException() : IsamUsageException( "Too many open databases", JET_errTooManyAttachedDatabases)
        {
        }

        IsamTooManyAttachedDatabasesException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamTooManyAttachedDatabasesException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDiskFullException : public IsamDiskException
    {
    public:
        IsamDiskFullException() : IsamDiskException( "No space left on disk", JET_errDiskFull)
        {
        }

        IsamDiskFullException( String ^ description, Exception^ innerException ) :
            IsamDiskException( description, innerException )
        {
        }

        IsamDiskFullException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamDiskException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamPermissionDeniedException : public IsamUsageException
    {
    public:
        IsamPermissionDeniedException() : IsamUsageException( "Permission denied", JET_errPermissionDenied)
        {
        }

        IsamPermissionDeniedException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamPermissionDeniedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamFileNotFoundException : public IsamStateException
    {
    public:
        IsamFileNotFoundException() : IsamStateException( "File not found", JET_errFileNotFound)
        {
        }

        IsamFileNotFoundException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamFileNotFoundException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamFileInvalidTypeException : public IsamInconsistentException
    {
    public:
        IsamFileInvalidTypeException() : IsamInconsistentException( "Invalid file type", JET_errFileInvalidType)
        {
        }

        IsamFileInvalidTypeException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamFileInvalidTypeException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamFileAlreadyExistsException : public IsamInconsistentException
    {
    public:
        IsamFileAlreadyExistsException() : IsamInconsistentException( "File already exists", JET_errFileAlreadyExists)
        {
        }

        IsamFileAlreadyExistsException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamFileAlreadyExistsException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamAfterInitializationException : public IsamUsageException
    {
    public:
        IsamAfterInitializationException() : IsamUsageException( "Cannot Restore after init.", JET_errAfterInitialization)
        {
        }

        IsamAfterInitializationException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamAfterInitializationException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLogCorruptedException : public IsamCorruptionException
    {
    public:
        IsamLogCorruptedException() : IsamCorruptionException( "Logs could not be interpreted", JET_errLogCorrupted)
        {
        }

        IsamLogCorruptedException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamLogCorruptedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidOperationException : public IsamUsageException
    {
    public:
        IsamInvalidOperationException() : IsamUsageException( "Invalid operation", JET_errInvalidOperation)
        {
        }

        IsamInvalidOperationException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamInvalidOperationException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamSessionSharingViolationException : public IsamUsageException
    {
    public:
        IsamSessionSharingViolationException() : IsamUsageException( "Multiple threads are using the same session", JET_errSessionSharingViolation)
        {
        }

        IsamSessionSharingViolationException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamSessionSharingViolationException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamEntryPointNotFoundException : public IsamUsageException
    {
    public:
        IsamEntryPointNotFoundException() : IsamUsageException( "An entry point in a DLL we require could not be found", JET_errEntryPointNotFound)
        {
        }

        IsamEntryPointNotFoundException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamEntryPointNotFoundException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamSessionContextAlreadySetException : public IsamUsageException
    {
    public:
        IsamSessionContextAlreadySetException() : IsamUsageException( "Specified session already has a session context set", JET_errSessionContextAlreadySet)
        {
        }

        IsamSessionContextAlreadySetException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamSessionContextAlreadySetException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamSessionContextNotSetByThisThreadException : public IsamUsageException
    {
    public:
        IsamSessionContextNotSetByThisThreadException() : IsamUsageException( "Tried to reset session context, but current thread did not originally set the session context", JET_errSessionContextNotSetByThisThread)
        {
        }

        IsamSessionContextNotSetByThisThreadException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamSessionContextNotSetByThisThreadException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamSessionInUseException : public IsamUsageException
    {
    public:
        IsamSessionInUseException() : IsamUsageException( "Tried to terminate session in use", JET_errSessionInUse)
        {
        }

        IsamSessionInUseException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamSessionInUseException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRecordFormatConversionFailedException : public IsamCorruptionException
    {
    public:
        IsamRecordFormatConversionFailedException() : IsamCorruptionException( "Internal error during dynamic record format conversion", JET_errRecordFormatConversionFailed)
        {
        }

        IsamRecordFormatConversionFailedException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamRecordFormatConversionFailedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamOneDatabasePerSessionException : public IsamUsageException
    {
    public:
        IsamOneDatabasePerSessionException() : IsamUsageException( "Just one open user database per session is allowed (JET_paramOneDatabasePerSession)", JET_errOneDatabasePerSession)
        {
        }

        IsamOneDatabasePerSessionException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamOneDatabasePerSessionException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRollbackErrorException : public IsamFatalException
    {
    public:
        IsamRollbackErrorException() : IsamFatalException( "error during rollback", JET_errRollbackError)
        {
        }

        IsamRollbackErrorException( String ^ description, Exception^ innerException ) :
            IsamFatalException( description, innerException )
        {
        }

        IsamRollbackErrorException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamFatalException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamFlushMapVersionUnsupportedException : public IsamUsageException
    {
    public:
        IsamFlushMapVersionUnsupportedException() : IsamUsageException( "The version of the persisted flush map is not supported by this version of the engine.", JET_errFlushMapVersionUnsupported)
        {
        }

        IsamFlushMapVersionUnsupportedException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamFlushMapVersionUnsupportedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamFlushMapDatabaseMismatchException : public IsamUsageException
    {
    public:
        IsamFlushMapDatabaseMismatchException() : IsamUsageException( "The persisted flush map and the database do not match.", JET_errFlushMapDatabaseMismatch)
        {
        }

        IsamFlushMapDatabaseMismatchException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamFlushMapDatabaseMismatchException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamFlushMapUnrecoverableException : public IsamStateException
    {
    public:
        IsamFlushMapUnrecoverableException() : IsamStateException( "The persisted flush map cannot be reconstructed.", JET_errFlushMapUnrecoverable)
        {
        }

        IsamFlushMapUnrecoverableException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamFlushMapUnrecoverableException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRBSFileCorruptException : public IsamCorruptionException
    {
    public:
        IsamRBSFileCorruptException() : IsamCorruptionException( "RBS file is corrupt", JET_errRBSFileCorrupt)
        {
        }

        IsamRBSFileCorruptException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamRBSFileCorruptException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRBSHeaderCorruptException : public IsamCorruptionException
    {
    public:
        IsamRBSHeaderCorruptException() : IsamCorruptionException( "RBS header is corrupt", JET_errRBSHeaderCorrupt)
        {
        }

        IsamRBSHeaderCorruptException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamRBSHeaderCorruptException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRBSDbMismatchException : public IsamInconsistentException
    {
    public:
        IsamRBSDbMismatchException() : IsamInconsistentException( "RBS is out of sync with the database file", JET_errRBSDbMismatch)
        {
        }

        IsamRBSDbMismatchException( String ^ description, Exception^ innerException ) :
            IsamInconsistentException( description, innerException )
        {
        }

        IsamRBSDbMismatchException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamInconsistentException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamBadRBSVersionException : public IsamStateException
    {
    public:
        IsamBadRBSVersionException() : IsamStateException( "Version of revert snapshot file is not compatible with Jet version", JET_errBadRBSVersion)
        {
        }

        IsamBadRBSVersionException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamBadRBSVersionException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamOutOfRBSSpaceException : public IsamFragmentationException
    {
    public:
        IsamOutOfRBSSpaceException() : IsamFragmentationException( "Revert snapshot file has reached its maximum size", JET_errOutOfRBSSpace)
        {
        }

        IsamOutOfRBSSpaceException( String ^ description, Exception^ innerException ) :
            IsamFragmentationException( description, innerException )
        {
        }

        IsamOutOfRBSSpaceException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamFragmentationException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRBSInvalidSignException : public IsamStateException
    {
    public:
        IsamRBSInvalidSignException() : IsamStateException( "RBS signature is not set in the RBS header", JET_errRBSInvalidSign)
        {
        }

        IsamRBSInvalidSignException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamRBSInvalidSignException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRBSInvalidRecordException : public IsamStateException
    {
    public:
        IsamRBSInvalidRecordException() : IsamStateException( "Invalid RBS record found in the revert snapshot", JET_errRBSInvalidRecord)
        {
        }

        IsamRBSInvalidRecordException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamRBSInvalidRecordException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRBSRCInvalidRBSException : public IsamStateException
    {
    public:
        IsamRBSRCInvalidRBSException() : IsamStateException( "The database cannot be reverted to the expected time as there are some invalid revert snapshots to revert to that time.", JET_errRBSRCInvalidRBS)
        {
        }

        IsamRBSRCInvalidRBSException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamRBSRCInvalidRBSException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRBSRCNoRBSFoundException : public IsamStateException
    {
    public:
        IsamRBSRCNoRBSFoundException() : IsamStateException( "The database cannot be reverted to the expected time as there no revert snapshots to revert to that time.", JET_errRBSRCNoRBSFound)
        {
        }

        IsamRBSRCNoRBSFoundException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamRBSRCNoRBSFoundException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRBSRCBadDbStateException : public IsamStateException
    {
    public:
        IsamRBSRCBadDbStateException() : IsamStateException( "The database revert to the expected time failed as the database has a bad dbstate.", JET_errRBSRCBadDbState)
        {
        }

        IsamRBSRCBadDbStateException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamRBSRCBadDbStateException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRBSMissingReqLogsException : public IsamStateException
    {
    public:
        IsamRBSMissingReqLogsException() : IsamStateException( "The required logs for the revert snapshot are missing.", JET_errRBSMissingReqLogs)
        {
        }

        IsamRBSMissingReqLogsException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamRBSMissingReqLogsException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRBSLogDivergenceFailedException : public IsamStateException
    {
    public:
        IsamRBSLogDivergenceFailedException() : IsamStateException( "The required logs for the revert snapshot are diverged with logs in the log directory.", JET_errRBSLogDivergenceFailed)
        {
        }

        IsamRBSLogDivergenceFailedException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamRBSLogDivergenceFailedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRBSRCCopyLogsRevertStateException : public IsamStateException
    {
    public:
        IsamRBSRCCopyLogsRevertStateException() : IsamStateException( "The database cannot be reverted to the expected time as we are in copying logs stage from previous revert request and a further revert in the past is requested which might leave logs in corrupt state.", JET_errRBSRCCopyLogsRevertState)
        {
        }

        IsamRBSRCCopyLogsRevertStateException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamRBSRCCopyLogsRevertStateException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseIncompleteRevertException : public IsamStateException
    {
    public:
        IsamDatabaseIncompleteRevertException() : IsamStateException( "The database cannot be attached because it is currently being reverted using revert snapshot.", JET_errDatabaseIncompleteRevert)
        {
        }

        IsamDatabaseIncompleteRevertException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamDatabaseIncompleteRevertException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRBSRCRevertCancelledException : public IsamStateException
    {
    public:
        IsamRBSRCRevertCancelledException() : IsamStateException( "The database revert has been cancelled.", JET_errRBSRCRevertCancelled)
        {
        }

        IsamRBSRCRevertCancelledException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamRBSRCRevertCancelledException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRBSRCInvalidDbFormatVersionException : public IsamStateException
    {
    public:
        IsamRBSRCInvalidDbFormatVersionException() : IsamStateException( "The database format version for the databases to be reverted doesn't support applying the revert snapshot.", JET_errRBSRCInvalidDbFormatVersion)
        {
        }

        IsamRBSRCInvalidDbFormatVersionException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamRBSRCInvalidDbFormatVersionException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamRBSCannotDetermineDivergenceException : public IsamStateException
    {
    public:
        IsamRBSCannotDetermineDivergenceException() : IsamStateException( "The required logs for the revert snapshot are missing in log directory and hence we cannot determine if those logs are diverged with the logs in snapshot directory.", JET_errRBSCannotDetermineDivergence)
        {
        }

        IsamRBSCannotDetermineDivergenceException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamRBSCannotDetermineDivergenceException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamDatabaseAlreadyRunningMaintenanceException : public IsamUsageException
    {
    public:
        IsamDatabaseAlreadyRunningMaintenanceException() : IsamUsageException( "The operation did not complete successfully because the database is already running maintenance on specified database", JET_errDatabaseAlreadyRunningMaintenance)
        {
        }

        IsamDatabaseAlreadyRunningMaintenanceException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamDatabaseAlreadyRunningMaintenanceException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamCallbackFailedException : public IsamStateException
    {
    public:
        IsamCallbackFailedException() : IsamStateException( "A callback failed", JET_errCallbackFailed)
        {
        }

        IsamCallbackFailedException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamCallbackFailedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamCallbackNotResolvedException : public IsamUsageException
    {
    public:
        IsamCallbackNotResolvedException() : IsamUsageException( "A callback function could not be found", JET_errCallbackNotResolved)
        {
        }

        IsamCallbackNotResolvedException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamCallbackNotResolvedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamSpaceHintsInvalidException : public IsamUsageException
    {
    public:
        IsamSpaceHintsInvalidException() : IsamUsageException( "An element of the JET space hints structure was not correct or actionable.", JET_errSpaceHintsInvalid)
        {
        }

        IsamSpaceHintsInvalidException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamSpaceHintsInvalidException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamOSSnapshotInvalidSequenceException : public IsamUsageException
    {
    public:
        IsamOSSnapshotInvalidSequenceException() : IsamUsageException( "OS Shadow copy API used in an invalid sequence", JET_errOSSnapshotInvalidSequence)
        {
        }

        IsamOSSnapshotInvalidSequenceException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamOSSnapshotInvalidSequenceException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamOSSnapshotTimeOutException : public IsamOperationException
    {
    public:
        IsamOSSnapshotTimeOutException() : IsamOperationException( "OS Shadow copy ended with time-out", JET_errOSSnapshotTimeOut)
        {
        }

        IsamOSSnapshotTimeOutException( String ^ description, Exception^ innerException ) :
            IsamOperationException( description, innerException )
        {
        }

        IsamOSSnapshotTimeOutException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamOperationException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamOSSnapshotNotAllowedException : public IsamStateException
    {
    public:
        IsamOSSnapshotNotAllowedException() : IsamStateException( "OS Shadow copy not allowed (backup or recovery in progress)", JET_errOSSnapshotNotAllowed)
        {
        }

        IsamOSSnapshotNotAllowedException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamOSSnapshotNotAllowedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamOSSnapshotInvalidSnapIdException : public IsamUsageException
    {
    public:
        IsamOSSnapshotInvalidSnapIdException() : IsamUsageException( "invalid JET_OSSNAPID", JET_errOSSnapshotInvalidSnapId)
        {
        }

        IsamOSSnapshotInvalidSnapIdException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamOSSnapshotInvalidSnapIdException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTooManyTestInjectionsException : public IsamUsageException
    {
    public:
        IsamTooManyTestInjectionsException() : IsamUsageException( "Internal test injection limit hit", JET_errTooManyTestInjections)
        {
        }

        IsamTooManyTestInjectionsException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamTooManyTestInjectionsException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamTestInjectionNotSupportedException : public IsamStateException
    {
    public:
        IsamTestInjectionNotSupportedException() : IsamStateException( "Test injection not supported", JET_errTestInjectionNotSupported)
        {
        }

        IsamTestInjectionNotSupportedException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamTestInjectionNotSupportedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamInvalidLogDataSequenceException : public IsamStateException
    {
    public:
        IsamInvalidLogDataSequenceException() : IsamStateException( "Some how the log data provided got out of sequence with the current state of the instance", JET_errInvalidLogDataSequence)
        {
        }

        IsamInvalidLogDataSequenceException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamInvalidLogDataSequenceException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLSCallbackNotSpecifiedException : public IsamUsageException
    {
    public:
        IsamLSCallbackNotSpecifiedException() : IsamUsageException( "Attempted to use Local Storage without a callback function being specified", JET_errLSCallbackNotSpecified)
        {
        }

        IsamLSCallbackNotSpecifiedException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamLSCallbackNotSpecifiedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLSAlreadySetException : public IsamUsageException
    {
    public:
        IsamLSAlreadySetException() : IsamUsageException( "Attempted to set Local Storage for an object which already had it set", JET_errLSAlreadySet)
        {
        }

        IsamLSAlreadySetException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamLSAlreadySetException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamLSNotSetException : public IsamStateException
    {
    public:
        IsamLSNotSetException() : IsamStateException( "Attempted to retrieve Local Storage from an object which didn't have it set", JET_errLSNotSet)
        {
        }

        IsamLSNotSetException( String ^ description, Exception^ innerException ) :
            IsamStateException( description, innerException )
        {
        }

        IsamLSNotSetException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamStateException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamFileIOBeyondEOFException : public IsamCorruptionException
    {
    public:
        IsamFileIOBeyondEOFException() : IsamCorruptionException( "a read was issued to a location beyond EOF (writes will expand the file)", JET_errFileIOBeyondEOF)
        {
        }

        IsamFileIOBeyondEOFException( String ^ description, Exception^ innerException ) :
            IsamCorruptionException( description, innerException )
        {
        }

        IsamFileIOBeyondEOFException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamCorruptionException( info, context )
        {
        }

    };

    [Serializable]
    public ref class IsamFileCompressedException : public IsamUsageException
    {
    public:
        IsamFileCompressedException() : IsamUsageException( "read/write access is not supported on compressed files", JET_errFileCompressed)
        {
        }

        IsamFileCompressedException( String ^ description, Exception^ innerException ) :
            IsamUsageException( description, innerException )
        {
        }

        IsamFileCompressedException(
            System::Runtime::Serialization::SerializationInfo^ info,
            System::Runtime::Serialization::StreamingContext context
        )
            : IsamUsageException( info, context )
        {
        }

    };
public ref class EseExceptionHelper
{
public:
static IsamErrorException^ JetErrToException( const JET_ERR err )
    {
    switch ( err )
        {
        case JET_errOutOfThreads:
            return gcnew IsamOutOfThreadsException;
        case JET_errTooManyIO:
            return gcnew IsamTooManyIOException;
        case JET_errTaskDropped:
            return gcnew IsamTaskDroppedException;
        case JET_errInternalError:
            return gcnew IsamInternalErrorException;
        case JET_errDisabledFunctionality:
            return gcnew IsamDisabledFunctionalityException;
        case JET_errUnloadableOSFunctionality:
            return gcnew IsamUnloadableOSFunctionalityException;
        case JET_errDeviceMissing:
            return gcnew IsamDeviceMissingException;
        case JET_errDeviceMisconfigured:
            return gcnew IsamDeviceMisconfiguredException;
        case JET_errDeviceTimeout:
            return gcnew IsamDeviceTimeoutException;
        case JET_errDeviceFailure:
            return gcnew IsamDeviceFailureException;
        case JET_errDatabaseBufferDependenciesCorrupted:
            return gcnew IsamDatabaseBufferDependenciesCorruptedException;
        case JET_errBadPageLink:
            return gcnew IsamBadPageLinkException;
        case JET_errNTSystemCallFailed:
            return gcnew IsamNTSystemCallFailedException;
        case JET_errBadParentPageLink:
            return gcnew IsamBadParentPageLinkException;
        case JET_errSPAvailExtCorrupted:
            return gcnew IsamSPAvailExtCorruptedException;
        case JET_errSPOwnExtCorrupted:
            return gcnew IsamSPOwnExtCorruptedException;
        case JET_errDbTimeCorrupted:
            return gcnew IsamDbTimeCorruptedException;
        case JET_errKeyTruncated:
            return gcnew IsamKeyTruncatedException;
        case JET_errDatabaseLeakInSpace:
            return gcnew IsamDatabaseLeakInSpaceException;
        case JET_errBadEmptyPage:
            return gcnew IsamBadEmptyPageException;
        case JET_errBadLineCount:
            return gcnew IsamBadLineCountException;
        case JET_errPageTagCorrupted:
            return gcnew IsamPageTagCorruptedException;
        case JET_errNodeCorrupted:
            return gcnew IsamNodeCorruptedException;
        case JET_errCannotSeparateIntrinsicLV:
            return gcnew IsamCannotSeparateIntrinsicLVException;
        case JET_errSeparatedLongValue:
            return gcnew IsamSeparatedLongValueException;
        case JET_errMustBeSeparateLongValue:
            return gcnew IsamMustBeSeparateLongValueException;
        case JET_errInvalidPreread:
            return gcnew IsamInvalidPrereadException;
        case JET_errInvalidColumnReference:
            return gcnew IsamInvalidColumnReferenceException;
        case JET_errStaleColumnReference:
            return gcnew IsamStaleColumnReferenceException;
        case JET_errCompressionIntegrityCheckFailed:
            return gcnew IsamCompressionIntegrityCheckFailedException;
        case JET_errLogFileCorrupt:
            return gcnew IsamLogFileCorruptException;
        case JET_errNoBackupDirectory:
            return gcnew IsamNoBackupDirectoryException;
        case JET_errBackupDirectoryNotEmpty:
            return gcnew IsamBackupDirectoryNotEmptyException;
        case JET_errBackupInProgress:
            return gcnew IsamBackupInProgressException;
        case JET_errRestoreInProgress:
            return gcnew IsamRestoreInProgressException;
        case JET_errMissingPreviousLogFile:
            return gcnew IsamMissingPreviousLogFileException;
        case JET_errLogWriteFail:
            return gcnew IsamLogWriteFailException;
        case JET_errLogDisabledDueToRecoveryFailure:
            return gcnew IsamLogDisabledDueToRecoveryFailureException;
        case JET_errLogGenerationMismatch:
            return gcnew IsamLogGenerationMismatchException;
        case JET_errBadLogVersion:
            return gcnew IsamBadLogVersionException;
        case JET_errInvalidLogSequence:
            return gcnew IsamInvalidLogSequenceException;
        case JET_errLoggingDisabled:
            return gcnew IsamLoggingDisabledException;
        case JET_errLogSequenceEnd:
            return gcnew IsamLogSequenceEndException;
        case JET_errNoBackup:
            return gcnew IsamNoBackupException;
        case JET_errInvalidBackupSequence:
            return gcnew IsamInvalidBackupSequenceException;
        case JET_errBackupNotAllowedYet:
            return gcnew IsamBackupNotAllowedYetException;
        case JET_errDeleteBackupFileFail:
            return gcnew IsamDeleteBackupFileFailException;
        case JET_errMakeBackupDirectoryFail:
            return gcnew IsamMakeBackupDirectoryFailException;
        case JET_errInvalidBackup:
            return gcnew IsamInvalidBackupException;
        case JET_errRecoveredWithErrors:
            return gcnew IsamRecoveredWithErrorsException;
        case JET_errMissingLogFile:
            return gcnew IsamMissingLogFileException;
        case JET_errLogDiskFull:
            return gcnew IsamLogDiskFullException;
        case JET_errBadLogSignature:
            return gcnew IsamBadLogSignatureException;
        case JET_errBadCheckpointSignature:
            return gcnew IsamBadCheckpointSignatureException;
        case JET_errCheckpointCorrupt:
            return gcnew IsamCheckpointCorruptException;
        case JET_errBadPatchPage:
            return gcnew IsamBadPatchPageException;
        case JET_errRedoAbruptEnded:
            return gcnew IsamRedoAbruptEndedException;
        case JET_errDatabaseLogSetMismatch:
            return gcnew IsamDatabaseLogSetMismatchException;
        case JET_errLogFileSizeMismatch:
            return gcnew IsamLogFileSizeMismatchException;
        case JET_errCheckpointFileNotFound:
            return gcnew IsamCheckpointFileNotFoundException;
        case JET_errRequiredLogFilesMissing:
            return gcnew IsamRequiredLogFilesMissingException;
        case JET_errSoftRecoveryOnBackupDatabase:
            return gcnew IsamSoftRecoveryOnBackupDatabaseException;
        case JET_errLogFileSizeMismatchDatabasesConsistent:
            return gcnew IsamLogFileSizeMismatchDatabasesConsistentException;
        case JET_errLogSectorSizeMismatch:
            return gcnew IsamLogSectorSizeMismatchException;
        case JET_errLogSectorSizeMismatchDatabasesConsistent:
            return gcnew IsamLogSectorSizeMismatchDatabasesConsistentException;
        case JET_errLogSequenceEndDatabasesConsistent:
            return gcnew IsamLogSequenceEndDatabasesConsistentException;
        case JET_errDatabaseDirtyShutdown:
            return gcnew IsamDatabaseDirtyShutdownException;
        case JET_errConsistentTimeMismatch:
            return gcnew IsamConsistentTimeMismatchException;
        case JET_errEndingRestoreLogTooLow:
            return gcnew IsamEndingRestoreLogTooLowException;
        case JET_errStartingRestoreLogTooHigh:
            return gcnew IsamStartingRestoreLogTooHighException;
        case JET_errGivenLogFileHasBadSignature:
            return gcnew IsamGivenLogFileHasBadSignatureException;
        case JET_errGivenLogFileIsNotContiguous:
            return gcnew IsamGivenLogFileIsNotContiguousException;
        case JET_errMissingRestoreLogFiles:
            return gcnew IsamMissingRestoreLogFilesException;
        case JET_errMissingFullBackup:
            return gcnew IsamMissingFullBackupException;
        case JET_errDatabaseAlreadyUpgraded:
            return gcnew IsamDatabaseAlreadyUpgradedException;
        case JET_errDatabaseIncompleteUpgrade:
            return gcnew IsamDatabaseIncompleteUpgradeException;
        case JET_errMissingCurrentLogFiles:
            return gcnew IsamMissingCurrentLogFilesException;
        case JET_errDbTimeTooOld:
            return gcnew IsamDbTimeTooOldException;
        case JET_errDbTimeTooNew:
            return gcnew IsamDbTimeTooNewException;
        case JET_errMissingFileToBackup:
            return gcnew IsamMissingFileToBackupException;
        case JET_errLogTornWriteDuringHardRestore:
            return gcnew IsamLogTornWriteDuringHardRestoreException;
        case JET_errLogTornWriteDuringHardRecovery:
            return gcnew IsamLogTornWriteDuringHardRecoveryException;
        case JET_errLogCorruptDuringHardRestore:
            return gcnew IsamLogCorruptDuringHardRestoreException;
        case JET_errLogCorruptDuringHardRecovery:
            return gcnew IsamLogCorruptDuringHardRecoveryException;
        case JET_errBadRestoreTargetInstance:
            return gcnew IsamBadRestoreTargetInstanceException;
        case JET_errRecoveredWithoutUndo:
            return gcnew IsamRecoveredWithoutUndoException;
        case JET_errCommittedLogFilesMissing:
            return gcnew IsamCommittedLogFilesMissingException;
        case JET_errSectorSizeNotSupported:
            return gcnew IsamSectorSizeNotSupportedException;
        case JET_errRecoveredWithoutUndoDatabasesConsistent:
            return gcnew IsamRecoveredWithoutUndoDatabasesConsistentException;
        case JET_errCommittedLogFileCorrupt:
            return gcnew IsamCommittedLogFileCorruptException;
        case JET_errLogSequenceChecksumMismatch:
            return gcnew IsamLogSequenceChecksumMismatchException;
        case JET_errPageInitializedMismatch:
            return gcnew IsamPageInitializedMismatchException;
        case JET_errUnicodeTranslationFail:
            return gcnew IsamUnicodeTranslationFailException;
        case JET_errUnicodeNormalizationNotSupported:
            return gcnew IsamUnicodeNormalizationNotSupportedException;
        case JET_errUnicodeLanguageValidationFailure:
            return gcnew IsamUnicodeLanguageValidationFailureException;
        case JET_errExistingLogFileHasBadSignature:
            return gcnew IsamExistingLogFileHasBadSignatureException;
        case JET_errExistingLogFileIsNotContiguous:
            return gcnew IsamExistingLogFileIsNotContiguousException;
        case JET_errLogReadVerifyFailure:
            return gcnew IsamLogReadVerifyFailureException;
        case JET_errCheckpointDepthTooDeep:
            return gcnew IsamCheckpointDepthTooDeepException;
        case JET_errRestoreOfNonBackupDatabase:
            return gcnew IsamRestoreOfNonBackupDatabaseException;
        case JET_errLogFileNotCopied:
            return gcnew IsamLogFileNotCopiedException;
        case JET_errSurrogateBackupInProgress:
            return gcnew IsamSurrogateBackupInProgressException;
        case JET_errTransactionTooLong:
            return gcnew IsamTransactionTooLongException;
        case JET_errEngineFormatVersionNoLongerSupportedTooLow:
            return gcnew IsamEngineFormatVersionNoLongerSupportedTooLowException;
        case JET_errEngineFormatVersionNotYetImplementedTooHigh:
            return gcnew IsamEngineFormatVersionNotYetImplementedTooHighException;
        case JET_errEngineFormatVersionParamTooLowForRequestedFeature:
            return gcnew IsamEngineFormatVersionParamTooLowForRequestedFeatureException;
        case JET_errEngineFormatVersionSpecifiedTooLowForLogVersion:
            return gcnew IsamEngineFormatVersionSpecifiedTooLowForLogVersionException;
        case JET_errEngineFormatVersionSpecifiedTooLowForDatabaseVersion:
            return gcnew IsamEngineFormatVersionSpecifiedTooLowForDatabaseVersionException;
        case JET_errBackupAbortByServer:
            return gcnew IsamBackupAbortByServerException;
        case JET_errInvalidGrbit:
            return gcnew IsamInvalidGrbitException;
        case JET_errTermInProgress:
            return gcnew IsamTermInProgressException;
        case JET_errFeatureNotAvailable:
            return gcnew IsamFeatureNotAvailableException;
        case JET_errInvalidName:
            return gcnew IsamInvalidNameException;
        case JET_errInvalidParameter:
            return gcnew IsamInvalidParameterException;
        case JET_errDatabaseFileReadOnly:
            return gcnew IsamDatabaseFileReadOnlyException;
        case JET_errInvalidDatabaseId:
            return gcnew IsamInvalidDatabaseIdException;
        case JET_errOutOfMemory:
            return gcnew IsamOutOfMemoryException;
        case JET_errOutOfDatabaseSpace:
            return gcnew IsamOutOfDatabaseSpaceException;
        case JET_errOutOfCursors:
            return gcnew IsamOutOfCursorsException;
        case JET_errOutOfBuffers:
            return gcnew IsamOutOfBuffersException;
        case JET_errTooManyIndexes:
            return gcnew IsamTooManyIndexesException;
        case JET_errTooManyKeys:
            return gcnew IsamTooManyKeysException;
        case JET_errRecordDeleted:
            return gcnew IsamRecordDeletedException;
        case JET_errReadVerifyFailure:
            return gcnew IsamReadVerifyFailureException;
        case JET_errPageNotInitialized:
            return gcnew IsamPageNotInitializedException;
        case JET_errOutOfFileHandles:
            return gcnew IsamOutOfFileHandlesException;
        case JET_errDiskReadVerificationFailure:
            return gcnew IsamDiskReadVerificationFailureException;
        case JET_errDiskIO:
            return gcnew IsamDiskIOException;
        case JET_errInvalidPath:
            return gcnew IsamInvalidPathException;
        case JET_errRecordTooBig:
            return gcnew IsamRecordTooBigException;
        case JET_errInvalidDatabase:
            return gcnew IsamInvalidDatabaseException;
        case JET_errNotInitialized:
            return gcnew IsamNotInitializedException;
        case JET_errAlreadyInitialized:
            return gcnew IsamAlreadyInitializedException;
        case JET_errInitInProgress:
            return gcnew IsamInitInProgressException;
        case JET_errFileAccessDenied:
            return gcnew IsamFileAccessDeniedException;
        case JET_errBufferTooSmall:
            return gcnew IsamBufferTooSmallException;
        case JET_errTooManyColumns:
            return gcnew IsamTooManyColumnsException;
        case JET_errInvalidBookmark:
            return gcnew IsamInvalidBookmarkException;
        case JET_errColumnInUse:
            return gcnew IsamColumnInUseException;
        case JET_errInvalidBufferSize:
            return gcnew IsamInvalidBufferSizeException;
        case JET_errColumnNotUpdatable:
            return gcnew IsamColumnNotUpdatableException;
        case JET_errIndexInUse:
            return gcnew IsamIndexInUseException;
        case JET_errNullKeyDisallowed:
            return gcnew IsamNullKeyDisallowedException;
        case JET_errNotInTransaction:
            return gcnew IsamNotInTransactionException;
        case JET_errMustRollback:
            return gcnew IsamMustRollbackException;
        case JET_errTooManyActiveUsers:
            return gcnew IsamTooManyActiveUsersException;
        case JET_errInvalidLanguageId:
            return gcnew IsamInvalidLanguageIdException;
        case JET_errInvalidCodePage:
            return gcnew IsamInvalidCodePageException;
        case JET_errInvalidLCMapStringFlags:
            return gcnew IsamInvalidLCMapStringFlagsException;
        case JET_errVersionStoreOutOfMemoryAndCleanupTimedOut:
            return gcnew IsamVersionStoreOutOfMemoryAndCleanupTimedOutException;
        case JET_errVersionStoreOutOfMemory:
            return gcnew IsamVersionStoreOutOfMemoryException;
        case JET_errCannotIndex:
            return gcnew IsamCannotIndexException;
        case JET_errRecordNotDeleted:
            return gcnew IsamRecordNotDeletedException;
        case JET_errTooManyMempoolEntries:
            return gcnew IsamTooManyMempoolEntriesException;
        case JET_errOutOfObjectIDs:
            return gcnew IsamOutOfObjectIDsException;
        case JET_errOutOfLongValueIDs:
            return gcnew IsamOutOfLongValueIDsException;
        case JET_errOutOfAutoincrementValues:
            return gcnew IsamOutOfAutoincrementValuesException;
        case JET_errOutOfDbtimeValues:
            return gcnew IsamOutOfDbtimeValuesException;
        case JET_errOutOfSequentialIndexValues:
            return gcnew IsamOutOfSequentialIndexValuesException;
        case JET_errRunningInOneInstanceMode:
            return gcnew IsamRunningInOneInstanceModeException;
        case JET_errRunningInMultiInstanceMode:
            return gcnew IsamRunningInMultiInstanceModeException;
        case JET_errSystemParamsAlreadySet:
            return gcnew IsamSystemParamsAlreadySetException;
        case JET_errSystemPathInUse:
            return gcnew IsamSystemPathInUseException;
        case JET_errLogFilePathInUse:
            return gcnew IsamLogFilePathInUseException;
        case JET_errTempPathInUse:
            return gcnew IsamTempPathInUseException;
        case JET_errInstanceNameInUse:
            return gcnew IsamInstanceNameInUseException;
        case JET_errSystemParameterConflict:
            return gcnew IsamSystemParameterConflictException;
        case JET_errInstanceUnavailable:
            return gcnew IsamInstanceUnavailableException;
        case JET_errInstanceUnavailableDueToFatalLogDiskFull:
            return gcnew IsamInstanceUnavailableDueToFatalLogDiskFullException;
        case JET_errInvalidSesparamId:
            return gcnew IsamInvalidSesparamIdException;
        case JET_errTooManyRecords:
            return gcnew IsamTooManyRecordsException;
        case JET_errInvalidDbparamId:
            return gcnew IsamInvalidDbparamIdException;
        case JET_errOutOfSessions:
            return gcnew IsamOutOfSessionsException;
        case JET_errWriteConflict:
            return gcnew IsamWriteConflictException;
        case JET_errTransTooDeep:
            return gcnew IsamTransTooDeepException;
        case JET_errInvalidSesid:
            return gcnew IsamInvalidSesidException;
        case JET_errWriteConflictPrimaryIndex:
            return gcnew IsamWriteConflictPrimaryIndexException;
        case JET_errInTransaction:
            return gcnew IsamInTransactionException;
        case JET_errTransReadOnly:
            return gcnew IsamTransReadOnlyException;
        case JET_errSessionWriteConflict:
            return gcnew IsamSessionWriteConflictException;
        case JET_errRecordTooBigForBackwardCompatibility:
            return gcnew IsamRecordTooBigForBackwardCompatibilityException;
        case JET_errCannotMaterializeForwardOnlySort:
            return gcnew IsamCannotMaterializeForwardOnlySortException;
        case JET_errSesidTableIdMismatch:
            return gcnew IsamSesidTableIdMismatchException;
        case JET_errInvalidInstance:
            return gcnew IsamInvalidInstanceException;
        case JET_errDirtyShutdown:
            return gcnew IsamDirtyShutdownException;
        case JET_errReadPgnoVerifyFailure:
            return gcnew IsamReadPgnoVerifyFailureException;
        case JET_errReadLostFlushVerifyFailure:
            return gcnew IsamReadLostFlushVerifyFailureException;
        case JET_errFileSystemCorruption:
            return gcnew IsamFileSystemCorruptionException;
        case JET_errRecoveryVerifyFailure:
            return gcnew IsamRecoveryVerifyFailureException;
        case JET_errFilteredMoveNotSupported:
            return gcnew IsamFilteredMoveNotSupportedException;
        case JET_errDatabaseDuplicate:
            return gcnew IsamDatabaseDuplicateException;
        case JET_errDatabaseInUse:
            return gcnew IsamDatabaseInUseException;
        case JET_errDatabaseNotFound:
            return gcnew IsamDatabaseNotFoundException;
        case JET_errDatabaseInvalidName:
            return gcnew IsamDatabaseInvalidNameException;
        case JET_errDatabaseInvalidPages:
            return gcnew IsamDatabaseInvalidPagesException;
        case JET_errDatabaseCorrupted:
            return gcnew IsamDatabaseCorruptedException;
        case JET_errDatabaseLocked:
            return gcnew IsamDatabaseLockedException;
        case JET_errCannotDisableVersioning:
            return gcnew IsamCannotDisableVersioningException;
        case JET_errInvalidDatabaseVersion:
            return gcnew IsamInvalidDatabaseVersionException;
        case JET_errPageSizeMismatch:
            return gcnew IsamPageSizeMismatchException;
        case JET_errTooManyInstances:
            return gcnew IsamTooManyInstancesException;
        case JET_errDatabaseSharingViolation:
            return gcnew IsamDatabaseSharingViolationException;
        case JET_errAttachedDatabaseMismatch:
            return gcnew IsamAttachedDatabaseMismatchException;
        case JET_errDatabaseInvalidPath:
            return gcnew IsamDatabaseInvalidPathException;
        case JET_errForceDetachNotAllowed:
            return gcnew IsamForceDetachNotAllowedException;
        case JET_errCatalogCorrupted:
            return gcnew IsamCatalogCorruptedException;
        case JET_errPartiallyAttachedDB:
            return gcnew IsamPartiallyAttachedDBException;
        case JET_errDatabaseSignInUse:
            return gcnew IsamDatabaseSignInUseException;
        case JET_errDatabaseCorruptedNoRepair:
            return gcnew IsamDatabaseCorruptedNoRepairException;
        case JET_errInvalidCreateDbVersion:
            return gcnew IsamInvalidCreateDbVersionException;
        case JET_errDatabaseIncompleteIncrementalReseed:
            return gcnew IsamDatabaseIncompleteIncrementalReseedException;
        case JET_errDatabaseInvalidIncrementalReseed:
            return gcnew IsamDatabaseInvalidIncrementalReseedException;
        case JET_errDatabaseFailedIncrementalReseed:
            return gcnew IsamDatabaseFailedIncrementalReseedException;
        case JET_errNoAttachmentsFailedIncrementalReseed:
            return gcnew IsamNoAttachmentsFailedIncrementalReseedException;
        case JET_errDatabaseNotReady:
            return gcnew IsamDatabaseNotReadyException;
        case JET_errDatabaseAttachedForRecovery:
            return gcnew IsamDatabaseAttachedForRecoveryException;
        case JET_errTransactionsNotReadyDuringRecovery:
            return gcnew IsamTransactionsNotReadyDuringRecoveryException;
        case JET_errTableLocked:
            return gcnew IsamTableLockedException;
        case JET_errTableDuplicate:
            return gcnew IsamTableDuplicateException;
        case JET_errTableInUse:
            return gcnew IsamTableInUseException;
        case JET_errObjectNotFound:
            return gcnew IsamObjectNotFoundException;
        case JET_errDensityInvalid:
            return gcnew IsamDensityInvalidException;
        case JET_errTableNotEmpty:
            return gcnew IsamTableNotEmptyException;
        case JET_errInvalidTableId:
            return gcnew IsamInvalidTableIdException;
        case JET_errTooManyOpenTables:
            return gcnew IsamTooManyOpenTablesException;
        case JET_errIllegalOperation:
            return gcnew IsamIllegalOperationException;
        case JET_errTooManyOpenTablesAndCleanupTimedOut:
            return gcnew IsamTooManyOpenTablesAndCleanupTimedOutException;
        case JET_errCannotDeleteTempTable:
            return gcnew IsamCannotDeleteTempTableException;
        case JET_errCannotDeleteSystemTable:
            return gcnew IsamCannotDeleteSystemTableException;
        case JET_errCannotDeleteTemplateTable:
            return gcnew IsamCannotDeleteTemplateTableException;
        case JET_errExclusiveTableLockRequired:
            return gcnew IsamExclusiveTableLockRequiredException;
        case JET_errFixedDDL:
            return gcnew IsamFixedDDLException;
        case JET_errFixedInheritedDDL:
            return gcnew IsamFixedInheritedDDLException;
        case JET_errCannotNestDDL:
            return gcnew IsamCannotNestDDLException;
        case JET_errDDLNotInheritable:
            return gcnew IsamDDLNotInheritableException;
        case JET_errInvalidSettings:
            return gcnew IsamInvalidSettingsException;
        case JET_errClientRequestToStopJetService:
            return gcnew IsamClientRequestToStopJetServiceException;
        case JET_errIndexHasPrimary:
            return gcnew IsamIndexHasPrimaryException;
        case JET_errIndexDuplicate:
            return gcnew IsamIndexDuplicateException;
        case JET_errIndexNotFound:
            return gcnew IsamIndexNotFoundException;
        case JET_errIndexMustStay:
            return gcnew IsamIndexMustStayException;
        case JET_errIndexInvalidDef:
            return gcnew IsamIndexInvalidDefException;
        case JET_errInvalidCreateIndex:
            return gcnew IsamInvalidCreateIndexException;
        case JET_errTooManyOpenIndexes:
            return gcnew IsamTooManyOpenIndexesException;
        case JET_errMultiValuedIndexViolation:
            return gcnew IsamMultiValuedIndexViolationException;
        case JET_errIndexBuildCorrupted:
            return gcnew IsamIndexBuildCorruptedException;
        case JET_errPrimaryIndexCorrupted:
            return gcnew IsamPrimaryIndexCorruptedException;
        case JET_errSecondaryIndexCorrupted:
            return gcnew IsamSecondaryIndexCorruptedException;
        case JET_errInvalidIndexId:
            return gcnew IsamInvalidIndexIdException;
        case JET_errIndexTuplesSecondaryIndexOnly:
            return gcnew IsamIndexTuplesSecondaryIndexOnlyException;
        case JET_errIndexTuplesNonUniqueOnly:
            return gcnew IsamIndexTuplesNonUniqueOnlyException;
        case JET_errIndexTuplesTextBinaryColumnsOnly:
            return gcnew IsamIndexTuplesTextBinaryColumnsOnlyException;
        case JET_errIndexTuplesVarSegMacNotAllowed:
            return gcnew IsamIndexTuplesVarSegMacNotAllowedException;
        case JET_errIndexTuplesInvalidLimits:
            return gcnew IsamIndexTuplesInvalidLimitsException;
        case JET_errIndexTuplesCannotRetrieveFromIndex:
            return gcnew IsamIndexTuplesCannotRetrieveFromIndexException;
        case JET_errIndexTuplesKeyTooSmall:
            return gcnew IsamIndexTuplesKeyTooSmallException;
        case JET_errInvalidLVChunkSize:
            return gcnew IsamInvalidLVChunkSizeException;
        case JET_errColumnCannotBeEncrypted:
            return gcnew IsamColumnCannotBeEncryptedException;
        case JET_errCannotIndexOnEncryptedColumn:
            return gcnew IsamCannotIndexOnEncryptedColumnException;
        case JET_errColumnNoChunk:
            return gcnew IsamColumnNoChunkException;
        case JET_errColumnDoesNotFit:
            return gcnew IsamColumnDoesNotFitException;
        case JET_errNullInvalid:
            return gcnew IsamNullInvalidException;
        case JET_errColumnTooBig:
            return gcnew IsamColumnTooBigException;
        case JET_errColumnNotFound:
            return gcnew IsamColumnNotFoundException;
        case JET_errColumnDuplicate:
            return gcnew IsamColumnDuplicateException;
        case JET_errMultiValuedColumnMustBeTagged:
            return gcnew IsamMultiValuedColumnMustBeTaggedException;
        case JET_errColumnRedundant:
            return gcnew IsamColumnRedundantException;
        case JET_errInvalidColumnType:
            return gcnew IsamInvalidColumnTypeException;
        case JET_errNoCurrentIndex:
            return gcnew IsamNoCurrentIndexException;
        case JET_errKeyIsMade:
            return gcnew IsamKeyIsMadeException;
        case JET_errBadColumnId:
            return gcnew IsamBadColumnIdException;
        case JET_errBadItagSequence:
            return gcnew IsamBadItagSequenceException;
        case JET_errCannotBeTagged:
            return gcnew IsamCannotBeTaggedException;
        case JET_errDefaultValueTooBig:
            return gcnew IsamDefaultValueTooBigException;
        case JET_errMultiValuedDuplicate:
            return gcnew IsamMultiValuedDuplicateException;
        case JET_errLVCorrupted:
            return gcnew IsamLVCorruptedException;
        case JET_errMultiValuedDuplicateAfterTruncation:
            return gcnew IsamMultiValuedDuplicateAfterTruncationException;
        case JET_errDerivedColumnCorruption:
            return gcnew IsamDerivedColumnCorruptionException;
        case JET_errInvalidPlaceholderColumn:
            return gcnew IsamInvalidPlaceholderColumnException;
        case JET_errColumnCannotBeCompressed:
            return gcnew IsamColumnCannotBeCompressedException;
        case JET_errColumnNoEncryptionKey:
            return gcnew IsamColumnNoEncryptionKeyException;
        case JET_errRecordNotFound:
            return gcnew IsamRecordNotFoundException;
        case JET_errRecordNoCopy:
            return gcnew IsamRecordNoCopyException;
        case JET_errNoCurrentRecord:
            return gcnew IsamNoCurrentRecordException;
        case JET_errRecordPrimaryChanged:
            return gcnew IsamRecordPrimaryChangedException;
        case JET_errKeyDuplicate:
            return gcnew IsamKeyDuplicateException;
        case JET_errAlreadyPrepared:
            return gcnew IsamAlreadyPreparedException;
        case JET_errKeyNotMade:
            return gcnew IsamKeyNotMadeException;
        case JET_errUpdateNotPrepared:
            return gcnew IsamUpdateNotPreparedException;
        case JET_errDecompressionFailed:
            return gcnew IsamDecompressionFailedException;
        case JET_errUpdateMustVersion:
            return gcnew IsamUpdateMustVersionException;
        case JET_errDecryptionFailed:
            return gcnew IsamDecryptionFailedException;
        case JET_errEncryptionBadItag:
            return gcnew IsamEncryptionBadItagException;
        case JET_errTooManySorts:
            return gcnew IsamTooManySortsException;
        case JET_errTooManyAttachedDatabases:
            return gcnew IsamTooManyAttachedDatabasesException;
        case JET_errDiskFull:
            return gcnew IsamDiskFullException;
        case JET_errPermissionDenied:
            return gcnew IsamPermissionDeniedException;
        case JET_errFileNotFound:
            return gcnew IsamFileNotFoundException;
        case JET_errFileInvalidType:
            return gcnew IsamFileInvalidTypeException;
        case JET_errFileAlreadyExists:
            return gcnew IsamFileAlreadyExistsException;
        case JET_errAfterInitialization:
            return gcnew IsamAfterInitializationException;
        case JET_errLogCorrupted:
            return gcnew IsamLogCorruptedException;
        case JET_errInvalidOperation:
            return gcnew IsamInvalidOperationException;
        case JET_errSessionSharingViolation:
            return gcnew IsamSessionSharingViolationException;
        case JET_errEntryPointNotFound:
            return gcnew IsamEntryPointNotFoundException;
        case JET_errSessionContextAlreadySet:
            return gcnew IsamSessionContextAlreadySetException;
        case JET_errSessionContextNotSetByThisThread:
            return gcnew IsamSessionContextNotSetByThisThreadException;
        case JET_errSessionInUse:
            return gcnew IsamSessionInUseException;
        case JET_errRecordFormatConversionFailed:
            return gcnew IsamRecordFormatConversionFailedException;
        case JET_errOneDatabasePerSession:
            return gcnew IsamOneDatabasePerSessionException;
        case JET_errRollbackError:
            return gcnew IsamRollbackErrorException;
        case JET_errFlushMapVersionUnsupported:
            return gcnew IsamFlushMapVersionUnsupportedException;
        case JET_errFlushMapDatabaseMismatch:
            return gcnew IsamFlushMapDatabaseMismatchException;
        case JET_errFlushMapUnrecoverable:
            return gcnew IsamFlushMapUnrecoverableException;
        case JET_errRBSFileCorrupt:
            return gcnew IsamRBSFileCorruptException;
        case JET_errRBSHeaderCorrupt:
            return gcnew IsamRBSHeaderCorruptException;
        case JET_errRBSDbMismatch:
            return gcnew IsamRBSDbMismatchException;
        case JET_errBadRBSVersion:
            return gcnew IsamBadRBSVersionException;
        case JET_errOutOfRBSSpace:
            return gcnew IsamOutOfRBSSpaceException;
        case JET_errRBSInvalidSign:
            return gcnew IsamRBSInvalidSignException;
        case JET_errRBSInvalidRecord:
            return gcnew IsamRBSInvalidRecordException;
        case JET_errRBSRCInvalidRBS:
            return gcnew IsamRBSRCInvalidRBSException;
        case JET_errRBSRCNoRBSFound:
            return gcnew IsamRBSRCNoRBSFoundException;
        case JET_errRBSRCBadDbState:
            return gcnew IsamRBSRCBadDbStateException;
        case JET_errRBSMissingReqLogs:
            return gcnew IsamRBSMissingReqLogsException;
        case JET_errRBSLogDivergenceFailed:
            return gcnew IsamRBSLogDivergenceFailedException;
        case JET_errRBSRCCopyLogsRevertState:
            return gcnew IsamRBSRCCopyLogsRevertStateException;
        case JET_errDatabaseIncompleteRevert:
            return gcnew IsamDatabaseIncompleteRevertException;
        case JET_errRBSRCRevertCancelled:
            return gcnew IsamRBSRCRevertCancelledException;
        case JET_errRBSRCInvalidDbFormatVersion:
            return gcnew IsamRBSRCInvalidDbFormatVersionException;
        case JET_errRBSCannotDetermineDivergence:
            return gcnew IsamRBSCannotDetermineDivergenceException;
        case JET_errDatabaseAlreadyRunningMaintenance:
            return gcnew IsamDatabaseAlreadyRunningMaintenanceException;
        case JET_errCallbackFailed:
            return gcnew IsamCallbackFailedException;
        case JET_errCallbackNotResolved:
            return gcnew IsamCallbackNotResolvedException;
        case JET_errSpaceHintsInvalid:
            return gcnew IsamSpaceHintsInvalidException;
        case JET_errOSSnapshotInvalidSequence:
            return gcnew IsamOSSnapshotInvalidSequenceException;
        case JET_errOSSnapshotTimeOut:
            return gcnew IsamOSSnapshotTimeOutException;
        case JET_errOSSnapshotNotAllowed:
            return gcnew IsamOSSnapshotNotAllowedException;
        case JET_errOSSnapshotInvalidSnapId:
            return gcnew IsamOSSnapshotInvalidSnapIdException;
        case JET_errTooManyTestInjections:
            return gcnew IsamTooManyTestInjectionsException;
        case JET_errTestInjectionNotSupported:
            return gcnew IsamTestInjectionNotSupportedException;
        case JET_errInvalidLogDataSequence:
            return gcnew IsamInvalidLogDataSequenceException;
        case JET_errLSCallbackNotSpecified:
            return gcnew IsamLSCallbackNotSpecifiedException;
        case JET_errLSAlreadySet:
            return gcnew IsamLSAlreadySetException;
        case JET_errLSNotSet:
            return gcnew IsamLSNotSetException;
        case JET_errFileIOBeyondEOF:
            return gcnew IsamFileIOBeyondEOFException;
        case JET_errFileCompressed:
            return gcnew IsamFileCompressedException;
        default:
            return gcnew IsamErrorException( L"Unknown error", err );
        }
    }
};

}
}
}
