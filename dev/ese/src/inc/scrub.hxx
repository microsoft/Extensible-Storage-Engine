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

class DATABASESCANNER
{
    public:
        DATABASESCANNER()   { }
        ~DATABASESCANNER()  { }
        static VOID TermInst( const INST * const pinst ) { }
};

#else

ERR ErrSCANDumpMSysScan( __in PIB * const ppib, const IFMP ifmp );

struct OBJIDINFO
{
    OBJID   objidFDP;
    PGNO    pgnoFDP;
    BOOL    fUnique;

    static BOOL CmpObjid( const OBJIDINFO&, const OBJIDINFO& );
};

struct SCRUBCONSTANTS
{
    DBTIME      dbtimeLastScrub;
    
    CPRINTF     *pcprintfVerbose;
    CPRINTF     *pcprintfDebug;

    const OBJIDINFO *pobjidinfo;
    LONG        cobjidinfo;
    OBJID       objidMax;
};


struct SCRUBSTATS
{
    LONG        err;

    LONG        cpgSeen;
    LONG        cpgUnused;
    LONG        cpgUnchanged;
    LONG        cpgZeroed;
    LONG        cpgUsed;
    LONG        cpgUnknownObjid;
    LONG        cNodes;
    LONG        cFlagDeletedNodesZeroed;
    LONG        cFlagDeletedNodesNotZeroed;
    LONG        cOrphanedLV;
    LONG        cVersionBitsReset;
};


struct SCRUBCONTEXT
{
    const SCRUBCONSTANTS * pconstants;
    SCRUBSTATS * pstats;
};


class SCRUBTASK : public DBTASK
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
    

class SCRUBDB
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

#endif
