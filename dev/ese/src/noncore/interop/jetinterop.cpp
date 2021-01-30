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

    class FastReallocBuffer
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

    inline FastReallocBuffer::FastReallocBuffer( const size_t cb, BYTE* const rgb )
    {
        m_pbMin = rgb;
        m_pbMax = rgb + cb;
        m_pb    = m_pbMin;
    }

    inline void* FastReallocBuffer::Realloc( void* const pv, const ULONG cb )
    {
        void*           pvAlloc = NULL;
        const size_t    cbAlign = sizeof( void* );
        size_t          cbAlloc = cb + cbAlign - 1;
        cbAlloc = cbAlloc - cbAlloc % cbAlign;
        if (cbAlloc < cb)
        {
            return pvAlloc;
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
            }
            else
            {
                delete [] pv;
            }
        }
        else
        {
        }

        return pvAlloc;
    }

    void* JET_API FastReallocBuffer::Realloc_( FastReallocBuffer* const pfrb, void* const pv, const ULONG cb )
    {
        return pfrb->Realloc( pv, cb );
    }

    void FastReallocBuffer::FreeEnumeratedColumns( ULONG& cEnumColumn, ::JET_ENUMCOLUMN*& rgEnumColumn )
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
            throw gcnew IsamInvalidParameterException();
        }
    }

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
            ::JET_SESID ,
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

    MSINTERNAL delegate void MJetOnEndOnlineDefragCallbackDelegate();

    private ref class MJetDefragCallbackInfo
    {
    private:
        MJetOnEndOnlineDefragCallbackDelegate^ m_callback;

        GCHandle m_gcHandle;

    public:
        MJetDefragCallbackInfo(MJetOnEndOnlineDefragCallbackDelegate^ callback)
            : m_callback(callback)
        {
        }

        MJetOnEndOnlineDefragCallbackDelegate^ GetUserCallback() { return this->m_callback; }

        GCHandle GetGCHandle() { return this->m_gcHandle; }

        void SetGCHandle(GCHandle gcHandle) { this->m_gcHandle = gcHandle; }

        JET_ERR MJetOnEndOLDCallbackOwn(
            ::JET_SESID     ,
            ::JET_DBID      ,
            ::JET_TABLEID   ,
            ::JET_CBTYP     ,
            void *          ,
            void *          ,
            void *          ,
            ::JET_API_PTR   )
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

    MSINTERNAL delegate Exception^ ExceptionTranslator(IsamErrorException^);

    MSINTERNAL interface class IJetInteropCommon
    {
        void SetExceptionTranslator(ExceptionTranslator^ translator);

        MJET_INSTANCE MJetInit();

        MJET_INSTANCE MJetInit(MJET_INSTANCE instance);

        MJET_INSTANCE MJetInit(MJET_INSTANCE instance, MJET_GRBIT grbit);

        MJET_INSTANCE MJetCreateInstance(String^ name);

        MJET_INSTANCE MJetCreateInstance(
            String^ name,
            String^ displayName,
            MJET_GRBIT grbit);

        MJET_INSTANCE MJetCreateInstance(String^ name, String^ displayName);

        void MJetTerm();

        void MJetTerm(MJET_INSTANCE instance);

        void MJetTerm(MJET_INSTANCE instance, MJET_GRBIT grbit);

        void MJetStopService();

        void MJetStopServiceInstance(MJET_INSTANCE instance);

        void MJetStopBackup();

        void MJetStopBackupInstance(MJET_INSTANCE instance);

        void MJetSetSystemParameter(
            MJET_INSTANCE instance,
            MJET_SESID sesid,
            MJET_PARAM paramid,
            Int64 param,
            String^ pstr);

        void MJetSetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            Int64 param);

        void MJetSetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            String^ pstr);

        void MJetSetSystemParameter(MJET_PARAM paramid, Int64 param);

        void MJetSetSystemParameter(MJET_PARAM paramid, String^ pstr);

        void MJetGetSystemParameter(
            MJET_INSTANCE instance,
            MJET_SESID sesid,
            MJET_PARAM paramid,
            Int64% param,
            String^% pstr);

        void MJetGetSystemParameter(
            MJET_INSTANCE instance,
            MJET_SESID sesid,
            MJET_PARAM paramid,
            Int64% param);

        void MJetGetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            Int64% param);

        void MJetGetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            String^% pstr);

        void MJetGetSystemParameter(MJET_PARAM paramid, Int64% param);

        void MJetGetSystemParameter(MJET_PARAM paramid, String^% pstr);

        MJET_SESID MJetBeginSession(
            MJET_INSTANCE instance,
            String^ user,
            String^ password);

        MJET_SESID MJetBeginSession(MJET_INSTANCE instance);

        MJET_SESID MJetDupSession(MJET_SESID sesid);

        void MJetEndSession(MJET_SESID sesid, MJET_GRBIT grbit);

        void MJetEndSession(MJET_SESID sesid);

        Int64 MJetGetVersion(MJET_SESID sesid);

        MJET_WRN MJetIdle(MJET_SESID sesid, MJET_GRBIT grbit);

        MJET_DBID MJetCreateDatabase(
            MJET_SESID sesid,
            String^ file,
            String^ connect,
            MJET_GRBIT grbit,
            MJET_WRN% wrn);

        MJET_DBID MJetCreateDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit,
            MJET_WRN% wrn);

        MJET_DBID MJetCreateDatabase(MJET_SESID sesid, String^ file);

        MJET_DBID MJetCreateDatabase(
            MJET_SESID sesid,
            String^ file,
            Int64 maxPages,
            MJET_GRBIT grbit,
            MJET_WRN% wrn);

        MJET_WRN MJetAttachDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit);

        MJET_WRN MJetAttachDatabase(MJET_SESID sesid, String^ file);

        MJET_WRN MJetAttachDatabase(
            MJET_SESID sesid,
            String^ file,
            Int64 maxPages,
            MJET_GRBIT grbit);

        MJET_WRN MJetDetachDatabase(MJET_SESID sesid, String^ file);

        MJET_WRN MJetDetachDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit);

        MJET_OBJECTINFO MJetGetTableInfo(MJET_TABLEID tableid);

        String^ MJetGetTableInfoName(MJET_TABLEID tableid);

        int MJetGetTableInfoInitialSize(MJET_TABLEID tableid);

        int MJetGetTableInfoDensity(MJET_TABLEID tableid);

        String^ MJetGetTableInfoTemplateTable(MJET_TABLEID tableid);

        int MJetGetTableInfoOwnedPages(MJET_TABLEID tableid);

        int MJetGetTableInfoAvailablePages(MJET_TABLEID tableid);

        MJET_OBJECTINFO MJetGetObjectInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ object);

        MJET_OBJECTINFO MJetGetObjectInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_OBJTYP objtyp,
            String^ container,
            String^ object);

        MJET_OBJECTLIST MJetGetObjectInfoList(MJET_SESID sesid, MJET_DBID dbid);

        MJET_OBJECTLIST MJetGetObjectInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_OBJTYP objtyp);

        MJET_OBJECTLIST MJetGetObjectInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_OBJTYP objtyp,
            String^ container);

        MJET_OBJECTLIST MJetGetObjectInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_OBJTYP objtyp,
            String^ container,
            String^ object);

        Int64 MJetGetCheckpointFileInfo(String^ checkpoint);

        MJET_DBINFO MJetGetDatabaseFileInfo(String^ database);

        MJET_TABLEID MJetCreateTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name,
            Int32 pages,
            Int32 density);

        MJET_TABLEID MJetCreateTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name);

        void MJetCreateTableColumnIndex(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_TABLECREATE tablecreate);

        void MJetDeleteTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name);

        void MJetRenameTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ oldName,
            String^ newName);

        MJET_COLUMNBASE MJetGetColumnInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            Int64 colid);

        MJET_COLUMNBASE MJetGetColumnInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            MJET_COLUMNID columnid);

        MJET_COLUMNBASE MJetGetColumnInfo(MJET_TABLEID tableid, String^ column);

        MJET_COLUMNBASE MJetGetColumnInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ column);

        MJET_COLUMNLIST MJetGetColumnInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            Boolean sortByColumnid);

        MJET_COLUMNLIST MJetGetColumnInfoList(MJET_TABLEID tableid, Boolean sortByColumnid);

        MJET_COLUMNLIST MJetGetColumnInfoList(MJET_TABLEID tableid);

        MJET_COLUMNID MJetAddColumn(
            MJET_TABLEID tableid,
            String^ name,
            MJET_COLUMNDEF% definition,
            array<Byte>^ defaultValue);

        MJET_COLUMNID MJetAddColumn(
            MJET_TABLEID tableid,
            String^ name,
            MJET_COLUMNDEF% definition);

        void MJetDeleteColumn(MJET_TABLEID tableid, String^ name);

        void MJetDeleteColumn(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit);

        void MJetRenameColumn(
            MJET_TABLEID tableid,
            String^ oldName,
            String^ newName,
            MJET_GRBIT grbit);

        void MJetSetColumnDefaultValue(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ column,
            array<Byte>^ defaultValue,
            MJET_GRBIT grbit);

        MJET_INDEXLIST MJetGetIndexInfo(MJET_TABLEID tableid, String^ index);

        MJET_INDEXLIST MJetGetIndexInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index);

        MJET_INDEXLIST MJetGetIndexInfoList(MJET_TABLEID tableid);

        MJET_INDEXLIST MJetGetIndexInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table);

        int MJetGetIndexInfoDensity(MJET_TABLEID tableid, String^ index);

        int MJetGetIndexInfoDensity(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index);

        int MJetGetIndexInfoLCID(MJET_TABLEID tableid, String^ index);

        int MJetGetIndexInfoLCID(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index);

        int MJetGetIndexInfoKeyMost(MJET_TABLEID tableid, String^ index);

        int MJetGetIndexInfoKeyMost(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index);

        void MJetCreateIndex(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit,
            String^ key,
            Int32 density);

        void MJetCreateIndex(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit,
            String^ key);

        void MJetCreateIndex(
            MJET_TABLEID tableid,
            String^ name,
            String^ key);

        void MJetCreateIndex(MJET_TABLEID tableid, array<MJET_INDEXCREATE>^ indexcreates);

        void MJetDeleteIndex(MJET_TABLEID tableid, String^ name);

        void MJetBeginTransaction(MJET_SESID sesid);

        void MJetBeginTransaction(MJET_SESID sesid, MJET_GRBIT grbit);

        void MJetBeginTransaction(
            MJET_SESID sesid,
            Int32 trxid,
            MJET_GRBIT grbit);

        void MJetCommitTransaction(MJET_SESID sesid, MJET_GRBIT grbit);

        void MJetCommitTransaction(MJET_SESID sesid);

        void MJetRollback(MJET_SESID sesid, MJET_GRBIT grbit);

        void MJetRollback(MJET_SESID sesid);

        void MJetGetDatabaseInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            array<Byte>^ resultBuffer,
            MJET_DBINFO_LEVEL InfoLevel);

        MJET_DBINFO MJetGetDatabaseInfo(
            MJET_SESID sesid,
            MJET_DBID dbid);

        Int64 MJetGetDatabaseInfoFilesize(
            MJET_SESID sesid,
            MJET_DBID dbid);

        String^ MJetGetDatabaseInfoFilename(
            MJET_SESID sesid,
            MJET_DBID dbid);

        MJET_DBID MJetOpenDatabase(
            MJET_SESID sesid,
            String^ file,
            String^ connect,
            MJET_GRBIT grbit,
            MJET_WRN% wrn);

        MJET_DBID MJetOpenDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit,
            MJET_WRN% wrn);

        MJET_DBID MJetOpenDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit);

        MJET_DBID MJetOpenDatabase(MJET_SESID sesid, String^ file);

        void MJetCloseDatabase(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_GRBIT grbit);

        void MJetCloseDatabase(MJET_SESID sesid, MJET_DBID dbid);

        MJET_TABLEID MJetOpenTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name,
            array<Byte>^ parameters,
            MJET_GRBIT grbit);

        MJET_TABLEID MJetOpenTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name,
            MJET_GRBIT grbit);

        MJET_TABLEID MJetOpenTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name);

        Boolean TryOpenTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name,
            MJET_TABLEID% tableid);

        void MJetSetTableSequential(MJET_TABLEID tableid, MJET_GRBIT grbit);

        void MJetResetTableSequential(MJET_TABLEID tableid, MJET_GRBIT grbit);

        void MJetCloseTable(MJET_TABLEID tableid);

        void MJetDelete(MJET_TABLEID tableid);

        array<Byte>^ MJetUpdate(MJET_TABLEID tableid);

        Nullable<Int64> MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit,
            MJET_RETINFO retinfo,
            ArraySegment<Byte> buffer);

        array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit,
            MJET_RETINFO retinfo,
            Int64 maxdata);

        array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit,
            MJET_RETINFO retinfo);

        array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit);

        array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit,
            Int64 maxdata);

        array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            Int64 maxdata);

        array<Byte>^ MJetRetrieveColumn(MJET_TABLEID tableid, MJET_COLUMNID columnid);

        Int64 MJetRetrieveColumnSize(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            Int64 itagSequence,
            MJET_GRBIT grbit);

        Int64 MJetRetrieveColumnSize(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            Int64 itagSequence);

        Int64 MJetRetrieveColumnSize(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit);

        Int64 MJetRetrieveColumnSize(MJET_TABLEID tableid, MJET_COLUMNID columnid);

        array<MJET_ENUMCOLUMN>^ MJetEnumerateColumns(
            MJET_TABLEID tableid,
            array<MJET_ENUMCOLUMNID>^ columnids,
            MJET_GRBIT grbit,
            MJET_WRN& wrn);

        array<MJET_ENUMCOLUMN>^ MJetEnumerateColumns(
            MJET_TABLEID tableid,
            array<MJET_ENUMCOLUMNID>^ columnids,
            MJET_GRBIT grbit);

        array<MJET_ENUMCOLUMN>^ MJetEnumerateColumns(MJET_TABLEID tableid, array<MJET_ENUMCOLUMNID>^ columnids);

        array<MJET_ENUMCOLUMN>^ MJetEnumerateColumns(MJET_TABLEID tableid);

        MJET_WRN MJetSetColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            array<Byte>^ data,
            MJET_GRBIT grbit,
            MJET_SETINFO setinfo);

        MJET_WRN MJetSetColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            array<Byte>^ data,
            MJET_GRBIT grbit);

        MJET_WRN MJetSetColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            array<Byte>^ data);

        void MJetPrepareUpdate(MJET_TABLEID tableid, MJET_PREP prep);

        MJET_RECPOS MJetGetRecordPosition(MJET_TABLEID tableid);

        void MJetGotoPosition(MJET_TABLEID tableid, MJET_RECPOS recpos);

        MJET_TABLEID MJetDupCursor(MJET_TABLEID tableid, MJET_GRBIT grbit);

        MJET_TABLEID MJetDupCursor(MJET_TABLEID tableid);

        String^ MJetGetCurrentIndex(MJET_TABLEID tableid);

        void MJetSetCurrentIndex(MJET_TABLEID tableid, String^ name);

        void MJetSetCurrentIndex(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit);

        void MJetSetCurrentIndex(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit,
            Int64 itag);

        MJET_WRN MJetMove(
            MJET_TABLEID tableid,
            Int32 rows,
            MJET_GRBIT grbit);

        MJET_WRN MJetMove(MJET_TABLEID tableid, Int32 rows);

        MJET_WRN MJetMove(
            MJET_TABLEID tableid,
            MJET_MOVE rows,
            MJET_GRBIT grbit);

        MJET_WRN MJetMove(MJET_TABLEID tableid, MJET_MOVE rows);

        void MoveBeforeFirst(MJET_TABLEID tableid);

        void MoveAfterLast(MJET_TABLEID tableid);

        Boolean TryMoveFirst(MJET_TABLEID tableid);

        Boolean TryMoveNext(MJET_TABLEID tableid);

        Boolean TryMoveLast(MJET_TABLEID tableid);

        Boolean TryMovePrevious(MJET_TABLEID tableid);

        Boolean TryMove(MJET_TABLEID tableid, MJET_MOVE rows);

        Boolean TryMove(MJET_TABLEID tableid, Int32 rows);

        void MJetGetLock(MJET_TABLEID tableid, MJET_GRBIT grbit);

        MJET_WRN MJetMakeKey(
            MJET_TABLEID tableid,
            array<Byte>^ data,
            MJET_GRBIT grbit);

        MJET_WRN MJetSeek(MJET_TABLEID tableid, MJET_GRBIT grbit);

        Boolean TrySeek(MJET_TABLEID tableid, MJET_GRBIT grbit);

        Boolean TrySeek(MJET_TABLEID tableid);

        array<Byte>^ MJetGetBookmark(MJET_TABLEID tableid);

        MJET_SECONDARYINDEXBOOKMARK MJetGetSecondaryIndexBookmark(MJET_TABLEID tableid, MJET_GRBIT grbit);

        MJET_SECONDARYINDEXBOOKMARK MJetGetSecondaryIndexBookmark(MJET_TABLEID tableid);

        MJET_WRN MJetDefragment(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            Int32% passes,
            Int32% seconds,
            MJET_GRBIT grbit);

        MJET_WRN MJetDefragment2(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            Int32% passes,
            Int32% seconds,
            MJetOnEndOnlineDefragCallbackDelegate^ callback,
            MJET_GRBIT grbit);

        Int64 MJetSetDatabaseSize(
            MJET_SESID sesid,
            String^ database,
            Int64 pages);

        Int64 MJetGrowDatabase(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pages);

        MJET_WRN MJetResizeDatabase(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pages,
            [Out] Int64% actualPages,
            MJET_GRBIT grbit);

        void MJetSetSessionContext(MJET_SESID sesid, IntPtr context);

        void MJetResetSessionContext(MJET_SESID sesid);

        void MJetGotoBookmark(MJET_TABLEID tableid, array<Byte>^ bookmark);

        void MJetGotoSecondaryIndexBookmark(
            MJET_TABLEID tableid,
            array<Byte>^ secondaryKey,
            array<Byte>^ bookmark,
            MJET_GRBIT grbit);

        array<MJET_COLUMNID>^ MJetOpenTempTable(
            MJET_SESID sesid,
            array<MJET_COLUMNDEF>^ columndefs,
            MJET_UNICODEINDEX unicodeindex,
            MJET_GRBIT grbit,
            MJET_TABLEID% tableid);

        array<MJET_COLUMNID>^ MJetOpenTempTable(
            MJET_SESID sesid,
            array<MJET_COLUMNDEF>^ columndefs,
            MJET_TABLEID% tableid);

        void MJetSetIndexRange(MJET_TABLEID tableid, MJET_GRBIT grbit);

        Boolean TrySetIndexRange(MJET_TABLEID tableid, MJET_GRBIT grbit);

        void ResetIndexRange(MJET_TABLEID tableid);

        Int64 MJetIndexRecordCount(MJET_TABLEID tableid, Int64 maxRecordsToCount);

        array<Byte>^ MJetRetrieveKey(MJET_TABLEID tableid, MJET_GRBIT grbit);

        array<Byte>^ MJetRetrieveKey(MJET_TABLEID tableid);

