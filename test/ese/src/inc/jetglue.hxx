// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
/*
**      JetGlue.hxx
**
**      NT wrappers for Jet calls to provide exception support   e
*/

#pragma once
#include <iostream>
#include <stdlib.h>
#include "esetest.h"
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

// Define logging file (can be overriden at compile time with -DNO_LOGGING flag)
#ifdef NO_LOGGING
#define RESULTS_LOG     "nul:"
#else
#define RESULTS_LOG     "results.log"
#endif

typedef void (*JetGlueInfoCallback)(void*, char*);

typedef FILE *FILEP;

const long MAX_INFO_LEVEL=9;
const long MAX_LONG = 0x7fffffff;
const long MAX_COLUMN_TEXT = JET_cbFullNameMost;
const long MAX_COLUMN_BINARY = MAX_COLUMN_TEXT;
const long MAX_COLUMN_LONGVALUE_DEFAULT = MAX_COLUMN_TEXT;
const long MIN_NAME_LENGTH = 4;
const long MAX_KEY_CHARS = 2000;
//const long MAX_KEY_SECTIONS = JET_ccolKeyMost;
const long MAX_TRANSACTION_LEVEL = 5+1+5; //Jet Limit + 0th level + extra slack


#define JERR_COUNT(rge) (sizeof(rge)/sizeof(JET_ERR))

/* Debug Information Levels */
const long kelAlways=0L;
const long kelErr=(kelAlways +1);
const long kelCmds=(kelErr +1);
const long kelProgress=(kelCmds +1);
const long kelDBInfo=(kelProgress + 1);
const long kelTblInfo=(kelDBInfo + 1);
const long kelColInfo=(kelTblInfo + 1);
const long kelIndexInfo=(kelColInfo);
const long kelFieldInfo=(kelColInfo + 1);
const long kelFieldData=(kelFieldInfo + 1);
const long kelInfo=(kelFieldData+1);

/* Some contants used by the random datas generator */
#define kfNone          0
#define kfLower     1
#define kfUpper     2
#define kfNumer     4
#define kfPunct     8
#define kfWhiteSpace    16
#define kfAlpha     (kfLower | kfUpper)
#define kfAlphaNum  (kfAlpha | kfNumer)
#define kfAll       (kfAlphaNum | kfPunct | kfWhiteSpace)

#define TEST_bitCleanBackup     0x00000001
#define TEST_bitSkipTruncate        0x00000002

// The exception code that we use. I just made it up. It looks slightly like 'ESE code'.
enum { excJetGlueExceptionCode = 0x0e5ec0de };

void SetupLogging(BOOL, char*, char* =NULL, long=0, long=80);
double DoubleGetTimeDiff(
    const _timeb *ptimeb1,
    const _timeb *ptimeb2
)
;
void _cdecl PrintN(long, __format_string char*, ...);

void JetGlueInit(void);
void JetGlueClose(void);
void JetGlueOutputToResultLog(long=kelErr);
void JetGlueStartInfoCounter(void);
void JetGlueSetProfileGranularity(unsigned long);
int JetGlueGetInfoLevel(void);
void JetGlueSetInfoLevel(int);
void JetGlueSetInfoCallback(JetGlueInfoCallback, void*);
void JetGlueSetCompaction(void);
void JetGlueSetErrorHandlerExceptionFlags(DWORD=0);
bool FJetGlueExternalBackupRestore(
)
;
void JetGlueSetExternalBackupRestore(
    _In_ const bool fExternal
)
;
bool FJetGlueAtomicBackupRestore(
)
;
void JetGlueSetAtomicBackupRestore(
    _In_ const bool fAtomic
)
;
void JetGlueActivateLongvalue();
void JetGlueActivateiLongvalue();
void JetGlueThrow(
    _In_ const char* szErr,
    _In_ JET_ERR    err = JET_errSuccess
)
;
void JetGlueSetErrorHandlerAction(BOOL fAssertOnError=TRUE);
void JetGlueSetAllowConcurrentDDL(BOOL fAllowConcurrentDDL=TRUE);
void JetGlueSetUpdatesInTransaction(BOOL fValue=TRUE);
void JetGlueSetRecordsToUpdateBeforeCommit(long lUpdates);

/* Data Generation */
void
szRandom(
    __out_ecount( len ) char *sz,
    unsigned long len,
    long flags
);
void rgRandom( __out_bcount( cb ) void* pv, unsigned long cb);
void FillJCDRandom( _Out_ JET_COLUMNDEF *);
void FixupJCDBasedOnColtyp(
    __inout JET_COLUMNDEF * const pcolumndef
)
;
void FillJCCRandom( _Out_ JET_COLUMNCREATE*);
void FillJTC3Random(JET_TABLECREATE3 *, char*, char* =0, unsigned long = 0, JET_SPACEHINTS * pjsphTable = NULL, JET_SPACEHINTS * pjsphLV = NULL, unsigned long ulSeparateLV = 0 );
void ColDataRandom(JET_COLUMNDEF *, unsigned long*, void*);
void FillColSz(JET_COLTYP, char *, char *, unsigned long);



/* Magic Functions */
void JetDateToSz(JET_DATESERIAL dt, char* sz);
void JObjTyp2Sz(JET_OBJTYP, char*);
void JColTyp2Sz(JET_COLTYP, char*);
void JBitIndex2Sz(JET_GRBIT, char*);
void JBitColumn2Sz(JET_GRBIT, char*);
void JBitSeek2Sz(JET_GRBIT, char*);
void JBitTableOpen2Sz(JET_GRBIT, char*);
void JSort2Sz(long, char*);
void JPrep2Sz(long, char*);
void JBinary2Ascii(char *, char*, long);
JET_COLTYP JCTFromTokenSz(char **psz);
JET_GRBIT JCGrBitsFromTokenSz(char** psz);

void
JTraceOp2Sz(
    _In_ const JET_TRACEOP  val,
    _Out_ char*             sz
)
;
void
JTraceTag2Sz(
    _In_ const JET_TRACETAG val,
    _Out_ char*             sz
)
;

inline BOOL FErrInRg(JET_ERR err, int c, const JET_ERR rgErr[] )
{
    while(c--)
        if (rgErr[c] == err) return TRUE;
    return FALSE;
}



