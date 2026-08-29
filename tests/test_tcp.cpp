import std;
import modforge.address;
import modforge.tcp;

// TCP 测试：阻塞字节流语义 + 4 字节长度前缀分帧。
// 只用环回地址 + 临时端口；所有阻塞读写都先设超时，避免测试挂死。
int test_tcp() {
    // 顺序固定：先 listen，再 connect，最后 accept —— 单线程下才不会互相等待
    TCP server(Address("127.0.0.1", 0));
    if (server.bind() != 0) return 1;
    if (server.listen() != 0) return 2;
    if (!server.is_listener()) return 3;

    const int port = server.local_address().port();
    if (port == 0) return 4;

    TCP client(Address("127.0.0.1", port));
    if (client.connect() != 0) return 5;

    TCP conn = server.accept();
    if (conn.fd() < 0) return 6;
    if (conn.is_listener()) return 7;

    if (!conn.set_timeout(std::chrono::seconds(3), std::chrono::seconds(3))) return 8;
    if (!client.set_timeout(std::chrono::seconds(3), std::chrono::seconds(3))) return 9;

    // 1) 原始字节流往返（HTTP 解析将来就用这一层）
    {
        const std::string req = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
        if (!client.write_all(std::span<const char>(req.data(), req.size()))) return 10;

        std::string got(req.size(), '\0');
        const auto n = conn.read_some(std::span<char>(got.data(), got.size()));
        if (n != static_cast<std::ptrdiff_t>(req.size())) return 11;
        if (got != req) return 12;
    }

    // 2) 分帧边界：连发三条（含一条空消息），必须按序、按边界收全
    {
        const std::vector<std::string> msgs = {"first", "second-message", ""};
        for (const auto& m : msgs) {
            if (!conn.send_message(std::span<const char>(m.data(), m.size()))) return 13;
        }
        for (const auto& m : msgs) {
            auto got = client.recv_message();
            if (!got) return 14;
            if (std::string(got->begin(), got->end()) != m) return 15;
        }
    }

    // 3) 大消息：远超单次 send 的上限，验证 send_all / recv_all 的循环补齐
    {
        const std::string big(256 * 1024, 'x');
        if (!conn.send_message(std::span<const char>(big.data(), big.size()))) return 16;

        auto got = client.recv_message();
        if (!got) return 17;
        if (got->size() != big.size()) return 18;
        if (std::string(got->begin(), got->end()) != big) return 19;
    }

    // 4) 对端关闭：read_some 应返回 0（而不是 -1）
    {
        client.close();
        char buf[16]{};
        const auto n = conn.read_some(std::span<char>(buf, sizeof(buf)));
        if (n != 0) return 20;
    }

    return 0;
}
