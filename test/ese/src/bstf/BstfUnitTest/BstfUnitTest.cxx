// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <cstdio>

#include "testerr.h"
#include "bstf.hxx"

void TestReportErr( long err, unsigned long ulLine, const char *szFileName )
    {
    printf( "error %d at line %d of %s \r\n", err, ulLine, szFileName );
    }

UNITTEST * UNITTEST::s_punittestHead;

UNITTEST::UNITTEST()
    {
    m_punittestNext = s_punittestHead;
    s_punittestHead = this;
    }

UNITTEST::~UNITTEST()
    {
    }

