# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Workaround for CMake's directory scoping; see CMake issue 18399
add_custom_command(
    OUTPUT ${INC_OUTPUT_DIRECTORY}/_tracetagimpl.h
    COMMAND ${CMAKE_COMMAND} -E
        touch_nocreate ${INC_OUTPUT_DIRECTORY}/_tracetagimpl.h
    DEPENDS gen_tracetagimpl
)

add_library(oswinnt STATIC
    # With CMake, this header has to be included explicitly as a source
    ${INC_OUTPUT_DIRECTORY}/_tracetagimpl.h
    osstd.cxx

    # Common sources between oslite and oswinnt
    ${OS_SOURCES}
)

add_dependencies(oswinnt
    gen_etw
    gen_jetmsgex
)

target_precompile_headers(oswinnt PRIVATE
    osstd.hxx
)

target_compile_definitions(oswinnt PRIVATE
    DEBUGGER_EXTENSION_ESE
    OS_LAYER_VIOLATIONS
    PERFMON_SUPPORT
)

target_include_directories(oswinnt PRIVATE
    ${ESE_DEV}/src/inc/_osu/
    ./
)