JET_ERR JGSetSystemParameter(JET_SESID sesid, unsigned long paramid, DWORD_PTR lParam, const char  *sz);
JET_ERR JGInit(JET_INSTANCE *pinstance = 0);
JET_ERR JGTerm(JET_INSTANCE instance = 0);
JET_ERR JGTerm2(JET_INSTANCE instance, JET_GRBIT grbit);
JET_ERR JGBackup(_In_ const DWORD grbitTestOptions, const char *szBackupPath, JET_GRBIT grbit, JET_PFNSTATUS pfnStatus );
JET_ERR JGRestore(const char *szPath);
JET_ERR JGBeginSession(JET_SESID *pjsi, const char* name, const char* password);
JET_ERR JGGetVersion(JET_SESID pjsi, unsigned long* ver);
JET_ERR JGEndSession(JET_SESID sesid, JET_GRBIT grbit);
JET_ERR JGAttachDatabase(JET_SESID sesid, const char *szName, JET_GRBIT jbit);
JET_ERR JGCreateDatabase(_In_ JET_SESID sesid, _In_z_ const char  *szFilename, _In_opt_z_ const char  *szConnect, _Out_ JET_DBID  *pdbid, _In_ JET_GRBIT grbit, _In_ int cIgnoreErr=0, _In_reads_opt_(cIgnoreErr) const JET_ERR rgErr[]=NULL);
JET_ERR JGOpenDatabase(JET_SESID sesid, const char  *szFilename, const char  *szConnect, JET_DBID  *pdbid, JET_GRBIT grbit);
JET_ERR JGDetachDatabase(JET_SESID sesid, const char  *szFilename);
JET_ERR JGCloseDatabase(JET_SESID sesid, JET_DBID dbid,    JET_GRBIT grbit);
JET_ERR JGGetObjectInfo(JET_SESID sesid, JET_DBID dbid, JET_OBJTYP objtyp, const char  *szContainerName, const char  *szObjectName, void  *pvResult, unsigned long cbMax, unsigned long InfoLevel);
JET_ERR JGMove(JET_SESID sesid, JET_TABLEID tableid, long cRow, JET_GRBIT grbit, int cIgnoreErr=0, JET_ERR *rgErr=NULL);
JET_ERR JGCreateTable(JET_SESID sesid, JET_DBID dbid, const char  *szTableName, unsigned long lPages, unsigned long lDensity,JET_TABLEID  *ptableid);
JET_ERR JGCreateTableColumnIndex(JET_SESID sesid, JET_DBID dbid, JET_TABLECREATE *ptablecreate,int cIgnoreErr=0, JET_ERR *rgErr=0);
JET_ERR JGCreateTableColumnIndex3(JET_SESID sesid, JET_DBID dbid, JET_TABLECREATE3 *ptablecreate,int cIgnoreErr=0, JET_ERR *rgErr=0);
JET_ERR JGOpenTable(JET_SESID sesid, JET_DBID dbid, const char  *szTableName, const void  *pvParameters, unsigned long cbParameters, JET_GRBIT grbit, JET_TABLEID  *ptableid, int cIgnoreErr=0, JET_ERR *rgErr=NULL);
JET_ERR JGOpenTempTable(JET_SESID sesid, const JET_COLUMNDEF *prgcolumndef, unsigned long ccolumn,JET_GRBIT grbit, JET_TABLEID *ptableid, JET_COLUMNID *prgcolumnid, int cIgnoreErr=0, JET_ERR *rgErr=NULL);
JET_ERR JGCloseTable(_In_ JET_SESID sesid, _In_ JET_TABLEID tableid, _In_ int cIgnoreErr = 0, _In_reads_opt_(cIgnoreErr) const JET_ERR rgErr[] = NULL);
JET_ERR JGGetDatabaseInfo(JET_SESID sesid, JET_DBID dbid, void  *pvResult, unsigned long cbMax, unsigned long InfoLevel);
JET_ERR JGGetTableColumnInfo(JET_SESID sesid, JET_TABLEID tableid, const char  *szColumnName, void  *pvResult, unsigned long cbMax, unsigned long InfoLevel);
JET_ERR JGComputeStats(JET_SESID sesid, JET_TABLEID tableid);
JET_ERR JGGetTableInfo(JET_SESID sesid, JET_TABLEID tableid, void  *pvResult, unsigned long cbMax, unsigned long InfoLevel);
JET_ERR JGRenameTable(JET_SESID sesid, JET_DBID dbid, const char  *szTableName, const char  *szTableNew);
JET_ERR JGDeleteTable(JET_SESID sesid, JET_DBID dbid, const char  *szTableName, int cIgnoreErr, JET_ERR *rgErr);
JET_ERR JGAddColumn(JET_SESID sesid, JET_TABLEID tableid, const char  *szColumn, const JET_COLUMNDEF  *pcolumndef, const void  *pvDefault, unsigned long cbDefault, JET_COLUMNID  *pcolumnid);
JET_ERR JGRenameColumn(JET_SESID sesid, JET_TABLEID tableid, const char  *szColumn, const char  *szColumnNew);
JET_ERR JGDeleteColumn(JET_SESID sesid, JET_TABLEID tableid, const char  *szColumn, int cIgnoreErr=0, const JET_ERR rgErr[] = NULL);
JET_ERR JGBeginTransaction(JET_SESID sesid);
JET_ERR JGCommitTransaction(JET_SESID sesid, JET_GRBIT grbit);
JET_ERR JGRollback(JET_SESID sesid, JET_GRBIT grbit);
JET_ERR JGPrepareUpdate(JET_SESID sesid, JET_TABLEID tableid, unsigned long prep,int cIgnoreErr=0, JET_ERR *rgErr=NULL);
JET_ERR JGSetColumn(JET_SESID sesid, JET_TABLEID tableid, JET_COLUMNID columnid, const void  *pvData, unsigned long cbData, JET_GRBIT grbit, JET_SETINFO  *psetinfo,int cIgnoreErr=0, JET_ERR *rgErr=NULL);
JET_ERR JGSetColumns(JET_SESID sesid, JET_TABLEID tableid, JET_SETCOLUMN *psetcolumn, unsigned long csetcolumn, int cIgnoreErr=0, JET_ERR *rgErr=NULL);
JET_ERR JGUpdate(JET_SESID sesid, JET_TABLEID tableid, void  *pvBookmark, unsigned long cbBookmark, unsigned long  *pcbActual,int cIgnoreErr=0, JET_ERR *rgErr=NULL);
JET_ERR JGEscrowUpdate(JET_SESID sesid, JET_TABLEID tableid, JET_COLUMNID columnid, void *pv, unsigned long cbMax, void *pvOld, unsigned long cbOldMax, unsigned long *pcbOldActual, JET_GRBIT grbit, int cIgnoreErr=0, JET_ERR *rgErr=NULL);
JET_ERR JGDelete(JET_SESID sesid, JET_TABLEID jti, int cIgnoreErr=0, JET_ERR *rgErr=NULL);
JET_ERR JGRetrieveColumn(JET_SESID sesid, JET_TABLEID tableid, JET_COLUMNID columnid, void  *pvData, unsigned long cbData, unsigned long  *pcbActual, JET_GRBIT grbit, JET_RETINFO  *pretinfo, int cIgnoreErr=0, JET_ERR *rgErr=NULL);
JET_ERR JGRetrieveColumns( JET_SESID sesid, JET_TABLEID tableid, JET_RETRIEVECOLUMN *pretrievecolumn, unsigned long cretrievecolumn, int cIgnoreErr=0, JET_ERR *rgErr=NULL);
JET_ERR JGRetrieveKey(JET_SESID sesid, JET_TABLEID tableid, void  *pvData, unsigned long cbMax, unsigned long  *pcbActual, JET_GRBIT grbit, int cIgnoreErr=0, JET_ERR *rgErr=NULL);
JET_ERR JGSetCurrentIndex(JET_SESID sesid, JET_TABLEID tableid, const char  *szIndexName);
JET_ERR JGGetCurrentIndex(JET_SESID sesid, JET_TABLEID tableid, char *szIndexName, unsigned long cchIndexName, int cIgnoreErr=0, JET_ERR *rgErr=NULL);
JET_ERR JGCreateIndex(
    JET_SESID sesid,
    JET_TABLEID tableid,
    const char  *szIndexName,
    JET_GRBIT grbit,
    const char  *szKey,
    unsigned long cbKey,
    unsigned long lDensity,
    int cIgnoreErr=0,
    __in_ecount( cIgnoreErr ) JET_ERR *rgErr=NULL
    );
