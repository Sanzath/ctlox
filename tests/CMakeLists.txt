add_executable(ctlox_tests
        v1/test_ct.cpp
        main.cpp
        v1/test_scanner.cpp
        v1/test_parser.cpp
        v1/test_interpreter.cpp
)

target_link_libraries(ctlox_tests PRIVATE ctlox_lib)