#ifdef INTERNALUSE
        void MJetTracing(MJET_TRACEOP traceOp, MJET_TRACETAG traceTag, IntPtr ul);

        void MJetTracing(MJET_TRACEOP traceOp, MJET_TRACETAG traceTag, MJET_DBID dbid);
#endif
    };

    MSINTERNAL interface class IJetInteropPublicOnly : public IJetInteropCommon
    {
        MJET_SESID MakeManagedSesid(::JET_SESID _sesid, MJET_INSTANCE instance);

        MJET_DBID MakeManagedDbid(::JET_DBID _dbid);

        MJET_TABLEID MakeManagedTableid(::JET_TABLEID _tableid, MJET_SESID sesid, MJET_DBID dbid);

        MJET_DBINFO MakeManagedDbinfo(IntPtr _handle, unsigned int _cbconst);

        MJET_DBINFO MakeManagedDbinfo(const ::JET_DBINFOMISC& _dbinfomisc);

        String^ MJetGetColumnName(MJET_TABLEID tableid, MJET_COLUMNID columnid);

        String^ MJetGetColumnName(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ tablename,
            MJET_COLUMNID columnid);

        array<Byte>^ BytesFromObject(
            MJET_COLTYP type,
            Boolean isASCII,
            Object^ o);
    };

    MSINTERNAL interface class IJetInteropWinXP : public IJetInteropPublicOnly
    {
        void MJetResetCounter(MJET_SESID sesid, Int32 CounterType);

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

        void MJetUpgradeDatabase(
            MJET_SESID sesid,
            String^ database,
            String^ streamingFile,
            MJET_GRBIT grbit);

        MJET_SESSIONINFO MJetGetSessionInfo(MJET_SESID sesid);

        void MJetDBUtilities(MJET_DBUTIL dbutil);

        Boolean IsDbgFlagDefined();

        Boolean IsRtmFlagDefined();

        Boolean IsNativeUnicode();

#ifdef INTERNALUSE
        MJET_EMITDATACTX MakeManagedEmitLogDataCtx(const ::JET_EMITDATACTX& _emitctx);

        MJET_EMITDATACTX MakeManagedEmitLogDataCtxFromBytes(array<Byte>^ bytes);

        MJET_WRN MJetConsumeLogData(
            MJET_INSTANCE instance,
            MJET_EMITDATACTX EmitLogDataCtx,
            array<Byte>^ data,
            MJET_GRBIT grbit);

        MJET_WRN MJetConsumeLogData(
            MJET_INSTANCE instance,
            array<Byte>^ emitMetaDataAsManagedBytes,
            array<Byte>^ data,
            MJET_GRBIT grbit);
#endif
    };

    MSINTERNAL interface class IJetInteropExchange : public IJetInteropCommon
    {
        MJET_SESID MakeManagedSesid(::JET_SESID _sesid, MJET_INSTANCE instance);

        MJET_DBID MakeManagedDbid(::JET_DBID _dbid);

        MJET_TABLEID MakeManagedTableid(::JET_TABLEID _tableid, MJET_SESID sesid, MJET_DBID dbid);

        MJET_DBINFO MakeManagedDbinfo(IntPtr _handle, unsigned int _cbconst);

        MJET_DBINFO MakeManagedDbinfo(const ::JET_DBINFOMISC7& _dbinfomisc);

        array<Byte>^ GetBytes(MJET_DBINFO dbinfo);

#ifdef INTERNALUSE
        MJET_INSTANCE MJetInit(
            MJET_INSTANCE instance,
            array<MJET_RSTMAP>^ rstmaps,
            Int64 genStop,
            MJetInitCallback^ callback,
            MJET_GRBIT grbit);
#endif

        MJET_INSTANCE MJetInit(
            MJET_INSTANCE instance,
            array<MJET_RSTMAP>^ rstmaps,
            Int64 ,
            MJET_GRBIT grbit);

        MJET_INSTANCE MJetInit(
            MJET_INSTANCE instance,
            array<MJET_RSTMAP>^ rstmaps,
            MJET_GRBIT grbit);

        void MJetGetResourceParam(
            MJET_INSTANCE instance,
            MJET_RESOPER resoper,
            MJET_RESID resid,
            Int64% param);

        void MJetResetCounter(MJET_SESID sesid, Int32 CounterType);

        Int32 MJetGetCounter(MJET_SESID sesid, Int32 CounterType);

        void MJetTestHookEnableFaultInjection(
            Int32 id,
            JET_ERR err,
            Int32 ulProbability,
            MJET_GRBIT grbit);

        void MJetTestHookEnableConfigOverrideInjection(
            Int32 id,
            Int64 ulOverride,
            Int32 ulProbability,
            MJET_GRBIT grbit);

        IntPtr MJetTestHookEnforceContextFail(
            IntPtr callback);

        void MJetTestHookEvictCachedPage(
            MJET_DBID dbid,
            Int32 pgno);

        void MJetSetNegativeTesting(
            MJET_NEGATIVE_TESTING_BITS grbit);

        void MJetResetNegativeTesting(
            MJET_NEGATIVE_TESTING_BITS grbit);

        array<MJET_INSTANCE_INFO>^ MJetGetInstanceInfo();

        MJET_PAGEINFO MJetGetPageInfo(array<Byte>^ page, Int64 PageNumber);

        MJET_LOGINFOMISC MJetGetLogFileInfo(String^ logfile);

        void MJetRemoveLogfile(String^ database, String^ logfile);

        array<Byte>^ MJetGetDatabasePages(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pgnoStart,
            Int64 cpg,
            MJET_GRBIT grbit);

        void MJetOnlinePatchDatabasePage(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pgno,
            array<Byte>^ token,
            array<Byte>^ data,
            MJET_GRBIT grbit);

        void MJetBeginDatabaseIncrementalReseed(
            MJET_INSTANCE instance,
            String^ pstrDatabase,
            MJET_GRBIT grbit);

        void MJetEndDatabaseIncrementalReseed(
            MJET_INSTANCE instance,
            String^ pstrDatabase,
            Int64 genMinRequired,
            Int64 genFirstDivergedLog,
            Int64 genMaxRequired,
            MJET_GRBIT grbit);

        void MJetPatchDatabasePages(
            MJET_INSTANCE instance,
            String^ pstrDatabase,
            Int64 pgnoStart,
            Int64 cpg,
            array<Byte>^ rgb,
            MJET_GRBIT grbit);

        MJET_COLUMNBASE MJetGetColumnInfo(MJET_TABLEID tableid, Int64 colid);

        MJET_COLUMNBASE MJetGetColumnInfo(MJET_TABLEID tableid, MJET_COLUMNID columnid);

        MJET_INDEXCREATE MJetGetIndexInfoCreate(MJET_TABLEID tableid, String^ index);

        MJET_INDEXCREATE MJetGetIndexInfoCreate(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index);

        array<Byte>^ MJetUpdate(MJET_TABLEID tableid, MJET_GRBIT grbit);

        MJET_RECSIZE MJetGetRecordSize(MJET_TABLEID tableid, MJET_GRBIT grbit);

        MJET_RECSIZE MJetGetRecordSize(MJET_TABLEID tableid);

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

        void MJetUpgradeDatabase(
            MJET_SESID sesid,
            String^ database,
            String^ streamingFile,
            MJET_GRBIT grbit);

        void MJetSetMaxDatabaseSize(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pages,
            MJET_GRBIT grbit);

        Int64 MJetGetMaxDatabaseSize(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_GRBIT grbit);

        MJET_SESSIONINFO MJetGetSessionInfo(MJET_SESID sesid);

        void MJetDBUtilities(MJET_DBUTIL dbutil);

        void MJetUpdateDBHeaderFromTrailer(String^ database);

        void MJetChecksumLogFromMemory(MJET_SESID sesid, String^ wszLog, String^ wszBase, array<Byte>^ buffer);

        MJET_TABLEID MJetOpenTemporaryTable(MJET_SESID sesid, MJET_OPENTEMPORARYTABLE% opentemporarytable);

        Boolean IsDbgFlagDefined();

        Boolean IsRtmFlagDefined();

        Boolean IsNativeUnicode();

#ifdef INTERNALUSE
        MJET_EMITDATACTX MakeManagedEmitLogDataCtx(const ::JET_EMITDATACTX& _emitctx);

        MJET_EMITDATACTX MakeManagedEmitLogDataCtxFromBytes(array<Byte>^ bytes);

        MJET_WRN MJetConsumeLogData(
            MJET_INSTANCE instance,
            MJET_EMITDATACTX EmitLogDataCtx,
            array<Byte>^ data,
            MJET_GRBIT grbit);

        MJET_WRN MJetConsumeLogData(
            MJET_INSTANCE instance,
            array<Byte>^ emitMetaDataAsManagedBytes,
            array<Byte>^ data,
            MJET_GRBIT grbit);
#endif
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

        virtual ::JET_INSTANCE GetUnmanagedInst(MJET_INSTANCE instance)
        {
            ::JET_INSTANCE _inst = (::JET_INSTANCE)(instance.NativeValue);
            return _inst;
        }

        virtual MJET_INSTANCE MakeManagedInst(::JET_INSTANCE _instance)
        {
            MJET_INSTANCE instance;
            instance.NativeValue = _instance;
            return instance;
        }

        virtual ::JET_SESID GetUnmanagedSesid(MJET_SESID sesid)
        {
            ::JET_SESID _sesid = (::JET_SESID)(sesid.NativeValue);
            return _sesid;
        }

        virtual MJET_SESID MakeManagedSesid(::JET_SESID _sesid, MJET_INSTANCE instance)
        {
            MJET_SESID sesid;
            sesid.Instance = instance;
            sesid.NativeValue = _sesid;
            return sesid;
        }

        virtual ::JET_DBID GetUnmanagedDbid(MJET_DBID dbid)
        {
            ::JET_DBID _dbid = (::JET_DBID)(dbid.NativeValue);
            return _dbid;
        }

        virtual MJET_DBID MakeManagedDbid(::JET_DBID _dbid)
        {
            MJET_DBID dbid;
            dbid.NativeValue = _dbid;
            return dbid;
        }

        virtual ::JET_TABLEID GetUnmanagedTableid(MJET_TABLEID tableid)
        {
            ::JET_TABLEID _tableid = (::JET_TABLEID)(tableid.NativeValue);
            return _tableid;
        }

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

        virtual MJET_TABLEID MakeManagedTableid(::JET_TABLEID _tableid, MJET_SESID sesid)
        {
            return MakeManagedTableid( _tableid, sesid, MakeManagedDbid( JET_dbidNil ) );
        }

        virtual ::JET_COLUMNID GetUnmanagedColumnid(MJET_COLUMNID columnid)
        {
            ::JET_COLUMNID _columnid = (::JET_COLUMNID)(columnid.NativeValue);
            return _columnid;
        }

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

        virtual void FreeUnmanagedColumndef(::JET_COLUMNDEF )
        {
            return;
        }

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
            }
            catch ( ... )
            {
                FreeUnmanagedColumndefs( _pcolumndef, _i );
                throw;
            }

            return _pcolumndef;
        }

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

        virtual void FreeUnmanagedOpenTemporaryTable(__in __drv_freesMem(object) ::JET_OPENTEMPORARYTABLE _opentemporarytable)
        {
            FreeUnmanagedColumndefs( (::JET_COLUMNDEF*)_opentemporarytable.prgcolumndef, _opentemporarytable.ccolumn );
            delete _opentemporarytable.pidxunicode;
            ::delete [] _opentemporarytable.prgcolumnid;
        }
