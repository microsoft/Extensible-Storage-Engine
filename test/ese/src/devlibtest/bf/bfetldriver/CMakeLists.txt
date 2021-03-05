add_library(bfetldriver STATIC
    bfetldriver.cxx
)

target_compile_definitions(bfetldriver PRIVATE
    _MBCS
    BUILD_ENV_IS_EX
    ESE_FLAVOUR_IS_ESE
)

target_include_directories(bfetldriver PRIVATE
    ${ESE_DEV}/published/inc/
    ${INC_OUTPUT_DIRECTORY}
    ${ESE_TEST}/src/inc
    ${ESE_TEST}/src/devlibtest/bf/bfftldriver/
    ./
    ../
)