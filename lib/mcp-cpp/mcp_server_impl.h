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

#include "mcp-cpp/mcp_server.h"

namespace Mcp {

class McpServerImpl : public McpServer
{
public:
	McpServerImpl(const std::string& server_name, const std::string& version);

	virtual void AddTool(
		const McpTool& tool,
		std::function <void(const std::string& session_id, const std::map<std::string, std::string>& args)> callback
		);

	virtual bool Run(std::unique_ptr<McpServerTransport> transport);
	virtual void Stop();
	virtual bool IsRunning();

	virtual void SendResponse(const std::string& session_id, const nlohmann::json& response);
	virtual void SendError(const std::string& session_id, int code, const std::string& message);
	virtual void SendToolNotification(const std::string& session_id, const std::string& method, const nlohmann::json& params);
	virtual void SendToolResponse(const std::string& session_id, const std::string& method, std::vector<McpContent> contents);

protected:
	virtual void OnClose(const std::string& session_id);
	virtual bool OnRecv(const std::string& session_id, const std::string& request_str);

private:
	std::string m_server_name;
	std::string m_version;

	struct McpToolInfo {
		std::string name;
		std::string description;
		std::map<std::string, McpProperty> input_schema;
		std::map<std::string, McpProperty> output_schema;
		std::function <void(const std::string& session_id, const std::map<std::string, std::string>& args)> callback;
	};
	std::map<std::string, McpToolInfo> m_tools;

	std::unique_ptr<McpServerTransport> m_transport;

	std::map<std::string, int> m_request_id;

	std::unique_ptr<std::thread> m_worker;
	bool m_is_running;

	void OnInitialize(const std::string& session_id, const nlohmann::json& request);
	void OnLoggingSetLevel(const std::string& session_id, const nlohmann::json& request);
	void OnPing(const std::string& session_id, const nlohmann::json& request);
	void OnToolsList(const std::string& session_id, const nlohmann::json& request);
	void OnToolCall(const std::string& session_id, const nlohmann::json& request);

	static std::string GetPropertyValue(const std::map<std::string, McpProperty>& output_schema, McpPropertyValue type, bool escape);
};

}