#endif

        virtual ::JET_SETINFO GetUnmanagedSetinfo(MJET_SETINFO setinfo)
        {
            ::JET_SETINFO _setinfo;
            _setinfo.cbStruct = sizeof( _setinfo );
            _setinfo.ibLongValue = (unsigned long)setinfo.IbLongValue;
            _setinfo.itagSequence = (unsigned long)setinfo.ItagSequence;
            return _setinfo;
        }

        virtual ::JET_RETINFO GetUnmanagedRetinfo(MJET_RETINFO retinfo)
        {
            ::JET_RETINFO _retinfo;
            _retinfo.cbStruct = sizeof( _retinfo );
            _retinfo.ibLongValue = (unsigned long)retinfo.IbLongValue;
            _retinfo.itagSequence = (unsigned long)retinfo.ItagSequence;
            return _retinfo;
        }

        virtual ::JET_RECPOS GetUnmanagedRecpos(MJET_RECPOS recpos)
        {
            ::JET_RECPOS _recpos;
            _recpos.cbStruct = sizeof( _recpos );
            _recpos.centriesLT = (unsigned long)recpos.EntriesLessThan;
            _recpos.centriesTotal = (unsigned long)recpos.TotalEntries;
            return _recpos;
        }

        virtual MJET_RECPOS MakeManagedRecpos(const ::JET_RECPOS& _recpos)
        {
            MJET_RECPOS recpos;
            recpos.EntriesLessThan = _recpos.centriesLT;
            recpos.TotalEntries = _recpos.centriesTotal;
            return recpos;
        }

        virtual ::JET_RSTMAP GetUnmanagedRstmap(MJET_RSTMAP rstmap)
        {
            ::JET_RSTMAP _rstmap;


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

        virtual MJET_OBJECTINFO MakeManagedObjectInfo(const ::JET_OBJECTINFO& _objectinfo)
        {
            MJET_OBJECTINFO objectinfo;
            objectinfo.Objtyp = (MJET_OBJTYP)_objectinfo.objtyp;
            objectinfo.Grbit = (MJET_GRBIT)_objectinfo.grbit;
            objectinfo.Flags = (MJET_GRBIT)_objectinfo.flags;
            return objectinfo;
        }

        virtual MJET_OBJECTLIST MakeManagedObjectList(MJET_SESID sesid, const ::JET_OBJECTLIST& _objectlist)
        {
            MJET_OBJECTLIST objectlist;
            objectlist.Tableid = MakeManagedTableid( _objectlist.tableid, sesid );
            objectlist.Records = _objectlist.cRecord;

            bool fASCII = !IsNativeUnicode();
            objectlist.ColumnidContainerName = MakeManagedColumnid( _objectlist.columnidcontainername, MJET_COLTYP::Text, fASCII );
            objectlist.ColumnidObjectName = MakeManagedColumnid( _objectlist.columnidobjectname, MJET_COLTYP::Text, fASCII );
            objectlist.ColumnidObjectType = MakeManagedColumnid( _objectlist.columnidobjtyp, MJET_COLTYP::Long, false );
            objectlist.ColumnidGrbit = MakeManagedColumnid( _objectlist.columnidgrbit, MJET_COLTYP::Long, false );
            objectlist.ColumnidFlags = MakeManagedColumnid( _objectlist.columnidflags, MJET_COLTYP::Long, false );

            return objectlist;
        }

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

        virtual MJET_COLUMNLIST MakeManagedColumnList(MJET_SESID sesid, const ::JET_COLUMNLIST& _columnlist)
        {
            MJET_COLUMNLIST columnlist;
            columnlist.Tableid = MakeManagedTableid( _columnlist.tableid, sesid );
            columnlist.Records = _columnlist.cRecord;

            bool fASCII = !IsNativeUnicode();
            columnlist.ColumnidColumnName = MakeManagedColumnid( _columnlist.columnidcolumnname, MJET_COLTYP::Text, fASCII );
            columnlist.ColumnidMaxLength = MakeManagedColumnid( _columnlist.columnidcbMax, MJET_COLTYP::Long, false );
            columnlist.ColumnidGrbit = MakeManagedColumnid( _columnlist.columnidgrbit, MJET_COLTYP::Long, false );
            columnlist.ColumnidDefault = MakeManagedColumnid( _columnlist.columnidDefault, MJET_COLTYP::LongBinary, false );
            columnlist.ColumnidTableName = MakeManagedColumnid( _columnlist.columnidBaseTableName, MJET_COLTYP::Text, fASCII );

            return columnlist;
        }

        virtual MJET_INDEXLIST MakeManagedIndexList(MJET_SESID sesid, const ::JET_INDEXLIST& _indexlist)
        {
            MJET_INDEXLIST indexlist;
            indexlist.Tableid = MakeManagedTableid( _indexlist.tableid, sesid );
            indexlist.Records = _indexlist.cRecord;

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
            ic.Key = MakeManagedString( _indexCreate.szKey, (_indexCreate.cbKey / sizeof(_TCHAR)) - 1 );
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

        virtual DateTime MakeManagedDateTime(const ::JET_LOGTIME& _logtime)
        {
            try
            {
                System::DateTime datetime(
                    _logtime.bYear+1900,
                    (0 == _logtime.bMonth ) ? 1 : _logtime.bMonth,
                    (0 == _logtime.bDay ) ? 1 : _logtime.bDay,
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

                e->Data->Add( "diagnostics", message );
                WCHAR* wszMessage = GetUnmanagedStringW( message );
                OutputDebugStringW( wszMessage );
                FreeUnmanagedStringW( wszMessage );

                throw;
            }
        }

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
            _signature.logtimeCreate = _jtT;

            for ( int ichT = 0; ichT < JET_MAX_COMPUTERNAME_LENGTH + 1; ichT++ )
            {
                _signature.szComputerName[ichT] = 0;
            }

            return _signature;
        }

        virtual MJET_LGPOS MakeManagedLgpos(const ::JET_LGPOS& _lgpos)
        {
            MJET_LGPOS lgpos;

            lgpos.Generation = _lgpos.lGeneration;
            lgpos.Sector = _lgpos.isec;
            lgpos.ByteOffset = _lgpos.ib;
            return lgpos;
        }

        virtual JET_LGPOS MakeUnmanagedLgpos(MJET_LGPOS& lgpos)
        {
            JET_LGPOS _lgpos;

            _lgpos.lGeneration = (long) lgpos.Generation;
            _lgpos.isec = (unsigned short) lgpos.Sector;
            _lgpos.ib = (unsigned short) lgpos.ByteOffset;
            return _lgpos;
        }

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
        virtual MJET_DBINFO MakeManagedDbinfo(IntPtr _handle, unsigned int _cbconst)
        {
            if ( sizeof( JET_DBINFOMISC ) == _cbconst )
            {
                const ::JET_DBINFOMISC * const _pdbinfomisc = (JET_DBINFOMISC*) _handle.ToPointer();
                return MakeManagedDbinfo( *_pdbinfomisc );
            }

            throw TranslatedException(gcnew IsamInvalidParameterException());;
        }

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
        virtual MJET_DBINFO MakeManagedDbinfo(IntPtr _handle, unsigned int _cbconst)
        {
            if ( sizeof( JET_DBINFOMISC4 ) == _cbconst )
            {
                const ::JET_DBINFOMISC4 * const _pdbinfomisc = (JET_DBINFOMISC4*) _handle.ToPointer();
                return MakeManagedDbinfo( *_pdbinfomisc );
            }

            throw TranslatedException(gcnew IsamInvalidParameterException());;
        }

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
        virtual MJET_DBINFO MakeManagedDbinfo(IntPtr _handle, unsigned int _cbconst)
        {
            if ( sizeof( JET_DBINFOMISC7 ) == _cbconst )
            {
                const ::JET_DBINFOMISC7 * const _pdbinfomisc = (JET_DBINFOMISC7*) _handle.ToPointer();
                return MakeManagedDbinfo( *_pdbinfomisc );
            }

            throw TranslatedException(gcnew IsamInvalidParameterException());;
        }

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

        virtual MJET_INSTANCE MJetInit()
        {
            ::JET_INSTANCE _inst = ~((JET_API_PTR)0);

            Call( JetInit( &_inst ) );
            return MakeManagedInst( _inst );
        }

        virtual MJET_INSTANCE MJetInit(MJET_INSTANCE instance)
        {
            ::JET_INSTANCE _inst = GetUnmanagedInst( instance );

            Call( ::JetInit( &_inst ) );
            return MakeManagedInst( _inst );
        }

        virtual MJET_INSTANCE MJetInit(MJET_INSTANCE instance, MJET_GRBIT grbit)
        {
            ::JET_INSTANCE _inst = GetUnmanagedInst( instance );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;

            Call( ::JetInit2( &_inst, _grbit ) );
            return MakeManagedInst( _inst );
        }

#if (ESENT_WINXP)
#else
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
                    callbackInfo = gcnew MJetInitCallbackInfo( this, callback );

                    MJetInitStatusCallbackManagedDelegate ^ delegatefp = gcnew MJetInitStatusCallbackManagedDelegate(callbackInfo, &MJetInitCallbackInfo::MJetInitCallbackOwn);

                    gch = GCHandle::Alloc(delegatefp);

                    ::JET_PFNSTATUS pfn = static_cast<::JET_PFNSTATUS>( Marshal::GetFunctionPointerForDelegate( delegatefp ).ToPointer());

#if DEBUG
                    GC::Collect();
#endif

                    _rstinfo.pfnStatus = pfn;
                }

                err = ::JetInit3( &_inst, &_rstinfo, _grbit );

                if ( err == JET_errCallbackFailed )
                {

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

        virtual MJET_INSTANCE MJetInit(
            MJET_INSTANCE instance,
            array<MJET_RSTMAP>^ rstmaps,
            Int64 ,
            MJET_GRBIT grbit)
        {
            return MJetInit( instance, rstmaps, 0, nullptr, grbit );
        }

        virtual MJET_INSTANCE MJetInit(
            MJET_INSTANCE instance,
            array<MJET_RSTMAP>^ rstmaps,
            MJET_GRBIT grbit)
        {
            return MJetInit( instance, rstmaps, 0, nullptr, grbit );
        }
#endif

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

        virtual MJET_INSTANCE MJetCreateInstance(String^ name, String^ displayName)
        {
            return MJetCreateInstance( name, displayName, MJET_GRBIT::NoGrbit );
        }

        virtual void MJetTerm()
        {
            ::JET_INSTANCE _inst = ~((JET_API_PTR)0);
            ::JetTerm( _inst );
        }

        virtual void MJetTerm(MJET_INSTANCE instance)
        {
            ::JET_INSTANCE _inst = GetUnmanagedInst( instance );
            ::JetTerm( _inst );
        }

        virtual void MJetTerm(MJET_INSTANCE instance, MJET_GRBIT grbit)
        {
            ::JET_INSTANCE _inst = GetUnmanagedInst( instance );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            ::JetTerm2( _inst, _grbit );
        }

        virtual void MJetStopService()
        {
            ::JetStopService();
        }

        virtual void MJetStopServiceInstance(MJET_INSTANCE instance)
        {
            ::JET_INSTANCE _inst = GetUnmanagedInst( instance );
            ::JetStopServiceInstance( _inst );
        }

        virtual void MJetStopBackup()
        {
            ::JetStopBackup();
        }

        virtual void MJetStopBackupInstance(MJET_INSTANCE instance)
        {
            ::JET_INSTANCE _inst = GetUnmanagedInst( instance );
            ::JetStopBackupInstance( _inst );
        }

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

        virtual void MJetSetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            Int64 param)
        {
            MJET_SESID sesid = MakeManagedSesid( ~(JET_API_PTR)0, instance );
            MJetSetSystemParameter( instance, sesid, paramid, param, L"" );
        }

        virtual void MJetSetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            String^ pstr)
        {
            MJET_SESID sesid = MakeManagedSesid( ~(JET_API_PTR)0, instance );
            MJetSetSystemParameter( instance, sesid, paramid, 0, pstr );
        }

        virtual void MJetSetSystemParameter(MJET_PARAM paramid, Int64 param)
        {
            MJET_INSTANCE instance = MakeManagedInst( ~(JET_API_PTR)0 );
            MJetSetSystemParameter( instance, paramid, param );
        }

        virtual void MJetSetSystemParameter(MJET_PARAM paramid, String^ pstr)
        {
            MJET_INSTANCE instance = MakeManagedInst( ~(JET_API_PTR)0 );
            MJetSetSystemParameter( instance, paramid, pstr );
        }

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

        virtual void MJetGetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            Int64% param)
        {
            MJET_SESID sesid = MakeManagedSesid( ~(JET_API_PTR)0, instance );
            MJetGetSystemParameter( instance, sesid, paramid, param );
        }

        virtual void MJetGetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            String^% pstr)
        {
            MJET_SESID sesid = MakeManagedSesid( ~(JET_API_PTR)0, instance );
            System::Int64 param = 0;
            MJetGetSystemParameter( instance, sesid, paramid, param, pstr );
        }

        virtual void MJetGetSystemParameter(MJET_PARAM paramid, Int64% param)
        {
            MJET_INSTANCE instance = MakeManagedInst( ~(JET_API_PTR)0 );
            MJetGetSystemParameter( instance, paramid, param );
        }

        virtual void MJetGetSystemParameter(MJET_PARAM paramid, String^% pstr)
        {
            MJET_INSTANCE instance = MakeManagedInst( ~(JET_API_PTR)0 );
            MJetGetSystemParameter( instance, paramid, pstr );
        }

#if (ESENT_PUBLICONLY)
#else
#if (ESENT_WINXP)
#else
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

        

#if (ESENT_PUBLICONLY)
#else
        virtual void MJetResetCounter(MJET_SESID sesid, Int32 CounterType)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            long _type = CounterType;
            ::JetResetCounter( _sesid, _type );
        }

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

        virtual IntPtr MJetTestHookEnforceContextFail(
            IntPtr callback)
        {
            ::JET_TESTHOOKAPIHOOKING _params;
            _params.cbStruct = sizeof( _params );
            _params.pfnNew = callback.ToPointer();
            Call( ::JetTestHook( opTestHookEnforceContextFail, &_params ) );
            return IntPtr( const_cast<void *>( _params.pfnOld ) );
        }
        
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

        virtual void MJetSetNegativeTesting(
            MJET_NEGATIVE_TESTING_BITS bits)
        {
            DWORD dw = (DWORD)bits;
            Call( ::JetTestHook( opTestHookSetNegativeTesting, &dw ) );
        }

        virtual void MJetResetNegativeTesting(
            MJET_NEGATIVE_TESTING_BITS bits)
        {
            DWORD dw = (DWORD)bits;
            Call( ::JetTestHook( opTestHookResetNegativeTesting, &dw ) );
        }

