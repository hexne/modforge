cmake_minimum_required(VERSION 3.29)

project(NeLib)

set(VERSION "1.0.0")

set(CMAKE_CXX_STANDARD 23)

file(GLOB module_files "*.cppm")

# gnu 不支持指定 libc++
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    list(REMOVE_ITEM module_files
            "${CMAKE_CURRENT_SOURCE_DIR}/Image.cppm"
            "${CMAKE_CURRENT_SOURCE_DIR}/Video.cppm"
            "${CMAKE_CURRENT_SOURCE_DIR}/Camera.cppm"
            "${CMAKE_CURRENT_SOURCE_DIR}/Matrix.cppm"
            "${CMAKE_CURRENT_SOURCE_DIR}/BP.cppm"
            "${CMAKE_CURRENT_SOURCE_DIR}/CNN.cppm"
    )
    message(STATUS "libstdc++ with GNU")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(CMAKE_CXX_FLAGS MATCHES "-stdlib=libc\\+\\+" OR CMAKE_CXX_FLAGS_INIT MATCHES "-stdlib=libc\\+\\+")
        message(STATUS "Using libc++ with Clang (explicitly specified)")
        add_compile_options(-D_LIBCPP_ENABLE_EXPERIMENTAL)
        set(LIBCPP_LINK "c++ c++abi c++experimental")
    else()
        list(REMOVE_ITEM module_files
                "${CMAKE_CURRENT_SOURCE_DIR}/Image.cppm"
                "${CMAKE_CURRENT_SOURCE_DIR}/Video.cppm"
                "${CMAKE_CURRENT_SOURCE_DIR}/Camera.cppm"
                "${CMAKE_CURRENT_SOURCE_DIR}/Matrix.cppm"
                "${CMAKE_CURRENT_SOURCE_DIR}/BP.cppm"
                "${CMAKE_CURRENT_SOURCE_DIR}/CNN.cppm"
        )
        message(STATUS "Using libstdc++ with Clang")
    endif ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(STATUS "MSVC")
endif()



find_package(OpenCV)
if(NOT OpenCV_FOUND)
   list(REMOVE_ITEM module_files
        "${CMAKE_CURRENT_SOURCE_DIR}/Image.cppm"
        "${CMAKE_CURRENT_SOURCE_DIR}/Video.cppm"
        "${CMAKE_CURRENT_SOURCE_DIR}/Camera.cppm"
   )
    message(STATUS "没有找到opencv库，因此下列模块跳过编译: Image、Video、Camera")
endif ()



find_package(jsoncpp CONFIG)
set(JsonCPP_LINK JsonCpp::JsonCpp)
if(NOT jsoncpp_FOUND)
   list(REMOVE_ITEM module_files
        "${CMAKE_CURRENT_SOURCE_DIR}/Json.cppm"
   )
    message(STATUS "没有找到jsoncpp库，因此下列模块跳过编译: Json")
set(JsonCPP_LINK "")
endif ()


add_library(NeLib STATIC)
target_sources(NeLib
    PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
    ${module_files}
)
target_link_libraries(NeLib
        ${OpenCV_LIBS}
        ${JsonCPP_LINK}
        ${LIBCPP_LINK}
)
target_compile_features(NeLib PUBLIC cxx_std_23)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/NeLibConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/NeLibConfig.cmake"
    @ONLY
)

install(FILES "tools.h" DESTINATION /usr/lib/NeLib)
install(TARGETS  ${PROJECT_NAME}  DESTINATION /usr/lib/NeLib EXPORT ${PROJECT_NAME} FILE_SET cxx_modules)
install(EXPORT ${PROJECT_NAME} DESTINATION /usr/lib/cmake/NeLib/)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/NeLibConfig.cmake"
        DESTINATION "${test_INSTALL_DIR}/lib/cmake/NeLib/")
