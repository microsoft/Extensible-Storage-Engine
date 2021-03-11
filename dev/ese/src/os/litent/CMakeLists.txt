# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Workaround for CMake's directory scoping
add_custom_command(
    OUTPUT ${INC_OUTPUT_DIRECTORY}/_tracetagimpl.h
    COMMAND ${CMAKE_COMMAND} -E
        touch_nocreate ${INC_OUTPUT_DIRECTORY}/_tracetagimpl.h
    DEPENDS gen_tracetagimpl
)

add_library(oslite STATIC
    # With CMake, this header has to be included explicitly as a source
    ${INC_OUTPUT_DIRECTORY}/_tracetagimpl.h
    osstd.cxx

    # Stubs/thunks
    violated.cxx

    # Common sources between oslite and oswinnt
    ${OS_SOURCES}
)

add_dependencies(oslite
    gen_etw
    gen_jetmsgex
)

target_compile_definitions(oslite PRIVATE
    _MBCS
)

target_precompile_headers(oslite PRIVATE
    osstd.hxx
)

target_include_directories(oslite PRIVATE
    ./
)