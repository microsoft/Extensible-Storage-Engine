# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

add_library(_esefile STATIC
    checksum.cxx
    copyfile.cxx
    status.cxx
    xsum.cxx
)

target_include_directories(_esefile PRIVATE
    ${INC_OUTPUT_DIRECTORY}/jet
)