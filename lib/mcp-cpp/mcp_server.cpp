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
	std::function <void(const std::string& session_id, const std::map<std::string, std::string>& args)> callback
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

void McpServer::OnClose(const std::string& session_id)
{
	m_request_id.erase(session_id);
}

bool McpServer::OnRecv(const std::string& session_id, const std::string& request_str)
{
	try
	{
		auto request = nlohmann::json::parse(request_str);

		if (request.contains("method"))
		{
			std::string method = request.at("method");
			if (method == "notifications/initialized" ||
				method == "notifications/cancelled")
			{
				return false;
			}

			if (request.contains("id"))
			{
				m_request_id[session_id] = request.at("id").get<int>();

				if (method == "initialize")
				{
					OnInitialize(session_id, request);
				}
				else if (method == "logging/setLevel")
				{
					OnLoggingSetLevel(session_id, request);
				}
				else if (method == "ping")
				{
					OnPing(session_id, request);
				}
				else if (method == "tools/list")
				{
					OnToolsList(session_id, request);
				}
				else if (method == "tools/call")
				{
					OnToolCall(session_id, request);
				}
				else
				{
					SendError(session_id, -32601, "Method not found");
				}
			}
			else
			{
				SendError(session_id, -32600, "Invalid request");
			}
		}
		else
		{
			SendError(session_id, -32600, "Invalid request");
		}
	}
	catch (const nlohmann::json::parse_error& e)
	{
		SendError(session_id, -32700, "Parse error");
	}

	return true;
}

void McpServer::OnInitialize(const std::string& session_id, const nlohmann::json& request)
{
	auto response = R"(
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

	response["id"] = request.at("id").get<int>();
	response["result"]["serverInfo"]["name"] = m_server_name;
	response["result"]["serverInfo"]["version"] = m_version;

	SendResponse(session_id, response);
}

void McpServer::OnLoggingSetLevel(const std::string& session_id, const nlohmann::json& request)
{
	auto response = R"(
		{
			"jsonrpc": "2.0",
			"id": 0,
			"result": {}
		}
	)"_json;

	response["id"] = request.at("id").get<int>();

	SendResponse(session_id, response);
}

void McpServer::OnPing(const std::string& session_id, const nlohmann::json& request)
{
	auto response = R"(
		{
			"jsonrpc": "2.0",
			"id": null,
			"result": {}
		}
	)"_json;

	response["id"] = request.at("id").get<int>();

	SendResponse(session_id, response);
}

void McpServer::OnToolsList(const std::string& session_id, const nlohmann::json& request)
{
	auto response = R"(
		{
			"jsonrpc": "2.0",
			"id": 0,
			"result": {
				"tools": []
			}
		}
	)"_json;

	response["id"] = request.at("id").get<int>();

	for (auto it = m_tools.begin(); it != m_tools.end(); it++)
	{
		McpTool& tool = it->second;

		auto tool_record = R"(
			{
				"name": "",
				"description": "",
				"inputSchema": {
					"type": "object",
					"properties" : {},
					"required": []
				}
			}
		)"_json;

		tool_record["name"] = tool.name;
		tool_record["description"] = tool.description;

		if (tool.input_schema.size() > 0)
		{
			for (auto it = tool.input_schema.begin(); it != tool.input_schema.end(); it++)
			{
				auto property_record = R"(
					{ 
						"type": "",
						"description": ""
					}
				)"_json;

				const auto& prop = it->second;
				property_record["type"] = GetPropertyType(prop.property_type);
				property_record["description"] = prop.description;
				tool_record["inputSchema"]["properties"][prop.property_name] = property_record;

				if (prop.required)
				{
					tool_record["inputSchema"]["required"].emplace_back(prop.property_name);
				}
			}
		}

		if (tool.output_schema.size() > 0)
		{
			auto output_schema = R"(
				{ 
					"type": "object",
					"properties": {
						"content": {
							"type": "array",
							"items": {
								"type": "object",
								"properties": {
								},
								"required": []
							}
						}
					},
					"required": [ "content" ]
				}
			)"_json;

			for (auto it = tool.output_schema.begin(); it != tool.output_schema.end(); it++)
			{
				auto property_record = R"(
					{ 
						"type": "",
						"description": ""
					}
				)"_json;

				const auto& prop = it->second;
				property_record["type"] = GetPropertyType(prop.property_type);
				property_record["description"] = prop.description;
				output_schema["properties"]["content"]["items"]["properties"][prop.property_name] = property_record;

				if (prop.required)
				{
					output_schema["properties"]["content"]["items"]["required"].emplace_back(prop.property_name);
				}
			}

			tool_record["outputSchema"] = output_schema;
		}

		response["result"]["tools"].emplace_back(tool_record);
	}

	SendResponse(session_id, response);
}

