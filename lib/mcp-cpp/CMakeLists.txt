find_package(OpenSSL REQUIRED)

add_compile_definitions(MG_TLS=MG_TLS_OPENSSL)

add_library(mcp-cpp STATIC 
    platform/platform_posix.cpp
    mcp_server.cpp
    mcp_client.cpp
)

target_include_directories(mcp-cpp PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(mcp-cpp PRIVATE ssl crypto)
