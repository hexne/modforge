/*******************************************************************************
 * @Author : yongheng
 * @Data   : 2024/11/19 21:15
*******************************************************************************/

module;
#include <string>
#include <vector>
#include <functional>
#include "tools.h"
export module Param;


export
NAMESPACE_BEGIN(nl)
class Param {
    std::vector<std::string> params_;
    // 方案2 :
    template<size_t ... index>
    void add_param_callback_impl(auto && tuple, auto &&callback, std::index_sequence<index...>) {
        auto lam = [&] (const std::string &str) {
            if (std::ranges::find(params_, str) != params_.end())
                callback();
        };
        (lam(std::get<index>(tuple)), ...);
    }
public:
    Param(const int argc,const char *const argv[]) {
        for (int i = 0;i < argc; ++i)
            params_.push_back(argv[i]);
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
    void add_param_callback(auto &&... args) {
        auto tuple = std::forward_as_tuple(args...);
        auto callback = std::get<sizeof ...(args) - 1>(tuple);
        add_param_callback_impl(tuple,callback,std::make_index_sequence<sizeof...(args) - 1>());
    }
    [[nodiscard]]
    bool have_param() const {
        return params_.size() > 1;
    }
    [[nodiscard]]
    size_t count_param() const {
        return params_.size() - 1;
    }

};

NAMESPACE_END(nl)
