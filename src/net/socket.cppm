/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/29 20:50:50
********************************************************************************/
module;
#include <cerrno>
#include <cstring>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <winsock2.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif
export module modforge.socket;
import std;
import modforge.address;

// socket 类型：构造时传入，类内不写死 SOCK_STREAM / SOCK_DGRAM
export enum class SocketType {
    stream,    // SOCK_STREAM，面向连接（TCP）
    datagram,  // SOCK_DGRAM，无连接（UDP）
};

// socket 底座：fd 生命周期 + TCP/UDP 通用系统调用的薄映射。
// 由 TCP / UDP 组合持有（不继承，各自的接口由各自类声明）：
//   - 协议特有的调用不在这里：listen/accept 在 TCP，sendto/recvfrom 在 UDP
//     （谁调用系统调用，谁 include 它的头）
//   - "策略"也不在这里：循环补齐的 send_all/recv_all 是字节流假设，只在 tcp.cppm
// 收发一律为阻塞语义。
//
// 平台说明（2026-09-04）：
//   - POSIX（Linux/macOS）：真实系统调用。
//   - Windows：当前为空实现（仅保证编译/链接通过，避免跨平台报错）。
//     构造不抛异常（fd_ = -1），所有系统调用类方法返回失败值
//     （bind/connect/recv/send = -1，set_timeout = false，local_address = 默认 Address）。
//     待后续接入 Winsock（WSAStartup + SOCKET 句柄 + closesocket + WSAGetLastError）后替换。
export class Socket {
    Address addr_{};
    int fd_ = -1;

    static int to_native(SocketType type) {
        // SOCK_STREAM / SOCK_DGRAM 在 winsock2.h 中数值与 POSIX 一致（1 / 2）
        return type == SocketType::stream ? SOCK_STREAM : SOCK_DGRAM;
    }

public:
    explicit Socket(SocketType type) {
#ifdef _WIN32
        // Windows 空实现：不创建真实 socket
        fd_ = -1;
#else
        fd_ = ::socket(AF_INET, to_native(type), 0);
        if (fd_ < 0) {
            throw std::runtime_error(std::string("socket failed: ") + std::strerror(errno));
        }
#endif
    }

    Socket(SocketType type, const Address& addr) : addr_(addr) {
#ifdef _WIN32
        // Windows 空实现：不创建真实 socket
        fd_ = -1;
#else
        fd_ = ::socket(AF_INET, to_native(type), 0);
        if (fd_ < 0) {
            throw std::runtime_error(std::string("socket failed: ") + std::strerror(errno));
        }
#endif
    }

    // 接管已有的 fd（accept() 返回的新连接用）
    explicit Socket(const int fd) : fd_(fd) {
#ifdef _WIN32
        // Windows 空实现：无 int fd 概念（Winsock 用 SOCKET 句柄），一律视为无效
        fd_ = -1;
#else
        if (fd_ < 0) {
            throw std::invalid_argument("invalid socket fd");
        }
#endif
    }

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept
        : addr_(other.addr_), fd_(std::exchange(other.fd_, -1)) {}

    Socket& operator=(Socket&& other) noexcept {
        if (this != &other) {
            close();
            addr_ = other.addr_;
            fd_ = std::exchange(other.fd_, -1);
        }
        return *this;
    }

    [[nodiscard]] int fd() const { return fd_; }
    [[nodiscard]] const Address& address() const { return addr_; }

    // 绑定到本地地址（服务端）
    int bind() {
#ifdef _WIN32
        return -1; // Windows 空实现
#else
        return ::bind(fd_, addr_.socket_address(), addr_.size());
#endif
    }

    // 连接到对端（TCP 三次握手；UDP 只是记下默认对端，不发任何包）
    int connect() {
#ifdef _WIN32
        return -1; // Windows 空实现
#else
        return ::connect(fd_, addr_.socket_address(), addr_.size());
#endif
    }

    // 单次收发：TCP 由上层循环补齐；UDP 一次调用就是一个数据报。
    // 返回 std::ptrdiff_t 而非 ssize_t，避免把 POSIX 名字漏进导入方（与 tcp.cppm 的 read_some 一致）
    std::ptrdiff_t recv(std::span<char> buf) {
#ifdef _WIN32
        return -1; // Windows 空实现
#else
        return static_cast<std::ptrdiff_t>(::recv(fd_, buf.data(), buf.size(), 0));
#endif
    }
    std::ptrdiff_t send(std::span<const char> buf) {
#ifdef _WIN32
        return -1; // Windows 空实现
#else
        return static_cast<std::ptrdiff_t>(::send(fd_, buf.data(), buf.size(), 0));
#endif
    }

    // 查询本机绑定的地址（bind 到端口 0 时用它取内核分配的实际端口）
    [[nodiscard]] Address local_address() const {
#ifdef _WIN32
        return Address{}; // Windows 空实现
#else
        sockaddr_in sa{};
        socklen_t len = sizeof(sa);
        if (::getsockname(fd_, reinterpret_cast<sockaddr*>(&sa), &len) != 0)
            return Address{};
        return Address(sa);
#endif
    }

    // 读写超时，由内核在阻塞读写上生效
    bool set_timeout(std::chrono::milliseconds recv_timeout,
                     std::chrono::milliseconds send_timeout) {
#ifdef _WIN32
        (void)recv_timeout;
        (void)send_timeout;
        return false; // Windows 空实现
#else
        auto to_timeval = [](std::chrono::milliseconds ms) {
            auto secs = std::chrono::duration_cast<std::chrono::seconds>(ms);
            timeval tv{};
            tv.tv_sec  = static_cast<long>(secs.count());
            tv.tv_usec = static_cast<long>((ms - secs).count()) * 1000;
            return tv;
        };

        timeval rt = to_timeval(recv_timeout);
        timeval st = to_timeval(send_timeout);

        return ::setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &rt, sizeof(rt)) == 0
            && ::setsockopt(fd_, SOL_SOCKET, SO_SNDTIMEO, &st, sizeof(st)) == 0;
#endif
    }

    void close() noexcept {
        if (fd_ >= 0) {
#ifndef _WIN32
            ::close(fd_);
#endif
            fd_ = -1;
        }
    }

    ~Socket() { close(); }
};
