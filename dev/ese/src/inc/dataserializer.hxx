// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  ================================================================
class DataBinding
//  ================================================================
//
//  A data binding specifies how a variable can be serialized. It
//  provides a column name, column type and the ability to set/get
//  the value from pv/cb pairs.
//
//-
{
public:
    virtual ~DataBinding() {}

    // name of the column to store the information in
    const char * const SzColumn() const { return m_szColumn; }

    // type of the column
    virtual JET_COLTYP Coltyp() const = 0;

    // Get the contents of the bound variable as a pv/cb pair,
    // suitable for storage
    virtual void GetPvCb(__out const void ** ppv, __out size_t * pcb) const = 0;

    // Set the bound variable from a stored pv/cb pair
    virtual ERR ErrSetFromPvCb(const void * const pv, const size_t cb) = 0;

    // Set the bound variable to its default value
    virtual void SetToDefault() = 0;

    // Print the data binding
    void Print(CPRINTF * const pcprintf) const;


protected:
    DataBinding(const char * const szColumn) : m_szColumn(szColumn) {}

private:
    const char * const m_szColumn;
};


//  ================================================================
template<class T>
class DataBindingOf : public DataBinding
//  ================================================================
//
//  Data binding for a specific type. This converts between the 
//  type T and pv/cb pairs.
//
//  This template can be used for simple data types (int, long). There
//  is a separate specialization for strings.
//
//-
{
public:
    // Create an object/name binding
    DataBindingOf(T * const pt, const char * const szColumn) :
        DataBinding(szColumn),
        m_pt(pt)
    {
        Assert(m_pt);
        Assert(SzColumn());
    }
    virtual ~DataBindingOf() {}

    // this is the default column type, it can be specialized
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

// Specialized templates for different types of bindings

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


//  ================================================================
template<>
class DataBindingOf<char*> : public DataBinding
//  ================================================================
//
//  DataBindingOf specialization for strings.
//
//-
{
public:
    // Create a char* binding to the given variable with the given maximum size.
    // The maximum size includes the terminating NULL
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
        if(cb >= m_cchMax)  // need space for a NULL terminator
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


//  ================================================================
class DataBindings
//  ================================================================
//
//  This is a set of column bindings, which can be iterated over.
//
//-
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


//  ================================================================
class IDataStore
//  ================================================================
//
//  An interface which set and retrieve pv/cb pairs by name.
//
//-
{
public:
    virtual ~IDataStore() {}

    // DDL
    virtual ERR ErrColumnExists(
        const char * const szColumn,
        __out bool * const pfExists,
        __out JET_COLTYP * const pcoltyp) const = 0;
    virtual ERR ErrCreateColumn(const char * const szColumn, const JET_COLTYP coltyp) = 0;

    // DML

    // Get the column data. Returns JET_wrnColumnNull if the column isn't present or has no data
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


//  ================================================================
class DataSerializer
//  ================================================================
//
//  Serialize/deserialize a set of variables. The variables, and their
//  serialization mechanisms are specified by the given DataBindings.
//  Storage for the serialized data is given by the IDataStore object.
//  
//-
{
public:
    // Create a serializer to serialize/deserialize the given bindings
    DataSerializer(const DataBindings& bindings);
    ~DataSerializer();

    // Sets all bindings to default values
    void SetBindingsToDefault();
    
    // Save the bindings to the given store
    ERR ErrSaveBindings(IDataStore * const pstore);

    // Load the bindings from the given store
    ERR ErrLoadBindings(const IDataStore * const pstore);

    // Print all the variables
    void Print(CPRINTF * const pcprintf);

private:
    ERR ErrCreateAllColumns_(IDataStore * const pstore);
    
private:
    const DataBindings& m_bindings;
};


//  ================================================================
namespace TableDataStoreFactory
//  ================================================================
//
//  Used to create a TableDataStore. The table is created if necessary
//  and a new record inserted if it is empty. A new SESID and TABLEID
//  are created, which are closed in the destructor of the
//  TableDataStore.
//
//-
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


//  ================================================================
class MemoryDataStore : public IDataStore
//  ================================================================
//
//  Used for testing serialization. This simply stores data to an
//  in-memory object.
//
//-
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
    // find the index of the given column, -1 if the column doesn't exist
    INT IColumn(const char * const szColumn) const;
    
private:
    bool m_fInUpdate;
    static const INT m_ccolumnsMax = 64;
    const char * m_rgszColumns[m_ccolumnsMax];
    JET_COLTYP m_rgcoltyps[m_ccolumnsMax];
    unique_ptr<BYTE> m_rgpbData[m_ccolumnsMax];
    size_t m_rgcbData[m_ccolumnsMax];
    INT m_ccolumns;

private: // not implemented
    MemoryDataStore(const MemoryDataStore&);
    MemoryDataStore& operator=(const MemoryDataStore&);
};

