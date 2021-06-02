// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <eseback2.h>
#include <jet.h>
#include <stdio.h>

using namespace System;
using namespace System::Runtime::InteropServices;
using namespace System::Text;
using namespace System::Collections;
using namespace System::Reflection;
using namespace System::Threading;
using namespace System::Diagnostics;

#include "coltyps.h"
#include "warnings.h"
#include "params.h"
#include "grbits.h"
#include "objtyps.h"
#include "exceptions.h"
#include "structs.h"

namespace Microsoft
{
namespace Exchange
{
namespace Isam
{
    // Unmanaged function pointer prototypes.
    typedef JET_ERR (*PfnSimpleTrace)(const char * const sz);

    // Managed delegates that match the unmanaged prototypes.
    private delegate JET_ERR PrepareInstanceForBackupDelegate(
        PESEBACK_CONTEXT    pBackupContext,
        JET_INSTANCE        ulInstanceId,
        void *              pvReserved);
    private delegate JET_ERR DoneWithInstanceForBackupDelegate(
        PESEBACK_CONTEXT    pBackupContext,
        JET_INSTANCE        ulInstanceId,
        unsigned long       fComplete,
        void *              pvReserved);
    private delegate JET_ERR GetDatabasesInfoDelegate(
        PESEBACK_CONTEXT        pBackupContext,
        unsigned long *         pcInfo,
        INSTANCE_BACKUP_INFO ** prgInfo,
        unsigned long           fReserved);
    private delegate JET_ERR FreeDatabasesInfoDelegate(
        PESEBACK_CONTEXT        pBackupContext,
        unsigned long           cInfo,
        INSTANCE_BACKUP_INFO *  rgInfo);
    private delegate JET_ERR IsSGReplicatedDelegate(
        PESEBACK_CONTEXT    pContext,
        JET_INSTANCE        jetinst,
        BOOL *              pfReplicated,
        ULONG               cbSGGuid,
        WCHAR *             wszSGGuid,
        ULONG *             pcInfo,
        LOGSHIP_INFO **     prgInfo);
    private delegate JET_ERR FreeShipLogInfoDelegate(
        ULONG           pcInfo,
        LOGSHIP_INFO *  prgInfo);
    private delegate JET_ERR ServerAccessCheckDelegate();
    private delegate JET_ERR TraceDelegate(const char * const szData);

    // A managed version of the flags to MINSTANCE_BACKUP_INFO
    MSINTERNAL enum class DatabaseBackupInfoFlags
    {
        MOUNTED = 0x10,
    };

    // A managed version of the DATABASE_BACKUP_INFO structure.
    MSINTERNAL value struct MDATABASE_BACKUP_INFO
    {
        property String^ Name;
        property String^ Path;
        property Guid Guid;
        property DatabaseBackupInfoFlags Flags;
    };

    // A managed version of the INSTANCE_BACKUP_INFO structure
    MSINTERNAL value struct MINSTANCE_BACKUP_INFO
    {
        property MJET_INSTANCE Instance;
        property String^ Name;
        property array<MDATABASE_BACKUP_INFO>^ Databases;
    };

    // Enumerations of values for MLOGSHIP_INFO.Type
    MSINTERNAL enum class ESE_LOGSHIP
    {
        STANDBY = 0x1,
        CLUSTER = 0x2,
        LOCAL = 0x4,
    };

    // A managed version of the LOGSHIP_INFO structure
    MSINTERNAL value struct MLOGSHIP_INFO
    {
        property ESE_LOGSHIP Type;
        property String^ Name;
    };

