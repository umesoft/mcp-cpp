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

void test_assistant()
{
    openai::start("", "", true, "http://172.16.2.229:1234/v1/");

	auto request = R"(
	    {
	        "model": "gpt-oss-20B",
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
			"tools": [
	    		{
	      			"type": "function",
					"function": {
						"name": "get_current_time",
				        "description": "Get the current time in UTC",
        				"parameters": {
          					"type": "object",
	          				"properties": {},
			    	      	"required": []
						}
					}
				}
			],
			"tool_choice": "auto"
		}
	)"_json;

    auto response = openai::chat().create(request);
    std::cout << "Response is:\n" << response.dump(2) << '\n';
    
    auto choices = response["choices"];
    for(auto it = choices.begin(); it != choices.end(); it++)
    {
    	auto message = (*it)["message"];
		request["messages"].push_back(message);

	    auto tool_calls = message["tool_calls"];
	    for(auto it2 = tool_calls.begin(); it2 != tool_calls.end(); it2++)
	    {
			auto func_output = R"(
		    	{
		    		"role": "tool", 
		    		"tool_call_id": "",
		    		"content": "2025-09-17 10:15:23 UTC"
				}
			)"_json;
			func_output["tool_call_id"] = (*it2)["id"];
			request["messages"].push_back(func_output);
			break;
		}
		break;
	}
	
    auto response2 = openai::chat().create(request);
    std::cout << "Response is:\n" << response2.dump(2) << '\n';
}

int main()
{
	/*
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
	*/
	
	test_assistant();
	
    return 0;
}
