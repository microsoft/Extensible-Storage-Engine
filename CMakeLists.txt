# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.19)
project (ese)

# Keeping at verbose while the project build files are still in flux
set(CMAKE_MESSAGE_LOG_LEVEL "VERBOSE")
message(STATUS "[ CXX Default Flags ] = ${CMAKE_CXX_FLAGS}")
message(STATUS "[ CXX Debug Flags   ] = ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "[ CXX Release Flags ] = ${CMAKE_CXX_FLAGS_RELEASE}")

# CMake debug configuration: these symbols are expected to be defined
add_compile_options("$<$<CONFIG:Debug>:-DDEBUG>")
add_compile_options("$<$<CONFIG:Debug>:-DDBG=1>")
add_compile_options("$<$<CONFIG:Debug>:-DDEBUG_BUILD>")
add_compile_options("$<$<CONFIG:Debug>:-DMEM_CHECK>")
add_compile_options("$<$<CONFIG:Debug>:-DNDEBUG>")

# CMake release configuration: these symbols are expected to be defined.
# Point of order: In the code, for historical/internal reasons, we often
# refer to the `Release` configuration as `Retail`
add_compile_options("$<$<CONFIG:Release>:-DRTM>")
add_compile_options("$<$<CONFIG:Release>:-DNDEBUG>")

# 64-bit symbols for Visual Studio C++
if("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
    if("${CMAKE_GENERATOR_PLATFORM}" MATCHES "x64")
        message(STATUS "NOTE: Setting 64-bit symbols")
        add_compile_definitions(
            _AMD64_
            _AMD64_SIMULATOR_
            _AMD64_SIMULATOR_PERF_
            _AMD64_WORKAROUND_
            _SKIP_IF_SIMULATOR_
            _WIN64
            AMD64
        )
    elseif("${CMAKE_GENERATOR_PLATFORM}" MATCHES "Win32")
        message(STATUS "NOTE: Setting 32-bit symbols")
        add_compile_definitions(
            _X86_=1
            FPO=0
            i386=1
            STD_CALL
        )
    else()
        message(FATAL_ERROR "NOTE: NOT setting 64-bit OR 32-bit symbols")
    endif()
else()
    message(FATAL_ERROR "NOTE: NOT setting 64-bit OR 32-bit symbols")
endif()

# CMake chooses reasonable prefixes and suffixes for Unix-like platforms,
# but we'd rather keep them the same for now. May revisit this decision later.
set(CMAKE_STATIC_LIBRARY_PREFIX)
set(CMAKE_STATIC_LIBRARY_SUFFIX .lib)
set(CMAKE_EXECUTABLE_SUFFIX .exe)

# Set certain CMake path variables
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Generated build artifacts path shortcut
set(GEN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/gen)

# The publish headers path shortcut
set(INC_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/inc)

# Ensure the above paths exist
file(MAKE_DIRECTORY ${GEN_OUTPUT_DIRECTORY})
file(MAKE_DIRECTORY ${INC_OUTPUT_DIRECTORY})
file(MAKE_DIRECTORY ${INC_OUTPUT_DIRECTORY}/jet)

# Source path shortcuts
set(ESE_DEV ${CMAKE_SOURCE_DIR}/dev/ese)
set(ESE_TEST ${CMAKE_SOURCE_DIR}/test/ese)

# Compiler-specific project-wide flags and settings
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # using GCC

    # -g Enables extra debugging info for GDB
    # NOTE: if you wish to debug a core dump (instead of debugging a running program).
    # you will need to run `ulimit -c unlimited` first, to enable core dump generation.
    add_compile_options(-g)

    # Stop after the first error
    # This can be removed once GCC build is fully working
    add_compile_options(-fmax-errors=1)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Using Visual C++

    add_compile_options(
        /W4 # Show level-4 lint-like informational warnings
        /WX # warnings as errors

        # Suppress certain warnings by number
        /wd4267 # 'var': conversion from 'size_t' to 'type', possible loss of data
        /wd4996 # deprecated declaration
        /wd4984 # using c++17 "if constexpr( xx )" syntax
        /wd4706 # assignment within a conditional expression
        /wd4291 # no matching operator delete found
        /wd4714 # function not inlined - TODO: add it back

        /Wv:19.24 # Force compiler warnings later than v19.24 to not be shown
    )
endif()

# On Windows, we can use the Message Compiler for resource compilation
if (WIN32)
    find_program(CMAKE_MESSAGE_COMPILER mc.exe HINTS "${sdk_bindir}")
    if (NOT CMAKE_MESSAGE_COMPILER)
        message(FATAL_ERROR "message compiler not found: required to build")
    else ()
        message(STATUS "SUCCESS: Found Message Compiler: '${CMAKE_MESSAGE_COMPILER}'")
    endif (NOT CMAKE_MESSAGE_COMPILER)
else ()
    # non-Windows environments
    message(FATAL_ERROR "gettext-based resource compilation is not implemented yet")
endif (WIN32)

set(ESE_DEBUG_LIBRARIES
    debug ucrtd
    debug vcruntimed
    debug msvcrtd
    debug msvcprtd
)

set(ESE_RELEASE_LIBRARIES
    optimized ucrt
    optimized vcruntime
    optimized msvcrt
    optimized msvcprt
)

link_libraries(
    ${ESE_DEBUG_LIBRARIES}
    ${ESE_RELEASE_LIBRARIES}
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Using Visual C++
    add_compile_options(
        /Oi # generate intrinsic functions
        /Ot
        /GL # whole program optimization
        # IMPORTANT: `library.cxx` reference-compares the strings defined in
        # the shared header `osstd_.hxx`, and string pooling is necessary for
        # the comparison to succeed. Without string pooling, ese.dll will fail
        # to load at runtime: watch out for NTSTATUS error 0xC0000142.
        /GF # Pool strings.
        /Gm-
        /MD
        /Zp8
        /GS
        /Gy # function-level linking
        /Zc:wchar_t-
        /Zc:forScope
        /Zc:inline-
        /GR-
        /TP
        /FC
        /Zl
        /Zo # enhance optimized debugging (previously, `/d2Zi+`)
        /Z7
    )

    # Should be used with /GL (see above)
    add_link_options(
        /LTCG # link-time generation
        /INCREMENTAL:NO # incremental incompatible with /LTCG
    )
endif()

# Finally, in this project, we configure and build these subdirectories:
add_subdirectory(dev)
add_subdirectory(test)