add_executable(RESMGRUNIT
    basic.cxx
    main.cxx
)

target_compile_definitions(RESMGRUNIT PRIVATE
    _MBCS
    BUILD_ENV_IS_EX
    ESE_FLAVOUR_IS_ESE
)

target_include_directories(RESMGRUNIT PRIVATE
    ${ESE_DEV}/published/inc/
    ${INC_OUTPUT_DIRECTORY}
    ${ESE_TEST}/src/inc
)

target_link_libraries(RESMGRUNIT PRIVATE
    BstfUnitTest
    sync
)