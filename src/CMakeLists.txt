include_directories(.)

add_library(ctlox_lib INTERFACE)
target_include_directories(ctlox_lib INTERFACE ../include)

add_executable(ctlox
        main.cpp
        v1.cpp
        v2.cpp
)

target_link_libraries(ctlox PRIVATE ctlox_lib)
