#include "mcp-cpp/mcp_client.h"
#include "openai-cpp/openai.hpp"

#include <iostream>

int main()
{
    /*
    Mcp::McpClient client;
    client.Test();
    */

    openai::start("", "", true, "http://127.0.0.1:1234/v1/");

    auto chat = openai::chat().create(R"(
    {
        "model": "phi-4",
        "messages":[{"role":"user", "content":"hello!"}]
    }
    )"_json);
    std::cout << "Response is:\n" << chat.dump(2) << '\n'; 

    return 0;
}
