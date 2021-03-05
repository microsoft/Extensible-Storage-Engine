if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # These are GCC-specific flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # using Visual Studio C++
    # ??
endif()

add_compile_definitions(_MBCS)

add_library(checksum STATIC
    ../checksum.cxx
    ../checksum_amd64.cxx
)

add_dependencies(checksum
    gen_jetmsgex
)

target_precompile_headers(checksum PRIVATE
    ../checksumstd.hxx
)

target_include_directories(checksum PRIVATE
    ${GEN_OUTPUT_DIRECTORY}/_res/ # For jetmsgex.h
    ${INC_OUTPUT_DIRECTORY}/jet
    ${ESE_DEV}/src/inc/_osu
)