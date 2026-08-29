# ModForge 的 `import std;` 前置配置：必须在 project() 之前生效。
#
# - 供 modforge 自身构建使用（根 CMakeLists.txt 顶部 include）
# - 供下游在自身 project() 之前 include，随后即可 find_package(modforge)
#   或 add_subdirectory(modforge)。
#
# 背景：CMake 的 `import std;` 支持是实验特性，必须在 CXX 语言启用（project()）之前
# 设置好 CMAKE_EXPERIMENTAL_CXX_IMPORT_STD（UUID 随 CMake 版本变化）与
# CMAKE_CXX_MODULE_STD，否则 configure 阶段报
# "Experimental `import std` support not enabled"。

if(NOT DEFINED CMAKE_EXPERIMENTAL_CXX_IMPORT_STD)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.30.0 AND CMAKE_VERSION VERSION_LESS 3.31.8)
        set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")
    elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 3.31.8 AND CMAKE_VERSION VERSION_LESS 4.0.0)
        set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
    elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 4.0.0 AND CMAKE_VERSION VERSION_LESS 4.0.3)
        set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "a9e1cf81-9932-4810-974b-6eccaf14e457")
    elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 4.0.3 AND CMAKE_VERSION VERSION_LESS 4.3.0)
        set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
    elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 4.3.0 AND CMAKE_VERSION VERSION_LESS 4.4.0)
        set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")
    elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 4.4.0)
        set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "f35a9ac6-8463-4d38-8eec-5d6008153e7d")
    endif()
endif()

set(CMAKE_CXX_MODULE_STD 1)
