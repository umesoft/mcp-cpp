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
#include <time.h>

using namespace Mcp;

int main()
{
	McpServer server("MCP Test");

#if 0
	server.SetTls(
		"cert.pem",
		"key.pem"
	);

	server.SetAuthorization(
		"\"https://***tenant name***.us.auth0.com\"",
		"\"***api permission***\""
	);
#endif

	server.AddTool(
		"get_current_time",
		"指定された場所の現在時刻を取得",
		std::vector<McpServer::McpProperty> {
			{ "location", McpServer::PROPERTY_STRING, "場所", false }
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

	server.Run(
		"localhost:8000/mcp",
		10 * 60 * 1000
	);

	return 0;
}
