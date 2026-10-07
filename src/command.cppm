/********************************************************************************
* @Author : hexne
* @Date   : 2026/09/20 10:54:18
********************************************************************************/

module;
export module modforge.command;

import std;
import modforge.os;


export class CommandResult {
public:
    std::optional<std::string> output;
    std::optional<int> exit_code;
    std::optional<int> signal;
};

// 两个平台共用这个内部接口；平台差异只存在于下面的 #if 分支里。
struct ProcessControl;

[[nodiscard]] CommandResult run_impl(std::span<const std::string> arguments,
                                     const std::shared_ptr<ProcessControl>& control);

#if defined(_WIN32)

// 句柄 RAII：保证任何提前 return 都不会泄漏句柄
class HandleGuard {
public:
    HandleGuard() = default;
    explicit HandleGuard(HANDLE handle) noexcept : handle_(handle) {}
    ~HandleGuard() { reset(); }

    HandleGuard(const HandleGuard&) = delete;
    HandleGuard& operator=(const HandleGuard&) = delete;

    HandleGuard(HandleGuard&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }

    HandleGuard& operator=(HandleGuard&& other) noexcept {
        if (this != &other) {
            reset();
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    void reset(HANDLE handle = nullptr) noexcept {
        if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE) {
            ::CloseHandle(handle_);
        }
        handle_ = handle;
    }

    [[nodiscard]] HANDLE get() const noexcept { return handle_; }

private:
    HANDLE handle_{};
};

// 调用方（stop()）与工作线程共享，用来在任意时刻结束子进程。
// process 句柄的“关闭”也由工作线程在锁内完成，避免 stop() 拿到已关闭的句柄。
struct ProcessControl {
    std::mutex mutex;
    HANDLE process{};
    HANDLE job{};            // 子进程所属的 Job Object，stop() 时整棵树连坐
    HANDLE reader_thread{};  // 正阻塞在 ReadFile 上的线程（真实 Win32 句柄）
    bool stop_requested{};

    // 全部在锁内做：保证不会拿到已经关闭、甚至被复用的句柄
    void stop() noexcept {
        std::lock_guard lock{mutex};
        stop_requested = true;
        if (job != nullptr) {
            ::TerminateJobObject(job, 1);  // 连同子进程派生出的后代一起终止
        } else if (process != nullptr) {
            ::TerminateProcess(process, 1);  // 没有 job 时只能退化成杀直接子进程
        }
        if (reader_thread != nullptr) {
            ::CancelSynchronousIo(reader_thread);  // 打断阻塞中的同步 ReadFile
        }
    }
};

// UTF-8 -> UTF-16（忽略编码问题的前提下按 UTF-8 处理）
[[nodiscard]] std::wstring widen(std::string_view utf8) {
    if (utf8.empty()) {
        return {};
    }

    const int size = ::MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                                           static_cast<int>(utf8.size()), nullptr, 0);
    if (size <= 0) {
        return {};
    }

    std::wstring wide(static_cast<std::size_t>(size), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()),
                          wide.data(), size);
    return wide;
}

// 参数原样按空格拼接；需要引号或转义时由调用方自己写，例如：
//   Command::run("cmd", "/c", "dir", R"("C:\Program Files")")
[[nodiscard]] std::string build_command_line(std::span<const std::string> arguments) {
    std::string command_line;
    for (const auto& argument : arguments) {
        if (!command_line.empty()) {
            command_line.push_back(' ');
        }
        command_line += argument;   // 原样拼接，引号/转义由调用者负责
    }
    return command_line;
}

