add_compile_definitions(_UNICODE)
add_compile_definitions(UNICODE)

add_library(_xpress STATIC
    xencode.c
    xdecode.c)

# Makes the headers in this directory available to consumers
target_include_directories(_xpress PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})