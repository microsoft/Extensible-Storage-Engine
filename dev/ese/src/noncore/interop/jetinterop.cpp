// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#if defined( JET_UNICODE )
#if !defined( UNICODE )
#define UNICODE
#endif
#if !defined( _UNICODE )
#define _UNICODE
#endif
#else
#undef UNICODE
#undef _UNICODE
#endif

#include <windows.h>
#include <winnls.h>
#include <string.h>
#include <stdlib.h>
#include <tchar.h>

#if (ESENT_WINXP)
#undef JET_VERSION
#define JET_VERSION 0x0a01
#else
#endif
#if (ESENT_PUBLICONLY)
#include "esent.h"
#else
#ifdef ESENT
#include "esent_x.h"
#else
#include "jet.h"
#endif
#endif

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
#include "move.h"
#include "resource.h"
#include "assert.h"
#include "exception.h"
#include "privconsts.h"

//  Interop layer, this presents the 'C' JET API as a managed class with a lot of static members.
//  As a coding convention, non managed variables start with '_'

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

#pragma unmanaged

#ifdef JET_UNICODE

#define GetUnmanagedString  GetUnmanagedStringW
#define FreeUnmanagedString FreeUnmanagedStringW
#define MakeManagedString   MakeManagedStringW

#else
#define GetUnmanagedString  GetUnmanagedStringA
#define FreeUnmanagedString FreeUnmanagedStringA
#define MakeManagedString   MakeManagedStringA

#endif

    //  ================================================================
    class FastReallocBuffer
        //  ================================================================
    {
    public:

        FastReallocBuffer( const size_t cb, BYTE* const rgb );

        void* Realloc( void* const pv, const ULONG cb );
        static void* JET_API Realloc_( FastReallocBuffer* const pfrb, void* const pv, const ULONG cb );

        void FreeEnumeratedColumns( ULONG&              cEnumColumn,
            ::JET_ENUMCOLUMN*&  rgEnumColumn );

    private:

        BYTE*   m_pbMin;
        BYTE*   m_pbMax;
        BYTE*   m_pb;
    };

    //  ================================================================
    inline FastReallocBuffer::FastReallocBuffer( const size_t cb, BYTE* const rgb )
        //  ================================================================
    {
        m_pbMin = rgb;
        m_pbMax = rgb + cb;
        m_pb    = m_pbMin;
    }

    //  ================================================================
    inline void* FastReallocBuffer::Realloc( void* const pv, const ULONG cb )
        //  ================================================================
    {
        void*           pvAlloc = NULL;
        const size_t    cbAlign = sizeof( void* );
        size_t          cbAlloc = cb + cbAlign - 1;
        cbAlloc = cbAlloc - cbAlloc % cbAlign;
        // Block integer overflow from causing damage.
        if (cbAlloc < cb)
        {
            return pvAlloc; // == NULL
        }

        if ( !pv )
        {
            if ( (size_t)(m_pbMax - m_pb) >= cbAlloc )
            {
                pvAlloc = m_pb;
                m_pb += cbAlloc;
            }
            else
            {
                pvAlloc = new BYTE[ cb ];
            }
        }
        else if ( !cbAlloc )
        {
            if ( m_pbMin <= pv && pv <= m_pbMax )
            {
                //  nop:  slab allocs are not freed until FreeEnumeratedColumns() is called
            }
            else
            {
                delete [] pv;
            }
        }
        else
        {
            //  nyi:  realloc
        }

        return pvAlloc;
    }

    //  ================================================================
    void* JET_API FastReallocBuffer::Realloc_( FastReallocBuffer* const pfrb, void* const pv, const ULONG cb )
        //  ================================================================
    {
        return pfrb->Realloc( pv, cb );
    }

    //  ================================================================
    void FastReallocBuffer::FreeEnumeratedColumns( ULONG& cEnumColumn, ::JET_ENUMCOLUMN*& rgEnumColumn )
        //  ================================================================
    {
        size_t                  iEnumColumn         = 0;
        ::JET_ENUMCOLUMN*       pEnumColumn         = NULL;
        size_t                  iEnumColumnValue    = 0;
        ::JET_ENUMCOLUMNVALUE*  pEnumColumnValue    = NULL;

        if ( rgEnumColumn )
        {
            for ( iEnumColumn = 0; iEnumColumn < cEnumColumn; iEnumColumn++ )
            {
                pEnumColumn = rgEnumColumn + iEnumColumn;

                if ( pEnumColumn->err != JET_wrnColumnSingleValue )
                {
                    if ( pEnumColumn->rgEnumColumnValue )
                    {
                        for (   iEnumColumnValue = 0;
                            iEnumColumnValue < pEnumColumn->cEnumColumnValue;
                            iEnumColumnValue++ )
                        {
                            pEnumColumnValue = pEnumColumn->rgEnumColumnValue + iEnumColumnValue;

                            if ( pEnumColumn->pvData )
                            {
                                Realloc( pEnumColumnValue->pvData, 0 );
                            }
                        }

                        Realloc( pEnumColumn->rgEnumColumnValue, 0 );
                    }
                }
                else
                {
                    if ( pEnumColumn->pvData )
                    {
                        Realloc( pEnumColumn->pvData, 0 );
                    }
                }
            }

            Realloc( rgEnumColumn, 0 );
        }

        m_pb = m_pbMin;

        cEnumColumn     = 0;
        rgEnumColumn    = NULL;
    }

#pragma managed

#ifdef INTERNALUSE
    /// <summary>
    /// Interface that needs to be implemented in order
    //  to get callback notifications during recovery.
    //  There are 2 callbacks: one indicating progress (Progress)
    //  the other to drive individual log replay (RecoveryControl).
    //  The RecoveryControl should return MJET_RECOVERY_RETVALS.Default
    //  in order to continue recovery without any special treatment.
    /// </summary>
    MSINTERNAL interface class MJetInitCallback
    {
        MJET_RECOVERY_RETVALS RecoveryControl( MJET_RECOVERY_CONTROL^ snrecovery );
        void PagePatch( MJET_SNPATCHREQUEST snpatch );
        void CorruptedPage( int pageNumber );
        void Progress( MJET_SNT snt, MJET_SNPROG snprog );
    };
#endif

#if (ESENT_WINXP)
#else
    /// <summary>
    /// Given an integer, representing a status notification type, return a MJET_SNT
    /// </summary>
    static MJET_SNT Snt( ::JET_SNT _snt )
    {
        switch( _snt )
        {
        case JET_sntProgress:
            return MJET_SNT::Progress;
        case JET_sntFail:
            return MJET_SNT::Fail;
        case JET_sntBegin:
            return MJET_SNT::Begin;
        case JET_sntComplete:
            return MJET_SNT::Complete;
#if (ESENT_PUBLICONLY)
#else
        case JET_sntPagePatchRequest:
            return MJET_SNT::PagePatchRequest;
        case JET_sntCorruptedPage:
            return MJET_SNT::CorruptedPage;

        case JET_sntOpenLog:
            return MJET_SNT::OpenLog;
        case JET_sntOpenCheckpoint:
            return MJET_SNT::OpenCheckpoint;
        case JET_sntMissingLog:
            return MJET_SNT::MissingLog;
        case JET_sntBeginUndo:
            return MJET_SNT::BeginUndo;
        case JET_sntNotificationEvent:
            return MJET_SNT::NotificationEvent;
        case JET_sntSignalErrorCondition:
            return MJET_SNT::SignalErrorCondition;

        case JET_sntAttachedDb:
            return MJET_SNT::AttachedDb;
        case JET_sntDetachingDb:
            return MJET_SNT::DetachingDb;

        case JET_sntCommitCtx:
            return MJET_SNT::CommitCtx;

#endif
        default:
            // UNDONE:
            // Assert( 0 );
            throw gcnew IsamInvalidParameterException();
        }
    }

    /// <summary>
    /// Given an integer, representing a status notification process, return a MJET_SNP
    /// </summary>
    static MJET_SNP Snp( ::JET_SNP _snp )
    {
        switch( _snp )
        {
        case JET_snpRepair:
            return MJET_SNP::Repair;
        case JET_snpCompact:
            return MJET_SNP::Compact;
        case JET_snpRestore:
            return MJET_SNP::Restore;
        case JET_snpBackup:
            return MJET_SNP::Backup;
        case JET_snpUpgrade:
            return MJET_SNP::Upgrade;
        case JET_snpScrub:
            return MJET_SNP::Scrub;
        case JET_snpUpgradeRecordFormat:
            return MJET_SNP::UpgradeRecordFormat;
        case JET_snpRecoveryControl:
            return MJET_SNP::RecoveryControl;
        case JET_snpExternalAutoHealing:
            return MJET_SNP::ExternalAutoHealing;

        default:
            // UNDONE:
            // Assert( 0 );
            throw gcnew IsamInvalidParameterException();
        }
    }
#endif

#ifdef INTERNALUSE
    ref class JetInterop;
    private ref class MJetInitCallbackInfo
    {
    public:
        MJetInitCallbackInfo( JetInterop^ interop, MJetInitCallback^ UserCallback )
        {
            m_UserCallback = UserCallback;
            m_interop = interop;
        }

        MJetInitCallback^   GetUserCallback()                   { return m_UserCallback; }
        void                SetException( Exception^ e )        { m_Exception = e; }
        Exception^          GetException( )                     { return m_Exception; }

#if (ESENT_WINXP)
#else
        JET_ERR MJetInitCallbackOwn(
            ::JET_SESID /*_sesid*/,
            ::JET_SNP _snp,
            ::JET_SNT _snt,
            void * _pv);
#endif

    private:
        JetInterop^             m_interop;
        MJetInitCallback^       m_UserCallback;
        Exception^              m_Exception;
    };
#endif

    // <summary>
    // Delegate type for callbacks that are supplied by the mamaged code which calls online defragmentation
    // </summary>
    MSINTERNAL delegate void MJetOnEndOnlineDefragCallbackDelegate();

    // <summary>
    // Contains info about one managed defragmentation callback and its associated GCHandle
    // used to prevent the garbage collector from relocating the managed delegate.
    // Once passed as argument to an unmanaged function, the gc handle cannot be freed
    // before the callback has been called.
    // </summary>
    private ref class MJetDefragCallbackInfo
    {
    private:
        // User-supplied callback
        MJetOnEndOnlineDefragCallbackDelegate^ m_callback;

        // GC Handle needed to pin the delegate until the callback is called
        GCHandle m_gcHandle;

    public:
        // Constructor
        MJetDefragCallbackInfo(MJetOnEndOnlineDefragCallbackDelegate^ callback)
            : m_callback(callback)
        {
        }

        // Retrieve the user-supplied callback
        MJetOnEndOnlineDefragCallbackDelegate^ GetUserCallback() { return this->m_callback; }

        // Retrieve the gc handle
        GCHandle GetGCHandle() { return this->m_gcHandle; }

        // Set the gc handle
        void SetGCHandle(GCHandle gcHandle) { this->m_gcHandle = gcHandle; }

        // <summary>
        // Managed callback with unmanaged parameters that will be called
        // at the end of the defrag task by unmanaged code
        // </summary>
        JET_ERR MJetOnEndOLDCallbackOwn(
            ::JET_SESID     /*sesid*/,
            ::JET_DBID      /*dbid*/,
            ::JET_TABLEID   /*tableid*/,
            ::JET_CBTYP     /*cbtyp*/,
            void *          /*pvArg1*/,
            void *          /*pvArg2*/,
            void *          /*pvContext*/,
            ::JET_API_PTR   /*ulUnused*/)
        {
            JET_ERR err = JET_errSuccess;

            if (this->m_callback != nullptr)
            {
                try
                {
                    this->m_callback();
                }
                catch (IsamOutOfMemoryException^)
                {
                    err = JET_errOutOfMemory;
                }
                finally
                {
                    this->m_gcHandle.Free();
                }
            }

            return err;
        }
    };

    // <summary>
    // Internal managed delegate that is passed to the unmanaged functions which take a JET_CALLBACK function pointer
    // </summary>
    MSINTERNAL delegate JET_ERR MJET_CALLBACK(
        ::JET_SESID     sesid,
        ::JET_DBID      dbid,
        ::JET_TABLEID   tableid,
        ::JET_CBTYP     cbtyp,
        void *          pvArg1,
        void *          pvArg2,
        void *          pvContext,
        ::JET_API_PTR   ulUnused);

    MSINTERNAL delegate JET_ERR MJetInitStatusCallbackManagedDelegate( ::JET_SESID _sesid, ::JET_SNP _snp, ::JET_SNT _snt, void * _pv );

    /// <summary>
    /// Delegate type used by <seealso cref="IJetInterop"/> to specify a custom exception translator.
    /// </summary>
    MSINTERNAL delegate Exception^ ExceptionTranslator(IsamErrorException^);

    MSINTERNAL interface class IJetInteropCommon
    {
        /// <summary>
        /// Enable Jet to throw exceptions that are meaningful to Jet's consumers.
        /// Thus, rather than catching the Jet exception, wrapping it and re-throwing, the consumer
        /// simply tells Jet what to throw.
        /// </summary>
        /// <param name="translator">The function to use to translate a Jet exception into an exception useful to the consumner.
        /// Pass null to reset to the default behavior of throwing the Jet exception.</param>
        void SetExceptionTranslator(ExceptionTranslator^ translator);

        /// <summary>
        /// Initialize Jet, returning a new instance or throwing an exception
        /// </summary>
        MJET_INSTANCE MJetInit();

        /// <summary>
        /// Initialize Jet, returning a new instance or throwing an exception
        /// </summary>
        MJET_INSTANCE MJetInit(MJET_INSTANCE instance);

        /// <summary>
        /// Initialize Jet, returning a new instance or throwing an exception
        /// </summary>
        MJET_INSTANCE MJetInit(MJET_INSTANCE instance, MJET_GRBIT grbit);

        /// <summary>
        /// Create a new instance, returning the instance or throwing an exception
        /// </summary>
        MJET_INSTANCE MJetCreateInstance(String^ name);

        /// <summary>
        /// Create a new instance, returning the instance or throwing an exception
        /// </summary>
        MJET_INSTANCE MJetCreateInstance(
            String^ name,
            String^ displayName,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        MJET_INSTANCE MJetCreateInstance(String^ name, String^ displayName);

        /// <summary>
        /// Terminate an instance
        /// </summary>
        void MJetTerm();

        /// <summary>
        /// Terminate an instance
        /// </summary>
        void MJetTerm(MJET_INSTANCE instance);

        /// <summary>
        /// Terminate an instance
        /// </summary>
        void MJetTerm(MJET_INSTANCE instance, MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        void MJetStopService();

        /// <summary>
        /// </summary>
        void MJetStopServiceInstance(MJET_INSTANCE instance);

        /// <summary>
        /// </summary>
        void MJetStopBackup();

        /// <summary>
        /// </summary>
        void MJetStopBackupInstance(MJET_INSTANCE instance);

        /// <summary>
        /// </summary>
        void MJetSetSystemParameter(
            MJET_INSTANCE instance,
            MJET_SESID sesid,
            MJET_PARAM paramid,
            Int64 param,
            String^ pstr);

        /// <summary>
        /// </summary>
        void MJetSetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            Int64 param);

        /// <summary>
        /// </summary>
        void MJetSetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            String^ pstr);

        /// <summary>
        /// </summary>
        void MJetSetSystemParameter(MJET_PARAM paramid, Int64 param);

        /// <summary>
        /// </summary>
        void MJetSetSystemParameter(MJET_PARAM paramid, String^ pstr);

        /// <summary>
        /// </summary>
        void MJetGetSystemParameter(
            MJET_INSTANCE instance,
            MJET_SESID sesid,
            MJET_PARAM paramid,
            Int64% param,
            String^% pstr);

        /// <summary>
        /// </summary>
        void MJetGetSystemParameter(
            MJET_INSTANCE instance,
            MJET_SESID sesid,
            MJET_PARAM paramid,
            Int64% param);

        /// <summary>
        /// </summary>
        void MJetGetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            Int64% param);

        /// <summary>
        /// </summary>
        void MJetGetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            String^% pstr);

        /// <summary>
        /// </summary>
        void MJetGetSystemParameter(MJET_PARAM paramid, Int64% param);

        /// <summary>
        /// </summary>
        void MJetGetSystemParameter(MJET_PARAM paramid, String^% pstr);

        /// <summary>
        /// </summary>
        MJET_SESID MJetBeginSession(
            MJET_INSTANCE instance,
            String^ user,
            String^ password);

        /// <summary>
        /// </summary>
        MJET_SESID MJetBeginSession(MJET_INSTANCE instance);

        /// <summary>
        /// </summary>
        MJET_SESID MJetDupSession(MJET_SESID sesid);

        /// <summary>
        /// </summary>
        void MJetEndSession(MJET_SESID sesid, MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        void MJetEndSession(MJET_SESID sesid);

        /// <summary>
        /// </summary>
        Int64 MJetGetVersion(MJET_SESID sesid);

        /// <summary>
        /// </summary>
        MJET_WRN MJetIdle(MJET_SESID sesid, MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        MJET_DBID MJetCreateDatabase(
            MJET_SESID sesid,
            String^ file,
            String^ connect,
            MJET_GRBIT grbit,
            MJET_WRN% wrn);

        /// <summary>
        /// </summary>
        MJET_DBID MJetCreateDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit,
            MJET_WRN% wrn);

        /// <summary>
        /// </summary>
        MJET_DBID MJetCreateDatabase(MJET_SESID sesid, String^ file);

        /// <summary>
        /// </summary>
        MJET_DBID MJetCreateDatabase(
            MJET_SESID sesid,
            String^ file,
            Int64 maxPages,
            MJET_GRBIT grbit,
            MJET_WRN% wrn);

        /// <summary>
        /// </summary>
        MJET_WRN MJetAttachDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        MJET_WRN MJetAttachDatabase(MJET_SESID sesid, String^ file);

        /// <summary>
        /// </summary>
        MJET_WRN MJetAttachDatabase(
            MJET_SESID sesid,
            String^ file,
            Int64 maxPages,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        MJET_WRN MJetDetachDatabase(MJET_SESID sesid, String^ file);

        /// <summary>
        /// </summary>
        MJET_WRN MJetDetachDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        MJET_OBJECTINFO MJetGetTableInfo(MJET_TABLEID tableid);

        /// <summary>
        /// </summary>
        String^ MJetGetTableInfoName(MJET_TABLEID tableid);

        /// <summary>
        /// </summary>
        int MJetGetTableInfoInitialSize(MJET_TABLEID tableid);

        /// <summary>
        /// </summary>
        int MJetGetTableInfoDensity(MJET_TABLEID tableid);

        /// <summary>
        /// </summary>
        String^ MJetGetTableInfoTemplateTable(MJET_TABLEID tableid);

        /// <summary>
        /// </summary>
        int MJetGetTableInfoOwnedPages(MJET_TABLEID tableid);

        /// <summary>
        /// </summary>
        int MJetGetTableInfoAvailablePages(MJET_TABLEID tableid);

        /// <summary>
        /// </summary>
        MJET_OBJECTINFO MJetGetObjectInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ object);

        /// <summary>
        /// </summary>
        MJET_OBJECTINFO MJetGetObjectInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_OBJTYP objtyp,
            String^ container,
            String^ object);

        /// <summary>
        /// </summary>
        MJET_OBJECTLIST MJetGetObjectInfoList(MJET_SESID sesid, MJET_DBID dbid);

        /// <summary>
        /// </summary>
        MJET_OBJECTLIST MJetGetObjectInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_OBJTYP objtyp);

        /// <summary>
        /// </summary>
        MJET_OBJECTLIST MJetGetObjectInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_OBJTYP objtyp,
            String^ container);

        /// <summary>
        /// </summary>
        MJET_OBJECTLIST MJetGetObjectInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_OBJTYP objtyp,
            String^ container,
            String^ object);

        /// <summary>
        /// Given the name of a checkpoint file, return the generation of the checkpoint
        /// TODO: replace this with a call to an exported ESE function.
        /// </summary>
        Int64 MJetGetCheckpointFileInfo(String^ checkpoint);

        /// <summary>
        /// Given the name of a database file, return information about the database in a MJET_DBINFO struct.
        /// </summary>
        MJET_DBINFO MJetGetDatabaseFileInfo(String^ database);

        /// <summary>
        /// </summary>
        MJET_TABLEID MJetCreateTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name,
            Int32 pages,
            Int32 density);

        /// <summary>
        /// </summary>
        MJET_TABLEID MJetCreateTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name);

        /// <summary>
        /// </summary>
        void MJetCreateTableColumnIndex(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_TABLECREATE tablecreate);

        /// <summary>
        /// </summary>
        void MJetDeleteTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name);

        /// <summary>
        /// </summary>
        void MJetRenameTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ oldName,
            String^ newName);

        /// <summary>
        /// </summary>
        MJET_COLUMNBASE MJetGetColumnInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            Int64 colid);

        /// <summary>
        /// </summary>
        MJET_COLUMNBASE MJetGetColumnInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            MJET_COLUMNID columnid);

        /// <summary>
        /// </summary>
        MJET_COLUMNBASE MJetGetColumnInfo(MJET_TABLEID tableid, String^ column);

        /// <summary>
        /// </summary>
        MJET_COLUMNBASE MJetGetColumnInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ column);

        /// <summary>
        /// </summary>
        MJET_COLUMNLIST MJetGetColumnInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            Boolean sortByColumnid);

        /// <summary>
        /// </summary>
        MJET_COLUMNLIST MJetGetColumnInfoList(MJET_TABLEID tableid, Boolean sortByColumnid);

        /// <summary>
        /// </summary>
        MJET_COLUMNLIST MJetGetColumnInfoList(MJET_TABLEID tableid);

        /// <summary>
        /// </summary>
        MJET_COLUMNID MJetAddColumn(
            MJET_TABLEID tableid,
            String^ name,
            MJET_COLUMNDEF% definition,
            array<Byte>^ defaultValue);

        /// <summary>
        /// </summary>
        MJET_COLUMNID MJetAddColumn(
            MJET_TABLEID tableid,
            String^ name,
            MJET_COLUMNDEF% definition);

        /// <summary>
        /// </summary>
        void MJetDeleteColumn(MJET_TABLEID tableid, String^ name);

        /// <summary>
        /// </summary>
        void MJetDeleteColumn(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        void MJetRenameColumn(
            MJET_TABLEID tableid,
            String^ oldName,
            String^ newName,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        void MJetSetColumnDefaultValue(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ column,
            array<Byte>^ defaultValue,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        MJET_INDEXLIST MJetGetIndexInfo(MJET_TABLEID tableid, String^ index);

        /// <summary>
        /// </summary>
        MJET_INDEXLIST MJetGetIndexInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index);

        /// <summary>
        /// </summary>
        MJET_INDEXLIST MJetGetIndexInfoList(MJET_TABLEID tableid);

        /// <summary>
        /// </summary>
        MJET_INDEXLIST MJetGetIndexInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table);

        /// <summary>
        /// </summary>
        int MJetGetIndexInfoDensity(MJET_TABLEID tableid, String^ index);

        /// <summary>
        /// </summary>
        int MJetGetIndexInfoDensity(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index);

        /// <summary>
        /// </summary>
        int MJetGetIndexInfoLCID(MJET_TABLEID tableid, String^ index);

        /// <summary>
        /// </summary>
        int MJetGetIndexInfoLCID(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index);

        /// <summary>
        /// </summary>
        int MJetGetIndexInfoKeyMost(MJET_TABLEID tableid, String^ index);

        /// <summary>
        /// </summary>
        int MJetGetIndexInfoKeyMost(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index);

        /// <summary>
        /// </summary>
        void MJetCreateIndex(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit,
            String^ key,
            Int32 density);

        /// <summary>
        /// </summary>
        void MJetCreateIndex(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit,
            String^ key);

        /// <summary>
        /// </summary>
        void MJetCreateIndex(
            MJET_TABLEID tableid,
            String^ name,
            String^ key);

        void MJetCreateIndex(MJET_TABLEID tableid, array<MJET_INDEXCREATE>^ indexcreates);

        /// <summary>
        /// </summary>
        void MJetDeleteIndex(MJET_TABLEID tableid, String^ name);

        /// <summary>
        /// </summary>
        void MJetBeginTransaction(MJET_SESID sesid);

        /// <summary>
        /// </summary>
        void MJetBeginTransaction(MJET_SESID sesid, MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        void MJetBeginTransaction(
            MJET_SESID sesid,
            Int32 trxid,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        void MJetCommitTransaction(MJET_SESID sesid, MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        void MJetCommitTransaction(MJET_SESID sesid);

        /// <summary>
        /// </summary>
        void MJetRollback(MJET_SESID sesid, MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        void MJetRollback(MJET_SESID sesid);

        /// <summary>
        /// </summary>
        void MJetGetDatabaseInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            array<Byte>^ resultBuffer,
            MJET_DBINFO_LEVEL InfoLevel);

        /// <summary>
        /// </summary>
        MJET_DBINFO MJetGetDatabaseInfo(
            MJET_SESID sesid,
            MJET_DBID dbid);

        /// <summary>
        /// </summary>
        Int64 MJetGetDatabaseInfoFilesize(
            MJET_SESID sesid,
            MJET_DBID dbid);

        /// <summary>
        /// </summary>
        String^ MJetGetDatabaseInfoFilename(
            MJET_SESID sesid,
            MJET_DBID dbid);

        /// <summary>
        /// </summary>
        MJET_DBID MJetOpenDatabase(
            MJET_SESID sesid,
            String^ file,
            String^ connect,
            MJET_GRBIT grbit,
            MJET_WRN% wrn);

        /// <summary>
        /// </summary>
        MJET_DBID MJetOpenDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit,
            MJET_WRN% wrn);

        /// <summary>
        /// </summary>
        MJET_DBID MJetOpenDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        MJET_DBID MJetOpenDatabase(MJET_SESID sesid, String^ file);

        /// <summary>
        /// </summary>
        void MJetCloseDatabase(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        void MJetCloseDatabase(MJET_SESID sesid, MJET_DBID dbid);

        /// <summary>
        /// </summary>
        MJET_TABLEID MJetOpenTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name,
            array<Byte>^ parameters,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        MJET_TABLEID MJetOpenTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        MJET_TABLEID MJetOpenTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name);

        /// <summary>
        /// </summary>
        Boolean TryOpenTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name,
            MJET_TABLEID% tableid);

        /// <summary>
        /// </summary>
        void MJetSetTableSequential(MJET_TABLEID tableid, MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        void MJetResetTableSequential(MJET_TABLEID tableid, MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        void MJetCloseTable(MJET_TABLEID tableid);

        /// <summary>
        /// </summary>
        void MJetDelete(MJET_TABLEID tableid);

        /// <summary>
        /// </summary>
        array<Byte>^ MJetUpdate(MJET_TABLEID tableid);

        /// <summary>
        /// Returns NULL if the column is null, otherwise returns the number of bytes read into the buffer
        /// </summary>
        Nullable<Int64> MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit,
            MJET_RETINFO retinfo,
            ArraySegment<Byte> buffer);

        /// <summary>
        /// Returns NULL if the column is null, returns a zero-length array for a zero-length column
        /// </summary>
        array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit,
            MJET_RETINFO retinfo,
            Int64 maxdata);

        /// <summary>
        /// </summary>
        array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit,
            MJET_RETINFO retinfo);

        /// <summary>
        /// </summary>
        array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit);

        /// <summary>
        /// Returns NULL if the column is null, returns a zere-length array for a zero-length column
        /// </summary>
        array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit,
            Int64 maxdata);

        /// <summary>
        /// </summary>
        array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            Int64 maxdata);

        /// <summary>
        /// </summary>
        array<Byte>^ MJetRetrieveColumn(MJET_TABLEID tableid, MJET_COLUMNID columnid);

        /// <summary>
        /// </summary>
        Int64 MJetRetrieveColumnSize(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            Int64 itagSequence,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        Int64 MJetRetrieveColumnSize(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            Int64 itagSequence);

        /// <summary>
        /// </summary>
        Int64 MJetRetrieveColumnSize(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        Int64 MJetRetrieveColumnSize(MJET_TABLEID tableid, MJET_COLUMNID columnid);

        /// <summary>
        /// </summary>
        array<MJET_ENUMCOLUMN>^ MJetEnumerateColumns(
            MJET_TABLEID tableid,
            array<MJET_ENUMCOLUMNID>^ columnids,
            MJET_GRBIT grbit,
            MJET_WRN& wrn);

        /// <summary>
        /// </summary>
        array<MJET_ENUMCOLUMN>^ MJetEnumerateColumns(
            MJET_TABLEID tableid,
            array<MJET_ENUMCOLUMNID>^ columnids,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        array<MJET_ENUMCOLUMN>^ MJetEnumerateColumns(MJET_TABLEID tableid, array<MJET_ENUMCOLUMNID>^ columnids);

        /// <summary>
        /// </summary>
        array<MJET_ENUMCOLUMN>^ MJetEnumerateColumns(MJET_TABLEID tableid);

        /// <summary>
        /// </summary>
        MJET_WRN MJetSetColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            array<Byte>^ data,
            MJET_GRBIT grbit,
            MJET_SETINFO setinfo);

        /// <summary>
        /// </summary>
        MJET_WRN MJetSetColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            array<Byte>^ data,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        MJET_WRN MJetSetColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            array<Byte>^ data);

        /// <summary>
        /// </summary>
        void MJetPrepareUpdate(MJET_TABLEID tableid, MJET_PREP prep);

        /// <summary>
        /// </summary>
        MJET_RECPOS MJetGetRecordPosition(MJET_TABLEID tableid);

        /// <summary>
        /// </summary>
        void MJetGotoPosition(MJET_TABLEID tableid, MJET_RECPOS recpos);

        /// <summary>
        /// </summary>
        MJET_TABLEID MJetDupCursor(MJET_TABLEID tableid, MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        MJET_TABLEID MJetDupCursor(MJET_TABLEID tableid);

        /// <summary>
        /// </summary>
        String^ MJetGetCurrentIndex(MJET_TABLEID tableid);

        /// <summary>
        /// </summary>
        void MJetSetCurrentIndex(MJET_TABLEID tableid, String^ name);

        /// <summary>
        /// </summary>
        void MJetSetCurrentIndex(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        void MJetSetCurrentIndex(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit,
            Int64 itag);

        /// <summary>
        /// </summary>
        MJET_WRN MJetMove(
            MJET_TABLEID tableid,
            Int32 rows,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        MJET_WRN MJetMove(MJET_TABLEID tableid, Int32 rows);

        /// <summary>
        /// </summary>
        MJET_WRN MJetMove(
            MJET_TABLEID tableid,
            MJET_MOVE rows,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        MJET_WRN MJetMove(MJET_TABLEID tableid, MJET_MOVE rows);

        /// <summary>
        /// Convenience function that doesn't throw an exception
        /// </summary>
        void MoveBeforeFirst(MJET_TABLEID tableid);

        /// <summary>
        /// Convenience function that doesn't throw an exception
        /// </summary>
        void MoveAfterLast(MJET_TABLEID tableid);

        /// <summary>
        /// Convenience function that doesn't throw an exception if runs out of records,
        /// just returns false
        /// </summary>
        Boolean TryMoveFirst(MJET_TABLEID tableid);

        /// <summary>
        /// Convenience function that doesn't throw an exception if runs out of records,
        /// just returns false
        /// </summary>
        Boolean TryMoveNext(MJET_TABLEID tableid);

        /// <summary>
        /// Convenience function that doesn't throw an exception for out of records,
        /// just returns false
        /// </summary>
        Boolean TryMoveLast(MJET_TABLEID tableid);

        /// <summary>
        /// Convenience function that doesn't throw an exception if runs out of records,
        /// just returns false
        /// </summary>
        Boolean TryMovePrevious(MJET_TABLEID tableid);

        /// <summary>
        /// Convenience function that doesn't throw an exception if runs out of records,
        /// just returns false
        /// </summary>
        Boolean TryMove(MJET_TABLEID tableid, MJET_MOVE rows);

        /// <summary>
        /// Convenience function that doesn't throw an exception if runs out of records,
        /// just returns false
        /// </summary>
        Boolean TryMove(MJET_TABLEID tableid, Int32 rows);

        /// <summary>
        /// </summary>
        void MJetGetLock(MJET_TABLEID tableid, MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        MJET_WRN MJetMakeKey(
            MJET_TABLEID tableid,
            array<Byte>^ data,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        MJET_WRN MJetSeek(MJET_TABLEID tableid, MJET_GRBIT grbit);

        /// <summary>
        /// Convenience function that doesn't throw an exception if it doesn't find a record, but
        /// just returns false
        /// </summary>
        Boolean TrySeek(MJET_TABLEID tableid, MJET_GRBIT grbit);

        /// <summary>
        /// Convenience function that doesn't throw an exception if it doesn't find a record, but
        /// just returns false
        /// </summary>
        Boolean TrySeek(MJET_TABLEID tableid);

        /// <summary>
        /// </summary>
        array<Byte>^ MJetGetBookmark(MJET_TABLEID tableid);

        /// <summary>
        /// </summary>
        MJET_SECONDARYINDEXBOOKMARK MJetGetSecondaryIndexBookmark(MJET_TABLEID tableid, MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        MJET_SECONDARYINDEXBOOKMARK MJetGetSecondaryIndexBookmark(MJET_TABLEID tableid);

        /// <summary>
        /// </summary>
        MJET_WRN MJetDefragment(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            Int32% passes,
            Int32% seconds,
            MJET_GRBIT grbit);

        /// <summary>
        /// Jet Online Defragmentation API that takes a callback
        /// </summary>
        MJET_WRN MJetDefragment2(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            Int32% passes,
            Int32% seconds,
            MJetOnEndOnlineDefragCallbackDelegate^ callback,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        Int64 MJetSetDatabaseSize(
            MJET_SESID sesid,
            String^ database,
            Int64 pages);

        /// <summary>
        /// </summary>
        Int64 MJetGrowDatabase(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pages);

        /// <summary>
        /// </summary>
        MJET_WRN MJetResizeDatabase(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pages,
            [Out] Int64% actualPages,
            MJET_GRBIT grbit);

        /// <summary>
        /// Sets the session to be associated with a thread and debugging context.
        //
        /// When used before a transaction is begun, will allow the session's context to be reset
        /// to make the session mobile / usable across threads even while an transaction is open.
        //
        /// When used after a transaction is already open, will associate this thread and debugging
        /// context with this session, so the session + transaction can be used on the thread.
        //
        /// Note: context is truncated from an Int64 to an Int32 on 32-bit architectures, but since
        /// this is for debugging only, this is fine.
        /// </summary>
        void MJetSetSessionContext(MJET_SESID sesid, IntPtr context);

        /// <summary>
        /// Resets the session to be un-associated with a thread and debugging context.
        //
        /// Can only be used when a session already had a context set before a transaction was
        /// begun.  This allows the session + open transaction to have a different thread and
        /// session associated with it via JetSetSessionContext.
        /// </summary>
        void MJetResetSessionContext(MJET_SESID sesid);

        /// <summary>
        /// </summary>
        void MJetGotoBookmark(MJET_TABLEID tableid, array<Byte>^ bookmark);

        /// <summary>
        /// </summary>
        void MJetGotoSecondaryIndexBookmark(
            MJET_TABLEID tableid,
            array<Byte>^ secondaryKey,
            array<Byte>^ bookmark,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        array<MJET_COLUMNID>^ MJetOpenTempTable(
            MJET_SESID sesid,
            array<MJET_COLUMNDEF>^ columndefs,
            MJET_UNICODEINDEX unicodeindex,
            MJET_GRBIT grbit,
            MJET_TABLEID% tableid);

        /// <summary>
        /// </summary>
        array<MJET_COLUMNID>^ MJetOpenTempTable(
            MJET_SESID sesid,
            array<MJET_COLUMNDEF>^ columndefs,
            MJET_TABLEID% tableid);

        /// <summary>
        /// </summary>
        void MJetSetIndexRange(MJET_TABLEID tableid, MJET_GRBIT grbit);

        /// <summary>
        /// Convenience function that doesn't throw an exception if range is empty,
        /// just returns false
        /// </summary>
        Boolean TrySetIndexRange(MJET_TABLEID tableid, MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        void ResetIndexRange(MJET_TABLEID tableid);

        /// <summary>
        /// </summary>
        Int64 MJetIndexRecordCount(MJET_TABLEID tableid, Int64 maxRecordsToCount);

        /// <summary>
        /// </summary>
        array<Byte>^ MJetRetrieveKey(MJET_TABLEID tableid, MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        array<Byte>^ MJetRetrieveKey(MJET_TABLEID tableid);

#ifdef INTERNALUSE
        /// <summary>
        /// </summary>
        void MJetTracing(MJET_TRACEOP traceOp, MJET_TRACETAG traceTag, IntPtr ul);

        /// <summary>
        /// </summary>
        void MJetTracing(MJET_TRACEOP traceOp, MJET_TRACETAG traceTag, MJET_DBID dbid);
#endif
    };

    MSINTERNAL interface class IJetInteropPublicOnly : public IJetInteropCommon
    {
        /// <summary>
        /// Make a managed MJET_SESID from an unmanaged sesid and a managed instance
        /// </summary>
        MJET_SESID MakeManagedSesid(::JET_SESID _sesid, MJET_INSTANCE instance);

        /// <summary>
        /// Make a managed MJET_DBID from an unmanaged dbid
        /// </summary>
        MJET_DBID MakeManagedDbid(::JET_DBID _dbid);

        /// <summary>
        /// Make a managed MJET_TABLEID from an unmanaged tableid and managed sesid and dbid
        /// </summary>
        MJET_TABLEID MakeManagedTableid(::JET_TABLEID _tableid, MJET_SESID sesid, MJET_DBID dbid);

        /// <summary>
        /// Make a managed MJET_DBINFO from an unmanaged byte array containing JET_DBINFOMISC structure
        /// </summary>
        MJET_DBINFO MakeManagedDbinfo(IntPtr _handle, unsigned int _cbconst);

        /// <summary>
        /// Make a managed MJET_DBINFO from an unmanaged JET_DBINFOMISC structure
        /// </summary>
        MJET_DBINFO MakeManagedDbinfo(const ::JET_DBINFOMISC& _dbinfomisc);

        String^ MJetGetColumnName(MJET_TABLEID tableid, MJET_COLUMNID columnid);

        ///  there is currently no efficient means to retrieve the
        ///  name of a specific column from JET by columnid.  so, we are
        ///  going to reach into the catalog and fetch it directly
        String^ MJetGetColumnName(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ tablename,
            MJET_COLUMNID columnid);

        ///  there is currently no efficient means to retrieve the
        ///  name of a specific column from JET by columnid.  so, we are
        ///  going to reach into the catalog and fetch it directly
        array<Byte>^ BytesFromObject(
            MJET_COLTYP type,
            Boolean isASCII,
            Object^ o);
    };

    MSINTERNAL interface class IJetInteropWinXP : public IJetInteropPublicOnly
    {
        /// <summary>
        /// </summary>
        void MJetResetCounter(MJET_SESID sesid, Int32 CounterType);

        /// <summary>
        /// </summary>
        Int32 MJetGetCounter(MJET_SESID sesid, Int32 CounterType);

        void MJetDatabaseScan(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int32 msecSleep,
            MJET_GRBIT grbit);

        void MJetConvertDDL(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_DDLCALLBACKDLL ddlcallbackdll);

        /// <summary>
        /// </summary>
        void MJetUpgradeDatabase(
            MJET_SESID sesid,
            String^ database,
            String^ streamingFile,
            MJET_GRBIT grbit);

        /// <summary>
        /// Get information about the session.
        /// </summary>
        MJET_SESSIONINFO MJetGetSessionInfo(MJET_SESID sesid);

        /// <summary>
        /// General database utilities
        /// </summary>
        void MJetDBUtilities(MJET_DBUTIL dbutil);

        /// <summary>
        /// Returns if the build type is DEBUG for the interop layer.
        /// </summary>
        Boolean IsDbgFlagDefined();

        /// <summary>
        /// Returns if the RTM macro is defined for the interop layer.
        /// </summary>
        Boolean IsRtmFlagDefined();

        /// <summary>
        /// Returns whether the interop layer calls the native ESE Unicode or ASCII APIs.
        /// </summary>
        Boolean IsNativeUnicode();

#ifdef INTERNALUSE
        /// <summary>
        /// Given an unmanaged JET_EMITDATACTX , return a managed MJET_EMITDATACTX struct.
        /// </summary>
        MJET_EMITDATACTX MakeManagedEmitLogDataCtx(const ::JET_EMITDATACTX& _emitctx);

        /// <summary>
        /// Given a managed array of bytes holding an image of an unmanged JET_EMITDATACTX struct,
        /// load a managed MJET_EMITDATACTX.
        /// </summary>
        MJET_EMITDATACTX MakeManagedEmitLogDataCtxFromBytes(array<Byte>^ bytes);

        /// <summary>
        /// This is the API wrapper for the native JetConsumeLogData API.  Note the data is an array of bytes
        /// so combines the typical void * pv and ULONG cb native params into one param.
        /// </summary>
        MJET_WRN MJetConsumeLogData(
            MJET_INSTANCE instance,
            MJET_EMITDATACTX EmitLogDataCtx,
            array<Byte>^ data,
            MJET_GRBIT grbit);

        /// <summary>
        /// This variant of MJetConsumeLogData is used by HA to allow for a binary pass-through of the emit metadata so that
        /// we avoid converting between managed/unmanged EMITDATACTX, which is currently a lossy transformation.
        /// </summary>
        MJET_WRN MJetConsumeLogData(
            MJET_INSTANCE instance,
            array<Byte>^ emitMetaDataAsManagedBytes,
            array<Byte>^ data,
            MJET_GRBIT grbit);
#endif
    };

    MSINTERNAL interface class IJetInteropExchange : public IJetInteropCommon
    {
        /// <summary>
        /// Make a managed MJET_SESID from an unmanaged sesid and a managed instance
        /// </summary>
        MJET_SESID MakeManagedSesid(::JET_SESID _sesid, MJET_INSTANCE instance);

        /// <summary>
        /// Make a managed MJET_DBID from an unmanaged dbid
        /// </summary>
        MJET_DBID MakeManagedDbid(::JET_DBID _dbid);

        /// <summary>
        /// Make a managed MJET_TABLEID from an unmanaged tableid and managed sesid and dbid
        /// </summary>
        MJET_TABLEID MakeManagedTableid(::JET_TABLEID _tableid, MJET_SESID sesid, MJET_DBID dbid);

        /// <summary>
        /// Make a managed MJET_DBINFO from an unmanaged byte array containing JET_DBINFOMISC7 structure
        /// </summary>
        MJET_DBINFO MakeManagedDbinfo(IntPtr _handle, unsigned int _cbconst);

        /// <summary>
        /// Make a managed MJET_DBINFO from an unmanaged JET_DBINFOMISC7 structure
        /// </summary>
        MJET_DBINFO MakeManagedDbinfo(const ::JET_DBINFOMISC7& _dbinfomisc);

        /// <summary>
        /// Turn a managed MJET_DBINFO into an array of bytes. This array can be
        /// converted back into an MJET_DBINFO with MakeManagerDbinfo. Yes, this
        /// is home-made serialization.
        /// </summary>
        array<Byte>^ GetBytes(MJET_DBINFO dbinfo);

#ifdef INTERNALUSE
        /// <summary>
        /// Initialize Jet, returning a new instance or throwing an exception.
        /// Databases to recover, a callback for recovery and
        /// the maximum generation to be replayed
        /// (only when performing recovery without undo) may be specified.
        /// </summary>
        MJET_INSTANCE MJetInit(
            MJET_INSTANCE instance,
            array<MJET_RSTMAP>^ rstmaps,
            Int64 genStop,
            MJetInitCallback^ callback,
            MJET_GRBIT grbit);
#endif

        /// <summary>
        /// Initialize Jet, returning a new instance or throwing an exception.
        /// Databases to recover and the maximum generation to be replayed
        /// (only when performing recovery without undo) may be specified.
        /// </summary>
        MJET_INSTANCE MJetInit(
            MJET_INSTANCE instance,
            array<MJET_RSTMAP>^ rstmaps,
            Int64 /*genStop*/,
            MJET_GRBIT grbit);

        /// <summary>
        /// Initialize Jet, returning a new instance or throwing an exception.
        /// Databases to recover may be specified.
        /// </summary>
        MJET_INSTANCE MJetInit(
            MJET_INSTANCE instance,
            array<MJET_RSTMAP>^ rstmaps,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        void MJetGetResourceParam(
            MJET_INSTANCE instance,
            MJET_RESOPER resoper,
            MJET_RESID resid,
            Int64% param);

        /// <summary>
        /// </summary>
        void MJetResetCounter(MJET_SESID sesid, Int32 CounterType);

        /// <summary>
        /// </summary>
        Int32 MJetGetCounter(MJET_SESID sesid, Int32 CounterType);

        /// <summary>
        /// </summary>
        void MJetTestHookEnableFaultInjection(
            Int32 id,
            JET_ERR err,
            Int32 ulProbability,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        void MJetTestHookEnableConfigOverrideInjection(
            Int32 id,
            Int64 ulOverride,
            Int32 ulProbability,
            MJET_GRBIT grbit);

        IntPtr MJetTestHookEnforceContextFail(
            IntPtr callback);

        /// <summary>
        /// </summary>
        void MJetTestHookEvictCachedPage(
            MJET_DBID dbid,
            Int32 pgno);

        /// <summary>
        /// </summary>
        void MJetSetNegativeTesting(
            MJET_NEGATIVE_TESTING_BITS grbit);

        /// <summary>
        /// </summary>
        void MJetResetNegativeTesting(
            MJET_NEGATIVE_TESTING_BITS grbit);

        /// <summary>
        /// The JetGetInstanceInfo function retrieves information about the instances that are running.
        /// </summary>
        /// <remarks>
        /// If there are no active instances, MJetGetInstanceInfo will return an array with 0 elements.
        /// </remarks>
        array<MJET_INSTANCE_INFO>^ MJetGetInstanceInfo();

        /// <summary>
        /// Given raw page data, return information about those pages
        /// </summary>
        /// <remarks>
        /// The unmanaged API takes multiple pages at one time. We simplify this API by returning
        /// information for only one page at a time.
        /// </remarks>
        MJET_PAGEINFO MJetGetPageInfo(array<Byte>^ page, Int64 PageNumber);

        /// <summary>
        /// </summary>
        MJET_LOGINFOMISC MJetGetLogFileInfo(String^ logfile);

        /// <summary>
        /// Given a database and a logfile, update the database header so that the logfile is no longer in the committed
        /// range of logfiles. This causes data loss. The logfile is not deleted.
        /// </summary>
        void MJetRemoveLogfile(String^ database, String^ logfile);

        /// <summary>
        /// </summary>
        array<Byte>^ MJetGetDatabasePages(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pgnoStart,
            Int64 cpg,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        void MJetOnlinePatchDatabasePage(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pgno,
            array<Byte>^ token,
            array<Byte>^ data,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        void MJetBeginDatabaseIncrementalReseed(
            MJET_INSTANCE instance,
            String^ pstrDatabase,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        void MJetEndDatabaseIncrementalReseed(
            MJET_INSTANCE instance,
            String^ pstrDatabase,
            Int64 genMinRequired,
            Int64 genFirstDivergedLog,
            Int64 genMaxRequired,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        void MJetPatchDatabasePages(
            MJET_INSTANCE instance,
            String^ pstrDatabase,
            Int64 pgnoStart,
            Int64 cpg,
            array<Byte>^ rgb,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        MJET_COLUMNBASE MJetGetColumnInfo(MJET_TABLEID tableid, Int64 colid);

        /// <summary>
        /// </summary>
        MJET_COLUMNBASE MJetGetColumnInfo(MJET_TABLEID tableid, MJET_COLUMNID columnid);

        /// <summary>
        /// </summary>
        MJET_INDEXCREATE MJetGetIndexInfoCreate(MJET_TABLEID tableid, String^ index);

        /// <summary>
        /// </summary>
        MJET_INDEXCREATE MJetGetIndexInfoCreate(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index);

        /// <summary>
        /// </summary>
        array<Byte>^ MJetUpdate(MJET_TABLEID tableid, MJET_GRBIT grbit);

        MJET_RECSIZE MJetGetRecordSize(MJET_TABLEID tableid, MJET_GRBIT grbit);

        MJET_RECSIZE MJetGetRecordSize(MJET_TABLEID tableid);

        /// <summary>
        /// Prereads the keys, returns the number of keys preread
        /// </summary>
        Int32 MJetPrereadKeys(
            MJET_TABLEID tableid,
            array<array<Byte>^>^ keys,
            MJET_GRBIT grbit);

        void MJetDatabaseScan(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int32 msecSleep,
            MJET_GRBIT grbit);

        void MJetConvertDDL(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_DDLCALLBACKDLL ddlcallbackdll);

        /// <summary>
        /// </summary>
        void MJetUpgradeDatabase(
            MJET_SESID sesid,
            String^ database,
            String^ streamingFile,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        void MJetSetMaxDatabaseSize(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pages,
            MJET_GRBIT grbit);

        /// <summary>
        /// </summary>
        Int64 MJetGetMaxDatabaseSize(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_GRBIT grbit);

        /// <summary>
        /// Get information about the session.
        /// </summary>
        MJET_SESSIONINFO MJetGetSessionInfo(MJET_SESID sesid);

        /// <summary>
        /// General database utilities
        /// </summary>
        void MJetDBUtilities(MJET_DBUTIL dbutil);

        /// <summary>
        /// Wrapper function for Header/Trailer update code
        /// </summary>
        void MJetUpdateDBHeaderFromTrailer(String^ database);

        /// <summary>
        /// Wrapper function for ChecksumLogFromMemory update code
        /// </summary>
        void MJetChecksumLogFromMemory(MJET_SESID sesid, String^ wszLog, String^ wszBase, array<Byte>^ buffer);

        /// <summary>
        /// </summary>
        MJET_TABLEID MJetOpenTemporaryTable(MJET_SESID sesid, MJET_OPENTEMPORARYTABLE% opentemporarytable);

        /// <summary>
        /// Returns if the build type is DEBUG for the interop layer.
        /// </summary>
        Boolean IsDbgFlagDefined();

        /// <summary>
        /// Returns if the RTM macro is defined for the interop layer.
        /// </summary>
        Boolean IsRtmFlagDefined();

        /// <summary>
        /// Returns whether the interop layer calls the native ESE Unicode or ASCII APIs.
        /// </summary>
        Boolean IsNativeUnicode();

#ifdef INTERNALUSE
        /// <summary>
        /// Given an unmanaged JET_EMITDATACTX , return a managed MJET_EMITDATACTX struct.
        /// </summary>
        MJET_EMITDATACTX MakeManagedEmitLogDataCtx(const ::JET_EMITDATACTX& _emitctx);

        /// <summary>
        /// Given a managed array of bytes holding an image of an unmanged JET_EMITDATACTX struct,
        /// load a managed MJET_EMITDATACTX.
        /// </summary>
        MJET_EMITDATACTX MakeManagedEmitLogDataCtxFromBytes(array<Byte>^ bytes);

        /// <summary>
        /// This is the API wrapper for the native JetConsumeLogData API.  Note the data is an array of bytes
        /// so combines the typical void * pv and ULONG cb native params into one param.
        /// </summary>
        MJET_WRN MJetConsumeLogData(
            MJET_INSTANCE instance,
            MJET_EMITDATACTX EmitLogDataCtx,
            array<Byte>^ data,
            MJET_GRBIT grbit);

        /// <summary>
        /// This variant of MJetConsumeLogData is used by HA to allow for a binary pass-through of the emit metadata so that
        /// we avoid converting between managed/unmanged EMITDATACTX, which is currently a lossy transformation.
        /// </summary>
        MJET_WRN MJetConsumeLogData(
            MJET_INSTANCE instance,
            array<Byte>^ emitMetaDataAsManagedBytes,
            array<Byte>^ data,
            MJET_GRBIT grbit);
#endif
        /// <summary>
        /// </summary>
        void MJetStopServiceInstance2(
            MJET_INSTANCE instance,
            MJET_GRBIT grbit);

    };

#if (ESENT_PUBLICONLY)
    typedef IJetInteropPublicOnly IJetInterop;
#else
    #if (ESENT_WINXP)
        typedef IJetInteropWinXP IJetInterop;
    #else
        typedef IJetInteropExchange IJetInterop;
    #endif
#endif

    MSINTERNAL ref class JetInterop : public IJetInterop
    {
    public:
        JetInterop()
        {
            this->m_exceptionTranslator = gcnew ExceptionTranslator(&JetInterop::DoNothingTranslator);
        }

        /// <summary>
        /// Enable Jet to throw exceptions that are meaningful to Jet's consumers.
        /// Thus, rather than catching the Jet exception, wrapping it and re-throwing, the consumer
        /// simply tells Jet what to throw.
        /// </summary>
        /// <param name="translator">The function to use to translate a Jet exception into an exception useful to the consumner.
        /// Pass null to reset to the default behavior of throwing the Jet exception.</param>
        virtual void SetExceptionTranslator(ExceptionTranslator^ translator)
        {
            if (translator == nullptr)
            {
                this->m_exceptionTranslator = gcnew ExceptionTranslator(&JetInterop::DoNothingTranslator);
            }
            else
            {
                this->m_exceptionTranslator = translator;
            }
        }

        /// <summary>
        /// </summary>
        virtual ::JET_INSTANCE GetUnmanagedInst(MJET_INSTANCE instance)
        {
            ::JET_INSTANCE _inst = (::JET_INSTANCE)(instance.NativeValue);
            return _inst;
        }

        /// <summary>
        /// </summary>
        virtual MJET_INSTANCE MakeManagedInst(::JET_INSTANCE _instance)
        {
            MJET_INSTANCE instance;
            instance.NativeValue = _instance;
            return instance;
        }

        /// <summary>
        /// </summary>
        virtual ::JET_SESID GetUnmanagedSesid(MJET_SESID sesid)
        {
            ::JET_SESID _sesid = (::JET_SESID)(sesid.NativeValue);
            return _sesid;
        }

        /// <summary>
        /// </summary>
        virtual MJET_SESID MakeManagedSesid(::JET_SESID _sesid, MJET_INSTANCE instance)
        {
            MJET_SESID sesid;
            sesid.Instance = instance;
            sesid.NativeValue = _sesid;
            return sesid;
        }

        /// <summary>
        /// </summary>
        virtual ::JET_DBID GetUnmanagedDbid(MJET_DBID dbid)
        {
            ::JET_DBID _dbid = (::JET_DBID)(dbid.NativeValue);
            return _dbid;
        }

        /// <summary>
        /// </summary>
        virtual MJET_DBID MakeManagedDbid(::JET_DBID _dbid)
        {
            MJET_DBID dbid;
            dbid.NativeValue = _dbid;
            return dbid;
        }

        /// <summary>
        /// </summary>
        virtual ::JET_TABLEID GetUnmanagedTableid(MJET_TABLEID tableid)
        {
            ::JET_TABLEID _tableid = (::JET_TABLEID)(tableid.NativeValue);
            return _tableid;
        }

        /// <summary>
        /// </summary>
        virtual MJET_TABLEID MakeManagedTableid(
            ::JET_TABLEID _tableid,
            MJET_SESID sesid,
            MJET_DBID dbid)
        {
            MJET_TABLEID tableid;
            tableid.Sesid = sesid;
            tableid.Dbid = dbid;
            tableid.NativeValue = _tableid;
            return tableid;
        }

        /// <summary>
        /// </summary>
        virtual MJET_TABLEID MakeManagedTableid(::JET_TABLEID _tableid, MJET_SESID sesid)
        {
            // use this only for the temp db where a dbid isn't available
            return MakeManagedTableid( _tableid, sesid, MakeManagedDbid( JET_dbidNil ) );
        }

        /// <summary>
        /// </summary>
        virtual ::JET_COLUMNID GetUnmanagedColumnid(MJET_COLUMNID columnid)
        {
            ::JET_COLUMNID _columnid = (::JET_COLUMNID)(columnid.NativeValue);
            return _columnid;
        }

        /// <summary>
        /// </summary>
        virtual MJET_COLUMNID MakeManagedColumnid(
            ::JET_COLUMNID _columnid,
            MJET_COLTYP coltyp,
            bool fASCII)
        {
            MJET_COLUMNID columnid;
            columnid.Coltyp = coltyp;
            columnid.isASCII = fASCII;
            columnid.NativeValue = _columnid;
            return columnid;
        }

        /// <summary>
        /// </summary>
        virtual ::JET_COLUMNDEF GetUnmanagedColumndef(MJET_COLUMNDEF% columndef)
        {
            ::JET_COLUMNDEF _columndef = { 0 };

            _columndef.cbStruct = sizeof( _columndef );
            _columndef.columnid = GetUnmanagedColumnid( columndef.Columnid );
            _columndef.coltyp = (::JET_COLTYP) columndef.Coltyp;
            _columndef.wCountry = 0;
            _columndef.langid = (unsigned short)columndef.Langid;
            _columndef.cp = columndef.ASCII ? 1252 : 1200;
            _columndef.wCollate = 0;
            _columndef.cbMax = columndef.MaxLength;
            _columndef.grbit = (::JET_GRBIT) columndef.Grbit;

            return _columndef;
        }

        /// <summary>
        /// </summary>
        virtual void FreeUnmanagedColumndef(::JET_COLUMNDEF /*_columndef*/)
        {
            return;
        }

        /// <summary>
        /// </summary>
        virtual MJET_COLUMNDEF MakeManagedColumndef(::JET_COLUMNDEF _columndef)
        {
            MJET_COLTYP coltyp = ( MJET_COLTYP )_columndef.coltyp;
            bool ASCII = 1252 == _columndef.cp;

            MJET_COLUMNDEF columndef;
            columndef.Columnid = MakeManagedColumnid( _columndef.columnid, coltyp, ASCII );
            columndef.Coltyp = coltyp;
            columndef.Langid = _columndef.langid;
            columndef.ASCII = ASCII;
            columndef.MaxLength = _columndef.cbMax;
            columndef.Grbit = (MJET_GRBIT)_columndef.grbit;

            return columndef;
        }

        /// <summary>
        /// </summary>
        virtual ::JET_COLUMNDEF* GetUnmanagedColumndefs(array<MJET_COLUMNDEF>^ columndefs)
        {
            int _i = 0;
            const int _ccolumndef = columndefs->Length;
            ::JET_COLUMNDEF* _pcolumndef = 0;

            try
            {
                _pcolumndef = new ::JET_COLUMNDEF[ _ccolumndef ];
                if ( 0 == _pcolumndef )
                {
                    throw TranslatedException(gcnew IsamOutOfMemoryException());
                }
                memset( _pcolumndef, 0, sizeof( *_pcolumndef ) * _ccolumndef );

                for ( _i = 0; _i < _ccolumndef; ++_i )
                {
                    _pcolumndef[ _i ] = GetUnmanagedColumndef( columndefs[ _i ] );
                }
                //Assert( _ccolumndef == _i );
            }
            catch ( ... )
            {
                FreeUnmanagedColumndefs( _pcolumndef, _i );
                throw;
            }

            return _pcolumndef;
        }

        /// <summary>
        /// </summary>
        virtual void FreeUnmanagedColumndefs(::JET_COLUMNDEF* _pcolumndef, const int _ccolumndef)
        {

            for ( int _i = 0; _i < _ccolumndef; ++_i )
            {
                FreeUnmanagedColumndef( _pcolumndef[ _i ] );
            }
            delete[] _pcolumndef;
        }

#if (ESENT_WINXP)
#else
        /// <summary>
        /// </summary>
        virtual ::JET_OPENTEMPORARYTABLE GetUnmanagedOpenTemporaryTable(MJET_OPENTEMPORARYTABLE opentemporarytable)
        {
            ::JET_OPENTEMPORARYTABLE    _opentemporarytable = { 0 };
            ::JET_UNICODEINDEX          _idxunicode         = { 0 };

            try
            {
                _opentemporarytable.cbStruct = sizeof( _opentemporarytable );
                _opentemporarytable.prgcolumndef = GetUnmanagedColumndefs( opentemporarytable.Columndefs );
                _opentemporarytable.ccolumn = opentemporarytable.Columndefs->Length;
                _opentemporarytable.pidxunicode = ::new ::JET_UNICODEINDEX;
                if ( 0 == _opentemporarytable.pidxunicode )
                {
                    throw TranslatedException(gcnew IsamOutOfMemoryException());
                }
                _idxunicode = GetUnmanagedUnicodeIndex( opentemporarytable.UnicodeIndex );
                _opentemporarytable.pidxunicode->lcid = _idxunicode.lcid;
                _opentemporarytable.pidxunicode->dwMapFlags = _idxunicode.dwMapFlags;
                _opentemporarytable.grbit = (::JET_GRBIT)opentemporarytable.Grbit;
                _opentemporarytable.prgcolumnid = ::new ::JET_COLUMNID[ _opentemporarytable.ccolumn ];
                if ( 0 == _opentemporarytable.prgcolumnid )
                {
                    throw TranslatedException(gcnew IsamOutOfMemoryException());
                }
                memset( _opentemporarytable.prgcolumnid, 0, sizeof( *_opentemporarytable.prgcolumnid ) * _opentemporarytable.ccolumn );
                _opentemporarytable.cbKeyMost = opentemporarytable.KeyLengthMax;
                _opentemporarytable.cbVarSegMac = opentemporarytable.VarSegMac;
                _opentemporarytable.tableid = 0;
            }
            catch( ... )
            {
                FreeUnmanagedOpenTemporaryTable( _opentemporarytable );
                throw;
            }

            return _opentemporarytable;
        }

        /// <summary>
        /// </summary>
        virtual void FreeUnmanagedOpenTemporaryTable(_In_ __drv_freesMem(object) ::JET_OPENTEMPORARYTABLE _opentemporarytable)
        {
            FreeUnmanagedColumndefs( (::JET_COLUMNDEF*)_opentemporarytable.prgcolumndef, _opentemporarytable.ccolumn );
            delete _opentemporarytable.pidxunicode;
            ::delete [] _opentemporarytable.prgcolumnid;
        }
#endif

        /// <summary>
        /// </summary>
        virtual ::JET_SETINFO GetUnmanagedSetinfo(MJET_SETINFO setinfo)
        {
            ::JET_SETINFO _setinfo;
            _setinfo.cbStruct = sizeof( _setinfo );
            _setinfo.ibLongValue = (unsigned long)setinfo.IbLongValue;
            _setinfo.itagSequence = (unsigned long)setinfo.ItagSequence;
            return _setinfo;
        }

        /// <summary>
        /// </summary>
        virtual ::JET_RETINFO GetUnmanagedRetinfo(MJET_RETINFO retinfo)
        {
            ::JET_RETINFO _retinfo;
            _retinfo.cbStruct = sizeof( _retinfo );
            _retinfo.ibLongValue = (unsigned long)retinfo.IbLongValue;
            _retinfo.itagSequence = (unsigned long)retinfo.ItagSequence;
            return _retinfo;
        }

        /// <summary>
        /// </summary>
        virtual ::JET_RECPOS GetUnmanagedRecpos(MJET_RECPOS recpos)
        {
            ::JET_RECPOS _recpos;
            _recpos.cbStruct = sizeof( _recpos );
            _recpos.centriesLT = (unsigned long)recpos.EntriesLessThan;
            _recpos.centriesTotal = (unsigned long)recpos.TotalEntries;
            return _recpos;
        }

        /// <summary>
        /// </summary>
        virtual MJET_RECPOS MakeManagedRecpos(const ::JET_RECPOS& _recpos)
        {
            MJET_RECPOS recpos;
            recpos.EntriesLessThan = _recpos.centriesLT;
            recpos.TotalEntries = _recpos.centriesTotal;
            return recpos;
        }

        /// <summary>
        /// This allocates memory (for the strings)
        /// </summary>
        virtual ::JET_RSTMAP GetUnmanagedRstmap(MJET_RSTMAP rstmap)
        {
            ::JET_RSTMAP _rstmap;

            // initialize to NULL for exception safety
            // if the first alloc throws an exception all members will be NULL

            _rstmap.szDatabaseName = 0;
            _rstmap.szNewDatabaseName = 0;

            try
            {
                _rstmap.szDatabaseName = GetUnmanagedString( rstmap.DatabaseName );
                _rstmap.szNewDatabaseName = GetUnmanagedString( rstmap.NewDatabaseName );
            }
            catch( ... )
            {
                FreeUnmanagedString( _rstmap.szDatabaseName );
                FreeUnmanagedString( _rstmap.szNewDatabaseName );
                throw;
            }

            return _rstmap;
        }

        /// <summary>
        /// </summary>
        virtual MJET_OBJECTINFO MakeManagedObjectInfo(const ::JET_OBJECTINFO& _objectinfo)
        {
            MJET_OBJECTINFO objectinfo;
            objectinfo.Objtyp = (MJET_OBJTYP)_objectinfo.objtyp;
            objectinfo.Grbit = (MJET_GRBIT)_objectinfo.grbit;
            objectinfo.Flags = (MJET_GRBIT)_objectinfo.flags;
            return objectinfo;
        }

        /// <summary>
        /// </summary>
        virtual MJET_OBJECTLIST MakeManagedObjectList(MJET_SESID sesid, const ::JET_OBJECTLIST& _objectlist)
        {
            MJET_OBJECTLIST objectlist;
            objectlist.Tableid = MakeManagedTableid( _objectlist.tableid, sesid );
            objectlist.Records = _objectlist.cRecord;

            // types are taken from info.cxx
            bool fASCII = !IsNativeUnicode();
            objectlist.ColumnidContainerName = MakeManagedColumnid( _objectlist.columnidcontainername, MJET_COLTYP::Text, fASCII );
            objectlist.ColumnidObjectName = MakeManagedColumnid( _objectlist.columnidobjectname, MJET_COLTYP::Text, fASCII );
            objectlist.ColumnidObjectType = MakeManagedColumnid( _objectlist.columnidobjtyp, MJET_COLTYP::Long, false );
            objectlist.ColumnidGrbit = MakeManagedColumnid( _objectlist.columnidgrbit, MJET_COLTYP::Long, false );
            objectlist.ColumnidFlags = MakeManagedColumnid( _objectlist.columnidflags, MJET_COLTYP::Long, false );

            return objectlist;
        }

        /// <summary>
        /// </summary>
        virtual MJET_COLUMNBASE MakeManagedColumnbase(const ::JET_COLUMNBASE& _columnbase)
        {
            MJET_COLUMNBASE columnbase;
            columnbase.Coltyp = (MJET_COLTYP)_columnbase.coltyp;
            columnbase.Langid = _columnbase.langid;
            columnbase.ASCII = ( 1252 == _columnbase.cp );
            columnbase.Columnid = MakeManagedColumnid( _columnbase.columnid, columnbase.Coltyp, columnbase.ASCII );
            columnbase.MaxLength = _columnbase.cbMax;
            columnbase.Grbit = (MJET_GRBIT)_columnbase.grbit;
            columnbase.TableName = MakeManagedString( _columnbase.szBaseTableName );
            columnbase.ColumnName = MakeManagedString( _columnbase.szBaseColumnName );
            return columnbase;
        }

        /// <summary>
        /// </summary>
        virtual MJET_COLUMNLIST MakeManagedColumnList(MJET_SESID sesid, const ::JET_COLUMNLIST& _columnlist)
        {
            MJET_COLUMNLIST columnlist;
            columnlist.Tableid = MakeManagedTableid( _columnlist.tableid, sesid );
            columnlist.Records = _columnlist.cRecord;

            // types are taken from info.cxx
            bool fASCII = !IsNativeUnicode();
            columnlist.ColumnidColumnName = MakeManagedColumnid( _columnlist.columnidcolumnname, MJET_COLTYP::Text, fASCII );
            columnlist.ColumnidMaxLength = MakeManagedColumnid( _columnlist.columnidcbMax, MJET_COLTYP::Long, false );
            columnlist.ColumnidGrbit = MakeManagedColumnid( _columnlist.columnidgrbit, MJET_COLTYP::Long, false );
            columnlist.ColumnidDefault = MakeManagedColumnid( _columnlist.columnidDefault, MJET_COLTYP::LongBinary, false );
            columnlist.ColumnidTableName = MakeManagedColumnid( _columnlist.columnidBaseTableName, MJET_COLTYP::Text, fASCII );

            return columnlist;
        }

        /// <summary>
        /// </summary>
        virtual MJET_INDEXLIST MakeManagedIndexList(MJET_SESID sesid, const ::JET_INDEXLIST& _indexlist)
        {
            MJET_INDEXLIST indexlist;
            indexlist.Tableid = MakeManagedTableid( _indexlist.tableid, sesid );
            indexlist.Records = _indexlist.cRecord;

            // types are taken from info.cxx
            bool fASCII = !IsNativeUnicode();
            indexlist.ColumnidIndexName = MakeManagedColumnid( _indexlist.columnidindexname, MJET_COLTYP::Text, fASCII );
            indexlist.ColumnidGrbitIndex = MakeManagedColumnid( _indexlist.columnidgrbitIndex, MJET_COLTYP::Long, false );
            indexlist.ColumnidCColumn = MakeManagedColumnid( _indexlist.columnidcColumn, MJET_COLTYP::Long, false );
            indexlist.ColumnidIColumn = MakeManagedColumnid( _indexlist.columnidiColumn, MJET_COLTYP::Long, false );
            indexlist.ColumnidGrbitColumn = MakeManagedColumnid( _indexlist.columnidgrbitColumn, MJET_COLTYP::Long, false );
            indexlist.ColumnidColumnName = MakeManagedColumnid( _indexlist.columnidcolumnname, MJET_COLTYP::Text, fASCII );
            indexlist.ColumnidLCMapFlags = MakeManagedColumnid( _indexlist.columnidLCMapFlags, MJET_COLTYP::Long, false );

            return indexlist;
        }

        virtual MJET_INDEXCREATE MakeManagedIndexCreate(const ::JET_INDEXCREATE2& _indexCreate)
        {
            MJET_INDEXCREATE ic;

            ic.IndexName = MakeManagedString( _indexCreate.szIndexName );
            ic.Key = MakeManagedString( _indexCreate.szKey, (_indexCreate.cbKey / sizeof(_TCHAR)) - 1 ); // -1 to keep just 1 null-terminator. the next conversion back to an unmanaged string will result in 2 null-terminators (managed strings arent null-terminated)
            ic.KeyLengthMax = _indexCreate.cbKeyMost;
            ic.Grbit = (MJET_GRBIT) _indexCreate.grbit;
            ic.Density = _indexCreate.ulDensity;

            if( JET_bitIndexUnicode & _indexCreate.grbit )
            {
                ic.Lcid = _indexCreate.pidxunicode->lcid;
                ic.MapFlags = _indexCreate.pidxunicode->dwMapFlags;
            }
            else
            {
                ic.Lcid = (Int32) _indexCreate.lcid;
            }

            if( JET_bitIndexTuples & _indexCreate.grbit )
            {
                ic.TupleLimits = gcnew MJET_TUPLELIMITS();
                ic.TupleLimits->LengthMin = _indexCreate.ptuplelimits->chLengthMin;
                ic.TupleLimits->LengthMax = _indexCreate.ptuplelimits->chLengthMax;
                ic.TupleLimits->ToIndexMax = _indexCreate.ptuplelimits->chToIndexMax;
                ic.TupleLimits->OffsetIncrement = _indexCreate.ptuplelimits->cchIncrement;
                ic.TupleLimits->OffsetStart = _indexCreate.ptuplelimits->ichStart;
            }
            else
            {
                ic.VarSegMac = _indexCreate.cbVarSegMac;
            }

            if( _indexCreate.cConditionalColumn > 0 )
            {
                ic.ConditionalColumns = gcnew array<MJET_CONDITIONALCOLUMN>( _indexCreate.cConditionalColumn );
                for(UINT i = 0; i < _indexCreate.cConditionalColumn; i++)
                {
                    ic.ConditionalColumns[i].ColumnName = MakeManagedString( _indexCreate.rgconditionalcolumn[i].szColumnName );
                    ic.ConditionalColumns[i].Grbit = (MJET_GRBIT) _indexCreate.rgconditionalcolumn[i].grbit;
                }
            }

            if( _indexCreate.pSpacehints )
            {
                ic.SpaceHints = MakeManagedSpaceHints( *_indexCreate.pSpacehints );
            }

            return ic;
        }

#if (ESENT_WINXP)
#else
        virtual MJET_RECSIZE MakeManagedRecSize(::JET_RECSIZE2 _recsize)
        {
            MJET_RECSIZE recsize;

            recsize.Data = _recsize.cbData;
            recsize.LongValueData = _recsize.cbLongValueData;
            recsize.Overhead = _recsize.cbOverhead;
            recsize.LongValueOverhead = _recsize.cbLongValueOverhead;
            recsize.NonTaggedColumns = _recsize.cNonTaggedColumns;
            recsize.TaggedColumns = _recsize.cTaggedColumns;
            recsize.LongValues = _recsize.cLongValues;
            recsize.MultiValues = _recsize.cMultiValues;
            recsize.CompressedColumns = _recsize.cCompressedColumns;
            recsize.DataCompressed = _recsize.cbDataCompressed;
            recsize.LongValueDataCompressed = _recsize.cbLongValueDataCompressed;

            return recsize;
        }
#endif

#if (ESENT_PUBLICONLY)
#else
#if (ESENT_WINXP)
#else
        /// <summary>
        /// Convert an unmanaged JET_PAGEINFO to a managed MJET_PAGEINFO structure
        /// </summary>
        virtual MJET_PAGEINFO MakeManagedPageinfo(const ::JET_PAGEINFO2& _pageinfo)
        {
            MJET_PAGEINFO pageinfo;

            pageinfo.PageNumber = _pageinfo.pageInfo.pgno;
            pageinfo.IsInitialized = !!(_pageinfo.pageInfo.fPageIsInitialized);
            pageinfo.IsCorrectableError = !!(_pageinfo.pageInfo.fCorrectableError);
            pageinfo.Dbtime = _pageinfo.pageInfo.dbtime;
            pageinfo.StructureChecksum = _pageinfo.pageInfo.structureChecksum;
            pageinfo.Flags = _pageinfo.pageInfo.flags;

            unsigned __int64 rgchecksumExpected[4];
            rgchecksumExpected[0] = _pageinfo.pageInfo.checksumExpected;
            for( int ichecksum = 0; ichecksum < ( _countof( rgchecksumExpected ) - 1 ); ++ichecksum )
            {
                rgchecksumExpected[ichecksum+1] = _pageinfo.rgChecksumExpected[ichecksum];
            }

            unsigned __int64 rgchecksumActual[4];
            rgchecksumActual[0] = _pageinfo.pageInfo.checksumActual;
            for( int ichecksum = 0; ichecksum < ( _countof( rgchecksumActual ) - 1 ); ++ichecksum )
            {
                rgchecksumActual[ichecksum+1] = _pageinfo.rgChecksumActual[ichecksum];
            }

            pageinfo.ExpectedChecksum = MakeManagedBytes( (BYTE *)rgchecksumExpected, sizeof(rgchecksumExpected) );
            pageinfo.ActualChecksum = MakeManagedBytes( (BYTE *)rgchecksumActual, sizeof(rgchecksumActual) );

            return pageinfo;
        };
#endif
#endif

        /// <summary>
        /// ESE uses a JET_LOGTIME structure to store dates and times. Convert to a standard, managed DateTime
        /// </summary>
        virtual DateTime MakeManagedDateTime(const ::JET_LOGTIME& _logtime)
        {
            try
            {
                System::DateTime datetime(
                    _logtime.bYear+1900,
                    (0 == _logtime.bMonth ) ? 1 : _logtime.bMonth, // 0 when uninit
                    (0 == _logtime.bDay ) ? 1 : _logtime.bDay, // 0 when uninit
                    _logtime.bHours,
                    _logtime.bMinutes,
                    _logtime.bSeconds,
                    (int)(((UInt32)_logtime.bMillisecondsHigh << 7) | (UInt32)_logtime.bMillisecondsLow),
                    _logtime.fTimeIsUTC ? DateTimeKind::Utc : DateTimeKind::Local );

                return datetime;
            }
            catch (ArgumentOutOfRangeException^ e)
            {
                String^ message = String::Format(
                    "Year, Month, Day, Hour, Minutes and Seconds parameters describe an un-representable DateTime: year:{0}, month:{1}, day:{2}, hour:{3}, minutes:{4}, seconds:{5}, milli-seconds:{6} isUtc:{7}.\n",
                    _logtime.bYear,
                    _logtime.bMonth,
                    _logtime.bDay,
                    _logtime.bHours,
                    _logtime.bMinutes,
                    _logtime.bSeconds,
                    (int)(((UInt32)_logtime.bMillisecondsHigh << 7) | (UInt32)_logtime.bMillisecondsLow),
                    _logtime.fTimeIsUTC );

                // Add diagnostic information to the exception so
                // we can actually figure out the values.
                e->Data->Add( "diagnostics", message );
                WCHAR* wszMessage = GetUnmanagedStringW( message );
                OutputDebugStringW( wszMessage );
                FreeUnmanagedStringW( wszMessage );

                throw;
            }
        }

        /// <summary>
        /// ESE uses a JET_LOGTIME structure to store dates and times. Convert to a standard, managed DateTime
        /// </summary>
        virtual ::JET_LOGTIME MakeUnmanagedDateTime(DateTime time)
        {
            ::JET_LOGTIME _logtime = {0};
            _logtime.bYear = (char) (time.Year - 1900);
            _logtime.bMonth = (char) time.Month;
            _logtime.bDay = (char) time.Day;
            _logtime.bHours = (char) time.Hour;
            _logtime.bMinutes = (char) time.Minute;
            _logtime.bSeconds = (char) time.Second;
            _logtime.fTimeIsUTC = ( time.Kind == DateTimeKind::Utc );
            _logtime.bMillisecondsLow = time.Millisecond & 0x7F;
            _logtime.bMillisecondsHigh = (char)((time.Millisecond & 0x380) >> 7);
            return _logtime;
        }

        /// <summary>
        /// </summary>
        virtual MJET_SIGNATURE MakeManagedSignature(const ::JET_SIGNATURE& _signature)
        {
            MJET_SIGNATURE signature;

            signature.Random = _signature.ulRandom;

            UInt64 ui64T = 0;
            ui64T = _signature.logtimeCreate.bFiller2;
            ui64T = (ui64T << 8) | (UCHAR)_signature.logtimeCreate.bFiller1;
            ui64T = (ui64T << 8) | (UCHAR)_signature.logtimeCreate.bYear;
            ui64T = (ui64T << 8) | _signature.logtimeCreate.bMonth;
            ui64T = (ui64T << 8) | _signature.logtimeCreate.bDay;
            ui64T = (ui64T << 8) | _signature.logtimeCreate.bHours;
            ui64T = (ui64T << 8) | _signature.logtimeCreate.bMinutes;
            ui64T = (ui64T << 8) | _signature.logtimeCreate.bSeconds;
            signature.CreationTimeUInt64 = ui64T;

            signature.CreationTime = MakeManagedDateTime( _signature.logtimeCreate );
            return signature;
        }

        /// <summary>
        /// </summary>
        virtual JET_SIGNATURE MakeUnmanagedSignature(MJET_SIGNATURE& signature)
        {
            JET_SIGNATURE _signature;

            _signature.ulRandom = (UInt32)signature.Random;

            UInt64 ui64T = signature.CreationTimeUInt64;
            JET_LOGTIME _jtT;
            _jtT.bSeconds = (BYTE)( ui64T & 0xff );
            ui64T = ( ui64T >> 8 );
            _jtT.bMinutes = (BYTE)( ui64T & 0xff );
            ui64T = ( ui64T >> 8 );
            _jtT.bHours = (BYTE)( ui64T & 0xff );
            ui64T = ( ui64T >> 8 );
            _jtT.bDay = (BYTE)( ui64T & 0xff );
            ui64T = ( ui64T >> 8 );
            _jtT.bMonth = (BYTE)( ui64T & 0xff );
            ui64T = ( ui64T >> 8 );
            _jtT.bYear = (BYTE)( ui64T & 0xff );
            ui64T = ( ui64T >> 8 );
            _jtT.bFiller1 = (BYTE)( ui64T & 0xff );
            ui64T = ( ui64T >> 8 );
            _jtT.bFiller2 = (BYTE)( ui64T & 0xff );
            /// ui64T = ( ui64T >> 8 );
            _signature.logtimeCreate = _jtT;

            for ( int ichT = 0; ichT < JET_MAX_COMPUTERNAME_LENGTH + 1; ichT++ )
            {
                _signature.szComputerName[ichT] = 0;
            }

            return _signature;
        }

        /// <summary>
        /// Given an unmanaged JET_LGPOS structure, return a managed MJET_LGPOS struct.
        /// </summary>
        virtual MJET_LGPOS MakeManagedLgpos(const ::JET_LGPOS& _lgpos)
        {
            MJET_LGPOS lgpos;

            lgpos.Generation = _lgpos.lGeneration;
            lgpos.Sector = _lgpos.isec;
            lgpos.ByteOffset = _lgpos.ib;
            return lgpos;
        }

        /// <summary>
        /// Given a managed MJET_LGPOS structure, return an unmanaged JET_LGPOS struct.
        /// </summary>
        virtual JET_LGPOS MakeUnmanagedLgpos(MJET_LGPOS& lgpos)
        {
            JET_LGPOS _lgpos;

            _lgpos.lGeneration = (long) lgpos.Generation;
            _lgpos.isec = (unsigned short) lgpos.Sector;
            _lgpos.ib = (unsigned short) lgpos.ByteOffset;
            return _lgpos;
        }

        /// <summary>
        /// Given an unmanaged JET_BKINFO structure, return a managed MJET_BKINFO struct.
        /// </summary>
        virtual MJET_BKINFO MakeManagedBkinfo(const ::JET_BKINFO& _bkinfo)
        {
            MJET_BKINFO bkinfo;

            bkinfo.Lgpos = MakeManagedLgpos( _bkinfo.lgposMark );
            bkinfo.BackupTime = MakeManagedDateTime( _bkinfo.logtimeMark );
#if (ESENT_WINXP)
#else
            bkinfo.SnapshotBackup = _bkinfo.bklogtimeMark.fOSSnapshot;
#endif
            bkinfo.LowestGeneration = _bkinfo.genLow;
            bkinfo.HighestGeneration = _bkinfo.genHigh;
            return bkinfo;
        }

        /// <summary>
        /// Given an unmanaged JET_BKINFO structure, return a managed MJET_BKINFO struct.
        /// </summary>
        virtual ::JET_BKINFO MakeUnmanagedBkinfo(MJET_BKINFO bkinfo)
        {
            ::JET_BKINFO _bkinfo;

            _bkinfo.lgposMark = MakeUnmanagedLgpos( bkinfo.Lgpos );
            _bkinfo.logtimeMark = MakeUnmanagedDateTime( bkinfo.BackupTime );
#if (ESENT_WINXP)
#else
            _bkinfo.bklogtimeMark.fOSSnapshot = bkinfo.SnapshotBackup;
#endif
            _bkinfo.genLow = (unsigned long) bkinfo.LowestGeneration;
            _bkinfo.genHigh = (unsigned long) bkinfo.HighestGeneration;
            return _bkinfo;
        }

#if (ESENT_PUBLICONLY)
#else
#if (ESENT_WINXP)
#else
        /// <summary>
        /// Given an unmadaged JET_LOGINFOMISC, return a managed MJET_LOGINFOMISC struct.
        /// </summary>
        virtual MJET_LOGINFOMISC MakeManagedLoginfoMisc(const ::JET_LOGINFOMISC2& _loginfomisc)
        {
            MJET_LOGINFOMISC loginfomisc;

            loginfomisc.Generation = _loginfomisc.ulGeneration;
            loginfomisc.Signature = MakeManagedSignature( _loginfomisc.signLog );
            loginfomisc.CreationTime = MakeManagedDateTime( _loginfomisc.logtimeCreate );
            loginfomisc.PreviousGenerationCreationTime = MakeManagedDateTime( _loginfomisc.logtimePreviousGeneration );
            loginfomisc.Flags = _loginfomisc.ulFlags;
            loginfomisc.VersionMajor = _loginfomisc.ulVersionMajor;
            loginfomisc.VersionMinor = _loginfomisc.ulVersionMinor;
            loginfomisc.VersionUpdate = _loginfomisc.ulVersionUpdate;
            loginfomisc.SectorSize = _loginfomisc.cbSectorSize;
            loginfomisc.HeaderSize = _loginfomisc.cbHeader;
            loginfomisc.FileSize = _loginfomisc.cbFile;
            loginfomisc.DatabasePageSize = _loginfomisc.cbDatabasePageSize;
            loginfomisc.CheckpointGeneration = _loginfomisc.lgposCheckpoint.lGeneration;

            return loginfomisc;
        }
#endif
#endif

        /// <summary>
        /// Given an integer, representing a database state, return a MJET_DBSTATE
        /// </summary>
        virtual MJET_DBSTATE Dbstate(unsigned long dbstate)
        {
            switch( dbstate )
            {
            case JET_dbstateJustCreated:
                return MJET_DBSTATE::JustCreated;
            case JET_dbstateDirtyShutdown:
                return MJET_DBSTATE::DirtyShutdown;
            case JET_dbstateCleanShutdown:
                return MJET_DBSTATE::CleanShutdown;
            case JET_dbstateBeingConverted:
                return MJET_DBSTATE::BeingConverted;
#if (ESENT_PUBLICONLY)
#else
#if (ESENT_WINXP)
#else
            case JET_dbstateIncrementalReseedInProgress:
                return MJET_DBSTATE::IncrementalReseedInProgress;
            case JET_dbstateDirtyAndPatchedShutdown:
                return MJET_DBSTATE::DirtyAndPatchedShutdown;
#endif
#endif
            default:
                return MJET_DBSTATE::Unknown;
            }
        }

    public:

#if (ESENT_WINXP)
        typedef JET_DBINFOMISC  _CURR_DBINFOMISC;
        /// <summary>
        /// Make a managed MJET_DBINFO from an unmanaged byte array containing JET_DBINFOMISC structure
        /// </summary>
        virtual MJET_DBINFO MakeManagedDbinfo(IntPtr _handle, unsigned int _cbconst)
        {
            if ( sizeof( JET_DBINFOMISC ) == _cbconst )
            {
                const ::JET_DBINFOMISC * const _pdbinfomisc = (JET_DBINFOMISC*) _handle.ToPointer();
                return MakeManagedDbinfo( *_pdbinfomisc );
            }

            throw TranslatedException(gcnew IsamInvalidParameterException());;
        }

        /// <summary>
        /// Make a managed MJET_DBINFO from an unmanaged JET_DBINFOMISC structure
        /// </summary>
        virtual MJET_DBINFO MakeManagedDbinfo(const ::JET_DBINFOMISC& _dbinfomisc)
        {
            MJET_DBINFO dbinfo;

            dbinfo.Version = _dbinfomisc.ulVersion;
            dbinfo.Update = _dbinfomisc.ulUpdate;
            dbinfo.Signature = MakeManagedSignature( _dbinfomisc.signDb );

            dbinfo.State = Dbstate( _dbinfomisc.dbstate );

            dbinfo.ConsistentLgpos = MakeManagedLgpos( _dbinfomisc.lgposConsistent );
            dbinfo.ConsistentTime = MakeManagedDateTime( _dbinfomisc.logtimeConsistent );
            dbinfo.AttachLgpos = MakeManagedLgpos( _dbinfomisc.lgposAttach );
            dbinfo.AttachTime = MakeManagedDateTime( _dbinfomisc.logtimeAttach );
            dbinfo.DetachLgpos = MakeManagedLgpos( _dbinfomisc.lgposDetach );
            dbinfo.DetachTime = MakeManagedDateTime( _dbinfomisc.logtimeDetach );

            dbinfo.LogSignature = MakeManagedSignature( _dbinfomisc.signLog );

            dbinfo.LastFullBackup = MakeManagedBkinfo( _dbinfomisc.bkinfoFullPrev );
            dbinfo.LastIncrementalBackup = MakeManagedBkinfo( _dbinfomisc.bkinfoIncPrev );
            dbinfo.CurrentFullBackup = MakeManagedBkinfo( _dbinfomisc.bkinfoFullCur );

            dbinfo.ShadowCatalogDisabled = !!(_dbinfomisc.fShadowingDisabled);
            dbinfo.UpgradeDatabase = !!(_dbinfomisc.fUpgradeDb);

            dbinfo.OSMajorVersion = _dbinfomisc.dwMajorVersion;
            dbinfo.OSMinorVersion = _dbinfomisc.dwMinorVersion;
            dbinfo.OSBuild = _dbinfomisc.dwBuildNumber;
            dbinfo.OSServicePackNumber = _dbinfomisc.lSPNumber;

            dbinfo.DatabasePageSize = _dbinfomisc.cbPageSize;

            return dbinfo;
        }
#else
#if (ESENT_PUBLICONLY)
        typedef JET_DBINFOMISC4 _CURR_DBINFOMISC;
        /// <summary>
        /// Make a managed MJET_DBINFO from an unmanaged byte array containing JET_DBINFOMISC4 structure
        /// </summary>
        virtual MJET_DBINFO MakeManagedDbinfo(IntPtr _handle, unsigned int _cbconst)
        {
            if ( sizeof( JET_DBINFOMISC4 ) == _cbconst )
            {
                const ::JET_DBINFOMISC4 * const _pdbinfomisc = (JET_DBINFOMISC4*) _handle.ToPointer();
                return MakeManagedDbinfo( *_pdbinfomisc );
            }

            throw TranslatedException(gcnew IsamInvalidParameterException());;
        }

        /// <summary>
        /// Make a managed MJET_DBINFO from an unmanaged JET_DBINFOMISC4 structure
        /// </summary>
        virtual MJET_DBINFO MakeManagedDbinfo(const ::JET_DBINFOMISC4& _dbinfomisc)
        {
            MJET_DBINFO dbinfo;

            dbinfo.Version = _dbinfomisc.ulVersion;
            dbinfo.Update = _dbinfomisc.ulUpdate;
            dbinfo.Signature = MakeManagedSignature( _dbinfomisc.signDb );

            dbinfo.State = Dbstate( _dbinfomisc.dbstate );

            dbinfo.ConsistentLgpos = MakeManagedLgpos( _dbinfomisc.lgposConsistent );
            dbinfo.ConsistentTime = MakeManagedDateTime( _dbinfomisc.logtimeConsistent );
            dbinfo.AttachLgpos = MakeManagedLgpos( _dbinfomisc.lgposAttach );
            dbinfo.AttachTime = MakeManagedDateTime( _dbinfomisc.logtimeAttach );
            dbinfo.DetachLgpos = MakeManagedLgpos( _dbinfomisc.lgposDetach );
            dbinfo.DetachTime = MakeManagedDateTime( _dbinfomisc.logtimeDetach );

            dbinfo.LogSignature = MakeManagedSignature( _dbinfomisc.signLog );

            dbinfo.LastFullBackup = MakeManagedBkinfo( _dbinfomisc.bkinfoFullPrev );
            dbinfo.LastIncrementalBackup = MakeManagedBkinfo( _dbinfomisc.bkinfoIncPrev );
            dbinfo.CurrentFullBackup = MakeManagedBkinfo( _dbinfomisc.bkinfoFullCur );

            dbinfo.ShadowCatalogDisabled = !!(_dbinfomisc.fShadowingDisabled);
            dbinfo.UpgradeDatabase = !!(_dbinfomisc.fUpgradeDb);

            dbinfo.OSMajorVersion = _dbinfomisc.dwMajorVersion;
            dbinfo.OSMinorVersion = _dbinfomisc.dwMinorVersion;
            dbinfo.OSBuild = _dbinfomisc.dwBuildNumber;
            dbinfo.OSServicePackNumber = _dbinfomisc.lSPNumber;

            dbinfo.DatabasePageSize = _dbinfomisc.cbPageSize;

            dbinfo.MinimumLogGenerationRequired = _dbinfomisc.genMinRequired;
            dbinfo.MaximumLogGenerationRequired = _dbinfomisc.genMaxRequired;
            dbinfo.MaximumLogGenerationCreationTime = MakeManagedDateTime( _dbinfomisc.logtimeGenMaxCreate );

            dbinfo.RepairCount = _dbinfomisc.ulRepairCount;
            dbinfo.LastRepairTime = MakeManagedDateTime( _dbinfomisc.logtimeRepair );
            dbinfo.RepairCountOld = _dbinfomisc.ulRepairCountOld;

            dbinfo.SuccessfulECCPageFixes = _dbinfomisc.ulECCFixSuccess;
            dbinfo.LastSuccessfulECCPageFix = MakeManagedDateTime( _dbinfomisc.logtimeECCFixSuccess );
            dbinfo.SuccessfulECCPageFixesOld = _dbinfomisc.ulECCFixSuccessOld;

            dbinfo.UnsuccessfulECCPageFixes = _dbinfomisc.ulECCFixFail;
            dbinfo.LastUnsuccessfulECCPageFix = MakeManagedDateTime( _dbinfomisc.logtimeECCFixFail );
            dbinfo.UnsuccessfulECCPageFixesOld = _dbinfomisc.ulECCFixFailOld;

            dbinfo.BadChecksums = _dbinfomisc.ulBadChecksum;
            dbinfo.LastBadChecksum = MakeManagedDateTime( _dbinfomisc.logtimeBadChecksum );
            dbinfo.BadChecksumsOld = _dbinfomisc.ulBadChecksumOld;

            dbinfo.CommittedGeneration = _dbinfomisc.genCommitted;

            dbinfo.LastCopyBackup = MakeManagedBkinfo( _dbinfomisc.bkinfoCopyPrev );
            dbinfo.LastDifferentialBackup = MakeManagedBkinfo( _dbinfomisc.bkinfoDiffPrev );

            return dbinfo;
        }
#else
        typedef JET_DBINFOMISC7 _CURR_DBINFOMISC;
        /// <summary>
        /// Make a managed MJET_DBINFO from an unmanaged byte array containing JET_DBINFOMISC7 structure
        /// </summary>
        virtual MJET_DBINFO MakeManagedDbinfo(IntPtr _handle, unsigned int _cbconst)
        {
            if ( sizeof( JET_DBINFOMISC7 ) == _cbconst )
            {
                const ::JET_DBINFOMISC7 * const _pdbinfomisc = (JET_DBINFOMISC7*) _handle.ToPointer();
                return MakeManagedDbinfo( *_pdbinfomisc );
            }

            throw TranslatedException(gcnew IsamInvalidParameterException());;
        }

        /// <summary>
        /// Turn a managed MJET_DBINFO into an array of bytes. This array can be
        /// converted back into an MJET_DBINFO with MakeManagerDbinfo. Yes, this
        /// is home-made serialization.
        /// </summary>
        virtual array<Byte>^ GetBytes(MJET_DBINFO dbinfo)
        {
            _CURR_DBINFOMISC _dbinfo;

            _dbinfo.ulVersion = (unsigned long) dbinfo.Version;
            _dbinfo.ulUpdate = (unsigned long) dbinfo.Update;
            _dbinfo.signDb = MakeUnmanagedSignature( dbinfo.Signature );

            _dbinfo.dbstate = (unsigned long) dbinfo.State;

            _dbinfo.lgposConsistent = MakeUnmanagedLgpos( dbinfo.ConsistentLgpos );
            _dbinfo.logtimeConsistent = MakeUnmanagedDateTime( dbinfo.ConsistentTime );
            _dbinfo.lgposAttach = MakeUnmanagedLgpos( dbinfo.AttachLgpos );
            _dbinfo.logtimeAttach = MakeUnmanagedDateTime( dbinfo.AttachTime );
            _dbinfo.lgposDetach = MakeUnmanagedLgpos( dbinfo.DetachLgpos );
            _dbinfo.logtimeDetach = MakeUnmanagedDateTime( dbinfo.DetachTime );

            _dbinfo.signLog = MakeUnmanagedSignature( dbinfo.LogSignature );

            _dbinfo.bkinfoFullPrev = MakeUnmanagedBkinfo( dbinfo.LastFullBackup );
            _dbinfo.bkinfoIncPrev = MakeUnmanagedBkinfo( dbinfo.LastIncrementalBackup );
            _dbinfo.bkinfoFullCur = MakeUnmanagedBkinfo( dbinfo.CurrentFullBackup );

            _dbinfo.fShadowingDisabled = dbinfo.ShadowCatalogDisabled ? 1 : 0;
            _dbinfo.fUpgradeDb = dbinfo.UpgradeDatabase ? 1 : 0;

            _dbinfo.dwMajorVersion = (unsigned long) dbinfo.OSMajorVersion;
            _dbinfo.dwMinorVersion = (unsigned long) dbinfo.OSMinorVersion;
            _dbinfo.dwBuildNumber = (unsigned long) dbinfo.OSBuild;
            _dbinfo.lSPNumber = (long) dbinfo.OSServicePackNumber;

            _dbinfo.cbPageSize = (unsigned long) dbinfo.DatabasePageSize;

            _dbinfo.genMinRequired = (unsigned long) dbinfo.MinimumLogGenerationRequired;
            _dbinfo.genMaxRequired = (unsigned long) dbinfo.MaximumLogGenerationRequired;
            _dbinfo.logtimeGenMaxCreate = MakeUnmanagedDateTime( dbinfo.MaximumLogGenerationCreationTime );

            _dbinfo.ulRepairCount = (unsigned long) dbinfo.RepairCount;
            _dbinfo.logtimeRepair = MakeUnmanagedDateTime( dbinfo.LastRepairTime );
            _dbinfo.ulRepairCountOld = (unsigned long) dbinfo.RepairCountOld;

            _dbinfo.ulECCFixSuccess = (unsigned long) dbinfo.SuccessfulECCPageFixes;
            _dbinfo.logtimeECCFixSuccess = MakeUnmanagedDateTime( dbinfo.LastSuccessfulECCPageFix );
            _dbinfo.ulECCFixSuccessOld = (unsigned long) dbinfo.SuccessfulECCPageFixesOld;

            _dbinfo.ulECCFixFail = (unsigned long) dbinfo.UnsuccessfulECCPageFixes;
            _dbinfo.logtimeECCFixFail = MakeUnmanagedDateTime( dbinfo.LastUnsuccessfulECCPageFix );
            _dbinfo.ulECCFixFailOld = (unsigned long) dbinfo.UnsuccessfulECCPageFixesOld;

            _dbinfo.ulBadChecksum = (unsigned long) dbinfo.BadChecksums;
            _dbinfo.logtimeBadChecksum = MakeUnmanagedDateTime( dbinfo.LastBadChecksum );
            _dbinfo.ulBadChecksumOld = (unsigned long) dbinfo.BadChecksumsOld;

            _dbinfo.genCommitted = (unsigned long) dbinfo.CommittedGeneration;

            _dbinfo.bkinfoCopyPrev = MakeUnmanagedBkinfo( dbinfo.LastCopyBackup );
            _dbinfo.bkinfoDiffPrev = MakeUnmanagedBkinfo( dbinfo.LastDifferentialBackup );

            _dbinfo.ulIncrementalReseedCount = (unsigned long) dbinfo.IncrementalReseedCount;
            _dbinfo.logtimeIncrementalReseed = MakeUnmanagedDateTime( dbinfo.LastIncrementalReseedTime );
            _dbinfo.ulIncrementalReseedCountOld = (unsigned long) dbinfo.IncrementalReseedCountOld;

            _dbinfo.ulPagePatchCount = (unsigned long) dbinfo.PagePatchCount;
            _dbinfo.logtimePagePatch = MakeUnmanagedDateTime( dbinfo.LastPagePatchTime );
            _dbinfo.ulPagePatchCountOld = (unsigned long) dbinfo.PagePatchCountOld;

            _dbinfo.logtimeChecksumPrev = MakeUnmanagedDateTime( dbinfo.LastChecksumTime );
            _dbinfo.logtimeChecksumStart = MakeUnmanagedDateTime( dbinfo.ChecksumStartTime );
            _dbinfo.cpgDatabaseChecked = (unsigned long) dbinfo.PageCheckedCount;

            return MakeManagedBytes((BYTE*)&_dbinfo, sizeof(_dbinfo));
        }


        /// <summary>
        /// Make a managed MJET_DBINFO from an unmanaged JET_DBINFOMISC7 structure
        /// </summary>
        virtual MJET_DBINFO MakeManagedDbinfo(const ::JET_DBINFOMISC7& _dbinfomisc)
        {
            MJET_DBINFO dbinfo;

            dbinfo.Version = _dbinfomisc.ulVersion;
            dbinfo.Update = _dbinfomisc.ulUpdate;
            dbinfo.Signature = MakeManagedSignature( _dbinfomisc.signDb );

            dbinfo.State = Dbstate( _dbinfomisc.dbstate );

            dbinfo.ConsistentLgpos = MakeManagedLgpos( _dbinfomisc.lgposConsistent );
            dbinfo.ConsistentTime = MakeManagedDateTime( _dbinfomisc.logtimeConsistent );
            dbinfo.AttachLgpos = MakeManagedLgpos( _dbinfomisc.lgposAttach );
            dbinfo.AttachTime = MakeManagedDateTime( _dbinfomisc.logtimeAttach );
            dbinfo.DetachLgpos = MakeManagedLgpos( _dbinfomisc.lgposDetach );
            dbinfo.DetachTime = MakeManagedDateTime( _dbinfomisc.logtimeDetach );

            dbinfo.LogSignature = MakeManagedSignature( _dbinfomisc.signLog );

            dbinfo.LastFullBackup = MakeManagedBkinfo( _dbinfomisc.bkinfoFullPrev );
            dbinfo.LastIncrementalBackup = MakeManagedBkinfo( _dbinfomisc.bkinfoIncPrev );
            dbinfo.CurrentFullBackup = MakeManagedBkinfo( _dbinfomisc.bkinfoFullCur );

            dbinfo.ShadowCatalogDisabled = !!(_dbinfomisc.fShadowingDisabled);
            dbinfo.UpgradeDatabase = !!(_dbinfomisc.fUpgradeDb);

            dbinfo.OSMajorVersion = _dbinfomisc.dwMajorVersion;
            dbinfo.OSMinorVersion = _dbinfomisc.dwMinorVersion;
            dbinfo.OSBuild = _dbinfomisc.dwBuildNumber;
            dbinfo.OSServicePackNumber = _dbinfomisc.lSPNumber;

            dbinfo.DatabasePageSize = _dbinfomisc.cbPageSize;

            dbinfo.MinimumLogGenerationRequired = _dbinfomisc.genMinRequired;
            dbinfo.MaximumLogGenerationRequired = _dbinfomisc.genMaxRequired;
            dbinfo.MaximumLogGenerationCreationTime = MakeManagedDateTime( _dbinfomisc.logtimeGenMaxCreate );

            dbinfo.RepairCount = _dbinfomisc.ulRepairCount;
            dbinfo.LastRepairTime = MakeManagedDateTime( _dbinfomisc.logtimeRepair );
            dbinfo.RepairCountOld = _dbinfomisc.ulRepairCountOld;

            dbinfo.SuccessfulECCPageFixes = _dbinfomisc.ulECCFixSuccess;
            dbinfo.LastSuccessfulECCPageFix = MakeManagedDateTime( _dbinfomisc.logtimeECCFixSuccess );
            dbinfo.SuccessfulECCPageFixesOld = _dbinfomisc.ulECCFixSuccessOld;

            dbinfo.UnsuccessfulECCPageFixes = _dbinfomisc.ulECCFixFail;
            dbinfo.LastUnsuccessfulECCPageFix = MakeManagedDateTime( _dbinfomisc.logtimeECCFixFail );
            dbinfo.UnsuccessfulECCPageFixesOld = _dbinfomisc.ulECCFixFailOld;

            dbinfo.BadChecksums = _dbinfomisc.ulBadChecksum;
            dbinfo.LastBadChecksum = MakeManagedDateTime( _dbinfomisc.logtimeBadChecksum );
            dbinfo.BadChecksumsOld = _dbinfomisc.ulBadChecksumOld;

            dbinfo.CommittedGeneration = _dbinfomisc.genCommitted;

            dbinfo.LastCopyBackup = MakeManagedBkinfo( _dbinfomisc.bkinfoCopyPrev );
            dbinfo.LastDifferentialBackup = MakeManagedBkinfo( _dbinfomisc.bkinfoDiffPrev );

            dbinfo.IncrementalReseedCount = _dbinfomisc.ulIncrementalReseedCount;
            dbinfo.LastIncrementalReseedTime = MakeManagedDateTime( _dbinfomisc.logtimeIncrementalReseed );
            dbinfo.IncrementalReseedCountOld = _dbinfomisc.ulIncrementalReseedCountOld;

            dbinfo.PagePatchCount = _dbinfomisc.ulPagePatchCount;
            dbinfo.LastPagePatchTime = MakeManagedDateTime( _dbinfomisc.logtimePagePatch );
            dbinfo.PagePatchCountOld = _dbinfomisc.ulPagePatchCountOld;

            dbinfo.LastChecksumTime = MakeManagedDateTime( _dbinfomisc.logtimeChecksumPrev );
            dbinfo.ChecksumStartTime = MakeManagedDateTime( _dbinfomisc.logtimeChecksumStart );
            dbinfo.PageCheckedCount = _dbinfomisc.cpgDatabaseChecked;

            dbinfo.LastReAttachLgpos = MakeManagedLgpos( _dbinfomisc.lgposLastReAttach );
            dbinfo.LastReAttachTime = MakeManagedDateTime( _dbinfomisc.logtimeLastReAttach );

            return dbinfo;
        }
#endif
#endif

        array<MJET_DBINFO>^ MakeManagedDbinfoArray(_CURR_DBINFOMISC* _rgdbinfomisc, int _cdbinfomisc)
        {
            array<MJET_DBINFO>^ dbInfoArray = gcnew array<MJET_DBINFO>( _cdbinfomisc );
            for( int _iDatabaseInfo = 0; _iDatabaseInfo < _cdbinfomisc; ++_iDatabaseInfo )
            {
                dbInfoArray[_iDatabaseInfo] = MakeManagedDbinfo( _rgdbinfomisc[_iDatabaseInfo] );
            }
            return dbInfoArray;
        }

        literal System::Int32 MoveFirst     = 0x80000000;
        literal System::Int32 MovePrevious  = -1;
        literal System::Int32 MoveNext      = 1;
        literal System::Int32 MoveLast      = 0x7fffffff;

    public:

        /// <summary>
        /// Initialize Jet, returning a new instance or throwing an exception
        /// </summary>
        virtual MJET_INSTANCE MJetInit()
        {
            // UNDONE: Why doesn't this next line work?
            // Something like one of these would be ideal:
            // ::JET_INSTANCE _inst = JET_INSTANCE.Nil();
            // ::JET_INSTANCE _inst = JET_instanceNil;
            // Originally this:
            // ::JET_INSTANCE _inst = ~0;
            // gives these errors in Exchange:
            // 1>c:\src\e12\chess\sources\dev\ese\src\interop\jetinterop.cpp(951) : error C2220: warning treated as error - no 'object' file generated
            // 1>c:\src\e12\chess\sources\dev\ese\src\interop\jetinterop.cpp(951) : error C4245: 'initializing' : conversion from 'int' to 'JET_INSTANCE', signed/unsigned mismatch
            // b/c Exchange has better signed/unsigned checking, BUT I think this is a canaray to a real 64-bit issue.
            //
            // Obvious change to:
            // ::JET_INSTANCE _inst = ~((System::UInt64)0);
            // gives these errors:
            // 1>c:\src\e12\chess\sources\dev\ese\src\interop\jetinterop.cpp(951) : error C2220: warning treated as error - no 'object' file generated
            // 1>c:\src\e12\chess\sources\dev\ese\src\interop\jetinterop.cpp(951) : error C4309: 'initializing' : truncation of constant value
            //
            // This however works:
            ::JET_INSTANCE _inst = ~((JET_API_PTR)0);
            // But due to the type def we're using for JET_API_PTR here is this
            // typedef __w64 unsigned long JET_API_PTR;
            // And I can't help but wonder how this works on a 64-bit CLR system?
            // Like this would produce bad parameter passing to the native 64-bit
            // esent.dll/ese.dll, because we'd thing JET_API_PTR is 4 bytes, when
            // on the 64-bit system it is 8 bytes? Interesting.
            //
            // UNDONE: There were several other parameters that I casted as (JET_API_PTR) when I think it is compiling 32-bit ...
            // So the change of this block (inserted in jet_mirror) needs to be reviewed, and fixed right.
            // This does work for 32-bit though.
            // Note: This function is technically not needed, it isn't used anywhere, but some of the other cases the issue still applies.
            //
            // Others tried:
            // ::JET_INSTANCE _inst = (System::Int64)(~((System::Int64)0))
            // ::JET_INSTANCE _inst = (System::UInt64)(~((System::UInt64)0));
            // ::JET_INSTANCE _inst = (~(JET_API_PTR)0);

            Call( JetInit( &_inst ) );
            return MakeManagedInst( _inst );
        }

        /// <summary>
        /// Initialize Jet, returning a new instance or throwing an exception
        /// </summary>
        virtual MJET_INSTANCE MJetInit(MJET_INSTANCE instance)
        {
            ::JET_INSTANCE _inst = GetUnmanagedInst( instance );

            Call( ::JetInit( &_inst ) );
            return MakeManagedInst( _inst );
        }

        /// <summary>
        /// Initialize Jet, returning a new instance or throwing an exception
        /// </summary>
        virtual MJET_INSTANCE MJetInit(MJET_INSTANCE instance, MJET_GRBIT grbit)
        {
            ::JET_INSTANCE _inst = GetUnmanagedInst( instance );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;

            Call( ::JetInit2( &_inst, _grbit ) );
            return MakeManagedInst( _inst );
        }

#if (ESENT_WINXP)
#else
        /// <summary>
        /// Initialize Jet, returning a new instance or throwing an exception.
        /// Databases to recover, a callback for recovery and
        /// the maximum generation to be replayed
        /// (only when performing recovery without undo) may be specified.
        /// </summary>
        virtual MJET_INSTANCE MJetInit(
            MJET_INSTANCE instance,
            array<MJET_RSTMAP>^ rstmaps,
            Int64 genStop,
            MJetInitCallback^ callback,
            MJET_GRBIT grbit)
        {
            ::JET_INSTANCE _inst = GetUnmanagedInst( instance );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            long _crstfilemap = (rstmaps != nullptr) ? rstmaps->Length : 0;
            ::JET_RSTINFO _rstinfo = { 0 };
            MJetInitCallbackInfo^ callbackInfo;
            GCHandle gch;

            try
            {
                JET_ERR err = JET_errSuccess;

                _rstinfo.cbStruct = sizeof(_rstinfo);
                _rstinfo.rgrstmap = (_crstfilemap > 0) ? new ::JET_RSTMAP[_crstfilemap] : NULL;
                _rstinfo.crstmap = _crstfilemap;
                if( _crstfilemap > 0 && NULL == _rstinfo.rgrstmap )
                {
                    throw TranslatedException(gcnew IsamOutOfMemoryException());
                }

                // initialize the array to 0 for exception safety

                if (_crstfilemap > 0)
                {
                    int _irstmap;
                    for( _irstmap = 0; _irstmap < _crstfilemap; ++_irstmap )
                    {
                        _rstinfo.rgrstmap[_irstmap].szDatabaseName = 0;
                        _rstinfo.rgrstmap[_irstmap].szNewDatabaseName = 0;
                    }

                    for( _irstmap = 0; _irstmap< _crstfilemap; ++_irstmap )
                    {
                        _rstinfo.rgrstmap[_irstmap] = GetUnmanagedRstmap( rstmaps[_irstmap] );
                    }
                }

                if ( genStop > 0 )
                {
                    _rstinfo.lgposStop.lGeneration = (long)genStop;
                    _rstinfo.lgposStop.ib = (unsigned short)~0;
                    _rstinfo.lgposStop.isec = (unsigned short)~0;
                }

                if ( nullptr != callback )
                {
                    // store the callback info
                    //
                    callbackInfo = gcnew MJetInitCallbackInfo( this, callback );

                    // the magic from this sample:
                    // http://msdn2.microsoft.com/en-us/library/367eeye0(en-us,vs.80).aspx
                    MJetInitStatusCallbackManagedDelegate ^ delegatefp = gcnew MJetInitStatusCallbackManagedDelegate(callbackInfo, &MJetInitCallbackInfo::MJetInitCallbackOwn);

                    gch = GCHandle::Alloc(delegatefp);

                    ::JET_PFNSTATUS pfn = static_cast<::JET_PFNSTATUS>( Marshal::GetFunctionPointerForDelegate( delegatefp ).ToPointer());

#if DEBUG
                    // In debug-builds only:
                    // force garbage collection cycle to prove
                    // that the delegate doesn't get disposed
                    GC::Collect();
#endif

                    _rstinfo.pfnStatus = pfn;
                }

                err = ::JetInit3( &_inst, &_rstinfo, _grbit );

                if ( err == JET_errCallbackFailed )
                {
                    //System::Diagnostics::Debug::Assert( nullptr != callback );
                    // Package the exception into an Exception. It will be present as
                    // an Inner Exception. If it is just re-thrown, the exception
                    // code is accurate, but the embedded callstack will be lost,
                    // substituted with the current location.

                    throw gcnew Exception( "CallbackFailed", callbackInfo->GetException() );
                }

                Call( err );
            }
            __finally
            {
                if( _rstinfo.rgrstmap )
                {
                    int _irstmap;
                    for( _irstmap = 0; _irstmap < _crstfilemap; ++_irstmap )
                    {
                        FreeUnmanagedRstmap( _rstinfo.rgrstmap[_irstmap] );
                    }
                    delete [] _rstinfo.rgrstmap;
                }

                if ( gch.IsAllocated )
                {
                    gch.Free();
                }

            }
            return MakeManagedInst( _inst );
        }

        /// <summary>
        /// Initialize Jet, returning a new instance or throwing an exception.
        /// Databases to recover and the maximum generation to be replayed
        /// (only when performing recovery without undo) may be specified.
        /// </summary>
        virtual MJET_INSTANCE MJetInit(
            MJET_INSTANCE instance,
            array<MJET_RSTMAP>^ rstmaps,
            Int64 /*genStop*/,
            MJET_GRBIT grbit)
        {
            return MJetInit( instance, rstmaps, 0, nullptr, grbit );
        }

        /// <summary>
        /// Initialize Jet, returning a new instance or throwing an exception.
        /// Databases to recover may be specified.
        /// </summary>
        virtual MJET_INSTANCE MJetInit(
            MJET_INSTANCE instance,
            array<MJET_RSTMAP>^ rstmaps,
            MJET_GRBIT grbit)
        {
            return MJetInit( instance, rstmaps, 0, nullptr, grbit );
        }
#endif

        /// <summary>
        /// Create a new instance, returning the instance or throwing an exception
        /// </summary>
        virtual MJET_INSTANCE MJetCreateInstance(String^ name)
        {
            ::JET_INSTANCE _inst = 0;
            _TCHAR * _szInstanceName = 0;

            try
            {
                _szInstanceName = GetUnmanagedString(name);
                Call( ::JetCreateInstance( &_inst, _szInstanceName ) );
            }
            __finally
            {
                FreeUnmanagedString( _szInstanceName );
            }
            return MakeManagedInst( _inst );
        }

        /// <summary>
        /// Create a new instance, returning the instance or throwing an exception
        /// </summary>
        virtual MJET_INSTANCE MJetCreateInstance(
            String^ name,
            String^ displayName,
            MJET_GRBIT grbit)
        {
            ::JET_INSTANCE _inst = 0;
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            _TCHAR * _szInstanceName = 0;
            _TCHAR * _szDisplayName = 0;

            try
            {
                _szInstanceName = GetUnmanagedString(name);
                _szDisplayName = GetUnmanagedString(displayName);
                Call( ::JetCreateInstance2( &_inst, _szInstanceName, _szDisplayName, _grbit ) );
            }
            __finally
            {
                FreeUnmanagedString( _szInstanceName );
                FreeUnmanagedString( _szDisplayName );
            }
            return MakeManagedInst( _inst );
        }

        /// <summary>
        /// </summary>
        virtual MJET_INSTANCE MJetCreateInstance(String^ name, String^ displayName)
        {
            return MJetCreateInstance( name, displayName, MJET_GRBIT::NoGrbit );
        }

        /// <summary>
        /// Terminate an instance
        /// </summary>
        virtual void MJetTerm()
        {
            ::JET_INSTANCE _inst = ~((JET_API_PTR)0);
            ::JetTerm( _inst );
        }

        /// <summary>
        /// Terminate an instance
        /// </summary>
        virtual void MJetTerm(MJET_INSTANCE instance)
        {
            ::JET_INSTANCE _inst = GetUnmanagedInst( instance );
            ::JetTerm( _inst );
        }

        /// <summary>
        /// Terminate an instance
        /// </summary>
        virtual void MJetTerm(MJET_INSTANCE instance, MJET_GRBIT grbit)
        {
            ::JET_INSTANCE _inst = GetUnmanagedInst( instance );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            ::JetTerm2( _inst, _grbit );
        }

        /// <summary>
        /// </summary>
        virtual void MJetStopService()
        {
            ::JetStopService();
        }

        /// <summary>
        /// </summary>
        virtual void MJetStopServiceInstance(MJET_INSTANCE instance)
        {
            ::JET_INSTANCE _inst = GetUnmanagedInst( instance );
            ::JetStopServiceInstance( _inst );
        }

        /// <summary>
        /// </summary>
        virtual void MJetStopBackup()
        {
            ::JetStopBackup();
        }

        /// <summary>
        /// </summary>
        virtual void MJetStopBackupInstance(MJET_INSTANCE instance)
        {
            ::JET_INSTANCE _inst = GetUnmanagedInst( instance );
            ::JetStopBackupInstance( _inst );
        }

        /// <summary>
        /// </summary>
        virtual void MJetSetSystemParameter(
            MJET_INSTANCE instance,
            MJET_SESID sesid,
            MJET_PARAM paramid,
            Int64 param,
            String^ pstr)
        {
            ::JET_INSTANCE _inst = GetUnmanagedInst( instance );
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            unsigned long _paramid = (unsigned long) paramid;
            JET_API_PTR _plParam = (JET_API_PTR)param;
            _TCHAR * _sz = 0;

            try
            {
                _sz = GetUnmanagedString( pstr );
                Call( ::JetSetSystemParameter( &_inst, _sesid, _paramid, _plParam, _sz ) );
            }
            __finally
            {
                FreeUnmanagedString( _sz );
            }
        }

        /// <summary>
        /// </summary>
        virtual void MJetSetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            Int64 param)
        {
            MJET_SESID sesid = MakeManagedSesid( ~(JET_API_PTR)0, instance );
            MJetSetSystemParameter( instance, sesid, paramid, param, L"" );
        }

        /// <summary>
        /// </summary>
        virtual void MJetSetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            String^ pstr)
        {
            MJET_SESID sesid = MakeManagedSesid( ~(JET_API_PTR)0, instance );
            MJetSetSystemParameter( instance, sesid, paramid, 0, pstr );
        }

        /// <summary>
        /// </summary>
        virtual void MJetSetSystemParameter(MJET_PARAM paramid, Int64 param)
        {
            MJET_INSTANCE instance = MakeManagedInst( ~(JET_API_PTR)0 );
            MJetSetSystemParameter( instance, paramid, param );
        }

        /// <summary>
        /// </summary>
        virtual void MJetSetSystemParameter(MJET_PARAM paramid, String^ pstr)
        {
            MJET_INSTANCE instance = MakeManagedInst( ~(JET_API_PTR)0 );
            MJetSetSystemParameter( instance, paramid, pstr );
        }

        /// <summary>
        /// </summary>
        virtual void MJetGetSystemParameter(
            MJET_INSTANCE instance,
            MJET_SESID sesid,
            MJET_PARAM paramid,
            Int64% param,
            String^% pstr)
        {
            ::JET_INSTANCE _inst = GetUnmanagedInst( instance );
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            unsigned long _paramid = (unsigned long) paramid;
            JET_API_PTR _plParam = 0;
            const int _cbMax = 1024;
            _TCHAR _sz[_cbMax];

            Call( ::JetGetSystemParameter( _inst, _sesid, _paramid, &_plParam, _sz, _cbMax ) );
            param = (System::Int64) _plParam;
            _sz[_cbMax-1] = 0;
            pstr = MakeManagedString( _sz );
        }

        /// <summary>
        /// </summary>
        virtual void MJetGetSystemParameter(
            MJET_INSTANCE instance,
            MJET_SESID sesid,
            MJET_PARAM paramid,
            Int64% param)
        {
            ::JET_INSTANCE _inst = GetUnmanagedInst( instance );
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            unsigned long _paramid = (unsigned long) paramid;
            JET_API_PTR _plParam = 0;

            Call( ::JetGetSystemParameter( _inst, _sesid, _paramid, &_plParam, NULL, sizeof(_plParam) ) );
            param = (System::Int64) _plParam;
        }

        /// <summary>
        /// </summary>
        virtual void MJetGetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            Int64% param)
        {
            MJET_SESID sesid = MakeManagedSesid( ~(JET_API_PTR)0, instance );
            MJetGetSystemParameter( instance, sesid, paramid, param );
        }

        /// <summary>
        /// </summary>
        virtual void MJetGetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            String^% pstr)
        {
            MJET_SESID sesid = MakeManagedSesid( ~(JET_API_PTR)0, instance );
            System::Int64 param = 0;
            MJetGetSystemParameter( instance, sesid, paramid, param, pstr );
        }

        /// <summary>
        /// </summary>
        virtual void MJetGetSystemParameter(MJET_PARAM paramid, Int64% param)
        {
            MJET_INSTANCE instance = MakeManagedInst( ~(JET_API_PTR)0 );
            MJetGetSystemParameter( instance, paramid, param );
        }

        /// <summary>
        /// </summary>
        virtual void MJetGetSystemParameter(MJET_PARAM paramid, String^% pstr)
        {
            MJET_INSTANCE instance = MakeManagedInst( ~(JET_API_PTR)0 );
            MJetGetSystemParameter( instance, paramid, pstr );
        }

#if (ESENT_PUBLICONLY)
#else
#if (ESENT_WINXP)
#else
        /// <summary>
        /// </summary>
        virtual void MJetGetResourceParam(
            MJET_INSTANCE instance,
            MJET_RESOPER resoper,
            MJET_RESID resid,
            Int64% param)
        {
            ::JET_INSTANCE _inst = GetUnmanagedInst( instance );
            ::JET_RESID _resid = (::JET_RESID) resid;
            ::JET_RESOPER _resoper = (::JET_RESOPER) resoper;
            JET_API_PTR _plParam = 0;

            Call( ::JetGetResourceParam( _inst, _resoper, _resid, &_plParam ) );
            param = (System::Int64) _plParam;
        }
#endif
#endif

        /*

        UNDONE

        JET_ERR JET_API MJetSetResourceParam(
        JET_INSTANCE    instance,
        JET_RESOPER     resoper,
        JET_RESID       resid,
        JET_API_PTR     ulParam );

        JET_ERR JET_API MJetEnableMultiInstance(    JET_SETSYSPARAM *   psetsysparam,
        unsigned long       csetsysparam,
        unsigned long *     pcsetsucceed);

        */

#if (ESENT_PUBLICONLY)
#else
        /// <summary>
        /// </summary>
        virtual void MJetResetCounter(MJET_SESID sesid, Int32 CounterType)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            long _type = CounterType;
            ::JetResetCounter( _sesid, _type );
        }

        /// <summary>
        /// </summary>
        virtual Int32 MJetGetCounter(MJET_SESID sesid, Int32 CounterType)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            long _type = CounterType;
            long _value = 0;
            ::JetGetCounter( _sesid, _type, &_value );
            System::Int32 value = _value;
            return value;
        }
#endif

#if (ESENT_PUBLICONLY)
#else
#if (ESENT_WINXP)
#else
        /// <summary>
        /// </summary>
        virtual void MJetTestHookEnableFaultInjection(
            Int32 id,
            JET_ERR err,
            Int32 ulProbability,
            MJET_GRBIT grbit)
        {
            ::JET_TESTHOOKTESTINJECTION _params;

            _params.cbStruct = sizeof( _params );
            _params.ulID = id;
            _params.pv = ( ::JET_API_PTR )err;
            _params.type = JET_TestInjectFault;
            _params.ulProbability = ulProbability;
            _params.grbit = ( ::JET_GRBIT ) grbit;

            Call( ::JetTestHook( opTestHookTestInjection, &_params ) );
        }

        /// <summary>
        /// </summary>
        virtual void MJetTestHookEnableConfigOverrideInjection(
            Int32 id,
            Int64 ulOverride,
            Int32 ulProbability,
            MJET_GRBIT grbit)
        {
            ::JET_TESTHOOKTESTINJECTION _params;

            _params.cbStruct = sizeof( _params );
            _params.ulID = id;
            _params.pv = ( ::JET_API_PTR )ulOverride;
            _params.type = JET_TestInjectConfigOverride;
            _params.ulProbability = ulProbability;
            _params.grbit = ( ::JET_GRBIT ) grbit;

            Call( ::JetTestHook( opTestHookTestInjection, &_params ) );
        }

        /// <summary>
        /// </summary>
        virtual IntPtr MJetTestHookEnforceContextFail(
            IntPtr callback)
        {
            ::JET_TESTHOOKAPIHOOKING _params;
            _params.cbStruct = sizeof( _params );
            _params.pfnNew = callback.ToPointer();
            Call( ::JetTestHook( opTestHookEnforceContextFail, &_params ) );
            return IntPtr( const_cast<void *>( _params.pfnOld ) );
        }
        
        /// <summary>
        /// </summary>
        virtual void MJetTestHookEvictCachedPage(
            MJET_DBID dbid,
            Int32 pgno)
        {
            ::JET_TESTHOOKEVICTCACHE _evicttarget;

            _evicttarget.cbStruct = sizeof( _evicttarget );
            _evicttarget.ulTargetContext = ( ::JET_API_PTR ) GetUnmanagedDbid( dbid );
            _evicttarget.ulTargetData = ( ::JET_API_PTR ) pgno;
            _evicttarget.grbit = ( ::JET_GRBIT ) JET_bitTestHookEvictDataByPgno;

            Call( ::JetTestHook( opTestHookEvictCache, &_evicttarget ) );
        }

        /// <summary>
        /// </summary>
        virtual void MJetSetNegativeTesting(
            MJET_NEGATIVE_TESTING_BITS bits)
        {
            DWORD dw = (DWORD)bits;
            Call( ::JetTestHook( opTestHookSetNegativeTesting, &dw ) );
        }

        /// <summary>
        /// </summary>
        virtual void MJetResetNegativeTesting(
            MJET_NEGATIVE_TESTING_BITS bits)
        {
            DWORD dw = (DWORD)bits;
            Call( ::JetTestHook( opTestHookResetNegativeTesting, &dw ) );
        }

#endif
#endif

        /// <summary>
        /// </summary>
        virtual MJET_SESID MJetBeginSession(
            MJET_INSTANCE instance,
            String^ user,
            String^ password)
        {
            ::JET_INSTANCE _inst = GetUnmanagedInst( instance );
            ::JET_SESID _sesid;
            _TCHAR * _szUserName = 0;
            _TCHAR * _szPassword = 0;
            try
            {
                _szUserName = GetUnmanagedString(user);
                _szPassword = GetUnmanagedString(password);
                Call( ::JetBeginSession( _inst, &_sesid, _szUserName, _szPassword ) );
            }
            __finally
            {
                FreeUnmanagedString( _szUserName );
                FreeUnmanagedString( _szPassword );
            }

            return MakeManagedSesid( _sesid, instance );
        }

        /// <summary>
        /// </summary>
        virtual MJET_SESID MJetBeginSession(MJET_INSTANCE instance)
        {
            return MJetBeginSession( instance, L"", L"" );
        }

        /// <summary>
        /// </summary>
        virtual MJET_SESID MJetDupSession(MJET_SESID sesid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_SESID _sesidNew;
            Call( ::JetDupSession( _sesid, &_sesidNew ) );
            return MakeManagedSesid( _sesidNew, sesid.Instance );
        }

        /// <summary>
        /// </summary>
        virtual void MJetEndSession(MJET_SESID sesid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            Call( ::JetEndSession( _sesid, _grbit ) );
        }

        /// <summary>
        /// </summary>
        virtual void MJetEndSession(MJET_SESID sesid)
        {
            MJetEndSession( sesid, MJET_GRBIT::NoGrbit );
        }

        /// <summary>
        /// </summary>
        virtual Int64 MJetGetVersion(MJET_SESID sesid)
        {
            System::Int64 version;
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            unsigned long _version;
            Call( ::JetGetVersion( _sesid, &_version ) );
            version = _version;
            return version;
        }

        /// <summary>
        /// </summary>
        virtual MJET_WRN MJetIdle(MJET_SESID sesid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;

            MJET_WRN wrn = CallW( ::JetIdle( _sesid, _grbit ) );
            return wrn;
        }

        /// <summary>
        /// </summary>
        virtual MJET_DBID MJetCreateDatabase(
            MJET_SESID sesid,
            String^ file,
            String^ connect,
            MJET_GRBIT grbit,
            MJET_WRN% wrn)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            _TCHAR * _szFilename = 0;
            _TCHAR * _szConnect = 0;
            ::JET_DBID _dbid;
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;

            try
            {
                _szFilename = GetUnmanagedString(file);
                _szConnect = GetUnmanagedString(connect);
                wrn = CallW( ::JetCreateDatabase( _sesid, _szFilename, _szConnect, &_dbid, _grbit ) );
            }
            __finally
            {
                FreeUnmanagedString( _szFilename );
                FreeUnmanagedString( _szConnect );
            }
            return MakeManagedDbid( _dbid );
        }

        /// <summary>
        /// </summary>
        virtual MJET_DBID MJetCreateDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit,
            MJET_WRN% wrn)
        {
            return MJetCreateDatabase( sesid, file, L"", grbit, wrn );
        }

        /// <summary>
        /// </summary>
        virtual MJET_DBID MJetCreateDatabase(MJET_SESID sesid, String^ file)
        {
            MJET_WRN wrn;
            return MJetCreateDatabase( sesid, file, MJET_GRBIT::NoGrbit, wrn );
        }

        /// <summary>
        /// </summary>
        virtual MJET_DBID MJetCreateDatabase(
            MJET_SESID sesid,
            String^ file,
            Int64 maxPages,
            MJET_GRBIT grbit,
            MJET_WRN% wrn)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            _TCHAR * _szFilename = 0;
            const unsigned long _cpgMax = (unsigned long)maxPages;
            ::JET_DBID _dbid;
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;

            try
            {
                _szFilename = GetUnmanagedString(file);
                wrn = CallW( ::JetCreateDatabase2( _sesid, _szFilename, _cpgMax, &_dbid, _grbit ) );
            }
            __finally
            {
                FreeUnmanagedString( _szFilename );
            }
            return MakeManagedDbid( _dbid );
        }

        /// <summary>
        /// </summary>
        virtual MJET_WRN MJetAttachDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            _TCHAR * _szFilename = 0;
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            MJET_WRN wrn;
            try
            {
                _szFilename = GetUnmanagedString( file );
                wrn = CallW( ::JetAttachDatabase( _sesid, _szFilename, _grbit ) );
            }
            __finally
            {
                FreeUnmanagedString( _szFilename );
            }
            return wrn;
        }

        /// <summary>
        /// </summary>
        virtual MJET_WRN MJetAttachDatabase(MJET_SESID sesid, String^ file)
        {
            return MJetAttachDatabase( sesid, file, MJET_GRBIT::NoGrbit );
        }

        /// <summary>
        /// </summary>
        virtual MJET_WRN MJetAttachDatabase(
            MJET_SESID sesid,
            String^ file,
            Int64 maxPages,
            MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            _TCHAR * _szFilename = 0;
            const unsigned long _cpgMax = (unsigned long)maxPages;
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            MJET_WRN wrn;
            try
            {
                _szFilename = GetUnmanagedString( file );
                wrn = CallW( ::JetAttachDatabase2( _sesid, _szFilename, _cpgMax, _grbit ) );
            }
            __finally
            {
                FreeUnmanagedString( _szFilename );
            }
            return wrn;
        }

        /// <summary>
        /// </summary>
        virtual MJET_WRN MJetDetachDatabase(MJET_SESID sesid, String^ file)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            _TCHAR * _szFilename = 0;
            MJET_WRN wrn;

            try
            {
                _szFilename = GetUnmanagedString( file );
                wrn = CallW( ::JetDetachDatabase( _sesid, _szFilename ) );
            }
            __finally
            {
                FreeUnmanagedString( _szFilename );
            }
            return wrn;
        }

        /// <summary>
        /// </summary>
        virtual MJET_WRN MJetDetachDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            _TCHAR * _szFilename = 0;
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            MJET_WRN wrn;
            try
            {
                _szFilename = GetUnmanagedString( file );
                wrn = CallW( ::JetDetachDatabase2( _sesid, _szFilename, _grbit ) );
            }
            __finally
            {
                FreeUnmanagedString( _szFilename );
            }
            return wrn;
        }

        /// <summary>
        /// </summary>
        virtual MJET_OBJECTINFO MJetGetTableInfo(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_OBJECTINFO _objectinfo;

            _objectinfo.cbStruct = sizeof( _objectinfo );

            Call( ::JetGetTableInfo( _sesid, _tableid, &_objectinfo, sizeof( _objectinfo ), JET_TblInfo ) );

            return MakeManagedObjectInfo( _objectinfo );
        }

        /// <summary>
        /// </summary>
        virtual String^ MJetGetTableInfoName(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            _TCHAR _szName[JET_cbNameMost+1];

            memset( _szName, 0, sizeof( _szName ) );

            Call( ::JetGetTableInfo( _sesid, _tableid, _szName, sizeof( _szName ), JET_TblInfoName ) );

            return MakeManagedString( _szName );
        }

        /// <summary>
        /// </summary>
        virtual int MJetGetTableInfoInitialSize(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            unsigned long _rgsize[ 2 ];

            Call( ::JetGetTableInfo( _sesid, _tableid, _rgsize, sizeof( _rgsize ), JET_TblInfoSpaceAlloc ) );

            return _rgsize[ 0 ];
        }

        /// <summary>
        /// </summary>
        virtual int MJetGetTableInfoDensity(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            unsigned long _rgsize[ 2 ];

            Call( ::JetGetTableInfo( _sesid, _tableid, _rgsize, sizeof( _rgsize ), JET_TblInfoSpaceAlloc ) );

            return _rgsize[ 1 ];
        }

        /// <summary>
        /// </summary>
        virtual String^ MJetGetTableInfoTemplateTable(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            _TCHAR _szName[JET_cbNameMost+1];

            memset( _szName, 0, sizeof( _szName ) );

            Call( ::JetGetTableInfo( _sesid, _tableid, _szName, sizeof( _szName ), JET_TblInfoTemplateTableName ) );

            return MakeManagedString( _szName );
        }

        /// <summary>
        /// </summary>
        virtual int MJetGetTableInfoOwnedPages(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            unsigned long _cpg;

            Call( ::JetGetTableInfo( _sesid, _tableid, &_cpg, sizeof( _cpg ), JET_TblInfoSpaceOwned ) );

            return _cpg;
        }

        /// <summary>
        /// </summary>
        virtual int MJetGetTableInfoAvailablePages(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            unsigned long _cpg;

            Call( ::JetGetTableInfo( _sesid, _tableid, &_cpg, sizeof( _cpg ), JET_TblInfoSpaceAvailable ) );

            return _cpg;
        }

        /// <summary>
        /// </summary>
        virtual MJET_OBJECTINFO MJetGetObjectInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ object)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            ::JET_OBJECTINFO _objectinfo;
            _TCHAR * _szObject = 0;

            _objectinfo.cbStruct = sizeof( _objectinfo );

            try
            {
                _szObject = GetUnmanagedString( object );
                Call( ::JetGetObjectInfo( _sesid, _dbid, JET_objtypNil, NULL, _szObject, &_objectinfo, sizeof( _objectinfo ), JET_ObjInfo ) );
            }
            __finally
            {
                FreeUnmanagedString( _szObject );
            }
            return MakeManagedObjectInfo( _objectinfo );
        }

        /// <summary>
        /// </summary>
        virtual MJET_OBJECTINFO MJetGetObjectInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_OBJTYP objtyp,
            String^ container,
            String^ object)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            ::JET_OBJECTINFO _objectinfo;
            ::JET_OBJTYP _objtyp = ( ::JET_OBJTYP ) objtyp;
            _TCHAR * _szContainer = 0;
            _TCHAR * _szObject = 0;

            _objectinfo.cbStruct = sizeof( _objectinfo );

            try
            {
                _szContainer = GetUnmanagedString( container );
                _szObject = GetUnmanagedString( object );
                Call( ::JetGetObjectInfo( _sesid, _dbid, _objtyp, _szContainer, _szObject, &_objectinfo, sizeof( _objectinfo ), JET_ObjInfo ) );
            }
            __finally
            {
                FreeUnmanagedString( _szObject );
                FreeUnmanagedString( _szContainer );
            }
            return MakeManagedObjectInfo( _objectinfo );
        }

        /// <summary>
        /// </summary>
        virtual MJET_OBJECTLIST MJetGetObjectInfoList(MJET_SESID sesid, MJET_DBID dbid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            ::JET_OBJECTLIST _objectlist;

            _objectlist.cbStruct = sizeof( _objectlist );

            Call( ::JetGetObjectInfo( _sesid, _dbid, JET_objtypNil, NULL, NULL, &_objectlist, sizeof( _objectlist ), JET_ObjInfoListNoStats ) );
            return MakeManagedObjectList( sesid, _objectlist );
        }

        /// <summary>
        /// </summary>
        virtual MJET_OBJECTLIST MJetGetObjectInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_OBJTYP objtyp)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            ::JET_OBJTYP _objtyp = ( ::JET_OBJTYP ) objtyp;
            ::JET_OBJECTLIST _objectlist;

            _objectlist.cbStruct = sizeof( _objectlist );

            Call( ::JetGetObjectInfo( _sesid, _dbid, _objtyp, NULL, NULL, &_objectlist, sizeof( _objectlist ), JET_ObjInfoListNoStats ) );
            return MakeManagedObjectList( sesid, _objectlist );
        }

        /// <summary>
        /// </summary>
        virtual MJET_OBJECTLIST MJetGetObjectInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_OBJTYP objtyp,
            String^ container)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            ::JET_OBJTYP _objtyp = ( ::JET_OBJTYP ) objtyp;
            _TCHAR * _szContainer = 0;
            ::JET_OBJECTLIST _objectlist;

            _objectlist.cbStruct = sizeof( _objectlist );

            try
            {
                _szContainer = GetUnmanagedString( container );
                Call( ::JetGetObjectInfo( _sesid, _dbid, _objtyp, _szContainer, NULL, &_objectlist, sizeof( _objectlist ), JET_ObjInfoListNoStats ) );
            }
            __finally
            {
                FreeUnmanagedString( _szContainer );
            }
            return MakeManagedObjectList( sesid, _objectlist );
        }

        /// <summary>
        /// </summary>
        virtual MJET_OBJECTLIST MJetGetObjectInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_OBJTYP objtyp,
            String^ container,
            String^ object)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            ::JET_OBJTYP _objtyp = ( ::JET_OBJTYP ) objtyp;
            _TCHAR * _szContainer = 0;
            _TCHAR * _szObject = 0;
            ::JET_OBJECTLIST _objectlist;

            _objectlist.cbStruct = sizeof( _objectlist );

            try
            {
                _szContainer = GetUnmanagedString( container );
                _szObject = GetUnmanagedString( object );
                Call( ::JetGetObjectInfo( _sesid, _dbid, _objtyp, _szContainer, _szObject, &_objectlist, sizeof( _objectlist ), JET_ObjInfoListNoStats ) );
            }
            __finally
            {
                FreeUnmanagedString( _szObject );
                FreeUnmanagedString( _szContainer );
            }
            return MakeManagedObjectList( sesid, _objectlist );
        }

#if (ESENT_PUBLICONLY)
#else
#if (ESENT_WINXP)
#else
        /// <summary>
        /// The JetGetInstanceInfo function retrieves information about the instances that are running.
        /// </summary>
        /// <remarks>
        /// If there are no active instances, MJetGetInstanceInfo will return an array with 0 elements.
        /// </remarks>
        virtual array<MJET_INSTANCE_INFO>^ MJetGetInstanceInfo()
        {
            array<MJET_INSTANCE_INFO>^ instances = nullptr;
            unsigned long _cInstanceInfo = 0;
            ::JET_INSTANCE_INFO* _pInstanceInfo = NULL;

            try
            {
                Call( ::JetGetInstanceInfo(
                    &_cInstanceInfo,
                    &_pInstanceInfo ) );
                instances = gcnew array<MJET_INSTANCE_INFO>( _cInstanceInfo );
                for ( unsigned long _iInstanceInfo = 0; _iInstanceInfo < _cInstanceInfo; _iInstanceInfo++ )
                {
                    instances[ _iInstanceInfo ].Instance = MakeManagedInst( _pInstanceInfo[ _iInstanceInfo ].hInstanceId );
                    instances[ _iInstanceInfo ].InstanceName = MakeManagedString( _pInstanceInfo[ _iInstanceInfo ].szInstanceName );
                    const int cDatabases = static_cast<int>( _pInstanceInfo[ _iInstanceInfo ].cDatabases );
                    const ::JET_API_PTR cDatabasesApiPtr = static_cast<::JET_API_PTR>( cDatabases );
                    if ( cDatabases < 0 || cDatabasesApiPtr != _pInstanceInfo[ _iInstanceInfo ].cDatabases )
                    {
                        throw TranslatedException(gcnew IsamOutOfMemoryException());
                    }
                    instances[ _iInstanceInfo ].DatabaseFileName = gcnew array<System::String^>( cDatabases );
                    instances[ _iInstanceInfo ].DatabaseDisplayName = gcnew array<System::String^>( cDatabases );
                    for ( int _iDatabase = 0; _iDatabase < cDatabases; _iDatabase++ )
                    {
                        instances[ _iInstanceInfo ].DatabaseFileName[ _iDatabase ] = MakeManagedString( _pInstanceInfo[ _iInstanceInfo ].szDatabaseFileName[ _iDatabase ] );
                        instances[ _iInstanceInfo ].DatabaseDisplayName[ _iDatabase ] = MakeManagedString( _pInstanceInfo[ _iInstanceInfo ].szDatabaseDisplayName[ _iDatabase ] );
                    }
                }
            }
            __finally
            {
                if ( _pInstanceInfo != NULL )
                {
                    Call( ::JetFreeBuffer( ( char * )_pInstanceInfo ) );
                }
            }

            return instances;
        }

        /// <summary>
        /// Given raw page data, return information about those pages
        /// </summary>
        /// <remarks>
        /// The unmanaged API takes multiple pages at one time. We simplify this API by returning
        /// information for only one page at a time.
        /// </remarks>
        virtual MJET_PAGEINFO MJetGetPageInfo(array<Byte>^ page, Int64 PageNumber)
        {
            void * _pvData = NULL;
            int _cbData = 0;
            ::JET_PAGEINFO2 _pageinfo;

            __int64 _pageNumber = PageNumber;
            _pageinfo.pageInfo.pgno = static_cast<unsigned long>( _pageNumber );

            try
            {
                // ESE requires the data to be aligned
                GetAlignedUnmanagedBytes( page, &_pvData, &_cbData );

                Call( ::JetGetPageInfo2(
                    _pvData,
                    _cbData,
                    &_pageinfo,
                    sizeof( _pageinfo ),
                    0,
                    JET_PageInfo2 ) );
            }
            __finally
            {
                FreeAlignedUnmanagedBytes( _pvData );
            }

            return MakeManagedPageinfo( _pageinfo );
        }

        /// <summary>
        /// </summary>
        virtual MJET_LOGINFOMISC MJetGetLogFileInfo(String^ logfile)
        {
            ::JET_LOGINFOMISC2 _loginfomisc;
            _TCHAR * _szLogfile = 0;
            try
            {
                _szLogfile = GetUnmanagedString(logfile);
                Call( ::JetGetLogFileInfo( _szLogfile, &_loginfomisc, sizeof( _loginfomisc ), JET_LogInfoMisc2 ) );
            }
            __finally
            {
                FreeUnmanagedString( _szLogfile );
            }

            try
            {
                return MakeManagedLoginfoMisc( _loginfomisc );
            }
            catch (ArgumentOutOfRangeException^ ex)
            {
                // UNDONE:
                // Catch in debug; so that we don't accidentally think that ESE is
                // returning a -501, but it's introduced by this managed code layer.
                // Assert( 0 );

                String^ message = String::Format("Corrupt log file: {0}", logfile);
                throw gcnew IsamLogFileCorruptException(message, ex);
            }
        }
#endif
#endif

        /// <summary>
        /// Given the name of a checkpoint file, return the generation of the checkpoint
        /// TODO: replace this with a call to an exported ESE function.
        /// </summary>
        virtual Int64 MJetGetCheckpointFileInfo(String^ checkpoint)
        {
            wchar_t * _szCheckpoint = 0;
            unsigned long ulCheckpointGeneration = 0;
            try
            {
                _szCheckpoint = GetUnmanagedStringW(checkpoint);
                Call(ErrGetCheckpointGeneration(_szCheckpoint, &ulCheckpointGeneration));
            }
            __finally
            {
                FreeUnmanagedStringW(_szCheckpoint);
            }
            return ulCheckpointGeneration;
        }

#if (ESENT_WINXP)
        /// <summary>
        /// Given the name of a database file, return information about the database in a MJET_DBINFO struct.
        /// </summary>
        virtual MJET_DBINFO MJetGetDatabaseFileInfo(String^ database)
        {
            ::JET_DBINFOMISC _dbinfomisc;
            _TCHAR * _szDatabase = 0;
            try
            {
                _szDatabase = GetUnmanagedString(database);
                Call( ::JetGetDatabaseFileInfo( _szDatabase, &_dbinfomisc, sizeof( _dbinfomisc ), JET_DbInfoMisc ) );
            }
            __finally
            {
                FreeUnmanagedString( _szDatabase );
            }
            return MakeManagedDbinfo( _dbinfomisc );
        }
#else
#if (ESENT_PUBLICONLY)
        /// <summary>
        /// Given the name of a database file, return information about the database in a MJET_DBINFO struct.
        /// </summary>
        virtual MJET_DBINFO MJetGetDatabaseFileInfo(String^ database)
        {
            ::JET_DBINFOMISC4 _dbinfomisc4;
            _TCHAR * _szDatabase = 0;
            try
            {
                _szDatabase = GetUnmanagedString(database);
                Call( ::JetGetDatabaseFileInfo( _szDatabase, &_dbinfomisc4, sizeof( _dbinfomisc4 ), JET_DbInfoMisc ) );
            }
            __finally
            {
                FreeUnmanagedString( _szDatabase );
            }
            return MakeManagedDbinfo( _dbinfomisc4 );
        }
#else
        /// <summary>
        /// Given the name of a database file, return information about the database in a MJET_DBINFO struct.
        /// </summary>
        virtual MJET_DBINFO MJetGetDatabaseFileInfo(String^ database)
        {
            ::JET_DBINFOMISC7 _dbinfomisc7;
            _TCHAR * _szDatabase = 0;
            try
            {
                _szDatabase = GetUnmanagedString(database);
                Call( ::JetGetDatabaseFileInfo( _szDatabase, &_dbinfomisc7, sizeof( _dbinfomisc7 ), JET_DbInfoMisc ) );
            }
            __finally
            {
                FreeUnmanagedString( _szDatabase );
            }
            return MakeManagedDbinfo( _dbinfomisc7 );
        }
#endif
#endif

#if (ESENT_PUBLICONLY)
#else
#if (ESENT_WINXP)
#else
        /// <summary>
        /// Given a database and a logfile, update the database header so that the logfile is no longer in the committed
        /// range of logfiles. This causes data loss. The logfile is not deleted.
        /// </summary>
        virtual void MJetRemoveLogfile(String^ database, String^ logfile)
        {
            wchar_t * _wszDatabase = 0;
            wchar_t * _wszLogfile = 0;
            try
            {
                _wszDatabase = GetUnmanagedStringW(database);
                _wszLogfile = GetUnmanagedStringW(logfile);
                Call( ::JetRemoveLogfileW( _wszDatabase, _wszLogfile, 0 ) );
            }
            __finally
            {
                FreeUnmanagedStringW( _wszDatabase );
                FreeUnmanagedStringW( _wszLogfile );
            }
        }
#endif
#endif

#if (ESENT_PUBLICONLY)
#else
#if (ESENT_WINXP)
#else
        /// <summary>
        /// </summary>
        virtual array<Byte>^ MJetGetDatabasePages(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pgnoStart,
            Int64 cpg,
            MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            unsigned long _pgnoStart = (unsigned long) pgnoStart;
            unsigned long _cpg = (unsigned long) cpg;
            ::JET_GRBIT _grbit = (JET_GRBIT) grbit;

            ::JET_API_PTR _cbPage = 0;

            // can't return more than 64k pages at one time (that would be at least 256MB of memory)
            if( _cpg >= 64*1024 )
            {
                throw TranslatedException(gcnew IsamInvalidParameterException());
            }

            Call( ::JetGetSystemParameter( NULL, NULL, JET_paramDatabasePageSize, &_cbPage, NULL, 0 ) );

            BYTE* _rgbData = NULL;
            unsigned long _cbData = _cpg * (unsigned long)_cbPage;
            unsigned long _cbDataActual = 0;

            try
            {
                _rgbData = (BYTE *) VirtualAlloc( NULL, _cbData, MEM_COMMIT, PAGE_READWRITE );
                if ( !_rgbData )
                {
                    throw TranslatedException(gcnew IsamOutOfMemoryException());
                }

                Call( ::JetGetDatabasePages( _sesid, _dbid, _pgnoStart, _cpg, _rgbData, _cbData, &_cbDataActual, _grbit ) );

                return MakeManagedBytes( _rgbData, _cbDataActual );
            }
            __finally
            {
                if( _rgbData )
                {
                    (void)VirtualFree( _rgbData, 0, MEM_RELEASE );
                }
            }
        }

        /// <summary>
        /// </summary>
        virtual void MJetOnlinePatchDatabasePage(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pgno,
            array<Byte>^ token,
            array<Byte>^ data,
            MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            unsigned long _pgno = (unsigned long) pgno;
            ::JET_GRBIT _grbit = (JET_GRBIT) grbit;

            void* _rgbToken = NULL;
            int _cbToken = 0;

            void* _rgbData = NULL;
            int _cbData = 0;

            try
            {
                GetAlignedUnmanagedBytes( data, &_rgbData, &_cbData );
                GetUnmanagedBytes( token, &_rgbToken, &_cbToken );
                Call( ::JetOnlinePatchDatabasePage( _sesid, _dbid, _pgno, _rgbToken, _cbToken, _rgbData, _cbData, _grbit ) );
            }
            __finally
            {
                FreeAlignedUnmanagedBytes( _rgbData );
                FreeUnmanagedBytes( _rgbToken );
            }
        }

        /// <summary>
        /// </summary>
        virtual void MJetBeginDatabaseIncrementalReseed(
            MJET_INSTANCE instance,
            String^ pstrDatabase,
            MJET_GRBIT grbit)
        {
            ::JET_INSTANCE _instance = GetUnmanagedInst( instance );
            _TCHAR * _szDatabase = 0;
            ::JET_GRBIT _grbit = (JET_GRBIT) grbit;

            try
            {
                _szDatabase = GetUnmanagedString( pstrDatabase );
                Call( ::JetBeginDatabaseIncrementalReseed( _instance, _szDatabase, _grbit ) );
            }
            __finally
            {
                FreeUnmanagedString( _szDatabase );
            }
        }

        /// <summary>
        /// </summary>
        virtual void MJetEndDatabaseIncrementalReseed(
            MJET_INSTANCE instance,
            String^ pstrDatabase,
            Int64 genMinRequired,
            Int64 genFirstDivergedLog,
            Int64 genMaxRequired,
            MJET_GRBIT grbit)
        {
            ::JET_INSTANCE _instance = GetUnmanagedInst( instance );
            _TCHAR * _szDatabase = 0;
            unsigned long _genMinRequired = (unsigned long) genMinRequired;
            unsigned long _genFirstDivergedLog   = (unsigned long) genFirstDivergedLog;
            unsigned long _genMaxRequired = (unsigned long) genMaxRequired;
            ::JET_GRBIT _grbit = (JET_GRBIT) grbit;

            try
            {
                _szDatabase = GetUnmanagedString( pstrDatabase );
                Call( ::JetEndDatabaseIncrementalReseed( _instance, _szDatabase, _genMinRequired, _genFirstDivergedLog, _genMaxRequired, _grbit ) );
            }
            __finally
            {
                FreeUnmanagedString( _szDatabase );
            }
        }

        /// <summary>
        /// </summary>
        virtual void MJetPatchDatabasePages(
            MJET_INSTANCE instance,
            String^ pstrDatabase,
            Int64 pgnoStart,
            Int64 cpg,
            array<Byte>^ rgb,
            MJET_GRBIT grbit)
        {
            ::JET_INSTANCE _instance = GetUnmanagedInst( instance );
            _TCHAR * _szDatabase = 0;
            unsigned long _pgnoStart = (unsigned long) pgnoStart;
            unsigned long _cpg = (unsigned long) cpg;
            ::JET_GRBIT _grbit = (JET_GRBIT) grbit;

            void* _rgbData = NULL;
            int _cbData = 0;

            try
            {
                _szDatabase = GetUnmanagedString( pstrDatabase );

                GetUnmanagedBytes( rgb, &_rgbData, &_cbData );

                Call( ::JetPatchDatabasePages( _instance, _szDatabase, _pgnoStart, _cpg, _rgbData, _cbData, _grbit ) );
            }
            __finally
            {
                FreeUnmanagedBytes( _rgbData );
                FreeUnmanagedString( _szDatabase );
            }
        }
#endif
#endif

        /// <summary>
        /// </summary>
        virtual MJET_TABLEID MJetCreateTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name,
            Int32 pages,
            Int32 density)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            _TCHAR * _szTableName = 0;
            unsigned long _lPages = pages;
            unsigned long _lDensity = density;
            ::JET_TABLEID _tableid;
            try
            {
                _szTableName = GetUnmanagedString(name);
                Call( ::JetCreateTable( _sesid, _dbid, _szTableName, _lPages, _lDensity, &_tableid ) );
            }
            __finally
            {
                FreeUnmanagedString( _szTableName );
            }
            return MakeManagedTableid( _tableid, sesid, dbid );
        }

        /// <summary>
        /// </summary>
        virtual MJET_TABLEID MJetCreateTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name)
        {
            return MJetCreateTable( sesid, dbid, name, 16, 100 );
        }

#if (ESENT_WINXP)
        /// <summary>
        /// </summary>
        virtual void MJetCreateTableColumnIndex(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_TABLECREATE tablecreate)
        {

            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            ::JET_TABLECREATE2 _tablecreate = { 0 };

            try
            {
                _tablecreate = GetUnmanagedTablecreate( tablecreate );
                Call( ::JetCreateTableColumnIndex2( _sesid, _dbid, &_tablecreate ) );
                Call( ::JetCloseTable( _sesid, _tablecreate.tableid ) );
            }
            __finally {
                FreeUnmanagedTablecreate( _tablecreate );
            }
        }
#else
        /// <summary>
        /// </summary>
        virtual void MJetCreateTableColumnIndex(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_TABLECREATE tablecreate)
        {

            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            ::JET_TABLECREATE3 _tablecreate = { 0 };

            try
            {
                _tablecreate = GetUnmanagedTablecreate( tablecreate );
                Call( ::JetCreateTableColumnIndex3( _sesid, _dbid, &_tablecreate ) );
                Call( ::JetCloseTable( _sesid, _tablecreate.tableid ) );
            }
            __finally {
                FreeUnmanagedTablecreate( _tablecreate );
            }
        }
#endif

        /// <summary>
        /// </summary>
        virtual void MJetDeleteTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            _TCHAR * _szTableName = 0;
            try
            {
                _szTableName = GetUnmanagedString(name);
                Call( ::JetDeleteTable( _sesid, _dbid, _szTableName ) );
            }
            __finally
            {
                FreeUnmanagedString( _szTableName );
            }
        }

        /// <summary>
        /// </summary>
        virtual void MJetRenameTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ oldName,
            String^ newName)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            _TCHAR * _szName = 0;
            _TCHAR * _szNameNew = 0;

            try
            {
                _szName = GetUnmanagedString(oldName);
                _szNameNew = GetUnmanagedString(newName);
                Call( ::JetRenameTable( _sesid, _dbid, _szName, _szNameNew ) );
            }
            __finally
            {
                FreeUnmanagedString( _szName );
                FreeUnmanagedString( _szNameNew );
            }
        }

#if (ESENT_WINXP)
#else
        /// <summary>
        /// </summary>
        virtual MJET_COLUMNBASE MJetGetColumnInfo(MJET_TABLEID tableid, Int64 colid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_COLUMNBASE _columnbase;
            ::JET_COLUMNID _columnid = ::JET_COLUMNID( colid );

            _columnbase.cbStruct = sizeof( _columnbase );

            Call( JetGetTableColumnInfo( _sesid, _tableid, (_TCHAR *)&_columnid, &_columnbase, sizeof( _columnbase ), JET_ColInfoBaseByColid ) );
            return MakeManagedColumnbase( _columnbase );
        }
#endif

        /// <summary>
        /// </summary>
        virtual MJET_COLUMNBASE MJetGetColumnInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            Int64 colid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            ::JET_COLUMNBASE _columnbase;
            _TCHAR * _szTable = 0;
            ::JET_COLUMNID _columnid = ::JET_COLUMNID( colid );

            _columnbase.cbStruct = sizeof( _columnbase );

#if (ESENT_WINXP)
            System::String ^ columnName;

            try
            {
                _szTable = GetUnmanagedString( table );

                Call( ::JetGetColumnInfo( _sesid, _dbid, _szTable, (_TCHAR *)&_columnid, &_columnbase, sizeof( _columnbase ), JET_ColInfoByColid ) );
                MJET_COLUMNID columnid = MakeManagedColumnid( _columnid, (MJET_COLTYP)_columnbase.coltyp, ( 1252 == _columnbase.cp ));
                columnName = MJetGetColumnName( sesid, dbid, table, columnid );
            }
            __finally
            {
                FreeUnmanagedString( _szTable );
            }
            MJET_COLUMNBASE columnbase = MakeManagedColumnbase( _columnbase );
            columnbase.TableName = table;
            columnbase.ColumnName = columnName;
            return columnbase;
#else
            try
            {
                _szTable = GetUnmanagedString( table );
                Call( ::JetGetColumnInfo( _sesid, _dbid, _szTable, (_TCHAR *)&_columnid, &_columnbase, sizeof( _columnbase ), JET_ColInfoBaseByColid ) );
            }
            __finally
            {
                FreeUnmanagedString( _szTable );
            }
            return MakeManagedColumnbase( _columnbase );
#endif
        }

        /// <summary>
        /// </summary>
        virtual MJET_COLUMNBASE MJetGetColumnInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            MJET_COLUMNID columnid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            ::JET_COLUMNBASE _columnbase;
            _TCHAR * _szTable = 0;
            ::JET_COLUMNID _columnid = GetUnmanagedColumnid( columnid );

            _columnbase.cbStruct = sizeof( _columnbase );

#if (ESENT_WINXP)
            System::String ^ columnName;
            try
            {
                _szTable = GetUnmanagedString( table );
                Call( ::JetGetColumnInfo( _sesid, _dbid, _szTable, (_TCHAR *)&_columnid, &_columnbase, sizeof( _columnbase ), JET_ColInfoByColid ) );
                columnName = MJetGetColumnName( sesid, dbid, table, columnid );
            }
            __finally
            {
                FreeUnmanagedString( _szTable );
            }
            MJET_COLUMNBASE columnbase = MakeManagedColumnbase( _columnbase );
            columnbase.TableName = table;
            columnbase.ColumnName = columnName;
            return columnbase;
#else
            try
            {
                _szTable = GetUnmanagedString( table );
                Call( ::JetGetColumnInfo( _sesid, _dbid, _szTable, (_TCHAR *)&_columnid, &_columnbase, sizeof( _columnbase ), JET_ColInfoBaseByColid ) );
            }
            __finally
            {
                FreeUnmanagedString( _szTable );
            }
            return MakeManagedColumnbase( _columnbase );
#endif
        }

#if (ESENT_WINXP)
#else
        /// <summary>
        /// </summary>
        virtual MJET_COLUMNBASE MJetGetColumnInfo(MJET_TABLEID tableid, MJET_COLUMNID columnid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_COLUMNBASE _columnbase;
            ::JET_COLUMNID _columnid = GetUnmanagedColumnid( columnid );

            _columnbase.cbStruct = sizeof( _columnbase );

            Call( JetGetTableColumnInfo( _sesid, _tableid, (_TCHAR *)&_columnid, &_columnbase, sizeof( _columnbase ), JET_ColInfoBaseByColid ) );
            return MakeManagedColumnbase( _columnbase );
        }
#endif

        /// <summary>
        /// </summary>
        virtual MJET_COLUMNBASE MJetGetColumnInfo(MJET_TABLEID tableid, String^ column)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_COLUMNBASE _columnbase;
            _TCHAR * _szColumn = 0;

            _columnbase.cbStruct = sizeof( _columnbase );

            try
            {
                _szColumn = GetUnmanagedString( column );
                Call( JetGetTableColumnInfo( _sesid, _tableid, _szColumn, &_columnbase, sizeof( _columnbase ), JET_ColInfoBase ) );
            }
            __finally
            {
                FreeUnmanagedString( _szColumn );
            }
            return MakeManagedColumnbase( _columnbase );
        }

        /// <summary>
        /// </summary>
        virtual MJET_COLUMNBASE MJetGetColumnInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ column)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            ::JET_COLUMNBASE _columnbase;
            _TCHAR * _szTable = 0;
            _TCHAR * _szColumn = 0;

            _columnbase.cbStruct = sizeof( _columnbase );

            try
            {
                _szTable = GetUnmanagedString( table );
                _szColumn = GetUnmanagedString( column );
                Call( ::JetGetColumnInfo( _sesid, _dbid, _szTable, _szColumn, &_columnbase, sizeof( _columnbase ), JET_ColInfoBase ) );
            }
            __finally
            {
                FreeUnmanagedString( _szTable );
                FreeUnmanagedString( _szColumn );
            }
            return MakeManagedColumnbase( _columnbase );
        }

        /// <summary>
        /// </summary>
        virtual MJET_COLUMNLIST MJetGetColumnInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            Boolean sortByColumnid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            ::JET_GRBIT _grbit = sortByColumnid ? JET_ColInfoListSortColumnid : JET_ColInfoList;
            ::JET_COLUMNLIST _columnlist;
            _TCHAR * _szTable = 0;

            _columnlist.cbStruct = sizeof( _columnlist );

            try
            {
                _szTable = GetUnmanagedString( table );
                Call( ::JetGetColumnInfo( _sesid, _dbid, _szTable, 0, &_columnlist, sizeof( _columnlist ), _grbit ) );
            }
            __finally
            {
                FreeUnmanagedString( _szTable );
            }
            return MakeManagedColumnList( sesid, _columnlist );
        }

        /// <summary>
        /// </summary>
        virtual MJET_COLUMNLIST MJetGetColumnInfoList(MJET_TABLEID tableid, Boolean sortByColumnid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = sortByColumnid ? JET_ColInfoListSortColumnid : JET_ColInfoList;
            ::JET_COLUMNLIST _columnlist;

            _columnlist.cbStruct = sizeof( _columnlist );

            Call( JetGetTableColumnInfo( _sesid, _tableid, 0, &_columnlist, sizeof( _columnlist ), _grbit ) );
            return MakeManagedColumnList( tableid.Sesid, _columnlist );
        }

        /// <summary>
        /// </summary>
        virtual MJET_COLUMNLIST MJetGetColumnInfoList(MJET_TABLEID tableid)
        {
            return MJetGetColumnInfoList( tableid, false );
        }

        /// <summary>
        /// </summary>
        virtual MJET_COLUMNID MJetAddColumn(
            MJET_TABLEID tableid,
            String^ name,
            MJET_COLUMNDEF% definition,
            array<Byte>^ defaultValue)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            _TCHAR * _szColumnName = 0;
            ::JET_COLUMNDEF _columndef = GetUnmanagedColumndef( definition );
            ::JET_COLUMNID _columnid;
            void * _pvDefault = 0;
            int _cbDefault;

            try
            {
                _szColumnName = GetUnmanagedString( name );
                GetUnmanagedBytes( defaultValue, &_pvDefault, &_cbDefault );
                Call( ::JetAddColumn( _sesid, _tableid, _szColumnName, &_columndef, _pvDefault, _cbDefault, &_columnid ) );
            }
            __finally
            {
                FreeUnmanagedBytes( _pvDefault );
                FreeUnmanagedString( _szColumnName );
            }

            definition.Columnid = MakeManagedColumnid( _columnid, definition.Coltyp, definition.ASCII );
            return definition.Columnid;
        }

        /// <summary>
        /// </summary>
        virtual MJET_COLUMNID MJetAddColumn(
            MJET_TABLEID tableid,
            String^ name,
            MJET_COLUMNDEF% definition)
        {
            return MJetAddColumn( tableid, name, definition, nullptr );
        }

        /// <summary>
        /// </summary>
        virtual void MJetDeleteColumn(MJET_TABLEID tableid, String^ name)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            _TCHAR * _szColumnName = 0;

            try
            {
                _szColumnName = GetUnmanagedString( name );
                Call( ::JetDeleteColumn( _sesid, _tableid, _szColumnName ) );
            }
            __finally
            {
                FreeUnmanagedString( _szColumnName );
            }
        }

        /// <summary>
        /// </summary>
        virtual void MJetDeleteColumn(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            _TCHAR * _szColumnName = 0;
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            try
            {
                _szColumnName = GetUnmanagedString( name );
                Call( ::JetDeleteColumn2( _sesid, _tableid, _szColumnName, _grbit ) );
            }
            __finally
            {
                FreeUnmanagedString( _szColumnName );
            }
        }

        /// <summary>
        /// </summary>
        virtual void MJetRenameColumn(
            MJET_TABLEID tableid,
            String^ oldName,
            String^ newName,
            MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            _TCHAR * _szName = 0;
            _TCHAR * _szNameNew = 0;
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;

            try
            {
                _szName = GetUnmanagedString( oldName );
                _szNameNew = GetUnmanagedString( newName );
                Call( ::JetRenameColumn( _sesid, _tableid, _szName, _szNameNew, _grbit ) );
            }
            __finally
            {
                FreeUnmanagedString( _szName );
                FreeUnmanagedString( _szNameNew );
            }
        }

        /// <summary>
        /// </summary>
        virtual void MJetSetColumnDefaultValue(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ column,
            array<Byte>^ defaultValue,
            MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            _TCHAR * _szTableName = 0;
            _TCHAR * _szColumnName = 0;
            void * _pvDefault = 0;
            int _cbDefault;

            try
            {
                _szTableName = GetUnmanagedString( table );
                _szColumnName = GetUnmanagedString( column );
                GetUnmanagedBytes( defaultValue, &_pvDefault, &_cbDefault );
                Call( ::JetSetColumnDefaultValue( _sesid, _dbid, _szTableName, _szColumnName, _pvDefault, _cbDefault, _grbit ) );
            }
            __finally
            {
                FreeUnmanagedBytes( _pvDefault );
                FreeUnmanagedString( _szTableName );
                FreeUnmanagedString( _szColumnName );
            }

        }

        /// <summary>
        /// </summary>
        virtual MJET_INDEXLIST MJetGetIndexInfo(MJET_TABLEID tableid, String^ index)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_INDEXLIST _indexlist;
            _TCHAR * _szIndex = 0;

            _indexlist.cbStruct = sizeof( _indexlist );

            try
            {
                _szIndex = GetUnmanagedString( index );
                Call( ::JetGetTableIndexInfo( _sesid, _tableid, _szIndex, &_indexlist, sizeof( _indexlist ), JET_IdxInfo ) );
            }
            __finally
            {
                FreeUnmanagedString( _szIndex );
            }
            return MakeManagedIndexList( tableid.Sesid, _indexlist );
        }

        /// <summary>
        /// </summary>
        virtual MJET_INDEXLIST MJetGetIndexInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            ::JET_INDEXLIST _indexlist;
            _TCHAR * _szTable = 0;
            _TCHAR * _szIndex = 0;

            _indexlist.cbStruct = sizeof( _indexlist );

            try
            {
                _szTable = GetUnmanagedString( table );
                _szIndex = GetUnmanagedString( index );
                Call( ::JetGetIndexInfo( _sesid, _dbid, _szTable, _szIndex, &_indexlist, sizeof( _indexlist ), JET_IdxInfo ) );
            }
            __finally
            {
                FreeUnmanagedString( _szTable );
                FreeUnmanagedString( _szIndex );
            }
            return MakeManagedIndexList( sesid, _indexlist );
        }

        /// <summary>
        /// </summary>
        virtual MJET_INDEXLIST MJetGetIndexInfoList(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_INDEXLIST _indexlist;

            _indexlist.cbStruct = sizeof( _indexlist );

            Call( ::JetGetTableIndexInfo( _sesid, _tableid, 0, &_indexlist, sizeof( _indexlist ), JET_IdxInfoList ) );
            return MakeManagedIndexList( tableid.Sesid, _indexlist );
        }

        /// <summary>
        /// </summary>
        virtual MJET_INDEXLIST MJetGetIndexInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            ::JET_INDEXLIST _indexlist;
            _TCHAR * _szTable = 0;

            _indexlist.cbStruct = sizeof( _indexlist );

            try
            {
                _szTable = GetUnmanagedString( table );
                Call( ::JetGetIndexInfo( _sesid, _dbid, _szTable, 0, &_indexlist, sizeof( _indexlist ), JET_IdxInfoList ) );
            }
            __finally
            {
                FreeUnmanagedString( _szTable );
            }
            return MakeManagedIndexList( sesid, _indexlist );
        }

        /// <summary>
        /// </summary>
        virtual int MJetGetIndexInfoDensity(MJET_TABLEID tableid, String^ index)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            unsigned long _ulDensity = 0;
            _TCHAR * _szIndex = 0;

            try
            {
                _szIndex = GetUnmanagedString( index );
                Call( ::JetGetTableIndexInfo( _sesid, _tableid, _szIndex, &_ulDensity, sizeof( _ulDensity ), JET_IdxInfoSpaceAlloc ) );
            }
            __finally
            {
                FreeUnmanagedString( _szIndex );
            }
            return int( _ulDensity );
        }

        /// <summary>
        /// </summary>
        virtual int MJetGetIndexInfoDensity(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            unsigned long _ulDensity = 0;
            _TCHAR * _szTable = 0;
            _TCHAR * _szIndex = 0;

            try
            {
                _szTable = GetUnmanagedString( table );
                _szIndex = GetUnmanagedString( index );
                Call( ::JetGetIndexInfo( _sesid, _dbid, _szTable, _szIndex, &_ulDensity, sizeof( _ulDensity ), JET_IdxInfoSpaceAlloc ) );
            }
            __finally
            {
                FreeUnmanagedString( _szTable );
                FreeUnmanagedString( _szIndex );
            }
            return int( _ulDensity );
        }

        /// <summary>
        /// </summary>
        virtual int MJetGetIndexInfoLCID(MJET_TABLEID tableid, String^ index)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::LCID _lcid;
            _TCHAR * _szIndex = 0;

            try
            {
                _szIndex = GetUnmanagedString( index );
                Call( ::JetGetTableIndexInfo( _sesid, _tableid, _szIndex, &_lcid, sizeof( _lcid ), JET_IdxInfoLCID ) );
            }
            __finally
            {
                FreeUnmanagedString( _szIndex );
            }
            return int( _lcid );
        }

        /// <summary>
        /// </summary>
        virtual int MJetGetIndexInfoLCID(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            ::LCID _lcid;
            _TCHAR * _szTable = 0;
            _TCHAR * _szIndex = 0;

            try
            {
                _szTable = GetUnmanagedString( table );
                _szIndex = GetUnmanagedString( index );
                Call( ::JetGetIndexInfo( _sesid, _dbid, _szTable, _szIndex, &_lcid, sizeof( _lcid ), JET_IdxInfoLCID ) );
            }
            __finally
            {
                FreeUnmanagedString( _szTable );
                FreeUnmanagedString( _szIndex );
            }
            return int( _lcid );
        }

#if (ESENT_WINXP)
        /// <summary>
        /// </summary>
        virtual int MJetGetIndexInfoKeyMost(MJET_TABLEID /*tableid*/, String^ /*index*/)
        {
            return JET_cbKeyMost;
        }

        /// <summary>
        /// </summary>
        virtual int MJetGetIndexInfoKeyMost(
            MJET_SESID /*sesid*/,
            MJET_DBID /*dbid*/,
            String^ /*table*/,
            String^ /*index*/)
        {
            return JET_cbKeyMost;
        }
#else
        /// <summary>
        /// </summary>
        virtual int MJetGetIndexInfoKeyMost(MJET_TABLEID tableid, String^ index)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            unsigned short _cbKeyMost = 0;
            _TCHAR * _szIndex = 0;

            try
            {
                _szIndex = GetUnmanagedString( index );
                Call( ::JetGetTableIndexInfo( _sesid, _tableid, _szIndex, &_cbKeyMost, sizeof( _cbKeyMost ), JET_IdxInfoKeyMost ) );
            }
            __finally
            {
                FreeUnmanagedString( _szIndex );
            }
            return int( _cbKeyMost );
        }

        /// <summary>
        /// </summary>
        virtual int MJetGetIndexInfoKeyMost(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            unsigned short _cbKeyMost = 0;
            _TCHAR * _szTable = 0;
            _TCHAR * _szIndex = 0;

            try
            {
                _szTable = GetUnmanagedString( table );
                _szIndex = GetUnmanagedString( index );
                Call( ::JetGetIndexInfo( _sesid, _dbid, _szTable, _szIndex, &_cbKeyMost, sizeof( _cbKeyMost ), JET_IdxInfoKeyMost ) );
            }
            __finally
            {
                FreeUnmanagedString( _szTable );
                FreeUnmanagedString( _szIndex );
            }
            return int( _cbKeyMost );
        }
#endif

#if ESENT_WINXP
#else
        /// <summary>
        /// </summary>
        virtual MJET_INDEXCREATE MJetGetIndexInfoCreate(MJET_TABLEID tableid, String^ index)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            _TCHAR * _szIndex = 0;
            ULONG _cbData = 4*1024;
            void * _pvData = NULL;
            ::JET_ERR _err = JET_errSuccess;

            try
            {
                _szIndex = GetUnmanagedString( index );

                for( int i = 0; i < 3; i++ )
                {
                    _pvData = VirtualAlloc( NULL, _cbData, MEM_COMMIT, PAGE_READWRITE );
                    if ( !_pvData )
                    {
                        throw TranslatedException(gcnew IsamOutOfMemoryException());
                    }

                    _err = ::JetGetTableIndexInfo( _sesid, _tableid, _szIndex, _pvData, _cbData, JET_IdxInfoCreateIndex2 );
                    if( _err != JET_errBufferTooSmall )
                    {
                        break;
                    }

                    VirtualFree( _pvData, 0, MEM_RELEASE );
                    _cbData *= 2;
                    _pvData = NULL;
                }

                Call( _err );
                return MakeManagedIndexCreate( *(::JET_INDEXCREATE2 *) _pvData );
            }
            __finally
            {
                FreeUnmanagedString( _szIndex );
                if(_pvData)
                    VirtualFree(_pvData, 0, MEM_RELEASE);
            }
        }

        /// <summary>
        /// </summary>
        virtual MJET_INDEXCREATE MJetGetIndexInfoCreate(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            _TCHAR * _szTable = 0;
            _TCHAR * _szIndex = 0;
            ULONG _cbData = 4*1024;
            void * _pvData = NULL;
            ::JET_ERR _err = JET_errSuccess;

            try
            {
                _szTable = GetUnmanagedString( table );
                _szIndex = GetUnmanagedString( index );

                for( int i = 0; i < 3; i++ )
                {
                    _pvData = VirtualAlloc( NULL, _cbData, MEM_COMMIT, PAGE_READWRITE );
                    if ( !_pvData )
                    {
                        throw TranslatedException(gcnew IsamOutOfMemoryException());
                    }

                    _err = ::JetGetIndexInfo( _sesid, _dbid, _szTable, _szIndex, _pvData, _cbData, JET_IdxInfoCreateIndex2 );
                    if( _err != JET_errBufferTooSmall )
                    {
                        break;
                    }

                    VirtualFree( _pvData, 0, MEM_RELEASE );
                    _cbData *= 2;
                    _pvData = NULL;
                }

                Call( _err );
                return MakeManagedIndexCreate( *(::JET_INDEXCREATE2 *) _pvData );
            }
            __finally
            {
                FreeUnmanagedString( _szTable );
                FreeUnmanagedString( _szIndex );
                if( _pvData )
                    VirtualFree( _pvData, _cbData, MEM_RELEASE );
            }
        }
#endif

        /// <summary>
        /// </summary>
        virtual void MJetCreateIndex(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit,
            String^ key,
            Int32 density)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            _TCHAR * _szName = 0;
            _TCHAR * _szKey = 0;
            unsigned long _cbKey = (key->Length + 1) * sizeof(_TCHAR);
            unsigned long _lDensity = density;

            try
            {
                _szName = GetUnmanagedString( name );
                _szKey = GetUnmanagedString( key );
                Call( ::JetCreateIndex( _sesid, _tableid, _szName, _grbit, _szKey, _cbKey, _lDensity ) );
            }
            __finally
            {
                FreeUnmanagedString( _szName );
                FreeUnmanagedString( _szKey );
            }
        }

        /// <summary>
        /// </summary>
        virtual void MJetCreateIndex(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit,
            String^ key)
        {
            MJetCreateIndex( tableid, name, grbit, key, 100 );
        }

        /// <summary>
        /// </summary>
        virtual void MJetCreateIndex(
            MJET_TABLEID tableid,
            String^ name,
            String^ key)
        {
            MJetCreateIndex( tableid, name, MJET_GRBIT::NoGrbit, key );
        }

#if (ESENT_WINXP)
        virtual void MJetCreateIndex(MJET_TABLEID tableid, array<MJET_INDEXCREATE>^ indexcreates)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            const int _cindexcreate = indexcreates->Length;
            ::JET_INDEXCREATE* _rgindexcreate = NULL;

            try
            {
                // allocate JET_INDEXCREATE array and zero all pointers
                //
                _rgindexcreate = GetUnmanagedIndexCreates( indexcreates );
                Call( ::JetCreateIndex2( _sesid, _tableid, _rgindexcreate, _cindexcreate ) );
            }
            __finally
            {
                FreeUnmanagedIndexCreates( _rgindexcreate, _cindexcreate );
            }
        }
#else
        virtual void MJetCreateIndex(MJET_TABLEID tableid, array<MJET_INDEXCREATE>^ indexcreates)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            const int _cindexcreate = indexcreates->Length;
            ::JET_INDEXCREATE2* _rgindexcreate = NULL;

            try
            {
                // allocate JET_INDEXCREATE array and zero all pointers
                //
                _rgindexcreate = GetUnmanagedIndexCreates( indexcreates );
                Call( ::JetCreateIndex3( _sesid, _tableid, _rgindexcreate, _cindexcreate ) );
            }
            __finally
            {
                FreeUnmanagedIndexCreates( _rgindexcreate, _cindexcreate );
            }
        }
#endif

        /// <summary>
        /// </summary>
        virtual void MJetDeleteIndex(MJET_TABLEID tableid, String^ name)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            _TCHAR * _szName = 0;

            try
            {
                _szName = GetUnmanagedString( name );
                Call( ::JetDeleteIndex( _sesid, _tableid, _szName ) );
            }
            __finally
            {
                FreeUnmanagedString( _szName );
            }
        }

        /// <summary>
        /// </summary>
        virtual void MJetBeginTransaction(MJET_SESID sesid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            Call( ::JetBeginTransaction3( _sesid, 40997, 0 ) );
        }

        /// <summary>
        /// </summary>
        virtual void MJetBeginTransaction(MJET_SESID sesid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            Call( ::JetBeginTransaction3( _sesid, 57381, _grbit ) );
        }

        /// <summary>
        /// </summary>
        virtual void MJetBeginTransaction(
            MJET_SESID sesid,
            Int32 trxid,
            MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            Call( ::JetBeginTransaction3( _sesid, (unsigned long)trxid, _grbit ) );
        }

        /// <summary>
        /// </summary>
        virtual void MJetCommitTransaction(MJET_SESID sesid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            Call( ::JetCommitTransaction( _sesid, _grbit ) );
        }

        /// <summary>
        /// </summary>
        virtual void MJetCommitTransaction(MJET_SESID sesid)
        {
            MJetCommitTransaction( sesid, MJET_GRBIT::NoGrbit );
        }

        /// <summary>
        /// </summary>
        virtual void MJetRollback(MJET_SESID sesid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            Call( ::JetRollback( _sesid, _grbit ) );
        }

        /// <summary>
        /// </summary>
        virtual void MJetRollback(MJET_SESID sesid)
        {
            MJetRollback( sesid, MJET_GRBIT::NoGrbit );
        }

        /// <summary>
        /// </summary>
        virtual void MJetGetDatabaseInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            array<Byte>^ resultBuffer,
            MJET_DBINFO_LEVEL InfoLevel)
        {
            ::JET_SESID _sesid      = GetUnmanagedSesid( sesid );
            ::JET_DBID  _dbid       = GetUnmanagedDbid( dbid );
            void*       _pvBuffer   = 0;
            int         _cbBufferSize;

            try{
                GetUnmanagedBytes( resultBuffer, &_pvBuffer, &_cbBufferSize );
                Call( ::JetGetDatabaseInfo( _sesid, _dbid, _pvBuffer, _cbBufferSize, (unsigned long)InfoLevel) );
                Marshal::Copy( (IntPtr) _pvBuffer, resultBuffer, 0, _cbBufferSize );
            }
            __finally
            {
                FreeUnmanagedBytes( _pvBuffer );
            }
        }

#if (ESENT_WINXP)
        /// <summary>
        /// </summary>
        virtual MJET_DBINFO MJetGetDatabaseInfo(
            MJET_SESID sesid,
            MJET_DBID dbid)
        {
            ::JET_SESID _sesid      = GetUnmanagedSesid( sesid );
            ::JET_DBID  _dbid       = GetUnmanagedDbid( dbid );
            ::JET_DBINFOMISC _dbinfomisc;

            Call( ::JetGetDatabaseInfo( _sesid, _dbid, &_dbinfomisc, sizeof( _dbinfomisc ), JET_DbInfoMisc ) );
            return MakeManagedDbinfo( _dbinfomisc );
        }
#else
#if (ESENT_PUBLICONLY)
        /// <summary>
        /// </summary>
        virtual MJET_DBINFO MJetGetDatabaseInfo(
            MJET_SESID sesid,
            MJET_DBID dbid)
        {
            ::JET_SESID _sesid      = GetUnmanagedSesid( sesid );
            ::JET_DBID  _dbid       = GetUnmanagedDbid( dbid );
            ::JET_DBINFOMISC4 _dbinfomisc4;

            Call( ::JetGetDatabaseInfo( _sesid, _dbid, &_dbinfomisc4, sizeof( _dbinfomisc4 ), JET_DbInfoMisc ) );
            return MakeManagedDbinfo( _dbinfomisc4 );
        }
#else
        /// <summary>
        /// </summary>
        virtual MJET_DBINFO MJetGetDatabaseInfo(
            MJET_SESID sesid,
            MJET_DBID dbid)
        {
            ::JET_SESID _sesid      = GetUnmanagedSesid( sesid );
            ::JET_DBID  _dbid       = GetUnmanagedDbid( dbid );
            ::JET_DBINFOMISC7 _dbinfomisc7;

            Call( ::JetGetDatabaseInfo( _sesid, _dbid, &_dbinfomisc7, sizeof( _dbinfomisc7 ), JET_DbInfoMisc ) );
            return MakeManagedDbinfo( _dbinfomisc7 );
        }
#endif
#endif

#if (ESENT_WINXP)
#else
        /// <summary>
        /// </summary>
        virtual String^ MJetGetDatabaseInfoFilename(MJET_SESID sesid, MJET_DBID dbid)
        {
            ::JET_SESID _sesid      = GetUnmanagedSesid( sesid );
            ::JET_DBID  _dbid       = GetUnmanagedDbid( dbid );
            const int   _cwchBuffer     = 1024;
            _TCHAR      _rgwchBuffer[_cwchBuffer];

            Call( ::JetGetDatabaseInfo( _sesid, _dbid, _rgwchBuffer, sizeof( _rgwchBuffer ), JET_DbInfoFilename ) );
            return MakeManagedString( _rgwchBuffer );
        }

        /// <summary>
        /// </summary>
        virtual Int64 MJetGetDatabaseInfoFilesize(MJET_SESID sesid, MJET_DBID dbid)
        {
            ::JET_SESID _sesid      = GetUnmanagedSesid( sesid );
            ::JET_DBID  _dbid       = GetUnmanagedDbid( dbid );
            unsigned long filesize;

            Call( ::JetGetDatabaseInfo( _sesid, _dbid, &filesize, sizeof( filesize ), JET_DbInfoFilesize ) );
            return (System::Int64)filesize;
        }

#endif

        /// <summary>
        /// </summary>
        virtual MJET_DBID MJetOpenDatabase(
            MJET_SESID sesid,
            String^ file,
            String^ connect,
            MJET_GRBIT grbit,
            MJET_WRN% wrn)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            _TCHAR * _szFilename = 0;
            _TCHAR * _szConnect = 0;
            ::JET_DBID _dbid;

            try
            {
                _szFilename = GetUnmanagedString( file );
                _szConnect = GetUnmanagedString( connect );
                wrn = CallW( ::JetOpenDatabase( _sesid, _szFilename, _szConnect, &_dbid, _grbit ) );
            }
            __finally
            {
                FreeUnmanagedString( _szFilename );
                FreeUnmanagedString( _szConnect );
            }
            return MakeManagedDbid( _dbid );
        }

        /// <summary>
        /// </summary>
        virtual MJET_DBID MJetOpenDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit,
            MJET_WRN% wrn)
        {
            return MJetOpenDatabase( sesid, file, L"", grbit, wrn );
        }

        /// <summary>
        /// </summary>
        virtual MJET_DBID MJetOpenDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit)
        {
            MJET_WRN wrn;
            return MJetOpenDatabase( sesid, file, L"", grbit, wrn );
        }

        /// <summary>
        /// </summary>
        virtual MJET_DBID MJetOpenDatabase(MJET_SESID sesid, String^ file)
        {
            return MJetOpenDatabase( sesid, file, MJET_GRBIT::NoGrbit );
        }

        /// <summary>
        /// </summary>
        virtual void MJetCloseDatabase(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            Call( ::JetCloseDatabase( _sesid, _dbid, _grbit ) );
        }

        /// <summary>
        /// </summary>
        virtual void MJetCloseDatabase(MJET_SESID sesid, MJET_DBID dbid)
        {
            MJetCloseDatabase( sesid, dbid, MJET_GRBIT::NoGrbit );
        }

        /// <summary>
        /// </summary>
        virtual MJET_TABLEID MJetOpenTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name,
            array<Byte>^ parameters,
            MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            _TCHAR * _szName = 0;
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            ::JET_TABLEID _tableid;
            void * _pvParameters = 0;
            int _cbParameters;

            try
            {
                _szName = GetUnmanagedString( name );
                GetUnmanagedBytes( parameters, &_pvParameters, &_cbParameters );
                Call( ::JetOpenTable( _sesid, _dbid, _szName, _pvParameters, _cbParameters, _grbit, &_tableid ) );
            }
            __finally
            {
                FreeUnmanagedString( _szName );
                FreeUnmanagedBytes( _pvParameters );
            }
            return MakeManagedTableid( _tableid, sesid, dbid );
        }

        /// <summary>
        /// </summary>
        virtual MJET_TABLEID MJetOpenTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name,
            MJET_GRBIT grbit)
        {
            return MJetOpenTable( sesid, dbid, name, nullptr , grbit );
        }

        /// <summary>
        /// </summary>
        virtual MJET_TABLEID MJetOpenTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name)
        {
            return MJetOpenTable( sesid, dbid, name, MJET_GRBIT::NoGrbit );
        }

        /// <summary>
        /// </summary>
        virtual Boolean TryOpenTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name,
            MJET_TABLEID% tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            _TCHAR * _szName = 0;
            ::JET_TABLEID _tableid;

            tableid = *(new MJET_TABLEID());
            try
            {
                _szName = GetUnmanagedString( name );
                if ( FCall( ::JetOpenTable( _sesid, _dbid, _szName, NULL, 0, 0, &_tableid ), JET_errObjectNotFound ) )
                {
                    tableid = MakeManagedTableid( _tableid, sesid, dbid );
                    return true;
                }
                return false;
            }
            __finally
            {
                FreeUnmanagedString( _szName );
            }
        }

        /// <summary>
        /// </summary>
        virtual void MJetSetTableSequential(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            Call( ::JetSetTableSequential( _sesid, _tableid, _grbit ) );
        }

        /// <summary>
        /// </summary>
        virtual void MJetResetTableSequential(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            Call( ::JetResetTableSequential( _sesid, _tableid, _grbit ) );
        }

        /// <summary>
        /// </summary>
        virtual void MJetCloseTable(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            Call( ::JetCloseTable( _sesid, _tableid ) );
        }

        /// <summary>
        /// </summary>
        virtual void MJetDelete(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            Call( ::JetDelete( _sesid, _tableid ) );
        }

        /// <summary>
        /// </summary>
        virtual array<Byte>^ MJetUpdate(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );

#if (ESENT_WINXP)
            const size_t _cbKeyMost = JET_cbKeyMost;
#else
            const size_t _cbKeyMost = JET_cbKeyMostMost;
#endif
            BYTE _rgbBookmark[ _cbKeyMost ];
            unsigned long _cbActual = 0xDDDDDDDD;

            Call( ::JetUpdate( _sesid, _tableid, _rgbBookmark, sizeof( _rgbBookmark ), &_cbActual ) );

            // work around ancient bug in ESE where the bookmark args are not set on
            // output for TTs in insert mode. why 0xDDDDDDDD? FillClientBuffer rules!

            if ( _cbActual == 0xDDDDDDDD )
            {
                _cbActual = 0;
            }

            return MakeManagedBytes( _rgbBookmark, _cbActual );
        }

#if (ESENT_WINXP)
#else
        /// <summary>
        /// </summary>
        virtual array<Byte>^ MJetUpdate(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;

            BYTE _rgbBookmark[JET_cbKeyMostMost];
            unsigned long _cbActual = 0xDDDDDDDD;

            Call( ::JetUpdate2( _sesid, _tableid, _rgbBookmark, sizeof( _rgbBookmark ), &_cbActual, _grbit ) );

            // work around ancient bug in ESE where the bookmark args are not set on
            // output for TTs in insert mode. why 0xDDDDDDDD? FillClientBuffer rules!

            if ( _cbActual == 0xDDDDDDDD )
            {
                _cbActual = 0;
            }

            return MakeManagedBytes( _rgbBookmark, _cbActual );
        }
#endif

        /*

        UNDONE

        JET_ERR JET_API MJetEscrowUpdate(
        JET_SESID       sesid,
        JET_TABLEID     tableid,
        JET_COLUMNID    columnid,
        void            *pv,
        unsigned long   cbMax,
        void            *pvOld,
        unsigned long   cbOldMax,
        unsigned long   *pcbOldActual,
        JET_GRBIT       grbit );

        */

        /// <summary>
        /// Returns NULL if the column is null, otherwise returns the number of bytes read into the buffer
        /// </summary>
        virtual Nullable<Int64> MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit,
            MJET_RETINFO retinfo,
            ArraySegment<Byte> buffer)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_COLUMNID _columnid = GetUnmanagedColumnid( columnid );
            ::JET_RETINFO _retinfo = GetUnmanagedRetinfo( retinfo );
            unsigned long _cbMax = (unsigned long)buffer.Count;
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;

            unsigned long _cbActual;
            JET_ERR _err;

            pin_ptr<Byte> _pb = &buffer.Array[buffer.Offset];

            Call( _err = ::JetRetrieveColumn( _sesid, _tableid, _columnid, _pb, _cbMax, &_cbActual, _grbit, &_retinfo ) );
            if( JET_wrnColumnNull != _err )
            {
                return ( _cbActual < _cbMax ) ? _cbActual : _cbMax;
            }

            return Nullable<Int64>();
        }

        /// <summary>
        /// Returns NULL if the column is null, returns a zero-length array for a zero-length column
        /// </summary>
        virtual array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit,
            MJET_RETINFO retinfo,
            Int64 maxdata)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_COLUMNID _columnid = GetUnmanagedColumnid( columnid );
            ::JET_RETINFO _retinfo = GetUnmanagedRetinfo( retinfo );
            unsigned long _cbMax = (unsigned long)maxdata;
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;

            const unsigned long _cbBuf = 1024;
            BYTE _rgbBuf[_cbBuf];
            BYTE * _pb = _rgbBuf;
            unsigned long _cbToRetrieve = (_cbMax > _cbBuf ) ? _cbBuf : _cbMax;
            unsigned long _cbActual;

            try
            {
                JET_ERR _err;
                //System::Diagnostics::Debug::Assert( _cbToRetrieve <= _cbBuf );
                //System::Diagnostics::Debug::Assert( _cbToRetrieve <= _cbMax );
                Call( _err =::JetRetrieveColumn( _sesid, _tableid, _columnid, _pb, _cbToRetrieve, &_cbActual, _grbit, &_retinfo ) );
                if( _cbActual > _cbToRetrieve && _cbMax > _cbToRetrieve )
                {
                    // the buffer wasn't big enough, and the caller wants more data than was retrieved, allocate a buffer for the data
                    // retrieve the smaller of the actual size and the maximum we want

                    _cbToRetrieve = ( _cbMax < _cbActual ) ? _cbMax : _cbActual;
                    _pb = new BYTE[_cbToRetrieve];
                    if( 0 == _pb )
                    {
                        throw TranslatedException(gcnew IsamOutOfMemoryException());
                    }
                    //System::Diagnostics::Debug::Assert( _cbToRetrieve <= _cbMax );
                    Call( _err = ::JetRetrieveColumn( _sesid, _tableid, _columnid, _pb, _cbToRetrieve, &_cbActual, _grbit, &_retinfo ) );
                    //System::Diagnostics::Debug::Assert( _cbActual >= _cbToRetrieve );
                }

                if( JET_wrnColumnNull == _err )
                {
                    return nullptr;
                }
                else
                {
                    const unsigned long _cbToReturn = ( _cbActual < _cbToRetrieve ) ? _cbActual : _cbToRetrieve;
                    return MakeManagedBytes( _pb, _cbToReturn );
                }
            }
            __finally
            {
                if( _rgbBuf != _pb )
                {
                    delete [] _pb;
                }
            }
            return nullptr;
        }

        /// <summary>
        /// </summary>
        virtual array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit,
            MJET_RETINFO retinfo)
        {
            return MJetRetrieveColumn( tableid, columnid, grbit, retinfo, -1 );
        }

        /// <summary>
        /// </summary>
        virtual array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit)
        {
            return MJetRetrieveColumn( tableid, columnid, grbit, -1 );
        }

        /// <summary>
        /// Returns NULL if the column is null, returns a zere-length array for a zero-length column
        /// </summary>
        virtual array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit,
            Int64 maxdata)
        {
            MJET_RETINFO retinfo;
            retinfo.IbLongValue = 0;
            retinfo.ItagSequence = 1;
            return MJetRetrieveColumn( tableid, columnid, grbit, retinfo, maxdata );
        }

        /// <summary>
        /// </summary>
        virtual array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            Int64 maxdata)
        {
            return MJetRetrieveColumn( tableid, columnid, MJET_GRBIT::NoGrbit, maxdata );
        }

        /// <summary>
        /// </summary>
        virtual array<Byte>^ MJetRetrieveColumn(MJET_TABLEID tableid, MJET_COLUMNID columnid)
        {
            return MJetRetrieveColumn( tableid, columnid, MJET_GRBIT::NoGrbit, -1 );
        }

        /// <summary>
        /// </summary>
        virtual Int64 MJetRetrieveColumnSize(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            Int64 itagSequence,
            MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_COLUMNID _columnid = GetUnmanagedColumnid( columnid );
            ::JET_GRBIT _grbit = (::JET_GRBIT)grbit;

            unsigned long _cbActual = 0;
            ::JET_RETINFO _retinfo = { sizeof( ::JET_RETINFO ), 0, (unsigned long)itagSequence };

            MJET_WRN wrn = CallW( ::JetRetrieveColumn( _sesid, _tableid, _columnid, NULL, 0, &_cbActual, _grbit, &_retinfo ) );

            if ( wrn == MJET_WRN::ColumnNull )
            {
                return 0;
            }
            else
            {
                return (System::Int64)_cbActual;
            }
        }

        /// <summary>
        /// </summary>
        virtual Int64 MJetRetrieveColumnSize(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            Int64 itagSequence)
        {
            return MJetRetrieveColumnSize( tableid, columnid, itagSequence, MJET_GRBIT::NoGrbit );
        }

        /// <summary>
        /// </summary>
        virtual Int64 MJetRetrieveColumnSize(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit)
        {
            return MJetRetrieveColumnSize( tableid, columnid, 1, grbit );
        }

        /// <summary>
        /// </summary>
        virtual Int64 MJetRetrieveColumnSize(MJET_TABLEID tableid, MJET_COLUMNID columnid)
        {
            return MJetRetrieveColumnSize( tableid, columnid, 1, MJET_GRBIT::NoGrbit );
        }

        /*

        UNDONE

        JET_ERR JET_API MJetRetrieveColumns(
        JET_SESID       sesid,
        JET_TABLEID     tableid,
        JET_RETRIEVECOLUMN  *pretrievecolumn,
        unsigned long   cretrievecolumn );

        JET_ERR JET_API MJetRetrieveTaggedColumnList(
        JET_SESID       sesid,
        JET_TABLEID     tableid,
        unsigned long   *pcColumns,
        void            *pvData,
        unsigned long   cbData,
        JET_COLUMNID    columnidStart,
        JET_GRBIT       grbit );

        */

#if (ESENT_WINXP)
        virtual String^ MJetGetColumnName(MJET_TABLEID tableid, MJET_COLUMNID columnid)
        {
            System::String^ tableName = MJetGetTableInfoName( tableid );
            return MJetGetColumnName( tableid.Sesid, tableid.Dbid, tableName, columnid );
        }

        virtual String^ MJetGetColumnName(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ tablename,
            MJET_COLUMNID columnid)
        {
            ///  there is currently no efficient means to retrieve the
            ///  name of a specific column from JET by columnid.  so, we are
            ///  going to reach into the catalog and fetch it directly

            MJET_TABLEID tableidCatalog = MJetOpenTable( sesid, dbid, "MSysObjects", MJET_GRBIT::TableReadOnly );
            try
            {
                MJetSetCurrentIndex( tableidCatalog, "RootObjects" );
                MJetMakeKey( tableidCatalog, BytesFromObject( MJET_COLTYP::Bit, false, true ), MJET_GRBIT::NewKey );

                MJetMakeKey( tableidCatalog, BytesFromObject( MJET_COLTYP::Text, true, tablename ), MJET_GRBIT::NoGrbit );
                MJetSeek( tableidCatalog, MJET_GRBIT::SeekEQ );
                array<System::Byte>^ objidTable = MJetRetrieveColumn( tableidCatalog, MJetGetColumnInfo( tableidCatalog, "ObjidTable" ).Columnid );
                MJetSetCurrentIndex( tableidCatalog, "Id" );
                MJetMakeKey( tableidCatalog, objidTable, MJET_GRBIT::NewKey );
                MJetMakeKey( tableidCatalog, BytesFromObject( MJET_COLTYP::Short, false, 2 ), MJET_GRBIT::NoGrbit );
                MJetMakeKey( tableidCatalog, BytesFromObject( MJET_COLTYP::Long, false, columnid.NativeValue ), MJET_GRBIT::NoGrbit );
                MJetSeek( tableidCatalog, MJET_GRBIT::SeekEQ );

                array<System::Byte>^ columnNameBytes = MJetRetrieveColumn( tableidCatalog, MJetGetColumnInfo( tableidCatalog, "Name" ).Columnid );
                return gcnew System::String( Encoding::ASCII->GetChars(columnNameBytes) );
            }
            finally
            {
                MJetCloseTable( tableidCatalog );
            }
        }

        virtual array<Byte>^ BytesFromObject(
            MJET_COLTYP type,
            Boolean isASCII,
            Object^ o)
        {
            switch( type )
            {
            case MJET_COLTYP::Bit:
                return BitConverter::GetBytes( Convert::ToBoolean( o ) );
            case MJET_COLTYP::UnsignedByte:
                {
                    array<System::Byte>^bytes = gcnew array<System::Byte>(1);
                    bytes[ 0 ] = (Byte)&o;
                    return bytes;
                }
            case MJET_COLTYP::Short:
                return BitConverter::GetBytes( Convert::ToInt16( o ) );
            case MJET_COLTYP::Long:
                return BitConverter::GetBytes( Convert::ToInt32( o ) );
            case MJET_COLTYP::Text:
                if( isASCII )
                {
                    return Encoding::ASCII->GetBytes( o->ToString() );
                }
                else
                {
                    return Encoding::Unicode->GetBytes( o->ToString() );
                }
            }
            return gcnew array<System::Byte>(0);

        }
#else
#endif

        /// <summary>
        /// </summary>
        virtual array<MJET_ENUMCOLUMN>^ MJetEnumerateColumns(
            MJET_TABLEID tableid,
            array<MJET_ENUMCOLUMNID>^ columnids,
            MJET_GRBIT grbit,
            MJET_WRN& wrn)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            array<MJET_ENUMCOLUMN>^ columns = nullptr;
            unsigned long _cEnumColumnId = 0;
            ::JET_ENUMCOLUMNID* _rgEnumColumnId = NULL;
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            const size_t _cbFRB = 8192;
            BYTE _rgbFRB[ _cbFRB ];
            FastReallocBuffer _frb( _cbFRB, _rgbFRB );
            unsigned long _cEnumColumn = 0;
            ::JET_ENUMCOLUMN* _rgEnumColumn = NULL;

            try
            {
                wrn = MJET_WRN::None;
                if ( columnids != nullptr && ( columnids->Length < 0 || columnids->Length > 0xFFFFFFFF ) )
                {
                    String^ param = "columnids.Length";
                    Object^ value = columnids->Length;
                    String^ message = String::Format( "{0} must be between 0 and 4294967295, inclusive", param );
                    throw gcnew ArgumentOutOfRangeException( param, value, message );
                }
                _cEnumColumnId = columnids == nullptr ? 0 : columnids->Length;
                if ( _cEnumColumnId > 0 )
                {
                    if ( ( _rgEnumColumnId = new ::JET_ENUMCOLUMNID[ _cEnumColumnId ] ) == NULL )
                    {
                        throw TranslatedException(gcnew IsamOutOfMemoryException());
                    }
                    for ( unsigned long _iEnumColumnId = 0; _iEnumColumnId < _cEnumColumnId; _iEnumColumnId++ )
                    {
                        ::JET_ENUMCOLUMNID* _pEnumColumnId = &_rgEnumColumnId[ _iEnumColumnId ];

                        _pEnumColumnId->columnid = GetUnmanagedColumnid( columnids[ _iEnumColumnId ].Columnid );

                        if ( columnids[ _iEnumColumnId ].itagSequences != nullptr && ( columnids[ _iEnumColumnId ].itagSequences->Length < 0 || columnids[ _iEnumColumnId ].itagSequences->Length > 0xFFFFFFFF ) )
                        {
                            String ^ param = String::Format( "columnids[{0}].itagSequences.Length", _iEnumColumnId );
                            Object ^ value = columnids[ _iEnumColumnId ].itagSequences->Length;
                            String ^ message = String::Format( "{0} must be between 0 and 4294967295, inclusive", param );
                            throw gcnew ArgumentOutOfRangeException( param, value, message );
                        }
                        _pEnumColumnId->ctagSequence = columnids[ _iEnumColumnId ].itagSequences == nullptr ? 0 : columnids[ _iEnumColumnId ].itagSequences->Length;
                        _pEnumColumnId->rgtagSequence = NULL;
                        if ( _pEnumColumnId->ctagSequence > 0 )
                        {
                            if ( ( _pEnumColumnId->rgtagSequence = new unsigned long[ _pEnumColumnId->ctagSequence ] ) == NULL )
                            {
                                throw TranslatedException(gcnew IsamOutOfMemoryException());
                            }
                            for ( unsigned long _iEnumColumnValue = 0; _iEnumColumnValue < _pEnumColumnId->ctagSequence; _iEnumColumnValue++ )
                            {
                                if ( columnids[ _iEnumColumnId ].itagSequences[ _iEnumColumnValue ] < 1 || columnids[ _iEnumColumnId ].itagSequences[ _iEnumColumnValue ] > 0xFFFFFFFF )
                                {
                                    String ^ param = String::Format( "columnids[{0}].itagSequences[{1}]", _iEnumColumnId , _iEnumColumnValue );
                                    Object ^ value = columnids[ _iEnumColumnId ].itagSequences[ _iEnumColumnValue ] ;
                                    String ^ message = String::Format( "{0} must be between 0 and 4294967295, inclusive", param );
                                    throw gcnew ArgumentOutOfRangeException( param, value, message );
                                }
                                _pEnumColumnId->rgtagSequence[ _iEnumColumnValue ] = columnids[ _iEnumColumnId ].itagSequences[ _iEnumColumnValue ];
                            }
                        }
                    }
                }

                wrn = CallW( ::JetEnumerateColumns( _sesid,
                    _tableid,
                    _cEnumColumnId,
                    _rgEnumColumnId,
                    &_cEnumColumn,
                    &_rgEnumColumn,
                    (JET_PFNREALLOC)FastReallocBuffer::Realloc_,
                    &_frb,
                    (ULONG)-1,
                    ( ( _grbit & ~ JET_bitEnumerateIgnoreDefault ) | JET_bitEnumerateCompressOutput ) ) );

                columns = gcnew array<MJET_ENUMCOLUMN>( _cEnumColumn );
                for ( unsigned long _iEnumColumn = 0; _iEnumColumn < _cEnumColumn; _iEnumColumn++ )
                {
                    ::JET_ENUMCOLUMN* _pEnumColumn = &_rgEnumColumn[ _iEnumColumn ];
                    ::JET_COLUMNID _columnid = _rgEnumColumn[ _iEnumColumn ].columnid;
                    ::JET_COLUMNBASE _columnbase = { sizeof( ::JET_COLUMNBASE ) };

#if (ESENT_WINXP)
                    Call( ::JetGetTableColumnInfo( _sesid, _tableid, (_TCHAR *)&_columnid, &_columnbase, sizeof( _columnbase ), JET_ColInfoByColid ) );

                    columns[ _iEnumColumn ].Columnid = MakeManagedColumnid( _columnid, (MJET_COLTYP)_columnbase.coltyp, ( 1252 == _columnbase.cp ) );
                    columns[ _iEnumColumn ].ColumnName = MJetGetColumnName( tableid, columns[ _iEnumColumn ].Columnid );
#else
                    Call( ::JetGetTableColumnInfo( _sesid, _tableid, (_TCHAR *)&_columnid, &_columnbase, sizeof( _columnbase ), JET_ColInfoBaseByColid ) );

                    columns[ _iEnumColumn ].Columnid = MakeManagedColumnid( _columnbase.columnid, (MJET_COLTYP)_columnbase.coltyp, ( 1252 == _columnbase.cp ) );
                    columns[ _iEnumColumn ].ColumnName = MakeManagedString( _columnbase.szBaseColumnName );
#endif

                    switch ( _pEnumColumn->err )
                    {
                    case JET_errSuccess:
                        columns[ _iEnumColumn ].ColumnValues = gcnew array<MJET_ENUMCOLUMNVALUE>( _pEnumColumn->cEnumColumnValue );
                        for ( unsigned long _iEnumColumnValue = 0; _iEnumColumnValue < _pEnumColumn->cEnumColumnValue; _iEnumColumnValue++ )
                        {
                            ::JET_ENUMCOLUMNVALUE* _pEnumColumnValue = &_pEnumColumn->rgEnumColumnValue[ _iEnumColumnValue ];

                            columns[ _iEnumColumn ].ColumnValues[ _iEnumColumnValue ].itagSequence = _pEnumColumnValue->itagSequence;
                            switch ( _pEnumColumnValue->err )
                            {
                            case JET_errSuccess:
                                columns[ _iEnumColumn ].ColumnValues[ _iEnumColumnValue ].data = MakeManagedBytes( (BYTE*)_pEnumColumnValue->pvData, _pEnumColumnValue->cbData );
                                break;

                            case JET_wrnColumnNull:
                                columns[ _iEnumColumn ].ColumnValues[ _iEnumColumnValue ].data = nullptr;
                                break;

#if (ESENT_WINXP)
#else
                            case JET_wrnColumnNotInRecord:
#endif
                            case JET_wrnColumnPresent:
                                columns[ _iEnumColumn ].ColumnValues[ _iEnumColumnValue ].data = MakeManagedBytes( NULL, 0 );
                                break;

                            default:
                                //System::Diagnostics::Debug::Assert( false );
                                break;
                            }
                        }
                        break;

                    case JET_wrnColumnSingleValue:
                        columns[ _iEnumColumn ].ColumnValues = gcnew array<MJET_ENUMCOLUMNVALUE>( 1 );
                        columns[ _iEnumColumn ].ColumnValues[ 0 ].itagSequence = 1;
                        columns[ _iEnumColumn ].ColumnValues[ 0 ].data = MakeManagedBytes( (BYTE*)_pEnumColumn->pvData, _pEnumColumn->cbData );
                        break;

                    case JET_wrnColumnNull:
                        columns[ _iEnumColumn ].ColumnValues = gcnew array<MJET_ENUMCOLUMNVALUE>( 1 );
                        columns[ _iEnumColumn ].ColumnValues[ 0 ].itagSequence = 1;
                        columns[ _iEnumColumn ].ColumnValues[ 0 ].data = nullptr;
                        break;

                    case JET_wrnColumnPresent:
                        columns[ _iEnumColumn ].ColumnValues = gcnew array<MJET_ENUMCOLUMNVALUE>(0);
                        break;

                    default:
                        //System::Diagnostics::Debug::Assert( false );
                        break;
                    }
                }
            }
            __finally
            {
                if ( _rgEnumColumnId )
                {
                    for ( unsigned long _iEnumColumnId = 0; _iEnumColumnId < _cEnumColumnId; _iEnumColumnId++ )
                    {
                        ::JET_ENUMCOLUMNID* _pEnumColumnId = &_rgEnumColumnId[ _iEnumColumnId ];

                        delete [] _pEnumColumnId->rgtagSequence;
                    }
                    delete [] _rgEnumColumnId;
                }
                _frb.FreeEnumeratedColumns( _cEnumColumn, _rgEnumColumn );
            }

            return columns;
        }

        /// <summary>
        /// </summary>
        virtual array<MJET_ENUMCOLUMN>^ MJetEnumerateColumns(
            MJET_TABLEID tableid,
            array<MJET_ENUMCOLUMNID>^ columnids,
            MJET_GRBIT grbit)
        {
            MJET_WRN wrn;

            return MJetEnumerateColumns( tableid, columnids, grbit, wrn );
        }

        /// <summary>
        /// </summary>
        virtual array<MJET_ENUMCOLUMN>^ MJetEnumerateColumns(MJET_TABLEID tableid, array<MJET_ENUMCOLUMNID>^ columnids)
        {
            MJET_WRN wrn;

            return MJetEnumerateColumns( tableid, columnids, MJET_GRBIT::NoGrbit, wrn );
        }

        /// <summary>
        /// </summary>
        virtual array<MJET_ENUMCOLUMN>^ MJetEnumerateColumns(MJET_TABLEID tableid)
        {
            array<MJET_ENUMCOLUMNID>^ columnids;
            MJET_WRN wrn;

            return MJetEnumerateColumns( tableid, columnids, MJET_GRBIT::NoGrbit, wrn );
        }

#if (ESENT_WINXP)
#else
        virtual MJET_RECSIZE MJetGetRecordSize(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_RECSIZE2 _recsize = {0};
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;

            Call (::JetGetRecordSize2(_sesid, _tableid, &_recsize, _grbit));
            return MakeManagedRecSize(_recsize);
        }

        virtual MJET_RECSIZE MJetGetRecordSize(MJET_TABLEID tableid)
        {
            return MJetGetRecordSize( tableid, MJET_GRBIT::NoGrbit );
        }
#endif

        /// <summary>
        /// </summary>
        virtual MJET_WRN MJetSetColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            array<Byte>^ data,
            MJET_GRBIT grbit,
            MJET_SETINFO setinfo)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_COLUMNID _columnid = GetUnmanagedColumnid( columnid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            ::JET_SETINFO _setinfo = GetUnmanagedSetinfo( setinfo );
            void * _pvData = 0;
            int _cbData;
            MJET_WRN wrn;

            try
            {
                GetUnmanagedBytes( data, &_pvData, &_cbData );
                wrn = CallW( ::JetSetColumn( _sesid, _tableid, _columnid, _pvData, _cbData, _grbit, &_setinfo ) );
            }
            __finally
            {
                FreeUnmanagedBytes( _pvData );
            }
            return wrn;
        }

        /// <summary>
        /// </summary>
        virtual MJET_WRN MJetSetColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            array<Byte>^ data,
            MJET_GRBIT grbit)
        {
            MJET_SETINFO setinfo;
            setinfo.IbLongValue = 0;
            setinfo.ItagSequence = 1;
            return MJetSetColumn( tableid, columnid, data, grbit, setinfo );
        }

        /// <summary>
        /// </summary>
        virtual MJET_WRN MJetSetColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            array<Byte>^ data)
        {
            return MJetSetColumn( tableid, columnid, data, MJET_GRBIT::NoGrbit );
        }

        /*

        UNDONE

        JET_ERR JET_API MJetSetColumns(
        JET_SESID       sesid,
        JET_TABLEID     tableid,
        JET_SETCOLUMN   *psetcolumn,
        unsigned long   csetcolumn );

        */

        /// <summary>
        /// </summary>
        virtual void MJetPrepareUpdate(MJET_TABLEID tableid, MJET_PREP prep)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            unsigned long _prep = ( unsigned long ) prep;
            Call( ::JetPrepareUpdate( _sesid, _tableid, _prep ) );
        }

        /// <summary>
        /// </summary>
        virtual MJET_RECPOS MJetGetRecordPosition(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_RECPOS _recpos;
            Call( ::JetGetRecordPosition( _sesid, _tableid, &_recpos, sizeof( ::JET_RECPOS ) ) );

            return MakeManagedRecpos( _recpos );
        }

        /// <summary>
        /// </summary>
        virtual void MJetGotoPosition(MJET_TABLEID tableid, MJET_RECPOS recpos)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_RECPOS _recpos = GetUnmanagedRecpos( recpos );
            Call( ::JetGotoPosition( _sesid, _tableid, &_recpos ) );
        }

        /*

        UNDONE

        JET_ERR JET_API MJetGetCursorInfo(
        JET_SESID       sesid,
        JET_TABLEID     tableid,
        void            *pvResult,
        unsigned long   cbMax,
        unsigned long   InfoLevel );

        */

        /// <summary>
        /// </summary>
        virtual MJET_TABLEID MJetDupCursor(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            ::JET_TABLEID _tableidNew;
            Call( ::JetDupCursor( _sesid, _tableid, &_tableidNew, _grbit ) );

            return MakeManagedTableid( _tableidNew, tableid.Sesid, tableid.Dbid );
        }

        /// <summary>
        /// </summary>
        virtual MJET_TABLEID MJetDupCursor(MJET_TABLEID tableid)
        {
            return MJetDupCursor( tableid, MJET_GRBIT::NoGrbit );
        }

        /// <summary>
        /// </summary>
        virtual String^ MJetGetCurrentIndex(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );

            _TCHAR _szName[JET_cbNameMost+1];
            Call( ::JetGetCurrentIndex( _sesid, _tableid, _szName, _countof( _szName ) ) );

            String ^ name = MakeManagedString( _szName );
            return name;
        }

        /// <summary>
        /// </summary>
        virtual void MJetSetCurrentIndex(MJET_TABLEID tableid, String^ name)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            _TCHAR * _szIndexName = 0;

            try
            {
                _szIndexName = GetUnmanagedString( name );
                Call( ::JetSetCurrentIndex( _sesid, _tableid, _szIndexName ) ) ;
            }
            __finally
            {
                FreeUnmanagedString( _szIndexName );
            }
        }

        /// <summary>
        /// </summary>
        virtual void MJetSetCurrentIndex(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            _TCHAR * _szIndexName = 0;

            try
            {
                _szIndexName = GetUnmanagedString( name );
                Call( ::JetSetCurrentIndex2( _sesid, _tableid, _szIndexName, _grbit ) ) ;
            }
            __finally
            {
                FreeUnmanagedString( _szIndexName );
            }
        }

        /// <summary>
        /// </summary>
        virtual void MJetSetCurrentIndex(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit,
            Int64 itag)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            _TCHAR * _szIndexName = 0;
            unsigned long _itagSequence = (unsigned long)itag;

            try
            {
                _szIndexName = GetUnmanagedString( name );
                Call( ::JetSetCurrentIndex3( _sesid, _tableid, _szIndexName, _grbit, _itagSequence ) ) ;
            }
            __finally
            {
                FreeUnmanagedString( _szIndexName );
            }
        }

        /*

        UNDONE

        JET_ERR JET_API MJetSetCurrentIndex4(
        JET_SESID       sesid,
        JET_TABLEID     tableid,
        const _TCHAR        *szIndexName,
        JET_INDEXID     *pindexid,
        JET_GRBIT       grbit,
        unsigned long   itagSequence );

        */

        /// <summary>
        /// </summary>
        virtual MJET_WRN MJetMove(
            MJET_TABLEID tableid,
            Int32 rows,
            MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            long _cRow = rows;
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            MJET_WRN wrn = CallW( ::JetMove( _sesid, _tableid, _cRow, _grbit ) ) ;
            return wrn;
        }

        /// <summary>
        /// </summary>
        virtual MJET_WRN MJetMove(MJET_TABLEID tableid, Int32 rows)
        {
            return MJetMove( tableid, rows, MJET_GRBIT::NoGrbit );
        }

        /// <summary>
        /// </summary>
        virtual MJET_WRN MJetMove(
            MJET_TABLEID tableid,
            MJET_MOVE rows,
            MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            long _cRow = (long)rows;
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            MJET_WRN wrn = CallW( ::JetMove( _sesid, _tableid, _cRow, _grbit ) ) ;
            return wrn;
        }

        /// <summary>
        /// </summary>
        virtual MJET_WRN MJetMove(MJET_TABLEID tableid, MJET_MOVE rows)
        {
            return MJetMove( tableid, (long)rows, MJET_GRBIT::NoGrbit );
        }

        /// <summary>
        /// Convenience function that doesn't throw an exception
        /// </summary>
        virtual void MoveBeforeFirst(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );

            if( FCall( ::JetMove( _sesid, _tableid, JET_MoveFirst, 0 ), JET_errNoCurrentRecord ) )
            {
                FCall( ::JetMove( _sesid, _tableid, JET_MovePrevious, 0 ), JET_errNoCurrentRecord );
            }
        }

        /// <summary>
        /// Convenience function that doesn't throw an exception
        /// </summary>
        virtual void MoveAfterLast(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );

            if( FCall( ::JetMove( _sesid, _tableid, JET_MoveLast, 0 ), JET_errNoCurrentRecord ) )
            {
                FCall( ::JetMove( _sesid, _tableid, JET_MoveNext, 0 ), JET_errNoCurrentRecord );
            }
        }

        /// <summary>
        /// Convenience function that doesn't throw an exception if runs out of records,
        /// just returns false
        /// </summary>
        virtual Boolean TryMoveFirst(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );

            return FCall( ::JetMove( _sesid, _tableid, JET_MoveFirst, 0 ), JET_errNoCurrentRecord );
        }

        /// <summary>
        /// Convenience function that doesn't throw an exception if runs out of records,
        /// just returns false
        /// </summary>
        virtual Boolean TryMoveNext(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );

            return FCall( ::JetMove( _sesid, _tableid, JET_MoveNext, 0 ), JET_errNoCurrentRecord );
        }

        /// <summary>
        /// Convenience function that doesn't throw an exception for out of records,
        /// just returns false
        /// </summary>
        virtual Boolean TryMoveLast(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );

            return FCall( ::JetMove( _sesid, _tableid, JET_MoveLast, 0 ), JET_errNoCurrentRecord );
        }

        /// <summary>
        /// Convenience function that doesn't throw an exception if runs out of records,
        /// just returns false
        /// </summary>
        virtual Boolean TryMovePrevious(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );

            return FCall( ::JetMove( _sesid, _tableid, JET_MovePrevious, 0 ), JET_errNoCurrentRecord );
        }

        /// <summary>
        /// Convenience function that doesn't throw an exception if runs out of records,
        /// just returns false
        /// </summary>
        virtual Boolean TryMove(MJET_TABLEID tableid, MJET_MOVE rows)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            long _cRow = (long)rows;

            return FCall( ::JetMove( _sesid, _tableid, _cRow, 0 ), JET_errNoCurrentRecord );
        }

        /// <summary>
        /// Convenience function that doesn't throw an exception if runs out of records,
        /// just returns false
        /// </summary>
        virtual Boolean TryMove(MJET_TABLEID tableid, Int32 rows)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );

            return FCall( ::JetMove( _sesid, _tableid, rows, 0 ), JET_errNoCurrentRecord );
        }

        /// <summary>
        /// </summary>
        virtual void MJetGetLock(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            Call( ::JetGetLock( _sesid, _tableid, _grbit ) );
        }

        /// <summary>
        /// </summary>
        virtual MJET_WRN MJetMakeKey(
            MJET_TABLEID tableid,
            array<Byte>^ data,
            MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;

            void * _pvData = 0;
            int _cbData;
            MJET_WRN wrn;

            try
            {
                GetUnmanagedBytes( data, &_pvData, &_cbData );
                wrn = CallW( ::JetMakeKey( _sesid, _tableid, _pvData, _cbData, _grbit ) );
            }
            __finally
            {
                FreeUnmanagedBytes( _pvData );
            }
            return wrn;
        }

#if (ESENT_WINXP)
#else
        /// <summary>
        /// Prereads the keys, returns the number of keys preread
        /// </summary>
        virtual Int32 MJetPrereadKeys(
            MJET_TABLEID tableid,
            array<array<Byte>^>^ keys,
            MJET_GRBIT grbit)
        {
            const ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            const ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            const ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            const int _ckeys = keys->Length;

            long _ckeysPreread = 0;
            unsigned long * _rgcbKeys = 0;
            void ** _rgpvKeys = 0;

            try
            {
                // allocate the two arrays
                _rgcbKeys = new unsigned long[_ckeys];
                if ( 0 == _rgcbKeys ) {
                    throw TranslatedException(gcnew IsamOutOfMemoryException());
                }
                _rgpvKeys = new void*[_ckeys];
                if ( 0 == _rgpvKeys ) {
                    throw TranslatedException(gcnew IsamOutOfMemoryException());
                }

                // zero-init the arrays
                for( int _ikey = 0; _ikey < _ckeys; ++_ikey )
                {
                    _rgpvKeys[_ikey] = 0;
                    _rgcbKeys[_ikey] = 0;
                }

                // copy the keys
                for( int _ikey = 0; _ikey < _ckeys; ++_ikey )
                {
                    GetUnmanagedBytes( keys[_ikey], _rgpvKeys+_ikey, (int *)_rgcbKeys+_ikey );
                }

                Call( ::JetPrereadKeys( _sesid, _tableid, (const void **)_rgpvKeys, _rgcbKeys, _ckeys, &_ckeysPreread, _grbit ) );
            }
            __finally
            {
                if( _rgpvKeys )
                {
                    for( int _ikey = 0; _ikey < _ckeys; ++_ikey )
                    {
                        FreeUnmanagedBytes( _rgpvKeys[_ikey] );
                    }
                }
                delete[] _rgpvKeys;
                delete[] _rgcbKeys;
            }

            System::Int32 value = _ckeysPreread;
            return value;
        }
#endif

        /// <summary>
        /// </summary>
        virtual MJET_WRN MJetSeek(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            MJET_WRN wrn = CallW( ::JetSeek( _sesid, _tableid, _grbit ) );
            return wrn;
        }

        /// <summary>
        /// Convenience function that doesn't throw an exception if it doesn't find a record, but
        /// just returns false
        /// </summary>
        virtual Boolean TrySeek(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;

            return FCall( ::JetSeek( _sesid, _tableid, _grbit ), JET_errRecordNotFound );
        }

        /// <summary>
        /// Convenience function that doesn't throw an exception if it doesn't find a record, but
        /// just returns false
        /// </summary>
        virtual Boolean TrySeek(MJET_TABLEID tableid)
        {
            return TrySeek( tableid, MJET_GRBIT::SeekEQ );
        }

        /// <summary>
        /// </summary>
        virtual array<Byte>^ MJetGetBookmark(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );

#if (ESENT_WINXP)
            const size_t _cbKeyMost = JET_cbKeyMost;
#else
            const size_t _cbKeyMost = JET_cbKeyMostMost;
#endif
            BYTE _rgbBookmark[ _cbKeyMost ];
            unsigned long _cbActual = 0;

            Call( ::JetGetBookmark( _sesid, _tableid, _rgbBookmark, sizeof( _rgbBookmark ), &_cbActual ) );

            return MakeManagedBytes( _rgbBookmark, _cbActual );
        }

        /// <summary>
        /// </summary>
        virtual MJET_SECONDARYINDEXBOOKMARK MJetGetSecondaryIndexBookmark(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;

#if (ESENT_WINXP)
            const size_t _cbKeyMost = JET_cbKeyMost;
#else
            const size_t _cbKeyMost = JET_cbKeyMostMost;
#endif
            BYTE _rgbSecondaryBookmark[ _cbKeyMost ];
            BYTE _rgbPrimaryBookmark[ _cbKeyMost ];
            unsigned long _cbActualSecondary = 0;
            unsigned long _cbActualPrimary = 0;

            Call( ::JetGetSecondaryIndexBookmark(
                _sesid,
                _tableid,
                _rgbSecondaryBookmark,
                sizeof( _rgbSecondaryBookmark ),
                &_cbActualSecondary,
                _rgbPrimaryBookmark,
                sizeof( _rgbPrimaryBookmark ),
                &_cbActualPrimary,
                _grbit ) );

            MJET_SECONDARYINDEXBOOKMARK bookmark;
            bookmark.SecondaryBookmark = MakeManagedBytes( _rgbSecondaryBookmark, _cbActualSecondary );
            bookmark.PrimaryBookmark = MakeManagedBytes( _rgbPrimaryBookmark, _cbActualPrimary );
            return bookmark;
        }

        /// <summary>
        /// </summary>
        virtual MJET_SECONDARYINDEXBOOKMARK MJetGetSecondaryIndexBookmark(MJET_TABLEID tableid)
        {
            return MJetGetSecondaryIndexBookmark( tableid, MJET_GRBIT::NoGrbit );
        }

        /*

        UNDONE

        JET_ERR JET_API MJetGetSecondaryIndexBookmark(
        JET_SESID       sesid,
        JET_TABLEID     tableid,
        void *          pvSecondaryKey,
        unsigned long   cbSecondaryKeyMax,
        unsigned long * pcbSecondaryKeyActual,
        void *          pvPrimaryBookmark,
        unsigned long   cbPrimaryBookmarkMax,
        unsigned long * pcbPrimaryKeyActual,
        const JET_GRBIT grbit );

        JET_ERR JET_API MJetCompact(
        JET_SESID       sesid,
        const _TCHAR        *szDatabaseSrc,
        const _TCHAR        *szDatabaseDest,
        JET_PFNSTATUS   pfnStatus,
        JET_CONVERT     *pconvert,
        JET_GRBIT       grbit );

        */

        /// <summary>
        /// </summary>
        virtual MJET_WRN MJetDefragment(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            Int32% passes,
            Int32% seconds,
            MJET_GRBIT grbit)
        {
            ::JET_SESID     _sesid              = GetUnmanagedSesid( sesid );
            ::JET_DBID      _dbid               = GetUnmanagedDbid( dbid );
            _TCHAR *            _szTableName        = 0;
            ::JET_GRBIT     _grbit              = ( ::JET_GRBIT ) grbit;
            unsigned long   _cPasses            = passes;
            unsigned long   _cSeconds           = seconds;

            MJET_WRN wrn;

            try
            {
                _szTableName = GetUnmanagedString( table );
                wrn = CallW( ::JetDefragment( _sesid, _dbid, _szTableName, &_cPasses, &_cSeconds, _grbit ) );
            }
            __finally
            {
                FreeUnmanagedString( _szTableName );
            }

            passes  = _cPasses;
            seconds = _cSeconds;
            return wrn;
        }

        /// <summary>
        /// Jet Online Defragmentation API that takes a callback
        /// </summary>
        virtual MJET_WRN MJetDefragment2(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            Int32% passes,
            Int32% seconds,
            MJetOnEndOnlineDefragCallbackDelegate^ callback,
            MJET_GRBIT grbit)
        {
            ::JET_SESID     _sesid              = GetUnmanagedSesid( sesid );
            ::JET_DBID      _dbid               = GetUnmanagedDbid( dbid );
            _TCHAR *            _szTableName        = 0;
            ::JET_GRBIT     _grbit              = ( ::JET_GRBIT ) grbit;
            unsigned long   _cPasses            = passes;
            unsigned long   _cSeconds           = seconds;

            MJET_WRN wrn = MJET_WRN::None;
            ::JET_CALLBACK pfn = NULL;
            MJetDefragCallbackInfo^ callbackInfo = nullptr;

            try
            {
                _szTableName = GetUnmanagedString( table );

                if (nullptr != callback)
                {
                    // the magic from this sample:
                    // http://msdn2.microsoft.com/en-us/library/367eeye0(en-us,vs.80).aspx
                    callbackInfo = gcnew MJetDefragCallbackInfo(callback);

                    MJET_CALLBACK^ delegatefp = gcnew MJET_CALLBACK(callbackInfo, &MJetDefragCallbackInfo::MJetOnEndOLDCallbackOwn);

                    // The handle will only be freed once the callback has been called
                    callbackInfo->SetGCHandle(GCHandle::Alloc(delegatefp));

                    // Marshal delegate to function pointer
                    pfn = static_cast<::JET_CALLBACK>(Marshal::GetFunctionPointerForDelegate(delegatefp).ToPointer());
                }

                wrn = CallW(::JetDefragment2(_sesid, _dbid, _szTableName, &_cPasses, &_cSeconds, pfn, _grbit));
            }
            __finally
            {
                FreeUnmanagedString( _szTableName );

                // If the call fails, the callback will never be
                // called so we free the gc handle ourselves.
                if (wrn != MJET_WRN::None && callbackInfo != nullptr)
                {
                    callbackInfo->GetGCHandle().Free();
                }
            }

            passes     = _cPasses;
            seconds    = _cSeconds;
            return wrn;
        }

        /*

        UNDONE

        JET_ERR JET_API MJetDefragment3(
        JET_SESID       vsesid,
        const _TCHAR        *szDatabaseName,
        const _TCHAR        *szTableName,
        unsigned long   *pcPasses,
        unsigned long   *pcSeconds,
        JET_CALLBACK    callback,
        void            *pvContext,
        JET_GRBIT       grbit );
        */

#if (ESENT_PUBLICONLY)
#else
        virtual void MJetDatabaseScan(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int32 msecSleep,
            MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            int _cmsecSleep = msecSleep;

            Call( JetDatabaseScan( _sesid, _dbid, NULL, _cmsecSleep, NULL, _grbit ) );
        }
#endif

#if (ESENT_PUBLICONLY)
#else
        virtual void MJetConvertDDL(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_DDLCALLBACKDLL ddlcallbackdll)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            ::JET_DDLCALLBACKDLL _ddlcallbackdll = { 0 };

            try
            {
                _ddlcallbackdll.szOldDLL = GetUnmanagedString( ddlcallbackdll.OldDLL );
                _ddlcallbackdll.szNewDLL = GetUnmanagedString( ddlcallbackdll.NewDLL );
                Call( ::JetConvertDDL( _sesid, _dbid, opDDLConvChangeCallbackDLL, &_ddlcallbackdll, sizeof(_ddlcallbackdll) ) );
            }
            __finally
            {
                FreeUnmanagedString( _ddlcallbackdll.szOldDLL );
                FreeUnmanagedString( _ddlcallbackdll.szNewDLL );
            }

        }

        /// <summary>
        /// </summary>
        virtual void MJetUpgradeDatabase(
            MJET_SESID sesid,
            String^ database,
            String^ streamingFile,
            MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            _TCHAR * _szDbFilename = 0;
            _TCHAR * _szSLVFilename = 0;
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;

            try
            {
                _szDbFilename = GetUnmanagedString( database );
                _szSLVFilename = GetUnmanagedString( streamingFile );
                Call( ::JetUpgradeDatabase( _sesid, _szDbFilename, _szSLVFilename, _grbit ) );
            }
            __finally
            {
                FreeUnmanagedString( _szDbFilename );
                FreeUnmanagedString( _szSLVFilename );
            }
        }
#endif

#if (ESENT_PUBLICONLY)
#else
#if (ESENT_WINXP)
#else
        /// <summary>
        /// </summary>
        virtual void MJetSetMaxDatabaseSize(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pages,
            MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            unsigned long _cpg = (unsigned long)pages;
            Call( ::JetSetMaxDatabaseSize( _sesid, _dbid, _cpg, _grbit ) );
        }

        /// <summary>
        /// </summary>
        virtual Int64 MJetGetMaxDatabaseSize(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            unsigned long _cpg;
            Call( ::JetGetMaxDatabaseSize( _sesid, _dbid, &_cpg, _grbit ) );
            System::Int64 pages = _cpg;
            return pages;
        }
#endif
#endif

        /// <summary>
        /// </summary>
        virtual Int64 MJetSetDatabaseSize(
            MJET_SESID sesid,
            String^ database,
            Int64 pages)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            _TCHAR * _szDatabase = 0;
            unsigned long _cpg = (unsigned long)pages;;
            unsigned long _cpgReal;

            try
            {
                _szDatabase = GetUnmanagedString( database );
                Call( ::JetSetDatabaseSize( _sesid, _szDatabase, _cpg, &_cpgReal ) );
            }
            __finally
            {
                FreeUnmanagedString( _szDatabase );
            }

            System::Int64 realPages = _cpgReal;
            return realPages;
        }

        /// <summary>
        /// </summary>
        virtual Int64 MJetGrowDatabase(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pages)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            unsigned long _cpg = (unsigned long)pages;;
            unsigned long _cpgReal;

            Call( ::JetGrowDatabase( _sesid, _dbid, _cpg, &_cpgReal ) );

            System::Int64 realPages = _cpgReal;
            return realPages;
        }

        /// <summary>
        /// </summary>
        virtual MJET_WRN MJetResizeDatabase(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pages,
            [Out] Int64% actualPages,
            MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            unsigned long _cpg = (unsigned long)pages;
            unsigned long _cpgActual;
            MJET_WRN wrn;

            wrn = CallW( ::JetResizeDatabase( _sesid, _dbid, _cpg, &_cpgActual, _grbit ) );

            actualPages = (System::Int64) _cpgActual;
            
            return wrn;
        }

        /// <summary>
        /// Sets the session to be associated with a thread and debugging context.
        //
        /// When used before a transaction is begun, will allow the session's context to be reset
        /// to make the session mobile / usable across threads even while an transaction is open.
        //
        /// When used after a transaction is already open, will associate this thread and debugging
        /// context with this session, so the session + transaction can be used on the thread.
        //
        /// Note: context is truncated from an Int64 to an Int32 on 32-bit architectures, but since
        /// this is for debugging only, this is fine.
        /// </summary>
        virtual void MJetSetSessionContext(MJET_SESID sesid, IntPtr context)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            JET_API_PTR _context = (JET_API_PTR)context.ToPointer();

            Call( ::JetSetSessionContext( _sesid, _context ) );
        }

        /// <summary>
        /// Resets the session to be un-associated with a thread and debugging context.
        //
        /// Can only be used when a session already had a context set before a transaction was
        /// begun.  This allows the session + open transaction to have a different thread and
        /// session associated with it via JetSetSessionContext.
        /// </summary>
        virtual void MJetResetSessionContext(MJET_SESID sesid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );

            Call( ::JetResetSessionContext( _sesid ) );
        }

#if (ESENT_PUBLICONLY)
#else
        /// <summary>
        /// Get information about the session.
        /// </summary>
        virtual MJET_SESSIONINFO MJetGetSessionInfo(MJET_SESID sesid)
        {
            ::JET_SESID         _sesid          = GetUnmanagedSesid( sesid );
            ::JET_SESSIONINFO   _sessionInfo    = { 0 };

            Call( ::JetGetSessionInfo( _sesid, &_sessionInfo, sizeof(_sessionInfo), JET_SessionInfo ) );

            System::IntPtr trxContext( (void *)_sessionInfo.ulTrxContext );

            MJET_SESSIONINFO sessionInfo;
            sessionInfo.TrxBegin0   = _sessionInfo.ulTrxBegin0;
            sessionInfo.TrxLevel    = _sessionInfo.ulTrxLevel;
            sessionInfo.Procid      = _sessionInfo.ulProcid;
            sessionInfo.Flags       = _sessionInfo.ulFlags;
            sessionInfo.TrxContext  = trxContext;
            return sessionInfo;
        }
#endif

#if (ESENT_PUBLICONLY)
#else
        /// <summary>
        /// General database utilities
        /// </summary>
        virtual void MJetDBUtilities( MJET_DBUTIL dbutil )
        {
            ::JET_DBUTIL _dbutil = { 0 };

            try
            {
                _dbutil = GetUnmanagedDbutil( dbutil );
                Call( ::JetDBUtilities( &_dbutil ) );
            }
            __finally
            {
                FreeUnmanagedDbutil( _dbutil );
            }
        }

#if (ESENT_WINXP)
#else
        /// <summary>
        /// Wrapper function for Header/Trailer update code
        /// </summary>
        virtual void MJetUpdateDBHeaderFromTrailer( String^ database )
        {
            ::JET_DBUTIL_W _dbutil = { 0 };

            wchar_t * _wszDatabase = 0;
            try
            {
                _wszDatabase = GetUnmanagedStringW( database );
                _dbutil.cbStruct = sizeof( _dbutil );
                _dbutil.szDatabase = _wszDatabase;
                _dbutil.op = opDBUTILUpdateDBHeaderFromTrailer;
                Call( ::JetDBUtilitiesW( &_dbutil ) );
            }
            __finally
            {
                FreeUnmanagedStringW( _wszDatabase );
            }
        }
#endif

#if (ESENT_WINXP)
#else
        /// <summary>
        /// Wrapper function for LogChecksum
        /// </summary>
        virtual void MJetChecksumLogFromMemory( MJET_SESID sesid, String^ wszLog, String^ wszBase, array<Byte>^ buffer )
            {
            ::JET_DBUTIL_W  _dbutil   = { 0 };
            wchar_t *       _wszLog   = NULL;
            wchar_t *       _wszBase  = NULL;
            void *          _pvBuffer = NULL;

            try
                {
                _wszLog  = GetUnmanagedStringW( wszLog );
                _wszBase = GetUnmanagedStringW( wszBase );

                _dbutil.cbStruct                     = sizeof( _dbutil );
                _dbutil.op                           = opDBUTILChecksumLogFromMemory;
                _dbutil.sesid                        = GetUnmanagedSesid( sesid );
                _dbutil.checksumlogfrommemory.szLog  = _wszLog;
                _dbutil.checksumlogfrommemory.szBase = _wszBase;

                int _cbBuffer;
                GetUnmanagedBytes( buffer, &_pvBuffer, &_cbBuffer );

                _dbutil.checksumlogfrommemory.pvBuffer  = _pvBuffer;
                _dbutil.checksumlogfrommemory.cbBuffer  = (long)_cbBuffer;
                _dbutil.grbitOptions                    = JET_bitDBUtilOptionSuppressConsoleOutput;

                Call( ::JetDBUtilitiesW( &_dbutil ) );
                }
            __finally
                {
                FreeUnmanagedStringW( _wszLog );
                FreeUnmanagedStringW( _wszBase );
                FreeUnmanagedBytes( _pvBuffer );
                }

            return;
            }
#endif

#endif

        /// <summary>
        /// </summary>
        virtual void MJetGotoBookmark( MJET_TABLEID tableid, array<Byte>^ bookmark )
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );

            void * _pvBookmark = 0;
            int _cbBookmark;

            try
            {
                GetUnmanagedBytes( bookmark, &_pvBookmark, &_cbBookmark );
                Call( ::JetGotoBookmark( _sesid, _tableid, _pvBookmark, _cbBookmark ) );
            }
            __finally
            {
                FreeUnmanagedBytes( _pvBookmark );
            }
        }

        /// <summary>
        /// </summary>
        virtual void MJetGotoSecondaryIndexBookmark(
            MJET_TABLEID tableid,
            array<Byte>^ secondaryKey,
            array<Byte>^ bookmark,
            MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;

            void * _pvSecondaryKey = 0;
            int _cbSecondaryKey;

            void * _pvBookmark = 0;
            int _cbBookmark;

            try
            {
                GetUnmanagedBytes( secondaryKey, &_pvSecondaryKey, &_cbSecondaryKey );
                GetUnmanagedBytes( bookmark, &_pvBookmark, &_cbBookmark );
                Call( ::JetGotoSecondaryIndexBookmark( _sesid, _tableid, _pvSecondaryKey, _cbSecondaryKey, _pvBookmark, _cbBookmark, _grbit ) );
            }
            __finally
            {
                FreeUnmanagedBytes( _pvBookmark );
                FreeUnmanagedBytes( _pvSecondaryKey );
            }
        }

        /*

        UNDONE

        JET_ERR JET_API MJetIntersectIndexes(
        JET_SESID sesid,
        JET_INDEXRANGE * rgindexrange,
        unsigned long cindexrange,
        JET_RECORDLIST * precordlist,
        JET_GRBIT grbit );

        JET_ERR JET_API MJetComputeStats( JET_SESID sesid, JET_TABLEID tableid );

        */

#if (ESENT_WINXP)
#else
        /// <summary>
        /// </summary>
        virtual MJET_TABLEID MJetOpenTemporaryTable(MJET_SESID sesid, MJET_OPENTEMPORARYTABLE% opentemporarytable)
        {
            ::JET_SESID                 _sesid = 0;
            ::JET_OPENTEMPORARYTABLE    _opentemporarytable = { 0 };

            MJET_TABLEID                tableid;

            try
            {
                _sesid = GetUnmanagedSesid( sesid );

                _opentemporarytable = GetUnmanagedOpenTemporaryTable( opentemporarytable );

                Call( ::JetOpenTemporaryTable( _sesid, &_opentemporarytable ) );

                tableid = MakeManagedTableid( _opentemporarytable.tableid, sesid );

                opentemporarytable.Columnids = gcnew array<MJET_COLUMNID>( opentemporarytable.Columndefs->Length );
                for( unsigned long _icolumn = 0; _icolumn < _opentemporarytable.ccolumn; _icolumn++ )
                {
                    opentemporarytable.Columnids[ _icolumn ] = MakeManagedColumnid( _opentemporarytable.prgcolumnid[ _icolumn ],
                        opentemporarytable.Columndefs[ _icolumn ].Coltyp,
                        opentemporarytable.Columndefs[ _icolumn ].ASCII );
                }
            }
            __finally
            {
                FreeUnmanagedOpenTemporaryTable( _opentemporarytable );
            }

            return tableid;
        }
#endif

        /// <summary>
        /// </summary>
        virtual array<MJET_COLUMNID>^ MJetOpenTempTable(
            MJET_SESID sesid,
            array<MJET_COLUMNDEF>^ columndefs,
            MJET_UNICODEINDEX unicodeindex,
            MJET_GRBIT grbit,
            MJET_TABLEID% tableid)
        {
            ::JET_SESID _sesid = 0;
            ::JET_COLUMNDEF* _rgcolumndef = 0;
            unsigned long _ccolumn = 0;
            ::JET_UNICODEINDEX _idxunicode = { 0 };
            ::JET_GRBIT _grbit = 0;
            ::JET_TABLEID _tableid = 0;
            ::JET_COLUMNID* _rgcolumnid = 0;

            array<MJET_COLUMNID>^ columnids = nullptr;

            try
            {
                _sesid = GetUnmanagedSesid( sesid );

                _rgcolumndef = GetUnmanagedColumndefs( columndefs );
                _ccolumn = columndefs->Length;

                _idxunicode = GetUnmanagedUnicodeIndex( unicodeindex );
                _grbit = ( ::JET_GRBIT ) grbit;

                _rgcolumnid = new ::JET_COLUMNID[ _ccolumn ];
                if ( 0 == _rgcolumnid ) {
                    throw TranslatedException(gcnew IsamOutOfMemoryException());
                }
                memset( _rgcolumnid, 0, sizeof( _rgcolumnid[ 0 ] ) * _ccolumn );

                Call( ::JetOpenTempTable3( _sesid, _rgcolumndef, _ccolumn, &_idxunicode, _grbit, &_tableid, _rgcolumnid ) );
                tableid = MakeManagedTableid( _tableid, sesid );

                columnids = gcnew array<MJET_COLUMNID>( _ccolumn );
                for( unsigned long _icolumn = 0; _icolumn < _ccolumn; ++_icolumn )
                {
                    columnids[_icolumn] = MakeManagedColumnid(
                        _rgcolumnid[_icolumn],
                        columndefs[_icolumn].Coltyp,
                        columndefs[_icolumn].ASCII );
                }
            }
            __finally
            {
                FreeUnmanagedColumndefs( _rgcolumndef, _ccolumn );
                delete[] _rgcolumnid;
            }

            return columnids;
        }

        /// <summary>
        /// </summary>
        virtual array<MJET_COLUMNID>^ MJetOpenTempTable(
            MJET_SESID sesid,
            array<MJET_COLUMNDEF>^ columndefs,
            MJET_TABLEID% tableid)
        {
            MJET_UNICODEINDEX unicodeindex;
            unicodeindex.Lcid = 0;
            unicodeindex.MapFlags = 0;
            return MJetOpenTempTable( sesid, columndefs, unicodeindex, MJET_GRBIT::NoGrbit, tableid );
        }

        /*
        UNDONE

        JET_ERR JET_API MJetBackup( const _TCHAR *szBackupPath, JET_GRBIT grbit, JET_PFNSTATUS pfnStatus );
        JET_ERR JET_API MJetBackupInstance( JET_INSTANCE    instance,
        const _TCHAR        *szBackupPath,
        JET_GRBIT       grbit,
        JET_PFNSTATUS   pfnStatus );

        JET_ERR JET_API MJetRestore(const _TCHAR *sz, JET_PFNSTATUS pfn );
        JET_ERR JET_API MJetRestore2(const _TCHAR *sz, const _TCHAR *szDest, JET_PFNSTATUS pfn );

        JET_ERR JET_API MJetRestoreInstance(    JET_INSTANCE instance,
        const _TCHAR *sz,
        const _TCHAR *szDest,
        JET_PFNSTATUS pfn );
        */

        /// <summary>
        /// </summary>
        virtual void MJetSetIndexRange(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            Call( ::JetSetIndexRange( _sesid, _tableid, _grbit ) );
        }

        /// <summary>
        /// Convenience function that doesn't throw an exception if range is empty,
        /// just returns false
        /// </summary>
        virtual Boolean TrySetIndexRange(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            return FCall( ::JetSetIndexRange( _sesid, _tableid, _grbit ), JET_errNoCurrentRecord );
        }

        /// <summary>
        /// </summary>
        virtual void ResetIndexRange(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            (void)FCall( ::JetSetIndexRange( _sesid, _tableid, JET_bitRangeRemove ), JET_errInvalidOperation );
        }

        /// <summary>
        /// </summary>
        virtual Int64 MJetIndexRecordCount(MJET_TABLEID tableid, Int64 maxRecordsToCount)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            unsigned long _crec;
            unsigned long _crecMax = (unsigned long)maxRecordsToCount;
            Call( ::JetIndexRecordCount( _sesid, _tableid, &_crec, _crecMax ) );

            System::Int64 records = _crec;
            return records;
        }

        /// <summary>
        /// </summary>
        virtual array<Byte>^ MJetRetrieveKey(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;

#if (ESENT_WINXP)
            const size_t _cbKeyMost = JET_cbKeyMost;
#else
            const size_t _cbKeyMost = JET_cbKeyMostMost;
#endif
            BYTE _rgbKey[ _cbKeyMost + 1]; // +1 for keys made with prefix or wildcard
            unsigned long _cbActual;

            Call( ::JetRetrieveKey( _sesid, _tableid, _rgbKey, sizeof( _rgbKey ), &_cbActual, _grbit ) );

            return MakeManagedBytes( _rgbKey, _cbActual );
        }

        /// <summary>
        /// </summary>
        virtual array<Byte>^ MJetRetrieveKey(MJET_TABLEID tableid)
        {
            return MJetRetrieveKey( tableid, MJET_GRBIT::NoGrbit );
        }

        /*

        UNDONE

        JET_ERR JET_API MJetBeginExternalBackup( JET_GRBIT grbit );
        JET_ERR JET_API MJetBeginExternalBackupInstance( JET_INSTANCE instance, JET_GRBIT grbit );

        JET_ERR JET_API MJetGetAttachInfo( void *pv,
        unsigned long cbMax,
        unsigned long *pcbActual );
        JET_ERR JET_API MJetGetAttachInfoInstance(  JET_INSTANCE    instance,
        void            *pv,
        unsigned long   cbMax,
        unsigned long   *pcbActual );

        JET_ERR JET_API MJetOpenFile( const _TCHAR *szFilename,
        JET_HANDLE  *phfFile,
        unsigned long *pulFileSizeLow,
        unsigned long *pulFileSizeHigh );

        JET_ERR JET_API MJetOpenFileInstance(   JET_INSTANCE instance,
        const _TCHAR *szFilename,
        JET_HANDLE  *phfFile,
        unsigned long *pulFileSizeLow,
        unsigned long *pulFileSizeHigh );

        JET_ERR JET_API MJetOpenFileSectionInstance(
        JET_INSTANCE instance,
        _TCHAR *szFile,
        JET_HANDLE *phFile,
        long iSection,
        long cSections,
        unsigned long *pulSectionSizeLow,
        long *plSectionSizeHigh);

        JET_ERR JET_API MJetReadFile( JET_HANDLE hfFile,
        void *pv,
        unsigned long cb,
        unsigned long *pcb );
        JET_ERR JET_API MJetReadFileInstance(   JET_INSTANCE instance,
        JET_HANDLE hfFile,
        void *pv,
        unsigned long cb,
        unsigned long *pcb );
        JET_ERR JET_API MJetAsyncReadFileInstance(  JET_INSTANCE instance,
        JET_HANDLE hfFile,
        void* pv,
        unsigned long cb,
        JET_OLP *pjolp );

        JET_ERR JET_API MJetCheckAsyncReadFileInstance(     JET_INSTANCE instance,
        void *pv,
        int cb,
        unsigned long pgnoFirst );

        JET_ERR JET_API MJetCloseFile( JET_HANDLE hfFile );
        JET_ERR JET_API MJetCloseFileInstance( JET_INSTANCE instance, JET_HANDLE hfFile );

        JET_ERR JET_API MJetGetLogInfo( void *pv,
        unsigned long cbMax,
        unsigned long *pcbActual );
        JET_ERR JET_API MJetGetLogInfoInstance( JET_INSTANCE instance,
        void *pv,
        unsigned long cbMax,
        unsigned long *pcbActual );

        JET_ERR JET_API MJetGetLogInfoInstance2(    JET_INSTANCE instance,
        void *pv,
        unsigned long cbMax,
        unsigned long *pcbActual,
        JET_LOGINFO * pLogInfo);

        JET_ERR JET_API MJetGetTruncateLogInfoInstance( JET_INSTANCE instance,
        void *pv,
        unsigned long cbMax,
        unsigned long *pcbActual );

        JET_ERR JET_API MJetTruncateLog( void );
        JET_ERR JET_API MJetTruncateLogInstance( JET_INSTANCE instance );

        JET_ERR JET_API MJetEndExternalBackup( void );
        JET_ERR JET_API MJetEndExternalBackupInstance( JET_INSTANCE instance );

        JET_ERR JET_API MJetEndExternalBackupInstance2( JET_INSTANCE instance, JET_GRBIT grbit );

        JET_ERR JET_API MJetExternalRestore(    _TCHAR *szCheckpointFilePath,
        _TCHAR *szLogPath,
        JET_RSTMAP *rgstmap,
        long crstfilemap,
        _TCHAR *szBackupLogPath,
        long genLow,
        long genHigh,
        JET_PFNSTATUS pfn );

        JET_ERR JET_API MJetExternalRestore2(   _TCHAR *szCheckpointFilePath,
        _TCHAR *szLogPath,
        JET_RSTMAP *rgstmap,
        long crstfilemap,
        _TCHAR *szBackupLogPath,
        JET_LOGINFO * pLogInfo,
        _TCHAR *szTargetInstanceName,
        _TCHAR *szTargetInstanceLogPath,
        _TCHAR *szTargetInstanceCheckpointPath,
        JET_PFNSTATUS pfn );

        JET_ERR JET_API MJetRegisterCallback(
        JET_SESID               sesid,
        JET_TABLEID             tableid,
        JET_CBTYP               cbtyp,
        JET_CALLBACK            pCallback,
        void *                  pvContext,
        JET_HANDLE              *phCallbackId );

        JET_ERR JET_API MJetUnregisterCallback(
        JET_SESID               sesid,
        JET_TABLEID             tableid,
        JET_CBTYP               cbtyp,
        JET_HANDLE              hCallbackId );

        JET_ERR JET_API MJetFreeBuffer( BYTE *pbBuf );

        JET_ERR JET_API MJetSetLS(
        JET_SESID       sesid,
        JET_TABLEID     tableid,
        JET_LS          ls,
        JET_GRBIT       grbit );

        JET_ERR JET_API MJetGetLS(
        JET_SESID       sesid,
        JET_TABLEID     tableid,
        JET_LS          *pls,
        JET_GRBIT       grbit );

        JET_ERR JET_API MJetOSSnapshotPrepare( JET_OSSNAPID * psnapId, const JET_GRBIT grbit );
        JET_ERR JET_API MJetOSSnapshotPrepareInstance( JET_OSSNAPID snapId, JET_INSTANCE    instance, const JET_GRBIT grbit );
        JET_ERR JET_API MJetOSSnapshotFreeze( const JET_OSSNAPID snapId, unsigned long *pcInstanceInfo, JET_INSTANCE_INFO ** paInstanceInfo, const JET_GRBIT grbit );
        JET_ERR JET_API MJetOSSnapshotThaw( const JET_OSSNAPID snapId, const JET_GRBIT grbit );
        JET_ERR JET_API MJetOSSnapshotAbort( const JET_OSSNAPID snapId, const JET_GRBIT grbit );
        JET_ERR JET_API MJetOSSnapshotTruncateLog( const JET_OSSNAPID snapId, const JET_GRBIT grbit );
        JET_ERR JET_API MJetOSSnapshotTruncateLogInstance( const JET_OSSNAPID snapId, JET_INSTANCE  instance, const JET_GRBIT grbit );
        JET_ERR JET_API MJetOSSnapshotGetFreezeInfo( const JET_OSSNAPID snapId, unsigned long *pcInstanceInfo, JET_INSTANCE_INFO ** paInstanceInfo, const JET_GRBIT grbit );
        JET_ERR JET_API MJetOSSnapshotEnd( const JET_OSSNAPID snapId, const JET_GRBIT grbit );
        */

        /// <summary>
        /// Returns whether the interop layer calls the native ESE Unicode or ASCII APIs.
        /// </summary>
        virtual Boolean IsNativeUnicode()
        {
#ifdef JET_UNICODE
            return true;
#else
            return false;
#endif
        }

#if (ESENT_PUBLICONLY)
#else

        /// <summary>
        /// Returns if the build type is DEBUG for the interop layer.
        /// </summary>
        virtual Boolean IsDbgFlagDefined()
        {
#ifdef DEBUG
            return true;
#else
            return false;
#endif
        }

        /// <summary>
        /// Returns if the RTM macro is defined for the interop layer.
        /// </summary>
        virtual Boolean IsRtmFlagDefined()
        {
            // For debug builds, always consider the RTM flag to be off, as it always contains more expensive code.
            if ( IsDbgFlagDefined() )
            {
                return false;
            }
            else
            {
#ifdef RTM
                return true;
#else
                return false;
#endif
            }
        }

        /// <summary>
        /// Given an unmanaged JET_EMITDATACTX , return a managed MJET_EMITDATACTX struct.
        /// </summary>
        virtual MJET_EMITDATACTX MakeManagedEmitLogDataCtx(const ::JET_EMITDATACTX& _emitctx)
        {
            MJET_EMITDATACTX emitctx;

            emitctx.Version = _emitctx.dwVersion;
            emitctx.SequenceNum = _emitctx.qwSequenceNum;
            emitctx.GrbitOperationalFlags = (MJET_GRBIT)_emitctx.grbitOperationalFlags;
            emitctx.LogDataLgpos = MakeManagedLgpos( _emitctx.lgposLogData );
            emitctx.LogDataSize = _emitctx.cbLogData;

            emitctx.EmitTime = MakeManagedDateTime( _emitctx.logtimeEmit);

            return emitctx;
        }

        /// <summary>
        /// Given a managed array of bytes holding an image of an unmanged JET_EMITDATACTX struct,
        /// load a managed MJET_EMITDATACTX.
        /// </summary>

        virtual MJET_EMITDATACTX MakeManagedEmitLogDataCtxFromBytes(array<Byte>^ bytes)
        {
            if ( bytes->Length != sizeof( ::JET_EMITDATACTX ) )
            {
                throw TranslatedException(gcnew IsamInvalidParameterException());
            }

            pin_ptr<Byte>pBytes = &bytes[0];
            ::JET_EMITDATACTX* pEmit = (::JET_EMITDATACTX*)pBytes;

            return MakeManagedEmitLogDataCtx( *pEmit );
        }

        /// <summary>
        /// This is the API wrapper for the native JetConsumeLogData API.  Note the data is an array of bytes
        /// so combines the typical void * pv and ULONG cb native params into one param.
        ///
        /// WARNING: This is a lossy function. The MakeUnmanagedEmitLogDataCtx() does not support conversion of the log timestamp.
        /// The alternate MJetConsumeLogData() (below) is used by HA to ensure binary preservation of the emit metadata.
        ///
        /// </summary>
        virtual MJET_WRN MJetConsumeLogData(
            MJET_INSTANCE instance,
            MJET_EMITDATACTX EmitLogDataCtx,
            array<Byte>^ data,
            MJET_GRBIT grbit)
        {
            ::JET_INSTANCE _inst = GetUnmanagedInst( instance );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            ::JET_EMITDATACTX _emitctx = MakeUnmanagedEmitLogDataCtx( EmitLogDataCtx );
            void * _pvData = NULL;
            int _cbData = 0;

            MJET_WRN wrn;

            try
            {
                GetUnmanagedBytes( data, &_pvData, &_cbData );
                wrn = CallW( ::JetConsumeLogData( _inst, &_emitctx, _pvData, _cbData, _grbit ) );
            }
            __finally
            {
                FreeUnmanagedBytes( _pvData );
            }

            return wrn;
        }

        /// <summary>
        /// This variant of MJetConsumeLogData is used by HA to allow for a binary pass-through of the emit metadata so that
        /// we avoid converting between managed/unmanged EMITDATACTX, which is currently a lossy transformation.
        /// </summary>
        virtual MJET_WRN MJetConsumeLogData(
            MJET_INSTANCE instance,
            array<Byte>^ emitMetaDataAsManagedBytes,
            array<Byte>^ data,
            MJET_GRBIT grbit)
        {
            ::JET_INSTANCE _inst = GetUnmanagedInst( instance );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            void * _pvData = NULL;
            int _cbData = 0;
            void * _pvEmitMetaData = NULL;
            int _cbEmitMetaData = 0;

            MJET_WRN wrn;

            try
            {
                GetUnmanagedBytes( data, &_pvData, &_cbData );

                GetUnmanagedBytes( emitMetaDataAsManagedBytes, &_pvEmitMetaData, &_cbEmitMetaData );

                wrn = CallW( ::JetConsumeLogData( _inst, (::JET_EMITDATACTX*)_pvEmitMetaData,
                    _pvData, _cbData, _grbit ) );
            }
            __finally
            {
                FreeUnmanagedBytes( _pvData );
                FreeUnmanagedBytes( _pvEmitMetaData );
            }

            return wrn;
        }
#endif // ESENT_PUBLICONLY

        /// <summary>
        /// </summary>
        virtual void MJetStopServiceInstance2(
            MJET_INSTANCE instance,
            MJET_GRBIT grbit)
        {
            ::JET_INSTANCE _inst = GetUnmanagedInst( instance );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;

            MJET_WRN wrn;

            try
            {
                wrn = CallW( ::JetStopServiceInstance2( _inst, _grbit ) );
            }
            __finally
            {
            }

            return;
        }

#ifdef INTERNALUSE
        /// <summary>
        /// </summary>
        virtual void MJetTracing(MJET_TRACEOP traceOp, MJET_TRACETAG traceTag, IntPtr ul)
        {
            ::JET_TRACEOP jetTraceOp = (JET_TRACEOP)traceOp;
            ::JET_TRACETAG jetTraceTag = (JET_TRACETAG)traceTag;
            ::JET_API_PTR jetUl = (JET_API_PTR)ul.ToPointer();
            Call( ::JetTracing( jetTraceOp, jetTraceTag, jetUl ) );
        }
    
        /// <summary>
        /// </summary>
        virtual void MJetTracing(MJET_TRACEOP traceOp, MJET_TRACETAG traceTag, MJET_DBID dbid)
        {
            ::JET_TRACEOP jetTraceOp = (JET_TRACEOP)traceOp;
            ::JET_TRACETAG jetTraceTag = (JET_TRACETAG)traceTag;
            ::JET_DBID jetDbid = GetUnmanagedDbid(dbid);
            Call( ::JetTracing( jetTraceOp, jetTraceTag, (Int64)jetDbid ) );
        }
#endif

    private:
        /// <summary>
        /// </summary>
        static void FreeUnmanagedRstmap(::JET_RSTMAP _rstmap)
        {
            FreeUnmanagedString( _rstmap.szDatabaseName );
            FreeUnmanagedString( _rstmap.szNewDatabaseName );
        }

        /// <summary>
        /// *The caller is responsible for deleting the memory this routine allocates*
        /// </summary>
        inline static char * GetUnmanagedStringA(String^ pstr)
        {
            return (char *)Marshal::StringToHGlobalAnsi(pstr).ToPointer();
        }

        /// <summary>
        /// </summary>
        inline static void FreeUnmanagedStringA(__in_z char * _sz)
        {
            System::IntPtr hGlobal( _sz );
            Marshal::FreeHGlobal( hGlobal );
        }

        inline static String^ MakeManagedStringA(__in_z const char * _sz)
        {
            return Marshal::PtrToStringAnsi( (IntPtr) const_cast<char *>(_sz) );
        }

        inline static String^ MakeManagedStringA(__in_ecount(len) const char *_sz, _In_ int len)
        {
            return Marshal::PtrToStringAnsi( (IntPtr) const_cast<char *>(_sz), len );
        }

        /// <summary>
        /// Convert a managed string into an unmanaged unicode string.
        /// </summary>
        /// <remarks>
        /// The caller is responsible for deleting the memory this routine allocates
        /// </remarks>
        inline static wchar_t * GetUnmanagedStringW(String^ pstr)
        {
            return (wchar_t *)Marshal::StringToHGlobalUni(pstr).ToPointer();
        }

        /// <summary>
        /// Free an unmanaged string allocated by GetUnmanagedUnicodeString
        /// </summary>
        inline static void FreeUnmanagedStringW(__in_z wchar_t * const _sz)
        {
            System::IntPtr hGlobal( _sz );
            Marshal::FreeHGlobal( hGlobal );
        }

        inline static String^ MakeManagedStringW(__in_z const wchar_t * _sz)
        {
            return Marshal::PtrToStringUni( (IntPtr) const_cast<wchar_t *>(_sz) );
        }

        inline static String^ MakeManagedStringW(__in_ecount(len) const wchar_t *_sz, _In_ int len)
        {
            return Marshal::PtrToStringUni( (IntPtr) const_cast<wchar_t *>(_sz), len );
        }

        /// <summary>
        /// </summary>
        static array<Byte>^ MakeManagedBytes(BYTE* _pb, int _cb)
        {
            array<System::Byte>^ bytes = gcnew array<System::Byte>(_cb);
            if ( _cb > 0 )
            {
                Marshal::Copy( (IntPtr) _pb, bytes, 0, _cb );
            }
            return bytes;
        }

        /// <summary>
        /// Copies a managed Byte array to an aligned unmanaged array.
        /// *The caller is responsible for deleting the memory this routine allocates*
        /// </summary>
        __success( *_ppv != 0 ) void GetAlignedUnmanagedBytes(
            array<Byte>^ bytes,
            __deref_out_bcount(*_pcb) void ** _ppv,
            _Out_ int * _pcb)
        {
            *_ppv = 0;
            *_pcb = 0;

            if ( bytes != nullptr )
            {
                System::Int32 length = bytes->Length;

                *_ppv = VirtualAlloc( 0, max(1, length ), MEM_COMMIT, PAGE_READWRITE );
                if( 0 == *_ppv )
                {
                    throw TranslatedException(gcnew IsamOutOfMemoryException());
                }
                *_pcb = length;

                if ( length > 0 )
                {
                    System::IntPtr dest = (IntPtr) *_ppv;
                    Marshal::Copy( bytes, 0, dest, length );
                }
            }
        }

        /// <summary>
        /// </summary>
        static void FreeAlignedUnmanagedBytes(_In_ void * _pv)
        {
            if ( _pv != NULL )
            {
                VirtualFree( _pv, 0, MEM_RELEASE );
            }
        }

        /// <summary>
        /// Copies a managed Byte array to an unmanaged array.
        /// *The caller is responsible for deleting the memory this routine allocates*
        /// </summary>
        void GetUnmanagedBytes(
            array<Byte>^ bytes,
            void ** _ppv,
            int * _pcb)
        {
            *_ppv = 0;
            *_pcb = 0;

            if ( bytes != nullptr )
            {
                System::Int32 length = bytes->Length;

                *_ppv = new BYTE[max( 1, length )];
                if( 0 == *_ppv )
                {
                    throw TranslatedException(gcnew IsamOutOfMemoryException());
                }
                *_pcb = length;

                if ( length > 0 )
                {
                    // System::IntPtr dest( *_ppv ); //Ewww!
                    System::IntPtr dest = (IntPtr) *_ppv;
                    Marshal::Copy( bytes, 0, dest, length );
                }
            }
        }

        /// <summary>
        /// </summary>
        static void FreeUnmanagedBytes(void * _pv)
        {
            delete[] _pv;
        }

#if (ESENT_WINXP)
#else
        /// <summary>
        /// </summary>
        static ::JET_TUPLELIMITS GetUnmanagedTupleLimits(MJET_TUPLELIMITS tuplelimits)
        {
            ::JET_TUPLELIMITS _tuplelimits = { 0 };

            _tuplelimits.chLengthMin        = ( unsigned long )tuplelimits.LengthMin;
            _tuplelimits.chLengthMax    = ( unsigned long )tuplelimits.LengthMax;
            _tuplelimits.chToIndexMax   = ( unsigned long )tuplelimits.ToIndexMax;
            _tuplelimits.cchIncrement   = ( unsigned long )tuplelimits.OffsetIncrement;
            _tuplelimits.ichStart       = ( unsigned long )tuplelimits.OffsetStart;
            return _tuplelimits;
        }
#endif

        /// <summary>
        /// </summary>
        static ::JET_UNICODEINDEX GetUnmanagedUnicodeIndex(MJET_UNICODEINDEX unicodeindex)
        {
            ::JET_UNICODEINDEX _unicodeindex = { 0 };

            _unicodeindex.lcid          = ( unsigned long )unicodeindex.Lcid;
            _unicodeindex.dwMapFlags    = ( unsigned long )unicodeindex.MapFlags;

            return _unicodeindex;
        }

        /// <summary>
        /// </summary>
        static ::JET_CONDITIONALCOLUMN GetUnmanagedConditionalColumn(MJET_CONDITIONALCOLUMN conditionalcolumn)
        {
            ::JET_CONDITIONALCOLUMN _conditionalcolumn = { 0 };

            _conditionalcolumn.cbStruct     = sizeof( _conditionalcolumn );
            _conditionalcolumn.szColumnName = GetUnmanagedString( conditionalcolumn.ColumnName );
            _conditionalcolumn.grbit        = (::JET_GRBIT) conditionalcolumn.Grbit;

            return _conditionalcolumn;
        }

        /// <summary>
        /// </summary>
        static void FreeUnmanagedConditionalColumn(::JET_CONDITIONALCOLUMN _conditionalcolumn)
        {
            FreeUnmanagedString( _conditionalcolumn.szColumnName );
        }

        /// <summary>
        /// </summary>
        ::JET_CONDITIONALCOLUMN* GetUnmanagedConditionalColumns(array<MJET_CONDITIONALCOLUMN>^ conditionalcolumns)
        {
            int _i = 0;
            const int _cconditionalcolumn = conditionalcolumns->Length;
            ::JET_CONDITIONALCOLUMN* _pconditioalcolumn = 0;

            try
            {
                _pconditioalcolumn = new ::JET_CONDITIONALCOLUMN[ _cconditionalcolumn ];
                if ( 0 == _pconditioalcolumn )
                {
                    throw TranslatedException(gcnew IsamOutOfMemoryException());
                }
                memset( _pconditioalcolumn, 0, sizeof( *_pconditioalcolumn ) * _cconditionalcolumn );

                for ( _i = 0; _i < _cconditionalcolumn; ++_i )
                {
                    _pconditioalcolumn[ _i ] = GetUnmanagedConditionalColumn( conditionalcolumns[ _i ] );
                }
                //Assert( _cconditionalcolumn == _i );
            }
            catch ( ... )
            {
                FreeUnmanagedConditionalColumns( _pconditioalcolumn, _i );
                throw;
            }
            return _pconditioalcolumn;
        }

        /// <summary>
        /// </summary>
        static void FreeUnmanagedConditionalColumns(::JET_CONDITIONALCOLUMN* _pconditionalcolumn, const int _cconditionalcolumn)
        {
            for ( int _i = 0; _i < _cconditionalcolumn; ++_i )
            {
                FreeUnmanagedConditionalColumn( _pconditionalcolumn[ _i ] );
            }
            delete[] _pconditionalcolumn;
        }

#if (ESENT_WINXP)
#else
        /// <summary>
        /// </summary>
        static ::JET_SPACEHINTS GetUnmanagedSpaceHints(MJET_SPACEHINTS spacehints)
        {
            ::JET_SPACEHINTS _spacehints = { 0 };

            _spacehints.cbStruct            = sizeof( _spacehints );
            _spacehints.ulInitialDensity    = (unsigned long) spacehints.InitialDensity;
            _spacehints.cbInitial           = (unsigned long) spacehints.InitialSize;
            _spacehints.grbit               = ( ::JET_GRBIT ) spacehints.Grbit;
            _spacehints.ulMaintDensity      = (unsigned long) spacehints.MaintenanceDensity;
            _spacehints.ulGrowth            = (unsigned long) spacehints.GrowthRate;
            _spacehints.cbMinExtent         = (unsigned long) spacehints.MinimumExtentSize;
            _spacehints.cbMaxExtent         = (unsigned long) spacehints.MaximumExtentSize;

            return _spacehints;
        }

        static MJET_SPACEHINTS MakeManagedSpaceHints(const ::JET_SPACEHINTS& _spacehints)
        {
            MJET_SPACEHINTS spacehints;

            spacehints.InitialDensity           = _spacehints.ulInitialDensity;
            spacehints.InitialSize          = _spacehints.cbInitial;
            spacehints.Grbit                = (MJET_GRBIT) _spacehints.grbit;
            spacehints.MaintenanceDensity   = _spacehints.ulMaintDensity;
            spacehints.GrowthRate           = _spacehints.ulGrowth;
            spacehints.MinimumExtentSize    = _spacehints.cbMinExtent;
            spacehints.MaximumExtentSize    = _spacehints.cbMaxExtent;

            return spacehints;
        }

        /// Read-only space hints are used in debug to make sure ESE doesn't modify the
        /// passed-in space hints.
#if DEBUG
        static bool _fROSpaceHints = true;
#else
        static bool _fROSpaceHints = false;
#endif

        /// <summary>
        /// </summary>
        ::JET_SPACEHINTS * AllocateUnmanagedSpaceHints(MJET_SPACEHINTS spacehints)
        {
            ::JET_SPACEHINTS * _pspacehints;
            if( _fROSpaceHints )
            {
                _pspacehints = (::JET_SPACEHINTS *)VirtualAlloc( 0, sizeof(::JET_SPACEHINTS), MEM_COMMIT, PAGE_READWRITE );
            }
            else
            {
                _pspacehints = new ::JET_SPACEHINTS();
            }

            if ( 0 == _pspacehints )
            {
                throw TranslatedException(gcnew IsamOutOfMemoryException());
            }
            *_pspacehints = GetUnmanagedSpaceHints( spacehints );

            if( _fROSpaceHints )
            {
                DWORD dwOldProtect;
                VirtualProtect( _pspacehints, sizeof(::JET_SPACEHINTS), PAGE_READONLY, &dwOldProtect );
            }

            return _pspacehints;
        }

        /// <summary>
        /// </summary>
        static void FreeUnmanagedSpaceHints(::JET_SPACEHINTS * _pspacehints)
        {
            if( _fROSpaceHints )
            {
                if( _pspacehints )
                {
                    VirtualFree( _pspacehints, 0, MEM_RELEASE );
                }
            }
            else
            {
                delete _pspacehints;
            }
        }
#endif

#if (ESENT_WINXP)
        /// <summary>
        /// </summary>
        ::JET_INDEXCREATE GetUnmanagedIndexCreate(MJET_INDEXCREATE indexcreate)
        {
            ::JET_INDEXCREATE _indexcreate = { 0 };

            try
            {
                _indexcreate.cbStruct = sizeof( _indexcreate );
                _indexcreate.szIndexName = GetUnmanagedString( indexcreate.IndexName );
                _indexcreate.szKey = GetUnmanagedString( indexcreate.Key );
                _indexcreate.cbKey = ( unsigned long )( indexcreate.Key->Length + 1 ) * sizeof(_TCHAR);
                _indexcreate.grbit = ( ::JET_GRBIT ) indexcreate.Grbit;
                _indexcreate.ulDensity = ( unsigned long )indexcreate.Density;
                _indexcreate.err = 0;

                if ( JET_bitIndexUnicode & _indexcreate.grbit )
                {
                    _indexcreate.pidxunicode = new ::JET_UNICODEINDEX;
                    if ( 0 == _indexcreate.pidxunicode )
                    {
                        throw TranslatedException(gcnew IsamOutOfMemoryException());
                    }

                    _indexcreate.pidxunicode->lcid = ( unsigned long )indexcreate.Lcid;
                    _indexcreate.pidxunicode->dwMapFlags = ( unsigned long )indexcreate.MapFlags;
                }
                else
                {
                    _indexcreate.lcid = ( unsigned long )indexcreate.Lcid;
                }

                _indexcreate.cbVarSegMac = ( unsigned long )indexcreate.VarSegMac;

                // conditional columns
                if( indexcreate.ConditionalColumns )
                {
                    _indexcreate.rgconditionalcolumn = GetUnmanagedConditionalColumns( indexcreate.ConditionalColumns );
                    _indexcreate.cConditionalColumn = indexcreate.ConditionalColumns->Length;
                }
            }
            catch ( ... )
            {
                FreeUnmanagedIndexCreate( _indexcreate );
                throw;
            }

            return _indexcreate;
        }

        /// <summary>
        /// </summary>
        static void FreeUnmanagedIndexCreate(::JET_INDEXCREATE _indexcreate)
        {
            FreeUnmanagedString( _indexcreate.szIndexName );
            FreeUnmanagedString( _indexcreate.szKey );

            if( JET_bitIndexUnicode & _indexcreate.grbit )
            {
                delete _indexcreate.pidxunicode;
            }

            FreeUnmanagedConditionalColumns( _indexcreate.rgconditionalcolumn, _indexcreate.cConditionalColumn );
        }

        /// <summary>
        /// </summary>
        ::JET_INDEXCREATE* GetUnmanagedIndexCreates(array<MJET_INDEXCREATE>^ indexcreates)
        {
            int _i = 0;
            const int _cindexcreate = indexcreates->Length;
            ::JET_INDEXCREATE* _pindexcreate = 0;
            try
            {
                _pindexcreate = new ::JET_INDEXCREATE[ _cindexcreate ];
                if ( 0 == _pindexcreate )
                {
                    throw TranslatedException(gcnew IsamOutOfMemoryException());
                }
                memset( _pindexcreate, 0, sizeof( *_pindexcreate ) * _cindexcreate );

                for ( _i = 0; _i < _cindexcreate; ++_i )
                {
                    _pindexcreate[ _i ] = GetUnmanagedIndexCreate( indexcreates[ _i ] );
                }
                //Assert( _cindexcreate == _i );
            }
            catch ( ... )
            {
                FreeUnmanagedIndexCreates( _pindexcreate, _i );
                throw;
            }

            return _pindexcreate;
        }

        /// <summary>
        /// </summary>
        static void FreeUnmanagedIndexCreates(::JET_INDEXCREATE* _pindexcreate, const int _cindexcreate)
        {
            for ( int _i = 0; _i < _cindexcreate; ++_i )
            {
                FreeUnmanagedIndexCreate( _pindexcreate[ _i ] );
            }
            delete[] _pindexcreate;
        }
#else
        /// <summary>
        /// </summary>
        ::JET_INDEXCREATE2 GetUnmanagedIndexCreate(MJET_INDEXCREATE indexcreate)
        {
            ::JET_INDEXCREATE2 _indexcreate = { 0 };

            try
            {
                _indexcreate.cbStruct = sizeof( _indexcreate );
                _indexcreate.szIndexName = GetUnmanagedString( indexcreate.IndexName );
                _indexcreate.szKey = GetUnmanagedString( indexcreate.Key );
                _indexcreate.cbKey = ( unsigned long )( indexcreate.Key->Length + 1 ) * sizeof(_TCHAR);
                _indexcreate.grbit = ( ::JET_GRBIT ) indexcreate.Grbit;
                _indexcreate.ulDensity = ( unsigned long )indexcreate.Density;
                _indexcreate.err = 0;

                if ( JET_bitIndexUnicode & _indexcreate.grbit )
                {
                    _indexcreate.pidxunicode = new ::JET_UNICODEINDEX;
                    if ( 0 == _indexcreate.pidxunicode )
                    {
                        throw TranslatedException(gcnew IsamOutOfMemoryException());
                    }

                    _indexcreate.pidxunicode->lcid = ( unsigned long )indexcreate.Lcid;
                    _indexcreate.pidxunicode->dwMapFlags = ( unsigned long )indexcreate.MapFlags;
                }
                else
                {
                    _indexcreate.lcid = ( unsigned long )indexcreate.Lcid;
                }

                if ( JET_bitIndexTupleLimits & _indexcreate.grbit )
                {
                    _indexcreate.ptuplelimits = new ::JET_TUPLELIMITS;
                    if ( 0 == _indexcreate.ptuplelimits )
                    {
                        throw TranslatedException(gcnew IsamOutOfMemoryException());
                    }

                    if ( indexcreate.TupleLimits )
                    {
                        *_indexcreate.ptuplelimits = GetUnmanagedTupleLimits( *indexcreate.TupleLimits );
                    }
                    else
                    {
                        _indexcreate.ptuplelimits->chLengthMin = ( unsigned long )indexcreate.LengthMin;
                        _indexcreate.ptuplelimits->chLengthMax = ( unsigned long )indexcreate.LengthMax;
                        _indexcreate.ptuplelimits->chToIndexMax = ( unsigned long )indexcreate.ToIndexMax;
                        _indexcreate.ptuplelimits->cchIncrement = 1;
                        _indexcreate.ptuplelimits->ichStart = 0;
                    }
                }
                else
                {
                    _indexcreate.cbVarSegMac = ( unsigned long )indexcreate.VarSegMac;
                }

                // conditional columns
                if( indexcreate.ConditionalColumns )
                {
                    _indexcreate.rgconditionalcolumn = GetUnmanagedConditionalColumns( indexcreate.ConditionalColumns );
                    _indexcreate.cConditionalColumn = indexcreate.ConditionalColumns->Length;
                }

                _indexcreate.cbKeyMost = indexcreate.KeyLengthMax;

                // space hints
                if( indexcreate.SpaceHints )
                {
                    _indexcreate.pSpacehints = AllocateUnmanagedSpaceHints( *indexcreate.SpaceHints );
                }
            }
            catch ( ... )
            {
                FreeUnmanagedIndexCreate( _indexcreate );
                throw;
            }

            return _indexcreate;
        }

        /// <summary>
        /// </summary>
        static void FreeUnmanagedIndexCreate(_In_ __drv_freesMem(object) ::JET_INDEXCREATE2 _indexcreate)
        {
            FreeUnmanagedString( _indexcreate.szIndexName );
            FreeUnmanagedString( _indexcreate.szKey );

            if( JET_bitIndexTupleLimits & _indexcreate.grbit )
            {
                delete _indexcreate.ptuplelimits;
            }

            if( JET_bitIndexUnicode & _indexcreate.grbit )
            {
                delete _indexcreate.pidxunicode;
            }
            FreeUnmanagedSpaceHints( _indexcreate.pSpacehints );

            FreeUnmanagedConditionalColumns( _indexcreate.rgconditionalcolumn, _indexcreate.cConditionalColumn );
        }

        /// <summary>
        /// </summary>
        ::JET_INDEXCREATE2* GetUnmanagedIndexCreates(array<MJET_INDEXCREATE>^ indexcreates)
        {
            int _i = 0;
            const int _cindexcreate = indexcreates->Length;
            ::JET_INDEXCREATE2* _pindexcreate = 0;
            try
            {
                _pindexcreate = new ::JET_INDEXCREATE2[ _cindexcreate ];
                if ( 0 == _pindexcreate )
                {
                    throw TranslatedException(gcnew IsamOutOfMemoryException());
                }
                memset( _pindexcreate, 0, sizeof( *_pindexcreate ) * _cindexcreate );

                for ( _i = 0; _i < _cindexcreate; ++_i )
                {
                    _pindexcreate[ _i ] = GetUnmanagedIndexCreate( indexcreates[ _i ] );
                }
                //Assert( _cindexcreate == _i );
            }
            catch ( ... )
            {
                FreeUnmanagedIndexCreates( _pindexcreate, _i );
                throw;
            }

            return _pindexcreate;
        }

        /// <summary>
        /// </summary>
        static void FreeUnmanagedIndexCreates(::JET_INDEXCREATE2* _pindexcreate, const int _cindexcreate)
        {
            for ( int _i = 0; _i < _cindexcreate; ++_i )
            {
                FreeUnmanagedIndexCreate( _pindexcreate[ _i ] );
            }
            delete[] _pindexcreate;
        }
#endif

        /// <summary>
        /// </summary>
        ::JET_COLUMNCREATE GetUnmanagedColumnCreate(MJET_COLUMNCREATE columncreate)
        {
            ::JET_COLUMNCREATE _columncreate = { 0 };

            try
            {
                _columncreate.cbStruct = sizeof( _columncreate );
                _columncreate.szColumnName = GetUnmanagedString( columncreate.ColumnName );
                _columncreate.coltyp = ( ::JET_COLTYP ) columncreate.Coltyp;
                _columncreate.cbMax = ( unsigned long )columncreate.MaxLength;
                _columncreate.grbit = ( ::JET_GRBIT ) columncreate.Grbit;

                void* _pv = 0;
                int _cb = 0;
                GetUnmanagedBytes( columncreate.DefaultValue, &_pv, &_cb );
                _columncreate.pvDefault = _pv;
                _columncreate.cbDefault = ( unsigned long )_cb;

                _columncreate.cp = ( unsigned long )columncreate.Cp;
                _columncreate.columnid = 0;
                _columncreate.err = 0;
            }
            catch ( ... )
            {
                FreeUnmanagedColumnCreate( _columncreate );
                throw;
            }

            return _columncreate;
        }

        /// <summary>
        /// </summary>
        static void FreeUnmanagedColumnCreate(::JET_COLUMNCREATE _columncreate)
        {
            FreeUnmanagedString( _columncreate.szColumnName );
            FreeUnmanagedBytes( _columncreate.pvDefault );
        }

        /// <summary>
        /// </summary>
        ::JET_COLUMNCREATE* GetUnmanagedColumnCreates(array<MJET_COLUMNCREATE>^ columncreates)
        {
            int _i = 0;
            const int _ccolumncreate = columncreates->Length;
            ::JET_COLUMNCREATE* _pcolumncreate = 0;

            try
            {
                _pcolumncreate = new ::JET_COLUMNCREATE[ _ccolumncreate ];
                if ( 0 == _pcolumncreate )
                {
                    throw TranslatedException(gcnew IsamOutOfMemoryException());
                }
                memset( _pcolumncreate, 0, sizeof( *_pcolumncreate ) * _ccolumncreate );

                for ( _i = 0; _i < _ccolumncreate; ++_i )
                {
                    _pcolumncreate[ _i ] = GetUnmanagedColumnCreate( columncreates[ _i ] );
                }
                //Assert( _ccolumncreate == _i );
            }
            catch ( ... )
            {
                FreeUnmanagedColumnCreates( _pcolumncreate, _i );
                throw;
            }

            return _pcolumncreate;
        }

        /// <summary>
        /// </summary>
        static void FreeUnmanagedColumnCreates(::JET_COLUMNCREATE* _pcolumncreate, const int _ccolumncreate)
        {
            for ( int _i = 0; _i < _ccolumncreate; ++_i )
            {
                FreeUnmanagedColumnCreate( _pcolumncreate[ _i ] );
            }
            delete[] _pcolumncreate;
        }

#if (ESENT_WINXP)
        /// <summary>
        /// </summary>
        static ::JET_TABLECREATE2 GetUnmanagedTablecreate(MJET_TABLECREATE tablecreate)
        {
            ::JET_TABLECREATE2 _tablecreate = { 0 };

            try
            {
                _tablecreate.cbStruct = sizeof( _tablecreate );
                _tablecreate.szTableName = GetUnmanagedString( tablecreate.TableName );
                _tablecreate.szTemplateTableName = GetUnmanagedString( tablecreate.TemplateTableName );
                _tablecreate.ulPages = ( unsigned long ) tablecreate.Pages;
                _tablecreate.ulDensity = ( unsigned long ) tablecreate.Density;
                _tablecreate.szCallback = 0;
                _tablecreate.cbtyp = JET_cbtypNull;
                _tablecreate.grbit = ( ::JET_GRBIT ) tablecreate.Grbit;
                _tablecreate.tableid = 0;
                _tablecreate.cCreated = 0;

                // columns
                if( tablecreate.ColumnCreates )
                {
                    _tablecreate.rgcolumncreate = GetUnmanagedColumnCreates( tablecreate.ColumnCreates );
                    _tablecreate.cColumns = tablecreate.ColumnCreates->Length;
                }

                // indexes
                if( tablecreate.IndexCreates )
                {
                    _tablecreate.rgindexcreate = GetUnmanagedIndexCreates( tablecreate.IndexCreates );
                    _tablecreate.cIndexes = tablecreate.IndexCreates->Length;
                }
            }
            catch ( ... )
            {
                FreeUnmanagedTablecreate( _tablecreate );
                throw;
            }

            return _tablecreate;
        }

        /// <summary>
        /// </summary>
        static void FreeUnmanagedTablecreate(::JET_TABLECREATE2 _tablecreate)
        {
            FreeUnmanagedString( _tablecreate.szTableName );
            FreeUnmanagedString( _tablecreate.szTemplateTableName );

            FreeUnmanagedColumnCreates( _tablecreate.rgcolumncreate, _tablecreate.cColumns );
            FreeUnmanagedIndexCreates( _tablecreate.rgindexcreate, _tablecreate.cIndexes );
        }
#else
        /// <summary>
        /// </summary>
        ::JET_TABLECREATE3 GetUnmanagedTablecreate(MJET_TABLECREATE tablecreate)
        {
            ::JET_TABLECREATE3 _tablecreate = { 0 };

            try
            {
                _tablecreate.cbStruct = sizeof( _tablecreate );
                _tablecreate.szTableName = GetUnmanagedString( tablecreate.TableName );
                _tablecreate.szTemplateTableName = GetUnmanagedString( tablecreate.TemplateTableName );
                _tablecreate.ulPages = ( unsigned long ) tablecreate.Pages;
                _tablecreate.ulDensity = ( unsigned long ) tablecreate.Density;
                _tablecreate.szCallback = 0;
                _tablecreate.cbtyp = JET_cbtypNull;
                _tablecreate.grbit = ( ::JET_GRBIT ) tablecreate.Grbit;
                _tablecreate.tableid = 0;
                _tablecreate.cCreated = 0;
                _tablecreate.cbSeparateLV = ( unsigned long) tablecreate.SeparateLVSize;

                // columns
                if( tablecreate.ColumnCreates )
                {
                    _tablecreate.rgcolumncreate = GetUnmanagedColumnCreates( tablecreate.ColumnCreates );
                    _tablecreate.cColumns = tablecreate.ColumnCreates->Length;
                }

                // indexes
                if( tablecreate.IndexCreates )
                {
                    _tablecreate.rgindexcreate = GetUnmanagedIndexCreates( tablecreate.IndexCreates );
                    _tablecreate.cIndexes = tablecreate.IndexCreates->Length;
                }

                // space hints
                if( tablecreate.SequentialSpaceHints )
                {
                    _tablecreate.pSeqSpacehints = AllocateUnmanagedSpaceHints( *tablecreate.SequentialSpaceHints );
                }
                if( tablecreate.LongValueSpaceHints )
                {
                    _tablecreate.pLVSpacehints = AllocateUnmanagedSpaceHints( *tablecreate.LongValueSpaceHints );
                }
            }
            catch ( ... )
            {
                FreeUnmanagedTablecreate( _tablecreate );
                throw;
            }

            return _tablecreate;
        }

        /// <summary>
        /// </summary>
        static void FreeUnmanagedTablecreate(::JET_TABLECREATE3 _tablecreate)
        {
            FreeUnmanagedString( _tablecreate.szTableName );
            FreeUnmanagedString( _tablecreate.szTemplateTableName );

            FreeUnmanagedSpaceHints( _tablecreate.pSeqSpacehints );
            FreeUnmanagedSpaceHints( _tablecreate.pLVSpacehints );

            FreeUnmanagedColumnCreates( _tablecreate.rgcolumncreate, _tablecreate.cColumns );
            FreeUnmanagedIndexCreates( _tablecreate.rgindexcreate, _tablecreate.cIndexes );
        }
#endif

#if (ESENT_PUBLICONLY)
#else
        /// <summary>
        /// </summary>
        ::JET_DBUTIL GetUnmanagedDbutil(MJET_DBUTIL dbutil)
        {
            ::JET_DBUTIL    _dbutil     = { 0 };

            _dbutil.cbStruct            = sizeof( _dbutil );
            _dbutil.sesid               = GetUnmanagedSesid( dbutil.Sesid );
            _dbutil.dbid                = GetUnmanagedDbid( dbutil.Dbid );
            _dbutil.tableid             = GetUnmanagedTableid( dbutil.Tableid );

            _dbutil.op                  = (::DBUTIL_OP)( dbutil.Operation );
            _dbutil.edbdump             = (::EDBDUMP_OP)( dbutil.EDBDumpOperation );
            _dbutil.grbitOptions        = (::JET_GRBIT)( dbutil.Grbit );

            _dbutil.pgno                = (long)( dbutil.PageNumber );
            _dbutil.iline               = (long)( dbutil.Line );

            _dbutil.lGeneration         = (long)( dbutil.Generation );
            _dbutil.isec                = (long)( dbutil.Sector );
            _dbutil.ib                  = (long)( dbutil.SectorOffset );

            _dbutil.cRetry              = (long)( dbutil.RetryCount );

            try
            {
                _dbutil.szDatabase = GetUnmanagedString( dbutil.DatabaseName );
                _dbutil.szBackup = GetUnmanagedString( dbutil.BackupName );
                _dbutil.szTable = GetUnmanagedString( dbutil.TableName );
                _dbutil.szIndex = GetUnmanagedString( dbutil.IndexName );
                _dbutil.szIntegPrefix = GetUnmanagedString( dbutil.IntegPrefix );
            }
            catch ( ... )
            {
                FreeUnmanagedDbutil( _dbutil );
                throw;
            }

            return _dbutil;
        }

        /// <summary>
        /// </summary>
        static void FreeUnmanagedDbutil(::JET_DBUTIL _dbutil)
        {
            FreeUnmanagedString( _dbutil.szDatabase );
            FreeUnmanagedString( _dbutil.szBackup );
            FreeUnmanagedString( (_TCHAR *)_dbutil.szTable );
            FreeUnmanagedString( (_TCHAR *)_dbutil.szIndex );
            FreeUnmanagedString( _dbutil.szIntegPrefix );
        }

        /// <summary>
        /// Makes an unmanaged JET_EMITDATACTX given a managed MJET_EMITDATACTX.
        /// This is a lossy conversion.  The "logtime" is not converted since the MakeUnamangedLogtime() does not exist.
        /// To avoid this loss, HA uses an alternate MJetConsumeLogData which simply takes an array of bytes so that there
        /// is a simple binary pass through of bytes from the original "Emit" function to the final "Consume" function.
        ///
        /// </summary>

        ::JET_EMITDATACTX MakeUnmanagedEmitLogDataCtx(MJET_EMITDATACTX& emitctx)
        {
            ::JET_EMITDATACTX   _emitctx    = { 0 };

            _emitctx.cbStruct               = sizeof( _emitctx );
            _emitctx.dwVersion              = (long) emitctx.Version;
            _emitctx.qwSequenceNum          = (unsigned __int64)( emitctx.SequenceNum );
            _emitctx.grbitOperationalFlags  = (::JET_GRBIT)( emitctx.GrbitOperationalFlags );
            _emitctx.lgposLogData           = MakeUnmanagedLgpos( emitctx.LogDataLgpos );
            _emitctx.cbLogData              = (long) emitctx.LogDataSize;

            return _emitctx;
        }
#endif

#if (ESENT_PUBLICONLY)
#else
#if (ESENT_WINXP)
#else
    public:
        /// <summary>
        /// Given an unmanaged JET_SNPROG structure, return a managed MJET_SNPROG struct.
        /// </summary>
        MJET_SNPROG MakeManagedSNProg(const ::JET_SNPROG * _psnprog)
        {
            MJET_SNPROG snprog;

            if ( NULL == _psnprog || _psnprog->cbStruct != sizeof( ::JET_SNPROG ) )
            {
                throw TranslatedException(gcnew IsamInvalidParameterException());
            }
            snprog.Done = _psnprog->cunitDone;
            snprog.Total = _psnprog->cunitTotal;
            return snprog;
        }

        MJET_RECOVERY_CONTROL^ MakeManagedRecoveryControl(const ::JET_RECOVERYCONTROL * _prc )
        {
            MJET_RECOVERY_CONTROL^ recoveryControl = nullptr;

            if ( NULL == _prc || _prc->cbStruct != sizeof( ::JET_RECOVERYCONTROL ) )
            {
                throw TranslatedException(gcnew IsamInvalidParameterException());
            }

            switch(_prc->sntUnion)
            {
                case JET_sntOpenLog:
                {
                    MJET_SNRECOVERY^ snrecovery = gcnew MJET_SNRECOVERY();
                    recoveryControl = snrecovery;
                    snrecovery->GenNext = _prc->OpenLog.lGenNext;
                    snrecovery->LogFileName = MakeManagedStringW( _prc->OpenLog.wszLogFile );
                    snrecovery->IsCurrentLog = !!(_prc->OpenLog.fCurrentLog);
                    snrecovery->Reason = (MJET_OPENLOG_REASONS) _prc->OpenLog.eReason;
                    snrecovery->DatabaseInfo = MakeManagedDbinfoArray( _prc->OpenLog.rgdbinfomisc, _prc->OpenLog.cdbinfomisc );
                    break;
                }

                case JET_sntOpenCheckpoint:
                {
                    MJET_SNOPEN_CHECKPOINT^ snchkpt = gcnew MJET_SNOPEN_CHECKPOINT();
                    recoveryControl = snchkpt;
                    snchkpt->Checkpoint = MakeManagedStringW( _prc->OpenCheckpoint.wszCheckpoint );
                    break;
                }

                case JET_sntMissingLog:
                {
                    MJET_SNMISSING_LOG^ snmissLog = gcnew MJET_SNMISSING_LOG();
                    recoveryControl = snmissLog;
                    snmissLog->GenMissing = _prc->MissingLog.lGenMissing;
                    snmissLog->IsCurrentLog = !!(_prc->MissingLog.fCurrentLog);
                    snmissLog->NextAction = (MJET_RECOVERY_ACTIONS) _prc->MissingLog.eNextAction;
                    snmissLog->LogFileName = MakeManagedStringW( _prc->MissingLog.wszLogFile );
                    snmissLog->DatabaseInfo = MakeManagedDbinfoArray( _prc->MissingLog.rgdbinfomisc, _prc->MissingLog.cdbinfomisc );
                    break;
                }

                case JET_sntBeginUndo:
                {
                    MJET_SNBEGIN_UNDO^ snbeginUndo = gcnew MJET_SNBEGIN_UNDO();
                    recoveryControl = snbeginUndo;
                    snbeginUndo->DatabaseInfo = MakeManagedDbinfoArray( _prc->BeginUndo.rgdbinfomisc, _prc->BeginUndo.cdbinfomisc );
                    break;
                }

                case JET_sntNotificationEvent:
                {
                    MJET_SNNOTIFICATION_EVENT^ snEvent = gcnew MJET_SNNOTIFICATION_EVENT();
                    recoveryControl = snEvent;
                    snEvent->EventId = _prc->NotificationEvent.EventID;
                    break;
                }

                case JET_sntSignalErrorCondition:
                {
                    MJET_SNSIGNAL_ERROR^ snError = gcnew MJET_SNSIGNAL_ERROR();
                    recoveryControl = snError;
                    break;
                }

                case JET_sntAttachedDb:
                {
                    MJET_SNATTACHED_DB^ snAttached = gcnew MJET_SNATTACHED_DB();
                    recoveryControl = snAttached;
                    snAttached->DatabaseName = MakeManagedStringW( _prc->AttachedDb.wszDbPath );
                    break;
                }

                case JET_sntDetachingDb:
                {
                    MJET_SNDETACHING_DB^ snDetaching = gcnew MJET_SNDETACHING_DB();
                    recoveryControl = snDetaching;
                    snDetaching->DatabaseName = MakeManagedStringW( _prc->DetachingDb.wszDbPath );
                    break;
                }

                case JET_sntCommitCtx:
                {
                    MJET_SNCOMMIT_CTX^ snCommitCtx = gcnew MJET_SNCOMMIT_CTX();
                    recoveryControl = snCommitCtx;
                    snCommitCtx->Data = MakeManagedBytes( (BYTE *)_prc->CommitCtx.pbCommitCtx, _prc->CommitCtx.cbCommitCtx );
                    break;
                }

                default:
                    //Assert( _prc->sntUnion == -1 );
                    break;
            }

            recoveryControl->ErrDefault = ( _prc->errDefault == JET_errSuccess ? nullptr : EseExceptionHelper::JetErrToException( _prc->errDefault ) );
            recoveryControl->Instance = MakeManagedInst( _prc->instance );
            return recoveryControl;
        }

        MJET_SNPATCHREQUEST MakeManagedSNPatchRequest(const ::JET_SNPATCHREQUEST * _psnpatchrequest)
        {
            MJET_SNPATCHREQUEST snpatchrequest;

            if ( NULL == _psnpatchrequest || _psnpatchrequest->cbStruct != sizeof( ::JET_SNPATCHREQUEST ) )
            {
                throw TranslatedException(gcnew IsamInvalidParameterException());
            }

            snpatchrequest.PageNumber = _psnpatchrequest->pageNumber;
            snpatchrequest.LogFileName = Marshal::PtrToStringUni( (IntPtr) const_cast<wchar_t *>(_psnpatchrequest->szLogFile) );
            snpatchrequest.Instance = MakeManagedInst( _psnpatchrequest->instance );
            snpatchrequest.DatabaseInfo = MakeManagedDbinfo( _psnpatchrequest->dbinfomisc );
            snpatchrequest.Token = MakeManagedBytes( (BYTE *)_psnpatchrequest->pvToken, _psnpatchrequest->cbToken );
            snpatchrequest.Data = MakeManagedBytes( (BYTE *)_psnpatchrequest->pvData, _psnpatchrequest->cbData );
            snpatchrequest.DatabaseId = _psnpatchrequest->dbid;

            return snpatchrequest;
        }
    private:
#endif
#endif

        /// ================================================================
        static unsigned long CalculateHeaderChecksum(__in_bcount(cbHeader) const void * const pvHeader, _In_ const int cbHeader)
            /// ================================================================
            //
            /// Plain old unrolled-loop checksum that should work on any processor.
            /// Copied from checksum.cxx
            //
            //-
        {
            const unsigned long * pul = reinterpret_cast<const unsigned long *>(pvHeader);
            int cbT = cbHeader;

            // remove the first unsigned long, as it is the checksum itself

            unsigned long ulChecksum = 0x89abcdef ^ pul[0];
            do
            {
                ulChecksum ^= pul[0]
                ^ pul[1]
                ^ pul[2]
                ^ pul[3]
                ^ pul[4]
                ^ pul[5]
                ^ pul[6]
                ^ pul[7];
                cbT -= 32;
                pul += 8;
            } while ( cbT );

            return ulChecksum;
        }

        /// ================================================================
        static JET_ERR ErrFromWin32Err(_In_ const DWORD dwWinError, _In_ const JET_ERR errDefault)
            /// ================================================================
        {
            switch ( dwWinError )
            {
            case NO_ERROR:
                return JET_errSuccess;

            case ERROR_DISK_FULL:
                return JET_errDiskFull;

            case ERROR_HANDLE_EOF:
            case ERROR_VC_DISCONNECTED:
            case ERROR_IO_DEVICE:
            case ERROR_DEVICE_NOT_CONNECTED:
                return JET_errDiskIO;

            case ERROR_FILE_CORRUPT:
            case ERROR_DISK_CORRUPT:
                return JET_errFileSystemCorruption;

            case ERROR_NOT_READY:
            case ERROR_NO_MORE_FILES:
            case ERROR_FILE_NOT_FOUND:
                return JET_errFileNotFound;

            case ERROR_PATH_NOT_FOUND:
            case ERROR_DIRECTORY:
            case ERROR_BAD_NET_NAME:
            case ERROR_BAD_PATHNAME:
                return JET_errInvalidPath;

            case ERROR_ACCESS_DENIED:
            case ERROR_SHARING_VIOLATION:
            case ERROR_LOCK_VIOLATION:
            case ERROR_WRITE_PROTECT:
                return JET_errFileAccessDenied;

            case ERROR_TOO_MANY_OPEN_FILES:
                return JET_errOutOfFileHandles;
                break;

            case ERROR_NO_SYSTEM_RESOURCES:
            case ERROR_NOT_ENOUGH_MEMORY:
            case ERROR_WORKING_SET_QUOTA:
                return JET_errOutOfMemory;

            default:
                return errDefault;
            }
        }

        /// ================================================================
        static JET_ERR ErrReadFileHeader(
            _In_ const wchar_t * const szFile,
            __out_bcount_full(cbHeader) void * const pvHeader,
            const DWORD cbHeader,
            const __int64 ibOffset)
            /// ================================================================
        {
            JET_ERR err = JET_errInternalError;

            const HANDLE hFile = CreateFileW(
                szFile,
                GENERIC_READ,
                FILE_SHARE_READ,
                0,
                OPEN_EXISTING,
                0,
                0 );
            if( INVALID_HANDLE_VALUE == hFile )
            {
                err = ErrFromWin32Err(GetLastError(), JET_errDiskIO);
                goto HandleError;
            }

            LARGE_INTEGER offset;
            offset.QuadPart = ibOffset;
            if( !SetFilePointerEx( hFile, offset, NULL, FILE_BEGIN ) )
            {
                err = ErrFromWin32Err(GetLastError(), JET_errDiskIO);
                goto HandleError;
            }

            DWORD dwBytesRead;
            if( !ReadFile( hFile, pvHeader, cbHeader, &dwBytesRead, 0 ) )
            {
                err = ErrFromWin32Err(GetLastError(), JET_errDiskIO);
                goto HandleError;
            }

            if( cbHeader != dwBytesRead )
            {
                err = JET_errDiskIO;
                goto HandleError;
            }

            err = JET_errSuccess;

HandleError:
            (void)CloseHandle( hFile );
            return err;
        }

        /// ================================================================
        static JET_ERR ErrVerifyCheckpoint(__in_bcount(cbHeader) const void * const pvHeader, _In_ const int cbHeader)
            /// ================================================================
            //
            /// Make sure this data is actually a checkpoint header.
            //
            //-
        {
            JET_ERR err = JET_errInternalError;

            const unsigned long * const pulHeader = reinterpret_cast<const unsigned long *>(pvHeader);
            const unsigned long ulChecksumExpected = CalculateHeaderChecksum( pvHeader, cbHeader );
            const unsigned long ulChecksumActual = pulHeader[0];

            if( ulChecksumActual != ulChecksumExpected )
            {
                err = JET_errCheckpointCorrupt;
                goto HandleError;
            }

            err = JET_errSuccess;
HandleError:
            return err;
        }

        /// ================================================================
        static JET_ERR ErrGetCheckpointGeneration(_In_ const wchar_t * const szCheckpoint, _Out_ unsigned long * pulGeneration)
            /// ================================================================
        {
            JET_ERR err = JET_errInternalError;

            const int cbHeader = 4096;
            void * const pvHeader = VirtualAlloc( 0, cbHeader, MEM_COMMIT, PAGE_READWRITE ); // allocated with VirtualAlloc, free with VirtualFree
            if( 0 == pvHeader )
            {
                err = JET_errOutOfMemory;
                goto HandleError;
            }

            err = ErrReadFileHeader(
                szCheckpoint,
                pvHeader,
                cbHeader,
                0);
            if( err < JET_errSuccess )
            {
                goto HandleError;
            }

            err = ErrVerifyCheckpoint( pvHeader, cbHeader );
            if( err < JET_errSuccess )
            {
                goto HandleError;
            }

            const unsigned long * const pulHeader = reinterpret_cast<unsigned long *>(pvHeader);
            *pulGeneration = pulHeader[4];

            err = JET_errSuccess;

HandleError:
            if( pvHeader )
            {
                (void)VirtualFree( pvHeader, 0, MEM_RELEASE );
            }
            return err;
        }

        /// <summary>
        /// invoke error handler on unexpected error
        /// </summary>
        void Call(const JET_ERR errFn)
        {
            if (errFn < JET_errSuccess )
            {
                HandleError(errFn);
            }
        };

        /// <summary>
        /// invoke error handler on unexpected error
        /// </summary>
        MJET_WRN CallW(const JET_ERR errFn)
        {
            if ( errFn < JET_errSuccess )
            {
                HandleError(errFn);
            }

            MJET_WRN wrn = (MJET_WRN)errFn;
            return wrn;
        };

        /// <summary>
        /// return TRUE on success or warning,
        /// FALSE if specified error/warning code is encountered, or
        /// invoke error handler otherwise
        /// </summary>
        bool FCall(const JET_ERR errFn, const JET_ERR errX)
        {
            bool fResult;

            if (errFn == errX)
            {
                fResult = false;
            }
            else if (errFn >= JET_errSuccess)
            {
                fResult = true;
            }
            else
            {
                fResult = false;
                HandleError(errFn);
            }

            return fResult;
        };

        void HandleError(const JET_ERR errFn)
        {
            IsamErrorException^ ex = EseExceptionHelper::JetErrToException(errFn);
            throw TranslatedException(ex);
        }

        /// <summary>
        /// Translate the input exception to something useful to the consumer of this class.
        /// </summary>
        /// <param name="exception">The Jet exception detailing the problem in some Jet method.</param>
        /// <returns>
        /// The same exception (pass thru) if the consumer has not specified a translator or if this translator returns null.
        /// Otherwise, returns the translated exception.
        /// </returns>
        Exception^ TranslatedException(IsamErrorException^ exception)
        {
            Exception^ translatedException = this->m_exceptionTranslator(exception);
            return translatedException == nullptr ? exception : translatedException;
        }

        /// <summary>
        /// Pass thru of the input exception.
        /// </summary>
        /// <param name="exception">The Jet exception detailing the problem in some Jet method.</param>
        /// <returns>
        /// The same exception.
        /// </returns>
        static Exception^ DoNothingTranslator(IsamErrorException^ exception)
        {
            return exception;
        }

        /// <summary>
        /// A consumer-specified function to be used to translate a Jet exception into something
        /// useful to the consumer.
        /// </summary>
        ExceptionTranslator^ m_exceptionTranslator;
    };

    MSINTERNAL ref class Interop
    {
    private:
        /// <summary>
        /// The default instance calls directly into Jet.
        /// </summary>
        static IJetInterop^ defaultInstance;

        /// <summary>
        /// Typically, this variable points at the <seealso cref="defaultInstance"/> and so the class will call
        /// directly into Jet. However, test code can override this behavior to inject faults, etc.
        /// </summary>
        static IJetInterop^ activeInstance;

        /// <summary>
        /// The type initializer set the default behavior of the class to call straight into Jet.
        /// </summary>
        static Interop()
        {
            Interop::defaultInstance = gcnew JetInterop();
            Interop::activeInstance = Interop::defaultInstance;
        }

        /// <summary>
        /// Retrieve the currently active instance of the <seealso cref="IJetInterop"/> interface.
        /// Typically, this will be <seealso cref="JetInterop"/>, but tests may provide their
        /// own implementation to inject faults, etc.
        /// </summary>
        static property IJetInterop^ Instance
        {
            IJetInterop^ get()
            {
                return Interop::activeInstance;
            }
        }

        /// <summary>
        /// A convenience class to make it painless for test code to return <see cref="Interop"/> to the
        /// default behavior of calling directly into Jet.
        /// </summary>
        ref class ResetTestHook
        {
            IJetInterop^ oldInstance;
            Boolean disposed;

        public:
            ResetTestHook(IJetInterop^ instance)
                : oldInstance(instance)
            {
            }

            /// <summary>
            /// Unset the test hook when this <see cref="ResetTestHook"/> instance is disposed or cleaned up in the finalizer.
            /// </summary>
            ~ResetTestHook()
            {
                if (!this->disposed)
                {
                    this->disposed = true;
                    Interop::SetTestHook(this->oldInstance);
                }
            }
        };

    public:
        /// <summary>
        /// Enable Jet to throw exceptions that are meaningful to Jet's consumers.
        /// Thus, rather than catching the Jet exception, wrapping it and re-throwing, the consumer
        /// simply tells Jet what to throw.
        /// </summary>
        /// <param name="translator">The function to use to translate a Jet exception into an exception useful to the consumner.
        /// Pass null to reset to the default behavior of throwing the Jet exception.</param>
        static void SetExceptionTranslator(ExceptionTranslator^ translator)
        {
            Instance->SetExceptionTranslator(translator);
        }

        /// <summary>
        /// Set or unset a test hook to inject faults, record state, etc.
        /// </summary>
        /// <param name="testHook">The test hook. If null, will reset the class' behavior to call directly into Jet.</param>
        static IDisposable^ SetTestHook(IJetInterop^ testHook)
        {
            if (testHook == nullptr)
            {
                Interop::activeInstance = Interop::defaultInstance;
                return nullptr;
            }

            IDisposable^ testHookGuard = gcnew ResetTestHook(Interop::activeInstance);
            Interop::activeInstance = testHook;
            return testHookGuard;
        }

#if (ESENT_WINXP)
        /// <summary>
        /// Make a managed MJET_SESID from an unmanaged sesid and a managed instance
        /// </summary>
        static MJET_SESID MakeManagedSesid(::JET_SESID _sesid, MJET_INSTANCE instance)
        {
            return Instance->MakeManagedSesid(_sesid, instance);
        }

        /// <summary>
        /// Make a managed MJET_DBID from an unmanaged dbid
        /// </summary>
        static MJET_DBID MakeManagedDbid(::JET_DBID _dbid)
        {
            return Instance->MakeManagedDbid(_dbid);
        }

        /// <summary>
        /// Make a managed MJET_TABLEID from an unmanaged tableid and managed sesid and dbid
        /// </summary>
        static MJET_TABLEID MakeManagedTableid(::JET_TABLEID _tableid, MJET_SESID sesid, MJET_DBID dbid)
        {
            return Instance->MakeManagedTableid(_tableid, sesid, dbid);
        }

        /// <summary>
        /// Make a managed MJET_DBINFO from an unmanaged byte array containing JET_DBINFOMISC structure
        /// </summary>
        static MJET_DBINFO MakeManagedDbinfo(IntPtr _handle, unsigned int _cbconst)
        {
            return Instance->MakeManagedDbinfo(_handle, _cbconst);
        }

        /// <summary>
        /// Make a managed MJET_DBINFO from an unmanaged JET_DBINFOMISC structure
        /// </summary>
        static MJET_DBINFO MakeManagedDbinfo(const ::JET_DBINFOMISC& _dbinfomisc)
        {
            return Instance->MakeManagedDbinfo(_dbinfomisc);
        }
#else
#if (ESENT_PUBLICONLY)
        /// <summary>
        /// Make a managed MJET_SESID from an unmanaged sesid and a managed instance
        /// </summary>
        static MJET_SESID MakeManagedSesid(::JET_SESID _sesid, MJET_INSTANCE instance)
        {
            return Instance->MakeManagedSesid(_sesid, instance);
        }

        /// <summary>
        /// Make a managed MJET_DBID from an unmanaged dbid
        /// </summary>
        static MJET_DBID MakeManagedDbid(::JET_DBID _dbid)
        {
            return Instance->MakeManagedDbid(_dbid);
        }

        /// <summary>
        /// Make a managed MJET_TABLEID from an unmanaged tableid and managed sesid and dbid
        /// </summary>
        static MJET_TABLEID MakeManagedTableid(::JET_TABLEID _tableid, MJET_SESID sesid, MJET_DBID dbid)
        {
            return Instance->MakeManagedTableid(_tableid, sesid, dbid);
        }

        /// <summary>
        /// Make a managed MJET_DBINFO from an unmanaged byte array containing JET_DBINFOMISC4 structure
        /// </summary>
        static MJET_DBINFO MakeManagedDbinfo(IntPtr _handle, unsigned int _cbconst)
        {
            return Instance->MakeManagedDbinfo(_handle, _cbconst);
        }

        /// <summary>
        /// Make a managed MJET_DBINFO from an unmanaged JET_DBINFOMISC4 structure
        /// </summary>
        static MJET_DBINFO MakeManagedDbinfo(const ::JET_DBINFOMISC4& _dbinfomisc)
        {
            return Instance->MakeManagedDbinfo(_dbinfomisc);
        }
#else
        /// <summary>
        /// Make a managed MJET_SESID from an unmanaged sesid and a managed instance
        /// </summary>
        static MJET_SESID MakeManagedSesid(::JET_SESID _sesid, MJET_INSTANCE instance)
        {
            return Instance->MakeManagedSesid(_sesid, instance);
        }

        /// <summary>
        /// Make a managed MJET_DBID from an unmanaged dbid
        /// </summary>
        static MJET_DBID MakeManagedDbid(::JET_DBID _dbid)
        {
            return Instance->MakeManagedDbid(_dbid);
        }

        /// <summary>
        /// Make a managed MJET_TABLEID from an unmanaged tableid and managed sesid and dbid
        /// </summary>
        static MJET_TABLEID MakeManagedTableid(::JET_TABLEID _tableid, MJET_SESID sesid, MJET_DBID dbid)
        {
            return Instance->MakeManagedTableid(_tableid, sesid, dbid);
        }

        /// <summary>
        /// Make a managed MJET_DBINFO from an unmanaged byte array containing JET_DBINFOMISC7 structure
        /// </summary>
        static MJET_DBINFO MakeManagedDbinfo(IntPtr _handle, unsigned int _cbconst)
        {
            return Instance->MakeManagedDbinfo(_handle, _cbconst);
        }

        /// <summary>
        /// Make a managed MJET_DBINFO from an unmanaged JET_DBINFOMISC7 structure
        /// </summary>
        static MJET_DBINFO MakeManagedDbinfo(const ::JET_DBINFOMISC7& _dbinfomisc)
        {
            return Instance->MakeManagedDbinfo(_dbinfomisc);
        }

        /// <summary>
        /// Turn a managed MJET_DBINFO into an array of bytes. This array can be
        /// converted back into an MJET_DBINFO with MakeManagerDbinfo. Yes, this
        /// is home-made serialization.
        /// </summary>
        static array<Byte>^ GetBytes(MJET_DBINFO dbinfo)
        {
            return Instance->GetBytes(dbinfo);
        }

#endif
#endif

    public:
        literal System::Int32 MoveFirst     = 0x80000000;
        literal System::Int32 MovePrevious  = -1;
        literal System::Int32 MoveNext      = 1;
        literal System::Int32 MoveLast      = 0x7fffffff;

    public:

        /// <summary>
        /// Initialize Jet, returning a new instance or throwing an exception
        /// </summary>
        static MJET_INSTANCE MJetInit()
        {
            return Instance->MJetInit();
        }

        /// <summary>
        /// Initialize Jet, returning a new instance or throwing an exception
        /// </summary>
        static MJET_INSTANCE MJetInit(MJET_INSTANCE instance)
        {
            return Instance->MJetInit(instance);
        }

        /// <summary>
        /// Initialize Jet, returning a new instance or throwing an exception
        /// </summary>
        static MJET_INSTANCE MJetInit(MJET_INSTANCE instance, MJET_GRBIT grbit)
        {
            return Instance->MJetInit(instance, grbit);
        }

#if (ESENT_WINXP)
#else
#ifdef INTERNALUSE
        /// <summary>
        /// Initialize Jet, returning a new instance or throwing an exception.
        /// Databases to recover, a callback for recovery and
        /// the maximum generation to be replayed
        /// (only when performing recovery without undo) may be specified.
        /// </summary>
        static MJET_INSTANCE MJetInit(
            MJET_INSTANCE instance,
            array<MJET_RSTMAP>^ rstmaps,
            Int64 genStop,
            MJetInitCallback^ callback,
            MJET_GRBIT grbit)
        {
            return Instance->MJetInit(instance, rstmaps, genStop, callback, grbit);
        }
#endif

        /// <summary>
        /// Initialize Jet, returning a new instance or throwing an exception.
        /// Databases to recover and the maximum generation to be replayed
        /// (only when performing recovery without undo) may be specified.
        /// </summary>
        static MJET_INSTANCE MJetInit(
            MJET_INSTANCE instance,
            array<MJET_RSTMAP>^ rstmaps,
            Int64 genStop,
            MJET_GRBIT grbit)
        {
            return Instance->MJetInit(instance, rstmaps, genStop, grbit);
        }

        /// <summary>
        /// Initialize Jet, returning a new instance or throwing an exception.
        /// Databases to recover may be specified.
        /// </summary>
        static MJET_INSTANCE MJetInit(
            MJET_INSTANCE instance,
            array<MJET_RSTMAP>^ rstmaps,
            MJET_GRBIT grbit)
        {
            return Instance->MJetInit(instance, rstmaps, grbit);
        }
#endif

        /// <summary>
        /// Create a new instance, returning the instance or throwing an exception
        /// </summary>
        static MJET_INSTANCE MJetCreateInstance(String^ name)
        {
            return Instance->MJetCreateInstance(name);
        }

        /// <summary>
        /// Create a new instance, returning the instance or throwing an exception
        /// </summary>
        static MJET_INSTANCE MJetCreateInstance(
            String^ name,
            String^ displayName,
            MJET_GRBIT grbit)
        {
            return Instance->MJetCreateInstance(name, displayName, grbit);
        }

        /// <summary>
        /// </summary>
        static MJET_INSTANCE MJetCreateInstance(String^ name, String^ displayName)
        {
            return Instance->MJetCreateInstance(name, displayName);
        }

        /// <summary>
        /// Terminate an instance
        /// </summary>
        static void MJetTerm()
        {
            Instance->MJetTerm();
        }

        /// <summary>
        /// Terminate an instance
        /// </summary>
        static void MJetTerm(MJET_INSTANCE instance)
        {
            Instance->MJetTerm(instance);
        }

        /// <summary>
        /// Terminate an instance
        /// </summary>
        static void MJetTerm(MJET_INSTANCE instance, MJET_GRBIT grbit)
        {
            Instance->MJetTerm(instance, grbit);
        }

        /// <summary>
        /// </summary>
        static void MJetStopService()
        {
            Instance->MJetStopService();
        }

        /// <summary>
        /// </summary>
        static void MJetStopServiceInstance(MJET_INSTANCE instance)
        {
            Instance->MJetStopServiceInstance(instance);
        }

        /// <summary>
        /// </summary>
        static void MJetStopBackup()
        {
            Instance->MJetStopBackup();
        }

        /// <summary>
        /// </summary>
        static void MJetStopBackupInstance(MJET_INSTANCE instance)
        {
            Instance->MJetStopBackupInstance(instance);
        }

        /// <summary>
        /// </summary>
        static void MJetSetSystemParameter(
            MJET_INSTANCE instance,
            MJET_SESID sesid,
            MJET_PARAM paramid,
            Int64 param,
            String^ pstr)
        {
            Instance->MJetSetSystemParameter(instance, sesid, paramid, param, pstr);
        }

        /// <summary>
        /// </summary>
        static void MJetSetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            Int64 param)
        {
            Instance->MJetSetSystemParameter(instance, paramid, param);
        }

        /// <summary>
        /// </summary>
        static void MJetSetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            String^ pstr)
        {
            Instance->MJetSetSystemParameter(instance, paramid, pstr);
        }

        /// <summary>
        /// </summary>
        static void MJetSetSystemParameter(MJET_PARAM paramid, Int64 param)
        {
            Instance->MJetSetSystemParameter(paramid, param);
        }

        /// <summary>
        /// </summary>
        static void MJetSetSystemParameter(MJET_PARAM paramid, String^ pstr)
        {
            Instance->MJetSetSystemParameter(paramid, pstr);
        }

        /// <summary>
        /// </summary>
        static void MJetGetSystemParameter(
            MJET_INSTANCE instance,
            MJET_SESID sesid,
            MJET_PARAM paramid,
            Int64% param,
            String^% pstr)
        {
            Instance->MJetGetSystemParameter(instance, sesid, paramid, param, pstr);
        }

        /// <summary>
        /// </summary>
        static void MJetGetSystemParameter(
            MJET_INSTANCE instance,
            MJET_SESID sesid,
            MJET_PARAM paramid,
            Int64% param)
        {
            Instance->MJetGetSystemParameter(instance, sesid, paramid, param);
        }

        /// <summary>
        /// </summary>
        static void MJetGetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            Int64% param)
        {
            Instance->MJetGetSystemParameter(instance, paramid, param);
        }

        /// <summary>
        /// </summary>
        static void MJetGetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            String^% pstr)
        {
            Instance->MJetGetSystemParameter(instance, paramid, pstr);
        }

        /// <summary>
        /// </summary>
        static void MJetGetSystemParameter(MJET_PARAM paramid, Int64% param)
        {
            Instance->MJetGetSystemParameter(paramid, param);
        }

        /// <summary>
        /// </summary>
        static void MJetGetSystemParameter(MJET_PARAM paramid, String^% pstr)
        {
            Instance->MJetGetSystemParameter(paramid, pstr);
        }

#if (ESENT_PUBLICONLY)
#else
#if (ESENT_WINXP)
#else
        /// <summary>
        /// </summary>
        static void MJetGetResourceParam(
            MJET_INSTANCE instance,
            MJET_RESOPER resoper,
            MJET_RESID resid,
            Int64% param)
        {
            Instance->MJetGetResourceParam(instance, resoper, resid, param);
        }
#endif
#endif

        /*

        UNDONE

        JET_ERR JET_API MJetSetResourceParam(
        JET_INSTANCE    instance,
        JET_RESOPER     resoper,
        JET_RESID       resid,
        JET_API_PTR     ulParam );

        JET_ERR JET_API MJetEnableMultiInstance(    JET_SETSYSPARAM *   psetsysparam,
        unsigned long       csetsysparam,
        unsigned long *     pcsetsucceed);

        */

#if (ESENT_PUBLICONLY)
#else
        /// <summary>
        /// </summary>
        static void MJetResetCounter(MJET_SESID sesid, Int32 CounterType)
        {
            Instance->MJetResetCounter(sesid, CounterType);
        }

        /// <summary>
        /// </summary>
        static Int32 MJetGetCounter(MJET_SESID sesid, Int32 CounterType)
        {
            return Instance->MJetGetCounter(sesid, CounterType);
        }
#endif

#if (ESENT_PUBLICONLY)
#else
#if (ESENT_WINXP)
#else
        /// <summary>
        /// </summary>
        static void MJetTestHookEnableFaultInjection(
            Int32 id,
            JET_ERR err,
            Int32 ulProbability,
            MJET_GRBIT grbit)
        {
            Instance->MJetTestHookEnableFaultInjection(id, err, ulProbability, grbit);
        }

        /// <summary>
        /// </summary>
        static void MJetTestHookEnableConfigOverrideInjection(
            Int32 id,
            Int64 ulOverride,
            Int32 ulProbability,
            MJET_GRBIT grbit)
        {
            Instance->MJetTestHookEnableConfigOverrideInjection(id, ulOverride, ulProbability, grbit);
        }

        /// <summary>
        /// </summary>
        static IntPtr MJetTestHookEnforceContextFail(
            IntPtr callback)
        {
            return Instance->MJetTestHookEnforceContextFail(callback);
        }

        /// <summary>
        /// </summary>
        static void MJetTestHookEvictCachedPage(
            MJET_DBID dbid,
            Int32 pgno)
        {
            Instance->MJetTestHookEvictCachedPage(dbid, pgno);
        }

        /// <summary>
        /// </summary>
        static void MJetSetNegativeTesting(
            MJET_NEGATIVE_TESTING_BITS bits)
        {
            Instance->MJetSetNegativeTesting( bits );
        }

        /// <summary>
        /// </summary>
        static void MJetResetNegativeTesting(
            MJET_NEGATIVE_TESTING_BITS bits)
        {
            Instance->MJetResetNegativeTesting( bits );
        }

#endif
#endif

        /// <summary>
        /// </summary>
        static MJET_SESID MJetBeginSession(
            MJET_INSTANCE instance,
            String^ user,
            String^ password)
        {
            return Instance->MJetBeginSession(instance, user, password);
        }

        /// <summary>
        /// </summary>
        static MJET_SESID MJetBeginSession(MJET_INSTANCE instance)
        {
            return Instance->MJetBeginSession(instance);
        }

        /// <summary>
        /// </summary>
        static MJET_SESID MJetDupSession(MJET_SESID sesid)
        {
            return Instance->MJetDupSession(sesid);
        }

        /// <summary>
        /// </summary>
        static void MJetEndSession(MJET_SESID sesid, MJET_GRBIT grbit)
        {
            Instance->MJetEndSession(sesid, grbit);
        }

        /// <summary>
        /// </summary>
        static void MJetEndSession(MJET_SESID sesid)
        {
            Instance->MJetEndSession(sesid);
        }

        /// <summary>
        /// </summary>
        static Int64 MJetGetVersion(MJET_SESID sesid)
        {
            return Instance->MJetGetVersion(sesid);
        }

        /// <summary>
        /// </summary>
        static MJET_WRN MJetIdle(MJET_SESID sesid, MJET_GRBIT grbit)
        {
            return Instance->MJetIdle(sesid, grbit);
        }

        /// <summary>
        /// </summary>
        static MJET_DBID MJetCreateDatabase(
            MJET_SESID sesid,
            String^ file,
            String^ connect,
            MJET_GRBIT grbit,
            MJET_WRN% wrn)
        {
            return Instance->MJetCreateDatabase(sesid, file, connect, grbit, wrn);
        }

        /// <summary>
        /// </summary>
        static MJET_DBID MJetCreateDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit,
            MJET_WRN% wrn)
        {
            return Instance->MJetCreateDatabase(sesid, file, grbit, wrn);
        }

        /// <summary>
        /// </summary>
        static MJET_DBID MJetCreateDatabase(MJET_SESID sesid, String^ file)
        {
            return Instance->MJetCreateDatabase(sesid, file);
        }

        /// <summary>
        /// </summary>
        static MJET_DBID MJetCreateDatabase(
            MJET_SESID sesid,
            String^ file,
            Int64 maxPages,
            MJET_GRBIT grbit,
            MJET_WRN% wrn)
        {
            return Instance->MJetCreateDatabase(sesid, file, maxPages, grbit, wrn);
        }

        /// <summary>
        /// </summary>
        static MJET_WRN MJetAttachDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit)
        {
            return Instance->MJetAttachDatabase(sesid, file, grbit);
        }

        /// <summary>
        /// </summary>
        static MJET_WRN MJetAttachDatabase(MJET_SESID sesid, String^ file)
        {
            return Instance->MJetAttachDatabase(sesid, file);
        }

        /// <summary>
        /// </summary>
        static MJET_WRN MJetAttachDatabase(
            MJET_SESID sesid,
            String^ file,
            Int64 maxPages,
            MJET_GRBIT grbit)
        {
            return Instance->MJetAttachDatabase(sesid, file, maxPages, grbit);
        }

        /// <summary>
        /// </summary>
        static MJET_WRN MJetDetachDatabase(MJET_SESID sesid, String^ file)
        {
            return Instance->MJetDetachDatabase(sesid, file);
        }

        /// <summary>
        /// </summary>
        static MJET_WRN MJetDetachDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit)
        {
            return Instance->MJetDetachDatabase(sesid, file, grbit);
        }

        /// <summary>
        /// </summary>
        static MJET_OBJECTINFO MJetGetTableInfo(MJET_TABLEID tableid)
        {
            return Instance->MJetGetTableInfo(tableid);
        }

        /// <summary>
        /// </summary>
        static String^ MJetGetTableInfoName(MJET_TABLEID tableid)
        {
            return Instance->MJetGetTableInfoName(tableid);
        }

        /// <summary>
        /// </summary>
        static int MJetGetTableInfoInitialSize(MJET_TABLEID tableid)
        {
            return Instance->MJetGetTableInfoInitialSize(tableid);
        }

        /// <summary>
        /// </summary>
        static int MJetGetTableInfoDensity(MJET_TABLEID tableid)
        {
            return Instance->MJetGetTableInfoDensity(tableid);
        }

        /// <summary>
        /// </summary>
        static String^ MJetGetTableInfoTemplateTable(MJET_TABLEID tableid)
        {
            return Instance->MJetGetTableInfoTemplateTable(tableid);
        }

        /// <summary>
        /// </summary>
        static int MJetGetTableInfoOwnedPages(MJET_TABLEID tableid)
        {
            return Instance->MJetGetTableInfoOwnedPages(tableid);
        }

        /// <summary>
        /// </summary>
        static int MJetGetTableInfoAvailablePages(MJET_TABLEID tableid)
        {
            return Instance->MJetGetTableInfoAvailablePages(tableid);
        }

        /// <summary>
        /// </summary>
        static MJET_OBJECTINFO MJetGetObjectInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ object)
        {
            return Instance->MJetGetObjectInfo(sesid, dbid, object);
        }

        /// <summary>
        /// </summary>
        static MJET_OBJECTINFO MJetGetObjectInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_OBJTYP objtyp,
            String^ container,
            String^ object)
        {
            return Instance->MJetGetObjectInfo(sesid, dbid, objtyp, container, object);
        }

        /// <summary>
        /// </summary>
        static MJET_OBJECTLIST MJetGetObjectInfoList(MJET_SESID sesid, MJET_DBID dbid)
        {
            return Instance->MJetGetObjectInfoList(sesid, dbid);
        }

        /// <summary>
        /// </summary>
        static MJET_OBJECTLIST MJetGetObjectInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_OBJTYP objtyp)
        {
            return Instance->MJetGetObjectInfoList(sesid, dbid, objtyp);
        }

        /// <summary>
        /// </summary>
        static MJET_OBJECTLIST MJetGetObjectInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_OBJTYP objtyp,
            String^ container)
        {
            return Instance->MJetGetObjectInfoList(sesid, dbid, objtyp, container);
        }

        /// <summary>
        /// </summary>
        static MJET_OBJECTLIST MJetGetObjectInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_OBJTYP objtyp,
            String^ container,
            String^ object)
        {
            return Instance->MJetGetObjectInfoList(sesid, dbid, objtyp, container, object);
        }

#if (ESENT_PUBLICONLY)
#else
#if (ESENT_WINXP)
#else
        /// <summary>
        /// The JetGetInstanceInfo function retrieves information about the instances that are running.
        /// </summary>
        /// <remarks>
        /// If there are no active instances, MJetGetInstanceInfo will return an array with 0 elements.
        /// </remarks>
        static array<MJET_INSTANCE_INFO>^ MJetGetInstanceInfo()
        {
            return Instance->MJetGetInstanceInfo();
        }

        /// <summary>
        /// Given raw page data, return information about those pages
        /// </summary>
        /// <remarks>
        /// The unmanaged API takes multiple pages at one time. We simplify this API by returning
        /// information for only one page at a time.
        /// </remarks>
        static MJET_PAGEINFO MJetGetPageInfo(array<Byte>^ page, Int64 PageNumber)
        {
            return Instance->MJetGetPageInfo(page, PageNumber);
        }

        /// <summary>
        /// </summary>
        static MJET_LOGINFOMISC MJetGetLogFileInfo(String^ logfile)
        {
            return Instance->MJetGetLogFileInfo(logfile);
        }
#endif
#endif

        /// <summary>
        /// Given the name of a checkpoint file, return the generation of the checkpoint
        /// TODO: replace this with a call to an exported ESE function.
        /// </summary>
        static Int64 MJetGetCheckpointFileInfo(String^ checkpoint)
        {
            return Instance->MJetGetCheckpointFileInfo(checkpoint);
        }

#if (ESENT_WINXP)
        /// <summary>
        /// Given the name of a database file, return information about the database in a MJET_DBINFO struct.
        /// </summary>
        static MJET_DBINFO MJetGetDatabaseFileInfo(String^ database)
        {
            return Instance->MJetGetDatabaseFileInfo(database);
        }
#else
#if (ESENT_PUBLICONLY)
        /// <summary>
        /// Given the name of a database file, return information about the database in a MJET_DBINFO struct.
        /// </summary>
        static MJET_DBINFO MJetGetDatabaseFileInfo(String^ database)
        {
            return Instance->MJetGetDatabaseFileInfo(database);
        }
#else
        /// <summary>
        /// Given the name of a database file, return information about the database in a MJET_DBINFO struct.
        /// </summary>
        static MJET_DBINFO MJetGetDatabaseFileInfo(String^ database)
        {
            return Instance->MJetGetDatabaseFileInfo(database);
        }
#endif
#endif

#if (ESENT_PUBLICONLY)
#else
#if (ESENT_WINXP)
#else
        /// <summary>
        /// Given a database and a logfile, update the database header so that the logfile is no longer in the committed
        /// range of logfiles. This causes data loss. The logfile is not deleted.
        /// </summary>
        static void MJetRemoveLogfile(String^ database, String^ logfile)
        {
            Instance->MJetRemoveLogfile(database, logfile);
        }
#endif
#endif

#if (ESENT_PUBLICONLY)
#else
#if (ESENT_WINXP)
#else
        /// <summary>
        /// </summary>
        static array<Byte>^ MJetGetDatabasePages(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pgnoStart,
            Int64 cpg,
            MJET_GRBIT grbit)
        {
            return Instance->MJetGetDatabasePages(sesid, dbid, pgnoStart, cpg, grbit);
        }

        /// <summary>
        /// </summary>
        static void MJetOnlinePatchDatabasePage(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pgno,
            array<Byte>^ token,
            array<Byte>^ data,
            MJET_GRBIT grbit)
        {
            Instance->MJetOnlinePatchDatabasePage(sesid, dbid, pgno, token, data, grbit);
        }

        /// <summary>
        /// </summary>
        static void MJetBeginDatabaseIncrementalReseed(
            MJET_INSTANCE instance,
            String^ pstrDatabase,
            MJET_GRBIT grbit)
        {
            Instance->MJetBeginDatabaseIncrementalReseed(instance, pstrDatabase, grbit);
        }

        /// <summary>
        /// </summary>
        static void MJetEndDatabaseIncrementalReseed(
            MJET_INSTANCE instance,
            String^ pstrDatabase,
            Int64 genMinRequired,
            Int64 genFirstDivergedLog,
            Int64 genMaxRequired,
            MJET_GRBIT grbit)
        {
            Instance->MJetEndDatabaseIncrementalReseed(instance, pstrDatabase, genMinRequired, genFirstDivergedLog, genMaxRequired, grbit);
        }

        /// <summary>
        /// </summary>
        static void MJetPatchDatabasePages(
            MJET_INSTANCE instance,
            String^ pstrDatabase,
            Int64 pgnoStart,
            Int64 cpg,
            array<Byte>^ rgb,
            MJET_GRBIT grbit)
        {
            Instance->MJetPatchDatabasePages(instance, pstrDatabase, pgnoStart, cpg, rgb, grbit);
        }
#endif
#endif

        /// <summary>
        /// </summary>
        static MJET_TABLEID MJetCreateTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name,
            Int32 pages,
            Int32 density)
        {
            return Instance->MJetCreateTable(sesid, dbid, name, pages, density);
        }

        /// <summary>
        /// </summary>
        static MJET_TABLEID MJetCreateTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name)
        {
            return Instance->MJetCreateTable(sesid, dbid, name);
        }

#if (ESENT_WINXP)
        /// <summary>
        /// </summary>
        static void MJetCreateTableColumnIndex(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_TABLECREATE tablecreate)
        {
            Instance->MJetCreateTableColumnIndex(sesid, dbid, tablecreate);
        }
#else
        /// <summary>
        /// </summary>
        static void MJetCreateTableColumnIndex(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_TABLECREATE tablecreate)
        {
            Instance->MJetCreateTableColumnIndex(sesid, dbid, tablecreate);
        }
#endif

        /// <summary>
        /// </summary>
        static void MJetDeleteTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name)
        {
            Instance->MJetDeleteTable(sesid, dbid, name);
        }

        /// <summary>
        /// </summary>
        static void MJetRenameTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ oldName,
            String^ newName)
        {
            Instance->MJetRenameTable(sesid, dbid, oldName, newName);
        }

#if (ESENT_WINXP)
#else
        /// <summary>
        /// </summary>
        static MJET_COLUMNBASE MJetGetColumnInfo(MJET_TABLEID tableid, Int64 colid)
        {
            return Instance->MJetGetColumnInfo(tableid, colid);
        }
#endif

        /// <summary>
        /// </summary>
        static MJET_COLUMNBASE MJetGetColumnInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            Int64 colid)
        {
            return Instance->MJetGetColumnInfo(sesid, dbid, table, colid);
        }

        /// <summary>
        /// </summary>
        static MJET_COLUMNBASE MJetGetColumnInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            MJET_COLUMNID columnid)
        {
            return Instance->MJetGetColumnInfo(sesid, dbid, table, columnid);
        }

#if (ESENT_WINXP)
#else
        /// <summary>
        /// </summary>
        static MJET_COLUMNBASE MJetGetColumnInfo(MJET_TABLEID tableid, MJET_COLUMNID columnid)
        {
            return Instance->MJetGetColumnInfo(tableid, columnid);
        }
#endif

        /// <summary>
        /// </summary>
        static MJET_COLUMNBASE MJetGetColumnInfo(MJET_TABLEID tableid, String^ column)
        {
            return Instance->MJetGetColumnInfo(tableid, column);
        }

        /// <summary>
        /// </summary>
        static MJET_COLUMNBASE MJetGetColumnInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ column)
        {
            return Instance->MJetGetColumnInfo(sesid, dbid, table, column);
        }

        /// <summary>
        /// </summary>
        static MJET_COLUMNLIST MJetGetColumnInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            Boolean sortByColumnid)
        {
            return Instance->MJetGetColumnInfoList(sesid, dbid, table, sortByColumnid);
        }

        /// <summary>
        /// </summary>
        static MJET_COLUMNLIST MJetGetColumnInfoList(MJET_TABLEID tableid, Boolean sortByColumnid)
        {
            return Instance->MJetGetColumnInfoList(tableid, sortByColumnid);
        }

        /// <summary>
        /// </summary>
        static MJET_COLUMNLIST MJetGetColumnInfoList(MJET_TABLEID tableid)
        {
            return Instance->MJetGetColumnInfoList(tableid);
        }

        /// <summary>
        /// </summary>
        static MJET_COLUMNID MJetAddColumn(
            MJET_TABLEID tableid,
            String^ name,
            MJET_COLUMNDEF% definition,
            array<Byte>^ defaultValue)
        {
            return Instance->MJetAddColumn(tableid, name, definition, defaultValue);
        }

        /// <summary>
        /// </summary>
        static MJET_COLUMNID MJetAddColumn(
            MJET_TABLEID tableid,
            String^ name,
            MJET_COLUMNDEF% definition)
        {
            return Instance->MJetAddColumn(tableid, name, definition);
        }

        /// <summary>
        /// </summary>
        static void MJetDeleteColumn(MJET_TABLEID tableid, String^ name)
        {
            Instance->MJetDeleteColumn(tableid, name);
        }

        /// <summary>
        /// </summary>
        static void MJetDeleteColumn(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit)
        {
            Instance->MJetDeleteColumn(tableid, name, grbit);
        }

        /// <summary>
        /// </summary>
        static void MJetRenameColumn(
            MJET_TABLEID tableid,
            String^ oldName,
            String^ newName,
            MJET_GRBIT grbit)
        {
            Instance->MJetRenameColumn(tableid, oldName, newName, grbit);
        }

        /// <summary>
        /// </summary>
        static void MJetSetColumnDefaultValue(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ column,
            array<Byte>^ defaultValue,
            MJET_GRBIT grbit)
        {
            Instance->MJetSetColumnDefaultValue(sesid, dbid, table, column, defaultValue, grbit);
        }

        /// <summary>
        /// </summary>
        static MJET_INDEXLIST MJetGetIndexInfo(MJET_TABLEID tableid, String^ index)
        {
            return Instance->MJetGetIndexInfo(tableid, index);
        }

        /// <summary>
        /// </summary>
        static MJET_INDEXLIST MJetGetIndexInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index)
        {
            return Instance->MJetGetIndexInfo(sesid, dbid, table, index);
        }

        /// <summary>
        /// </summary>
        static MJET_INDEXLIST MJetGetIndexInfoList(MJET_TABLEID tableid)
        {
            return Instance->MJetGetIndexInfoList(tableid);
        }

        /// <summary>
        /// </summary>
        static MJET_INDEXLIST MJetGetIndexInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table)
        {
            return Instance->MJetGetIndexInfoList(sesid, dbid, table);
        }

        /// <summary>
        /// </summary>
        static int MJetGetIndexInfoDensity(MJET_TABLEID tableid, String^ index)
        {
            return Instance->MJetGetIndexInfoDensity(tableid, index);
        }

        /// <summary>
        /// </summary>
        static int MJetGetIndexInfoDensity(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index)
        {
            return Instance->MJetGetIndexInfoDensity(sesid, dbid, table, index);
        }

        /// <summary>
        /// </summary>
        static int MJetGetIndexInfoLCID(MJET_TABLEID tableid, String^ index)
        {
            return Instance->MJetGetIndexInfoLCID(tableid, index);
        }

        /// <summary>
        /// </summary>
        static int MJetGetIndexInfoLCID(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index)
        {
            return Instance->MJetGetIndexInfoLCID(sesid, dbid, table, index);
        }

#if (ESENT_WINXP)
        /// <summary>
        /// </summary>
        static int MJetGetIndexInfoKeyMost(MJET_TABLEID tableid, String^ index)
        {
            return Instance->MJetGetIndexInfoKeyMost(tableid, index);
        }

        /// <summary>
        /// </summary>
        static int MJetGetIndexInfoKeyMost(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index)
        {
            return Instance->MJetGetIndexInfoKeyMost(sesid, dbid, table, index);
        }
#else
        /// <summary>
        /// </summary>
        static int MJetGetIndexInfoKeyMost(MJET_TABLEID tableid, String^ index)
        {
            return Instance->MJetGetIndexInfoKeyMost(tableid, index);
        }

        /// <summary>
        /// </summary>
        static int MJetGetIndexInfoKeyMost(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index)
        {
            return Instance->MJetGetIndexInfoKeyMost(sesid, dbid, table, index);
        }
#endif

#if ESENT_WINXP
#else
        /// <summary>
        /// </summary>
        static MJET_INDEXCREATE MJetGetIndexInfoCreate(MJET_TABLEID tableid, String^ index)
        {
            return Instance->MJetGetIndexInfoCreate(tableid, index);
        }

        /// <summary>
        /// </summary>
        static MJET_INDEXCREATE MJetGetIndexInfoCreate(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index)
        {
            return Instance->MJetGetIndexInfoCreate(sesid, dbid, table, index);
        }
#endif

        /// <summary>
        /// </summary>
        static void MJetCreateIndex(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit,
            String^ key,
            Int32 density)
        {
            Instance->MJetCreateIndex(tableid, name, grbit, key, density);
        }

        /// <summary>
        /// </summary>
        static void MJetCreateIndex(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit,
            String^ key)
        {
            Instance->MJetCreateIndex(tableid, name, grbit, key);
        }

        /// <summary>
        /// </summary>
        static void MJetCreateIndex(
            MJET_TABLEID tableid,
            String^ name,
            String^ key)
        {
            Instance->MJetCreateIndex(tableid, name, key);
        }

#if (ESENT_WINXP)
        static void MJetCreateIndex(MJET_TABLEID tableid, array<MJET_INDEXCREATE>^ indexcreates)
        {
            Instance->MJetCreateIndex(tableid, indexcreates);
        }
#else
        static void MJetCreateIndex(MJET_TABLEID tableid, array<MJET_INDEXCREATE>^ indexcreates)
        {
            Instance->MJetCreateIndex(tableid, indexcreates);
        }
#endif

        /// <summary>
        /// </summary>
        static void MJetDeleteIndex(MJET_TABLEID tableid, String^ name)
        {
            Instance->MJetDeleteIndex(tableid, name);
        }

        /// <summary>
        /// </summary>
        static void MJetBeginTransaction(MJET_SESID sesid)
        {
            Instance->MJetBeginTransaction(sesid);
        }

        /// <summary>
        /// </summary>
        static void MJetBeginTransaction(MJET_SESID sesid, MJET_GRBIT grbit)
        {
            Instance->MJetBeginTransaction(sesid, grbit);
        }

        /// <summary>
        /// </summary>
        static void MJetBeginTransaction(
            MJET_SESID sesid,
            Int32 trxid,
            MJET_GRBIT grbit)
        {
            Instance->MJetBeginTransaction(sesid, trxid, grbit);
        }

        /// <summary>
        /// </summary>
        static void MJetCommitTransaction(MJET_SESID sesid, MJET_GRBIT grbit)
        {
            Instance->MJetCommitTransaction(sesid, grbit);
        }

        /// <summary>
        /// </summary>
        static void MJetCommitTransaction(MJET_SESID sesid)
        {
            Instance->MJetCommitTransaction(sesid);
        }

        /// <summary>
        /// </summary>
        static void MJetRollback(MJET_SESID sesid, MJET_GRBIT grbit)
        {
            Instance->MJetRollback(sesid, grbit);
        }

        /// <summary>
        /// </summary>
        static void MJetRollback(MJET_SESID sesid)
        {
            Instance->MJetRollback(sesid);
        }

        /// <summary>
        /// </summary>
        static void MJetGetDatabaseInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            array<Byte>^ resultBuffer,
            MJET_DBINFO_LEVEL InfoLevel)
        {
            Instance->MJetGetDatabaseInfo(sesid, dbid, resultBuffer, InfoLevel);
        }

#if (ESENT_WINXP)
        /// <summary>
        /// </summary>
        static MJET_DBINFO MJetGetDatabaseInfo(
            MJET_SESID sesid,
            MJET_DBID dbid)
        {
            return Instance->MJetGetDatabaseInfo(sesid, dbid);
        }
#else
#if (ESENT_PUBLICONLY)
        /// <summary>
        /// </summary>
        static MJET_DBINFO MJetGetDatabaseInfo(
            MJET_SESID sesid,
            MJET_DBID dbid)
        {
            return Instance->MJetGetDatabaseInfo(sesid, dbid);
        }
#else
        /// <summary>
        /// </summary>
        static MJET_DBINFO MJetGetDatabaseInfo(
            MJET_SESID sesid,
            MJET_DBID dbid)
        {
            return Instance->MJetGetDatabaseInfo(sesid, dbid);
        }
#endif
#endif

#if (ESENT_WINXP)
#else
        /// <summary>
        /// </summary>
        static String^ MJetGetDatabaseInfoFilename(
            MJET_SESID sesid,
            MJET_DBID dbid)
        {
            return Instance->MJetGetDatabaseInfoFilename(sesid, dbid);
        }

        /// <summary>
        /// </summary>
        static Int64 MJetGetDatabaseInfoFilesize(
            MJET_SESID sesid,
            MJET_DBID dbid)
        {
            return Instance->MJetGetDatabaseInfoFilesize(sesid, dbid);
        }
#endif

        /// <summary>
        /// </summary>
        static MJET_DBID MJetOpenDatabase(
            MJET_SESID sesid,
            String^ file,
            String^ connect,
            MJET_GRBIT grbit,
            MJET_WRN% wrn)
        {
            return Instance->MJetOpenDatabase(sesid, file, connect, grbit, wrn);
        }

        /// <summary>
        /// </summary>
        static MJET_DBID MJetOpenDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit,
            MJET_WRN% wrn)
        {
            return Instance->MJetOpenDatabase(sesid, file, grbit, wrn);
        }

        /// <summary>
        /// </summary>
        static MJET_DBID MJetOpenDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit)
        {
            return Instance->MJetOpenDatabase(sesid, file, grbit);
        }

        /// <summary>
        /// </summary>
        static MJET_DBID MJetOpenDatabase(MJET_SESID sesid, String^ file)
        {
            return Instance->MJetOpenDatabase(sesid, file);
        }

        /// <summary>
        /// </summary>
        static void MJetCloseDatabase(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_GRBIT grbit)
        {
            Instance->MJetCloseDatabase(sesid, dbid, grbit);
        }

        /// <summary>
        /// </summary>
        static void MJetCloseDatabase(MJET_SESID sesid, MJET_DBID dbid)
        {
            Instance->MJetCloseDatabase(sesid, dbid);
        }

        /// <summary>
        /// </summary>
        static MJET_TABLEID MJetOpenTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name,
            array<Byte>^ parameters,
            MJET_GRBIT grbit)
        {
            return Instance->MJetOpenTable(sesid, dbid, name, parameters, grbit);
        }

        /// <summary>
        /// </summary>
        static MJET_TABLEID MJetOpenTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name,
            MJET_GRBIT grbit)
        {
            return Instance->MJetOpenTable(sesid, dbid, name, grbit);
        }

        /// <summary>
        /// </summary>
        static MJET_TABLEID MJetOpenTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name)
        {
            return Instance->MJetOpenTable(sesid, dbid, name);
        }

        /// <summary>
        /// Convenience function that doesn't throw an exception if it doesn't find the table, but
        /// just returns false
        /// </summary>
        static Boolean TryOpenTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name,
            [System::Runtime::InteropServices::OutAttribute]
            MJET_TABLEID% tableid)
        {
            return Instance->TryOpenTable(sesid, dbid, name, tableid);
        }

        /// <summary>
        /// </summary>
        static void MJetSetTableSequential(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            Instance->MJetSetTableSequential(tableid, grbit);
        }

        /// <summary>
        /// </summary>
        static void MJetResetTableSequential(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            Instance->MJetResetTableSequential(tableid, grbit);
        }

        /// <summary>
        /// </summary>
        static void MJetCloseTable(MJET_TABLEID tableid)
        {
            Instance->MJetCloseTable(tableid);
        }

        /// <summary>
        /// </summary>
        static void MJetDelete(MJET_TABLEID tableid)
        {
            Instance->MJetDelete(tableid);
        }

        /// <summary>
        /// </summary>
        static array<Byte>^ MJetUpdate(MJET_TABLEID tableid)
        {
            return Instance->MJetUpdate(tableid);
        }

#if (ESENT_WINXP)
#else
        /// <summary>
        /// </summary>
        static array<Byte>^ MJetUpdate(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            return Instance->MJetUpdate(tableid, grbit);
        }
#endif

        /*

        UNDONE

        JET_ERR JET_API MJetEscrowUpdate(
        JET_SESID       sesid,
        JET_TABLEID     tableid,
        JET_COLUMNID    columnid,
        void            *pv,
        unsigned long   cbMax,
        void            *pvOld,
        unsigned long   cbOldMax,
        unsigned long   *pcbOldActual,
        JET_GRBIT       grbit );

        */

        /// <summary>
        /// Returns NULL if the column is null, otherwise returns the number of bytes read into the buffer
        /// </summary>
        static Nullable<Int64> MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit,
            MJET_RETINFO retinfo,
            ArraySegment<Byte> buffer)
        {
            return Instance->MJetRetrieveColumn(tableid, columnid, grbit, retinfo, buffer);
        }

        /// <summary>
        /// Returns NULL if the column is null, returns a zero-length array for a zero-length column
        /// </summary>
        static array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit,
            MJET_RETINFO retinfo,
            Int64 maxdata)
        {
            return Instance->MJetRetrieveColumn(tableid, columnid, grbit, retinfo, maxdata);
        }

        /// <summary>
        /// </summary>
        static array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit,
            MJET_RETINFO retinfo)
        {
            return Instance->MJetRetrieveColumn(tableid, columnid, grbit, retinfo);
        }

        /// <summary>
        /// </summary>
        static array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit)
        {
            return Instance->MJetRetrieveColumn(tableid, columnid, grbit);
        }

        /// <summary>
        /// Returns NULL if the column is null, returns a zere-length array for a zero-length column
        /// </summary>
        static array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit,
            Int64 maxdata)
        {
            return Instance->MJetRetrieveColumn(tableid, columnid, grbit, maxdata);
        }

        /// <summary>
        /// </summary>
        static array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            Int64 maxdata)
        {
            return Instance->MJetRetrieveColumn(tableid, columnid, maxdata);
        }

        /// <summary>
        /// </summary>
        static array<Byte>^ MJetRetrieveColumn(MJET_TABLEID tableid, MJET_COLUMNID columnid)
        {
            return Instance->MJetRetrieveColumn(tableid, columnid);
        }

        /// <summary>
        /// </summary>
        static Int64 MJetRetrieveColumnSize(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            Int64 itagSequence,
            MJET_GRBIT grbit)
        {
            return Instance->MJetRetrieveColumnSize(tableid, columnid, itagSequence, grbit);
        }

        /// <summary>
        /// </summary>
        static Int64 MJetRetrieveColumnSize(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            Int64 itagSequence)
        {
            return Instance->MJetRetrieveColumnSize(tableid, columnid, itagSequence);
        }

        /// <summary>
        /// </summary>
        static Int64 MJetRetrieveColumnSize(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit)
        {
            return Instance->MJetRetrieveColumnSize(tableid, columnid, grbit);
        }

        /// <summary>
        /// </summary>
        static Int64 MJetRetrieveColumnSize(MJET_TABLEID tableid, MJET_COLUMNID columnid)
        {
            return Instance->MJetRetrieveColumnSize(tableid, columnid);
        }

        /*

        UNDONE

        JET_ERR JET_API MJetRetrieveColumns(
        JET_SESID       sesid,
        JET_TABLEID     tableid,
        JET_RETRIEVECOLUMN  *pretrievecolumn,
        unsigned long   cretrievecolumn );

        JET_ERR JET_API MJetRetrieveTaggedColumnList(
        JET_SESID       sesid,
        JET_TABLEID     tableid,
        unsigned long   *pcColumns,
        void            *pvData,
        unsigned long   cbData,
        JET_COLUMNID    columnidStart,
        JET_GRBIT       grbit );

        */

#if (ESENT_WINXP)
        static String^ MJetGetColumnName(MJET_TABLEID tableid, MJET_COLUMNID columnid)
        {
            return Instance->MJetGetColumnName(tableid, columnid);
        }

        static String^ MJetGetColumnName(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ tablename,
            MJET_COLUMNID columnid)
        {
            return Instance->MJetGetColumnName(sesid, dbid, tablename, columnid);
        }

        static array<Byte>^ BytesFromObject(
            MJET_COLTYP type,
            Boolean isASCII,
            Object^ o)
        {
            return Instance->BytesFromObject(type, isASCII, o);
        }
#else
#endif

        /// <summary>
        /// </summary>
        static array<MJET_ENUMCOLUMN>^ MJetEnumerateColumns(
            MJET_TABLEID tableid,
            array<MJET_ENUMCOLUMNID>^ columnids,
            MJET_GRBIT grbit,
            MJET_WRN& wrn)
        {
            return Instance->MJetEnumerateColumns(tableid, columnids, grbit, wrn);
        }

        /// <summary>
        /// </summary>
        static array<MJET_ENUMCOLUMN>^ MJetEnumerateColumns(
            MJET_TABLEID tableid,
            array<MJET_ENUMCOLUMNID>^ columnids,
            MJET_GRBIT grbit)
        {
            return Instance->MJetEnumerateColumns(tableid, columnids, grbit);
        }

        /// <summary>
        /// </summary>
        static array<MJET_ENUMCOLUMN>^ MJetEnumerateColumns(MJET_TABLEID tableid, array<MJET_ENUMCOLUMNID>^ columnids)
        {
            return Instance->MJetEnumerateColumns(tableid, columnids);
        }

        /// <summary>
        /// </summary>
        static array<MJET_ENUMCOLUMN>^ MJetEnumerateColumns(MJET_TABLEID tableid)
        {
            return Instance->MJetEnumerateColumns(tableid);
        }

#if (ESENT_WINXP)
#else
        static MJET_RECSIZE MJetGetRecordSize(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            return Instance->MJetGetRecordSize(tableid, grbit);
        }

        static MJET_RECSIZE MJetGetRecordSize(MJET_TABLEID tableid)
        {
            return Instance->MJetGetRecordSize(tableid);
        }
#endif

        /// <summary>
        /// </summary>
        static MJET_WRN MJetSetColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            array<Byte>^ data,
            MJET_GRBIT grbit,
            MJET_SETINFO setinfo)
        {
            return Instance->MJetSetColumn(tableid, columnid, data, grbit, setinfo);
        }

        /// <summary>
        /// </summary>
        static MJET_WRN MJetSetColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            array<Byte>^ data,
            MJET_GRBIT grbit)
        {
            return Instance->MJetSetColumn(tableid, columnid, data, grbit);
        }

        /// <summary>
        /// </summary>
        static MJET_WRN MJetSetColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            array<Byte>^ data)
        {
            return Instance->MJetSetColumn(tableid, columnid, data);
        }

        /*

        UNDONE

        JET_ERR JET_API MJetSetColumns(
        JET_SESID       sesid,
        JET_TABLEID     tableid,
        JET_SETCOLUMN   *psetcolumn,
        unsigned long   csetcolumn );

        */

        /// <summary>
        /// </summary>
        static void MJetPrepareUpdate(MJET_TABLEID tableid, MJET_PREP prep)
        {
            Instance->MJetPrepareUpdate(tableid, prep);
        }

        /// <summary>
        /// </summary>
        static MJET_RECPOS MJetGetRecordPosition(MJET_TABLEID tableid)
        {
            return Instance->MJetGetRecordPosition(tableid);
        }

        /// <summary>
        /// </summary>
        static void MJetGotoPosition(MJET_TABLEID tableid, MJET_RECPOS recpos)
        {
            Instance->MJetGotoPosition(tableid, recpos);
        }

        /*

        UNDONE

        JET_ERR JET_API MJetGetCursorInfo(
        JET_SESID       sesid,
        JET_TABLEID     tableid,
        void            *pvResult,
        unsigned long   cbMax,
        unsigned long   InfoLevel );

        */

        /// <summary>
        /// </summary>
        static MJET_TABLEID MJetDupCursor(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            return Instance->MJetDupCursor(tableid, grbit);
        }

        /// <summary>
        /// </summary>
        static MJET_TABLEID MJetDupCursor(MJET_TABLEID tableid)
        {
            return Instance->MJetDupCursor(tableid);
        }

        /// <summary>
        /// </summary>
        static String^ MJetGetCurrentIndex(MJET_TABLEID tableid)
        {
            return Instance->MJetGetCurrentIndex(tableid);
        }

        /// <summary>
        /// </summary>
        static void MJetSetCurrentIndex(MJET_TABLEID tableid, String^ name)
        {
            Instance->MJetSetCurrentIndex(tableid, name);
        }

        /// <summary>
        /// </summary>
        static void MJetSetCurrentIndex(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit)
        {
            Instance->MJetSetCurrentIndex(tableid, name, grbit);
        }

        /// <summary>
        /// </summary>
        static void MJetSetCurrentIndex(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit,
            Int64 itag)
        {
            Instance->MJetSetCurrentIndex(tableid, name, grbit, itag);
        }

        /*

        UNDONE

        JET_ERR JET_API MJetSetCurrentIndex4(
        JET_SESID       sesid,
        JET_TABLEID     tableid,
        const _TCHAR        *szIndexName,
        JET_INDEXID     *pindexid,
        JET_GRBIT       grbit,
        unsigned long   itagSequence );

        */

        /// <summary>
        /// </summary>
        static MJET_WRN MJetMove(
            MJET_TABLEID tableid,
            Int32 rows,
            MJET_GRBIT grbit)
        {
            return Instance->MJetMove(tableid, rows, grbit);
        }

        /// <summary>
        /// </summary>
        static MJET_WRN MJetMove(MJET_TABLEID tableid, Int32 rows)
        {
            return Instance->MJetMove(tableid, rows);
        }

        /// <summary>
        /// </summary>
        static MJET_WRN MJetMove(
            MJET_TABLEID tableid,
            MJET_MOVE rows,
            MJET_GRBIT grbit)
        {
            return Instance->MJetMove(tableid, rows, grbit);
        }

        /// <summary>
        /// </summary>
        static MJET_WRN MJetMove(MJET_TABLEID tableid, MJET_MOVE rows)
        {
            return Instance->MJetMove(tableid, rows);
        }

        /// <summary>
        /// Convenience function that doesn't throw an exception
        /// </summary>
        static void MoveBeforeFirst(MJET_TABLEID tableid)
        {
            Instance->MoveBeforeFirst(tableid);
        }

        /// <summary>
        /// Convenience function that doesn't throw an exception
        /// </summary>
        static void MoveAfterLast(MJET_TABLEID tableid)
        {
            Instance->MoveAfterLast(tableid);
        }

        /// <summary>
        /// Convenience function that doesn't throw an exception if runs out of records,
        /// just returns false
        /// </summary>
        static Boolean TryMoveFirst(MJET_TABLEID tableid)
        {
            return Instance->TryMoveFirst(tableid);
        }

        /// <summary>
        /// Convenience function that doesn't throw an exception if runs out of records,
        /// just returns false
        /// </summary>
        static Boolean TryMoveNext(MJET_TABLEID tableid)
        {
            return Instance->TryMoveNext(tableid);
        }

        /// <summary>
        /// Convenience function that doesn't throw an exception for out of records,
        /// just returns false
        /// </summary>
        static Boolean TryMoveLast(MJET_TABLEID tableid)
        {
            return Instance->TryMoveLast(tableid);
        }

        /// <summary>
        /// Convenience function that doesn't throw an exception if runs out of records,
        /// just returns false
        /// </summary>
        static Boolean TryMovePrevious(MJET_TABLEID tableid)
        {
            return Instance->TryMovePrevious(tableid);
        }

        /// <summary>
        /// Convenience function that doesn't throw an exception if runs out of records,
        /// just returns false
        /// </summary>
        static Boolean TryMove(MJET_TABLEID tableid, MJET_MOVE rows)
        {
            return Instance->TryMove(tableid, rows);
        }

        /// <summary>
        /// Convenience function that doesn't throw an exception if runs out of records,
        /// just returns false
        /// </summary>
        static Boolean TryMove(MJET_TABLEID tableid, Int32 rows)
        {
            return Instance->TryMove(tableid, rows);
        }

        /// <summary>
        /// </summary>
        static void MJetGetLock(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            Instance->MJetGetLock(tableid, grbit);
        }

        /// <summary>
        /// </summary>
        static MJET_WRN MJetMakeKey(
            MJET_TABLEID tableid,
            array<Byte>^ data,
            MJET_GRBIT grbit)
        {
            return Instance->MJetMakeKey(tableid, data, grbit);
        }

#if (ESENT_WINXP)
#else
        /// <summary>
        /// Prereads the keys, returns the number of keys preread
        /// </summary>
        static Int32 MJetPrereadKeys(
            MJET_TABLEID tableid,
            array<array<Byte>^>^ keys,
            MJET_GRBIT grbit)
        {
            return Instance->MJetPrereadKeys(tableid, keys, grbit);
        }
#endif

        /// <summary>
        /// </summary>
        static MJET_WRN MJetSeek(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            return Instance->MJetSeek(tableid, grbit);
        }

        /// <summary>
        /// Convenience function that doesn't throw an exception if it doesn't find a record, but
        /// just returns false
        /// </summary>
        static Boolean TrySeek(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            return Instance->TrySeek(tableid, grbit);
        }

        /// <summary>
        /// Convenience function that doesn't throw an exception if it doesn't find a record, but
        /// just returns false
        /// </summary>
        static Boolean TrySeek(MJET_TABLEID tableid)
        {
            return Instance->TrySeek(tableid);
        }

        /// <summary>
        /// </summary>
        static array<Byte>^ MJetGetBookmark(MJET_TABLEID tableid)
        {
            return Instance->MJetGetBookmark(tableid);
        }

        /// <summary>
        /// </summary>
        static MJET_SECONDARYINDEXBOOKMARK MJetGetSecondaryIndexBookmark(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            return Instance->MJetGetSecondaryIndexBookmark(tableid, grbit);
        }

        /// <summary>
        /// </summary>
        static MJET_SECONDARYINDEXBOOKMARK MJetGetSecondaryIndexBookmark(MJET_TABLEID tableid)
        {
            return Instance->MJetGetSecondaryIndexBookmark(tableid);
        }

        /*

        UNDONE

        JET_ERR JET_API MJetGetSecondaryIndexBookmark(
        JET_SESID       sesid,
        JET_TABLEID     tableid,
        void *          pvSecondaryKey,
        unsigned long   cbSecondaryKeyMax,
        unsigned long * pcbSecondaryKeyActual,
        void *          pvPrimaryBookmark,
        unsigned long   cbPrimaryBookmarkMax,
        unsigned long * pcbPrimaryKeyActual,
        const JET_GRBIT grbit );

        JET_ERR JET_API MJetCompact(
        JET_SESID       sesid,
        const _TCHAR        *szDatabaseSrc,
        const _TCHAR        *szDatabaseDest,
        JET_PFNSTATUS   pfnStatus,
        JET_CONVERT     *pconvert,
        JET_GRBIT       grbit );

        */

        /// <summary>
        /// </summary>
        static MJET_WRN MJetDefragment(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            Int32% passes,
            Int32% seconds,
            MJET_GRBIT grbit)
        {
            return Instance->MJetDefragment(sesid, dbid, table, passes, seconds, grbit);
        }

        /// <summary>
        /// Jet Online Defragmentation API that takes a callback
        /// </summary>
        static MJET_WRN MJetDefragment2(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            Int32% passes,
            Int32% seconds,
            MJetOnEndOnlineDefragCallbackDelegate^ callback,
            MJET_GRBIT grbit)
        {
            return Instance->MJetDefragment2(sesid, dbid, table, passes, seconds, callback, grbit);
        }

        /*

        UNDONE

        JET_ERR JET_API MJetDefragment3(
        JET_SESID       vsesid,
        const _TCHAR        *szDatabaseName,
        const _TCHAR        *szTableName,
        unsigned long   *pcPasses,
        unsigned long   *pcSeconds,
        JET_CALLBACK    callback,
        void            *pvContext,
        JET_GRBIT       grbit );
        */

#if (ESENT_PUBLICONLY)
#else
        static void MJetDatabaseScan(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int32 msecSleep,
            MJET_GRBIT grbit)
        {
            Instance->MJetDatabaseScan(sesid, dbid, msecSleep, grbit);
        }
#endif

#if (ESENT_PUBLICONLY)
#else
        static void MJetConvertDDL(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_DDLCALLBACKDLL ddlcallbackdll)
        {
            Instance->MJetConvertDDL(sesid, dbid, ddlcallbackdll);
        }

        /// <summary>
        /// </summary>
        static void MJetUpgradeDatabase(
            MJET_SESID sesid,
            String^ database,
            String^ streamingFile,
            MJET_GRBIT grbit)
        {
            Instance->MJetUpgradeDatabase(sesid, database, streamingFile, grbit);
        }
#endif

#if (ESENT_PUBLICONLY)
#else
#if (ESENT_WINXP)
#else
        /// <summary>
        /// </summary>
        static void MJetSetMaxDatabaseSize(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pages,
            MJET_GRBIT grbit)
        {
            Instance->MJetSetMaxDatabaseSize(sesid, dbid, pages, grbit);
        }

        /// <summary>
        /// </summary>
        static Int64 MJetGetMaxDatabaseSize(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_GRBIT grbit)
        {
            return Instance->MJetGetMaxDatabaseSize(sesid, dbid, grbit);
        }
#endif
#endif

        /// <summary>
        /// </summary>
        static Int64 MJetSetDatabaseSize(
            MJET_SESID sesid,
            String^ database,
            Int64 pages)
        {
            return Instance->MJetSetDatabaseSize(sesid, database, pages);
        }

        /// <summary>
        /// </summary>
        static Int64 MJetGrowDatabase(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pages)
        {
            return Instance->MJetGrowDatabase(sesid, dbid, pages);
        }

        /// <summary>
        /// </summary>
        static MJET_WRN MJetResizeDatabase(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pages,
            [Out] Int64% actualPages,
            MJET_GRBIT grbit)
        {
            return Instance->MJetResizeDatabase(sesid, dbid, pages, actualPages, grbit);
        }

        /// <summary>
        /// Sets the session to be associated with a thread and debugging context.
        //
        /// When used before a transaction is begun, will allow the session's context to be reset
        /// to make the session mobile / usable across threads even while an transaction is open.
        //
        /// When used after a transaction is already open, will associate this thread and debugging
        /// context with this session, so the session + transaction can be used on the thread.
        //
        /// Note: context is truncated from an Int64 to an Int32 on 32-bit architectures, but since
        /// this is for debugging only, this is fine.
        /// </summary>
        static void MJetSetSessionContext(MJET_SESID sesid, IntPtr context)
        {
            Instance->MJetSetSessionContext(sesid, context);
        }

        /// <summary>
        /// Resets the session to be un-associated with a thread and debugging context.
        //
        /// Can only be used when a session already had a context set before a transaction was
        /// begun.  This allows the session + open transaction to have a different thread and
        /// session associated with it via JetSetSessionContext.
        /// </summary>
        static void MJetResetSessionContext(MJET_SESID sesid)
        {
            Instance->MJetResetSessionContext(sesid);
        }

        /// <summary>
        /// </summary>
        static void MJetTracing(MJET_TRACEOP traceOp, MJET_TRACETAG traceTag, IntPtr ul)
        {
            Instance->MJetTracing(traceOp, traceTag, ul);
        }

        /// <summary>
        /// </summary>
        static void MJetTracing(MJET_TRACEOP traceOp, MJET_TRACETAG traceTag, MJET_DBID dbid)
        {
            Instance->MJetTracing(traceOp, traceTag, dbid);
        }

#if (ESENT_PUBLICONLY)
#else
        /// <summary>
        /// Get information about the session.
        /// </summary>
        static MJET_SESSIONINFO MJetGetSessionInfo(MJET_SESID sesid)
        {
            return Instance->MJetGetSessionInfo(sesid);
        }
#endif

#if (ESENT_PUBLICONLY)
#else
        /// <summary>
        /// General database utilities
        /// </summary>
        static void MJetDBUtilities(MJET_DBUTIL dbutil)
        {
            Instance->MJetDBUtilities(dbutil);
        }

#if (ESENT_WINXP)
#else
        /// <summary>
        /// Wrapper function for Header/Trailer update code
        /// </summary>
        static void MJetUpdateDBHeaderFromTrailer(String^ database)
        {
            Instance->MJetUpdateDBHeaderFromTrailer(database);
        }
#endif

#if (ESENT_WINXP)
#else
        /// <summary>
        /// Wrapper function for ChecksumLogFromMemory
        /// </summary>
        static void MJetChecksumLogFromMemory(MJET_SESID sesid, String^ wszLog, String^ wszBase, array<Byte>^ buffer)
        {
            Instance->MJetChecksumLogFromMemory(sesid, wszLog, wszBase, buffer);
        }
#endif
#endif

        /// <summary>
        /// </summary>
        static void MJetGotoBookmark(MJET_TABLEID tableid, array<Byte>^ bookmark)
        {
            Instance->MJetGotoBookmark(tableid, bookmark);
        }

        /// <summary>
        /// </summary>
        static void MJetGotoSecondaryIndexBookmark(
            MJET_TABLEID tableid,
            array<Byte>^ secondaryKey,
            array<Byte>^ bookmark,
            MJET_GRBIT grbit)
        {
            Instance->MJetGotoSecondaryIndexBookmark(tableid, secondaryKey, bookmark, grbit);
        }

        /*

        UNDONE

        JET_ERR JET_API MJetIntersectIndexes(
        JET_SESID sesid,
        JET_INDEXRANGE * rgindexrange,
        unsigned long cindexrange,
        JET_RECORDLIST * precordlist,
        JET_GRBIT grbit );

        JET_ERR JET_API MJetComputeStats( JET_SESID sesid, JET_TABLEID tableid );

        */

#if (ESENT_WINXP)
#else
        /// <summary>
        /// </summary>
        static MJET_TABLEID MJetOpenTemporaryTable(MJET_SESID sesid, MJET_OPENTEMPORARYTABLE% opentemporarytable)
        {
            return Instance->MJetOpenTemporaryTable(sesid, opentemporarytable);
        }
#endif

        /// <summary>
        /// </summary>
        static array<MJET_COLUMNID>^ MJetOpenTempTable(
            MJET_SESID sesid,
            array<MJET_COLUMNDEF>^ columndefs,
            MJET_UNICODEINDEX unicodeindex,
            MJET_GRBIT grbit,
            MJET_TABLEID% tableid)
        {
            return Instance->MJetOpenTempTable(sesid, columndefs, unicodeindex, grbit, tableid);
        }

        /// <summary>
        /// </summary>
        static array<MJET_COLUMNID>^ MJetOpenTempTable(
            MJET_SESID sesid,
            array<MJET_COLUMNDEF>^ columndefs,
            MJET_TABLEID% tableid)
        {
            return Instance->MJetOpenTempTable(sesid, columndefs, tableid);
        }

        /*
        UNDONE

        JET_ERR JET_API MJetBackup( const _TCHAR *szBackupPath, JET_GRBIT grbit, JET_PFNSTATUS pfnStatus );
        JET_ERR JET_API MJetBackupInstance( JET_INSTANCE    instance,
        const _TCHAR        *szBackupPath,
        JET_GRBIT       grbit,
        JET_PFNSTATUS   pfnStatus );

        JET_ERR JET_API MJetRestore(const _TCHAR *sz, JET_PFNSTATUS pfn );
        JET_ERR JET_API MJetRestore2(const _TCHAR *sz, const _TCHAR *szDest, JET_PFNSTATUS pfn );

        JET_ERR JET_API MJetRestoreInstance(    JET_INSTANCE instance,
        const _TCHAR *sz,
        const _TCHAR *szDest,
        JET_PFNSTATUS pfn );
        */

        /// <summary>
        /// </summary>
        static void MJetSetIndexRange(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            Instance->MJetSetIndexRange(tableid, grbit);
        }

        /// <summary>
        /// Convenience function that doesn't throw an exception if range is empty,
        /// just returns false
        /// </summary>
        static Boolean TrySetIndexRange(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            return Instance->TrySetIndexRange(tableid, grbit);
        }

        /// <summary>
        /// </summary>
        static void ResetIndexRange(MJET_TABLEID tableid)
        {
            Instance->ResetIndexRange(tableid);
        }

        /// <summary>
        /// </summary>
        static Int64 MJetIndexRecordCount(MJET_TABLEID tableid, Int64 maxRecordsToCount)
        {
            return Instance->MJetIndexRecordCount(tableid, maxRecordsToCount);
        }

        /// <summary>
        /// </summary>
        static array<Byte>^ MJetRetrieveKey(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            return Instance->MJetRetrieveKey(tableid, grbit);
        }

        /// <summary>
        /// </summary>
        static array<Byte>^ MJetRetrieveKey(MJET_TABLEID tableid)
        {
            return Instance->MJetRetrieveKey(tableid);
        }

        /*

        UNDONE

        JET_ERR JET_API MJetBeginExternalBackup( JET_GRBIT grbit );
        JET_ERR JET_API MJetBeginExternalBackupInstance( JET_INSTANCE instance, JET_GRBIT grbit );

        JET_ERR JET_API MJetGetAttachInfo( void *pv,
        unsigned long cbMax,
        unsigned long *pcbActual );
        JET_ERR JET_API MJetGetAttachInfoInstance(  JET_INSTANCE    instance,
        void            *pv,
        unsigned long   cbMax,
        unsigned long   *pcbActual );

        JET_ERR JET_API MJetOpenFile( const _TCHAR *szFilename,
        JET_HANDLE  *phfFile,
        unsigned long *pulFileSizeLow,
        unsigned long *pulFileSizeHigh );

        JET_ERR JET_API MJetOpenFileInstance(   JET_INSTANCE instance,
        const _TCHAR *szFilename,
        JET_HANDLE  *phfFile,
        unsigned long *pulFileSizeLow,
        unsigned long *pulFileSizeHigh );

        JET_ERR JET_API MJetOpenFileSectionInstance(
        JET_INSTANCE instance,
        _TCHAR *szFile,
        JET_HANDLE *phFile,
        long iSection,
        long cSections,
        unsigned long *pulSectionSizeLow,
        long *plSectionSizeHigh);

        JET_ERR JET_API MJetReadFile( JET_HANDLE hfFile,
        void *pv,
        unsigned long cb,
        unsigned long *pcb );
        JET_ERR JET_API MJetReadFileInstance(   JET_INSTANCE instance,
        JET_HANDLE hfFile,
        void *pv,
        unsigned long cb,
        unsigned long *pcb );
        JET_ERR JET_API MJetAsyncReadFileInstance(  JET_INSTANCE instance,
        JET_HANDLE hfFile,
        void* pv,
        unsigned long cb,
        JET_OLP *pjolp );

        JET_ERR JET_API MJetCheckAsyncReadFileInstance(     JET_INSTANCE instance,
        void *pv,
        int cb,
        unsigned long pgnoFirst );

        JET_ERR JET_API MJetCloseFile( JET_HANDLE hfFile );
        JET_ERR JET_API MJetCloseFileInstance( JET_INSTANCE instance, JET_HANDLE hfFile );

        JET_ERR JET_API MJetGetLogInfo( void *pv,
        unsigned long cbMax,
        unsigned long *pcbActual );
        JET_ERR JET_API MJetGetLogInfoInstance( JET_INSTANCE instance,
        void *pv,
        unsigned long cbMax,
        unsigned long *pcbActual );

        JET_ERR JET_API MJetGetLogInfoInstance2(    JET_INSTANCE instance,
        void *pv,
        unsigned long cbMax,
        unsigned long *pcbActual,
        JET_LOGINFO * pLogInfo);

        JET_ERR JET_API MJetGetTruncateLogInfoInstance( JET_INSTANCE instance,
        void *pv,
        unsigned long cbMax,
        unsigned long *pcbActual );

        JET_ERR JET_API MJetTruncateLog( void );
        JET_ERR JET_API MJetTruncateLogInstance( JET_INSTANCE instance );

        JET_ERR JET_API MJetEndExternalBackup( void );
        JET_ERR JET_API MJetEndExternalBackupInstance( JET_INSTANCE instance );

        JET_ERR JET_API MJetEndExternalBackupInstance2( JET_INSTANCE instance, JET_GRBIT grbit );

        JET_ERR JET_API MJetExternalRestore(    _TCHAR *szCheckpointFilePath,
        _TCHAR *szLogPath,
        JET_RSTMAP *rgstmap,
        long crstfilemap,
        _TCHAR *szBackupLogPath,
        long genLow,
        long genHigh,
        JET_PFNSTATUS pfn );

        JET_ERR JET_API MJetExternalRestore2(   _TCHAR *szCheckpointFilePath,
        _TCHAR *szLogPath,
        JET_RSTMAP *rgstmap,
        long crstfilemap,
        _TCHAR *szBackupLogPath,
        JET_LOGINFO * pLogInfo,
        _TCHAR *szTargetInstanceName,
        _TCHAR *szTargetInstanceLogPath,
        _TCHAR *szTargetInstanceCheckpointPath,
        JET_PFNSTATUS pfn );

        JET_ERR JET_API MJetRegisterCallback(
        JET_SESID               sesid,
        JET_TABLEID             tableid,
        JET_CBTYP               cbtyp,
        JET_CALLBACK            pCallback,
        void *                  pvContext,
        JET_HANDLE              *phCallbackId );

        JET_ERR JET_API MJetUnregisterCallback(
        JET_SESID               sesid,
        JET_TABLEID             tableid,
        JET_CBTYP               cbtyp,
        JET_HANDLE              hCallbackId );

        JET_ERR JET_API MJetFreeBuffer( BYTE *pbBuf );

        JET_ERR JET_API MJetSetLS(
        JET_SESID       sesid,
        JET_TABLEID     tableid,
        JET_LS          ls,
        JET_GRBIT       grbit );

        JET_ERR JET_API MJetGetLS(
        JET_SESID       sesid,
        JET_TABLEID     tableid,
        JET_LS          *pls,
        JET_GRBIT       grbit );

        JET_ERR JET_API MJetOSSnapshotPrepare( JET_OSSNAPID * psnapId, const JET_GRBIT grbit );
        JET_ERR JET_API MJetOSSnapshotPrepareInstance( JET_OSSNAPID snapId, JET_INSTANCE    instance, const JET_GRBIT grbit );
        JET_ERR JET_API MJetOSSnapshotFreeze( const JET_OSSNAPID snapId, unsigned long *pcInstanceInfo, JET_INSTANCE_INFO ** paInstanceInfo, const JET_GRBIT grbit );
        JET_ERR JET_API MJetOSSnapshotThaw( const JET_OSSNAPID snapId, const JET_GRBIT grbit );
        JET_ERR JET_API MJetOSSnapshotAbort( const JET_OSSNAPID snapId, const JET_GRBIT grbit );
        JET_ERR JET_API MJetOSSnapshotTruncateLog( const JET_OSSNAPID snapId, const JET_GRBIT grbit );
        JET_ERR JET_API MJetOSSnapshotTruncateLogInstance( const JET_OSSNAPID snapId, JET_INSTANCE  instance, const JET_GRBIT grbit );
        JET_ERR JET_API MJetOSSnapshotGetFreezeInfo( const JET_OSSNAPID snapId, unsigned long *pcInstanceInfo, JET_INSTANCE_INFO ** paInstanceInfo, const JET_GRBIT grbit );
        JET_ERR JET_API MJetOSSnapshotEnd( const JET_OSSNAPID snapId, const JET_GRBIT grbit );
        */

#if (ESENT_PUBLICONLY)
#else

        /// <summary>
        /// Returns if the build type is DEBUG for the interop layer.
        /// </summary>
        static Boolean IsDbgFlagDefined()
        {
            return Instance->IsDbgFlagDefined();
        }

        /// <summary>
        /// Returns if the RTM macro is defined for the interop layer.
        /// </summary>
        static Boolean IsRtmFlagDefined()
        {
            return Instance->IsRtmFlagDefined();
        }

        /// <summary>
        /// Returns whether the interop layer calls the native ESE Unicode or ASCII APIs.
        /// </summary>
        static Boolean IsNativeUnicode()
        {
            return Instance->IsNativeUnicode();
        }

        /// <summary>
        /// Given an unmanaged JET_EMITDATACTX , return a managed MJET_EMITDATACTX struct.
        /// </summary>
        static MJET_EMITDATACTX MakeManagedEmitLogDataCtx(const ::JET_EMITDATACTX& _emitctx)
        {
            return Instance->MakeManagedEmitLogDataCtx(_emitctx);
        }

        /// <summary>
        /// Given a managed array of bytes holding an image of an unmanged JET_EMITDATACTX struct,
        /// load a managed MJET_EMITDATACTX.
        /// </summary>

        static MJET_EMITDATACTX MakeManagedEmitLogDataCtxFromBytes(array<Byte>^ bytes)
        {
            return Instance->MakeManagedEmitLogDataCtxFromBytes(bytes);
        }

        /// <summary>
        /// This is the API wrapper for the native JetConsumeLogData API.  Note the data is an array of bytes
        /// so combines the typical void * pv and ULONG cb native params into one param.
        /// </summary>
        static MJET_WRN MJetConsumeLogData(
            MJET_INSTANCE instance,
            MJET_EMITDATACTX EmitLogDataCtx,
            array<Byte>^ data,
            MJET_GRBIT grbit)
        {
            return Instance->MJetConsumeLogData(instance, EmitLogDataCtx, data, grbit);
        }

        /// <summary>
        /// This variant of MJetConsumeLogData is used by HA to allow for a binary pass-through of the emit metadata so that
        /// we avoid converting between managed/unmanged EMITDATACTX, which is currently a lossy transformation.
        /// </summary>
        static MJET_WRN MJetConsumeLogData(
            MJET_INSTANCE instance,
            array<Byte>^ emitMetaData,
            array<Byte>^ data,
            MJET_GRBIT grbit)
        {
            return Instance->MJetConsumeLogData(instance, emitMetaData, data, grbit);
        }
#endif

        /// <summary>
        /// </summary>
        static void MJetStopServiceInstance2(
            MJET_INSTANCE instance,
            MJET_GRBIT grbit)
        {
            return Instance->MJetStopServiceInstance2(instance, grbit);
        }


    };

#if (ESENT_WINXP)
#else

#ifdef INTERNALUSE
    JET_ERR MJetInitCallbackInfo::MJetInitCallbackOwn(
        ::JET_SESID /*_sesid*/,
        ::JET_SNP _snp,
        ::JET_SNT _snt,
        void * _pv)
    {
        MJetInitCallback^ UserCallback = GetUserCallback();

        JET_ERR err = JET_errSuccess;

#if (ESENT_PUBLICONLY)
#else
        try
        {
            MJET_SNP snp = Snp( _snp );
            MJET_SNT snt = Snt( _snt );

            if ( snp ==  MJET_SNP::RecoveryControl )
            {
                ::JET_RECOVERYCONTROL * _prcRecovery = (::JET_RECOVERYCONTROL *)_pv;
                MJET_RECOVERY_RETVALS ret = UserCallback->RecoveryControl( m_interop->MakeManagedRecoveryControl( _prcRecovery ) );
                if(ret == MJET_RECOVERY_RETVALS::Default)
                {
                    return _prcRecovery->errDefault;
                }
                else
                {
                    return (JET_ERR) ret;
                }
            }
            else if ( snp == MJET_SNP::Restore )
            {
                switch ( snt )
                {
                    case MJET_SNT::Progress:
                    {
                        ::JET_SNPROG * _psnProg = (::JET_SNPROG *)_pv;

                        UserCallback->Progress( Snt( _snt ), m_interop->MakeManagedSNProg( _psnProg ) );
                    }
                    break;

                    default:
                        // Do nothing if we don't understand it.
                    break;
                }
            }
            else if ( snp == MJET_SNP::ExternalAutoHealing )
            {
                switch ( snt )
                {
                    case MJET_SNT::PagePatchRequest:
                    {
                        ::JET_SNPATCHREQUEST * _psnpatchrequest = (::JET_SNPATCHREQUEST *)_pv;

                        //UNDONE: Assert( snp == MJET_SNP::ExternalAutoHealing );

                        UserCallback->PagePatch( m_interop->MakeManagedSNPatchRequest( _psnpatchrequest ) );
                    }
                    break;

                    case MJET_SNT::CorruptedPage:
                    {
                        ::JET_SNCORRUPTEDPAGE * _psncorruptedpage = (::JET_SNCORRUPTEDPAGE *)_pv;

                        //UNDONE: Assert( snp == MJET_SNP::ExternalAutoHealing );

                        UserCallback->CorruptedPage( (int)_psncorruptedpage->pageNumber );
                    }
                    break;

                    default:
                        // Do nothing if we don't understand it.
                    break;
                }
            }
            else
            {
                //  feign comprehension ...
                err = JET_errSuccess;
            }
            // if/else if/else on SNP

        }
        catch( IsamRecoveredWithoutUndoException^ )
        {
            err = JET_errRecoveredWithoutUndo;
        }
        catch( IsamOutOfMemoryException^ )
        {
            err = JET_errOutOfMemory;
        }
        catch( Exception^ e )
        {

            err = JET_errCallbackFailed;
            SetException( e );
        }
        catch (Object^)
        {
            // Silence FxCop rule Microsoft.Security:CA2102
            throw;
        }
#endif

        return err;
    }
#endif
#endif

}
}
}
