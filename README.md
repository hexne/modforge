# ModForge

> Modern C++ Utility Library

一个基于现代 C++ 的通用基础库，提供容器、并发、文件、时间、字符串、静态反射序列化等常用模块。

## ✨ Features

- 🧩 以 C++20 modules 组织，全部通过 `import modforge;` 使用
- 🚀 现代 C++ / C++26，无第三方依赖
- 🧵 并发与无锁数据结构（SPSC / MPSC / SPMC / MPMC）
- 🔍 C++26 静态反射序列化（可选模块，默认关闭）
- 🧪 以单文件单测试函数方式维护模块测试
- 🛠️ 覆盖参数解析、文件系统、时间、线程池、定时器、树结构、信号与终端辅助模块

## 📦 Modules

| 模块               | 说明                               | 状态 |
| ------------------ |------------------------------------| :--: |
| `args_parser`      | 命令行参数解析                     |  ✅   |
| `directory`        | 目录操作                           |  ✅   |
| `file`             | 文件操作                           |  ✅   |
| `lock_free_queue`  | SPSC / MPSC / SPMC / MPMC 无锁队列 |  ✅   |
| `thread_pool`      | 线程池                             |  ✅   |
| `time`             | 时间工具（UTC / 本地时间、解析与格式化） |  ✅   |
| `timer`            | 定时器、协程定时器                 |  ✅   |
| `signal`           | 信号/回调机制                      |  ✅   |
| `terminal`         | 终端尺寸与光标控制                 |  ✅   |
| `table`            | 终端表格渲染                       |  ✅   |
| `string_utils`     | 字符串工具                         |  ✅   |
| `tree`             | 通用 N 叉树与索引树                |  ✅   |
| `utils`            | 通用工具                           |  ✅   |
| `static_serialize` | C++26 静态反射序列化               |  🔌   |
| `event`            | 事件模块（当前为空壳，未接入总入口） |  🚧   |

图例：✅ 默认构建 · 🔌 可选，需显式开启 · 🚧 占位未实现

## 🔗 Module Dependencies

```mermaid
graph TD
    modforge --> args_parser
    modforge --> directory
    modforge --> file
    modforge --> lock_free_queue
    modforge --> thread_pool
    modforge --> time
    modforge --> timer
    modforge --> tree
    modforge --> utils
    modforge --> string_utils
    modforge --> signal
    modforge --> terminal
    modforge --> table
    modforge -.-> static_serialize

    args_parser --> string_utils
    args_parser --> utils
    directory --> string_utils
    file --> utils
    thread_pool --> lock_free_queue
    timer --> time
    table --> terminal
```

虚线表示 `static_serialize` 仅在开启 `MODFORGE_ENABLE_REFLECTION` 时存在。
`event` 未接入总入口，如需使用得单独 `import modforge.event;`。

## 🔨 Build

需要 **GCC trunk**（17.0.0 experimental）。系统 GCC 16.2.1 无法构建：`directory` 模块用到
`std::filesystem::path::display_string()`，该接口在 16.2.1 的 libstdc++ 中尚不存在。

```bash
cmake -GNinja -DCMAKE_CXX_COMPILER="/path/gcc-trunk/bin/g++" ..      
```

## 🔌 可选模块

| 选项                          | 默认 | 影响的模块            |
|-----------------------------|------|------------------|
| `MODFORGE_ENABLE_REFLECTION` | OFF  | `static_serialize` |

命令行开启：

```bash
cmake -S . -B build-check -G Ninja -DMODFORGE_ENABLE_REFLECTION=ON
```

作为子项目引入时，在 `add_subdirectory()` 之前设置：

```cmake
set(MODFORGE_ENABLE_REFLECTION ON CACHE BOOL "" FORCE)
add_subdirectory(modforge)
```

关闭时 `static_serialize` 不参与编译，`test_static_serialize` 也不会注册到 CTest。
开启后 `-freflection` 会随 `modforge` 目标传递给下游，**下游整体将切入反射方言**——
不用反射的项目保持默认 OFF 即可，不受影响。

## 📖 在项目中引入

```cmake
add_subdirectory(modforge)
target_link_libraries(my_app PRIVATE modforge)
```

之后即可 `import modforge;` 使用，各模块的接口见 `src/*.cppm` 与 `tests/` 下对应的用例。

## 🧪 测试

项目使用 CTest，默认为 14 个用例，开启反射后为 15 个。

```bash
ctest --test-dir build-check --output-on-failure
```

| 测试 | 模块 |
| --- | --- |
| `test_args_parser` | `args_parser` |
| `test_directory` | `directory` |
| `test_file` | `file` |
| `test_lock_free_queue` | `lock_free_queue` |
| `test_string_utils` | `string_utils` |
| `test_time` | `time` |
| `test_timer` | `timer` |
| `test_thread_pool` | `thread_pool` |
| `test_tree` | `tree` |
| `test_utils` | `utils` |
| `test_signal` | `signal` |
| `test_event` | `event`（空测试） |
| `test_table` | `table` |
| `test_terminal` | `terminal` |
| `test_static_serialize` | `static_serialize`（🔌 可选） |

单个测试可直接运行：

```bash
./build-check/tests/tests test_timer
./build-check/tests/tests test_signal
./build-check/tests/tests test_tree
```

## 🗺️ Roadmap

### 基础工具

- [ ] Copy-on-Write

### 通信与事件

- [ ] Event Bus
- [ ] `event` 模块实体化（当前为空壳，且未接入总入口）

### 终端与配置

- [x] Terminal Table
- [ ] Configuration

### 工程化

- [ ] `find_package` 安装分发（install / export 规则）