// 真正的平台实现：CreatePipe + CreateProcessW，stdout/stderr 合并读取
[[nodiscard]] CommandResult run_impl(std::span<const std::string> arguments, const std::shared_ptr<ProcessControl>& control) {
    CommandResult result;
    result.output.emplace();

    const std::string command_line = build_command_line(arguments);
    std::wstring wide_command_line = widen(command_line);
    if (wide_command_line.empty()) {
        result.output = "command line is empty";
        return result;
    }

    SECURITY_ATTRIBUTES attributes{};
    attributes.nLength = sizeof(attributes);
    attributes.bInheritHandle = TRUE;
    attributes.lpSecurityDescriptor = nullptr;

    HANDLE read_end = nullptr;
    HANDLE write_end = nullptr;
    if (::CreatePipe(&read_end, &write_end, &attributes, 0) == FALSE) {
        result.output = "CreatePipe failed, GetLastError=" + std::to_string(::GetLastError());
        return result;
    }

    HandleGuard pipe_read{read_end};
    HandleGuard pipe_write{write_end};

    // 父进程持有的读端不能被子进程继承
    SetHandleInformation(pipe_read.get(), HANDLE_FLAG_INHERIT, 0);

    // Job Object：把子进程归进一个 job，stop() 才能对整棵进程树连坐。
    // KILL_ON_JOB_CLOSE 是第二道保险：job 句柄一关，残留进程也会被收掉。
    HandleGuard job{::CreateJobObjectW(nullptr, nullptr)};
    if (job.get() != nullptr) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
        limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (SetInformationJobObject(job.get(), JobObjectExtendedLimitInformation, &limits,
                                      sizeof(limits)) == FALSE) {
            job.reset();  // 配置失败就退回"只杀直接子进程"
        }
    }

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);
    if (startup.hStdInput == nullptr || startup.hStdInput == INVALID_HANDLE_VALUE) {
        startup.hStdInput = nullptr;  // 没有可用 stdin 时不重定向
    }
    startup.hStdOutput = pipe_write.get();
    startup.hStdError = pipe_write.get();

    PROCESS_INFORMATION process_info{};
    // CREATE_SUSPENDED：先挂住，等归入 job 之后再放行，避免它抢跑出 job 之外
    if (::CreateProcessW(nullptr, wide_command_line.data(), nullptr, nullptr, TRUE,
                         CREATE_NO_WINDOW | CREATE_SUSPENDED, nullptr, nullptr, &startup,
                         &process_info) == FALSE) {
        result.output = "CreateProcessW failed, GetLastError=" + std::to_string(::GetLastError());
        return result;
    }

    HandleGuard process{process_info.hProcess};
    HandleGuard thread{process_info.hThread};

    // 进程还在挂起状态：先归入 job 再放行，它之后派生的进程就都在 job 里
    if (job.get() != nullptr && ::AssignProcessToJobObject(job.get(), process.get()) == FALSE) {
        job.reset();  // 例如外层 job 不允许嵌套，退回"只杀直接子进程"
    }

    // 父进程必须放掉写端，否则子进程退出后读端等不到 EOF
    pipe_write.reset();

    bool stop_immediately = false;
    {
        std::lock_guard lock{control->mutex};
        control->process = process.get();
        control->job = job.get();
        stop_immediately = control->stop_requested;
    }

    ::ResumeThread(thread.get());

    // 补掉"进程登记之前就有人调了 stop()"的窗口
    if (stop_immediately) {
        control->stop();
    }

    // 边读边追加；子进程结束时写端关闭，ReadFile 返回 0
    std::thread reader{[&] {
        // 先把自己的真实线程句柄登记出去，好让 stop() 能打断这个阻塞的 ReadFile
        HANDLE self = nullptr;
        DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(),
                          &self, THREAD_TERMINATE, FALSE, 0);
        {
            std::lock_guard lock{control->mutex};
            control->reader_thread = self;
        }

        std::array<char, 4096> buffer{};
        DWORD count = 0;
        while (::ReadFile(pipe_read.get(), buffer.data(), static_cast<DWORD>(buffer.size()),
                          &count, nullptr) != FALSE &&
               count > 0) {
            result.output->append(buffer.data(), static_cast<std::size_t>(count));

            // 补掉"取消正好落在两次 ReadFile 之间的缝隙里"的窗口
            std::lock_guard lock{control->mutex};
            if (control->stop_requested) {
                break;
            }
        }

        // 先摘牌再关闭，配合 stop() 的锁，杜绝"拿到已复用句柄"的窗口
        {
            std::lock_guard lock{control->mutex};
            control->reader_thread = nullptr;
        }
        if (self != nullptr) {
            ::CloseHandle(self);
        }
    }};

    ::WaitForSingleObject(process.get(), INFINITE);

    DWORD exit_code = 0;
    if (::GetExitCodeProcess(process.get(), &exit_code) != FALSE) {
        result.exit_code = static_cast<int>(exit_code);
    }

    // 直接子进程已退出。若它留下了后代（如 cmd /c start ...），在这里一并收掉：
    // 否则那些后代握着管道写端不放，读线程永远等不到 EOF。
    if (job.get() != nullptr) {
        ::TerminateJobObject(job.get(), 1);
    }

    reader.join();
    pipe_read.reset();

    {
        std::lock_guard lock{control->mutex};
        control->process = nullptr;
        control->job = nullptr;
    }

    return result;
}

#elif defined(__linux__)

