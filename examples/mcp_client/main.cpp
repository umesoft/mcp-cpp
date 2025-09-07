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

    auto chat = openai::chat().create(
        R"(
            {
                "model": "phi-4",
                "messages":[{"role":"user", "content":"現在の時刻を教えて"}],
                "tools": [
                    {
                        "type": "function",
                        "function" : {
                            "name": "get_current_time",
                            "description": "Get the current time",
                            "parameters": {
                                "type": "object",
                                "properties": {
                                    "timezone": {
                                        "type": "string",
                                        "description": "(Optional) The timezone to get the current time for, e.g. 'America/New_York'"
                                    }
                                }
                            }
                        }
                    }
                ]
            }
        )"_json
    );
    std::cout << "Response is:\n" << chat.dump(2) << '\n'; 

    return 0;
}