#endif
#endif

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

        virtual MJET_SESID MJetBeginSession(MJET_INSTANCE instance)
        {
            return MJetBeginSession( instance, L"", L"" );
        }

        virtual MJET_SESID MJetDupSession(MJET_SESID sesid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_SESID _sesidNew;
            Call( ::JetDupSession( _sesid, &_sesidNew ) );
            return MakeManagedSesid( _sesidNew, sesid.Instance );
        }

        virtual void MJetEndSession(MJET_SESID sesid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            Call( ::JetEndSession( _sesid, _grbit ) );
        }

        virtual void MJetEndSession(MJET_SESID sesid)
        {
            MJetEndSession( sesid, MJET_GRBIT::NoGrbit );
        }

        virtual Int64 MJetGetVersion(MJET_SESID sesid)
        {
            System::Int64 version;
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            unsigned long _version;
            Call( ::JetGetVersion( _sesid, &_version ) );
            version = _version;
            return version;
        }

        virtual MJET_WRN MJetIdle(MJET_SESID sesid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;

            MJET_WRN wrn = CallW( ::JetIdle( _sesid, _grbit ) );
            return wrn;
        }

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

        virtual MJET_DBID MJetCreateDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit,
            MJET_WRN% wrn)
        {
            return MJetCreateDatabase( sesid, file, L"", grbit, wrn );
        }

        virtual MJET_DBID MJetCreateDatabase(MJET_SESID sesid, String^ file)
        {
            MJET_WRN wrn;
            return MJetCreateDatabase( sesid, file, MJET_GRBIT::NoGrbit, wrn );
        }

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

        virtual MJET_WRN MJetAttachDatabase(MJET_SESID sesid, String^ file)
        {
            return MJetAttachDatabase( sesid, file, MJET_GRBIT::NoGrbit );
        }

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

        virtual MJET_OBJECTINFO MJetGetTableInfo(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_OBJECTINFO _objectinfo;

            _objectinfo.cbStruct = sizeof( _objectinfo );

            Call( ::JetGetTableInfo( _sesid, _tableid, &_objectinfo, sizeof( _objectinfo ), JET_TblInfo ) );

            return MakeManagedObjectInfo( _objectinfo );
        }

        virtual String^ MJetGetTableInfoName(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            _TCHAR _szName[JET_cbNameMost+1];

            memset( _szName, 0, sizeof( _szName ) );

            Call( ::JetGetTableInfo( _sesid, _tableid, _szName, sizeof( _szName ), JET_TblInfoName ) );

            return MakeManagedString( _szName );
        }

        virtual int MJetGetTableInfoInitialSize(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            unsigned long _rgsize[ 2 ];

            Call( ::JetGetTableInfo( _sesid, _tableid, _rgsize, sizeof( _rgsize ), JET_TblInfoSpaceAlloc ) );

            return _rgsize[ 0 ];
        }

        virtual int MJetGetTableInfoDensity(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            unsigned long _rgsize[ 2 ];

            Call( ::JetGetTableInfo( _sesid, _tableid, _rgsize, sizeof( _rgsize ), JET_TblInfoSpaceAlloc ) );

            return _rgsize[ 1 ];
        }

        virtual String^ MJetGetTableInfoTemplateTable(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            _TCHAR _szName[JET_cbNameMost+1];

            memset( _szName, 0, sizeof( _szName ) );

            Call( ::JetGetTableInfo( _sesid, _tableid, _szName, sizeof( _szName ), JET_TblInfoTemplateTableName ) );

            return MakeManagedString( _szName );
        }

        virtual int MJetGetTableInfoOwnedPages(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            unsigned long _cpg;

            Call( ::JetGetTableInfo( _sesid, _tableid, &_cpg, sizeof( _cpg ), JET_TblInfoSpaceOwned ) );

            return _cpg;
        }

        virtual int MJetGetTableInfoAvailablePages(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            unsigned long _cpg;

            Call( ::JetGetTableInfo( _sesid, _tableid, &_cpg, sizeof( _cpg ), JET_TblInfoSpaceAvailable ) );

            return _cpg;
        }

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

        virtual MJET_OBJECTLIST MJetGetObjectInfoList(MJET_SESID sesid, MJET_DBID dbid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_DBID _dbid = GetUnmanagedDbid( dbid );
            ::JET_OBJECTLIST _objectlist;

            _objectlist.cbStruct = sizeof( _objectlist );

            Call( ::JetGetObjectInfo( _sesid, _dbid, JET_objtypNil, NULL, NULL, &_objectlist, sizeof( _objectlist ), JET_ObjInfoListNoStats ) );
            return MakeManagedObjectList( sesid, _objectlist );
        }

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

        virtual MJET_PAGEINFO MJetGetPageInfo(array<Byte>^ page, Int64 PageNumber)
        {
            void * _pvData = NULL;
            int _cbData = 0;
            ::JET_PAGEINFO2 _pageinfo;

            __int64 _pageNumber = PageNumber;
            _pageinfo.pageInfo.pgno = static_cast<unsigned long>( _pageNumber );

            try
            {
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

                String^ message = String::Format("Corrupt log file: {0}", logfile);
                throw gcnew IsamLogFileCorruptException(message, ex);
            }
        }
#endif
#endif

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

        virtual MJET_TABLEID MJetCreateTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name)
        {
            return MJetCreateTable( sesid, dbid, name, 16, 100 );
        }

#if (ESENT_WINXP)
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

        virtual MJET_COLUMNLIST MJetGetColumnInfoList(MJET_TABLEID tableid)
        {
            return MJetGetColumnInfoList( tableid, false );
        }

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

        virtual MJET_COLUMNID MJetAddColumn(
            MJET_TABLEID tableid,
            String^ name,
            MJET_COLUMNDEF% definition)
        {
            return MJetAddColumn( tableid, name, definition, nullptr );
        }

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

        virtual MJET_INDEXLIST MJetGetIndexInfoList(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_INDEXLIST _indexlist;

            _indexlist.cbStruct = sizeof( _indexlist );

            Call( ::JetGetTableIndexInfo( _sesid, _tableid, 0, &_indexlist, sizeof( _indexlist ), JET_IdxInfoList ) );
            return MakeManagedIndexList( tableid.Sesid, _indexlist );
        }

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
        virtual int MJetGetIndexInfoKeyMost(MJET_TABLEID , String^ )
        {
            return JET_cbKeyMost;
        }

        virtual int MJetGetIndexInfoKeyMost(
            MJET_SESID ,
            MJET_DBID ,
            String^ ,
            String^ )
        {
            return JET_cbKeyMost;
        }
#else
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

        virtual void MJetCreateIndex(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit,
            String^ key)
        {
            MJetCreateIndex( tableid, name, grbit, key, 100 );
        }

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
                _rgindexcreate = GetUnmanagedIndexCreates( indexcreates );
                Call( ::JetCreateIndex3( _sesid, _tableid, _rgindexcreate, _cindexcreate ) );
            }
            __finally
            {
                FreeUnmanagedIndexCreates( _rgindexcreate, _cindexcreate );
            }
        }
#endif

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

        virtual void MJetBeginTransaction(MJET_SESID sesid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            Call( ::JetBeginTransaction3( _sesid, 40997, 0 ) );
        }

        virtual void MJetBeginTransaction(MJET_SESID sesid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            Call( ::JetBeginTransaction3( _sesid, 57381, _grbit ) );
        }

        virtual void MJetBeginTransaction(
            MJET_SESID sesid,
            Int32 trxid,
            MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            Call( ::JetBeginTransaction3( _sesid, (unsigned long)trxid, _grbit ) );
        }

        virtual void MJetCommitTransaction(MJET_SESID sesid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            Call( ::JetCommitTransaction( _sesid, _grbit ) );
        }

        virtual void MJetCommitTransaction(MJET_SESID sesid)
        {
            MJetCommitTransaction( sesid, MJET_GRBIT::NoGrbit );
        }

        virtual void MJetRollback(MJET_SESID sesid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            Call( ::JetRollback( _sesid, _grbit ) );
        }

        virtual void MJetRollback(MJET_SESID sesid)
        {
            MJetRollback( sesid, MJET_GRBIT::NoGrbit );
        }

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
        virtual String^ MJetGetDatabaseInfoFilename(MJET_SESID sesid, MJET_DBID dbid)
        {
            ::JET_SESID _sesid      = GetUnmanagedSesid( sesid );
            ::JET_DBID  _dbid       = GetUnmanagedDbid( dbid );
            const int   _cwchBuffer     = 1024;
            _TCHAR      _rgwchBuffer[_cwchBuffer];

            Call( ::JetGetDatabaseInfo( _sesid, _dbid, _rgwchBuffer, sizeof( _rgwchBuffer ), JET_DbInfoFilename ) );
            return MakeManagedString( _rgwchBuffer );
        }

        virtual Int64 MJetGetDatabaseInfoFilesize(MJET_SESID sesid, MJET_DBID dbid)
        {
            ::JET_SESID _sesid      = GetUnmanagedSesid( sesid );
            ::JET_DBID  _dbid       = GetUnmanagedDbid( dbid );
            unsigned long filesize;

            Call( ::JetGetDatabaseInfo( _sesid, _dbid, &filesize, sizeof( filesize ), JET_DbInfoFilesize ) );
            return (System::Int64)filesize;
        }

#endif

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

        virtual MJET_DBID MJetOpenDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit,
            MJET_WRN% wrn)
        {
            return MJetOpenDatabase( sesid, file, L"", grbit, wrn );
        }

        virtual MJET_DBID MJetOpenDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit)
        {
            MJET_WRN wrn;
            return MJetOpenDatabase( sesid, file, L"", grbit, wrn );
        }

        virtual MJET_DBID MJetOpenDatabase(MJET_SESID sesid, String^ file)
        {
            return MJetOpenDatabase( sesid, file, MJET_GRBIT::NoGrbit );
        }

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

        virtual void MJetCloseDatabase(MJET_SESID sesid, MJET_DBID dbid)
        {
            MJetCloseDatabase( sesid, dbid, MJET_GRBIT::NoGrbit );
        }

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

        virtual MJET_TABLEID MJetOpenTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name,
            MJET_GRBIT grbit)
        {
            return MJetOpenTable( sesid, dbid, name, nullptr , grbit );
        }

        virtual MJET_TABLEID MJetOpenTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name)
        {
            return MJetOpenTable( sesid, dbid, name, MJET_GRBIT::NoGrbit );
        }

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

        virtual void MJetSetTableSequential(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            Call( ::JetSetTableSequential( _sesid, _tableid, _grbit ) );
        }

        virtual void MJetResetTableSequential(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            Call( ::JetResetTableSequential( _sesid, _tableid, _grbit ) );
        }

        virtual void MJetCloseTable(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            Call( ::JetCloseTable( _sesid, _tableid ) );
        }

        virtual void MJetDelete(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            Call( ::JetDelete( _sesid, _tableid ) );
        }

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


            if ( _cbActual == 0xDDDDDDDD )
            {
                _cbActual = 0;
            }

            return MakeManagedBytes( _rgbBookmark, _cbActual );
        }

#if (ESENT_WINXP)
#else
        virtual array<Byte>^ MJetUpdate(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;

            BYTE _rgbBookmark[JET_cbKeyMostMost];
            unsigned long _cbActual = 0xDDDDDDDD;

            Call( ::JetUpdate2( _sesid, _tableid, _rgbBookmark, sizeof( _rgbBookmark ), &_cbActual, _grbit ) );


            if ( _cbActual == 0xDDDDDDDD )
            {
                _cbActual = 0;
            }

            return MakeManagedBytes( _rgbBookmark, _cbActual );
        }
