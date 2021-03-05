if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Using Visual C++

    # cctypessuite intentionally tests variable use before initialization,
    # which triggers a run-time check that pops up a dialog.
    # This check is enabled by default in the Debug configuration, so we have
    # to remove the flag that enables it just for this test target
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
endif()

add_executable(CcLayerUnit
    cclayerunitmain.cxx
    ccstdoutsuite.cxx
    cctypessuite.cxx
    ccoperatorssuite.cxx
    ccerrorssuite.cxx
)

target_compile_definitions(CcLayerUnit PRIVATE
    _MBCS
    BUILD_ENV_IS_EX
    ESE_FLAVOUR_IS_ESE
)

target_include_directories(CcLayerUnit PRIVATE
    ${ESE_DEV}/published/inc/
    ${INC_OUTPUT_DIRECTORY}
    ${ESE_TEST}/src/inc
)

target_link_libraries(CcLayerUnit PRIVATE
    BstfUnitTest
)