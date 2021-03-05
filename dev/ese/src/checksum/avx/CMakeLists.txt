if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # These are GCC-specific flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # using Visual Studio C++
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX")
endif()

add_compile_definitions(_MBCS)

add_library(checksum_avx STATIC
    checksum_avx.cxx
)

add_dependencies(checksum_avx
    gen_jetmsgex
)

target_include_directories(checksum_avx PRIVATE
    ${GEN_OUTPUT_DIRECTORY}/_res/ # For jetmsgex.h
    ${ESE_DEV}/published/inc
    ${ESE_DEV}/published/inc/os
    ${INC_OUTPUT_DIRECTORY}/jet
    ${ESE_DEV}/src/checksum
    ${ESE_DEV}/src/inc/_osu
)