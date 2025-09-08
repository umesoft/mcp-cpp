find_package(CURL REQUIRED)

add_executable(mcp_client main.cpp)

target_include_directories(mcp_client PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(mcp_client PRIVATE mcp-cpp ${CURL_LIBRARIES})
