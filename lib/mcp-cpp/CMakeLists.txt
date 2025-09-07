file(GLOB LIB_SOURCES *.cpp)
add_library(mcp-cpp STATIC ${LIB_SOURCES})

target_include_directories(mcp-cpp PUBLIC ${PROJECT_SOURCE_DIR}/include)
