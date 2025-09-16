/*
 *  Copyright (C) 2025 UmeSoftware LLC
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "mcp-cpp/mcp_client.h"
#include "mcp-cpp/mcp_http_client_transport.h"
#include "openai-cpp/openai.hpp"

using namespace Mcp;

void test()
{
    openai::start("", "", true, "http://127.0.0.1:1234/v1/");

    auto chat = openai::chat().create(R"(
    {
        "model": "phi-4",
        "messages":[{"role":"user", "content":"hello!"}]
    }
    )"_json);
    std::cout << "Response is:\n" << chat.dump(2) << '\n';
}

int main()
{
	std::shared_ptr<McpHttpClientTransport> transport = std::move(McpHttpClientTransport::CreateInstance(
        "http://localhost:8000", 
        "/mcp")
    );
	
    auto client = McpClient::CreateInstance("MCP Test Client", "1.0.0.0");
	client->Initialize(transport);
	
    std::vector<McpTool> tools;
    client->ToolsList(tools);

    std::map<std::string, std::string> args;
    args["value"] = "3";
    client->ToolsCall("count_down", args);
	
    client->Shutdown();

    return 0;
}
