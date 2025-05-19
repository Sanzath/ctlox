cmake_minimum_required(VERSION 3.31)
project(ctlox)

set(CMAKE_CXX_STANDARD 23)

include_directories(.)

add_executable(ctlox
        ct.h
        ctlox.cpp
        interpreter_ct.h
        numbers.h
        parser_ct.h
        scanner_ct.h
        string_ct.h
        types.h
        interpreter.h)
