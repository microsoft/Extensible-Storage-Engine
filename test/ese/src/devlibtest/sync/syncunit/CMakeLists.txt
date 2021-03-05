add_executable(SYNCUNIT
    main.cxx
    util.cxx
    atomics.cxx
    atomicsmt.cxx
    initonce.cxx
    inittermlock.cxx
    leakdetection.cxx
    meteredsection.cxx
    semaphore.cxx
    semaphoreperf.cxx
)

target_include_directories(SYNCUNIT PRIVATE
    ${ESE_DEV}/published/inc/
    ${INC_OUTPUT_DIRECTORY}
    ${ESE_TEST}/src/inc
)

# E.g., pthreads
find_package (Threads REQUIRED)

target_link_libraries(SYNCUNIT PRIVATE
    BstfUnitTest
    sync
    # E.g., pthreads
    ${CMAKE_THREAD_LIBS_INIT}
)