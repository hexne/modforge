/*******************************************************************************
 * @Author : hexne
 * @Data   : 2024/11/19 21:15
*******************************************************************************/

module;
#ifdef Debug
#include <iostream>
#endif

#include <map>
#include <functional>
#include <ostream>
#include <string>
#include <vector>
export module modforge.param;


class Param {
    std::vector<std::string> params_;
    std::map<std::string, std::function<void()>> support_params_;
    // 方案2 :
    template<size_t ... index>
    void add_param_impl(auto && tuple, auto &&callback, std::index_sequence<index...>) {
        auto lam = [&] (const std::string &str) {
            support_params_.insert({str,callback});
        };
        (lam(std::get<index>(tuple)), ...);
    }
public:
    Param(const int argc,const char *const argv[]) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            // - a -a - asd -asd
            if (arg == "-" || arg[0] == '-' && arg[1] != '-') {
                if (arg.size() == 1) {
                    for (auto ch : std::string_view{argv[i+1]}) {
                        params_.push_back({ch});
                    }
                    i += 1;
                }
                else {
                    for (auto ch : std::string_view{arg.begin() + 1, arg.end()}) {
                        params_.push_back({ch});
                    }
                }
            }
            // -- all --all
            else if (arg == "--" || arg.find("--") != std::string::npos) {
                if (arg.size() == 2) {
                    params_.push_back({argv[i+1]});
                    i += 1;
                }
                else {
                    params_.push_back(std::string(arg.begin() + 2, arg.end()));
                }

            }
            else {
                throw std::invalid_argument("Invalid argument");
            }
        }
    }

    /*
     * 方案1 :
    template<int index>
    void add_param_callback_impl(auto &&tuple, std::function<void(const std::string &)> func) {
        if constexpr (index + 1 >= std::tuple_size_v<std::remove_reference_t<decltype(tuple)>> ) {
            return;
        }
        else {
            func(std::get<index>(tuple));
            add_param_callback_impl<index+1>(tuple, func);
        }
    }

    void add_param_callback(auto &&... args) {
        auto tuple = std::forward_as_tuple(args...);
        auto callback = std::get<sizeof ...(args) - 1>(tuple);
        add_param_callback_impl<0>(std::forward_as_tuple(args...), [this,&callback] (const std::string &str){
            if (std::ranges::find(params_,str) != params_.end()) {
                callback();
            }
        });
    }
    */
    void analyze() {

        for (auto arg : params_) {
            if (!support_params_.contains(arg)  ) {
                throw std::invalid_argument("Invalid argument : " + arg);
            }
            support_params_.find(arg)->second();
        }


    }

    void add_param(auto &&... args) {
        auto tuple = std::forward_as_tuple(args...);
        auto callback = std::get<sizeof ...(args) - 1>(tuple);
        add_param_impl(tuple,callback,std::make_index_sequence<sizeof...(args) - 1>());
    }
    [[nodiscard]]
    bool have_param() const {
        return params_.size() > 1;
    }
    [[nodiscard]]
    size_t count_param() const {
        return params_.size();
    }

};
