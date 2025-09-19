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
#include <thread>

using namespace Mcp;

int main()
{
	auto server = McpServer::CreateInstance("MCP Test Server", "1.0.0.0");

	server->AddTool(
		{
			.name = "get_current_time",
			.description = "Get the current time in UTC.",
		},
		[&server](const std::string& session_id, const std::map<std::string, std::string>& args)
		{
			time_t now = time(NULL);
    		struct tm* tm_info = gmtime(&now);

			char date_str[24];
			snprintf(
				date_str, 
				sizeof(date_str), 
				"%04d-%02d-%02dT%02d:%02d:%02dZ",
				tm_info->tm_year + 1900,
				tm_info->tm_mon + 1,
				tm_info->tm_mday,
				tm_info->tm_hour,
				tm_info->tm_min,
				tm_info->tm_sec
			);

			server->SendToolResponse(
				session_id,
				"count_down",
				CreateSimpleContent(date_str)
			);
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

				server->SendToolResponse(
					session_id,
					"count_down",
					CreateSimpleContent("finish!")
				);
			});
		}
	);

#ifdef USE_HTTP_TRANSPORT
	auto transport = McpHttpServerTransport::CreateInstance("localhost:8000", "/mcp");

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
	auto transport = McpStdioServerTransport::CreateInstance();
#endif

	server->Run(std::move(transport));

	return 0;
}
