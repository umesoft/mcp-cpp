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
#include <memory>
#include <string>
#include <vector>

#include "mcp_type.h"
#include "mcp_server_transport.h"

namespace Mcp {

class McpServer : public McpServerTransport::Handler
{
public:
	static std::unique_ptr<McpServer> CreateInstance(const std::string& server_name, const std::string& version);

	virtual ~McpServer() {}

	virtual void AddTool(
		const McpTool& tool,
		std::function <void(const std::string& session_id, const std::map<std::string, std::string>& args)> callback
		) = 0;

	virtual bool Run(std::shared_ptr<McpServerTransport> transport) = 0;

	virtual void SendResponse(const std::string& session_id, const nlohmann::json& response) = 0;
	virtual void SendError(const std::string& session_id, int code, const std::string& message) = 0;
	virtual void SendToolNotification(const std::string& session_id, const std::string& method, const nlohmann::json& params) = 0;
	virtual void SendToolResponse(const std::string& session_id, const std::string& method, std::vector<McpContent> contents) = 0;

protected:
	McpServer() {}
};

}
