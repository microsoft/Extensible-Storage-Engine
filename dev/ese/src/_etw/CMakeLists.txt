# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

add_custom_command(
    OUTPUT ${GEN_OUTPUT_DIRECTORY}/_etw/Microsoft-ETW-ESE.man
    COMMAND perl ${CMAKE_CURRENT_SOURCE_DIR}/genetw.pl ESE_OSS ${CMAKE_CURRENT_SOURCE_DIR}/Microsoft-ETW-ESE.mc ${ESE_DEV}/published/inc/jethdr.w ${ESE_DEV}/src/inc/_jet.hxx ${ESE_DEV}/published/inc/tcconst.hxx ${GEN_OUTPUT_DIRECTORY}/_etw/Microsoft-ETW-ESE.man
    DEPENDS
        Microsoft-ETW-ESE.mc
        genetw.pl
        ${ESE_DEV}/published/inc/jethdr.w
        ${ESE_DEV}/published/inc/tcconst.hxx
        ${ESE_DEV}/src/inc/_jet.hxx
)

add_custom_command(
    OUTPUT
        ${GEN_OUTPUT_DIRECTORY}/_etw/Microsoft-ETW-ESE.h
        ${GEN_OUTPUT_DIRECTORY}/_etw/Microsoft-ETW-ESE.rc
    COMMAND ${CMAKE_MESSAGE_COMPILER} -um ${GEN_OUTPUT_DIRECTORY}/_etw/Microsoft-ETW-ESE.man -h ${GEN_OUTPUT_DIRECTORY}/_etw -r ${GEN_OUTPUT_DIRECTORY}/_etw
    DEPENDS ${GEN_OUTPUT_DIRECTORY}/_etw/Microsoft-ETW-ESE.man
)

add_custom_target(gen_etw
    DEPENDS
        ${GEN_OUTPUT_DIRECTORY}/_etw/Microsoft-ETW-ESE.man
        ${GEN_OUTPUT_DIRECTORY}/_etw/Microsoft-ETW-ESE.h
        ${GEN_OUTPUT_DIRECTORY}/_etw/Microsoft-ETW-ESE.rc
)

add_library(EtwEseProviderResources MODULE
    manifestresource.rc
)

add_dependencies(EtwEseProviderResources
    gen_etw
)

set_target_properties(EtwEseProviderResources
    PROPERTIES
        LINK_FLAGS "/NOENTRY"
)

target_compile_definitions(EtwEseProviderResources PRIVATE
    FILETYPE_DLL
    _UNICODE
    UNICODE
)

target_include_directories(EtwEseProviderResources PRIVATE
    ${GEN_OUTPUT_DIRECTORY}/_etw/
    ${INC_OUTPUT_DIRECTORY}
    ./
)
