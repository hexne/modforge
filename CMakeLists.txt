cmake_minimum_required(VERSION 4.0.0)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")

set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_MODULE_STD 1)

project(modforge VERSION "1.0.0")

file(GLOB module_files "*.cppm")

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    add_compile_options(-D_LIBCPP_ENABLE_EXPERIMENTAL)
    set(libc_link c++experimental)
endif()

add_library(modforge STATIC)

target_sources(modforge
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${module_files}
)

target_link_libraries(modforge
        ${libc_link}
)

add_library(unused_std_target STATIC)
target_sources(unused_std_target
        PRIVATE
        FILE_SET unused_files TYPE CXX_MODULES
        # BASE_DIRS /usr/include/c++/15.1.1/bits
        # FILES /usr/include/c++/15.1.1/bits/std.cc /usr/include/c++/15.1.1/bits/std.compat.cc
        BASE_DIRS /usr/share/libc++/v1
        FILES /usr/share/libc++/v1/std.cppm /usr/share/libc++/v1/std.compat.cppm
)

target_compile_features(modforge PUBLIC cxx_std_23)