find_package(CURL REQUIRED)

add_executable(mcp-agent main.cpp)

target_include_directories(mcp-agent PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(mcp-agent PRIVATE mcp-cpp ${CURL_LIBRARIES})
