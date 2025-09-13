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

#define NOMINMAX

#include "platform/platform.h"
#include "mcp-cpp/mcp_server.h"
#include "jwt-cpp/jwt.h"

namespace Mcp {

McpServer::McpServer(const char* server_name, const char* version)
	: m_server_name(server_name)
	, m_version(version)
	, m_transport(nullptr)
{
}

void McpServer::AddTool(
	const char* tool_name,
	const char* tool_description,
	const std::vector<McpProperty>& input_schema,
	const std::vector<McpProperty>& output_schema,
	std::function <std::vector<McpContent>(const std::map<std::string, std::string>& args)> callback
)
{
	McpTool tool;
	tool.name = tool_name;
	tool.description = tool_description;
	for (auto it = input_schema.begin(); it != input_schema.end(); it++)
	{
		tool.input_schema[it->property_name] = *it;
	}
	for (auto it = output_schema.begin(); it != output_schema.end(); it++)
	{
		tool.output_schema[it->property_name] = *it;
	}
	tool.callback = callback;
	m_tools[tool_name] = tool;
}

bool McpServer::Run(McpServerTransport* transport)
{
	m_transport = transport;

	m_transport->Open(this);

	while (true)
	{
		if (!m_transport->ProcRequest())
		{
			break;
		}
	}

	m_transport->Close();
	m_transport = nullptr;

	return true;
}

bool McpServer::OnRecv(const std::string& request_str, std::string& response_str)
{
	nlohmann::json response;

	try
	{
		auto request = nlohmann::json::parse(request_str);

		if (request.contains("method"))
		{
			std::string method = request.at("method");
			if (method == "notifications/initialized")
			{
				return false;
			}

			if (method == "initialize")
			{
				OnInitialize(request, response);
			}
			else if (method == "ping")
			{
				response = R"(
					{
						"jsonrpc": "2.0",
						"id": null,
						"result": {}
					}
				)"_json;
			}
			else if (method == "logging/setLevel")
			{
				OnLoggingSetLevel(request, response);
			}
			else if (method == "tools/list")
			{
				OnToolsList(request, response);
			}
			else if (method == "tools/call")
			{
				nlohmann::json result_or_error;
				if (OnToolCall(request, result_or_error))
				{
					response = R"(
						{
							"jsonrpc": "2.0",
							"id": null,
							"result": {}
						}
					)"_json;
					response["result"] = result_or_error;
				}
				else
				{
					response = R"(
						{
							"jsonrpc": "2.0",
							"id": null,
							"error": {}
						}
					)"_json;
					response["error"] = result_or_error;
				}
			}
			else
			{
				response = R"(
					{
						"jsonrpc": "2.0",
						"id": null,
						"error": {
							"code": -32601,
							"message": "Method not found"
						}
					}
				)"_json;
			}
		}
		else
		{
			response = R"(
				{
					"jsonrpc": "2.0",
					"id": null,
					"error": {
						"code": -32600,
						"message": "Invalid request"
					}
				}
			)"_json;
		}

		if (request.contains("id"))
		{
			response["id"] = request.at("id").get<int>();
		}
	}
	catch (const nlohmann::json::parse_error& e)
	{
		response = R"(
			{
				"jsonrpc": "2.0",
				"id": null,
				"error": {
					"code": -32700,
					"message": "Parse error"
				}
			}
		)"_json;
	}

	response_str = response.dump();

	return true;
}

void McpServer::OnInitialize(const nlohmann::json& request, nlohmann::json& response)
{
	response = R"(
		{
			"jsonrpc": "2.0",
			"id": 0,
			"result": {
				"protocolVersion": "2025-06-18",
				"capabilities": {
					"logging": {},
					"tools": {}
				},
				"serverInfo": {
					"name": "",
					"version": ""
				}
			}
		}
	)"_json;

	response["result"]["serverInfo"]["name"] = m_server_name;
	response["result"]["serverInfo"]["version"] = m_version;
}

void McpServer::OnLoggingSetLevel(const nlohmann::json& request, nlohmann::json& response)
{
	response = R"(
		{
			"jsonrpc": "2.0",
			"id": 0,
			"result": {}
		}
	)"_json;
}

