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

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} ${Boost_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PUBLIC ${Boost_LIBRARIES})
