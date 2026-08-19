import std;

int test_args_parser();
int test_directory();
int test_file();
int test_lock_free_queue();
int test_string_utils();
int test_time();
int test_timer();
int test_thread_pool();
int test_tree();
int test_utils();

int main(int argc, char** argv) {
    if (argc != 2) {
        throw std::runtime_error("usage: tests <test_name>");
    }

    std::map<std::string, std::function<int()>> tests = {
        {"test_args_parser", test_args_parser},
        {"test_directory", test_directory},
        {"test_file", test_file},
        {"test_lock_free_queue", test_lock_free_queue},
        {"test_string_utils", test_string_utils},
        {"test_time", test_time},
        {"test_timer", test_timer},
        {"test_thread_pool", test_thread_pool},
        {"test_tree", test_tree},
        {"test_utils", test_utils},
    };

    const std::string name = argv[1];
    const auto it = tests.find(name);
    if (it == tests.end()) {
        throw std::runtime_error("unknown test: " + name);
    }

    return it->second();
}
