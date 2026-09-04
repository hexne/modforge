/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/29 20:50:50
********************************************************************************/
module;
#include <cerrno>
#include <cstring>
#ifdef _WIN32
// mingw-gcc modules bug workaround：见 src/terminal.cppm 注释（cstddef 预热 c++config.h guard）
#include <cstddef>
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

/** @brief socket 类型：构造时传入，不写死 SOCK_STREAM / SOCK_DGRAM */
export enum class SocketType {
    stream,    // 面向连接（TCP）
    datagram,  // 无连接（UDP）
};

#ifdef _WIN32
// winsock 惰性初始化：首次调用即完成，之后线程安全（C++ magic static）
void ensure_winsock_started() {
    static const bool ok = [] {
        WSADATA data;
        return ::WSAStartup(MAKEWORD(2, 2), &data) == 0;
    }();
    if (!ok)
        throw std::runtime_error("WSAStartup failed");
}
#endif

/** @brief 平台化错误消息（POSIX: strerror；Windows: WSAGetLastError）
 *  @param what 失败的操作名
 *  @return 拼接了平台错误描述的消息
 */
std::string socket_error_message(const char* what) {
#ifdef _WIN32
    return std::string(what) + " failed: error " + std::to_string(::WSAGetLastError());
#else
    return std::string(what) + " failed: " + std::strerror(errno);
#endif
}

/** @brief socket 底座：句柄生命周期 + TCP/UDP 通用系统调用的薄映射，由 TCP/UDP 组合持有
 *  @note  句柄内部统一存 std::intptr_t、无效值恒 -1（Windows INVALID_SOCKET 位模式即 -1），
 *         平台差异只在系统调用点分支；fd() 返回平台原生句柄类型（POSIX int / Windows SOCKET）。
 *         协议特有调用（listen/accept、sendto/recvfrom）在各协议模块，不在此类。
 */
export class Socket {
    Address addr_{};
    std::intptr_t fd_ = -1;  // 无效值恒 -1（Windows INVALID_SOCKET 位模式即 -1）

    static int to_native(SocketType type) {
        return type == SocketType::stream ? SOCK_STREAM : SOCK_DGRAM;  // winsock 与 POSIX 数值一致
    }

public:
    /** @brief 创建 socket
     *  @param type socket 类型（stream=TCP / datagram=UDP）
     */
    explicit Socket(SocketType type) {
#ifdef _WIN32
        ensure_winsock_started();
        const SOCKET s = ::socket(AF_INET, to_native(type), 0);
        fd_ = (s == INVALID_SOCKET) ? -1 : static_cast<std::intptr_t>(s);
#else
        fd_ = ::socket(AF_INET, to_native(type), 0);
#endif
        if (fd_ < 0)
            throw std::runtime_error(socket_error_message("socket"));
    }

    /** @brief 创建 socket 并指定端点
     *  @param type socket 类型（stream=TCP / datagram=UDP）
     *  @param addr 关联的端点地址
     */
    Socket(SocketType type, const Address& addr) : addr_(addr) {
#ifdef _WIN32
        ensure_winsock_started();
        const SOCKET s = ::socket(AF_INET, to_native(type), 0);
        fd_ = (s == INVALID_SOCKET) ? -1 : static_cast<std::intptr_t>(s);
#else
        fd_ = ::socket(AF_INET, to_native(type), 0);
#endif
        if (fd_ < 0)
            throw std::runtime_error(socket_error_message("socket"));
    }