#endif

        

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
                Call( _err =::JetRetrieveColumn( _sesid, _tableid, _columnid, _pb, _cbToRetrieve, &_cbActual, _grbit, &_retinfo ) );
                if( _cbActual > _cbToRetrieve && _cbMax > _cbToRetrieve )
                {

                    _cbToRetrieve = ( _cbMax < _cbActual ) ? _cbMax : _cbActual;
                    _pb = new BYTE[_cbToRetrieve];
                    if( 0 == _pb )
                    {
                        throw TranslatedException(gcnew IsamOutOfMemoryException());
                    }
                    Call( _err = ::JetRetrieveColumn( _sesid, _tableid, _columnid, _pb, _cbToRetrieve, &_cbActual, _grbit, &_retinfo ) );
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

        virtual array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit,
            MJET_RETINFO retinfo)
        {
            return MJetRetrieveColumn( tableid, columnid, grbit, retinfo, -1 );
        }

        virtual array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit)
        {
            return MJetRetrieveColumn( tableid, columnid, grbit, -1 );
        }

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

        virtual array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            Int64 maxdata)
        {
            return MJetRetrieveColumn( tableid, columnid, MJET_GRBIT::NoGrbit, maxdata );
        }

        virtual array<Byte>^ MJetRetrieveColumn(MJET_TABLEID tableid, MJET_COLUMNID columnid)
        {
            return MJetRetrieveColumn( tableid, columnid, MJET_GRBIT::NoGrbit, -1 );
        }

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

        virtual Int64 MJetRetrieveColumnSize(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            Int64 itagSequence)
        {
            return MJetRetrieveColumnSize( tableid, columnid, itagSequence, MJET_GRBIT::NoGrbit );
        }

        virtual Int64 MJetRetrieveColumnSize(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit)
        {
            return MJetRetrieveColumnSize( tableid, columnid, 1, grbit );
        }

        virtual Int64 MJetRetrieveColumnSize(MJET_TABLEID tableid, MJET_COLUMNID columnid)
        {
            return MJetRetrieveColumnSize( tableid, columnid, 1, MJET_GRBIT::NoGrbit );
        }

        

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

        virtual array<MJET_ENUMCOLUMN>^ MJetEnumerateColumns(
            MJET_TABLEID tableid,
            array<MJET_ENUMCOLUMNID>^ columnids,
            MJET_GRBIT grbit)
        {
            MJET_WRN wrn;

            return MJetEnumerateColumns( tableid, columnids, grbit, wrn );
        }

        virtual array<MJET_ENUMCOLUMN>^ MJetEnumerateColumns(MJET_TABLEID tableid, array<MJET_ENUMCOLUMNID>^ columnids)
        {
            MJET_WRN wrn;

            return MJetEnumerateColumns( tableid, columnids, MJET_GRBIT::NoGrbit, wrn );
        }

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

        virtual MJET_WRN MJetSetColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            array<Byte>^ data)
        {
            return MJetSetColumn( tableid, columnid, data, MJET_GRBIT::NoGrbit );
        }

        

        virtual void MJetPrepareUpdate(MJET_TABLEID tableid, MJET_PREP prep)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            unsigned long _prep = ( unsigned long ) prep;
            Call( ::JetPrepareUpdate( _sesid, _tableid, _prep ) );
        }

        virtual MJET_RECPOS MJetGetRecordPosition(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_RECPOS _recpos;
            Call( ::JetGetRecordPosition( _sesid, _tableid, &_recpos, sizeof( ::JET_RECPOS ) ) );

            return MakeManagedRecpos( _recpos );
        }

        virtual void MJetGotoPosition(MJET_TABLEID tableid, MJET_RECPOS recpos)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_RECPOS _recpos = GetUnmanagedRecpos( recpos );
            Call( ::JetGotoPosition( _sesid, _tableid, &_recpos ) );
        }

        

        virtual MJET_TABLEID MJetDupCursor(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            ::JET_TABLEID _tableidNew;
            Call( ::JetDupCursor( _sesid, _tableid, &_tableidNew, _grbit ) );

            return MakeManagedTableid( _tableidNew, tableid.Sesid, tableid.Dbid );
        }

        virtual MJET_TABLEID MJetDupCursor(MJET_TABLEID tableid)
        {
            return MJetDupCursor( tableid, MJET_GRBIT::NoGrbit );
        }

        virtual String^ MJetGetCurrentIndex(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );

            _TCHAR _szName[JET_cbNameMost+1];
            Call( ::JetGetCurrentIndex( _sesid, _tableid, _szName, _countof( _szName ) ) );

            String ^ name = MakeManagedString( _szName );
            return name;
        }

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

        virtual MJET_WRN MJetMove(MJET_TABLEID tableid, Int32 rows)
        {
            return MJetMove( tableid, rows, MJET_GRBIT::NoGrbit );
        }

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

        virtual MJET_WRN MJetMove(MJET_TABLEID tableid, MJET_MOVE rows)
        {
            return MJetMove( tableid, (long)rows, MJET_GRBIT::NoGrbit );
        }

        virtual void MoveBeforeFirst(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );

            if( FCall( ::JetMove( _sesid, _tableid, JET_MoveFirst, 0 ), JET_errNoCurrentRecord ) )
            {
                FCall( ::JetMove( _sesid, _tableid, JET_MovePrevious, 0 ), JET_errNoCurrentRecord );
            }
        }

        virtual void MoveAfterLast(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );

            if( FCall( ::JetMove( _sesid, _tableid, JET_MoveLast, 0 ), JET_errNoCurrentRecord ) )
            {
                FCall( ::JetMove( _sesid, _tableid, JET_MoveNext, 0 ), JET_errNoCurrentRecord );
            }
        }

        virtual Boolean TryMoveFirst(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );

            return FCall( ::JetMove( _sesid, _tableid, JET_MoveFirst, 0 ), JET_errNoCurrentRecord );
        }

        virtual Boolean TryMoveNext(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );

            return FCall( ::JetMove( _sesid, _tableid, JET_MoveNext, 0 ), JET_errNoCurrentRecord );
        }

        virtual Boolean TryMoveLast(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );

            return FCall( ::JetMove( _sesid, _tableid, JET_MoveLast, 0 ), JET_errNoCurrentRecord );
        }

        virtual Boolean TryMovePrevious(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );

            return FCall( ::JetMove( _sesid, _tableid, JET_MovePrevious, 0 ), JET_errNoCurrentRecord );
        }

        virtual Boolean TryMove(MJET_TABLEID tableid, MJET_MOVE rows)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            long _cRow = (long)rows;

            return FCall( ::JetMove( _sesid, _tableid, _cRow, 0 ), JET_errNoCurrentRecord );
        }

        virtual Boolean TryMove(MJET_TABLEID tableid, Int32 rows)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );

            return FCall( ::JetMove( _sesid, _tableid, rows, 0 ), JET_errNoCurrentRecord );
        }

        virtual void MJetGetLock(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            Call( ::JetGetLock( _sesid, _tableid, _grbit ) );
        }

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
                _rgcbKeys = new unsigned long[_ckeys];
                if ( 0 == _rgcbKeys ) {
                    throw TranslatedException(gcnew IsamOutOfMemoryException());
                }
                _rgpvKeys = new void*[_ckeys];
                if ( 0 == _rgpvKeys ) {
                    throw TranslatedException(gcnew IsamOutOfMemoryException());
                }

                for( int _ikey = 0; _ikey < _ckeys; ++_ikey )
                {
                    _rgpvKeys[_ikey] = 0;
                    _rgcbKeys[_ikey] = 0;
                }

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

        virtual MJET_WRN MJetSeek(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            MJET_WRN wrn = CallW( ::JetSeek( _sesid, _tableid, _grbit ) );
            return wrn;
        }

        virtual Boolean TrySeek(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;

            return FCall( ::JetSeek( _sesid, _tableid, _grbit ), JET_errRecordNotFound );
        }

        virtual Boolean TrySeek(MJET_TABLEID tableid)
        {
            return TrySeek( tableid, MJET_GRBIT::SeekEQ );
        }

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

        virtual MJET_SECONDARYINDEXBOOKMARK MJetGetSecondaryIndexBookmark(MJET_TABLEID tableid)
        {
            return MJetGetSecondaryIndexBookmark( tableid, MJET_GRBIT::NoGrbit );
        }

        

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
                    callbackInfo = gcnew MJetDefragCallbackInfo(callback);

                    MJET_CALLBACK^ delegatefp = gcnew MJET_CALLBACK(callbackInfo, &MJetDefragCallbackInfo::MJetOnEndOLDCallbackOwn);

                    callbackInfo->SetGCHandle(GCHandle::Alloc(delegatefp));

                    pfn = static_cast<::JET_CALLBACK>(Marshal::GetFunctionPointerForDelegate(delegatefp).ToPointer());
                }

                wrn = CallW(::JetDefragment2(_sesid, _dbid, _szTableName, &_cPasses, &_cSeconds, pfn, _grbit));
            }
            __finally
            {
                FreeUnmanagedString( _szTableName );

                if (wrn != MJET_WRN::None && callbackInfo != nullptr)
                {
                    callbackInfo->GetGCHandle().Free();
                }
            }

            passes     = _cPasses;
            seconds    = _cSeconds;
            return wrn;
        }

        

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

        virtual void MJetSetSessionContext(MJET_SESID sesid, IntPtr context)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );
            JET_API_PTR _context = (JET_API_PTR)context.ToPointer();

            Call( ::JetSetSessionContext( _sesid, _context ) );
        }

        virtual void MJetResetSessionContext(MJET_SESID sesid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( sesid );

            Call( ::JetResetSessionContext( _sesid ) );
        }

#if (ESENT_PUBLICONLY)
#else
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

        

#if (ESENT_WINXP)
#else
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

        

        virtual void MJetSetIndexRange(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            Call( ::JetSetIndexRange( _sesid, _tableid, _grbit ) );
        }

        virtual Boolean TrySetIndexRange(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            ::JET_GRBIT _grbit = ( ::JET_GRBIT ) grbit;
            return FCall( ::JetSetIndexRange( _sesid, _tableid, _grbit ), JET_errNoCurrentRecord );
        }

        virtual void ResetIndexRange(MJET_TABLEID tableid)
        {
            ::JET_SESID _sesid = GetUnmanagedSesid( tableid.Sesid );
            ::JET_TABLEID _tableid = GetUnmanagedTableid( tableid );
            (void)FCall( ::JetSetIndexRange( _sesid, _tableid, JET_bitRangeRemove ), JET_errInvalidOperation );
        }

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
            BYTE _rgbKey[ _cbKeyMost + 1];
            unsigned long _cbActual;

            Call( ::JetRetrieveKey( _sesid, _tableid, _rgbKey, sizeof( _rgbKey ), &_cbActual, _grbit ) );

            return MakeManagedBytes( _rgbKey, _cbActual );
        }

        virtual array<Byte>^ MJetRetrieveKey(MJET_TABLEID tableid)
        {
            return MJetRetrieveKey( tableid, MJET_GRBIT::NoGrbit );
        }

        

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

        virtual Boolean IsDbgFlagDefined()
        {
#ifdef DEBUG
            return true;
#else
            return false;
#endif
        }

        virtual Boolean IsRtmFlagDefined()
        {
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
#endif

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
        virtual void MJetTracing(MJET_TRACEOP traceOp, MJET_TRACETAG traceTag, IntPtr ul)
        {
            ::JET_TRACEOP jetTraceOp = (JET_TRACEOP)traceOp;
            ::JET_TRACETAG jetTraceTag = (JET_TRACETAG)traceTag;
            ::JET_API_PTR jetUl = (JET_API_PTR)ul.ToPointer();
            Call( ::JetTracing( jetTraceOp, jetTraceTag, jetUl ) );
        }
    
        virtual void MJetTracing(MJET_TRACEOP traceOp, MJET_TRACETAG traceTag, MJET_DBID dbid)
        {
            ::JET_TRACEOP jetTraceOp = (JET_TRACEOP)traceOp;
            ::JET_TRACETAG jetTraceTag = (JET_TRACETAG)traceTag;
            ::JET_DBID jetDbid = GetUnmanagedDbid(dbid);
            Call( ::JetTracing( jetTraceOp, jetTraceTag, (Int64)jetDbid ) );
        }
#endif

    private:
        static void FreeUnmanagedRstmap(::JET_RSTMAP _rstmap)
        {
            FreeUnmanagedString( _rstmap.szDatabaseName );
            FreeUnmanagedString( _rstmap.szNewDatabaseName );
        }

        inline static char * GetUnmanagedStringA(String^ pstr)
        {
            return (char *)Marshal::StringToHGlobalAnsi(pstr).ToPointer();
        }

        inline static void FreeUnmanagedStringA(__in_z char * _sz)
        {
            System::IntPtr hGlobal( _sz );
            Marshal::FreeHGlobal( hGlobal );
        }

        inline static String^ MakeManagedStringA(__in_z const char * _sz)
        {
            return Marshal::PtrToStringAnsi( (IntPtr) const_cast<char *>(_sz) );
        }

        inline static String^ MakeManagedStringA(__in_ecount(len) const char *_sz, __in int len)
        {
            return Marshal::PtrToStringAnsi( (IntPtr) const_cast<char *>(_sz), len );
        }

        inline static wchar_t * GetUnmanagedStringW(String^ pstr)
        {
            return (wchar_t *)Marshal::StringToHGlobalUni(pstr).ToPointer();
        }

        inline static void FreeUnmanagedStringW(__in_z wchar_t * const _sz)
        {
            System::IntPtr hGlobal( _sz );
            Marshal::FreeHGlobal( hGlobal );
        }

        inline static String^ MakeManagedStringW(__in_z const wchar_t * _sz)
        {
            return Marshal::PtrToStringUni( (IntPtr) const_cast<wchar_t *>(_sz) );
        }

        inline static String^ MakeManagedStringW(__in_ecount(len) const wchar_t *_sz, __in int len)
        {
            return Marshal::PtrToStringUni( (IntPtr) const_cast<wchar_t *>(_sz), len );
        }

        static array<Byte>^ MakeManagedBytes(BYTE* _pb, int _cb)
        {
            array<System::Byte>^ bytes = gcnew array<System::Byte>(_cb);
            if ( _cb > 0 )
            {
                Marshal::Copy( (IntPtr) _pb, bytes, 0, _cb );
            }
            return bytes;
        }

        __success( *_ppv != 0 ) void GetAlignedUnmanagedBytes(
            array<Byte>^ bytes,
            __deref_out_bcount(*_pcb) void ** _ppv,
            __out int * _pcb)
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

        static void FreeAlignedUnmanagedBytes(__in void * _pv)
        {
            if ( _pv != NULL )
            {
                VirtualFree( _pv, 0, MEM_RELEASE );
            }
        }

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
                    System::IntPtr dest = (IntPtr) *_ppv;
                    Marshal::Copy( bytes, 0, dest, length );
                }
            }
        }

        static void FreeUnmanagedBytes(void * _pv)
        {
            delete[] _pv;
        }

#if (ESENT_WINXP)
#else
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

        static ::JET_UNICODEINDEX GetUnmanagedUnicodeIndex(MJET_UNICODEINDEX unicodeindex)
        {
            ::JET_UNICODEINDEX _unicodeindex = { 0 };

            _unicodeindex.lcid          = ( unsigned long )unicodeindex.Lcid;
            _unicodeindex.dwMapFlags    = ( unsigned long )unicodeindex.MapFlags;

            return _unicodeindex;
        }

        static ::JET_CONDITIONALCOLUMN GetUnmanagedConditionalColumn(MJET_CONDITIONALCOLUMN conditionalcolumn)
        {
            ::JET_CONDITIONALCOLUMN _conditionalcolumn = { 0 };

            _conditionalcolumn.cbStruct     = sizeof( _conditionalcolumn );
            _conditionalcolumn.szColumnName = GetUnmanagedString( conditionalcolumn.ColumnName );
            _conditionalcolumn.grbit        = (::JET_GRBIT) conditionalcolumn.Grbit;

            return _conditionalcolumn;
        }

        static void FreeUnmanagedConditionalColumn(::JET_CONDITIONALCOLUMN _conditionalcolumn)
        {
            FreeUnmanagedString( _conditionalcolumn.szColumnName );
        }

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
            }
            catch ( ... )
            {
                FreeUnmanagedConditionalColumns( _pconditioalcolumn, _i );
                throw;
            }
            return _pconditioalcolumn;
        }

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

#if DEBUG
        static bool _fROSpaceHints = true;
#else
        static bool _fROSpaceHints = false;
#endif

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
            }
            catch ( ... )
            {
                FreeUnmanagedIndexCreates( _pindexcreate, _i );
                throw;
            }

            return _pindexcreate;
        }

        static void FreeUnmanagedIndexCreates(::JET_INDEXCREATE* _pindexcreate, const int _cindexcreate)
        {
            for ( int _i = 0; _i < _cindexcreate; ++_i )
            {
                FreeUnmanagedIndexCreate( _pindexcreate[ _i ] );
            }
            delete[] _pindexcreate;
        }
#else
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

                if( indexcreate.ConditionalColumns )
                {
                    _indexcreate.rgconditionalcolumn = GetUnmanagedConditionalColumns( indexcreate.ConditionalColumns );
                    _indexcreate.cConditionalColumn = indexcreate.ConditionalColumns->Length;
                }

                _indexcreate.cbKeyMost = indexcreate.KeyLengthMax;

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

        static void FreeUnmanagedIndexCreate(__in __drv_freesMem(object) ::JET_INDEXCREATE2 _indexcreate)
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
            }
            catch ( ... )
            {
                FreeUnmanagedIndexCreates( _pindexcreate, _i );
                throw;
            }

            return _pindexcreate;
        }

        static void FreeUnmanagedIndexCreates(::JET_INDEXCREATE2* _pindexcreate, const int _cindexcreate)
        {
            for ( int _i = 0; _i < _cindexcreate; ++_i )
            {
                FreeUnmanagedIndexCreate( _pindexcreate[ _i ] );
            }
            delete[] _pindexcreate;
        }
#endif

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

        static void FreeUnmanagedColumnCreate(::JET_COLUMNCREATE _columncreate)
        {
            FreeUnmanagedString( _columncreate.szColumnName );
            FreeUnmanagedBytes( _columncreate.pvDefault );
        }

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
            }
            catch ( ... )
            {
                FreeUnmanagedColumnCreates( _pcolumncreate, _i );
                throw;
            }

            return _pcolumncreate;
        }

        static void FreeUnmanagedColumnCreates(::JET_COLUMNCREATE* _pcolumncreate, const int _ccolumncreate)
        {
            for ( int _i = 0; _i < _ccolumncreate; ++_i )
            {
                FreeUnmanagedColumnCreate( _pcolumncreate[ _i ] );
            }
            delete[] _pcolumncreate;
        }

#if (ESENT_WINXP)
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

                if( tablecreate.ColumnCreates )
                {
                    _tablecreate.rgcolumncreate = GetUnmanagedColumnCreates( tablecreate.ColumnCreates );
                    _tablecreate.cColumns = tablecreate.ColumnCreates->Length;
                }

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

        static void FreeUnmanagedTablecreate(::JET_TABLECREATE2 _tablecreate)
        {
            FreeUnmanagedString( _tablecreate.szTableName );
            FreeUnmanagedString( _tablecreate.szTemplateTableName );

            FreeUnmanagedColumnCreates( _tablecreate.rgcolumncreate, _tablecreate.cColumns );
            FreeUnmanagedIndexCreates( _tablecreate.rgindexcreate, _tablecreate.cIndexes );
        }
