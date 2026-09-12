/********************************************************************************
* @Author : hexne
* @Date   : 2026/09/12 00:42:55
********************************************************************************/

module;
#include <cerrno>
#ifdef _WIN32
// Windows 侧平台层为空实现，无需系统头
#else
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
export module modforge.command;

import std;

NAMESPACE_BEGIN

export struct CommandResult {
    std::optional<std::string> output{};
    std::optional<std::string> error{};
    std::optional<int> exit_code{};
};

export class Command {

public:
    static CommandResult execute(std::string_view command, std::chrono::milliseconds timeout = std::chrono::milliseconds::max()) {
        return {};
    }

    void run(std::string_view command, std::chrono::milliseconds timeout = std::chrono::milliseconds::max()) {

    }

};

NAMESPACE_END
