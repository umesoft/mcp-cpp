find_package(CURL REQUIRED)

add_executable(mcp-client main.cpp)

target_include_directories(mcp-client PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(mcp-client PRIVATE mcp-cpp ${CURL_LIBRARIES})
