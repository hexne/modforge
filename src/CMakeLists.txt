add_library(namespace_definitions INTERFACE)
target_compile_definitions(namespace_definitions INTERFACE
        "NAMESPACE_BEGIN=namespace modforge {"
        "NAMESPACE_END=}"
)

file(GLOB cv_files "./cv/*.cppm")
file(GLOB core_files "./core/*.cppm")
file(GLOB deep_learning_files "./deep_learning/*.cppm")
file(GLOB modforge_files "./*.cppm")

set(LibMod STATIC)
add_library(core ${LibMod})
target_sources(core
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${core_files}
)
target_link_libraries(core PRIVATE namespace_definitions)

add_library(cv ${LibMod})
target_sources(cv
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${cv_files}
)
find_package(OpenCV REQUIRED)
target_link_libraries(cv PRIVATE ${OpenCV_LIBS} namespace_definitions)


add_library(deep_learning ${LibMod})
target_sources(deep_learning
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${deep_learning_files}
)
target_link_libraries(deep_learning PRIVATE core namespace_definitions)

add_library(modforge ${LibMod})
target_sources(modforge
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${modforge_files}
)
target_link_libraries(modforge PUBLIC core cv deep_learning)


target_compile_features(core PUBLIC cxx_std_26)
target_compile_features(cv PUBLIC cxx_std_26)
target_compile_features(deep_learning PUBLIC cxx_std_26)
