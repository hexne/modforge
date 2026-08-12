import std;

int test_time();
int test_timer();
int test_thread_pool();

int main(int argc, char** argv) {
    if (argc != 2) {
        throw std::runtime_error("usage: tests <test_name>");
    }

    std::map<std::string, std::function<int()>> tests = {
        {"test_time", test_time},
        {"test_timer", test_timer},
        {"test_thread_pool", test_thread_pool},
    };

    const std::string name = argv[1];
    const auto it = tests.find(name);
    if (it == tests.end()) {
        throw std::runtime_error("unknown test: " + name);
    }

    return it->second();
}
