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
	McpServer server("MCP Test", "1.0.0.0");

	server.AddTool(
		"get_current_time",
		"Get the current time at the specified location.",
		std::vector<McpServer::McpProperty>
		{
			{ "location", McpServer::PROPERTY_STRING, "location", false }
		},
		std::vector<McpServer::McpProperty> {
			{ "date", McpServer::PROPERTY_STRING, "date", true },
			{ "time", McpServer::PROPERTY_STRING, "time", true }
		},
		[&server](const std::string& session_id, const std::map<std::string, std::string>& args)
		{
			std::vector<McpServer::McpContent> contents;

			char date_str[20];
			char time_str[20];
			time_t now = time(NULL);
    		struct tm* tm_info = localtime(&now);
			strftime(date_str, sizeof(date_str), "%Y/%m/%d", tm_info);
			strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);

			McpServer::McpContent content;

			content.properties.push_back({
				.property_name = "date",
				.value = date_str
				});
			content.properties.push_back({
				.property_name = "time",
				.value = time_str
				});
			contents.push_back(content);

			server.SendToolResponse(session_id, "get_current_time", contents);
		}
	);

	std::unique_ptr<std::thread> count_down_worker;

	server.AddTool(
		"count_down",
		"Counts down from a specified value.",
		std::vector<McpServer::McpProperty>
		{
			{ "value", McpServer::PROPERTY_STRING, "Start counting down from this value", true }
		},
		std::vector<McpServer::McpProperty> {},
		[&server, &count_down_worker](const std::string& session_id, const std::map<std::string, std::string>& args)
		{
			if (count_down_worker && count_down_worker->joinable())
			{
				count_down_worker->join();
			}

			int start_value = atoi(args.at("value").c_str());

			count_down_worker = std::make_unique<std::thread>([&server, session_id, start_value]
			{
				for (int i = start_value; i >= 0; i--)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));

					auto params = R"(
						{
							"value": 0
						}
					)"_json;
					params["value"] = i;

					server.SendToolNotification(session_id, "count_down", params, i == 0);
				}
			});

			std::vector<McpServer::McpContent> contents;

			McpServer::McpContent content;
			content.value = "start!",
			contents.emplace_back(content);

			server.SendToolResponse(session_id, "count_down", contents, false);
		}
	);

	std::unique_ptr<std::thread> count_down2_worker;

	server.AddTool(
		"count_down2",
		"Counts down from a specified value.(Type2)",
		std::vector<McpServer::McpProperty>
	{
		{ "value", McpServer::PROPERTY_STRING, "Start counting down from this value", true }
	},
		std::vector<McpServer::McpProperty> {},
		[&server, &count_down2_worker](const std::string& session_id, const std::map<std::string, std::string>& args)
		{
			if (count_down2_worker && count_down2_worker->joinable())
			{
				count_down2_worker->join();
			}

			int start_value = atoi(args.at("value").c_str());

			count_down2_worker = std::make_unique<std::thread>([&server, session_id, start_value]
			{
				for (int i = start_value; i > 0; i--)
				{
					auto params = R"(
						{
							"value": 0
						}
					)"_json;
					params["value"] = i;

					server.SendToolNotification(session_id, "count_down", params, false);

					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				}

				std::vector<McpServer::McpContent> contents;

				McpServer::McpContent content;
				content.value = "startfinish!",
				contents.emplace_back(content);

				server.SendToolResponse(session_id, "count_down", contents);
			});
		}
	);

#ifdef USE_HTTP_TRANSPORT
	McpHttpServerTransport transport("localhost:8000", "/mcp");

#if 0
	transport.SetTls(
		"cert.pem",
		"key.pem"
	);

	transport.SetAuthorization(
		"\"https://***tenant name***.us.auth0.com\"",
		"\"***api permission***\""
	);
#endif
#else
	McpStdioServerTransport transport;
#endif

	server.Run(&transport);
	
	return 0;
}
