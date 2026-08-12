add_library(namespace_definitions INTERFACE)
target_compile_definitions(namespace_definitions INTERFACE
        "NAMESPACE_BEGIN=namespace modforge {"
        "NAMESPACE_END=}"
)

file(GLOB modforge_files "./*.cppm")

set(LibMod STATIC)
add_library(modforge ${LibMod})
target_sources(modforge
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${modforge_files}
)

target_link_libraries(modforge PRIVATE namespace_definitions)

target_compile_features(modforge PUBLIC cxx_std_26)