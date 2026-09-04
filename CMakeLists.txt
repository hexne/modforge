cmake_minimum_required(VERSION 3.30.0)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "")

include(cmake/modforge-init.cmake)

option(MODFORGE_ENABLE_REFLECTION "启用 C++26 静态反射序列化模块（需要编译器支持 -freflection）" OFF)

project(modforge VERSION 0.1.0)
add_subdirectory(src)
add_subdirectory(tests)

target_compile_features(modforge PUBLIC cxx_std_26)

# Windows：address 模块换用 winsock2.h 实现（inet_pton/inet_ntop/htons 等），需链接 ws2_32。
# socket/tcp/udp 在 Windows 侧目前是空实现（仅编译/链接通过），不调用 winsock 函数，
# 但统一在此链接，待后续接入 Winsock 时无需再改。
if(WIN32)
    target_link_libraries(modforge PUBLIC ws2_32)
endif()

if(MODFORGE_ENABLE_REFLECTION)
    target_compile_options(modforge PUBLIC -freflection)
    target_compile_definitions(modforge PUBLIC MODFORGE_ENABLE_REFLECTION)
endif()

include(CTest)

# ---- install / find_package 分发 ----
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS modforge
        EXPORT modforgeTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        FILE_SET cxx_modules DESTINATION ${CMAKE_INSTALL_LIBDIR}/modforge)

install(TARGETS namespace_definitions
        EXPORT modforgeTargets)

install(EXPORT modforgeTargets
        FILE modforgeTargets.cmake
        NAMESPACE modforge::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/modforge)

configure_package_config_file(
        cmake/modforgeConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/modforgeConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/modforge)

write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/modforgeConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/modforgeConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/modforgeConfigVersion.cmake
        cmake/modforge-init.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/modforge)

# ---- uninstall ----
if(NOT TARGET uninstall)
    configure_file(
            cmake/uninstall.cmake.in
            ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake
            @ONLY)
    add_custom_target(uninstall
            COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake)
endif()
