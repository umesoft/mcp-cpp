add_executable(mcp-server main.cpp)

target_include_directories(mcp-server PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(mcp-server PRIVATE mcp-cpp)