#else
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

                if( tablecreate.ColumnCreates )
                {
                    _tablecreate.rgcolumncreate = GetUnmanagedColumnCreates( tablecreate.ColumnCreates );
                    _tablecreate.cColumns = tablecreate.ColumnCreates->Length;
                }

                if( tablecreate.IndexCreates )
                {
                    _tablecreate.rgindexcreate = GetUnmanagedIndexCreates( tablecreate.IndexCreates );
                    _tablecreate.cIndexes = tablecreate.IndexCreates->Length;
                }

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

        static void FreeUnmanagedDbutil(::JET_DBUTIL _dbutil)
        {
            FreeUnmanagedString( _dbutil.szDatabase );
            FreeUnmanagedString( _dbutil.szBackup );
            FreeUnmanagedString( (_TCHAR *)_dbutil.szTable );
            FreeUnmanagedString( (_TCHAR *)_dbutil.szIndex );
            FreeUnmanagedString( _dbutil.szIntegPrefix );
        }


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

        static unsigned long CalculateHeaderChecksum(__in_bcount(cbHeader) const void * const pvHeader, __in const int cbHeader)
        {
            const unsigned long * pul = reinterpret_cast<const unsigned long *>(pvHeader);
            int cbT = cbHeader;


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

        static JET_ERR ErrFromWin32Err(__in const DWORD dwWinError, __in const JET_ERR errDefault)
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

        static JET_ERR ErrReadFileHeader(
            __in const wchar_t * const szFile,
            __out_bcount_full(cbHeader) void * const pvHeader,
            const DWORD cbHeader,
            const __int64 ibOffset)
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

        static JET_ERR ErrVerifyCheckpoint(__in_bcount(cbHeader) const void * const pvHeader, __in const int cbHeader)
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

        static JET_ERR ErrGetCheckpointGeneration(__in const wchar_t * const szCheckpoint, __out unsigned long * pulGeneration)
        {
            JET_ERR err = JET_errInternalError;

            const int cbHeader = 4096;
            void * const pvHeader = VirtualAlloc( 0, cbHeader, MEM_COMMIT, PAGE_READWRITE );
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

        void Call(const JET_ERR errFn)
        {
            if (errFn < JET_errSuccess )
            {
                HandleError(errFn);
            }
        };

        MJET_WRN CallW(const JET_ERR errFn)
        {
            if ( errFn < JET_errSuccess )
            {
                HandleError(errFn);
            }

            MJET_WRN wrn = (MJET_WRN)errFn;
            return wrn;
        };

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

        Exception^ TranslatedException(IsamErrorException^ exception)
        {
            Exception^ translatedException = this->m_exceptionTranslator(exception);
            return translatedException == nullptr ? exception : translatedException;
        }

        static Exception^ DoNothingTranslator(IsamErrorException^ exception)
        {
            return exception;
        }

        ExceptionTranslator^ m_exceptionTranslator;
    };

    MSINTERNAL ref class Interop
    {
    private:
        static IJetInterop^ defaultInstance;

        static IJetInterop^ activeInstance;

        static Interop()
        {
            Interop::defaultInstance = gcnew JetInterop();
            Interop::activeInstance = Interop::defaultInstance;
        }

        static property IJetInterop^ Instance
        {
            IJetInterop^ get()
            {
                return Interop::activeInstance;
            }
        }

        ref class ResetTestHook
        {
            IJetInterop^ oldInstance;
            Boolean disposed;

        public:
            ResetTestHook(IJetInterop^ instance)
                : oldInstance(instance)
            {
            }

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
        static void SetExceptionTranslator(ExceptionTranslator^ translator)
        {
            Instance->SetExceptionTranslator(translator);
        }

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
        static MJET_SESID MakeManagedSesid(::JET_SESID _sesid, MJET_INSTANCE instance)
        {
            return Instance->MakeManagedSesid(_sesid, instance);
        }

        static MJET_DBID MakeManagedDbid(::JET_DBID _dbid)
        {
            return Instance->MakeManagedDbid(_dbid);
        }

        static MJET_TABLEID MakeManagedTableid(::JET_TABLEID _tableid, MJET_SESID sesid, MJET_DBID dbid)
        {
            return Instance->MakeManagedTableid(_tableid, sesid, dbid);
        }

        static MJET_DBINFO MakeManagedDbinfo(IntPtr _handle, unsigned int _cbconst)
        {
            return Instance->MakeManagedDbinfo(_handle, _cbconst);
        }

        static MJET_DBINFO MakeManagedDbinfo(const ::JET_DBINFOMISC& _dbinfomisc)
        {
            return Instance->MakeManagedDbinfo(_dbinfomisc);
        }
#else
#if (ESENT_PUBLICONLY)
        static MJET_SESID MakeManagedSesid(::JET_SESID _sesid, MJET_INSTANCE instance)
        {
            return Instance->MakeManagedSesid(_sesid, instance);
        }

        static MJET_DBID MakeManagedDbid(::JET_DBID _dbid)
        {
            return Instance->MakeManagedDbid(_dbid);
        }

        static MJET_TABLEID MakeManagedTableid(::JET_TABLEID _tableid, MJET_SESID sesid, MJET_DBID dbid)
        {
            return Instance->MakeManagedTableid(_tableid, sesid, dbid);
        }

        static MJET_DBINFO MakeManagedDbinfo(IntPtr _handle, unsigned int _cbconst)
        {
            return Instance->MakeManagedDbinfo(_handle, _cbconst);
        }

        static MJET_DBINFO MakeManagedDbinfo(const ::JET_DBINFOMISC4& _dbinfomisc)
        {
            return Instance->MakeManagedDbinfo(_dbinfomisc);
        }
#else
        static MJET_SESID MakeManagedSesid(::JET_SESID _sesid, MJET_INSTANCE instance)
        {
            return Instance->MakeManagedSesid(_sesid, instance);
        }

        static MJET_DBID MakeManagedDbid(::JET_DBID _dbid)
        {
            return Instance->MakeManagedDbid(_dbid);
        }

        static MJET_TABLEID MakeManagedTableid(::JET_TABLEID _tableid, MJET_SESID sesid, MJET_DBID dbid)
        {
            return Instance->MakeManagedTableid(_tableid, sesid, dbid);
        }

        static MJET_DBINFO MakeManagedDbinfo(IntPtr _handle, unsigned int _cbconst)
        {
            return Instance->MakeManagedDbinfo(_handle, _cbconst);
        }

        static MJET_DBINFO MakeManagedDbinfo(const ::JET_DBINFOMISC7& _dbinfomisc)
        {
            return Instance->MakeManagedDbinfo(_dbinfomisc);
        }

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

        static MJET_INSTANCE MJetInit()
        {
            return Instance->MJetInit();
        }

        static MJET_INSTANCE MJetInit(MJET_INSTANCE instance)
        {
            return Instance->MJetInit(instance);
        }

        static MJET_INSTANCE MJetInit(MJET_INSTANCE instance, MJET_GRBIT grbit)
        {
            return Instance->MJetInit(instance, grbit);
        }

#if (ESENT_WINXP)
#else
#ifdef INTERNALUSE
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

        static MJET_INSTANCE MJetInit(
            MJET_INSTANCE instance,
            array<MJET_RSTMAP>^ rstmaps,
            Int64 genStop,
            MJET_GRBIT grbit)
        {
            return Instance->MJetInit(instance, rstmaps, genStop, grbit);
        }

        static MJET_INSTANCE MJetInit(
            MJET_INSTANCE instance,
            array<MJET_RSTMAP>^ rstmaps,
            MJET_GRBIT grbit)
        {
            return Instance->MJetInit(instance, rstmaps, grbit);
        }
#endif

        static MJET_INSTANCE MJetCreateInstance(String^ name)
        {
            return Instance->MJetCreateInstance(name);
        }

        static MJET_INSTANCE MJetCreateInstance(
            String^ name,
            String^ displayName,
            MJET_GRBIT grbit)
        {
            return Instance->MJetCreateInstance(name, displayName, grbit);
        }

        static MJET_INSTANCE MJetCreateInstance(String^ name, String^ displayName)
        {
            return Instance->MJetCreateInstance(name, displayName);
        }

        static void MJetTerm()
        {
            Instance->MJetTerm();
        }

        static void MJetTerm(MJET_INSTANCE instance)
        {
            Instance->MJetTerm(instance);
        }

        static void MJetTerm(MJET_INSTANCE instance, MJET_GRBIT grbit)
        {
            Instance->MJetTerm(instance, grbit);
        }

        static void MJetStopService()
        {
            Instance->MJetStopService();
        }

        static void MJetStopServiceInstance(MJET_INSTANCE instance)
        {
            Instance->MJetStopServiceInstance(instance);
        }

        static void MJetStopBackup()
        {
            Instance->MJetStopBackup();
        }

        static void MJetStopBackupInstance(MJET_INSTANCE instance)
        {
            Instance->MJetStopBackupInstance(instance);
        }

        static void MJetSetSystemParameter(
            MJET_INSTANCE instance,
            MJET_SESID sesid,
            MJET_PARAM paramid,
            Int64 param,
            String^ pstr)
        {
            Instance->MJetSetSystemParameter(instance, sesid, paramid, param, pstr);
        }

        static void MJetSetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            Int64 param)
        {
            Instance->MJetSetSystemParameter(instance, paramid, param);
        }

        static void MJetSetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            String^ pstr)
        {
            Instance->MJetSetSystemParameter(instance, paramid, pstr);
        }

        static void MJetSetSystemParameter(MJET_PARAM paramid, Int64 param)
        {
            Instance->MJetSetSystemParameter(paramid, param);
        }

        static void MJetSetSystemParameter(MJET_PARAM paramid, String^ pstr)
        {
            Instance->MJetSetSystemParameter(paramid, pstr);
        }

        static void MJetGetSystemParameter(
            MJET_INSTANCE instance,
            MJET_SESID sesid,
            MJET_PARAM paramid,
            Int64% param,
            String^% pstr)
        {
            Instance->MJetGetSystemParameter(instance, sesid, paramid, param, pstr);
        }

        static void MJetGetSystemParameter(
            MJET_INSTANCE instance,
            MJET_SESID sesid,
            MJET_PARAM paramid,
            Int64% param)
        {
            Instance->MJetGetSystemParameter(instance, sesid, paramid, param);
        }

        static void MJetGetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            Int64% param)
        {
            Instance->MJetGetSystemParameter(instance, paramid, param);
        }

        static void MJetGetSystemParameter(
            MJET_INSTANCE instance,
            MJET_PARAM paramid,
            String^% pstr)
        {
            Instance->MJetGetSystemParameter(instance, paramid, pstr);
        }

        static void MJetGetSystemParameter(MJET_PARAM paramid, Int64% param)
        {
            Instance->MJetGetSystemParameter(paramid, param);
        }

        static void MJetGetSystemParameter(MJET_PARAM paramid, String^% pstr)
        {
            Instance->MJetGetSystemParameter(paramid, pstr);
        }

#if (ESENT_PUBLICONLY)
#else
#if (ESENT_WINXP)
#else
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

        

#if (ESENT_PUBLICONLY)
#else
        static void MJetResetCounter(MJET_SESID sesid, Int32 CounterType)
        {
            Instance->MJetResetCounter(sesid, CounterType);
        }

        static Int32 MJetGetCounter(MJET_SESID sesid, Int32 CounterType)
        {
            return Instance->MJetGetCounter(sesid, CounterType);
        }
#endif

#if (ESENT_PUBLICONLY)
#else
#if (ESENT_WINXP)
#else
        static void MJetTestHookEnableFaultInjection(
            Int32 id,
            JET_ERR err,
            Int32 ulProbability,
            MJET_GRBIT grbit)
        {
            Instance->MJetTestHookEnableFaultInjection(id, err, ulProbability, grbit);
        }

        static void MJetTestHookEnableConfigOverrideInjection(
            Int32 id,
            Int64 ulOverride,
            Int32 ulProbability,
            MJET_GRBIT grbit)
        {
            Instance->MJetTestHookEnableConfigOverrideInjection(id, ulOverride, ulProbability, grbit);
        }

        static IntPtr MJetTestHookEnforceContextFail(
            IntPtr callback)
        {
            return Instance->MJetTestHookEnforceContextFail(callback);
        }

        static void MJetTestHookEvictCachedPage(
            MJET_DBID dbid,
            Int32 pgno)
        {
            Instance->MJetTestHookEvictCachedPage(dbid, pgno);
        }

        static void MJetSetNegativeTesting(
            MJET_NEGATIVE_TESTING_BITS bits)
        {
            Instance->MJetSetNegativeTesting( bits );
        }

        static void MJetResetNegativeTesting(
            MJET_NEGATIVE_TESTING_BITS bits)
        {
            Instance->MJetResetNegativeTesting( bits );
        }

#endif
#endif

        static MJET_SESID MJetBeginSession(
            MJET_INSTANCE instance,
            String^ user,
            String^ password)
        {
            return Instance->MJetBeginSession(instance, user, password);
        }

        static MJET_SESID MJetBeginSession(MJET_INSTANCE instance)
        {
            return Instance->MJetBeginSession(instance);
        }

        static MJET_SESID MJetDupSession(MJET_SESID sesid)
        {
            return Instance->MJetDupSession(sesid);
        }

        static void MJetEndSession(MJET_SESID sesid, MJET_GRBIT grbit)
        {
            Instance->MJetEndSession(sesid, grbit);
        }

        static void MJetEndSession(MJET_SESID sesid)
        {
            Instance->MJetEndSession(sesid);
        }

        static Int64 MJetGetVersion(MJET_SESID sesid)
        {
            return Instance->MJetGetVersion(sesid);
        }

        static MJET_WRN MJetIdle(MJET_SESID sesid, MJET_GRBIT grbit)
        {
            return Instance->MJetIdle(sesid, grbit);
        }

        static MJET_DBID MJetCreateDatabase(
            MJET_SESID sesid,
            String^ file,
            String^ connect,
            MJET_GRBIT grbit,
            MJET_WRN% wrn)
        {
            return Instance->MJetCreateDatabase(sesid, file, connect, grbit, wrn);
        }

        static MJET_DBID MJetCreateDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit,
            MJET_WRN% wrn)
        {
            return Instance->MJetCreateDatabase(sesid, file, grbit, wrn);
        }

        static MJET_DBID MJetCreateDatabase(MJET_SESID sesid, String^ file)
        {
            return Instance->MJetCreateDatabase(sesid, file);
        }

        static MJET_DBID MJetCreateDatabase(
            MJET_SESID sesid,
            String^ file,
            Int64 maxPages,
            MJET_GRBIT grbit,
            MJET_WRN% wrn)
        {
            return Instance->MJetCreateDatabase(sesid, file, maxPages, grbit, wrn);
        }

        static MJET_WRN MJetAttachDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit)
        {
            return Instance->MJetAttachDatabase(sesid, file, grbit);
        }

        static MJET_WRN MJetAttachDatabase(MJET_SESID sesid, String^ file)
        {
            return Instance->MJetAttachDatabase(sesid, file);
        }

        static MJET_WRN MJetAttachDatabase(
            MJET_SESID sesid,
            String^ file,
            Int64 maxPages,
            MJET_GRBIT grbit)
        {
            return Instance->MJetAttachDatabase(sesid, file, maxPages, grbit);
        }

        static MJET_WRN MJetDetachDatabase(MJET_SESID sesid, String^ file)
        {
            return Instance->MJetDetachDatabase(sesid, file);
        }

        static MJET_WRN MJetDetachDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit)
        {
            return Instance->MJetDetachDatabase(sesid, file, grbit);
        }

        static MJET_OBJECTINFO MJetGetTableInfo(MJET_TABLEID tableid)
        {
            return Instance->MJetGetTableInfo(tableid);
        }

        static String^ MJetGetTableInfoName(MJET_TABLEID tableid)
        {
            return Instance->MJetGetTableInfoName(tableid);
        }

        static int MJetGetTableInfoInitialSize(MJET_TABLEID tableid)
        {
            return Instance->MJetGetTableInfoInitialSize(tableid);
        }

        static int MJetGetTableInfoDensity(MJET_TABLEID tableid)
        {
            return Instance->MJetGetTableInfoDensity(tableid);
        }

        static String^ MJetGetTableInfoTemplateTable(MJET_TABLEID tableid)
        {
            return Instance->MJetGetTableInfoTemplateTable(tableid);
        }

        static int MJetGetTableInfoOwnedPages(MJET_TABLEID tableid)
        {
            return Instance->MJetGetTableInfoOwnedPages(tableid);
        }

        static int MJetGetTableInfoAvailablePages(MJET_TABLEID tableid)
        {
            return Instance->MJetGetTableInfoAvailablePages(tableid);
        }

        static MJET_OBJECTINFO MJetGetObjectInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ object)
        {
            return Instance->MJetGetObjectInfo(sesid, dbid, object);
        }

        static MJET_OBJECTINFO MJetGetObjectInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_OBJTYP objtyp,
            String^ container,
            String^ object)
        {
            return Instance->MJetGetObjectInfo(sesid, dbid, objtyp, container, object);
        }

        static MJET_OBJECTLIST MJetGetObjectInfoList(MJET_SESID sesid, MJET_DBID dbid)
        {
            return Instance->MJetGetObjectInfoList(sesid, dbid);
        }

        static MJET_OBJECTLIST MJetGetObjectInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_OBJTYP objtyp)
        {
            return Instance->MJetGetObjectInfoList(sesid, dbid, objtyp);
        }

        static MJET_OBJECTLIST MJetGetObjectInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_OBJTYP objtyp,
            String^ container)
        {
            return Instance->MJetGetObjectInfoList(sesid, dbid, objtyp, container);
        }

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
        static array<MJET_INSTANCE_INFO>^ MJetGetInstanceInfo()
        {
            return Instance->MJetGetInstanceInfo();
        }

        static MJET_PAGEINFO MJetGetPageInfo(array<Byte>^ page, Int64 PageNumber)
        {
            return Instance->MJetGetPageInfo(page, PageNumber);
        }

        static MJET_LOGINFOMISC MJetGetLogFileInfo(String^ logfile)
        {
            return Instance->MJetGetLogFileInfo(logfile);
        }