    // This interface describes the methods that a client has to implement
    // to support ESEBACK callbacks. A object implementing this interface
    // is passed in when ESEBACK is registered.
    MSINTERNAL interface class IEsebackCallbacks
    {
        Int32 PrepareInstanceForBackup(IntPtr context, MJET_INSTANCE instance, IntPtr reserved);
        Int32 DoneWithInstanceForBackup(IntPtr context, MJET_INSTANCE instance, UInt32 complete, IntPtr reserved);
        Int32 GetDatabasesInfo(
            IntPtr context,
            [System::Runtime::InteropServices::OutAttribute] array<MINSTANCE_BACKUP_INFO>^% instances,
            UInt32 reserved);
        Int32 IsSGReplicated(
            IntPtr context,
            MJET_INSTANCE instance,
            [System::Runtime::InteropServices::OutAttribute] Boolean% isReplicated,
            [System::Runtime::InteropServices::OutAttribute] Guid% guid,
            [System::Runtime::InteropServices::OutAttribute] array<MLOGSHIP_INFO>^% info);
        Int32 ServerAccessCheck();
        Int32 Trace(String^ data);
    };

    // Holds a reference to the IEsebackCallbacks object that is being
    // used for callbacks. This is used by the client-facing Eseback
    // class (to register/unregister) and the ESE-facing InteropCallbacks
    // class (for callbacks).
    private ref class EsebackCallbacks abstract sealed
    {
    public:
        static property IEsebackCallbacks^ ManagedCallbacks;
    };

