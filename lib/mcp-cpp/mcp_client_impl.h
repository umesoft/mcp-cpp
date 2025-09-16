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

#include "mcp-cpp/McpClient.h"

namespace Mcp {

class McpClientImpl : public McpClient {
public:
	virtual ~McpClientImpl();

	virtual bool Initialize(std::shared_ptr<McpClientTransport> m_transport);
	virtual void Shutdown();

	virtual bool ToolsList(std::vector<McpTool>& tools);
    virtual bool ToolsCall(std::string name, const std::map<std::string, std::string>& args);

protected:
    McpClientImpl();
};

}
