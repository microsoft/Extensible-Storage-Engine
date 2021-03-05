if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(
        /wd4477
        /wd4312
        /wd4459
        /wd4505
        /wd4456
        /wd5205
    )
endif()

add_subdirectory(bstf)
add_subdirectory(blue)
add_subdirectory(devlibtest)
add_subdirectory(samples)