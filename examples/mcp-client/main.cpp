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

// #define USE_HTTP_TRANSPORT

#ifdef USE_HTTP_TRANSPORT
#include "mcp-cpp/mcp_http_client_transport.h"
#else
#include "mcp-cpp/mcp_stdio_client_transport.h"
#endif

#include <iostream>

using namespace Mcp;

int main()
{
#ifdef USE_HTTP_TRANSPORT
	std::shared_ptr<McpHttpClientTransport> transport = std::move(McpHttpClientTransport::CreateInstance("http://localhost:8000", "/mcp"));
#else
#ifdef _WIN32
	std::shared_ptr<McpStdioClientTransport> transport = std::move(McpStdioClientTransport::CreateInstance(L"mcp-server.exe"));
#else
	std::shared_ptr<McpStdioClientTransport> transport = std::move(McpStdioClientTransport::CreateInstance(L"../mcp-server/mcp-server"));
#endif
#endif

    auto client = McpClient::CreateInstance("MCP Test Client", "1.0.0.0");
	client->Initialize(transport);
	
	std::vector<McpTool> tools;
	client->ToolsList(tools);

	std::map<std::string, std::string> args = {
		{ "value", "5"  }
	};
	nlohmann::json content;
	client->ToolsCall(
		"count_down", 
		args, 
		content,
		[](const nlohmann::json& notification)
		{
			std::cout << "Notification: " << notification.dump(2) << std::endl;
			return true;
		}
	);

	std::cout << "content: " << content.dump(2) << std::endl;

    client->Shutdown();
	
    return 0;
}
