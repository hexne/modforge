/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/29 21:12:03
********************************************************************************/

module;
#include <cerrno>
#ifndef _WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif
export module modforge.udp;
import std;
import modforge.address;
import modforge.socket;

// 阻塞式 UDP：组合持有 socket 底座，对外只声明 UDP 该有的接口。
// 与 TCP 的关键区别（这就是它不共用 TCP 接口的理由）：
//   - 一次调用 = 一个数据报，绝不循环补齐
//   - 缓冲区小于数据报时，多余部分被内核静默丢弃（不报错、不排队）
//   - recv_from 返回 0 是"收到合法的空数据报"，不是对端关闭（UDP 没有"对端关闭"）
//   - 每个数据报都可能来自不同对端，地址由 recv_from 的出参带回
//
// 平台说明（2026-09-04）：
//   - POSIX：真实实现，send_to/recv_from 直接调用系统调用。
//   - Windows：当前为空实现（仅保证编译/链接通过），send_to/recv_from 返回 -1。
export class UDP {
public:
    UDP() : socket_(SocketType::datagram) {}
    explicit UDP(const Address& addr) : socket_(SocketType::datagram, addr) {}

    UDP(UDP&&) noexcept = default;
    UDP& operator=(UDP&&) noexcept = default;

    [[nodiscard]] int fd() const { return socket_.fd(); }

    int bind()    { return socket_.bind(); }

    // 连接态 UDP：只记下默认对端（不发任何包），此后内核只把该对端的数据报交给你。
    // connect 之后仍用 send_to / recv_from 收发即可。
    int connect() { return socket_.connect(); }

    // 查询本机绑定的地址。bind 到端口 0 时用它拿到内核分配的实际端口。
    [[nodiscard]] Address local_address() const { return socket_.local_address(); }

    // 发送一个数据报到指定对端。数据报要么整体发出，要么失败，没有"发了一半"。
    // 返回 >=0 为发出的字节数（一般等于 data.size()）；-1 表示出错（errno 有效）。
    std::ptrdiff_t send_to(std::span<const char> data, const Address& peer) {
#ifdef _WIN32
        (void)data;
        (void)peer;
        return -1; // Windows 空实现
#else
        while (true) {
            std::ptrdiff_t n = ::sendto(socket_.fd(), data.data(), data.size(), 0,
                                        peer.socket_address(), peer.size());
            if (n >= 0)
                return static_cast<std::ptrdiff_t>(n);
            if (errno == EINTR)   // 被信号打断，数据报未发出，重试
                continue;
            return -1;
        }
#endif
    }

    // 接收一个数据报并带回发送方地址。
    // 返回 >=0 为收到的字节数（0 = 空数据报，合法）；-1 表示出错（errno 有效）。
    // 注意：buf 应不小于单个数据报的最大长度，否则多余部分被静默截断。
    std::ptrdiff_t recv_from(std::span<char> buf, Address& peer) {
#ifdef _WIN32
        (void)buf;
        (void)peer;
        return -1; // Windows 空实现
#else
        while (true) {
            sockaddr_in sa{};
            socklen_t len = sizeof(sa);
            std::ptrdiff_t n = ::recvfrom(socket_.fd(), buf.data(), buf.size(), 0,
                                          reinterpret_cast<sockaddr*>(&sa), &len);
            if (n >= 0) {
                peer = Address(sa);
                return static_cast<std::ptrdiff_t>(n);
            }
            if (errno == EINTR)
                continue;
            return -1;
        }
#endif
    }

    // 读写超时，由内核在阻塞读写上生效
    bool set_timeout(std::chrono::milliseconds recv_timeout,
                     std::chrono::milliseconds send_timeout) {
        return socket_.set_timeout(recv_timeout, send_timeout);
    }

    void close() { socket_.close(); }

    ~UDP() { close(); }

private:
    Socket socket_;
};
