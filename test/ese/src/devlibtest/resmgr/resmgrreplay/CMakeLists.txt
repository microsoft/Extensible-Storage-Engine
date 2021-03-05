add_executable(RESMGRREPLAY
    main.cxx
    resmgrreplay.cxx
)

target_compile_definitions(RESMGRREPLAY PRIVATE
    _MBCS
    BUILD_ENV_IS_EX
    ESE_FLAVOUR_IS_ESE
)

target_include_directories(RESMGRREPLAY PRIVATE
    ${ESE_DEV}/published/inc/
    ${INC_OUTPUT_DIRECTORY}
    ${ESE_TEST}/src/inc
    ${ESE_TEST}/src/devlibtest/bf
    ${ESE_TEST}/src/devlibtest/bf/bfetldriver
    ${ESE_TEST}/src/devlibtest/bf/bfftldriver
    ${ESE_TEST}/src/devlibtest/resmgr/resmgremulator
    ${ESE_TEST}/src/devlibtest/resmgr/resmgrimpl/
    ${ESE_TEST}/src/devlibtest/resmgr/resmgrimpl/resmgrlrutest
    ${ESE_TEST}/src/devlibtest/resmgr/resmgrimpl/resmgrlruktest
    ${ESE_TEST}/src/devlibtest/resmgr/resmgrimpl/resmgrbeladys
    ${ESE_TEST}/src/devlibtest/resmgr/resmgrimpl/resmgrlrutestif
    ${ESE_TEST}/src/devlibtest/resmgr/resmgrimpl/resmgrlruktestif
    ${ESE_TEST}/src/devlibtest/resmgr/resmgrimpl/resmgrlrukeseif
    ${ESE_TEST}/src/devlibtest/resmgr/resmgrimpl/resmgrbeladysif
)

target_link_libraries(RESMGRREPLAY PRIVATE
    # Windows libraries
    advapi32
    comdlg32
    gdi32
    kernel32
    loadperf
    netapi32
    ole32
    oleaut32
    shell32
    shlwapi
    tdh
    version
    winspool
    user32
    uuid
    wsock32

    # ESE libraries
    checksum
    checksum_avx
    oslite
    sync

    # ESE test libraries
    BstfUnitTest
    bfetldriver
    bfftldriver
    esetestlib
    RESMGREMULATOR
    RESMGRBELADYSIF
    RESMGRLRUKTESTIF
    RESMGRLRUTESTIF
    RESMGRLRUKESEIF
)