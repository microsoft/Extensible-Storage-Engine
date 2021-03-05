add_library(BstfUnitTest
    BstfUnitRunner.cxx
    BstfUnitTest.cxx
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # using Visual Studio C++

    add_compile_options(
        /wd4477 # Suppresses format string warnings
        /wd4312 # Suppresses type cast warnings
    )
endif()

target_include_directories(BstfUnitTest PRIVATE
    ${INC_OUTPUT_DIRECTORY}
    ${DEV_HEADER_OUTPUT_DIRECTORY}
    ${ESE_TEST}/src/inc
)