# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Test-specific compiler options
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Using Microsoft Visual C++

    # Suppress certain warnings by number
    add_compile_options(
        /wd4101 # 'identifier': unreferenced local variable
        /wd4189 # 'identifier': local variable is initialized but not referenced
        /wd4267 # 'var' : conversion from 'size_t' to 'type', possible loss of data
        /wd4302 # 'conversion': truncation from 'type1' to 'type2'
        /wd4311 # 'variable': pointer truncation from 'type1' to 'type2'
        /wd4312 # 'operation': conversion from 'type1' to 'type2' of greater size
        /wd4477 # 'function' : format string 'string' requires an argument of type 'type', but variadic argument number has type 'type'
        /wd4595 # 'operator new': non-member operator new or delete functions may not be declared inline
        /wd4838 # conversion from 'type_1' to 'type_2' requires a narrowing conversion
        /wd4996 # Deprecated declaration
    )
endif()

add_subdirectory(src)