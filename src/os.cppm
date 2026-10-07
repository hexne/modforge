/********************************************************************************
* @Author : hexne
* @Date   : 2026/09/19 18:18:19
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
#include <windows.h>
#include <imagehlp.h>
#include <dbghelp.h>
#elifdef __linux__
// _GNU_SOURCE 必须在第一个系统头之前定义：pipe2 / pthread_setaffinity_np 是 GNU 扩展，
// 定义晚了会被头文件里的 feature 判定关掉。g++ 默认自带，别的编译器不保证。
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <poll.h>
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#endif
export module modforge.os;

#ifdef _WIN32
export {
    // command
    using ::PIMAGE_IMPORT_DESCRIPTOR;
    using ::PVOID;
    using ::BOOL;
    using ::LONG;
    using ::ULONG;
    using ::DWORD;
    using ::HANDLE;
    using ::JOBOBJECT_EXTENDED_LIMIT_INFORMATION;
    using ::PROCESS_INFORMATION;
    using ::SECURITY_ATTRIBUTES;
    using ::STARTUPINFOW;
    using ::UINT;
    using ::WORD;
    using ::IMAGE_IMPORT_DESCRIPTOR;
    using ::IMAGE_THUNK_DATA;
    using ::IMAGE_IMPORT_BY_NAME;
    using ::IMAGE_EXPORT_DIRECTORY;
    using ::DWORD;
    using ::LOADED_IMAGE;

    using ::MapAndLoad;
    using ::ImageDirectoryEntryToData;
    using ::UnMapAndLoad;
    using ::ImageRvaToVa;

    using ::AssignProcessToJobObject;
    using ::CancelSynchronousIo;
    using ::CloseHandle;
    using ::CreateJobObjectW;
    using ::CreatePipe;
    using ::CreateProcessW;
    using ::DuplicateHandle;
    using ::GetCurrentProcess;
    using ::GetCurrentThread;
    using ::GetExitCodeProcess;
    using ::GetLastError;
    using ::GetStdHandle;
    using ::JobObjectExtendedLimitInformation;
    using ::MultiByteToWideChar;
    using ::ReadFile;
    using ::ResumeThread;
    using ::SetHandleInformation;
    using ::SetInformationJobObject;
    using ::TerminateJobObject;
    using ::TerminateProcess;
    using ::WaitForSingleObject;
    using ::UnDecorateSymbolName;
    using ::WideCharToMultiByte;
    using ::DWORD64;
    using ::ULONG64;
    using ::CHAR;
    using ::PIMAGE_NT_HEADERS;
    using ::PLOADED_IMAGE;
    using ::ImageNtHeader;
    using ::PIMAGE_EXPORT_DIRECTORY;


    using ::IMAGEHLP_SYMBOL_TYPE_INFO;
    using ::SymGetTypeInfo;
    using ::TI_GET_SYMNAME;
    using ::LocalFree;
    using ::TI_GET_TYPEID;
    using ::TI_GET_SYMTAG;

    // modforge cursor
    using ::GetAsyncKeyState;
    using ::GetCursorPos;
    using ::mouse_event;
    using ::POINT;
    using ::SetCursorPos;

    // modforge thread_pool
    using ::DWORD_PTR;
    using ::SetThreadAffinityMask;

    // modforge net
    using ::accept;
    using ::bind;
    using ::closesocket;
    using ::connect;
    using ::getsockname;
    using ::htons;
    using ::inet_ntop;
    using ::inet_pton;
    using ::listen;
    using ::ntohs;
    using ::recv;
    using ::recvfrom;
    using ::send;
    using ::sendto;
    using ::setsockopt;
    using ::sockaddr;
    using ::sockaddr_in;
    using ::SOCKET;
    using ::socklen_t;
    using ::socket;
    using ::WSADATA;
    using ::WSAGetLastError;
    using ::WSAStartup;
}

constexpr BOOL TRUE_IMPL = TRUE;
#undef TRUE
export constexpr BOOL TRUE = TRUE_IMPL;

constexpr BOOL FALSE_IMPL = FALSE;
#undef FALSE
export constexpr BOOL FALSE = FALSE_IMPL;

constexpr DWORD STARTF_USESTDHANDLES_IMPL = STARTF_USESTDHANDLES;
#undef STARTF_USESTDHANDLES
export constexpr DWORD STARTF_USESTDHANDLES = STARTF_USESTDHANDLES_IMPL;

constexpr DWORD HANDLE_FLAG_INHERIT_IMPL = HANDLE_FLAG_INHERIT;
#undef HANDLE_FLAG_INHERIT
export constexpr DWORD HANDLE_FLAG_INHERIT = HANDLE_FLAG_INHERIT_IMPL;

constexpr DWORD CREATE_NO_WINDOW_IMPL = CREATE_NO_WINDOW;
#undef CREATE_NO_WINDOW
export constexpr DWORD CREATE_NO_WINDOW = CREATE_NO_WINDOW_IMPL;

constexpr DWORD STD_INPUT_HANDLE_IMPL = STD_INPUT_HANDLE;
#undef STD_INPUT_HANDLE
export constexpr DWORD STD_INPUT_HANDLE = STD_INPUT_HANDLE_IMPL;

constexpr UINT CP_UTF8_IMPL = CP_UTF8;
#undef CP_UTF8
export constexpr UINT CP_UTF8 = CP_UTF8_IMPL;

inline const HANDLE INVALID_HANDLE_VALUE_IMPL = INVALID_HANDLE_VALUE;
#undef INVALID_HANDLE_VALUE
export inline const HANDLE INVALID_HANDLE_VALUE = INVALID_HANDLE_VALUE_IMPL;

constexpr DWORD CREATE_SUSPENDED_IMPL = CREATE_SUSPENDED;
#undef CREATE_SUSPENDED
export constexpr DWORD CREATE_SUSPENDED = CREATE_SUSPENDED_IMPL;

constexpr DWORD JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE_IMPL = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
#undef JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
export constexpr DWORD JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE_IMPL;

constexpr DWORD THREAD_TERMINATE_IMPL = THREAD_TERMINATE;
#undef THREAD_TERMINATE
export constexpr DWORD THREAD_TERMINATE = THREAD_TERMINATE_IMPL;

constexpr DWORD INFINITE_IMPL = INFINITE;
#undef INFINITE
export constexpr DWORD INFINITE = INFINITE_IMPL;

constexpr DWORD MOUSEEVENTF_LEFTDOWN_IMPL = MOUSEEVENTF_LEFTDOWN;
#undef MOUSEEVENTF_LEFTDOWN
export constexpr DWORD MOUSEEVENTF_LEFTDOWN = MOUSEEVENTF_LEFTDOWN_IMPL;

constexpr DWORD MOUSEEVENTF_LEFTUP_IMPL = MOUSEEVENTF_LEFTUP;
#undef MOUSEEVENTF_LEFTUP
export constexpr DWORD MOUSEEVENTF_LEFTUP = MOUSEEVENTF_LEFTUP_IMPL;

constexpr DWORD MOUSEEVENTF_RIGHTDOWN_IMPL = MOUSEEVENTF_RIGHTDOWN;
#undef MOUSEEVENTF_RIGHTDOWN
export constexpr DWORD MOUSEEVENTF_RIGHTDOWN = MOUSEEVENTF_RIGHTDOWN_IMPL;

constexpr DWORD MOUSEEVENTF_RIGHTUP_IMPL = MOUSEEVENTF_RIGHTUP;
#undef MOUSEEVENTF_RIGHTUP
export constexpr DWORD MOUSEEVENTF_RIGHTUP = MOUSEEVENTF_RIGHTUP_IMPL;

constexpr DWORD MOUSEEVENTF_WHEEL_IMPL = MOUSEEVENTF_WHEEL;
#undef MOUSEEVENTF_WHEEL
export constexpr DWORD MOUSEEVENTF_WHEEL = MOUSEEVENTF_WHEEL_IMPL;

constexpr int VK_LBUTTON_IMPL = VK_LBUTTON;
#undef VK_LBUTTON
export constexpr int VK_LBUTTON = VK_LBUTTON_IMPL;

constexpr int VK_RBUTTON_IMPL = VK_RBUTTON;
#undef VK_RBUTTON
export constexpr int VK_RBUTTON = VK_RBUTTON_IMPL;

constexpr auto INADDR_ANY_IMPL = INADDR_ANY;
#undef INADDR_ANY
export constexpr auto INADDR_ANY = INADDR_ANY_IMPL;

constexpr int SOCK_STREAM_IMPL = SOCK_STREAM;
#undef SOCK_STREAM
export constexpr int SOCK_STREAM = SOCK_STREAM_IMPL;

constexpr int SOCK_DGRAM_IMPL = SOCK_DGRAM;
#undef SOCK_DGRAM
export constexpr int SOCK_DGRAM = SOCK_DGRAM_IMPL;

constexpr SOCKET INVALID_SOCKET_IMPL = INVALID_SOCKET;
#undef INVALID_SOCKET
export constexpr SOCKET INVALID_SOCKET = INVALID_SOCKET_IMPL;

constexpr int SOCKET_ERROR_IMPL = SOCKET_ERROR;
#undef SOCKET_ERROR
export constexpr int SOCKET_ERROR = SOCKET_ERROR_IMPL;

constexpr int AF_INET_IMPL = AF_INET;
#undef AF_INET
export constexpr int AF_INET = AF_INET_IMPL;

constexpr int SOL_SOCKET_IMPL = SOL_SOCKET;
#undef SOL_SOCKET
export constexpr int SOL_SOCKET = SOL_SOCKET_IMPL;

constexpr int SO_RCVTIMEO_IMPL = SO_RCVTIMEO;
#undef SO_RCVTIMEO
export constexpr int SO_RCVTIMEO = SO_RCVTIMEO_IMPL;

constexpr int SO_SNDTIMEO_IMPL = SO_SNDTIMEO;
#undef SO_SNDTIMEO
export constexpr int SO_SNDTIMEO = SO_SNDTIMEO_IMPL;

constexpr int INET_ADDRSTRLEN_IMPL = INET_ADDRSTRLEN;
#undef INET_ADDRSTRLEN
export constexpr int INET_ADDRSTRLEN = INET_ADDRSTRLEN_IMPL;

constexpr int IMAGE_DIRECTORY_ENTRY_IMPORT_IMPL = IMAGE_DIRECTORY_ENTRY_IMPORT;
#undef IMAGE_DIRECTORY_ENTRY_IMPORT
export constexpr int IMAGE_DIRECTORY_ENTRY_IMPORT = IMAGE_DIRECTORY_ENTRY_IMPORT_IMPL;

constexpr int IMAGE_DIRECTORY_ENTRY_EXPORT_IMPL = IMAGE_DIRECTORY_ENTRY_EXPORT;
#undef IMAGE_DIRECTORY_ENTRY_EXPORT
export constexpr int IMAGE_DIRECTORY_ENTRY_EXPORT = IMAGE_DIRECTORY_ENTRY_EXPORT_IMPL;

constexpr int UNDNAME_COMPLETE_IMPL = UNDNAME_COMPLETE;
#undef UNDNAME_COMPLETE
export constexpr int UNDNAME_COMPLETE = UNDNAME_COMPLETE_IMPL;

#undef MAKEWORD
export constexpr unsigned short MAKEWORD(unsigned char low, unsigned char high) noexcept {
    return static_cast<unsigned short>(low) | (static_cast<unsigned short>(high) << 8);
}

#undef IMAGE_SNAP_BY_ORDINAL
export constexpr unsigned short IMAGE_SNAP_BY_ORDINAL(unsigned char Ordinal) noexcept {
    return IMAGE_SNAP_BY_ORDINAL64(Ordinal);
}

#undef IMAGE_ORDINAL
export constexpr unsigned short IMAGE_ORDINAL(unsigned char Ordinal) noexcept {
    return IMAGE_ORDINAL64(Ordinal);
}
#endif

#ifdef __linux__
export {
    // modforge terminal / cursor
    using ::winsize;
    using ::ioctl;
    using ::read;
    using ::close;

    // modforge command
    using ::pid_t;
    using ::kill;
    using ::write;
    using ::pipe2;
    using ::waitpid;
    using ::ssize_t;
    using ::environ;
    using ::pollfd;
    using ::poll;

    using ::posix_spawnp;
    using ::posix_spawn_file_actions_t;
    using ::posix_spawn_file_actions_init;
    using ::posix_spawn_file_actions_destroy;
    using ::posix_spawn_file_actions_adddup2;
    using ::posix_spawn_file_actions_addclose;
    using ::posix_spawnattr_t;
    using ::posix_spawnattr_init;
    using ::posix_spawnattr_destroy;
    using ::posix_spawnattr_setflags;
    using ::posix_spawnattr_setpgroup;

    // modforge net
    // SOCK_STREAM / SOCK_DGRAM 在 glibc 里是 enum __socket_type 的枚举量（外加一个自指宏），
    // 名字已经是实体，直接 using 转发；写成 constexpr 会与枚举量重名冲突。
    using ::SOCK_STREAM;
    using ::SOCK_DGRAM;

    using ::sockaddr;
    using ::sockaddr_in;
    using ::socklen_t;
    using ::timeval;
    using ::socket;
    using ::bind;
    using ::listen;
    using ::accept;
    using ::connect;
    using ::send;
    using ::recv;
    using ::sendto;
    using ::recvfrom;
    using ::setsockopt;
    using ::getsockname;
    using ::htons;
    using ::ntohs;
    using ::inet_pton;
    using ::inet_ntop;

    // modforge thread_pool
    using ::cpu_set_t;
    using ::pthread_setaffinity_np;
}


constexpr int STDOUT_FILENO_IMPL = STDOUT_FILENO;
#undef STDOUT_FILENO
export constexpr int STDOUT_FILENO = STDOUT_FILENO_IMPL;

constexpr int TIOCGWINSZ_IMPL = TIOCGWINSZ;
#undef TIOCGWINSZ
export constexpr int TIOCGWINSZ = TIOCGWINSZ_IMPL;

constexpr int STDIN_FILENO_IMPL = STDIN_FILENO;
#undef STDIN_FILENO
export constexpr int STDIN_FILENO = STDIN_FILENO_IMPL;

constexpr int STDERR_FILENO_IMPL = STDERR_FILENO;
#undef STDERR_FILENO
export constexpr int STDERR_FILENO = STDERR_FILENO_IMPL;

// ---- modforge command ----
constexpr int O_CLOEXEC_IMPL = O_CLOEXEC;
#undef O_CLOEXEC
export constexpr int O_CLOEXEC = O_CLOEXEC_IMPL;

constexpr int O_NONBLOCK_IMPL = O_NONBLOCK;
#undef O_NONBLOCK
export constexpr int O_NONBLOCK = O_NONBLOCK_IMPL;

constexpr int SIGKILL_IMPL = SIGKILL;
#undef SIGKILL
export constexpr int SIGKILL = SIGKILL_IMPL;

constexpr int EINTR_IMPL = EINTR;
#undef EINTR
export constexpr int EINTR = EINTR_IMPL;

constexpr int POSIX_SPAWN_SETPGROUP_IMPL = POSIX_SPAWN_SETPGROUP;
#undef POSIX_SPAWN_SETPGROUP
export constexpr int POSIX_SPAWN_SETPGROUP = POSIX_SPAWN_SETPGROUP_IMPL;

constexpr int POLLIN_IMPL = POLLIN;
#undef POLLIN
export constexpr int POLLIN = POLLIN_IMPL;

constexpr int POLLHUP_IMPL = POLLHUP;
#undef POLLHUP
export constexpr int POLLHUP = POLLHUP_IMPL;

constexpr int POLLERR_IMPL = POLLERR;
#undef POLLERR
export constexpr int POLLERR = POLLERR_IMPL;

// WIF*/WEXITSTATUS/WTERMSIG 是函数式宏，只能包装成函数导出。
// 走平台自己的展开结果，不手抄位运算——状态字编码是平台实现细节。
inline bool WIFEXITED_IMPL(int status) noexcept { return WIFEXITED(status); }
#undef WIFEXITED
export inline bool WIFEXITED(int status) noexcept { return WIFEXITED_IMPL(status); }

