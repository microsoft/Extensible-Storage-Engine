# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Using Microsoft Visual C++

    add_compile_options(
        /wd4214 # function not inlined - TODO: add it back
        /wd4100 # unreferenced formal parameter
        /wd4214 # nonstandard extension used: bit field of non-integral type
    )
endif()

add_executable(BookStoreSample
    BookStoreSample.c
)

target_compile_definitions(BookStoreSample PRIVATE
    _MBCS
    WIN32
)

target_link_libraries(BookStoreSample PRIVATE
    ese

    # Windows dependencies
    advapi32
    kernel32
    user32
)