#endif
#endif

        static Int64 MJetGetCheckpointFileInfo(String^ checkpoint)
        {
            return Instance->MJetGetCheckpointFileInfo(checkpoint);
        }

#if (ESENT_WINXP)
        static MJET_DBINFO MJetGetDatabaseFileInfo(String^ database)
        {
            return Instance->MJetGetDatabaseFileInfo(database);
        }
#else
#if (ESENT_PUBLICONLY)
        static MJET_DBINFO MJetGetDatabaseFileInfo(String^ database)
        {
            return Instance->MJetGetDatabaseFileInfo(database);
        }
#else
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
        static array<Byte>^ MJetGetDatabasePages(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pgnoStart,
            Int64 cpg,
            MJET_GRBIT grbit)
        {
            return Instance->MJetGetDatabasePages(sesid, dbid, pgnoStart, cpg, grbit);
        }

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

        static void MJetBeginDatabaseIncrementalReseed(
            MJET_INSTANCE instance,
            String^ pstrDatabase,
            MJET_GRBIT grbit)
        {
            Instance->MJetBeginDatabaseIncrementalReseed(instance, pstrDatabase, grbit);
        }

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

        static MJET_TABLEID MJetCreateTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name,
            Int32 pages,
            Int32 density)
        {
            return Instance->MJetCreateTable(sesid, dbid, name, pages, density);
        }

        static MJET_TABLEID MJetCreateTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name)
        {
            return Instance->MJetCreateTable(sesid, dbid, name);
        }

#if (ESENT_WINXP)
        static void MJetCreateTableColumnIndex(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_TABLECREATE tablecreate)
        {
            Instance->MJetCreateTableColumnIndex(sesid, dbid, tablecreate);
        }
#else
        static void MJetCreateTableColumnIndex(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_TABLECREATE tablecreate)
        {
            Instance->MJetCreateTableColumnIndex(sesid, dbid, tablecreate);
        }
#endif

        static void MJetDeleteTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name)
        {
            Instance->MJetDeleteTable(sesid, dbid, name);
        }

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
        static MJET_COLUMNBASE MJetGetColumnInfo(MJET_TABLEID tableid, Int64 colid)
        {
            return Instance->MJetGetColumnInfo(tableid, colid);
        }
#endif

        static MJET_COLUMNBASE MJetGetColumnInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            Int64 colid)
        {
            return Instance->MJetGetColumnInfo(sesid, dbid, table, colid);
        }

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
        static MJET_COLUMNBASE MJetGetColumnInfo(MJET_TABLEID tableid, MJET_COLUMNID columnid)
        {
            return Instance->MJetGetColumnInfo(tableid, columnid);
        }
#endif

        static MJET_COLUMNBASE MJetGetColumnInfo(MJET_TABLEID tableid, String^ column)
        {
            return Instance->MJetGetColumnInfo(tableid, column);
        }

        static MJET_COLUMNBASE MJetGetColumnInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ column)
        {
            return Instance->MJetGetColumnInfo(sesid, dbid, table, column);
        }

        static MJET_COLUMNLIST MJetGetColumnInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            Boolean sortByColumnid)
        {
            return Instance->MJetGetColumnInfoList(sesid, dbid, table, sortByColumnid);
        }

        static MJET_COLUMNLIST MJetGetColumnInfoList(MJET_TABLEID tableid, Boolean sortByColumnid)
        {
            return Instance->MJetGetColumnInfoList(tableid, sortByColumnid);
        }

        static MJET_COLUMNLIST MJetGetColumnInfoList(MJET_TABLEID tableid)
        {
            return Instance->MJetGetColumnInfoList(tableid);
        }

        static MJET_COLUMNID MJetAddColumn(
            MJET_TABLEID tableid,
            String^ name,
            MJET_COLUMNDEF% definition,
            array<Byte>^ defaultValue)
        {
            return Instance->MJetAddColumn(tableid, name, definition, defaultValue);
        }

        static MJET_COLUMNID MJetAddColumn(
            MJET_TABLEID tableid,
            String^ name,
            MJET_COLUMNDEF% definition)
        {
            return Instance->MJetAddColumn(tableid, name, definition);
        }

        static void MJetDeleteColumn(MJET_TABLEID tableid, String^ name)
        {
            Instance->MJetDeleteColumn(tableid, name);
        }

        static void MJetDeleteColumn(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit)
        {
            Instance->MJetDeleteColumn(tableid, name, grbit);
        }

        static void MJetRenameColumn(
            MJET_TABLEID tableid,
            String^ oldName,
            String^ newName,
            MJET_GRBIT grbit)
        {
            Instance->MJetRenameColumn(tableid, oldName, newName, grbit);
        }

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

        static MJET_INDEXLIST MJetGetIndexInfo(MJET_TABLEID tableid, String^ index)
        {
            return Instance->MJetGetIndexInfo(tableid, index);
        }

        static MJET_INDEXLIST MJetGetIndexInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index)
        {
            return Instance->MJetGetIndexInfo(sesid, dbid, table, index);
        }

        static MJET_INDEXLIST MJetGetIndexInfoList(MJET_TABLEID tableid)
        {
            return Instance->MJetGetIndexInfoList(tableid);
        }

        static MJET_INDEXLIST MJetGetIndexInfoList(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table)
        {
            return Instance->MJetGetIndexInfoList(sesid, dbid, table);
        }

        static int MJetGetIndexInfoDensity(MJET_TABLEID tableid, String^ index)
        {
            return Instance->MJetGetIndexInfoDensity(tableid, index);
        }

        static int MJetGetIndexInfoDensity(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index)
        {
            return Instance->MJetGetIndexInfoDensity(sesid, dbid, table, index);
        }

        static int MJetGetIndexInfoLCID(MJET_TABLEID tableid, String^ index)
        {
            return Instance->MJetGetIndexInfoLCID(tableid, index);
        }

        static int MJetGetIndexInfoLCID(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index)
        {
            return Instance->MJetGetIndexInfoLCID(sesid, dbid, table, index);
        }

#if (ESENT_WINXP)
        static int MJetGetIndexInfoKeyMost(MJET_TABLEID tableid, String^ index)
        {
            return Instance->MJetGetIndexInfoKeyMost(tableid, index);
        }

        static int MJetGetIndexInfoKeyMost(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index)
        {
            return Instance->MJetGetIndexInfoKeyMost(sesid, dbid, table, index);
        }
#else
        static int MJetGetIndexInfoKeyMost(MJET_TABLEID tableid, String^ index)
        {
            return Instance->MJetGetIndexInfoKeyMost(tableid, index);
        }

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
        static MJET_INDEXCREATE MJetGetIndexInfoCreate(MJET_TABLEID tableid, String^ index)
        {
            return Instance->MJetGetIndexInfoCreate(tableid, index);
        }

        static MJET_INDEXCREATE MJetGetIndexInfoCreate(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ table,
            String^ index)
        {
            return Instance->MJetGetIndexInfoCreate(sesid, dbid, table, index);
        }
#endif

        static void MJetCreateIndex(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit,
            String^ key,
            Int32 density)
        {
            Instance->MJetCreateIndex(tableid, name, grbit, key, density);
        }

        static void MJetCreateIndex(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit,
            String^ key)
        {
            Instance->MJetCreateIndex(tableid, name, grbit, key);
        }

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

        static void MJetDeleteIndex(MJET_TABLEID tableid, String^ name)
        {
            Instance->MJetDeleteIndex(tableid, name);
        }

        static void MJetBeginTransaction(MJET_SESID sesid)
        {
            Instance->MJetBeginTransaction(sesid);
        }

        static void MJetBeginTransaction(MJET_SESID sesid, MJET_GRBIT grbit)
        {
            Instance->MJetBeginTransaction(sesid, grbit);
        }

        static void MJetBeginTransaction(
            MJET_SESID sesid,
            Int32 trxid,
            MJET_GRBIT grbit)
        {
            Instance->MJetBeginTransaction(sesid, trxid, grbit);
        }

        static void MJetCommitTransaction(MJET_SESID sesid, MJET_GRBIT grbit)
        {
            Instance->MJetCommitTransaction(sesid, grbit);
        }

        static void MJetCommitTransaction(MJET_SESID sesid)
        {
            Instance->MJetCommitTransaction(sesid);
        }

        static void MJetRollback(MJET_SESID sesid, MJET_GRBIT grbit)
        {
            Instance->MJetRollback(sesid, grbit);
        }

        static void MJetRollback(MJET_SESID sesid)
        {
            Instance->MJetRollback(sesid);
        }

        static void MJetGetDatabaseInfo(
            MJET_SESID sesid,
            MJET_DBID dbid,
            array<Byte>^ resultBuffer,
            MJET_DBINFO_LEVEL InfoLevel)
        {
            Instance->MJetGetDatabaseInfo(sesid, dbid, resultBuffer, InfoLevel);
        }

#if (ESENT_WINXP)
        static MJET_DBINFO MJetGetDatabaseInfo(
            MJET_SESID sesid,
            MJET_DBID dbid)
        {
            return Instance->MJetGetDatabaseInfo(sesid, dbid);
        }
#else
#if (ESENT_PUBLICONLY)
        static MJET_DBINFO MJetGetDatabaseInfo(
            MJET_SESID sesid,
            MJET_DBID dbid)
        {
            return Instance->MJetGetDatabaseInfo(sesid, dbid);
        }
#else
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
        static String^ MJetGetDatabaseInfoFilename(
            MJET_SESID sesid,
            MJET_DBID dbid)
        {
            return Instance->MJetGetDatabaseInfoFilename(sesid, dbid);
        }

        static Int64 MJetGetDatabaseInfoFilesize(
            MJET_SESID sesid,
            MJET_DBID dbid)
        {
            return Instance->MJetGetDatabaseInfoFilesize(sesid, dbid);
        }
#endif

        static MJET_DBID MJetOpenDatabase(
            MJET_SESID sesid,
            String^ file,
            String^ connect,
            MJET_GRBIT grbit,
            MJET_WRN% wrn)
        {
            return Instance->MJetOpenDatabase(sesid, file, connect, grbit, wrn);
        }

        static MJET_DBID MJetOpenDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit,
            MJET_WRN% wrn)
        {
            return Instance->MJetOpenDatabase(sesid, file, grbit, wrn);
        }

        static MJET_DBID MJetOpenDatabase(
            MJET_SESID sesid,
            String^ file,
            MJET_GRBIT grbit)
        {
            return Instance->MJetOpenDatabase(sesid, file, grbit);
        }

        static MJET_DBID MJetOpenDatabase(MJET_SESID sesid, String^ file)
        {
            return Instance->MJetOpenDatabase(sesid, file);
        }

        static void MJetCloseDatabase(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_GRBIT grbit)
        {
            Instance->MJetCloseDatabase(sesid, dbid, grbit);
        }

        static void MJetCloseDatabase(MJET_SESID sesid, MJET_DBID dbid)
        {
            Instance->MJetCloseDatabase(sesid, dbid);
        }

        static MJET_TABLEID MJetOpenTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name,
            array<Byte>^ parameters,
            MJET_GRBIT grbit)
        {
            return Instance->MJetOpenTable(sesid, dbid, name, parameters, grbit);
        }

        static MJET_TABLEID MJetOpenTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name,
            MJET_GRBIT grbit)
        {
            return Instance->MJetOpenTable(sesid, dbid, name, grbit);
        }

        static MJET_TABLEID MJetOpenTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name)
        {
            return Instance->MJetOpenTable(sesid, dbid, name);
        }

        static Boolean TryOpenTable(
            MJET_SESID sesid,
            MJET_DBID dbid,
            String^ name,
            [System::Runtime::InteropServices::OutAttribute]
            MJET_TABLEID% tableid)
        {
            return Instance->TryOpenTable(sesid, dbid, name, tableid);
        }

        static void MJetSetTableSequential(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            Instance->MJetSetTableSequential(tableid, grbit);
        }

        static void MJetResetTableSequential(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            Instance->MJetResetTableSequential(tableid, grbit);
        }

        static void MJetCloseTable(MJET_TABLEID tableid)
        {
            Instance->MJetCloseTable(tableid);
        }

        static void MJetDelete(MJET_TABLEID tableid)
        {
            Instance->MJetDelete(tableid);
        }

        static array<Byte>^ MJetUpdate(MJET_TABLEID tableid)
        {
            return Instance->MJetUpdate(tableid);
        }

#if (ESENT_WINXP)
#else
        static array<Byte>^ MJetUpdate(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            return Instance->MJetUpdate(tableid, grbit);
        }
