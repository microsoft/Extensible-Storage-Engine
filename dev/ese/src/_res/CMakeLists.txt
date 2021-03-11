# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

add_custom_command(
    OUTPUT ${GEN_OUTPUT_DIRECTORY}/_res/jetmsgex.mc
    COMMAND perl ${CMAKE_CURRENT_SOURCE_DIR}/fixmc.pl ${CMAKE_CURRENT_SOURCE_DIR}/jetmsg.mc ${GEN_OUTPUT_DIRECTORY}/_res/jetmsgex.mc
    DEPENDS
        fixmc.pl
        jetmsg.mc
)

set(MC_INPUTS
    ${GEN_OUTPUT_DIRECTORY}/_res/jetmsgex.mc
)

foreach (MC_INPUT ${MC_INPUTS})
    get_filename_component(MC_OUTPUT ${MC_INPUT} NAME_WLE)

    string(REPLACE ".mc" "" MC_OUTPUT ${MC_OUTPUT})
    add_custom_command(
        OUTPUT
            ${GEN_OUTPUT_DIRECTORY}/_res/${MC_OUTPUT}.h
            ${GEN_OUTPUT_DIRECTORY}/_res/${MC_OUTPUT}.rc
        COMMAND ${CMAKE_MESSAGE_COMPILER} -h ${GEN_OUTPUT_DIRECTORY}/_res -r ${GEN_OUTPUT_DIRECTORY}/_res -v ${MC_INPUT}
        DEPENDS ${MC_INPUT}
    )

    add_custom_target(gen_${MC_OUTPUT}
        DEPENDS
            ${MC_INPUT}
            ${GEN_OUTPUT_DIRECTORY}/_res/${MC_OUTPUT}.h
            ${GEN_OUTPUT_DIRECTORY}/_res/${MC_OUTPUT}.rc
    )
endforeach()
