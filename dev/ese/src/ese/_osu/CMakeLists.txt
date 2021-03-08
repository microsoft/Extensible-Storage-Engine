# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

add_library(_osu STATIC
    checksumu.cxx
    configu.cxx
    encryptu.cxx
    eventu.cxx
    fileu.cxx
    hapublishu.cxx
    normu.cxx
    osu.cxx
    syncu.cxx
    timeu.cxx
)

add_dependencies(_osu
    gen_jetmsgex
)

target_precompile_headers(_osu PRIVATE
    osustd.hxx
)

target_compile_definitions(_osu PRIVATE
    _MBCS
    PERFMON_SUPPORT
)

target_include_directories(_osu PRIVATE
    ${GEN_OUTPUT_DIRECTORY}/_res/
    ${INC_OUTPUT_DIRECTORY}
    ${ESE_DEV}/src/inc/_osu/
    ./
)