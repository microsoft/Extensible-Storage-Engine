# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

add_compile_definitions(_UNICODE)
add_compile_definitions(UNICODE)

add_library(_lz4 STATIC
    lz4.c)

# Makes the headers in this directory available to consumers
target_include_directories(_lz4 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
