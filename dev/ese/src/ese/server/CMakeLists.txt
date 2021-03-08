# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Workaround for CMake's directory scoping; see CMake issue 18399
add_custom_command(
    OUTPUT
        ${GEN_OUTPUT_DIRECTORY}/ese/jetapitable.cxx
    COMMAND ${CMAKE_COMMAND} -E
        touch_nocreate ${GEN_OUTPUT_DIRECTORY}/ese/jetapitable.cxx
    DEPENDS
        gen_jetapitable
)

add_custom_command(
    OUTPUT
        ${GEN_OUTPUT_DIRECTORY}/ese/server/ese.def
    COMMAND
        cl /nologo /c /DLIBNAME=ESE /DDEBUGGER_EXTENSION /EP /Tc ${ESE_DEV}/src/ese/esent.def > ${GEN_OUTPUT_DIRECTORY}/ese/server/ese.def
    DEPENDS
        ${ESE_DEV}/src/ese/esent.def
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Using Microsoft Visual C++

    # See documentation for C2712
    string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

# NOTE: `ese` is a shared library (a DLL), not a static one
add_library(ese SHARED
    ${GEN_OUTPUT_DIRECTORY}/ese/server/ese.def
    ${GEN_OUTPUT_DIRECTORY}/ese/jetapitable.cxx
    ${ESE_SOURCES}
    ../ese.rc
    ../std.cxx
)

set_target_properties(ese
    PROPERTIES
    DEFINE_SYMBOL "ESE"
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Using Microsoft Visual C++
    target_link_options(ese PRIVATE
        /ENTRY:"DLLEntryPoint"
        ${MSVC_ESE_LINK_OPTIONS}
    )
endif()

add_dependencies(ese gen_jetapitable)

target_precompile_headers(ese PRIVATE
    ../std.hxx
)

target_link_libraries(ese PRIVATE
    ${ESE_LIBRARIES}
)

target_include_directories(ese PRIVATE
    ${GEN_OUTPUT_DIRECTORY}/_res/
    ${INC_OUTPUT_DIRECTORY}
    ${ESE_DEV}/src/inc/_osu/
    ./
    ../
)