#endif

        

        static Nullable<Int64> MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit,
            MJET_RETINFO retinfo,
            ArraySegment<Byte> buffer)
        {
            return Instance->MJetRetrieveColumn(tableid, columnid, grbit, retinfo, buffer);
        }

        static array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit,
            MJET_RETINFO retinfo,
            Int64 maxdata)
        {
            return Instance->MJetRetrieveColumn(tableid, columnid, grbit, retinfo, maxdata);
        }

        static array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit,
            MJET_RETINFO retinfo)
        {
            return Instance->MJetRetrieveColumn(tableid, columnid, grbit, retinfo);
        }

        static array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit)
        {
            return Instance->MJetRetrieveColumn(tableid, columnid, grbit);
        }

        static array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit,
            Int64 maxdata)
        {
            return Instance->MJetRetrieveColumn(tableid, columnid, grbit, maxdata);
        }

        static array<Byte>^ MJetRetrieveColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            Int64 maxdata)
        {
            return Instance->MJetRetrieveColumn(tableid, columnid, maxdata);
        }

        static array<Byte>^ MJetRetrieveColumn(MJET_TABLEID tableid, MJET_COLUMNID columnid)
        {
            return Instance->MJetRetrieveColumn(tableid, columnid);
        }

        static Int64 MJetRetrieveColumnSize(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            Int64 itagSequence,
            MJET_GRBIT grbit)
        {
            return Instance->MJetRetrieveColumnSize(tableid, columnid, itagSequence, grbit);
        }

        static Int64 MJetRetrieveColumnSize(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            Int64 itagSequence)
        {
            return Instance->MJetRetrieveColumnSize(tableid, columnid, itagSequence);
        }

        static Int64 MJetRetrieveColumnSize(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            MJET_GRBIT grbit)
        {
            return Instance->MJetRetrieveColumnSize(tableid, columnid, grbit);
        }

        static Int64 MJetRetrieveColumnSize(MJET_TABLEID tableid, MJET_COLUMNID columnid)
        {
            return Instance->MJetRetrieveColumnSize(tableid, columnid);
        }

        

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

        static array<MJET_ENUMCOLUMN>^ MJetEnumerateColumns(
            MJET_TABLEID tableid,
            array<MJET_ENUMCOLUMNID>^ columnids,
            MJET_GRBIT grbit,
            MJET_WRN& wrn)
        {
            return Instance->MJetEnumerateColumns(tableid, columnids, grbit, wrn);
        }

        static array<MJET_ENUMCOLUMN>^ MJetEnumerateColumns(
            MJET_TABLEID tableid,
            array<MJET_ENUMCOLUMNID>^ columnids,
            MJET_GRBIT grbit)
        {
            return Instance->MJetEnumerateColumns(tableid, columnids, grbit);
        }

        static array<MJET_ENUMCOLUMN>^ MJetEnumerateColumns(MJET_TABLEID tableid, array<MJET_ENUMCOLUMNID>^ columnids)
        {
            return Instance->MJetEnumerateColumns(tableid, columnids);
        }

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

        static MJET_WRN MJetSetColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            array<Byte>^ data,
            MJET_GRBIT grbit,
            MJET_SETINFO setinfo)
        {
            return Instance->MJetSetColumn(tableid, columnid, data, grbit, setinfo);
        }

        static MJET_WRN MJetSetColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            array<Byte>^ data,
            MJET_GRBIT grbit)
        {
            return Instance->MJetSetColumn(tableid, columnid, data, grbit);
        }

        static MJET_WRN MJetSetColumn(
            MJET_TABLEID tableid,
            MJET_COLUMNID columnid,
            array<Byte>^ data)
        {
            return Instance->MJetSetColumn(tableid, columnid, data);
        }

        

        static void MJetPrepareUpdate(MJET_TABLEID tableid, MJET_PREP prep)
        {
            Instance->MJetPrepareUpdate(tableid, prep);
        }

        static MJET_RECPOS MJetGetRecordPosition(MJET_TABLEID tableid)
        {
            return Instance->MJetGetRecordPosition(tableid);
        }

        static void MJetGotoPosition(MJET_TABLEID tableid, MJET_RECPOS recpos)
        {
            Instance->MJetGotoPosition(tableid, recpos);
        }

        

        static MJET_TABLEID MJetDupCursor(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            return Instance->MJetDupCursor(tableid, grbit);
        }

        static MJET_TABLEID MJetDupCursor(MJET_TABLEID tableid)
        {
            return Instance->MJetDupCursor(tableid);
        }

        static String^ MJetGetCurrentIndex(MJET_TABLEID tableid)
        {
            return Instance->MJetGetCurrentIndex(tableid);
        }

        static void MJetSetCurrentIndex(MJET_TABLEID tableid, String^ name)
        {
            Instance->MJetSetCurrentIndex(tableid, name);
        }

        static void MJetSetCurrentIndex(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit)
        {
            Instance->MJetSetCurrentIndex(tableid, name, grbit);
        }

        static void MJetSetCurrentIndex(
            MJET_TABLEID tableid,
            String^ name,
            MJET_GRBIT grbit,
            Int64 itag)
        {
            Instance->MJetSetCurrentIndex(tableid, name, grbit, itag);
        }

        

        static MJET_WRN MJetMove(
            MJET_TABLEID tableid,
            Int32 rows,
            MJET_GRBIT grbit)
        {
            return Instance->MJetMove(tableid, rows, grbit);
        }

        static MJET_WRN MJetMove(MJET_TABLEID tableid, Int32 rows)
        {
            return Instance->MJetMove(tableid, rows);
        }

        static MJET_WRN MJetMove(
            MJET_TABLEID tableid,
            MJET_MOVE rows,
            MJET_GRBIT grbit)
        {
            return Instance->MJetMove(tableid, rows, grbit);
        }

        static MJET_WRN MJetMove(MJET_TABLEID tableid, MJET_MOVE rows)
        {
            return Instance->MJetMove(tableid, rows);
        }

        static void MoveBeforeFirst(MJET_TABLEID tableid)
        {
            Instance->MoveBeforeFirst(tableid);
        }

        static void MoveAfterLast(MJET_TABLEID tableid)
        {
            Instance->MoveAfterLast(tableid);
        }

        static Boolean TryMoveFirst(MJET_TABLEID tableid)
        {
            return Instance->TryMoveFirst(tableid);
        }

        static Boolean TryMoveNext(MJET_TABLEID tableid)
        {
            return Instance->TryMoveNext(tableid);
        }

        static Boolean TryMoveLast(MJET_TABLEID tableid)
        {
            return Instance->TryMoveLast(tableid);
        }

        static Boolean TryMovePrevious(MJET_TABLEID tableid)
        {
            return Instance->TryMovePrevious(tableid);
        }

        static Boolean TryMove(MJET_TABLEID tableid, MJET_MOVE rows)
        {
            return Instance->TryMove(tableid, rows);
        }

        static Boolean TryMove(MJET_TABLEID tableid, Int32 rows)
        {
            return Instance->TryMove(tableid, rows);
        }

        static void MJetGetLock(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            Instance->MJetGetLock(tableid, grbit);
        }

        static MJET_WRN MJetMakeKey(
            MJET_TABLEID tableid,
            array<Byte>^ data,
            MJET_GRBIT grbit)
        {
            return Instance->MJetMakeKey(tableid, data, grbit);
        }

#if (ESENT_WINXP)
#else
        static Int32 MJetPrereadKeys(
            MJET_TABLEID tableid,
            array<array<Byte>^>^ keys,
            MJET_GRBIT grbit)
        {
            return Instance->MJetPrereadKeys(tableid, keys, grbit);
        }
#endif

        static MJET_WRN MJetSeek(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            return Instance->MJetSeek(tableid, grbit);
        }

        static Boolean TrySeek(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            return Instance->TrySeek(tableid, grbit);
        }

        static Boolean TrySeek(MJET_TABLEID tableid)
        {
            return Instance->TrySeek(tableid);
        }

        static array<Byte>^ MJetGetBookmark(MJET_TABLEID tableid)
        {
            return Instance->MJetGetBookmark(tableid);
        }

        static MJET_SECONDARYINDEXBOOKMARK MJetGetSecondaryIndexBookmark(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            return Instance->MJetGetSecondaryIndexBookmark(tableid, grbit);
        }

        static MJET_SECONDARYINDEXBOOKMARK MJetGetSecondaryIndexBookmark(MJET_TABLEID tableid)
        {
            return Instance->MJetGetSecondaryIndexBookmark(tableid);
        }

        

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
        static void MJetSetMaxDatabaseSize(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pages,
            MJET_GRBIT grbit)
        {
            Instance->MJetSetMaxDatabaseSize(sesid, dbid, pages, grbit);
        }

        static Int64 MJetGetMaxDatabaseSize(
            MJET_SESID sesid,
            MJET_DBID dbid,
            MJET_GRBIT grbit)
        {
            return Instance->MJetGetMaxDatabaseSize(sesid, dbid, grbit);
        }
#endif
#endif

        static Int64 MJetSetDatabaseSize(
            MJET_SESID sesid,
            String^ database,
            Int64 pages)
        {
            return Instance->MJetSetDatabaseSize(sesid, database, pages);
        }

        static Int64 MJetGrowDatabase(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pages)
        {
            return Instance->MJetGrowDatabase(sesid, dbid, pages);
        }

        static MJET_WRN MJetResizeDatabase(
            MJET_SESID sesid,
            MJET_DBID dbid,
            Int64 pages,
            [Out] Int64% actualPages,
            MJET_GRBIT grbit)
        {
            return Instance->MJetResizeDatabase(sesid, dbid, pages, actualPages, grbit);
        }

        static void MJetSetSessionContext(MJET_SESID sesid, IntPtr context)
        {
            Instance->MJetSetSessionContext(sesid, context);
        }

        static void MJetResetSessionContext(MJET_SESID sesid)
        {
            Instance->MJetResetSessionContext(sesid);
        }

        static void MJetTracing(MJET_TRACEOP traceOp, MJET_TRACETAG traceTag, IntPtr ul)
        {
            Instance->MJetTracing(traceOp, traceTag, ul);
        }

        static void MJetTracing(MJET_TRACEOP traceOp, MJET_TRACETAG traceTag, MJET_DBID dbid)
        {
            Instance->MJetTracing(traceOp, traceTag, dbid);
        }

#if (ESENT_PUBLICONLY)
#else
        static MJET_SESSIONINFO MJetGetSessionInfo(MJET_SESID sesid)
        {
            return Instance->MJetGetSessionInfo(sesid);
        }
#endif

#if (ESENT_PUBLICONLY)
#else
        static void MJetDBUtilities(MJET_DBUTIL dbutil)
        {
            Instance->MJetDBUtilities(dbutil);
        }

#if (ESENT_WINXP)
#else
        static void MJetUpdateDBHeaderFromTrailer(String^ database)
        {
            Instance->MJetUpdateDBHeaderFromTrailer(database);
        }
#endif

#if (ESENT_WINXP)
#else
        static void MJetChecksumLogFromMemory(MJET_SESID sesid, String^ wszLog, String^ wszBase, array<Byte>^ buffer)
        {
            Instance->MJetChecksumLogFromMemory(sesid, wszLog, wszBase, buffer);
        }
#endif
#endif

        static void MJetGotoBookmark(MJET_TABLEID tableid, array<Byte>^ bookmark)
        {
            Instance->MJetGotoBookmark(tableid, bookmark);
        }

        static void MJetGotoSecondaryIndexBookmark(
            MJET_TABLEID tableid,
            array<Byte>^ secondaryKey,
            array<Byte>^ bookmark,
            MJET_GRBIT grbit)
        {
            Instance->MJetGotoSecondaryIndexBookmark(tableid, secondaryKey, bookmark, grbit);
        }

        

#if (ESENT_WINXP)
#else
        static MJET_TABLEID MJetOpenTemporaryTable(MJET_SESID sesid, MJET_OPENTEMPORARYTABLE% opentemporarytable)
        {
            return Instance->MJetOpenTemporaryTable(sesid, opentemporarytable);
        }
#endif

        static array<MJET_COLUMNID>^ MJetOpenTempTable(
            MJET_SESID sesid,
            array<MJET_COLUMNDEF>^ columndefs,
            MJET_UNICODEINDEX unicodeindex,
            MJET_GRBIT grbit,
            MJET_TABLEID% tableid)
        {
            return Instance->MJetOpenTempTable(sesid, columndefs, unicodeindex, grbit, tableid);
        }

        static array<MJET_COLUMNID>^ MJetOpenTempTable(
            MJET_SESID sesid,
            array<MJET_COLUMNDEF>^ columndefs,
            MJET_TABLEID% tableid)
        {
            return Instance->MJetOpenTempTable(sesid, columndefs, tableid);
        }

        

        static void MJetSetIndexRange(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            Instance->MJetSetIndexRange(tableid, grbit);
        }

        static Boolean TrySetIndexRange(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            return Instance->TrySetIndexRange(tableid, grbit);
        }

        static void ResetIndexRange(MJET_TABLEID tableid)
        {
            Instance->ResetIndexRange(tableid);
        }

        static Int64 MJetIndexRecordCount(MJET_TABLEID tableid, Int64 maxRecordsToCount)
        {
            return Instance->MJetIndexRecordCount(tableid, maxRecordsToCount);
        }

        static array<Byte>^ MJetRetrieveKey(MJET_TABLEID tableid, MJET_GRBIT grbit)
        {
            return Instance->MJetRetrieveKey(tableid, grbit);
        }

        static array<Byte>^ MJetRetrieveKey(MJET_TABLEID tableid)
        {
            return Instance->MJetRetrieveKey(tableid);
        }

        

#if (ESENT_PUBLICONLY)
#else

        static Boolean IsDbgFlagDefined()
        {
            return Instance->IsDbgFlagDefined();
        }

        static Boolean IsRtmFlagDefined()
        {
            return Instance->IsRtmFlagDefined();
        }

        static Boolean IsNativeUnicode()
        {
            return Instance->IsNativeUnicode();
        }

        static MJET_EMITDATACTX MakeManagedEmitLogDataCtx(const ::JET_EMITDATACTX& _emitctx)
        {
            return Instance->MakeManagedEmitLogDataCtx(_emitctx);
        }


        static MJET_EMITDATACTX MakeManagedEmitLogDataCtxFromBytes(array<Byte>^ bytes)
        {
            return Instance->MakeManagedEmitLogDataCtxFromBytes(bytes);
        }

        static MJET_WRN MJetConsumeLogData(
            MJET_INSTANCE instance,
            MJET_EMITDATACTX EmitLogDataCtx,
            array<Byte>^ data,
            MJET_GRBIT grbit)
        {
            return Instance->MJetConsumeLogData(instance, EmitLogDataCtx, data, grbit);
        }

        static MJET_WRN MJetConsumeLogData(
            MJET_INSTANCE instance,
            array<Byte>^ emitMetaData,
            array<Byte>^ data,
            MJET_GRBIT grbit)
        {
            return Instance->MJetConsumeLogData(instance, emitMetaData, data, grbit);
        }
#endif

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
        ::JET_SESID ,
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


                        UserCallback->PagePatch( m_interop->MakeManagedSNPatchRequest( _psnpatchrequest ) );
                    }
                    break;

                    case MJET_SNT::CorruptedPage:
                    {
                        ::JET_SNCORRUPTEDPAGE * _psncorruptedpage = (::JET_SNCORRUPTEDPAGE *)_pv;


                        UserCallback->CorruptedPage( (int)_psncorruptedpage->pageNumber );
                    }
                    break;

                    default:
                    break;
                }
            }
            else
            {
                err = JET_errSuccess;
            }

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
