# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

add_executable(embeddedunittest
    embeddedunittest.cxx
)

target_compile_definitions(embeddedunittest PRIVATE
    _MBCS
)

target_link_libraries(embeddedunittest
    sync
    kernel32
    advapi32
    eselibwithtests
    oslite
)

target_include_directories(embeddedunittest PRIVATE
    ${GEN_OUTPUT_DIRECTORY}/_res/ # For jetmsgex.h
    ${INC_OUTPUT_DIRECTORY}
    ${ESE_DEV}/src/inc/_osu/
    ${ESE_DEV}/src/os/
    ${ESE_DEV}/src/os/blockcache
    ./
    ../
)