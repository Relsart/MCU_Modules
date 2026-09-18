aux_source_directory(./ srcDir)
get_filename_component(DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(NAME_TEST ${PROJECT_NAME}_builders_${DIR_NAME})

add_executable(${NAME_TEST}
    ${PROJECT_SOURCE_DIR}/testSrc/test.cpp
    ${PROJECT_SOURCE_DIR}/../src/EventLoop.cpp
    ${PROJECT_SOURCE_DIR}/../src/Interface.cpp
    ${srcDir}
)

include_directories(${PROJECT_SOURCE_DIR}/../include)
include_directories(${PROJECT_SOURCE_DIR}/testSrc/slotSignalTest)

target_link_libraries(${NAME_TEST} gmock gtest pthread gcov)
set_target_properties(${NAME_TEST} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
add_dependencies(${PROJECT_NAME} ${NAME_TEST})
add_test(NAME ${NAME_TEST} COMMAND ${NAME_TEST})
