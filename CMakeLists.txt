cmake_minimum_required(VERSION 3.29)

project(NeLib)
set(VERSION "1.0.0")
set(CMAKE_CXX_STANDARD 23)


include(GNUInstallDirs)

file(GLOB module_files "*.cppm")

function(remove_opencv_modules)
    list(FILTER module_files EXCLUDE REGEX ".*/(Image|Video|Camera|CNN)\\.cppm$")
    set(module_files ${module_files} PARENT_SCOPE)
    message(STATUS "remove gcc ice module")
endfunction()

function(remove_libstdcxx_unsupported_modules)
    list(FILTER module_files EXCLUDE REGEX ".*/(Image|Video|Camera|Matrix|BP|CNN)\\.cppm$")
    set(module_files ${module_files} PARENT_SCOPE)
    message(STATUS "Removed unsupported modules for libstdc++")
endfunction()


if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    remove_libstdcxx_unsupported_modules()
    message(STATUS "libstdc++ with GNU")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(CMAKE_CXX_FLAGS MATCHES "-stdlib=libc\\+\\+" OR CMAKE_CXX_FLAGS_INIT MATCHES "-stdlib=libc\\+\\+")
        message(STATUS "Using libc++ with Clang (explicitly specified)")
        add_compile_options(-D_LIBCPP_ENABLE_EXPERIMENTAL)
        add_link_options(-lc++experimental)
    else()
        remove_libstdcxx_unsupported_modules()
        message(STATUS "Using libstdc++ with Clang")
    endif ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(STATUS "MSVC")
endif()

find_package(OpenCV QUIET)
if(NOT OpenCV_FOUND)
    remove_opencv_modules()
    message(STATUS "没有找到opencv库，因此下列模块跳过编译: Image、Video、Camera")
endif ()


add_library(NeLib STATIC)
target_sources(NeLib
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${module_files}
)
target_link_libraries(NeLib
        ${OpenCV_LIBS}
)
target_compile_features(NeLib PUBLIC cxx_std_23)

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/NeLibConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/NeLibConfig.cmake"
        @ONLY
)

install(FILES "tools.h" DESTINATION ${CMAKE_INSTALL_LIBDIR}/NeLib)
install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}
        FILE_SET cxx_modules
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/NeLib)

install(EXPORT ${PROJECT_NAME}
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NeLib)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/NeLibConfig.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NeLib)

add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -E echo "uninstall NeLib..."
        COMMAND ${CMAKE_COMMAND} -E remove_directory "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/NeLib"
        COMMAND ${CMAKE_COMMAND} -E echo "remove $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/NeLib"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/NeLib"
        COMMAND ${CMAKE_COMMAND} -E echo "remove $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/NeLib"
        COMMAND ${CMAKE_COMMAND} -E echo "finish."
)