void McpServer::OnToolCall(const std::string& session_id, const nlohmann::json& request)
{
	std::string name = request["params"]["name"];

	auto it = m_tools.find(name);
	if (it == m_tools.end())
	{
		SendError(session_id, -32602, "Unknown tool: invalid_tool_name");
		return;
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
			SendError(session_id, -32602, "Unknown tool: missing_required_params");
			return;
		}
	}

	tool.callback(session_id, arguments);
}

void McpServer::SendResponse(const std::string& session_id, const nlohmann::json& response)
{
	m_transport->SendResponse(session_id, response.dump());

	m_request_id.erase(session_id);
}

void McpServer::SendError(const std::string& session_id, int code, const std::string& message)
{
	auto response = R"(
		{
			"jsonrpc": "2.0",
			"id": null,
			"error": {
				"code": -32600,
				"message": "Invalid request"
			}
		}
	)"_json;

	auto it = m_request_id.find(session_id);
	if (it != m_request_id.end())
	{
		response["id"] = it->second;
	}

	response["error"]["code"] = code;
	response["error"]["message"] = message;

	m_transport->SendResponse(session_id, response.dump());

	m_request_id.erase(session_id);
}

void McpServer::SendToolNotification(const std::string& session_id, const std::string& method, const nlohmann::json& params, bool is_finish)
{
	auto notification = R"(
		{
			"jsonrpc": "2.0",
			"method": "",
			"params": {}
		}
	)"_json;

	notification["method"] = "notifications/" + method;
	notification["params"] = params;

	m_transport->SendResponse(session_id, notification.dump(), is_finish);
}

void McpServer::SendToolResponse(const std::string& session_id, const std::string& method, std::vector<McpContent> contents, bool is_finish)
{
	auto it2 = m_tools.find(method);
	if (it2 == m_tools.end())
	{
		return;
	}
	McpTool& tool = it2->second;

	auto result = R"(
		{
			"content": []
		}
	)"_json;

	if (tool.output_schema.size() == 0)
	{
		for (auto it = contents.begin(); it != contents.end(); it++)
		{
			auto content = R"(
				{
					"type": "text",
					"text": ""
				}
			)"_json;
			content["text"] = it->value;

			result["content"].emplace_back(content);
		}
	}
	else
	{
		auto structured_content = R"(
			{
				"content": []
			}
		)"_json;

		for (auto it = contents.begin(); it != contents.end(); it++)
		{
			McpContent& content = *it;

			std::string content_json = "{\"type\": \"text\",\"text\": \"{";
			std::string structured_content_json = "{";

			for (size_t j = 0; j < content.properties.size(); j++)
			{
				if (j > 0)
				{
					content_json += ",";
					structured_content_json += ",";
				}
				content_json += "\\\"" + content.properties[j].property_name + "\\\": " + GetPropertyValue(tool, content.properties[j], true);
				structured_content_json += "\"" + content.properties[j].property_name + "\": " + GetPropertyValue(tool, content.properties[j], false);
			}

			content_json += "}\"}";
			result["content"].emplace_back(nlohmann::json::parse(content_json));

			structured_content_json += "}";
			structured_content["content"].emplace_back(nlohmann::json::parse(structured_content_json));
		}

		result["structuredContent"] = structured_content;
	}

	auto response = R"(
		{
			"jsonrpc": "2.0",
			"id": null,
			"result": {}
		}
	)"_json;

	auto it = m_request_id.find(session_id);
	if (it != m_request_id.end())
	{
		response["id"] = it->second;
	}
	response["result"] = result;

	m_transport->SendResponse(session_id, response.dump(), is_finish);

	if (is_finish)
	{
		m_request_id.erase(session_id);
	}
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

}
