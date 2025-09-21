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

#include "openai-cpp/openai.hpp"

#include <iostream>

using namespace Mcp;

int main()
{
	//-----------------------------------------------------
	// Initialize & Call Tools/List
	//-----------------------------------------------------

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

	//-----------------------------------------------------
	// Ask an LLM, "What time is it now in San Diego?"
	//-----------------------------------------------------

	openai::start("", "", true, "http://localhost:1234/v1/");

	auto request = R"(
	    {
			"model": "phi-4",
	        "messages": [
	        	{
	        		"role": "system", 
	        		"content": "You are a helpful assistant."
	        	},
	        	{
	        		"role": "user", 
	        		"content": "What time is it now in San Diego?"
	        	}
	        ],
			"tools": [],
			"tool_choice": "auto"
		}
	)"_json;

	for (auto it = tools.begin(); it != tools.end(); it++)
	{
		auto tool = R"(
			{
				"type": "function",
				"function": {
					"name": "",
					"description": "",
					"parameters": {
						"type": "object",
						"properties": {},
						"required": []
					}
				}
			}
		)"_json;

		tool["function"]["name"] = it->name;
		tool["function"]["description"] = it->description;

		request["tools"].push_back(tool);
	}

	std::cout << "LLM Request is:\n" << request.dump(2) << '\n';
	auto response = openai::chat().create(request);
	std::cout << "LLM Response is:\n" << response.dump(2) << '\n';

	//-----------------------------------------------------
	// Check the response for a tool call
	//-----------------------------------------------------

	nlohmann::json tool_message;
	std::string tool_call_id = "";
	nlohmann::json content;

	for (auto it = response["choices"].begin(); it != response["choices"].end(); it++)
	{
		auto message = (*it)["message"];
		if (message.contains("tool_calls"))
		{
			for (auto it2 = message["tool_calls"].begin(); it2 != message["tool_calls"].end(); it2++)
			{
				auto tool_call = (*it2);
				if (tool_call["function"]["name"] == "get_current_time")
				{
					tool_message = message;
					tool_call_id = tool_call["id"];

					// Call the tool via MCP
					std::map<std::string, std::string> args;
					client->ToolsCall("get_current_time", args, content);
					std::cout << "MCP Response is:\n" << content.dump(2) << std::endl;
					break;
				}
			}
		}
	}

	if (tool_call_id.empty())
	{
		std::cout << "No tool call found in the response.\n";
		return 0;
	}

	//-----------------------------------------------------
	// Set tools call result and call the LLM again
	//-----------------------------------------------------

	request["messages"].push_back(tool_message);

	auto func_output = R"(
		{
		    "role": "tool", 
		    "tool_call_id": "",
		    "content": []
		}
	)"_json;

	func_output["tool_call_id"] = tool_call_id;
	func_output["content"] = content;

	request["messages"].push_back(func_output);

	std::cout << "LLM Request is:\n" << request.dump(2) << '\n';
	auto response2 = openai::chat().create(request);
	std::cout << "LLM Response is:\n" << response2.dump(2) << '\n';

	//-----------------------------------------------------
	// Shutdown
	//-----------------------------------------------------

    client->Shutdown();
	
    return 0;
}
