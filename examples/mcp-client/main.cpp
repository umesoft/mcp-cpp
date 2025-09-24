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

#define USE_HTTP_TRANSPORT

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

	auto authorization = transport->GetAuthorization();
	authorization->SetRedirectPortNo(3210);

#else
#ifdef _WIN32
	std::shared_ptr<McpStdioClientTransport> transport = std::move(McpStdioClientTransport::CreateInstance(L"mcp-server.exe"));
#else
	std::shared_ptr<McpStdioClientTransport> transport = std::move(McpStdioClientTransport::CreateInstance(L"../mcp-server/mcp-server"));
#endif
#endif

    auto client = McpClient::CreateInstance("MCP Test Client", "1.0.0.0");
	if (!client->Initialize(transport))
	{
		return 0;
	}
	
	std::vector<McpTool> tools;
	if (!client->ToolsList(tools))
	{
		return 0;
	}

	nlohmann::json content;
	if (!client->ToolsCall(
		"count_down",
		{
			{ "value", "5"  }
		},
		content,
		[](const std::string& method, const nlohmann::json& params)
		{
			std::cout << "Notification: " << method << std::endl << params.dump(2) << std::endl;
			return true;
		}
	))
	{
		return 0;
	}

	std::cout << "content: " << content.dump(2) << std::endl;

    client->Shutdown();

    return 0;
}