JET_ERR JGRenameIndex(JET_SESID sesid, JET_TABLEID tableid, const char  *szIndex, const char  *szIndexNew);
JET_ERR JGDeleteIndex(JET_SESID sesid, JET_TABLEID tableid, const char  *szIndexName);
JET_ERR JGGetIndexInfo(JET_SESID sesid, JET_DBID dbid, const char  *szTableName, const char  *szIndexName, void  *pvResult, unsigned long cbResult, unsigned long InfoLevel);
JET_ERR JGGetTableIndexInfo(JET_SESID sesid, JET_TABLEID tablieid, const char  *szIndexName, void  *pvResult, unsigned long cbResult, unsigned long InfoLevel);
JET_ERR JGMakeKey(JET_SESID sesid, JET_TABLEID tableid, const void *pvData, unsigned long cbData, JET_GRBIT grbit, int cIgnoreErr=0, JET_ERR *rgErr=NULL);
JET_ERR JGSeek(JET_SESID sesid, JET_TABLEID tableid, JET_GRBIT grbit, int cIgnoreErr=0, JET_ERR *rgErr=NULL);
JET_ERR JGGetBookmark(JET_SESID sesid, JET_TABLEID tableid,void *pvBookmark, unsigned long cbMax,unsigned long *pcbActual, int cIgnoreErr=0, JET_ERR *rgErr=NULL);
JET_ERR JGGotoBookmark(JET_SESID sesid, JET_TABLEID tableid, void *pvBookmark, unsigned long cbBookmark, int cIgnoreErr=0, JET_ERR *rgErr=NULL);
JET_ERR JGBeginExternalBackup( JET_GRBIT grbit );
JET_ERR JGGetAttachInfo( void *pv,  unsigned long cbMax, unsigned long *pcbActual );
JET_ERR JGOpenFile( const char *szFileName, JET_HANDLE  *phfFile, unsigned long *pulFileSizeLow, unsigned long *pulFileSizeHigh );
JET_ERR JGReadFile( JET_HANDLE hfFile, void *pv, unsigned long cb, unsigned long *pcb );
JET_ERR JGCloseFile( JET_HANDLE hfFile );
JET_ERR JGGetLogInfo( void *pv, unsigned long cbMax, unsigned long *pcbActual );
JET_ERR JGTruncateLog( void );
JET_ERR JGEndExternalBackup( void );
JET_ERR JGExternalRestore( char *szCheckpointFilePath, char *szLogPath, JET_RSTMAP *rgstmap, int crstfilemap, char *szBackupLogPath, int genLow, int genHigh, JET_PFNSTATUS pfn );
JET_ERR JGDefragment(JET_SESID sesid, JET_DBID dbid, const char  *szTableName, unsigned long *pcPasses, unsigned long *pcSeconds, JET_GRBIT grbit, int cIgnoreErr=0, JET_ERR *rgErr=NULL);

JET_ERR JET_API JGTracing(
    const JET_TRACEOP   traceop,
    const JET_TRACETAG  tracetag,
    const JET_API_PTR   ul );


JET_ERR JET_API
JGResizeDatabase(
    _In_  JET_SESID         sesid,
    _In_  JET_DBID          dbid,
    _In_  unsigned long     cpg,
    _Out_ unsigned long *   pcpgActual,
    _In_  const JET_GRBIT   grbit );


/* Utility funtions */
inline long lAbs(long n) {return n>0?n:-n;}
void ParseRange(char *, long*, long*);


/*
********************************************************
**                                                     *
**  Pattern Generators                                 *
**                                                     *
********************************************************
*/
class cPatTyphoon
{
private:
    long currentIndex;

public:
    cPatTyphoon(long c=0) {this->currentIndex=c;}
    virtual long operator [] (long lIndex){return 0;}
    virtual long Next(void) {return (*this)[currentIndex++];}
    virtual void Initialize(void){}
};


/*
** Trying to force serialization of rand()
** StandardRand() returns a number in the range of
** 0-(2^31-2). The implementation is straight off
** the Standard Random number generator proposal
** in:
** "Random Number Generators : Good Ones are Hard to Find"
** Stephen K. Park and Keith W. Miller, CACM v31.No10 pp1195
**
** Verified for z(10001)=1043618065 (-1, due to our normalization)
*/

// These are the original random functions. They were ported to esetest.lib Unfortunately, the names 'Rand' and 'StandardRand'
// were swapped (BARF)

// I'm going to rely on function overloading to dismbiguate it all.

//3 // same as esetest.lib
// inline long GetStandardSeed(void) {return gStandardRandSeed;}
//3 // same as esetest.lib
// inline void SeedStandardRand(long seed)

inline long Rand(long &lCurrentSeed)
{
    //3 // in esetest.lib
    return StandardRand( lCurrentSeed );
}

inline long StandardRand()
{
    //3 // in esetest.lib
    return Rand();
}

// END of what was moved to esetest.lib

inline long StandardRand(long lo, long hi)
{
    return lo + (lAbs(StandardRand()%(hi-lo+1)));
}

inline BOOL StandardRandProbability(double prob) // return true with probability prob
{
    const long probModulo = 0xfffffff;

    // special case 0.
    if ( 0.0 == prob )
    {
        return FALSE;
    }
    return ((StandardRand() & probModulo) < (prob * probModulo));
}

class cPatRandom: public cPatTyphoon
{
private:
    long seed;

public:
//  cPatRandom(long seed=1) {val = seed;}
    cPatRandom(long seed=1) {this->seed = seed;}
    virtual long operator[](long i) const {return 0;}
    virtual long Next(void) {return StandardRand();}
    virtual void Initialize(void) {SeedStandardRand(seed);}
    /*
    {
        #define A ((unsigned long)0x41A7)
        #define High(x) (((unsigned long)(x)&0xffff0000)>>16)
        #define Low(x) ((unsigned long)(x)&0xffff)
        unsigned long temp;

        temp = A*High(val) + High(A*Low(val));
        val = ((temp & 0x7fff) << 16) + High(temp<<1) + Low(A*val);

        if (Low(val) == 0x8000)
            return 0;
        else
            return Low(val);
    }
    */
};


class cPatSine: public cPatTyphoon
{
private:
    double amplitude, phase, scale;

public:
    cPatSine(double a=1.0, double p=0.0, double s= 0.01) {amplitude = a; phase = p; scale =s;}
    virtual long operator[](long i) const {return (long)(amplitude*sin(i/scale) + phase);}
};

class cPatSquare: public cPatTyphoon
{
private:
    double amplitude, phase, scale;

public:
    cPatSquare(double a=1.0, double p=0.0, double s= 1.0) {amplitude = a; phase = p; scale =s;}
    virtual long operator[](long i) const
    {
        return static_cast<long>( amplitude* ( ((long)(i/scale)%2)?1:-1 ) + phase );
    }
};

class cPatLine : public cPatTyphoon
{
private:
    long m,c;

public:
    cPatLine(long m=0, long c=0, long seed = 0):cPatTyphoon(seed) {this->m = m; this->c =c;}
    virtual long operator[](long i) const {return (m*i) + c;}
};



/*
********************************************************
**
**      Utility classes
**
********************************************************
*/
/* A general-purpose stack which grows dynamically */
class PtrStack
{
private:
    void **rgpv;
    // These are actually NOT 'count of bytes', but 'count of elements'.
    long cb;
    long cbMax;
    long cbPad;

public:
    PtrStack(void) {cb = 0; cbPad = 4; cbMax = cbPad; rgpv=new void * [cbMax];}
    void Push(void *pv)
    {
        if (cb >= cbMax)
        {
            void **rgpvNew;
            long cbMaxNew;
            long i;


            cbMaxNew = cbMax + cbPad;
            rgpvNew = new void * [cbMaxNew] ;

            for (i=0; i< cbMax; i++)
                rgpvNew[i] = rgpv[i];

            delete[] rgpv;
            rgpv = rgpvNew;
            cbMax = cbMaxNew;
        }
        Add(pv, cb);
        cb++;
    }
    void Add(void *pv, long i) {rgpv[i] = pv;}
    void Remove(long from, long to)
    {
        long i;
        long cbDelete = to-from+1;
        long cbMove=cb-to-1;

        //printf("count:%ld\n", cb);
        if (cbDelete<=0)
        {
            return;
        }

        for (i=0; i< cbMove; i++)
        {
            rgpv[i+from] = rgpv[i+to+1];
        }
        cb -= cbDelete;
    }
    long Find(void *pv)
    {
        long found=-1;
        long i;

        for (i = 0; i< cb; i++)
        {
            if (pv == rgpv[i])
            {
                found = i;
                break;
            }
        }
        return found;
    }

