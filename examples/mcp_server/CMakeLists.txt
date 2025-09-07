add_executable(mcp_server main.cpp)

target_include_directories(mcp_server PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(mcp_server PRIVATE mcp-cpp)