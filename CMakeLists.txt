cmake_minimum_required(VERSION 3.10)

project(mcp_cpp,
    VERSION 1.0
    DESCRIPTION "C++ implementation of Model Context Protocol"
    LANGUAGES CXX
)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
endif()

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(lib/mcp-cpp)

add_subdirectory(examples/mcp_server EXCLUDE_FROM_ALL)
add_subdirectory(examples/mcp_client EXCLUDE_FROM_ALL)
