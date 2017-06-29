cmake_minimum_required(VERSION 2.8)

project(cic-test)

find_package (GTest REQUIRED)

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
)

add_test(NAME cic_testing
         COMMAND ${PROJECT_NAME})

run_test(${PROJECT_NAME})
