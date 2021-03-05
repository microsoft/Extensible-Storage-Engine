add_executable(ERRVALIDATOR
    util.cxx
    errvalidator.cxx
)

target_compile_definitions(ERRVALIDATOR PRIVATE
    _MBCS
    BUILD_ENV_IS_EX
    ESE_FLAVOUR_IS_ESE
)

target_include_directories(ERRVALIDATOR PRIVATE
    ${ESE_DEV}/published/inc/
    ${INC_OUTPUT_DIRECTORY}
    ${ESE_TEST}/src/inc
)

target_link_libraries(ERRVALIDATOR PRIVATE
    _errstr
)