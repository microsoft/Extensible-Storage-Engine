# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

add_custom_command(
    OUTPUT ${GEN_OUTPUT_DIRECTORY}/err.cxx
    COMMAND perl ${CMAKE_CURRENT_SOURCE_DIR}/err.pl ${ESE_DEV}/published/inc/jethdr.w ${GEN_OUTPUT_DIRECTORY}/err.cxx
    DEPENDS
        err.pl
        errdata.txt
        ${ESE_DEV}/published/inc/jethdr.w
)

add_library(_errstr STATIC
    ${GEN_OUTPUT_DIRECTORY}/err.cxx
    errorhierarchy.cxx
    errdata.cxx
)

target_include_directories(_errstr PRIVATE
    ${INC_OUTPUT_DIRECTORY}/jet
)