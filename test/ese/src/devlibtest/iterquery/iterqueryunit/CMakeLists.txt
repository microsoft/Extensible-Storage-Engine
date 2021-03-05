add_executable(IterQueryUnit
    main.cxx
    IqCompileChecksSuite.cxx
)

target_compile_definitions(IterQueryUnit PRIVATE
    _MBCS
    BUILD_ENV_IS_EX
    ESE_FLAVOUR_IS_ESE
)

target_include_directories(IterQueryUnit PRIVATE
    ${ESE_DEV}/published/inc/
    ${INC_OUTPUT_DIRECTORY}
    ${ESE_TEST}/src/inc
)

target_link_libraries(IterQueryUnit PRIVATE
    BstfUnitTest
    sync
)