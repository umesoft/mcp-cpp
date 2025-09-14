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

#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "mcp_server_transport.h"

namespace Mcp {

class McpServer : public McpServerTransport::Handler
{
public:
	McpServer(const char* server_name, const char* version);

	enum PropertyType {
		PROPERTY_NUMBER = 1,
		PROPERTY_TEXT,
		PROPERTY_STRING,
		PROPERTY_OBJECT
	};
	struct McpProperty {
		std::string property_name;
		PropertyType property_type;
		std::string description;
		bool required;
	};
	struct McpPropertyValue {
		std::string property_name;
		std::string value;
	};
	struct McpContent {
		PropertyType property_type;
		std::string value;
		std::vector<McpPropertyValue> properties;
	};

	void AddTool(
		const char* tool_name, 
		const char* tool_description, 
		const std::vector<McpProperty>& input_schema,
		const std::vector<McpProperty>& output_schema,
		std::function <void(const std::string& session_id, const std::map<std::string, std::string>& args)> callback
		);

	bool Run(McpServerTransport* transport);

	void SendResponse(const std::string& session_id, const nlohmann::json& response);
	void SendError(const std::string& session_id, int code, const std::string& message);
	void SendToolNotification(const std::string& session_id, const std::string& method, const nlohmann::json& params, bool is_finish = true);
	void SendToolResponse(const std::string& session_id, const std::string& method, std::vector<McpContent> contents, bool is_finish = true);

protected:
	virtual void OnClose(const std::string& session_id);
	virtual bool OnRecv(const std::string& session_id, const std::string& request_str);

private:
	std::string m_server_name;
	std::string m_version;

	struct McpTool {
		std::string name;
		std::string description;
		std::map<std::string, McpProperty> input_schema;
		std::map<std::string, McpProperty> output_schema;
		std::function <void(const std::string& session_id, const std::map<std::string, std::string>& args)> callback;
	};
	std::map<std::string, McpTool> m_tools;

	McpServerTransport* m_transport;

	std::map<std::string, int> m_request_id;

	void OnInitialize(const std::string& session_id, const nlohmann::json& request);
	void OnLoggingSetLevel(const std::string& session_id, const nlohmann::json& request);
	void OnPing(const std::string& session_id, const nlohmann::json& request);
	void OnToolsList(const std::string& session_id, const nlohmann::json& request);
	void OnToolCall(const std::string& session_id, const nlohmann::json& request);

	static std::string GetPropertyType(PropertyType type);
	static std::string GetPropertyValue(const McpTool& tool, McpPropertyValue type, bool escape);
};

}
