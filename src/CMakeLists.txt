add_library(namespace_definitions INTERFACE)
target_compile_definitions(namespace_definitions INTERFACE
        "NAMESPACE_BEGIN=namespace modforge {"
        "NAMESPACE_END=}"
)

file(GLOB modforge_files "${CMAKE_CURRENT_SOURCE_DIR}/*.cppm")
if(NOT MODFORGE_ENABLE_REFLECTION)
    list(REMOVE_ITEM modforge_files "${CMAKE_CURRENT_SOURCE_DIR}/static_serialize.cppm")
endif()

set(LibMod STATIC)
add_library(modforge ${LibMod})
target_sources(modforge
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${modforge_files}
)

# 命名空间宏只在库自身构建时使用（编译本库模块源码）。
# 下游 find_package 重编安装的模块源码时，由 modforgeConfig.cmake 通过
# IMPORTED_CXX_MODULES_COMPILE_OPTIONS 注入同样的宏——只作用于模块编译，
# 不进入消费方自己代码的编译定义，避免 NAMESPACE_BEGIN/END 泄漏。
target_link_libraries(modforge PRIVATE namespace_definitions)