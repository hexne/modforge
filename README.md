# modforge

`modforge` 是一个基于 C++26 Modules 的工具库，目前包含三类能力：

- `core`：通用工具（时间、定时器、范围、线程池、目录等）
- `cv`：基于 OpenCV 的图像封装
- `deep_learning`：张量与向量基础能力

本文档描述的是当前仓库中代码的实际状态。

## 目录结构

```text
.
├─ src/         # 库模块源码
├─ tests/       # 测试（由 CMake 生成统一测试入口）
├─ examples/    # 示例代码
└─ resource/    # #embed 使用的资源
```

## 模块清单

### 顶层模块

- `modforge`
- `export import modforge.core;`
- `export import modforge.cv;`
- `export import modforge.deep_learning;`

### core

- `modforge.average_queue`
- `modforge.command`
- `modforge.console`
- `modforge.directory`
- `modforge.progress_bar`
- `modforge.range`
- `modforge.thread_pool`
- `modforge.time`
- `modforge.timer`

注意：`modforge.static_serialize` 为独立模块，不在 `modforge.core` 中重导出。

### cv

- `modforge.image`

### deep_learning

- `modforge.tensor`
- `modforge.deep_learning.tools`（当前为占位实现）

## 构建要求

项目使用了较新的 C++ 特性，需要：

- CMake：
- 主工程：`>= 3.30`
- `tests/` 与 `examples/`：`>= 4.0`
- 编译器/工具链支持：
- C++26 Modules（`import std`）
- `#embed`
- 当前仓库使用的反射编译选项（`-freflection`）
- OpenCV（`cv` 模块依赖）

## 构建库

```bash
cmake -S . -B build -G Ninja
cmake --build build
cmake --install build --prefix ./install
```

## 构建并运行测试

```bash
cmake -S tests -B tests/build -G Ninja
cmake --build tests/build
./tests/build/tests
```

## 构建并运行示例

```bash
cmake -S examples -B examples/build -G Ninja
cmake --build examples/build
./examples/build/examples
```

## 最小示例

```cpp
import modforge;
import std;

int main() {
    modforge::AverageQueue<int, 3> q{1, 2, 3};
    std::println("avg = {}", q.average());
    return 0;
}
```

## 当前状态说明

- API 仍在演进，尚未完全稳定。
- `modforge.deep_learning.tools` 尚未实现完整功能。
- `Console` 目前依赖 POSIX 头文件（`sys/ioctl.h`、`unistd.h`），跨平台行为存在差异。
- `cv` 模块在配置阶段即依赖 OpenCV。

## 许可证

MIT（见仓库根目录 `LICENSE`）。
