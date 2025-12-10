cmake_minimum_required(VERSION 3.30.0)
set(CMAKE_CXX_STANDARD 26)

# 针对不同 CMake 版本的 std import hack
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.30.0 AND CMAKE_VERSION VERSION_LESS_EQUAL 3.31.7)
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")
elseif(CMAKE_VERSION EQUAL 3.31.8)
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 4.0.0 AND CMAKE_VERSION VERSION_LESS_EQUAL 4.0.2)
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "a9e1cf81-9932-4810-974b-6eccaf14e457")
elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 4.0.3)
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
endif()

set(CMAKE_CXX_MODULE_STD 1)

project(modforge VERSION 0.0.1 LANGUAGES CXX)

add_subdirectory(src)

include(CMakePackageConfigHelpers)

# 生成版本文件
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/modforgeConfigVersion.cmake"
        COMPATIBILITY SameMajorVersion
)

# 配置 .in 模板生成最终的 Config.cmake
configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/modforgeConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/modforgeConfig.cmake"
        INSTALL_DESTINATION lib/cmake/modforge
)

# 安装导出的 targets
install(TARGETS core cv deep_learning modforge
        EXPORT modforgeTargets
        FILE_SET cxx_modules DESTINATION include
)

install(EXPORT modforgeTargets
        NAMESPACE modforge::
        DESTINATION lib/cmake/modforge
)

# 安装 Config 和 Version 文件
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/modforgeConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/modforgeConfigVersion.cmake"
        DESTINATION lib/cmake/modforge
)


if(NOT TARGET modforge::core)
    add_library(modforge::core ALIAS core)
endif()
if(NOT TARGET modforge::cv)
    add_library(modforge::cv ALIAS cv)
endif()
if(NOT TARGET modforge::deep_learning)
    add_library(modforge::deep_learning ALIAS deep_learning)
endif()
if(NOT TARGET modforge::modforge)
    add_library(modforge::modforge ALIAS modforge)
endif()