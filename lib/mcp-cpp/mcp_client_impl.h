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

#include "mcp-cpp/mcp_client.h"

namespace Mcp {

class McpClientImpl : public McpClient {
public:
    McpClientImpl(const std::string& name, const std::string& version);
	virtual ~McpClientImpl();

	virtual bool Initialize(std::shared_ptr<McpClientTransport> transport);
	virtual void Shutdown();

	virtual bool ToolsList(std::vector<McpTool>& tools);
    virtual bool ToolsCall(std::string name, const std::map<std::string, std::string>& args, nlohmann::json& content);

private:
	std::string m_name;
	std::string m_version;
	int m_request_id;

	std::shared_ptr<McpClientTransport> m_transport;

	bool IsCorrectResponse(const std::string& response_str, nlohmann::json& response_json);
};

}
