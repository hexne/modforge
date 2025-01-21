# NeLib
- [x] Image
- [x] Camera
- [x] Video
- [x] File
- [x] MultArray
- [x] StringEncrypt
- [x] BigNumber <sup> @TODO, 暂时仅支持整数，不支持浮点数
- [x] Time
- [x] TestRunTime
- [x] ThreadPool
- [x] MemoryPool
- [x] Mouse <sup> @TODO, linux版本未实现, 无需终端版本
- [x] EventListener <sup> @TODO, linux版本未实现, 应当实现一个windows/linux的终端版本
- [x] ProgressBar <sup> @TODO, 应当优化进度条的显示
- [ ] TerminalMenu <sup> @TODO, 不能使用键盘上下按键进行选择
- [x] Matrix
- [x] NeuralNetwork

# 安装
git clone https://github.com/hexne/NeLib.git  
cd NeLib  
mkdir build && cd build  
cmake .. -G Ninja -DCMAKE_CXX_COMPILER=/usr/bin/clang++  
cmake --build .  
sudo cmake --install .  


# 使用
### CMakeLists.txt
```
cmake_minimum_required(VERSION 3.29)
project(untitled)

set(CMAKE_CXX_STANDARD 23)

find_package(NeLib REQUIRED)


add_executable(untitled
    main.cpp
)

target_link_libraries(untitled
    NeLib
)

```
### xmake
```
add_rules("mode.release", "mode.debug")

add_repositories("nelib ~/Projects/xmake-repo")

set_languages("c++2b")
add_requires("nelib 2024.11.15", {configs = {modules = true}})

target("test")
    set_kind("binary")
    add_files("src/*.cpp")
    add_packages("nelib")
    set_policy("build.c++.modules", true)
```