inline int WEXITSTATUS_IMPL(int status) noexcept { return WEXITSTATUS(status); }
#undef WEXITSTATUS
export inline int WEXITSTATUS(int status) noexcept { return WEXITSTATUS_IMPL(status); }

inline bool WIFSIGNALED_IMPL(int status) noexcept { return WIFSIGNALED(status); }
#undef WIFSIGNALED
export inline bool WIFSIGNALED(int status) noexcept { return WIFSIGNALED_IMPL(status); }

inline int WTERMSIG_IMPL(int status) noexcept { return WTERMSIG(status); }
#undef WTERMSIG
export inline int WTERMSIG(int status) noexcept { return WTERMSIG_IMPL(status); }

// errno 也是宏（展开成 (*__errno_location())），而且指向 thread_local 存储：
// 导出成变量或引用都不行——引用的绑定发生在模块初始化线程上，别的线程会读到错的地址。
// 只导出按次取值的函数。
inline int errno_impl() noexcept { return errno; }
#undef errno
export inline int errno_value() noexcept { return errno_impl(); }

// ---- modforge net ----
constexpr int AF_INET_IMPL = AF_INET;
#undef AF_INET
export constexpr int AF_INET = AF_INET_IMPL;

constexpr int SOL_SOCKET_IMPL = SOL_SOCKET;
#undef SOL_SOCKET
export constexpr int SOL_SOCKET = SOL_SOCKET_IMPL;

