project(modforge_tests)
add_executable(tests
    tests.cpp
    test_args_parser.cpp
    test_directory.cpp
    test_file.cpp
    test_lock_free_queue.cpp
    test_string_utils.cpp
    test_thread_pool.cpp
    test_time.cpp
    test_timer.cpp
    test_tree.cpp
    test_utils.cpp
)
target_link_libraries(tests PRIVATE modforge)
target_compile_features(tests PUBLIC cxx_std_26)

include(CTest)
add_test(NAME test_args_parser COMMAND tests "test_args_parser")
add_test(NAME test_directory COMMAND tests "test_directory")
add_test(NAME test_file COMMAND tests "test_file")
add_test(NAME test_lock_free_queue COMMAND tests "test_lock_free_queue")
add_test(NAME test_string_utils COMMAND tests "test_string_utils")
add_test(NAME test_time COMMAND tests "test_time")
add_test(NAME test_timer COMMAND tests "test_timer")
add_test(NAME test_thread_pool COMMAND tests "test_thread_pool")
add_test(NAME test_tree COMMAND tests "test_tree")
add_test(NAME test_utils COMMAND tests "test_utils")