    // Methods that convert unmanaged structures to managed, or free the memory
    // allocated by unmanaged structures.
    private ref class StructConversion abstract sealed
    {
    private:
        // Allocate an unmanaged array the same size as the managed array.
        // This function allocates memory. Free with FreeUnmanagedArray.
        // The array is zero-initialized.
        template<class T> static T * AllocateUnmanagedArray(Array^ const managed)
        {
            T* const rgT = new T[managed->Length];
            if (nullptr == rgT)
            {
                throw gcnew OutOfMemoryException("Failed to allocate unmanaged array");
            }

            memset(rgT, 0, sizeof(T) * managed->Length);
            return rgT;
        }

        // Frees an array allocated by AllocateUnmanagedArray.
        template<class T> static void FreeUnmanagedArray(T * const rgT)
        {
            if (rgT)
            {
                delete[] rgT;
            }
        }

        // Given an MDATABASE_BACKUP_INFO, return its unmanaged equivalent.
        // This function allocates memory. Free with FreeUnmanagedDatabaseBackupInfo.
        static DATABASE_BACKUP_INFO MakeUnmanagedDatabaseBackupInfo(MDATABASE_BACKUP_INFO managed)
        {
            DATABASE_BACKUP_INFO databaseInfo;
            databaseInfo.wszDatabaseDisplayName = WszFromString(managed.Name);

            // The database stream in DATABASE_BACKUP_INFO is of the form
            //  <EDB file>\0<SLV file>\0\0
            // There is no SLV file so the form is effectively:
            //  <EDB file>\0\0\0
            // The length of a managed string does not include a null terminator
            // so we manually add all of them here.
            String^ const streams = String::Format("{0}\0\0\0", managed.Path);
            databaseInfo.cwDatabaseStreams = streams->Length;
            databaseInfo.wszDatabaseStreams = WszFromString(streams);

            databaseInfo.guidDatabase = GetUnmanagedGuid(managed.Guid);
            databaseInfo.ulIconIndexDatabase = 0;
            databaseInfo.fDatabaseFlags = (UInt32)managed.Flags;

            return databaseInfo;
        }

        // Frees a DATABASE_BACKUP_INFO created by MakeUnmanagedDatabaseBackupInfo.
        static void FreeUnmanagedDatabaseBackupInfo(DATABASE_BACKUP_INFO databaseInfo)
        {
            FreeWsz(databaseInfo.wszDatabaseDisplayName);
            FreeWsz(databaseInfo.wszDatabaseStreams);
        }

        // Given an array of MDATABASE_BACKUP_INFO, return the unmanaged equivalent.
        // This function allocates memory. Free with FreeUnmanagedDatabaseBackupInfos.
        static DATABASE_BACKUP_INFO * MakeUnmanagedDatabaseBackupInfos(array<MDATABASE_BACKUP_INFO>^ const databases)
        {
            DATABASE_BACKUP_INFO* const rgDatabaseInfo = AllocateUnmanagedArray<DATABASE_BACKUP_INFO>(databases);
            for (int iDatabaseInfo = 0; iDatabaseInfo < databases->Length; ++iDatabaseInfo)
            {
                rgDatabaseInfo[iDatabaseInfo] = MakeUnmanagedDatabaseBackupInfo(databases[iDatabaseInfo]);
            }
            return rgDatabaseInfo;
        }

        // Frees an array of DATABASE_BACKUP_INFO created by MakeUnmanagedDatabaseBackupInfos.
        static void FreeUnmanagedDatabaseBackupInfos(const int cInfo, DATABASE_BACKUP_INFO * const rgDatabaseInfo)
        {
            if (rgDatabaseInfo)
            {
                for (int iDatabaseInfo = 0; iDatabaseInfo < cInfo; ++iDatabaseInfo)
                {
                    FreeUnmanagedDatabaseBackupInfo(rgDatabaseInfo[iDatabaseInfo]);
                }
                FreeUnmanagedArray(rgDatabaseInfo);
            }
        }

        // Given an MINSTANCE_BACKUP_INFO, return its unmanaged equivalent.
        // This function allocates memory. Free with FreeUnmanagedBackupInfo.
        static INSTANCE_BACKUP_INFO MakeUnmanagedBackupInfo(MINSTANCE_BACKUP_INFO managed)
        {
            INSTANCE_BACKUP_INFO instanceInfo;
            instanceInfo.hInstanceId = managed.Instance.NativeValue;
            instanceInfo.wszInstanceName = WszFromString(managed.Name);
            instanceInfo.ulIconIndexInstance = 0;
            instanceInfo.cDatabase = managed.Databases->Length;
            instanceInfo.rgDatabase = MakeUnmanagedDatabaseBackupInfos(managed.Databases);
            instanceInfo.cIconDescription = 0;
            instanceInfo.rgIconDescription = nullptr;
            return instanceInfo;
        }

        // Frees an INSTANCE_BACKUP_INFO allocated by MakeUnmanagedBackupInfo.
        static void FreeUnmanagedBackupInfo(INSTANCE_BACKUP_INFO instanceInfo)
        {
            FreeWsz(instanceInfo.wszInstanceName);
            FreeUnmanagedDatabaseBackupInfos(instanceInfo.cDatabase, instanceInfo.rgDatabase);
        }

        // Given an MLOGSHIP_INFO, return its unmanaged equivalent.
        // This function allocates memory. Free with FreeUnmanagedLogshipInfo.
        static LOGSHIP_INFO MakeUnmanagedLogshipInfo(MLOGSHIP_INFO managed)
        {
            LOGSHIP_INFO logshipInfo;
            logshipInfo.ulType = (UInt32)managed.Type;
            logshipInfo.cchName = managed.Name->Length;
            logshipInfo.wszName = WszFromString(managed.Name);
            return logshipInfo;
        }

        // Frees an LOGSHIP_INFO allocated by MakeUnmanagedLogshipInfo.
        static void FreeUnmanagedLogshipInfo(LOGSHIP_INFO logshipInfo)
        {
            FreeWsz(logshipInfo.wszName);
        }

    public:
        // Take a native string and return a native wchar_t*.
        // This function allocates memory. Free with FreeWsz.
        static wchar_t* WszFromString(String^ const s)
        {
            return static_cast<wchar_t*>(Marshal::StringToHGlobalUni(s).ToPointer());
        }

        // Free a string allocated by WszFromString.
        static void FreeWsz(wchar_t * const wsz)
        {
            if (wsz)
            {
                Marshal::FreeHGlobal(IntPtr(wsz));
            }
        }

        // Take a managed System::Guid and return a native GUID.
        static GUID GetUnmanagedGuid(Guid managed)
        {
            GUID guid;
            pin_ptr<void> pv = &managed;
            memcpy(&guid, pv, sizeof(guid));
            return guid;
        }

        // Given an array of MINSTANCE_BACKUP_INFO, return the unmanaged equivalent.
        // This function allocates memory. Free with FreeUnmanagedBackupInfos.
        static INSTANCE_BACKUP_INFO * MakeUnmanagedBackupInfos(array<MINSTANCE_BACKUP_INFO>^ const instances)
        {
            const int cInfo = instances->Length;
            INSTANCE_BACKUP_INFO* rgInfo = nullptr;
            try
            {
                rgInfo = AllocateUnmanagedArray<INSTANCE_BACKUP_INFO>(instances);
                for (int iInfo = 0; iInfo < cInfo; ++iInfo)
                {
                    rgInfo[iInfo] = MakeUnmanagedBackupInfo(instances[iInfo]);
                }
                return rgInfo;
            }
            catch (...)
            {
                FreeUnmanagedBackupInfos(cInfo, rgInfo);
                throw;
            }
        }

        // Frees an array of INSTANCE_BACKUP_INFO allocated by MakeUnmanagedBackupInfos.
        static void FreeUnmanagedBackupInfos(const int cInfo, INSTANCE_BACKUP_INFO * const rgInfo)
        {
            if (rgInfo)
            {
                for (int iInfo = 0; iInfo < cInfo; ++iInfo)
                {
                    FreeUnmanagedBackupInfo(rgInfo[iInfo]);
                }
                FreeUnmanagedArray(rgInfo);
            }
        }

        // Given an array of MLOGSHIP_INFO, return the unmanaged equivalent.
        // This function allocates memory. Free with FreeUnmanagedLogshipInfos.
        static LOGSHIP_INFO * MakeUnmanagedLogshipInfos(array<MLOGSHIP_INFO>^ const infos)
        {
            const int cInfo = infos->Length;
            LOGSHIP_INFO* rgInfo = nullptr;
            try
            {
                rgInfo = AllocateUnmanagedArray<LOGSHIP_INFO>(infos);
                for (int iInfo = 0; iInfo < cInfo; ++iInfo)
                {
                    rgInfo[iInfo] = MakeUnmanagedLogshipInfo(infos[iInfo]);
                }
                return rgInfo;
            }
            catch (...)
            {
                FreeUnmanagedLogshipInfos(cInfo, rgInfo);
                throw;
            }
        }

        // Frees an array of LOGSHIP_INFO allocated by MakeUnmanagedLogshipInfos.
        static void FreeUnmanagedLogshipInfos(const int cInfo, LOGSHIP_INFO * const rgInfo)
        {
            if (rgInfo)
            {
                for (int iInfo = 0; iInfo < cInfo; ++iInfo)
                {
                    FreeUnmanagedLogshipInfo(rgInfo[iInfo]);
                }
                FreeUnmanagedArray(rgInfo);
            }
        }
    };

