# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

add_executable(COLLECTIONUNIT
    countedinvasivelinkedlist.cpp
    main.cxx
    objectpool.cxx
    redblacktree.cxx
    singlelinkedlist.cxx
    queuelist.cxx
    array.cxx
    rrnextprev.cxx
    table.cxx
    approximateindex.cxx
    concurrentmodset.cxx
)

target_compile_definitions(COLLECTIONUNIT PRIVATE
    _MBCS
    BUILD_ENV_IS_EX
    ESE_FLAVOUR_IS_ESE
)

target_include_directories(COLLECTIONUNIT PRIVATE
    ${ESE_DEV}/published/inc/
    ${INC_OUTPUT_DIRECTORY}
    ${ESE_TEST}/src/inc
)

target_link_libraries(COLLECTIONUNIT PRIVATE
    BstfUnitTest
    checksum
    checksum_avx
    oslite
    sync
)