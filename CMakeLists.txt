cmake_minimum_required(VERSION 4.4)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "f35a9ac6-8463-4d38-8eec-5d6008153e7d")
set(CMAKE_CXX_MODULE_STD 1)

add_compile_options(
        -freflection
)

project(modforge)
add_subdirectory(src)
add_subdirectory(tests)

include(CTest)