    // Callback interop class. The methods in this class are called by
    // ESEBACK, using native functions generated by Marshal::GetFunctionPointerForDelegate.
    // The methods then convert the data to managed form and then call the
    // managed callbacks held in the EsebackCallbacks class. The function
    // pointers are generated in ths Init method.
    class InteropCallbacks
    {
    public:
        static JET_ERR PrepareInstanceForBackup(
            PESEBACK_CONTEXT    pBackupContext,
            JET_INSTANCE        ulInstanceId,
            void *              pvReserved)
        {
            IntPtr context(pBackupContext);
            MJET_INSTANCE instance;
            instance.NativeValue = static_cast<Int64>(ulInstanceId);
            IntPtr reserved(pvReserved);
            return EsebackCallbacks::ManagedCallbacks->PrepareInstanceForBackup(context, instance, reserved);
        }

        static JET_ERR DoneWithInstanceForBackup(
            PESEBACK_CONTEXT    pBackupContext,
            JET_INSTANCE        ulInstanceId,
            unsigned long       fComplete,
            void *              pvReserved)
        {
            IntPtr context(pBackupContext);
            MJET_INSTANCE instance;
            instance.NativeValue = static_cast<Int64>(ulInstanceId);
            IntPtr reserved(pvReserved);
            return EsebackCallbacks::ManagedCallbacks->DoneWithInstanceForBackup(context, instance, fComplete, reserved);
        }

