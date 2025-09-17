#include "mcp_client_impl.h"

namespace Mcp {

std::unique_ptr<McpClient> McpClient::CreateInstance(const std::string& name, const std::string& version)
{
	return std::make_unique<McpClientImpl>(name, version);
}

McpClientImpl::McpClientImpl(const std::string& name, const std::string& version)
    : m_name(name)
    , m_version(version)
{
}

McpClientImpl::~McpClientImpl()
{
}

bool McpClientImpl::Initialize(std::shared_ptr<McpClientTransport> transport)
{
    m_transport = transport;

    auto initialize = R"(
        {
            "jsonrpc": "2.0",
            "id": 1,
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

    initialize["params"]["clientInfo"]["name"] = m_name;
    initialize["params"]["clientInfo"]["version"] = m_version;

    m_transport->Initialize(initialize.dump());

	return true;
}

void McpClientImpl::Shutdown()
{
    m_transport->Shutdown();
    m_transport.reset();
}

bool McpClientImpl::ToolsList(std::vector<McpTool>& tools)
{
    auto tool_call = R"(
        {
            "jsonrpc": "2.0",
            "id": 2,
            "method": "tools/list",
            "params": {}
        }
    )"_json;

    std::string response;
    if (!m_transport->SendRequest(tool_call.dump(), response))
    {
        return false;
    }

    try
    {
        auto response_json = nlohmann::json::parse(response);

		if (response_json.contains("result"))
		{
			if(response_json["result"].contains("tools"))
			{
                auto tools_json = response_json["result"]["tools"];
				for (auto it = tools_json.begin(); it != tools_json.end(); it++)
				{
					McpTool tool;
                    tool.name = (*it)["name"].get<std::string>();
                    tool.description = (*it)["description"].get<std::string>();
					if (it->find("inputSchema") != it->end())
					{
						auto input_schema_json = (*it)["inputSchema"];
						/* Under development...
						for (auto it2 = input_schema_json.begin(); it2 != input_schema_json.end(); it2++)
						{
							McpProperty property;
							// property.name = (*it2)["propertyName"].get<std::string>();
							std::string type_str = (*it2)["type"].get<std::string>();
							property.type = StringToMcpPropertyType(type_str);
							property.description = (*it2)["description"].get<std::string>();
							tool.input_schema.emplace_back(property);
						}
                        */
					}
					if (it->find("outputSchema") != it->end())
					{
						auto output_schema_json = (*it)["outputSchema"];
                        /* Under development...
                        for (auto it2 = output_schema_json.begin(); it2 != output_schema_json.end(); it2++)
						{
							McpProperty property;
							// property.name = (*it2)["propertyName"].get<std::string>();
							std::string type_str = (*it2)["type"].get<std::string>();
							property.type = StringToMcpPropertyType(type_str);
							property.description = (*it2)["description"].get<std::string>();
							tool.output_schema.emplace_back(property);
						}
                        */
					}
					tools.emplace_back(tool);
				}
			}
		}
    }
    catch (const nlohmann::json::parse_error& e)
    {
        return false;
    }

    return true;
}

bool McpClientImpl::ToolsCall(std::string name, const std::map<std::string, std::string>& args, nlohmann::json& content)
{
    auto tool_call = R"(
        {
            "jsonrpc": "2.0",
            "id": 2,
            "method": "tools/call",
            "params": {
                "name": "",
                "arguments": {}
            }
        }
    )"_json;

	tool_call["params"]["name"] = name;
	for (auto it = args.begin(); it != args.end(); it++)
	{
		tool_call["params"]["arguments"][it->first] = it->second;
	}

    std::string response;
	if (!m_transport->SendRequest(tool_call.dump(), response))
	{
		return false;
	}

    try
    {
        auto response_json = nlohmann::json::parse(response);

		content = response_json["result"]["content"];
    }
    catch (const nlohmann::json::parse_error& e)
    {
        return false;
    }
    
    return true;
}

}
