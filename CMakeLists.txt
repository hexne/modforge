cmake_minimum_required(VERSION 4.0.0)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "a9e1cf81-9932-4810-974b-6eccaf14e457")

set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_MODULE_STD 1)

project(modforge VERSION "1.0.0")

file(GLOB module_files "*.cppm")

add_library(modforge STATIC)

target_sources(modforge
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${module_files}
)
