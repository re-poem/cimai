# cimai 测试套件
set(CIMAI_TESTS 
    test_core 
    test_golden
)
foreach(t IN LISTS CIMAI_TESTS)
    add_executable(${t} ${t}.c)
    target_link_libraries(${t} PRIVATE cimai)
    target_include_directories(${t} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME ${t} COMMAND ${t})
endforeach()

# golden 测试需要定位 tests/data 下的参考文件
target_compile_definitions(test_golden PRIVATE TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
