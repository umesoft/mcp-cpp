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

#include "mcp_client_transport.h"

namespace Mcp {

class McpHttpClientTransport : public McpClientTransport {
public:
	static std::unique_ptr<McpHttpClientTransport> CreateInstance(
		const std::string& host, 
		const std::string& entry_point,
		std::function <void(const std::string& url, std::string& token)> auth_callback = nullptr
	);
	
	virtual ~McpHttpClientTransport() {}

protected:
	McpHttpClientTransport() {}
};

}
