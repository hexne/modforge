import std;
import modforge.address;
import modforge.socket;

// socket 底座测试：fd 生命周期与协议无关的通用调用。
// 只用环回地址 + 临时端口，不依赖外网；不测协议语义（那是 tcp / udp 两个测试的事）。
int test_socket() {
    // 1) 两种类型都能创建
    {
        Socket s(SocketType::stream);
        if (s.fd() < 0) return 1;
    }
    {
        Socket s(SocketType::datagram);
        if (s.fd() < 0) return 2;
    }

    // 2) close() 后 fd 置为 -1
    {
        Socket s(SocketType::stream);
        if (s.fd() < 0) return 3;
        s.close();
        if (s.fd() != -1) return 4;
        s.close();                 // 重复关闭应安全
        if (s.fd() != -1) return 5;
    }

    // 3) 移动构造：资源转移，被移动对象失效（fd 泄漏的高发区）
    {
        Socket a(SocketType::stream);
        const int fd = a.fd();
        Socket b(std::move(a));
        if (b.fd() != fd) return 6;
        if (a.fd() != -1) return 7;
    }

    // 4) 移动赋值
    {
        Socket a(SocketType::stream);
        Socket b(SocketType::datagram);
        const int fd = a.fd();
        b = std::move(a);
        if (b.fd() != fd) return 8;
        if (a.fd() != -1) return 9;
    }

    // 5) bind 到端口 0 后能查到内核分配的实际端口
    {
        Socket s(SocketType::stream, Address("127.0.0.1", 0));
        if (s.bind() != 0) return 10;
        if (s.local_address().port() == 0) return 11;
        if (s.local_address().ip() != "127.0.0.1") return 12;

        // 同一个 socket 重复 bind 应失败
        if (s.bind() == 0) return 13;
    }

    // 6) 超时设置生效
    {
        Socket s(SocketType::datagram);
        if (!s.set_timeout(std::chrono::seconds(1), std::chrono::seconds(1))) return 14;
    }

    // 7) 非法 fd 构造应抛异常
    {
        bool thrown = false;
        try {
            Socket s(-1);
        } catch (const std::invalid_argument&) {
            thrown = true;
        }
        if (!thrown) return 15;
    }

    // 8) 析构确实回收 fd：反复创建销毁不应耗尽描述符
    {
        for (int i = 0; i < 256; ++i) {
            Socket tmp(SocketType::stream);
            if (tmp.fd() < 0) return 16;
        }
    }

    return 0;
}
