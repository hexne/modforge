cmake_minimum_required(VERSION 3.29)

project(modforge)
set(VERSION "1.0.0")
set(CMAKE_CXX_STANDARD 26)

file(GLOB module_files "*.cppm")

add_library(modforge STATIC)
target_sources(modforge
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${module_files}
)

target_compile_features(modforge PUBLIC cxx_std_26)