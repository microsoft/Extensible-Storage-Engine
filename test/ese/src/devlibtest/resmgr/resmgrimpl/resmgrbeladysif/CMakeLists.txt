# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

add_library(RESMGRBELADYSIF STATIC
    resmgrbeladysif.cxx
)

target_compile_definitions(RESMGRBELADYSIF PRIVATE
    _MBCS
    BUILD_ENV_IS_EX
    ESE_FLAVOUR_IS_ESE
)

target_include_directories(RESMGRBELADYSIF PRIVATE
    ${ESE_DEV}/published/inc/
    ${INC_OUTPUT_DIRECTORY}
    ${ESE_TEST}/src/inc
    ${ESE_TEST}/src/devlibtest/bf
    ${ESE_TEST}/src/devlibtest/bf/bfetldriver
    ${ESE_TEST}/src/devlibtest/bf/bfftldriver
    ${ESE_TEST}/src/devlibtest/resmgr/resmgremulator
    ${ESE_TEST}/src/devlibtest/resmgr/resmgrimpl/resmgrlrutest
    ${ESE_TEST}/src/devlibtest/resmgr/resmgrimpl/resmgrlruktest
    ${ESE_TEST}/src/devlibtest/resmgr/resmgrimpl/resmgrbeladys
    ${ESE_TEST}/src/devlibtest/resmgr/resmgrimpl/resmgrlrutestif
    ${ESE_TEST}/src/devlibtest/resmgr/resmgrimpl/resmgrlruktestif
    ${ESE_TEST}/src/devlibtest/resmgr/resmgrimpl/resmgrlrukeseif
    ${ESE_TEST}/src/devlibtest/resmgr/resmgrimpl/resmgrbeladysif
)