// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_CPRINTF_HXX_INCLUDED
#define _OS_CPRINTF_HXX_INCLUDED


#include <stdarg.h>


//  ==================================================================================================================
class CPRINTF
//  ==================================================================================================================
{
    public:
        CPRINTF() {}
        virtual ~CPRINTF() {}

        static void SetThreadPrintfPrefix( __in const _TCHAR * szPrefix );

    public:
        virtual void __cdecl operator()( const _TCHAR* szFormat, ... ) = 0;
};

//  ==================================================================================================================
class CPRINTFNULL : public CPRINTF
//  ==================================================================================================================
{
    public:
        void __cdecl operator()( const _TCHAR* szFormat, ... );
        static CPRINTF* PcprintfInstance();
};

//  ================================================================
INLINE void __cdecl CPRINTFNULL::operator()( const _TCHAR* szFormat, ... )
//  ================================================================
{
    va_list arg_ptr;
    va_start( arg_ptr, szFormat );
    va_end( arg_ptr );
}

//  ================================================================
INLINE CPRINTF* CPRINTFNULL::PcprintfInstance()
//  ================================================================
{
    extern CPRINTFNULL g_cprintfNull;
    return &g_cprintfNull;
}


//  ==================================================================================================================
class CPRINTFDBGOUT : public CPRINTF
//  ==================================================================================================================
//
//  Calls OutputDebugString()
//
//-
{
    public:
        void __cdecl operator()( const _TCHAR* szFormat, ... );
        static CPRINTF* PcprintfInstance();
};


//  ==================================================================================================================
class CPRINTFSTDOUT : public CPRINTF
//  ==================================================================================================================
{
    public:
        void __cdecl operator()( const _TCHAR* szFormat, ... );
        static CPRINTF* PcprintfInstance();
};

//  ================================================================
INLINE CPRINTF* CPRINTFSTDOUT::PcprintfInstance()
//  ================================================================
{
    extern CPRINTFSTDOUT g_cprintfStdout;
    return &g_cprintfStdout;
}

//  ================================================================
INLINE void __cdecl CPRINTFSTDOUT::operator()( const _TCHAR* szFormat, ... )
//  ================================================================
{
    va_list arg_ptr;
    va_start( arg_ptr, szFormat );
    _vtprintf( szFormat, arg_ptr );
    va_end( arg_ptr );
}


//  ==================================================================================================================
class CPRINTINTRINBUF : public CPRINTF
//  ==================================================================================================================
{
    public:
        CPRINTINTRINBUF();
    
        virtual void Reset();
        virtual BOOL FContains( _In_z_ const CHAR * const szFind );
        virtual ULONG CContains( _In_z_ const CHAR * const szFind );

        void Print( CPRINTF & pcprintf );

        void __cdecl operator()( const _TCHAR* szFormat, ... );

    private:
        //  probably should templatize this buffer size.
        const static ULONG  s_cchBuffer = 2048;
        CHAR                m_rgchBuffer[ s_cchBuffer ];
        QWORD               m_cyichAppendMax;  // CYclic Index count CHaracaters ... as in the real index is % buffer size.

        void Append_( const CHAR * const szPrint );

        ULONG IchAppendNext_() const
        {
            QWORD qw = m_cyichAppendMax % s_cchBuffer;
            Assert( qw < (QWORD)ulMax );
            return (ULONG)qw;
        }

    public: // private but unit test needs access

        class BUFCURSOR
        {
        private:
            const CPRINTINTRINBUF * const   m_pprtbuf;
            QWORD                           m_ichCurrency;
            QWORD                           m_ichNext;

            ULONG IchReadNext_() const
            {
                return m_ichNext % s_cchBuffer;
            }

        public:

            BUFCURSOR( const CPRINTINTRINBUF * const pprtbuf ) :
                m_pprtbuf( pprtbuf )
            {

                Assert( m_pprtbuf->m_rgchBuffer[ m_pprtbuf->IchAppendNext_() ] == '\0' );
                m_ichCurrency = m_pprtbuf->m_cyichAppendMax;

                if ( m_ichCurrency < s_cchBuffer )
                {
                    m_ichNext = 0;
                }
                else
                {
                    ULONG ichExpected = ULONG( m_ichCurrency % s_cchBuffer );
                    m_ichNext = m_ichCurrency - s_cchBuffer;
                    Assert( ichExpected == IchReadNext_() );
                }
            }

            const CHAR * SzNext()
            {
                // Check currency ... if it moved, we have to write some more clever code
                // to ensure we haven't overwritten the next string we would return.  And
                // maybe even some locking as well.
                Assert( m_ichCurrency == m_pprtbuf->m_cyichAppendMax );

                // If we're on a valid character / non-NUL of the current string, increment to try to get to the end.
                while( m_pprtbuf->m_rgchBuffer[ IchReadNext_() ] != '\0' && m_ichNext < m_ichCurrency )
                {
                    m_ichNext++;
                }

                // We are at the end of current string, increment one more time to move to next string (if there is more data).
                while( m_pprtbuf->m_rgchBuffer[ IchReadNext_() ] == '\0' && m_ichNext < m_ichCurrency )
                {
                    m_ichNext++;
                }

                return m_pprtbuf->m_rgchBuffer[ IchReadNext_() ] == '\0' ? NULL : &( m_pprtbuf->m_rgchBuffer[ IchReadNext_() ] );
            }
        };

        friend class BUFCURSOR;
};

