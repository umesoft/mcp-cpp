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

#include "mcp_client_impl.h"

namespace Mcp {

std::unique_ptr<McpClient> McpClient::CreateInstance(const std::string& name, const std::string& version)
{
	return std::make_unique<McpClientImpl>(name, version);
}

McpClientImpl::McpClientImpl(const std::string& name, const std::string& version)
    : m_name(name)
    , m_version(version)
    , m_request_id(1)
{
}

McpClientImpl::~McpClientImpl()
{
}

bool McpClientImpl::Initialize(std::shared_ptr<McpClientTransport> transport)
{
    m_transport = transport;

    m_request_id = 1;

    auto initialize = R"(
        {
            "jsonrpc": "2.0",
            "id": 0,
            "method": "initialize",
                "params": {
                "protocolVersion": "2025-06-18",
                "capabilities": {},
                "clientInfo": {
                    "name": "",
                    "title": "",
                    "version": ""
                }
            }
        }    
    )"_json;

    initialize["id"] = m_request_id;
    initialize["params"]["clientInfo"]["name"] = m_name;
    initialize["params"]["clientInfo"]["version"] = m_version;

    nlohmann::json response_json;
    if (!m_transport->Initialize(
        initialize.dump(), 
        [this, &response_json](const std::string& response)->bool
        {
			return this->IsCorrectResponse(response, response_json);
        }
    ))
    {
        return false;
    }

    auto initialize_notification = R"(
        {
            "jsonrpc": "2.0",
            "method": "notifications/initialized"
        }    
    )"_json;

    if (!m_transport->SendNotification(initialize_notification.dump()))
    {
        return false;
    }

	return true;
}

void McpClientImpl::Shutdown()
{
    m_transport->Shutdown();
    m_transport.reset();
}

bool McpClientImpl::ToolsList(std::vector<McpTool>& tools)
{
    m_request_id++;

    auto tool_list = R"(
        {
            "jsonrpc": "2.0",
            "id": 0,
            "method": "tools/list",
            "params": {}
        }
    )"_json;

    tool_list["id"] = m_request_id;

    nlohmann::json response_json;
    if (!m_transport->SendRequest(
        tool_list.dump(), 
        [this, &response_json](const std::string& response)->bool
        {
            return this->IsCorrectResponse(response, response_json);
        }
    ))
    {
        return false;
    }

	auto result_it = response_json.find("result");
	if (result_it != response_json.end())
	{
		auto tools_it = result_it->find("tools");
		if(tools_it != result_it->end())
		{
			for (auto it = tools_it->begin(); it != tools_it->end(); it++)
			{
				McpTool tool;

                tool.name = it->value("name", "");
				if (tool.name.empty())
				{
					continue;
				}
                tool.description = it->value("description", "");

                ParseSchema(*it, "inputSchema", tool.input_schema);
                ParseSchema(*it, "outputSchema", tool.output_schema);

				tools.emplace_back(tool);
			}
		}
	}

    return true;
}

void McpClientImpl::ParseSchema(const nlohmann::json& schema_json, std::string schema_tag, std::vector<McpProperty>& properties)
{
    auto input_schema_it = schema_json.find(schema_tag);
    if (input_schema_it != schema_json.end())
    {
        std::set<std::string> required_properties;
        auto required_it = input_schema_it->find("required");
        for (auto it2 = required_it->begin(); it2 != required_it->end(); it2++)
        {
            required_properties.insert(*it2);
        }

        auto properties_it = input_schema_it->find("properties");
        if (properties_it != input_schema_it->end())
        {
            for (auto it2 = properties_it->begin(); it2 != properties_it->end(); it2++)
            {
                McpProperty property;

                property.name = it2.key();
                if (property.name.empty())
                {
                    continue;
                }

                std::string type_str = it2->value("type", "");
                property.type = StringToMcpPropertyType(type_str);
                if (property.type == MCP_PROPERTY_TYPE_UNKNOWN)
                {
                    continue;
                }

                property.description = it2->value("description", "");

                if (required_properties.find(property.name) != required_properties.end())
                {
                    property.required = true;
                }
                else
                {
                    property.required = false;
                }

                properties.emplace_back(property);
            }
        }
    }
}

bool McpClientImpl::ToolsCall(std::string name, const std::map<std::string, std::string>& args, nlohmann::json& content)
{
    m_request_id++;

    auto tool_call = R"(
        {
            "jsonrpc": "2.0",
            "id": 0,
            "method": "tools/call",
            "params": {
                "name": "",
                "arguments": {}
            }
        }
    )"_json;

    tool_call["id"] = m_request_id;
    tool_call["params"]["name"] = name;
	for (auto it = args.begin(); it != args.end(); it++)
	{
		tool_call["params"]["arguments"][it->first] = it->second;
	}

    nlohmann::json response_json;
	if (!m_transport->SendRequest(
        tool_call.dump(), 
        [this, &response_json](const std::string& response)->bool
        {
            return this->IsCorrectResponse(response, response_json);
        }
    ))
	{
		return false;
	}

    auto result_it = response_json.find("result");
    if (result_it != response_json.end())
    {
        auto content_it = result_it->find("content");
        if (content_it != result_it->end())
        {
            content = *content_it;
            return true;
        }
    }

    return false;
}

bool McpClientImpl::IsCorrectResponse(const std::string& response_str, nlohmann::json& response_json)
{
    try
    {
        response_json = nlohmann::json::parse(response_str);
        auto id_it = response_json.find("id");
        if (id_it != response_json.end())
        {
            if (*id_it == m_request_id)
            {
                return true;
            }
        }
        return false;
    }
    catch (const nlohmann::json::parse_error& e)
    {
        return false;
    }

    return true;
}

}