    /** @brief 接管已有句柄（accept() 返回的新连接用）
     *  @param fd 平台原生句柄，intptr_t 兼容 int fd / SOCKET
     */
    explicit Socket(const std::intptr_t fd) : fd_(fd) {
        if (fd_ < 0)
            throw std::invalid_argument("invalid socket handle");
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

    /** @brief 平台原生句柄：POSIX int / Windows SOCKET（命名 SOCKET 需自行 include 平台头） */
#ifdef _WIN32
    [[nodiscard]] SOCKET fd() const { return static_cast<SOCKET>(fd_); }
#else
    [[nodiscard]] int fd() const { return static_cast<int>(fd_); }
#endif
    [[nodiscard]] const Address& address() const { return addr_; }

    /** @brief 绑定到本地地址（服务端） */
    int bind() {
#ifdef _WIN32
        return ::bind(fd(), addr_.socket_address(), static_cast<int>(addr_.size()));
#else
        return ::bind(fd(), addr_.socket_address(), addr_.size());
#endif
    }

    /** @brief 连接到对端（TCP 三次握手；UDP 仅记下默认对端，不发包） */
    int connect() {
#ifdef _WIN32
        return ::connect(fd(), addr_.socket_address(), static_cast<int>(addr_.size()));
#else
        return ::connect(fd(), addr_.socket_address(), addr_.size());
#endif
    }

    /** @brief 单次接收数据
     *  @param buf 接收缓冲区；单次调用最多填一个数据报（UDP）或一段字节流（TCP）
     *  @return >0 字节数；0 = 对端关闭（TCP）/ 空数据报（UDP）；-1 = 出错
     *  @note  返回 std::ptrdiff_t 而非 ssize_t，避免把 POSIX 名字漏进导入方
     */
    std::ptrdiff_t recv(std::span<char> buf) {
#ifdef _WIN32
        // winsock 的长度参数是 int，超长 buffer 需 clamp（实际 >2GB 单次读不现实，防御而已）
        const int len = static_cast<int>(
            std::min(buf.size(), static_cast<size_t>(std::numeric_limits<int>::max())));
        const int n = ::recv(fd(), buf.data(), len, 0);
        return n == SOCKET_ERROR ? -1 : static_cast<std::ptrdiff_t>(n);
#else
        return static_cast<std::ptrdiff_t>(::recv(fd(), buf.data(), buf.size(), 0));
#endif
    }
    /** @brief 单次发送数据
     *  @param buf 待发送数据；UDP 下整体作为一个数据报
     *  @return >0 已发送字节数；-1 = 出错
     */
    std::ptrdiff_t send(std::span<const char> buf) {
#ifdef _WIN32
        const int len = static_cast<int>(
            std::min(buf.size(), static_cast<size_t>(std::numeric_limits<int>::max())));
        const int n = ::send(fd(), buf.data(), len, 0);
        return n == SOCKET_ERROR ? -1 : static_cast<std::ptrdiff_t>(n);
#else
        return static_cast<std::ptrdiff_t>(::send(fd(), buf.data(), buf.size(), 0));
#endif
    }

    /** @brief 查询本机绑定地址（bind 到端口 0 时取内核分配的实际端口） */
    [[nodiscard]] Address local_address() const {
#ifdef _WIN32
        sockaddr_in sa{};
        int len = sizeof(sa);
        if (::getsockname(fd(), reinterpret_cast<sockaddr*>(&sa), &len) != 0)
            return Address{};
        return Address(sa);
#else
        sockaddr_in sa{};
        socklen_t len = sizeof(sa);
        if (::getsockname(fd(), reinterpret_cast<sockaddr*>(&sa), &len) != 0)
            return Address{};
        return Address(sa);
#endif
    }

    /** @brief 设置阻塞读写的内核超时
     *  @param recv_timeout 接收超时（毫秒）
     *  @param send_timeout 发送超时（毫秒）
     *  @return 两个方向均设置成功时为 true
     *  @note  POSIX 用 timeval、winsock 用 DWORD，语义一致
     */
    bool set_timeout(std::chrono::milliseconds recv_timeout,
                     std::chrono::milliseconds send_timeout) {
#ifdef _WIN32
        const DWORD rt = static_cast<DWORD>(recv_timeout.count());
        const DWORD st = static_cast<DWORD>(send_timeout.count());
        return ::setsockopt(fd(), SOL_SOCKET, SO_RCVTIMEO,
                            reinterpret_cast<const char*>(&rt), sizeof(rt)) == 0
            && ::setsockopt(fd(), SOL_SOCKET, SO_SNDTIMEO,
                            reinterpret_cast<const char*>(&st), sizeof(st)) == 0;
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

        return ::setsockopt(fd(), SOL_SOCKET, SO_RCVTIMEO, &rt, sizeof(rt)) == 0
            && ::setsockopt(fd(), SOL_SOCKET, SO_SNDTIMEO, &st, sizeof(st)) == 0;
#endif
    }

    void close() noexcept {
        if (fd_ >= 0) {
#ifdef _WIN32
            ::closesocket(fd());
#else
            ::close(fd());
#endif
            fd_ = -1;
        }
    }

    ~Socket() { close(); }
};
