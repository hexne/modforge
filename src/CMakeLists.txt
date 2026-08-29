add_library(namespace_definitions INTERFACE)
target_compile_definitions(namespace_definitions INTERFACE
        "NAMESPACE_BEGIN=namespace modforge {"
        "NAMESPACE_END=}"
)

file(GLOB modforge_files "${CMAKE_CURRENT_SOURCE_DIR}/*.cppm")
if(NOT MODFORGE_ENABLE_REFLECTION)
    list(REMOVE_ITEM modforge_files "${CMAKE_CURRENT_SOURCE_DIR}/static_serialize.cppm")
endif()

set(LibMod STATIC)
add_library(modforge ${LibMod})
target_sources(modforge
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${modforge_files}
)

target_link_libraries(modforge PRIVATE namespace_definitions)