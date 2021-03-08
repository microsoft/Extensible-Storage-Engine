# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Workaround for CMake's directory scoping
add_library(esetestlib
    dev.cxx
    esetest.cxx
    esetest_static.cxx
    Disk.cxx
    EtwCollection.cxx
    EventLog.cxx
    PerfCollection.cxx
    PerfCtr.cxx
    PerfReporting.cxx
)

add_dependencies(esetestlib
    gen_etw
)

target_compile_definitions(esetestlib PRIVATE
    _MBCS
    BUILD_ENV_IS_EX
    ESE_FLAVOUR_IS_ESE
)

target_include_directories(esetestlib PRIVATE
    ${GEN_OUTPUT_DIRECTORY}/_etw/
    ${ESE_DEV}/published/inc/
    ${INC_OUTPUT_DIRECTORY}
    ${ESE_TEST}/src/inc
    ./
    ../
)