    void Remove(long i) {if ((i>= 0) && (i<cb)) Remove(i,i);}
    void Remove(void *pv) { Remove(Find(pv));}
    void *operator[](long i) {return this->rgpv[i];}
    long operator[](void *pv) {return this->Find(pv);}
    long ItsSize(void) {return cb;}
    void *Top(void) {return this->rgpv[cb-1];}
    BOOL IsMember(void* pv){return ((*this)[pv] != -1);}
};


/*
****************************************************
**
**      Jet wrapper classes
**
****************************************************
*/

class JetSession;
class JetTable;



/* Structure to hold Verification column data */
typedef struct _JetVerificationData
{
    unsigned long lCheckSum;
// $cleanup -SOMEONE I see no use for storing a pointer in the Database
//  class JetSession * pjs;
} JetVerificationData;

/* Structure to hold checksum information */
class JetCheckSum
{
private:
    unsigned long lCheckSum;
    long lCSTemp;

public:
    JetCheckSum(void):lCheckSum(0), lCSTemp(0) {}

    void Set(unsigned long l) {lCheckSum = l;}
    void Begin(void) {lCSTemp=0;}
    void Add(unsigned long lCount) {lCSTemp += lCount;}
    void Subtract(unsigned long lCount) {lCSTemp -= lCount;}
    void Commit(void) {lCheckSum += lCSTemp;}
    BOOL Compare(unsigned long l) {return l== this->CheckSum();}
    unsigned long CheckSum(void) {return lCheckSum;}
    unsigned long CheckSumInProgress(void) {return lCSTemp;}

    static unsigned long Compute(long lcb, char * pbuffer);
};

class JetBookMark
{
public:
    long size;
    char *psz;

public:
    JetBookMark(void) {this->size=0; this->psz =0;}
    ~JetBookMark() {delete this->psz;}
    JetBookMark(long cb, char* sz)
    {
        this->psz = new char[cb];
        memcpy(this->psz, sz, cb);
        this->size = cb;
    }
};
typedef JetBookMark *PJBM;

/* Struct to store column info */
#define MAX_COLUMN_NAME JET_cbNameMost
class JetColumn
{
private:

public:
//    JET_COLUMNID jci;
    JET_COLUMNDEF jcd;
    char szName[MAX_COLUMN_NAME+1];
    JetCheckSum jcs;
    unsigned long lcsDefault;

public:
    JetColumn(void):lcsDefault(0) {szName[0]='0';}

    unsigned long ComputeCheckSum(JetSession*, JetTable*);
    unsigned long ComputeFieldCheckSum(JetSession*, JetTable*);

    friend std::ostream& operator<<(std::ostream&, JetColumn&);
    friend std::istream& operator>>(std::istream&, JetColumn&);
    friend FILEP operator>>(FILEP, JetColumn&);
    friend BOOL operator== (JetColumn&, JetColumn&);
};
typedef JetColumn *PJCI;

/* Struct to store index info */
#define MAX_INDEX_NAME JET_cbNameMost
class JetIndex
{
public:
    char szName[MAX_INDEX_NAME+1];
    PtrStack rgColumns;
    JET_GRBIT grbit;

private:
public:
    JetIndex(void) {szName[0]='\0'; grbit=0;};

    PJCI Column(long i) {return ((PJCI)(this->rgColumns)[i]);}
    long ColumnCount(void) {return this->rgColumns.ItsSize();}

    friend std::ostream& operator<<(std::ostream&, JetIndex&);
    friend std::istream& operator>>(std::istream&, JetIndex&);
    friend FILEP operator>>(FILEP, JetIndex&);
    friend BOOL operator== (JetIndex&, JetIndex&);
};
typedef JetIndex *PJII;

/* class to store table info */
#define MAX_TABLE_NAME  JET_cbNameMost
class JetTable
{
private:
    PtrStack *rgColumns;
    PtrStack *rgIndices;
    PtrStack *rgBookmarks;

public:
    JET_TABLEID jti;
    char szName[MAX_TABLE_NAME+1];
    BOOL fHasPrimaryIndex;      // Does this table have a primary index?
    BOOL fIsOpen;
    long cSecondary;    // Number of Secondary indices
    JetColumn *pjcVerify;
    BOOL fDMLVerify;
    long lRowCount;
    PJII pjiiCurrent;
    HANDLE mtxIndexCreation;

private:
    void JetTableInit(void);

public:
    JetTable(void) {JetTableInit();}
    JetTable(JetTable*);
    ~JetTable();


    //  Note will take JET_TABLECREATE or JET_TABLECREATE3 and do the right thing as long
    //  as the cbStruct is set correctly.
    void Create(JetSession *pjs, JET_TABLECREATE3 *pjtc, BOOL fClose, BOOL fVerify=FALSE);
    PJCI Column(long i) {return ((PJCI)(*this->rgColumns)[i]);}
    PJCI Column(char *sz);
    PJII Index(long i) {return ((PJII)(*this->rgIndices)[i]);}
    PJII Index(char *sz);
    PJII Index(PJCI pjci);
    PJBM Bookmark(long i) {return (PJBM)(*this->rgBookmarks)[i];}
    long ColumnCount(void) {return this->rgColumns->ItsSize();}
    long IndexCount(void) {return this->rgIndices->ItsSize();}
    long BookmarkCount(void) {return this->rgBookmarks->ItsSize();}
    void Add(PJCI pjci) {this->rgColumns->Push(pjci);}
    void Remove(PJCI pjci) {this->rgColumns->Remove(pjci);}
    void Add(PJII pjii) {this->rgIndices->Push(pjii);}
    void Remove(PJII pjii) {this->rgIndices->Remove(pjii);}
    void Add(PJBM pjbm) {this->rgBookmarks->Push(pjbm);}
    void Remove(PJBM pjbm) {this->rgBookmarks->Remove(pjbm);}
    //void RemoveColumns(long from, long to) {this->rgColumns->Remove(from, to);}
    void RemoveIndices(long from, long to) {this->rgIndices->Remove(from, to);}
    void Close(JetSession*);
    void Open(JetSession*, JET_GRBIT=0);
    void Delete(JetSession*);
    void CreateCheckSumColumn(JetSession*);
    void DeleteCheckSumColumn(JetSession*);
    void CreateColumn(JetSession*, char*, JET_COLUMNDEF*);
    void DeleteColumn(JetSession*, long lFrom, long lTo);
    void LoadFromDatabase(JetSession*);
    void CreateIndexMtx(void);
    void DestroyIndexMtx(void);


    void VerifySetColumn(JetSession*, unsigned long);
    unsigned long VerifyColumnCS(JetSession*);
    void VerifyCurrentRecord(JetSession*);

    long RowCount(JetSession*){return this->lRowCount;}
    unsigned long RecordCheckSum();
    void RecordCSBegin(void);
    void RecordCSCommit(void);
    void RecordCSSubtractFromDB(JetSession*);

