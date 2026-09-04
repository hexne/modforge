/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/29 19:40:03
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
#include <sys/socket.h>
#endif
export module modforge.tcp;
import std;
import modforge.address;
import modforge.socket;

/** @brief 阻塞式 TCP 连接：组合持有 socket 底座
 *  @note  listen/accept 在此（UDP 无）；TCP 是字节流，send_all/recv_all 循环补齐
 *         两层接口：原始字节流 read_some/write_all，与 4 字节长度前缀分帧
 *         send_message/recv_message；超时由内核生效（set_timeout），不做事件循环
 */
export class TCP {
public:
    /** @brief 分帧单条消息上限，防损坏长度前缀导致巨量分配 */
    inline static constexpr int k_max_message_size = 64 * 1024 * 1024;

    TCP() : socket_(SocketType::stream) {}

    /** @brief 指定端点构造
     *  @param addr 关联的本地或对端地址
     */
    explicit TCP(const Address& addr) : socket_(SocketType::stream, addr) {}

    TCP(TCP&&) noexcept = default;
    TCP& operator=(TCP&&) noexcept = default;

    [[nodiscard]] auto fd() const { return socket_.fd(); }

    [[nodiscard]] bool is_listener() const { return is_listener_; }

    int connect() { return socket_.connect(); }
    int bind()    { return socket_.bind(); }

    /** @brief 开始监听（服务端）
     *  @param backlog 已完成连接队列长度
     *  @return 0 成功；-1 出错
     */
    int listen(int backlog = 128) {
        is_listener_ = true;
        return ::listen(fd(), backlog);  // POSIX/winsock 同名同参
    }

    /** @brief 阻塞接受一个连接；失败抛异常
     *  @return 代表新连接的 TCP 对象
     */
    TCP accept() const {
#ifdef _WIN32
        const SOCKET s = ::accept(fd(), nullptr, nullptr);
        if (s == INVALID_SOCKET) {
            throw std::runtime_error(std::string("accept failed: error ")
                + std::to_string(::WSAGetLastError()));
        }
        return TCP(static_cast<std::intptr_t>(s));
#else
        const int s = ::accept(fd(), nullptr, nullptr);
        if (s < 0) {
            throw std::runtime_error(std::string("accept failed: ") + std::strerror(errno));
        }
        return TCP(static_cast<std::intptr_t>(s));
#endif
    }

    /** @brief 本机绑定地址（bind 端口 0 后取内核分配的实际端口） */
    [[nodiscard]] Address local_address() const { return socket_.local_address(); }

    /** @brief 读一次原始字节
     *  @param buf 接收缓冲区
     *  @return >0 字节数；0 对端关闭；-1 出错
     *  @note  返回 ptrdiff_t，避免把 POSIX ssize_t 漏进导入方
     */
    std::ptrdiff_t read_some(std::span<char> buf) {
        return static_cast<std::ptrdiff_t>(socket_.recv(buf));
    }

    /** @brief 阻塞写完整个缓冲区
     *  @param data 待发送数据
     *  @return false 表示连接已断或出错
     */
    bool write_all(std::span<const char> data) { return send_all(data); }

    /** @brief 按 4 字节长度前缀发送一整条消息
     *  @param msg 消息内容
     *  @return 超上限或发送失败时为 false
     */
    bool send_message(std::span<const char> msg) {
        using length_type = int;
        if (msg.size() > static_cast<size_t>(k_max_message_size))
            return false;

        length_type len = static_cast<length_type>(msg.size());
        if (!send_all(std::span<const char>(reinterpret_cast<const char*>(&len), sizeof(len))))
            return false;

        return send_all(msg);
    }

    /** @brief 阻塞收齐一整条分帧消息
     *  @return 消息内容；连接关闭、出错或长度非法时返回 nullopt
     */
    std::optional<std::vector<char>> recv_message() {
        using length_type = int;

        length_type len{};
        if (!recv_all(std::span<char>(reinterpret_cast<char*>(&len), sizeof(len))))
            return std::nullopt;

        if (len < 0 || len > k_max_message_size)
            return std::nullopt;

        std::vector<char> msg(static_cast<size_t>(len));
        if (len > 0 && !recv_all(std::span<char>(msg.data(), msg.size())))
            return std::nullopt;

        return msg;
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

    /** @brief 关闭并复位监听状态 */
    void close() {
        socket_.close();
        is_listener_ = false;
    }

    ~TCP() { close(); }

private:
    explicit TCP(const std::intptr_t fd) : socket_(fd) {}  // accept 新连接用

    bool is_listener_ = false;

    /** @brief 阻塞写完整缓冲区
     *  @param data 待发送数据
     *  @return 全部写完为 true；连接断开或出错为 false
     */
    bool send_all(std::span<const char> data) {
        size_t sent = 0;
        while (sent < data.size()) {
            std::ptrdiff_t n = socket_.send(data.subspan(sent));
            if (n > 0) {
                sent += static_cast<size_t>(n);
                continue;
            }
#ifdef _WIN32
            return false;                  // winsock 无 EINTR 语义，失败即退出
#else
            if (n < 0 && errno == EINTR)   // 被信号打断，重试
                continue;
            return false;                  // n == 0 或真实错误
#endif
        }
        return true;
    }

    /** @brief 阻塞读满缓冲区
     *  @param data 接收缓冲区
     *  @return 读满为 true；对端关闭或出错为 false
     */
    bool recv_all(std::span<char> data) {
        size_t got = 0;
        while (got < data.size()) {
            std::ptrdiff_t n = socket_.recv(data.subspan(got));
            if (n > 0) {
                got += static_cast<size_t>(n);
                continue;
            }
            if (n == 0)  // 对端关闭
                return false;
#ifdef _WIN32
            return false;  // winsock 无 EINTR，失败即退出
#else
            if (n < 0 && errno == EINTR)  // 被信号打断则重试
                continue;
            return false;
#endif
        }
        return true;
    }

    Socket socket_;
};
