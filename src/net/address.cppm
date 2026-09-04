/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/29 20:05:49
********************************************************************************/
module;
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif
export module modforge.address;
import std;

// 端点抽象：IPv4 地址 + 端口，为 TCP / UDP 共用。
export class Address {
    sockaddr_in addr_{};
public:
    // 默认构造：0.0.0.0:0
    Address() {
        addr_.sin_family = AF_INET;
        addr_.sin_addr.s_addr = INADDR_ANY;
        addr_.sin_port = 0;
    }

    // 从 IP + 端口构造
    Address(const std::string& ip, const int port) {
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);

        if (::inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) <= 0) {
            throw std::runtime_error("invalid IPv4 address");
        }
    }

    // 从 sockaddr_in 构造（用于 UDP recvfrom / getsockname）
    explicit Address(const sockaddr_in& sa) : addr_(sa) {}

    // 返回端口（主机字节序）
    int port() const {
        return ntohs(addr_.sin_port);
    }

    // 返回 IP 字符串
    std::string ip() const {
        char buf[INET_ADDRSTRLEN];
        const char* p = ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
        return p ? std::string(p) : std::string{};
    }

    const sockaddr_in& as_sockaddr_in() const { return addr_; }
    sockaddr_in& as_sockaddr_in() { return addr_; }

    const sockaddr* socket_address() const {
        return reinterpret_cast<const sockaddr*>(&addr_);
    }
    sockaddr* socket_address() {
        return reinterpret_cast<sockaddr*>(&addr_);
    }

    socklen_t size() const { return sizeof(sockaddr_in); }
};