//  ================================================================
INLINE CPRINTINTRINBUF::CPRINTINTRINBUF()
//  ================================================================
{
    Reset();
}

//  ================================================================
INLINE void CPRINTINTRINBUF::Reset()
//  ================================================================
{
    // We can get away with this instead of full memset(), as long as m_cyichAppendMax = 0.
    m_rgchBuffer[ 0 ] = '\0';
    m_cyichAppendMax = 0;
}

//  ================================================================
INLINE void __cdecl CPRINTINTRINBUF::operator()( const _TCHAR* szFormat, ... )
//  ================================================================
{
    CHAR rgchBuf[ 1024 ];

    va_list arg_ptr;
    va_start( arg_ptr, szFormat );
    StringCbVPrintfA( rgchBuf, sizeof( rgchBuf ), (CHAR*)szFormat, arg_ptr );
    va_end( arg_ptr );

    Append_( rgchBuf );
}

//  ================================================================
INLINE void CPRINTINTRINBUF::Append_( const CHAR * const szPrint )
//  ================================================================
{
    Assert( IchAppendNext_() < s_cchBuffer );

    const size_t cbNeeded = strlen( szPrint ) + 1;
    const size_t cbLeft = s_cchBuffer - IchAppendNext_() - 1;

    if ( cbNeeded < cbLeft )
    {
        // We have enough room, concatonate.
        OSStrCbCopyA( &( m_rgchBuffer[ IchAppendNext_() + 1 ] ), cbLeft, szPrint );
        m_cyichAppendMax += cbNeeded;

        // Now we have to wipe out what ever partial string we have wipped out.
        while( m_rgchBuffer[ IchAppendNext_() ] != '\0' && IchAppendNext_() != 0 )
        {
            m_rgchBuffer[ IchAppendNext_() ] = '\0';
        }
    }
    else
    {
        Expected( fFalse ); // completely untested
        // We do not have enough room, wrap around ...
        for( ULONG ich = IchAppendNext_() + 1; ich < s_cchBuffer; ich++ )
        {
            m_rgchBuffer[ ich ] = '\0';
            m_cyichAppendMax++;
        }
        Assert( IchAppendNext_() == 0 );
    }
}

//  ================================================================
INLINE BOOL CPRINTINTRINBUF::FContains( _In_z_ const CHAR * const szFind )
//  ================================================================
{
    BUFCURSOR csr( this );

    const CHAR * szT = NULL;
    while( ( szT = csr.SzNext() ) != NULL )
    {
        if ( strstr( szT, szFind ) != NULL )
        {
            return fTrue;
        }
    }

    return fFalse;
}

//  ================================================================
INLINE ULONG CPRINTINTRINBUF::CContains( _In_z_ const CHAR * const szFind )
//  ================================================================
//  Note: returns number of appended strings that contain search string / szFind, but not if there
//  are multiple hits in a single appended string (i.e. appended by operator()).
{
    BUFCURSOR csr( this );

    ULONG cHits = 0;

    const CHAR * szT = NULL;
    while( ( szT = csr.SzNext() ) != NULL )
    {
        if ( strstr( szT, szFind ) != NULL )
        {
            cHits++;
        }
    }

    return cHits;
}

//  ================================================================
INLINE void CPRINTINTRINBUF::Print( CPRINTF & cprintf )
//  ================================================================
{
    BUFCURSOR csr( this );

    const CHAR * szT = NULL;
    const BOOL fSzId = fTrue;
    ULONG i = 0;
    while( ( szT = csr.SzNext() ) != NULL )
    {
        if ( fSzId )
            cprintf( (_TCHAR*)"[%d] %hs", i, szT );
        else
            cprintf( (_TCHAR*)"%hs", szT );
        i++;
    }
}

#ifdef DEBUG

//  ==================================================================================================================
class CPRINTFDEBUG : public CPRINTF
//  ==================================================================================================================
{
    public:
        void __cdecl operator()( const _TCHAR* szFormat, ... );
        static CPRINTF* PcprintfInstance();
};

//  ================================================================
INLINE CPRINTF* CPRINTFDEBUG::PcprintfInstance()
//  ================================================================
{
    extern CPRINTFDEBUG g_cprintfDEBUG;
    return &g_cprintfDEBUG;
}

//  ================================================================
INLINE void __cdecl CPRINTFDEBUG::operator()( const _TCHAR* szFormat, ... )
//  ================================================================
{
    va_list arg_ptr;
    va_start( arg_ptr, szFormat );
    _vtprintf( szFormat, arg_ptr );
    va_end( arg_ptr );
}

