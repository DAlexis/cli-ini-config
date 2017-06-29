cmake_minimum_required(VERSION 2.8)

project(cic)

find_package (Boost COMPONENTS date_time program_options system filesystem REQUIRED)

set(LIB_SOURCE
    cic.cpp
    cic.cpp
)

set(${PROJECT_NAME}_USED_INCDIRS
    # Add here includes used by library
)

include_directories(${${PROJECT_NAME}_USED_INCDIRS})

add_library(${PROJECT_NAME} STATIC ${LIB_SOURCE})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_rvalue_references)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_lambdas)

export_include_dirs(
    ${PROJECT_SOURCE_DIR}
    ${${PROJECT_NAME}_USED_INCDIRS}
)

export_libs(
    ${PROJECT_NAME}
    ${Boost_LIBRARIES}
)
