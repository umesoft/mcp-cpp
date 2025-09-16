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

#include "mcp-cpp/mcp_http_client_transport.h"

#include "curl/curl.h"

#include <memory>
#include <string>

namespace Mcp {

class McpHttpClientTransportImpl : public McpHttpClientTransport {
public:
	McpHttpClientTransportImpl(const std::string& host, const std::string& entry_point);
	virtual ~McpHttpClientTransportImpl();

	virtual bool Initialize(const std::string& request);
	virtual void Shutdown();
	virtual bool SendRequest(const std::string& request, std::string& response);

private:
	std::string m_host;
	std::string m_entry_point;
	std::string m_url;

	CURL* m_curl;

	std::map<std::string, std::string> m_headers;
	std::string m_header_buffer;
	std::string m_response;
	std::string m_response_buffer;

	std::string m_session_id;

	static size_t HeaderCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
	static size_t WriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
};

}