        static JET_ERR GetDatabasesInfo(
            PESEBACK_CONTEXT            pBackupContext,
            unsigned long               * pcInfo,
            INSTANCE_BACKUP_INFO        ** prgInfo,
            unsigned long               fReserved)
        {
            IntPtr context(pBackupContext);
            array<MINSTANCE_BACKUP_INFO>^ instances = nullptr;
            JET_ERR err = EsebackCallbacks::ManagedCallbacks->GetDatabasesInfo(context, instances, fReserved);
            *pcInfo = instances->Length;
            *prgInfo = StructConversion::MakeUnmanagedBackupInfos(instances);
            return err;
        }

        static JET_ERR FreeDatabasesInfo(
            PESEBACK_CONTEXT            /* pBackupContext */,
            unsigned long               cInfo,
            INSTANCE_BACKUP_INFO *      rgInfo)
        {
            StructConversion::FreeUnmanagedBackupInfos(cInfo, rgInfo);
            return 0;
        }

        static JET_ERR IsSGReplicated(
            PESEBACK_CONTEXT    pContext,
            JET_INSTANCE        jetinst,
            BOOL *              pfReplicated,
            ULONG               cbSGGuid,
            WCHAR *             wszSGGuid,
            ULONG *             pcInfo,
            LOGSHIP_INFO **     prgInfo)
        {
            IntPtr context(pContext);
            MJET_INSTANCE instance;
            instance.NativeValue = static_cast<Int64>(jetinst);
            Boolean isReplicated;
            Guid guid;
            array<MLOGSHIP_INFO>^ infos = nullptr;
            
            JET_ERR err = EsebackCallbacks::ManagedCallbacks->IsSGReplicated(context, instance, isReplicated, guid, infos);
            *pfReplicated = isReplicated;
            if (nullptr != infos)
            {
                *pcInfo = infos->Length;
                *prgInfo = StructConversion::MakeUnmanagedLogshipInfos(infos);

                if (nullptr != wszSGGuid)
                {
                    wchar_t * szT = nullptr;
                    try
                    {
                        // Fortunately Guid::ToString() matches the format we want
                        szT = StructConversion::WszFromString(guid.ToString());
                        wcscpy_s(wszSGGuid, cbSGGuid / sizeof(WCHAR), szT);
                    }
                    __finally
                    {
                        StructConversion::FreeWsz(szT);
                    }
                }
            }
            else
            {
                *pcInfo = 0;
                *prgInfo = nullptr;
            }

            return err;
        }

        static JET_ERR FreeShipLogInfo(
            ULONG           cInfo,
            LOGSHIP_INFO *  rgInfo)
        {
            StructConversion::FreeUnmanagedLogshipInfos(cInfo, rgInfo);
            return 0;
        }

        static JET_ERR ServerAccessCheck()
        {
            return EsebackCallbacks::ManagedCallbacks->ServerAccessCheck();
        }

        static JET_ERR Trace(const char* const szData)
        {
            String^ data = gcnew String(szData);
            return EsebackCallbacks::ManagedCallbacks->Trace(data);
        }
    };

#pragma unmanaged
    // This is a purely native class that implements the trace callback. This is
    // needed because there is no native/managed interop for varargs functions.
    // Instead we sprintf the data into a buffer and then call an interop trace
    // method that takes a string.
    class NativeCallbacks
    {
    public:
        static JET_ERR ErrESECBTrace( const char* const szFormat, ... )
        {
            char szTraceBuffer[4096];
            va_list args;
            va_start(args, szFormat);

            (void)_vsnprintf_s( szTraceBuffer, _countof(szTraceBuffer), _TRUNCATE, szFormat, args);
            va_end( args );
            return InteropCallbacks::Trace(szTraceBuffer);
        }
    };
#pragma managed

