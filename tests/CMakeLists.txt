add_executable(test
        ${transform_src}
        test.cpp)

target_link_libraries(test ${GTEST_LIBRARIES} pthread)