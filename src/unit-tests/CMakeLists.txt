cmake_minimum_required(VERSION 2.8)

project(cic-test)

find_package (GTest REQUIRED)
find_package (Threads)

include_directories (
    ${GTEST_INCLUDE_DIR}
    ${cic_INCLUDE_DIRS}
)

set(EXE_SOURCES
    cictest.cpp
)

add_executable(${PROJECT_NAME} ${EXE_SOURCES})

target_link_libraries (${PROJECT_NAME}
    gtest
    gtest_main
    ${cic_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
)

add_test(NAME cic_testing
         COMMAND ${PROJECT_NAME})

add_unit_test(${PROJECT_NAME})