    // Provides methods to register and unregister ESEBACK.
    MSINTERNAL ref class Eseback abstract sealed
    {
    public:
        [FlagsAttribute]
        enum class Flags : UInt32
        {
            BACKUP_DATA_TRANSFER_SHAREDMEM = ESE_BACKUP_DATA_TRANSFER_SHAREDMEM,
            BACKUP_DATA_TRANSFER_SOCKETS = ESE_BACKUP_DATA_TRANSFER_SOCKETS,
            REGISTER_BACKUP_LOCAL = ESE_REGISTER_BACKUP_LOCAL,
            REGISTER_BACKUP_REMOTE = ESE_REGISTER_BACKUP_REMOTE,
            REGISTER_ONLINE_RESTORE_LOCAL = ESE_REGISTER_ONLINE_RESTORE_LOCAL,
            REGISTER_ONLINE_RESTORE_REMOTE = ESE_REGISTER_ONLINE_RESTORE_REMOTE,
            REGISTER_SURROGATE_BACKUP_LOCAL = ESE_REGISTER_SURROGATE_BACKUP_LOCAL,
            REGISTER_SURROGATE_BACKUP_REMOTE = ESE_REGISTER_SURROGATE_BACKUP_REMOTE,
            REGISTER_LOGSHIP_SEED_LOCAL = ESE_REGISTER_LOGSHIP_SEED_LOCAL,
            REGISTER_LOGSHIP_SEED_REMOTE = ESE_REGISTER_LOGSHIP_SEED_REMOTE,
            REGISTER_LOGSHIP_UPDATER_LOCAL = ESE_REGISTER_LOGSHIP_UPDATER_LOCAL,
            REGISTER_LOGSHIP_UPDATER_REMOTE = ESE_REGISTER_LOGSHIP_UPDATER_REMOTE,
            REGISTER_EXCHANGE_SERVER_LOCAL = ESE_REGISTER_EXCHANGE_SERVER_LOCAL,
            REGISTER_EXCHANGE_SERVER_REMOTE = ESE_REGISTER_EXCHANGE_SERVER_REMOTE,
        };

        static Int32 BackupRestoreRegister(
            String^ displayName,
            Flags flags,
            String^ endpointAnnotation,
            IEsebackCallbacks^ callbacks,
            Guid crimsonPublisher)
        {
            EsebackCallbacks::ManagedCallbacks = callbacks;
            ESEBACK_CALLBACKS esebackCallbacks;
            esebackCallbacks.pfnPrepareInstance = InteropCallbacks::PrepareInstanceForBackup;
            esebackCallbacks.pfnDoneWithInstance = InteropCallbacks::DoneWithInstanceForBackup;
            esebackCallbacks.pfnGetDatabasesInfo = InteropCallbacks::GetDatabasesInfo;
            esebackCallbacks.pfnFreeDatabasesInfo = InteropCallbacks::FreeDatabasesInfo;
            esebackCallbacks.pfnIsSGReplicated = InteropCallbacks::IsSGReplicated;
            esebackCallbacks.pfnFreeShipLogInfo = InteropCallbacks::FreeShipLogInfo;
            esebackCallbacks.pfnServerAccessCheck = InteropCallbacks::ServerAccessCheck;
            esebackCallbacks.pfnTrace = NativeCallbacks::ErrESECBTrace;

            GUID guid = StructConversion::GetUnmanagedGuid(crimsonPublisher);
            wchar_t* wszDisplayName = nullptr;
            wchar_t* wszEndpointAnnotation = nullptr;
            try
            {
                wszDisplayName = StructConversion::WszFromString(displayName);
                wszEndpointAnnotation = StructConversion::WszFromString(endpointAnnotation);
                return HrESEBackupRestoreRegister2(
                    wszDisplayName,
                    (UInt32)flags,
                    wszEndpointAnnotation,
                    &esebackCallbacks,
                    &guid);
            }
            __finally
            {
                StructConversion::FreeWsz(wszDisplayName);
                StructConversion::FreeWsz(wszEndpointAnnotation);
            }
        }

        static Int32 BackupRestoreUnregister()
        {
            Int32 result = HrESEBackupRestoreUnregister();
            EsebackCallbacks::ManagedCallbacks = nullptr;
            return result;
        }
    };
}
}
}
