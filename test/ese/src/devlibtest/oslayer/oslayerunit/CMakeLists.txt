# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

add_executable(oslayerunit
    util.cxx
    UtilMm.cxx
    main.cxx
    allocioreq.cxx
    BetaSiteModeStagingFeatureSuite.cxx
    IoStatsSuite.cxx
    encryptiontest.cxx
    errortest.cxx
    fileiomax.cxx
    FileReadVsWriteModesCompatibilitySuite.cxx
    FileRenameTestSuite.cxx
    ftltracing.cxx
    librarytest.cxx
    mathtest.cxx
    memfile.cxx
    memorybitmaptest.cxx
    MemoryMappedIoSuite.cxx
    MemoryNotificationTests.cxx
    memorytest.cxx
    NORMNLSVersionEqualsSuite.cxx
    normtest.cxx
    pathtest.cxx
    PrintfBufSuite.cxx
    setsizetest.cxx
    sparsefiletest.cxx
    stringtest.cxx
    timerqueuetest.cxx
    timetest.cxx
    tlsperftest.cxx
    tlstest.cxx
    types_ce_test.cxx
    UppercaseTextNormalizationSuite.cxx
    writecontiguoustest.cxx
    # As of Windows 8.1, the OS will report its version as 6.2 (Windows 8)
    # if the executable is not manifested for a later version
    # FNORMIGetNLSExWithVersionInfoIsSupported() is affected by this,
    # and NormalizationTest explicitly checks the OS version and the function's
    # corresponding return value
    oslayerunit.exe.manifest
)

target_precompile_headers(oslayerunit PRIVATE
    osunitstd.hxx
)

target_compile_definitions(oslayerunit PRIVATE
    _MBCS
    BUILD_ENV_IS_EX
    ESE_FLAVOUR_IS_ESE
)

target_include_directories(oslayerunit PRIVATE
    ${ESE_DEV}/published/inc/
    ${INC_OUTPUT_DIRECTORY}
    ${ESE_TEST}/src/inc
)

target_link_libraries(oslayerunit PRIVATE
    BstfUnitTest
    checksum
    checksum_avx
    oslite
    sync

    # Windows
    kernel32
    rpcrt4
    advapi32
)