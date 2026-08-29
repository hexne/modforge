/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/29 20:50:50
********************************************************************************/
module;
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
export module socket;
import std;
import address;

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
export class Socket {
    Address addr_{};
    int fd_ = -1;

    static int to_native(SocketType type) {
        return type == SocketType::stream ? SOCK_STREAM : SOCK_DGRAM;
    }

public:
    explicit Socket(SocketType type) {
        fd_ = ::socket(AF_INET, to_native(type), 0);
        if (fd_ < 0) {
            throw std::runtime_error(std::string("socket failed: ") + std::strerror(errno));
        }
    }

    Socket(SocketType type, const Address& addr) : addr_(addr) {
        fd_ = ::socket(AF_INET, to_native(type), 0);
        if (fd_ < 0) {
            throw std::runtime_error(std::string("socket failed: ") + std::strerror(errno));
        }
    }

    // 接管已有的 fd（accept() 返回的新连接用）
    explicit Socket(const int fd) : fd_(fd) {
        if (fd_ < 0) {
            throw std::invalid_argument("invalid socket fd");
        }
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
    int bind() { return ::bind(fd_, addr_.socket_address(), addr_.size()); }

    // 连接到对端（TCP 三次握手；UDP 只是记下默认对端，不发任何包）
    int connect() { return ::connect(fd_, addr_.socket_address(), addr_.size()); }

    // 单次收发：TCP 由上层循环补齐；UDP 一次调用就是一个数据报
    ssize_t recv(std::span<char> buf) { return ::recv(fd_, buf.data(), buf.size(), 0); }
    ssize_t send(std::span<const char> buf) { return ::send(fd_, buf.data(), buf.size(), 0); }

    // 查询本机绑定的地址（bind 到端口 0 时用它取内核分配的实际端口）
    [[nodiscard]] Address local_address() const {
        sockaddr_in sa{};
        socklen_t len = sizeof(sa);
        if (::getsockname(fd_, reinterpret_cast<sockaddr*>(&sa), &len) != 0)
            return Address{};
        return Address(sa);
    }

    // 读写超时，由内核在阻塞读写上生效
    bool set_timeout(std::chrono::milliseconds recv_timeout,
                     std::chrono::milliseconds send_timeout) {
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
    }

    void close() noexcept {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

    ~Socket() { close(); }
};
