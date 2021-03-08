# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

add_library(RESMGREMULATOR STATIC
    rmemulator.cxx
)

target_compile_definitions(RESMGREMULATOR PRIVATE
    BUILD_ENV_IS_EX
    ESE_FLAVOUR_IS_ESE
)

target_include_directories(RESMGREMULATOR PRIVATE
    ${ESE_DEV}/published/inc/
    ${INC_OUTPUT_DIRECTORY}
    ${ESE_TEST}/src/inc
    ${ESE_TEST}/src/devlibtest/bf
    ${ESE_TEST}/src/devlibtest/bf/bfftldriver/
)