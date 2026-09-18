cmake_minimum_required(VERSION 3.17)
set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG ${DEBUG_OPT} "-ggdb -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")

project(SlotSignalTest)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

add_compile_options(--coverage)
add_compile_options(-fno-inline -fno-inline-small-functions -fno-default-inline)
add_compile_options(-fno-sanitize=all)
add_definitions(-Wno-narrowing)

#add_compile_options(-MMD -MP)
#add_compile_options(-I)
#add_definitions(-DCMAKE_NO_SYSTEM_FROM_IMPORTED=ON)

add_executable(${PROJECT_NAME} "testSrc/test.cpp")
add_definitions(-DUNIT_TEST)

target_link_libraries(${PROJECT_NAME} gmock gtest pthread gcov)

enable_testing()
add_subdirectory(./testSrc)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND cp ${CMAKE_SOURCE_DIR}/report.sh ${CMAKE_BINARY_DIR}
  COMMAND ./report.sh ${PROJECT_NAME})
