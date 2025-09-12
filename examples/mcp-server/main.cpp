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

#include "mcp-cpp/mcp_server.h"

// #define USE_HTTP_TRANSPORT

#ifdef USE_HTTP_TRANSPORT
#include "mcp-cpp/mcp_http_server_transport.h"
#else
#include "mcp-cpp/mcp_stdio_server_transport.h"
#endif

#include <time.h>

using namespace Mcp;

int main()
{
	McpServer server("MCP Test");

	server.AddTool(
		"get_current_time",
		"Get the current time at the specified location.",
		std::vector<McpServer::McpProperty> {
			{ "location", McpServer::PROPERTY_STRING, "location", false }
		},
		std::vector<McpServer::McpProperty> {},
		[](const std::map<std::string, std::string>& args) -> std::vector<McpServer::McpContent> {
			std::vector<McpServer::McpContent> contents;

			char datetime_str[20];
			time_t now = time(NULL);
    		struct tm* tm_info = localtime(&now);
    		strftime(datetime_str, sizeof(datetime_str), "%Y/%m/%d %H:%M:%S", tm_info);

			McpServer::McpContent content{
				.property_type = McpServer::PROPERTY_TEXT,
				.value = datetime_str,
			};
			contents.push_back(content);

			return contents;
		}
	);

	server.AddTool(
		"count_down",
		"Counts down from a specified value.",
		std::vector<McpServer::McpProperty> {
			{ "value", McpServer::PROPERTY_STRING, "Start counting down from this value", true }
	},
		std::vector<McpServer::McpProperty> {},
		[&server](const std::map<std::string, std::string>& args) -> std::vector<McpServer::McpContent> {
			std::vector<McpServer::McpContent> contents;

			std::string start_value_str = args.at("value");

			int start_value = atoi(start_value_str.c_str());

			for (int i = start_value; i > 0; i--)
			{
				auto params = R"(
					{
						"value": 0
					}
				)"_json;

				params["value"] = i;

				server.SendNotification("count_down", params);
			}

			McpServer::McpContent content{
				.property_type = McpServer::PROPERTY_TEXT,	
				.value = "finish!",
			};
			contents.push_back(content);

			return contents;
		}
	);

#ifdef USE_HTTP_TRANSPORT
	McpHttpServerTransport* transport = new McpHttpServerTransport();

#if 0
	transport->SetTls(
		"cert.pem",
		"key.pem"
	);

	transport->SetAuthorization(
		"\"https://***tenant name***.us.auth0.com\"",
		"\"***api permission***\""
	);
#endif

	transport->SetEntryPoint(
		"localhost:8000/mcp",
		10 * 60 * 1000
	);
#else
	McpStdioServerTransport* transport = new McpStdioServerTransport();
#endif

	server.Run(transport);
	
	delete transport;

	return 0;
}
