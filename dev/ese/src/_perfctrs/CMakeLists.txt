add_custom_command(
    OUTPUT
        ${GEN_OUTPUT_DIRECTORY}/_perfctrs/eseperf.hxx
        ${GEN_OUTPUT_DIRECTORY}/_perfctrs/eseperf.ini
        ${GEN_OUTPUT_DIRECTORY}/_perfctrs/perfdata.cxx
        ${GEN_OUTPUT_DIRECTORY}/_perfctrs/perfdisp.cxx
    COMMAND perl ${CMAKE_CURRENT_SOURCE_DIR}/perfdata.pl ESE ${GEN_OUTPUT_DIRECTORY}/_perfctrs/ eseperf.hxx eseperf.ini ${CMAKE_CURRENT_SOURCE_DIR}/perfdata.txt ESE
    DEPENDS
        perfdata.pl
        perfdata.txt
)

add_library(_perfctrs STATIC
    ${GEN_OUTPUT_DIRECTORY}/_perfctrs/perfdata.cxx
    ${GEN_OUTPUT_DIRECTORY}/_perfctrs/perfdisp.cxx
)

target_compile_definitions(_perfctrs PRIVATE
    _MBCS
)

target_include_directories(_perfctrs PRIVATE
    ${GEN_OUTPUT_DIRECTORY}/_res/
    ${INC_OUTPUT_DIRECTORY}
    ${ESE_DEV}/src/inc/_osu/
    ./
)