#define DBGprintf       (*CPRINTFDEBUG::PcprintfInstance())

#endif  //  DEBUG

//  ==================================================================================================================
class CPRINTFFILE : public CPRINTF
//  ==================================================================================================================
{
    public:
        CPRINTFFILE( const WCHAR* wszFile );
        ~CPRINTFFILE();
        
        void __cdecl operator()( const _TCHAR* szFormat, ... );
        
    private:
        void* m_hFile;
        void* m_hMutex;
};

//  ==================================================================================================================
class CWPRINTFFILE : public CPRINTF
//  ==================================================================================================================
{
    public:
        CWPRINTFFILE( const WCHAR* szFile );
        ~CWPRINTFFILE();

    // If _UNICODE is defined, then we only want a single function.
    // If _UNICODE is not defined, we need two different functions.
#ifndef _UNICODE
        void __cdecl operator()( const _TCHAR* szFormat, ... );
#endif
        void __cdecl operator()( const wchar_t * wszFormat, ... );
        ERR m_errLast;
        
    private:
        void* m_hFile;
        void* m_hMutex;
};

//  ==================================================================================================================
class CPRINTFINDENT : public CPRINTF
//  ==================================================================================================================
{
    public:
        CPRINTFINDENT( CPRINTF* pcprintf, const _TCHAR* szPrefix = NULL );
    
        void __cdecl operator()( const _TCHAR* szFormat, ... );

        virtual void Indent();
        virtual void Unindent();
        
    protected:
        CPRINTFINDENT();
        
    private:
        CPRINTF* const      m_pcprintf;
        INT                 m_cindent;
        const _TCHAR* const m_szPrefix;
};

//  ================================================================
INLINE CPRINTFINDENT::CPRINTFINDENT( CPRINTF* pcprintf, const _TCHAR* szPrefix ) :
//  ================================================================
    m_cindent( 0 ),
    m_pcprintf( pcprintf ),
    m_szPrefix( szPrefix )
{
}
    
//  ================================================================
INLINE void __cdecl CPRINTFINDENT::operator()( const _TCHAR* szFormat, ... )
//  ================================================================
{
    _TCHAR rgchBuf[1024];
    va_list arg_ptr;
    va_start( arg_ptr, szFormat );
    StringCbVPrintf( rgchBuf, sizeof(rgchBuf), szFormat, arg_ptr );
    va_end( arg_ptr );

    for( INT i = 0; i < m_cindent; i++ )
    {
        (*m_pcprintf)( _T( "\t" ) );
    }

    if( m_szPrefix )
    {
        (*m_pcprintf)( _T( "%s" ), m_szPrefix );
    }
    (*m_pcprintf)( _T( "%s" ), rgchBuf );
}

//  ================================================================
INLINE void CPRINTFINDENT::Indent()
//  ================================================================
{
    ++m_cindent;
}

//  ================================================================
INLINE void CPRINTFINDENT::Unindent()
//  ================================================================
{
    if( m_cindent > 0 )
    {
        --m_cindent;
    }
}

//  ================================================================
INLINE CPRINTFINDENT::CPRINTFINDENT( ) :
//  ================================================================
    m_cindent( 0 ),
    m_pcprintf( 0 ),
    m_szPrefix( 0 )
{
}
    

//  ==================================================================================================================
class CPRINTFTLSPREFIX : public CPRINTFINDENT
//  ==================================================================================================================
//
//  Prefix all output from a certain thread with a Tls-specific string
//  to allow the multi-threaded output to be sorted
//
//-
{
    public:
        CPRINTFTLSPREFIX( CPRINTF* pcprintf, const _TCHAR * const szPrefix = NULL );
    
        void __cdecl operator()( const _TCHAR* szFormat, ... );

        void Indent();
        void Unindent();
        
    private:
        CPRINTF* const      m_pcprintf;
        INT                 m_cindent;
        const _TCHAR* const m_szPrefix;
};


//  ==================================================================================================================
class CPRINTFFN : public CPRINTF
//  ==================================================================================================================
{
    public:
        CPRINTFFN( INT (__cdecl *pfnPrintf)(const _TCHAR*, ... ) ) : m_pfnPrintf( pfnPrintf ) {}
        ~CPRINTFFN() {}

        void __cdecl operator()( const _TCHAR* szFormat, ... )
        {
            _TCHAR rgchBuf[1024];
            
            va_list arg_ptr;
            va_start( arg_ptr, szFormat );
            StringCbVPrintf(rgchBuf, sizeof(rgchBuf), szFormat, arg_ptr);
            va_end( arg_ptr );

            (*m_pfnPrintf)( _T( "%s" ), rgchBuf );
        }

    private:
        INT (__cdecl *m_pfnPrintf)( const _TCHAR*, ... );
};
    


//  retrieves the current width of stdout

DWORD UtilCprintfStdoutWidth();


#endif  //  _OS_CPRINTF_HXX_INCLUDED