    friend std::ostream& operator<<(std::ostream&, JetTable&);
    friend std::istream& operator>>(std::istream&, JetTable&);
    friend FILEP operator>>(FILEP, JetTable&);
    friend BOOL operator==(JetTable&, JetTable&);
};

typedef JetTable *PJTI;

class JetTableArray : public PtrStack
{
public:
    JetTableArray(void):PtrStack(){}
    JetTableArray(JetTableArray*);
    ~JetTableArray();
    PJTI operator[](long i) {return (PJTI) (*(PtrStack*)this)[i];}
};


/* A class to hold information about a particular Jet Session */
class JetSession
{
private:
    char user[256];
    char password[256];
    char szDatabase[512];
    JetTableArray *rgTables[MAX_TRANSACTION_LEVEL];
    BOOL fDetach;

public:
    JET_SESID jsi;
    JET_DBID jdi;
    int nTransactionLevel;
    long gLVMaxChunks;
    long gLVMaxChunkSize;
    char *rgLVBuffer;  // Buffer for LV chunks
    BOOL fDMLVerify;

private:
    BOOL LoadVerifyArchive(void);

public:
    JetSession(char* szDatabase, BOOL fCreate, BOOL fAttach=FALSE, BOOL fDetach=FALSE, BOOL fLoad=FALSE, JET_GRBIT grbitLV=JET_bitNil, BOOL fDMLVerify=TRUE, char* szUser="admin", char* szPassword="\0", JET_GRBIT grbitCreateAttach=JET_bitNil);
    ~JetSession();

    void LoadFromDatabase(void);

    PJTI Table(long i) {return ((PJTI)(*this->rgTables[nTransactionLevel])[i]);}
    PJTI Table(char *sz);
    long TableCount(void) {return this->rgTables[nTransactionLevel]->ItsSize();}
    void Add(PJTI pjti) {this->rgTables[nTransactionLevel]->Push(pjti);}
    void Remove(long from, long to) {this->rgTables[nTransactionLevel]->Remove(from, to);}
    void Remove(PJTI pjti) {this->rgTables[nTransactionLevel]->Remove(pjti);}
    void BeginTransaction(void);
    void CommitTransaction(void);
    void RollbackTransaction(void);
    BOOL VerifyDML(void);
    BOOL IsThereVerifyArchive(void);
    void DeleteVerifyArchive(void);
    void Archive(void);
    void AllocateLVBuffer(void);  // Buffer for LV chunks
};

class JetTest
{
public:
    static JetTableArray *pjtaLevel0;
    static double dEscrowDefaultValueDMLProb;
    static double dDefaultValueDDLProb;
    static double dDefaultValueDMLProb;
    static double dEscrowColumnProb;
    static BOOL fLongvalues;
    static BOOL fiLongvalues;
    static BOOL fUpdateRecordsInTransaction;
    static long lRecordsToUpdateBeforeCommit;
    static BOOL fAllowConcurrentDDL;
// $cleanup SOMEONE  Moving rgLVBuffer into JetSession so that there
// is only one per thread
//  static char *rgLVBuffer;

public:
    ~JetTest();
// $cleanup SOMEONE  Moving rgLVBuffer into JetSession so that there
// is only one per thread
//  static void AllocateLVBuffer(void);
};


typedef void (*TyphoonCmdCallback)(JetSession *pjs, void*);

/* An abstract class for typhoon cmds. */
class TyphoonCmd
{
protected:
    TyphoonCmdCallback itsCallback;

public:
    TyphoonCmd(void) {itsCallback = NULL;}
    virtual void PrintInfo(void)=0;
    virtual void Play(JetSession *pjs) {this->PrintInfo();}
};



class TCSemaphore : public TyphoonCmd
{
protected:
    static const size_t cchName     = 256;
    char szName[cchName];

    HANDLE WINAPI MyOpenSemaphore(_In_  DWORD dwDesiredAccess,  _In_  BOOL bInheritHandle,  _In_  char* cszName)
    {
        wchar_t* wszName;
        size_t convertedChars = 0;
        size_t newsize = 0; 
        
        newsize = strlen(cszName) + 1;
        wszName = new wchar_t[newsize];
        mbstowcs_s(&convertedChars, wszName, newsize, cszName, _TRUNCATE);
        
        delete wszName;

        return OpenSemaphoreW(dwDesiredAccess, bInheritHandle, wszName);
    }

public:
    TCSemaphore(void) {szName[0]='\0';}
    TCSemaphore(char *sz)
    {
        StringCchCopyA( szName, cchName, sz );
    }
    virtual void PrintInfo(void) {PrintN(kelCmds, "Semaphore:%s\n", this->szName);}
};

class TCSignalSemaphore: public TCSemaphore
{
typedef TCSemaphore super;
public:
    TCSignalSemaphore(char *sz):super(sz) {}
    virtual void PrintInfo(void) {PrintN(kelCmds, "Signal "); super::PrintInfo();}
    virtual void Play(JetSession *pjs)
    {
        HANDLE hSemaphore;

        TyphoonCmd::Play(pjs);

        if (hSemaphore = MyOpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, this->szName))
            ReleaseSemaphore(hSemaphore, 1, NULL);
        else
            PrintN(kelErr, "Could not open Semaphore:%s\n", this->szName);
    }
};

class TCWaitSemaphore: public TCSemaphore
{
typedef TCSemaphore super;
public:
    virtual void PrintInfo(void) {PrintN(kelCmds, "Wait for "); super::PrintInfo();}
    TCWaitSemaphore(char *sz):super(sz) {}
    virtual void Play(JetSession *pjs)
    {
        HANDLE hSemaphore;

        TyphoonCmd::Play(pjs);

        if (hSemaphore = MyOpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, this->szName))
        {
            WaitForSingleObject(hSemaphore, INFINITE);
            PrintN(kelProgress, "Semaphore %s released. Continuing execution...\n", this->szName);
        }
        else
            PrintN(kelErr, "Could not open Semaphore:%s\n", this->szName);
    }
};

class TCPattern : public TyphoonCmd
{
private:
    cPatTyphoon *newPat;

public:
    static cPatTyphoon *gPatTyphoon;
    //_declspec (thread) static cPatTyphoon *gPatTyphoon;

public:
    TCPattern(cPatTyphoon* pt) {newPat = pt;};
    ~TCPattern() { if(newPat) delete newPat; };
    virtual void PrintInfo(void) {PrintN(kelCmds, "New Pattern Generator\n");};
    virtual void Play(JetSession *pjs)
    {
        TyphoonCmd::Play(pjs);

        gPatTyphoon = newPat;
        newPat->Initialize();
    }
};

// Just call JetGlueSetInfoLevel from Typhoon since the behavior
// is global.  -SOMEONE
/*
class TCDebugLevel: public TyphoonCmd
{
private:
    int newLevel;
public:
    TCDebugLevel(void) {newLevel = kelErr;}
    TCDebugLevel(int newl) {newLevel = newl;}
    virtual void PrintInfo(void) {PrintN(kelCmds, "DebugLevel:%ld\n", JetGlueGetInfoLevel());}
    virtual void Play(JetSession *pjs) {TyphoonCmd::Play(pjs);JetGlueSetInfoLevel(newLevel);}
};
*/

class TCThreadPriority: public TyphoonCmd
{
private:
    int priority;
public:
    TCThreadPriority(void) {priority = THREAD_PRIORITY_NORMAL;}
    TCThreadPriority(int p) {priority = p;}
    virtual void PrintInfo(void) {PrintN(kelCmds, "New Thread Priority Level:%ld\n", priority);}
    virtual void Play(JetSession *pjs) {TyphoonCmd::Play(pjs);SetThreadPriority(GetCurrentThread(), priority);}
};

