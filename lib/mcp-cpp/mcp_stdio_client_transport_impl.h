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

#include "mcp-cpp/mcp_stdio_client_transport.h"

namespace Mcp {

class McpStdioClientTransportImpl : public McpStdioClientTransport {
public:
	McpStdioClientTransportImpl(const std::wstring& filepath, int timeout);
	virtual ~McpStdioClientTransportImpl();

	virtual bool Initialize(
		const std::string& request,
		std::function <bool(const std::string& response)> callback
	);
	virtual void Shutdown();
	virtual bool SendRequest(
		const std::string& request,
		std::function <bool(const std::string& response)> callback
	);
	virtual bool SendNotification(const std::string& notification);

protected:
	std::wstring m_filepath;
	std::string m_response_str;
	std::vector<char> m_request_buffer;
	int m_timeout;

	virtual bool OnCreateProcess() = 0;
	virtual void OnTerminateProcess() = 0;
	virtual bool OnSendRequest(
		const std::string& request,
		std::function <bool(const std::string& response)> callback
	) = 0;
	virtual bool OnSendNotification(const std::string& notification) = 0;

	bool AppendResponse(char* response_buffer, int size, std::string& response_str);
};

}
