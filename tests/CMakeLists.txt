add_executable(ctlox_tests
        main.cpp
        v1/test_ct.cpp
        v1/test_scanner.cpp
        v1/test_parser.cpp
        v1/test_interpreter.cpp
        v2/test_scanner.cpp
)

target_link_libraries(ctlox_tests PRIVATE ctlox_lib)