class TCBackup: public TyphoonCmd
{
protected:
    static const size_t cchDir  = _MAX_PATH;
    char szDir[cchDir];
    BOOL fIncremental;
    BOOL fOverwrite;
    BOOL fTruncate;

public:
    TCBackup(void)
    {
        strcpy(szDir, "Backup");
    }
    TCBackup(char* sz, BOOL fI=FALSE, BOOL fO=FALSE, BOOL fT=TRUE)
    {
        StringCchCopyA( szDir, cchDir, sz );
        fIncremental=fI;
        fOverwrite = fO;
        fTruncate = fT;
    }
    virtual void PrintInfo(void) {PrintN(kelCmds, "Backup jet %s by %s existing copy to:%s\n", fIncremental?"incrementally":"completely", fOverwrite?"overwriting":"retaining",szDir);}
    virtual void Play(JetSession *pjs)
    {
        JET_GRBIT grb=0;
    DWORD     grbTestOptions = 0;

        if (this->fIncremental)
            grb |= JET_bitBackupIncremental;

        if (this->fOverwrite)
            grbTestOptions |= TEST_bitCleanBackup;

    if (!this->fTruncate)
            grbTestOptions |= TEST_bitSkipTruncate;

        TyphoonCmd::Play(pjs);
        JGBackup(grbTestOptions, szDir, grb, NULL);
    }
};


class TCRestore: public TyphoonCmd
{
protected:
    static const size_t cchDir  = _MAX_PATH;
    char szDir[cchDir];
public:
    TCRestore(void) {strcpy(szDir, "Restore");}
    TCRestore(char* sz)
    {
        StringCchCopyA( szDir, cchDir, sz );
    }
    virtual void PrintInfo(void) {PrintN(kelCmds, "Restore jet from:%s\n", szDir);}
    virtual void Play(JetSession *pjs) {TyphoonCmd::Play(pjs);JGRestore(szDir);}
};

class TCIdle: public TyphoonCmd
{
protected:
    long lIterations;
    BOOL fInfinite;
    BOOL fCompact;
public:
    TCIdle(long lIterations, BOOL fCompact=FALSE){this->fCompact = fCompact; this->fInfinite = !(this->lIterations = lIterations); }
    virtual void PrintInfo(void) {PrintN(kelCmds, "Idling...\n");}
    virtual void Play(JetSession *pjs)
    {
        JET_ERR err = JET_errSuccess;
        long lIterations = this->lIterations;

        TyphoonCmd::Play(pjs);
 //     JGSetSystemParameter(pjs->jsi, JET_paramOnLineCompact, JET_bitCompactOn, NULL);
        do
        {
            PrintN(kelProgress, "idle:%ld\n", lIterations);
            err = JetIdle(pjs->jsi, fCompact?JET_bitIdleCompact:0);
            if (!fInfinite)
                lIterations--;
        }
        while ((err == JET_errSuccess) && (lIterations || fInfinite));
    }
};

class TCSleep : public TyphoonCmd
{
protected:
    long cmsecSleep;
public:
    TCSleep(long cmsecSleep)
        : cmsecSleep( cmsecSleep ) {}
    virtual void PrintInfo(void) {PrintN(kelCmds, "Sleeping %d msec...\n", cmsecSleep);}
    virtual void Play(JetSession *pjs)
    {
        JET_ERR err = JET_errSuccess;

        TyphoonCmd::Play(pjs);

        PrintN(kelProgress, "sleep:%ld\n", this->cmsecSleep);
        Sleep( cmsecSleep );
    }
};

class TCTransaction: public TyphoonCmd
{
protected:
    long level;

public:
    virtual void PrintInfo(void) {PrintN(kelCmds, "Transaction Level:%ld\n", level);};
};


class TCBeginTransaction: public TCTransaction
{
public:
    virtual void PrintInfo(void) {PrintN(kelCmds, "JetBeginTransaction ");TCTransaction::PrintInfo();};
    virtual void Play(JetSession * pjs)
    {
        level = pjs->nTransactionLevel+1; // Print the new level
        TCTransaction::Play(pjs);
        pjs->BeginTransaction();
    }
};

class TCCommitTransaction: public TCTransaction
{
public:
    virtual void PrintInfo(void) {PrintN(kelCmds, "JetCommitTransaction ");TCTransaction::PrintInfo();};
    virtual void Play(JetSession * pjs)
    {
        level = pjs->nTransactionLevel;
        TCTransaction::Play(pjs);
        pjs->CommitTransaction();
    }

};

class TCRollback : public TCTransaction
{
public:
    virtual void PrintInfo(void) {PrintN(kelCmds, "JetRollback"); TCTransaction::PrintInfo();};
    virtual void Play(JetSession *pjs)
    {
        level = pjs->nTransactionLevel;
        TCTransaction::Play(pjs);
        pjs->RollbackTransaction();
    }
};

class TCHardExit : public TyphoonCmd
{
typedef TyphoonCmd super;

public:
    static BOOL fHardExit; // We use this flag to indicate to the try-exception block that the exception is expected
public:
    virtual void PrintInfo(void) {PrintN(kelCmds, "Hard Exit. Terminating shamelessly\n");};
    virtual void Play(JetSession *pjs);
};



class TCLongValueParam //: public TyphoonCmd
{
public:
    static long gLVMaxChunks;
    static long gLVMaxChunkSize;

    static void SetLVParameters(long mc, long mcs)
    {
        // Backward compatibility. These assignments have a global effect for subsequent long value assignments
        gLVMaxChunks = mc;
        gLVMaxChunkSize = mcs;
    }
// class will only be used for static members -SOMEONE
/*
protected:
    unsigned long ulMaxChunks, ulMaxChunkSize;

public:
    TCLongValueParam(long mc, long mcs)
    {
        ulMaxChunks = mc;
        ulMaxChunkSize = mcs;
        // Backward compatibility. These assignments have a global effect for subsequent long value assignments
        gLVMaxChunks = mc;
        gLVMaxChunkSize = mcs;
    }
    virtual void PrintInfo() {PrintN(kelCmds, "Long Value parameters, MaxChunks:%ld, MaxChunkSize:%ld\n", gLVMaxChunks, gLVMaxChunks);}
    virtual void Play(JetSession *pjs)
    {
        TyphoonCmd::Play(pjs);
        pjs->gLVMaxChunks = ulMaxChunks;
        pjs->gLVMaxChunkSize = ulMaxChunkSize;

        // Backward compatibility. These assignments have a global effect for subsequent long value assignments
        gLVMaxChunks = mc;
        gLVMaxChunkSize = mcs;
  }
*/
};


class TCCreateTables: public TyphoonCmd
{
private:
    long cb;
    unsigned long ulDensity;
    BOOL fClose;
    BOOL fBase;
    unsigned long   cbSeparateLV;
    JET_SPACEHINTS  jsphTable;
    JET_SPACEHINTS  jsphLV;

public:
    TCCreateTables(void) {TCCreateTables(0);}
    TCCreateTables(long cb, BOOL fClose=FALSE) {this->cb = cb; this->fClose=fClose;}
    TCCreateTables* SetDensity( unsigned long ulDensity ) { return SetSpaceHints( NULL, NULL, 0, ulDensity); }
    TCCreateTables* SetSpaceHints( JET_SPACEHINTS * pjsphTable, JET_SPACEHINTS * pjsphLV, unsigned long cbSeparateLV, unsigned long ulDensity ) {
        if ( pjsphTable )
            {
            this->jsphTable = *pjsphTable;
            }
        else
            {
            this->jsphTable.cbStruct = 0;
            }
        if ( pjsphLV )
            {
            this->jsphLV = *pjsphLV;
            }
        else
            {
            this->jsphLV.cbStruct = 0;
            }
        this->cbSeparateLV = cbSeparateLV;
        this->ulDensity = ulDensity; 
        return this; 
        }
    virtual void PrintInfo() {PrintN(kelCmds, "Create %ld NormalTables, all %s\n", cb, fClose?"closed":"open");}
    virtual void Play(JetSession *);
};

