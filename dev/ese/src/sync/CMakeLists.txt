# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

add_compile_definitions(_MBCS)

add_library(sync STATIC
    currproc.cxx
    sync.cxx
)

target_include_directories(sync PRIVATE
    ${INC_OUTPUT_DIRECTORY}/jet
)

target_link_libraries(sync
    synchronization
)