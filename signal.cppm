/*******************************************************************************
 * @Author : hexne
 * @Data   : 2025/02/12 18:02
*******************************************************************************/

module;
#include <csignal>

#include <functional>
#include <iostream>
#include <map>
#include <ostream>
#include <stdexcept>
#include <string>
export module modforge.signal;

void Signal(const std::string &signal, const std::function<void(int)> &callback) {
    static std::map<std::string,int> signal_map {
        {"<Ctrl-C>", SIGINT}
    };
    if (signal_map.find(signal) == signal_map.end())
        throw std::invalid_argument("Signal not found");

    ::signal(signal_map[signal], callback.target<void(int)>());
    std::cout << "注册成功" << std::endl;
}
