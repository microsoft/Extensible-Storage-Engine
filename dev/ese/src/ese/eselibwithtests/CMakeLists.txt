# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# This library is `ese` + tests. See ENABLE_JET_UNIT_TEST being defined below
# used throughout the code base

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
        ${GEN_OUTPUT_DIRECTORY}/ese/eselibwithtests/ese.def
    COMMAND
        cl /nologo /c /DLIBNAME=ESElibwithtests /DDEBUGGER_EXTENSION /EP /Tc ${ESE_DEV}/src/ese/esent.def > ${GEN_OUTPUT_DIRECTORY}/ese/eselibwithtests/ese.def
    DEPENDS
        ${ESE_DEV}/src/ese/esent.def
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Using Microsoft Visual C++

    # See documentation for C2712
    string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

add_library(eselibwithtests SHARED
    ${GEN_OUTPUT_DIRECTORY}/ese/eselibwithtests/ese.def
    ${GEN_OUTPUT_DIRECTORY}/ese/jetapitable.cxx
    ${ESE_SOURCES}
    ../ese.rc
    ../std.cxx

    # Tests
    daehelpers_test.cxx
    ../clogredomaps_test.cxx
    ../cpage_test.cxx
    ../checksum_test.cxx
    ../flushmap_test.cxx
    ../fmp_test.cxx
    ../jettest.cxx
    ../logprereader_test.cxx
    ../log_test.cxx
    ../node_test.cxx
    ../oslayer_test.cxx
    ../oslayer_iomgr_test.cxx
    ../osu_test.cxx
    ../rbscleaner_test.cxx
)

target_compile_definitions(eselibwithtests PRIVATE
    ENABLE_JET_UNIT_TEST
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Using Microsoft Visual C++
    target_link_options(eselibwithtests PRIVATE
        /ENTRY:"DLLEntryPoint"
        ${MSVC_ESE_LINK_OPTIONS}
    )
endif()

add_dependencies(eselibwithtests gen_jetapitable)

target_precompile_headers(eselibwithtests PRIVATE
    ../std.hxx
)

target_link_libraries(eselibwithtests PRIVATE
    ${ESE_LIBRARIES}
)

target_include_directories(eselibwithtests PRIVATE
    ${GEN_OUTPUT_DIRECTORY}/_res/ # For jetmsgex.h
    ${INC_OUTPUT_DIRECTORY}
    ${ESE_DEV}/src/_xpress/
    ${ESE_DEV}/src/inc/_osu/
    ${ESE_DEV}/src/os/
    ${ESE_DEV}/src/os/blockcache/
    ./
    ../
)