void McpServer::OnToolsList(const nlohmann::json& request, nlohmann::json& response)
{
	response = R"(
		{
			"jsonrpc": "2.0",
			"id": 0,
			"result": {}
		}
	)"_json;

	std::string tools_json = "";
	for (auto it = m_tools.begin(); it != m_tools.end(); it++)
	{
		McpTool& tool = it->second;

		if (!tools_json.empty()) {
			tools_json += ",";
		}
		else {
			tools_json = "{ \"tools\": [";
		}
		tools_json += "{"
			"\"name\": \"" + tool.name + "\","
			"\"description\": \"" + tool.description + "\"";

		if (tool.input_schema.size() > 0)
		{
			tools_json += ",\"inputSchema\": {"
				"\"type\": \"object\","
				"\"properties\": {";

			std::string required_properties = "";

			int i = 0;
			for (auto it = tool.input_schema.begin(); it != tool.input_schema.end(); it++)
			{
				if (i > 0)
				{
					tools_json += ",";
				}
				const auto& prop = it->second;
				tools_json += "\"" + prop.property_name + "\": {"
					"\"type\": \"" + GetPropertyType(prop.property_type) +
					"\",\"description\": \"" + prop.description + "\"}";

				if (prop.required)
				{
					if (!required_properties.empty())
					{
						required_properties += ",";
					}
					required_properties += "\"" + prop.property_name + "\"";
				}
				i++;
			}

			tools_json += "}";

			if (!required_properties.empty()) {
				tools_json += ", \"required\": [" + required_properties + "]";
			}

			tools_json += "}";
		}

		if (tool.output_schema.size() > 0)
		{
			tools_json += ",\"outputSchema\": {"
				"\"type\": \"object\","
				"\"properties\": {"
				"\"content\": {"
				"\"type\": \"array\","
				"\"items\": {"
				"\"type\": \"object\","
				"\"properties\": {";

			std::string required_properties = "";

			int i = 0;
			for (auto it = tool.output_schema.begin(); it != tool.output_schema.end(); it++)
			{
				if (i > 0)
				{
					tools_json += ",";
				}
				const auto& prop = it->second;
				tools_json += "\"" + prop.property_name + "\": {"
					"\"type\": \"" + GetPropertyType(prop.property_type) +
					"\",\"description\": \"" + prop.description + "\"}";

				if (prop.required)
				{
					if (!required_properties.empty())
					{
						required_properties += ",";
					}
					required_properties += "\"" + prop.property_name + "\"";
				}
				i++;
			}

			tools_json += "}";

			if (!required_properties.empty())
			{
				tools_json += ", \"required\": [" + required_properties + "]";
			}

			tools_json += "}}},\"required\": [\"content\"]}";
		}

		tools_json += "}";
	}
	tools_json += "]}";

	response["result"] = nlohmann::json::parse(tools_json);
}

bool McpServer::OnToolCall(const nlohmann::json& request, nlohmann::json& response)
{
	std::string name = request["params"]["name"];

	auto it = m_tools.find(name);
	if (it == m_tools.end())
	{
		response = R"(
			{
				"code": -32602,
				"message": "Unknown tool: invalid_tool_name"
			}
		)"_json;
		return false;
	}

	auto raequestArguments = request["params"]["arguments"];

	std::map<std::string, std::string> arguments;

	McpTool& tool = it->second;
	for (auto it2 = tool.input_schema.begin(); it2 != tool.input_schema.end(); it2++)
	{
		const auto& prop = it2->second;

		if (raequestArguments.contains(prop.property_name))
		{
			arguments[prop.property_name] = raequestArguments.at(prop.property_name);
		}
		else
		{
			arguments[prop.property_name] = "";
		}

		if (prop.required && arguments[prop.property_name].empty())
		{
			response = R"(
				{
					"code": -32602,
					"message": "Invalid_param: missing_required_params"
				}
			)"_json;
			return false;
		}
	}

	std::vector<McpContent> contents = tool.callback(arguments);
	std::string content_json = "";
	std::string structured_content_json = "";

	if (tool.output_schema.size() == 0)
	{
		for (size_t i = 0; i < contents.size(); i++)
		{
			if (i > 0) {
				content_json += ",";
			}
			else {
				content_json = "{\"content\": [";
			}
			content_json += "{"
				"\"type\": \"" + GetPropertyType(contents[i].property_type) + "\","
				"\"text\": \"" + contents[i].value + "\""
				"}";
		}
		content_json += "]}";
	}
	else
	{
		for (size_t i = 0; i < contents.size(); i++)
		{
			if (i > 0)
			{
				content_json += ",";
				structured_content_json += ",";
			}
			else {
				content_json = "{\"content\": [";
				content_json = "\"structuredContent\": {\"content\": [";
			}
			content_json += "{\"type\": \"text\",\"text\": \"{";
			structured_content_json += "{";
			for (size_t j = 0; j < contents[i].properties.size(); j++)
			{
				if (j > 0)
				{
					content_json += ",";
					structured_content_json += ",";
				}
				content_json += "\\\"" + contents[i].properties[j].property_name + "\\\": " + GetPropertyValue(tool, contents[i].properties[j], true);
				structured_content_json += "\"" + contents[i].properties[j].property_name + "\": " + GetPropertyValue(tool, contents[i].properties[j], false);
			}
			content_json += "}\"";
			content_json += "}";
			structured_content_json += "}";
		}
		content_json = "],";
		content_json += structured_content_json;
		content_json += "]}}";
	}

	response = nlohmann::json::parse(content_json);

	return true;
}

std::string McpServer::GetPropertyType(PropertyType type)
{
	switch (type) {
	case PROPERTY_NUMBER:
		return "number";
	case PROPERTY_TEXT:
		return "text";
	case PROPERTY_STRING:
		return "string";
	case PROPERTY_OBJECT:
		return "object";
	default:
		return "unknown";
	}
}

std::string McpServer::GetPropertyValue(const McpTool& tool, McpPropertyValue value, bool escape)
{
	auto it = tool.output_schema.find(value.property_name);
	if (it == tool.output_schema.end())
	{
		return "";
	}

	switch (it->second.property_type) {
	case PROPERTY_NUMBER:
		return value.value;
	case PROPERTY_STRING:
		if (escape)
		{
			return "\\\"" + value.value + "\\\"";
		}
		else
		{
			return "\"" + value.value + "\"";
		}
	default:
		return "";
	}
}

void McpServer::SendNotification(const std::string& method, const nlohmann::json& params)
{
	auto notification = R"(
		{
			"jsonrpc": "2.0",
			"method": "",
			"params": {
				"value": 0
			}
		}
	)"_json;

	notification["method"] = "notification/" + method;
	notification["params"] = params;

	m_transport->SendNotification(notification.dump());
}

}
