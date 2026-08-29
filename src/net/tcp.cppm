/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/29 19:40:03
********************************************************************************/

module;
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
export module tcp;
import std;
import address;
import socket;

// 阻塞式 TCP 连接：组合持有 socket 底座，对外只声明 TCP 该有的接口。
//   - listen / accept（UDP 没有）
//   - send_all / recv_all 循环补齐（TCP 是字节流，一次调用不保证收发完整）
// 提供两层接口：
//   1) 原始字节流  read_some / write_all   —— HTTP、WebSocket 等协议解析用这层
//   2) 4 字节长度前缀的分帧 send_message / recv_message —— 自定义消息协议用这层
// 配合线程池每连接一线程使用；不做事件循环，读写超时交给内核（set_timeout）。
export class TCP {
public:
    // 单条消息上限，防止损坏/恶意的长度前缀导致巨量分配
    inline static constexpr int k_max_message_size = 64 * 1024 * 1024;

    TCP() : socket_(SocketType::stream) {}
    explicit TCP(const Address& addr) : socket_(SocketType::stream, addr) {}

    TCP(TCP&&) noexcept = default;
    TCP& operator=(TCP&&) noexcept = default;

    [[nodiscard]] int fd() const { return socket_.fd(); }

    [[nodiscard]] bool is_listener() const { return is_listener_; }

    int connect() { return socket_.connect(); }
    int bind()    { return socket_.bind(); }

    int listen(int backlog = 128) {
        is_listener_ = true;
        return ::listen(socket_.fd(), backlog);
    }

    // 阻塞 accept；失败抛异常
    TCP accept() const {
        int fd = ::accept(socket_.fd(), nullptr, nullptr);
        if (fd < 0) {
            throw std::runtime_error(std::string("accept failed: ") + std::strerror(errno));
        }
        return TCP(fd);
    }

    // 查询本机绑定的地址。bind 到端口 0 时用它拿到内核分配的实际端口。
    [[nodiscard]] Address local_address() const { return socket_.local_address(); }

    // -------------------------
    // 原始字节流
    // -------------------------

    // 读一次：>0 为读到的字节数；0 表示对端关闭；-1 表示出错（errno 有效）
    // 用 std::ptrdiff_t 而非 ssize_t，避免把 POSIX 名字漏进导入方的全局命名空间
    std::ptrdiff_t read_some(std::span<char> buf) {
        return static_cast<std::ptrdiff_t>(socket_.recv(buf));
    }

    // 阻塞直到全部写完；false 表示连接已断或出错
    bool write_all(std::span<const char> data) { return send_all(data); }

    // -------------------------
    // 4 字节长度前缀分帧
    // -------------------------

    bool send_message(std::span<const char> msg) {
        using length_type = int;
        if (msg.size() > static_cast<size_t>(k_max_message_size))
            return false;

        length_type len = static_cast<length_type>(msg.size());
        if (!send_all(std::span<const char>(reinterpret_cast<const char*>(&len), sizeof(len))))
            return false;

        return send_all(msg);
    }

    // 阻塞直到收齐一整条消息；nullopt 表示连接关闭、出错或长度非法
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

    // 读写超时，由内核在阻塞读写上生效
    bool set_timeout(std::chrono::milliseconds recv_timeout,
                     std::chrono::milliseconds send_timeout) {
        return socket_.set_timeout(recv_timeout, send_timeout);
    }

    // 关闭并复位监听状态
    void close() {
        socket_.close();
        is_listener_ = false;
    }

    ~TCP() { close(); }

private:
    explicit TCP(const int fd) : socket_(fd) {}

    bool is_listener_ = false;

    // 阻塞写：直到全部写完或出错
    bool send_all(std::span<const char> data) {
        size_t sent = 0;
        while (sent < data.size()) {
            ssize_t n = socket_.send(data.subspan(sent));
            if (n > 0) {
                sent += static_cast<size_t>(n);
                continue;
            }
            if (n < 0 && errno == EINTR)   // 被信号打断，重试
                continue;
            return false;                  // n == 0 或真实错误
        }
        return true;
    }

    // 阻塞读：直到读满或出错/对端关闭
    bool recv_all(std::span<char> data) {
        size_t got = 0;
        while (got < data.size()) {
            ssize_t n = socket_.recv(data.subspan(got));
            if (n > 0) {
                got += static_cast<size_t>(n);
                continue;
            }
            if (n == 0)                    // 对端关闭
                return false;
            if (n < 0 && errno == EINTR)
                continue;
            return false;
        }
        return true;
    }

    Socket socket_;
};