class FdGuard {
public:
    FdGuard() = default;
    explicit FdGuard(int fd) noexcept : fd_(fd) {}
    ~FdGuard() { reset(); }

    FdGuard(const FdGuard&) = delete;
    FdGuard& operator=(const FdGuard&) = delete;

    FdGuard(FdGuard&& other) noexcept : fd_(other.fd_) {
        other.fd_ = -1;
    }

    FdGuard& operator=(FdGuard&& other) noexcept {
        if (this != &other) {
            reset();
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    void reset(int fd = -1) noexcept {
        if (fd_ >= 0) {
            ::close(fd_);
        }
        fd_ = fd;
    }

    [[nodiscard]] int release() noexcept {
        const int fd = fd_;
        fd_ = -1;
        return fd;
    }

    [[nodiscard]] int get() const noexcept { return fd_; }

private:
    int fd_{-1};
};

struct ProcessControl {
    std::mutex mutex;
    pid_t pid{};
    int stop_write_fd{-1};
    bool stop_requested{};

    void stop() noexcept {
        std::lock_guard lock{mutex};
        stop_requested = true;

        if (pid > 0) {
            if (::kill(-pid, SIGKILL) == -1) {
                (void)::kill(pid, SIGKILL);
            }
        }

        if (stop_write_fd >= 0) {
            const char byte = 1;
            (void)::write(stop_write_fd, &byte, 1);
        }
    }
};

[[nodiscard]] std::string errno_message(std::string_view prefix, int code) {
    return std::string{prefix} + ", errno=" + std::to_string(code);
}

// Linux 实现：pipe2 + posix_spawnp，stdout/stderr 合并读取。
// 子进程通过 POSIX_SPAWN_SETPGROUP 单独成组，stop() 时用 kill(-pid, SIGKILL) 终止整组。
[[nodiscard]] CommandResult run_impl(std::span<const std::string> arguments,
                                     const std::shared_ptr<ProcessControl>& control) {
    CommandResult result;
    result.output.emplace();

    if (arguments.empty()) {
        result.output = "command line is empty";
        return result;
    }

    std::vector<char*> argv;
    argv.reserve(arguments.size() + 1);
    for (const auto& argument : arguments) {
        argv.push_back(const_cast<char*>(argument.c_str()));
    }
    argv.push_back(nullptr);

    int output_pipe[2]{-1, -1};
    int stop_pipe[2]{-1, -1};

    if (::pipe2(output_pipe, O_CLOEXEC) == -1) {
        result.output = errno_message("pipe2(output) failed", errno_value());
        return result;
    }
    FdGuard out_read{output_pipe[0]};
    FdGuard out_write{output_pipe[1]};

    if (::pipe2(stop_pipe, O_CLOEXEC | O_NONBLOCK) == -1) {
        result.output = errno_message("pipe2(stop) failed", errno_value());
        return result;
    }
    FdGuard stop_read{stop_pipe[0]};
    FdGuard stop_write{stop_pipe[1]};

    posix_spawn_file_actions_t actions;
    if (::posix_spawn_file_actions_init(&actions) != 0) {
        result.output = "posix_spawn_file_actions_init failed";
        return result;
    }

    struct ActionsGuard {
        posix_spawn_file_actions_t* actions;
        ~ActionsGuard() { ::posix_spawn_file_actions_destroy(actions); }
    } actions_guard{&actions};

    posix_spawnattr_t attributes;
    if (::posix_spawnattr_init(&attributes) != 0) {
        result.output = "posix_spawnattr_init failed";
        return result;
    }

    struct AttributesGuard {
        posix_spawnattr_t* attributes;
        ~AttributesGuard() { ::posix_spawnattr_destroy(attributes); }
    } attributes_guard{&attributes};

    int action_error = 0;
    if ((action_error = ::posix_spawn_file_actions_adddup2(
             &actions, out_write.get(), STDOUT_FILENO)) != 0 ||
        (action_error = ::posix_spawn_file_actions_adddup2(
             &actions, out_write.get(), STDERR_FILENO)) != 0 ||
        (action_error = ::posix_spawn_file_actions_addclose(
             &actions, out_read.get())) != 0 ||
        (action_error = ::posix_spawn_file_actions_addclose(
             &actions, out_write.get())) != 0 ||
        (action_error = ::posix_spawn_file_actions_addclose(
             &actions, stop_read.get())) != 0 ||
        (action_error = ::posix_spawn_file_actions_addclose(
             &actions, stop_write.get())) != 0) {
        result.output = errno_message("posix_spawn_file_actions failed", action_error);
        return result;
    }

    const short spawn_flags = POSIX_SPAWN_SETPGROUP;
    if ((action_error = ::posix_spawnattr_setflags(&attributes, spawn_flags)) != 0 ||
        (action_error = ::posix_spawnattr_setpgroup(&attributes, 0)) != 0) {
        result.output = errno_message("posix_spawnattr setup failed", action_error);
        return result;
    }

    pid_t pid = -1;
    const int spawn_error = ::posix_spawnp(
        &pid, argv[0], &actions, &attributes, environ, argv.data());
    if (spawn_error != 0) {
        result.output = errno_message("posix_spawnp failed", spawn_error);
        return result;
    }

    out_write.reset();
    const int stop_write_fd = stop_write.release();

    bool stop_now = false;
    {
        std::lock_guard lock{control->mutex};
        control->pid = pid;
        control->stop_write_fd = stop_write_fd;
        stop_now = control->stop_requested;
    }
    if (stop_now) {
        control->stop();
    }

    // 读线程同时监听输出管道和 stop 管道，避免 stop() 后阻塞在 read()。
    std::thread reader{[&] {
        std::array<char, 4096> buffer{};
        while (true) {
            pollfd fds[2]{};
            fds[0].fd = out_read.get();
            fds[0].events = POLLIN;
            fds[1].fd = stop_read.get();
            fds[1].events = POLLIN;

            const int ready = ::poll(fds, 2, -1);
            if (ready < 0) {
                if (errno_value() == EINTR) {
                    continue;
                }
                break;
            }

            if ((fds[1].revents & POLLIN) != 0) {
                char byte = 0;
                while (::read(stop_read.get(), &byte, 1) > 0) {
                }
                break;
            }

            if ((fds[0].revents & (POLLIN | POLLHUP | POLLERR)) != 0) {
                const ssize_t count = ::read(out_read.get(), buffer.data(), buffer.size());
                if (count > 0) {
                    result.output->append(buffer.data(), static_cast<std::size_t>(count));
                    continue;
                }
                if (count == 0) {
                    break;
                }
                if (errno_value() == EINTR) {
                    continue;
                }
                break;
            }
        }
    }};

    int status = 0;
    pid_t waited = -1;
    do {
        waited = ::waitpid(pid, &status, 0);
    } while (waited == -1 && errno_value() == EINTR);

    // 直接子进程退出后，再把同组残留后代清掉。
    (void)::kill(-pid, SIGKILL);
    reader.join();

    if (waited == pid) {
        if (WIFEXITED(status)) {
            result.exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            result.signal = WTERMSIG(status);
        }
    } else {
        result.output = errno_message("waitpid failed", errno_value());
    }

    {
        std::lock_guard lock{control->mutex};
        control->pid = 0;
        control->stop_write_fd = -1;
    }
    ::close(stop_write_fd);

    return result;
}
#endif

NAMESPACE_BEGIN
export namespace Command {

// 需要停止：调用方自己持有 stop_source，靠它 request_stop()
template <class... Args>
    requires (sizeof...(Args) > 0) && (std::convertible_to<Args, std::string> && ...)
[[nodiscard]]
std::future<CommandResult> run(std::stop_token token, Args&&... args) {
    std::vector<std::string> arguments;
    arguments.reserve(sizeof...(Args));
    (arguments.emplace_back(std::forward<Args>(args)), ...);

    auto control = std::make_shared<ProcessControl>();

    return std::async(std::launch::async, [control, token, arguments = std::move(arguments)]() mutable {
                          std::stop_callback on_stop{token, [control] { control->stop(); }};
                          return run_impl(arguments, control);
                      });
}

// 不需要停止能力：内部给一个永远不会被 request 的 token。
// 顺序不能颠倒——这里是靠调用上面那个带 token 的重载来委托的。
template <class... Args>
    requires (sizeof...(Args) > 0) && (std::convertible_to<Args, std::string> && ...)
[[nodiscard]] std::future<CommandResult> run(Args&&... args) {
    return run(std::stop_token{}, std::forward<Args>(args)...);
}

// 给 timeout 时间：这期间结束了返回 true；超时就 request_stop() 并返回 false。
template <class Rep, class Period>
bool stop(std::stop_source& source, std::future<CommandResult>& future,
          const std::chrono::duration<Rep, Period>& timeout) {
    if (future.wait_for(timeout) != std::future_status::ready) {
        source.request_stop();
        return false;
    }
    return true;
}

}  // namespace Command
NAMESPACE_END