# E.g., pthreads
find_package (Threads REQUIRED)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # These are GCC-specific flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx")

    # We have some strings in the code, such as "<???>", that are interpreted as trigraphs by GCC
    add_compile_options(-Wno-trigraphs)

    # warning: offsetof within non-standard-layout type 'IOREQ' is conditionally-supported
    add_compile_options(-Winvalid-offsetof)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # using Visual Studio C++
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX")
endif()

add_custom_command(
    OUTPUT ${GEN_OUTPUT_DIRECTORY}/_tracetagimpl.h
    COMMAND perl ${CMAKE_CURRENT_SOURCE_DIR}/gentracetagimpl.pl ${ESE_DEV}/published/inc/jethdr.w ${GEN_OUTPUT_DIRECTORY}/_tracetagimpl.h
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/gentracetagimpl.pl
        ${ESE_DEV}/published/inc/jethdr.w
)

add_custom_target(gen_tracetagimpl
    COMMAND ${CMAKE_COMMAND} -E
        copy ${GEN_OUTPUT_DIRECTORY}/_tracetagimpl.h ${INC_OUTPUT_DIRECTORY}/_tracetagimpl.h
    DEPENDS ${GEN_OUTPUT_DIRECTORY}/_tracetagimpl.h
)

set(OS_SOURCES
    ${ESE_DEV}/src/os/osblockcache.cxx
    ${ESE_DEV}/src/os/cprintf.cxx
    ${ESE_DEV}/src/os/config.cxx
    ${ESE_DEV}/src/os/dev.cxx
    ${ESE_DEV}/src/os/dllentry.cxx
    ${ESE_DEV}/src/os/edbg.cxx
    ${ESE_DEV}/src/os/encrypt.cxx
    ${ESE_DEV}/src/os/error.cxx
    ${ESE_DEV}/src/os/event.cxx
    ${ESE_DEV}/src/os/hapublish.cxx
    ${ESE_DEV}/src/os/library.cxx
    ${ESE_DEV}/src/os/math.cxx
    ${ESE_DEV}/src/os/memfile.cxx
    ${ESE_DEV}/src/os/memory.cxx
    ${ESE_DEV}/src/os/memorynotification.cxx
    ${ESE_DEV}/src/os/norm.cxx
    ${ESE_DEV}/src/os/os.cxx
    ${ESE_DEV}/src/os/oseventtrace.cxx
    ${ESE_DEV}/src/os/osdiag.cxx
    ${ESE_DEV}/src/os/osdisk.cxx
    ${ESE_DEV}/src/os/osfile.cxx
    ${ESE_DEV}/src/os/iostats.cxx
    ${ESE_DEV}/src/os/osfs.cxx
    ${ESE_DEV}/src/os/ostimerqueue.cxx
    ${ESE_DEV}/src/os/perfmon.cxx
    ${ESE_DEV}/src/os/reftrace.cxx
    ${ESE_DEV}/src/os/string.cxx
    ${ESE_DEV}/src/os/sysinfo.cxx
    ${ESE_DEV}/src/os/task.cxx
    ${ESE_DEV}/src/os/thread.cxx
    ${ESE_DEV}/src/os/time.cxx
    ${ESE_DEV}/src/os/trace.cxx
)

include_directories(
    ${GEN_OUTPUT_DIRECTORY}/_etw/
    ${GEN_OUTPUT_DIRECTORY}/_res/
    ${INC_OUTPUT_DIRECTORY}
    ${INC_OUTPUT_DIRECTORY}/jet/
    ./
    ./blockcache
)

add_subdirectory(litent)
add_subdirectory(winnt)