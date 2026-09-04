/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/29 20:05:49
********************************************************************************/
module;
#ifdef _WIN32
// mingw-gcc modules bug workaround：见 src/terminal.cppm 注释（cstddef 预热 c++config.h guard）
#include <cstddef>
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

/** @brief 端点抽象：IPv4 地址 + 端口，TCP/UDP 共用 */
export class Address {
    sockaddr_in addr_{};
public:
    /** @brief 默认构造：0.0.0.0:0 */
    Address() {
        addr_.sin_family = AF_INET;
        addr_.sin_addr.s_addr = INADDR_ANY;
        addr_.sin_port = 0;
    }

    /** @brief 从 IP 字符串 + 端口构造，非法 IPv4 抛异常
     *  @param ip 点分十进制 IPv4 字符串
     *  @param port 端口号（主机字节序，内部转为网络序）
     */
    Address(const std::string& ip, const int port) {
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);

        if (::inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) <= 0) {
            throw std::runtime_error("invalid IPv4 address");
        }
    }

    /** @brief 从原生 sockaddr_in 构造（UDP recvfrom / getsockname 用）
     *  @param sa 原生 IPv4 地址结构
     */
    explicit Address(const sockaddr_in& sa) : addr_(sa) {}

    /** @brief 端口（主机字节序） */
    int port() const {
        return ntohs(addr_.sin_port);
    }

    /** @brief IP 字符串 */
    std::string ip() const {
        char buf[INET_ADDRSTRLEN];
        const char* p = ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
        return p ? std::string(p) : std::string{};
    }

    /** @brief 底层原生地址（读写），供系统调用使用 */
    const sockaddr_in& as_sockaddr_in() const { return addr_; }
    sockaddr_in& as_sockaddr_in() { return addr_; }

    const sockaddr* socket_address() const {
        return reinterpret_cast<const sockaddr*>(&addr_);
    }
    sockaddr* socket_address() {
        return reinterpret_cast<sockaddr*>(&addr_);
    }

    /** @brief sockaddr 结构长度 */
    socklen_t size() const { return sizeof(sockaddr_in); }
};
