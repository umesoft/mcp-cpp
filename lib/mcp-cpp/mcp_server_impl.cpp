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

#include "mcp_server_impl.h"
#include "jwt-cpp/jwt.h"

namespace Mcp {

std::unique_ptr<McpServer> McpServer::CreateInstance(const std::string& server_name, const std::string& version)
{
	return  std::make_unique<McpServerImpl>(server_name, version);
}

McpServerImpl::McpServerImpl(const std::string& server_name, const std::string& version)
	: m_server_name(server_name)
	, m_version(version)
	, m_transport(nullptr)
{
}

void McpServerImpl::AddTool(
	const McpTool& tool,
	std::function <void(const std::string& session_id, const std::map<std::string, std::string>& args)> callback
)
{
	McpToolInfo tool_info;
	tool_info.name = tool.name;
	tool_info.description = tool.description;
	for (auto it = tool.input_schema.begin(); it != tool.input_schema.end(); it++)
	{
		tool_info.input_schema[it->name] = *it;
	}
	for (auto it = tool.output_schema.begin(); it != tool.output_schema.end(); it++)
	{
		tool_info.output_schema[it->name] = *it;
	}
	tool_info.callback = callback;
	m_tools[tool.name] = tool_info;
}

bool McpServerImpl::Run(std::unique_ptr<McpServerTransport> transport)
{
	m_transport = std::move(transport);

	m_transport->Open(this);

	while (true)
	{
		if (!m_transport->ProcRequest())
		{
			break;
		}
	}

	m_transport->Close();
	m_transport.reset();

	return true;
}

void McpServerImpl::OnClose(const std::string& session_id)
{
	m_request_id.erase(session_id);
}

bool McpServerImpl::OnRecv(const std::string& session_id, const std::string& request_str)
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

void McpServerImpl::OnInitialize(const std::string& session_id, const nlohmann::json& request)
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

void McpServerImpl::OnLoggingSetLevel(const std::string& session_id, const nlohmann::json& request)
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

void McpServerImpl::OnPing(const std::string& session_id, const nlohmann::json& request)
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

void McpServerImpl::OnToolsList(const std::string& session_id, const nlohmann::json& request)
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
		McpToolInfo& tool_info = it->second;

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

		tool_record["name"] = tool_info.name;
		tool_record["description"] = tool_info.description;

		if (tool_info.input_schema.size() > 0)
		{
			for (auto it = tool_info.input_schema.begin(); it != tool_info.input_schema.end(); it++)
			{
				auto property_record = R"(
					{ 
						"type": "",
						"description": ""
					}
				)"_json;

				const auto& prop = it->second;
				property_record["type"] = McpPropertyTypeToString(prop.type);
				property_record["description"] = prop.description;
				tool_record["inputSchema"]["properties"][prop.name] = property_record;

				if (prop.required)
				{
					tool_record["inputSchema"]["required"].emplace_back(prop.name);
				}
			}
		}

		if (tool_info.output_schema.size() > 0)
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

			for (auto it = tool_info.output_schema.begin(); it != tool_info.output_schema.end(); it++)
			{
				auto property_record = R"(
					{ 
						"type": "",
						"description": ""
					}
				)"_json;

				const auto& prop = it->second;
				property_record["type"] = McpPropertyTypeToString(prop.type);
				property_record["description"] = prop.description;
				output_schema["properties"]["content"]["items"]["properties"][prop.name] = property_record;

				if (prop.required)
				{
					output_schema["properties"]["content"]["items"]["required"].emplace_back(prop.name);
				}
			}

			tool_record["outputSchema"] = output_schema;
		}

		response["result"]["tools"].emplace_back(tool_record);
	}

	SendResponse(session_id, response);
}

void McpServerImpl::OnToolCall(const std::string& session_id, const nlohmann::json& request)
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

	McpToolInfo& tool_info = it->second;
	for (auto it2 = tool_info.input_schema.begin(); it2 != tool_info.input_schema.end(); it2++)
	{
		const auto& prop = it2->second;

		if (raequestArguments.contains(prop.name))
		{
			arguments[prop.name] = raequestArguments.at(prop.name);
		}
		else
		{
			arguments[prop.name] = "";
		}

		if (prop.required && arguments[prop.name].empty())
		{
			SendError(session_id, -32602, "Unknown tool: missing_required_params");
			return;
		}
	}

	tool_info.callback(session_id, arguments);
}

void McpServerImpl::SendResponse(const std::string& session_id, const nlohmann::json& response)
{
	m_transport->SendResponse(session_id, response.dump());

	m_request_id.erase(session_id);
}

void McpServerImpl::SendError(const std::string& session_id, int code, const std::string& message)
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

void McpServerImpl::SendToolNotification(const std::string& session_id, const std::string& method, const nlohmann::json& params)
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

	m_transport->SendResponse(session_id, notification.dump(), false);
}

void McpServerImpl::SendToolResponse(const std::string& session_id, const std::string& method, std::vector<McpContent> contents)
{
	auto it2 = m_tools.find(method);
	if (it2 == m_tools.end())
	{
		return;
	}
	McpToolInfo& tool_info = it2->second;

	auto response = R"(
		{
			"jsonrpc": "2.0",
			"id": null,
			"result": {
				"content": []
			}
		}
	)"_json;

	auto it = m_request_id.find(session_id);
	if (it == m_request_id.end())
	{
		return;
	}

	response["id"] = it->second;

	if (tool_info.output_schema.size() == 0)
	{
		for (auto it = contents.begin(); it != contents.end(); it++)
		{
			auto content = R"(
				{
					"type": "text",
					"text": ""
				}
			)"_json;

			if (it->properties.size() > 0)
			{
				content["text"] = it->properties[0].value;
			}

			response["result"]["content"].emplace_back(content);
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
				content_json += "\\\"" + content.properties[j].name + "\\\": " + GetPropertyValue(tool_info.output_schema, content.properties[j], true);
				structured_content_json += "\"" + content.properties[j].name + "\": " + GetPropertyValue(tool_info.output_schema, content.properties[j], false);
			}

			content_json += "}\"}";
			response["result"]["content"].emplace_back(nlohmann::json::parse(content_json));

			structured_content_json += "}";
			structured_content["content"].emplace_back(nlohmann::json::parse(structured_content_json));
		}

		response["result"]["structuredContent"] = structured_content;
	}

	m_transport->SendResponse(session_id, response.dump(), true);

	m_request_id.erase(session_id);
}

std::string McpServerImpl::GetPropertyValue(const std::map<std::string, McpProperty>& output_schema, McpPropertyValue value, bool escape)
{
	auto it = output_schema.find(value.name);
	if (it == output_schema.end())
	{
		return "";
	}

	switch (it->second.type) {
	case MCP_PROPERTY_TYPE_NUMBER:
		return value.value;
	case MCP_PROPERTY_TYPE_STRING:
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
