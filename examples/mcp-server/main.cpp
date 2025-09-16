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

#define USE_HTTP_TRANSPORT

#ifdef USE_HTTP_TRANSPORT
#include "mcp-cpp/mcp_http_server_transport.h"
#else
#include "mcp-cpp/mcp_stdio_server_transport.h"
#endif

#include <time.h>
#include <thread>

using namespace Mcp;

int main()
{
	std::unique_ptr<McpServer> server = McpServer::CreateInstance("MCP Test", "1.0.0.0");

	server->AddTool(
		{
			.name = "get_current_time",
			.description = "Get the current time at the specified location.",
			.input_schema = std::vector<McpProperty>
				{
					{ "location", MCP_PROPERTY_TYPE_STRING, "location", false }
				},
			.output_schema = std::vector<McpProperty>
				{
					{ "date", MCP_PROPERTY_TYPE_STRING, "date", true },
					{ "time", MCP_PROPERTY_TYPE_STRING, "time", true }
				}
		},
		[&server](const std::string& session_id, const std::map<std::string, std::string>& args)
		{
			std::vector<McpContent> contents;

			char date_str[20];
			char time_str[20];
			time_t now = time(NULL);
    		struct tm* tm_info = localtime(&now);
			strftime(date_str, sizeof(date_str), "%Y/%m/%d", tm_info);
			strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);

			McpContent content;
			content.properties.push_back({
				.property_name = "date",
				.value = date_str
				});
			content.properties.push_back({
				.property_name = "time",
				.value = time_str
				});
			contents.emplace_back(content);

			server->SendToolResponse(session_id, "get_current_time", contents);
		}
	);

	std::unique_ptr<std::thread> count_down_worker;

	server->AddTool(
		{
			.name = "count_down",
			.description = "Counts down from a specified value.",
			.input_schema = std::vector<McpProperty>
				{
					{ "value", MCP_PROPERTY_TYPE_STRING, "Start counting down from this value", true }
				}
		},
		[&server, &count_down_worker](const std::string& session_id, const std::map<std::string, std::string>& args)
		{
			if (count_down_worker && count_down_worker->joinable())
			{
				count_down_worker->join();
			}

			int start_value = atoi(args.at("value").c_str());

			count_down_worker = std::make_unique<std::thread>([&server, session_id, start_value]
			{
				for (int i = start_value; i > 0; i--)
				{
					auto params = R"(
						{
							"value": 0
						}
					)"_json;
					params["value"] = i;

					server->SendToolNotification(session_id, "count_down", params);

					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				}

				std::vector<McpContent> contents;

				McpContent content;
				content.properties.push_back({
					.value = "finish!"
					});
				contents.emplace_back(content);

				server->SendToolResponse(session_id, "count_down", contents);
			});
		}
	);

#ifdef USE_HTTP_TRANSPORT
	std::shared_ptr<McpHttpServerTransport> transport = std::move(McpHttpServerTransport::CreateInstance("localhost:8000", "/mcp"));

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
#else
	std::shared_ptr<McpStdioServerTransport> transport = std::move(McpStdioServerTransport::CreateInstance());
#endif

	server->Run(transport);

	return 0;
}
