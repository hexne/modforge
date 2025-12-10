cmake_minimum_required(VERSION 4.0.0)

set(CMAKE_CXX_STANDARD 26)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.30.0 AND CMAKE_VERSION VERSION_LESS_EQUAL 3.31.7)
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")
elseif(CMAKE_VERSION EQUAL 3.31.8)
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 4.0.0 AND CMAKE_VERSION VERSION_LESS_EQUAL 4.0.2)
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "a9e1cf81-9932-4810-974b-6eccaf14e457")
elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 4.0.3) # AND CMAKE_VERSION VERSION_LESS_EQUAL 4.1.0)
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_MODULE_STD 1)

project(tests)

# list(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/custom_libs/modforge")
# find_package(modforge)
add_subdirectory("../" modforge)

# 搜索所有测试 cpp 文件
file(GLOB_RECURSE TEST_SOURCES
        "${CMAKE_SOURCE_DIR}/core/*.cpp"
        "${CMAKE_SOURCE_DIR}/cv/*.cpp"
        "${CMAKE_SOURCE_DIR}/deep_learning/*.cpp"
)

# 生成 main.cpp
set(MAIN_FILE "${CMAKE_BINARY_DIR}/main.cpp")

# 写入 main.cpp
file(WRITE ${MAIN_FILE} "import std;\n\n")

# 先写函数声明
foreach(src ${TEST_SOURCES})
    get_filename_component(fname ${src} NAME_WE)
    file(APPEND ${MAIN_FILE} "int ${fname}();\n")
endforeach()

file(APPEND ${MAIN_FILE} "\n")

file(APPEND ${MAIN_FILE} "std::map<std::string, std::function<int()>> test_funcs {\n")
foreach(src ${TEST_SOURCES})
    get_filename_component(fname ${src} NAME_WE)
    file(RELATIVE_PATH relpath ${CMAKE_SOURCE_DIR} ${src})
    file(APPEND ${MAIN_FILE} "    {\"${relpath}\", ${fname}},\n")
endforeach()
file(APPEND ${MAIN_FILE} "};\n\n")


file(APPEND ${MAIN_FILE} "int main() {\n")
file(APPEND ${MAIN_FILE} "    for (const auto& [name, func] : test_funcs) {\n")
file(APPEND ${MAIN_FILE} "        int line = func();\n")
file(APPEND ${MAIN_FILE} "        if (line)\n")
file(APPEND ${MAIN_FILE} "            std::println(\" [ FAILED ] {} (LINE {})\", name, line);\n")
file(APPEND ${MAIN_FILE} "        else\n")
file(APPEND ${MAIN_FILE} "            std::println(\" [  PASS  ] {} \", name);\n")
file(APPEND ${MAIN_FILE} "    }\n")
file(APPEND ${MAIN_FILE} "    return 0;\n")
file(APPEND ${MAIN_FILE} "}\n")


# 添加可执行文件
add_executable(tests ${MAIN_FILE} ${TEST_SOURCES})
target_link_libraries(tests modforge::modforge stdc++exp)