class TCCreateBaseTables: public TyphoonCmd
{
private:
    long cb;
    unsigned long ulDensity;
    long cbColumns;
    BOOL fClose;
    BOOL fBase;
    unsigned long   cbSeparateLV;
    JET_SPACEHINTS  jsphTable;
    JET_SPACEHINTS  jsphLV;

public:
    TCCreateBaseTables(long cb=0, long cbC=0, BOOL fClose=FALSE) {this->cb = cb; this->cbColumns = cbC; this->fClose=fClose;}
    TCCreateBaseTables* SetDensity( unsigned long ulDensity ) { return SetSpaceHints( NULL, NULL, 0, ulDensity); }
    TCCreateBaseTables* SetSpaceHints( JET_SPACEHINTS * pjsphTable, JET_SPACEHINTS * pjsphLV, unsigned long cbSeparateLV, unsigned long ulDensity ) {
        if ( pjsphTable )
            {
            this->jsphTable = *pjsphTable;
            }
        else
            {
            this->jsphTable.cbStruct = 0;
            }
        if ( pjsphLV )
            {
            this->jsphLV = *pjsphLV;
            }
        else
            {
            this->jsphLV.cbStruct = 0;
            }
        this->cbSeparateLV = cbSeparateLV;
        this->ulDensity = ulDensity; 
        return this; 
        }
    virtual void PrintInfo() {PrintN(kelCmds, "Create %ld Base Tables, all %s with %ld columns\n", cb, fClose?"closed":"open", this->cbColumns);}
    virtual void Play(JetSession *);
};

class TCDefragDatabase: public TyphoonCmd
{
private:
    static  BOOL    m_fDefragInProgress;
    unsigned long   m_cPasses;
    unsigned long   m_cSeconds;

public:
    TCDefragDatabase(void) {TCDefragDatabase(0, 0);}
    TCDefragDatabase(unsigned long cPasses, unsigned long cSeconds)
    {
        m_cPasses   = cPasses;
        m_cSeconds  = cSeconds;
    };
    virtual void PrintInfo()
    {
        if ( m_fDefragInProgress )
        {
            PrintN(kelCmds, "Stopping online defragmentation\n");
        }
        else
        {
            PrintN(kelCmds, "Starting online defragmentation, %lu passes, %lu seconds\n", m_cPasses, m_cSeconds);
        }
    };
    virtual void Play(JetSession *);
};

class TCResizeDatabase : public TyphoonCmd
{
private:
    unsigned long   m_cpgDesired;
    unsigned long   m_cpgActual;
    unsigned long   m_grbit;

public:
    TCResizeDatabase(void) {TCResizeDatabase(0, JET_bitNil);}
    TCResizeDatabase(unsigned long cpgDesired, JET_GRBIT grbit)
    {
        m_cpgDesired = cpgDesired;
        m_grbit = grbit;
    };
    virtual void PrintInfo()
    {
        PrintN(kelCmds, "Resizing database, cpageDesired = %d, grbit = %#x.\n", m_cpgDesired, m_grbit);
    };
    virtual void Play(JetSession *);
};

class TCRangeTableOps: public TyphoonCmd
{
protected:
    long from;
    long to;

public:
    TCRangeTableOps(void){}
    TCRangeTableOps(long s, long e) {from = s; to =e;}
    virtual void PrintInfo() {PrintN(kelCmds, " Tables from:%ld to:%ld\n", from, to);}
    virtual void Play(JetSession *pjs);
    virtual void Play(JetSession *pjs, PJTI pjti){}
};

