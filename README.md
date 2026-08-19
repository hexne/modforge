# modforge

modforge 是一个基于 C++ 模块（C++26）的工具库示例工程，内含若干可复用模块以及对应的单元测试。

## 功能概览

- 参数解析
- 文件/目录工具
- 无锁队列（多种并发队列实现）
- 线程池
- 时间/定时器工具
- 树结构（Tree / IndexedTree）
- 字符串工具（拆分/拼接）
- 通用辅助工具

## 构建要求

- CMake 3.30 及以上
- GCC 17
- Ninja

示例：

```bash
export CXX=/path/gcc-trunk/bin/g++
cmake -S . -B build-check -G Ninja
```

## 构建与运行测试

构建测试目标：

```bash
cmake --build build-check --target tests
```

运行所有测试：

```bash
ctest --test-dir build-check --output-on-failure
```

也可以直接按测试名运行单个测试：

```bash
./build-check/tests/tests test_args_parser
./build-check/tests/tests test_tree
./build-check/tests/tests test_string_utils
```

## 仓库结构

- `src/`：项目模块源码（每个 `.cppm` 为一个模块）
- `tests/`：测试代码，每个测试文件包含单个测试入口函数