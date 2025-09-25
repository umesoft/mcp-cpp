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
#include "mcp_client_authorization_impl.h"

#include <curl/curl.h>

namespace Mcp {

class McpHttpClientTransportImpl : public McpHttpClientTransport {
public:
	McpHttpClientTransportImpl(
		const std::string& host, 
		const std::string& entry_point,
		std::function <bool(const std::string& url)> auth_callback = nullptr
	);
	virtual ~McpHttpClientTransportImpl();

	virtual McpClientAuthorization* GetAuthorization() { return m_authorization.get(); }

	virtual bool Initialize(
		const std::string& client_name,
		const std::string& request,
		std::function <bool(const std::string& response)> callback
	);
	virtual void Shutdown();
	virtual bool SendRequest(
		const std::string& request,
		std::function <bool(const std::string& response)> callback
	);
	virtual bool SendNotification(const std::string& notification);

private:
	std::string m_host;
	std::string m_entry_point;
	std::string m_url;

	std::function <bool(const std::string& response)> m_callback;
	std::function <bool(const std::string& url)> m_auth_callback;

	std::unique_ptr<McpClientAuthorizationImpl> m_authorization;

	CURL* m_curl;

	std::map<std::string, std::string> m_headers;
	std::string m_header_buffer;
	std::string m_response;
	std::string m_response_buffer;

	std::string m_mcp_session_id;

	static size_t HeaderCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
	static size_t WriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata);

	bool Send(
		const std::string& request, 
		std::function <bool(const std::string& response)> callback,
		std::string& response, 
		int& status_code
	);

	std::optional<std::string> ExtractResourceMetadata(const std::string& header);
};

}
