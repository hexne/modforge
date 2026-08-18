import std;
import modforge;

int test_args_parser() {
    modforge::ArgsParser parser;

    bool seen_v = false;
    bool seen_d = false;
    std::string name_value;

    parser.add_args("--verbose", "-v", std::function<void(std::optional<modforge::InputArgs>)>([&](std::optional<modforge::InputArgs> a){
        (void)a;
        seen_v = true;
    }));

    parser.add_args("--dry-run", "-d", std::function<void(std::optional<modforge::InputArgs>)>([&](std::optional<modforge::InputArgs> a){
        (void)a;
        seen_d = true;
    }));

    parser.add_args("--name", "-n", std::function<void(std::optional<modforge::InputArgs>)>([&](std::optional<modforge::InputArgs> a){
        if (a) name_value = a->get_value();
    }));

    const char* argv[] = {"app", "-vd", "--name=alice", "-n=bob"};
    int argc = sizeof(argv) / sizeof(argv[0]);

    parser.analysis(argc, const_cast<char**>(argv));

    if (!seen_v) return 1;
    if (!seen_d) return 2;
    if (name_value != "bob") return 3;

    modforge::ArgsParser parser2;
    std::vector<std::string> seen_values;
    std::vector<std::string> seen_flags;
    std::string last_name;

    parser2.add_args("--verbose", "-v", std::function<void(std::optional<modforge::InputArgs>)>([&](std::optional<modforge::InputArgs> a){
        (void)a;
        seen_flags.push_back("verbose");
    }));
    parser2.add_args("--dry-run", "-d", std::function<void(std::optional<modforge::InputArgs>)>([&](std::optional<modforge::InputArgs> a){
        (void)a;
        seen_flags.push_back("dry-run");
    }));
    parser2.add_args("--name", "-n", std::function<void(std::optional<modforge::InputArgs>)>([&](std::optional<modforge::InputArgs> a){
        if (a) {
            auto value = a->get_value();
            seen_values.push_back(value);
            last_name = value;
        }
    }));

    const char* more_argv[] = {"app", "-vd", "--name=alice", "-n=bob", "--name=charlie"};
    int more_argc = sizeof(more_argv) / sizeof(more_argv[0]);
    parser2.analysis(more_argc, const_cast<char**>(more_argv));

    if (seen_flags.size() != 2) return 4;
    if (seen_flags[0] != "verbose" || seen_flags[1] != "dry-run") return 5;
    if (seen_values.size() != 3) return 6;
    if (seen_values[0] != "alice" || seen_values[1] != "bob" || seen_values[2] != "charlie") return 7;
    if (last_name != "charlie") return 8;

    modforge::ArgsParser parser3;
    std::string repeated_value;
    parser3.add_args("--name", "-n", std::function<void(std::optional<modforge::InputArgs>)>([&](std::optional<modforge::InputArgs> a){
        if (a) repeated_value = a->get_value();
    }));

    const char* repeated_argv[] = {"app", "--name=first", "-n=second"};
    int repeated_argc = sizeof(repeated_argv) / sizeof(repeated_argv[0]);
    parser3.analysis(repeated_argc, const_cast<char**>(repeated_argv));
    if (repeated_value != "second") return 9;

    modforge::InputArgs value_holder;
    value_holder.set_value("ready");
    if (value_holder.get_value() != "ready") return 10;

    modforge::ArgsParser parser4;
    bool saw_alpha = false;
    bool saw_beta = false;
    parser4.add_args("--alpha", "-a", std::function<void(std::optional<modforge::InputArgs>)>([&](std::optional<modforge::InputArgs> a){
        (void)a;
        saw_alpha = true;
    }));
    parser4.add_args("--beta", "-b", std::function<void(std::optional<modforge::InputArgs>)>([&](std::optional<modforge::InputArgs> a){
        (void)a;
        saw_beta = true;
    }));

    const char* combined_argv[] = {"app", "-ab"};
    int combined_argc = sizeof(combined_argv) / sizeof(combined_argv[0]);
    parser4.analysis(combined_argc, const_cast<char**>(combined_argv));
    if (!saw_alpha || !saw_beta) return 11;

    modforge::ArgsParser parser5;
    parser5.add_args("--name", "-n", [](std::optional<modforge::InputArgs>) {});
    try {
        const char* bad_argv[] = {"app", "--unknown"};
        parser5.analysis(2, const_cast<char**>(bad_argv));
        return 12;
    } catch (const std::runtime_error&) {
    }

    try {
        const char* bad_argv2[] = {"app", "--name=alice=extra"};
        parser5.analysis(2, const_cast<char**>(bad_argv2));
        return 13;
    } catch (const std::runtime_error&) {
    }

    try {
        const char* positional_argv[] = {"app", "positional"};
        parser5.analysis(2, const_cast<char**>(positional_argv));
        return 14;
    } catch (const std::runtime_error&) {
    }

    return 0;
}
