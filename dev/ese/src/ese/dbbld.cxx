// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <std.hxx>

LOCAL const BYTE  mskDbBuild = fDbBuildEsentAttached | fDbBuildEseAttached | fDbBuildPrivate;

LOCAL PCSTR g_mpszBldState[ mskDbBuild + 1 ] = {
    "",        //  no flags - pre EFV 9400.
    " OS",     //  #ifdef ESENT - OS build (Windows built-in esent.dll)
    " SB",     //  #ifndef ESENT - SuBstrate build or Separate or Storage Build  (built as ese.dll generally)
    " OS SB",
    " HH UV",  // should not be possible.
    " OS UV",
    " SB UV",
    " OS SB UV",
};

void DBFILEHDR::SetBuildState()
{
    #if !defined( OFFICIAL_BUILD )
    m_rgbDbFlags[3] |= fDbBuildPrivate;
    #endif

    m_rgbDbFlags[3] |=
#ifdef ESENT
        fDbBuildEsentAttached;
#else
        fDbBuildEseAttached;
#endif

    //  other misc. build options that are built in - vis. to debugger ext only

    m_rgbDbFlags[3] |= fDbExternalExtentCacheBuildOption;
}

PCSTR DBFILEHDR::SzBuildFlags() const
{
    static_assert( _countof( g_mpszBldState ) == 8, "DbBldMskArrayOff" );
    BYTE bDbBuildFlags = m_rgbDbFlags[3] & mskDbBuild;
    if ( bDbBuildFlags < _countof( g_mpszBldState ) )
    {
        return g_mpszBldState[ bDbBuildFlags ];
    }

    return " UV";
}
