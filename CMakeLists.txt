cmake_minimum_required(VERSION 3.30)

project(modforge)
set(VERSION "1.0.0")

file(GLOB module_files "*.cppm")

add_library(modforge STATIC)
target_sources(modforge
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${module_files}
)

if(CMAKE_COMPILER_ID STREQUAL "Clang")
add_compile_options(-D_LIBCPP_ENABLE_EXPERIMENTAL)
target_link_libraries(modforge
        c++
        c++abi
        c++experimental
)
endif()


target_compile_features(modforge PUBLIC cxx_std_23)