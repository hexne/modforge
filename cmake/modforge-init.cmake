# ModForge 的构建前置配置：include 本文件只负责「定义」下面几个函数，
# 什么时候生效由调用方决定。
#
# - 供 modforge 自身构建使用（根 CMakeLists.txt 顶部 include）
# - 供下游在自身 project() 之前 include，随后即可 find_package(modforge)
#   或 add_subdirectory(modforge)。
#
# enable_import_std()  必须在 project() 之前调用。调用太晚或压根不调的后果不同：
#                      - 在 project() 之后才调：generate 阶段报
#                        "Experimental `import std` support not enabled when detecting
#                         toolchain; it must be set before `CXX` is enabled"
#                      - 完全不调：std 模块不会被接线，编译 `import std;` 直接报
#                        "fatal error: 未知的已编译模块接口：no such module"
#                      背景：CMake 的 `import std;` 支持是实验特性，需要按 CMake
#                      版本填好 CMAKE_EXPERIMENTAL_CXX_IMPORT_STD（UUID 随版本变化），
#                      并打开 CMAKE_CXX_MODULE_STD。
# enable_reflection()  必须在 add_subdirectory(src) 之前调用，开关反射模块是否参与编译。
# enable_embed(dir)    project() 之后、创建目标之前调用，见文件末尾。

# enable_import_std() —— 必须在 project() 之前调用。
# 函数自带作用域，两个变量都得用 PARENT_SCOPE 写回调用方，否则 project() 读不到。
function(enable_import_std)
    if(NOT DEFINED CMAKE_EXPERIMENTAL_CXX_IMPORT_STD)
        if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.30.0 AND CMAKE_VERSION VERSION_LESS 3.31.8)
            set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508" PARENT_SCOPE)
        elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 3.31.8 AND CMAKE_VERSION VERSION_LESS 4.0.0)
            set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444" PARENT_SCOPE)
        elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 4.0.0 AND CMAKE_VERSION VERSION_LESS 4.0.3)
            set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "a9e1cf81-9932-4810-974b-6eccaf14e457" PARENT_SCOPE)
        elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 4.0.3 AND CMAKE_VERSION VERSION_LESS 4.3.0)
            set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444" PARENT_SCOPE)
        elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 4.3.0 AND CMAKE_VERSION VERSION_LESS 4.4.0)
            set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b" PARENT_SCOPE)
        elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 4.4.0)
            set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "f35a9ac6-8463-4d38-8eec-5d6008153e7d" PARENT_SCOPE)
        endif()
    endif()

    set(CMAKE_CXX_MODULE_STD 1 PARENT_SCOPE)
endfunction()

# enable_reflection() —— 用「调用」代替原来的 option(MODFORGE_ENABLE_REFLECTION ... OFF)，
# 必须在 add_subdirectory(src) 之前调用：src/CMakeLists.txt 靠这个开关决定要不要把
# static_serialize.cppm 与 config_generator.cppm 编进 modforge 目标。
#
# 这里只拨开关，不挂编译选项——因为 -freflection 与 MODFORGE_ENABLE_REFLECTION 宏必须等
# modforge 目标建出来之后才能加（目标是 src/CMakeLists.txt 里创建的），根 CMakeLists 中
# 那段 if 块仍然保留。而且必须保留 PUBLIC：下游 find_package 后要靠它拿到 -freflection。
function(enable_reflection)
    set(MODFORGE_ENABLE_REFLECTION ON CACHE BOOL
            "启用 C++26 静态反射序列化模块（需要编译器支持 -freflection）" FORCE)
endfunction()

# ---- 资源嵌入（#embed）----
#
# enable_embed(<embed-dir>)
#   指定嵌入资源的搜索目录，并为「当前目录及其后续子目录」打开嵌入所需编译选项。
#   - 相对路径按调用方所在目录（CMAKE_CURRENT_SOURCE_DIR）解析
#   - 目录不存在则直接 configure 失败，避免编到一半才报找不到文件
#   - 解析后的绝对路径写入缓存变量 EMBED_DIR，供其它 CMake 脚本复用
#
#   期望在 project() 之后、创建目标之前调用一次。下面两种写法等价，只写一条：
#       enable_embed(resource)                                  # 相对调用方目录
#       enable_embed(${CMAKE_CURRENT_SOURCE_DIR}/resource)      # 显式绝对路径
#
#   注意：这里沿用了原写法，同时打开 -freflection 与 --embed-dir。若某处只需要 #embed
#   而不需要反射，删掉 -freflection 那一项即可。
function(enable_embed embed_dir)
    if("${embed_dir}" STREQUAL "")
        message(FATAL_ERROR "enable_embed(): embed_dir 不能为空")
    endif()

    if(NOT IS_ABSOLUTE "${embed_dir}")
        set(embed_dir "${CMAKE_CURRENT_SOURCE_DIR}/${embed_dir}")
    endif()
    cmake_path(NORMAL_PATH embed_dir)

    if(NOT IS_DIRECTORY "${embed_dir}")
        message(FATAL_ERROR "enable_embed(): 目录不存在：${embed_dir}")
    endif()

    set(EMBED_DIR "${embed_dir}" CACHE INTERNAL "embed resource directory" FORCE)
    add_compile_options(-freflection "--embed-dir=${EMBED_DIR}")
endfunction()
