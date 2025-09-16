find_package(OpenSSL REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(uuid REQUIRED IMPORTED_TARGET uuid)

add_compile_definitions(MG_TLS=MG_TLS_OPENSSL)

add_library(mcp-cpp STATIC 
    platform/platform_posix.cpp
    mcp_server_transport.cpp
    mcp_http_server_transport_impl.cpp
    mcp_stdio_server_transport_impl.cpp
    mcp_server_impl.cpp
    mcp_client.cpp
)

target_include_directories(mcp-cpp PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(mcp-cpp PRIVATE ssl crypto PkgConfig::uuid)
