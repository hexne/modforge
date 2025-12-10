file(GLOB cv_files "./cv/*.cppm")
file(GLOB core_files "./core/*.cppm")
file(GLOB deep_learning_files "./deep_learning/*.cppm")
file(GLOB modforge_files "./*.cppm")


#if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
#    # 移除 cv 中的 image 和 video 文件
#    list(REMOVE_ITEM cv_files
#            "${CMAKE_CURRENT_SOURCE_DIR}/cv/image.cppm"
#            "${CMAKE_CURRENT_SOURCE_DIR}/cv/modforge.cv.cppm"
#    )
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # 移除 core 中的 time 文件
    list(REMOVE_ITEM core_files
            "${CMAKE_CURRENT_SOURCE_DIR}/core/time.cppm"
    )
endif()


set(LibMod STATIC)
add_library(core ${LibMod})
target_sources(core
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${core_files}
)

#add_library(cv ${LibMod})
#target_sources(cv
#        PUBLIC
#        FILE_SET cxx_modules TYPE CXX_MODULES FILES
#        ${cv_files}
#)
#find_package(OpenCV REQUIRED)
#target_link_libraries(cv PUBLIC ${OpenCV_LIBS})


add_library(deep_learning ${LibMod})
target_sources(deep_learning
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${deep_learning_files}
)
target_link_libraries(deep_learning PUBLIC core)

add_library(modforge ${LibMod})
target_sources(modforge
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${modforge_files}
)
target_link_libraries(modforge PUBLIC core deep_learning)

target_compile_features(core PUBLIC cxx_std_26)
#target_compile_features(cv PUBLIC cxx_std_26)
target_compile_features(deep_learning PUBLIC cxx_std_26)