constexpr int SO_RCVTIMEO_IMPL = SO_RCVTIMEO;
#undef SO_RCVTIMEO
export constexpr int SO_RCVTIMEO = SO_RCVTIMEO_IMPL;

constexpr int SO_SNDTIMEO_IMPL = SO_SNDTIMEO;
#undef SO_SNDTIMEO
export constexpr int SO_SNDTIMEO = SO_SNDTIMEO_IMPL;

constexpr int INET_ADDRSTRLEN_IMPL = INET_ADDRSTRLEN;
#undef INET_ADDRSTRLEN
export constexpr int INET_ADDRSTRLEN = INET_ADDRSTRLEN_IMPL;

constexpr auto INADDR_ANY_IMPL = INADDR_ANY;
#undef INADDR_ANY
export constexpr auto INADDR_ANY = INADDR_ANY_IMPL;

// ---- modforge thread_pool ----
// CPU_ZERO / CPU_SET 是操作 cpu_set_t 的语句式宏，同样只能包成函数。
inline void CPU_ZERO_IMPL(cpu_set_t* set) noexcept { CPU_ZERO(set); }
#undef CPU_ZERO
export inline void CPU_ZERO(cpu_set_t* set) noexcept { CPU_ZERO_IMPL(set); }

inline void CPU_SET_IMPL(int cpu, cpu_set_t* set) noexcept { CPU_SET(cpu, set); }
#undef CPU_SET
export inline void CPU_SET(int cpu, cpu_set_t* set) noexcept { CPU_SET_IMPL(cpu, set); }

#endif
