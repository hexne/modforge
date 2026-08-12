project(modforge_tests)
add_executable(tests
    tests.cpp
    test_time.cpp
    test_timer.cpp
    test_thread_pool.cpp
)
target_link_libraries(tests PRIVATE modforge)
target_compile_features(tests PUBLIC cxx_std_26)

include(CTest)
add_test(NAME test_time COMMAND tests "test_time")
add_test(NAME test_timer COMMAND tests "test_timer")
add_test(NAME test_thread_pool COMMAND tests "test_thread_pool")
