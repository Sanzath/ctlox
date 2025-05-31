cmake_minimum_required(VERSION 3.31)
project(ctlox)

set(CMAKE_CXX_STANDARD 23)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (MSVC)
        add_compile_options(
                /clang:-fconstexpr-steps=2147483647
                /clang:-fconstexpr-depth=256
        )
    else ()
        add_compile_options(
                -fconstexpr-steps=2147483647
                -fconstexpr-depth=256
        )
    endif ()
endif ()

add_subdirectory(src)
add_subdirectory(tests)
