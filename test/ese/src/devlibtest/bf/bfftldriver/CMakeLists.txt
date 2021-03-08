# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Using Visual C++

    # Suppress certain warnings by number
    add_compile_options(
        /wd4840 # (VS2017+) non-portable use of class 'class'
    )
endif()

add_library(bfftldriver STATIC
    bfftldriver.cxx
)

target_compile_definitions(bfftldriver PRIVATE
    _MBCS
    BUILD_ENV_IS_EX
    ESE_FLAVOUR_IS_ESE
)

target_include_directories(bfftldriver PRIVATE
    ${ESE_DEV}/published/inc/
    ${INC_OUTPUT_DIRECTORY}
    ${ESE_TEST}/src/inc
    ./
    ../
)