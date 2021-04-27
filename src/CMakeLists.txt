cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

project("cli-ini-config")

add_subdirectory(cic)
add_subdirectory(example1)
add_subdirectory(example2)

# To enable ctest usage
enable_testing()
include(detect-gtest.cmake)
if(GTEST_FOUND)
	add_subdirectory(unit-tests)
endif()

