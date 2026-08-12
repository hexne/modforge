/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/12
********************************************************************************/
module;
export module modforge.args_parser;

import std;

export class ArgsParser {
public:
    explicit ArgsParser(std::vector<std::string> args = {}) {
        for (std::size_t i = 1; i < args.size(); ++i) {
            auto pos = args[i].find('=');
            if (pos != std::string::npos) {
                options_[args[i].substr(0, pos)] = args[i].substr(pos + 1);
            } else {
                flags_.push_back(args[i]);
            }
        }
    }

    bool has_flag(const std::string& flag) const {
        return std::find(flags_.begin(), flags_.end(), flag) != flags_.end();
    }

    std::string get_value(const std::string& name, const std::string& default_value = "") const {
        auto it = options_.find(name);
        return it == options_.end() ? default_value : it->second;
    }

private:
    std::vector<std::string> flags_;
    std::map<std::string, std::string> options_;
};
