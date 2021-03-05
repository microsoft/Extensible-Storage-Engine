add_library(_log STATIC
    logstd.cxx
    log.cxx
    logutil.cxx
    logread.cxx
    logread_legacy.cxx
    logredo.cxx
    logredomap.cxx
    logshadow.cxx
    logstream.cxx
    logwrite.cxx
    logdump.cxx
    rstmap.cxx
)

add_dependencies(_log
    gen_jetmsgex
)

target_precompile_headers(_log PRIVATE
    logstd.hxx
)

target_compile_definitions(_log PRIVATE
    _MBCS
    PERFMON_SUPPORT
)

target_include_directories(_log PRIVATE
    ${GEN_OUTPUT_DIRECTORY}/_res/
    ${INC_OUTPUT_DIRECTORY}
    ${ESE_DEV}/src/inc/_osu/
    ./
)