class TCCreateHierTables : public TCRangeTableOps
{
private:
    long cb;
    unsigned long ulDensity;
    unsigned long   cbSeparateLV;
    JET_SPACEHINTS  jsphTable;
    JET_SPACEHINTS  jsphLV;

public:
    TCCreateHierTables(long s, long e, long c):TCRangeTableOps(s,e){this->cb = c;}
    TCCreateHierTables* SetDensity( unsigned long ulDensity ) { return SetSpaceHints( NULL, NULL, 0, ulDensity); }
    TCCreateHierTables* SetSpaceHints( JET_SPACEHINTS * pjsphTable, JET_SPACEHINTS * pjsphLV, unsigned long cbSeparateLV, unsigned long ulDensity ) {
        if ( pjsphTable )
            {
            this->jsphTable = *pjsphTable;
            }
        else
            {
            this->jsphTable.cbStruct = 0;
            }
        if ( pjsphLV )
            {
            this->jsphLV = *pjsphLV;
            }
        else
            {
            this->jsphLV.cbStruct = 0;
            }
        this->cbSeparateLV = cbSeparateLV;
        this->ulDensity = ulDensity; 
        return this; 
        }
    virtual void PrintInfo() {PrintN(kelCmds, "Create %ld Hierachial Tables", this->cb); TCRangeTableOps::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
};

class TCRenameTables: public TCRangeTableOps
{
public:
    TCRenameTables(long s, long e):TCRangeTableOps(s,e){}
    virtual void PrintInfo() {PrintN(kelCmds, "Rename"); TCRangeTableOps::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
};

class TCOpenTables: public TCRangeTableOps
{
public:
    TCOpenTables(long s, long e):TCRangeTableOps(s,e){}
    virtual void PrintInfo() {PrintN(kelCmds, "Open"); TCRangeTableOps::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
};

class TCCloseTables: public TCRangeTableOps
{
public:
    TCCloseTables(long s, long e):TCRangeTableOps(s,e){}
    virtual void PrintInfo() {PrintN(kelCmds, "Close"); TCRangeTableOps::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
};

class TCDeleteTables: public TCRangeTableOps
{
public:
    TCDeleteTables(long s, long e):TCRangeTableOps(s,e){}
    virtual void PrintInfo() {PrintN(kelCmds, "Delete"); TCRangeTableOps::PrintInfo();}
    virtual void Play(JetSession *pjs);
    virtual void Play(JetSession *pjs, PJTI pjti);
};

class TCComputeStats: public TCRangeTableOps
{
public:
    TCComputeStats(long s, long e):TCRangeTableOps(s,e){}
    virtual void PrintInfo() {PrintN(kelCmds, "Jet Compute Statistics"); TCRangeTableOps::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
};

class TCRangeTables: public TCRangeTableOps
{
public:
    TCRangeTables(long s, long e):TCRangeTableOps(s,e){}
    virtual void Play(JetSession *pjs);
    virtual void Play(JetSession *pjs, PJTI pjti){}
};


class TCCreateIndices: public TCRangeTables
{
private:
    long cb;

public:
    TCCreateIndices(long s, long e, long cb):TCRangeTables(s,e){this->cb = cb;}
    virtual void PrintInfo() {PrintN(kelCmds, "Create %ld Indices for", cb); TCRangeTables::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
};


class TCBookmarkRemember: public TCRangeTables
{
private:
    long cb;

public:
    TCBookmarkRemember(long s, long e, long c):TCRangeTables(s,e){this->cb = c;}
    virtual void PrintInfo() {PrintN(kelCmds, "Remember '%ld' Bookmarks for", this->cb); TCRangeTables::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
};

class TCBookmarkGoto: public TCRangeTables
{
public:
    TCBookmarkGoto(long s, long e):TCRangeTables(s,e){}
    virtual void PrintInfo() {PrintN(kelCmds, "Goto Bookmarks for"); TCRangeTables::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
};

class TCCreateSpecificIndices: public TCRangeTables
{
private:
    static const size_t cchIndices  = 1024;
    char szIndices[cchIndices];

public:
    TCCreateSpecificIndices(long s, long e, char* sz):TCRangeTables(s,e)
        {
            StringCchCopyA( szIndices, cchIndices, sz );
        }
    virtual void PrintInfo() {PrintN(kelCmds, "Create '%s' Indices for", szIndices); TCRangeTables::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
};


class TCRangeIndices: public TCRangeTables
{
protected:
    long to;
    long from;

public:
    TCRangeIndices(long ts, long te, long s, long e):TCRangeTables(ts, te){from=s; to=e;}
    virtual void PrintInfo() {PrintN(kelCmds, " Indices from:%ld to:%ld for", from, to); TCRangeTables::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
    virtual void Play(JetSession *pjs, PJTI pjti, PJII pjii){}
};

class TCSetIndices: public TCRangeTables
{
private:
    long iIndex;

public:
    TCSetIndices(long s, long e, long i=-1):TCRangeTables(s,e){iIndex = i;}
    virtual void PrintInfo() {PrintN(kelCmds, "Set Index"); TCRangeTables::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
};


class TCRenameIndices: public TCRangeIndices
{
public:
    TCRenameIndices(long ts, long te, long s, long e):TCRangeIndices(ts, te, s, e){}
    virtual void PrintInfo() {PrintN(kelCmds, "Rename"); TCRangeIndices::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti, PJII pjii);
};

class TCDeleteIndices: public TCRangeIndices
{
public:
    TCDeleteIndices(long ts, long te, long s, long e):TCRangeIndices(ts, te, s, e){}
    virtual void PrintInfo() {PrintN(kelCmds, "Delete"); TCRangeIndices::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
    virtual void Play(JetSession *pjs, PJTI pjti, PJII pjii);
};

class TCCreateColumns: public TCRangeTables
{
private:
    long cb;

public:
    TCCreateColumns(long s, long e, long cb):TCRangeTables(s,e){this->cb = cb;}
    virtual void PrintInfo() {PrintN(kelCmds, "Create %ld Columns for", cb); TCRangeTables::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
};

class TCCreateSpecificColumns: public TCRangeTables
{
private:
    static const size_t cchCols = 1024;
    char szCols[cchCols];

public:
    TCCreateSpecificColumns(long s, long e, char* sz):TCRangeTables(s,e)
    {
        StringCchCopyA( szCols, cchCols, sz );
    }
    virtual void PrintInfo() {PrintN(kelCmds, "Create '%s' Columns for", szCols); TCRangeTables::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
};


class TCMoveVerify: public TCRangeTables
{
private:
    long cMoves;

public:
    TCMoveVerify(long ts, long te, long cb):TCRangeTables(ts, te){this->cMoves = cb;}
    virtual void PrintInfo() {PrintN(kelCmds, "No. of verifications through moves == %ld for", cMoves); TCRangeTables::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
};


class TCSeekVerify : public TCRangeTables
{
private:
    long cSeeks;

public:
    TCSeekVerify(long ts, long te, long cr):TCRangeTables(ts, te){cSeeks = cr;}
    virtual void PrintInfo() {PrintN(kelCmds, "SeekVerify %ld records", cSeeks); TCRangeTables::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
};


class TCCreateRecords: public TCRangeTables
{
private:
    long cRecords;

public:
    TCCreateRecords(long ts, long te, long cr):TCRangeTables(ts, te){cRecords = cr;}
    virtual void PrintInfo() {PrintN(kelCmds, "Create %ld Records", cRecords); TCRangeTables::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
};

class TCEscrowUpdates: public TCRangeTables
{
private:
    long cRecords;

public:
    TCEscrowUpdates(long ts, long te, long cr):TCRangeTables(ts, te){cRecords = cr;}
    virtual void PrintInfo() {PrintN(kelCmds, "Escrow Update %ld Records", cRecords); TCRangeTables::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
};


class TCCreateRandomRecords: public TCRangeTables
{
public:
    typedef enum
    {
        crrAllOperations,
        crrInsertOnly,
        crrReplaceOnly
    }CRRCode;

private:
    long cRecords;
    long cMoveGranularity;
    CRRCode crrCode;

public:
    TCCreateRandomRecords(long ts, long te, long cr, long cmg, CRRCode crr=crrAllOperations):TCRangeTables(ts, te){cRecords = cr; cMoveGranularity = cmg; crrCode = crr; }
    virtual void PrintInfo() {PrintN(kelCmds, "Create randomly %ld Records", cRecords); TCRangeTables::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
};


class TCDeleteRandomRecords: public TCRangeTables
{
private:
    long cMoveGranularity;
    long cDelete;

public:
    TCDeleteRandomRecords(long ts, long te, long cd, long cmg=0):TCRangeTables(ts, te){cDelete = cd; cMoveGranularity = cmg;}
    virtual void PrintInfo() {PrintN(kelCmds, "Delete %ld Records Randomly, granularity=%ld", cDelete, cMoveGranularity); TCRangeTables::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
};


class TCRangeRecords: public TCRangeTables
{
private:
    long to;
    long from;

public:
    TCRangeRecords(long ts, long te, long s, long e):TCRangeTables(ts, te){from=s; to=e;}
    virtual void PrintInfo() {PrintN(kelCmds, " Records from:%ld to:%ld for",from,  to); TCRangeTables::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
    virtual void Play(JetSession *pjs, PJTI pjti, long from, long to)=0;
};

class TCDeleteRecords: public TCRangeRecords
{
public:
    TCDeleteRecords(long ts, long te, long s, long e):TCRangeRecords(ts, te, s, e){}
    virtual void PrintInfo() {PrintN(kelCmds, "Delete"); TCRangeRecords::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti, long from, long to);
};



/*
class TCCreateColumnsTo: public TCRangeTables
{
private:
    long cb;

public:
    TCCreateColumnsTo(long s, long e, long cb):TCRangeTables(s,e){this->cb = cb;}
    virtual void PrintInfo() {PrintN(kelCmds, "Create upto %ld Columns for", cb); TCRangeTables::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
};
*/

class TCRangeColumns: public TCRangeTables
{
protected:
    long to;
    long from;

public:
    TCRangeColumns(long ts, long te, long s, long e):TCRangeTables(ts, te){from=s; to=e;}
    virtual void PrintInfo() {PrintN(kelCmds, " Columns from:%ld to:%ld for", from, to); TCRangeTables::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
    virtual void Play(JetSession *pjs, PJTI pjti, PJCI pjci){}
};

class TCRenameColumns: public TCRangeColumns
{
public:
    TCRenameColumns(long ts, long te, long s, long e):TCRangeColumns(ts, te, s, e){}
    virtual void PrintInfo() {PrintN(kelCmds, "Rename"); TCRangeColumns::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti, PJCI pjci);
};

class TCDeleteColumns: public TCRangeColumns
{
public:
    TCDeleteColumns(long ts, long te, long s, long e):TCRangeColumns(ts, te, s, e){}
    virtual void PrintInfo() {PrintN(kelCmds, "Delete"); TCRangeColumns::PrintInfo();}
    virtual void Play(JetSession *pjs, PJTI pjti);
//    virtual void Play(JetSession *pjs, PJTI pjti, PJCI pjci);
};


