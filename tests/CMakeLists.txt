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
    test_signal.cpp
    test_event.cpp
    test_table.cpp
    test_id_generator.cpp
)
# Windows 侧 terminal / socket / tcp / udp 为空实现（仅编译/链接通过），行为测试无意义，剔除。
if(NOT WIN32)
    target_sources(tests PRIVATE
        test_terminal.cpp
        test_socket.cpp
        test_tcp.cpp
        test_udp.cpp
    )
endif()
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
add_test(NAME test_signal COMMAND tests "test_signal")
add_test(NAME test_event COMMAND tests "test_event")
add_test(NAME test_table COMMAND tests "test_table")
add_test(NAME test_id_generator COMMAND tests "test_id_generator")

if(NOT WIN32)
    add_test(NAME test_terminal COMMAND tests "test_terminal")
    add_test(NAME test_socket COMMAND tests "test_socket")
    add_test(NAME test_tcp COMMAND tests "test_tcp")
    add_test(NAME test_udp COMMAND tests "test_udp")
endif()

if(MODFORGE_ENABLE_REFLECTION)
    target_sources(tests PRIVATE test_static_serialize.cpp test_config_generator.cpp)
    add_test(NAME test_static_serialize COMMAND tests "test_static_serialize")
    add_test(NAME test_config_generator COMMAND tests "test_config_generator")
endif()
