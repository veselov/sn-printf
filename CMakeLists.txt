cmake_minimum_required(VERSION 3.5)
project(sn-printf LANGUAGES "C")

set(CMAKE_C_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE on)

configure_file(sn-printf-config.h.default sn-printf-config.h COPYONLY)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror -Wswitch -Wstrict-prototypes -DSN_PRINTF_TEST")

add_executable(sn-print-test
        sn-printf.c
        sn-printf.h
        tests/tests.c
        tests/test-fw.c
        tests/test-fw.h
        ${CMAKE_CURRENT_BINARY_DIR}/sn-printf-config.h
)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

enable_testing()
add_test(NAME test COMMAND sn-print-test)
set_tests_properties(test PROPERTIES TIMEOUT 600)

