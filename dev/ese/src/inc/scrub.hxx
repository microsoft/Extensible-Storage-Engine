// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

BOOL FSCANSystemTable( const CHAR * const szTableName );

ERR ErrIsamDatabaseScan(
    const JET_SESID         vsesid,
    const JET_DBID          vdbid,
    __inout_opt ULONG * const   pcsec,
    const ULONG             cmsecSleep,
    const JET_CALLBACK      pfnCallback,
    const JET_GRBIT         grbit );

#ifdef MINIMAL_FUNCTIONALITY

//  ================================================================
class DATABASESCANNER
//  ================================================================
//
//  A thunked out, empty class to support references to the 
//  DATABASESCANNER.
//
//-
{
    public:
        DATABASESCANNER()   { }
        ~DATABASESCANNER()  { }
        static VOID TermInst( const INST * const pinst ) { }
};

#else

ERR ErrSCANDumpMSysScan( __in PIB * const ppib, const IFMP ifmp );

//  ================================================================
struct OBJIDINFO
//  ================================================================
{
    OBJID   objidFDP;
    PGNO    pgnoFDP;
    BOOL    fUnique;

    static BOOL CmpObjid( const OBJIDINFO&, const OBJIDINFO& );
};

//  ================================================================
struct SCRUBCONSTANTS
//  ================================================================
{
    DBTIME      dbtimeLastScrub;
    
    CPRINTF     *pcprintfVerbose;
    CPRINTF     *pcprintfDebug;

    const OBJIDINFO *pobjidinfo;    //  information on objid's used in database
    LONG        cobjidinfo;         //  count of OBJIDINFO structures
    OBJID       objidMax;           //  all objid's >= this are ignored
};


//  ================================================================
struct SCRUBSTATS
//  ================================================================
{
    LONG        err;                //  error condition from the first thread to encounter an error

    LONG        cpgSeen;            //  total pages seen
    LONG        cpgUnused;          //  zeroed pages we encountered
    LONG        cpgUnchanged;       //  pages with dbtime < dbtimeLastScrub
    LONG        cpgZeroed;          //  pages we zeroed out completely
    LONG        cpgUsed;            //  pages being used by the database
    LONG        cpgUnknownObjid;    //  page whose objid >= objidMax
    LONG        cNodes;             //  total leaf nodes seen (including flag-deleted)
    LONG        cFlagDeletedNodesZeroed;    //  flag deleted nodes we zeroed
    LONG        cFlagDeletedNodesNotZeroed; //  flag deleted nodes not zeroed because of versions
    LONG        cOrphanedLV;        //  orphaned LVs we zeroed
    LONG        cVersionBitsReset;  //  version store bits removed from nodes
};


//  ================================================================
struct SCRUBCONTEXT
//  ================================================================
{
    const SCRUBCONSTANTS * pconstants;
    SCRUBSTATS * pstats;
};


//  ================================================================
class SCRUBTASK : public DBTASK
//  ================================================================
{
    public:
        SCRUBTASK( const IFMP ifmp, const PGNO pgnoFirst, const CPG cpg, const SCRUBCONTEXT * const pcontext );
        virtual ~SCRUBTASK();
        
        virtual ERR ErrExecuteDbTask( PIB * const ppib );
        VOID HandleError( const ERR err );

    protected:
        ERR ErrProcessPage_(
            PIB * const ppib,
            CSR * const pcsr );
        ERR ErrProcessUnusedPage_(
            PIB * const ppib,
            CSR * const pcsr );
        ERR ErrProcessUsedPage_(
            PIB * const ppib,
            CSR * const pcsr );

        ERR FNodeHasVersions_( const OBJID objidFDP, const KEYDATAFLAGS& kdf, const TRX trxOldest ) const;
        
    protected:
        const PGNO      m_pgnoFirst;
        const CPG       m_cpg;
        const SCRUBCONTEXT  * const m_pcontext;

        const OBJIDINFO     * m_pobjidinfoCached;
        OBJID           m_objidNotFoundCached;
        
    private:
        SCRUBTASK( const SCRUBTASK& );
        SCRUBTASK& operator=( const SCRUBTASK& );
};
    

//  ================================================================
class SCRUBDB
//  ================================================================
{
    public:
        SCRUBDB( const IFMP ifmp );
        ~SCRUBDB();

        ERR ErrInit( PIB * const ppib, const INT cThreads );
        ERR ErrTerm();

        ERR ErrScrubPages( const PGNO pgnoFirst, const CPG cpg );

        volatile const SCRUBSTATS& Scrubstats() const;
        
    private:
        SCRUBDB( const SCRUBDB& );
        SCRUBDB& operator=( const SCRUBDB& );
        
    private:
        const IFMP  m_ifmp;
        TASKMGR     m_taskmgr;

        SCRUBCONSTANTS  m_constants;
        SCRUBSTATS      m_stats;
        SCRUBCONTEXT    m_context;
};


ERR ErrDBUTLScrub( JET_SESID sesid, const JET_DBUTIL_W *pdbutil );
ERR ErrSCRUBZeroLV( PIB * const ppib,
                    const IFMP ifmp,
                    CSR * const pcsr,
                    const INT iline );

#endif  // !MINIMAL_FUNCTIONALITY
