import std;
import modforge.address;
import modforge.udp;

// UDP 测试：数据报语义。与 TCP 的根本差异都在这里被固定下来。
// 只用环回地址 + 临时端口；所有阻塞读写都先设超时，避免测试挂死。
int test_udp() {
    UDP rx(Address("127.0.0.1", 0));
    if (rx.bind() != 0) return 1;

    const int port = rx.local_address().port();
    if (port == 0) return 2;

    // 常规收发用宽松超时（环回上实际瞬间返回，设它是为了防止出 bug 时挂死 CI）
    if (!rx.set_timeout(std::chrono::seconds(5), std::chrono::seconds(5))) return 3;

    UDP tx;
    if (!tx.set_timeout(std::chrono::seconds(5), std::chrono::seconds(5))) return 4;

    char buf[256]{};
    Address peer;

    // 1) 数据报往返，并带回发送方地址
    {
        const std::string msg = "hello";
        if (tx.send_to(std::span<const char>(msg.data(), msg.size()),
                       Address("127.0.0.1", port))
            != static_cast<std::ptrdiff_t>(msg.size()))
            return 6;

        const auto n = rx.recv_from(std::span<char>(buf, sizeof(buf)), peer);
        if (n != static_cast<std::ptrdiff_t>(msg.size())) return 7;
        if (std::string(buf, n) != msg) return 8;
        if (peer.ip() != "127.0.0.1") return 9;
        if (peer.port() == 0) return 10;
    }

    // 2) 数据报边界：三报必须分三次收到，绝不合并（TCP 会粘包，UDP 不能）
    {
        for (int i = 0; i < 3; ++i) {
            const std::string d = "d" + std::to_string(i);
            if (tx.send_to(std::span<const char>(d.data(), d.size()),
                           Address("127.0.0.1", port))
                != static_cast<std::ptrdiff_t>(d.size()))
                return 11;
        }
        for (int i = 0; i < 3; ++i) {
            const auto n = rx.recv_from(std::span<char>(buf, sizeof(buf)), peer);
            if (n != 2) return 12;
            if (std::string(buf, n) != "d" + std::to_string(i)) return 13;
        }
    }

    // 3) 空数据报：返回 0 是合法的，不是"对端关闭"（UDP 没有对端关闭这回事）
    {
        if (tx.send_to(std::span<const char>(), Address("127.0.0.1", port)) != 0) return 14;
        if (rx.recv_from(std::span<char>(buf, sizeof(buf)), peer) != 0) return 15;
    }

    // 4) 缓冲区小于数据报：多余部分被内核静默截断（固定此行为，防止将来误改成循环补齐）
    {
        const std::string big(100, 'y');
        if (tx.send_to(std::span<const char>(big.data(), big.size()),
                       Address("127.0.0.1", port))
            != static_cast<std::ptrdiff_t>(big.size()))
            return 16;

        char small[10]{};
        const auto n = rx.recv_from(std::span<char>(small, sizeof(small)), peer);
        if (n != 10) return 17;
    }

    // 5) 超时：空闲时应返回 -1，而不是永久阻塞。
    //    这里单独用较短的超时，避免拖慢整个套件；顺便断言等待时长，
    //    确认是"等了一会儿才超时"，既不是立刻失败，也不是卡住。
    {
        if (!rx.set_timeout(std::chrono::milliseconds(200), std::chrono::milliseconds(200)))
            return 18;

        const auto t0 = std::chrono::steady_clock::now();
        const auto n = rx.recv_from(std::span<char>(buf, sizeof(buf)), peer);
        const auto waited = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - t0).count();

        if (n != -1) return 19;
        if (waited < 100 || waited > 2000) return 20;
    }

    return 0;
}
