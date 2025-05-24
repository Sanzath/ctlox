cmake_minimum_required(VERSION 3.31)
project(ctlox)

set(CMAKE_CXX_STANDARD 23)

include_directories(.)

add_executable(ctlox
        include/ctlox/v1/ct.h
        ctlox.cpp
        include/ctlox/v1/interpreter_ct.h
        include/ctlox/v1/numbers.h
        include/ctlox/v1/parser_ct.h
        include/ctlox/v1/scanner_ct.h
        include/ctlox/v1/string_ct.h
        include/ctlox/v1/types.h
)
