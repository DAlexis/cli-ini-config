cmake_minimum_required(VERSION 2.8)

project(cic-example)

include_directories (
    ${cic_INCLUDE_DIRS}
)

set(EXE_SOURCES
    example.cpp
)

add_executable(${PROJECT_NAME} ${EXE_SOURCES})

target_link_libraries (${PROJECT_NAME}
    ${cic_LIBRARY}
)
