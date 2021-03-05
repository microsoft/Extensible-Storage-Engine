add_executable(SvcInitSample
    SvcInitMainGoo.c
    SvcInitSampleA.c
    SvcInitSampleB.c
)

target_compile_options(SvcInitSample PRIVATE
    /wd4214 # nonstandard extension used: bit field types other than int
    /wd4201 # nonstandard extension used: nameless struct/union
)

target_compile_definitions(SvcInitSample PRIVATE
    _MBCS
    WIN32
)

target_link_libraries(SvcInitSample PRIVATE
    ese

    # Windows dependencies
    advapi32
    kernel32
    user32
)

target_include_directories(SvcInitSample PRIVATE
    ${ESE_DEV}/published/inc/
    ${INC_OUTPUT_DIRECTORY}
    ${ESE_TEST}/src/inc
    ./
)