// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "collectionunittest.hxx"

class ObjectPoolTest : public UNITTEST
{
    private:
        static ObjectPoolTest s_instance;

    protected:
        ObjectPoolTest() {}
    public:
        ~ObjectPoolTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const { return true; }
        bool FRunUnderESENT() const { return true; }
        bool FRunUnderESE97() const { return true; }

        ERR ErrTest();

    private:
        struct TestStruct
        {
            LONG            l;
            double          d;
            BYTE            b;
        };
        
        ERR ErrAllocateOneObject();
        ERR ErrReallocateObject();
        ERR ErrAllocateTwoObjects();
        ERR ErrAllocateManyObjects();
};

ObjectPoolTest ObjectPoolTest::s_instance;

const char * ObjectPoolTest::SzName() const     { return "ObjectPool tests"; };
const char * ObjectPoolTest::SzDescription() const
{
    return "Tests the ObjectPool class";
}

ERR ObjectPoolTest::ErrAllocateOneObject()
{
    ERR err = JET_errSuccess;
    
    ObjectPool<TestStruct, 2> pool;
    TestStruct * const pstruct = pool.Allocate();
    TestCheck(NULL != pstruct);
    pool.Free(pstruct);

HandleError:
    return err;
}

ERR ObjectPoolTest::ErrReallocateObject()
{
    ERR err = JET_errSuccess;
    
    ObjectPool<TestStruct, 2> pool;
    TestStruct * const pstruct1 = pool.Allocate();
    pool.Free(pstruct1);
    TestStruct * const pstruct2 = pool.Allocate();
    TestCheck(pstruct1 == pstruct2);
    pool.Free(pstruct2);

HandleError:
    return err;
}

ERR ObjectPoolTest::ErrAllocateTwoObjects()
{
    ERR err = JET_errSuccess;
    
    ObjectPool<TestStruct, 2> pool;
    TestStruct * const pstruct1 = pool.Allocate();
    TestStruct * const pstruct2 = pool.Allocate();
    TestCheck(pstruct1 != pstruct2);
    pool.Free(pstruct1);
    pool.Free(pstruct2);

HandleError:
    return err;
}

ERR ObjectPoolTest::ErrAllocateManyObjects()
{
    ERR err = JET_errSuccess;
    
    const INT cstructs = 5;
    ObjectPool<TestStruct, cstructs-1> pool;
    TestStruct * rgpstructs[cstructs];
    for(INT i = 0; i < cstructs; ++i)
    {
        rgpstructs[i] = pool.Allocate();
        TestCheck(NULL != rgpstructs[i]);
    }
    for(INT i = 0; i < cstructs; ++i)
    {
        pool.Free(rgpstructs[i]);
    }   

HandleError:
    return err;
}

ERR ObjectPoolTest::ErrTest()
{   
    if( JET_errSuccess == ErrAllocateOneObject()
        && JET_errSuccess == ErrReallocateObject()
        && JET_errSuccess == ErrAllocateTwoObjects()
        && JET_errSuccess == ErrAllocateManyObjects() )
    {
        return JET_errSuccess;
    }
    return JET_errInvalidParameter;
}


