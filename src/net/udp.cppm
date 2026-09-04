/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/29 21:12:03
********************************************************************************/

module;
#include <cerrno>
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
#include <sys/types.h>
#endif
export module modforge.udp;
import std;
import modforge.address;
import modforge.socket;

/** @brief 阻塞式 UDP：组合持有 socket 底座
 *  @note  一次调用 = 一个数据报（不循环补齐）；缓冲区小于数据报时多余部分被内核丢弃；
 *         recv_from 返回 0 是合法空数据报（UDP 无"对端关闭"）；每个数据报可来自不同对端，
 *         地址由 recv_from 出参带回
 */
export class UDP {
public:
    UDP() : socket_(SocketType::datagram) {}

    /** @brief 指定端点构造
     *  @param addr 关联的本地或对端地址
     */
    explicit UDP(const Address& addr) : socket_(SocketType::datagram, addr) {}

    UDP(UDP&&) noexcept = default;
    UDP& operator=(UDP&&) noexcept = default;

    [[nodiscard]] auto fd() const { return socket_.fd(); }

    int bind()    { return socket_.bind(); }

    /** @brief 连接态 UDP：记下默认对端（不发包），此后仅收该对端数据报 */
    int connect() { return socket_.connect(); }

    /** @brief 本机绑定地址（bind 端口 0 后取内核分配的实际端口） */
    [[nodiscard]] Address local_address() const { return socket_.local_address(); }

    /** @brief 发送一个数据报到指定对端（整体发出或失败，无"发一半"）
     *  @param data 待发送数据，作为一个数据报
     *  @param peer 目标对端地址
     *  @return >=0 发出的字节数（通常等于 data.size()）；-1 出错
     */
    std::ptrdiff_t send_to(std::span<const char> data, const Address& peer) {
#ifdef _WIN32
        const int n = ::sendto(fd(), data.data(), static_cast<int>(data.size()), 0,
                               peer.socket_address(), static_cast<int>(peer.size()));
        return n == SOCKET_ERROR ? -1 : static_cast<std::ptrdiff_t>(n);
#else
        while (true) {
            std::ptrdiff_t n = ::sendto(fd(), data.data(), data.size(), 0,
                                        peer.socket_address(), peer.size());
            if (n >= 0)
                return static_cast<std::ptrdiff_t>(n);
            if (errno == EINTR)   // 被信号打断，数据报未发出，重试
                continue;
            return -1;
        }
#endif
    }

    /** @brief 接收一个数据报并带回发送方地址
     *  @param buf 接收缓冲区，应不小于最大数据报长度
     *  @param peer 出参，收到数据报的发送方地址
     *  @return >=0 收到的字节数（0 = 空数据报，合法）；-1 出错
     *  @note  buf 不足时多余部分被内核静默截断
     */
    std::ptrdiff_t recv_from(std::span<char> buf, Address& peer) {
#ifdef _WIN32
        sockaddr_in sa{};
        int len = sizeof(sa);
        const int n = ::recvfrom(fd(), buf.data(), static_cast<int>(buf.size()), 0,
                                 reinterpret_cast<sockaddr*>(&sa), &len);
        if (n == SOCKET_ERROR)
            return -1;
        peer = Address(sa);
        return static_cast<std::ptrdiff_t>(n);
#else
        while (true) {
            sockaddr_in sa{};
            socklen_t len = sizeof(sa);
            std::ptrdiff_t n = ::recvfrom(fd(), buf.data(), buf.size(), 0,
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

    /** @brief 设置阻塞读写的内核超时
     *  @param recv_timeout 接收超时（毫秒）
     *  @param send_timeout 发送超时（毫秒）
     *  @return 两个方向均设置成功时为 true
     */
    bool set_timeout(std::chrono::milliseconds recv_timeout,
                     std::chrono::milliseconds send_timeout) {
        return socket_.set_timeout(recv_timeout, send_timeout);
    }

    void close() { socket_.close(); }

    ~UDP() { close(); }

private:
    Socket socket_;
};
