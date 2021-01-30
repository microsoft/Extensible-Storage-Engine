// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

class DataBinding
{
public:
    virtual ~DataBinding() {}

    const char * const SzColumn() const { return m_szColumn; }

    virtual JET_COLTYP Coltyp() const = 0;

    virtual void GetPvCb(__out const void ** ppv, __out size_t * pcb) const = 0;

    virtual ERR ErrSetFromPvCb(const void * const pv, const size_t cb) = 0;

    virtual void SetToDefault() = 0;

    void Print(CPRINTF * const pcprintf) const;


protected:
    DataBinding(const char * const szColumn) : m_szColumn(szColumn) {}

private:
    const char * const m_szColumn;
};


template<class T>
class DataBindingOf : public DataBinding
{
public:
    DataBindingOf(T * const pt, const char * const szColumn) :
        DataBinding(szColumn),
        m_pt(pt)
    {
        Assert(m_pt);
        Assert(SzColumn());
    }
    virtual ~DataBindingOf() {}

    virtual JET_COLTYP Coltyp() const { return JET_coltypLongBinary; }

    virtual void GetPvCb(__out const void ** ppv, __out size_t * pcb) const
    {
        Assert(ppv);
        Assert(pcb);
        *ppv = m_pt;
        *pcb = sizeof(T);
    }

    virtual ERR ErrSetFromPvCb(const void * const pv, const size_t cb)
    {
        Assert(pv);
        
        ERR err = JET_errSuccess;
        if(cb != sizeof(T))
        {
            Call(ErrERRCheck(JET_errInvalidBufferSize));
        }
        *m_pt = *((const T *)pv);

    HandleError:
        return err;
    }

    virtual void SetToDefault()
    {
        *m_pt = T();
    }
    
private:
    T * const m_pt;
};


template<>
JET_COLTYP DataBindingOf<unsigned char>::Coltyp() const { return JET_coltypUnsignedByte; }

template<>
JET_COLTYP DataBindingOf<SHORT>::Coltyp() const { return JET_coltypShort; }

template<>
JET_COLTYP DataBindingOf<USHORT>::Coltyp() const { return JET_coltypUnsignedShort; }

template<>
JET_COLTYP DataBindingOf<INT>::Coltyp() const { return JET_coltypLong; }

template<>
JET_COLTYP DataBindingOf<UINT>::Coltyp() const { return JET_coltypUnsignedLong; }

template<>
JET_COLTYP DataBindingOf<LONG>::Coltyp() const { return JET_coltypLong; }

template<>
JET_COLTYP DataBindingOf<ULONG>::Coltyp() const { return JET_coltypUnsignedLong; }

template<>
JET_COLTYP DataBindingOf<__int64>::Coltyp() const { return JET_coltypLongLong; }


template<>
class DataBindingOf<char*> : public DataBinding
{
public:
    DataBindingOf(char * const sz, const size_t cchMax, const char * const szColumn) :
        DataBinding(szColumn),
        m_sz(sz),
        m_cchMax(cchMax)
    {
        Assert(m_sz);
        Assert(0 != m_cchMax);
        Assert(SzColumn());
    }
    virtual ~DataBindingOf() {}
    
    virtual JET_COLTYP Coltyp() const { return JET_coltypLongText; }

    virtual void GetPvCb(__out const void ** ppv, __out size_t * pcb) const
    {
        Assert(ppv);
        Assert(pcb);
        *ppv = m_sz;
        (VOID) StringCbLengthA(m_sz, m_cchMax, pcb);
    }

    virtual ERR ErrSetFromPvCb(const void * const pv, const size_t cb)
    {
        Assert(pv);

        ERR err = JET_errSuccess;
        if(cb >= m_cchMax)
        {
            Call(ErrERRCheck(JET_errInvalidBufferSize));
        }
        
        memcpy(m_sz, pv, cb);
        m_sz[cb] = 0;

    HandleError:
        return err;
    }

    virtual void SetToDefault()
    {
        m_sz[0] = 0;
    }
    
private:
    char * const m_sz;
    const size_t m_cchMax;
};


class DataBindings
{
public:
    DataBindings();
    ~DataBindings();

    void AddBinding(DataBinding * const pbinding);

    typedef DataBinding* const * iterator;
    iterator begin() const;
    iterator end() const;

private:
    static const INT m_cbindingsMax = 64;
    INT m_cbindings;
    DataBinding* m_rgpbindings[m_cbindingsMax];
};


class IDataStore
{
public:
    virtual ~IDataStore() {}

    virtual ERR ErrColumnExists(
        const char * const szColumn,
        __out bool * const pfExists,
        __out JET_COLTYP * const pcoltyp) const = 0;
    virtual ERR ErrCreateColumn(const char * const szColumn, const JET_COLTYP coltyp) = 0;


    virtual ERR ErrLoadDataFromColumn(
        const char * const szColumn,
        __out void * pv,
        __out size_t * const pcbActual,
        const size_t cbMax) const = 0;
    virtual ERR ErrStoreDataToColumn(const char * const szColumn, const void * const pv, const size_t cb) = 0;

    virtual ERR ErrPrepareUpdate() = 0;
    virtual ERR ErrCancelUpdate() = 0;
    virtual ERR ErrUpdate() = 0;
    ERR ErrDataStoreUnavailable() const { return m_errDataStoreUnavailable; }
    
protected:
    IDataStore() : m_errDataStoreUnavailable(JET_errSuccess) { }
    ERR m_errDataStoreUnavailable;
};


class DataSerializer
{
public:
    DataSerializer(const DataBindings& bindings);
    ~DataSerializer();

    void SetBindingsToDefault();
    
    ERR ErrSaveBindings(IDataStore * const pstore);

    ERR ErrLoadBindings(const IDataStore * const pstore);

    void Print(CPRINTF * const pcprintf);

private:
    ERR ErrCreateAllColumns_(IDataStore * const pstore);
    
private:
    const DataBindings& m_bindings;
};


namespace TableDataStoreFactory
{
    ERR ErrOpenOrCreate(
        INST * const pinst,
        const wchar_t * wszDatabase,
        const char * const szTable,
        IDataStore ** ppstore);

    ERR ErrOpenExisting(
        INST * const pinst,
        const wchar_t * wszDatabase,
        const char * const szTable,
        IDataStore ** ppstore);
}


class MemoryDataStore : public IDataStore
{
public:
    MemoryDataStore();
    ~MemoryDataStore();

    ERR ErrColumnExists(
        const char * const szColumn,
        __out bool * const pfExists,
        __out JET_COLTYP * const pcoltyp) const;
    ERR ErrCreateColumn(const char * const szColumn, const JET_COLTYP coltyp);

    ERR ErrLoadDataFromColumn(
        const char * const szColumn,
        __out void * pv, __out size_t * const pcbActual,
        const size_t cbMax) const;

    ERR ErrPrepareUpdate();
    ERR ErrStoreDataToColumn(const char * const szColumn, const void * const pv, const size_t cb);
    ERR ErrCancelUpdate();
    ERR ErrUpdate();

private:
    INT IColumn(const char * const szColumn) const;
    
private:
    bool m_fInUpdate;
    static const INT m_ccolumnsMax = 64;
    const char * m_rgszColumns[m_ccolumnsMax];
    JET_COLTYP m_rgcoltyps[m_ccolumnsMax];
    unique_ptr<BYTE> m_rgpbData[m_ccolumnsMax];
    size_t m_rgcbData[m_ccolumnsMax];
    INT m_ccolumns;

private:
    MemoryDataStore(const MemoryDataStore&);
    MemoryDataStore& operator=(const MemoryDataStore&);
};

